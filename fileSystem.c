/**************************************************0**************************************************
** This program implements a FAT file system with an interactive console. 							**
** It supports writing and reading of file to virtual drive along with other functionalities.		**
**																									**		
* Bytes per Sector: 512  | 19532 Sectors in 10MB Sectors per Cluster: 2 | 9766 Clusters in 10MB
* Each cluster can represent up to 65536  
* RD states where it starts and how big it is
* and then the program goes through the array and gives out pointers until it reaches 
* that file size Program freads from the array file_allocation_table, it reads the amount
* of clusters that make up the file.
******************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#define BPS 512 //Bytes per Sector
#define SPC 2 //Sectors per Cluster 
#define BPRD 32 //Bytes per Root Directory Entry

#define RD_offset 11*BPS//RD is this many bytes in
#define data_offset 21*BPS//data is this many bytes in

typedef unsigned char BYTE; //This will make it easier for me later when I need to take a 2 or 4  byte data type like a short and have to store it byte by byte


void createVBR();
void pseudoWrite();
void write_to_disk();
unsigned char * create_meta();
unsigned short find_emptyCluster(unsigned short);
void create_RD_entry(unsigned char * , unsigned short);
unsigned char * find_fileName(); 
unsigned char * find_fileExt(); 
void traverse_file_allocation_table(unsigned short);
unsigned int find_fileSize(char *);
unsigned int gobbledygook();
unsigned int text_create();
unsigned short convertShort(char *);
unsigned int convertInt(char *);
unsigned short check_RD(char *); 
unsigned short check_sub(char * , unsigned short); 
unsigned char check_directory(char *);
unsigned char check_directory_sub(char * , unsigned short);
char * retrieve_meta(int);
unsigned char directory_check();
unsigned short time_encode();
unsigned short date_encode();

unsigned char volume_boot_record[1*BPS] = {0x00}; //This is just general info about my file system. ex:BPS = 512
unsigned short file_allocation_table[10 * BPS] = {0x00};//This is where all the pointers to the data are 
unsigned char root_directory[10 * BPS] = {0x00};//This tells us where to start in the FAT chain
unsigned char data_section[500 * BPS * SPC] = {0x00};//Where all the files go

static FILE * drive;

unsigned short pwd = 0;//this is a global to say where you are

/*********************************************************************************************
 * This function calls on the operations needed to read/write from the virtual dre along     *
 * with related string parsing.                                                              *
 *                                                                                           *
 * Postconditions:                                                                           *
 * @return void                                                                              *	
 *********************************************************************************************/
int main() {

    drive = fopen("Drive10MB" , "r+");
    int j = 0;
    copy_FAT_to_memory();
    copy_RD_to_memory(); //temporary fix  in theory i shouldn't need this but since I fwrite my whole RD and reopening the file I wouldn't have any RD bytes in memory. to fix this I need a more controlled fwrite
    copy_data_to_memory(); //temporary fix see above

    file_allocation_table[0] = 0xF8FF;
    file_allocation_table[1] = 0xFFFF;

    int file_size; //The amount of clusters that the file is
    while(1) 
        choices();
}

void copy_FAT_to_memory(){

    fseek(drive , 1*BPS , SEEK_SET);

    unsigned char byte;
    int index=0;

    while(index < BPS*10) {
        byte = fgetc(drive);
        file_allocation_table[index] = byte;
        index++;
    }
}
void copy_RD_to_memory(){

    fseek(drive , 11*BPS , SEEK_SET);

    unsigned char byte;
    int index=0;

    while(index < BPS*10) {
        byte = fgetc(drive);
        root_directory[index] = byte;
        index++;
    }
}
void copy_data_to_memory(){

    fseek(drive , 21*BPS , SEEK_SET);

    unsigned char byte;
    int index=0;

    while(index < BPS*500) {
        byte = fgetc(drive);
        data_section[index] = byte;
        index++;
    }
}




