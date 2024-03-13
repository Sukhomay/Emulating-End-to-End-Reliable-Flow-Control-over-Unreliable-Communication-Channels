/*------------------- STANDARD LIBRARIES ---------------------*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

/*----------------- MACROS -----------------*/
#define SOCK_MTP 115
#define ENOBOUND 1003
#define NOMSG 1005

#define KEY_MTP_TABLE 105
#define KEY_SHARED_RESOURCE 35
#define KEY_ENTRY_SEM 23
#define KEY_EXIT_SEM 21
#define KEY_MUTEX 19
#define KEY_MUTEX_SWND 28
#define KEY_MUTEX_RECVBUF 89
#define KEY_MUTEX_SENDBUF 65

#define SIZE_SM 25
#define KB 1024
#define IP_SIZE 20
#define SEND_BUFFSIZE 10
#define RECV_BUFFSIZE 5
#define SWND_SIZE 5
#define RWND_SIZE 5
#define MAX_SEQ_NO 16

#define TIMEOUT_S 4
#define TIMEOUT_US 0
#define T 5


/*------------------ STRUCTURES ----------------*/
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
    int last_ack_seqno; // seq_no 
    int last_ack_emptyspace; // rwndsize
    int last_sent; // index no
    time_t last_active_time[SWND_SIZE];
    int new_entry; // index no
    int last_seq_no; // Used in m-sendto(...) to keep track of sequence in which messages are sent
} send_window;

typedef struct receive_window
{
    int window[5];
    int last_inorder_received;
    int nospace;
} receive_window;

typedef struct mtp_socket
{
    int free;
    pid_t pid;
    int udp_sockid;
    char dest_ip[IP_SIZE];
    unsigned short int dest_port;
    message send_buff[SEND_BUFFSIZE];
    message recv_buff[RECV_BUFFSIZE];
    send_window swnd;
    receive_window rwnd;
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

/*--------------- FUNCTION DECLARATIONS ---------------*/
int m_socket(int domain, int type, int protocol);
int m_bind(int socket_id, char *src_ip, unsigned short int src_port, char *dest_ip, unsigned short int dest_port);
int m_sendto(int socket_id, char *buffer, int size, int flags, struct sockaddr *dest, int len);
int m_recvfrom(int socket_id, char *buffer, int size, int flags, struct sockaddr *dest, int *len);
int m_close(int socket_id);