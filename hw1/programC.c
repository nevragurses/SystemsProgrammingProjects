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

void printArray(double A[], int size);
char* readLineFromFile(int fdInput,int line);
int findFileSize(int fdInput,char*buf);
double* SymbolToDouble(char* complexArr);
double sizeOfLine(double *complexNum);
void writeLineInFile(int fdInput,int no,char* row);
void mergeSort(int fdInput, int low, int high);
int merge(int fdInput, int low, int mid, int high);

int main(int argc, char *argv[]){
    char *inputFile=(char*)malloc(100);
    char *destFile=(char*)malloc(100);
    int option; 
    //getting command line arguments.
    while((option = getopt(argc, argv, ":i:")) != -1){ 
        switch(option){
            case 'i': 
            inputFile=optarg;
            break;
            case '?':
            printf("unknown option: %c\n", optopt);
            break;
        } 
    }

    if((optind!=3)){
        perror("It is not fit format in program C!\n");
        exit(EXIT_FAILURE);
    }
    int flagBlock;
    int fdInput;
 
    if((fdInput=open(inputFile,O_RDWR))==-1){
        perror("Failed to open input file in Program C \n");
        exit(EXIT_FAILURE);
    }
    char temp[1];
    mergeSort(fdInput,0,findFileSize(fdInput,temp)-2);


    if((close(fdInput))<0){
        perror("Failed to close input file in Program C \n");
        exit(EXIT_FAILURE);
    }
   
    return 0; 
}
//Reading a line from file.
char* readLineFromFile(int fdInput,int line){
    struct flock lockRd;
    int flagRead;
    char *bufNew=(char*)malloc(1);
    char* string=(char*)malloc(1024);
    char *temp=(char*)malloc(1);
    int exitFlag=0;
    int find=0;
    int bytes;
    int lineSize=0;
    lseek(fdInput,0,SEEK_SET);
    do {
        memset(&lockRd,0,sizeof(lockRd));
        lockRd.l_type=F_RDLCK;
        flagRead = fcntl(fdInput,F_SETLKW,&lockRd);
        if(flagRead==-1){
            perror("Failed blocking to read input file operation in Program C\n");
            exit(EXIT_FAILURE);
        }    
        if((bytes=read(fdInput,bufNew,1))==-1){
            perror("Failed to read input file in Program C\n");
        }
        lockRd.l_type=F_UNLCK;
        flagRead=fcntl(fdInput,F_SETLKW,&lockRd);
        if(flagRead==-1){
            perror("Failed unblocking to read input file operation in Program C\n");
            exit(EXIT_FAILURE);
        }    
        temp[0]=bufNew[0];
        if(bufNew[0]==10){
            lineSize++;
            
        }
        if(lineSize==line && temp[0]!=10){
            strcat(string,temp);

        }
        if(lineSize>line)
            exitFlag=1;

    }while(bytes>0  && exitFlag==0);
    lseek(fdInput,0,SEEK_SET);
    return string;

}
//Finds line number of file.
int findFileSize(int fdInput,char*buf){
    struct flock lockRd;
    int flagRead;
    int bytes=0;
    int sizeFile=0;
    do {
        memset(&lockRd,0,sizeof(lockRd));
        lockRd.l_type=F_RDLCK;
        flagRead = fcntl(fdInput,F_SETLKW,&lockRd);
        if(flagRead==-1){
            perror("Failed blocking to read input file operation in Program C\n");
            exit(EXIT_FAILURE);
        }    
        if((bytes=read(fdInput,buf,1))==-1){
            perror("Failed to read input file in Program C\n");
            return 1;
        }
        lockRd.l_type=F_UNLCK;
        flagRead=fcntl(fdInput,F_SETLKW,&lockRd);
        if(flagRead==-1){
            perror("Failed unblocking to read input file operation in Program C\n");
            exit(EXIT_FAILURE);
        }    
        if(buf[0]==10){
            sizeFile++;
        }
    }while(bytes>0);
    lseek(fdInput,0,SEEK_SET);
    return sizeFile; 
}        
  
