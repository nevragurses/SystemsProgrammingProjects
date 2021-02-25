#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#define LENG 2 /*Length of global ingredients array.*/
static sem_t sem;
static sem_t sem_wholesaler;
static char *ingredients;      /*Global ingredients array.*/
static int finish;             /*To control whether finished supplying or not.*/
static void *chefs(void *arg); /*Chefs function that is using by 6 thread.*/
int main(int argc, char *argv[])
{
    char *inputFile;
    int option;
    char string[300];
    int writeRes;
    /*Getting command line arguments.*/
    while ((option = getopt(argc, argv, ":i:")) != -1)
    {
        switch (option)
        {
        case 'i':
            inputFile = optarg;
            break;
        case '?':
            perror("It does not fit format!Format must be :./program -i filePath\n");
            exit(EXIT_FAILURE);
            break;
        }
    }
    /*This condition to control format of command line arguments.*/
    if ((optind != 3))
    {
        perror("It does not fit format!Format must be :./program -i filePath\n");
        exit(EXIT_FAILURE);
    }
    ingredients =(char*) malloc((LENG + 1) * sizeof(char)); /*allocating heap area for ingredients array.*/
    ingredients[0]='\0';
    ingredients[1]='\0';

    /*Initializing an unnamed semaphore for threads.*/
    if (sem_init(&sem, 0, 1) == -1)
    {
        perror("Failed to sem_init operation for semaphore named sem in main!\n");
        exit(EXIT_FAILURE);
    }
    /*Initializing another unnamed semaphore for threads. */
    if (sem_init(&sem_wholesaler, 0, 0) == -1)
    {
        perror("Failed to sem_init operation for sem_wholesaler semaphore in main!\n");
        exit(EXIT_FAILURE);
    }
    int fdInput;
    /*Opening input file.*/
    if ((fdInput = open(inputFile, O_RDONLY)) == -1)
    {
        perror("Failed to open input file in main.\n");
        exit(EXIT_FAILURE);
    }
    int bytes;
    char keep[1];
    /*Controls the file whether it has a letter except M or W or F or W or not.If it has,print error message.*/
    do
    {
        if ((bytes = read(fdInput, keep, 1)) == -1)
        {
            perror("Failed to read input file in main.\n");
            exit(EXIT_FAILURE);
        }
        if (keep[0] != 'W' && keep[0] != 'S' && keep[0] != 'F' && keep[0] != 'M' && keep[0] != '\n')
        {
            perror("Error! Letters in line in input file must be M or W or S or F\n");
            exit(EXIT_FAILURE);
        }
    } while (bytes > 0);
    lseek(fdInput, 0, SEEK_SET);
    char keep2[2];
    int newlineCount=0;
    bytes = 0;
    /*Controls the file whether it has a line that includes 2 same ingredients or not.If it has,print error message.*/
    do
    {
        if ((bytes = read(fdInput, keep2, 3)) == -1)
        {
            perror("Failed to read input file in main.\n");
            exit(EXIT_FAILURE);
        }
        if (keep2[0] == keep2[1])
        {
            perror("Error! In a line of input file,there must be 2 distinct letter.\n");
            exit(EXIT_FAILURE);
        }
        newlineCount++;
    } while (bytes > 0);
    /*If size of file less than 10,print error.*/
    if(newlineCount<11){
        perror("Error! In input file,there must be at least 10 row.\n");
        exit(EXIT_FAILURE);
    }
    lseek(fdInput, 0, SEEK_SET);
    finish = 0;
    pthread_t thread_chefs[6];
    int s, i;
    /*Creating 6 threads for chefs.*/
    int save[6];
    for (i = 0; i < 6; i++)
    {
        save[i] = i;
        if ((s = pthread_create(&thread_chefs[i], NULL, chefs,(void*) &save[i])) != 0)
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
    int tmpBytes;
    char tmp[3];
    char str[3];
    /*Main thread that is wholesaler's thread.*/
    do
    {
        if ((tmpBytes = read(fdInput, tmp, 3)) == -1)  /*Read a line from file that includes 2 letters in each line.*/
        {
            sprintf(string, "Failed to read input file in main.\n");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        if (tmpBytes == 0) /*End of file*/
        {               
            finish = 1; /*For notify  the chefs that no more ingredients.*/
            break;
        }
        /*Protect wholesaler by using semaphore.*/
        if (sem_wait(&sem) == -1)
        {
            sprintf(string, "Failed to sem_wait operation in main.\n");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        char stringPrint[50];
        str[0] = tmp[0];
        str[1] = tmp[1];
        /*Selecting message that will print according to letters.*/
        if (str[0] == 'W' && str[1] == 'S')
        {
            sprintf(stringPrint, "walnuts and sugar");
        }
        if (str[0] == 'S' && str[1] == 'W')
        {
            sprintf(stringPrint, "sugar and walnuts");
        }
        if (str[0] == 'F' && str[1] == 'W')
        {
            sprintf(stringPrint, "flour and walnuts");
        }
        if (str[0] == 'W' && str[1] == 'F')
        {
            sprintf(stringPrint, "walnuts and flour");
        }
        if (str[0] == 'S' && str[1] == 'F')
        {
            sprintf(stringPrint, "sugar and flour");
        }
        if (str[0] == 'F' && str[1] == 'S')
        {
            sprintf(stringPrint, "flour and sugar");
        }
        if (str[0] == 'W' && str[1] == 'M')
        {
            sprintf(stringPrint, "walnuts and milk");
        }
        if (str[0] == 'M' && str[1] == 'W')
        {
            sprintf(stringPrint, "milk and walnuts");
        }
        if (str[0] == 'M' && str[1] == 'F')
        {
            sprintf(stringPrint, "milk and flour");
        }
        if (str[0] == 'F' && str[1] == 'M')
        {
            sprintf(stringPrint, "flour and milk");
        }
        if (str[0] == 'M' && str[1] == 'S')
        {
            sprintf(stringPrint, "milk and sugar");
        }
        if (str[0] == 'S' && str[1] == 'M')
        {
            sprintf(stringPrint, "sugar and milk");
        }
        /*Print message on screen*/
        printf("the wholesaler delivers %s\n", stringPrint);

        /*Put ingredients in global array.*/
        ingredients[0] = str[0];
        ingredients[1] = str[1];

        printf("the wholesaler is waiting for the dessert\n");
        /*Post operation for semaphore.*/
        if (sem_post(&sem) == -1)
        {
            sprintf(string, "Failed to sem_post operation in main.\n");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        /*This semaphore is using to wait for wholesaler while chef is preparing dessert.*/
        if (sem_wait(&sem_wholesaler) == -1)
        {
            sprintf(string, "Failed to sem_wait operation in main.\n");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        printf("the wholesaler has obtained the dessert and left to sell it\n");

    } while (tmpBytes > 0);

    /*Waiting for chefs by main thread.*/
    for (i = 0; i < 6; i++)
    {
        if ((s = pthread_join(thread_chefs[i], NULL)) != 0)
        {
            sprintf(string, "Pthread_join error in main");
            writeRes = write(STDOUT_FILENO, string, strlen(string));
            if (writeRes == -1)
            {
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
    }

    free(ingredients); /*Free memory area allocated by malloc.*/
    /*Destroy the unnamed semaphores.*/
    if (sem_destroy(&sem) == -1)
    {
        perror("sem_destroy error in main!");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&sem_wholesaler) == -1)
    {
        perror("sem_destroy error in main!");
        exit(EXIT_FAILURE);
    }

    return 0;
}
/*Chef function that is using by all 6 threads.*/
static void *chefs(void *arg)
{
    int chefNum = *((int *)arg);
    chefNum = chefNum + 1;
    char string[300];
    int writeRes = 0;
    srand(time(NULL));
    /*Before taking ingredients,wait messages of all chefs printing on screen.*/
    if (chefNum == 1)
        printf("chef %d is waiting for walnuts and sugar\n", chefNum);
    if (chefNum == 2)
        printf("chef %d is waiting for flour and walnuts\n", chefNum);
    if (chefNum == 3)
        printf("chef %d is waiting for sugar and flour\n", chefNum);
    if (chefNum == 4)
        printf("chef %d is waiting for walnuts and milk\n", chefNum);
    if (chefNum == 5)
        printf("chef %d is waiting for milk and flour\n", chefNum);
    if (chefNum == 6)
        printf("chef %d is waiting for milk and sugar\n", chefNum);
    /*Chefs are prepating dessert by  taking lack ingredients from global array provided by wholesaler.*/
    while (finish != 1)
    {
        /*Chef 1 takes walnuts and sugar.*/
        if ((chefNum == 1 && ingredients[0] == 'W' && ingredients[1] == 'S') || (chefNum == 1 && ingredients[0] == 'S' && ingredients[1] == 'W'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 1 has taken the walnuts\n");
            printf("chef 1  has taken the sugar\n");
            printf("chef 1  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 1 has delivered the dessert to the wholesaler\n");
            printf("chef 1  is waiting for sugar and walnuts\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
        /*Chef 2 takes flour and walnuts.*/
        else if ((chefNum == 2 && ingredients[0] == 'F' && ingredients[1] == 'W') || (chefNum == 2 && ingredients[0] == 'W' && ingredients[1] == 'F'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 2 has taken the flour\n");
            printf("chef 2  has taken the walnuts\n");
            printf("chef 2  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 2 has delivered the dessert to the wholesaler\n");
            printf("chef 2  is waiting for flour and walnuts\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
        /*Chef 3 takes sugar and flour.*/
        else if ((chefNum == 3 && ingredients[0] == 'S' && ingredients[1] == 'F') || (chefNum == 3 && ingredients[0] == 'F' && ingredients[1] == 'S'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 3 has taken the sugar\n");
            printf("chef 3  has taken the flour\n");
            printf("chef 3  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 3 has delivered the dessert to the wholesaler\n");
            printf("chef 3  is waiting for sugar and flour\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
        /*Chef 4 takes milk and wanluts*/
        else if ((chefNum == 4 && ingredients[0] == 'M' && ingredients[1] == 'W') || (chefNum == 4 && ingredients[0] == 'W' && ingredients[1] == 'M'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 4 has taken the walnuts \n");
            printf("chef 4  has taken the milk\n");
            printf("chef 4  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 4 has delivered the dessert to the wholesaler\n");
            printf("chef 4  is waiting for milk and walnuts\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
        /*Chef 5 takes milk and flour*/
        else if ((chefNum == 5 && ingredients[0] == 'M' && ingredients[1] == 'F') || (chefNum == 5 && ingredients[0] == 'F' && ingredients[1] == 'M'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 5 has taken the milk\n");
            printf("chef 5  has taken the flour\n");
            printf("chef 5  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 5 has delivered the dessert to the wholesaler\n");
            printf("chef 5  is waiting for milk and flour\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
        /*Chef 6 takes milk and sugar.s*/
        else if ((chefNum == 6 && ingredients[0] == 'M' && ingredients[1] == 'S') || (chefNum == 6 && ingredients[0] == 'S' && ingredients[1] == 'M'))
        {
            if (sem_wait(&sem) == -1)
            {
                sprintf(string, "Failed to sem_wait operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            printf("Chef 6 has taken the milk\n");
            printf("chef 6  has taken the sugar\n");
            printf("chef 6  is preparing the dessert\n");
            ingredients[0] = '\0';
            ingredients[1] = '\0';
            int random = (rand() % 5) + 1;
            sleep(random);
            printf("chef 6 has delivered the dessert to the wholesaler\n");
            printf("chef 6  is waiting for milk and sugar\n");
            if (sem_post(&sem) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_wholesaler) == -1)
            {
                sprintf(string, "Failed to sem_post operation in chefs function.\n");
                writeRes = write(STDOUT_FILENO, string, strlen(string));
                if (writeRes == -1)
                {
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}