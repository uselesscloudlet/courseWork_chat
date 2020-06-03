#pragma once

#ifndef CLIENT_H
#define CLIENT_H

#include "../global_def.h"

void client(); // обработка клиента
void* server_recv(void* data); // получение данных клиентом от сервера

#endif