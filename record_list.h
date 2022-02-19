#ifndef RECORD_LIST_
#define RECORD_LIST_
#define MAXSIZE 80
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hp_ht.h"

struct record_node{   

	int id;
	char name[15];
	char surname[25];
	char address[50];
	struct record_node * next;

};



struct record_list{ 

	struct record_node * head;
	struct record_node * tail;
	int record_node_counter;
};	

void insert_record_node(int id, char * name, char * surname, char * address);
void destroy_record_list();
void initialize_record_list();
void print_record_list();
void tokenize_record_lines_and_insert();

#endif