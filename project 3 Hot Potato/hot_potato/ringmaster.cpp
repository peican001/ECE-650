

#include "webuser.h"

#define BACKLOG 512
using namespace std;

int main(int argc, char* argv[]){

    if (argc != 4){
        cerr << "Invalid format! Syntax: ./ringmaster <port_num> <num_players> <num_hops> !" << endl;
        return EXIT_FAILURE;
    }

    ringmaster Ringmaster(argv); 

    Ringmaster.init_ringserver();

    Ringmaster.linktoPlayers();

    Ringmaster.bind_neighb_player();

    potato hot_potato(Ringmaster.num_hops);
    
    if (Ringmaster.num_hops == 0){
        //hot_potato.flag = 1;
        hot_potato.setflag(1);


        Ringmaster.send_allplayers_potato(hot_potato);
        Ringmaster.close_players();

        //close(socket_fd);
        Ringmaster.close_ringmaster();

        //freeaddrinfo(host_info_list);
        return EXIT_SUCCESS;
    }
    else{
        srand ((unsigned int)(time(NULL)) + Ringmaster.num_hops);
        int start_player = rand() % Ringmaster.num_players;
        cout << "Ready to start the game, sending potato to player " << start_player << endl;
        //send(player_sock_fd[start_player], &hot_potato, sizeof(hot_potato), 0);
        Ringmaster.send_oneplayer_potato(start_player, hot_potato);

        Ringmaster.deal_final_potato();

        Ringmaster.close_ringmaster();

        return EXIT_SUCCESS;
    }

}

