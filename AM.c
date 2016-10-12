# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "BF_lib/BF.h"
# include "AM.h"


extern int AM_errno;

struct first_block_info{
        int attr1_size;
	      char type1;
        int attr2_size;
        char type2;
        int Record_size;
        int key_plus_pointer_size;
        int root;
};

struct data_block_info{
        char kind;
        int records; // #Records inside a Data Block
        int next_data_block; };

struct tree_block_info{
        char kind;
        int records; // #Pairs(key,pointer) inside a Tree Block
        int next_tree_block;
        int smaller_keys; };

int keycmp(int,void*,void*); //Arguments: fileDesc, key1 pointer, key2 pointer. Returns <0 if key1<key2 ,0 if key1=key2 , >0 if key1>key2.
int find_pointer(int,void*,void*);
int new_entry(int,void*,void*);
int split(int,int,void*,int,void*,void*);
int insert_rec(int,int,void*,void*);
int search(int,void*);
void printblock(int fileDesc,int blocknum);
void PrintFile(int fileDesc);

int OpenFilesTable [MAXFILES];//Stores fileDescriptors of files from BF

int OpenScansTable [MAXSCANS][5];
  //[i][0] OpenFilesTable row
	//[i][1] Operation Code [1-6]
	//[i][2] Block Number that Search into tree returned
	//[i][3] # of readed recoreds into current block
	//[i][4] Current Scanning data block

char * OpenFileNames[MAXFILES];//Names of open files. Synced update with OpenFilesTable

void * ValuesTable[MAXSCANS]; //Pointer to values for each scan. Synced update with OpenScans Table

struct first_block_info * OpenFilesInfo[MAXFILES];//Pointer to 'first_block_info' for each file. Synced update with OpenFilesTable



void AM_Init()
{
	BF_Init();

	int i,k,l;

	for(i=0;i<MAXFILES;i++){
		OpenFileNames[i]=NULL;
		OpenFilesInfo[i]=NULL;
		ValuesTable[i]=NULL;
		OpenFilesTable[i]=-1;
	}
	for(k=0;k<MAXSCANS;k++){
		for(l=0;l<5;l++)
			OpenScansTable[k][l]=-1;
	}
}

