Implementation of Database Systems Work 1

The folder contains 3 source files (main.c, hp.c, ht.c), 3 header files
(record_list.h, hp_ht.h, BF.h), 1 makefile file for compilation, 2 .a files (BF_32.a, BF_64.a) containing libraries for the use of BF functions ( 32-bit or 64 -bit accordingly), a test file containing records (let max number of records given = records15K.txt) and 2 output files (HEAP for heap files and HASH for static hash files).
If we want another record file,we just go to line 75 in main.c and enter the name of the input file we want.

The functions of each file are described in detail below:

a) Header files

record_list.h: Includes the statements of the functions used in the "main.c" file as well as a single linked list (struct record_list) where we temporarily store the records we receive from the input file and then use the insert_entry function for each record.

hp_ht.h: Includes the statements of the functions used in the pile file (HP) and the static hash file (HT), as well as the basic data structure standards defined by the pronunciation (Record for the format of the record, HP_info for the heap file and HT_info for the hash file).

BF.h: Includes the statements of the basic functions we use to implement the program, as given.

b) Source files

main.c: The main function starts at line 100. At the beginning we declare some indicative test values ​​based on which we debug our program (then call the HP or HT functions respectively). If we want to edit a heap file (HP), we need to uncheck line 112 - 136 and comment on lines 138 - 159. If we want to edit a static hash file (HT) we run the program as it is. The main terminates after freeing the space we have reserved in bytes and releasing the list of records that we create in line 93.

hp.c: For batch files, we have implemented the following implementation: The first block that will be created will contain the "special information". The first 5 bytes contain the word ("HEAP\0") which informs us that it is a heap file. Then, the next cell will contain the attrType ie the character 'i' if the key is integer or the character 'c' if the key is a character. The next cell is attrLength which is also an integer. Then, in the next cell we place the attrName and then starts the space where we will place the registrations. Each block of records (in the first block the remaining space left is bound by the first entering records) has the following implementation. The first position contains an integer which informs us about the number of entries that have been entered in the block. The next cell is also an integer that states how many records a block can hold (it helps us to enter records correctly, so that when a block is filled with records we can allocate the memory in time for a new block). The remaining cells are filled with Record type records until the block is filled. The same process is done until the entry of the registrations is completed. Then we can call delete_entry to delete a record with any key type we want, or call HP_GetAllEntries or a combination. If the "NULL" argument is given, all the records in the file will be printed, otherwise the record that has the key we are looking for will be printed. If the registration does not exist, a message is printed.
In addition to the requested functions, the following auxiliary functions were implemented:

a) int create_new_block (int filedesc, int block_num) (line 143). Used every time we need to allocate a new block. Takes as an argument block_num which is the current number of the block we are passing.

b) int delete_record_key_id_HP (line 234). To delete an entry with the id key, we conventionally change the id value to '-1'.

c) int delete_record_key_name_or_surname_or_address (line 294). To delete a record with the key name, change the name to 'NAME_DELETED'. Similarly, to delete an entry with a surname key, change the name to 'SURNAME_DELETED'. Finally, to delete an entry with a key the address, we change the name to 'ADDRESS_DELETED'.

d) int transverse_block_list_key_id_HP (line 437). It crosses the blocks of the file and prints the attributes of the record that has a key value equal to the value of the search key (with the id key).

e) int transverse_block_list_key_name_or_surname_or_address (line 495). It crosses the blocks of the file and prints the attributes of the record that has a key value equal to the value of the search key (key name or surname or to address).

f) void print_all_entries_HP (line 582). Prints all entries entered in the file.

ht.c: For static hash files, we have implemented the following implementation: The information blocks we initially create will include "special information" and buckets. The buckets in turn contain either the number '-1' (meaning that they have not received a record through the corresponding hashfunction) or the block number into which a record will be inserted. Regarding the information blocks, the first 5 bytes include the word ("HASH\0") which informs us that it is a static hash file. Then, the next cell will contain the attrType ie the character 'i' if the key is integer or the character 'c' if the key is a character. The next cell is attrLength which is also an integer. Then, in the next cell we place the attrName and in the next number the buckets that the block contains. Then in the next cell we keep the offset from the beginning of the block until the address where the first bucket of the block starts (We will need it to easily go to the buckets).Finally in the next position of the block we hold an integer with the number of blocks of information (which we will need later to print the statistics). So after we build the information blocks, we must now enter the records correctly. For each entry we enter we take the value of its key and give it as an argument to the corresponding hashfunction. The hashfunction returns a number, which leads us to the corresponding information block and bucket where we need to enter the registration. Then, we create a new block that will now be a data block (with the data of each record) and the bucket will have the number of that block.Each data block is as follows. In the first cell there is the block number that the next entry will enter if the block overflows. If the number is '-1' then it means that the block is not full yet, so more entries will be made. The next cell indicates the number of records entered in the block and the next cell, the number of records that a block of size BLOCKSIZE can hold. In the next cell the block contains Record type records (in the following cells).Then we can call delete_entry to delete a record with any key type we want, or call HT_GetAllEntries or a combination. If the "NULL" argument is given, all the records in the file will be printed, otherwise the record that has the key we are looking for will be printed. If the registration does not exist, a message is printed. Finally we call through the main and the HashStatistics function which runs through the file and prints the requested statistics.
In addition to the requested functions, the following auxiliary functions were implemented:

a) int hash (line 20). Returns the bucket number into which an entry with key type 'c' will be inserted.

b) int hash (line 34). Returns the bucket number into which an entry with key type 'i' will be inserted.

c) int create_new_record_block (line 357). It blocks a new block when a data block overflows.

d) int delete_record_key_id_HT (line 447). To delete an entry with the id key, we conventionally change the id value to '-1'.

e) int delete_record_key_name (line 536). To delete a record with the name key, we conventionally change the id value to 'NAME_DELETED'.

f) int delete_record_key_surname (line 624). To delete an entry with the surname key, we conventionally change the id value to 'SURNAME_DELETED'.

g) int delete_record_key_address (line 712). To delete an entry with an address key, we conventionally change the id value to 'ADDRESS_DELETED'.

h) int transverse_block_list_key_id_HT (line 802). Hash-removes the value of the key and goes to the corresponding bucket where the record is located. Then, it prints its attributes. (Id key)

i) int transverse_block_list_key_name (line 883). Hash-removes the value of the key and goes to the corresponding bucket where the record is located. Then, it prints its attributes. (With the name key)

j) int transverse_block_list_key_surname (line 964). Hash-removes the value of the key and goes to the corresponding bucket where the record is located. Then it prints its attributes. (Surname key)

k) int transverse_block_list_key_address (line 1045). Hash-removes the value of the key and goes to the corresponding bucket where the record is located. Then, it prints its attributes. (Address key)

l) void print_all_entries_HT (line 1126). It traverses all data blocks and prints the attributes of all current recordings.

The implementation has been performed for 64-bit format. If the program has to run properly in 32-bit format, it is enough to edit the command in line 14 of the makefile and instead of "BF_64.a -no-pie" change it to "BF_32.a -no-pie".
Command for compiling is 'make', and 'make clean' is for cleaning object files of the program.
Execution command from the command line is './project1'.

Implementing Language: C
Implementation Environment: Linux