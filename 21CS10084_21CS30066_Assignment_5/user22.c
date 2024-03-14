#include "msocket.h"

int main()
{
    int id1 = m_socket(AF_INET, SOCK_MTP, 0);
    printf("%d\n", id1);
    // printTable();

    m_bind(id1, "127.0.0.1", 7511, "127.0.0.1", 4876);

    char buff[KB];
    char buff2[KB];

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(4876);
    dest.sin_family = AF_INET;
    int len = sizeof(dest);

    for (int i = 0; i < 50; i++)
    {
        while (m_recvfrom(id1, buff, KB, 0, (struct sockaddr *)&dest, &len) < 0)
        {
            // sleep(1);
        }
        printf("Received message: %s\n", buff);
    }


    // while(1);
}