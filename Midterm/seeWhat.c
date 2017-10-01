#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <math.h>
#include <sys/wait.h>
#include <time.h>


#define FIFONAME "tempfifo"
#define MAINFIFO "mainfifo"
#define SHOWFIFO "showfifo"
#define SHOW_RESULT2 "showfifo2"
#define SHOWFIFO2 "showfifo3"

#define MAT_LOG "seeWhat.log"
#define SIZE 20
#define FIFOARG 1

static time_t ctrlcCurrentTime;
static pid_t server_pid;
static pid_t parent_pid;
static pid_t result_pid;
static pid_t child1_pid;
static pid_t child2_pid;

static char tempArr[100];
float buffer[10][10];
float output[10][10];


/*Bu islem orginal matris icin determinant yapar*/
float bigDeterminant(float[20][20], int );

/*Bu islem nxn lik matris icin determinant yapar*/
/*Inverse etmek icin determinant transpoze
 ve cofactor fonksiyonlarina ihtiyacimiz var*/ 

float determinant(float[][10], int);

void cofactor(int);

void transpose(float [][10], int);

/*2d convolution icin referansım :
http://www.songho.ca/dsp/convolution/convolution2d_example.html*/
/* Burada nxn lik matrisin 2boyutlu convolute islemini gerceklestirdik .*/

void TwoDimConvolution(int rows , int cols);

/* SIGUSR1 , SIGUSR2,SIGINT signallerini handle ettik */

void signal_handler(int signal);