void choices() {

    printf("\n\tPWD:%d\n" , pwd);

    int choice;


    printf("What would you like? \n1)Read File/Change Directory\n2)Write File/Create Directory\n3)Exit\n");

    scanf("%d" , &choice);

    switch(choice) {
        case 1://read file
            printf("Choice 1\n");
            file_read(pwd);
            break;
        case 2://write file or directory
            printf("Choice 2\n");
            pseudoWrite();
            write_to_disk(drive);
            break;
        case 3://Exit
            printf("Choice 3\n");
            fclose(drive);
            exit(1);
            break;
    }
    
}
/*********************************************************************************************
 * This function reads data from the file system and displays to screen.                     *
 * Preconditions: 																			 *
 * @params directory_location - short 
 * Postconditions:  																		 *
 * @return void																				 *
 *********************************************************************************************/

void file_read(unsigned short directory_location) {

    unsigned short startCluster;
    unsigned char directory;

    unsigned short match;

    char fileName[8];

    printf("Which file/directory: ");
    scanf("%08s" , fileName);

    printf("%s\n" , fileName);

    if(fileName[0] == 'R' && fileName[1] == 'O' &&  fileName[2] == 'O' && fileName[3] == 'T' ){
        pwd = 0;
    }

    else{
   
        if(directory_location == 0) {//we will make this the default which is the RD
            startCluster= check_RD(fileName);
            directory = check_directory(fileName);
        }
        else{//it's a subdirectory
            printf("hey man we made it to subdirectory\n");
           startCluster = check_sub(fileName , directory_location); 
            printf("hey woman we made it to subdirectory\n");
           directory = check_directory_sub(fileName , directory_location);
        }

    

    if (startCluster) {
        if(!directory){//this means that it's a file in the current directory 
            traverse_file_allocation_table(startCluster); //traverse FAT using the start cluster
            pseudoClose();
        }
        else{
            pwd=startCluster;
           
        }
    }
    else{
        printf("Sorry there is no file by that name");
    }
}
}
/*********************************************************************************************
 * This function checks if the file is closed, if not check again.		                     *
 *																							 *
 * Postconditions:  																		 *
 * @return void																				 *
 *********************************************************************************************/
void pseudoClose() {

    int choice = 0;

    printf("Close file now?\n1)Yes\n2)No\n");
    scanf("%d" , &choice );

    if (choice==1) {
        system("clear");
    }
    else{
        pseudoClose();
    }

}
/*********************************************************************************************
 * This function reads data from the file system and displays to screen.                     *
 * Preconditions: 																			 *
 * @params filename - char pointer															 *
 * @params directory_location - unsigned short  											 *		     *
 * Postconditions:  																		 *
 * @return unsigned short																	 *
 *********************************************************************************************/

unsigned short check_sub(char * fileName , unsigned short directory_location) {

    unsigned char byte;
    char check[8];
    
    unsigned char  * metaData;
    
    unsigned short startCluster = 0;
    
    int index = 0;
    int entry_offset= (RD_offset+(BPS*10) + (BPS*SPC*(directory_location-2))); 
    int end = entry_offset+ (10*BPS);//size of a dir
   
    while(entry_offset < end) { //while it's smaller than the size of the RD
        fseek(drive , entry_offset , SEEK_SET );
        while (index < 8) {//copy the entry name
            byte = fgetc(drive);
            check[index] = byte;
            index++;
        }
        
        int string_check = strcmp(check , fileName);

        if(string_check == 0){
            printf("\n\n\t\t\tWe found a match boys!\n");//now we should find his startCluster
            metaData = retrieve_meta(entry_offset);//this is where the RD entry starts on the drive
            startCluster = convertShort(metaData);
            \
        }

        entry_offset += 32; 
        index=0;

    }

    return startCluster;

}
/*********************************************************************************************
 * This function checks the root directory   							                     *
 * Preconditions: 																			 *
 * @params filename - char pointer															 *		     *
 * Postconditions:  																		 *
 * @return unsigned short																	 *
 *********************************************************************************************/

