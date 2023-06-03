
#include "webuser.h"

using namespace std;

int main(int argc, char *argv[]){
    if (argc != 3) {
      cerr << "Invalid format! Syntax: player <machine_name> <port_num>\n" << endl;
      return EXIT_FAILURE;
    }

    player Player;
    Player.init_ringmaster(argv);

    Player.getinfo();

    Player.init_player();

    send(Player.ringmaster_fd,&Player.player_port,sizeof(Player.player_port),0);

    Player.init_rightplayer();

    Player.init_leftplayer();
    int max_fd = Player.findmaxfd();

    bool flag = true;

    while (flag){

        Player.getpotato(max_fd, flag);
    }

    Player.endgame();

    return EXIT_SUCCESS;

}