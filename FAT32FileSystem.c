/*
	Jacob Hobosn
	Vannessa Myron
	Cameron Heffelfinger
	
	4/30/20
	Fat32 File System Navigation program

	Division of Labor:
	Implimentation for Instruction intake: Jacob
	exit: Jacob
	info: Jacob
	size: Jacob
	ls: Vanessa
	cd: Vanessa
	create:Jacob  
	mkdir: Jacob
	mv: Jacob
	open: Vanessa
	close: Vanessa
	read: Not Implemented
	write: Not Implemented
	rm: Cameron/Jacob
	cp: Cameron/Jacob

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>


typedef struct
{
    char** tokens;
    int numTokens;
} instruction;

typedef struct{
	unsigned char DIR_name[11];
	unsigned char DIR_Attributes;
	unsigned char DIR_NTRes;
	unsigned char DIR_CrtTimeTenth;
	unsigned short DIR_CrtTime;
	unsigned short DIR_CrtDate;
	unsigned short DIR_LstAccDate;
	unsigned short DIR_FstClusHI;
	unsigned short DIR_WrtTime;
	unsigned short DIR_WrtDate;
	unsigned short DIR_FstClusLO;
	unsigned int DIR_FileSize;
	
} __attribute__((packed)) DirEntry;


struct FileFAT{
int cluster;
char fileName[50];
char fileMode[10];
struct FileFAT *next;
struct FileFAT *previous;
 };


typedef struct {
	unsigned char name[100];
	unsigned int curr_clust_num;
	char curr_clust_path[50];
	char curr_path[50][100];
	int curr;
}__attribute__((packed)) ENVIR;

// Global Variables
ENVIR environment;
int FirstDataSector;
struct FileFAT *head = NULL; 
struct FileFAT *ptr;
struct FileFAT *secondPtr = NULL;
int f;

unsigned short BPB_BytsPerSec;	
unsigned char BPB_SecPerClus;	
unsigned short BPB_RsvdSecCnt;	
unsigned char BPB_NumFATs;	
unsigned int BPB_TotSec32;
unsigned int BPB_FATSz32;	
unsigned int BPB_RootClus;	

//Functions 

void addToken(instruction* instr_ptr, char* tok);
void printTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
int firstSectorOfCluster(unsigned int N);
int size(char* arg, int image);
void create(char* arg, int image);
void mkdir(char* arg, int image);
int nextEmptyClus(int image);
void ls(int image, unsigned int clusNum);
void lsName(char *name,int image, unsigned int clustNum);
int cd(char *name, int image, unsigned int dirClustNum);
void pathAppend(int curr_clusterNum,char * pathName);
void deleteAppend();
void fileOpen(int image, char *fileName, char *mode);
void addFile(int image, char *fileName, char *fileMode);
void closeFile(char *fileName);
void mv(char* arg1, char* arg2, int image);
void createEmptyDirEntry(int image, unsigned int offSet);
void rm(int image, unsigned int clusNum, char* fileName);

int main() {
	
	
	int f = open("fat32.img", O_RDWR);
	pread(f, &BPB_BytsPerSec, 2, 11);
	pread(f, &BPB_SecPerClus, 1, 13);
	pread(f, &BPB_RsvdSecCnt, 2, 14);
	pread(f, &BPB_NumFATs, 1, 16);
	pread(f, &BPB_TotSec32, 4, 32);
	pread(f, &BPB_FATSz32, 4, 36);
	pread(f, &BPB_RootClus, 4, 44);
	

	




	 environment.curr = 0;  // initialize the tracker
 		char pathName[2];
 		strcpy(pathName, "/Root");// for the rootcluster
         pathAppend(BPB_RootClus,pathName);	
	//calculate the FirstDataSector
	FirstDataSector = BPB_RsvdSecCnt + (BPB_NumFATs * BPB_FATSz32);
	
	
		//Variables for Intake
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
	
	

	while (1) {
		// for the current environment info
		char *user = getenv("USER");
		char *machine = getenv("MACHINE");
		char *pwd = getenv("PWD");
			
		printf("%s@%s:%s", user, machine, pwd);

			int j = 0;
		while(j < environment.curr)
		{
			if(j == 0){
		        printf("%s/", "");
			}
			else
			{
			printf("%s/", environment.curr_path[j]);
			}
			j++;
		}
		printf("> ");


		// loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;

			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;
				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		addNull(&instr);
		
		//Add functions here
		
		
		
		if(strcmp(instr.tokens[0], "exit") == 0){
			if(close(f) != 0){
				printf("There was a problem close fat32.img\n");
			}
			else{
				clearInstruction(&instr);
				printf("Successfully Exited\n");
			}
			break;
		}

		if(strcmp(instr.tokens[0], "info") == 0) {
			printf("%s%hu\n", "Bytes per sector: ", BPB_BytsPerSec);
			printf("%s%u\n", "Sectors per cluster: ", BPB_SecPerClus);	
			printf("%s%hu\n", "Reserved sector count: ", BPB_RsvdSecCnt);	
			printf("%s%u\n", "Number of FATs: ", BPB_NumFATs);	
			printf("%s%u\n", "Total sectors: ", BPB_TotSec32);	
			printf("%s%u\n", "FAT size: ", BPB_FATSz32);
			printf("%s%u\n", "Root Cluster: ", BPB_RootClus);
		}
		
		if(strcmp(instr.tokens[0], "size") == 0) {
			int s;
			s = size(instr.tokens[1], f);
			if(s >= 0)
				printf("%u\n", s);  
		}
		
		
		if(strcmp(instr.tokens[0], "creat") == 0){
			create(instr.tokens[1], f);
		}

		if(strcmp(instr.tokens[0], "mkdir") == 0){
			mkdir(instr.tokens[1], f);
		}
		
		if(strcmp(instr.tokens[0], "mv") == 0){
			if(instr.tokens[2] == NULL)
				printf("Invalid command: second argument required\n");
			else
				mv(instr.tokens[1], instr.tokens[2], f);
		}
		
		if(strcmp(instr.tokens[0], "ls") == 0)
 		{
 		if(instr.tokens[1] != NULL && strcmp(instr.tokens[1], ".") !=0)
 			{	

 				if(strcmp(instr.tokens[1], "..") == 0)
 				{
 					if(environment.curr -2 == 0)
 					{
 					      ls(f, BPB_RootClus);
 					}
 					else		
 					  ls(f, environment.curr_clust_path[environment.curr-2]);

 				}
 				else 	
 				{// I would need the cd function for this 
 		                lsName(instr.tokens[1], f, environment.curr_clust_num);

		}
 			} 
 		else
                 {
                  ls(f,environment.curr_clust_num);
                 }



 		}

 		if(strcmp(instr.tokens[0], "rm") == 0){
 			rm(f, environment.curr_clust_num, instr.tokens[1]);
    	}

    	/*
		if(strcmp(instr.tokens[0], "cp") == 0){
 			if(instr.tokens[1] == NULL){	//No Filename provide, print error
 				printf("Error, no filename provided");
 			}
 			else if(instr.tokens[2] == NULL){	//Case of no TO arg
 				cp(f, instr.tokens[1],  "TO");
 			}
 			else{							//Case of TO arg
 				cp(f, instr.tokens[1], instr.tokens[2]);
 			}
 		}
		*/
		if(strcmp(instr.tokens[0], "cd") == 0){
			if(instr.tokens[1] == NULL){
			printf("Error: No argument\n");

			}
		else{
		 	if(strcmp(instr.tokens[1], ".") == 0){

			 // nothing should be done
			
			}
			else if(strcmp(instr.tokens[1], "..") == 0){
			deleteAppend();
			}
		
			else{
				int getNewCluster = cd(instr.tokens[1], f, environment.curr_clust_num);
				if(getNewCluster != environment.curr_clust_num && getNewCluster != -1){
				pathAppend(cd(instr.tokens[1], f, environment.curr_clust_num), instr.tokens[1]);
				}
		
			 }

	        }
		
		}
		
		if(strcmp(instr.tokens[0], "open") == 0){
		if(instr.tokens[1] == NULL)
		{
		printf("Error: please enter a file name and the mode.\n");
		
		}
		else if(instr.tokens[2] == NULL)
		{
		 printf("Error: please enter a mode.\n");

		}
		else{
		fileOpen(f, instr.tokens[1], instr.tokens[2]);
		}

		}
		if(strcmp(instr.tokens[0], "close") == 0){
			if(instr.tokens[1] == NULL){
			  printf("Error: Please write a filename.\n");
			}
			else 
			{
			closeFile(instr.tokens[1]);
			}
		}

		clearInstruction(&instr);
	}
	
	
	
	



    return 0;
}

