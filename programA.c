#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> //for malloc operation.
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
char* convertComplex(char* arr,int bytes); //convert 32 byte complex number.
void operations(char* inputFile,char* destFile,int time);//makes all operations.

int main(int argc, char *argv[]) { 
    char *inputFile,*destFile;
    int time,option; 

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
                perror("Time is not in range for program A!");
                exit(EXIT_FAILURE);
            }
            break;
         case '?': 
            perror("It does not fit format for Program A!");
            exit(EXIT_FAILURE);
            break;
      }
      
    }
    if((optind!=6)){
        perror("It is not fit format in program A!\n");
        exit(EXIT_FAILURE);
    }
    operations(inputFile,destFile,time);
    return 0;
}   
//This function makes all operations for program A.
void operations(char* inputFile,char* destFile,int time){
        int fdInput,fdDest,bytes=0;
        struct flock lockRd;
        struct flock lockWr;
        struct stat fileState;
        char buf[32];
        char* bufWr;
        int flagRead=0,flagWr=0, flagReadDest,fd,counter=0, flag=0;
        off_t current,end;
        struct flock locked;
        char buffer[1];
        size_t remain,byteNow,bufferSize=1024*32;
        int readRes,writeRes,fstateRes;
        char buffer2[bufferSize];
        ssize_t readByte,writeByte;
        int flagLast=0,flagFirst=0,generalCounter=0;
        char* res;
        int exitFlag=0;
    
        //opening input file.
        if((fdInput=open(inputFile,O_RDONLY))==-1){
            perror("Failed to open input file in Program A\n");
            exit(EXIT_FAILURE);
        }
        //opening output file.
        if((fdDest=open(destFile,O_RDWR))==-1){
            perror("Failed to open destination file in Program A\n");
            exit(EXIT_FAILURE);
        }
    
        current = 0;
        //This loop reads input file and writes complex numbers in first empty row in  output file.
        do{
            
            memset(&lockRd,0,sizeof(lockRd));
            lockRd.l_type=F_RDLCK;
            flagRead = fcntl(fdInput,F_SETLKW,&lockRd);
            if(flagRead==-1){
                perror("Failed blocking to read input file operation in Program A\n");
                exit(EXIT_FAILURE);
            }    
            if((bytes=read(fdInput,buf,32))==-1){
                perror("Failed to read input file in Program A\n");
                exit(EXIT_FAILURE);
            }
            lockRd.l_type=F_UNLCK;
            flagRead=fcntl(fdInput,F_SETLKW,&lockRd);
            if(flagRead==-1){
                perror("Failed unblocking to read input file operation in Program A\n");
                exit(EXIT_FAILURE);
            }    
            res=convertComplex(buf,bytes);
            if(bytes!=32){
                exitFlag=1;
            }
            if(bytes>0 && exitFlag==0){   
                current=lseek(fdDest,0,SEEK_SET);
                counter=0;   
                flag=0;
                flagLast=0; 
                flagFirst=0;
                generalCounter=0;
                //This loop for searching first empty line in output file,and writing complex numbers in it.
                do{
                    memset(&locked,0,sizeof(locked));
                    locked.l_type=F_RDLCK;
                    flagReadDest = fcntl(fdDest,F_SETLKW,&locked);
                    if((fd=read(fdDest,buffer,1))==-1){
                        perror("Failed to read  output file operation in Program A\n");
                        exit(EXIT_FAILURE);
                    }
                    locked.l_type=F_UNLCK;
                    flagReadDest=fcntl(fdDest,F_SETLKW,&locked);
                    if(flagReadDest==-1){
                        perror("Failed unblocking to read output file operation in Program A\n");  
                        exit(EXIT_FAILURE);
                    }     
                    if(buffer[0]==10){ //There is end of line.
                        counter ++;
                    }
                    else{
                        counter=0;
                    }
                    memset(&lockWr,0,sizeof(lockWr));
                    lockWr.l_type=F_WRLCK;
                    flagWr = fcntl(fdDest,F_SETLKW,&lockWr);
                    if(flagWr==-1){
                        perror("Failed blocking to write output file operation in Program A \n");
                        exit(EXIT_FAILURE);
                    }    

                    //This condition for if first line is empty.Then I extending place in file and I preventing overwriting.    
                    if(lseek(fdDest,0,SEEK_CUR)==1 && buffer[0]==10){
                        current=lseek(fdDest,-1,SEEK_CUR);
                        fstateRes=fstat(fdDest, &fileState);
                        if (fstateRes==-1){
                            perror("Failed fstate operation in Program A\n");
                            exit(EXIT_FAILURE);
                        } 
                    
                        if (fstateRes== 0)
                        {
                            if (fileState.st_size > current)
                            {
                                remain = fileState.st_size - current;
                                end = fileState.st_size; 
                                while (remain != 0)
                                {
                                    if(bufferSize<remain)
                                        byteNow=bufferSize;
                                    else
                                        byteNow=remain;    
                                    readByte = end - byteNow;
                                    writeByte = readByte + strlen(res);
                                    lseek(fdDest, readByte, SEEK_SET);
                                    readRes=read(fdDest, buffer2, byteNow); 
                                    if (readRes != byteNow){
                                        perror("Failed inner read operation in Program A\n");
                                        exit(EXIT_FAILURE);
                                    }    
                                    lseek(fdDest, writeByte, SEEK_SET);
                                    writeRes=write(fdDest, buffer2, byteNow);
                                    if (writeRes != byteNow){
                                        perror("Failed inner read operation in Program A\n"); 
                                        exit(EXIT_FAILURE);  
                                    }    
                                    remain = remain- byteNow;
                                    end = end - byteNow; 
                                }
                            }    
                            lseek(fdDest, current, SEEK_SET); 
                            write(fdDest, res, strlen(res));
                            write(fdDest,"\n", strlen("\n"));
                            usleep(time * 1000);
                        }  
                        lseek(fdDest,0,SEEK_CUR); 
                        flag=1;
                        counter=1; 
                    }
                    if(fd==0 && flag==0 ){
                        write(fdDest, res, strlen(res));
                        write(fdDest,"\n", strlen("\n"));
                        usleep(time * 1000);
                        flagLast=1;
                    }
                    //This condition is for empty line that is not first line is founded,then I extending place in file and I preventing overwriting.
                    if(counter==2 && flag==0 && flagLast==0){
                        current=lseek(fdDest,-1,SEEK_CUR) ;
                        fstateRes=fstat(fdDest, &fileState);
                        if (fstateRes==-1){
                            perror("Failed fstate operation in Program A\n");
                            exit(EXIT_FAILURE);
                        } 
                        if (fstateRes== 0)
                        {
                            if (fileState.st_size > current)
                            {
                                remain = fileState.st_size - current;
                                end = fileState.st_size; 
                                while (remain != 0)
                                {
                                    if(bufferSize<remain)
                                        byteNow=bufferSize;
                                    else
                                        byteNow=remain;    
                                    readByte = end - byteNow;
                                    writeByte = readByte + strlen(res);
                                    lseek(fdDest, readByte, SEEK_SET);
                                    readRes=read(fdDest, buffer2, byteNow); 
                                    if (readRes != byteNow){
                                        perror("Failed inner read operation in Program A\n");
                                        exit(EXIT_FAILURE);
                                    }    
                                    lseek(fdDest, writeByte, SEEK_SET);
                                    writeRes=write(fdDest, buffer2, byteNow);
                                    if (writeRes != byteNow){
                                        perror("Failed inner read operation in Program A \n");
                                        exit(EXIT_FAILURE);  
                                    }     
                                    remain = remain- byteNow;
                                    end = end - byteNow; 
                                }
                            }    
                            lseek(fdDest, current, SEEK_SET); 
                            write(fdDest, res, strlen(res));
                            write(fdDest,"\n", strlen("\n"));
                            usleep(time * 1000);
                        }   
                        lseek(fdDest,0,SEEK_CUR); 
                        flag=1;
                        counter=1;  
                    }
                    lockWr.l_type=F_UNLCK;
                    flagWr=fcntl(fdDest,F_SETLKW,&lockWr);
                    if(flagWr==-1){
                        perror("Failed unblocking to write file operation in Program A\n");
                        exit(EXIT_FAILURE);
                    }    
                    
                }while(fd>0 && exitFlag==0);          
            }    
        }while(bytes>0);
        if(close(fdInput)<0){
            perror("Close  file error in Program A");
            exit(EXIT_FAILURE);
        }
        if(close(fdDest)<0){
            perror("Close file error in Program A");
            exit(EXIT_FAILURE);
        }
    
}

//This function for converting 32 byte to 16 complex number.
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

