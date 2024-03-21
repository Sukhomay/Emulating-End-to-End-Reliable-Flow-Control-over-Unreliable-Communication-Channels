#include "msocket.h"

#define MAX_BUFFER_SIZE 1024
#define RECEIVED_FILE_NAME "received_file.txt"

int main()
{
    int id1 = m_socket(AF_INET, SOCK_MTP, 0);
    if(id1 < 0)
    {
        perror("socket");
    }

    if(m_bind(id1, "127.0.0.1", 3001, "127.0.0.1", 2001) < 0)
    {
        perror("bind");
    }


    char buff[KB];

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(2001);
    dest.sin_family = AF_INET;
    int len = sizeof(dest);

    int file = open(RECEIVED_FILE_NAME, O_WRONLY | O_CREAT, 0666);
    if (file < 0)
    {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    // Receive file contents and write to the new file
    char buffer[KB+1];
    ssize_t bytes_received;
    socklen_t client_addr_len;
    int i = 0;
    while (1)
    {
        i++;
        while ((bytes_received = m_recvfrom(id1, buffer, KB, 0, (struct sockaddr *)&dest, &len)) < 0)
        {
            
        }
        // Write received data to file
        printf("Received message chunk: %d\n", i);

        if(buffer[0] == '#')
        {
            break;
        }
        
        buffer[bytes_received] = '\0';
        int res = write(file, buffer, strlen(buffer));
    }

    printf("File '%s' received and written to '%s'.\n", RECEIVED_FILE_NAME, RECEIVED_FILE_NAME);

    // Close file and socket
    close(file);

    int slp = 120;
    printf("Sleeping for %d seconds so that ack can go ..", slp);
    sleep(slp);

    return 0;

    // while(1);
}