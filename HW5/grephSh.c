/*


			    CSE 244 System Programming  HW_5
					Serhat Guzel 131044015
					
					    grephSh.c

*/
/********************************LIBRARIES*************************************/
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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>

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
void numberOfDirectoryAndFile(const char *directory_path );

#ifndef PATH_MAX
	#define PATH_MAX 255
#endif
#define BUFFER 512
#define DIR_MAX 100
#define OUTPUT "log.txt"

static volatile sig_atomic_t mainPid;


sem_t *mySem; /*Semaphore pointer*/
int shm_id_one; /*Shared memory id*/
int shm_id_two; /*Shared memory id*/
int shmArrId;
int * sharedMemory[100];
int sharedSize;
key_t shm_key;
int s=0;
int *SharedMemCount; /*Integer pointer for SharedMemCount (with shared memory)*/

struct structGrep {
    char filePath[BUFFER];
    char stringName[PATH_MAX];
    
};

struct messageQueue {
	long mtype;
    int grep_number;
};
pthread_mutex_t mutex_ ;
int count;
int countThread;
pthread_t ptid ;
int numberOfDirectory, numberOfFile;
static int numberOfThread[100];
static int numberOfThreadSize=0;
FILE * opf; /* Log dosyasini acmak icin*/

/******************************MAIN FUNCTION***********************************/
int main(int argc , char *argv []){

	FILE * opf;
	
	int total;
	struct sigaction act;
	struct timespec start, end;
	long delta_us;
	int t;
    
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
	
	mainPid = getpid();

	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) {
        perror("Failed to SIGINT handler");
        return 1;
    }
    
	if(argc != 3){ /* Usage durumu: istenen argüman girilmeyince*/
		
		fprintf(stderr, "Usage : ./grepSh <dirname> <string>\n");
		return 1;
	}
	
	unlink(OUTPUT);
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);

	
	/*Get shared memory (semaphore)*/
    shm_id_one = shmget(IPC_PRIVATE, sizeof (sem_t), IPC_CREAT | 0644);

    /*Attach sahred memory (Semaphore)*/
    mySem = (sem_t*) shmat(shm_id_one, NULL, 0);

    /*Semaphore initialize*/
    sem_init(mySem, 1, 1) ;
     
    shmArrId = shmget(IPC_PRIVATE, sizeof (sem_t), IPC_CREAT | 0644);

    sharedMemory[100] = (int *) shmat(shmArrId , 0, 0);

    shm_id_two = shmget(IPC_PRIVATE , sizeof(int)  , IPC_CREAT | 0644);

    SharedMemCount = (int*) shmat(shm_id_two , NULL , 0);
        
    
	numberOfDirectoryAndFile(argv[1]);
	total = readFromDirectory( argv[1] , argv[2]  , argv[1]); 

	shmdt(SharedMemCount);
	shmdt(sharedMemory);
	shmctl(shmArrId , IPC_RMID , 0);

	shmctl(shm_id_two , IPC_RMID , 0);

	/*Semaphore destroy*/
    sem_destroy(mySem);

    /*Shared semophere memory detach*/
    shmdt(mySem);

    /*Release shared memory id*/
    shmctl(shm_id_one, IPC_RMID, 0);

    clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	
	delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
	
	opf = fopen(OUTPUT , "a");
	
	fprintf(opf, "\n\n\n %d %s were found in total.\n", total , argv[2]);
	
	printf( "\n\n\n %d %s were found in total.\n", total , argv[2]);
	printf( "Number of directories Searched: %d\n", numberOfDirectory);
	printf( "Number of files Searched: %d\n", numberOfFile);
	printf( "Number of lines Searched: %d\n", count);
	for(t=0; t<numberOfThreadSize ; ++t){
		printf("Number of cascade thread created: %d", numberOfThread[t]);
	}
	printf("\nMax # of threads running concurrently: %d\n",countThread );
	printf("Elapsed: %f miliseconds\n", (double)delta_us);
	printf("The program was successfully terminated\n");

	
	fclose(opf);
	
	return 0;
}