int AM_CreateIndex(char *fileName,char attrType1,int attrLength1,char attrType2,int attrLength2)
{
	FILE *file;
	if (file = fopen(fileName,"r")){
    fclose(file);
		AM_errno=AM_EXISTING_FILE;
  	return AM_EXISTING_FILE;
  }

  if(attrType1=='i' && attrLength1!=sizeof(int)){
		printf("\nNOT COMPATIBLE SIZES FOR STANDARD TYPES...!!!\n");
    AM_errno=AM_CANNOT_CREATE_FILE;
    return AM_CANNOT_CREATE_FILE;
	}
	else if(attrType1=='f' && attrLength1!=sizeof(int)){
    printf("\nNOT COMPATIBLE SIZES FOR STANDARD TYPES...!!!\n");
    AM_errno=AM_CANNOT_CREATE_FILE;
    return AM_CANNOT_CREATE_FILE;
	}
	else if(attrType2=='i' && attrLength2!=sizeof(int)){
    printf("\nNOT COMPATIBLE SIZES FOR STANDARD TYPES...!!!\n");
    AM_errno=AM_CANNOT_CREATE_FILE;
    return AM_CANNOT_CREATE_FILE;
	}
	else if(attrType2=='f' && attrLength2!=sizeof(float)){
    printf("\nNOT COMPATIBLE SIZES FOR STANDARD TYPES...!!!\n");
    AM_errno=AM_CANNOT_CREATE_FILE;
    return AM_CANNOT_CREATE_FILE;
	}

	if(BF_CreateFile(fileName)!=0){
		BF_PrintError("\nERROR IN CREATING THE BLOCK FILE\n");
		AM_errno=AM_CANNOT_CREATE_FILE;
		return AM_CANNOT_CREATE_FILE;
	}

	struct first_block_info fbinf;

	fbinf.attr1_size=attrLength1;
	fbinf.type1=attrType1;
	fbinf.attr2_size=attrLength2;
	fbinf.type2=attrType2;
	fbinf.Record_size = attrLength1 + attrLength2;
	fbinf.key_plus_pointer_size = attrLength1 + sizeof(int);//because with pointer we mean block number

	int fileDesc;
	fileDesc=BF_OpenFile(fileName);

	if(fileDesc<0){
		BF_PrintError("ERROR IN OPENING THE FILE TO INSERT THE INFO BLOCK !!!\n");
		AM_errno=AM_CANNOT_OPEN_FILE;
		return AM_CANNOT_OPEN_FILE;
	}

////////////////Allocating the information block of the file/////////////////////

	if(BF_AllocateBlock(fileDesc)<0){
    BF_PrintError("ERROR IN ALLOCATING THE FIRST BLOCK OF THE FILE !!!\n");
		AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
    return AM_CANNOT_ALLOCATE_BLOCK;
	}

	void *block;
	if(BF_ReadBlock(fileDesc,0,&block)<0){
		BF_PrintError("ERROR IN READING THE FIRST BLOCK OF THE FILE !!!\n");
		AM_errno=AM_CANNOT_READ_BLOCK;
		return AM_CANNOT_READ_BLOCK;
	}

////////////Allocating the first tree node///////////////////////////////////

  if(BF_AllocateBlock(fileDesc)<0){
    BF_PrintError("ERROR IN ALLOCATING THE FIRST BLOCK OF THE FILE !!!\n");
    AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
    return AM_CANNOT_ALLOCATE_BLOCK;
  }

  void *block2;
  if(BF_ReadBlock(fileDesc,1,&block2)<0){
    BF_PrintError("ERROR IN READING THE FIRST BLOCK OF THE FILE !!!\n");
    AM_errno=AM_CANNOT_READ_BLOCK;
    return AM_CANNOT_READ_BLOCK;
  }

	fbinf.root=1;
  struct tree_block_info tbinf;
  tbinf.kind=1;
  tbinf.records=0;
  tbinf.next_tree_block=-1;

////////////////Allocating the first data block////////////////////////

  if(BF_AllocateBlock(fileDesc)<0){
    BF_PrintError("ERROR IN ALLOCATING THE FIRST BLOCK OF THE FILE !!!\n");
    AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
    return AM_CANNOT_ALLOCATE_BLOCK;
  }

  void *block3;
  if(BF_ReadBlock(fileDesc,2,&block3)<0){
    BF_PrintError("ERROR IN READING THE FIRST BLOCK OF THE FILE !!!\n");
    AM_errno=AM_CANNOT_READ_BLOCK;
    return AM_CANNOT_READ_BLOCK;
  }

	tbinf.smaller_keys=BF_GetBlockCounter(fileDesc)-1;
	struct data_block_info dbinf;

	dbinf.kind=0;
	dbinf.records=0;
	dbinf.next_data_block=0;

	memcpy(block3,&dbinf,sizeof(dbinf));
	memcpy(block2,&tbinf,sizeof(tbinf));
	memcpy(block,&fbinf,sizeof(fbinf));


	if(BF_WriteBlock(fileDesc,0)<0){///writing the info block
		BF_PrintError("ERROR IN WRITING THE FIRST BLOCK IN THE FILE !!!\n");
		AM_errno=AM_CANNOT_WRITE_BLOCK;
		return AM_CANNOT_WRITE_BLOCK;
	}
  if(BF_WriteBlock(fileDesc,1)<0){//writing the tree block
    BF_PrintError("ERROR IN WRITING THE FIRST BLOCK IN THE FILE !!!\n");
    AM_errno=AM_CANNOT_WRITE_BLOCK;
    return AM_CANNOT_WRITE_BLOCK;
  }

  if(BF_WriteBlock(fileDesc,2)<0){//writing the data block
    BF_PrintError("ERROR IN WRITING THE FIRST BLOCK IN THE FILE !!!\n");
    AM_errno=AM_CANNOT_WRITE_BLOCK;
    return AM_CANNOT_WRITE_BLOCK;
}

	if(BF_CloseFile(fileDesc)<0){
		BF_PrintError("ERROR IN CLOSING THE BLOCK FILE !!!\n");
		AM_errno=AM_CANNOT_CLOSE_FILE;
		return AM_CANNOT_CLOSE_FILE;
	}

	return AME_OK;
}

int AM_DestroyIndex(char *fileName)
{
	int is_open=0;
	int i;

	for(i=0;i<MAXFILES;i++){
		if(OpenFileNames[i]!=NULL && strcmp(OpenFileNames[i],fileName)==0){
			is_open=1;
			AM_errno=AM_REMAINING_OPENINGS;
			return AM_REMAINING_OPENINGS;
		}
	}

	if(is_open==0){
		if(remove(fileName)==0){
			printf("ALL OK IN DESTROYING THE FILE \"%s\" !!!\n",fileName);
			return AME_OK;
		}
		else{
			AM_errno=AM_CANNOT_DESTROY_FILE;
			return AM_CANNOT_DESTROY_FILE;
		}
	}
}


int AM_OpenIndex(char *fileName)
{
  int fileDesc;
	int i;

  fileDesc=BF_OpenFile(fileName);
  if(fileDesc<0){
    BF_PrintError("ERROR IN OPENING THE FILE !!!\n");
    AM_errno=AM_CANNOT_OPEN_FILE;
    return AM_CANNOT_OPEN_FILE;
  }

	void *block;
  if(BF_ReadBlock(fileDesc,0,&block)<0){
    BF_PrintError("ERROR IN READING THE FIRST BLOCK OF THE FILE !!!\n");
    AM_errno=AM_CANNOT_READ_BLOCK;
    return AM_CANNOT_READ_BLOCK;
  }
	int tb;
	for(i=0;i<MAXFILES;i++){
		if(OpenFilesTable[i]==-1){
			tb=i;
			OpenFilesTable[i]=fileDesc;
			OpenFileNames[i]=malloc(sizeof(char[strlen(fileName)]));
			strcpy(OpenFileNames[i],fileName);
			OpenFilesInfo[i]=malloc(sizeof(struct first_block_info));
		  memcpy(OpenFilesInfo[i],block,sizeof(struct first_block_info));

		  if(BF_WriteBlock(fileDesc,0)<0){
		    BF_PrintError("ERROR IN WRITING THE FIRST BLOCK IN THE FILE !!!\n");
        AM_errno=AM_CANNOT_WRITE_BLOCK;
        return AM_CANNOT_WRITE_BLOCK;
      }
			return tb;
		}
	}
}

