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

using namespace std;

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc,char** argv)
{
  msg r,t; //mesajul receptionat si transmis
  cs mesaj; //variabila pentru stocarea unui mesaj
  unsigned int j;
  int bgn = 0; //folosit pentru a vedea de unde incep calculele cu fereastra
  int wnd; //dimensiunea ferestrei
  int msg_count; //numarul total de mesaje ce sunt asteptate
  int file; //file descriptor pentru fisierul de iesire
  int expected_message = 0; //folosit ca sa vad ce mesaj ma astept sa primesc
  int nr_msg = 0;
  int write_check; //variabila pentru a verifica scrierea in fisier
  char checksum_r; //checksum
  int timeout; //vreau sa primesc timeout-ul de la sender
  vector<cs>::iterator it; //iterator pentru vectorul de mesaje

  //am nevoie de conversie pentru a nu primi warning
  init(const_cast<char*>(HOST),PORT);

//vreau sa primesc numarul de pachete
  while(1)
  {
    if (recv_message(&r) < 0){
      perror("Receive message");
      mesaj.akk = 'N';
    }

    //calculez checksum
    mesaj = *((cs *)r.payload);

    checksum_r = calculate_checksum(mesaj);  

    if (checksum_r != mesaj.checksum)
    {
      mesaj.akk = 'N';
    }
    else
    {
      //daca mesajul nu e corupt, salvez informatiile primite din receiver
      mesaj.akk = 'A';
      msg_count = atoi(mesaj.data) + 2;
      timeout = (int)mesaj.sequence_number;
      wnd = (int)mesaj.size;
    }

    mesaj.sequence_number = 0;

    memset(t.payload, 0, sizeof(t.payload));
    memcpy(t.payload, &mesaj, sizeof(mesaj));
    t.len = MSGSIZE;
    send_message(&t);

    //opresc bucla infinita daca mesajul a sosit necorupt
    if (mesaj.akk == 'A')
    {
      break;
    }
  }
  
  cout << "AYY\n";
  //initializez vectorul de pachete
  vector<int> packets(msg_count);  
  for (int i = 0; i < msg_count; i++)
  {
    packets[i] = i;
  }

  //initializez vectorul de mesaje cu un mesaj dummy
  mesaj.size = 0;
  vector<cs> mesaje(msg_count, mesaj);

  while(1)
  {
    //conditia pentru timeout, in care cer mesajul care trebuia sa ajunga
    if (recv_message_timeout(&r, 2 * timeout) < 0)
    {
      memset(t.payload, 0, sizeof(t.payload));
      memset(mesaj.data, 0, sizeof(mesaj.data));
      mesaj.akk = 'N';

      mesaj.sequence_number = expected_message;
      memcpy(t.payload, &mesaj, sizeof(mesaj));
      t.len = MSGSIZE;
      send_message(&t);

      continue;
    }

    mesaj = *((cs *)r.payload);
    //calculez checksum
    checksum_r = calculate_checksum(mesaj);

    //daca mesajul e corupt
    if (checksum_r != mesaj.checksum)
    {
      //sterg mesajul din fereastra, pentru a-l adauga la final
      packets.erase(packets.begin() + bgn);
      if ((bgn + wnd) < msg_count)
      {
        packets.insert(packets.begin() + (bgn + wnd - 1), expected_message);
      }
      else
      {
        packets.insert(packets.begin() + (msg_count - 1), expected_message);
      }

      //trimit NAK
      mesaj.akk = 'N';
      mesaj.sequence_number = expected_message;
    }

    else
    {
      //mesajul nu este corupt, dar nu este cel pe care il asteptam
      if (mesaj.sequence_number != expected_message)
      {
        //daca mesajul nu a fost primit deja
        if (mesaje[mesaj.sequence_number].size == 0)
        {
          mesaje[mesaj.sequence_number] = mesaj;

          //elimin din fereastra pachetul primit
          for (j = bgn; j < packets.size(); j++)
          {
            if (packets[j] == mesaj.sequence_number)
            {
              break;
            }
          }
          packets.erase(packets.begin() + j);
          packets.insert(packets.begin(), mesaj.sequence_number);
          bgn++;

          //asemanator cu cazul cu mesajul corupt, elimin mesajul asteptat
          //din fereastra, pentru a-l insera la final
          packets.erase(packets.begin() + bgn);
          if ((bgn + wnd) < msg_count)
          {
            packets.insert(packets.begin() + (bgn + wnd - 1), expected_message);
          }
          else
          {
            packets.insert(packets.begin() + (msg_count - 1), expected_message);
          }

          nr_msg++;
        }

        //daca mesajul e duplicat
        else
        {
          //elimin mesajul asteptat din fereastra, pentru a-l insera la final
          packets.erase(packets.begin() + bgn);
          if ((bgn + wnd) < msg_count)
          {
            packets.insert(packets.begin() + (bgn + wnd - 1), expected_message);
          }
          else
          {
            packets.insert(packets.begin() + (msg_count - 1), expected_message);
          }
        }

        //vreau sa trimit NAK
        mesaj.akk = 'N';
        mesaj.sequence_number = expected_message;
      }

      //mesajul nu e corupt si este exact mesajul pe care il asteptam
      else
      {
        //trimit ACK
        mesaj.akk = 'A';
        if (mesaje[mesaj.sequence_number].size == 0)
        {
          mesaje[mesaj.sequence_number] = mesaj;
          bgn++;
          nr_msg++;
        }
      } 
    }

    //verific daca am receptionat deja toate mesajele
    if (bgn == msg_count)
    {
      break;
    }
    
    //actualizez mesajul dorit si completez payload-ul cu datele din mesaj
    expected_message = packets[bgn];
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

  it = mesaje.begin();

  //construiesc numele fisierului de iesire
  string receive_name = "recv_";
  receive_name += ((cs)(*it)).data;
  file = open(receive_name.c_str(), O_WRONLY | O_CREAT, 0644);

  it++;

  //incepand cu mesajul 1 (mesajele propriu-zise), le scriu in fisierul de iesire
  for (; it != mesaje.end(); it++)
  {
    if ((write_check = write(file, ((cs)(*it)).data, ((cs)(*it)).size)) < 0){
      perror("Write error!");
      return -1;
    }
  }

  //inchid fisierul de iesire
  close(file);

  //trimit sender-ului un mesaj ca s-a terminat scrierea in fisier
  memset(t.payload, 0, sizeof(t.payload));
  sprintf(t.payload, "%s", "DONE");
  t.len = MSGSIZE;
  send_message(&t);

  return 0;
}