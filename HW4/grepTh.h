#include <stdio.h>
#include <stdlib.h> /*For exit and malloc*/
#include <string.h>
#include <unistd.h> /* for fork */
#include <sys/wait.h>
#include <dirent.h> /*Directory Access*/
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h> /*Path max*/
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/time.h>

int readFromFile( const char * filename , const char *string , const char * filePathName);
int readFromDirectory(  char * path , const char * string , const char*argv);
int size(const char * filename);
int isDirectory(const char * path);
int string_length(const char * str);
void copystr(char* destination,  const char* source);
char* concat(char* destination, const char* source);
int compare( const char * str1, const char * str2);
void * grepFileThreads(void *grep_data);
int countlines(const char * filename);
static void signal_handler(int signal_number);

#ifndef PATH_MAX
	#define PATH_MAX 255
#endif
#define BUFFER 512
#define DIR_MAX 100
#define FIFO_PERM S_IRUSR|S_IWUSR
#define FIFONAME "fifoname"
#define OUTPUT "log.txt"

static void signal_handler(int signal_number);

static volatile sig_atomic_t mainPid;
//char fifoArr[DIR_MAX][PATH_MAX];
//int t=0;
int count1,count2;
static int lines;

sem_t semlock; /*Semaphore pointer*/
sem_t semlock_t; /*Semaphore pointer*/


/*Verilen pathin directory olup olmadigi kontrolu*/
int isDirectory(const char * path){
	struct stat stat_array;

	if(stat(path , &stat_array) == -1 ){
		return 0;
	}
	else{
		return S_ISDIR(stat_array.st_mode);
	}
}
/*Bu fonksiyon bir stringi diger stringe ekleme yapar. */
char* concat(char* destination, const char* source){
	int i,j;
	
	
	for(i=0;destination[i] !='\0';i++);
	for(j=0;source[j] !='\0'; j++){
		destination[j+i] =source[j];
	}
	destination[j+i]='\0';
	
	return destination;
}
/*Iki string arasinda karsilastirma yapar.*/
int compare( const char * str1, const char * str2){
	int i,j;
	for(i=0;str1[i] !='\0';i++){
		for(j=0;str2[j] !='\0'; j++){
			if(i==j){
				
				if(str1[i]>str2[j]){
					return -1;
				}
				else if(str1[i]<str2[j]){
					return 1;
				}				
			}
		}
	}
	
	return 0;
}
/*stringi kopyalayip baska stringe ekler.*/
void copystr(char* destination, const char* source){
	
	int i;
	
	for(i=0;source[i] !='\0';i++){
	
		destination[i]=source[i];
	
	}
	
	destination[i]='\0';

}
/* Dosyadaki karakter boyutunu hesaplamak icin kullandik.*/
int size( const char * filename){

	int count = 0;
	FILE *input_file;
	
	input_file = fopen(filename,"r"); /* Dosya acma*/
	
	if(input_file == NULL){ /* Eger verilen dosya bulunamadıysa hata basiyor.*/
		
		perror("Could not open file.\n");
		
		exit(1);
	}
	
	lines += countlines(filename);
	
	fseek(input_file, 0, 2); /* Dosyadaki karakter boyutunu ogrendik.*/
	count = ftell(input_file);
	
	fclose(input_file);

	return count;
}
/* Verilen stringin uzunlugunu olçmek icin kullandik. */
int string_length(const char * str){

	int i=0;

	for(;str[i] != '\0' ; ++i);

	return i;

}