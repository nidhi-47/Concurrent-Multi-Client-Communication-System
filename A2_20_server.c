/*****************************************************
 *      CS F372 Operating Systems
 *          ASSIGNMENT 2 (SERVERv2)
 * 
 *  2020A7PS1712H Yash Vardhan Singh     
 *  2020A4PS2009H Nidhi Agarwal
 *  2020AAPS1779H Reuben George
 * 
 * *****************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

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

char list_of_clients[20][256];
int num_clients = -1;

int closed_clients = 0;

int primes[PRIMES];
int sizeOfPrimes;

int key_cnt = 0;

int total_req = 0;

// Declaration of thread condition variable
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
 
// declaring mutex
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

char *paths[33] = {"/bin/cat","/bin/chgrp","/bin/chmod","/bin/chown","/bin/cp","/bin/date","/bin/dd","/bin/df","/bin/dmesg","/bin/echo","/bin/false","/bin/hostname","/bin/kill","/bin/ln","/bin/login","/bin/ls","/bin/mkdir","/bin/mknod","/bin/more","/bin/mount","/bin/mv","/bin/ps","/bin/pwd","/bin/rm","/bin/rmdir","/bin/sed","/bin/sh","/bin/stty","/bin/su","/bin/sync","/bin/true","/bin/unmount","/bin/uname"};
//int keys[10] = {101,201,301,401,501,601,701,801,901,1001};

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
struct comm_channel *comm_channels[20]={NULL};

struct thread_args{
    struct connect_channel *con_ch;
    int cli_idx;
    int *thread_term;
    int comm_id;
    struct comm_channel *comm_ch;
};

void catch_int(int sig_num)
{
    //signal(SIGINT, catch_int);
    key_t serv_key = ftok("server",'Y');
    int serv_id = shmget(serv_key,sizeof(struct connect_channel),0);
    if(shmctl(serv_id,IPC_RMID,NULL) == -1){
        PRINT_INFO()
        perror("Connect channel couldn't be released!!\n");
        exit(1);
    }
    PRINT_INFO()
    debugm("Connect channel closed!\n")
    for(int i = 0; i<num_clients+1;i++){
        comm_channels[i]->svrp.response_code = 400;
        comm_channels[i]->outputting=1;
    }
    PRINT_INFO()
    debugm("SERVER DOWN\n")
    pid_t pid = getpid();
    sleep(1);
    kill(pid,SIGTERM);
}

int primeNumbersTillN(int n, int arr[]){
    bool prime[n+1];
    memset(prime, true, sizeof(prime));
    for(int k = 2; k*k<=n;k++){
        if(prime[k] == true){
            for(int j = k*k;j<=n; j+=k)
                prime[j] = false;
        }
    }
    int j =0;
    for(int i = 2; i<=n; i++){
        if(prime[i]){
            arr[j++]=i;
        }
    }
    return j;
}

void* processRequest(void* args){
    struct thread_args *targs = (struct thread_args*) args;
    targs->con_ch->comm_ch_est = 1;
    PRINT_INFO()
    debugwithsval("Communication Channel for client succesfully created with client name: ", list_of_clients[targs->cli_idx]);sleep(1);
    targs->con_ch->k_cnt=key_cnt;
    targs->comm_ch->inputting = 0;
    pthread_cond_signal(&cond1);
    while(targs->comm_ch->inputting == 0);
    while(targs->comm_ch->clrq.request_type != 9){
        while(targs->comm_ch->inputting == 0);
        if(targs->comm_ch->clrq.request_type == 1){
            targs->comm_ch->clrq.request_type = -11;
            targs->comm_ch->svrp.client_resp_seq_no++;
            total_req++;
            targs->comm_ch->svrp.server_resp_seq_no = total_req;
            PRINT_INFO()
            debugwithsval("Request on Comm Channel received from client: ", list_of_clients[targs->cli_idx])
            printf("Calculating...\n\n");
            char op = targs->comm_ch->clrq.crp.arm.op;
            int a = targs->comm_ch->clrq.crp.arm.a;
            int b = targs->comm_ch->clrq.crp.arm.b;
            printf("%d %c %d\n\n",a,op,b);
            if(op == '+'){
                targs->comm_ch->svrp.result = a+b;
            }
            else if(op == '-'){
                targs->comm_ch->svrp.result = a-b;
            }
            else if(op == '*'){
                targs->comm_ch->svrp.result = a*b;
            }
            else if(op == '/'){
                targs->comm_ch->svrp.result = a/b;
            }
            PRINT_INFO()
            printf("Result calculated: %d\n", targs->comm_ch->svrp.result);
            debugwithsval("Server responded to client: ", list_of_clients[targs->cli_idx])
            targs->comm_ch->svrp.response_code = 200;
            targs->comm_ch->inputting = 0;
            targs->comm_ch->outputting = 1;
        }
        else if(targs->comm_ch->clrq.request_type == 2){
            targs->comm_ch->clrq.request_type = -11;
            targs->comm_ch->svrp.client_resp_seq_no++;
            total_req++;
            targs->comm_ch->svrp.server_resp_seq_no = total_req;
            PRINT_INFO()
            debugwithsval("Request on Comm Channel received from client: ", list_of_clients[targs->cli_idx])
            printf("Calculating...\n\n");
            int num = targs->comm_ch->clrq.crp.eoo.num;
            printf("Checking if %d is even or odd ...\n\n",num);
            targs->comm_ch->svrp.result = num%2;
            PRINT_INFO()
            printf("Result calculated: %d\n", targs->comm_ch->svrp.result);
            debugwithsval("Server responded to client: ", list_of_clients[targs->cli_idx])
            targs->comm_ch->svrp.response_code = 200;
            targs->comm_ch->inputting = 0;
            targs->comm_ch->outputting = 1;
        }
        else if(targs->comm_ch->clrq.request_type == 3){
            targs->comm_ch->clrq.request_type = -11;
            targs->comm_ch->svrp.client_resp_seq_no++;
            total_req++;
            targs->comm_ch->svrp.server_resp_seq_no = total_req;
            PRINT_INFO()
            debugwithsval("Request on Comm Channel received from client: ", list_of_clients[targs->cli_idx])
            printf("Calculating...\n\n");
            int num = targs->comm_ch->clrq.crp.prm.num;
            printf("Checking if %d is prime or not ...\n\n",num);
            targs->comm_ch->svrp.result = 0;
            sizeOfPrimes = primeNumbersTillN(PRIMES,primes);
            for(int p = 0; p<sizeOfPrimes;p++){
                if(num == primes[p]){
                    targs->comm_ch->svrp.result = 1;
                    break;
                }
            }
            PRINT_INFO()
            printf("Result calculated: %d\n", targs->comm_ch->svrp.result);
            debugwithsval("Server responded to client: ", list_of_clients[targs->cli_idx])
            targs->comm_ch->svrp.response_code = 200;
            targs->comm_ch->inputting = 0;
            targs->comm_ch->outputting = 1;
        }
        else if(targs->comm_ch->clrq.request_type == 4){
            targs->comm_ch->clrq.request_type = -11;
            targs->comm_ch->svrp.client_resp_seq_no++;
            total_req++;
            targs->comm_ch->svrp.server_resp_seq_no = total_req;
            PRINT_INFO()
            debugwithsval("Request on Comm Channel received from client: ", list_of_clients[targs->cli_idx])
            printf("Unsupported request :(\n");
            PRINT_INFO()
            debugwithsval("Server responded to client: ", list_of_clients[targs->cli_idx])
            targs->comm_ch->svrp.response_code = 300;
            targs->comm_ch->inputting = 0;
            targs->comm_ch->outputting = 1;
        }
    }
    //debugn(targs->comm_ch->clrq.request_type)
    if(targs->comm_ch->clrq.request_type == 9){
        targs->comm_ch->svrp.client_resp_seq_no++;
        total_req++;
        targs->comm_ch->svrp.server_resp_seq_no = total_req;
        closed_clients--;
        //debugn(closed_clients)
        PRINT_INFO()
        debugwithsval("Terminating the client: ",list_of_clients[targs->cli_idx])
        PRINT_INFO()
        debugwithnval("Total request served to this client: ",targs->comm_ch->svrp.client_resp_seq_no)
        PRINT_INFO()
        debugwithnval("Total requests served by the server: ",total_req)
        if(shmctl(targs->comm_id,IPC_RMID,NULL) == -1){
            PRINT_INFO()
            perror("Client thread termination error!!\n");
            exit(1);
        }
        PRINT_INFO()
        debugwithsval("Comm Channel cleaned. Unregistered client: ", list_of_clients[targs->cli_idx])
        memset(list_of_clients[targs->cli_idx],0,sizeof(list_of_clients[targs->cli_idx]));
    }
    //*targs->thread_term = 1;
    pthread_exit(NULL);
}


int main(){
    signal(SIGINT, catch_int);
    PRINT_INFO()
    debugm("Server ON\n")
    PRINT_INFO()
    debugm("Server initiated and created Connect Channel\n")
    key_t conn_key = ftok("server",'Y');
    //debugh(conn_key)
    int conn_id = shmget(conn_key,sizeof(struct connect_channel),IPC_CREAT|0666);
    if(conn_id == -1){
        PRINT_INFO()
        perror("Connect Channel could not be established!\n");
        exit(1);
    }
    struct connect_channel* con_ch = (struct connect_channel*)shmat(conn_id,NULL,0);
    con_ch->k_cnt=key_cnt;
    con_ch-> comm_ch_est = 0;
    con_ch->conn_sync = 0;
    con_ch->semaphore = 1;
    for(int i =0; i<20; i++)
        memset(list_of_clients[i],0,sizeof(list_of_clients[i]));

    pthread_t cli_threads[20];
    struct thread_args targs[20];
    for(int i=0; i<20;i++)
        targs[i].thread_term == 0;

    char choice = 'Y';
    do{
        while(con_ch->conn_sync == 0);
        PRINT_INFO()
        debugm("Server received Register Request on Connect Channel")
        num_clients++;

        int uflag = 0;

        for(int c = 0; c<num_clients+1;c++){
            if(strcmp(list_of_clients[c],con_ch->client_name)==0){
                PRINT_INFO()
                debugwithsval("Server rejected Register Request from: ",list_of_clients[c])
                con_ch->comm_ch_est = 2;
                uflag = 1;
                break;
            }
        }
        if(uflag ==1){
            con_ch->semaphore++;
            con_ch->conn_sync = 0;
            memset(con_ch->client_name, 0, sizeof(con_ch->client_name));
            continue;
        }
        //debugwithsval("con_ch->client_name: ",con_ch->client_name)
        strcpy(list_of_clients[num_clients],con_ch->client_name);
        PRINT_INFO()
        printf("\nList of registered clients: \n");
        for(int i = 0 ;i<num_clients+1; i++){
            if(strlen(list_of_clients[i]) == 0)
                continue;
            printf("\n%s ", list_of_clients[i]);
        }
        PRINT_INFO()
        debugwithsval("Server registered client with ID: ", con_ch->client_name)
        //debugwithsval("key name: ",paths[key_cnt]);
        key_t comm_key = ftok(paths[key_cnt],'K');
        PRINT_INFO()
        debugwithnval("Unique key generated: ",comm_key)
        // debugn(key_cnt)
        //debugwithsval("paths[key_cnt]: ",paths[key_cnt])
        int comm_id = shmget(comm_key, sizeof(struct comm_channel), IPC_CREAT|0666);
        key_cnt++;
        if(comm_id == -1){
            perror("");
            PRINT_INFO()
            printf("Comm Channel could not be established for %s !\n",list_of_clients[num_clients]);
            exit(1);
        }
        struct comm_channel* comm_ch = (struct comm_channel*)shmat(comm_id,NULL,0);
        comm_channels[num_clients] = comm_ch;
        total_req++;
        comm_ch->svrp.response_code = -11;
        comm_ch->num_req  =0;
        comm_ch->svrp.client_resp_seq_no = 1;
        int thread_flag = 0;
        targs[num_clients].con_ch = con_ch;
        targs[num_clients].comm_id = comm_id;
        targs[num_clients].thread_term = &thread_flag;
        targs[num_clients].cli_idx = num_clients;
        targs[num_clients].comm_ch = comm_ch;
        PRINT_INFO()
        debugwithnval("Total clients active: ",num_clients+closed_clients+1)
        pthread_create(&cli_threads[num_clients],NULL,processRequest,(void*)(&(targs[num_clients])));

        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond1,&lock);
        pthread_mutex_unlock(&lock);

        con_ch->semaphore++;
        con_ch->conn_sync = 0;
        memset(con_ch->client_name, 0, sizeof(con_ch->client_name));
        //scanf(" %c", &choice);
    }while(choice == 'Y' || choice == 'y');

    pthread_mutex_destroy(&lock);

    if(shmdt(con_ch) == -1){
        PRINT_INFO()
        perror("shmdt");
        exit(1);
    }
    if(shmctl(conn_id,IPC_RMID,NULL) == -1){
        PRINT_INFO()
        perror("Connect channel couldn't be released!!\n");
        exit(1);
    }
    PRINT_INFO()
    debugm("Connect channel closed!\n")
    return 0;
}
