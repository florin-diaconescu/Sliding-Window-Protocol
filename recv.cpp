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
  unsigned int i, j;
  int bgn = 0; //folosit pentru a vedea de unde incep calculele cu fereastra
  int wnd; //dimensiunea ferestrei
  int msg_count, file, aux;
  int expected_message = 0;
  int no_timeout = 0;
  //int last_length; //lungimea ultimului mesaj
  int nrsort = 0;
  //int last_req_msg = -1;
  int write_check; //variabila pentru a verifica scrierea in fisier
  char checksum_r; //checksum
  int timeout;
  string sent_name;
  //set<cs> mesaje; //vreau sa nu inserez mesaje duplicate
  vector<cs>::iterator it;
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

    checksum_r ^= mesaj.akk;
    checksum_r ^= mesaj.sequence_number;
    checksum_r ^= mesaj.size;  

    if (checksum_r != mesaj.checksum){
      mesaj.akk = 'N';
      cout << "S-a pierdut primul pachet!\n";
    }
    else{
      mesaj.akk = 'A';
      cout << "E totul bine!\n";
    }

    msg_count = atoi(mesaj.data) + 2;
    timeout = (int)mesaj.sequence_number;
    wnd = (int)mesaj.size;
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
  vector<int> packets(msg_count);
  mesaj.size = 0;
  vector<cs> mesaje(msg_count, mesaj);
  for (int i = 0; i < msg_count; i++){
    packets[i] = i;
  }

  while(1){
    /*for (unsigned int i = 0 ; i < packets.size(); i++){
      cout <<packets[i] << " ";
    }
    cout << "\n";*/
    if (recv_message_timeout(&r, 2 * timeout) < 0){
      printf("[%s] Timeout, expected %d!\n", argv[0], expected_message);
      memset(t.payload, 0, sizeof(t.payload));
      memset(mesaj.data, 0, sizeof(mesaj.data));
      mesaj.akk = 'N';
      no_timeout++;
      
      //nrsort++;

      mesaj.sequence_number = expected_message;
      memcpy(t.payload, &mesaj, sizeof(mesaj));
      t.len = MSGSIZE;
      send_message(&t);

      continue;
    }

    mesaj = *((cs *)r.payload);

    printf("[%s] Got message %d!\n",argv[0], mesaj.sequence_number);

    checksum_r = 0;
    for (i = 0; i < PAYLOADSIZE; i++){
      checksum_r ^= mesaj.data[i];
    }

    checksum_r ^= mesaj.akk;
    checksum_r ^= mesaj.sequence_number;
    checksum_r ^= mesaj.size;  

    //out << checksum_r << " " << mesaj.checksum << "\n";
    if (checksum_r != mesaj.checksum){
      
      //last_req_msg = mesaj.sequence_number;
      printf("CORRUPT! %d", bgn);
      packets.erase(packets.begin() + bgn);
          if ((bgn + wnd) < aux){
            packets.insert(packets.begin() + (bgn + wnd - 1), expected_message);
          }
          else{
            packets.insert(packets.begin() + (aux - 1), expected_message);
          }
          //bgn--;
      mesaj.akk = 'N'; //corrupt
      mesaj.sequence_number = expected_message;
      //nrsort++;
    }

    else{

      //mesajul nu este corupt, dar nu este cel pe care il asteptam
      if (mesaj.sequence_number != expected_message){
        printf("[%s] Wrong message, expected %d!\n", argv[0], expected_message);
        if (mesaje[mesaj.sequence_number].size == 0){
          mesaje[mesaj.sequence_number] = mesaj;
          //  bgn++;
          packets.erase(packets.begin() + bgn);
          if ((bgn + wnd + 1) < aux){
            packets.insert(packets.begin() + (bgn + wnd - 1), expected_message);
          }
          else{
            cout << "E pe final!\n";
            packets.insert(packets.begin() + (aux - 1), expected_message);
          }

          for (j = bgn; j < packets.size(); j++){
            if (packets[j] == mesaj.sequence_number){
              break;
            }
          }

          packets.erase(packets.begin() + j);
          packets.insert(packets.begin(), mesaj.sequence_number);
          bgn++;
          cout << "--- bgn =  " << bgn << " ";
          //packets.erase(packets.begin() + bgn);
          
          nrsort++;
        }
        else{
          cout << "Nasol, mai exista deja!" << mesaj.sequence_number << "\n";
          packets.erase(packets.begin() + bgn);
          if ((bgn + wnd) < aux){
            packets.insert(packets.begin() + (bgn + wnd - 2), expected_message);
          }
          else{
            cout << "E pe final!\n";
            packets.insert(packets.begin() + (aux - 1), expected_message);
          }
        }

        mesaj.akk = 'N';

        mesaj.sequence_number = expected_message;

      }
      else{
        mesaj.akk = 'A';
        if (mesaje[mesaj.sequence_number].size == 0){

          mesaje[mesaj.sequence_number] = mesaj;
          //packets.erase(packets.begin() + bgn);
          bgn++;
          nrsort++;
        }
      } 
    }

    //verific daca am receptionat deja toate mesajele
    if (nrsort == msg_count){
      cout << "AAAAA" << "\n";
      break;
    }
    
    //bgn++;
    expected_message = packets[bgn];
    //cout << "Expected message: " << expected_message << "\n";
    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
  }

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
    if ((write_check = write(file, ((cs)(*it)).data, ((cs)(*it)).size)) < 0){
      perror("Write error!");
      return -1;
    }
  }

  close(file);
  cout << "Am luat timeout de: " << no_timeout << "\n";

  //trimit sender-ului un mesaj ca s-a terminat scrierea in fisier
  memset(t.payload, 0, sizeof(t.payload));
  sprintf(t.payload, "%s", "DONE");
  t.len = MSGSIZE;
  send_message(&t);

  return 0;
}

