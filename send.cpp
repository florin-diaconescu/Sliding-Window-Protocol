extern "C"{
  #include <string.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include "link_emulator/lib.h"
  #include "struct.h"
}

#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv){
  init(const_cast<char*>(HOST),PORT);
  msg t;
  cs mesaj;
  char *file_name = argv[1];
  int speed = atoi(argv[2]);
  int delay = atoi(argv[3]);
  int wnd = (speed * delay * 1000)/(MSGSIZE * 8);
  wnd /= 2; //nu vreau o fereastra prea mare

  int i, j, count, file, citit; //count - numarul total de pachete ce vor fi trimise
  //int msg_count = 0;
  cout << wnd << "\n";
  file = open(file_name, O_RDONLY);
  if (!file){
    perror("Fisierul nu s-a putut deschide!");
    return -1;
  }

//trimit numele fisierului
  while(1){
    memset(t.payload, 0, sizeof(t.payload));
    memset(mesaj.data, 0, sizeof(mesaj.data));
    mesaj.checksum = 0;
    mesaj.sequence_number = 0;
    sprintf(mesaj.data, "%s", file_name);

    for (i = 0; i < PAYLOADSIZE; i++){
      mesaj.checksum ^= mesaj.data[i];
    }

    mesaj.akk = 0;

    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    
    if (recv_message(&t)<0){
        perror("receive error");
    }
    else{
      mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply!\n", argv[0]);
        break;
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }
  }

//vreau sa trimit numarul de pachete
  lseek(file, 0, SEEK_SET);
  int marime = lseek(file, 0, SEEK_END);
  if (marime % PAYLOADSIZE){
    count = marime / PAYLOADSIZE;
  }
  else{
    count = marime / PAYLOADSIZE  + 3;
  }

  if (count < wnd){
    wnd = count - 1;
  }

  cout << "wnd = " << wnd << " count = " << count << "\n";

  while(1){
    memset(t.payload, 0, sizeof(t.payload));
    memset(mesaj.data, 0, sizeof(mesaj.data));
    mesaj.checksum = 0;
    mesaj.sequence_number = 0;
    //memcpy(mesaj.data, &count, sizeof(count));
    sprintf(mesaj.data, "%d", count);

    for (i = 0; i < PAYLOADSIZE; i++){
      mesaj.checksum ^= mesaj.data[i];
    }

    mesaj.akk = 0;

    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    
    if (recv_message(&t)<0){
        perror("receive error");
    }
    else{
      mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply!\n", argv[0]);
        break;
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }
  }

//vreau sa trimit fisierul

  lseek(file, 0, SEEK_SET);

  for (i = 0; i < wnd; i++){
    citit = read(file, mesaj.data, PAYLOADSIZE);
    if (citit < 0){
      perror("Nu s-a reusit citirea din fisier!\n");
      return -1;
    }

    mesaj.checksum = 0;
    mesaj.akk = 0;
    mesaj.sequence_number = i;

    for (j = 0; j < PAYLOADSIZE; j++){
      mesaj.checksum ^= mesaj.data[j];
    }

    //cout << mesaj.checksum << "\n";
    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = citit;
    send_message(&t);
    printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
  }

  for (i = 0; i <= (count - wnd); i++){
    //TODO : primeste ACK si trimite mesaj nou
    if (recv_message(&t)<0){
        perror("receive error");
    }
    else{
      cs mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply %d!\n", argv[0], mesaj.sequence_number);
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }

    citit = read(file, mesaj.data, PAYLOADSIZE);
    //cout<<"--------------" << citit << "\n";
    if (citit < 0){
      perror("Nu s-a reusit citirea din fisier!\n");
      return -1;
    }

    mesaj.checksum = 0;
    mesaj.akk = 0;
    mesaj.sequence_number = i + wnd;

    for (j = 0; j < PAYLOADSIZE; j++){
      mesaj.checksum ^= mesaj.data[j];

    }

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = citit;
    send_message(&t);
    printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
  }

  for (i = 0; i < wnd; i++){
    //TODO : primeste ACK-uri
    if (recv_message(&t)<0){
        perror("receive error");
    }
    else{
      cs mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply %d!\n", argv[0], mesaj.sequence_number);
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }
  }
  close(file);
  return 0;
}
