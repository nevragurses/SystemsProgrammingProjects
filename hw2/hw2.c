#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h> 
#include <math.h>
#include <signal.h>
void processP1(int pid,int fdInput,int fdTemp);
char* leastSquaresMethod(char* arr,int bytes);
void processP2(int fdTemp,int fdOutput,char* name);
char* calculateErrors(char *line);
int removeLine(char* path,int counter,int keepByte,int sizeFile);
int findFileSize(int fdInput,char*buf);
ssize_t readFile(int fd, void *buffer, size_t byteCount);
ssize_t writeFile(int fd,char * str);
void handler(int signum);
void findingSignalHandler(int no);
void sigTermHandler(int sigNo);
int sigCounter=0,finish=0;
int arrOfSignals[1064];
int inputFd,outputFd,tempFd,tempFd2;
char tempPath[100];
char inputPath[100];
sig_atomic_t usr2Signal = 0;
sig_atomic_t usr1Signal= 0;

int main(int argc, char *argv[]) { 
   char *inputFile,*outputFile;
   int time,option; 

   /*getting command line arguments.*/
   while((option = getopt(argc, argv, ":i:o:")) != -1){ 
      switch(option){
         case 'i':
            inputFile=optarg;
            break;
         case 'o': 
            outputFile=optarg;
            break;   
         case '?': 
            perror("It does not fit format!Format must be: ./program -i inputPath -o outputPath\n");
            exit(EXIT_FAILURE);
            break;
      }
   }
   if((optind!=5)){
      errno=EINVAL;
      perror("CommandLine argument numbers must be 5! Format must be: ./program -i inputPath -o outputPath\n");
      exit(EXIT_FAILURE);
   } 

   int fdInput,fdOutput,fdTemp,fdTemp2;
   strcpy(inputPath,inputFile); /*Copy of input file path in global variable.*/
   
   /*Open input file*/
   if((fdInput=open(inputFile,O_RDONLY))==-1){
      perror("Failed to open input file in main.\n");
      exit(EXIT_FAILURE);
   }
   inputFd=fdInput; /*Copy of input file descriptor in global variable.*/
    
   /*Open output file.*/ 
   if((fdOutput=open(outputFile,O_WRONLY))==-1){
      perror("Failed to open output file in main.\n");
      exit(EXIT_FAILURE);
   }
   outputFd=fdOutput;  /*Copy of output file descriptor in global variable.*/

   /*Creating temporary file with mkstemp*/
   char template[] = "templateXXXXXX";
   fdTemp=mkstemp(template);
   if(fdTemp==-1){
      perror("Failed to open temporary  file in main.\n");
      exit(EXIT_FAILURE);
   }
   tempFd=fdTemp; /*Copy of temporary file descriptor in global variable.*/
   strcpy(tempPath,template); /*Copy of temporary file path in global variable.*/
   
   /*Open temporary file */
   if((fdTemp2=open(template,O_RDONLY))==-1){
      perror("Failed to open temporary file in main.\n");
      exit(EXIT_FAILURE);
   }
   tempFd2=fdTemp2; /*Copy of temporary file descriptor in global variable.*/
   
   sigset_t  mask,block_mask;
   struct sigaction act;

   /*Handling for SIGUSR1 and SIGUSR2 signals*/
   sigfillset (&mask);
   act.sa_handler = handler;
   act.sa_mask = mask;
   act.sa_flags = 0;
   if(sigaction (SIGUSR1, &act, NULL)==-1){
      perror("SIGACTION error in main for SIGUSR1\n");
      exit(EXIT_FAILURE);
   }   
   if(sigaction (SIGUSR2, &act, NULL)==-1){
      perror("SIGACTION error in main for SIGUSR2");
      exit(EXIT_FAILURE);
   }   
  
  /*Handling for SIGTERM signal*/
   struct sigaction actionTerm;
   sigfillset (&block_mask);
   actionTerm.sa_handler = sigTermHandler;
   actionTerm.sa_flags = 0;

   if(sigaction (SIGTERM, &actionTerm, NULL)==-1)
      perror("SIGACTION error in main for SIGTERM\n");
      
   pid_t pid;
   int status;
   pid=fork();
   /*If fork result is not 0,this is parent process*/
   if(pid>0){
      int pidOfChild=(int)pid;
      processP1(pidOfChild,fdInput,fdTemp); /*Calling function of parent process.*/
      kill(pidOfChild,SIGUSR1);/*Sending SIGUSR1 signal for child process at the end of process P1.*/
      /*Close input file for process P1*/
      if(close(fdInput)<0){
        perror("Close input file error in main for parent process.\n");
        exit(EXIT_FAILURE);
      }
      /*Close temporary file for process P1*/
      if(close(fdTemp)<0){
        perror("Close  temporary file error in main for parent process.\n");
        exit(EXIT_FAILURE);
      }
      wait(&status);
   }
    /*If fork result is 0,this is child process*/
   else{
      int childPid=(int)getpid();
      /*If there is no input in temporary file,sigsuspend call is using for child process.*/
      if(usr2Signal==0 ){
         printf("SIGUSR2 signal is waiting...\n");
         sigdelset(&mask, SIGUSR2);
        // sigdelset(&mask, SIGUSR1);  
         sigsuspend(&mask);
      }   
      processP2(fdTemp2,fdOutput,template); /*Calling function of child process*/
      /*If  P1 has not  finished writing temporary file,don't remove files so sigsuspend  call is using.*/ 
      if(usr1Signal==0){
         printf("SIGUSR1 signal is waiting...\n");
         sigdelset(&mask, SIGUSR1); 
         sigsuspend(&mask);
      } 
      /*CLose temporary file*/
      if(close(fdTemp2)<0){
        perror("Close temporar file error in main for child process.\n");
        exit(EXIT_FAILURE);
      }
      /*Close output file*/
      if(close(fdOutput)<0){
        perror("Close otput file error in main for child process.\n");
        exit(EXIT_FAILURE);
      }  
      /*Removes input File from disk*/
      if(unlink(inputFile)<0){
         perror("Unlink input file error\n");
        exit(EXIT_FAILURE);
      }
     
     
   }
   return 0;
} 
/*This handler for SIGUSR1 and SIGUSR2 signals*/
void handler(int signum) {
   if(signum==SIGUSR1){
      puts("Handler caught SIGUSR1 signal.\n"); 
      usr1Signal=1;               
   }
   if(signum==SIGUSR2){
      puts("Handler caught SIGUSR2 signal.\n");  
      usr2Signal=1;             
   } 
}
/*This signal handler for SIGTERM signal.All open files are closing and input file and temporary file are deleting from disk.*/
void sigTermHandler(int sigNo){
   if(sigNo==SIGTERM){
      printf("Handler caught SIGTERM signal!All open files is closing and also input file and temporary file is deleting from disk.\n");
      if(close(inputFd)<0){
        perror("Close input file error in SIGTERM signal handler.\n");
        exit(EXIT_FAILURE);
      }
      if(close(outputFd)<0){
        perror("Close output file error in SIGTERM signal handler.\n");
        exit(EXIT_FAILURE);
      }  
      if(close(tempFd)<0){
        perror("Close temporary file error in SIGTERM signal handler.\n");
        exit(EXIT_FAILURE);
      }
      if(close(tempFd2)<0){
        perror("Close temporary file error in SIGTERM signal handler.\n");
        exit(EXIT_FAILURE);
      }  
      /*Delete input file from disk*/
      if(unlink(inputPath)<0){
         perror("Unlink input file error in SIGTERM signal handler.\n");
         exit(EXIT_FAILURE);
      }
      /*Delete temporary file from disk*/
      if(unlink(tempPath)<0){
         perror("Unlink temporary file error in SIGTERM signal handler.\n");
         exit(EXIT_FAILURE);
      }
      exit(EXIT_SUCCESS);

   }
}
/*This function makes tasks of parent process P1*/
void processP1(int pid,int fdInput,int fdTemp){  
   printf("~~~~~~~~~~ PROCESS P1 ~~~~~~~~~~\n");
   int bytes;
   char buf[20];
   int j=0;
   int byteCounter=0,lineEqCounter=0;
   struct stat fileStat;  
 
   do{
     bytes=readFile(fdInput,buf,20); /*Read 20 Byte  from file by calling readFile function.*/
     byteCounter=byteCounter+bytes;
     if(bytes==20){
         lineEqCounter++;
         writeFile(fdTemp,leastSquaresMethod(buf,bytes)); /*Call function for find line equation and write result in temporary file.*/
         if(j==0){
            kill(pid,SIGUSR2); /*If first result is writed,send SIGUSR2 signal for child process.*/
            j=1;
         }
     }
    }while(bytes>0);

   /*Writes some results on screen*/
   printf("Number of Bytes that is read: %d \n",byteCounter);
   printf("Number of estimated line equation: %d\n",lineEqCounter);
   if(sigCounter==0){
      printf("There were not sent signals to P1 while it was critical section.\n");
   }
   if(sigCounter!=0){
      int s=0;
      while(s < sigCounter){
         printf("The signal that's signal number is %d was sent while P1  was in critical section\n",arrOfSignals[s]);
         s++;
      }
   }
}
/*This function apply least square method and finds line eqaution*/
char* leastSquaresMethod(char* arr,int bytes){
   sigset_t mask;
   sigset_t signals;
   struct sigaction action;
  
   /*For blocking SIGINT and SIGSTOP signals*/
   if( (sigemptyset(&mask)==-1) || (sigaddset(&mask,SIGSTOP)==-1) ||(sigaddset(&mask,SIGINT)==-1)){
      perror("Failed to initialize the signal mask in leastSquaresMethod function.");
      exit(EXIT_FAILURE);
   }
   if(sigprocmask(SIG_BLOCK,&mask,NULL)==-1){
      perror("Failed to blocking signals operation in leastSquaresMethod function.");
      exit(EXIT_FAILURE);
   }

   /*This handling operation for finding which signals were sent while P1 in critical region.*/
   sigfillset (&signals);
   action.sa_handler = findingSignalHandler;
   action.sa_flags = 0;   
   action.sa_mask = signals;
   
   int s=1;
   while(s<=31){
      if(s!=9 && s!=19){
         if(sigaction (s, &action, NULL)==-1){
            perror("SIGACTION error in leastSquaresMethod function.\n");
            exit(EXIT_FAILURE);
         }
      }    
      ++s;
   }
   char* bufWr=malloc(64*1024);
   char copy[32];
   unsigned int sumX=0,sumY=0,mult=0,square=0;
   double numinatorB,denominatorB,b,numinatorA,a;
   int i=0;
   /*For finding 2D coordinates in 20 byte and applying least square method and finding line equation.*/
   for(i=0;i < bytes;i=i+2){
      sprintf(copy, "(%u,%u), ", (unsigned char)arr[i],(unsigned char)arr[i+1]);
      sumX=sumX+ (unsigned char)arr[i];
      sumY=sumY+ (unsigned char)arr[i+1];
      square= square+((unsigned char)arr[i]*(unsigned char)arr[i]);
      mult=mult+ ((unsigned char)arr[i] * (unsigned char)arr[i+1]);
      strcat(bufWr, copy);
   }
   numinatorB= mult - (sumX*sumY/10.0);
   denominatorB= square - (sumX*sumX/10.0);
   b=numinatorB/denominatorB;

   numinatorA=sumY- (b*sumX);
   a = numinatorA /10.0;

   sprintf(copy, "%.3fx+%.3f", b,a);
   strcat(bufWr, copy);
   strcat(bufWr,"\n");

   /*Unblocking signals*/
   if(sigprocmask(SIG_UNBLOCK,&mask,NULL)==-1){
      perror("Failed to unblocking signals operation in leastSquaresMethod function.");
      exit(EXIT_FAILURE);
   }

   return bufWr;
}
/*This signal handler keeps signals that were sent  for P1 while it is in critical region*/
 void findingSignalHandler(int no){
   arrOfSignals[sigCounter]=no;
   sigCounter++;
}     
/*This function makes tasks of child process P2*/
void processP2(int fdTemp2,int fdOutput,char* name){
   printf("~~~~~~~~~~ PROCESS P2 ~~~~~~~~~~\n");
   int bytes,z,flag=0;
   char* line=malloc(64*1024);
   char *temp=malloc(1);
   char buf[20];
   int newlineCounter=0,keepByte=0,generalCounter=0;
   
   int size=findFileSize(fdTemp2,buf)-1;
   do{
      bytes=readFile(fdTemp2,buf,1);    
      if(buf[0]==10){ /*Finds new line that is end of a line*/
         generalCounter++;
      }
      if(generalCounter <= size ) {
         if(buf[0]!=10){ /*Keeps a line in string*/
            temp[0]=buf[0];
            strcat(line,temp);
            keepByte++; 
         }
         if(buf[0]==10 && strlen(line)!=0){
            writeFile(fdOutput,calculateErrors(line)); /*Makes error calculations and writes result in output file*/
            for(z=0;z<strlen(line);z++){
               line[z]='\0';
            }
            removeLine(name,generalCounter,keepByte,size); /*Remove line*/
            lseek(fdTemp2,-(keepByte),SEEK_CUR);
            keepByte=0;
            flag=0;
      }
   }
   }while(bytes>0);
}
/*This function for removing line that is read.*/
int removeLine(char* path,int counter,int keepByte,int sizeFile){
    int fdNew,fd;
    char* temp2=malloc(1);
    char buff[1024*32];
    int innerCounter=0;
    if((fdNew=open(path,O_RDWR ))==-1){
        perror("Failed to open open temporary  file operation in removeLine function\n");
        exit(EXIT_FAILURE);
    }
    do{
      fd=read(fdNew,buff,1);
      if(buff[0]==10){
         innerCounter ++;
      }  
      /*Finds which line will delete, removes it and prevents overriding.*/
      if(innerCounter >=counter && innerCounter<=sizeFile){
         temp2[0]=buff[0];
         lseek(fdNew,-(keepByte+1),SEEK_CUR);
         writeFile(fdNew,buff); 
         lseek(fdNew,+(keepByte),SEEK_CUR);           
      }
    }while(fd>0);
    if(ftruncate(fdNew,lseek(fdNew,0,SEEK_CUR)-keepByte)==-1){
        perror("Ftruncate error in removeLine function\n");
        exit(EXIT_FAILURE);
    }


    if(close(fdNew)<0){
        perror("Close file error in removeLine function.\n");
        exit(EXIT_FAILURE);
    }
    return 0;

}
/*This function for finding line size of file.*/
int findFileSize(int fdInput,char*buf){
   int bytes=0;
   int sizeFile=0;
    
   do{
      bytes=readFile(fdInput,buf,1);
      if(buf[0]==10)
         sizeFile++;
   }
   while(bytes>0);
   lseek(fdInput,0,SEEK_SET);
   return sizeFile; 
}
/*This function makes error calculations for process P2*/
char* calculateErrors(char *line){
   sigset_t mask;
   /*Blocks SIGINT and SIGSTOP signals*/
   if( (sigemptyset(&mask)==-1) || (sigaddset(&mask,SIGSTOP)==-1) ||(sigaddset(&mask,SIGINT)==-1)){
      perror("Failed to initialize the signal mask in calculateErrors function.");
      exit(EXIT_FAILURE);
   }
   if(sigprocmask(SIG_BLOCK,&mask,NULL)==-1){
      perror("Failed to blocking signals  in calculateErrors function.");
      exit(EXIT_FAILURE);
   }   
   int i=0;
   char *b = malloc(1024*64);
   /*Copies given line in another string*/
	for(i=0;i<strlen(line)-1;i++)
	{
		b[i] = line[i];
	}
   b[i] = '\0'; 
   double mean=0,standardDev=0;
   double arr[22];
   char copy[1024*64];
   i=0;
   char* token = strtok(line, " +,(\n)\0"); 
   /*Records coordinates and line equation in array*/
   while( token != NULL ) {
        arr[i]=atof(token);
        token = strtok(NULL, " +,(\n)\0");
        i++;
   }
   /*Makes error calculations*/
   double coeffX=arr[20];
   double sumMAE=0,sumMSE=0;
   double c=arr[21];
   for(i=0;i<20;i=i+2){
      double dist=fabs((coeffX*arr[i] )+ (-1*arr[i+1])+c)/(sqrt(pow(coeffX,2)+1));
      sumMAE=sumMAE+ dist;
      sumMSE=sumMSE+ pow(dist,2.0);
   }
   double MSE=sumMSE/10.0;
   double MAE=sumMAE/10.0;
   double RMSE=sqrt(MSE);
   mean=(MSE+MAE+RMSE)/3.0;
   standardDev= sqrt((pow((MSE-mean),2)+ pow((MAE-mean),2) + pow((RMSE-mean),2))/3.0);
   printf("MAE:%.3f, MSE:%.3f, RMSE:%.3f, Mean:%.3f, Standard Deviation: %.3f \n",MAE,MSE,RMSE,mean,standardDev);
   sprintf(copy, ", %.3f, %.3f, %.3f",MAE,MSE,RMSE);
   strcat(b,copy);
   strcat(b,"\n");
   free(b);
   if(sigprocmask(SIG_UNBLOCK,&mask,NULL)==-1){
      perror("Failed to unblocking signals in calculateErrors function.");
      exit(EXIT_FAILURE);
   }
   return b;
}
/*Reading given file with lock it.*/
ssize_t readFile(int fd, void *buffer, size_t byteCount){
   struct flock lockRd;
   int flagRead=0;
   int bytes=0;
   memset(&lockRd,0,sizeof(lockRd));
   lockRd.l_type=F_RDLCK;
   flagRead = fcntl(fd,F_SETLKW,&lockRd);
   if(flagRead==-1){
         perror("Failed blocking to read  file operation.");
         exit(EXIT_FAILURE);
   }    
   if((bytes=read(fd,buffer,byteCount))==-1){
         perror("Failed to read input file.\n");
         exit(EXIT_FAILURE);
   }
   lockRd.l_type=F_UNLCK;
   flagRead=fcntl(fd,F_SETLKW,&lockRd);
   if(flagRead==-1){
      perror("Failed unblocking to  read file operation.\n");
      exit(EXIT_FAILURE);
   }    
   return bytes;
}
/*Writing given file with lock it.*/
ssize_t writeFile(int fd,char * str){
   struct flock lockWr;
   int flagWr=0;
   int writeRes;
   memset(&lockWr,0,sizeof(lockWr));
   lockWr.l_type=F_WRLCK;
   flagWr = fcntl(fd,F_SETLKW,&lockWr);
   if(flagWr==-1){
      perror("Failed blocking to write file operation. \n");
      exit(EXIT_FAILURE);
   }    
   if((writeRes =  write(fd, str, strlen(str)))==-1){
      perror("Failed write  file operation.\n");
      exit(EXIT_FAILURE);
   }   
   lockWr.l_type=F_UNLCK;
   flagWr=fcntl(fd,F_SETLKW,&lockWr);
   if(flagWr==-1){
      perror("Failed unblocking to write file operation.\n");
      exit(EXIT_FAILURE);
   }
   return writeRes;
} 
