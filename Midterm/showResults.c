#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <sys/wait.h>


#define SHOW_RESULT "showfifo"
#define SHOW_RESULT2 "showfifo2"
#define SHOW_RESULT3 "showfifo3"
#define PRINT_LOG "showResult.log"

static pid_t client_pid , server_pid , result_pid;
static char tempArr[100];
static int fd ,fd2 ,fd3 ;
static time_t ctrlcCurrentTime;



void signal_handler(int signal);


int main(int argc , char ** argv){

	FILE *show_result_log_ptr;

	float result1 , result2;
	
	
	char showResultLog[100];
	struct sigaction act;
	
	long elapsed1;
	long elapsed2;

	result_pid = getpid();

	act.sa_handler = signal_handler;

    /* Restart the system call, if at all possible*/
    act.sa_flags = 0;

    /* Block every signal during the handler*/
    sigfillset(&act.sa_mask);

    /*Set signal*/
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("Error: cannot handle SIGINT"); /* Should not happen*/
    }
    if (sigaction(SIGUSR1, &act, NULL) == -1) {
        perror("Error: cannot handle SIGUSR1"); /* Should not happen*/
    }
    if (sigaction(SIGUSR2, &act, NULL) == -1) {
        perror("Error: cannot handle SIGINT"); /* Should not happen*/
    }

    mkfifo(SHOW_RESULT2 ,0666);
    fd2 = open(SHOW_RESULT2 , O_WRONLY);
    write(fd2 , &result_pid, sizeof(pid_t));
    
    
    mkfifo(SHOW_RESULT ,0666);
	fd = open(SHOW_RESULT , O_RDONLY);

	read(fd , &client_pid , sizeof(pid_t) );
	read(fd , &server_pid , sizeof(pid_t) );
	unlink(SHOW_RESULT);
	
	sprintf(showResultLog , "log/%d-%s" , client_pid ,PRINT_LOG);
	show_result_log_ptr = fopen(showResultLog, "a+");
	
	for(;;){
		mkfifo(SHOW_RESULT3 ,0666);
		fd3 = open(SHOW_RESULT3 , O_RDONLY);
		read(fd3 , &result1 , sizeof(float) );
		read(fd3 , &result2 , sizeof(float) );
		read(fd3 , &elapsed1 , sizeof(long) );
		read(fd3 , &elapsed2 , sizeof(long) );
		
		unlink(SHOW_RESULT3);
		//unlink(SHOW_RESULT);
		sprintf(showResultLog , "log/%d-%s" , client_pid ,PRINT_LOG);
		show_result_log_ptr = fopen(showResultLog, "a+");

		printf ( "Client pid : %d  result1:	%f 	result2 :  %f \n" , client_pid , result1, result2);
		fprintf (show_result_log_ptr, "Client pid : %d  \n" , client_pid );
		fprintf (show_result_log_ptr, "result1:	%f 	elapsed: %ld \n" ,  result1, elapsed1);
		fprintf (show_result_log_ptr, "result2:	%f 	elapsed: %ld \n" ,  result2, elapsed2);
		
		fclose(show_result_log_ptr);
		

	}

	return 0;
}
void signal_handler(int signal) {
    const char *signal_name;
    FILE * fp;
    char ResultLog[100];
    sprintf(ResultLog , "log/%d-%s" , result_pid ,PRINT_LOG );
    fp = fopen(ResultLog, "a+");

    /* Find out which signal we're handling*/
    switch (signal) {
        
        case SIGINT:
        	fprintf(fp ,"\nCTRL C yapildi oluyorum\n");
            printf("Bu programda CTRL C yapildi program sonlandırılıyor\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp ,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            unlink(SHOW_RESULT);
            unlink(SHOW_RESULT3);
            
            printf("%d\n", server_pid );
            kill(server_pid , SIGUSR2);
            kill(client_pid , SIGUSR2);
            fclose(fp);
            exit(0);
        case SIGUSR2:
        	fprintf(fp ,"\nDiger programda CTRL C yapildi . program olduruluyorr\n");
            printf("CTRL C yapildi oluyorum\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp ,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            unlink(SHOW_RESULT);
            unlink(SHOW_RESULT3);
            
            fclose(fp);

            exit(0);
        
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }

}