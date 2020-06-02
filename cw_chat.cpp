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

int freeSocket() // функция, ищущая первый свободный сокет
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_s[i] == NULL)
        {
            return i;
        }
    }
}

void* recv_msg(void* data) // функция для получения сообщений клиентом от сервера
{
    char buffer[1024];

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

void sendToAll(const char* message) // функция отправки сообщения всем пользователям.
{
    for(size_t k = 0; k < MAX_CLIENTS; ++k)
    {
        if (client_s[k] != NULL)
        {
            send(client_s[k]->socket, message, strlen(message) + 1, 0);
        }
    }
}

void sendToUserByName(Client* data, char* message) // функция для отправки сообщения конкретному пользователю по @
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
    sprintf(buffer, "[PM] %s: %s", data->nickname, message);
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

void changeUserNickname(Client* data, char* message) // функция для изменения никнейма пользователем
{
    bool isNameUsed = false;
    for (size_t i = 0; i < strlen(message); ++i)
    {
        if (message[i] == ' ')
        {
            message[i] = '\0';
        }
    }
    if (strlen(message) == 0)
    {
        char text[1024];
        sprintf(text, "SERVER NOTIFICATION: You can not use void nickname. Use another nickname");
        send(data->socket, text, strlen(text) + 1, 0);
        return;
    }
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_s[i] != NULL)
        {
            if (!strcmp(client_s[i]->nickname, message))
            {
                isNameUsed = true;
            }
        }
    }
    if (isNameUsed)
    {
        char text[1024];
        sprintf(text, "SERVER NOTIFICATION: This nickname is already taken. Use another nickname");
        send(data->socket, text, strlen(text) + 1, 0);
        return;
    }
    printf("User changed nickname: [%s]->[%s]\n", data->nickname, message);
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

void* client_recv(void* data) // получение данных сервером от клиента
{
    Client* user = (Client*)data;
    
    while(true)
    {
        char buffer[1024];
        char buffer2[1024];
        bool setName = true;
        size_t i = 0;
        memset(buffer, 0, 1024);
        memset(buffer2, 0, 1024);
        int n = recv(user->socket, buffer, sizeof(buffer), 0);
        if (n == 0)
        {
            return 0;
        }

        printf("Received message from socket: [%s]: %s\n", user->nickname, buffer);

        // Check "/exit" command
        if (!strcmp(buffer, leaveChatMsg))
        {
            char tempMsg[1024];
            printf("User [%s] disconnected.\n", user->nickname);
            sprintf(tempMsg, "SERVER NOTIFICATION: User [%s] disconnected.", user->nickname);
            sendToAll(tempMsg);
            shutdown(user->socket, SHUT_RDWR);
            client_s[user->id] = NULL;
            delete user;
            return NULL;
        }
        
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
            else if (buffer[5] != ' ')
            {
                setName = false;
                break;
            }
        }
        if (setName)
        {
            changeUserNickname(user, buffer + i + 1);
        }

        if (!strcmp(user->nickname, standardName))
        {
            char text[1024];
            sprintf(text, "SERVER NOTIFICATION: You can not chatting. Change your nickname using the command /nick");
            send(user->socket, text, strlen(text) + 1, 0);
            continue;
        }

        // Online users
        if (!strcmp(buffer, checkOnline))
        {
            const char* text = "* Online users: *";
            sendToAll(text);
            for (size_t i = 0; i < MAX_CLIENTS; ++i)
            {
                if (client_s[i] != NULL)
                {
                    char buffer3[128];
                    sprintf(buffer3, "[ %s ]", client_s[i]->nickname);
                    sendToAll(buffer3);
                }
            }
            printf("\n");
        }

        // Help message
        if (!strcmp(buffer, helpMsg))
        {
            const char* text = "[ /nick [nickname] - change your nickname. ]\n[ /online - check online users. ]\n[ /help - check all commands. ]\n[ /exit - leave chat. ]";
            send(user->socket, text, strlen(text) + 1, 0);
        }

        if(buffer[0] == '@')
        {
            sprintf(buffer2, "%s: %s", user->nickname, buffer);
            Log = fopen("messages.log", "a");
            fprintf(Log, "%s\n", buffer2);
            fclose(Log);
            sendToUserByName(user, buffer);
        }
        else
        {
            sprintf(buffer2, "%s: %s", user->nickname, buffer);
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
        pthread_create(&thread, NULL, recv_msg, NULL);
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