int AM_CloseIndex(int fileDesc)
{
	int open_scan_exists=0;
	int i;

	for(i=0;i<MAXSCANS;i++){
    if(OpenScansTable[i][0]==fileDesc){
      open_scan_exists=1;
		  break;
    }
	}

	if(open_scan_exists==0){
		if(OpenFilesTable[fileDesc]!=-1){
			if(BF_CloseFile(OpenFilesTable[fileDesc])<0){
				BF_PrintError("ERROR IN CLOSING THE FILE !!!\n");
				AM_errno=AM_CANNOT_CLOSE_FILE;
				return AM_CANNOT_CLOSE_FILE;
			}
			OpenFilesTable[fileDesc]=OpenFilesTable[fileDesc]=-1;
			free(OpenFileNames[fileDesc]);
			OpenFileNames[fileDesc]=NULL;
			free(OpenFilesInfo[fileDesc]);
			OpenFilesInfo[fileDesc]=NULL;
			return AME_OK;
		}
	}
	else{
		AM_errno=AM_OPEN_SCAN;
		return AM_OPEN_SCAN;
	}
}



int AM_OpenIndexScan(int fileDesc,int op,void *value)
{
	int i,table_position;
	int full=0;//Checking if OpenScansTable is full
	for(i=0;i<MAXSCANS;i++){
		if(OpenScansTable[i][0]==-1){
			table_position=i;
			OpenScansTable[i][0]=fileDesc;//file descriptor
			OpenScansTable[i][1]=op;//operation to be done
			OpenScansTable[i][3]=0;//number of records that have been read
			ValuesTable[i]=value;
			full=1;
			break;
		}
	}

	if(full==0){
		AM_errno=AM_SCAN_TABLE_IS_FULL;
		return AM_SCAN_TABLE_IS_FULL;
	}

	int block_number_from_search;
	block_number_from_search=search(fileDesc,value);
	if(block_number_from_search<0){
		printf("SEARCH FAILED !");
		return block_number_from_search;
	}
	OpenScansTable[table_position][2]=block_number_from_search;//number of block that the search returns
	OpenScansTable[table_position][4]=block_number_from_search;//number of block to be readen each time from find next entry

	return table_position;
}

