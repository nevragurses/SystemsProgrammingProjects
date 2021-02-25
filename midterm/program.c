/* CSE344 – System Programming – MIDTERM PROJECT*/
/* Nevra GÜRSES -161044071*/
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> 
#include <fcntl.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <string.h> 
#include <signal.h>
#include <errno.h>
#include <semaphore.h>
#include <stdarg.h>
/*Shared Memory*/
struct SharedMemory {
    /*Unnamed semaphores and mutexes.*/    
    sem_t soup,mainCourse,desert; /*Semaphores for soup,main course and desert in kitchen.*/
    sem_t foodKitchen,foodCounter;   /*Semaphores for all item numbers in kitchen and on counter.*/
    sem_t fullKitchen,fullCounter;  /*Semaphores to control if kitchen and counter is full or not.*/
    sem_t emptyKitchen,emptyCounter;   /*Semaphores to control if kitchen and counter is empty or not.*/
    sem_t soupCounter,mcCounter,desertCounter; /*Semaphores for soup,main course and desert on counter.*/
    sem_t emptySoup,emptyMC,emptyDesert;
    sem_t fullSoup,fullMC,fullDesert;
    sem_t tableCount; /*Semaphore for tables.*/
    sem_t fullTable; /*Semaphore to control if all tables is full or not.*/
    sem_t m,m2; /*Mutexes to protect critical section.*/
    sem_t protect1,protect2; /*Mutexes to protect supplier and cook.*/

