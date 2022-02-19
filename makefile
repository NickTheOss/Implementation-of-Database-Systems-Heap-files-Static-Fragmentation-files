CC=gcc

objects=main.o hp.o ht.o 

SOURCE = main.c hp.c ht.c 

HEADER = record_list.h hp_ht.h 

OUT = project1

FLAGS = -g -c 

all : $(objects)
	$(CC) -g $(objects) -o $(OUT) BF_64.a -no-pie

main.o : main.c
	$(CC) $(FLAGS) main.c

hp.o : hp.c
	$(CC) $(FLAGS) hp.c
	
ht.o : ht.c
	$(CC) $(FLAGS) ht.c
	
	
clean:
	rm -f project1 $(objects)