void addToken(instruction* instr_ptr, char* tok)
{
    //extend token array to accomodate an additional token
    if (instr_ptr->numTokens == 0)
        instr_ptr->tokens = (char**) malloc(sizeof(char*));
    else
        instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

    //allocate char array for new token in new slot
    instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
    strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

    instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
    //extend token array to accomodate an additional token
    if (instr_ptr->numTokens == 0)
        instr_ptr->tokens = (char**)malloc(sizeof(char*));
    else
        instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

    instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
    instr_ptr->numTokens++;
}

void printTokens(instruction* instr_ptr)
{
    int i;
    printf("Tokens:\n");
    for (i = 0; i < instr_ptr->numTokens; i++) {
        if ((instr_ptr->tokens)[i] != NULL)
            printf("%s\n", (instr_ptr->tokens)[i]);
    }
}

void clearInstruction(instruction* instr_ptr)
{
    int i;
    for (i = 0; i < instr_ptr->numTokens; i++)
        free(instr_ptr->tokens[i]);

    free(instr_ptr->tokens);

    instr_ptr->tokens = NULL;
    instr_ptr->numTokens = 0;
}

// Obtain the first sector of clusters 
int firstSectorOfCluster(unsigned int N)
{
	int firstSectorofCluster = BPB_BytsPerSec * (FirstDataSector + (N - 2) * BPB_SecPerClus);
	return firstSectorofCluster;

}


