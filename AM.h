#ifndef AM_H_
#define AM_H_

int AM_errno;

#define INTEGER 'i'
#define FLOAT 'f'
#define STRING 'c'

#define MAXFILES 20
#define MAXSCANS 20
#define BLOCK_SIZE 1024

/* Error codes */
#define AME_OK 0
#define AME_EOF -1
#define AM_EXISTING_FILE -100
#define AM_CANNOT_CREATE_FILE -101
#define AM_CANNOT_OPEN_FILE -102
#define AM_CANNOT_ALLOCATE_BLOCK -103
#define AM_CANNOT_READ_BLOCK -104
#define AM_CANNOT_WRITE_BLOCK -105
#define AM_CANNOT_CLOSE_FILE -106
#define AM_REMAINING_OPENINGS -107
#define AM_CANNOT_DESTROY_FILE -108
#define AM_FULL_BLOCK -110
#define AM_CORRUPTED_BLOCK -109
#define AM_CANNOT_GET_BLOCK_COUNTER -113
#define AM_OPEN_SCAN -114
#define AM_SCAN_TABLE_IS_FULL -111
#define AM_NOT_COMPLETED_SCAN -112

#define EQUAL 1
#define NOT_EQUAL 2
#define LESS_THAN 3
#define GREATER_THAN 4
#define LESS_THAN_OR_EQUAL 5
#define GREATER_THAN_OR_EQUAL 6

void AM_Init( void );

void PrintFile(int); ///Testing function


int AM_CreateIndex(
  char *fileName,
  char attrType1, /* first attribute type: 'c' (string), 'i' (integer), 'f' (float) */
  int attrLength1, /* first attribute lenght: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
  char attrType2, /* τύπος πρώτου πεδίου: 'c' (συμβολοσειρά), 'i' (ακέραιος), 'f' (πραγματικός) */
  int attrLength2 /* μήκος δεύτερου πεδίου: 4 γιά 'i' ή 'f', 1-255 γιά 'c' */
);


int AM_DestroyIndex(char *fileName);

int AM_OpenIndex (char *fileName); //Returns 'fileDesc'. A small int that is associated with that opened index.

int AM_CloseIndex (int fileDesc);

int AM_InsertEntry(
  int fileDesc,
  void *value1, /* value of key-attribute */
  void *value2 /* value of second attribute */
);

int AM_OpenIndexScan(
  int fileDesc,
  int op, /* operation code */
  void *value /* value of key attribute */
);
//Return 'scanDesc'. A small int that is associated with that opened Scan.

void *AM_FindNextEntry(int scanDesc);


int AM_CloseIndexScan(int scanDesc);


void AM_PrintError(
  char *errString /* text to be printed */
);


#endif /* AM_H_ */
