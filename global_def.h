#pragma once

#ifndef GLOBAL_DEF_H
#define GLOBAL_DEF_H

#define MAX_CLIENTS 100

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

const char standardName[] = "noname";
const char changeNickname[] = "/nick";
const char checkOnline[] = "/online";
const char helpMsg[] = "/help";
const char leaveChatMsg[] = "/exit";

struct Client;

#endif