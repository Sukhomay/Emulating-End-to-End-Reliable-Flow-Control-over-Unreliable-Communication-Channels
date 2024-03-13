#include "msocket.h"
#define ERR -1
#define SUCC 0

// sembuf
struct sembuf pop, vop;
#define down(s) semop(s, &pop, 1)   // wait(s)
#define up(s) semop(s, &vop, 1)     // signal(s)

mtp_socket *create_shared_MTP_Table()
{
    int sm_key = ftok(".", KEY_MTP_TABLE);
    int sm_id = shmget(sm_key, (SIZE_SM)*sizeof(mtp_socket), 0777|IPC_CREAT);
    mtp_socket *MTP_Table = (mtp_socket *)shmat(sm_id, 0, 0);
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
}

void create_exit_semaphore(int *id)
{
    int sm_key = ftok(".", KEY_EXIT_SEM);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
}

void create_mutex(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX);
    *id = semget(sm_key, 1, 0777|IPC_CREAT);
}

void create_mutex_swnd(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_SWND);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

void create_mutex_recvbuf(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_RECVBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

void create_mutex_sendbuf(int *id)
{
    int sm_key = ftok(".", KEY_MUTEX_SENDBUF);
    *id = semget(sm_key, 1, 0777 | IPC_CREAT);
}

void my_strcpy(char *a1, char *a2, int size)
{
    for (int i = 0; i < size; i++)
    {
        a1[i] = a2[i];
    }
    return;
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

int m_socket(int domain, int type, int protocol)
{
    if(type != SOCK_MTP)
    {
        return ERR;
    }

    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtp_socket *MTP_Table = create_shared_MTP_Table();
    shared_variables *shared_resource = create_shared_variables();

    /* Entry Semaphore creation */
    int entry_sem;
    create_entry_semaphore(&entry_sem);

    /* Exit Semaphore creation */
    int exit_sem;
    create_exit_semaphore(&exit_sem);

    // int mutex;
    // create_mutex(&mutex);

    for (int i = 0; i < SIZE_SM; i++)
    {
        if(MTP_Table[i].free == 1)
        {
            // down(mutex);
            int user_mtp_id = i;
            MTP_Table[i].free = 0;
            MTP_Table[i].pid = getppid();
            
            shared_resource->status = 0;
            shared_resource->mtp_id = user_mtp_id;

            up(entry_sem);
            down(exit_sem);
            printf("678\n");

            if(shared_resource->error_no!=0)
            {
                printf("Error !!!\n");
                errno = shared_resource->error_no;
                MTP_Table[i].free = 1;
                // up(mutex);
                return ERR;
            }
            
            shmdt(MTP_Table);
            shmdt(shared_resource);

            // up(mutex);
            return user_mtp_id;
            
        }
    }
    errno = ENOBUFS; 
    printf("hhhhhhhhhhh\n");
    // up(mutex);   
    return ERR;    
}

int m_close(int socket_id)
{
    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtp_socket *MTP_Table = create_shared_MTP_Table();
    shared_variables *shared_resource = create_shared_variables();

    /* Entry Semaphore creation */
    int entry_sem;
    create_entry_semaphore(&entry_sem);

    /* Exit Semaphore creation */
    int exit_sem;
    create_exit_semaphore(&exit_sem);

    // int mutex;
    // create_mutex(&mutex);

    // down(mutex);
    if(MTP_Table[socket_id].free == 0)
    {
        MTP_Table[socket_id].free = 1;
        shared_resource->mtp_id = socket_id;
        
        shared_resource->status = 2;

        up(entry_sem);
        down(exit_sem);

        if(shared_resource->error_no!=0)
        {
            errno = shared_resource->error_no;
            MTP_Table[socket_id].free = 0;
            // up(mutex);
            return ERR;
        }

        int retval = shared_resource->return_value;

        shmdt(MTP_Table);
        shmdt(shared_resource);

        // up(mutex);
        return retval;
    }
    // up(mutex);
    return ERR;
}

int m_bind(int socket_id, char *src_ip, unsigned short int src_port, char *dest_ip, unsigned short int dest_port)
{
    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtp_socket *MTP_Table = create_shared_MTP_Table();
    shared_variables *shared_resource = create_shared_variables();

    /* Entry Semaphore creation */
    int entry_sem;
    create_entry_semaphore(&entry_sem);

    /* Exit Semaphore creation */
    int exit_sem;
    create_exit_semaphore(&exit_sem);

    // int mutex;
    // create_mutex(&mutex);

    // down(mutex);
    if(MTP_Table[socket_id].free == 0)
    {
        shared_resource->status = 1;
        shared_resource->mtp_id = socket_id;

        strcpy(MTP_Table[socket_id].dest_ip, dest_ip);
        MTP_Table[socket_id].dest_port = dest_port;

        shared_resource->src_addr.sin_addr.s_addr = inet_addr(src_ip);
        shared_resource->src_addr.sin_port = htons(src_port);
        shared_resource->src_addr.sin_family = AF_INET;


        up(entry_sem);
        down(exit_sem);

        if(shared_resource->error_no!=0)
        {
            errno = shared_resource->error_no;
            MTP_Table[socket_id].dest_ip[0] = '\0';
            MTP_Table[socket_id].dest_port = 0;
            // up(mutex);
            return ERR;
        }

        int retval = shared_resource->return_value;

        shmdt(MTP_Table);
        shmdt(shared_resource);

        // up(mutex);
        return retval;
    }
    // up(mutex);
    return ERR;
}


int m_sendto(int socket_id, char *buffer, int size, int flags, struct sockaddr *dest, int len)
{
    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtp_socket *MTP_Table = create_shared_MTP_Table();
    shared_variables *shared_resource = create_shared_variables();

    int mtx_sendbuf;
    create_mutex_sendbuf(&mtx_sendbuf);

    int mtx_swnd;
    create_mutex_swnd(&mtx_swnd);

    struct sockaddr_in *dest_in = (struct sockaddr_in *)dest;
    char *given_dest_ip = inet_ntoa(dest_in->sin_addr);
    unsigned short given_dest_port = ntohs(dest_in->sin_port);

    // if(strcmp(MTP_Table[socket_id].dest_ip, given_dest_ip) || (MTP_Table[socket_id].dest_port != given_dest_port))
    // {
    //     errno = ENOBOUND;
    //     return ERR;
    // }
    down(mtx_swnd);
    down(mtx_sendbuf);
    printf("$$$$$$$$$$$$$$$$ new_entry: %d\n", MTP_Table[socket_id].swnd.new_entry);
    if(MTP_Table[socket_id].send_buff[MTP_Table[socket_id].swnd.new_entry].sequence_no != -1)
    {
        errno = ENOBUFS;
        up(mtx_sendbuf);
        up(mtx_swnd);
        shmdt(MTP_Table);
        shmdt(shared_resource);

        return ERR;
    }

    int idx = MTP_Table[socket_id].swnd.new_entry;
    my_strcpy(MTP_Table[socket_id].send_buff[idx].data, buffer, KB);
    MTP_Table[socket_id].send_buff[idx].sequence_no = (MTP_Table[socket_id].swnd.last_seq_no)%MAX_SEQ_NO + 1;
    MTP_Table[socket_id].swnd.last_seq_no = MTP_Table[socket_id].send_buff[idx].sequence_no;
    MTP_Table[socket_id].swnd.new_entry = (MTP_Table[socket_id].swnd.new_entry+1)%SEND_BUFFSIZE;
    printf("$$$$$$$$$$$$$$$$ new_entry: %d\n", MTP_Table[socket_id].swnd.new_entry);

    shmdt(MTP_Table);
    shmdt(shared_resource);

    up(mtx_sendbuf);
    up(mtx_swnd);
}

int m_recvfrom(int socket_id, char *buffer, int size, int flags, struct sockaddr *dest, int *len)
{
    // populating sembuf appropiately for P(s) and V(s)
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1; 
    vop.sem_op = 1;

    mtp_socket *MTP_Table = create_shared_MTP_Table();
    shared_variables *shared_resource = create_shared_variables();

    int mtx_recvbuf;
    create_mutex_recvbuf(&mtx_recvbuf);

    down(mtx_recvbuf);
    int min_seqno = (MTP_Table[socket_id].rwnd.last_inorder_received)%MAX_SEQ_NO+1;
    
    for (int i = 0; i < RECV_BUFFSIZE; i++)
    {
        if(MTP_Table[socket_id].recv_buff[i].sequence_no == -1)
        {
            printf("recvbuff -- %d\n", i);
            continue;
        }
        min_seqno = min_logical(min_seqno, MTP_Table[socket_id].recv_buff[i].sequence_no, MAX_SEQ_NO);
    }
    if(min_seqno<=MTP_Table[socket_id].rwnd.last_inorder_received)
    {
        for (int i = 0; i < RECV_BUFFSIZE; i++)
        {
            if(MTP_Table[socket_id].recv_buff[i].sequence_no == min_seqno)
            {
                my_strcpy(buffer, MTP_Table[socket_id].recv_buff[i].data, KB);
                MTP_Table[socket_id].recv_buff[i].sequence_no=-1;
                printf("@@@@@@@@@@@@@\n");
                if (MTP_Table[socket_id].rwnd.nospace==1)
                {
                    printf("user %d : is space \n", socket_id);
                    // MTP_Table[socket_id].rwnd.nospace=0;
                }
                shmdt(MTP_Table);
                shmdt(shared_resource);
                up(mtx_recvbuf);
                return KB;
            }
        }
    }
    errno = NOMSG;
    up(mtx_recvbuf);
    shmdt(MTP_Table);
    shmdt(shared_resource);
    return ERR;

}


void printTable()
{
    mtp_socket *MTP_Table = create_shared_MTP_Table();
    printf("-----------------------------------------\n");
    printf("MTP_ID\tpid\tfree\tudp_sockid\n");
    for(int i=0; i<SIZE_SM; i++)
    {
        printf("%d\t%d\t%d\t%d\n", i, MTP_Table[i].pid, MTP_Table[i].free, MTP_Table[i].udp_sockid);
    }
    printf("-----------------------------------------\n");

}


// int main()
// {
    
//     /* Shared Memory creation */
//     mtp_socket *MTP_Table = create_shared_MTP_Table();

//     // for(int i=0; i<25; i++)
//     // {
//     //     m_socket(AF_INET, SOCK_MTP, 0);
//     // }

//     // for(int i=0; i<25; i++)
//     // {
//     //     printf("%d \n", MTP_Table[i].udp_sockid);
//     // }
//     // printTable();
//     int id1 = m_socket(AF_INET, SOCK_MTP, 0);
//     printf("%d %d\n", id1, MTP_Table[id1].udp_sockid);
//     // printTable();

//     // m_bind(id1, "127.0.0.1", 4876, "127.0.0.2", 7511);
//     // printf("%s \n", MTP_Table[id1].dest_ip);
//     // printf("%d \n", MTP_Table[id1].dest_port);

//     // int id2 = m_socket(AF_INET, SOCK_MTP, 0);
//     // printf("%d \n", MTP_Table[id2].udp_sockid);

//     // mtp_socket MTP_Table[socket_id] = MTP_Table[0];
//     // printf("%d\n", MTP_Table[socket_id].send_buff[MTP_Table[socket_id].swnd.new_entry].sequence_no);
//     // sleep(5);

//     // int id3 = m_socket(AF_INET, SOCK_MTP, 0);
//     // printf("%d \n", MTP_Table[id3].udp_sockid);

//     // m_close(id2);

//     // sleep(3);

//     // int id4 = m_socket(AF_INET, SOCK_MTP, 0);
//     // printf("%d \n", MTP_Table[id4].udp_sockid);

//     // sleep(3);

//     // m_close(id1);

//     // int id5 = m_socket(AF_INET, SOCK_MTP, 0);
//     // printf("%d \n", MTP_Table[id5].udp_sockid);

//     // sleep(3);

//     // int id6 = m_socket(AF_INET, SOCK_MTP, 0);
//     // printf("%d \n", MTP_Table[id6].udp_sockid);


//     shmdt(MTP_Table);
// }