/***************************HELPER FUNCTION************************************/
/*Bu fonksiyon ile toplamda kaç tane directory ve file olduğunu ve kaç tane line aradigimizi buluruz*/
void numberOfDirectoryAndFile(const char *directory_path ) {

    DIR *directory_pointer;
    struct dirent *dirent_pointer;
    char sub_directory_path[BUFFER];

    directory_pointer = opendir(directory_path);


    if (directory_pointer == NULL) {

        fprintf(stderr, "\nDirectory couldn't open..!\n");

    } else {
    	copystr(sub_directory_path , directory_path);
		
		while((dirent_pointer = readdir(directory_pointer)) != NULL){   /*read directory */
		/*Directorylerin icine girerek directory kalmayana kadar doner*/ 
			
			copystr(sub_directory_path , directory_path);

			concat(sub_directory_path , "/"); 
			
			if(compare(dirent_pointer->d_name,".") != 0 && compare(dirent_pointer->d_name,"..") != 0  ){
				/* . veya .. isimli klasor olup olmadigi kontrolu*/	
				concat(sub_directory_path , dirent_pointer->d_name);
					
				if(isDirectory(sub_directory_path) != 0){/* Eger path directory ise*/

					++numberOfDirectory;
					numberOfDirectoryAndFile(sub_directory_path);
					/*Suan icinde bulundugumuz directory icindeki directory sayisi kontrolu*/
				}
				else if(isDirectory(sub_directory_path) == 0 ){ 
					/*Suan icinde bulundugumuz directory icindeki file sayisi kontrolu*/
					++numberOfFile;
					countlines(sub_directory_path);
				}
			}
		}

        closedir(directory_pointer);
    }

}

