#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>
#include <pthread.h>
#include "msocket.h"

// sembuf
struct sembuf pop, vop;
#define down(s) semop(s, &pop, 1) // wait(s)
#define up(s) semop(s, &vop, 1)   // signal(s)

typedef struct argtype
{
    mtp_socket *MTP_Table;
    shared_variables *shared_resource;
} argtype;

struct sockaddr_in sock_converter(char *ip, unsigned short port)
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    return addr;
}

// To find the maximum sequence number out of two in the send window (circular fashion)
int max_logical(int x, int y, int size)
{
    int space = abs(x-y);
    if (space < size/2)
    {
        return (x >= y) ? x : y;
    }
    else
    {
        return (x >= y) ? y : x;
    }
}

int min_logical(int x, int y, int size)
{
    int space = abs(x - y);
    if (space < size/2)
    {
        return (x <= y) ? x : y;
    }
    else
    {
        return (x <= y) ? y : x;
    }
}

// To copy a character array
void my_strcpy(char *a1, char *a2, int size)
{
    for (int i = 0; i < size; i++)
    {
        a1[i] = a2[i];
    }
    return;
}

volatile sig_atomic_t sigint_received = 0;
int sm_id_MTP_Table, sm_id_shared_vars;
int mtx_swnd, mtx_recvbuf, mtx_sendbuf;

/*---------------------------------------MAIN THREAD----------------------------------------------------------*/
void sigint_handler(int signum)
{
    if (signum == SIGINT)
    {
        sigint_received = 1;
        // shmdt(MTP_Table);
        // shmdt(shared_resource);

        shmctl(sm_id_shared_vars, 0, 0);
        shmctl(sm_id_MTP_Table, 0, 0);
        exit(0);
    }
    return;
}

mtp_socket *create_shared_MTP_Table()
{
    int sm_key = ftok(".", KEY_MTP_TABLE);
    int sm_id_MTP_Table = shmget(sm_key, (SIZE_SM) * sizeof(mtp_socket), 0777 | IPC_CREAT);
    printf("smid : %d\n", sm_id_MTP_Table);
    mtp_socket *MTP_Table = (mtp_socket *)shmat(sm_id_MTP_Table, 0, 0);
    return MTP_Table;
}

void printTable()
{
    mtp_socket *MTP_Table = create_shared_MTP_Table();
    printf("-----------------------------------------\n");
    printf("MTP_ID\tpid\tfree\tudp_sockid\n");
    for (int i = 0; i < SIZE_SM; i++)
    {
        printf("%d\t%d\t%d\t%d\n", i, MTP_Table[i].pid, MTP_Table[i].free, MTP_Table[i].udp_sockid);
    }
    printf("-----------------------------------------\n");
}

shared_variables *create_shared_variables()
{
    int sm_key = ftok(".", KEY_SHARED_RESOURCE);
    int sm_id_shared_vars = shmget(sm_key, sizeof(int), 0777 | IPC_CREAT);
    shared_variables *vars = (shared_variables *)shmat(sm_id_shared_vars, 0, 0);
    return vars;
}

void create_entry_semaphore(int *id)
{
    int sm_key = ftok(".", KEY_ENTRY_SEM);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 0);
}

void create_exit_semaphore(int *id)
{
    int sm_key = ftok(".", KEY_EXIT_SEM);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 0);
}

void create_mutex(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 1);
}

void create_mutex_swnd(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_SWND);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 1);
}

void create_mutex_recvbuf(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_RECVBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 1);
}

void create_mutex_sendbuf(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_SENDBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
    semctl(*id, 0, SETVAL, 1);
}

void socket_handler(mtp_socket *MTP_Table, shared_variables *shared_resource, int *entry_sem, int *exit_sem)
{
    int socket_id;
    /* The main thread(after creating R and S) for the rest of its lifetime
    server the user processes for creating, binding and closing the sockets */

    printf("Main Thread ready to go...\n");
    while (1)
    {
        printf("lol\n");
        // printTable();
        shared_resource->error_no = 0;
        down(*entry_sem);
        /* Respond to socket creation call */
        if (shared_resource->status == 0)
        {
            socket_id = socket(AF_INET, SOCK_DGRAM, 0);
            MTP_Table[shared_resource->mtp_id].udp_sockid = socket_id;
            shared_resource->return_value = socket_id;
            shared_resource->error_no = errno;
        }
        /* Respond to bind call */
        else if (shared_resource->status == 1)
        {
            socket_id = MTP_Table[shared_resource->mtp_id].udp_sockid;
            shared_resource->return_value = bind(socket_id, (const struct sockaddr *)&(shared_resource->src_addr), sizeof(shared_resource->src_addr));
            shared_resource->error_no = errno;
        }
        /* Respond to close call */
        else if (shared_resource->status == 2)
        {
            socket_id = MTP_Table[shared_resource->mtp_id].udp_sockid;
            shared_resource->return_value = close(socket_id);
            shared_resource->error_no = errno;
        }
        shared_resource->status = -1;
        up(*exit_sem);
    }

    printf("out\n");
}