void *AM_FindNextEntry(int scanDesc)
{
	void *block;
	void *k;//for the key

	struct data_block_info *v;
	int key_size,record_size;
	key_size=(OpenFilesInfo[OpenScansTable[scanDesc][0]])->attr1_size;
	record_size=(OpenFilesInfo[OpenScansTable[scanDesc][0]])->Record_size;

	switch(OpenScansTable[scanDesc][1]){
	static void *dbinf;
	static void *dbinf2;
	static void *dbinf3;
	static void *dbinf4;
	static void *dbinf5;
	static void *dbinf6;

	static int first_time=0;
  static int first_time2=0;
  static int first_time3=0;
  static int first_time4=0;
  static int first_time5=0;
  static int first_time6=0;

	case 1:
    if(OpenScansTable[scanDesc][3]==0 && first_time==0 && OpenScansTable[scanDesc][2]==OpenScansTable[scanDesc][4]){  //for the first call
	    OpenScansTable[scanDesc][4]=OpenScansTable[scanDesc][2];
			first_time=1;
		}

		if(OpenScansTable[scanDesc][4]!=0){//only for the first time if no recors have been read
	    if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 1 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
			dbinf=block;
		  v=dbinf;
    }
    else{
      OpenScansTable[scanDesc][3]=0;//the scan has ended
			OpenScansTable[scanDesc][4]=0;
			first_time=0;
      AM_errno=AME_EOF;
      return NULL;
    }

		block = dbinf + sizeof(struct data_block_info);
		k=malloc(key_size);
		memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);

		if(keycmp(OpenScansTable[scanDesc][0],ValuesTable[scanDesc],k)==0){
			block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
			OpenScansTable[scanDesc][3]++;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
			free(k);
			return block;
		}
		OpenScansTable[scanDesc][3]++;
    if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
    AM_FindNextEntry(scanDesc);
		break;

  case 2:
    if(OpenScansTable[scanDesc][3]==0 && first_time2==0){//onny for the first call
      OpenScansTable[scanDesc][4]=2;//the first data block in the file that we created from the beggining
			first_time2=1;
		}
    if(OpenScansTable[scanDesc][4]!=0){
      if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 2 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
      dbinf2=block;
		  v=dbinf2;
    }
    else{
      //printf("I ARRIVED IN THE END OF THE FILE MY FRIEND !!\n");
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=0;
			first_time2=0;
      AM_errno=AME_EOF;
      return NULL;
    }

    block=dbinf2+sizeof(struct data_block_info);
		k=malloc(key_size);
    memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);
    if(keycmp(OpenScansTable[scanDesc][0],ValuesTable[scanDesc],k)!=0){
      block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
      OpenScansTable[scanDesc][3]++;
		  if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
				OpenScansTable[scanDesc][4]=v->next_data_block;
      }
			free(k);
			return block;
    }
    else{
      OpenScansTable[scanDesc][3]++;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
			AM_FindNextEntry(scanDesc);
		}
		break;
	case 3:
    if(OpenScansTable[scanDesc][3]==0 && first_time3==0){//only for the first call
      OpenScansTable[scanDesc][4]=2;//the first data block in the file that we created from the beggining
      first_time3=1;
    }
    if(OpenScansTable[scanDesc][4]!=0){
      if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 3 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
      dbinf3=block;
	    v=dbinf3;
    }
    else{
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=0;
      first_time3=0;
      AM_errno=AME_EOF;
      return NULL;
    }
    if(OpenScansTable[scanDesc][3]==v->records){
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=v->next_data_block;
    }
    block=dbinf3+sizeof(struct data_block_info);
    k=malloc(key_size);
    memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);
    if(keycmp(OpenScansTable[scanDesc][0],k,ValuesTable[scanDesc]) < 0){
      block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
      OpenScansTable[scanDesc][3]++;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
			free(k);
      return block;
    }
    else{
      OpenScansTable[scanDesc][3]++;
			AM_FindNextEntry(scanDesc);
    }
		break;
	case 4:
    if(OpenScansTable[scanDesc][3]==0 && first_time4==0){//only for the first call
      OpenScansTable[scanDesc][4]=OpenScansTable[scanDesc][2];//the first data block in the file that we created from the beggining
      first_time4=1;
    }
    if(OpenScansTable[scanDesc][4]!=0){
      if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 4 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
      dbinf4=block;
      v=dbinf4;
    }
    else{
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=0;
      first_time4=0;
      AM_errno=AME_EOF;
      return NULL;
    }
    block=dbinf4+sizeof(struct data_block_info);
    k=malloc(key_size);
    memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);
    if(keycmp(OpenScansTable[scanDesc][0],k,ValuesTable[scanDesc]) > 0){
      block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
      OpenScansTable[scanDesc][3]++;
      free(k);
			return block;
    }
    else{
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
      OpenScansTable[scanDesc][3]++;
		  AM_FindNextEntry(scanDesc);
    }
		break;
	case 5:
    if(OpenScansTable[scanDesc][3]==0 && first_time5==0){//only for the first call
      OpenScansTable[scanDesc][4]=2;//the first data block in the file that we created from the beggining
      first_time5=1;
    }
    if(OpenScansTable[scanDesc][4]!=0){
      if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 5 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
      dbinf5=block;
      v=dbinf5;
    }
    else {
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=0;
      first_time5=0;
      AM_errno=AME_EOF;
      return NULL;
    }
    block=dbinf5+sizeof(struct data_block_info);
    k=malloc(key_size);
    memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);
    if(keycmp(OpenScansTable[scanDesc][0],k,ValuesTable[scanDesc]) <= 0){
      block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
      OpenScansTable[scanDesc][3]++;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
			free(k);
      return block;
    }
    else{
      OpenScansTable[scanDesc][3]++;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
      AM_FindNextEntry(scanDesc);
    }
		break;
	case 6:
    if(OpenScansTable[scanDesc][3]==0 && first_time6==0){//only for the first call
      OpenScansTable[scanDesc][4]=OpenScansTable[scanDesc][2];//the first data block in the file that we created from the beggining
      first_time6=1;
    }
    if(OpenScansTable[scanDesc][4]!=0){
      if(BF_ReadBlock(OpenFilesTable[OpenScansTable[scanDesc][0]],OpenScansTable[scanDesc][4],&block)<0){
        BF_PrintError("ERROR IN READING BLOCK OF THE FILE IN FIND NEXT ENTRY CASE 6 !!!\n");
        AM_errno=AM_CANNOT_READ_BLOCK;
        return NULL;
      }
      dbinf6=block;
      v=dbinf6;
    }
    else{
      OpenScansTable[scanDesc][3]=0;
      OpenScansTable[scanDesc][4]=0;
      first_time6=0;
      AM_errno=AME_EOF;
      return NULL;
    }
    block=dbinf6+sizeof(struct data_block_info);
    k=malloc(key_size);
    memcpy(k,block+(OpenScansTable[scanDesc][3])*record_size,key_size);
    if(keycmp(OpenScansTable[scanDesc][0],k,ValuesTable[scanDesc]) >= 0){
      block=block+(OpenScansTable[scanDesc][3])*record_size+key_size;
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
      OpenScansTable[scanDesc][3]++;
      free(k);
		  return block;
    }
    else{
      if(OpenScansTable[scanDesc][3]==v->records){
        OpenScansTable[scanDesc][3]=0;
        OpenScansTable[scanDesc][4]=v->next_data_block;
      }
      OpenScansTable[scanDesc][3]++;
		  AM_FindNextEntry(scanDesc);
    }
		break;
	}
}

int AM_CloseIndexScan(int scanDesc){
		int i;
		for(i=0;i<5;i++){
			OpenScansTable[scanDesc][i]=-1;
		}
		ValuesTable[scanDesc]=NULL;//!!!!!!!!!!!!!!!!!!!!!!!!!
		return AME_OK;
}


