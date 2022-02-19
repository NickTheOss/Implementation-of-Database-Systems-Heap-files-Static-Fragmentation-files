#include "hp_ht.h"

// definitions are private
int create_new_block(int filedesc, int block_num);
int delete_record_key_id_HP(HP_info header_info, int key);
int delete_record_key_name_or_surname_or_address(HP_info header_info,char * key);
int transverse_block_list_key_id_HP(HP_info header_info, int key);
void print_all_entries_HP(HP_info header_info);
int transverse_block_list_key_name_or_surname_or_address(HP_info header_info, char * key);


int HP_CreateFile(char * filename,char attrType,char * attrName,int attrLength){

	int filedesc;
	void * block;
	int bytes_used = 0;

	BF_Init();  //arxikopoisi vivliothikis

	if(BF_CreateFile(filename)){

		BF_PrintError("Failed creating BF file\n");
		return -1;
	}

	if((filedesc = BF_OpenFile( filename )) < 0){

		BF_PrintError("Failed opening BF file\n");
		return -1;

	}

	if(BF_AllocateBlock( filedesc )){

		BF_PrintError("Failed allocating block\n");
		return -1;

	}

	if(BF_ReadBlock( filedesc, 0 , &block)){

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	char heap[] = "HEAP";   
	char * c_mem = (char *)block; 
	memcpy(c_mem, heap, strlen(heap)*sizeof(char)+sizeof(char));  
	c_mem += 5;  
	bytes_used += 5 * sizeof(char);  

	*c_mem = attrType;  
	c_mem++;  
	bytes_used += 1 * sizeof(char);  

	int * i_mem = (int * )c_mem;  

	*i_mem = attrLength;  
	i_mem++;  
	c_mem = (char * )i_mem;
	bytes_used += 1 * sizeof(int);  
	memcpy(c_mem, attrName, attrLength * sizeof(char) + sizeof(char));
	c_mem += attrLength + 1;
	bytes_used += (attrLength + 1) * sizeof(char);  
	i_mem = (int * )c_mem;
	*i_mem = 0;  
	bytes_used += 2 * sizeof(int); 
	i_mem++;  
	*i_mem = (BLOCK_SIZE-bytes_used)/sizeof(Record); 
	
	if(BF_WriteBlock(filedesc, 0)){  

		BF_PrintError("Failed to write block to disk\n");
		return -1;
	}

	if(BF_CloseFile(filedesc)){ 

		BF_PrintError("Failed closing file\n");
		return -1;

	}
	return 0;
}



HP_info * HP_OpenFile( char * filename ){

	int filedesc,attrLength;
	void * block;

	HP_info * hp = malloc(sizeof(HP_info));  
	if((filedesc = BF_OpenFile( filename )) < 0){ 

		BF_PrintError("Failed opening BF file\n");
		return NULL;

	}
	
	hp->filedesc = filedesc;

	if(BF_ReadBlock( filedesc, 0 , &block)){

		BF_PrintError("Failed reading block\n");
		return NULL;

	}

	char * c_mem = (char *)block;
	c_mem += 5;    
	hp->attrType = *c_mem;
	c_mem++;  
	int * i_mem = (int * )c_mem;
	hp->attrLength = *i_mem;
	attrLength = *i_mem;
	i_mem++;  
	c_mem = (char * )i_mem;
	hp->attrName = malloc(sizeof(char) * attrLength+ sizeof(char));
	memcpy(hp->attrName,c_mem,attrLength * sizeof(char) + sizeof(char));
	return hp;

}


int HP_CloseFile( HP_info * header_info ){

	if(BF_CloseFile(header_info->filedesc)){

		BF_PrintError("Failed closing file\n");
		return -1;

	}
	free(header_info->attrName);
	free(header_info);
	return 0;
}



int create_new_block(int filedesc, int block_num){

	void * block;

	if(BF_AllocateBlock( filedesc )){ 

		BF_PrintError("Failed allocating block\n");
		return -1;

	}

	if(BF_ReadBlock( filedesc, block_num + 1, &block)){  

		BF_PrintError("Failed reading block\n");
		return -1;

	}

	int * i_mem = (int * )block;
	*i_mem = 0;   
	i_mem++;
	*i_mem = (BLOCK_SIZE - (2 * sizeof(int))) / sizeof(Record);  

	if(BF_WriteBlock( filedesc, block_num + 1)){

		BF_PrintError("Failed writing block\n");
		return -1;

	}
}



int HP_InsertEntry(HP_info header_info, Record record ){

	void * block;
	int block_num,available_record_space,records_added_to_block;

	if((block_num = BF_GetBlockCounter(header_info.filedesc)) < 0){

		BF_PrintError("Failed getting block number\n");
		return -1;

	}

	block_num--;  

	if(BF_ReadBlock( header_info.filedesc, block_num , &block)){  

		BF_PrintError("Failed reading block\n");
		return -1;

	}
	char * c_mem = (char * )block;
	int * i_mem = (int *)c_mem;
	if(block_num == 0){ 

		c_mem += (6 * sizeof(char)); 
		i_mem = (int * )c_mem;
		c_mem += sizeof(int) * sizeof(char); 
		c_mem += ((*i_mem) + 1) * sizeof(char); 
		i_mem = (int * )c_mem;

	}

	records_added_to_block = *i_mem;
	(*i_mem)++;
	i_mem++;
	if(records_added_to_block == *i_mem){ 

		if(create_new_block(header_info.filedesc,block_num) == -1) return -1;

	} 
	i_mem++;
	block = (void * )i_mem; 
	block += (records_added_to_block * sizeof(Record));
	memcpy(block, &record, sizeof(Record));

	if(BF_WriteBlock(header_info.filedesc, block_num)){ 

		BF_PrintError("Failed to write block to disk\n");
		return -1;
	}

	return block_num;
	
}




int delete_record_key_id_HP(HP_info header_info, int key){

	bool flag = false;
	void * block;	
	int * i_mem;
	int total_records_file_per_block;
	for (int i = 0; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{

		if(BF_ReadBlock( header_info.filedesc, i , &block)){ 
			BF_PrintError("Failed reading block\n");
			return -1;

		}
		
		if(i == 0){

			char * c_mem = (char * )block;
			c_mem += (6 * sizeof(char)); 
			i_mem = (int * )c_mem;
			c_mem += sizeof(int) * sizeof(char); 
			c_mem += ((*i_mem) + 1) * sizeof(char); 
			i_mem = (int * )c_mem;
		}

		else 
			i_mem = (int * )block;
		
		i_mem++;
		total_records_file_per_block = *i_mem;
		i_mem++;

		Record * r_mem = (Record *)i_mem;
		for (int j = 0; j <= total_records_file_per_block; j++)
		{
			if(r_mem[j].id == key && (strlen(r_mem[j].name) > 0)){
				
				flag = true;
				r_mem[j].id = -1;
			}
			
		}
		if(BF_WriteBlock(header_info.filedesc,i)){

			BF_PrintError("Failed to write block to disk\n");
			return -1;
		}
	}

	if(!flag){
		printf("Error deleting record\n");
		return -1;
	}
	else
		return 0;

}



int delete_record_key_name_or_surname_or_address(HP_info header_info,char * key){

	bool flag = false;
	void * block;	
	int * i_mem;
	int total_records_file_per_block;
	for (int i = 0; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{
		
		if(BF_ReadBlock( header_info.filedesc, i , &block)){ 
			BF_PrintError("Failed reading block\n");
			return -1;

		}
		
		if(i == 0){

			char * c_mem = (char * )block;
			c_mem += (6 * sizeof(char)); 
			i_mem = (int * )c_mem;
			c_mem += sizeof(int) * sizeof(char); 
			c_mem += ((*i_mem) + 1) * sizeof(char); 
			i_mem = (int * )c_mem;

		}

		else 
			i_mem = (int * )block;
		
		i_mem++;
		total_records_file_per_block = *i_mem;
		i_mem++;

		Record * r_mem = (Record *)i_mem;
		if(!strcmp(header_info.attrName,"name")){  

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].name,key) && (strlen(r_mem[j].name) > 0)){

					flag = true;
					printf("search key value(NAME) to be deleted is = %s\n",key);
					strcpy(r_mem[j].name,"NAME_DELETED");
				}
			}

			if(BF_WriteBlock(header_info.filedesc,i)){

				BF_PrintError("Failed to write block to disk\n");
				return -1;
			}
		}

		else if(!strcmp(header_info.attrName,"surname")){ 

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].surname,key) && (strlen(r_mem[j].name) > 0)){

					flag = true;
					printf("search key value(SURNAME) to be deleted is = %s\n",key);
					strcpy(r_mem[j].surname,"SURNAME_DELETED");
				}
			}
			if(BF_WriteBlock(header_info.filedesc,i)){

				BF_PrintError("Failed to write block to disk\n");
				return -1;
			}
		}

		else if(!strcmp(header_info.attrName,"address")){  

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].address,key) && (strlen(r_mem[j].name) > 0)){

					flag = true;
					printf("search key value(ADDRESS) is = %s\n",key);
					strcpy(r_mem[j].address,"ADDRESS_DELETED");
				}
			}

			if(BF_WriteBlock(header_info.filedesc,i)){

				BF_PrintError("Failed to write block to disk\n");
				return -1;
			}
		}
	}

	if(!flag){
		printf("No entry found,error occured\n");
		return -1;
	}
	else
		return 0;

}




