#include <iostream>
#include <pqxx/pqxx>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "exerciser.h"

using namespace std;
using namespace pqxx;

void creatTB(string filename, connection* C){
  string line, sql_cmd;
  std::stringstream ss;
  ifstream ifs;
  ifs.open(filename.c_str(), ifstream::in);

  if(!ifs.is_open()){
    cerr << "error: cannot open file " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while (getline(ifs, line)) {
    sql_cmd += line;
    ss << line << std::endl;
    //cout << line << endl;
  }
  ifs.close();

  /* Create a transactional object. */
  work W(*C);

  /* Execute SQL query */
  //W.exec(sql_cmd);
  W.exec(ss.str());
  W.commit();
}

void addPlayer(string filename, connection* C){

  ifstream ifs;
  string line, f_name, l_name;
  int player_id, team_id, uniform_num, mpg, ppg, rpg, apg;
  double spg, bpg;

  ifs.open(filename.c_str(), ifstream::in);

  if(!ifs.is_open()){
    cerr << "error: cannot open file " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while (getline(ifs, line)) {
    stringstream ss;
    ss << line;
    ss >> player_id >> team_id >> uniform_num >> f_name >> l_name >> mpg >> ppg >> rpg >> apg >> spg >> bpg;
    work W(*C); 
    string sql_cmd = "INSERT INTO PLAYER (TEAM_ID, UNIFORM_NUM, FIRST_NAME, LAST_NAME, MPG, PPG, RPG, APG, SPG, BPG) VALUES ("  + to_string(team_id) + ", " + to_string(uniform_num) + ", " +  W.quote(f_name) + ", " + W.quote(l_name) + ", " + to_string(mpg) + ", " + to_string(ppg) + ", " + to_string(rpg) + ", " + to_string(apg) + ", " + to_string(spg) + ", " + to_string(bpg) + ");";
    
    W.exec(sql_cmd);
    W.commit();

  }
  ifs.close();


}

void addTeam(string filename, connection* C){
  ifstream ifs;
  string line, name;
  int team_id, state_id, color_id, wins, losses;

  ifs.open(filename.c_str(), ifstream::in);

  if(!ifs.is_open()){
    cerr << "error: cannot open file " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while (getline(ifs, line)) {
    stringstream ss;
    ss << line;
    ss >> team_id >> name >> state_id >> color_id >> wins >> losses;
     
    work W(*C); 
    string sql_cmd = "INSERT INTO TEAM (NAME, STATE_ID, COLOR_ID, WINS, LOSSES) VALUES(" + W.quote(name) + ", " + to_string(state_id) + ", " + to_string(color_id) + ", " + to_string(wins) + ", " + to_string(losses) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();

  }
  ifs.close();


}

void addState(string filename, connection* C){
  ifstream ifs;
  string line, name;
  int state_id;

  ifs.open(filename.c_str(), ifstream::in);

  if(!ifs.is_open()){
    cerr << "error: cannot open file " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while (getline(ifs, line)) {
    stringstream ss;
    ss << line;
    ss >> state_id >> name;
     
    work W(*C); 
    string sql_cmd = "INSERT INTO STATE (NAME) VALUES (" + W.quote(name) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();

  }
  ifs.close();

}

void addColor(string filename, connection* C){
  ifstream ifs;
  string line, name;
  int color_id;

  ifs.open(filename.c_str(), ifstream::in);

  if(!ifs.is_open()){
    cerr << "error: cannot open file " << filename << endl;
    exit(EXIT_FAILURE);
  }

  while (getline(ifs, line)) {
    stringstream ss;
    ss << line;
    ss >> color_id >> name;
     
    work W(*C); 
    string sql_cmd = "INSERT INTO COLOR (NAME) VALUES (" + W.quote(name) + ");";
    
    //cout<<sql<<endl;
    W.exec(sql_cmd);
    W.commit();

  }
  ifs.close();

}


int main (int argc, char *argv[]) 
{

  //Allocate & initialize a Postgres connection object
  connection *C;

  try{
    //Establish a connection to the database
    //Parameters: database name, user name, user password
    C = new connection("dbname=ACC_BBALL user=postgres password=passw0rd");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
  } catch (const std::exception &e){
    cerr << e.what() << std::endl;
    return 1;
  }


  //TODO: create PLAYER, TEAM, STATE, and COLOR tables in the ACC_BBALL database
  //      load each table with rows from the provided source txt files
  creatTB("create_table.sql", C);
  addState("state.txt", C);
  addColor("color.txt", C);
  addTeam("team.txt", C);
  addPlayer("player.txt", C);
  
  


  exercise(C);


  //Close database connection
  C->disconnect();

  return 0;
}