unsigned short check_RD(char * fileName) {

    unsigned char byte;
    char check[8];
    int entry_offset = RD_offset;
    unsigned char  * metaData;
    unsigned short startCluster = 0;
    
    int index = 0;
    int end = (RD_offset+(BPS*10));

    while(entry_offset < (RD_offset+(BPS*10))) { //while it's smaller than the size of the RD
        fseek(drive , entry_offset , SEEK_SET );
        while (index < 8) {//copy the entry name
            byte = fgetc(drive);
            check[index] = byte;
            index++;
        }
        
        int string_check = strcmp(check , fileName);

        if(string_check == 0){
            printf("\n\n\t\t\tWe found a match boys!\n");//now we should find his startCluster
            metaData = retrieve_meta(entry_offset);//this is where the RD entry starts on the drive
            
            startCluster = convertShort(metaData);
            
        }

        entry_offset += 32; 
        index=0;

    }

    return startCluster;

}
/*********************************************************************************************
 * This function checks the directory									                     *
 * Preconditions: 																			 *
 * @params filename - char pointer															 *		     *
 * Postconditions:  																		 *
 * @return unsigned char																	 *
 *********************************************************************************************/

unsigned char check_directory(char * fileName) {

    unsigned char byte;
    char check[8];
    int entry_offset = RD_offset;
    unsigned char  * metaData;
    //unsigned char metaData;
    unsigned char directory= 0;
    
    int index = 0;
    int end = (RD_offset+(BPS*10));
    
    while(entry_offset < (RD_offset+(BPS*10))) { //while it's smaller than the size of the RD
        fseek(drive , entry_offset , SEEK_SET );
        while (index < 8) {//copy the entry name
            byte = fgetc(drive);
            check[index] = byte;
            index++;
        }
        int string_check = strcmp(check , fileName);

        if(string_check == 0){
            printf("\n\n\t\t\tWe found a match boys!\n");//now we should find his startCluster
            metaData = retrieve_meta(entry_offset);//this is where the RD entry starts on the drive
           
            directory = metaData[11];
           
        }

        entry_offset += 32; 
        index=0;

    }

    return directory;

}
/*********************************************************************************************
 * This function checks the sub-directory               									 *
 * Preconditions: 																			 *
 * @params filename - char pointer															 *
 * @params directory_location - unsigned short  											 *		     *
 * Postconditions:  																		 *
 * @return unsigned short																	 *
 *********************************************************************************************/

unsigned char check_directory_sub(char * fileName , unsigned short directory_location){

    unsigned char byte;
    char check[8];
    unsigned char  * metaData;
   
    unsigned char directory= 0;
    
    int index = 0;
    int entry_offset= (RD_offset+(BPS*10) + (BPS*SPC*(directory_location-2))); 
    int end = entry_offset+ (10*BPS);


    while(entry_offset < end) { //while it's smaller than the size of the RD
        printf("WERTYUIKGTYJHGKJHJKKJ\n");
        fseek(drive , entry_offset , SEEK_SET );
        while (index < 8) {//copy the entry name
            byte = fgetc(drive);
            check[index] = byte;
            index++;
        }
        
        int string_check = strcmp(check , fileName);

        if(string_check == 0){
            printf("\n\n\t\t\tWe found a match boys!\n");//now we should find his startCluster
            metaData = retrieve_meta(entry_offset);//this is where the RD entry starts on the drive
            directory = metaData[11];
           
        }

        entry_offset += 32; 
        index=0;

    }

    return directory;

}
/*********************************************************************************************
 * This function retrieve_meta											                     *
 * Preconditions: 																			 *
 * @params entryStart - int																	 *		     *
 * Postconditions:  																		 *
 * @return char* 																			 *
 *********************************************************************************************/

char * retrieve_meta(int entryStart){

    static unsigned char metaData[] = {0x00};
    int index=0;
    unsigned char byte; 

    fseek(drive , entryStart , SEEK_SET);

    while(index < 32) {
        byte = fgetc(drive);
        metaData[index] = byte;
        index++;
    }

   return metaData; 

}

void file_write() {
    printf("file_Write not implemented yet\n");
}

void create_directory() {
    printf("create_directory not implemented yet\n");
}
/*********************************************************************************************
 * This function will copy a designated file										                     *
 * Preconditions: 																			 *
 * @params meatData - char pointer																	 *		     *
 * Postconditions:  																		 *
 * @return unsigned int																		 *
 *********************************************************************************************/

