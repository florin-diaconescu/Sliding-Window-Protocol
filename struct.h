#ifndef STRUCT
#define STRUCT

#define PAYLOADSIZE 1394

typedef struct cs{
  char data[1394];
  char checksum;
  char akk;
  short sequence_number;
  short size;

  bool operator<(const cs &csObj) const{
    return sequence_number < csObj.sequence_number;
  }
}cs;

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