void AM_PrintError(char *errString)
{
	printf("%s\n",errString);

	switch (AM_errno){

	case -100: printf("\nAM_ERROR : AM_EXISTING_FILE\n");
		break;
	case -101: printf("\nAM_ERROR : AM_CANNOT_CREATE_FILE\n");
		break;
	case -102 :printf("\nAM_ERROR : AM_CANNOT_OPEN_FILE\n");
		break;
	case -103: printf("\nAM_ERROR : AM_CANNOT_ALLOCATE_BLOCK\n");
		break;
	case -104: printf("\nAM_ERROR : AM_CANNOT_READ_BLOCK\n");
		break;
	case -105: printf("\nAM_ERROR : AM_CANNOT_WRITE_BLOCK\n");
		break;
	case -106: printf("\nAM_ERROR : AM_CANNOT_CLOSE_FILE\n");
		break;
	case -107: printf("\nAM_ERROR : AM_REMAINING_OPENINGS\n");
		break;
	case -108: printf("\nAM_ERROR : AM_CANNOT_DESTROY_FILE\n");
		break;
  case -109: printf("\nAM_ERROR : AM_CORRUPTED_BLOCK\n");
    break;
  case -110: printf("\nAM_ERROR : AM_BLOCK_FULL\n");
    break;
  case -111: printf("\nAM_ERROR : AM_SCAN_TABLE_IS_FULL\n");
    break;
	case -112: printf("\nAM_ERROR : AM_NOT_COMPLETED_SCAN\n");
    break;
	case -113: printf("\nAM_ERROR : AM_NOT_CANNOT_GET_BLOCK_COUNTER\n");
		break;
	case -114: printf("\nAM_ERROR : AM_OPEN_SCAN\n");
		break;
  case -115: printf("\nAM_ERROR : AM_FULL_BLOCK\n");
    break;
	}
}

int keycmp(int fileDesc,void *key1,void *key2){
	char type=OpenFilesInfo[fileDesc]->type1;

	if(type=='i'){ //KEY IS INT
		int *val1,*val2;
		val1=(int*)key1;
		val2=(int*)key2;
		if(*val1 < *val2)
			return -1;
		if(*val1 == *val2)
			return 0;
		return 1;
	}
	if(type == 'f'){ //KEY IS FLOAT
		float *val1,*val2;
		val1=(float*)key1;
		val2=(float*)key2;
		if(*val1 < *val2)
			return -1;
		if(*val1 == *val2)
			return 0;
		return 1;
	}
	return strcmp(key1,key2); //KEY IS STRING
}

int find_pointer(int fileDesc,void *block,void *entry){
	int *result;
	struct tree_block_info *block_info;
	void *iterator, *front;
	size_t offset,first_entry_lenght;

	first_entry_lenght=OpenFilesInfo[fileDesc]->attr1_size;


	block_info=block;
	if(block_info->kind==1){//Tree Block
		if(block_info->records==0) return block_info->smaller_keys; //Empty Index
		offset=first_entry_lenght + sizeof(int);
		iterator = block + sizeof(struct tree_block_info);
		front = offset*block_info->records + iterator;
		while(iterator < front){
			if(keycmp(fileDesc,entry,iterator)<0){
				result=iterator-sizeof(int);
				return *result;
			}
			iterator=iterator+offset;
		}
		result=iterator-sizeof(int);
		return *result;
	}
	else{
		AM_errno=AM_CORRUPTED_BLOCK;
		return AM_CORRUPTED_BLOCK;
	}
}

int new_entry(int fileDesc,void *block,void *record){ //After using new_entry we have to call BF_WRITEBLOCK
	int  max_records , i;
	char *kind = (char*)block;
	void *iterator;
	struct tree_block_info *info = block;
	size_t offset,attr1_size,record_size;

	attr1_size=OpenFilesInfo[fileDesc]->attr1_size;
	record_size=OpenFilesInfo[fileDesc]->Record_size;

	if(*kind==0){ //Data Block
		max_records=((size_t)BLOCK_SIZE - sizeof(struct data_block_info))/record_size;
		iterator=block + sizeof(struct data_block_info);
		offset=record_size;
	}
	else if(*kind==1){ //Tree Block
		max_records=((size_t)BLOCK_SIZE - sizeof(struct tree_block_info))/(attr1_size + sizeof(int));
		iterator=block + sizeof(struct tree_block_info);
		offset=attr1_size + sizeof(int);
	}
	else{
		AM_errno=AM_CORRUPTED_BLOCK;
		return AM_CORRUPTED_BLOCK;
	}
	if(info->records == max_records) return AM_FULL_BLOCK;

	for(i=0;i < info->records;i++){//i=#records that stay at current block
		if(keycmp(fileDesc,iterator,record)>=0) break;
		iterator=iterator+offset;
	}
	memmove(iterator+offset,iterator,(info->records - i)*offset);
	memmove(iterator,record,offset);
	info->records++;
	return AME_OK;
}

