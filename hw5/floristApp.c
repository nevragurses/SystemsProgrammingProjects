
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#define LENGTH_MAX_NAME 100 //max name length of flowers and florists.
#define MAX(a,b) (((a)>(b))?(a):(b)) //maximum of given 2 numbers.
/*Client structure*/
typedef struct client {
    int clientNumber;  //client number.
    double coordX;  //coordinate x of client.
    double coordY;  //coordinate y of client.
    char flowerName[LENGTH_MAX_NAME]; //name of flower
} client;
/*Florist structure*/
typedef struct florist {
    double coordX; //Corrdinate x of florist.
    double coordY; //Coordinate y of florist.
    int flowerCount; //flower count.
    char name[LENGTH_MAX_NAME];  //name of florist.
    double speed;  // deliver speed.
    char** flowers; //Flowers array.
    client* orderQueue;  //Client queue.
    int queueLength;  //length of queue.
    int sales;   //total sales.
    int totalTime;   //total time.

} florist;
struct florist* Florists=NULL;
struct client* Clients =NULL;
static int floristSize;
static int clientSize;
static pthread_mutex_t* mtx=NULL; //mutexes for all florists.
static pthread_cond_t* cond=NULL; //condition variables  for all florists.
static int orderFinish;
sigset_t  block_mask;
int fdGlob;
char* line1=NULL;
char*line2=NULL;
char*line3=NULL;
/*This function finds clients and florist number in file and allocates area from memory for them.*/
void find_clients_and_florists(int fdInput){
    int bytes=0,lineSize=0,z=0,first=0,gap=0;
    int clientNum=0,floristNum=0;
    char buf[1],temp[1];
    line1=(char*)malloc(1024*64 *sizeof(char*));
    do{
        if((bytes=read(fdInput,buf,1))==-1){
            perror("Failed to read input file in find_clients_and_florists function!\n");
            exit(EXIT_FAILURE);
        }
        if(buf[0]=='\n'){   
            gap++;
            lineSize++;
        }  
        else{ /*Blank line control.*/
            gap=0;
        }
        if(first==0 && gap!=2){
            temp[0]=buf[0];
            strcpy(line1,temp);
        }
        if(gap!=2 && first!=0){
            temp[0]=buf[0];
            strcat(line1,temp);
        }
        first++;  
        if(buf[0]=='\n' && gap==1){ /*End of a line is founded.*/ 
            if(strncmp(line1,"client",6) == 0 ){
                clientNum++;
            }
            else{
                floristNum++;
            }
            lineSize=0;
            first=0;
            for(z=0;z<(int)strlen(line1);z++){
                line1[z]='\0';
            }
        }   

    }while(bytes>0);
    Florists=(florist*) malloc(floristNum* sizeof(florist)); //allocate area for florists.
    floristSize=floristNum; 
    Clients=(client*) malloc(clientNum* sizeof(client));  //allocate area for clients.
    clientSize=clientNum;
    int q=0;
    while(q<floristSize){   //allocate area for queue of florists and initialize some elements.
        Florists[q].orderQueue=(client*) malloc(clientSize* sizeof(client));
        Florists[q].queueLength=0;
        Florists[q].sales=0;
        Florists[q].totalTime=0;
        q++;
    }
    orderFinish=0;


}
/*This function finds flower number of each florist and allocates area for flower array according to flower number.*/
void allocate_flowers(int fdInput){
    lseek(fdInput,0,SEEK_SET);
    int floristIndex=0,first=0;
    int bytes=0,lineSize=0,z=0,tokenCount=0,flowerNum=0,gap=0;
    char buf[1],temp[1];
    line2=(char*)malloc(1024*64*sizeof(char*));
    do{
        if((bytes=read(fdInput,buf,1))==-1){
            perror("Failed to read input file in find_clients_and_florists function!\n");
            exit(EXIT_FAILURE);
        }
        if(buf[0]=='\n'){ 
            gap++;
            lineSize++;
        }  
        else{
            gap=0;
        }
        if(first==0 && gap!=2){
            temp[0]=buf[0];
            strcpy(line2,temp);
        }
        if(gap!=2 && first!=0){
            temp[0]=buf[0];
            strcat(line2,temp);
        }
        first++;    
        if(buf[0]=='\n' && gap==1){  /*End of a line is founded.*/  
            if(strncmp(line2,"client",6) != 0 ){
                tokenCount=0;
                char*token=strtok(line2,"();:, \n");
                while(token!=NULL){
                    if(tokenCount>=4){
                        flowerNum++;
                    }
                    token=strtok(NULL,"();:, \n");
                    tokenCount++;
                }  
                Florists[floristIndex].flowerCount=flowerNum;
                Florists[floristIndex].flowers=(char**)malloc(flowerNum*sizeof(char*)); //Allocate area for flower array of each florist.
                floristIndex++;
                flowerNum=0; 
            }
            lineSize=0;
            first=0;
            for(z=0;z<(int)strlen(line2);z++){
                line2[z]='\0';
            }
        }   

    }while(bytes>0);
}
/*This function initializes florists and clients according to read file.*/
void initialize(int fdInput){
    lseek(fdInput,0,SEEK_SET);
    int floristIndex=0,clientIndex=0,flowerIndex=0,first=0;
    int bytes=0,lineSize=0,z=0,tokenCount=0,gap=0;
    char buf[1],temp[1];
    line3=(char*)malloc(1024*64*sizeof(char*));
    do{
        if((bytes=read(fdInput,buf,1))==-1){
            perror("Failed to read input file in find_clients_and_florists function!\n");
            exit(EXIT_FAILURE);
        }
        if(buf[0]=='\n'){ 
            gap++;
            lineSize++;
        }  
        else{ /*Gap line control.*/
            gap=0;
        }
        if(first==0 && gap!=2){
            temp[0]=buf[0];
            strcpy(line3,temp);
        }
        if(gap!=2 && first!=0){
            temp[0]=buf[0];
            strcat(line3,temp);
        } 
        first++;   
        if(buf[0]=='\n' && gap==1){ /*End of a line is founded.*/  
            if(strncmp(line3,"client",6) != 0 ){ //If line is florist.
                tokenCount=0;
                char*token=strtok(line3,"();:, \n");
                while(token!=NULL){
                    if(tokenCount==0){
                        strcpy(Florists[floristIndex].name,token);  //initialize name of florist.
                    }
                    else if(tokenCount==1){
                        Florists[floristIndex].coordX=atof(token); //initialize coordinate x of florist.
                    }
                    else if(tokenCount==2){
                        Florists[floristIndex].coordY=atof(token); //initialize coordinate y of florist.
                    }
                    else if(tokenCount==3){
                        Florists[floristIndex].speed=atof(token); //initialize speed of florist.
                    }
                    else if(tokenCount>=4){
                        Florists[floristIndex].flowers[flowerIndex]=(char*)malloc((strlen(token)+1)*sizeof(char));
                        strcpy(Florists[floristIndex].flowers[flowerIndex],token); //initialize flowers of florist.
                        flowerIndex++;
                    }
                    token=strtok(NULL,"();:, \n");
                    tokenCount++;
                }  
                floristIndex++;

            }
            else{ //If line is client.
                tokenCount=0;
                char*token=strtok(line3,"();:, \n");
                while(token!=NULL){
                    if(tokenCount==0){
                        Clients[clientIndex].clientNumber=clientIndex+1; //initialize client number of client.
                    }
                    else if(tokenCount==1){
                        Clients[clientIndex].coordX=atof(token); //initialize coordinate x of client.
                    }
                    else if(tokenCount==2){
                        Clients[clientIndex].coordY=atof(token); //initialize coordinate y of client.
                    }
                    else if(tokenCount==3){
                        strcpy(Clients[clientIndex].flowerName,token); //initialize flower name of client.
                    }
                    token=strtok(NULL,"();:, \n");
                    tokenCount++;
                
                }
                clientIndex++;
            }  
            flowerIndex=0; 
            first=0;
            lineSize=0;
            for(z=0;z<(int)strlen(line3);z++){
                line3[z]='\0';
            }
        }    

    }while(bytes>0);
}
/*Cherbyshev Distance function.*/
double  ChebyshevDistance(client c,florist f){
    double distance = MAX(fabs(c.coordX-f.coordX),fabs(c.coordY-f.coordY));
    return distance;
}
/*This function finds whether a flower is in given florist or not.*/
int findFlower(florist f,char* name){
    int i=0;
    for(i=0;i<f.flowerCount;i++){
        if(strcmp(f.flowers[i],name)==0){ //if flower is founded,return true.
            return 1;
        }
    }
    return 0; //flower is not founded.
}
/*Finds closest florist for given client according to ChebyshevDistance*/
int closestFlorist(client c){
    int i=0;
    int floristIndex=-5;
    double minimum= 2147483647; 
    for(i=0; i < floristSize ;i++){
        if((ChebyshevDistance(c,Florists[i]) < minimum) && (findFlower(Florists[i],c.flowerName)==1)){ 
            floristIndex=i;
            minimum=ChebyshevDistance(c,Florists[i]);
        }
    }
    return floristIndex;
    
}
/*This function to free allocated areas for florists and clients and mutexes.*/
void freeStructures(){
    int i=0;
    int j=0;
    for (i=0; i < floristSize; ++i) {
        for(j=0;j<Florists[i].flowerCount;j++){
            if(Florists[i].flowers[j]!=NULL){
                free(Florists[i].flowers[j]);
            }
        }
        if(Florists[i].orderQueue!=NULL)
            free(Florists[i].orderQueue);
        if(Florists[i].flowers!=NULL)    
            free(Florists[i].flowers);   
            
    }
    if(Florists!=NULL)
        free(Florists);
    if(Clients!=NULL)    
        free(Clients);
    if(mtx!=NULL)    
        free(mtx);
    if(cond!=NULL)    
        free(cond);
    if(line1!=NULL)    
        free(line1);
    if(line2!=NULL)    
        free(line2);
    if(line3!=NULL)    
        free(line3);        
       
}
/*Initializes thread mutexes and condition variables.*/
void initilializeMutexesAndConds(){
    mtx=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*floristSize);
    cond=(pthread_cond_t*)malloc(sizeof(pthread_cond_t)*floristSize);
    int i;
    int m,c;
    for(i=0;i<floristSize;i++){
        m=pthread_mutex_init (&mtx[i], NULL);             
        if(m!=0){
            perror("Error for thread mutex initialization!");
            exit(EXIT_FAILURE);
        }
        c=pthread_cond_init (&cond[i], NULL);
        if(c!=0){
            perror("Error for condition variable initialization!");
            exit(EXIT_FAILURE);
        }                        
    }
}
/*Florist thread function.*/
static void *florists(void *arg) {
    srand(time(NULL));
    int floristNum=*((int *)arg); //current florist.
    int prepareAndDeliverTime=0,queueIndex=0,preparetion=0;
    double distance=0;
    double deliver=0;
    int i=0;
    while(1){
        pthread_mutex_lock(&mtx[floristNum]); //lock mutex of current florist.
        while(Florists[floristNum].queueLength==0 && orderFinish!=clientSize ){ //block while queue is 0 and total delivered orders is not equal total client number.
            pthread_cond_wait(&cond[floristNum],&mtx[floristNum]);
        }
        if(orderFinish == clientSize){ //If total delivered orders is  equal to total client number.
            printf("%s closing shop.\n",Florists[floristNum].name); 
            pthread_mutex_unlock(&mtx[floristNum]); //unlock mutex of current florist.
            break;
        }    
        Florists[floristNum].queueLength -= 1;
        queueIndex=Florists[floristNum].queueLength;
        distance=ChebyshevDistance(Florists[floristNum].orderQueue[queueIndex],Florists[floristNum]);
        preparetion = (rand()%250)+1;
        deliver= distance / (double)Florists[floristNum].speed;
        prepareAndDeliverTime= preparetion + deliver;
        Florists[floristNum].sales +=1;
        usleep(prepareAndDeliverTime*1000);
        orderFinish+=1;
        Florists[floristNum].totalTime += prepareAndDeliverTime;
        printf("Florist %s has delivered a %s to client%d in %dms\n",Florists[floristNum].name,Florists[floristNum].orderQueue[queueIndex].flowerName,Florists[floristNum].orderQueue[queueIndex].clientNumber,prepareAndDeliverTime);
        if(orderFinish==clientSize){ //If total delivered orders is equal total client number.
            printf("All requests processed.\n");
            pthread_mutex_unlock(&mtx[floristNum]); //unlock current mutex.
            for(i=0;i<floristSize;i++){ //Send signals to florists that are  waiting own conditions.
                pthread_cond_signal(&cond[i]);
            }    
            printf("%s closing shop.\n",Florists[floristNum].name);
            break;
        } 
        pthread_mutex_unlock(&mtx[floristNum]);  //unlock mutex of current florist.
    }
     
    return  &Florists[floristNum]; //returning sale statistics.
}
/*Main thread function*/
void mainThread(){
    printf("Processing requests\n");
    int i;
    int floristIndex=-1;
    int queueIndex=-1;
    for(i=0;i<clientSize;i++){
        floristIndex=closestFlorist(Clients[i]); //Finds closest florist.
        pthread_mutex_lock(&mtx[floristIndex]);   //lock mutex of closest florist.

        queueIndex=Florists[floristIndex].queueLength; //current queue index.
        Florists[floristIndex].orderQueue[queueIndex]=Clients[i]; //add client to queue of closest florist.
        Florists[floristIndex].queueLength = Florists[floristIndex].queueLength+1;

        pthread_cond_signal(&cond[floristIndex]); //Send signal to closest florist.
        pthread_mutex_unlock(&mtx[floristIndex]); //unlock mutex.       
    }
}
/*Prints sale statistics by main thread on screen.*/
void saleStatistic(florist returnArr[]){
    int i=0;
    printf("\nSale statistics for today:\n");
    printf("------------------------------------------------------------------\n");
    printf("Florist                 # of sales             Total time \n");
    printf("------------------------------------------------------------------\n");
    for(i=0;i<floristSize;i++){
        printf("%-15s\t\t%-15d\t\t%dms\n",returnArr[i].name,returnArr[i].sales,returnArr[i].totalTime);
        
    }
    printf("------------------------------------------------------------------\n");
}
/*SIGINT signal handler.Exiting gracefully.*/
void sigIntHandler(int sigNo){
    if(sigNo==SIGINT){
        int i;
        char string[200];
        int writeRes=0;
        
        printf("\n~~~~~IN SIGINT HANDLER~~~~~\n CTRL-C entered, closing all open resources and exiting gracefully...\n");
        /*Mutex destroy of all florists.*/
        for(i=0;i<floristSize;i++){
            if(pthread_mutex_destroy(&mtx[i])!=0){
                sprintf(string, "Pthread mutex destroy error in handler\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            
            }
        } 
        /*Mutex destroy of all florists.*/ 
        for(i=0;i<floristSize;i++){
            if(pthread_cond_destroy(&cond[i])!=0){
                sprintf(string, "Error in pthread_cond_destroy\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }  
        /*Close file.*/
        if(close(fdGlob)<0){
            perror("Close file error in handler\n");
            exit(EXIT_FAILURE);
        }
        freeStructures(); //Free all allocated areas.  
        exit(EXIT_SUCCESS);
    }   

}
/*main function.*/
int main(int argc, char *argv[]){
    char *inputFile;

    int option;
    /*getting command line arguments.*/
    while((option = getopt(argc, argv, ":i:")) != -1){ 
        switch(option){
            case 'i':
                inputFile=optarg;
                break;  
            case '?': 
                perror("It does not fit format!Format must be: ./floristApp -i fileName\n");
                exit(EXIT_FAILURE);
                break;
        }
    }
    if((optind!=3)){
        errno=EINVAL;
        perror("CommandLine argument numbers must be 3! Format must be: /floristApp -i fileName\n");
        exit(EXIT_FAILURE);
    } 
    int fdInput;
    /*Open input file*/
    if((fdInput=open(inputFile,O_RDONLY))==-1){
      perror("Failed to open input file in main.\n");
      exit(EXIT_FAILURE);
    }
    fdGlob=fdInput;
     
    /*For SIGINT (ctrl_c)*/ 
    struct sigaction act;
    sigemptyset(&act.sa_mask); 
    act.sa_handler = sigIntHandler;
    act.sa_flags = 0;
    if(sigaction (SIGINT, &act, NULL)==-1){
        perror("SIGACTION error in main for SIGINT\n");
        exit(EXIT_FAILURE);
    } 
    
    printf("Florist application initializing from file: %s\n",inputFile);  
    find_clients_and_florists(fdInput); //finds clients and florist number and allocates area for florists and clients.
    lseek(fdInput,0,SEEK_SET);
    allocate_flowers(fdInput); //allocates area for flower array of each florist.
    initialize(fdInput); //initializes florists and clients.
    /*Masks SIGINT signal while threads are working*/
    int i,s;
    pthread_t thread_florists[floristSize];
    int save[floristSize];
    char string[200];
    int writeRes;
    florist returnArr[floristSize];
    void *returnValue;

    sigset_t mask;
    if( (sigemptyset(&mask)==-1)  ||(sigaddset(&mask,SIGINT)==-1)){
        perror("Failed to initialize the signal mask in leastSquaresMethod function.");
        exit(EXIT_FAILURE);
    }
    if(sigprocmask(SIG_BLOCK,&mask,NULL)==-1){
        perror("Failed to blocking signals operation in leastSquaresMethod function.");
        exit(EXIT_FAILURE);
    }
    initilializeMutexesAndConds(); //initializes mutexes and condition variables.
    
    /*Creating threads for florists.*/
    for (i = 0; i < floristSize; i++) {
        save[i] = i;
        if ((s = pthread_create(&thread_florists[i], NULL,florists,(void*) &save[i])) != 0)
        {
            sprintf(string, "Failed to pthread_create operation in main.\n");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
    }
    printf("%d florists have been created\n",floristSize);
    mainThread(); //Main thread function.   
    /*Waiting for florists by main thread.*/
    for (i = 0; i < floristSize; i++)
    {
        if ((s = pthread_join(thread_florists[i],&returnValue)) != 0)
        {
            sprintf(string, "Pthread_join error in main");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        florist* casting=(florist*)returnValue;
        returnArr[i]=*(casting);
    }
    if(sigprocmask(SIG_UNBLOCK,&mask,NULL)==-1){
        perror("Failed to unblocking signals operation in leastSquaresMethod function.");
        exit(EXIT_FAILURE);
    }
    saleStatistic(returnArr); //main thread is printing sale statistics.
    /*Mutex destroy of all florists.*/
    for(i=0;i<floristSize;++i){
        if(pthread_mutex_destroy(&mtx[i])!=0){
            sprintf(string, "Pthread mutex destroy error in main");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
          
        }
    } 
    /*Mutex destroy of all florists.*/ 
    for(i=0;i<floristSize;++i){
        if(pthread_cond_destroy(&cond[i])!=0){
            printf("Error in Condition destroy\n");
            exit(EXIT_FAILURE);
        }
    }
    /*Close file.*/
    if(close(fdInput)<0){
        perror("Close file error in main\n");
        exit(EXIT_FAILURE);
    }
    freeStructures(); //Free all allocated areas. 
    return 0;


}