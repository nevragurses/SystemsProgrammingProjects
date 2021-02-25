#include <stdio.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <stdatomic.h>
#define SA struct sockaddr 

#define MAX(a,b) (((a)>(b))?(a):(b)) //gives maximum of given 2 numbers.
/*Global pointers that are using while reading file.*/
char* line=NULL; 
char *line2=NULL;
FILE* fpOutput; //Log file pointer.
/*Node for cache structure.*/
typedef struct innerNode {
    int lastNum; //last number of path.
    char* path; //path between 2 nodes.
    int path_size; //size of path.
} innerNode;
/*Cache structure.*/
typedef struct cache {
    innerNode** pathArr;  //2D Innernode pointer to keep path between 2 node. 
    int pathArrLen; //keeps different source nodes.
    int *innerNode_size; //keeps path sizes that are different source nodes.
} cache;
cache Cache;
/*Node structure for graph representation*/
struct node 
{ 
    int dest; 
    struct node* next;
    int end; 
}; 
/*Adjacency list structure of graph*/
struct AdjList 
{ 
    struct node *head;  
}; 
/*Graph structure.*/    
struct Graph 
{ 
    int V; 
    struct AdjList* adjList; 
    int usedV;
}; 
/*Queue structure.*/
struct queue
{ 
    int front; 
    int* array;
    int end;
    int size; 
}; 
/*Mutexes and condition variables for threads*/ 
static pthread_mutex_t* mtx=NULL; 
static pthread_cond_t* condFull=NULL; 
static pthread_cond_t* condEmpty=NULL; 
/*Condition variables for reader-writer paradigm.*/
static pthread_cond_t readOk;
static pthread_cond_t  writeOk;
static pthread_cond_t  dynamic_condVar;
static pthread_mutex_t m; //mutex for reader/writer paradigm.
static pthread_mutex_t dynamic_mtx;
pthread_t* thread_server; //Server threads.
pthread_t dynamic_poll_thread; //dynamic pool thread.
int edgeNumber=0;
int* save;
/*Global variables for reader-writer paradigm.*/
int activeRd;
int activeWr;
int waitingWr;
int waitingRd;
int** queue;  //Client queue.
int* queueSize;
struct Graph* graph; 
int Vertex; //vertex number.
int startThreadNum; //thread number at beginning.
int lastThreadNum; //max thread number.
int currRunningNum;  //Current running thread number.
int readerPrioritization;  //Reader prioritization.
atomic_int sigintFlag=0;/*Sigint flag*/
/*This function finds total node number in file.*/
int findNodeNum(FILE* fdInput){
    ssize_t nread;  
    size_t len = 0;
    int  node=-1;
    while ((nread = getline(&line, &len, fdInput)) != -1) {
        if(line[0]!='#'){
            edgeNumber=edgeNumber+1;
            char*token=strtok(line,"\t\n");  
            while(token!=NULL){
                int num=atoi(token);
                node=MAX(num,node);
                token=strtok(NULL,"\t\n");
            }
        }   
    }
    return node+1;
}    
/****BELOW  9 FUNCTIONS FOR GRAPH REPRESENTATION******/

/*This function creates adjacency list node.*/
struct node* adjListNode(int dest) 
{ 
    struct node* newNode = (struct node*) malloc(sizeof(struct node)); 
    newNode->dest = dest; 
    newNode->next = NULL; 
    return newNode; 
}  
/*This function initalizes graph for V vertex.*/
struct Graph* initializeGraph() 
{ 
    
    struct Graph* graph =  (struct Graph*) malloc(sizeof(struct Graph)); 
    graph->V = Vertex; 
    graph->usedV = 0;

    graph->adjList =  (struct AdjList*) malloc(Vertex * sizeof(struct AdjList)); 
  
    int i; 
    for (i = 0; i < Vertex; ++i) 
        graph->adjList[i].head = NULL; 
  
