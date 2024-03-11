#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>

#define KEY_MTP_TABLE 34
#define KEY_SHARED_RESOURCE 35
#define KEY_ENTRY_SEM 23
#define KEY_EXIT_SEM 21
#define KEY_MUTEX 19

#define SIZE_SM 25
#define KB 1024
#define IP_SIZE 20
#define SEND_BUFFSIZE 10
#define RECV_BUFFSIZE 5
#define SWND_SIZE 5
#define RWND_SIZE 5

// sembuf
struct sembuf pop, vop;
#define down(s) semop(s, &pop, 1)   // wait(s)
#define up(s) semop(s, &vop, 1)     // signal(s)

typedef struct message
{
    int sequence_no;
    char data[KB];
}
message;

typedef struct send_window
{
    int left_idx;
    int right_idx;
    int new_entry;
    int first_sequence_no;
    int last_sequence_no;
}
send_window;

typedef struct mtp_socket
{
    int free;
    pid_t pid;
    int udp_sockid;
    char dest_ip[IP_SIZE];
    unsigned short int dest_port;
    message send_buff[SEND_BUFFSIZE];
    message recv_buff[RECV_BUFFSIZE];
    send_window swnd[SWND_SIZE];
    int rwnd[RWND_SIZE];
}
mtp_socket;

typedef struct shared_variables
{
    int status;
    int mtp_id;
    struct sockaddr_in src_addr;

    int return_value;
    int error_no;
}
shared_variables;


volatile sig_atomic_t sigint_received = 0;
int sm_id_MTP_Table, sm_id_shared_vars;

/*---------------------------------------MAIN THREAD----------------------------------------------------------*/
void sigint_handler(int signum)
{
    if(signum == SIGINT)
    {
        sigint_received = 1;
        return;
    }
    return;
}

mtp_socket *create_shared_MTP_Table()
{
    int sm_key = ftok(".", KEY_MTP_TABLE);
    int sm_id_MTP_Table = shmget(sm_key, (SIZE_SM)*sizeof(mtp_socket), 0777|IPC_CREAT);
    mtp_socket *MTP_Table = (mtp_socket *)shmat(sm_id_MTP_Table, 0, 0);
    return MTP_Table;
}

shared_variables *create_shared_variables()
{
    int sm_key = ftok(".", KEY_SHARED_RESOURCE);
    int sm_id_shared_vars = shmget(sm_key, sizeof(int), 0777|IPC_CREAT);
    shared_variables *vars = (shared_variables *)shmat(sm_id_shared_vars, 0, 0);
    return vars;
}

void create_entry_semaphore(int *id)
{
    int sm_key = ftok(".", KEY_ENTRY_SEM);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(*id, 0, SETVAL, 0);
}

void create_exit_semaphore(int *id)
{
    int sm_key = ftok(".", KEY_EXIT_SEM);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(*id, 0, SETVAL, 0);
}

void create_mutex(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
    semctl(*id, 0, SETVAL, 1);
}


void socket_handler(mtp_socket *MTP_Table, shared_variables *shared_resource, int *entry_sem, int *exit_sem)
{
    int socket_id;
    /* The main thread(after creating R and S) for the rest of its lifetime 
    server the user processes for creating, binding and closing the sockets */
    while (1)
    {
        printf("lol\n");
        shared_resource->error_no = 0;
        down(*entry_sem);
        /* Respond to socket creation call */
        if(shared_resource->status==0)
        {
            socket_id = socket(AF_INET, SOCK_DGRAM, 0);
            MTP_Table[shared_resource->mtp_id].udp_sockid = socket_id;
            shared_resource->return_value = socket_id;
            shared_resource->error_no = errno;
        }
        /* Respond to bind call */
        else if(shared_resource->status==1)
        {
            socket_id = MTP_Table[shared_resource->mtp_id].udp_sockid;
            shared_resource->return_value = bind(socket_id, (const struct sockaddr *)&(shared_resource->src_addr), sizeof(shared_resource->src_addr));
            shared_resource->error_no = errno;
        }
        /* Respond to close call */
        else if(shared_resource->status == 2)
        {
            socket_id = MTP_Table[shared_resource->mtp_id].udp_sockid;
            shared_resource->return_value = close(socket_id);
            shared_resource->error_no = errno;
        }
        shared_resource->status = -1;
        up(*exit_sem);
    }

    printf("out\n");

    shmdt(MTP_Table);
    shmdt(shared_resource);
    
    shmctl(sm_id_shared_vars, 0, 0);
    shmctl(sm_id_MTP_Table, 0 , 0);
}

/*--------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------R THREAD----------------------------------------------------------*/

void *R_Thread(void *arg)
{
    
}


int main()
{
    // signal(SIGINT, sigint_handler);

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;
    
    /* Shared Memory creation */
    mtp_socket *MTP_Table = create_shared_MTP_Table();
    for (int i = 0; i < SIZE_SM; i++)
    {
        MTP_Table[i].free = 1;
        // MTP_Table[i].udp_sockid = 256;
    }
    
    /* Shared Resouces creation for communication with the user process */
    shared_variables *shared_resource = create_shared_variables();

    /* Entry Semaphore creation */
    int entry_sem;
    create_entry_semaphore(&entry_sem);

    /* Exit Semaphore creation */
    int exit_sem;
    create_exit_semaphore(&exit_sem);

    int mtx;
    create_mutex(&mtx);



    socket_handler(MTP_Table, shared_resource, &entry_sem, &exit_sem);
}