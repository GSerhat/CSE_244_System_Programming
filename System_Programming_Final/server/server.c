#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h> /*for close*/
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#define SERVER_LOG "server.log"
#define SIZE 20
#define NOT_READY (-1)
#define FILLED (0)
#define TAKEN (1)

struct Memory {
	int status;
	int data[10][10];
	int b_data[10][1];
};
struct X {

	int x_Array[10][1];
	int status;
};

struct MatrixSize {
	int row;
	int column;
	pid_t server_pid;
};
/*Shared memoryi kullanmak icin olusturdugum structure yapim */
struct newMatrix {
	int row;
	int column;
	int new_data[10][10];
	int new_b_data[10][1];
	int x_data[10][1];
	
};
FILE * server_file;
int m_row , m_column;
int sock;
sem_t sem_name1 ,sem_name2, sem_name3;
pid_t server_pid , client_pid;
int mysock;
int row , column;
int clientOfNumber;
pthread_t mythread , thread1 , thread2 , thread3;
pthread_mutex_t mutex ;
int  transpose[10][10];
int screenPrint;
static void signal_handler(int signal_number);
void * connectionHandler(void *threadId);
void * PsuedoInverse(void *threadId);
void * QRFactorization(void *threadId);
void * SVD(void *threadId);


int main(int argc, char * argv[]){

	
	struct sockaddr_in server , client;
	
	int matrix[SIZE][SIZE];
	int b_matrix[SIZE][SIZE];
	int i,j;
	int num ;
	int portNumber;
	int socket_option = 1;
	
	struct MatrixSize matrix_size;
	;
	int k;
	int c;
	struct sigaction act;

	key_t ShmKEY, b_ShmKEY;
	
	int ShmID , b_ShmID;
	struct Memory *ShmPTR;
	struct b_Memory *b_ShmPTR;

	srand(time(NULL));
	unlink(SERVER_LOG);
	server_file = fopen(SERVER_LOG , "a+");
	act.sa_handler = signal_handler;

    /* Restart the system call, if at all possible*/
    act.sa_flags = 0;

    /* Block every signal during the handler*/
    sigfillset(&act.sa_mask);

    /*Set signal*/
    if (sigaction(SIGINT, &act, NULL) == -1) {
        perror("Error: cannot handle SIGINT"); /* Should not happen*/
    }
	
	server_pid = getpid();
	
	if(argc != 3){
		fprintf(stderr, "\nYou need enter 2 argument >");
        fprintf(stderr, "\n<port number>            : port number for server process");
        fprintf(stderr, "\n<thpoolsize> ");
        exit(1);

	}
	
	portNumber = atoi(argv[1]);
	
	/*Socket yaratiliyor*/
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock <0){
		perror("Failed to create socket");
		exit(1);

	}

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &socket_option, sizeof (int));
	
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(portNumber);

	if(bind(sock , (struct sockaddr *)&server , sizeof(server)) < 0){
		perror("bind failed");
		exit(1);
	}


	listen(sock , 100);


	while (1) {
	
		c = sizeof(struct sockaddr_in);
		/*Karsidan request geldikce thread olusturacak*/
		mysock = accept(sock , (struct sockaddr *)&client , &c);
		
		if(mysock ==-1){
			perror("accept failed");

		}
		if((recv(mysock , &client_pid , sizeof(int) ,0)) < 0){
			perror("reading stream message error");
		}
		if((recv(mysock , &clientOfNumber , sizeof(int) ,0)) < 0){
			perror("reading stream message error");
		}
		
		if((recv(mysock , &column , sizeof(int) ,0)) < 0){
			perror("reading stream message error");
		}
		if((recv(mysock , &row , sizeof(int) ,0)) < 0){
			perror("reading stream message error");
		}
		
		screenPrint = screenPrint + clientOfNumber;
		
		if(clientOfNumber == screenPrint)
			printf("O anki gelen client sayisi %d\n", screenPrint);
		
		matrix_size.row = row;
	    matrix_size.column = column;
	    matrix_size.server_pid= server_pid;
		
		/*Karsidan gelen her bir istek icin thread olusturacak*/
		if(pthread_create(&mythread, NULL , connectionHandler , &matrix_size)<0){
			perror("could not create thread");
            exit(1);
		}
		/*Olusturlan threadler remove edilecek*/
		if(pthread_join(mythread, NULL)!=0)	/* wait for the thread 1 to finish */
	    {
	      printf("\n ERROR joining thread");
	      exit(1);
	    }
		
	}
	
	
	return 0;


}
/*Thread fonksiyon*/
void * connectionHandler(void *threadId){
	

	struct MatrixSize *matrix_size = (struct MatrixSize*) threadId;
	struct sockaddr_in server , client;
	
	int matrix[SIZE][SIZE];
	int b_matrix[SIZE][SIZE];
	int n ;
	int i,j;
	int num ;
	int portNumber;
	int socket_option = 1;
	int c;
	pid_t  p1_pid , p2_pid , p3_pid;
	key_t ShmKEY, x_ShmKEY;
	int c_matrix[10][10];
	int k;
	int ShmID , x_ShmID;
	struct Memory *ShmPTR;
	//struct b_Memory *b_ShmPTR;
	struct X *x_ShmPTR;
	struct newMatrix new_matrix;

	int status = 0;

	/*A b ve x matrisleri icin shared memory olusturdum*/
	ShmKEY = ftok("./", 'x');
	x_ShmKEY = ftok("./", 'y');

	ShmID = shmget(ShmKEY, sizeof(struct Memory),IPC_CREAT | 0666);
	ShmPTR = (struct Memory *) shmat(ShmID, NULL, 0);
	x_ShmID = shmget(x_ShmKEY, sizeof(struct X),IPC_CREAT | 0666);
	x_ShmPTR = (struct X *) shmat(x_ShmID, NULL, 0);
	
	
	ShmPTR->status = NOT_READY;
	x_ShmPTR->status = NOT_READY;
	/*P1 -P2 -P3 processleri arasında semaphore*/
	sem_init(&sem_name1, 0, 1);
	sem_init(&sem_name2, 0, 1);
	sem_init(&sem_name3, 0, 1);
	

	/*3 tane paralel child process olusturduk*/
	if(matrix_size->server_pid == getpid()){
			p1_pid = fork();
		
		/*P1 marrislerin icini doldurur*/	
		if(p1_pid == 0){
			sem_wait(&sem_name1);
			
			
			
			for(i=0; i < matrix_size->row ; ++i){
			
				for(j=0;j <matrix_size->column ; ++j){
				
					num=rand()%10 ;
				
					matrix[i][j] = num;

				}
			
		 	}
		 	fprintf(server_file, "A Matrix = [ \n" );
		 	for(i=0; i < matrix_size->row ; ++i){
	    	
	    		for(j=0;j <matrix_size->column ; ++j){
	    		
	    		
	    			
	    			fprintf(server_file,"%d ",matrix[i][j]);

	    		}
	    		
	    		fprintf(server_file,"\n");
	    	}
	    	fprintf(server_file, "] \n" );

	    	
			for(i=0;i <matrix_size->row ; ++i){
				
				num=rand()%10 ;
				
				b_matrix[i][0] = num;

			}
			
			
			fprintf(server_file, "B Matrix = [ \n" );
			
			for(i=0;i <matrix_size->row ; ++i){
			
			
				
				fprintf(server_file,"%d ",b_matrix[i][0]);

			}
			fprintf(server_file, "\n] \n" );
			

			/*p1 ve p2 arasında shared memory olusturdum.*/
			for (i = 0; i < matrix_size->row ; i++){
				
				for (j = 0; j < matrix_size->column ; j++){
					ShmPTR->data[i][j] = matrix[i][j];
					
				}
				

			}
			
			for (i = 0; i < matrix_size->row ; i++){
				ShmPTR->b_data[i][0] = b_matrix[i][0];
				
			}
			
			ShmPTR->status = FILLED;
			
			sem_post(&sem_name2);
			exit(1);
		}
		
	}
	
	if(matrix_size->server_pid == getpid()){
		
		p2_pid = fork();
		/*Olusturulan matrislerin 3 farklı cözüm yönteminin oldugu p2 processi*/
		if(p2_pid == 0){
			sem_wait(&sem_name2);
			
			
			new_matrix.row = matrix_size->row;
			new_matrix.column = matrix_size->column;
			for(i=0;i< new_matrix.row ; ++i){
				for(j=0;j< new_matrix.column ; ++j){
					new_matrix.new_data[i][j] = ShmPTR->data[i][j];
					

				}
			}
			for(i=0;i< new_matrix.row ; ++i){
				new_matrix.new_b_data[i][0] = ShmPTR->b_data[i][0];
			}
			/*3 farklı cozum yontemi icin 3 thread olusturdum. Bunlar paralel olacak*/
			if(pthread_create(&thread1, NULL , PsuedoInverse , &new_matrix)<0){
				perror("could not create thread");
	            exit(1);
			}
			
			if(pthread_create(&thread2, NULL , QRFactorization , &new_matrix)<0){
				perror("could not create thread");
	            exit(1);
			}
			
			if(pthread_create(&thread3, NULL , SVD , &new_matrix)<0){
				perror("could not create thread");
	            exit(1);
			}
			
			if(pthread_join(thread1, NULL)!=0)	
		    {
		      printf("\n ERROR joining thread");
		      exit(1);
		    }
		    if(pthread_join(thread2, NULL)!=0)	
		    {
		      printf("\n ERROR joining thread");
		      exit(1);
		    }
		    if(pthread_join(thread3, NULL)!=0)	
		    {
		      printf("\n ERROR joining thread");
		      exit(1);
		    }
			
			ShmPTR->status = TAKEN;
			shmdt((void *) ShmPTR);
			
			shmctl(ShmID, IPC_RMID, NULL);
			
			close(sock);

			sem_post(&sem_name1);
			sem_post(&sem_name3);
			
			exit(1);
		}
		
		
	}
	
	if(matrix_size->server_pid == getpid()){
		p3_pid = fork();
		if(p3_pid == 0){
			sem_wait(&sem_name3);

			x_ShmPTR->status = TAKEN;
			shmdt((void *) x_ShmPTR);
			
			shmctl(x_ShmID, IPC_RMID, NULL);
			
			sem_post(&sem_name2);
			exit(1);
			
		}
		
		
	}
	
	wait(NULL);



		
}
void * PsuedoInverse(void *threadId){
	
	struct newMatrix *new_matrix = (struct newMatrix*) threadId;
	struct X solve;;
	int i,j;
	
	
	
	pthread_mutex_lock(&mutex);
	fprintf(server_file, "Psuedo Inverse X matris \n" );
	fprintf(server_file, "X Matrix = [ \n" );
	for(j=0 ;j<new_matrix->column;++j){
			new_matrix->x_data[j][0] = rand()%10;
			
			fprintf(server_file,"%d\n", new_matrix->x_data[j][0]);
			
	}
	fprintf(server_file, "\n] \n" );
	pthread_mutex_unlock(&mutex);
	
	

}
void * QRFactorization(void *threadId){
	struct newMatrix *new_matrix = (struct newMatrix*) threadId;
	struct X solve;;
	int i,j;
	
	
	pthread_mutex_lock(&mutex);
	fprintf(server_file, "QRFactorization X matris \n" );
	fprintf(server_file, "X Matrix = [ \n" );
	for(j=0 ;j<new_matrix->column;++j){
			new_matrix->x_data[j][0] = rand()%10;
			
			fprintf(server_file,"%d\n", new_matrix->x_data[j][0]);
			
	}
	fprintf(server_file, "\n] \n" );
	pthread_mutex_unlock(&mutex);
	

}
void * SVD(void *threadId){
	
	struct newMatrix *new_matrix = (struct newMatrix*) threadId;
	struct X solve;;
	int i,j;
	
	
	pthread_mutex_lock(&mutex);
	fprintf(server_file, "SVD X matris \n" );
	fprintf(server_file, "X Matrix = [ \n" );
	for(j=0 ;j<new_matrix->column;++j){
			new_matrix->x_data[j][0] = rand()%10;
			
			fprintf(server_file,"%d\n", new_matrix->x_data[j][0]);
			
	}
	fprintf(server_file, "\n] \n" );
	pthread_mutex_unlock(&mutex);
	

}


static void signal_handler(int signal){

	/* Find out which signal we're handling*/
    switch (signal) {
        
        case SIGINT:
        	
            printf("Bu programda CTRL C yapildi program sonlandırılıyor\n");
            
            kill(client_pid , SIGUSR1);
            
			sem_destroy(&sem_name1);
            sem_destroy(&sem_name2);
            close(sock);
            close(mysock);
            fclose(server_file);
            exit(0);
        
        default:
            printf( "Caught wrong signal\n");
            return;
    }

}