unsigned int copy_file(char * metaData) {


    char fileName[10];
    
    printf("Which file do you want to copy: ");
    scanf("%08s" , fileName);

    printf("%s\n" , fileName);

    FILE * file;

    file = fopen(fileName , "r");

    printf("opened OK\n");

    unsigned char byte = fgetc(file);


    fseek(file, 0L, SEEK_END);
    unsigned int size = ftell(file);//returns the cursor number of last byte so now we know the size
    rewind(file);


    unsigned short startCluster = convertShort(metaData);

    unsigned short index = ((startCluster-2)*BPS*SPC);
    int amount = 0;

    printf("Copy index :%d" , index );

    while(amount < size ) {
        byte = fgetc(file);
        data_section[index] = byte; 
        amount++;
        index++;
    }

    return size; 

}

/*********************************************************************************************
 * This function traverses the file allocation table										                     *
 * Preconditions: 																			 *
 * @params startCluster - short																	 *		     *
 * Postconditions:  																		 *
 * @return void 																			 *
 *********************************************************************************************/

void traverse_file_allocation_table(unsigned short startCluster) {


    unsigned short cluster;
    unsigned short byte;
    unsigned short start;
    int offset = (startCluster - 2)*SPC*BPS; 

    byte = 0;

    printf("\n\n\n\n Cluster before offset%02x \n\n\n\n" , startCluster);

    printf("\n\n\n\n Cluster %02x \n\n\n\n" , startCluster-2);

    while(byte < BPS*SPC) {
        printf("%02x " , data_section[offset]);
        byte++;
        offset++;
    }


    cluster = file_allocation_table[startCluster];

    byte = 0;
    
    if(cluster == 0xFFFF){
        printf("\n\n\n\nI'M DONEEEEE\n\n\n\n");
    }

    else{ 
           while(cluster != 0xFFFF) {
                printf("\n\n\n\n Cluster %d(of data region) \n\n\n\n" , (cluster-2));
                byte = 0;
                while (byte < SPC*BPS) {
                    printf("%02x " , data_section[offset]);
                    byte++;
                    offset++;
                }
           
            cluster = file_allocation_table[cluster];
        }
        printf("\n\n\n\nI'M DONEEEEE\n\n\n\n");
    }

}
void pseudoWrite() {

    unsigned short directory_location = pwd;
    

    unsigned char * metaData = create_meta();

    create_RD_entry(metaData , directory_location);

    //create_data(metaData);
    create_file_allocation_table_entries(metaData);


}
/*********************************************************************************************
 * This function creates file allocation table entries							             *
 * Preconditions: 																			 *
 * @params metatdata - char*															     *
 * Postconditions:  																		 *
 * @return void  																			 *
 *********************************************************************************************/

void create_file_allocation_table_entries(unsigned char * metaData){

    int size = 0;//we're going to have to take the bytes and construct it into an int

    //Little endian least significant byte
    unsigned int weight = 0;
    unsigned char temp = 0;
    int index = 28;

    //This takes the bytes and turns it into an int
    size = convertInt(metaData);

    printf("\n\n\n\n Calculated size:%d \n\n\n\n" , size);

    weight = 0;
    index = 26;

    unsigned short startCluster = 0;

    startCluster = convertShort(metaData);

    float clusterAmount = (size / (BPS*SPC));
    printf("\n\nThis:%d\n\n" , size);
    printf("\n\nThis:%d\n\n" , BPS*SPC);
    printf("\n\nThat:%f\n\n" , clusterAmount);
    unsigned short fatChain = startCluster;

    if (clusterAmount < 1 ){
        clusterAmount = 0;
    }
    
    while(clusterAmount != 0) {

        unsigned short nextCluster = find_emptyCluster((fatChain+1)); //this searches starting from the index after 
        file_allocation_table[fatChain] = nextCluster;
        
        clusterAmount--;
        if (clusterAmount < 1 ){
            clusterAmount = 0;
        }
        fatChain = nextCluster;
        
    }

    //We reach this point after we know we don't need any additional clusters.
    file_allocation_table[fatChain] = 0xFFFF;


}