    return graph; 
} 
/*This function gets node list of given source.*/
struct node* getNodeList(int src){
    struct node* curr = graph->adjList[src].head; 
    fprintf(fpOutput,"\n Adjacency list of vertex %d\n head ",src); 
    while (curr) 
    { 
        fprintf(fpOutput,"-> %d", curr->dest);
        curr = curr->next; 
    } 
    fprintf(fpOutput,"\n");
    return curr;
} 
/*This function finds connected node number of given source.*/
int elementNum(int src){
    int count=0;
    struct node* curr = graph->adjList[src].head; 
    while (curr) 
    { 
        count++;
        curr = curr->next; 
    }

    return count;
}
/*This function creates edge between 2 node that are source and destination.*/
void addEdge( int src, int dest) 
{ 
    if(graph->adjList[src].head == NULL){ //creating first edge.
        struct node* newNode = adjListNode(dest);
        graph->adjList[src].head = newNode; 
        graph->usedV =graph->usedV + 1;
    }
    else{
        struct node* curr = graph->adjList[src].head;
        while(curr->next!=NULL){
            curr=curr->next;
        } 
        struct node* newNode = adjListNode(dest);
        curr->next=newNode;
    }     
}
/*This function controls whether there is an edge between given 2 node or not.*/
int isEdge(int src, int destination){
    struct node* curr = graph->adjList[src].head; 
    while (curr!=NULL) 
    { 
        if(curr->dest==destination){
            return 1;
        }    
        curr=curr->next;
    }
    return 0;

}
/*This function prints graph structure.*/
void printGraph() 
{ 
    int v; 
    for (v = 0; v < graph->V; ++v) 
    { 
        struct node* curr = graph->adjList[v].head; 
        fprintf(fpOutput,"\n Adjacency list of vertex %d\n head ", v);
        while (curr) 
        { 
            fprintf(fpOutput,"-> %d", curr->dest); 
            curr = curr->next; 
        } 
        fprintf(fpOutput,"\n");
    } 
}
/*This function creates graph structure according to reading input file.*/
void createGraph(FILE* fdInput){
    fseek(fdInput,0,SEEK_SET);
    ssize_t nread;  
    size_t len = 0;
    int pair[2];
    int pairIndex=0;
    while ((nread = getline(&line2, &len, fdInput)) != -1) {
        if(line2[0]!='#'){
            char*token=strtok(line2,"\t\n");  
            while(token!=NULL){
                int num=atoi(token);
                pair[pairIndex]=num;
                pairIndex=pairIndex+1;
                token=strtok(NULL,"\t\n");
            }
            addEdge(pair[0],pair[1]);
        } 
        pairIndex=0;
    }
}
/*This function frees allocated graph.*/
void freeAllocatedGraph(){
    int v; 
    for (v = 0; v < graph->V; ++v) 
    { 
        struct node* curr = graph->adjList[v].head; 
        struct node* before;
        while (curr) 
        { 
            before=curr;
            curr = curr->next; 
            free(before);
        } 
    }
    free(graph->adjList );
    free(graph); 
    
}
   
/****BELOW  5 FUNCTIONS FOR CACHE STRUCTURE*****/

/*This function initializes cache structure.*/
void initializeCache(){
    Cache.pathArr=(innerNode**)malloc(Vertex*sizeof(innerNode*));
    Cache.pathArrLen=0;
    Cache.innerNode_size=(int*)malloc(Vertex*sizeof(int));
    int i;
    for(i=0;i<Vertex;i++){
        Cache.innerNode_size[i]=0;
    }
}
/*This function frees cache structure.*/
void freeCache(){
    int i,j;
    for(i=0;i<Vertex;i++){
        for(j=0;j<Cache.innerNode_size[i];j++){
            free( Cache.pathArr[i][j].path);
        }
        if(Cache.innerNode_size[i]!=0)
            free(Cache.pathArr[i]);
    }  
    free(Cache.innerNode_size);
    free(Cache.pathArr);  
}
/*This function adds path for given source and destination in cache structure.*/
void add_Cache(char* findedPath,int source,int dest){
  
    int index=source;
    int pathLength=(int)strlen(findedPath);
   
   
    if(Cache.innerNode_size[index]==0){ /*For every different source node*/
      
        int count=Cache.innerNode_size[index];
        Cache.pathArr[index]=(innerNode*)malloc(sizeof(innerNode));
        Cache.pathArr[index][count].path=(char*)malloc((pathLength+1)*sizeof(char));

        strcpy(Cache.pathArr[index][count].path,findedPath);

        Cache.pathArr[index][count].path_size=pathLength;
        Cache.innerNode_size[index]=1;
        Cache.pathArrLen = 1;
        Cache.pathArr[index][count].lastNum=dest;
       
       
    }
    else{/*For source nodes that are created before.New paths added innerNode structure of that source node.*/
        Cache.pathArr[index]=(innerNode*)realloc(Cache.pathArr[index],(Cache.innerNode_size[index]+1)*sizeof(innerNode));
        int count=Cache.innerNode_size[index];
        Cache.pathArr[index][count].path=(char*)malloc((pathLength+1)*sizeof(char));
        strcpy(Cache.pathArr[index][count].path,findedPath);
        Cache.pathArr[index][count].path_size=pathLength; 
        Cache.innerNode_size[index]=Cache.innerNode_size[index]+1;
        Cache.pathArr[index][count].lastNum=dest;
       
    }
}
/*This function gets path of given source and destination nodes from cache structure.*/
char* getPath(int src,int dest){  
    int j=0;
    int keep;
    char* returnPath=(char*)malloc(900000);
    for(j=0;j<Cache.innerNode_size[src];j++){
        if(dest == Cache.pathArr[src][j].lastNum){
            keep=j;
            
        }
    }
    strcpy(returnPath,Cache.pathArr[src][keep].path);
    return returnPath;
}
/*This function controls cache for finding whether given path is in cache or not.*/
int controlCache(int source,int dest){
    int j=0;
    for(j=0;j<Cache.innerNode_size[source];j++){
        if(dest == Cache.pathArr[source][j].lastNum){
            return 1;
        }
    }
     
    return 0;
}
/********BELOW 7 FUNCTIONS FOR QUEUE STRUCTURE.**********/

