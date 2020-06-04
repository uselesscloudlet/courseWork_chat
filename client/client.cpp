#include "client.h"

void client()
{
    struct sockaddr_in addr;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Socket error");
        return;
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(27075);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Connect error");
        return;
    }
    else
    {
        FILE* Log;
        printf("Connected to the server.\n");
        printf("  * LOADING MESSAGE HISTORY... *\n");
        Log = fopen("messages.log", "rt");
        char str[128];
        while (true)
        {
            char* strs = fgets(str, sizeof(str), Log);
            
            if (strs == NULL)
            {
                break;
            }
            printf("  %s", str);
        }
        printf("  * MESSAGES ARE LOADED *\n");
        printf("Use /help to see all commands.\n");
        fclose(Log);

        pthread_t thread;
        pthread_create(&thread, NULL, server_recv, &sock);
        while(true)
        {
            std::string buffer;
            getline(std::cin, buffer);
            if(buffer.length() > 1023)
            {
                printf("CLIENT NOTIFICATION: You can not send a message with more than 1024 characters\n");
                printf("CLIENT NOTIFICATION: Message not sent\n");
                continue;
            }
            send(sock, buffer.c_str(), buffer.length(), 0);
            if (!strcmp(buffer.c_str(), leaveChatMsg))
            {
                close(sock);
                exit(0);
            }
        }
    }
}

void* server_recv(void* data)
{
    char buffer[1024];
    int sock = *((int*)data);
    while(true)
    {
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n == 0)
        {
            return 0;
        }
        for (int i = 0; i < n; i++)
        {
            if (buffer[i] == '\0')
            {
                printf("\n");
            }
            else
            {
                printf("%c", buffer[i]);
            }
        }
    }
}