unsigned int power(unsigned int weight) {
    unsigned int num = 0;
    unsigned int product = 1;

    if (weight==0){
        return 1;
    }
    else{
        while(num < weight){
            product = product * 16;
            num++;
       }
        return product;
    }
    
}

/*********************************************************************************************
 * This function maps and creates file metadata												 *		                     *
 * Preconditions: 																			 *
 * @params none																	 		     *
 * Postconditions:  																		 *
 * @return char* 																			 *
 *********************************************************************************************/

unsigned char * create_meta() {

    BYTE *myByte;

    unsigned char * fileName;
    unsigned char * fileExt;
    unsigned char * directory;
    unsigned short fileTime;
    unsigned short fileDate;
    unsigned short startCluster;
    unsigned int fileSize;

   
    static unsigned char metaData[] = {0x00};

    //insert 0-7
    fileName = find_fileName();//This is metadata 0-7
    myByte = fileName;
    metaData[0] = myByte[0];
    metaData[1] = myByte[1];
    metaData[2] = myByte[2];
    metaData[3] = myByte[3];
    metaData[4] = myByte[4];
    metaData[5] = myByte[5];
    metaData[6] = myByte[6];
    metaData[7] = myByte[7];
    
    //insert 8-10
    fileExt = find_fileExt();//This is metadata 8-10 
    myByte=fileExt;
    metaData[8] = myByte[0];
    metaData[9] = myByte[1];
    metaData[10] = myByte[2];

    //insert 11
    directory = directory_check();//This is metadata 11
    metaData[11] = directory;//0 for file. 1 for directory


    //insert 14-15
    fileTime = time_encode();
    printf("Here is the time of creation:%x\n" , fileTime);
    myByte=&fileTime;
    metaData[14] = myByte[0];
    metaData[15] = myByte[1];

   //insert 22-23 (last modify time was same as creation)
    metaData[22] = myByte[0];
    metaData[23] = myByte[1];
  

    //insert 16-17
    fileDate = date_encode();
    printf("Here is the time of creation:%x\n" , fileDate);
    myByte=&fileDate;
    metaData[16] = myByte[0];
    metaData[17] = myByte[1];

    //insert 18-19 (last access date was same as creation)
    metaData[18] = myByte[0];
    metaData[19] = myByte[1];

 
    //insert 24-25 (last modify date was same as creation)
    metaData[24] = myByte[0];
    metaData[25] = myByte[1];

    //insert 26-27
    startCluster = find_emptyCluster(0);//This is metadata 26 and 27
    printf("\n\n%04x\n\n" , startCluster);
    myByte = &startCluster;
    metaData[26] = myByte[0];
    metaData[27] = myByte[1];


    //insert 28-31
    fileSize = find_fileSize(metaData); //This is metadata 28-31 (the end of the RD entry)
    myByte = &fileSize;
    metaData[28] = myByte[0];
    metaData[29] = myByte[1];
    metaData[30] = myByte[2];
    metaData[31] = myByte[3];

    int index = 0;
    while(index < 32 ){
    
        printf("Metadata%d:%02x\n" , index , metaData[index] );
        index++;

    }

    printf("done meta\n");

    return metaData;

}

unsigned char * find_fileName() {



    unsigned char name[8] = {20};

    
    printf("Name of file(only the first 8 characters will be accepted): ");
    scanf("%08s" , name); //%08s keeps the limit to 8

    printf("%s\n " , name);
    


    return name;

}

unsigned char * find_fileExt() {
    unsigned char string[3] = {0x20};

    
    int choice;
    int amount;

    printf("(1).cop(copy) \t Copies bytes from outside file to this drive \n(2).gbg(gobbledygook) \t Random bytes using given file size \n(3).dir(directory) \t Makes directory\n");
    scanf(" %d" , &choice);
    
    if(choice == 1) {
        string[0]='c';
        string[1]='o';
        string[2]='p';
    }
    else if(choice ==2) {
        string[0]='g';
        string[1]='b';
        string[2]='g';
    }
    else if(choice ==3) {
        string[0]='d';
        string[1]='i';
        string[2]='r';

    }
    else{
        printf("\n\nInvalid choice\n\n");
        find_fileExt();
    }


    return string ;
}

