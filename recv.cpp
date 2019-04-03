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
#include <algorithm>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10001

bool cmp_seq(cs A, cs B){
  return A.sequence_number < B.sequence_number;
}

int main(int argc,char** argv){
  msg r,t;
  cs mesaj;
  unsigned int i;
  int msg_count, file;
  int write_check; //variabila pentru a verifica scrierea in fisier
  char checksum_r; //checksum
  string sent_name;
  vector<cs> mesaje;

  init(const_cast<char*>(HOST),PORT);

//vreau sa primesc numele fisierului
  /*
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

  sent_name = r.payload;
  mesaj.sequence_number = 0;
  memset(t.payload, 0, sizeof(t.payload));
  memcpy(t.payload, &mesaj, sizeof(mesaj));
  t.len = MSGSIZE;
  send_message(&t);
  */
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
      mesaje.push_back(mesaj);
      msg_count--;
    }

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
  }

  sort(mesaje.begin(), mesaje.end(), cmp_seq);

  string receive_name = "recv_";
  receive_name += mesaje[0].data;
  file = open(receive_name.c_str(), O_WRONLY | O_CREAT, 0644);

  for (i = 1; i < mesaje.size() - 1; i++){
    if ((write_check = write(file, mesaje[i].data, PAYLOADSIZE)) < 0){
      perror("Write error!");
      return -1;
    }
  }

  if ((write_check = write(file, mesaje[i].data, r.len)) < 0){
    perror("Write error!");
    return -1;
  }

  close(file);

  //trimit sender-ului un mesaj ca s-a terminat scrierea in fisier
  memset(t.payload, 0, sizeof(t.payload));
  sprintf(t.payload, "%s", "DONE");
  t.len = MSGSIZE;
  send_message(&t);

  return 0;
}
