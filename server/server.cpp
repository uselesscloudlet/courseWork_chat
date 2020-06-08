#include "server.h"

Client* client_s[MAX_CLIENTS] = { NULL };

struct Client
{
    char nickname[24];
    int socket;
    int id;
    pthread_t thread;
};

void server(char* args)
{
    int port = atoi(args);
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
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Bind error");
        return;
    }
    listen(listener, 1);

    while(true)
    {
        int sock = accept(listener, NULL, NULL);
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

int freeSocket()
{
    for (size_t i = 0; i < MAX_CLIENTS; ++i)
    {
        if (client_s[i] == NULL)
        {
            return i;
        }
    }
}

void* client_recv(void* data)
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

        printf("SERVER NOTIFICATION: Received message from socket - [%s]: %s\n", user->nickname, buffer);

        // Check "/exit" command
        if (!strcmp(buffer, leaveChatMsg))
        {
            char tempMsg[1024];
            sprintf(tempMsg, "SERVER NOTIFICATION: User [%s] disconnected.", user->nickname);
            printf("%s\n", tempMsg);
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
            char text[1024] = "[ /nick [nickname] - change your nickname ]\n";
            strcat(text, "[ /online - check online users ]\n");
            strcat(text, "[ /help - check all commands ]\n");
            strcat(text, "[ /exit - leave chat ]\n");
            strcat(text, "[ @ - send private message ]");
            send(user->socket, text, strlen(text) + 1, 0);
        }

        if(buffer[0] == '@')
        {
            sprintf(buffer2, "%s: %s", user->nickname, buffer);
            sendToUserByName(user, buffer);
        }
        else
        {
            FILE* Log;
            sprintf(buffer2, "%s: %s", user->nickname, buffer);
            Log = fopen("messages.log", "a");
            fprintf(Log, "%s\n", buffer2);
            fclose(Log);
            sendToAll(buffer2);
        }
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
    bool isSended = false;
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
    char* temp = strchr(message, ' ');
    size_t pointer = temp - message + 1;
    char buf[1024];
    strcpy(buf, &message[pointer]);
    sprintf(buffer, "[PM] %s: %s", data->nickname, buf);
    for (size_t j = 0; j < MAX_CLIENTS; ++j)
    {
        if (client_s[j] != NULL)
        {
            if (!strcmp(nickname, client_s[j]->nickname))
            {
                send(client_s[j]->socket, buffer, strlen(buffer) + 1, 0);
                isSended = true;
            }
        }
    }
    if (!isSended)
    {
        char text[1024] = "SERVER NOTIFICATION: This user does not exist";
        send(data->socket, text, strlen(text) + 1, 0);
    }
}

void changeUserNickname(Client* data, char* message)
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
    if (strlen(message) > 23)
    {
        char text[1024];
        sprintf(text, "SERVER NOTIFICATION: Allowed nickname length is 24 characters");
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
    char text[1024];
    sprintf(text, "SERVER NOTIFICATION: User changed nickname - [%s]->[%s]", data->nickname, message);
    printf("%s\n", text);
    sendToAll(text);
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