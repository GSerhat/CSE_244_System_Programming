#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h> // for close
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/time.h>
#include <math.h>


#define DATA "Value a Socket"
#define MAX_SIZE 50
#define CLIENT_LOG "client.log"

sem_t sem_name;
/*Kullanıcıdan girdigim degerleri tutan structurem*/
struct MatrixSize {
	int row;
	int column;
};
int m_row , m_column;
int sock;
struct sockaddr_in server;
int i=0;
int portNumber;
int clientOfNumber;
FILE * client_file;
static void signal_handler(int signal_number);
void * connectionHandler(void *threadId);

int main(int argc , char *argv[]){

	int mysock;
	int row , column;
	
	struct sigaction act;
	pthread_t mythread[100];
	int i;
	float stn_dv;
	struct MatrixSize matrix_size;
	struct timeval resultOneCreateTime; /*Matris create time*/
    struct timeval resultOneCreateEndTime; /*Matris create end time*/
    float toplamstd =0.0;
	long average;
    long elapsed1;
    long total;
    long elapsedArray[128];

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



	if (argc != 5) {

        fprintf(stderr, "\n###################### USAGE ##################################\n");
        fprintf(stderr, "\nYou need enter <#of columns of A, m> <#of rows of A, p> <#of clients, q> , <port>");
        fprintf(stderr, "\n./client 3 2 2 5001\n");
        

        exit(1);
    }
    sem_init(&sem_name, 0, 1);
    m_column = atoi(argv[1]);
    m_row = atoi(argv[2]);
    clientOfNumber = atoi(argv[3]);
    portNumber = atoi(argv[4]);
    client_file = fopen(CLIENT_LOG , "a+");


    matrix_size.row = m_row;
	matrix_size.column = m_column;
    

    act.sa_handler = signal_handler;
    act.sa_flags = 0;

    if ((sigemptyset(&act.sa_mask) == -1) || (sigaction(SIGINT, &act, NULL) == -1) ) {
        perror("Signal set error!");
        exit(EXIT_FAILURE);
    }

    /*Kullanicidan aldigim deger kadar client olusturmak icin thread kullandim*/
	for(i=0; i<clientOfNumber;++i , ++toplamstd){
		
		gettimeofday(&resultOneCreateTime, NULL);
		if(pthread_create(&mythread[i], NULL , connectionHandler , &matrix_size)<0){
			perror("could not create thread");
            exit(1);
		}
		gettimeofday(&resultOneCreateEndTime, NULL);
		elapsed1 = (resultOneCreateEndTime.tv_sec-resultOneCreateTime.tv_sec)*1000000 + resultOneCreateEndTime.tv_usec-resultOneCreateTime.tv_usec;
		
		total = elapsed1 + total ;
		elapsedArray[i] = elapsed1;
		
		
	}
	
	average = total / clientOfNumber;
	for(toplamstd = 0.0 , i=0; i<clientOfNumber ; i++){
		toplamstd += pow(elapsedArray[i] - total , 2.0);
	}
	stn_dv = sqrt(toplamstd / (clientOfNumber-1));
	fprintf(client_file, "Average : %ld \n"  , average);
	fprintf(client_file, "Standart deviation : %f \n"  , stn_dv);
	


	/*Oluşan threadleri geri kaldirdim.*/
	for(i=0; i<clientOfNumber;++i){
		if(pthread_join(mythread[i], NULL)!=0)	/* wait for the thread 1 to finish */
	    {
	      printf("\n ERROR joining thread");
	      exit(1);
	    }
	}

	sem_destroy(&sem_name);

	fclose(client_file);

	return 0;

}
/*Thread fonsiyon*/
void * connectionHandler(void *threadId){
	pid_t client_pid = getpid();
	sem_wait(&sem_name);
	
	struct MatrixSize *matrix_size = (struct MatrixSize*) threadId;
	/*Socket olustu*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock <0){
		perror("Failed to create socket");
		exit(1);

	}
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_family = AF_INET;
	server.sin_port = htons(portNumber);
	/*Client sayisi row column ve ctrl c yapmak icin program pidsinin karsiya yolluyorum .*/
	if(connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("Not connected to Server");
		exit(1);
	}
	if( send(sock , &client_pid , sizeof(int) ,0) < 0 ){
		perror("Send failed");
		exit(1);
	}
	if( send(sock , &clientOfNumber , sizeof(int) ,0) < 0 ){
		perror("Send failed");
		exit(1);
	}
    if( send(sock , &matrix_size->column , sizeof(int) ,0) < 0 ){
		perror("Send failed");
		exit(1);
	}
	if( send(sock , &matrix_size->row , sizeof(int) ,0) < 0 ){
		perror("Send failed");
		exit(1);
	}
	
	printf("Sent %s\n" , DATA);
	sem_post(&sem_name);
}
static void signal_handler(int signal){

	/* Find out which signal we're handling*/
    switch (signal) {
        
        case SIGINT:
        	
            printf("Bu programda CTRL C yapildi program sonlandırılıyor\n");
            close(sock);
            exit(0);
        case SIGUSR1:
        	printf("\nDiger programda CTRL C yapildi . Program sonlandiriliyor\n");
            printf("CTRL C yapildi program sonlaniyor\n");
            close(sock);

            exit(0);
        
        default:
            printf("Caught wrong signal:\n");
            return;
    }

}