int HP_DeleteEntry( HP_info header_info,void * value ){

	int id,delete_success;
	char name[15],surname[25],address[50];

	if(header_info.attrType == 'i'){   
	
		id = *((int *)value);
		delete_success = delete_record_key_id_HP(header_info,id);

	}

	if(header_info.attrType == 'c'){

		if(!strcmp(header_info.attrName,"name")){ 

			strcpy(name,(char *)value);
			delete_success = delete_record_key_name_or_surname_or_address( header_info, name);


		}

		else if(!strcmp(header_info.attrName,"surname")){ 

			strcpy(surname,(char *)value);
			delete_success = delete_record_key_name_or_surname_or_address( header_info, surname);
		}

		else if(!strcmp(header_info.attrName,"address")){  

			strcpy(address,(char *)value);
			delete_success = delete_record_key_name_or_surname_or_address( header_info, address);
		}
	}

	return delete_success;  
}



int transverse_block_list_key_id_HP(HP_info header_info, int key){

	printf("search key(ID) value is = %d\n",key);
	bool flag = false;
	void * block;	
	int * i_mem;
	int total_records_file_per_block, blocks_read_until_records_found;
	for (int i = 0; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{
		
		if(BF_ReadBlock( header_info.filedesc, i , &block)){ 
			BF_PrintError("Failed reading block\n");
			return -1;

		}
		
		if(i == 0){

			char * c_mem = (char * )block;
			c_mem += (6 * sizeof(char)); 
			i_mem = (int * )c_mem;
			c_mem += sizeof(int) * sizeof(char); 
			c_mem += ((*i_mem) + 1) * sizeof(char); 
			i_mem = (int * )c_mem;

		}

		else 
			i_mem = (int * )block;
		
		i_mem++;
		total_records_file_per_block = *i_mem;
		i_mem++;

		Record * r_mem = (Record *)i_mem;
		for (int j = 0; j <= total_records_file_per_block; j++)
		{
			
			if(r_mem[j].id == key && (strlen(r_mem[j].name) > 0) && (r_mem[j].id != -1)){
				blocks_read_until_records_found = i+1;  
				flag = true;
				printf("Record found with attributes\nID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",r_mem[j].id,r_mem[j].name,r_mem[j].surname,r_mem[j].address);

			}	
		}
	}

	if(!flag){
		printf("No records found\n");
		return -1;
	}
	else
		return blocks_read_until_records_found;

}



int transverse_block_list_key_name_or_surname_or_address(HP_info header_info, char * key){

	bool flag = false;
	void * block;	
	int * i_mem;
	int total_records_file_per_block, blocks_read_until_records_found;
	for (int i = 0; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{
		
		if(BF_ReadBlock( header_info.filedesc, i , &block)){ 
			BF_PrintError("Failed reading block\n");
			return -1;

		}
		
		if(i == 0){

			char * c_mem = (char * )block;
			c_mem += (6 * sizeof(char)); 
			i_mem = (int * )c_mem;
			c_mem += sizeof(int) * sizeof(char); 
			c_mem += ((*i_mem) + 1) * sizeof(char); 
			i_mem = (int * )c_mem;

		}

		else 
			i_mem = (int * )block;
		
		i_mem++;
		total_records_file_per_block = *i_mem;
		i_mem++;

		Record * r_mem = (Record *)i_mem;
		if(!strcmp(header_info.attrName,"name")){

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].name,key) && (strlen(r_mem[j].name) > 0) && strcmp(r_mem[j].name,"NAME_DELETED")){
					blocks_read_until_records_found = i+1;  
					flag = true;
					printf("search key value(NAME) is = %s\n",key);
					printf("Record found with attributes\nID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",r_mem[j].id,r_mem[j].name,r_mem[j].surname,r_mem[j].address);
					
				}
			}
		}

		else if(!strcmp(header_info.attrName,"surname")){

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].surname,key) && (strlen(r_mem[j].name) > 0) && strcmp(r_mem[j].surname,"SURNAME_DELETED")){
					blocks_read_until_records_found = i+1;  
					flag = true;
					printf("search key value(SURNAME) is = %s\n",key);
					printf("Record found with attributes\nID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",r_mem[j].id,r_mem[j].name,r_mem[j].surname,r_mem[j].address);

				}
			}
		}

		else if(!strcmp(header_info.attrName,"address")){

			for (int j = 0; j <= total_records_file_per_block; j++)
			{
				if(!strcmp(r_mem[j].address,key) && (strlen(r_mem[j].name) > 0) && strcmp(r_mem[j].address,"ADDRESS_DELETED")){
					blocks_read_until_records_found = i+1;  
					flag = true;
					printf("search key value(ADDRESS) is = %s\n",key);
					printf("Record found with attributes\nID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",r_mem[j].id,r_mem[j].name,r_mem[j].surname,r_mem[j].address);

				}
			}
		}
	}

	if(!flag){
		printf("No records found\n");
		return -1;
	}
	else
		return blocks_read_until_records_found;

}


