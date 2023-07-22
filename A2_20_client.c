/*****************************************************
 *      CS F372 Operating Systems
 *          ASSIGNMENT 2 (CLIENTv2)
 * 
 *  2020A7PS1712H Yash Vardhan Singh     
 *  2020A4PS2009H Nidhi Agarwal
 *  2020AAPS1779H Reuben George
 * 
 * *****************************************************/


#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define PRIMES 1000

#define debugm(msg) printf("\n----------------------------\n%s",msg);
#define debugn(num) printf("\n----------------------------\n%s: %d\n----------------------------\n",#num,num);
#define debugh(hex) printf("\n----------------------------\n%s: %X\n----------------------------\n",#hex,hex);
#define debugp(ptr) printf("\n----------------------------\n%s: %ld\n----------------------------\n",#ptr,ptr);
#define debugwithsval(msg,val) printf("\n----------------------------\n%s%s\n----------------------------\n", msg, val);
#define debugwithnval(msg,val) printf("\n----------------------------\n%s%d\n----------------------------\n", msg, val);

#define PRINT_INFO() { \
	printf ( "\n----------------------------\nINFO %d:%d %ld %s %s %d \n", \
	 getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
	__LINE__); \
}

#define PRINT_ERROR(MSG, ...) { \
	printf ( "%s ERROR %d:%d %ld %s %s %d : [%d] " MSG ";;\n", \
	"TODO_PRINT_TIME", getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
	__LINE__,  errno, ##__VA_ARGS__);	\
	}
	
#define PRINT_ERR_EXIT(MSG, ...) { \
	printf ( "%s ERROR %d:%d %ld %s %s %d : [%d] " MSG ";;\n", \
	"TODO_PRINT_TIME", getpid(), getppid(), pthread_self(), __FILE__, __FUNCTION__, \
	__LINE__,  errno, ##__VA_ARGS__);	\
	_exit(-1); \
	}

char *paths[33] = {"/bin/cat","/bin/chgrp","/bin/chmod","/bin/chown","/bin/cp","/bin/date","/bin/dd","/bin/df","/bin/dmesg","/bin/echo","/bin/false","/bin/hostname","/bin/kill","/bin/ln","/bin/login","/bin/ls","/bin/mkdir","/bin/mknod","/bin/more","/bin/mount","/bin/mv","/bin/ps","/bin/pwd","/bin/rm","/bin/rmdir","/bin/sed","/bin/sh","/bin/stty","/bin/su","/bin/sync","/bin/true","/bin/unmount","/bin/uname"};
//int keys[10] = {101,201,301,401,501,601,701,801,901,1001};
//int key_cnt = 0;


struct connect_channel{
    int k_cnt;
    int comm_ch_est;
    int conn_sync;
    int semaphore;
    char client_name[256];
};


struct arithmetic{
    int a;
    int b;
    char op;
};

struct evenodd{   
    int num;
};

struct prime{   
    int num;
};

struct negative{   
    int num;
};

union client_request_payload{
    struct arithmetic arm;
    struct evenodd eoo;
    struct prime prm;
    struct negative neg;
};
struct client_request{
    int request_type;
    union client_request_payload crp;
};

struct server_response{
    int response_code;
    int client_resp_seq_no;
    int server_resp_seq_no;
    int result;
};

struct comm_channel{
    int num_req;
    int inputting;
    int outputting;
    struct client_request clrq;
    struct server_response svrp;
};

void displayMenu(){
    printf("------------------------------------\n");
    printf("1.Send Request \n");
    printf("\t a)Arithmetic \n");
    printf("\t b)EvenOrOdd \n");
    printf("\t c)isPrime \n");
    printf("\t d)isNegative \n");
    printf("2.Unregister\n");
    printf("3.Exit\n");
    printf("------------------------------------\n");
}