/*Creating queue structure*/
struct queue* createQueue() 
{ 
    struct queue* Queue =  (struct queue*) malloc(sizeof(struct queue)); 
    Queue->front = -1; 
    Queue->end = -1;
    Queue->size = 0;


    Queue->array =  (int*) malloc(Vertex * sizeof(int)); 
  
    int i; 
    for (i = 0; i < Vertex; ++i) 
        Queue->array[i] = 0; 
  
    return Queue; 
} 
/*Inserting element in queue.*/
void addQueue(struct queue* Queue, int item) 
{ 
    if (Queue->front == - 1)
        Queue->front = 0;
    
    Queue->end=(Queue->end)+1;
    Queue->array[Queue->end] = item;
    Queue->size=Queue->size+1;
    
} 
/*Deleting element from queue.*/   
void deleteQueue(struct queue* Queue) 
{ 
    if (Queue->front == - 1 || Queue->front > Queue->end)
    {
        return ;
    }
    else
    {
        Queue->front = Queue->front + 1;
        Queue->size =Queue->size-1;
    }
} 
/*This function for controlling whether queue is empty or not.*/
int controlEmpty(struct queue* Queue){
    if(Queue->size==0){
        return 1;
    }
    return 0;
} 
/*Getting first number from queue.*/ 
int getFront(struct queue* Queue){
    return  Queue->array[Queue->front];
}
/*Getting last number from queue.*/
int getRear(struct queue* Queue){
    return Queue->array[Queue->end];
}
/*This function for displaying queue elements.*/
void display(struct queue* Queue)
{
    int i;
    if ( Queue->front == - 1)
        fprintf(fpOutput,"Queue is empty \n");
    else
    {
        fprintf(fpOutput,"Queue is : ");
        for (i = Queue->front; i <= Queue->end; i++)
            fprintf(fpOutput,"%d ", Queue->array[i]);
        fprintf(fpOutput,"\n");
    }
} 
/*For free queue*/
void freeBFSQueue(struct queue* Queue){
    free(Queue->array);
    free(Queue);
}
/* Breadth-first search function for finding path.This is helper function for finding path function.*/
int BFS(int src, int dest,int pred[], int dist[]) 
{ 
   
    struct queue* Queue=createQueue();
   
    int* visited=(int*)malloc(sizeof(int)*2*Vertex);
    int i;
    for(i=0;i<Vertex;i++){
        visited[i]=0;
    }
   
  
    for (int i = 0; i < Vertex; i++) { 
        visited[i] = 0; 
        dist[i] = 2147483647; 
        pred[i] = -1; 
    } 
  

    visited[src] = 1; 
    dist[src] = 0; 
    addQueue(Queue,src);
  
   
    while (controlEmpty(Queue) == 0) { 
        int u =  getFront(Queue);
        deleteQueue(Queue);
        for (int i = 0; i < Vertex; i++) { 
                if(isEdge(u,i) == 1){
                    if (visited[i] == 0) { 
                        visited[i] = 1; 
                        dist[i] = dist[u] + 1; 
                        pred[i] = u; 
                        addQueue(Queue,i);
                        if (i == dest) {
                            freeBFSQueue(Queue);
                            free(visited);
                            return 1; 
                        }    
                     } 
                 }
        }    
    } 
    freeBFSQueue(Queue);
    free(visited);
    return 0; 
}
/*Finds shortest path between given 2 nodes.It applys BFS search algorithm for finding path.*/
char* getPathWithBFS(int s,int dest) 
{
    int* pred=(int*)malloc(2*Vertex*sizeof(int));
    int* dist=(int*)malloc(2*Vertex*sizeof(int));
     int i;
    for(i=0;i<Vertex;i++){
        pred[i]=0;
        dist[i]=0;
    }

    if (BFS( s, dest, pred, dist) == 0) {  /*If there is no path between given 2 nodes.*/
        free(pred);
        free(dist);
       
        return NULL;
    } 
    
    int* path=(int*)malloc(Vertex*sizeof(int));
    int pathLen=0; 
    int crawl = dest; 
    path[pathLen]=crawl;
    pathLen++;
    while (pred[crawl] != -1) { 
        path[pathLen]=pred[crawl]; 
        pathLen++;
        crawl = pred[crawl]; 
    } 
  
    char path2[10];
    char* total=(char*)malloc(sizeof(char));
    int maxIntLen=10;
    for (int i = pathLen- 1; i >= 0; i--) {
        if(i==0){
            sprintf(path2,"%d",path[i]);
        }    
        else{
            sprintf(path2,"%d->",path[i]);
        }    
        if(i==pathLen-1){
            total=(char*)realloc(total,maxIntLen*sizeof(char));
            strcpy(total,path2);
        }    
        else{
            total=(char*)realloc(total,maxIntLen*sizeof(char));
            strcat(total,path2);
        }  
        maxIntLen=maxIntLen+20;  
    } 
    
    free(path);
    free(pred);
    free(dist);
    return total; 
} 

