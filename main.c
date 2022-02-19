#include "record_list.h"

struct record_list list;


void destroy_record_list(){  

	struct record_node * current;

	while(list.head){

		current = list.head;
		list.head = list.head->next;
		free(current);
	}
}


void print_record_list(){  //test function

	struct record_node * current = list.head;

	while(current){
		printf("ID = %d NAME = %s\n",current->id,current->name);
		current = current->next;
		
	}
}



void insert_record_node(int id, char * name, char * surname, char * address){ 

	struct record_node * node = (struct record_node *)malloc(sizeof(struct record_node));

	node->next = NULL;
	node->id = id;
	strcpy(node->name,name);
	strcpy(node->surname,surname);
	strcpy(node->address,address);

	if( list.record_node_counter == 0){

		list.head = node;
		list.tail = node;
		list.record_node_counter++;

	}

	else
	{

		list.tail->next = node; 
		list.tail = node;
		list.record_node_counter++;
	}
}


void initialize_record_list(){  
	
	list.head = NULL;
	list.tail = NULL;;
	list.record_node_counter = 0;

}


void tokenize_record_lines_and_insert(){  

	int id;
	char buffer[MAXSIZE];
	char * token;
	char name[15],surname[25],address[50];
	FILE * fp = fopen("records15K.txt","r");   
	
	if(fp == NULL){  

		printf("Failed opening record file\n");
		exit(1);
	}

	while(fgets(buffer,MAXSIZE,fp)!=NULL){  

		token = strtok(buffer,"{,");  //id
		id = atoi(token);
		token = strtok(NULL,",\"");    //name
		strcpy(name,token);
		token = strtok(NULL,",\"");    //surname
		strcpy(surname,token);
		token = strtok(NULL,",\"}\n");    //address
		strcpy(address,token);
		insert_record_node(id,name,surname,address);

	}
}



int main(int argc, char ** argv) {

	int x,y,block_position, blocks_read, delete_success,close, hash_statistics;
	int id = 5;
	char name[] = "name_4";
	char surname[] = "surname_8";
	char address[] = "address_5";
	Record record;
	initialize_record_list();
	tokenize_record_lines_and_insert();
	struct record_node * current = list.head;

	/*HP_info * hp;
	x = HP_CreateFile("HEAP",'i',"id",4);

	if (!x) printf("Epitixis dimiourgia arxeiou\n");

	hp = HP_OpenFile("HEAP");
	

	while(current){

		record.id = current->id;
		strcpy(record.name,current->name);
		strcpy(record.surname,current->surname);
		strcpy(record.address,current->address);
		block_position = HP_InsertEntry( *hp, record );
		current = current->next;
		
	}

	delete_success = HP_DeleteEntry(*hp, &id);
	blocks_read = HP_GetAllEntries( *hp, NULL);

	if(blocks_read > 0) printf("Total Blocks read = %d\n", blocks_read);
	destroy_record_list();
	y = HP_CloseFile(hp);*/

	HT_info * ht;
	x = HT_CreateIndex("HASH",'i',"id",2,500);
	ht = HT_OpenIndex("HASH");

	while(current){

		record.id = current->id;
		strcpy(record.name,current->name);
		strcpy(record.surname,current->surname);
		strcpy(record.address,current->address);
		block_position = HT_InsertEntry( *ht, record );
		current = current->next;
		
	}

	delete_success = HT_DeleteEntry(*ht, name);
	blocks_read = HT_GetAllEntries( *ht, NULL);
	destroy_record_list();
	close = HT_CloseIndex(ht);
	hash_statistics = HashStatistics("HASH");
	return 0;
}
