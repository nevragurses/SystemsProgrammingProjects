#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#define SA struct sockaddr 
/*Get time as microsecond.While using after,it will be converted second.*/
unsigned long getTime(){
    struct timeval timeValue;
    gettimeofday(&timeValue,NULL);
    unsigned long microSecond=1000000 * timeValue.tv_sec + timeValue.tv_usec;
   
    return microSecond;
}
/*For send and get information via socket with server.*/
void func(int sockfd,int src,int dest,time_t startTime) 
{   
	int buff[2];  
    char buff2[900000]={0}; 
	
    bzero(buff, sizeof(buff)); 
    buff[0]=src; //source node.
    buff[1]=dest ;  //destination node.
   
    write(sockfd, buff, sizeof(buff)); //send nodes for server via socket.

    read(sockfd, buff2,sizeof(buff2)); //get answer via socket.
    
    printf("[ %f sec]Server's response  (%d): %s, arrived in %fsecond, shutting down\n",(getTime()/1000000.0),(int)getpid(), buff2,(getTime()-startTime)/(double)1000000); 
	
} 

int main(int argc, char *argv[]) { 
    int sockfd;
	struct sockaddr_in servaddr;
    char *addr;
 
    int  option,port,src,dest;
     /*getting command line arguments.*/
    while((option = getopt(argc, argv, ":a:p:s:d:")) != -1){ 
      switch(option){
        case 'a':
            addr=optarg;
            break;
        case 'p': 
            port= atoi(optarg);
            break; 
        case 's':
            src = atoi(optarg);
            break; 
        case 'd':
            dest = atoi(optarg);
            break;       
        case '?': 
            perror("It does not fit format!Format example is ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
            exit(EXIT_FAILURE);
        break;
      }
    }
    if((optind!=9)){
        errno=EINVAL;
        perror("CommandLine argument numbers must be 9! Format example is ./client -a 127.0.0.1 -p PORT -s 768 -d 979\n");
        exit(EXIT_FAILURE);
    } 
    time_t startTime;
    //socket create and varification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n");
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr(addr); 
	servaddr.sin_port = htons(port); 
    
    int flag = 1;  
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag))) {  
        perror("setsockopt fail");  
    }  

    printf("[ %f sec]Client (%d) connecting to %s:45647:%d\n",(getTime()/1000000.0),(int)getpid(), addr,(int)getpid());
	// connect the client socket to server socket 
	if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) { 
		printf("connection with the server failed...\n"); 
		exit(0); 
	} 
	else{
		printf("[ %f sec]Client (%d) connected and requesting a path from node %d to %d\n",(getTime()/1000000.0),(int)getpid(),src,dest);  
        startTime=getTime();
    }    

	//function for communication with server.
	func(sockfd,src,dest,startTime); 

	//close the socket 
	close(sockfd); 
} 