int size(char* arg, int image){
	
	DirEntry tempDir;
	unsigned int x = environment.curr_clust_num;	//First cluster we check
	unsigned int byteOffSet;
	unsigned char firstBit;
	
	
	char* space = " ";
	int i;
	for(i = strlen(arg); i < 11; i++){
		strcat(arg, space);
	}
	
	
	while(x < 0x0FFFFFF8){
		
		byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( x - 2) * BPB_SecPerClus);
		
		int i;
		
		do{

			pread(image, &tempDir, sizeof(DirEntry), byteOffSet);	//intake entire dir entry
			if(tempDir.DIR_Attributes != 15){
				
				
				//Print size if name matches of directory matches arg
				if(strncmp(tempDir.DIR_name, arg, 11) == 0)
					return tempDir.DIR_FileSize;
		}
			byteOffSet += 32;
		}while(tempDir.DIR_name[0] != 0);
		
		
		pread(image, &x, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + x * 4);	//Should read the value at the current cluster number and make it the new curr cluster number
		
	}
	
	printf("File does not exist\n");
	return -1;
	
}

void create(char* arg, int image){
	
	
	DirEntry temp;
	unsigned int eofValue = 0x0FFFFFF8;
	unsigned int tempClus;
	
	tempClus = nextEmptyClus(image);
	
	//printf("%s%i\n", "EmptyClus Found: ", tempClus);
	
	//Write to empty clus number EOFValue
	pwrite(image, &eofValue, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + tempClus * 4);
	
	int i;
	for(i = 0; i < strlen(arg); i++){
		temp.DIR_name[i] = arg[i];
	}
	
	//Pad name with spaces
	for(i = strlen(arg); i < 11; i++){
		temp.DIR_name[i] = 32;
	}
	
	unsigned int byteOffSet;
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( tempClus - 2) * BPB_SecPerClus);
	temp.DIR_FstClusLO = 0xFFFF & tempClus;
	temp.DIR_FstClusHI = (tempClus >> 16) & 0xFFFF;
	temp.DIR_FileSize = 0;
	temp.DIR_NTRes = 0;
	temp.DIR_CrtTimeTenth = 0;
	temp.DIR_CrtTime = 0;
	temp.DIR_CrtDate = 0;
	temp.DIR_LstAccDate = 0;
	temp.DIR_WrtTime = 0;
	temp.DIR_WrtDate = 0;
	
	//Putting the actual directory entry into the directory (What if the directory is full?)
	
	
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( environment.curr_clust_num - 2) * BPB_SecPerClus);
	DirEntry itr;
	
	//Read throught the directory until an unnocupied space is found
	while(1){
		pread(image, &itr, sizeof(itr), byteOffSet);
		if(itr.DIR_name[0] == 0){
			//Write new file into empty space
			pwrite(image, &temp, sizeof(temp), byteOffSet);
			return;
		}
		byteOffSet += 32;
	};
}

