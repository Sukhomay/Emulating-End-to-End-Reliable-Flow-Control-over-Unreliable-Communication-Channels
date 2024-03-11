/*------------------- STANDARD LIBRARIES ---------------------*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>

/*----------------- MACROS -----------------*/
#define SOCK_MTP 115

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

/*--------------- FUNCTION DECLARATIONS ---------------*/
int m_socket(int domain, int type, int protocol);
int m_bind(int socket_id, char *src_ip, unsigned short int src_port, char *dest_ip, unsigned short int dest_port);
// int m_sendto(int socket_id, char *buffer, int size, int flags, struct sock_addr *dest, int len);
// int m_recvfrom(int socket_id, char *buffer, int size, int flags, struct sock_addr *dest, int *len);
int m_close(int socket_id);