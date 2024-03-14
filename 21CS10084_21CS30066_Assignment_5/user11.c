#include "msocket.h"

int main()
{
    int id1 = m_socket(AF_INET, SOCK_MTP, 0);
    printf("%d\n", id1);
    // printTable();

    m_bind(id1, "127.0.0.1", 4876, "127.0.0.1", 7511);

    char buff[KB] = "afsfsdfsd";
    char buff2[KB];

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(7511);
    dest.sin_family = AF_INET;
    int len = sizeof(dest);

    for (int i = 0; i < 50; i++)
    {
        snprintf(buff, KB, "%d", i+1);
        // buff[0] = ();
        // buff[1] = ('A' + i%26);
        // buff[2] = '\0';
        while (m_sendto(id1, buff, KB, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
        {
            // sleep(1);    
        }
        printf("Sent message: %s\n", buff);
    }
    

    // while(1);
}