void mkdir(char* arg, int image){
	
	
	DirEntry temp;
	unsigned int eofValue = 0x0FFFFFF8;
	unsigned int tempClus;
	
	tempClus = nextEmptyClus(image);
	
	//printf("%s%i\n", "EmptyClus Found: ", tempClus);
	
	//Write to empty clus number EOFValue
	pwrite(image, &eofValue, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + tempClus * 4);
	
	int i;
	for(i = 0; i < strlen(arg); i++){
		temp.DIR_name[i] = arg[i];
	}
	
	//Pad name with spaces
	for(i = strlen(arg); i < 11; i++){
		temp.DIR_name[i] = 32;
	}
	
	 
	
	//Putting the actual directory entry into the directory (What if the directory is full?)
	
	unsigned int byteOffSet;
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( tempClus - 2) * BPB_SecPerClus);
	
	temp.DIR_FstClusLO = 0xFFFF & tempClus;
	temp.DIR_FstClusHI = (tempClus >> 16) & 0xFFFF;
	temp.DIR_Attributes = 0x10;
	temp.DIR_FileSize = 0;
	temp.DIR_NTRes = 0;
	temp.DIR_CrtTimeTenth = 0;
	temp.DIR_CrtTime = 0;
	temp.DIR_CrtDate = 0;
	temp.DIR_LstAccDate = 0;
	temp.DIR_WrtTime = 0;
	temp.DIR_WrtDate = 0;
	
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( environment.curr_clust_num - 2) * BPB_SecPerClus);
		
	DirEntry itr;
	
	//Read throught the directory until an unnocupied space is found
	while(1){
		pread(image, &itr, sizeof(itr), byteOffSet);
		if(itr.DIR_name[0] == 0){
			//Write new file into empty space
			pwrite(image, &temp, sizeof(temp), byteOffSet);
			break;
		}
		byteOffSet += 32;
	};
	
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( environment.curr_clust_num - 2) * BPB_SecPerClus);
	
	//Create parent direntry and enter values to cluster
	DirEntry parent;
	
	strncpy(parent.DIR_name, "..         ", 11);
	parent.DIR_FstClusLO = 0xFFFF & environment.curr_clust_num;
	parent.DIR_FstClusHI = (environment.curr_clust_num >> 16) & 0xFFFF;
	parent.DIR_Attributes = 0x10;
	parent.DIR_FileSize = 0;
	byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( tempClus - 2) * BPB_SecPerClus);
	pwrite(image, &parent, sizeof(parent), byteOffSet);
	
	//Insert current directory into cluster
	strncpy(temp.DIR_name, ".          ", 11);
	pwrite(image, &temp, sizeof(temp), byteOffSet + 32);
}

