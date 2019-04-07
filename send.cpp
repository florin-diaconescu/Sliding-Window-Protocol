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
  int no_ret = 0;
  vector<cs> mesaje;
  //wnd /= 2; //nu vreau o fereastra prea mare

  int i, j, count, file, citit; //count - numarul total de pachete ce vor fi trimise

  cout << wnd << "\n";
  file = open(file_name, O_RDONLY);
  if (!file)
  {
    perror("Fisierul nu s-a putut deschide!");
    return -1;
  }

//vreau sa trimit numarul de pachete
  lseek(file, 0, SEEK_SET);
  int marime = lseek(file, 0, SEEK_END);
  if (marime % PAYLOADSIZE)
  {
    count = marime / PAYLOADSIZE;
  }
  else
  {
    count = marime / PAYLOADSIZE  + 3;
  }

  if (count < wnd)
  {
    wnd = count - 1;
  }

  cout << "wnd = " << wnd << " count = " << count << "\n";

  while(1)
  {
    memset(t.payload, 0, sizeof(t.payload));
    memset(mesaj.data, 0, sizeof(mesaj.data));
    mesaj.checksum = 0;
    mesaj.sequence_number = timeout; //trimit timeout-ul in recv
    mesaj.size = wnd; //trimit dimensiunea ferestrei in recv
    
    sprintf(mesaj.data, "%d", count);

    for (i = 0; i < PAYLOADSIZE; i++)
    {
      mesaj.checksum ^= mesaj.data[i];
    }

    mesaj.akk = 0;

    mesaj.checksum ^= mesaj.akk;
    mesaj.checksum ^= mesaj.sequence_number;
    mesaj.checksum ^= mesaj.size;  

    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    
    if (recv_message_timeout(&t, 2*timeout)<0)
    {
        perror("receive error");
    }
    else
    {
      mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A')
      {
        printf("[%s] Got reply!\n", argv[0]);
        break;
      }
      else
      {
        printf("[%s] NAK!\n", argv[0]);
      }
    }
  }

  //trimit numele fisierului
  lseek(file, 0, SEEK_SET);

  mesaj.checksum = 0;
  mesaj.sequence_number = 0;
  sprintf(mesaj.data, "%s", file_name);

  for (i = 0; i < PAYLOADSIZE; i++)
  {
    mesaj.checksum ^= mesaj.data[i];
  }

  mesaj.akk = 0;
  mesaj.size = 0;

  mesaj.checksum ^= mesaj.akk;
  mesaj.checksum ^= mesaj.sequence_number;
  mesaj.checksum ^= mesaj.size;  

  mesaje.push_back(mesaj);

  i = 1;

  int aux = count;

  while ((citit = read(file, mesaj.data, PAYLOADSIZE)) > 0)
  {
    mesaj.checksum = 0;
    mesaj.akk = 0;
    mesaj.sequence_number = i;
    mesaj.size = citit;

    for (j = 0; j < PAYLOADSIZE; j++)
    {
      mesaj.checksum ^= mesaj.data[j];
    }

    mesaj.checksum ^= mesaj.akk;
    mesaj.checksum ^= mesaj.sequence_number;
    mesaj.checksum ^= mesaj.size;  

    mesaje.push_back(mesaj);
    i++;
  }

  i = 0;
  //vreau sa trimit fisierul
  while (1)
  {
    //-------------------------------------------------------------------------
    while ((wnd > 0) && (i <= (aux + 1)))
    {
      mesaj = mesaje[i];
      memset(t.payload, 0, sizeof(t.payload));
      memcpy(t.payload, &mesaj, sizeof(mesaj));
      t.len = mesaj.size;
      printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
      send_message(&t);
      
      i++;

      wnd--;
    }

    //--------------------------------------------------------------primesc mesaj
    if (recv_message(&t) < 0)
    {
        perror("receive error");
    }
    else{
      if (strcmp(t.payload, "RECEIVED") == 0)
      {
        printf("RECEIVED\n");
        break;
      }
      cs mesaj = *((cs *)t.payload);
      if (mesaj.akk == 'A')
      {
        printf("[%s] Got reply %d!\n", argv[0], mesaj.sequence_number);
        count--;
      }
      else
      {
        printf("[%s] NAK %d!\n", argv[0], mesaj.sequence_number);
        mesaj = mesaje[mesaj.sequence_number];
        memset(t.payload, 0, sizeof(t.payload));
        memcpy(t.payload, &mesaj, sizeof(mesaj));
        t.len = MSGSIZE;
        send_message(&t);
        printf("[%s] Am trimis mesajul %d!\n", argv[0], mesaj.sequence_number);
        wnd--;
        no_ret++;
      }
    }

    wnd++;
  }

  cout << "Am retransmis atatea mesaje: " << no_ret << "\n";
  //vreau mesajul de EXIT de la receiver, dupa ce a terminat de scris in fisier
  if (recv_message(&t)<0)
  {
   perror("receive error");
  }
  else
  {
    printf("%s\n", t.payload);
  }

  close(file);
  return 0;
}