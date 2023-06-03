#include "potato.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <time.h>
//#include "ringmaster.h"

class player{
    public:
    int player_fd;
    int port;
    int id;
    string ip;

    player* left_player;
    player* right_player;

};