void ls(int image, unsigned int clusNum){
	unsigned int byteOffSet;
	byteOffSet = firstSectorOfCluster(clusNum);
	//BPB_BytsPerSec * (FirstDataSector + ( clusNum - 2) * BPB_SecPerClus);
	DirEntry tempDir;
	int i;
	for(i = 0; i < 16; i++){
		
		
			pread(image, &tempDir, sizeof(DirEntry), byteOffSet);	//intake entire dir entry
			if(tempDir.DIR_Attributes != 15 && tempDir.DIR_name[0] != 0 && tempDir.DIR_name[0] != 46){
				int j;
				for(j = 0; j < 11; j++){
					if(tempDir.DIR_name[j] != ' ')
						printf("%c", tempDir.DIR_name[j]);
				}
				printf(" ");
				
			}
			byteOffSet += 32;
	}
	printf("\n");
}

void lsName(char *name,int image, unsigned int clustNum)
{
	int getCluster = cd(name, image, clustNum);
	if(clustNum != getCluster && getCluster != -1){
	ls(image, getCluster);
	}

}


int cd(char *name, int image, unsigned int dirClustNum)
{

        DirEntry tempDir;
        unsigned int x = environment.curr_clust_num;    //First cluster we check
        unsigned int byteOffSet;
	int clustNum = dirClustNum;
        char* space = " ";
        int i;
        for(i = strlen(name); i < 11; i++){
                strcat(name, space);
        }


        while(x < 0x0FFFFFF8){

                byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( x - 2) * BPB_SecPerClus);

                int i;

                do{

                        pread(image, &tempDir, sizeof(DirEntry), byteOffSet);   //intake entire dir entry
                        if(tempDir.DIR_Attributes != 15){
                                //Print size if name matches of directory matches arg
                                if(tempDir.DIR_Attributes & 0x10 && strncmp(tempDir.DIR_name, name, 11) == 0)
                                        return 0x100 * tempDir.DIR_FstClusHI + tempDir.DIR_FstClusLO;
                }
                        byteOffSet += 32;
                }while(tempDir.DIR_name[0] != 0);


                pread(image, &x, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + x * 4);   //Should read the value at $
		dirClustNum = x;
		
        }
	if(x >= 0x0FFFFFF8)
	{
	printf("Not a directory/ Directory does not exist\n");
	return -1;
	}
return dirClustNum;	
}

int nextEmptyClus(int image){
	
	unsigned int tempClus = BPB_RootClus;
	unsigned int clusValue;
	
	do{
		tempClus++;
		
		
		pread(image, &clusValue, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + tempClus * 4);

		//printf("%s%u%s%u\n", "Cluster Number: ", tempClus, "     Value in cluster", clusValue);
	}while(clusValue != 0);
	
	return tempClus;
	
}
void pathAppend(int curr_clusterNum,char * pathName)
 {	int j;
	char tempName[11];
 	environment.curr_clust_num = curr_clusterNum;
		for(j = 0;j < 11; j++)
		{
            if(pathName[j] != ' '){
				tempName[j] = pathName[j];  
			}

		} 
 	strcpy((char *)environment.name, tempName);
 	strcpy(environment.curr_path[environment.curr], tempName); // add the name to the current path name
  
	environment.curr_clust_path[environment.curr] = curr_clusterNum;
	environment.curr++; // update

 }


void deleteAppend(){
	if(environment.curr > 1){
	environment.curr_clust_num = environment.curr_clust_path[environment.curr-2];
	strcpy((char*)environment.name, environment.curr_path[environment.curr -2]);
	environment.curr--;

	 }

}


