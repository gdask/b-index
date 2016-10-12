#Project Description
###Contributors: Giorgos Daskalopoulos, Panagiotis Mavridis.

This project uses the BF library. BF stands for 'Block File'. BF library provides a way to store data into fixed-size blocks. Usually libraries like BF, are used from Databases and not for general Input/Output. <br>

Aim of the project is building a static library for indexing, using B+ Tree schemes. <br>

###AM library interface (Implemented by Panagiotis Mavridis):
  * AM_Init() : Initialization of global variables.
  * AM_CreateIndex(...) : Creation of a new index (Block-File).
  * AM_DestroyIndex(...) : Deletion of an existing index (Block-File).
  * AM_OpenIndex(...) : Opens a block file, reads header info, setting global variables properly in order to access data.
  * AM_CloseIndex(...) : Deletes any dynamically allocated data are associated with that file and close Block-File.
  * AM_InsertEntry(...) : Inserts entry at index.
  * AM_OpenIndexScan(...) : Setting up a new Scan into index.
  * AM_FindNextEntry(...) : Calling that function for the first time of Scan,returns the first tuple that validates the scanning 'query'. Every time that AM_FindNextEntry function is called,returns the next tuple. If there are no more tuples returns null.
  * AM_CloseIndexScan(...) : Deletes any dynamically allocated data are associated with that scan.
  * AM_PrintError(...) : Prints the latest error that occurred. <br>

*All above functions' arguments are explained in AM.h header*

###AM library inner functions (Implemented by Giorgos Daskalopoulos):
  * keycmp(key1*,key2*) : Compares two key values based on their type (string,int,float).
  * find_pointer(tree_block*,key*) : Iterates tree block to find id of next tree or data block that may contains key value.
  * new_entry(block*,entry*) :  Inserts entry into data or tree block.Returns proper error code if block is full.
  * split(block1*,block2*) : Given a full block1 and an uninitialized block2, transfers 1/2 of records to second block.
  * insert_rec(block_id,entry*,empty_temp_entry*) : Traverse recursivly tree, store data at proper data block, while returning to previous recursive calls, adds new entries at tree blocks if it is necessary.
  * search_rec(block_id,key*) : Traverse recursivly tree , returns block_id number that may contain entries with attribute1=key.
  * PrintFile() : Prints all tuples that there are stored into that file. Implemented mostly for debugging.

###Current implementation limitations:
  * B+ Tree doesn't support overfloat buckets. Inserting many identical records may lead to unexpected behavior.
  * Index is not thread safe.
  * Number of max concurrent scans is set at header, before compilation.
  * Maximum amount of indexes that library can handle is set at header, before compilation.
  * Entries have only two attributes.
  * String attributes are stored as fixed-size char arrays.