/*Initializes thread mutexes and condition variables.*/
void initilializeMutexesAndConds(){
    mtx=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t)*(lastThreadNum+1));
    condFull=(pthread_cond_t*)malloc(sizeof(pthread_cond_t)*(lastThreadNum+1));
    condEmpty=(pthread_cond_t*)malloc(sizeof(pthread_cond_t)*(lastThreadNum+1));
    int i;
    int m1,m2,m3,c,c2,c3,c4,c5;
    for(i=0;i<lastThreadNum+1;i++){
        m1=pthread_mutex_init (&mtx[i], NULL);             
        if(m1!=0){
            fprintf(fpOutput,"Error for thread mutex initialization!");
            exit(EXIT_FAILURE);
        }
        c=pthread_cond_init (&condEmpty[i], NULL);
        if(c!=0){
            fprintf(fpOutput,"Error for condition variable initialization!");
            exit(EXIT_FAILURE);
        }  
        c2=pthread_cond_init (&condFull[i], NULL);
        if(c2!=0){
            fprintf(fpOutput,"Error for condition variable initialization!");
            exit(EXIT_FAILURE);
        }                          
    }
    m2=pthread_mutex_init (&m, NULL);             
    if(m2!=0){
        fprintf(fpOutput,"Error for thread mutex initialization!");
        exit(EXIT_FAILURE);
    }
    m3=pthread_mutex_init (&dynamic_mtx, NULL);             
    if(m3!=0){
        fprintf(fpOutput,"Error for thread mutex initialization!");
        exit(EXIT_FAILURE);
    }
    c3=pthread_cond_init (&readOk, NULL);
    if(c3!=0){
        fprintf(fpOutput,"Error for condition variable initialization!");
        exit(EXIT_FAILURE);
    } 
    c4=pthread_cond_init (&writeOk, NULL);
    if(c4!=0){
        fprintf(fpOutput,"Error for condition variable initialization!");
        exit(EXIT_FAILURE);
    }
    c5=pthread_cond_init (&dynamic_condVar, NULL);
    if(c5!=0){
        fprintf(fpOutput,"Error for condition variable initialization!");
        exit(EXIT_FAILURE);
    }              
    activeRd=0;
    activeWr=0;
    waitingRd=0;
    waitingWr=0;
    currRunningNum=0;
}
/*Getting time as microsecond.After using,this microsecond will be converted second.*/
unsigned long getTime(){
    struct timeval timeValue;
    gettimeofday(&timeValue,NULL);
    return 1000000 * timeValue.tv_sec + timeValue.tv_usec;
}
/*Function for comminication between client and server.Messages is sending to client in this function.*/
/*And also reader-writer mechanism is used for informations of database structure.*/
void func(int sockfd,int threadNum) 
{ 
	int buff[2];
    

    int cacheRes=0;
    bzero(buff,2);   
    read(sockfd, buff, sizeof(buff));  //getting 2 nodes that are source and destination nodes from buffer on socket.
  
    
    if(readerPrioritization==0){ /*If there is reader prioritization.*/
        pthread_mutex_lock(&m);
        while((activeWr+activeRd)>0){
            waitingRd++;
            pthread_cond_wait(&readOk,&m);
            waitingRd--;
        }
    }
    else if(readerPrioritization==1){ /*If there is writer prioritization*/
        pthread_mutex_lock(&m);
        while((activeWr+waitingWr)>0){
            waitingRd++;
            pthread_cond_wait(&readOk,&m);
            waitingRd--;

        }
    }
    else if(readerPrioritization==2){ /*If there is equal  priorities to readers and writers */
        pthread_mutex_lock(&m);
    }
    activeRd++;
    pthread_mutex_unlock(&m); 
    cacheRes=controlCache(buff[0],buff[1]); //control cache by reader writer paradigm for controlling whether given source and dest nodes in cache or not.
    pthread_mutex_lock(&m);
    activeRd--;
    if(readerPrioritization==0){
        if(waitingRd>0){
            pthread_cond_signal(&readOk);
        }
        else if(waitingWr>0){
            pthread_cond_broadcast(&writeOk);
        }
        pthread_mutex_unlock(&m);
    }
    else if(readerPrioritization==1){
        if(activeRd==0 && waitingWr>0)
            pthread_cond_signal(&writeOk);     
        pthread_mutex_unlock(&m); 
    }
    else if(readerPrioritization==2){
        pthread_mutex_unlock(&m);
    }
   
    if(cacheRes==0){/*If given source and destination is not in cache.*/
        fprintf(fpOutput,"[ %f sec]Thread #%d: no path in database, calculating %d->%d\n",getTime()/1000000.0,threadNum,buff[0],buff[1]); 
        char* buff2= getPathWithBFS(buff[0],buff[1]); /*make bfs for finding path.*/
        if(buff2==NULL){ /*If there is no path between given nodes,print message.*/
            fprintf(fpOutput,"[ %f sec]Thread #%d: path not possible from node %d to %d\n",getTime()/1000000.0,threadNum,buff[0],buff[1]);
            write(sockfd,"NO PATH", strlen("NO PATH"));
            free(buff2);
        }
        else{//If there is a path between two nodes,add cache by using reader-writer paradigm and send path for client by socket.*/
            fprintf(fpOutput,"[ %f sec]Thread #%d: path calculated:%s\n",getTime()/1000000.0,threadNum,buff2);
            if(readerPrioritization==0){ //reader prioritization.
                pthread_mutex_lock(&m);
                while((activeRd+waitingRd)>0){
                    waitingWr++;
                    pthread_cond_wait(&writeOk,&m);
                    waitingWr--;
                }
            }
            else if(readerPrioritization==1){//Writer prioritization.
                pthread_mutex_lock(&m);
                while((activeWr+activeRd)>0){
                    waitingWr++;
                    pthread_cond_wait(&writeOk,&m);
                    waitingWr--;

                }
            }
            else if(readerPrioritization==2){//if there is equal prioritization.
                pthread_mutex_lock(&m);
            }
            
            activeWr++;
            pthread_mutex_unlock(&m); 
          
            fprintf(fpOutput,"[ %f sec]Thread #%d: responding to client and adding path to database\n",getTime()/1000000.0,threadNum);
            add_Cache(buff2,buff[0],buff[1]); //adding cache finded path.
         
            pthread_mutex_lock(&m);
            activeWr--;
            if(readerPrioritization==0){
                if(activeWr==0 && waitingRd >0){
                    pthread_cond_signal(&readOk);
                }
                pthread_mutex_unlock(&m);
            }
            else if(readerPrioritization==1){
                if(waitingWr>0)
                    pthread_cond_signal(&writeOk);  
                else if(waitingRd>0)
                    pthread_cond_broadcast(&readOk);        
                pthread_mutex_unlock(&m); 
            }
            else if(readerPrioritization==2){
                pthread_mutex_unlock(&m);
            }

            write(sockfd,buff2, strlen(buff2));
            free(buff2);
        }    
    }
    else{//If given source and dest is found in cache,get path from cache and send client.
        fprintf(fpOutput,"[ %f sec]Thread #%d: searching database for a path from node %d to node %d\n",getTime()/1000000.0,threadNum,buff[0],buff[1]);
        char* buff3=getPath(buff[0],buff[1]);
        fprintf(fpOutput,"[ %f sec]Thread #%d:  path found in database:%s\n",getTime()/1000000.0,threadNum,buff3);
        write(sockfd,buff3, strlen(buff3));
        free(buff3);
       
    }
    close(sockfd);
} 
/*Thread function.*/
static void *threadFunc(void *arg) {
    int serverNum=*((int *)arg);   //current thread number.
    
    fprintf(fpOutput,"[ %f sec]Thread #%d: waiting for connection\n",getTime()/1000000.0,serverNum);
   
    while(1){
       
        pthread_mutex_lock(&mtx[serverNum]); //lock mutex of current thread.
     
        while( queueSize[serverNum]==0 && sigintFlag==0 ){//block while queue is 0.
            pthread_cond_wait(&condFull[serverNum],&mtx[serverNum]);
        }

        if(sigintFlag==1){ //If sigint signal come, exit thread.
            pthread_mutex_unlock(&mtx[serverNum]);
            fprintf(fpOutput,"[ %f sec] thread is closing because SIGINT signal come. \n",getTime()/1000000.0); 
            break;
        }
       
        int fdSoc=queue[serverNum][queueSize[serverNum] - 1];//add file descriptor of common address in queue of current thread.
        queueSize[serverNum] = queueSize[serverNum]-1;//update queue size of current thread.
        currRunningNum=currRunningNum+1;
        func(fdSoc,serverNum);//function for writing result in client.
        pthread_cond_signal(&condEmpty[serverNum]); 
        pthread_mutex_unlock(&mtx[serverNum]);  
        currRunningNum=currRunningNum-1;

    }  
    return 0;  
} 
/*Dynamic pool thread function.*/
void *dynamic_control(void* arg) {
    int s=*((int *)arg);
    while(1){
        int i;
        pthread_mutex_lock(&dynamic_mtx);
        while((startThreadNum>=lastThreadNum || (4*(double)currRunningNum/(double)startThreadNum) < 3) && sigintFlag==0 ){
            pthread_cond_wait(&dynamic_condVar,&dynamic_mtx);
        }

        if(sigintFlag==1){ //If sigint signal come,close thread.
            pthread_mutex_unlock(&dynamic_mtx);
            fprintf(fpOutput,"[ %f sec]Dynamic thread exiting because  SIGINT signal come.\n",getTime()/1000000.0);
            break;
        }

        double newAmount=((double)startThreadNum*25.0)/100.0; //increase loading amount by 25%.
        if((startThreadNum+(int)newAmount)>lastThreadNum){
            newAmount=newAmount- ((startThreadNum+(int)newAmount) - lastThreadNum); 
            startThreadNum=lastThreadNum;
        }
        fprintf(fpOutput,"[ %f sec]System load 75 %%, pool extended to %d threads\n",getTime()/1000000.0,(startThreadNum+ (int)newAmount)); 
        thread_server=(pthread_t*)realloc(thread_server,(startThreadNum+(int)newAmount)*sizeof(pthread_t));
       


        for(i=startThreadNum; i < startThreadNum+(int)newAmount;i++){//Creating new threads.
            save[i] = i;
            if ((s = pthread_create(&thread_server[i], NULL,threadFunc,(void*) &save[i])) != 0)
            {
                exit(EXIT_FAILURE);
            }
        }
        startThreadNum=startThreadNum+(int)newAmount;
        pthread_mutex_unlock(&dynamic_mtx); 
    }
    return 0;
} 
/*Initializing queue of file pointer of accept address of socket.*/ 
void initializeQueue(){
    queue=(int**)malloc((lastThreadNum+1)*sizeof(int*));
    int i;
    for(i=0;i<lastThreadNum+1;i++){
        queue[i]=(int*)malloc(500*sizeof(int));
    }

    queueSize=(int*)malloc((lastThreadNum+1)*sizeof(int));
    for(i=0;i<lastThreadNum+1;i++){
        queueSize[i]=0;
   
    }
}
/*Free queue.*/
void freeQueue(){
    int i;
    for(i=0;i<lastThreadNum+1;i++){
        free(queue[i]);

    }
    free(queue);
    free(queueSize);
}
/*Daeomon code.*/
void daemon_code(FILE* secondServer){
        unlink("SECOND_SERVER");
        fclose(secondServer);
        pid_t pid, sid;
        
        pid = fork();
        if (pid < 0) {
                exit(EXIT_FAILURE);
        }
       
        if (pid > 0) {
                exit(EXIT_SUCCESS);
        }
 
        umask(0);
                
        sid = setsid();
        if (sid < 0) {
              
            exit(EXIT_FAILURE);
        }
    
        if ((chdir(".")) < 0) {
            exit(EXIT_FAILURE);
        }
        
      
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
}