void print_all_entries_HP(HP_info header_info){

	void * block;	
	int * i_mem;
	int total_records_file_per_block;
	printf("Given value is 'NULL' all records are printed below:\n");
	for (int i = 0; i < BF_GetBlockCounter(header_info.filedesc); i++)
	{

		if(BF_ReadBlock( header_info.filedesc, i , &block)){ 
			BF_PrintError("Failed reading block\n");
			exit(1);

		}
		
		if(i == 0){

			char * c_mem = (char * )block;
			c_mem += (6 * sizeof(char)); 
			i_mem = (int * )c_mem;
			c_mem += sizeof(int) * sizeof(char); 
			c_mem += ((*i_mem) + 1) * sizeof(char); 
			i_mem = (int * )c_mem;


		}

		else 
			i_mem = (int * )block;
		
		i_mem++;
		total_records_file_per_block = *i_mem;
		i_mem++;

		Record * r_mem = (Record *)i_mem;
		for (int j = 0; j <= total_records_file_per_block; j++)
		{

			if(strlen(r_mem[j].name) > 0 && r_mem[j].id != -1 && strcmp(r_mem[j].name,"NAME_DELETED") && strcmp(r_mem[j].surname,"SURNAME_DELETED") && strcmp(r_mem[j].address,"ADDRESS_DELETED")){
				printf("ID = %d\nNAME = %s\nSURNAME = %s\nADDRESS = %s\n",r_mem[j].id,r_mem[j].name,r_mem[j].surname,r_mem[j].address);
			}
		}
	}
}



int HP_GetAllEntries( HP_info header_info, void * value ){

	int id, blocks_read;
	char name[15],surname[25],address[50];

	if(value == NULL){

		print_all_entries_HP(header_info);
		return BF_GetBlockCounter(header_info.filedesc);

	}

	if(header_info.attrType == 'i'){    //id is the key
	
		id = *((int *)value);
		blocks_read = transverse_block_list_key_id_HP(header_info,id);

	}

	if(header_info.attrType == 'c'){

		if(!strcmp(header_info.attrName,"name")){   //name is the key

			strcpy(name,(char *)value);
			blocks_read = transverse_block_list_key_name_or_surname_or_address( header_info, name);


		}

		else if(!strcmp(header_info.attrName,"surname")){  //surname is the key

			strcpy(surname,(char *)value);
			blocks_read = transverse_block_list_key_name_or_surname_or_address( header_info, surname);
		}

		else if(!strcmp(header_info.attrName,"address")){  //address is the key

			strcpy(address,(char *)value);
			blocks_read = transverse_block_list_key_name_or_surname_or_address( header_info, address);
		}
	}

	return blocks_read;
}