unsigned char directory_check() {

    int directory;

    printf("Is this a file or directory?\n1)File\n2)Directory\n");

    scanf("%d" , &directory);

    if(directory == 1) {
        return 0x00;
    }
    else if (directory == 2){
        return 0x01;
    }
    else{
        printf("Invalid choice try again\n");
        return directory_check();
    }

}

unsigned short time_encode() {

    time_t now;
    struct tm* now_tm;
    now = time(NULL);
    now_tm = localtime(&now);
    unsigned int hour = now_tm->tm_hour;
    unsigned int min = now_tm->tm_min;
    unsigned int sec= now_tm->tm_sec;
    unsigned int year = (now_tm->tm_year) - 100;
    unsigned int month = (now_tm->tm_mon) + 1;
    unsigned int day = now_tm->tm_mday;

    
    hour = (hour<<11);
    min = (min<<5);
    sec= (sec/2) ;


    printf("hour:%04x\n" , hour);
    printf("min:%04x\n" , min);
    printf("sec:%04x\n" , sec);

    unsigned short value = ((hour+min+sec));

    printf("True time encoding:%04x\n" , value );

    return value;

}

unsigned short date_encode() {

    time_t now;
    struct tm* now_tm;
    now = time(NULL);
    now_tm = localtime(&now);
    unsigned int hour = now_tm->tm_hour;
    unsigned int min = now_tm->tm_min;
    unsigned int sec= now_tm->tm_sec;
    unsigned int year = (now_tm->tm_year) - 100;
    unsigned int month = (now_tm->tm_mon) + 1;
    unsigned int day = now_tm->tm_mday;

    printf("ASL;KDFJASKLDFJASKL;FDJ\n");
    
    year = (year + 2000);
    year = (year - 1980);
    printf("RThis the year%d\n" , year);
    year = (year <<9);
    month = (month <<5);


    printf("year:%04x\n" , year);
    printf("month:%04x\n" , month);
    printf("day:%04x\n" , day);


    unsigned short value = (year+month+day);

    printf("True time encoding:%04x\n" , value );

    return value;

}

unsigned int find_fileSize(char * metaData) {
    //since this is the last part of the RD entry we can start doing the actual data writing here
    //plus we need the data to know the file size
    
    unsigned int size = 0;
    unsigned char fileExt[3];

    fileExt[0] = metaData[8];
    fileExt[1] = metaData[9];
    fileExt[2] = metaData[10];


    if (fileExt[0]=='c' &&  fileExt[1]=='o' && fileExt[2]=='p' ) {
        printf("\n\n\n Hello \n\n\n");
        size = copy_file(metaData);
    }
    else if(fileExt[0]=='g' &&  fileExt[1]=='b' && fileExt[2]=='g' ) {
        printf("\n\n\n World \n\n\n");
        size = gobbledygook(metaData); 
    }
    else if(fileExt[0]=='d' &&  fileExt[1]=='i' && fileExt[2]=='r' ) {
        printf("\n\n\n Hello World Directoryn\n\n");
        size = 10*BPS;//size of the RD since dir is just a RD inside RD
    }
    else{
        printf("\n\n\n file ex:?%s \n%c\n%c\n%c" , fileExt, fileExt[0], fileExt[1], fileExt[2]);
    }

    printf("\nHere is the size:\n%x | %d\n" , size, size );

    return size;

}

unsigned short find_emptyCluster(unsigned short index) {



    while(file_allocation_table[index] != 0x0000 )         
        index++; //each FAT entry is 2 bytes in size which is a short


    return index;

}

unsigned int text_create() {

    char text[512];

    printf("What text do you want to write: ");

    char * pointer = text;

    return 2000;

}

