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
#include <algorithm>

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
  int timeout = 2 * delay;
  int last_message_received = -1;
  vector<cs> mesaje;
  //wnd /= 2; //nu vreau o fereastra prea mare

  int i, j, count, file, citit; //count - numarul total de pachete ce vor fi trimise

  cout << wnd << "\n";
  file = open(file_name, O_RDONLY);
  if (!file){
    perror("Fisierul nu s-a putut deschide!");
    return -1;
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
    
    sprintf(mesaj.data, "%d", count);

    for (i = 0; i < PAYLOADSIZE; i++){
      mesaj.checksum ^= mesaj.data[i];
    }

    mesaj.akk = 0;

    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    
    if (recv_message_timeout(&t, timeout)<0){
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

//trimit numele fisierului
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
  mesaje.push_back(mesaj);

//vreau sa trimit fisierul

  lseek(file, 0, SEEK_SET);
//-----------------------------------------------------------------------------
  for (i = 0; i < wnd; i++){
    citit = read(file, mesaj.data, PAYLOADSIZE);
    if (citit < 0){
      perror("Nu s-a reusit citirea din fisier!\n");
      return -1;
    }

    mesaj.checksum = 0;
    mesaj.akk = 0;
    mesaj.sequence_number = i + 1;

    for (j = 0; j < PAYLOADSIZE; j++){
      mesaj.checksum ^= mesaj.data[j];
    }

    //cout << mesaj.checksum << "\n";
    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = citit;
    send_message(&t);
    mesaje.push_back(mesaj);
    printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
  }

//-----------------------------------------------------------------------------
  for (i = 0; i <= (count - wnd); i++){
    //TODO : primeste ACK si trimite mesaj nou
    if (recv_message_timeout(&t, timeout) < 0){
        perror("receive error");
        //retrimit mesajul pe care trebuia sa il primesc
        mesaj = mesaje[last_message_received + 1];
        memset(t.payload, 0, sizeof(t.payload));
        memcpy(t.payload, &mesaj, sizeof(mesaj));
        t.len = MSGSIZE;
        send_message(&t);
        printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
    }
    else{
      cs mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply %d!\n", argv[0], mesaj.sequence_number);
        //daca nu am primit mesajul pe care il asteptam
        if (mesaj.sequence_number != (last_message_received + 1)){
          for (i = (last_message_received + 1); i < mesaj.sequence_number; i++){
            //retrimite mesaje
            mesaj = mesaje[i];
            memset(t.payload, 0, sizeof(t.payload));
            memcpy(t.payload, &mesaj, sizeof(mesaj));
            t.len = MSGSIZE;
            send_message(&t);
            printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
          }
        }
        else{
          last_message_received++;
        }
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }

    citit = read(file, mesaj.data, PAYLOADSIZE);
    if (citit < 0){
      perror("Nu s-a reusit citirea din fisier!\n");
      return -1;
    }

    mesaj.checksum = 0;
    mesaj.akk = 0;
    mesaj.sequence_number = i + wnd + 1;

    for (j = 0; j < PAYLOADSIZE; j++){
      mesaj.checksum ^= mesaj.data[j];

    }

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = citit;
    send_message(&t);
    mesaje.push_back(mesaj);
    printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
  }

//-----------------------------------------------------------------------------
  for (i = 0; i <= wnd; i++){
    //TODO : primeste ACK-uri
    if (recv_message_timeout(&t, timeout) < 0){
        //retrimit mesajul pe care trebuia sa il primesc
        perror("receive error");
        mesaj = mesaje[last_message_received + 1];
        memset(t.payload, 0, sizeof(t.payload));
        memcpy(t.payload, &mesaj, sizeof(mesaj));
        t.len = MSGSIZE;
        send_message(&t);
        printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
    }
    else{
      cs mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A'){
        printf("[%s] Got reply %d!\n", argv[0], mesaj.sequence_number);
        //daca nu am primit mesajul pe care il asteptam
        if (mesaj.sequence_number != (last_message_received + 1)){
          for (i = (last_message_received + 1); i < mesaj.sequence_number; i++){
            //retrimite mesaje
            mesaj = mesaje[i];
            memset(t.payload, 0, sizeof(t.payload));
            memcpy(t.payload, &mesaj, sizeof(mesaj));
            t.len = MSGSIZE;
            send_message(&t);
            printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
          }
        }
        else{
          last_message_received++;
        }
      }
      else{
        printf("[%s] NAK!\n", argv[0]);
      }
    }
  }

  //vreau mesajul de EXIT de la receiver, dupa ce a terminat de scris in fisier
  if (recv_message(&t)<0){
   perror("receive error");
  }

  close(file);
  return 0;
}
