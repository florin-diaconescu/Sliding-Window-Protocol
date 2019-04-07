#ifndef STRUCT
#define STRUCT

#define PAYLOADSIZE 1394

typedef struct cs{
  char data[1394]; //datele propriu-zise
  char checksum; //checksum
  char akk; //va fi 'A', pentru ACK, sau 'N', pentru NAK
  short sequence_number; //numarul de secventa al mesajului
  short size; //dimensiunea mesajului

  bool operator<(const cs &cs_obj) const{
    return sequence_number < cs_obj.sequence_number;
  }
}cs;

//functie pentru calcularea checksum-ului unui mesaj
char calculate_checksum(cs mesaj)
{
  char checksum = 0;
  int i;

  for (i = 0; i < PAYLOADSIZE; i++)
  {
    checksum ^= mesaj.data[i];
  }


  checksum ^= mesaj.akk;
  checksum ^= mesaj.sequence_number;
  checksum ^= mesaj.size; 

  return checksum;
}


#endif