/*Threath haberlesme */
void * grepFileThreads(void *grep_data){

	int result;
    key_t mqKey;
    int mqId;
    struct messageQueue message ;
    struct structGrep *data = (struct structGrep*) grep_data;
	result = readFromFile(data->filePath ,data->stringName, data->filePath);

	mqKey = (key_t) getpid();

	mqId = msgget(mqKey,IPC_CREAT|0666);
	
    message.grep_number = result ;
    message.mtype = 1;

    msgsnd(mqId , &message , sizeof(int) , 0);
    
	return 0;

}
static void signal_handler(int signal_number) {

	FILE* fp;
    /*Wait childs*/
    while (wait(NULL) != -1);
    fp = fopen(OUTPUT, "a+");
	if (getpid() == mainPid) {
    	
    	printf("Called SIGINT signal. Program ended.\n");
    	fprintf(fp,"Called SIGINT signal. Program ended.\n");

        
    }
    fclose(opf);
    fclose(fp);
    shmdt(SharedMemCount);
	shmctl(shm_id_two , IPC_RMID , 0);
	/*Semaphore destroy*/
    sem_destroy(mySem);
	/*Shared semophere memory detach*/
    shmdt(mySem);
	/*Release shared memory id*/
    shmctl(shm_id_one, IPC_RMID, 0);
    pthread_mutex_destroy(&mutex_);

    exit(1);
}
int countlines(const char * filename){
	
	int ch = 0;
        
    FILE *fileHandle;
    char buf[200];
    
    if ((fileHandle = fopen(filename, "r")) == NULL) {
        return -1;
    } 

    
	while(fgets(buf,sizeof(buf) , fileHandle) != NULL){
		++count;
	}
	
	
   fclose(fileHandle);
   return 0;

}
/*Fonksiyon directory icine girerek tek tek dosyalara process olusturur 
ve her birinin icinde istenen string arar. Eger directory icinde directory var 
ise onun icine recursive cagri yaparak onun icindeki dosyalarında istenen 
stringi arar. */
int readFromDirectory( char * path , const char * string , const char*argv){
	
	DIR * dirp; /*Open directory*/
	FILE * opf; /* Log dosyasini acmak icin*/
	struct dirent *direntp; /*Read directory*/
	
	int  count = 0 , status=0 , total=0,openFifo,countTempDir=0 , result=0,totalDir=0 ;
	int i=0 , l=0;
	/* temp dosyasini acarak toplam kaç tane hedef string bulundugunu belrtmek icin file acariz.*/
	int dirCount;
    int fileCount;
	char temp_path[PATH_MAX] , tempArr[100],fifoName[DIR_MAX];
	int fd[2] , openFifo2;
	pid_t pid,temp=0; /* fork islemi icin*/
	char new_path[PATH_MAX];
	pthread_t threadArray[PATH_MAX]; /*Thread array*/
	int threadCounter = 0; /*Thread process counter*/
	struct structGrep grepArray[BUFFER]; /*structGrep structure array to send  grep information thread process*/
	struct messageQueue message;
	int mqId; /*Message queue id*/
	key_t mqKey;
    
	pthread_mutex_init(&mutex_, NULL);
	opf = fopen(OUTPUT ,"a+"); /* log.txt acilir.*/
 	/*(89-122 satir blogu directory icine girerek her bir directorydeki file ve directory sayisini bulur*/
	if((dirp = opendir(path)) == NULL){  /*Open directory control*/
		
		fprintf(stderr , "Directory not opened3\n");
		
		exit(1);
	}
	else{
		
		copystr(temp_path , path);
		
		while((direntp = readdir(dirp)) != NULL){   /*read directory */
		/*Directorylerin icine girerek directory kalmayana kadar doner*/ 
			
			copystr(temp_path , path);

			concat(temp_path , "/"); 
			
			if(compare(direntp->d_name,".") != 0 && compare(direntp->d_name,"..") != 0  ){
				/* . veya .. isimli klasor olup olmadigi kontrolu*/	
				concat(temp_path , direntp->d_name);
					
				if(isDirectory(temp_path) != 0){/* Eger path directory ise*/

					++dirCount;
					/*Suan icinde bulundugumuz directory icindeki directory sayisi kontrolu*/
				}
				else if(isDirectory(temp_path) == 0 ){ 
					/*Suan icinde bulundugumuz directory icindeki file sayisi kontrolu*/
					++fileCount;
					
				}
			}
		}
	}
	/*Directory basa doner ve islemlere tekrardan baslariz*/
	rewinddir(dirp);
	/*126-161 blogu her bir directory icin fifo olusturur*/
	sleep(1);

	if((dirp = opendir(path)) == NULL){ /*open directory control*/
		
		fprintf(stderr , "Directory not opened1\n");
		
		exit(1);
	}
	else{
			
		copystr(new_path , path);
		while((direntp = readdir(dirp)) != NULL){ /*read directory*/

			copystr(new_path , path);

			concat(new_path , "/"); 
			if(compare(direntp->d_name,".") != 0 && compare(direntp->d_name,"..") != 0  ){
   				/* . veya .. isimli klasor olup olmadigi kontrolu*/
   				concat(new_path , direntp->d_name);
   				
   				
   				if(isDirectory(new_path) != 0){/* Eger path directory ise*/
   					
					pid = fork(); /*Fork control*/
						
					if( pid == 0){ /*child process*/
						
						closedir(dirp); /*Close directory*/

						/*Recursive*/
						
						readFromDirectory( new_path , string , argv);
						

						exit(1);
					}
					
				}
				else if(isDirectory(new_path) == 0 ){ /* Eger path file ise*/
					
					sprintf(grepArray[threadCounter].stringName, "%s",  string);
					sprintf(grepArray[threadCounter].filePath, "%s", new_path);
                    
					/*Her bir file icin threat oluşturur.*/
                    pthread_create(&threadArray[threadCounter], NULL, grepFileThreads, &grepArray[threadCounter]);
                    ++threadCounter;
                    
                    mqKey = (key_t) getpid();
    				/*Message queue communication open*/
				    mqId = msgget(mqKey , IPC_CREAT |0666);
				    	
				}
			}
		}
		closedir(dirp); /*Close directory*/
	}
	fclose(opf); /*Close file*/
	
	pthread_mutex_lock(&mutex_);
	numberOfThread[numberOfThreadSize] =  threadCounter;
	
	++numberOfThreadSize;
	pthread_mutex_unlock(&mutex_);
	
	countThread = threadCounter;

	for (i = 0; i < threadCounter; ++i) {
        
        if (pthread_join(threadArray[i], NULL) != 0) {
        
            perror("Some error in wait thread process!");
        
            exit(1);
            
        }
    }
    
    for(i =0; i < fileCount; ++i){

    	msgrcv(mqId , &message , sizeof(int) , 0 , 0) ;
    	/*Alt directoryden mesaji okudu*/
    	countTempDir += message.grep_number;
    	
	}
     
	while(wait(&status) != -1); /*Parent , childlari bekler.*/
    
    result += countTempDir ;
	
	if(getpid() == mainPid)
		
		result += *SharedMemCount ;
		/*Main process de son sonucu yazar*/
	else /*Yavru processler  shared memory de paylasir*/
        *SharedMemCount += result;
    
    msgctl(mqId , IPC_RMID , NULL); /*Remove  message queue*/
    pthread_mutex_destroy(&mutex_);
    return result;
}
/*Programin asil islem yapacagi fonksiyondur.Dosyadan alip arraya attigimiz 
karakterleri string ile karsılastiririz. 
Tabi whitespace tab ve enter karakterlerini ignore ederiz.*/
int readFromFile( const char * filename , const char *string , const char * filePathName){

	FILE * inf;
	FILE * opf;

	int string_size = 0 ; /* Argumanda verilen stringin boyutu*/
	int counter = 0 , row = 1 , column = 1 ; /* Dusey yatay hiza belirlemek icin*/
 	int flag = 0; /*Durum kosuluna göre flag tuttuk.*/
 	int k=0, tempCount=0;
	int t=0; /* karakter ilerlemek için kullandigimiz degisken*/
	int z=0;
    int i=0;
    int j=1;
	char *array;
	pid_t childPid;
	int array_size=0; /*Malloc ile dinamik yer ayirmak icin kullanacagimiz size degiskeni*/
	
	string_size = string_length(string); /* Stringin boyutunu aldik.*/
	ptid = pthread_self();
	childPid = getpid();
	array_size = size(filename);
	
	array = (char*)malloc((sizeof(char)*array_size)+1); /*dinamik yer ayirdik*/
	array[array_size] = '\0';


	/* Programin asil islemini yapacağı fonksiyonu cagirdik*/
	
	inf = fopen(filename , "r");
	opf = fopen(OUTPUT , "a+");

	for(z=0; z < array_size ; ++z){ /* Dosyadan okuyup arraya attik*/
		
		fscanf(inf , "%c" , &array[z]);
	}
	
	for(i=0 ; i < array_size; ++i){
		
		flag = 0;
		
		tempCount =0;
		
		if(array[i] == string[0]){
		
			k = column;
		
			t =0;
			
			for(j=1; j< string_size && flag== 0 ; ++j){
			
				if(array[i+j+t] == ' ' || array[i+j+t] == '\t' || array[i+j+t] == '\n'){
				/* ignore edip karakter atliyoruz.*/
					for(; array[i+j+t] == ' ' || array[i+j+t] == '\t' || array[i+j+t] == '\n' ; ++t);
				}

				if(array[i+j+t] ==  string[j])
				
					++tempCount;
			
					if(tempCount == string_size-1){
			
						flag = 1;
						
						counter ++;
						
						fprintf(opf , " %d -%ld : [%d , %d] %s first character is found.\n" , childPid,ptid , row , k , string);
					
					}
			}
			column++;
		}
		else{
			
			if(array[i] == ' ' || array[i] == '\t')
			
				column++;
			
			else if(array[i] == '\n'){ 
			
				row++;
			
				column = 1;
			
			}
			else
				column++;
		}

	}
	if(counter == 0){
		printf(" %s dosyasinda %s bulunamadi. \n" , filePathName , string );
	}
	else
		printf(" %s dosyasinda %d adet %s bulundu.\n" ,filePathName , counter , string);
	
	fclose(inf);
	fclose(opf);

	free(array);
	

	return counter;
}

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

/***********************************HW_5_END***********************************/