extern "C"{
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include "link_emulator/lib.h"
  #include "struct.h"
}

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
  msg r,t;
  cs mesaj;
  vector<int> inserted;
  unsigned int i;
  int msg_count, file, aux;
  int expected_message = 0;
  int last_length; //lungimea ultimului mesaj
  int last_req_msg = -1;
  int write_check; //variabila pentru a verifica scrierea in fisier
  char checksum_r; //checksum
  int timeout;
  string sent_name;
  set<cs> mesaje; //vreau sa nu inserez mesaje duplicate
  set<cs>::iterator it;
  pair<set<cs>::iterator, bool> ret;

  init(const_cast<char*>(HOST),PORT);

//vreau sa primesc numarul de pachete
  while(1){
    if (recv_message(&r) < 0){
      perror("Receive message");
      mesaj.akk = 'N';
    }

    printf("[%s] Got message!\n",argv[0]);

    mesaj = *((cs *)r.payload);
    checksum_r = 0;

    for (i = 0; i < PAYLOADSIZE; i++){
      checksum_r ^= mesaj.data[i];
    }

    if (checksum_r != mesaj.checksum){
      mesaj.akk = 'N';
      cout << "S-a pierdut primul pachet!\n";
    }
    else{
      mesaj.akk = 'A';
      cout << "E totul bine!\n";
    }

    msg_count = atoi(mesaj.data);
    timeout = mesaj.sequence_number;
    mesaj.sequence_number = 0;
    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    if (mesaj.akk == 'A'){
      break;
    }
  }
   
//vreau sa primesc pachetele
  aux = msg_count; //salvez numarul de mesaje total pentru scrierea in fisier

  while(1){
    //cout << "--------Am atatea mesaje: " << mesaje.size() << "------\n";
    if (recv_message_timeout(&r, timeout) < 0){
      printf("[%s] Timeout, expected %d!\n", argv[0], expected_message);
      memset(t.payload, 0, sizeof(t.payload));
      memset(mesaj.data, 0, sizeof(mesaj.data));
      mesaj.akk = 'N';
      //mesaj.sequence_number = expected_message;
      memcpy(t.payload, &mesaj, sizeof(mesaj));
      t.len = MSGSIZE;
      send_message(&t);

      continue;
    }

    mesaj = *((cs *)r.payload);


    checksum_r = 0;
    printf("[%s] Got message %d!\n",argv[0], mesaj.sequence_number);

    for (i = 0; i < PAYLOADSIZE; i++){
      checksum_r ^= mesaj.data[i];
    }

    if (checksum_r != mesaj.checksum){
      mesaj.akk = 'N'; //corrupt
      cout << "CORUPT! " << mesaj.sequence_number;
      //mesaj.sequence_number = expected_message;
      last_req_msg = mesaj.sequence_number;
    }
    else{    
      if(r.len != MSGSIZE && r.len != PAYLOADSIZE && r.len != 0){
        last_length = r.len;
      }
      //mesajul nu este corupt, dar nu este cel pe care il asteptam
      if (mesaj.sequence_number != expected_message && (expected_message != last_req_msg)){
        cout << "----------msg_sq_no " << mesaj.sequence_number << " exp_msg "<< expected_message << " last_req_msg "<< last_req_msg <<"\n";
        printf("[%s] Wrong message, expected %d!\n", argv[0], expected_message);
        ret = mesaje.insert(mesaj);
        if (ret.second){
          inserted.push_back(mesaj.sequence_number);
          msg_count--;
        }

        memset(t.payload, 0, sizeof(t.payload));
        memset(mesaj.data, 0, sizeof(mesaj.data));
        mesaj.akk = 'N';
        last_req_msg = expected_message;
        
        //next_expected_message = mesaj.sequence_number + 1;
        
        //cout << "--------- " << next_expected_message <<"\n";
        //daca mesajul nu exista deja in set
        

        mesaj.sequence_number = expected_message;
      }
      else{
        mesaj.akk = 'A';
        ret = mesaje.insert(mesaj);
        if(ret.second){
          inserted.push_back(mesaj.sequence_number);
          msg_count--;
        } 

        expected_message = 0;
        sort(inserted.begin(), inserted.end());
        for (i = 0; i < (inserted.size() - 1); i++){
          if (inserted[i + 1] - inserted [i] > 1){
            expected_message = inserted[i] + 1;
            //cout << "Mesajul dorit " << expected_message << "\n";
            break;
          }
        }
        if ((expected_message == 0)){// && (msg_count >= -1)){
          expected_message = inserted[i] + 1;
          //cout << "Mesajul dorit " << expected_message << "\n";
        }
        if (mesaje.size() > ((unsigned int)aux + 1)){
          break;
        }   
      } 
    }

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
  }

  cout << "!!!!!!!!!!!!!!! " << last_length << "\n";
  //trimit sender-ului ca s-a terminat primirea pachetelor
  memset(t.payload, 0, sizeof(t.payload));
  sprintf(t.payload, "%s", "RECEIVED");
  t.len = MSGSIZE;
  send_message(&t);
  /////////

  msg_count = aux;

  it = mesaje.begin();

  string receive_name = "recv_";
  receive_name += ((cs)(*it)).data;
  file = open(receive_name.c_str(), O_WRONLY | O_CREAT, 0644);

  it++;

  for (; it != mesaje.end(); it++){
    //ultimul mesaj este posibil sa aiba o dimensiune mai mica
    if (((cs)(*it)).sequence_number == (msg_count + 1)){
      
      if ((write_check = write(file, ((cs)(*it)).data, last_length)) < 0){
        perror("Write error!");
        return -1;
      }
    }
    else if ((write_check = write(file, ((cs)(*it)).data, PAYLOADSIZE)) < 0){
      perror("Write error!");
      return -1;
    }
  }

  close(file);

  //trimit sender-ului un mesaj ca s-a terminat scrierea in fisier
  memset(t.payload, 0, sizeof(t.payload));
  sprintf(t.payload, "%s", "DONE");
  t.len = MSGSIZE;
  send_message(&t);

  return 0;
}
