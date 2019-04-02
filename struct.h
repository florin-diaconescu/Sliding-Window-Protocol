#ifndef STRUCT
#define STRUCT

#define PAYLOADSIZE 1394

typedef struct{
  char data[1394];
  char checksum;
  char akk;
  int sequence_number;
}cs;

#endif