int split(int fileDesc,int block1num,void *block1,int block2num ,void *block2,void *min_value_block_2){
	int first_entry_length,second_entry_lenght;
	int copysize,*smaller_keys;
	char *kind;
	void *source,*dest;
	size_t key_plus_int_size,record_size;

	first_entry_length=OpenFilesInfo[fileDesc]->attr1_size;
	second_entry_lenght=OpenFilesInfo[fileDesc]->attr2_size;
	key_plus_int_size=OpenFilesInfo[fileDesc]->key_plus_pointer_size;
	record_size=OpenFilesInfo[fileDesc]->Record_size;

	kind=block1;
	if(*kind==0){ //Data Block
		struct data_block_info *block1_info = block1;
		struct data_block_info *block2_info = block2;


		block2_info->kind=*kind;
		block2_info->records=(block1_info->records)/2;
		block1_info->records=(block1_info->records)-(block2_info->records);

		block2_info->next_data_block=block1_info->next_data_block;
		block1_info->next_data_block=block2num;

		source=block1 + sizeof(struct data_block_info) + (block1_info->records)*(first_entry_length + second_entry_lenght);

		while(keycmp(fileDesc,source,source-record_size) == 0 && source > block1+sizeof(struct data_block_info)){
			source=source-record_size;
			block1_info->records--;
			block2_info->records++;
		}

		dest=block2 + sizeof(struct data_block_info);
		copysize=block1 + BLOCK_SIZE - source;

		memmove(min_value_block_2,source,first_entry_length);

	}
	else if(*kind==1){ //Tree Block
		struct tree_block_info *block1_info = block1;
		struct tree_block_info *block2_info = block2;

		block2_info->kind=*kind;
		block2_info->records=(block1_info->records)/2;
		block1_info->records=(block1_info->records)-(block2_info->records);
		block2_info->records=block2_info->records - 1;

		block2_info->next_tree_block=block1_info->next_tree_block;
		block1_info->next_tree_block=block2num;


		source=block1 + sizeof(struct tree_block_info) + (block1_info->records)*(key_plus_int_size);
		memmove(min_value_block_2,source,first_entry_length);
		source=source+first_entry_length;
		smaller_keys=source;
		block2_info->smaller_keys=*smaller_keys;
		source=source+sizeof(int);

		dest=block2 + sizeof(struct tree_block_info);
		copysize=block1 + BLOCK_SIZE - source;
	}
	else {
		AM_errno=AM_CORRUPTED_BLOCK;
		return AM_CORRUPTED_BLOCK;
	}
	memmove(dest,source,copysize);
	return AME_OK;
}

