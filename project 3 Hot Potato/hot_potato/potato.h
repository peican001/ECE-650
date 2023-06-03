#include <iostream>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <assert.h>

#include <arpa/inet.h>

#define Maxhops 512

using namespace std;

class potato{
  public:
    int hops;
    int trace[512];
    int count;
    int flag;

    potato():hops(0),count(0), flag(0){
        memset(&trace,0,sizeof(trace));
    }

    potato(int num_hops):hops(num_hops),count(0), flag(0){
        memset(&trace,0,sizeof(trace));
    }

    void setflag(int Flag){
      flag = Flag;
    }

    bool check_hops(){
      if (hops == 0) return true;
      else return false;
    }

    void printpath(){
      cout << "Trace of potato:" << endl;
        for(int i = 0; i < hops - 1; i++){
            cout << trace[i] << ",";
        }
        cout << trace[hops - 1] << endl;
    }
};



void exitWithError(const char *str){
  cerr << str << endl;
  exit(EXIT_FAILURE);
}


void send_full(int end_fd, potato *potato_ptr){
  int check_send = send(end_fd, potato_ptr, sizeof(*potato_ptr), 0);
  if(check_send < sizeof(*potato_ptr)){
    printf("to send full\n");
    check_send += send(end_fd, (potato_ptr+check_send), (sizeof(*potato_ptr)-check_send), 0);
  }
}