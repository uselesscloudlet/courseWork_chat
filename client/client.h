#pragma once

#ifndef CLIENT_H
#define CLIENT_H

#include "../global_def.h"

void client(char* args); // обработка клиента
void* server_recv(void* data); // получение данных клиентом от сервера
void take_ip(char* args, char* ip, size_t pointer);

#endif