//ın-place merging operation.
int merge(int fdInput, int low, int mid, int high){ 
    int start = mid + 1; 

    if (sizeOfLine(SymbolToDouble(readLineFromFile(fdInput,mid))) <= sizeOfLine(SymbolToDouble(readLineFromFile(fdInput,start))))
        return 0; 
    while (low <= mid && start <= high){ 
        if (sizeOfLine(SymbolToDouble(readLineFromFile(fdInput,low))) <= sizeOfLine(SymbolToDouble(readLineFromFile(fdInput,start))))
            low++; 
        else { 
            char* temp = readLineFromFile(fdInput,start); 
            int index = start; 
            while (index != low) { 
                writeLineInFile(fdInput,index,readLineFromFile(fdInput,index-1));
                index--; 
            } 
            writeLineInFile(fdInput,low,temp);
            low++; 
            mid++; 
            start++; 
        } 
    }
    return 1;
} 
//mergesort operation.
void mergeSort(int fdInput, int low, int high){ 
    if (low < high) { 
        int middle = low + (high - low)/2; 
        mergeSort(fdInput, low, middle); 
        mergeSort(fdInput, middle + 1, high); 
        merge(fdInput, low, middle, high); 
    }
}
//Change given line according to sorting.
void writeLineInFile(int fdInput,int no,char* row){
    char buffer[1];
    int readControl;
    int nLCounter=0;
    int stringLength;
    int length=0;
    int flag=1,i;
    int counter=0;
    
    stringLength=strlen(row);
    lseek(fdInput,0,SEEK_SET);
    do{       
        readControl=read(fdInput,buffer,1);
        if(buffer[0]==10)
            nLCounter++;
        else if(nLCounter==no && length<stringLength){
            char s[1];
            s[0]=row[length];
            counter=lseek(fdInput,-1,SEEK_CUR);
            write(fdInput,s,1);
            length ++;
            
        }
        if(nLCounter==no && length==stringLength){
            if(!flag){
                counter=lseek(fdInput,-1,SEEK_CUR);
                write(fdInput," ",1);
            }
            flag=0;   
        }
    }
    while(readControl>0);

    if(length<stringLength){
        for(i=length;i<stringLength;++i){
            char buffer2[1];
            buffer2[0]=row[i];
            counter ++;
            char buffer[64*1024];
            struct stat sb;
            int rc = -1;
            struct flock lock;
            memset(&lock,0,sizeof(lock));

            if (fstat(fdInput, &sb) == 0)
            {
                if (sb.st_size > counter)
                {
                    size_t remain = sb.st_size - counter;
                    off_t  end = sb.st_size; 
                    while (remain != 0)
                    {
                        ssize_t currentByte;
                        if (64*1024<remain)
                            currentByte=64*1024;
                        else
                            currentByte=remain;
                        ssize_t readByte = end - currentByte;
                        ssize_t writeByte = readByte + 1;
                        if(lseek(fdInput, readByte, SEEK_SET)<0){
                            perror("Lseek error in Program C\n");
                            exit(EXIT_FAILURE);
                        }
                        if (read(fdInput, buffer, currentByte) !=currentByte)
                            rc=-1;
                        if(lseek(fdInput, writeByte, SEEK_SET)<0){
                            perror("Lseek error in Program C \n");
                            exit(EXIT_FAILURE);
                        }
                        if (write(fdInput, buffer, currentByte) != currentByte)
                            rc=-1;

                        remain -= currentByte;
                        end-= currentByte;
                    }   
                }   
                if(lseek(fdInput, counter, SEEK_SET)<0){
                    perror("Lseek error in Program C\n");
                    exit(EXIT_FAILURE);
                }
                write(fdInput, buffer2, 1);
                rc = 0;
            }    
        }
    }
}
//Total size of complex numbers in a line.
double* SymbolToDouble(char* complexArr){
    double *complexNum=(double*)malloc(64*sizeof(double));
    char* token =  strtok(complexArr, "()+i,\n\0 "); 
    int i=0;
    while( token != NULL) {
        complexNum[i]=atof(token);
        token = strtok(NULL, "()+i,\n\0 ");
        i++;
   }
   return complexNum;
}
//size of a line.
double sizeOfLine(double *complexNum){
    int i;
    double sum=0;
    for(i=0;i<32;i=i+2){
        sum+=sqrt(pow(complexNum[i],2)+pow(complexNum[i+1],2));
    }
    return sum;
}
