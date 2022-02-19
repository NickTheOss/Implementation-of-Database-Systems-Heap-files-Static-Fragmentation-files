#include "hp_ht.h"

// definitions are private
int hash(const char * value, int buckets);
int hash_id( int id, int buckets);
int create_new_record_block(HT_info ht);
int insert_record_to_block(HT_info ht, int block_number, Record record);
int delete_record_key_id_HT(HT_info header_info,int key);
int delete_record_key_name(HT_info header_info,char * key);
int delete_record_key_surname(HT_info header_info,char * key);
int delete_record_key_address(HT_info header_info,char * key);
void print_all_entries_HT(HT_info header_info);
int transverse_block_list_key_id_HT(HT_info header_info, int key);
int transverse_block_list_key_name( HT_info header_info, char * key);
int transverse_block_list_key_surname( HT_info header_info, char * key);
int transverse_block_list_key_address( HT_info header_info, char * key);



int hash(const char * str, int buckets )
{
    unsigned long hash = 5381;
    int c;

    while (c = *str++)

        hash = ((hash << 5) + hash) + c; 

    return hash % buckets;
}



int hash_id( int key, int buckets)
{
	int c = 0x27d4eb2d; // a prime or an odd constant
  	key = (key ^ 61) ^ (key >> 16);
  	key = key + (key << 3);
 	key = key ^ (key >> 4);
  	key = key * c;
  	key = key ^ (key >> 15);
	return key % buckets;   
  
}



