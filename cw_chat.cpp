#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#define MAX_CLIENTS 100

int sock;
char standardName[] = "noname";
char changeNickname[] = "/nick";
char checkOnline[] = "/online";
char helpMsg[] = "/help";
char leaveChatMsg[] = "/exit";

FILE* Log;

struct Client
{
    char nickname[24];
    int socket;
    int id;
    pthread_t thread;
};
Client* client_s[MAX_CLIENTS] = { NULL };

int freeSocket()
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        if(client_s[i] == NULL)
        {
            return i;
        }
    }
}

void* recv_data(void* data)
{
    char buffer[1024];

    while(true)
    {
        int n = recv(sock, buffer, sizeof(buffer), 0);
        if (n == 0)
        {
            return 0;
        }
        printf("%s\n", buffer);
    }
}

void changeUserNickname(Client* data, char* message)
{
    
    printf("New name: [%s]->[%s]\n", data->nickname, message);
    for (size_t i = 0 ; i < 24; ++i)
    {
        if (message[i] == '\0' || message[i] == ' ')
        {
            data->nickname[i] = '\0';
            break;
        }
        data->nickname[i] = message[i];
    }
}

void sendToAll(const char* message)
{
    for(size_t k = 0; k < MAX_CLIENTS; ++k)
    {
        if (client_s[k] != NULL)
        {
            send(client_s[k]->socket, message, strlen(message) + 1, 0);
        }
    }
}

void sendToUserByName(Client* data, char* message)
{
    char buffer[1024];
    char nickname[24];
    size_t i;

    for (i = 1; i < strlen(message) && i < sizeof(nickname); ++i)
    {
        if (message[i] == '\0' || message[i] == ' ')
        {
            nickname[i - 1] = '\0';
            break;
        }
        nickname[i - 1] = message[i];
    }
    sprintf(buffer, "%s: %s", data->nickname, message);
    for (size_t j = 0; j < MAX_CLIENTS; ++j)
    {
        if (client_s[j] != NULL)
        {
            if (!strcmp(nickname, client_s[j]->nickname))
            {
                send(client_s[j]->socket, buffer, strlen(buffer) + 1, 0);
            }
        }
    }
}

void* client_recv(void* data)
{
    Client* Data = (Client*)data;
    
    while(true)
    {
        char buffer[1024];
        char buffer2[1024];
        memset(buffer, 0, 128);
        int n = recv(Data->socket, buffer, sizeof(buffer), 0);
        if (n == 0)
        {
            return 0;
        }
        if (!strcmp(buffer, leaveChatMsg))
        {
            printf("User [%s] disconnected.\n", Data->nickname);
            shutdown(Data->socket, SHUT_RDWR);
            client_s[Data->id] = NULL;
            delete Data;
            return NULL;
        }

        printf("Received message from socket: [%s]: %s\n", Data->nickname, buffer);
        bool setName = true;
        size_t i = 0;
        
        // Change nickname
        for (i; i < strlen(changeNickname); ++i)
        {
            if (changeNickname[i] != '\0' && buffer[i] != changeNickname[i])
            {
                setName = false;
                break;
            }
            else if(buffer[i] == '\0')
            {
                setName = false;
                break;
            }
        }
        if (setName)
        {
            changeUserNickname(Data, buffer + i + 1);
        }

        // Online users
        if (!strcmp(buffer, checkOnline))
        {
            const char* text = "* Online users: *";
            sendToAll(text);
            for (size_t j = 0; j < MAX_CLIENTS; ++j)
            {
                if (client_s[j] != NULL)
                {
                    char buffer3[128];
                    sprintf(buffer3, "[ %s ]", client_s[j]->nickname);
                    sendToAll(buffer3);
                }
            }
            printf("\n");
        }

        // Help message
        if (!strcmp(buffer, helpMsg))
        {
            const char* text = "[/nick [nickname] - change your nickname.]\n[/online - check online users.]\n[/help - check all commands.]\n[/exit - leave chat.]";
            sendToAll(text);
        }

        if(buffer[0] == '@')
        {
            sprintf(buffer2, "%s: %s", Data->nickname, buffer);
            Log = fopen("messages.log", "a");
            fprintf(Log, "%s\n", buffer2);
            fclose(Log);
            sendToUserByName(Data, buffer);
        }
        else
        {
            sprintf(buffer2, "%s: %s", Data->nickname, buffer);
            Log = fopen("messages.log", "a");
            fprintf(Log, "%s\n", buffer2);
            fclose(Log);
            sendToAll(buffer2);
        }
    }
}

void server()
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        client_s[i] = NULL;
    }
    struct sockaddr_in addr;
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("Socket error");
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(27075);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Bind error");
        return;
    }
    listen(listener, 1);

    while(true)
    {
        sock = accept(listener, NULL, NULL);
        if(sock < 0)
        {
            perror("Accept error");
        }
        else
        {
            Client* user = new Client;
            user->socket = sock;
            for (size_t i = 0; i < strlen(standardName); ++i)
            {
                user->nickname[i] = standardName[i];
            }
            int id = freeSocket();
            user->id = id;
            client_s[id] = user;
            pthread_create(&user->thread, NULL, client_recv, user);
        }
    }
}

void client()
{
    struct sockaddr_in addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
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
        pthread_create(&thread, NULL, recv_data, NULL);
        while(true)
        {
            std::string buffer;
            getline(std::cin, buffer);
            send(sock, buffer.c_str(), buffer.length(), 0);
            if (!strcmp(buffer.c_str(), leaveChatMsg))
            {
                close(sock);
                exit(0);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("The argument is specified incorrectly.\n");
        return 1;
    }
    else if (!strcmp(argv[1], "-s"))
    {
        printf("Online chat.\n");
        printf("Server.\n");
        server();
    }
    else if (!strcmp(argv[1], "-c"))
    {
        printf("Online chat.\n");
        printf("Client.\n");
        client();
    }
    else
    {
        printf("Invalid argument (use -s (server) or -c (client)).\n");
        return 1;
    }
}