unsigned int gobbledygook(char * metaData) {


    unsigned short index = 0;
    unsigned int amount = 0;
    unsigned int size = 0;

    struct timespec ts; //creating a timepsec called ts
    
    srand((time_t)ts.tv_nsec); //new seed every loop based off of nano seconds

    printf("How many random chars do you want: ");
    scanf("%d" , &size);

    //let's set the index to the starting cluster which we found earlier 
    index = (convertShort(metaData) - 2);
    printf("I'm going to start writing here:%d\n" , index);
    index = ((index*BPS*SPC));
    printf("Actually I'm going to start writing here:%d\n" , index);



    amount= size + index;


    while(index < amount) {
        unsigned char randomLetter = ((rand() % 26) + 65);
        data_section[index] = randomLetter;
        index++;
    }
    return size;
    
    
}

unsigned short convertShort(char * metaData){

    printf("we made it bro:%c\n" , metaData[26]);
    unsigned int weight = 0;
    unsigned int index = 26;//where the startCluster starts in the metadata array
    unsigned short startCluster = 0;

    char temp;

    while(weight < 4)  {
        temp = metaData[index];
        startCluster = startCluster + ((temp & 0x0F) * power(weight));
        printf("\n%x\n" , (temp >> 4));
        printf("\n%d\n" , startCluster);
        weight++;
        temp = metaData[index];
        startCluster = startCluster + ((temp >> 4)*power(weight));
        printf("\n%x\n" , (temp >> 4));
        printf("\n%d\n" , startCluster);
        index++;
        weight++;
    }

    printf("\n\n\tHere's your starting Cluster! %d\n" , startCluster);

    return startCluster;

}

unsigned int convertInt(char * metaData) {
    
    int weight = 0;
    int index = 28;
    unsigned int size = 0;

    unsigned char temp;

    while(weight < 8) {
        temp = metaData[index];
        size = size + ((temp & 0x0F)*power(weight));//This masks out the left 4 bits 
        printf("\n%x\n" , (temp >> 4));
        printf("\n%d\n" , size);
        weight++;
        temp = metaData[index];
        size = size + ((temp >> 4)*power(weight));
        printf("\n%x\n" , (temp >> 4));
        printf("\n%d\n" , size);
        index++;
        weight++;
    }

    return size;

}

void create_RD_entry(unsigned char * metaData , unsigned short directory_location){ //doesn't need return since the RD is global



    if(directory_location ==0){//RD 


        int RD_check = 0;

        printf("Made it to create_rd_entry\n");
        
        //finds the next empty RD entry
        while(root_directory[RD_check] != 0) {
            printf("RD iteration\n");
            RD_check+=BPRD;
        }

        //makes it so index offset won't affect the copying from the metadata array
        int index = RD_check; 
        int mindex = 0;

        while(index < (RD_check +  BPRD)) { 
            printf("Copying into index: %d from: %d\n" , index , mindex);
            root_directory[index] = metaData[mindex] ;
            index++; mindex++; 
        }

    }

    else {
        int directory_check = 0;
       fseek(drive , ((data_offset + ((directory_location-2)*BPS*SPC))) , SEEK_SET) ;//seeks me to where this directory is on disk
       unsigned char byte;
       int num;
       int index;
       byte  = fgetc(drive);
       while( byte!=0 ) {
           num = 0;
           while(num < 32){//jump to next entry 
            byte = fgetc(drive);
            num++;
           
           }
       }
       index = ftell(drive) - data_offset - 1;
       int mindex = 0;
       int end = index+32;
       while(mindex<32){
        data_section[index] = metaData[mindex];
        index++; mindex++;
        }
    }
}

void write_to_disk(FILE * drive) {



    fseek(drive , 0*BPS , SEEK_SET);//seek to sector 0 
    fwrite(volume_boot_record, sizeof(unsigned char) , 1 * BPS  , drive); 


    fseek(drive , 1*BPS , SEEK_SET);//seek to sector uno 
    fwrite(file_allocation_table , sizeof(unsigned char) , 10 * BPS  , drive); 


    fseek(drive , 11*BPS , SEEK_SET);//seek to sector 11 since fat is 10 sectors big 
    fwrite(root_directory, sizeof(unsigned char) , 10 * BPS  , drive); 


    fseek(drive , 21*BPS , SEEK_SET);//seek to sector 21 since rd is 10 sectors big 
    fwrite(data_section, sizeof(unsigned char) , 500 * BPS * SPC, drive); 

}