int HT_CreateIndex(char * filename,char attrType,char * attrName, int attrLength, int buckets){

	int filedesc, occupied_bytes, block_buckets, blocks_to_create, block_position;
	void * block;
	char * c_mem;
	int * i_mem;
	char hash[] = "HASH";  

	BF_Init(); 

	if(BF_CreateFile(filename)){

		BF_PrintError("Failed creating BF file\n");
		return -1;
	}

	if((filedesc = BF_OpenFile( filename )) < 0){

		BF_PrintError("Failed opening BF file\n");
		return -1;

	}
	
	occupied_bytes = sizeof(hash) +  sizeof(char) + sizeof(int) + (attrLength * sizeof(char) + sizeof(char)) + (3 * sizeof(int));
	block_buckets = (BLOCK_SIZE - occupied_bytes) / sizeof(int); //how many buckets fit in a single block because bucket cell = int value
	blocks_to_create = buckets / block_buckets; //how many blocks are about to be created
	
	if(blocks_to_create == 0) blocks_to_create = 1; 
	else if (buckets % block_buckets > 0) blocks_to_create++;

	for (int i = 0; i < blocks_to_create; ++i)

	{
		if(BF_AllocateBlock( filedesc )){

			BF_PrintError("Failed allocating block\n");
			return -1;

		}

		if((block_position = BF_GetBlockCounter( filedesc ) - 1) < 0){

			BF_PrintError("Failed getting block number\n");
			return -1;

		}

		if(BF_ReadBlock( filedesc, block_position , &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}

	    c_mem = (char *)block;
	    memcpy(c_mem, hash, strlen(hash) * sizeof(char) + sizeof(char));
	    c_mem += (strlen(hash) * sizeof(char) + sizeof(char));
	    *c_mem = attrType;
	    c_mem++;
	    i_mem = (int *)c_mem;
	    *i_mem = attrLength;
	    i_mem++;
	    c_mem = (char *)i_mem;
	    memcpy(c_mem, attrName, (attrLength * sizeof(char)) + sizeof(char));
	    c_mem += (attrLength * sizeof(char)+ sizeof(char));
	    i_mem = (int *)c_mem;
	    *i_mem = buckets;
	    i_mem++;

	    if( i == blocks_to_create-1) block_buckets = buckets % block_buckets;   //last block
	    *i_mem = block_buckets;
	    i_mem++;
	    *i_mem = blocks_to_create; 
	    i_mem++;

	    for (int j = 0; j < block_buckets; ++j)
	    {
	    	i_mem[j] = -1;
	    }

	    if(BF_WriteBlock( filedesc, block_position)){  

			BF_PrintError("Failed reading block\n");
			return -1;

		}

	}

	if(BF_CloseFile(filedesc)){  

		BF_PrintError("Failed closing file\n");
		return -1;

	}
	return 0;
}





HT_info * HT_OpenIndex( char * filename ){

	void * block;
	int filedesc;
	char * c_mem;
	int * i_mem;
	int attrLength;

	HT_info * ht = malloc(sizeof(HT_info));

	if((filedesc = BF_OpenFile( filename )) < 0){ 

		BF_PrintError("Failed opening BF file\n");
		return NULL;

	}
	
	ht->filedesc = filedesc;

	if(BF_ReadBlock( filedesc, 0 , &block)){

		BF_PrintError("Failed reading block\n");
		return NULL;

	}

	c_mem = (char *)block;    
	if(strcmp(c_mem, "HASH") != 0){

		printf("File is not a 'HASH' file, error occured\n");
		return NULL; // 

	} 
	c_mem += 5 * sizeof(char);
	ht->attrType = *c_mem;
	c_mem++;
	i_mem = (int*)c_mem;
	ht->attrLength = *i_mem;
	attrLength = ht->attrLength;
	i_mem++;
	c_mem = (char *)i_mem;
	ht->attrName = malloc(sizeof(char) * attrLength+ sizeof(char));
	memcpy(ht->attrName,c_mem,attrLength * sizeof(char) + sizeof(char));
	c_mem += (attrLength * sizeof(char) + sizeof(char));
	i_mem = (int *)c_mem;
	ht->numBuckets = *i_mem;
	i_mem++;
	ht->buckets_per_block = *i_mem;
	i_mem++;
	ht->total_bucket_blocks = *i_mem;
	i_mem++;
	ht->offset_to_buckets = ((void *)i_mem) - block;
	return ht;
}



int HT_CloseIndex( HT_info * header_info ){

	if(BF_CloseFile(header_info->filedesc)){

		BF_PrintError("Error closing hash file\n");
		return -1;
	}

	free(header_info->attrName);
	free(header_info);
	return 0;
}




int HT_InsertEntry(HT_info header_info, Record record ){

	void * block;
	int current_total_blocks, bucket_position, attrLength, offset_to_bucket_pointers, bucket_block, block_id;
	int * i_mem;
	char * c_mem;
	char attrType;
	char attrName[20];

	if(header_info.attrType == 'i'){

			bucket_position = hash_id(record.id, header_info.numBuckets);
	}
	else
	{
	
		if(!strcmp(header_info.attrName,"name")){

			bucket_position = hash(record.name, header_info.numBuckets);

		}

		else if(!strcmp(header_info.attrName,"surname")){

			bucket_position = hash(record.surname, header_info.numBuckets);

		}

		else if(!strcmp(header_info.attrName,"address")){

			bucket_position = hash(record.address, header_info.numBuckets);

		}

		
	}
	
	bucket_block = bucket_position / header_info.buckets_per_block; // which block contains the bucket
	bucket_position = bucket_position % header_info.buckets_per_block;
	

	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int)));
	i_mem = (int * )block;
	if(*i_mem == -1){

		if((block_id = create_new_record_block(header_info)) == -1) return -1;
		*i_mem = block_id; 
		if((block_id = insert_record_to_block( header_info, block_id, record)) == -1) return -1;
		
		if(BF_WriteBlock( header_info.filedesc, bucket_block)){

			BF_PrintError("Failed writing block\n");
			return -1;

		}
		return block_id;
	}

	if((block_id = insert_record_to_block( header_info, *i_mem, record)) == -1) return -1;

	return block_id;
}


