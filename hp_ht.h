#ifndef HP_HT_H_
#define HP_HT_H_
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "BF.h"
#include <stdbool.h>

typedef struct{  

	int id;
	char name[15];
	char surname[25];
	char address[50];

}Record;



typedef struct {

	int filedesc;  
	char attrType; 
	char * attrName;
	int attrLength;

}HP_info;



typedef struct {

	int filedesc;  
	char attrType; 
	char * attrName;
	int attrLength;
	int numBuckets;
	int buckets_per_block;  
	int offset_to_buckets; 
	int total_bucket_blocks; 

}HT_info;


//HP
int HP_CreateFile(char * fileName, char attrType, char * attrName, int attrLength); 
HP_info * HP_OpenFile( char * fileName );  
int HP_CloseFile( HP_info * header_info ); 
int HP_InsertEntry(HP_info header_info, Record record );  
int HP_DeleteEntry( HP_info header_info,void * value );
int HP_GetAllEntries( HP_info header_info, void * value );


//HT
int HT_CreateIndex(char * fileName, char attrType, char * attrName, int attrLength, int buckets); 
HT_info * HT_OpenIndex( char * fileName );    
int HT_CloseIndex( HT_info * header_info );  
int HT_InsertEntry(HT_info header_info, Record record );  
int HT_DeleteEntry( HT_info header_info,void * value );
int HT_GetAllEntries( HT_info header_info, void * value ); 
int HashStatistics( char * filename );

#endif