/*SIGINT handler function */
void handler(int signum){
    fprintf(fpOutput,"[ %f sec ] %d Termination signal received, waiting for ongoing threads to complete.\n",getTime()/1000000.0,signum);
    sigintFlag=1; //change flag.
  
}

// Main function.
int main(int argc, char *argv[]) { 
    char *pathToFile,*pathToLogFile;
    int  option,port;
    sigintFlag=0;

    struct sigaction action;
    sigemptyset(&action.sa_mask); 
    action.sa_flags = 0;
    action.sa_handler = handler;

     /*getting command line arguments.*/
    while((option = getopt(argc, argv, ":i:p:o:s:x:r:")) != -1){ 
      switch(option){
        case 'i':
            pathToFile=optarg;
            break;
        case 'p': 
            port = atoi(optarg);
            break; 
        case 'o':
            pathToLogFile=optarg;
            break;
        case 's': 
            startThreadNum = atoi(optarg);
            break; 
        case 'x': 
            lastThreadNum = atoi(optarg);
            break;
        case 'r': 
            readerPrioritization = atoi(optarg);
            break;                 
        case '?':
        perror("It does not fit format!Format must be: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24 -r readerPrioritization\n");
        exit(EXIT_FAILURE);
        break;
      }
    }
    if((optind!=13)){
        errno=EINVAL;
        perror("CommandLine argument numbers must be 13! Format must be: ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24 -r readerPrioritization \n");
        exit(EXIT_FAILURE);
    }
    int controlSecondServer;
    controlSecondServer=access("SECOND_SERVER",F_OK); /*For controlling whether there is already second server program or not.*/
    FILE*  secondServer; 
    /*If there is a second server program,print error.*/
    if(controlSecondServer!=-1){
        perror("There is already a server file exist.\n");
        exit(EXIT_FAILURE);
    }
    else{
        secondServer=fopen("SECOND_SERVER","w+");
        if(secondServer==NULL){
            perror("fopen error in main\n");
            exit(EXIT_FAILURE);
        }
    } 
   
    daemon_code(secondServer);

    fpOutput=fopen(pathToLogFile,"w+");/*Open log file.*/
    if (fpOutput == NULL) {
        fprintf(fpOutput,"fopen error in main\n");
        exit(EXIT_FAILURE);
    }

    if (sigaction(SIGINT, &action, NULL) != 0){
        exit(EXIT_FAILURE);
    }

    fprintf(fpOutput,"Executing with parameters: \n");
    fprintf(fpOutput,"-i %s\n",pathToFile);
    fprintf(fpOutput,"-p %d\n",port);
    fprintf(fpOutput,"-o %s\n",pathToLogFile);
    fprintf(fpOutput,"-s %d\n",startThreadNum);
    fprintf(fpOutput,"-x %d\n",lastThreadNum);
    fprintf(fpOutput,"-r %d\n",readerPrioritization);
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli; 
    socklen_t len;
     // socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		fprintf(fpOutput,"socket creation failed...\n"); 
		exit(EXIT_FAILURE); 
	} 
	
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	servaddr.sin_port = htons(port); 

    int flag = 1;  
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {  
        fprintf(fpOutput,"setsockopt fail");  
        exit(EXIT_FAILURE);
    }  
	// Binding newly created socket to given IP and verification. 
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
		fprintf(fpOutput,"socket bind failed because of "); 
        fprintf(fpOutput, "%s\n", strerror(errno)); 
        fclose(fpOutput);
		exit(EXIT_FAILURE); 
	} 

	// server is ready to listen and verification 
	if ((listen(sockfd, 5)) != 0) { 
		fprintf(fpOutput,"Listen failed...\n"); 
		exit(EXIT_FAILURE); 
	} 
	
	len = sizeof(cli);

    FILE *fdInput; 
    fdInput = fopen(pathToFile, "r");
    if (fdInput == NULL) {
        fprintf(fpOutput,"fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(fpOutput,"[ %f sec]Loading graph...\n",getTime()/1000000.0);
    unsigned long startGraph=getTime();
  
    Vertex = findNodeNum(fdInput);
    graph = initializeGraph(); 
    createGraph(fdInput);
    fprintf(fpOutput,"[ %f sec]Graph loaded in %f seconds with %d nodes and %d edges\n",getTime()/1000000.0,(getTime()-startGraph)/(double)1000000,Vertex,edgeNumber);
    initializeCache();

   

    thread_server=(pthread_t*)malloc(startThreadNum*sizeof(pthread_t));
    
    save=(int*)malloc((lastThreadNum+1)*sizeof(int));
    int i,s,s2;
   
    initilializeMutexesAndConds(); //initializes mutexes and condition variables.

    initializeQueue();//initializing queue.
     /*Creating threads for first threads.*/
    for (i = 0; i < startThreadNum; i++) {
        save[i] = i;
        if ((s = pthread_create(&thread_server[i], NULL,threadFunc,(void*) &save[i])) != 0)
        {
            fprintf(fpOutput,"Failed to pthread_create operation in main.\n");
            exit(EXIT_FAILURE);
        }
    }
    int sendParameter=0;
    /*Creating thread for dynamic pool.*/
    if ((s2 = pthread_create(&dynamic_poll_thread, NULL,dynamic_control,(void*)&sendParameter)) != 0){
        fprintf(fpOutput, "Failed to pthread_create operation in main.\n");
       
        exit(EXIT_FAILURE);
    }
	fprintf(fpOutput,"[ %f sec]A pool of %d threads has been created\n",getTime()/1000000.0,startThreadNum);
   
  
    int threadIndex=0;
    while(1){
        if(sigintFlag==1){/*If sigint signal come,close thread*/
            break;
        }    
        else{
            if(sigintFlag==0){
                connfd = accept(sockfd, (struct sockaddr*)&cli, &len); //Accepting socked fd's from client.
                if (connfd < 0 && sigintFlag==0) { 
                    fprintf(fpOutput,"server acccept failed...\n"); 
                    exit(EXIT_FAILURE); 
                }
            }
        }
        if(sigintFlag==1){
            break;
        }      
        pthread_mutex_lock(&mtx[threadIndex]);  
        pthread_mutex_lock(&dynamic_mtx);  
        fprintf(fpOutput,"[ %f sec]A connection has been delegated to thread id #%d, system load %f %%\n",getTime()/1000000.0,threadIndex,((100.0*(double)currRunningNum)/(double)startThreadNum)); 
    
        while(queueSize[threadIndex]==500 && sigintFlag==0){ /*If queue size is over,wait.*/
            pthread_cond_wait(&condEmpty[threadIndex],&mtx[threadIndex]);
        }
        if(sigintFlag==1){/*If sigint signal come,close thread*/
            pthread_mutex_unlock(&dynamic_mtx); 
            pthread_mutex_unlock(&mtx[threadIndex]);
            break;
        }
        pthread_cond_signal(&dynamic_condVar);  
        queue[threadIndex][queueSize[threadIndex]]=connfd; //add socket fd's in queue.
        queueSize[threadIndex]=queueSize[threadIndex]+1;

        if(currRunningNum==lastThreadNum){
            fprintf(fpOutput,"[ %f sec]No thread is available! Waiting for one.\n",getTime()/1000000.0);
        }

        pthread_cond_signal(&condFull[threadIndex]); 
    
        pthread_mutex_unlock(&dynamic_mtx); 
        pthread_mutex_unlock(&mtx[threadIndex]);

        threadIndex++;
        if(threadIndex==startThreadNum){
            threadIndex=0;
        } 
     
    } 
    /*Send signal to waiting  threads for closing after sigint signal.*/
    for(i=0;i<startThreadNum;++i){
        if(pthread_cond_signal(&condFull[i])!=0){
            fprintf(fpOutput, "Pthread cond error in main");
            exit(EXIT_FAILURE);
        }
    }
    /*Send signal to waiting  threads for closing after sigint signal.*/
    for(i=0;i<startThreadNum;++i){
        if(pthread_cond_signal(&condEmpty[i])!=0){
            fprintf(fpOutput, "Pthread cond error in main");
            exit(EXIT_FAILURE);
        }
    }
    /*Send signal to waiting  threads for closing after sigint signal.*/
    if(pthread_cond_signal(&dynamic_condVar)!=0){
        fprintf(fpOutput, "Pthread cond error in main");
        exit(EXIT_FAILURE);
    }
      /*Waiting for threads by main thread.*/
    for (i = 0; i < startThreadNum; i++)
    {
        if ((s = pthread_join(thread_server[i], NULL)) != 0)
        {
            fprintf(fpOutput, "Pthread_join error in main");
            
            exit(EXIT_FAILURE);
        }
    }
    /*Waiting for threads by main thread.*/
    if ((s = pthread_join(dynamic_poll_thread, NULL)) != 0){
        {
            fprintf(fpOutput, "Pthread_join error in main");
            
            exit(EXIT_FAILURE);
        }
    }       
    /*Mutex destroy of all threads.*/ 
    for(i=0;i<lastThreadNum+1;++i){
        if(pthread_mutex_destroy(&mtx[i])!=0){
            fprintf(fpOutput, "Pthread mutex destroy error in main");
           
            exit(EXIT_FAILURE);
          
        }
    } 
    /*Condition empty variable destroy of all threads.*/ 
    for(i=0;i<lastThreadNum+1;++i){
        if(pthread_cond_destroy(&condEmpty[i])!=0){
            fprintf(fpOutput,"Error in Condition destroy\n");
            exit(EXIT_FAILURE);
        }
    }
   /*Condition full variable destroy of all threads.*/ 
    for(i=0;i<lastThreadNum+1;++i){
        if(pthread_cond_destroy(&condFull[i])!=0){
            fprintf(fpOutput,"Error in Condition destroy\n");
            exit(EXIT_FAILURE);
        }
    }
    /*Free resources.*/
    fclose(fdInput);
    freeCache();
    freeAllocatedGraph();
    freeQueue();
    free(line);
    free(line2);
    free(mtx);
    free(save);
    free(thread_server);
    free(condEmpty);
    free(condFull);
    fclose(fpOutput);
    close(sockfd);
} 