    /*variables in shared memory.*/
    int exit;
    int  totalFlag,studentFlag;
};
/*Functions that I used in project.*/
void supplySoup();
void supplyMainCourse();
void supplyDesert();
void takeSoup(int cookNum);
void takeMainCourse(int cookNum);
void takeDesert(int cookNum);
void giveSoup(int cookNum);
void giveMainCourse(int cookNum);
void giveDesert(int cookNum);
void soupFromCounter();
void mainCourseFromCounter();
void desertFromCounter();
void sigIntHandler(int sigNo);
int fdGlob;
static struct SharedMemory *sharedMem;
int main(int argc, char *argv[]) { 
    int n,m,t,s,l;
    char *inputFile;
    int option; 
    char string[300];
    int writeRes;
    /*Getting command line arguments.*/
    while((option = getopt(argc, argv, ":N:M:T:S:L:F:")) != -1){ 
        switch(option){
        case 'N':
            n=atoi(optarg);
        break;
        case 'M': 
            m=atoi(optarg);
        break;   
        case 'T':
            t = atoi(optarg);
        break;
        case 'S':
            s=atoi(optarg);
            break;
        case 'L':
            l=atoi(optarg);
            break; 
        case 'F':
            inputFile=optarg;  
            break;         
         case '?': 
            sprintf(string,"It does not fit format!Format must be :\n./program -N number -M number -T number -S number -L number -F filePath\n");
            writeRes=write(STDOUT_FILENO, string, strlen(string));
            if(writeRes==-1){
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
            break;
      }  
   }
   /*This condition to control format of command line arguments.*/
   if((optind!=13)){
        sprintf(string,"It does not fit format!Format must be :\n./program -N number -M number -T number -S number -L number -F filePath\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    /*Contraints of commandline arguments.*/
    if( m <= n || n<=2 ){
        sprintf(string,"It does not comply constraints ! There must be M > N > 2 \n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);

    }
    if( s<=3 ){
        sprintf(string,"It does not comply constraints ! There must be S > 3\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);

    }
    if( m <= t || t < 1){
        sprintf(string,"It does not comply constraints ! There must be M > T >= 1\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);

    }
    if( l < 3 ){
        sprintf(string,"It does not comply constraints ! There must be L >= 3\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);

    }
    int k= (2*l*m) +1;

    int fd;
    char* buffer="SharedArea";  
    /*Open  an  object with a specified name for shared memory.*/
    fd = shm_open("SharedArea",O_CREAT |  O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1){
        sprintf(string,"Failed shm_open operation in main!");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    } 
    if (ftruncate(fd, sizeof(struct SharedMemory)) == -1){
        sprintf(string,"ftruncate error in main!");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
         exit(EXIT_FAILURE);
    } 
    /*Maps the shared memory object into the process’s virtual address space.*/
    sharedMem = mmap(NULL, sizeof(struct SharedMemory), PROT_READ | PROT_WRITE,MAP_SHARED, fd, 0); 
    if( MAP_FAILED == sharedMem ) {
        sprintf(string,"Mmap failed in main function!" );
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        return EXIT_FAILURE;
    }

    /*SigInt(ctrl-c) signal handler.*/
    sigset_t  block_mask;
    struct sigaction actionTerm;
    sigfillset (&block_mask);
    actionTerm.sa_handler = sigIntHandler;
    actionTerm.sa_flags = SA_RESTART;
    if(sigaction (SIGINT, &actionTerm, NULL)==-1){
        sprintf(string,"SIGACTION error in main function for SIGINT signal\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }  
    /*Initializes the unnamed semaphores.*/ 
    if (sem_init(&sharedMem->m,1,1) == -1 ||
        sem_init(&sharedMem->m2,1,1) == -1 ||   
        sem_init(&sharedMem->fullKitchen,1,0) == -1 || 
        sem_init(&sharedMem->fullTable,1,t) == -1 ||
        sem_init(&sharedMem->fullCounter,1,0) == -1 || 
        sem_init(&sharedMem->protect1,1,1) == -1 || 
        sem_init(&sharedMem->protect2,1,1) == -1 || 
        sem_init(&sharedMem->emptyKitchen,1,k) == -1 ||
        sem_init(&sharedMem->emptyCounter,1,s) == -1 || 
        sem_init(&sharedMem->emptySoup,1,l*m) == -1 ||
        sem_init(&sharedMem->emptyMC,1,l*m) == -1 || 
        sem_init(&sharedMem->emptyDesert,1,l*m) == -1||
        sem_init(&sharedMem->fullSoup,1,0) == -1 || 
        sem_init(&sharedMem->fullMC,1,0) == -1 || 
        sem_init(&sharedMem->fullDesert,1,0) == -1 || 
        sem_init(&sharedMem->soup,1,0) ||
        sem_init(&sharedMem->mainCourse,1,0) ==-1 || 
        sem_init(&sharedMem->desert,1,0)==-1 || 
        sem_init(&sharedMem->soupCounter,1,0)==-1 || 
        sem_init(&sharedMem->mcCounter,1,0)==-1||
        sem_init(&sharedMem->desertCounter,1,0) == -1 || 
        sem_init(&sharedMem->foodKitchen,1,0) == -1 || 
        sem_init(&sharedMem->foodCounter,1,0) ==-1 ||  
        sem_init(&sharedMem->tableCount,1,t)==-1
    ){
        sprintf(string,"Error in main function for sem_init operation. \n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    } 
    /*Initializes the variables in shared memory.*/   
    sharedMem->totalFlag=0;
    sharedMem->studentFlag=0;
    sharedMem->exit=0;
   
    int fdInput,bytes,generalCounter,sizeP=0,sizeC=0,sizeD=0,tmpBytes;
    char buf[3*l*m];
    char tmp[1];

    /*Opens input file.*/
    if((fdInput=open(inputFile,O_RDONLY))==-1){
        sprintf(string,"Failed to open input file in main.\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    /*This loop for controlling whether input file contain 3*L*M character or not.If does not contain,exit from program!*/
    do{
        if((tmpBytes=read(fdInput,tmp,1))==-1){
            sprintf(string,"Failed to read input file in main.\n");
            writeRes=write(STDOUT_FILENO, string, strlen(string));
            if(writeRes==-1){
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        if(tmp[0]=='P' ){
            ++sizeP;
            tmp[0]='\0';
        }
        if(tmp[0]=='C' ){
            ++sizeC;
            tmp[0]='\0';
        }
        if(tmp[0]=='D' ){
            ++sizeD;
            tmp[0]='\0';
        }    
    }while(tmpBytes>0);
    if(sizeP!=l*m || sizeC!=l*m || sizeD!=l*m){
        sprintf(string,"Error! Input file must be contain exactly 3*L*M character(LM times P, LM times C and LM times D)\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    lseek(fdInput,0,SEEK_SET);
    fdGlob=fdInput;
    int soupVal=0,mcVal=0,desertVal=0,totalVal=0;
    int i=0,j=0;
    pid_t pid;
    /*Creating n+m+1 process that are supplier,cooks and students.*/
    for(i=0;i<n+m+1; i++){
        pid=fork();
        if(pid==-1){
            sprintf(string,"fork error in main!\n");
            writeRes=write(STDOUT_FILENO, string, strlen(string));
            if(writeRes==-1){
                exit(EXIT_FAILURE);
            }    
            exit(EXIT_FAILURE);
        }
        /*Child processes.*/
        if(pid == 0){
            /*Supplier process.*/
            if(i==0){
                /*Supplier reads the file and deliver plate that is read character to the kitchen.*/
                while(j<3*l*m){
                    /*Reading the file as 1 byte.*/
                    if((bytes=read(fdInput,buf,1))==-1){
                        sprintf(string,"Failed to read input file.\n");
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }    
                        exit(EXIT_FAILURE);
                    }
                    /*Getting values of kitchen items.*/
                    sem_getvalue(&sharedMem->soup,&soupVal);
                    sem_getvalue(&sharedMem->mainCourse,&mcVal);
                    sem_getvalue(&sharedMem->desert,&desertVal);
                    sem_getvalue(&sharedMem->foodKitchen,&totalVal);

                    /*If character is P in file, soup is sending to kitchen.*/
                    if(buf[0] == 'P'){
                        sprintf(string,"The supplier is going to the kitchen to deliver soup:  kitchen items  P:%d, C:%d, D:%d = %d\n",soupVal,mcVal,desertVal,totalVal);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        } 
                        supplySoup();
                    }
                    /*If character is C in file, main course is sending to kitchen.*/
                    else if(buf[0]=='C'){
                        sprintf(string,"The supplier is going to the kitchen to deliver main course:  kitchen items  P:%d, C:%d, D:%d = %d\n",soupVal,mcVal,desertVal,totalVal);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        supplyMainCourse();
                    }
                    /*If character is D in file, desert is sending to kitchen.*/
                    else if(buf[0]=='D'){
                        sprintf(string,"The supplier is going to the kitchen to deliver desert:  kitchen items  P:%d, C:%d, D:%d = %d\n",soupVal,mcVal,desertVal,totalVal);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        supplyDesert();
                    }
                    ++j;
                }
                /*Supplier finished suppliying.*/
                sprintf(string,"The supplier finished supplying – GOODBYE! \n");
                writeRes=write(STDOUT_FILENO, string, strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
            /*Cook process.*/
            else if(i<=n){
                int valS=0,valC=0,valD=0,total=0,z=0,select;
                int val1=0,val2=0,val3=0,val4=0;
                while(sharedMem->totalFlag <3*l*m)
                {
                    sem_wait(&sharedMem->protect1); /*Mutex to protect the cook.*/

                    /*Getting values of items in kitchen.*/
                    sem_getvalue(&sharedMem->soup,&valS);
                    sem_getvalue(&sharedMem->mainCourse,&valC);
                    sem_getvalue(&sharedMem->desert,&valD);
                    sem_getvalue(&sharedMem->foodKitchen,&total);

                    select=sharedMem->totalFlag%3; /*for sending items in sequence to counter.*/

                    /*Cook is taking soup from kitchen and delivering soup for counter.*/
                    if(select==0 && valS>0){
                        sprintf(string,"Cook %d is  is going to the kitchen to wait for/get a plate - kitchen items  P:%d, C:%d, D:%d = %d\n",i-1,valS,valC,valD,total);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        takeSoup(i-1); /*Take soup from kitchen.*/

                        /*Getting values of items on counter.*/
                        sem_getvalue(&sharedMem->soupCounter,&val1);
                        sem_getvalue(&sharedMem->mcCounter,&val2);
                        sem_getvalue(&sharedMem->desertCounter,&val3);
                        sem_getvalue(&sharedMem->foodCounter,&val4);
                        sprintf(string,"Cook %d is going to the counter to deliver soup – counter items P:%d, C:%d, D:%d = %d\n",i-1,val1,val2,val3,val4);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        giveSoup(i-1); /*Deliver soup on counter.*/
                    }
                     /*Cook is taking main course from kitchen and delivering main course for counter.*/
                    else if(select==1 && valC>0){ 
                        sprintf(string,"Cook %d is  is going to the kitchen to wait for/get a plate - kitchen items  P:%d, C:%d, D:%d = %d\n",i-1,valS,valC,valD,total);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        takeMainCourse(i-1); /*Take main course from kitchen.*/

                        /*Getting values of items on counter.*/
                        sem_getvalue(&sharedMem->soupCounter,&val1);
                        sem_getvalue(&sharedMem->mcCounter,&val2);
                        sem_getvalue(&sharedMem->desertCounter,&val3);
                        sem_getvalue(&sharedMem->foodCounter,&val4);
                        sprintf(string,"Cook %d is going to the counter to deliver main course – counter items P:%d, C:%d, D:%d = %d\n",i-1,val1,val2,val3,val4);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        giveMainCourse(i-1); /*Deliver main course on counter.*/
                    }
                    /*Cook is taking desert from kitchen and delivering desert for counter.*/
                    else if(select==2 && valD>0){ 
                        sprintf(string,"Cook %d is  is going to the kitchen to wait for/get a plate - kitchen items  P:%d, C:%d, D:%d = %d\n",i-1,valS,valC,valD,total);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        takeDesert(i-1); /*Take desert from kitchen.*/

                        /*Getting values of items on counter.*/
                        sem_getvalue(&sharedMem->soupCounter,&val1);
                        sem_getvalue(&sharedMem->mcCounter,&val2);
                        sem_getvalue(&sharedMem->desertCounter,&val3);
                        sem_getvalue(&sharedMem->foodCounter,&val4);
                        sprintf(string,"Cook %d is going to the counter to deliver desert – counter items P:%d, C:%d, D:%d = %d\n",i-1,val1,val2,val3,val4);
                        writeRes=write(STDOUT_FILENO, string,strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        giveDesert(i-1); /*Deliver desert on counter.*/
                    } 
                    sem_post(&sharedMem->protect1);
                    z++;
                    
                }
                int valKitchen;
                sem_getvalue(&sharedMem->foodKitchen,&valKitchen);
                /*Cook finished serving.*/
                sprintf(string,"Cook %d finished serving - items at kitchen: %d – going home –  GOODBYE!!!\n",i-1,valKitchen);
                writeRes=write(STDOUT_FILENO, string,strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);  
            }
            /*Student process.*/
            else{
                int valSC=0,valMC=0,valDC=0,valTotal=0;
                int table=0,numofStudent=1;
                int roundVal=1;
                while(sharedMem->studentFlag < 3*l*m){
                    sem_wait(&sharedMem->protect2); /*Mutex to protect the student.*/

                    /*Getting values of items on counter.*/
                    sem_getvalue(&sharedMem->soupCounter,&valSC);
                    sem_getvalue(&sharedMem->mcCounter,&valMC);
                    sem_getvalue(&sharedMem->desertCounter,&valDC);
                    sem_getvalue(&sharedMem->foodCounter,&valTotal);

                    /*If there are soup,main course and desert on counter,take three plates together and go to find empty table.*/
                    if(valSC>0 && valMC>0 && valDC>0 ){
                        sprintf(string,"Student %d is going to the counter (round %d) - # of students at counter: %d and counter items P:%d, C:%d, D:%d = %d\n",i-n-1,roundVal,numofStudent,valSC,valMC,valDC,valTotal);
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        /*Taking three plates from counter.*/
                        soupFromCounter();
                        mainCourseFromCounter();
                        desertFromCounter();
                        sem_getvalue(&sharedMem->tableCount,&table);
                        sprintf(string,"Student %d got food and is going to get a table (round %d) - # of empty tables: %d\n",i-n-1,roundVal,table); 
                        writeRes=write(STDOUT_FILENO, string, strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        /*Student is searching empty table and after finding,is sitting and eating and last going to counter to new round.*/
                        sem_wait(&sharedMem->fullTable);
                        sem_wait(&sharedMem->tableCount);
                        sem_getvalue(&sharedMem->tableCount,&table);
                        sprintf(string,"Student %d sat at table %d to eat (round %d) - empty tables:%d\n",i-n-1,t-table,roundVal,table);
                        writeRes=write(STDOUT_FILENO, string,strlen(string));
                        if(writeRes==-1){
                            exit(EXIT_FAILURE);
                        }
                        sem_post(&sharedMem->tableCount);
                        sem_post(&sharedMem->fullTable);
                        roundVal=roundVal+1;
                        /*Student left table and going to counter to eat again.*/
                        sem_getvalue(&sharedMem->tableCount,&table);
                        if(roundVal!=l+1){
                            sprintf(string,"Student %d left table %d to eat again (round %d) - empty tables:%d\n",i-n-1,t-table+1,roundVal,table);
                            writeRes=write(STDOUT_FILENO, string, strlen(string));
                            if(writeRes==-1){
                                exit(EXIT_FAILURE);
                            }
                        }
                       
                    } 
                    sem_post(&sharedMem->protect2);
                    /*If student ate L times,he/she finished eating so exit loop.*/
                    if(roundVal == l+1){
                        break;

                    }
                }
                /*Student done eating.*/
                sprintf(string,"Student %d is done eating L = %d  times - going home – GOODBYE!!!\n",i-n-1,roundVal-1);
                writeRes=write(STDOUT_FILENO, string, strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            }
        }   
    }
    /*Waiting for child processes for preventing zombie processes.*/   
    for(i=0;i<n+m+1;++i){
        if(wait(NULL)==-1){
            sprintf(string,"Wait  system call error in main!\n");
            writeRes=write(STDOUT_FILENO, string, strlen(string));
            if(writeRes==-1){
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
    }
    /*Close input file.*/
    if(close(fdInput) == -1){
        sprintf(string,"Close input file error in main function.\n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    /*Destroy the unnamed semaphores.*/
    if (sem_destroy(&sharedMem->m) == -1 ||
        sem_destroy(&sharedMem->m2) == -1 ||   
        sem_destroy(&sharedMem->fullKitchen) == -1 || 
        sem_destroy(&sharedMem->fullTable) == -1 ||
        sem_destroy(&sharedMem->fullCounter) == -1 || 
        sem_destroy(&sharedMem->protect1) == -1 || 
        sem_destroy(&sharedMem->protect2) == -1 || 
        sem_destroy(&sharedMem->emptyKitchen) == -1 ||
        sem_destroy(&sharedMem->emptyCounter) == -1 || 
        sem_destroy(&sharedMem->emptySoup) == -1 ||
        sem_destroy(&sharedMem->emptyMC) == -1 || 
        sem_destroy(&sharedMem->emptyDesert) == -1||
        sem_destroy(&sharedMem->fullSoup) == -1 || 
        sem_destroy(&sharedMem->fullMC) == -1 || 
        sem_destroy(&sharedMem->fullDesert) == -1 || 
        sem_destroy(&sharedMem->soup) ||
        sem_destroy(&sharedMem->mainCourse) ==-1 || 
        sem_destroy(&sharedMem->desert)==-1 || 
        sem_destroy(&sharedMem->soupCounter)==-1 || 
        sem_destroy(&sharedMem->mcCounter)==-1||
        sem_destroy(&sharedMem->desertCounter) == -1 || 
        sem_destroy(&sharedMem->foodKitchen) == -1 || 
        sem_destroy(&sharedMem->foodCounter) ==-1 ||  
        sem_destroy(&sharedMem->tableCount)==-1
    ){
        sprintf(string,"Error in main function for sem_destroy operation. \n");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    } 
    /*Removes the shared memory object specified by name.*/  
    if(shm_unlink(buffer)==-1){
        sprintf(string,"UNLINK error in main!");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    /*Delete the mappings.*/ 
    if(munmap(sharedMem, sizeof(struct SharedMemory))==-1){
        sprintf(string,"MUNMAP error in main!");
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        exit(EXIT_FAILURE);
    }
    return 0;
} 
/*Supplier is delivering soup to kitchen.*/
void supplySoup(){
    char string[300];
    int val,val2,val3,val4,writeRes;

    sem_wait(&sharedMem->emptyKitchen); 
    sem_wait(&sharedMem->emptySoup);
    sem_wait(&sharedMem->m);

    sem_post(&sharedMem->soup); /*Delivering soup to kitchen.*/
    sem_post(&sharedMem->foodKitchen); /*Increase item number in kitchen.*/

    /*Getting values of items in kitchen.*/
    sem_getvalue(&sharedMem->soup,&val);
    sem_getvalue(&sharedMem->mainCourse,&val2);
    sem_getvalue(&sharedMem->desert,&val3);
    sem_getvalue(&sharedMem->foodKitchen,&val4);
    sprintf(string,"The supplier delivered soup – after delivery: kitchen items  P:%d, C:%d, D:%d = %d\n",val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string, strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->fullSoup);
    sem_post (&sharedMem->fullKitchen);
}
/*Supplier is delivering main course to kitchen.*/
void supplyMainCourse(){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->emptyKitchen);
    sem_wait(&sharedMem->emptyMC);
    sem_wait(&sharedMem->m);

    sem_post(&sharedMem->mainCourse); /*Delivering main course to kitchen.*/
    sem_post(&sharedMem->foodKitchen); /*Increase item number in kitchen.*/

    /*Getting values of items in kitchen.*/
    sem_getvalue(&sharedMem->soup,&val);
    sem_getvalue(&sharedMem->mainCourse,&val2);
    sem_getvalue(&sharedMem->desert,&val3);
    sem_getvalue(&sharedMem->foodKitchen,&val4);
    sprintf(string,"The supplier delivered main course – after delivery: kitchen items  P:%d, C:%d, D:%d = %d\n",val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string, strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->fullMC);
    sem_post (&sharedMem->fullKitchen);
}
/*Supplier is delivering desert to kitchen.*/
void supplyDesert(){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->emptyKitchen);
    sem_wait(&sharedMem->emptyDesert);
    sem_wait(&sharedMem->m);

    sem_post(&sharedMem->desert); /*Delivering desert to kitchen.*/
    sem_post(&sharedMem->foodKitchen); /*Increase item number in kitchen.*/

    /*Getting values of items in kitchen.*/
    sem_getvalue(&sharedMem->soup,&val);
    sem_getvalue(&sharedMem->mainCourse,&val2);
    sem_getvalue(&sharedMem->desert,&val3);
    sem_getvalue(&sharedMem->foodKitchen,&val4);
    sprintf(string,"The supplier delivered desert – after delivery: kitchen items  P:%d, C:%d, D:%d = %d\n",val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string, strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->fullDesert);
    sem_post (&sharedMem->fullKitchen);
}
/*Cook is getting soup from kitchen.*/
void takeSoup(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->fullKitchen);
    sem_wait(&sharedMem->fullSoup);
    sem_wait(&sharedMem->m);

    sem_wait(&sharedMem->soup);  /*Getting soup from kitchen.*/
    sem_wait(&sharedMem->foodKitchen); /*Decrease item number in kitchen.*/

    ++(sharedMem->totalFlag); 

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->emptySoup);
    sem_post (&sharedMem->emptyKitchen);  
}
/*Cook is getting main course from kitchen.*/
void takeMainCourse(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->fullKitchen);
    sem_wait(&sharedMem->fullMC);
    sem_wait(&sharedMem->m);

    sem_wait(&sharedMem->mainCourse); /*Getting main course from kitchen.*/
    sem_wait(&sharedMem->foodKitchen);  /*Decrease item number in kitchen.*/

    ++(sharedMem->totalFlag);

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->emptyMC);
    sem_post (&sharedMem->emptyKitchen);   
}
/*Cook is getting desert from kitchen.*/
void takeDesert(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->fullKitchen);
    sem_wait(&sharedMem->fullDesert);
    sem_wait(&sharedMem->m);

    sem_wait(&sharedMem->desert); /*Getting desert from kitchen.*/
    sem_wait(&sharedMem->foodKitchen); /*Decrease item number in kitchen.*/

    ++(sharedMem->totalFlag);

    sem_post(&sharedMem->m);
    sem_post(&sharedMem->emptyDesert);
    sem_post (&sharedMem->emptyKitchen);     
}
/*Cook is delivering soup on counter.*/
void giveSoup(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->emptyCounter);
    sem_wait(&sharedMem->m2);

    sem_post(&sharedMem->soupCounter);  /*Delivering soup on counter.*/  
    sem_post(&sharedMem->foodCounter);  /*Increase item number on counter.*/

    /*Getting values of items on counter.*/
    sem_getvalue(&sharedMem->soupCounter,&val);
    sem_getvalue(&sharedMem->mcCounter,&val2);
    sem_getvalue(&sharedMem->desertCounter,&val3);
    sem_getvalue(&sharedMem->foodCounter,&val4);
    sprintf(string,"Cook %d  placed soup on the counter - counter items P:%d, C:%d, D:%d = %d\n",cookNum,val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string,strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }
    sem_post(&sharedMem->m2);
    sem_post(&sharedMem->fullCounter);
}
/*Cook is delivering main course on counter.*/
void giveMainCourse(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->emptyCounter);
    sem_wait(&sharedMem->m2);

    sem_post(&sharedMem->mcCounter);  /*Delivering main course on counter.*/  
    sem_post(&sharedMem->foodCounter); /*Increase item number on counter.*/

    /*Getting values of items on counter.*/
    sem_getvalue(&sharedMem->soupCounter,&val);
    sem_getvalue(&sharedMem->mcCounter,&val2);
    sem_getvalue(&sharedMem->desertCounter,&val3);
    sem_getvalue(&sharedMem->foodCounter,&val4);
    sprintf(string,"Cook %d placed main course on the counter - counter items P:%d, C:%d, D:%d = %d\n",cookNum,val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string, strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }
    sem_post(&sharedMem->m2);
    sem_post(&sharedMem->fullCounter);
}
/*Cook is delivering desert on counter.*/
void giveDesert(int cookNum){
    char string[300];
    int val,val2,val3,val4,writeRes;
    sem_wait(&sharedMem->emptyCounter);
    sem_wait(&sharedMem->m2);

    sem_post(&sharedMem->desertCounter);  /*Delivering desert on counter.*/  
    sem_post(&sharedMem->foodCounter);  /*Increase item number on counter.*/

    /*Getting values of items on counter.*/
    sem_getvalue(&sharedMem->soupCounter,&val);
    sem_getvalue(&sharedMem->mcCounter,&val2);
    sem_getvalue(&sharedMem->desertCounter,&val3);
    sem_getvalue(&sharedMem->foodCounter,&val4);
    sprintf(string,"Cook %d placed desert on the counter - counter items P:%d, C:%d, D:%d = %d\n",cookNum,val,val2,val3,val4);
    writeRes=write(STDOUT_FILENO, string, strlen(string));
    if(writeRes==-1){
        exit(EXIT_FAILURE);
    }
    sem_post(&sharedMem->m2);
    sem_post(&sharedMem->fullCounter);
}
/*Student is getting soup from counter when all 3 place is avaible.*/
void soupFromCounter(){
    sem_wait(&sharedMem->fullCounter);
    sem_wait(&sharedMem->m2);

    sem_wait(&sharedMem->soupCounter);
    sem_wait(&sharedMem->foodCounter);
    ++(sharedMem->studentFlag);

    sem_post(&sharedMem->m2);
    sem_post (&sharedMem->emptyCounter);
}
/*Student is getting  main course from counter when all 3 place is avaible.*/
void mainCourseFromCounter(){
    sem_wait(&sharedMem->fullCounter);
    sem_wait(&sharedMem->m2);

    sem_wait(&sharedMem->mcCounter);
    sem_wait(&sharedMem->foodCounter);
    ++(sharedMem->studentFlag);

    sem_post(&sharedMem->m2);
    sem_post (&sharedMem->emptyCounter);
}
/*Student is getting desert from counter when all 3 place is avaible.*/
void desertFromCounter(){
    sem_wait(&sharedMem->fullCounter);
    sem_wait(&sharedMem->m2);

    sem_wait(&sharedMem->desertCounter);
    sem_wait(&sharedMem->foodCounter);
    ++(sharedMem->studentFlag);

    sem_post(&sharedMem->m2);
    sem_post (&sharedMem->emptyCounter);
}
/*SIGINT signal handler.Exiting gracefully.*/
void sigIntHandler(int sigNo){
   if(sigNo==SIGINT){
        int writeRes;
        char string[]="\n~~~~~IN SIGINT HANDLER~~~~~\n CTRL-C entered, closing all open resources and exiting gracefully...\n";
        writeRes=write(STDOUT_FILENO, string, strlen(string));
        if(writeRes==-1){
            exit(EXIT_FAILURE);
        }
        if(sharedMem->exit==0){
            /*Close the input file.*/
            if(close(fdGlob) == -1){
                sprintf(string,"Close input file error in main function.\n");
                writeRes=write(STDOUT_FILENO, string, strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            /*Destroy the all semaphores.*/
            if (sem_destroy(&sharedMem->m) == -1 ||
                sem_destroy(&sharedMem->m2) == -1 ||   
                sem_destroy(&sharedMem->fullKitchen) == -1 || 
                sem_destroy(&sharedMem->fullTable) == -1 ||
                sem_destroy(&sharedMem->fullCounter) == -1 || 
                sem_destroy(&sharedMem->protect1) == -1 || 
                sem_destroy(&sharedMem->protect2) == -1 || 
                sem_destroy(&sharedMem->emptyKitchen) == -1 ||
                sem_destroy(&sharedMem->emptyCounter) == -1 || 
                sem_destroy(&sharedMem->emptySoup) == -1 ||
                sem_destroy(&sharedMem->emptyMC) == -1 || 
                sem_destroy(&sharedMem->emptyDesert) == -1||
                sem_destroy(&sharedMem->fullSoup) == -1 || 
                sem_destroy(&sharedMem->fullMC) == -1 || 
                sem_destroy(&sharedMem->fullDesert) == -1 || 
                sem_destroy(&sharedMem->soup) ||
                sem_destroy(&sharedMem->mainCourse) ==-1 || 
                sem_destroy(&sharedMem->desert)==-1 || 
                sem_destroy(&sharedMem->soupCounter)==-1 || 
                sem_destroy(&sharedMem->mcCounter)==-1||
                sem_destroy(&sharedMem->desertCounter) == -1 || 
                sem_destroy(&sharedMem->foodKitchen) == -1 || 
                sem_destroy(&sharedMem->foodCounter) ==-1 ||  
                sem_destroy(&sharedMem->tableCount)==-1
            ){
            sprintf(string,"Error in  SIGINT handler for sem_destroy operation. \n");
            writeRes=write(STDOUT_FILENO, string, strlen(string));
            if(writeRes==-1){
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
            }  
            sharedMem->exit=1;

            /*Remove the shared memory object specified by name.*/
            if(shm_unlink("SharedArea")==-1){
                sprintf(string,"UNLINK error in SIGINT handler!");
                writeRes=write(STDOUT_FILENO, string, strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            /*Delete the mappings.*/
            if(munmap(sharedMem, sizeof(struct SharedMemory))==-1){
                sprintf(string,"MUNMAP error in SIGINT handler!");
                writeRes=write(STDOUT_FILENO, string, strlen(string));
                if(writeRes==-1){
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            } 
            
        }    
        exit(EXIT_SUCCESS);
   }  
}



