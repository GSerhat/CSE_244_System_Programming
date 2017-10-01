/*


			    CSE 244 System Programming  HW_2
					Serhat Guzel 131044015
					
					    main.c

*/
#include<stdio.h>
#include<stdlib.h> /*For exit and malloc*/
#include<unistd.h> /* for fork */
#include<sys/wait.h>
#include<dirent.h> /*Directory Access*/
#include<sys/stat.h>
#include<sys/types.h>
#include<limits.h> /*Path max*/


#ifndef PATH_MAX
	#define PATH_MAX 255
#endif
#define OUTPUT "log.txt"
#define TEMP_FILE "total.txt"


/*****************************PROTOTYPES***************************************/
int size(const char * filename);
int readFromFile( const char * filename , const char *string , const char * filePathName);
void readFromDirectory( const char * path , const char * string );
int isDirectory(const char * path);
int string_length(const char * str);
void copystr(char* destination,  const char* source);
char* concat(char* destination, const char* source);
int compare( const char * str1, const char * str2);

/******************************MAIN FUNCTION***********************************/
int main(int argc , char *argv []){

	FILE * inf ;
	FILE * opf;
	
	int total;
	
	if(argc != 3){ /* Usage durumu: istenen argüman girilmeyince*/
		
		fprintf(stderr, "Usage : dir string <dirname>\n");
		return 1;
	}

	readFromDirectory( argv[2] , argv[1] ); 

	inf = fopen(TEMP_FILE , "r");

	fscanf(inf , "%d" , &total);
	opf = fopen(OUTPUT , "a");
	fprintf(opf, "\n\n\n %d %s were found in total.\n", total , argv[1]);
	

	fclose(inf);
	fclose(opf);

	remove(TEMP_FILE);
	
	
	return 0;
}

/***************************HELPER FUNCTION************************************/

/*Fonksiyon directory icine girerek tek tek dosyalara process olusturur 
ve her birinin icinde istenen string arar. Eger directory icinde directory var 
ise onun icine recursive cagri yaparak onun icindeki dosyalarında istenen 
stringi arar. */
void readFromDirectory( const char * path , const char * string ){
	
	DIR * dirp; /*Open directory*/
	struct dirent *direntp; /*Read directory*/
	char new_path[PATH_MAX];
	int  count = 0;
	
	FILE * inf; 
	/* temp dosyasini acarak toplam kaç tane hedef string bulundugunu belrtmek icin file acariz.*/
	FILE * opf; /* Log dosyasini acmak icin*/
	pid_t pid; /* fork islemi icin*/

	
	if((dirp = opendir(path)) == NULL){ /*open directory control*/
		fprintf(stderr , "Directory not opened\n");
		exit(1);
	}
	else{
		opf = fopen(OUTPUT ,"a"); /* log.txt acilir.*/
		copystr(new_path , path);
		
		while((direntp = readdir(dirp)) != NULL){ /*read directory*/

			copystr(new_path , path);

			concat(new_path , "/"); 
			
			if(compare(direntp->d_name,".") != 0 && compare(direntp->d_name,"..") != 0  ){
   				/* . veya .. isimli klasor olup olmadigi kontrolu*/
   				concat(new_path , direntp->d_name);
   				
   				if(isDirectory(new_path) == 0){/* Eger path directory ise*/

					if((pid = fork()) == -1){
						
						fprintf(stderr, "Fork failed.\n" );
						
						exit(1);
					}
					else if( pid == 0){ /*child process*/
						/*Recursive*/
						readFromDirectory( new_path , string );
						
						exit(1);
					}
					
				}
				else if(isDirectory(new_path) != 0 ){ /* Eger path directory ise*/
					

					if((pid = fork()) == -1){
						
						fprintf(stderr, "Fork failed.\n" );
						
						exit(1);
					}
					else if( pid == 0){ /*child process*/
						
						if((inf = fopen(TEMP_FILE , "r") )!= NULL){ 
						/*Temp dosya acip string sayimizi burada tuttuk.*/
				
							fscanf(inf , "%d" , &count);
							
							count += readFromFile(new_path , string , direntp->d_name);
							
							fclose(inf);
							
							inf = fopen(TEMP_FILE , "w");
							
							fprintf(inf, "%d\n", count );
							
							fclose(inf);
							exit(1);

						}
						else{
							
							inf = fopen(TEMP_FILE , "w");
							
							fprintf(inf, "%d\n", readFromFile(new_path , string , direntp->d_name) );
							
							fclose(inf);
							exit(1);
						}
					}
				}
			}
			wait(NULL);
		}
	}
	
	fclose(opf);
	closedir(dirp);
	
	
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
	int array_size; /*Malloc ile dinamik yer ayirmak icin kullanacagimiz size degiskeni*/

	string_size = string_length(string); /* Stringin boyutunu aldik.*/

	array_size = size(filename);
	
	array = (char*)malloc((sizeof(char)*array_size)+1); /*dinamik yer ayirdik*/
	array[array_size] = '\0';


	/* Programin asil islemini yapacağı fonksiyonu cagirdik*/
	
	inf = fopen(filename , "r");
	opf = fopen(OUTPUT , "a");

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
						
						fprintf(opf , " %s : [%d , %d] %s first character is found.\n" , filePathName , row , k , string);
					
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
		return S_ISREG(stat_array.st_mode);
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
/***********************************HW_2***************************************/
