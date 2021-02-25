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

//Below define  preprocessor directives are used to calculate single values of matrix.
#define NR_END 1
#define FREE_ARG char*
#define SIGN(a,b) ((b) >= 0.0 ? fabs(a) : -fabs(a))
static double dmaxarg1,dmaxarg2;
#define DMAX(a,b) (dmaxarg1=(a),dmaxarg2=(b),(dmaxarg1) > (dmaxarg2) ?\
(dmaxarg1) : (dmaxarg2))
static int iminarg1,iminarg2;
#define IMIN(a,b) (iminarg1=(a),iminarg2=(b),(iminarg1) < (iminarg2) ?\
(iminarg1) : (iminarg2))

//Functions that I use in program.
int * convertAsciiInt(char *buffer,int bytes);
char* readForP1(char* inputFileA,int n);
int* divide(int* total,int size,char pos);
ssize_t readFile(int fd, void *buffer, size_t byteCount);
int** convert2D(int array[],int row,int column,int startPoint);
int ** multiplication(int** matrix1,int row1,int column1, int** matrix2,int column2);
int** createProductMatrix(int r,int first[][r],int  second[][r],int third[][r],int  fourth[][r]);
void sigIntHandler(int sigNo);
void sigChildHandler(int sigNo);
//Below functions are used for singular value calculation of matrix C .
void svdcmp(double **a, int m, int n, double w[], double **v);
double pythag(double a, double b);
void free_dvector(double *v, int nl, int nh);
double *dvector(int nl, int nh);
double **dmatrix(int nrl, int nrh, int ncl, int nch);
sig_atomic_t childExit=0; 
int main(int argc, char *argv[]) { 
   char *inputFileA,*inputFileB;
   int n,option; 
   //getting command line arguments.
   while((option = getopt(argc, argv, ":i:j:n:")) != -1){ 
      switch(option){
         case 'i':
            inputFileA=optarg;
            break;
         case 'j': 
            inputFileB=optarg;
            break;   
         case 'n':
            n = atoi(optarg);
            //If n is not positive integer,error message and exit.
            if(n<=0){
               perror("n must be positive integer!\n");
               exit(EXIT_FAILURE);
            }
            break;
         case '?': 
            perror("It does not fit format!Format must be ./program -i inputPathA -j inputPathB -n number\n");
            exit(EXIT_FAILURE);
            break;
      }  
   }
   //This condition to control format of command line arguments.
   if((optind!=7)){
        perror("It is not fit format!Format must be \n./program -i inputPathA -j inputPathB -n number\n");
        exit(EXIT_FAILURE);
    }
   
   int i=0,j=0,z=0,s=0;
   int size=pow(2,n)*pow(2,n);
   int matrixRow=sqrt(size);
   char *readA=readForP1(inputFileA,n); //read file A.
   char *readB=readForP1(inputFileB,n); //read file B.
   int *convertA=convertAsciiInt(readA,size); //convert reading file A  to ascii code interger equivalent.
   printf("\n"); 
   int *convertB=convertAsciiInt(readB,size); //convert reading file B  to ascii code interger equivalent.
   int **matrixA=convert2D(convertA,matrixRow,matrixRow,0); //creating matrix A.
   int **matrixB=convert2D(convertB,matrixRow,matrixRow,0); //creating matrix B.
   printf("~~~~~~~~ MATRIX A ~~~~~~~~\n"); //printing matrix A on screen.
   for(i=0;i<matrixRow;i++){
      for(j=0;j<matrixRow;j++){
         printf("%d  ",matrixA[i][j]);
      }
      printf("\n");
   }
   printf("\n~~~~~~~~ MATRIX B ~~~~~~~~\n"); //Printing matrix B on screen.
   for(i=0;i<matrixRow;i++){
      for(j=0;j<matrixRow;j++){
         printf("%d  ",matrixB[i][j]);
      }
      printf("\n");
   }
   //below operations for dividing matrices for making product operation in processes.
   int* up=divide(convertA,size,'u');
   int* down=divide(convertA,size,'d');
   int* left=divide(convertB,size,'l');
   int* right=divide(convertB,size,'r');
   int concatUpLeft[size],concatUpRight[size],concatDownLeft[size],concatDownRight[size];  
   for(i=0;i<size/2;i++){
      concatUpLeft[i]=up[i];
      concatUpRight[i]=up[i];
      concatDownLeft[i]=down[i];
      concatDownRight[i]=down[i];
   }
   for(j=0;j<size/2;j++){
      concatUpLeft[i]=left[j];
      concatUpRight[i]=right[j];
      concatDownLeft[i]=left[j];
      concatDownRight[i]=right[j];
      i++;
   } 
   //Sigchild signal handling.
   sigset_t  mask;
   struct sigaction sigchld;
   sigfillset(&mask);
   memset(&sigchld,0,sizeof(sigchld));
   sigchld.sa_handler=&sigChildHandler;
   sigchld.sa_flags = SA_RESTART;
   if(sigaction (SIGCHLD, &sigchld, NULL)==-1){
      perror("SIGACTION error in main for SIGCHLD signal\n");
      exit(EXIT_FAILURE);
   }
   
   //Creating bi-directional pipes for processess.
   setbuf(stdout,NULL);
   //printf("\nParent started\n");
   int fdP1[2],fdP2[2],fdP3[2],fdP4[2];
   int fdC1[2],fdC2[2],fdC3[2],fdC4[2];
   int dummy;
   if(pipe(fdP1)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }   
   if(pipe(fdP2)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }        
   if(pipe(fdP3)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }     
   if(pipe(fdP4)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }    
   if(pipe(fdC1)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }   
   if(pipe(fdC2)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }   
   if(pipe(fdC3)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }   
   if(pipe(fdC4)==-1){
      perror("Creating pipe error in main !\n");
      exit(EXIT_FAILURE);
   }   
   //Loop for creating 4 process.                       
   for(s=0;s<4;s++){
      pid_t pid;
      pid=fork();  //fork operation.
      if(pid==-1){
         perror("Fork error in main!");
         exit(EXIT_FAILURE);
      }  
      if(pid==0 && s==0){ //For children P2.
         double quarter=size/4;
         int row =(int) sqrt(quarter);
         int column=sqrt(size);
         int temp1[size];
         if(close(fdP1[1])==-1){ //close unused write-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC1[0])==-1){ //close unused read-end of  pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         read(fdP1[0], temp1, sizeof(temp1)); //reading read-end of pipe of parent.

         //creating 2D area for up half of matrix A.
         int **up2D = (int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            up2D[i] = (int *)malloc(column * sizeof(int)); 

         //converting up half of matrix A to 2D matrix according to first half of reading array from pipe.
         up2D=convert2D(temp1,row,column,0);

         //creating 2D area for left half of matrix B.
         int **left2D = (int **)malloc(column * sizeof(int *)); 
         for (i=0; i<column; i++) 
            left2D[i] = (int *)malloc(row * sizeof(int)); 

         //converting left half of matrix B to 2D matrix  according to remain half of reading array from pipe.
         left2D=convert2D(temp1,column,row,size/2);

         //Creating 2D area for result of one quarter of multiplication matrix.
         int** multResult1=(int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            multResult1[i] = (int *)malloc(row * sizeof(int)); 

         multResult1=multiplication(up2D,row,column,left2D,row); //Result of one quarter of multiplication matrix.
         //Assign multiplication matrix that is pointer to 2d array.
         int multResultReturn1[row][row];
         for(i=0;i<row;i++){
            for(j=0;j<row;j++){
               multResultReturn1[i][j]=multResult1[i][j];
               
            }
         }
         write(fdC1[1], multResultReturn1, sizeof(multResultReturn1)); //Writing one half of multiplication matrix in write-end of pipe of child.

         //Free to malloc areas.
         for(i = 0; i < row; i++){
            free(up2D[i]);
         }   
         free(up2D);
         for(i = 0; i < column; i++){
            free(left2D[i]);
         }   
         free(left2D);
         for(i = 0; i < row; i++){
            free(multResult1[i]);
         } 
         free(multResult1);
         if(close(fdP1[0])==-1){ //Close read-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC1[1])==-1){ //Close write-end of pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         } 
         exit(EXIT_SUCCESS); //exit success from children P2.  

      }
      if(pid==0 && s==1){ //For children P3.
         double quarter=size/4;
         int row =(int) sqrt(quarter);
         int column=sqrt(size);
         int temp2[size];
         if(close(fdP2[1])==-1){ //close unused write-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC2[0])==-1){ //close unused read-end of  pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         read(fdP2[0], temp2, sizeof(temp2)); //reading read-end of pipe of parent.,

         //creating 2D area for up half of matrix A.
         int **up2 = (int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            up2[i] = (int *)malloc(column * sizeof(int)); 

         //converting up half of matrix A to 2D matrix according to first half of reading array from pipe.
         up2=convert2D(temp2,row,column,0);

         //creating 2D area for right half of matrix B.
         int **right2D = (int **)malloc(column * sizeof(int *)); 
         for (i=0; i<column; i++) 
            right2D[i] = (int *)malloc(row * sizeof(int)); 

         //converting right half of matrix B to 2D matrix  according to remain half of reading array from pipe.
         right2D=convert2D(temp2,column,row,size/2);

         //Creating 2D area for result of one quarter of multiplication matrix.
         int** multResult2=(int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            multResult2[i] = (int *)malloc(row * sizeof(int)); 

         multResult2=multiplication(up2,row,column,right2D,row); //Result of one quarter of multiplication matrix.

         //Assign multiplication matrix that is pointer to 2d array.
         int multResultReturn2[row][row];
         for(i=0;i<row;i++){
            for(j=0;j<row;j++){
               multResultReturn2[i][j]=multResult2[i][j]; 
            }
         }
         write(fdC2[1], multResultReturn2, sizeof(multResultReturn2)); //Writing one half of multiplication matrix in write-end of pipe of child.

         //Free to malloc areas.
         for(i = 0; i < row; i++){
            free(up2[i]);
         }   
         free(up2);
         for(i = 0; i < column; i++){
            free(right2D[i]);
         }   
         free(right2D);
         for(i = 0; i < row; i++){
            free(multResult2[i]);
         } 
         free(multResult2);
         if(close(fdP2[0])==-1){ //Close read-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC2[1])==-1){ //Close write-end of pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }  
         exit(EXIT_SUCCESS);    //exit success from children P3.

      }
      if(pid==0 && s==2){ //For children P4.
         double quarter=size/4;
         int row =(int) sqrt(quarter);
         int column=sqrt(size);
         int temp3[size];
         if(close(fdP3[1])==-1){ //close unused write-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC3[0])==-1){ //close unused read-end of  pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         read(fdP3[0], temp3, sizeof(temp3)); //reading read-end of pipe of parent.

         //creating 2D area for down half of matrix A.
         int **down2D = (int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            down2D[i] = (int *)malloc(column * sizeof(int)); 

         //converting down half of matrix A to 2D matrix according to first half of reading array from pipe.
         down2D=convert2D(temp3,row,column,0);

         //creating 2D area for left half of matrix B.
         int **left2 = (int **)malloc(column * sizeof(int *)); 
         for (i=0; i<column; i++) 
            left2[i] = (int *)malloc(row * sizeof(int)); 

         //converting left half of matrix B to 2D matrix  according to remain half of reading array from pipe.
         left2=convert2D(temp3,column,row,size/2);

         //Creating 2D area for result of one quarter of multiplication matrix.
         int** multResult3=(int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            multResult3[i] = (int *)malloc(row * sizeof(int)); 

         multResult3=multiplication(down2D,row,column,left2,row);//Result of one quarter of multiplication matrix.

         //Assign multiplication matrix that is pointer to 2d array.
         int multResultReturn3[row][row];
         for(i=0;i<row;i++){
            for(j=0;j<row;j++){
                multResultReturn3[i][j]=multResult3[i][j];  
            }
         }
         write(fdC3[1], multResultReturn3, sizeof(multResultReturn3));  //Writing one half of multiplication matrix in write-end of pipe of child.

         //Free to malloc areas.
         for(i = 0; i < row; i++){
            free(down2D[i]);
         }   
         free(down2D);
         for(i = 0; i < column; i++){
            free(left2[i]);
         }   
         free(left2);
         for(i = 0; i < row; i++){
            free(multResult3[i]);
         } 
         free(multResult3);
         if(close(fdP3[0])==-1){ //Close read-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC3[1])==-1){ //Close write-end of pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         } 
         exit(EXIT_SUCCESS);    //exit success from children P4. 
      }
      if(pid==0 && s==3){ //For children P5.
         double quarter=size/4;
         int row =(int) sqrt(quarter);
         int column=sqrt(size);
         int temp4[size];
         if(close(fdP4[1])==-1){ //close unused write-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC4[0])==-1){ //close unused read-end of  pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         read(fdP4[0], temp4, sizeof(temp4)); //reading read-end of pipe of parent.
        
         //creating 2D area for down half of matrix A.
         int **down2 = (int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            down2[i] = (int *)malloc(column * sizeof(int)); 

         //converting down half of matrix A to 2D matrix according to first half of reading array from pipe.
         down2=convert2D(temp4,row,column,0);

         //creating 2D area for right half of matrix B.
         int **right2 = (int **)malloc(column * sizeof(int *)); 
         for (i=0; i<column; i++) 
            right2[i] = (int *)malloc(row * sizeof(int)); 

         //converting right half of matrix B to 2D matrix  according to remain half of reading array from pipe.
         right2=convert2D(temp4,column,row,size/2);

         //Creating 2D area for result of one quarter of multiplication matrix.
         int** multResult4=(int **)malloc(row * sizeof(int *)); 
         for (i=0; i<row; i++) 
            multResult4[i] = (int *)malloc(row * sizeof(int)); 

         multResult4=multiplication(down2,row,column,right2,row); //Result of one quarter of multiplication matrix.

         //Assign multiplication matrix that is pointer to 2d array.
         int multResultReturn4[row][row];
         for(i=0;i<row;i++){
            for(j=0;j<row;j++){
               multResultReturn4[i][j]=multResult4[i][j];
            }
         }
         write(fdC4[1], multResultReturn4, sizeof(multResultReturn4));  //Writing one half of multiplication matrix in write-end of pipe of child.

         //Free to malloc areas.
         for(i = 0; i < row; i++){
            free(down2[i]);
         }   
         free(down2);
         for(i = 0; i < column; i++){
            free(right2[i]);
         }   
         free(right2);
         for(i = 0; i < row; i++){
            free(multResult4[i]);
         } 
         free(multResult4);
         if(close(fdP4[0])==-1){ //Close read-end of pipe of parent.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         }
         if(close(fdC4[1])==-1){ //Close write-end of pipe of child.
            perror("Close pipe error in main!");
            exit(EXIT_FAILURE);
         } 
         exit(EXIT_SUCCESS);   //exit success from children P5.   

      }
   }
   
   //SigInt(ctrl-c) signal handler.
   sigset_t  block_mask;
   struct sigaction actionTerm;
   sigfillset (&block_mask);
   actionTerm.sa_handler = sigIntHandler;
   actionTerm.sa_flags = 0;
   if(sigaction (SIGINT, &actionTerm, NULL)==-1){
      perror("SIGACTION error in main for SIGINT\n");
      exit(EXIT_FAILURE);
   }   

   //Parent comes and closes write-end of pipes of child and read-end of pipes of parent.
   if(close(fdP1[0])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdP2[0])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
    if(close(fdP3[0])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdP4[0])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   }   
    if(close(fdC1[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdC2[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   }   
    if(close(fdC3[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdC4[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   //parent writes requirement parts of matrix A and matrix B for child processes.
   write(fdP1[1],concatUpLeft, sizeof(concatUpLeft)+1);
   write(fdP2[1],concatUpRight, sizeof(concatUpRight)+1);
   write(fdP3[1],concatDownLeft, sizeof(concatDownLeft)+1);
   write(fdP4[1],concatDownRight, sizeof(concatDownRight)+1);
   
   //parent closes write-end of pipes of parent.
   if(close(fdP1[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdP2[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdP3[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 
   if(close(fdP4[1])==-1) {
      perror("Close pipe error in main!");
      exit(EXIT_FAILURE);
   } 

   //Blocking parent process  until all of its children have completed their calculations.
   while(childExit<4){
      sigdelset(&mask, SIGCHLD);
      sigsuspend(&mask);
   }         
   double resQuarter=size/4;
   int resRow =(int) sqrt(resQuarter);
   int firstRes[resRow][resRow],secondRes[resRow][resRow],thirdRes[resRow][resRow],fourthRes[resRow][resRow];

   //Parent reads every quarter of multiplication calculations of children from read-end of pipe of child.
   if(read(fdC1[0],firstRes,sizeof(firstRes))==-1){
      perror("PARENT DID NOT GET EOF\n");
      exit(EXIT_FAILURE);
   }  
   if(read(fdC2[0],secondRes,sizeof(secondRes))==-1){
      perror("PARENT DID NOT GET EOF\n");
      exit(EXIT_FAILURE);
   }  
   if(read(fdC3[0],thirdRes,sizeof(thirdRes))==-1){
      perror("PARENT DID NOT GET EOF\n");
      exit(EXIT_FAILURE);
   }  
   if(read(fdC4[0],fourthRes,sizeof(fourthRes))==-1){
      perror("PARENT DID NOT GET EOF\n");
      exit(EXIT_FAILURE);
   }
 
   //Create 2D area for combine 4 quarter of multiplication matrix.
   int **product= (int **)malloc((2*resRow) * sizeof(int *)); 
   for (i=0; i<2*resRow; i++) 
         product[i] = (int *)malloc((2*resRow) * sizeof(int)); 

    product=createProductMatrix(resRow,firstRes,secondRes,thirdRes,fourthRes); //making multiplication matrix by combining 4 quarter of calculations of children.

    //Printing multiplication matrix C on screen.
    printf("\n~~~~~~~~ PRODUCT MATRIX C ~~~~~~~~\n");
    int returnedMatrix[resRow*2][resRow*2]; 
    for(i=0;i<resRow*2;i++){
       for(j=0;j<resRow*2;j++){
           returnedMatrix[i][j]=product[i][j];
           printf("%d  ",returnedMatrix[i][j]);
       }
       printf("\n");
    }
   int newSize=(2*resRow)+1;
   int k;

   //Below operations for calculating single values of multiplication matrix.
   double **matrixC = (double **)malloc(newSize * sizeof(double *)); 
   for (i=0; i<newSize; i++) 
      matrixC[i] = (double *)malloc(newSize* sizeof(double));
	
	double singValArr[newSize];
	double **output= (double **)malloc(newSize * sizeof(double *)); 
    for (i=0; i<newSize; i++) 
         output[i] = (double *)malloc(newSize* sizeof(double));
	
	k=0;
	for(i=1;i<newSize;++i){
		for(j=1;j<newSize;++j){
			matrixC[i][j]=returnedMatrix[i-1][j-1];
			++k;
		}
	}
	svdcmp(matrixC,newSize-1,newSize-1,singValArr,output);
	
   //prints single value calculation on screen.
   printf("\n\n~~~~~~~~ All the Singular Values of Product Matrix C ~~~~~~~~\n");
	for(i=1;i<newSize;++i){
		printf("%.3f ",singValArr[i]);
	} 
   printf("\n");
   printf("\n");
  
   //free malloc areas.
   for(i=0;i<newSize;i++){
      free(matrixC[i]);
      free(output[i]);
   }
   free(matrixC);
   free(output);
   for(i=0;i<resRow;i++){
      free(product[i]);
   }
   free(product);
   free(readA);
   free(readB);
   free(convertA);
   free(convertB);
   free(up);
   free(down);
   free(left);
   free(right);
   
   return 0;
}
//This is reading of files operations for parent process P1.
char* readForP1(char* inputFile,int n){
   int bytes;
   int size=pow(2,n)*pow(2,n);
   char* matrix;
   char *buf=malloc(size);
   int fdInput;
  
   /*Open input file.*/
   if((fdInput=open(inputFile,O_RDONLY))==-1){
      perror("Failed to open input file in readForP1 function.\n");
      exit(EXIT_FAILURE);
   }
   bytes=readFile(fdInput,buf,size); //Reading operation by calling function.
   //Closing input file.
   if(close(fdInput)<0){
      perror("Close  file error in readForP1 function.\n");
      exit(EXIT_FAILURE);
   }
   return buf;

}
//This function for converting each character that is read to  ASCII code integer equivalent.
int * convertAsciiInt(char *buffer,int bytes){
   int* arr;
   arr=(int*)malloc(bytes*sizeof(int));
   int newline= sqrt(bytes);
   int i=0;
   int count=0;
   for(i=0;i < bytes;i++){
      arr[i]=(int)buffer[i];
   }
   return arr;
}
//Reading given file with lock it.
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
   if((bytes=read(fd,buffer,byteCount))<byteCount){
         perror("There is no (2^n) x (2^n) character in input file,Failed to read file operation!\n");
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
//This function for converting 1D array to 2D array.
int** convert2D(int array[],int row,int column,int startPoint){
   int i,j,z=startPoint;
   int **temp = (int **)malloc(row * sizeof(int *)); 
   for (i=0; i<row; i++) 
         temp[i] = (int *)malloc(column * sizeof(int)); 
   z=startPoint;
   for(i=0;i<row;i++){
      for(j=0;j<column;j++){
         temp[i][j]=array[z];
         z++;
      }
   }
   return temp;
}

//This function for dividing a matrix into 4 part.
int* divide(int* total,int size,char pos){
   int i=0,j=0,count=1;
   int* newArr;
   newArr=(int*)malloc(size*sizeof(int));
   int newSize=size/2;
   double quarter=size/4;
   int control =(int) sqrt(quarter);
   //Up half of matrix.
   if(pos=='u'){
      for(i=0;i<newSize;i++){
         newArr[i]=total[i];

      } 
   }
   //Down half of matrix.
   if(pos=='d'){
      j=0;
      for(i=newSize;i<size;i++){
         newArr[j]=total[i];
         j++;
      } 
   }
   //Left half of matrix.
   if(pos=='l'){
      i=0,j=0;
      while(j<newSize){
         if(count==control){
            newArr[j]=total[i];
            i=i+control+1;
            j++;
            count=1;
            count=0;
         }
         else{
            newArr[j]=total[i];
            i++;
            j++;

         }
         count ++;
        
      }
   }
   //Right half of matrix.
   if(pos=='r'){
      i=control,j=0;
      while(j<newSize){
         if(count==control){
            newArr[j]=total[i];
            i=i+control+1;
            j++;
            count=1;
            count=0;
         }
         else{
            newArr[j]=total[i];
            i++;
            j++;

         }
         count ++;
        
      }
   }
   return newArr;
}
//Multiplication of two matrix operation.
int ** multiplication(int** matrix1,int row1,int column1,int** matrix2,int column2) 
{ 
   int i, j, k; 
   int **result = (int **)malloc(row1 * sizeof(int *)); 
   for (i=0; i<row1; i++) 
         result[i] = (int *)malloc(column1 * sizeof(int));   
   for (i = 0; i < row1; i++) 
   { 
      for (j = 0; j < column2; j++) 
      { 
         result[i][j] = 0; 
         for (k = 0; k < column1; k++){ 
            result[i][j] =result[i][j] + (matrix1[i][k]*matrix2[k][j]); 
         }   
      } 
   } 
    return result;
} 
//This function for combine 4 quarter of matrix and creating a result matrix.   
int** createProductMatrix(int r,int first[][r],int  second[][r],int third[][r],int  fourth[][r]){
   int i,j,k;
   int resultMatrix[2*r][2*r];
   int **productMatrix= (int **)malloc((2*r) * sizeof(int *)); 
   for (i=0; i<2*r; i++) 
         productMatrix[i] = (int *)malloc((2*r) * sizeof(int)); 
   //Up-left quarter of matrix.        
   for(i=0;i<r;i++){
      for(j=0;j<r;j++){
         resultMatrix[i][j]=first[i][j];    
      }
   }
   //Up-right quarter of matrix.
   for(i=0;i<r;i++){
      for(j=0;j<r;j++){
         resultMatrix[i][j+r]=second[i][j];
         
      }
   }
   //Down-left quarter of matrix.
    for(i=0;i<r;i++){
      for(j=0;j<r;j++){
         resultMatrix[i+r][j]=third[i][j];
         
      }
   }
   //Down-right quarter of matrix.
   for(i=0;i<r;i++){
      for(j=0;j<r;j++){
         resultMatrix[i+r][j+r]=fourth[i][j];  
      }
   }
   //Assign product matrix that is 2D array to 2D pointer.
   for(i=0;i<2*r;i++){
      for(j=0;j<2*r;j++){
         productMatrix[i][j]=resultMatrix[i][j];    
      }
   }
   return productMatrix;
}  
//SIGCHLD signal handler
void sigChildHandler(int sigNo){
   if(sigNo==SIGCHLD){
      printf("\n~~~~~IN SIGCHLD HANDLER~~~~~\n");
      int status;
      int waitPid=wait(&status); //synchronous wait for each of  children.
      printf("Process ID of terminated child is:%d\n",waitPid);
      childExit++;
   }   
}
//SIGINT signal handler
void sigIntHandler(int sigNo){
   if(sigNo==SIGINT){
      printf("\n~~~~~IN SIGINT HANDLER~~~~~\n");
      printf("CTRL-C entered,so all 5 process is closing and exiting...\n");
      exit(EXIT_SUCCESS);
   }  
}

//All the functions that are  below for calculating single value of matrix.I use this functions from that resource:
/*Singular value decomposition program, svdcmp, from "Numerical Recipes in C"
(Cambridge Univ. Press) by W.H. Press, S.A. Teukolsky, W.T. Vetterling,
and B.P. Flannery*/

double **dmatrix(int nrl, int nrh, int ncl, int nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	int i,nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;
	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	m += NR_END;
	m -= nrl;
	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	m[nrl] += NR_END;
	m[nrl] -= ncl;
	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}

double *dvector(int nl, int nh)
/* allocate a double vector with subscript range v[nl..nh] */
{
	double *v;
	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	return v-nl+NR_END;
}

void free_dvector(double *v, int nl, int nh)
/* free a double vector allocated with dvector() */
{
	free((FREE_ARG) (v+nl-NR_END));
}

double pythag(double a, double b)
/* compute (a2 + b2)^1/2 without destructive underflow or overflow */
{
	double absa,absb;
	absa=fabs(a);
	absb=fabs(b);
	if (absa > absb) return absa*sqrt(1.0+(absb/absa)*(absb/absa));
	else return (absb == 0.0 ? 0.0 : absb*sqrt(1.0+(absa/absb)*(absa/absb)));
}

/******************************************************************************/
void svdcmp(double **a, int m, int n, double w[], double **v)
/*******************************************************************************
Given a matrix a[1..m][1..n], this routine computes its singular value
decomposition, A = U.W.VT.  The matrix U replaces a on output.  The diagonal
matrix of singular values W is output as a vector w[1..n].  The matrix V (not
the transpose VT) is output as v[1..n][1..n].
*******************************************************************************/
{
	int flag,i,its,j,jj,k,l,nm;
	double anorm,c,f,g,h,s,scale,x,y,z,*rv1;

	rv1=dvector(1,n);
	g=scale=anorm=0.0; /* Householder reduction to bidiagonal form */
	for (i=1;i<=n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;
		if (i <= m) {
			for (k=i;k<=m;k++) scale += fabs(a[k][i]);
			if (scale) {
				for (k=i;k<=m;k++) {
					a[k][i] /= scale;
					s += a[k][i]*a[k][i];
				}
				f=a[i][i];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][i]=f-g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=i;k<=m;k++) s += a[k][i]*a[k][j];
					f=s/h;
					for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
				}
				for (k=i;k<=m;k++) a[k][i] *= scale;
			}
		}
		w[i]=scale *g;
		g=s=scale=0.0;
		if (i <= m && i != n) {
			for (k=l;k<=n;k++) scale += fabs(a[i][k]);
			if (scale) {
				for (k=l;k<=n;k++) {
					a[i][k] /= scale;
					s += a[i][k]*a[i][k];
				}
				f=a[i][l];
				g = -SIGN(sqrt(s),f);
				h=f*g-s;
				a[i][l]=f-g;
				for (k=l;k<=n;k++) rv1[k]=a[i][k]/h;
				for (j=l;j<=m;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[j][k]*a[i][k];
					for (k=l;k<=n;k++) a[j][k] += s*rv1[k];
				}
				for (k=l;k<=n;k++) a[i][k] *= scale;
			}
		}
		anorm = DMAX(anorm,(fabs(w[i])+fabs(rv1[i])));
	}
	for (i=n;i>=1;i--) { /* Accumulation of right-hand transformations. */
		if (i < n) {
			if (g) {
				for (j=l;j<=n;j++) /* Double division to avoid possible underflow. */
					v[j][i]=(a[i][j]/a[i][l])/g;
				for (j=l;j<=n;j++) {
					for (s=0.0,k=l;k<=n;k++) s += a[i][k]*v[k][j];
					for (k=l;k<=n;k++) v[k][j] += s*v[k][i];
				}
			}
			for (j=l;j<=n;j++) v[i][j]=v[j][i]=0.0;
		}
		v[i][i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=IMIN(m,n);i>=1;i--) { /* Accumulation of left-hand transformations. */
		l=i+1;
		g=w[i];
		for (j=l;j<=n;j++) a[i][j]=0.0;
		if (g) {
			g=1.0/g;
			for (j=l;j<=n;j++) {
				for (s=0.0,k=l;k<=m;k++) s += a[k][i]*a[k][j];
				f=(s/a[i][i])*g;
				for (k=i;k<=m;k++) a[k][j] += f*a[k][i];
			}
			for (j=i;j<=m;j++) a[j][i] *= g;
		} else for (j=i;j<=m;j++) a[j][i]=0.0;
		++a[i][i];
	}
	for (k=n;k>=1;k--) { /* Diagonalization of the bidiagonal form. */
		for (its=1;its<=30;its++) {
			flag=1;
			for (l=k;l>=1;l--) { /* Test for splitting. */
				nm=l-1; /* Note that rv1[1] is always zero. */
				if ((double)(fabs(rv1[l])+anorm) == anorm) {
					flag=0;
					break;
				}
				if ((double)(fabs(w[nm])+anorm) == anorm) break;
			}
			if (flag) {
				c=0.0; /* Cancellation of rv1[l], if l > 1. */
				s=1.0;
				for (i=l;i<=k;i++) {
					f=s*rv1[i];
					rv1[i]=c*rv1[i];
					if ((double)(fabs(f)+anorm) == anorm) break;
					g=w[i];
					h=pythag(f,g);
					w[i]=h;
					h=1.0/h;
					c=g*h;
					s = -f*h;
					for (j=1;j<=m;j++) {
						y=a[j][nm];
						z=a[j][i];
						a[j][nm]=y*c+z*s;
						a[j][i]=z*c-y*s;
					}
				}
			}
			z=w[k];
			if (l == k) { /* Convergence. */
				if (z < 0.0) { /* Singular value is made nonnegative. */
					w[k] = -z;
					for (j=1;j<=n;j++) v[j][k] = -v[j][k];
				}
				break;
			}
			if (its == 30) printf("no convergence in 30 svdcmp iterations");
			x=w[l]; /* Shift from bottom 2-by-2 minor. */
			nm=k-1;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=pythag(f,1.0);
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0; /* Next QR transformation: */
			for (j=l;j<=nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=pythag(f,h);
				rv1[j]=z;
				c=f/z;
				s=h/z;
				f=x*c+g*s;
				g = g*c-x*s;
				h=y*s;
				y *= c;
				for (jj=1;jj<=n;jj++) {
					x=v[jj][j];
					z=v[jj][i];
					v[jj][j]=x*c+z*s;
					v[jj][i]=z*c-x*s;
				}
				z=pythag(f,h);
				w[j]=z; /* Rotation can be arbitrary if z = 0. */
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}
				f=c*g+s*y;
				x=c*y-s*g;
				for (jj=1;jj<=m;jj++) {
					y=a[jj][j];
					z=a[jj][i];
					a[jj][j]=y*c+z*s;
					a[jj][i]=z*c-y*s;
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
	}
	free_dvector(rv1,1,n);
}

