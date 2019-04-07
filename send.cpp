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
#include <vector>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc,char** argv)
{
  init(const_cast<char*>(HOST),PORT);
  msg t;
  cs mesaj;
  char *file_name = argv[1]; //numele fisierului de intrare
  int speed = atoi(argv[2]); //viteza de transmisie
  int delay = atoi(argv[3]); //delay-ul
  int wnd = (speed * delay * 1000)/(MSGSIZE * 8); //calculez dimensiunea ferestrei
  int timeout = 2 * delay; //iau timeout-ul ca 2 * delay
  vector<cs> mesaje; //vectorul de mesaje

  int i, count, file, citit; //count - numarul total de pachete ce vor fi trimise

  file = open(file_name, O_RDONLY);
  if (!file)
  {
    perror("Fisierul nu s-a putut deschide!");
    return -1;
  }

//vreau sa trimit numarul de pachete
  lseek(file, 0, SEEK_SET);
  int marime = lseek(file, 0, SEEK_END); //dimensiunea totala a fisierului
  //calculez numarul de mesaje
  if (marime % PAYLOADSIZE)
  {
    count = marime / PAYLOADSIZE;
  }
  else
  {
    count = marime / PAYLOADSIZE  + 3;
  }

  //nu vreau ca fereastra sa fie mai mare ca numarul de mesaje
  if (count < wnd)
  {
    wnd = count - 1;
  }

  //vreau sa trimit numarul de mesaje in receiver
  while(1)
  {
    memset(t.payload, 0, sizeof(t.payload));
    memset(mesaj.data, 0, sizeof(mesaj.data));
    mesaj.sequence_number = timeout; //trimit timeout-ul in receiver
    mesaj.size = wnd; //trimit dimensiunea ferestrei in receiver
    
    sprintf(mesaj.data, "%d", count);

    //calculez checksum-ul mesajului
    mesaj.checksum = calculate_checksum(mesaj);

    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);
    
    if (recv_message_timeout(&t, 2 * timeout)<0)
    {
        perror("receive error");
    }
    else
    {
      mesaj = *((cs *)t.payload);
      //opresc bucla infinita doar daca primesc ACK de la receiver
      if (mesaj.akk == 'A')
      {
        break;
      }
    }
  }

  //salvez numele fisierului de intrare in mesaje[0]
  lseek(file, 0, SEEK_SET);

  mesaj.sequence_number = 0;
  mesaj.akk = 0;
  mesaj.size = 0;
  sprintf(mesaj.data, "%s", file_name);

  mesaj.checksum = calculate_checksum(mesaj); 

  mesaje.push_back(mesaj);

  i = 1;

  //salvez fiecare mesaj (cu bucati de 1394 de bytes din fisierul de
  //intrare salvate in mesaj.data), cu campurile aferente
  while ((citit = read(file, mesaj.data, PAYLOADSIZE)) > 0)
  {
    mesaj.akk = 0;
    mesaj.sequence_number = i;
    mesaj.size = citit;

    mesaj.checksum = calculate_checksum(mesaj); 

    mesaje.push_back(mesaj);
    i++;
  }

  i = 0; //folosit pentru numarul de secventa

  //vreau sa trimit fisierul
  while (1)
  {
    //-------------------------------------------------------------------------
    //cat timp ma incadrez in dimensiunea ferestrei si trimit mesaje valide, cu
    //numarul de secventa mai mic decat numarul maxim
    while ((wnd > 0) && (i <= (count + 1)))
    {
      mesaj = mesaje[i];
      memset(t.payload, 0, sizeof(t.payload));
      memcpy(t.payload, &mesaj, sizeof(mesaj));
      t.len = mesaj.size;
      send_message(&t);
      
      i++;
      wnd--;
    }

    //----------------------------------------------------------primesc mesaj
    if (recv_message(&t) < 0)
    {
        perror("receive error");
    }
    else{
      //intrerup bucla daca primesc de la receiver ca toate pachetele au fost
      //receptionate
      if (strcmp(t.payload, "RECEIVED") == 0)
      {
        break;
      }

      cs mesaj = *((cs *)t.payload);
      //daca primesc NAK, vreau sa retrimit mesajul pierdut/transmis incorect
      if (mesaj.akk == 'N')
      {
        mesaj = mesaje[mesaj.sequence_number];
        memset(t.payload, 0, sizeof(t.payload));
        memcpy(t.payload, &mesaj, sizeof(mesaj));
        t.len = MSGSIZE;
        send_message(&t);
        wnd--;
      }
    }

    wnd++;
  }

  //vreau mesajul de EXIT de la receiver, dupa ce a terminat de scris in fisier
  if (recv_message(&t)<0)
  {
   perror("receive error");
  }

  close(file);
  return 0;
}