int main(int argc, char*argv[]){

    if(argc!=2){
        PRINT_INFO()
        printf("INVALID ARGS!!!\n");
        return(1);
    }
    key_t conn_key = ftok("server",'Y');
    //debugh(conn_key)
    int conn_id = shmget(conn_key,sizeof(struct connect_channel),IPC_CREAT|0666);
    if(conn_id == -1){
        PRINT_INFO()
        perror("Connect Channel could not be established!\n");
        exit(1);
    }
    struct connect_channel* con_ch = (struct connect_channel*)shmat(conn_id,NULL,0666);

    PRINT_INFO()
    debugm("Register request sent by client")

    while(con_ch->semaphore < 1);

    strcpy(con_ch->client_name,argv[1]);
    con_ch->conn_sync = 1;
    con_ch->semaphore--;

    while(con_ch->comm_ch_est==0);
    //debugn(con_ch->comm_ch_est)
    if(con_ch->comm_ch_est == 2){
        PRINT_INFO()
        printf("Please use a unique name!!!\n");
        con_ch->comm_ch_est = 0;
        shmdt(con_ch);
        return (1);
    }
    con_ch->comm_ch_est = 0;

    //debugwithsval("key name: ",paths[con_ch->k_cnt]);
    key_t comm_key = ftok(paths[con_ch->k_cnt],'K');
    //key_t comm_key = ftok(argv[1],'K');
    // debugn(comm_key)
    PRINT_INFO()
    debugwithnval("Unique key generated: ",comm_key)
    // debugn(con_ch->k_cnt)
    //debugwithsval("paths[con_ch->k_cnt]: ",paths[con_ch->k_cnt])
    int comm_id = shmget(comm_key,sizeof(struct comm_channel),IPC_CREAT|0666);
    if(comm_id == -1){
        PRINT_INFO()
        perror("Comm Channel could not be established!\n");
        exit(1);
    }
    struct comm_channel *comm_ch = (struct comm_channel*)shmat(comm_id, NULL,0);
    PRINT_INFO()
    debugm("Client connected to comm channel succesfully!!!\n")
    int flg = 1;
    printf("\n");
    while(flg){
        displayMenu();
        char ch;
        scanf(" %c", &ch);
        switch(ch){
            case 'a':
                comm_ch->clrq.request_type = 1;
                PRINT_INFO()
                printf("Enter a <operator> b in order\n");
                scanf(" %d %c %d", &comm_ch->clrq.crp.arm.a,&comm_ch->clrq.crp.arm.op,&comm_ch->clrq.crp.arm.b);
                comm_ch->inputting = 1;
                PRINT_INFO()
                printf("Client has sent the request!!!\n");
                comm_ch->outputting = 0;
                if(comm_ch->svrp.response_code==400){
                    PRINT_INFO()
                    printf("SERVER DOWN!!! Terminating client...\n");
                    comm_ch->svrp.response_code = -11;
                    return (1);
                }
                while(comm_ch->outputting == 0);
                PRINT_INFO()
                printf("SERVER RESPONSE CODE: %d\n", comm_ch->svrp.response_code);
                if(comm_ch->svrp.response_code>=300){
                    PRINT_INFO()
                    printf("ERROR OPERATION NOT SUPPORTED\n");
                    break;
                }
                printf("SUCCESSFUL OPERATION\n");
                printf("Result is: %d\n",comm_ch->svrp.result);
                break;
            case 'b':
                comm_ch->clrq.request_type = 2;
                PRINT_INFO()
                printf("Enter a number\n");
                scanf(" %d",&comm_ch->clrq.crp.eoo.num);
                comm_ch->inputting = 1;
                PRINT_INFO()
                printf("Client has sent the request!!!\n");
                comm_ch->outputting = 0;
                if(comm_ch->svrp.response_code==400){
                    PRINT_INFO()
                    printf("SERVER DOWN!!! Terminating client...\n");
                    comm_ch->svrp.response_code = -11;
                    return (1);
                }
                while(comm_ch->outputting == 0);
                PRINT_INFO()
                printf("SERVER RESPONSE CODE: %d\n", comm_ch->svrp.response_code);
                if(comm_ch->svrp.response_code>=300){
                    printf("ERROR OPERATION NOT SUPPORTED\n");
                    break;
                }
                PRINT_INFO()
                printf("SUCCESSFUL OPERATION\n");
                if(comm_ch->svrp.result == 0){
                    printf("Number is EVEN\n");
                }
                else{
                    printf("Number is ODD\n");
                }
                break;
            case 'c':
                comm_ch->clrq.request_type = 3;
                PRINT_INFO()
                printf("Enter a number\n");
                scanf(" %d",&comm_ch->clrq.crp.prm.num);
                comm_ch->inputting = 1;
                PRINT_INFO()
                printf("Client has sent the request!!!\n");
                comm_ch->outputting = 0;
                if(comm_ch->svrp.response_code==400){
                    PRINT_INFO()
                    printf("SERVER DOWN!!! Terminating client...\n");
                    comm_ch->svrp.response_code = -11;
                    return (1);
                }
                while(comm_ch->outputting == 0);
                PRINT_INFO()
                printf("SERVER RESPONSE CODE: %d\n", comm_ch->svrp.response_code);
                if(comm_ch->svrp.response_code>=300){
                    printf("ERROR OPERATION NOT SUPPORTED\n");
                    break;
                }
                PRINT_INFO()
                printf("SUCCESSFUL OPERATION\n");
                if(comm_ch->svrp.result == 1){
                    printf("Number is PRIME\n");
                }
                else{
                    printf("Number is NOT PRIME\n");
                }
                break;
            case 'd':
                comm_ch->clrq.request_type = 4;
                PRINT_INFO()
                printf("Enter a number\n");
                scanf(" %d",&comm_ch->clrq.crp.neg.num);
                comm_ch->inputting = 1;
                PRINT_INFO()
                printf("Client has sent the request!!!\n");
                comm_ch->outputting = 0;
                if(comm_ch->svrp.response_code==400){
                    PRINT_INFO()
                    printf("SERVER DOWN!!! Terminating client...\n");
                    comm_ch->svrp.response_code = -11;
                    return (1);
                }
                while(comm_ch->outputting == 0);
                PRINT_INFO()
                printf("SERVER RESPONSE CODE: %d\n", comm_ch->svrp.response_code);
                if(comm_ch->svrp.response_code>=300){
                    PRINT_INFO()
                    printf("ERROR OPERATION NOT SUPPORTED\n");
                    break;
                }
                PRINT_INFO()
                printf("SUCCESSFUL OPERATION\n");
                if(comm_ch->svrp.result == 1){
                    printf("Number is NEGATIVE\n");
                }
                else{
                    printf("Number is NOT NEGATIVE\n");
                }
                break;
            case '2':
                comm_ch->inputting = 1;
                comm_ch->clrq.request_type = 9;
                PRINT_INFO()
                printf("Client has sent the request!!!\n");
                if(comm_ch->svrp.response_code==400){
                    PRINT_INFO()
                    printf("SERVER DOWN!!! Terminating client...\n");
                    comm_ch->svrp.response_code = -11;
                    return (1);
                }
                printf("Unregistering....\n");
                shmdt(comm_ch);
                flg=0;
                break;
            case '3':
                PRINT_INFO()
                printf("Exiting....\n");
                flg = 0;
                shmdt(comm_ch);
                break;
        }
        shmdt(con_ch);
    }
    return 0;
}
