#include "msocket.h"

int main()
{
    int id1 = m_socket(AF_INET, SOCK_MTP, 0);
    printf("%d\n", id1);
    // printTable();

    m_bind(id1, "127.0.0.1", 4876, "127.0.0.1", 7511);

    char buff[KB] = "1 !!";
    char buff2[KB] = "2 !!";
    char buff3[KB] = "3 !!";
    char buff4[KB] = "4 !!";
    char buff5[KB] = "5 !!";
    char buff6[KB] = "6 !!";
    char buff7[KB] = "7 !!";
    char buff8[KB] = "8 !!";
    char buff9[KB] = "9 !!";
    char buff10[KB] = "10 !!";
    char buff11[KB] = "11 !!";
    char buff12[KB] = "12 !!";

    struct sockaddr_in dest;
    dest.sin_addr.s_addr = inet_addr("127.0.0.1");
    dest.sin_port = htons(7511);
    dest.sin_family = AF_INET;
    
    int sb = m_sendto(id1, buff, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff2, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    // sleep(1);
    sb = m_sendto(id1, buff3, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff4, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    // sleep(1);
    sb = m_sendto(id1, buff5, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff6, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    // sleep(1);
    sb = m_sendto(id1, buff7, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff8, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff9, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff10, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sleep(10);
    sb = m_sendto(id1, buff11, KB, 0, (struct sockaddr *)&dest, sizeof(dest));
    sb = m_sendto(id1, buff12, KB, 0, (struct sockaddr *)&dest, sizeof(dest));

    
    
    // while(1);
}