int main(int argc , char ** argv){
	
	FILE *see_what_log_ptr;
	int  main_fd,fd , main_fd2 , main_fd3 , result_fd1 , result_fd2 , result_fd3 ,result_fd4;
	float matrix[20][20] ;
	pid_t inverse_pid , conv_pid ;
	int n;
	int divide =0;
	int count,count2;
	int a=0,b=0;
	float result1,result2;
	float orgDet , inverseDet , convDet;
	int i,j;
	int inverse_pipe_fd[2] , conv_pipe_fd[2];
	float inverseResult , convResult;
	char seeWhatLog[100];
	struct sigaction act;

	struct timeval resultOneCreateTime; /*Matris create time*/
    struct timeval resultOneCreateEndTime; /*Matris create end time*/
    struct timeval resultTwoCreateTime; /*Matris create time*/
    struct timeval resultTwoCreateEndTime; /*Matris create end time*/

    long elapsed1;
    long elapsed2;

	if(argc != 2){ /* Usage durumu: istenen argüman girilmeyince*/
		
		fprintf(stderr, "Usage : ./seeWhat <mainpipename>\n");
		return 1;
	}

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
    


	parent_pid = getpid();
	
	
	
	sprintf(tempArr , "%d-%s" , parent_pid , FIFONAME);

	result_fd2 = open(SHOW_RESULT2 , O_RDONLY);
    
    read(result_fd2 , &result_pid, sizeof(pid_t));
    
    close(result_fd2);
    
    unlink(SHOW_RESULT2);
	
	
	/* Buradaki fifolarda client pid sinin yollayip server pid sinin okuyoruz*/
	
	main_fd = open(MAINFIFO , O_RDONLY);
	
	read(main_fd , &server_pid , sizeof(pid_t));
	
	sleep(1);
	main_fd2 =  open("fifoname" , O_WRONLY);
	
	write(main_fd2 , &parent_pid , sizeof(pid_t));
	write(main_fd2 , &result_pid , sizeof(pid_t));
	

	result_fd1 = open(SHOWFIFO , O_WRONLY);
    write(result_fd1 , &parent_pid, sizeof(pid_t));
    write(result_fd1 , &server_pid, sizeof(pid_t));
	
	unlink(MAINFIFO);
	unlink("fifoname");
	
	close(main_fd);
	close(main_fd2);

	
	
	
	for(;;){
		/* signal yolladık .*/
		 
		kill(server_pid , SIGUSR1);
		
		sleep(2);
		/* Initialize*/
		
	    

	    //*Dosya acma*/
		sprintf(seeWhatLog , "log/%d-%s" , parent_pid , MAT_LOG);

	    see_what_log_ptr = fopen(seeWhatLog, "a+");
		
		fd = open(tempArr ,O_RDONLY);
		/*  size i ve gelen matrisi okuduk.*/
		read(fd , &n ,sizeof(int));

		for(int i=0; i < n ; ++i){
	    	
	    	for(int j=0;j <n ; ++j){
	    		
	    		
	    		matrix[i][j] = 0;

	    	}
	    	
	    }
		
		read(fd , matrix , 20*20*sizeof(int));

		for(int i=0; i < n ; ++i){
	    	
	    	for(int j=0;j <n ; ++j){
	    		
	    		
	    		printf("%f ",matrix[i][j]);

	    	}
	    	printf("\n");
	    }
		unlink(tempArr);
		close(fd);

		orgDet = bigDeterminant(matrix, n);


		pipe(inverse_pipe_fd);
		pipe(conv_pipe_fd);

		/* burdan itibaren iki ayrı islem icin 
		 2 process olusturduk ilk process inverse islemi yapıcak 
		 2. process ise 2d convolution islemi yapıcak . 
		 Bu islemleri yapmadan önce matrisi 4 parcaya ayirip
		 her bir nxn lik matriste islem yapacagiz.
		 Bu islemlerden sonra orginal matrisin determinantini alacagiz
		 shifted inverse matrisin determinanti ve 2d conv. edilmis 
		 matrisin determinantini alacagiz.
		 Bu islemlerin sonunda result1 ve result2 sonuclarinin elde etmis olacagiz--*/
		if((inverse_pid = fork()) ==-1){
			perror("Shifted inverse fork failed.\n" );
							
			exit(1);
		}
		else if(inverse_pid == 0){
			child1_pid = getpid();
			fprintf(see_what_log_ptr, "Original Matris = [ " );

			gettimeofday(&resultOneCreateTime, NULL);
			for(int i=0; i < n ; ++i){
		    	fprintf(see_what_log_ptr, "\t" );

		    	for(int j=0;j <n ; ++j){
		    		
		    		
		    		fprintf(see_what_log_ptr, "\t%f ",matrix[i][j]);

		    	}
		    	fprintf(see_what_log_ptr," ; ");
		    }

		    fprintf(see_what_log_ptr, " ];\n" );

			for(i=0; i < (n/2) ; ++i,++a){
				
				for(j=0; j < (n/2) ; ++j,++b){
					
					buffer[a][b] = matrix[i][j];
				
				}
				
				b=0;
			}
			
		   
		    cofactor((n/2));
		    
		    
		    a=0,b=0;
		    
		    for(i=0; i < (n/2) ; ++i,++a){
			
				for(j=0; j < (n/2) ; ++j,++b){
					
					 matrix[i][j] =buffer[a][b] ;
			
				}
			
				b=0;
			
			}
			
		    a=0,b=0;
		    
		    for(i=0; i < (n/2) ; ++i,++a){
			
				for(j=(n/2); j < n ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			
			cofactor((n/2));
		    
		    a=0,b=0;
		    
		    for( i=0; i < (n/2) ; ++i,++a){
			
				for(j=(n/2); j < n ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			
			}

		   
		    a=0,b=0;
		    
		    for(i=(n/2); i < n ; ++i,++a){
			
				for(j=0; j < (n/2) ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			
		    
		    cofactor((n/2));
		    
		    a=0,b=0;
		    
		    for (i=(n/2); i < n; ++i,++a){
			
				for(j=0; j < (n/2) ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			}
		    

		    a=0,b=0;
		    
			for(i=(n/2); i < n ; ++i,++a){
			
				for(j=(n/2); j < n ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			

			cofactor((n/2));
		    
		    a=0,b=0;
		    
		    for(i=(n/2); i < n ; ++i,++a){
			
				for(j=(n/2); j < n ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			
			}
		    
			
		    
		    inverseDet =  bigDeterminant(matrix, n);
		    
			result1 = orgDet - inverseDet ; 

			gettimeofday(&resultOneCreateEndTime, NULL);
			elapsed1 = (resultOneCreateEndTime.tv_sec-resultOneCreateTime.tv_sec)*1000000 + resultOneCreateEndTime.tv_usec-resultOneCreateTime.tv_usec;
			close(inverse_pipe_fd[0]);

                /* Send result through the output side of pipe */
            write(inverse_pipe_fd[1], &result1, sizeof(float));
		    
		    fprintf(see_what_log_ptr, "Shifted Inverse Matrix = [ " );

			for(int i=0; i < n ; ++i){
	    		fprintf(see_what_log_ptr, "\t" );
	    		for( j=0;j <n ; ++j){
	    		
	    		
	    			fprintf(see_what_log_ptr, "\t%f ",matrix[i][j]);

	    		}
	    		fprintf(see_what_log_ptr," ; ");
	    	}

	    	fprintf(see_what_log_ptr, " ];\n" );

		    exit(1);
		}

		wait(NULL);


		close(inverse_pipe_fd[1]);

        /* Read in a resultg from the pipe */
        read(inverse_pipe_fd[0], &inverseResult, sizeof(float));
		
		/*2d convolution islemi */
		if((conv_pid = fork()) ==-1){
			
			perror("2D convolution fork failed.\n" );
							
			exit(1);
		}
		else if(conv_pid == 0){

			child2_pid = getpid();

			gettimeofday(&resultTwoCreateTime, NULL);

			for(i=0; i < (n/2) ; ++i,++a){
			
				for(j=0; j < (n/2) ; ++j,++b){
					
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			
		   
		    TwoDimConvolution(n/2 , n/2);
		    
		    a=0,b=0;
		    
		    for(i=0; i < (n/2) ; ++i,++a){
			
				for( j=0; j < (n/2) ; ++j,++b){
					
					 matrix[i][j] =buffer[a][b] ;
			
				}
			
				b=0;
			
			}
			
		    a=0,b=0;
		    
		    for(i=0; i < (n/2) ; ++i,++a){
			
				for( j=(n/2); j < n ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			
			

		    TwoDimConvolution(n/2 , n/2);
		    
		    a=0,b=0;
		    
		    for(i=0; i < (n/2) ; ++i,++a){
			
				for(j=(n/2); j < n ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			
			}

		   
		    a=0,b=0;
		    
		    for(i=(n/2); i < n ; ++i,++a){
			
				for( j=0; j < (n/2) ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			
				b=0;
			
			}
			
		    TwoDimConvolution(n/2 , n/2);
		    
		    a=0,b=0;
		    
		    for(int i=(n/2); i < n; ++i,++a){
			
				for(int j=0; j < (n/2) ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			
			}
		    
		    a=0,b=0;
		    

		    for(int i=(n/2); i< n ; ++i,++a){
			
				for(int j=(n/2); j< n ; ++j,++b){
			
					buffer[a][b] = matrix[i][j];
			
				}
			

				b=0;
			
			}
			
			
		    TwoDimConvolution(n/2 , n/2);
		    
		    a=0,b=0;
		    
		    for(int i=(n/2); i < n ; ++i,++a){
			
				for(int j=(n/2); j < n ; ++j,++b){
			
					matrix[i][j] = buffer[a][b] ;
			
				}
			
				b=0;
			
			}
		    
			convDet =  bigDeterminant(matrix, n);
		    

		    result2 = orgDet - convDet ;
		    gettimeofday(&resultTwoCreateEndTime, NULL);
		    elapsed2 = (resultTwoCreateEndTime.tv_sec-resultTwoCreateTime.tv_sec)*1000000 + resultTwoCreateEndTime.tv_usec-resultTwoCreateTime.tv_usec;
		    close(conv_pipe_fd[0]);

                /* Send result through the output side of pipe */
            write(conv_pipe_fd[1], &result2, sizeof(float));
		    

		    fprintf(see_what_log_ptr, "2D Convolution Matrix = [ " );

			for(int i=0; i < n ; ++i){
	    		fprintf(see_what_log_ptr, "\t" );
	    		for(int j=0; j<n ; ++j){
	    		
	    		
	    			fprintf(see_what_log_ptr, "\t%f ",matrix[i][j]);

	    		}
	    		fprintf(see_what_log_ptr," ; ");
	    	}

	    	fprintf(see_what_log_ptr, " ];\n" );


		    exit(1);
		}

		wait(NULL);

		close(conv_pipe_fd[1]);

        /* Read in a result from the pipe */
        read(conv_pipe_fd[0], &convResult, sizeof(float));

        
        result_fd4 = open(SHOWFIFO2 , O_WRONLY);
        
        write(result_fd4 , &inverseResult, sizeof(float));
        write(result_fd4 , &convResult, sizeof(float));
         write(result_fd4 , &elapsed1, sizeof(long));
        write(result_fd4 , &elapsed2, sizeof(long));

        fprintf(see_what_log_ptr, "****************************************************\n\n");
		fclose(see_what_log_ptr);
	}
	

	return 0;

}
/* SIGUSR1 , SIGUSR2,SIGINT signallerini handle ettik */
void signal_handler(int signal) {
    const char *signal_name;
    FILE * fp;
    char seeWhatLog[100];
    sprintf(seeWhatLog , "log/%d-%s" , parent_pid , MAT_LOG);
    fp = fopen(seeWhatLog, "a+");

    /* Find out which signal we're handling*/
    switch (signal) {
        case SIGUSR1:
            signal_name = "SIGUSR1";
            
            break;
        case SIGINT:
        	fprintf(fp ,"\nCTRL C yapildi oluyorum\n");
            printf("Bu programda CTRL C yapildi program sonlandırılıyor\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp ,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            unlink(tempArr);
            unlink(MAINFIFO);
            unlink(FIFONAME);
            kill(server_pid , SIGUSR2);
            kill(result_pid , SIGUSR2);
            kill(child1_pid , SIGUSR2);
            kill(child2_pid , SIGUSR2);
            fclose(fp);
            exit(0);
        case SIGUSR2:
        	fprintf(fp ,"\nDiger programda CTRL C yapildi . program sonlandiriliyor\n");
            printf("CTRL C yapildi program sonlaniyor\n");
            ctrlcCurrentTime = time(NULL);
            fprintf(fp ,"Olme zamanı %ld\n" , ctrlcCurrentTime);
            unlink(tempArr);
            unlink(MAINFIFO);
            unlink(FIFONAME);
            fclose(fp);

            exit(0);
        
        default:
            fprintf(stderr, "Caught wrong signal: %d\n", signal);
            return;
    }

}
/*2d convolution icin referansım :
http://www.songho.ca/dsp/convolution/convolution2d_example.html*/
// Burada nxn lik matrisin 2boyutlu convolute islemini gerceklestirdik .
void TwoDimConvolution(int rows , int cols)
{
	
	float kernel[3][3] = { {0,0,0} , 
                         {0,1,0} ,
                         {0,0,0} };


    int k_Cols =3 , k_Rows=3;
    int k_CenterX,k_CenterY;
    int mm,nn;
    int n,m;
    int i,j,jj,ii;

    k_CenterX = k_Cols / 2;
    k_CenterY = k_Rows / 2;

    for(i=0; i < rows; ++i)              
    {
        for(j=0; j < cols; ++j)          
        {
            output[i][j] = 0;
        }
    }


    for(i=0; i < rows; ++i)              /* rows*/
    {
        for(j=0; j < cols; ++j)          /* columns*/
        {
            for(m=0; m < k_Rows; ++m)     /* kernel rows*/
            {
                mm = k_Rows - 1 - m;      /* row index of flipped kernel*/

                for(n=0; n < k_Cols; ++n) /* kernel columns*/
                {
                    nn = k_Cols - 1 - n;  /* column index of flipped kernel*/

                    /* index of input signal, used for checking boundary*/
                    ii = i + (m - k_CenterY);
                    jj = j + (n - k_CenterX);

                    /* ignore input samples which are out of bound */
                    if( ii >= 0 && ii < rows && jj >= 0 && jj < cols ){
                        output[i][j] += buffer[ii][jj] * kernel[mm][nn];
                        
                    }
                    
                }
            }
            
        }
    }
    
}
/* determinant cofactor ve transpoze fonksiyonları icin 
referansım: http://www.sanfoundry.com/c-program-find-inverse-matrix/*/

/*Bu islem orginal matris icin determinant yapar*/
float bigDeterminant(float num[20][20], int k)
{
  float s = 1, det = 0, b[20][20];
  int i, j, m, n, c;
  if (k == 1)
    {
     return (num[0][0]);
    }
  else
    {
     det = 0;
     for (c = 0; c < k; c++)
       {
        m = 0;
        n = 0;
        for (i = 0;i < k; i++)
          {
            for (j = 0 ;j < k; j++)
              {
                b[i][j] = 0;
                if (i != 0 && j != c)
                 {
                   b[m][n] = num[i][j];
                   if (n < (k - 2))
                    n++;
                   else
                    {
                     n = 0;
                     m++;
                     }
                   }
               }
             }
          det = det + s * (num[0][c] * bigDeterminant(b, k - 1));
          s = -1 * s;
          }
    }
 
    return (det);
}
/*Bu islem inverse edilmis ve 2d convolute edilmiş 
 matris icin determinant yapar*/
float determinant(float num[10][10], int k)
{
  float s = 1, det = 0, b[10][10];
  int i, j, m, n, c;
  if (k == 1)
    {
     return (num[0][0]);
    }
  else
    {
     det = 0;
     for (c = 0; c < k; c++)
       {
        m = 0;
        n = 0;
        for (i = 0;i < k; i++)
          {
            for (j = 0 ;j < k; j++)
              {
                b[i][j] = 0;
                if (i != 0 && j != c)
                 {
                   b[m][n] = num[i][j];
                   if (n < (k - 2))
                    n++;
                   else
                    {
                     n = 0;
                     m++;
                     }
                   }
               }
             }
          det = det + s * (num[0][c] * determinant(b, k - 1));
          s = -1 * s;
          }
    }
 
    return (det);
}
/*Inverse etmek icin determinant transpoze
 ve cofactor fonksiyonlarina ihtiyacimiz var*/ 
void cofactor(int f)
{
 float b[10][10], fac[10][10] , inverse[10][10];
 int p, q, m, n, i, j;
 for (q = 0;q < f; q++)
 {
   for (p = 0;p < f; p++)
    {
     m = 0;
     n = 0;
     for (i = 0;i < f; i++)
     {
       for (j = 0;j < f; j++)
        {
          if (i != q && j != p)
          {
            b[m][n] = buffer[i][j];
            if (n < (f - 2))
             n++;
            else
             {
               n = 0;
               m++;
               }
            }
        }
      }
      fac[q][p] = pow(-1, q + p) * determinant(b, f - 1);
    }
  }
  transpose(fac, f);
}
/*Finding transpose of matrix*/ 
void transpose(float fac[10][10],int r)
{
  int i, j;
  float b[10][10], inverse[10][10], d;
 
  for (i = 0;i < r; i++)
    {
     for (j = 0;j < r; j++)
       {
         b[i][j] = fac[j][i];
        }
    }
  d = determinant(buffer, r);
  for (i = 0;i < r; i++)
    {
     for (j = 0;j < r; j++)
       {
        buffer[i][j] = b[i][j] / d;
        }
    }
  
}
