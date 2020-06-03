#include "server/server.h"
#include "client/client.h"

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