#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/errno.h>

#define SIZE 20
#define FIFONAME "tempfifo"
#define MAINFIFO "mainfifo"
#define SERVER_LOG_NAME "timerServer.log"

static time_t current_time;
static time_t ctrlcCurrentTime;
static pid_t client_pid; /*Server pid flag*/
static pid_t server_pid; /*Server pid flag*/
static pid_t result_pid; /*Server pid flag*/
static pid_t child_pid; /*Server pid flag*/
static pid_t pid; /*pid flag*/
static char tempArr[100];
static int fd , main_fd , main_fd2 ;
FILE *server_log_ptr;
void signal_handler(int signal);

int main(int argc , char ** argv){

	
	
	
	float matrix[SIZE][SIZE];
	int n ;
	int i,j;
	int num ;
	
	
	
	char serverLog[100];
	long elapsed;
	
	
	double resolution; /*Resolution number*/
	
	struct sigaction act;
    


    struct timeval matrixCreateTime; /*Matris create time*/
    struct timeval matrixCreateEndTime; /*Matris create end time*/
    
    n = 2*((int)(argv[2][0] - '0'));
    
    pid =getpid();
	if(argc != 4){ /* Usage durumu: istenen argüman girilmeyince*/
		
		fprintf(stderr, "Usage : ./timeServer <ticks in miliseconds> <n> <mainpipename>\n");
		return 1;
	}
	printf("pid %d\n",pid );
	if(getpid()!= 0 ){
		act.sa_handler = &signal_handler;

    	/* Restart the system call, if at all possible*/
	    act.sa_flags = 0;

	    /*Block every signal during the handler*/
	    sigfillset(&act.sa_mask);

	    /*Set signal*/
	    if (sigaction(SIGUSR1, &act, NULL) == -1) {
	        perror("Error: cannot handle SIGUSR1"); /* Should not happen*/
	    }

	    if (sigaction(SIGINT, &act, NULL) == -1) {
	        perror("Error: cannot handle SIGINT");  /* Should not happen*/
	    }

	    if (sigaction(SIGUSR2, &act, NULL) == -1) {
	        perror("Error: cannot handle SIGINT");  /* Should not happen*/
	    }
	    
	}
	



    server_pid = getpid();

    /********************************************************/
	
	n = 2*((int)(argv[3][0] - '0'));
	
	
	
    

	mkfifo(MAINFIFO , 0666);
	
	main_fd = open(MAINFIFO , O_WRONLY);	

	write(main_fd , &server_pid , sizeof(pid_t) );
	

		
	mkfifo("fifoname" , 0666);
	
	main_fd2 = open("fifoname" , O_RDONLY);
		
	read(main_fd2 , &client_pid , sizeof(pid_t));	
	read(main_fd2 , &result_pid , sizeof(pid_t));
	
	
	
	
	sprintf(tempArr , "%d-%s" , client_pid , FIFONAME);
	

    

	sprintf(serverLog , "log/%d-%s" , client_pid , SERVER_LOG_NAME);
	for(;;){
		

		srand(time(NULL));
	    
	    server_log_ptr = fopen(serverLog, "a+");
	    
	    sleep(1);
	    
	    if((pid = fork()) ==-1){
		
			perror("Fork failed.\n" );
		

		
			exit(1);
		
		}
		
		else if( pid == 0){ /*child process*/
			child_pid = getpid();
		    /*Get server start time*/
			gettimeofday(&matrixCreateTime, NULL);
			
			current_time = time(NULL);
			for(i=0; i < n ; ++i){
			
				for(j=0;j <n ; ++j){
				
					num=rand()%10 ;
				
					matrix[i][j] = num;

				}
			
		 	}
		 	
		 	gettimeofday(&matrixCreateEndTime, NULL);
		 	elapsed = (matrixCreateEndTime.tv_sec-matrixCreateTime.tv_sec)*1000000 + matrixCreateEndTime.tv_usec-matrixCreateTime.tv_usec;
		 	
		 	
		 	fprintf(server_log_ptr, "Matrisin olusturuldugu zaman %ld \t", elapsed );
		 	fprintf(server_log_ptr, "\nClient pid si %d\t", client_pid );
		 	fprintf(server_log_ptr, "\nMatris 	= \n");
		 	


		 	for(i=0; i < n ; ++i){
			
				for(j=0;j <n ; ++j){
				
					
					fprintf(server_log_ptr, "%f ", matrix[i][j] );
					

				}
				
				fprintf(server_log_ptr,"\n");
			
		 	}
		 	
		 	//printf("%d - %d\n",getppid() , getpid());
		 	fprintf(server_log_ptr, "****************************************************\n\n");
		 	
			fclose(server_log_ptr);
			mkfifo(tempArr , 0666);
		    
		    fd = open(tempArr , O_WRONLY);
		    	
			
		    
			write(fd , &n , sizeof(int));
			
			write(fd,matrix,20*20*sizeof(int));

			exit(1);
		}
	 	
		wait(NULL);
		
	}
	
	return 0;

}
void signal_handler(int signal) {
    const char *signal_name;
    FILE * fp;
    char serverLog[100];
    sprintf(serverLog , "log/%d-%s" , client_pid , SERVER_LOG_NAME);
    fp = fopen(serverLog, "a+");
    /* Find out which signal we're handling*/
    switch (signal) {
        
        case SIGUSR1:
        	
        	
    		signal_name = "SIGUSR1";
        	printf("Sinyal geldi \n");
            
        	
            break;

        case SIGINT:
        	
    		fprintf(fp ,"CTRL C yapildi cikiyorum\n");
            printf("\nBu programda CTRL C yapildi program sonlandiriliyor\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            kill(client_pid , SIGUSR2);
            kill(result_pid , SIGUSR2);
            kill(child_pid , SIGUSR2);
            close(main_fd);
            close(main_fd2);
            close(fd);
            unlink(tempArr);
            unlink(MAINFIFO);
            unlink("fifoname");
            fclose(fp);
            
            exit(0);
    	case SIGUSR2:
        	fprintf(fp ,"\nDiger programda CTRL C yapildi .Program olduruluyor\n");
            printf("CTRL C yapildi oluyorum\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp ,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            kill(child_pid , SIGINT);
            unlink(tempArr);
            
            fclose(fp);
            
            exit(0);
        
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }

}