int insert_rec(int fileDesc,int blocknum,void *entry,void *new_child){
	void *block,*block2;
	char *kind,*block_to_add=new_child;
	int bf_fnum,new_entry_result,split_result,block2num,insert_rec_result;
	size_t attr1_size;

	attr1_size=OpenFilesInfo[fileDesc]->attr1_size;
	bf_fnum=OpenFilesTable[fileDesc];

	if(BF_ReadBlock(bf_fnum,blocknum,&block)<0){
		BF_PrintError("CANNOT READ BLOCK\n");
		AM_errno=AM_CANNOT_READ_BLOCK;
		return AM_CANNOT_READ_BLOCK;
	}
	kind=block;
	if(*kind !=0 && *kind!=1){
		AM_errno=AM_CORRUPTED_BLOCK;
		return AM_CORRUPTED_BLOCK;
	}
	if(*kind == 0){ //DATA BLOCK
		new_entry_result=new_entry(fileDesc,block,entry);
		if(new_entry_result == AME_OK){
			*block_to_add=0;
			if(BF_WriteBlock(bf_fnum,blocknum) < 0){
				BF_PrintError("CANNOT WRITE BLOCK\n");
				AM_errno=AM_CANNOT_WRITE_BLOCK;
				return AM_CANNOT_WRITE_BLOCK;
			}
			return AME_OK;
		}
		else if(new_entry_result == AM_FULL_BLOCK){
			char min_key_block2[attr1_size];

			if(BF_AllocateBlock(bf_fnum) < 0){
				BF_PrintError("CANNOT ALLOCATE BLOCK\n");
				AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
				return AM_CANNOT_ALLOCATE_BLOCK;
			}
			block2num=BF_GetBlockCounter(bf_fnum)-1;
			if(block2num <0 ){
				BF_PrintError("CANNOT GET BLOCK COUNTER\n");
				AM_errno=AM_CANNOT_GET_BLOCK_COUNTER;
				return AM_CANNOT_GET_BLOCK_COUNTER;
			}
			if(BF_ReadBlock(bf_fnum,block2num,&block2) < 0){
				BF_PrintError("CANNOT READ BLOCK\n");
				AM_errno=AM_CANNOT_READ_BLOCK;
				return AM_CANNOT_READ_BLOCK;
			}
			split_result=split(fileDesc,blocknum,block,block2num,block2,min_key_block2);
			if(split_result != AME_OK) return split_result;
			if(keycmp(fileDesc,entry,min_key_block2) < 0){//New entry at first block
				new_entry_result=new_entry(fileDesc,block,entry);
				if(new_entry_result != AME_OK) return new_entry_result;
			}
			else{ //New entry at new block
				new_entry_result=new_entry(fileDesc,block2,entry);
				if(new_entry_result != AME_OK) return new_entry_result;
			}
			*block_to_add=1;
			memmove(new_child + 1,min_key_block2,attr1_size);
			memmove(new_child + 1 + attr1_size,&block2num,sizeof(int));
			if( BF_WriteBlock(bf_fnum,blocknum) < 0 || BF_WriteBlock(bf_fnum,block2num) < 0){
				BF_PrintError("CANNOT WRITE BLOCK\n");
				AM_errno=AM_CANNOT_WRITE_BLOCK;
				return AM_CANNOT_WRITE_BLOCK;
			}
			return AME_OK;
		}
		else{
			return new_entry_result;
		}
	}
	else {//TREE BLOCK
		insert_rec_result=insert_rec(fileDesc,find_pointer(fileDesc,block,entry),entry,new_child);
		if(insert_rec_result != AME_OK) return insert_rec_result;
		if(*block_to_add == 0) return AME_OK;
		new_entry_result=new_entry(fileDesc,block,new_child+1); //There is a pair of (key,pointer) to add at tree
		*block_to_add=0;
		if(new_entry_result == AME_OK){
			if(BF_WriteBlock(bf_fnum,blocknum) < 0){
				BF_PrintError("CANNOT WRITE BLOCK\n");
				AM_errno=AM_CANNOT_WRITE_BLOCK;
				return AM_CANNOT_WRITE_BLOCK;
			}
			return AME_OK;
		}
		if(new_entry_result != AM_FULL_BLOCK) return new_entry_result;
		//FULL BLOCK CASE
		if(BF_AllocateBlock(bf_fnum) < 0){
			BF_PrintError("CANNOT ALLOCATE BLOCK\n");
			AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
			return AM_CANNOT_ALLOCATE_BLOCK;
		}
		block2num=BF_GetBlockCounter(bf_fnum)-1;
		if(block2num <0 ){
			BF_PrintError("CANNOT GET BLOCK COUNTER\n");
			AM_errno=AM_CANNOT_GET_BLOCK_COUNTER;
			return AM_CANNOT_GET_BLOCK_COUNTER;
		}
		if(BF_ReadBlock(bf_fnum,block2num,&block2) < 0){
			BF_PrintError("CANNOT READ BLOCK\n");
			AM_errno=AM_CANNOT_READ_BLOCK;
			return AM_CANNOT_READ_BLOCK;
		}
		char min_key_block2[attr1_size];

		split_result=split(fileDesc,blocknum,block,block2num,block2,min_key_block2);
		if(split_result != AME_OK) return split_result;
		if(keycmp(fileDesc,new_child+1,min_key_block2) < 0){ //New pair belongs at first block
			new_entry_result=new_entry(fileDesc,block,new_child+1);
			if(new_entry_result != AME_OK) return new_entry_result;
		}
		else{ //New pair belongs at new block
			new_entry_result=new_entry(fileDesc,block2,new_child+1);
			if(new_entry_result != AME_OK) return new_entry_result;
		}
		*block_to_add=1; //The Previous Recursive call will insert that pair
		memmove(new_child +1,min_key_block2,attr1_size);
		memmove(new_child + 1 + attr1_size,&block2num,sizeof(int));
		if( BF_WriteBlock(bf_fnum,blocknum) < 0 || BF_WriteBlock(bf_fnum,block2num) < 0){
			BF_PrintError("CANNOT WRITE BLOCK\n");
			AM_errno=AM_CANNOT_WRITE_BLOCK;
			return AM_CANNOT_WRITE_BLOCK;
		}
		//NEW ROOT CASE
		if(blocknum==OpenFilesInfo[fileDesc]->root){
			int new_root_num;
			struct tree_block_info *new_root_info;
			void *new_root;
			if(BF_AllocateBlock(bf_fnum) < 0){
				BF_PrintError("CANNOT ALLOCATE BLOCK\n");
				AM_errno=AM_CANNOT_ALLOCATE_BLOCK;
				return AM_CANNOT_ALLOCATE_BLOCK;
			}
			new_root_num=BF_GetBlockCounter(bf_fnum)-1;
			if(new_root_num <0 ){
				BF_PrintError("CANNOT GET BLOCK COUNTER\n");
				AM_errno=AM_CANNOT_GET_BLOCK_COUNTER;
				return AM_CANNOT_GET_BLOCK_COUNTER;
			}
			if(BF_ReadBlock(bf_fnum,new_root_num,&new_root) < 0){
				BF_PrintError("CANNOT READ BLOCK\n");
				AM_errno=AM_CANNOT_READ_BLOCK;
				return AM_CANNOT_READ_BLOCK;
			}
			new_root_info=new_root;
			new_root_info->kind=1;
			new_root_info->records=0;
			new_root_info->next_tree_block=0;
			new_root_info->smaller_keys=blocknum;
			new_entry_result=new_entry(fileDesc,new_root,new_child+1);
			if(new_entry_result != AME_OK) return new_entry_result;
			//MAKE THE TREE'S ROOT-NODE POINTER POINT TO THE NEW NODE!
			OpenFilesInfo[fileDesc]->root=new_root_num;

			struct first_block_info *fbinfo;
			if(BF_ReadBlock(bf_fnum,0,(void**)&fbinfo)<0){
				BF_PrintError("CANNOT READ BLOCK\n");
				AM_errno=AM_CANNOT_READ_BLOCK;
				return AM_CANNOT_READ_BLOCK;
			}
			fbinfo->root=new_root_num;

			if(BF_WriteBlock(bf_fnum,new_root_num) < 0 || BF_WriteBlock(bf_fnum,0) < 0){
				BF_PrintError("CANNOT WRITE BLOCK\n");
				AM_errno=AM_CANNOT_WRITE_BLOCK;
				return AM_CANNOT_WRITE_BLOCK;
			}
		}
		return AME_OK;
	}
}

