/*
 *  rapidpush.h
 *  rapidpush 
 *
 *  Created by C. Ackermann (info@rapidpush.net) on 2013-01-26. *
 */

#ifndef RAPIDPUSH_H_
#define RAPIDPUSH_H_

#define STRICT

#define SSL_PORT 443
#define HOSTNAME "rapidpush.net"
#define MESSAGESIZE 11400
#define BUFFERSIZE 512

/* priorities */
#define RAPIDPUSH_PRIORITY_DEBUG 0
#define RAPIDPUSH_PRIORITY_NOTICE 1
#define RAPIDPUSH_PRIORITY_NORMAL 2
#define RAPIDPUSH_PRIORITY_WARNING 3
#define RAPIDPUSH_PRIORITY_ALERT 4
#define RAPIDPUSH_PRIORITY_CRITICAL 5
#define RAPIDPUSH_PRIORITY_EMERGENCY 6

#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "ssleay32MT.lib")
#pragma comment(lib, "libeay32MT.lib")
#else
#define SOCKET int
#define SOCKET_ERROR -1
#define closesocket(socket) close(socket)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* openssl headers */
#include <openssl/ssl.h>
#include <openssl/err.h>

/* ssl/connection structure */
typedef struct {
    SOCKET socket;
    SSL* ssl_handle;
    SSL_CTX* ssl_context;
} rapidpush_connection;

char* rapidpush_notify(char* api_key, char* title, char* message, int priority, char* category, char* group, char* schedule_at);

#endif