void fileOpen(int image, char *fileName, char *mode)
{
  DirEntry tempDir;
        unsigned int x = environment.curr_clust_num;    //First cluster we check
        unsigned int byteOffSet;
        int clustNum;
        char* space = " ";
        int i;
        for(i = strlen(fileName); i < 11; i++){
                strcat(fileName, space);
        }

	if(strcmp(mode, "w") != 0 && strcmp(mode, "r") != 0 && strcmp(mode, "wr") != 0 && strcmp(mode, "rw") != 0){
		printf("Incorrect mode. Please use one of these options: w, r, rw, wr \n");
	}
	
	else if(OpenFile(fileName) == 1)
	{
		printf("File is already opened.\n");
	}
	
    else{	
        while(x < 0x0FFFFFF8){

			byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( x - 2) * BPB_SecPerClus);

			int i;

			do{

					pread(image, &tempDir, sizeof(DirEntry), byteOffSet);   //intake entire dir entry
					if(tempDir.DIR_Attributes != 15){
							//Print size if name matches of directory matches arg
						if(strncmp(tempDir.DIR_name, fileName, 11) == 0){
							
							if(tempDir.DIR_Attributes & 0x10){
								printf("Unable to open directory\n");
							}
							else{ 
								printf("File opened in %s mode \n", mode);
								addFile(image,fileName, mode);	
							}		
							return;
						}
					}
					byteOffSet += 32;
			}while(tempDir.DIR_name[0] != 0);


			pread(image, &x, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + x * 4);   //Should read the value at $
		   
		}
		printf("File not found\n");
 	}
}

int OpenFile(char *file_name)
{

struct FileFAT *ptr;

	for(ptr = head; ptr !=NULL; ptr = ptr->next){
		if(strncmp(ptr->fileName, file_name,11) == 0)
		{				
			return 1;
		}

	}
	return 0;
}

void addFile(int image, char *fileName, char *fileMode)
{
	int size = sizeof(struct FileFAT);
	struct FileFAT *ptrTemp = calloc(1, size);
	strcpy(ptrTemp->fileName, fileName);
	strcpy(ptrTemp->fileMode, fileMode);
	ptrTemp->next = NULL;
	
	if(head == NULL){
		head = ptrTemp;
		ptrTemp->previous = head;
	}
	else {
		struct FileFAT *ptr = head;
		while(ptr->next != NULL){
			ptr->previous = ptr;
			ptr = ptr->next; // iterate 
		}
		ptr->next = ptrTemp;
	}
}

void closeFile(char *fileName){
	char* space = " ";
			int i;
			for(i = strlen(fileName); i < 11; i++){
					strcat(fileName, space);
			}

	if(OpenFile(fileName) == 0)
	{

	printf("File is not opened.\n");

	}
	else
	{
		for(ptr = head; ptr !=NULL; secondPtr = ptr)
		{
			if(strncmp(ptr->fileName, fileName, 11)==0)
			{
				if(secondPtr != NULL)
				{
					secondPtr->next = ptr->next;
				}
				else
				{
				  head = ptr->next;


				}
				free(ptr);
				ptr = ptr->next;
				printf("File Closed\n");
				return;
			}
		}

	}

}

void mv(char* arg1, char* arg2, int image){
	
	DirEntry tempDir;
	DirEntry arg1Dir;
	unsigned int arg1OffSet;
	DirEntry arg2Dir;
	unsigned int arg2OffSet;
	unsigned int x = environment.curr_clust_num;	//First cluster we check
	unsigned int byteOffSet;
	bool arg2Exists;
	
	
	char* space = " ";
	int i;
	for(i = strlen(arg1); i < 11; i++){
		strcat(arg1, space);
	}
	
	for(i = strlen(arg2); i < 11; i++){
		strcat(arg2, space);
	}
	
	while(x < 0x0FFFFFF8){
		
		byteOffSet = BPB_BytsPerSec * (FirstDataSector + ( x - 2) * BPB_SecPerClus);
		
		int i;
		for(i = 0; i < 16; i++){
			
			pread(image, &tempDir, 32, byteOffSet);	//intake entire dir entry
				
			if(strncmp(tempDir.DIR_name, arg1, 11) == 0){
				;
				memcpy(&arg1Dir, &tempDir, 32);
				arg1OffSet = byteOffSet;
			}
			if(strncmp(tempDir.DIR_name, arg2, 11) == 0){
				memcpy(&arg2Dir, &tempDir, 32);//arg1Dir = tempDir;
				arg2OffSet = byteOffSet;
				arg2Exists = true;
			}
		
			byteOffSet += 32;
		}
		
		
		pread(image, &x, 4, BPB_RsvdSecCnt * BPB_BytsPerSec + x * 4);	//Should read the value at the current cluster number and make it the new curr cluster number
		
	}
	
	int k;
	int y;
	
	//Second argument doesn't exist then rename the first argument
	if(arg2Exists == false){
		int j;
		for(j = 0; j < 11; j++){
			arg1Dir.DIR_name[j] = arg2[j];
		}
		pwrite(image, &arg1Dir, 32, arg1OffSet);
	}
	
	//Both Files
	else if(arg1Dir.DIR_Attributes != 0x10 && arg2Dir.DIR_Attributes != 0x10){
		printf("The name is already being used by another file\n");
	}
	
	//First is a directory and second is a file
	else if(arg1Dir.DIR_Attributes == 0x10 && arg2Dir.DIR_Attributes != 0x10){
		printf("Cannot move directory: invalid argument destination\n");
	}
	
	
	else{
		
		DirEntry empDir;
		
				
		createEmptyDirEntry(image, arg1OffSet);	//Create a directory full of zeros
		
		unsigned int arg2Address;
		arg2Address = (arg2Dir.DIR_FstClusHI<<16)+arg2Dir.DIR_FstClusLO;	//compute address of cluster
		arg2Address = BPB_BytsPerSec * (FirstDataSector + ( arg2Address - 2) * BPB_SecPerClus);
		 
		while(1){
			
			pread(image, &empDir, 32, arg2Address);
			
			if(empDir.DIR_name[0] == 0){
				//Write new file into empty space
				pwrite(image, &arg1Dir, 32, arg2Address);	//Put the arg1 dirEntry into the first available space in arg2Address
				
				return;
			}
			arg2Address += 32;
		};
	}
	
	
	
}

