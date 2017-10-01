/*


			    CSE 244 System Programming  HW_4
					Serhat Guzel 131044015
					
					    grepTh.c

*/
/********************************LIBRARIES*************************************/
#include "grepTh.h"

struct structGrep {
    char filePath[BUFFER];
    char stringName[PATH_MAX];
    int tfd[2];
};

int count1,count2;
static int lines , say ;
pthread_t ptid ;


/******************************MAIN FUNCTION***********************************/
int main(int argc , char *argv []){

	FILE * opf;
	
	int total;
	struct sigaction act;
	struct timespec start, end;
	long delta_us;
    act.sa_handler = signal_handler;
    act.sa_flags = 0;
	mainPid = getpid();

	if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1)) {
        perror("Failed to SIGINT handler");
        return 1;
    }
    
	if(argc != 3){ /* Usage durumu: istenen argüman girilmeyince*/
		
		fprintf(stderr, "Usage : ./grepTh <dirname> <string>\n");
		return 1;
	}
	unlink(OUTPUT);
	clock_gettime(CLOCK_MONOTONIC_RAW, &start);
	total = readFromDirectory( argv[1] , argv[2]  , argv[1]); 
	clock_gettime(CLOCK_MONOTONIC_RAW, &end);
	delta_us = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;
	opf = fopen(OUTPUT , "a");
	
	fprintf(opf, "\n\n\n %d %s were found in total.\n", total , argv[2]);
	
	printf( "\n\n\n %d %s were found in total.\n", total , argv[2]);
	printf( "Number of directories Searhced: %d\n", count2);
	printf( "Number of files Searhced: %d\n", count1);
	printf( "Number of lines Searhced: %d\n", say);
	printf("Elapsed: %f miliseconds\n", (double)delta_us);
	printf("The program was successfully terminated\n");

	
	fclose(opf);
	
	return 0;
}

/***************************HELPER FUNCTION************************************/
void * grepFileThreads(void *grep_data){

	struct structGrep *data = (struct structGrep*) grep_data;

    int result,total;
    
	result = readFromFile(data->filePath ,data->stringName, data->filePath);
    
    read(data->tfd[0], &total, sizeof (int));
    
    total += result;
    
    write(data->tfd[1], &total, sizeof (int));

    return 0;

}
static void signal_handler(int signal_number) {

    int i;

    /*Wait childs*/
    while (wait(NULL) != -1);


    if (getpid() == mainPid) {
    	printf("Called SIGINT signal. Program ended.\n");
        /*Unlink junk files*/
        
            unlink(FIFONAME);

    }

    exit(1);
}
int countlines(const char * filename){
	
	int ch = 0;
    int count = 0;    
    FILE *fileHandle;
    char buf[200];

    if ((fileHandle = fopen(filename, "r")) == NULL) {
        return -1;
    } 

    
	while(fgets(buf,sizeof(buf) , fileHandle) != NULL)
		++count;
	

   fclose(fileHandle);

   return count;

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
	struct structGrep grep_data_array[BUFFER]; /*structGrep structure array to send  grep information thread process*/

    
	
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
	
	count1	+= fileCount;
	count2	+= dirCount;

	if (getpid() == mainPid) {

        /*Creates main fifo*/
        unlink(FIFONAME);
        if (mkfifo(FIFONAME, 0666) == -1) {
            perror("FIFO not created");
            exit(1);
        }

        if (sem_init(&semlock, 1, 1) == -1) {
	        perror("sem_init error!");
	        exit(1);
	    }

	    /*Semaphore initialize*/
	    if (sem_init(&semlock_t, 1, 1) == -1) {
	        perror("sem_init error!");
	        exit(1);
	    }

        
    }
	
	if(fileCount > 0){
		
			if(pipe(fd) == -1){/*Pipe control*/
			
				fprintf(stderr, "Pipe not created\n" );

				exit(1);
			
			}
			write(fd[1], &temp, sizeof (int));
		}
	
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
						/*Recursive*/
						
						readFromDirectory( new_path , string , argv);
						

						exit(1);
					}
					
				}
				else if(isDirectory(new_path) == 0 ){ /* Eger path file ise*/
					
					sprintf(grep_data_array[threadCounter].stringName, "%s",  string);
					sprintf(grep_data_array[threadCounter].filePath, "%s", new_path);
                    grep_data_array[threadCounter].tfd[0] = fd[0];
                    grep_data_array[threadCounter].tfd[1] = fd[1];
					

                    /*Thread process for file*/
                    pthread_create(&threadArray[threadCounter], NULL, grepFileThreads, &grep_data_array[threadCounter]);
                    ++threadCounter;
				}
			}
		}
		closedir(dirp); /*Close directory*/
	}
	fclose(opf); /*Close file*/

	

	for (i = 0; i < threadCounter; ++i) {
        
        if (pthread_join(threadArray[i], NULL) != 0) {
        
            perror("Some error in wait thread process!");
        
            unlink(FIFONAME);
        
            exit(1);
            
        }
    }

	
	if (getpid() == mainPid) {
		
        /*Root (parent) process reads all grep information from childs*/
        for (i = 0; i < dirCount; ++i) {
        	
            openFifo = open(FIFONAME, O_RDWR);
            
            read(openFifo, &countTempDir, sizeof (int));
            
           
            
            result += countTempDir;
            
        }

        close(openFifo);

    }	
	
	
	
    while(wait(&status) != -1); /*Parent , childlari bekler.*/
	
		
	if (fileCount > 0) {
        read(fd[0], &countTempDir, sizeof (int));

    }
   	result += countTempDir ;
    
	
	if (getpid() != mainPid) {

        
        if (sem_wait(&semlock) == -1) {
            perror("sem_wait error!");
           exit(1);
        }
        
        openFifo2 = open(FIFONAME, O_WRONLY);
        write(openFifo2, &result, sizeof (int));
        
        close(openFifo2);

        if (sem_post(&semlock) == -1) {
            perror("sem_post error!");
            exit(1);
        }
      

    }

    
	if(getpid() == mainPid){
		
    	unlink(FIFONAME);

        sem_destroy(&semlock);

    	/*Semaphore destroy*/
    	sem_destroy(&semlock_t);
        
    }


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
	sem_wait(&semlock_t);
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
	sem_post(&semlock_t);

	return counter;
}


/***********************************HW_4_END***********************************/