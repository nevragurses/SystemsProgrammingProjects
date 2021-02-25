#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <complex.h>
#include <math.h>
#include <time.h>
char* convertComplex(char* arr,int bytes);
char* fftArr(double complex in[],int size);
int findFileSize(int fdInput,char*buf);
void fourierTransform(double complex in[], int n);
int removeLine(char* path ,int counter,int keepByte,int sizeFile);
void  FFTAlgorithm(double complex in[], double complex out[], int n, int currentStep);
double complex* transformComplexNum(char* complexArr);

int main(int argc, char *argv[]) { 
    struct flock lock;
    srand(time(NULL));
    char *inputFile;
    char *destFile;
    int time;
    int option; 
    int fdInput,fdDest;
    //getting command line arguments.
    while((option = getopt(argc, argv, ":i:o:t")) != -1){ 
      switch(option){
         case 'i':
            inputFile=optarg;
            break;
         case 'o': 
            destFile=optarg;
            break;   
         case 't':
            time= atoi(argv[optind]);
            if(time<1 || time > 50){ 
                perror("Time is not in range for Program B!\n");
                exit(EXIT_FAILURE);
            }
            break;
         case '?': 
            perror("It does not fit format for Program B!\n");
            exit(EXIT_FAILURE);
            break;
      }
    }  
    //open input file.
    if((fdInput=open(inputFile,O_RDONLY))==-1){
        perror("Failed to open input file in Program B!\n");
        exit(EXIT_FAILURE);
    }
    //open output file.
    if((fdDest=open(destFile,O_WRONLY | O_APPEND))==-1){
        perror("Failed to open destination file in Program B \n");
        exit(EXIT_FAILURE);
    }
    int generalCounter=0;
    char* res;
    char* keep=(char*)malloc(1024*64);
    char* temp=malloc(1);
    char buf[32];
    int z,flagBlock,keepByte=0,startFlag=0,exitFlag=0,sleepFlag=0,bytes=0;
    struct stat fileState;
    int sizeFile=0,count=0,randomFlag=0,innerCounter=0;

    sizeFile=findFileSize(fdInput,buf)-1;
    int r=rand() % (sizeFile);
    lseek(fdInput,0,SEEK_SET);
    int flag=0;
    //This loop for reading input file,deleting line and then writing calculation in output file.
    do{
        if(flag==0){
            bytes=1;
            if((sizeFile=findFileSize(fdInput,buf)-1)<=0){
                usleep(1000*time);
            }
            else
                flag=1;
        }
        else{
            memset(&lock,0,sizeof(lock));
            lock.l_type=F_RDLCK;
            flagBlock = fcntl(fdInput,F_SETLKW,&lock);
            if(flagBlock==-1){
                perror("Failed blocking to read input file operation in Program B\n");
                exit(EXIT_FAILURE);
            }    
            if((bytes=read(fdInput,buf,1))==-1){
                perror("Failed to read input file in Program B\n");
                exit(EXIT_FAILURE);
            }
            if(buf[0]==10){
                generalCounter++;
                count ++;
            }
            else{
                count=0;
            }
            //This if condition provide to  read lines after random variable line  and make fft transforming and  deleting line by calling function.
            if(generalCounter >= r && generalCounter <= sizeFile ){
                if(buf[0]!=10){
                    temp[0]=buf[0];
                    strcat(keep,temp);
                    keepByte++;
                }    
                if(buf[0]==10 && strlen(keep)!=0){
                    lock.l_type=F_UNLCK;
                    flagBlock=fcntl(fdInput,F_SETLKW,&lock);
                    if(flagBlock==-1) {
                        perror("Failed unblocking to read operation in program B\n");
                        exit(EXIT_FAILURE);
                    }    
    
                    memset(&lock,0,sizeof(lock));
                    lock.l_type=F_WRLCK;
                    flagBlock = fcntl(fdDest,F_SETLKW,&lock);
                    if(flagBlock==-1){
                        perror("Failed blocking to write output file operation in Program B\n");
                        exit(EXIT_FAILURE);
                    }    
                    double complex* comp=transformComplexNum(keep);
                    fourierTransform(comp, 16);
                    char *result= fftArr(comp,16);

                    write(fdDest, result, strlen(result));
                    write(fdDest,"\n", strlen("\n"));
                    usleep(1000*time);  
                    for(z=0;z<strlen(keep);z++){
                            keep[z]='\0';
                    }
                    flagBlock=fcntl(fdDest,F_SETLKW,&lock);
                    if(flagBlock==-1){
                        perror("Failed unblocking to write output file operation in Program B\n");
                        exit(EXIT_FAILURE);
                    }      
                    removeLine(inputFile,generalCounter,keepByte,sizeFile);
                    lseek(fdInput,-(keepByte),SEEK_CUR);
                    keepByte=0;
                
                    
                }      
            }
            //This if condition provide to  read lines before random variable line  and make fft transforming and  deleting line by calling function.
            if(generalCounter > sizeFile)  {
                generalCounter=0;
                keepByte=0;
                count=0;
                lseek(fdInput,0,SEEK_SET);
                
                do{
                    memset(&lock,0,sizeof(lock));
                    lock.l_type=F_RDLCK;
                    flagBlock = fcntl(fdInput,F_SETLKW,&lock);
                    if(flagBlock==-1){
                        perror("Failed blocking to read input file operation in Program B\n");
                        exit(EXIT_FAILURE);
                    }    

                    if((bytes=read(fdInput,buf,1))==-1){
                        perror("Failed to read input file operation in Program B\n");
                        exit(EXIT_FAILURE);
                    }

                    if(buf[0]==10){
                        count ++;
                        generalCounter ++;
                    }
                    else{
                        count=0;
                    }
                    if(generalCounter <= r && bytes!=0){
                        if(buf[0]!=10){
                            temp[0]=buf[0];
                            strcat(keep,temp);
                            keepByte++;
                        }    
                        if(buf[0]==10 && strlen(keep)!=0){
                            lock.l_type=F_UNLCK;
                            flagBlock=fcntl(fdInput,F_SETLKW,&lock);
                            if(flagBlock==-1){
                                perror("Failed unblocking to read input file operation in Program B\n");
                                exit(EXIT_FAILURE);
                            }    
                            double complex* comp=transformComplexNum(keep);
                            fourierTransform(comp, 16);
                            char *result= fftArr(comp,16);
                            memset(&lock,0,sizeof(lock));

                            lock.l_type=F_WRLCK;
                            flagBlock = fcntl(fdDest,F_SETLKW,&lock);
                            if(flagBlock==-1){
                                perror("Failed blocking to write output file operation in Program B\n");
                                exit(EXIT_FAILURE);
                            }    

                            write(fdDest, result, strlen(result));
                            write(fdDest,"\n", strlen("\n"));

                            for(z=0;z<strlen(keep);z++){
                                    keep[z]='\0';
                            }
                            usleep(1000*time); 
                            lock.l_type=F_UNLCK;
                            flagBlock=fcntl(fdDest,F_SETLKW,&lock);
                            if(flagBlock==-1){
                                perror("Failed unblocking to read operation\n");
                                exit(EXIT_FAILURE);
                            }    
                            removeLine(inputFile,generalCounter,keepByte,sizeFile);
                            lseek(fdInput,-(keepByte),SEEK_CUR);
                            keepByte=0;   
                        }      
                    }
                    if(generalCounter==r){
                        exitFlag=1;
                    }
                }while(bytes>0 && exitFlag==0);  

            }

        } 
    }while(bytes>0 && exitFlag==0);
    if(close(fdInput)<0){
        perror("Close file error in Program B\n");
        exit(EXIT_FAILURE);
    }
    if(close(fdDest)<0){
        perror("Close file  error in Program B\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}
//This function for removing line that is read.
int removeLine(char* path ,int counter,int keepByte,int sizeFile){
    struct flock lock;
    int fdNew,fd,flagBlock;
    char *file;
    char* temp2=malloc(1);
    char buff[1024*32];
    int innerCounter=0;
    if((fdNew=open(path,O_RDWR ))==-1){
        perror("Failed to open read input  file operation in Program B\n");
        exit(EXIT_FAILURE);
    }
    do{
        memset(&lock,0,sizeof(lock));
        lock.l_type=F_RDLCK;
        flagBlock = fcntl(fdNew,F_SETLKW,&lock);
        if(flagBlock==-1){
            perror("Failed blocking to read  input file operation in Program B\n");
            exit(EXIT_FAILURE);
        }    

        if((fd=read(fdNew,buff,1))==-1){
            perror("Failed to read input file  Program B\n");
            exit(EXIT_FAILURE);
        }
        if(buff[0]==10){
            innerCounter ++;
        }  
        if(innerCounter >=counter && innerCounter<=sizeFile){
             lock.l_type=F_UNLCK;
            flagBlock=fcntl(fdNew,F_SETLKW,&lock);
            if(flagBlock==-1){
                perror("Failed unblocking to read input file operation in program B\n");
                exit(EXIT_FAILURE);
            }    
            lock.l_type=F_WRLCK;
            flagBlock = fcntl(fdNew,F_SETLKW,&lock);
            if(flagBlock==-1){
                perror("Failed blocking to write input file operation in Program B\n");
                exit(EXIT_FAILURE); 
            }    
            temp2[0]=buff[0];
            lseek(fdNew,-(keepByte+1),SEEK_CUR);
            write(fdNew,temp2,1);
            lseek(fdNew,+(keepByte),SEEK_CUR);  

            lock.l_type=F_UNLCK;
            flagBlock=fcntl(fdNew,F_SETLKW,&lock);
            if(flagBlock==-1){
                perror("Failed unblocking towrite input file operation in Program B\n"); 
                exit(EXIT_FAILURE); 
            }     

           
        }
    }while(fd>0);
    lock.l_type=F_WRLCK;
    flagBlock = fcntl(fdNew,F_SETLKW,&lock);
    if(flagBlock==-1){
        perror("Failed blocking to write input file operation in Program B\n"); 
        exit(EXIT_FAILURE);
    }    
    ftruncate(fdNew,lseek(fdNew,0,SEEK_CUR)-keepByte);

    lock.l_type=F_UNLCK;
    flagBlock=fcntl(fdNew,F_SETLKW,&lock);
    if(flagBlock==-1){
        perror("Failed unblocking to write input file operation in Program B\n"); 
        exit(EXIT_FAILURE);
    }      

    if(close(fdNew)<0){
        perror("Close file error\n");
        exit(EXIT_FAILURE);
    }

    return 0;

}
//This function for finding line size of file.
int findFileSize(int fdInput,char*buf){
    struct flock lock;
    int flagBlock;
    int bytes=0;
    int sizeFile=0;
    off_t offset;
    
    if((offset=lseek(fdInput,0,SEEK_CUR))<0){
        perror("Failed lseek operation in Program B\n");
        exit(EXIT_FAILURE);
    }
    do {
        memset(&lock,0,sizeof(lock));
        lock.l_type=F_RDLCK;
        flagBlock = fcntl(fdInput,F_SETLKW,&lock);
        if(flagBlock==-1){
            perror("Failed blocking to read input file operation in Program B\n");
            exit(EXIT_FAILURE);
        }    
        if((bytes=read(fdInput,buf,1))==-1){
            perror("Failed to read input file in Program B\n");
            exit(EXIT_FAILURE);
        }
       
        if(buf[0]==10){
            sizeFile++;
        }
    }while(bytes>0);
    lock.l_type=F_UNLCK;
    flagBlock=fcntl(fdInput,F_SETLKW,&lock);
    if(flagBlock==-1){
        perror("Failed unblocking to read input operation in Program B\n");
        exit(EXIT_FAILURE);
    }    
    if(lseek(fdInput,offset,SEEK_SET)<0){
        perror("Failed lseek operation in Program B\n");
        exit(EXIT_FAILURE);
    }

    return sizeFile; 
}
//This function is fft algorithm.        
void  FFTAlgorithm(double complex  in[],double complex  out[], int n, int currentStep)
{
	if (currentStep < n) {
		FFTAlgorithm(out, in, n, currentStep * 2);
		FFTAlgorithm(out + currentStep, in + currentStep, n,  currentStep * 2);
        int i=0;
        while(i<n){
            double complex x = cexp(-I * 3.141592 * i / n) * out[i + currentStep];
			in[i / 2]     = out[i] + x;
			in[(i + n)/2] = out[i] - x;
            i = i + (2*currentStep);

        }
	}
}
//This function is fourier transform that is using fft algorithm. 
void fourierTransform(double complex  in[], int n)
{
	double complex  out[n];
    int i=0;
    while(i<n){
            out[i]=in[i];
            i++;
    }
	FFTAlgorithm(in, out, n, 1);
}
//This function converts complex numbers in a line to string.
char* convertComplex(char* arr,int bytes){
    char* bufWr=malloc(64*1024);
    char copy[32];
    int i=0;
    for(i=0;i < bytes;i=i+2){
        sprintf(copy, "%d+i%d,", (int)arr[i],(int)arr[i+1]);
        strcat(bufWr, copy);
    }
    return bufWr;
}
//This function for getting fft transform as string.
char* fftArr(double complex in[],int size) {
    char* fft=malloc(64*1024);
    char fftArr[32];
    int i=0;
	for (i = 0; i < size; i++) {
        sprintf(fftArr, "(%f,%f),",creal(in[i]), cimag(in[i]));
        strcat(fft, fftArr);     
    }  
    return fft;      
}
//This function for transforming complex number string to actual complex number arrays.
double complex* transformComplexNum(char* complexArr){
    double complex* complexNum=malloc(64*1024);
    int arr[64];
    int i,j ;
    int z=0;
    double convertDouble;
    i=0;
    char* token = strtok(complexArr, "+i,\n\0 "); 
    while( token != NULL ) {
        arr[i]=atoi(token);
        token = strtok(NULL, "+i,\n\0 ");
        i++;
   }
   for(j=0;j<i ; j=j+2){
       convertDouble= arr[j] + arr[j+1]/pow(10, floor(log10(arr[j+1]) + 1));
       complexNum[z]=(double complex)convertDouble;
       z++;
   }
   return complexNum;
}