void createEmptyDirEntry(int image, unsigned int offSet){
	DirEntry temp;
	
	int i;
	
	for( i = 0; i < 11; i++){
		temp.DIR_name[i] = 0;
	}
	
	temp.DIR_Attributes = 0;
	temp.DIR_NTRes = 0;
	temp.DIR_CrtTimeTenth = 0;
	temp.DIR_CrtTime = 0;
	temp.DIR_CrtDate = 0;
	temp.DIR_LstAccDate = 0;
	temp.DIR_FstClusHI = 0;
	temp.DIR_WrtTime = 0;
	temp.DIR_WrtDate = 0;
	temp.DIR_FstClusLO = 0;
	temp.DIR_FileSize = 0;
	
	pwrite(image, &temp, 32, offSet);
}


void rm(int image, unsigned int clusNum, char* fileName){
 	char* space = " ";
 	int flag = 0;
 	int i;
 	for(i = strlen(fileName); i<11; i++){	//Need to pad so its 11 characters long
 		strcat(fileName, space);
 	}

 	//First check file exists
 	DirEntry tempDir;
 	unsigned int byteOffset;
 	byteOffset = firstSectorOfCluster(clusNum);
 	do{
 		pread(image, &tempDir, sizeof(DirEntry), byteOffset);
 		if(tempDir.DIR_Attributes & 0x10){	//Tried removing a directory not file
 			printf("Cannot rm a directory\n");
 			return;
 		}
 		if(strncmp(tempDir.DIR_name, fileName, 11) == 0){

 			flag = 1;
 			break;
 		}
 		byteOffset += 32;

 	}while(tempDir.DIR_name[0] != 0);

 	if(flag == 0){	//Not found
 		printf("File doesn't exist\n");
 		return;
 	}

 	//If it reaches here the file was found


 	//Grab our address for the first cluster of data
 	unsigned short lo = tempDir.DIR_FstClusLO;
 	unsigned short hi = tempDir.DIR_FstClusHI;

 	//addr contains our first data cluster
 	unsigned int addr = (hi << 16) + lo;
 	
	//Effectively overwrites it with zeroes
 	createEmptyDirEntry(image, byteOffset);

 	unsigned int N;
 	unsigned int empty = 00000000;

 	N = ((addr - FirstDataSector)/(BPB_SecPerClus));
 	//offset is the addr
 	//Second arg unsigned int with 8 zeros
 	pread(image, &tempDir, sizeof(DirEntry), N);
}

