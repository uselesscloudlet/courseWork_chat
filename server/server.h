#pragma once

#ifndef SERVER_H
#define SERVER_H

#include "../global_def.h"

void server(char* args); // обработка сервера
int freeSocket(); // функция, ищущая первый свободный сокет
void* client_recv(void* data); // получение данных сервером от клиента
void sendToAll(const char* message); // функция отправки сообщения всем пользователям.
void sendToUserByName(Client* data, char* message); // функция для отправки сообщения конкретному пользователю по @
void changeUserNickname(Client* data, char* message); // функция для изменения никнейма пользователем

#endif