int insert_record_to_block(HT_info ht, int block_number, Record record){

	void * block;
	int next_block_id, current_block_id, total_records_added;
	int new_block_created, total_records_capacity;
	int * i_mem;

	if(BF_ReadBlock( ht.filedesc, block_number, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}

	i_mem = (int * )block;
	next_block_id = *i_mem;   
	current_block_id = block_number; 
	
	while(next_block_id != -1){

		if(BF_ReadBlock( ht.filedesc, next_block_id , &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		current_block_id = next_block_id;
		i_mem = (int * )block;
		next_block_id = *i_mem; 


	}

	i_mem++;  
	total_records_added = *i_mem;
	i_mem++; 
	total_records_capacity = *i_mem;
	block += 3 * sizeof(int);
	block += total_records_added * sizeof(Record);
	memcpy(block, &record, sizeof(Record));
	total_records_added++;
	i_mem--;
	*i_mem = total_records_added; 

	if(total_records_added == total_records_capacity){

		next_block_id = create_new_record_block(ht);
		i_mem--;
		*i_mem = next_block_id;
	}
	if(BF_WriteBlock( ht.filedesc, current_block_id)){  

		BF_PrintError("Failed writing block\n");
		return -1;

	}

	return current_block_id;
}



int create_new_record_block(HT_info ht){

	void * block;
	int block_position;
	int * i_mem;

	if(BF_AllocateBlock( ht.filedesc )){

		BF_PrintError("Failed allocating block\n");
		return -1;

	}


	if((block_position = BF_GetBlockCounter( ht.filedesc ) - 1) < 0){

		BF_PrintError("Failed getting block number\n");
		return -1;

	}
	
	if(BF_ReadBlock( ht.filedesc, block_position , &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	i_mem = (int *)block; 
	*i_mem = -1;		
	i_mem++;
	*i_mem = 0;  
	i_mem++;
	*i_mem = (BLOCK_SIZE - 3 * sizeof(int)) / sizeof(Record);  

	if(BF_WriteBlock( ht.filedesc, block_position)){

		BF_PrintError("Failed writing1 block\n");
		return -1;

	}
	return block_position;

}




int HT_DeleteEntry( HT_info header_info,void * value ){


	int id, blocks_read;
	char name[15],surname[25],address[50];


	if(header_info.attrType == 'i'){    //id is the key
	
		id = *((int *)value);
		blocks_read = delete_record_key_id_HT(header_info,id);

	}

	if(header_info.attrType == 'c'){

		if(!strcmp(header_info.attrName,"name")){   //name is the key

			strcpy(name,(char *)value);
			blocks_read = delete_record_key_name(header_info,name);


		}

		else if(!strcmp(header_info.attrName,"surname")){  //surname is the key

			strcpy(surname,(char *)value);
			blocks_read = delete_record_key_surname(header_info,surname);
		}

		else if(!strcmp(header_info.attrName,"address")){  //address is the key

			strcpy(address,(char *)value);
			blocks_read = delete_record_key_address( header_info, address);
		}
	}

	return blocks_read;
}



int delete_record_key_id_HT(HT_info header_info, int key){

	int bucket_position, bucket_block;
	bucket_position = hash_id(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	Record * rec_mem;

	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block; 

	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int)));
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}
	current_block_id = *i_mem;

	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( key == rec_mem[i].id){

				rec_mem[i].id = -1;
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		current_block_id = next_block_id;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( key == rec_mem[i].id){

				rec_mem[i].id = -1;
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}

		}
	printf("Cannot delete record with id %d because it doesn't exist\n",key);
	return -1;

}



int delete_record_key_name(HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	Record * rec_mem;

	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block; 

	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int))); 
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}

	current_block_id = *i_mem;

	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].name)){

				strcpy(rec_mem[i].name, "NAME_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		current_block_id = next_block_id;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].name)){

				strcpy(rec_mem[i].name, "NAME_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
	printf("Cannot delete record with name %s because it doesn't exist\n",key);
	return -1;
}


int delete_record_key_surname(HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	Record * rec_mem;

	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block; 

	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int))); 
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}

	current_block_id = *i_mem;

	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].surname)){

				strcpy(rec_mem[i].name, "SURNAME_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		current_block_id = next_block_id;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].surname)){

				strcpy(rec_mem[i].name, "SURNAME_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
	printf("Cannot delete record with surname %s because it doesn't exist\n",key);
	return -1;
}


int delete_record_key_address(HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	Record * rec_mem;

	bucket_block = bucket_position / header_info.buckets_per_block;
	bucket_position = bucket_position % header_info.buckets_per_block; 

	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int))); 
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}

	current_block_id = *i_mem;

	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].address)){

				strcpy(rec_mem[i].name, "ADDRESS_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}

		current_block_id = next_block_id;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].address)){

				strcpy(rec_mem[i].name, "ADDRESS_DELETED");
				if(BF_WriteBlock( header_info.filedesc, current_block_id)){

					BF_PrintError("Failed writing block\n");
					return -1;

				}
				return 0;
			}
		}
	printf("Cannot delete record with address %s because it doesn't exist\n",key);
	return -1;
}



