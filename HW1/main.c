/*


			    CSE 244 System Programming  HW_1
					Serhat Guzel 131044015
					
					    main.c

*/
#include<stdio.h>
#include<stdlib.h> /*For exit and malloc*/

/*****************************PROTOTYPES***************************************/
int size(char * filename);
void readFromFile(char * filename , char *string , char * array , int array_size);
int string_length(char * str);
/******************************MAIN FUNCTION***********************************/
int main(int argc , char *argv []){

	char *array;
	int arr_size=0; /*Malloc ile dinamik yer ayirmak icin kullanacagimiz size degiskeni*/
	
	if(argc != 3){ /* Usage durumu: istenen argüman girilmeyince*/
		
		perror("Usage : ./exe string <filename>\n");
		return 1;
	}

	arr_size = size(argv[2]);
	
	array = (char*)malloc((sizeof(char)*arr_size)+1); /*dinamik yer ayirdik*/

	readFromFile(argv[2] , argv[1] , array , arr_size ); 
	/* Programin asil islemini yapacağı fonksiyonu cagirdik*/
	
	free(array);
	
	return 0;
}
/***************************HELPER FUNCTION************************************/
/* Dosyadaki karakter boyutunu hesaplamak icin kullandik.*/
int size(char * filename){

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
int string_length(char * str){

	int i=0;

	for(;str[i] != '\0' ; ++i);

	return i;
}
/*Programin asil islem yapacagi fonksiyondur.Dosyadan alip arraya attigimiz 
karakterleri string ile karsılastiririz. 
Tabi whitespace tab ve enter karakterlerini ignore ederiz.*/
void readFromFile(char * filename , char *string , char * array , int array_size){

	FILE * inf;

	int string_size = 0 ; /* Argumanda verilen stringin boyutu*/
	int counter = 0 , row = 1 , column = 1 ; /* Dusey yatay hiza belirlemek icin*/
 	int flag = 0; /*Durum kosuluna göre flag tuttuk.*/
 	int k=0, tempCount=0;
	int t=0; /* karakter ilerlemek için kullandigimiz degisken*/
	int z=0;
    int i=0;
    int j=1;
	string_size = string_length(string); /* Stringin boyutunu aldik.*/
	
	inf = fopen(filename , "r");
	
	for(z=0; z < array_size ; ++z){ /* Dosyadan okuyup arraya attik*/
		
		fscanf(inf , "%c" , &array[z]);
	}
	array[array_size] = '\0';

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
						
						printf("[%d , %d] konumunda ilk karakter bulundu.\n" , row , k);
					
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
	printf("%d adet %s bulundu.\n" , counter , string);
	
	fclose(inf);

}
/***********************************HW_1***************************************/
