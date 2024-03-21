#include "msocket.h"

#define MAX_BUFFER_SIZE 1024
#define FILE_NAME "Frankenstein.txt"
#define MAX_SEND_SIZE 1024 // 1 KB

int main()
{
    int id1 = m_socket(AF_INET, SOCK_MTP, 0);
    // printf("%d\n", id1);
    if(id1 < 0)
    {
        perror("socket");
    }

    if (m_bind(id1, "127.0.0.1", 2001, "127.0.0.1", 3001) < 0)
    {
        perror("bind");
    }

    char buffer[KB];

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(3001);
    dest.sin_family = AF_INET;
    int len = sizeof(dest);


    FILE *file = fopen(FILE_NAME, "rb");
    if (file == NULL)
    {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    // Read and send file contents in chunks of 1KB
    size_t bytes_read;
    ssize_t bytes_sent;
    int i = 0;
    while ((bytes_read = fread(buffer, sizeof(char), KB, file)) > 0)
    {
        i++;
        // Send the chunk of data
        while (m_sendto(id1, buffer, KB, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
        {
            
        }
        printf("Sent message chunk: %d\n", i);
        for(int i=0; i<KB; i++)
            buffer[i] = '\0';
    }

    buffer[0] = '#';
    while (m_sendto(id1, buffer, KB, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
    {
        
    }
    printf("Sent message chunk: %d\n", i);
    printf("Message: %s\n", buffer);

    int slp = 120;
    printf("Sleeping for %d seconds so that message can go ..", slp);
    sleep(slp);


    // Close file and socket
    fclose(file);


    printf("File '%s' sent successfully.\n", FILE_NAME);

    return 0;

    // while(1);
}