int transverse_block_list_key_id_HT(HT_info header_info, int key){

	int bucket_position, bucket_block;
	bucket_position = hash_id(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	int blocks_read = 0;
	Record * rec_mem;
	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block; 


	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int)));
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( key == rec_mem[i].id && rec_mem[i].id != -1){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		blocks_read++;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( key == rec_mem[i].id && rec_mem[i].id != -1){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		printf("Record with id %d doesn't exist\n",key);
		return -1;
}




int transverse_block_list_key_name( HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	int blocks_read = 0;
	Record * rec_mem;
	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block;


	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int))); 
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;

		for (int i = 0; i < total_block_records; i++)
		{
			if( (!strcmp(key, rec_mem[i].name)) && (strcmp(rec_mem[i].name,"NAME_DELETED"))){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		blocks_read++;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( !strcmp(key, rec_mem[i].name) && strcmp(rec_mem[i].name,"NAME_DELETED")){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		printf("Record with name %s doesn't exist\n",key);
		return -1;
}



int transverse_block_list_key_surname( HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	int blocks_read = 0;
	Record * rec_mem;
	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block;


	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int)));
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;

		for (int i = 0; i < total_block_records; i++)
		{
			if( (!strcmp(key, rec_mem[i].surname)) && (strcmp(rec_mem[i].surname,"SURNAME_DELETED"))){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		blocks_read++;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( (!strcmp(key, rec_mem[i].surname)) && (strcmp(rec_mem[i].surname,"SURNAME_DELETED"))){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		printf("Record with surname %s doesn't exist\n",key);
		return -1;
}



int transverse_block_list_key_address( HT_info header_info, char * key){

	int bucket_position, bucket_block;
	bucket_position = hash(key, header_info.numBuckets);
	void * block;
	int * i_mem;
	int total_block_records, next_block_id, current_block_id;
	int blocks_read = 0;
	Record * rec_mem;
	bucket_block = bucket_position / header_info.buckets_per_block; 
	bucket_position = bucket_position % header_info.buckets_per_block;


	if(BF_ReadBlock( header_info.filedesc, bucket_block, &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	block += (header_info.offset_to_buckets + (bucket_position * sizeof(int)));
	i_mem = (int * )block;

	if(BF_ReadBlock( header_info.filedesc, *i_mem, &block)){
		
		BF_PrintError("Failed reading block\n");
		return -1;

	}
	blocks_read++;
	i_mem = (int *)block;
	next_block_id = *i_mem;
	while(next_block_id != -1){

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;

		for (int i = 0; i < total_block_records; i++)
		{
			if( (!strcmp(key, rec_mem[i].address)) && (strcmp(rec_mem[i].address,"ADDRESS_DELETED"))){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		if(BF_ReadBlock( header_info.filedesc, next_block_id, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;

		}
		blocks_read++;
		i_mem = (int *)block;
		next_block_id = *i_mem;

	}

		i_mem++;
		total_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int i = 0; i < total_block_records; i++)
		{
			if( (!strcmp(key, rec_mem[i].address)) && (strcmp(rec_mem[i].address,"ADDRESS_DELETED"))){

				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[i].id,rec_mem[i].name,rec_mem[i].surname,rec_mem[i].address);
				return blocks_read;
			}


		}
		printf("Record with address %s doesn't exist\n",key);
		return -1;
}



void print_all_entries_HT(HT_info header_info){

	void * block;
	int * i_mem;
	int * pointer_to_data_block_cell;
	int * pointer_to_data_blocks;
	int total_data_block_records, next_block_id;
	int counter = 0;
	Record * rec_mem;
	
	for (int i = header_info.total_bucket_blocks; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{

		if(BF_ReadBlock( header_info.filedesc, i, &block)){

			BF_PrintError("Failed reading block\n");
			exit(1);

		}
		i_mem = (int *)block;
		i_mem++;
		total_data_block_records = *i_mem;
		i_mem += 2;
		rec_mem = (Record *)i_mem;
		for (int j = 0; j < total_data_block_records; j++)
		{
			
			if(rec_mem[j].id != -1 && strcmp(rec_mem[j].name,"NAME_DELETED") && strcmp(rec_mem[j].surname,"SURNAME_DELETED") && strcmp(rec_mem[j].address,"ADDRESS_DELETED")){
				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",rec_mem[j].id,rec_mem[j].name,rec_mem[j].surname,rec_mem[j].address);
			}	
		}
	}
}



int HT_GetAllEntries( HT_info header_info, void * value ){

	int id, blocks_read;
	char name[15],surname[25],address[50];


	if(value == NULL){

		print_all_entries_HT(header_info);
		return BF_GetBlockCounter(header_info.filedesc);

	}

	if(header_info.attrType == 'i'){    
	
		id = *((int *)value);
		blocks_read = transverse_block_list_key_id_HT(header_info,id);

	}

	if(header_info.attrType == 'c'){

		if(!strcmp(header_info.attrName,"name")){   

			strcpy(name,(char *)value);
			blocks_read = transverse_block_list_key_name( header_info, name);


		}

		else if(!strcmp(header_info.attrName,"surname")){  

			strcpy(surname,(char *)value);
			blocks_read = transverse_block_list_key_surname( header_info, surname);
		}

		else if(!strcmp(header_info.attrName,"address")){  

			strcpy(address,(char *)value);
			blocks_read = transverse_block_list_key_address( header_info, address);
		}
	}

	return blocks_read;
}


int HashStatistics( char * filename ){

	int filedesc, total_hash_file_blocks, overflow_block_counter = 0, temp, overflow_buckets = 0, total_records = 0;
	void * block;
	
	int * i_mem;
	int * i_record_mem;
	char * c_mem;
	int next_block_id;
	HT_info * ht;
	ht = HT_OpenIndex(filename); 
	void * bucket_block_pointer[ht->buckets_per_block]; 
	int total_records_per_bucket, valid_buckets = 0, valid_buckets_counter = 0, counter_overflow_print = 0;
	int * free_ptr;
	
	total_hash_file_blocks = BF_GetBlockCounter(ht->filedesc);
	printf("HT file has %d blocks\n",total_hash_file_blocks);
	
	void * mem;

	for (int i = 0; i < ht->total_bucket_blocks; i++)  
	{
	
		if(BF_ReadBlock( ht->filedesc, i, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;
		}
	
		block += ht->offset_to_buckets;  
		i_mem = (int *)block;

		for (int j = 0; j < ht->buckets_per_block; j++) 
		{	
			
			total_records_per_bucket = 0;
			if(i_mem[j] == -1 ) continue;  
			if(i_mem[j] == 0 ) break; 
			valid_buckets_counter++;
		}

	}

	int buckets_index[valid_buckets_counter];
	int blocks_per_overflow_buckets[valid_buckets_counter];
	valid_buckets_counter = 0;

	for (int i = 0; i < ht->total_bucket_blocks; i++)
	{
		mem = malloc(BLOCK_SIZE);
		if(BF_ReadBlock( ht->filedesc, i, &block)){

			BF_PrintError("Failed reading block\n");
			return -1;
		}
		memcpy(mem, block, BLOCK_SIZE);
		bucket_block_pointer[i] = mem;
		mem += ht->offset_to_buckets;
		i_mem = (int *)mem;

		for (int j = 0; j < ht->buckets_per_block; j++)
		{	

			total_records_per_bucket = 0;
			if(i_mem[j] == -1 ) continue;
			if(i_mem[j] == 0 ) break; 
			
			if(BF_ReadBlock( ht->filedesc, i_mem[j], &mem)){

				BF_PrintError("Failed reading block\n");
				return -1;
			}
			i_record_mem = (int *)mem;
			next_block_id = *i_record_mem;
			if(next_block_id != -1) overflow_buckets++;
			while(next_block_id != -1){
				i_record_mem++;
				overflow_block_counter++;
				total_records_per_bucket += *i_record_mem;
				if(BF_ReadBlock( ht->filedesc, next_block_id, &mem)){

					BF_PrintError("Failed reading block\n");
					return -1;
				}
				i_record_mem = (int *)mem;
				next_block_id = *i_record_mem;
			}
			i_record_mem++;
			total_records_per_bucket += *i_record_mem;
			buckets_index[valid_buckets_counter] = total_records_per_bucket;
			valid_buckets_counter++;
		}
	}

	for (int i = 0 ; i < valid_buckets_counter - 1; i++) //bubblesort
 	{
	    for (int j = 0 ; j < valid_buckets_counter - i - 1; j++)
	    {
	      if (buckets_index[j] > buckets_index[j+1])
	      {
	        temp = buckets_index[j];
	        buckets_index[j] = buckets_index[j+1];
	        buckets_index[j+1] = temp;
	      }
	    }
	}

	for (int i = 0; i < valid_buckets_counter; ++i)
	{
		total_records += buckets_index[i];
	}

	int total_record_blocks = BF_GetBlockCounter(ht->filedesc) - ht->total_bucket_blocks;
	int avg_blocks_per_bucket = total_record_blocks / valid_buckets_counter;

	printf("Max records per bucket = %d\n",buckets_index[valid_buckets_counter-1]);
	printf("Min records per bucket = %d\n",buckets_index[0]);
	printf("Average records per bucket = %d\n", total_records / valid_buckets_counter);
	printf("Average blocks per bucket = %d\n",avg_blocks_per_bucket);
	printf("Number of buckets containing overflow blocks = %d\n",overflow_buckets);
	printf("Average number of overflow blocks per bucket = %d\n", overflow_block_counter / overflow_buckets);

	
	for (int i = 0; i < ht->total_bucket_blocks; i++)
	{
		mem = bucket_block_pointer[i];
		free(mem);
	}

	return HT_CloseIndex(ht);
}