#include "potato.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <time.h>
//#include "player.h"

#define BACKLOG 512
using namespace std;

class player{
    public:
    int player_fd;
    int player_port;
    int player_id;
    string ip;
    int num_players;
    char *addr;

    player* left_player;
    player* right_player;

    int ringmaster_fd;

    player(){}

    player(int fd, int Port, int Id, string Ip): player_fd(fd), player_port(Port), player_id(Id), ip(Ip) {

    }

    void init_ringmaster(char *argv[]){
        const char *hostname = argv[1];
        const char *port = argv[2];

        ringmaster_fd = build_connecter(hostname, port);

    }

    int build_connecter(const char *hostname, const char *port){
    
        int status;
        int socket_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0) {
            cerr << "Error: cannot get address info for host" << endl;
            cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        socket_fd = socket(host_info_list->ai_family, 
                    host_info_list->ai_socktype, 
                    host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if
        freeaddrinfo(host_info_list);

        return socket_fd;

    }

    void getinfo(){
        recv(ringmaster_fd, &player_id, sizeof(player_id),MSG_WAITALL);
        recv(ringmaster_fd, &num_players, sizeof(num_players),MSG_WAITALL);
        cout << "Connected as player " << player_id << " out of " << num_players << " total players" <<endl;
    }

    void init_player(){
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        
        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags = AI_PASSIVE;
        
        int status = getaddrinfo(NULL, "", &host_info, &host_info_list);
        if (status != 0){
            cerr << "Error: cannot get address info for host" << endl;
            exit(EXIT_FAILURE);
        }

        // specify 0 in sin_port to let OS assign a port
        struct sockaddr_in *addr = (struct sockaddr_in *)(host_info_list->ai_addr);
        addr->sin_port = 0;
        
        player_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                host_info_list->ai_protocol);
        if (player_fd == -1) {
            cerr << "Error: cannot create player's socket" << endl;
            exit(EXIT_FAILURE);
        }

        int yes = 1;
        status = setsockopt(player_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(player_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot bind player's socket" << endl;
            exit(EXIT_FAILURE);
        }

        status = listen(player_fd, 100);
        if (status == -1) {
            cerr << "Error: cannot listen on player's socket" << endl;
            exit(EXIT_FAILURE);
        }
        
        struct sockaddr_in sock;
        socklen_t len = sizeof(sock);
        if (getsockname(player_fd, (struct sockaddr *) &sock, &len) == -1){
            cerr << "getsockname error!" << endl;
            exit(EXIT_FAILURE);
        }
        player_port = ntohs(sock.sin_port);
        freeaddrinfo(host_info_list);
    }

    void init_rightplayer(){
        player *Rplayer = new player;

        int right_player_port_int;
        char right_player_addr[100];
        recv(ringmaster_fd, &right_player_port_int, sizeof(right_player_port_int),MSG_WAITALL);
        recv(ringmaster_fd, &right_player_addr, sizeof(right_player_addr),MSG_WAITALL);
        char right_player_port[9];
        sprintf(right_player_port, "%d", right_player_port_int);

        Rplayer->addr = right_player_addr;
        Rplayer->player_port = right_player_port_int;

        Rplayer->player_fd = Rplayer->build_connecter(Rplayer->addr, right_player_port);
        Rplayer->player_id = (player_id+1)%num_players;

        right_player = Rplayer;
    }

    void init_leftplayer(){
        player *Lplayer = new player;

        struct sockaddr_storage socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        Lplayer->player_fd = accept(player_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
        if (Lplayer->player_fd == -1) {
            cerr << "Error: cannot accept connection on left player's socket" << endl;
            exit(EXIT_FAILURE);
        } //if
        Lplayer->ip = inet_ntoa(((struct sockaddr_in *)&socket_addr)->sin_addr);

        Lplayer->player_id = (player_id-1+num_players)%num_players;

        left_player = Lplayer;

    }

    int findmaxfd(){
        int max_fd = ringmaster_fd;
        if (right_player->player_fd > max_fd){
            max_fd = right_player->player_fd;
        }
        if (left_player->player_fd > max_fd){
            max_fd = left_player->player_fd;
        }
        return max_fd;

    }

    void getpotato(int max_fd, bool &flag){
        potato hot_potato;
        fd_set readfds;
        int nfds;
        //init_fdset(readfds,fds,nfds);
        FD_ZERO(&readfds);
        FD_SET(ringmaster_fd,&readfds);
        FD_SET(right_player->player_fd,&readfds);
        FD_SET(left_player->player_fd,&readfds);

        select(max_fd+1,&readfds,NULL,NULL,NULL);

        if (FD_ISSET(ringmaster_fd, &readfds)) {
            //cout << "test: receive from ringmaster" << endl;
            recv(ringmaster_fd, &hot_potato, sizeof(hot_potato), MSG_WAITALL);
        }
        else if (FD_ISSET(right_player->player_fd, &readfds)) {
            //cout << "test: receive from right player: " << right_player->player_id << endl;
            recv(right_player->player_fd, &hot_potato, sizeof(hot_potato), MSG_WAITALL);
        }
        else if (FD_ISSET(left_player->player_fd, &readfds)) {
            //cout << "test: receive from left player: " << left_player->player_id<< endl;
            recv(left_player->player_fd, &hot_potato, sizeof(hot_potato), MSG_WAITALL);
        }

        if (hot_potato.hops == 0 || hot_potato.flag == 1){
            //cerr << "test: potato is passed with error!" << endl;
            //cout << "test: end the game" << endl;
            //break;
            flag = false;
        }

        else {
            hot_potato.trace[hot_potato.count] = player_id;
            hot_potato.count += 1;

            if (hot_potato.count == hot_potato.hops){
                cout << "I'm it" << endl;
                //printf("sizeof final potato: %ld\n", sizeof(potato));
                hot_potato.flag = 1;
                send(ringmaster_fd, &hot_potato, sizeof(hot_potato),0);

                //break;
            }
            else{

                //printf("size of potato: %ld\n", sizeof(potato));
                
                srand ((unsigned int)(time(NULL)) + hot_potato.count + num_players);
                int next = rand() % 2;
                if (next == 0){
                    cout << "Sending potato to " << right_player->player_id << endl;
                    send(right_player->player_fd, &hot_potato, sizeof(hot_potato),0);
                }
                else{
                    cout << "Sending potato to " << left_player->player_id << endl;
                    send(left_player->player_fd, &hot_potato, sizeof(hot_potato),0);
                }
            }
        }
    }

    void endgame(){
        close(ringmaster_fd);
        close(player_fd);
        close(left_player->player_fd);
        close(right_player->player_fd); 
        delete right_player;
        delete left_player;
    }

    ~player(){
        
    }
};

class ringmaster{
    public:
    char* port;
    int num_players;
    int num_hops;
    int socket_fd;

    /*
    vector<int> player_sock_fd;
    vector<int> player_listen;
    vector<int> player_ports;
    vector<string> player_ip;
    vector<struct sockaddr_in> player_addr_s;
    */
    vector<player> players;

    ringmaster(){}

    ringmaster(char* argv[]):port(argv[1]), num_players(atoi(argv[2])), num_hops(atoi(argv[3])), socket_fd(0) {
        assert(num_players > 1);
        assert(num_hops >= 0);
        assert(num_hops <= Maxhops);
        cout << "Potato Ringmaster \nPlayers = " << num_players <<  "\nHops = " << num_hops << endl;
    }

    void init_ringserver(){
        int status;
        
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        const char *hostname = NULL;

        memset(&host_info, 0, sizeof(host_info));

        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;
        host_info.ai_flags    = AI_PASSIVE;

        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0) {
            cerr << "Error: cannot get address info for host" << endl;
            //cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket" << endl;
            //cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        int yes = 1;
        status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot bind socket" << endl;
            //cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        status = listen(socket_fd, 100);
        if (status == -1) {
            cerr << "Error: cannot listen on socket" << endl; 
            //cerr << "  (" << hostname << "," << port << ")" << endl;
            exit(EXIT_FAILURE);
        } //if

        freeaddrinfo(host_info_list);
    }

    void linktoPlayers(){
        for (int i=0; i < num_players; i++){
            string ip_addr;
            struct sockaddr_in player_addr;
            socklen_t addrlen = sizeof(player_addr);
            int player_fd = accept(socket_fd, (struct sockaddr *) &player_addr, &addrlen);
            if (player_fd == -1){
                cerr << "Error: cannot accept connection on socket when connecting" << endl;
                exit(EXIT_FAILURE);
            }

            ip_addr = inet_ntoa(((struct sockaddr_in *)&player_addr)->sin_addr);

            int playerid = i;
            int player_port;
            send(player_fd, &playerid, sizeof(playerid), 0);
            send(player_fd, &num_players, sizeof(num_players), 0);
            recv(player_fd, &player_port, sizeof(player_port), 0);

            player newplayer(player_fd, player_port, playerid, ip_addr);
            char player_addr_c[100];
            sprintf(player_addr_c, "%s", ip_addr.c_str());
            newplayer.addr = player_addr_c;

            /*
            player_sock_fd.push_back(player_fd);
            player_addr_s.push_back(player_addr);
            player_ip.push_back(ip_addr);
            player_ports.push_back(player_port);
            */

            players.push_back(newplayer);

            cout << "Player " << i << " is ready to play" << endl;
            
        }
    }

    void bind_neighb_player(){
        for(int i = 0; i < num_players; i++){
            //select the server player for curr player
            int right_player = (i + 1) % num_players;
            
            //format the info for socket transmission
            int right_player_port = players[right_player].player_port;
            string right_player_addr = players[right_player].ip;
            char right_player_addr_c[100];
            memset(right_player_addr_c, 0, sizeof(right_player_addr_c));
            //strcpy(player_server_addr_c, player_server_addr.c_str());
            sprintf(right_player_addr_c, "%s", right_player_addr.c_str());
            //send the server player's addr and port to curr player
            //for the curr player to operate
            int player_fd = players[i].player_fd;
            send(player_fd,&right_player_port,sizeof(right_player_port),0);
            send(player_fd,&right_player_addr_c,sizeof(right_player_addr_c),0);
        }
    }

    void send_allplayers_potato(potato& hot_potato){
        for (int i = 0; i < num_players; i++) {
            send(players[i].player_fd, &hot_potato, sizeof(hot_potato), 0);
        
        }
    }

    void send_oneplayer_potato(int player_id, potato &hot_potato){
        send(players[player_id].player_fd, &hot_potato, sizeof(hot_potato), 0);
    }

    void selfcheck(potato &final_potato){
        int player_receive_potato [num_players];
        memset(player_receive_potato, 0, num_players * sizeof(int));
        for (int i=0;i<num_players;i++){
            //printf("Trace of player %d: \n", i);
            for(int j=0;j<num_hops;j++){
                if(final_potato.trace[j]==i){
                    player_receive_potato[i] += 1;
                }
            }
            printf("player %d receives potato %d times.\n", i, player_receive_potato[i]);
        }
    }

    void deal_final_potato(){
        fd_set readfds;
        int max_fd = players[0].player_fd;

        FD_ZERO(&readfds);
        for (int i = 0;i < num_players; i++){
            FD_SET(players[i].player_fd, &readfds);
            if (players[i].player_fd > max_fd){
                max_fd = players[i].player_fd;
            }
        }
        select(max_fd+1, &readfds, NULL, NULL, NULL);

        potato final_potato;
        for (int i = 0; i < num_players; i++) {
            if (FD_ISSET(players[i].player_fd, &readfds)) {
                recv(players[i].player_fd, &final_potato, sizeof(final_potato), MSG_WAITALL);
                final_potato.flag = 1;
                break;
            }
        }
        assert(final_potato.count == final_potato.hops);

        send_allplayers_potato(final_potato);
        close_players();

        final_potato.printpath();

        //selfcheck(final_potato);
    }

    void close_players(){
        for (int i = 0;i < num_players; i++){
            close(players[i].player_fd);
        }
    }

    void close_ringmaster(){
        close(socket_fd);
    }

    

};