/*--------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------R THREAD----------------------------------------------------------*/

void *R_Thread(void *arg)
{
    argtype *total_shared_resource = (argtype *)arg;
    mtp_socket *MTP_Table = total_shared_resource->MTP_Table;
    shared_variables *shared_resource = total_shared_resource->shared_resource;

    down(mtx_recvbuf);
    for (int i = 0; i < SIZE_SM; i++)
    {
        for (int k = 0; k < RECV_BUFFSIZE; k++)
        {
            MTP_Table[i].recv_buff[k].sequence_no = -1;
        }
        for (int k = 0; k < RWND_SIZE; k++)
        {
            MTP_Table[i].rwnd.window[k] = k+1;
        }
        MTP_Table[i].rwnd.nospace = 0;
        MTP_Table[i].rwnd.last_inorder_received = 0;
    }
    up(mtx_recvbuf);

    fd_set read_fds;
    int max_fd_value = 0;
    int action = 0;
    struct timeval tout;

    printf("R Thread ready to go...\n");

    while (1)
    {
        tout.tv_sec = TIMEOUT_S;
        tout.tv_usec = TIMEOUT_US;

        FD_ZERO(&read_fds);
        for (int i = 0; i < SIZE_SM; i++)
        {
            if (!MTP_Table[i].free)
            {
                FD_SET(MTP_Table[i].udp_sockid, &read_fds);

                if (MTP_Table[i].udp_sockid > max_fd_value)
                    max_fd_value = MTP_Table[i].udp_sockid;
            }
        }

        action = select(max_fd_value + 1, &read_fds, NULL, NULL, &tout);
        /*  Timeout  */
        if (action == 0)
        {
            // continue; //// DO NOT USE IT
        }


        for (int i = 0; i < SIZE_SM; i++)
        {
            if(i == 1)
            {
                printf("RECV WINDOW : ");
                for (int k = 0; k < RWND_SIZE; k++)
                {
                    printf("%d ", MTP_Table[i].rwnd.window[k]);
                }
                printf("\n");
                printf("RECV BUFFER : ");
                for (int k = 0; k < RECV_BUFFSIZE; k++)
                {
                    printf("%d ", MTP_Table[i].recv_buff[k].sequence_no);
                }
                printf("\n");
                
            }
            if (!MTP_Table[i].free)
            {
                /* Some message received */
                if (FD_ISSET(MTP_Table[i].udp_sockid, &read_fds))
                {
                    char udp_data[KB + 6];
                    int udp_id = MTP_Table[i].udp_sockid;
                    struct sockaddr_in dest_addr = sock_converter(MTP_Table[i].dest_ip, MTP_Table[i].dest_port);
                    int len = sizeof(dest_addr);
                    int bytes = recvfrom(udp_id, udp_data, KB + 6, 0, (struct sockaddr *)&dest_addr, &len);
                    printf("RECEIEVD HERE for %d : %s\n", i, udp_data);
                    // Error in receiving
                    if (bytes <= 0)
                        continue;

                    udp_data[bytes] = '\0';
                    
                    // Message is acknowledgement
                    if (udp_data[0] == 'A')
                    {
                        down(mtx_swnd);
                        int ack_seqno = (int)(udp_data[1]-'a');
                        int curr_empty_space = (int)(udp_data[2]-'a');
                        send_window curr_swnd = MTP_Table[i].swnd;
                        int last_ack_seqno = curr_swnd.last_ack_seqno;

                        printf("user %d: last_ack_seqno: %d, ack_seqno: %d\n",i,  last_ack_seqno, ack_seqno);

                        int flag_dup_seq_ack = 0;
                        if (max_logical(last_ack_seqno, ack_seqno, MAX_SEQ_NO) == last_ack_seqno && curr_swnd.last_ack_emptyspace == curr_empty_space)
                        {
                            // Duplicate ACK received
                            printf("user %d: Got repeated ack\n", i);
                            up(mtx_swnd);
                            continue;
                        }
                        else
                        {
                            if (max_logical(last_ack_seqno, ack_seqno, MAX_SEQ_NO) == last_ack_seqno && curr_swnd.last_ack_emptyspace != curr_empty_space)
                            {
                                flag_dup_seq_ack = 1;
                            }
                            // Update the send window upon receiving valid ACK
                            down(mtx_sendbuf);
                            printf("user %d: old snwd: [%d, %d]\n",i, MTP_Table[i].swnd.left_idx, MTP_Table[i].swnd.right_idx);
                            int k = MTP_Table[i].swnd.left_idx;
                            if(flag_dup_seq_ack==0)
                            {
                                while (MTP_Table[i].send_buff[k].sequence_no != ack_seqno)
                                {
                                    MTP_Table[i].send_buff[k].sequence_no = -1;
                                    k = (k + 1) % SEND_BUFFSIZE;
                                }
                                MTP_Table[i].send_buff[k].sequence_no = -1;
                                k = (k+1)%SEND_BUFFSIZE;
                            }
                            

                            MTP_Table[i].swnd.left_idx = k;

                            MTP_Table[i].swnd.right_idx = (k + curr_empty_space - 1) % SEND_BUFFSIZE;
                            // while (left != (right + 1) % SEND_BUFFSIZE)
                            // {
                            //     if (MTP_Table[i].send_buff[left].sequence_no == -1)
                            //     {
                            //         break;
                            //     }
                            // MTP_Table[i].swnd.right_idx = (right) % SEND_BUFFSIZE;

                            //     left = (left + 1) % SEND_BUFFSIZE;
                            // }

                            MTP_Table[i].swnd.last_ack_seqno = ack_seqno;
                            printf("user %d: new snwd: [%d, %d]\n", i, MTP_Table[i].swnd.left_idx, MTP_Table[i].swnd.right_idx);
                            up(mtx_sendbuf);
                        }
                        if(i == 0)
                        {
                            printf("SEND BUFFER SEQ NO : ");
                            for (int k = 0; k < SEND_BUFFSIZE; k++)
                            {
                                printf("%d ", MTP_Table[i].send_buff[k].sequence_no);
                            }
                            printf("\n");
                        }
                        up(mtx_swnd);
                    }
                    // Message is user data
                    else
                    {
                        down(mtx_recvbuf);
                        int seq_no = (int)(udp_data[1]-'a');
                        for (int k = 0; k < RWND_SIZE; k++)
                        {
                            if (MTP_Table[i].rwnd.window[k] == seq_no)
                            {
                                for (int e = 0; e < RECV_BUFFSIZE; e++)
                                {
                                    if (MTP_Table[i].recv_buff[e].sequence_no == -1)
                                    {
                                        MTP_Table[i].recv_buff[e].sequence_no = (int)(udp_data[1]-'a');
                                        my_strcpy(MTP_Table[i].recv_buff[e].data, udp_data + 2, KB);
                                        printf("*** %s\n", udp_data);
                                        if (seq_no == (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NO + 1 )
                                        {
                                            MTP_Table[i].rwnd.last_inorder_received = seq_no;
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        int empty_space = 0;
                        int seqno_at_buff[17];
                        memset(seqno_at_buff, 0, sizeof(seqno_at_buff));
                        // Count the empty spaces
                        for (int k = 0; k < RECV_BUFFSIZE; k++)
                        {
                            if (MTP_Table[i].recv_buff[k].sequence_no == -1)
                            {
                                empty_space++;
                            }
                            else
                            {
                                seqno_at_buff[MTP_Table[i].recv_buff[k].sequence_no] = 1;
                            }
                        }
                        if (empty_space != 0)
                        {
                            // Update receive window
                            memset(MTP_Table[i].rwnd.window, -1, sizeof(MTP_Table[i].rwnd.window));
                            int rwnd_idx = 0;
                            int curr = (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NO + 1;
                            for (int e = 1; e <= empty_space; e++)
                            {
                                while (seqno_at_buff[curr] != 0)
                                {
                                    curr = (curr) % MAX_SEQ_NO + 1;
                                }
                                MTP_Table[i].rwnd.window[rwnd_idx++] = curr;
                                curr = (curr)%MAX_SEQ_NO + 1;
                            }
                        }
                        // Send the ACK
                        char ACK_data_udp[3] = {'A', (char)(MTP_Table[i].rwnd.last_inorder_received+'a'), (char)(empty_space+'a')};
                        int udp_id = MTP_Table[i].udp_sockid;
                        struct sockaddr_in dest_addr = sock_converter(MTP_Table[i].dest_ip, MTP_Table[i].dest_port);
                        int bytes = sendto(udp_id, ACK_data_udp, 3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        if (empty_space == 0)
                        {
                            printf("user %d : no space *******\n\n\n\n\n", i);
                            MTP_Table[i].rwnd.nospace = 1;
                        }
                        up(mtx_recvbuf);
                    }
                }
                else
                {
                    /* Receive buffer was acknowledged to be full earlier */
                    printf("User %d : nospace %d\n", i, MTP_Table[i].rwnd.nospace);
                    if (MTP_Table[i].rwnd.nospace==1)
                    {
                        down(mtx_recvbuf);
                        int empty_space = 0;
                        int seqno_at_buff[17];
                        memset(seqno_at_buff, 0, sizeof(seqno_at_buff));
                        // Count the empty spaces
                        for (int k = 0; k < RECV_BUFFSIZE; k++)
                        {
                            if (MTP_Table[i].recv_buff[k].sequence_no == -1)
                            {
                                empty_space++;
                            }
                            else
                            {
                                seqno_at_buff[MTP_Table[i].recv_buff[k].sequence_no] = 1;
                            }
                        }
                        if (empty_space != 0)
                        {
                            // Empty space in receive buffer
                            MTP_Table[i].rwnd.nospace = 0;

                            // Update receive window
                            memset(MTP_Table[i].rwnd.window, -1, sizeof(MTP_Table[i].rwnd.window));
                            int rwnd_idx = 0;
                            int curr = (MTP_Table[i].rwnd.last_inorder_received) % MAX_SEQ_NO + 1;
                            for (int e = 1; e <= empty_space; e++)
                            {
                                while (seqno_at_buff[curr] != 0)
                                {
                                    curr = (curr) % MAX_SEQ_NO + 1;
                                }
                                MTP_Table[i].rwnd.window[rwnd_idx++] = curr;
                                curr = (curr)%MAX_SEQ_NO + 1;
                            }
                            // Send the ACK
                            char ACK_data_udp[3] = {'A', (char)(MTP_Table[i].rwnd.last_inorder_received+'a'), (char)(empty_space+'a')};
                            int udp_id = MTP_Table[i].udp_sockid;
                            printf("User %d : Sending ACK new space : %s\n",i, ACK_data_udp);
                            struct sockaddr_in dest_addr = sock_converter(MTP_Table[i].dest_ip, MTP_Table[i].dest_port);
                            int bytes = sendto(udp_id, ACK_data_udp, 3, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
                        }
                        up(mtx_recvbuf);
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}

/*--------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------S THREAD----------------------------------------------------------*/

void *S_Thread(void *arg)
{
    argtype *total_shared_resource = (argtype *)arg;
    mtp_socket *MTP_Table = total_shared_resource->MTP_Table;
    shared_variables *shared_resource = total_shared_resource->shared_resource;

    down(mtx_swnd);
    down(mtx_sendbuf);
    for (int i = 0; i < SIZE_SM; i++)
    {
        for (int k = 0; k < SEND_BUFFSIZE; k++)
        {
            MTP_Table[i].send_buff[k].sequence_no = -1;
        }
        MTP_Table[i].swnd.left_idx = 0;
        MTP_Table[i].swnd.right_idx = (RECV_BUFFSIZE - 1) % SEND_BUFFSIZE;
        MTP_Table[i].swnd.new_entry = 0;
        MTP_Table[i].swnd.last_seq_no = 0;
        MTP_Table[i].swnd.last_sent = -1;
        MTP_Table[i].swnd.last_ack_seqno = 0;
        for (int k = 0; k < SWND_SIZE; k++)
        {
            MTP_Table[i].swnd.last_active_time[k] = (time_t)0;
        }
        
    }
    up(mtx_sendbuf);
    up(mtx_swnd);

    printf("S Thread ready to go...\n");

    while (1)
    {
        for (int i = 0; i < SIZE_SM; i++)
        {
            if (!MTP_Table[i].free)
            {
                down(mtx_swnd);
                down(mtx_sendbuf);
                // Checking valid portion to send
                send_window curr_swnd = MTP_Table[i].swnd;
                
                int left = curr_swnd.left_idx;
                int right = curr_swnd.right_idx;

                time_t curr_time = time(NULL);
                int is_tout = 0;
                if ((right + 1) % SEND_BUFFSIZE == left)
                {
                    // SWND EMPTY
                    up(mtx_sendbuf);
                    up(mtx_swnd);
                    continue;
                }
                while (left != (right + 1) % SEND_BUFFSIZE)
                {
                    if ((curr_time - curr_swnd.last_active_time[left] > (time_t)T) && (curr_swnd.last_active_time[left] > 0) && (MTP_Table[i].send_buff[left].sequence_no > 0))
                    {
                        is_tout = 1;
                        break;
                    }
                    left = (left + 1) % SEND_BUFFSIZE;
                }
                if (is_tout)
                {
                    left = curr_swnd.left_idx;
                    right = curr_swnd.right_idx;

                    while (left != (right + 1) % SEND_BUFFSIZE)
                    {
                        if(MTP_Table[i].send_buff[left].sequence_no < 0)
                        {
                            break;
                        }
                        char udp_data[KB + 2];
                        udp_data[0] = 'D';
                        udp_data[1] = (char)(MTP_Table[i].send_buff[left].sequence_no + 'a');
                        my_strcpy(udp_data + 2, MTP_Table[i].send_buff[left].data, KB);
                        struct sockaddr_in dest_addr = sock_converter(MTP_Table[i].dest_ip, MTP_Table[i].dest_port);
                        int bytes = sendto(MTP_Table[i].udp_sockid, udp_data, KB + 2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        printf("user %d: Sending data \"%s\" due to TIMEOUT at %d\n", i, udp_data, time(NULL));

                        MTP_Table[i].swnd.last_active_time[left] = time(NULL);
                        MTP_Table[i].swnd.last_sent = left;

                        left = (left + 1) % SEND_BUFFSIZE;
                    }
                }
                else
                {
                    left = (curr_swnd.last_sent + 1)%SEND_BUFFSIZE;
                    right = curr_swnd.right_idx;

                    while (left != (right + 1) % SEND_BUFFSIZE)
                    {
                        if(MTP_Table[i].send_buff[left].sequence_no < 0)
                        {
                            break;
                        }
                        char udp_data[KB + 2];
                        udp_data[0] = 'D';
                        udp_data[1] = (char)(MTP_Table[i].send_buff[left].sequence_no + 'a');
                        my_strcpy(udp_data + 2, MTP_Table[i].send_buff[left].data, KB);
                        struct sockaddr_in dest_addr = sock_converter(MTP_Table[i].dest_ip, MTP_Table[i].dest_port);
                        int bytes = sendto(MTP_Table[i].udp_sockid, udp_data, KB + 2, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

                        printf("user %d: Sending data \"%s\" due to NOT SENT EARLLIER at %d\n", i, udp_data, time(NULL));

                        MTP_Table[i].swnd.last_active_time[left] = time(NULL);
                        MTP_Table[i].swnd.last_sent = left;

                        left = (left + 1) % SEND_BUFFSIZE;
                    }
                }
                up(mtx_sendbuf);
                up(mtx_swnd);
            }
        }
        sleep(T / 2);
    }
}


int main()
{
    signal(SIGINT, sigint_handler);

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
        MTP_Table[i].pid = i + 5;
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

    create_mutex_swnd(&mtx_swnd);

    create_mutex_recvbuf(&mtx_recvbuf);

    create_mutex_sendbuf(&mtx_sendbuf);

    pthread_t R, S;

    argtype *arg = (argtype *)malloc(sizeof(argtype));
    arg->MTP_Table = MTP_Table;
    arg->shared_resource = shared_resource;

    // Create R thread
    if (pthread_create(&R, NULL, R_Thread, (void *)arg) != 0)
    {
        perror("Failed to create R thread");
        exit(EXIT_FAILURE);
    }

    // Create S thread
    if (pthread_create(&S, NULL, S_Thread, (void *)arg) != 0)
    {
        perror("Failed to create S thread");
        exit(EXIT_FAILURE);
    }

    sleep(1);
    // printTable();

    socket_handler(MTP_Table, shared_resource, &entry_sem, &exit_sem);

    // Join R thread
    if (pthread_join(R, NULL) != 0)
    {
        perror("Failed to join R thread");
        exit(EXIT_FAILURE);
    }

    // Join S thread
    if (pthread_join(S, NULL) != 0)
    {
        perror("Failed to join S thread");
        exit(EXIT_FAILURE);
    }
}