int AM_InsertEntry(int fileDesc,void *value1,void *value2){
	int root;
	size_t attr1_size,attr2_size,record_size,key_plus_pointer_size;

	attr1_size=OpenFilesInfo[fileDesc]->attr1_size;
	attr2_size=OpenFilesInfo[fileDesc]->attr2_size;
	record_size=OpenFilesInfo[fileDesc]->Record_size;
	key_plus_pointer_size=OpenFilesInfo[fileDesc]->key_plus_pointer_size;

	char entry[record_size];
	char new_child[key_plus_pointer_size + 1];

	memmove(entry,value1,attr1_size);
	memmove(entry+attr1_size,value2,attr2_size);
	root=OpenFilesInfo[fileDesc]->root;
	return insert_rec(fileDesc,root,entry,new_child);
}

int search_rec(int fileDesc,int blocknum,void *value){
	void *block;
	char *kind;
	int bf_fnum;

	bf_fnum=OpenFilesTable[fileDesc];
	if(BF_ReadBlock(bf_fnum,blocknum,&block) < 0){
		BF_PrintError("CANNOT READ BLOCK\n");
		AM_errno=AM_CANNOT_READ_BLOCK;
		return AM_CANNOT_READ_BLOCK;
	}
	kind=block;
	if(*kind==0){
		return blocknum;
	}
	else{
		return search_rec(fileDesc,find_pointer(fileDesc,block,value),value);
	}
}

int search(int fileDesc,void *value){
	int root;

	root=OpenFilesInfo[fileDesc]->root;
	return search_rec(fileDesc,root,value);
}

void PrintFile(int fileDesc){
	size_t attr1_size,record_size,key_plus_pointer_size;
	void *attr1,*attr2,*block;
	int bf_fnum,nblocks,i,j;
	char type1,type2,*kind;

	bf_fnum=OpenFilesTable[fileDesc];
	nblocks=BF_GetBlockCounter(bf_fnum);
	attr1_size=OpenFilesInfo[fileDesc]->attr1_size;
	record_size=OpenFilesInfo[fileDesc]->Record_size;
	key_plus_pointer_size=OpenFilesInfo[fileDesc]->key_plus_pointer_size;
	type1=OpenFilesInfo[fileDesc]->type1;
	type2=OpenFilesInfo[fileDesc]->type2;
	printf("Root At : %d\n",OpenFilesInfo[fileDesc]->root);

	for(i=1;i<nblocks;i++){
		BF_ReadBlock(bf_fnum,i,&block);
		kind=block;
		if(*kind==0){ //Data block

			struct data_block_info *info=block;

			printf("\nDATA BLOCK %d WITH %d ENTRIES,NEXT BLOCK IS %d\n",i,info->records,info->next_data_block);
			attr1=block+sizeof(struct data_block_info);
			attr2=block+sizeof(struct data_block_info)+attr1_size;
			for(j=1;j<= info->records;j++){
				if(type1=='c'){
					char *key=attr1;
					printf("Key: %s ,",key);
				}
				if(type1=='f'){
					float *key=attr1;
					printf("Key: %f ,",*key);
				}
				if(type1=='i'){
					int *key=attr1;
					printf("Key: %d ,",*key);
				}
				if(type2=='c'){
					char *cont=attr2;
					printf("Contents %s\n",cont);
				}
				if(type2=='f'){
					float *cont=attr2;
					printf("Contents %f\n",*cont);
				}
				if(type2=='i'){
					int *cont=attr2;
					printf("Contents %d\n",*cont);
				}
				attr1=attr1+record_size;
				attr2=attr2+record_size;
			}
		}
		else{ //Tree Block
			struct tree_block_info *info=block;

			printf("\nTREE BLOCK %d WITH %d ENTRIES,NEXT BLOCK %d,SMALLER KEYS AT %d BLOCK\n",i,info->records,info->next_tree_block,info->smaller_keys);
			attr1=block+sizeof(struct tree_block_info);
			attr2=block+sizeof(struct tree_block_info)+attr1_size;
			for(j=1;j<= info->records; j++){
				if(type1=='c'){
					char *key=attr1;
					printf("Keys => %s, ",key);
				}
				if(type1=='f'){
					float *key_f=(float*) attr1;
					printf("Keys => %f, ",*key_f);
				}
				if(type1=='i'){
					int *key_int=(int*) attr1;
					printf("Keys => %d, ",*key_int);
				}
				int *at_block=attr2;
				printf("At Block %d\n",*at_block);
				attr1=attr1+key_plus_pointer_size;
				attr2=attr2+key_plus_pointer_size;
			}
		}
	}
	return;
}
