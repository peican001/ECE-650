#include "query_funcs.h"


void add_player(connection *C, int team_id, int jersey_num, string first_name, string last_name,
                int mpg, int ppg, int rpg, int apg, double spg, double bpg)
{
    work W(*C); 
    string sql_cmd = "INSERT INTO PLAYER (TEAM_ID, UNIFORM_NUM, FIRST_NAME, LAST_NAME, MPG, PPG, RPG, APG, SPG, BPG) VALUES ("  + to_string(team_id) + ", " + to_string(jersey_num) + ", " +  W.quote(first_name) + ", " + W.quote(last_name) + ", " + to_string(mpg) + ", " + to_string(ppg) + ", " + to_string(rpg) + ", " + to_string(apg) + ", " + to_string(spg) + ", " + to_string(bpg) + ");";
    
    W.exec(sql_cmd);
    W.commit();
}


void add_team(connection *C, string name, int state_id, int color_id, int wins, int losses)
{
    work W(*C); 
    string sql_cmd = "INSERT INTO TEAM (NAME, STATE_ID, COLOR_ID, WINS, LOSSES) VALUES(" + W.quote(name) + ", " + to_string(state_id) + ", " + to_string(color_id) + ", " + to_string(wins) + ", " + to_string(losses) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();
}


void add_state(connection *C, string name)
{
    work W(*C); 
    string sql_cmd = "INSERT INTO STATE (NAME) VALUES (" + W.quote(name) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();
}


void add_color(connection *C, string name)
{
    work W(*C); 
    string sql_cmd = "INSERT INTO COLOR (NAME) VALUES (" + W.quote(name) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();
}

/*
 * All use_ params are used as flags for corresponding attributes
 * a 1 for a use_ param means this attribute is enabled (i.e. a WHERE clause is needed)
 * a 0 for a use_ param means this attribute is disabled
 */
void query1(connection *C,
	    int use_mpg, int min_mpg, int max_mpg,
            int use_ppg, int min_ppg, int max_ppg,
            int use_rpg, int min_rpg, int max_rpg,
            int use_apg, int min_apg, int max_apg,
            int use_spg, double min_spg, double max_spg,
            int use_bpg, double min_bpg, double max_bpg
            )
{
    string attr_name[6] = {"MPG","PPG","RPG","APG","SPG","BPG"};
    int filter[6] = {use_mpg,use_ppg,use_rpg,use_apg,use_spg,use_bpg};
    double Mins[6] = {min_mpg,min_ppg,min_rpg,min_apg,min_spg,min_bpg};
    double Maxs[6] = {max_mpg,max_ppg,max_rpg,max_apg,max_spg,max_bpg};

    bool startflag = false;
    stringstream query;
    query << "SELECT * FROM PLAYER";

    for (int i = 0; i < 6; i++){
        if(filter[i]){
            if(!startflag){
                query << " WHERE ";   
                startflag = true;
            }
            else{
                query << " AND ";
            }

            query << attr_name[i] << " BETWEEN " << Mins[i] << " AND " << Maxs[i];

        }
    }
    nontransaction N(*C);
    result R(N.exec(query.str()));
    cout<<"PLAYER_ID TEAM_ID UNIFORM_NUM FIRST_NAME LAST_NAME MPG PPG RPG APG SPG BPG"<<endl;
    for(result::iterator i = R.begin(); i!=R.end();i++){
        cout<<i[0].as<int>()<<" "<<i[1].as<int>()<<" "<<i[2].as<int>()<<" "
        <<i[3].as<string>()<<" "<<i[4].as<string>()<<" "<<i[5].as<int>()<<" "
        <<i[6].as<int>()<<" "<<i[7].as<int>()<<" "<<i[8].as<int>()<<" "
        <<fixed<<setprecision(1)<<i[9].as<double>()<<" "<<i[10].as<double>()<<endl;
    }

}


void query2(connection *C, string team_color)
{
    stringstream query;
    query<<"SELECT TEAM.NAME FROM TEAM, COLOR WHERE TEAM.COLOR_ID = COLOR.COLOR_ID AND COLOR.NAME = \'"<<team_color<<"\';";
    nontransaction N(*C);
    result R(N.exec(query.str()));
    cout<<"NAME"<<endl;
    for(result::iterator i = R.begin(); i!=R.end();i++){
        cout<<i[0].as<string>()<<endl;
    }
}


void query3(connection *C, string team_name)
{
    stringstream query;
    query<<"SELECT PLAYER.FIRST_NAME, PLAYER.LAST_NAME FROM TEAM, PLAYER WHERE PLAYER.TEAM_ID = TEAM.TEAM_ID AND TEAM.NAME = \'" << team_name <<"\' ORDER BY PPG DESC;";
    nontransaction N(*C);
    result R(N.exec(query.str()));
    cout<<"FIRST_NAME LAST_NAME"<<endl;
    for(result::iterator i = R.begin(); i!=R.end();i++){
        cout<<i[0].as<string>()<<" "<<i[1].as<string>()<<endl;
    }
}


void query4(connection *C, string team_state, string team_color)
{
    stringstream query;
    query<<"SELECT PLAYER.UNIFORM_NUM, PLAYER.FIRST_NAME, PLAYER.LAST_NAME FROM PLAYER, TEAM, STATE, COLOR WHERE TEAM.STATE_ID = STATE.STATE_ID AND TEAM.COLOR_ID = COLOR.COLOR_ID AND PLAYER.TEAM_ID = TEAM.TEAM_ID AND STATE.NAME = \'"<<team_state<<"\'" <<" AND COLOR.NAME = \'"<<team_color<<"\';";
        
    nontransaction N(*C);
    result R(N.exec(query.str()));
    cout<<"UNIFORM_NUM FIRST_NAME LAST_NAME"<<endl;
    for(result::iterator i = R.begin(); i!=R.end();i++){
        cout<<i[0].as<int>()<<" "<<i[1].as<string>()<<" "<<i[2].as<string>()<<endl;
    }
}


void query5(connection *C, int num_wins)
{
    stringstream query;
    query<<"SELECT PLAYER.FIRST_NAME, PLAYER.LAST_NAME, TEAM.NAME, TEAM.WINS FROM PLAYER, TEAM WHERE PLAYER.TEAM_ID = TEAM.TEAM_ID AND TEAM.WINS > \'"<<num_wins<<"\';";
    
    nontransaction N(*C);
    result R(N.exec(query.str()));
    cout<<"FIRST_NAME LAST_NAME NAME WINS"<<endl;
    for(result::iterator i = R.begin(); i!=R.end();i++){
        cout<<i[0].as<string>()<<" "<<i[1].as<string>()<<" "<<i[2].as<string>()<<" "<<i[3].as<int>()<<endl;
    }
}
