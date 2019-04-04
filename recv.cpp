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
#include <set>
#include <algorithm>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv){
  msg r,t;
  cs mesaj;
  unsigned int i;
  int msg_count, file, aux;
  int write_check; //variabila pentru a verifica scrierea in fisier
  char checksum_r; //checksum
  string sent_name;
  set<cs> mesaje; //vreau sa nu inserez mesaje duplicate
  set<cs>::iterator it;

  init(const_cast<char*>(HOST),PORT);

//vreau sa primesc numarul de pachete
  if (recv_message(&r)<0){
    perror("Receive message");
    return -1;
  }

  printf("[%s] Got message!\n",argv[0]);

  mesaj = *((cs *)r.payload);
  checksum_r = 0;

  for (i = 0; i < PAYLOADSIZE; i++){
    checksum_r ^= mesaj.data[i];
  }

  if (checksum_r != mesaj.checksum){
    mesaj.akk = 'N';
  }
  else{
    mesaj.akk = 'A';
  }

  msg_count = atoi(mesaj.data);
  mesaj.sequence_number = 0;
  memset(t.payload, 0, sizeof(t.payload));
  memcpy(t.payload, &mesaj, sizeof(mesaj));
  t.len = MSGSIZE;
  send_message(&t);
  
//vreau sa primesc pachetele
  aux = msg_count;

  while(msg_count >= -1){
    if (recv_message(&r)<0){
      perror("Receive message");
      return -1;
    }

    mesaj = *((cs *)r.payload);
    checksum_r = 0;
    printf("[%s] Got message %d!\n",argv[0], mesaj.sequence_number);

    for (i = 0; i < PAYLOADSIZE; i++){
      checksum_r ^= mesaj.data[i];
    }

    //cout << "checksum A: " << checksum_r << " checksum B: " << mesaj.checksum << "\n";
    if (checksum_r != mesaj.checksum){
      mesaj.akk = 'N';
    }
    else{
      mesaj.akk = 'A';
      mesaje.insert(mesaj);
      msg_count--;
    }

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
  }

  msg_count = aux;

  //sort(mesaje.begin(), mesaje.end(), cmp_seq);
  it = mesaje.begin();

  string receive_name = "recv_";
  receive_name += ((cs)(*it)).data;
  file = open(receive_name.c_str(), O_WRONLY | O_CREAT, 0644);

  it++;

  for (; it != mesaje.end(); it++){
    //ultimul mesaj este posibil sa aiba o dimensiune mai mica
    if (((cs)(*it)).sequence_number == (msg_count + 1)){
      if ((write_check = write(file, ((cs)(*it)).data, r.len)) < 0){
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
