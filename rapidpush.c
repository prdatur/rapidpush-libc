/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <abort@digitalise.net> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return J. Dijkstra (04/29/2010)
 * ----------------------------------------------------------------------------
 *  Modified by Adriano Maia (adriano@usk.bz)
 *  Modified by C. Ackermann
 */
#include "rapidpush.h"
#include "cJSON.h"

static int rapidpush_get_response_code(char* response);
static rapidpush_connection* rapidpush_ssl_connect();
static SOCKET rapidpush_tcp_connect();
static char* rapidpush_ssl_read(rapidpush_connection* c);
static void rapidpush_ssl_disconnect(rapidpush_connection* c);
static char rapidpush_int_to_hex(char code);
static char* rapidpush_url_encode(char* str);

char* rapidpush_notify(char* api_key, char* title, char* message, int priority, char* category, char* group, char* schedule_at)
{
	rapidpush_connection* c;
	char buffer[MESSAGESIZE];
	char* response = NULL;
	cJSON *root;
	char *out;
#ifdef _WINDOWS
	static int wsa_init = 0;
#endif
	
	title = rapidpush_url_encode(title);
	message = rapidpush_url_encode(message);
	category = rapidpush_url_encode(category);
	group = rapidpush_url_encode(group);
	schedule_at = rapidpush_url_encode(schedule_at);

	
	
	root=cJSON_CreateObject();	
	cJSON_AddItemToObject(root, "title", cJSON_CreateString(title));
	cJSON_AddItemToObject(root, "message", cJSON_CreateString(message));
	cJSON_AddItemToObject(root, "priority", cJSON_CreateNumber(priority));
	cJSON_AddItemToObject(root, "category", cJSON_CreateString(category));
	cJSON_AddItemToObject(root, "group", cJSON_CreateString(group));
	cJSON_AddItemToObject(root, "schedule_at", cJSON_CreateString(schedule_at));
	
	out=cJSON_Print(root);

#ifdef _WINDOWS
	if (wsa_init == 0)
	{
		WSAData wsad;

		if (WSAStartup(MAKEWORD(2, 2), &wsad) != 0)
		{
			fprintf(stderr, "Failed to initialize winsock (%d)\n", GetLastError());
			goto end;
		}

#ifdef _DEBUG
		printf("RapidPush [debug]: Initialized Winsock\n");
#endif

		wsa_init = 1;
	}
#endif

	if ((c = rapidpush_ssl_connect()) == NULL) goto end;

#ifdef _DEBUG
	printf("RapidPush [debug]: Connected\n");
#endif

	
	
	
	sprintf(buffer, "GET /api?apikey=%s&command=notify&data=%s\r\nHost: %s\r\n\r\n", 
			api_key, 
			rapidpush_url_encode(out), 
			HOSTNAME);
	
	if (SSL_write(c->ssl_handle, buffer, strlen(buffer)) <= 0)
	{
		fprintf(stderr, "Failed to write buffer to SSL connection\n");
		ERR_print_errors_fp(stderr);
		goto end;
	}

#ifdef _DEBUG
	printf("RapidPush [debug]: Written buffer: %s\n", buffer);
#endif

	response = rapidpush_ssl_read(c);

#ifdef _DEBUG
	OutputDebugString("RapidPush [debug]: server response:");
	if (response != NULL) printf("%s\n", response);
#endif

	rapidpush_ssl_disconnect(c);
	
end:
	free(title);
	free(message);
	free(category);
	free(group);
	free(schedule_at);
	
	return response;
}

static rapidpush_connection* rapidpush_ssl_connect()
{
	rapidpush_connection* c = (rapidpush_connection*)malloc(sizeof(rapidpush_connection));

	c->socket = rapidpush_tcp_connect();
	if (c->socket != SOCKET_ERROR)
	{
		SSL_library_init();
		SSL_load_error_strings();

		c->ssl_context = SSL_CTX_new(SSLv23_client_method());
		if (c->ssl_context == NULL) ERR_print_errors_fp(stderr);

		c->ssl_handle = SSL_new(c->ssl_context);
		if (c->ssl_handle == NULL) ERR_print_errors_fp(stderr);

		SSL_CTX_set_verify(c->ssl_context, SSL_VERIFY_PEER, SSL_VERIFY_NONE);

		if (!SSL_set_fd(c->ssl_handle, c->socket)) ERR_print_errors_fp(stderr);

		if (SSL_connect(c->ssl_handle) != 1) ERR_print_errors_fp(stderr);

#ifdef _DEBUG
		printf("RapidPush [debug]: SSL Handshake successful\n");
#endif
	}
	else
	{
#ifdef _WINDOWS
		fprintf(stderr, "Failed to retrieve a valid connected socket\n");
		return NULL;
#else
		perror("RapidPush: Failed to retrieve connected socket");
#endif
	}

	return c;
}

static SOCKET rapidpush_tcp_connect()
{
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	struct sockaddr_in server;
	struct hostent* host = gethostbyname(HOSTNAME);

	if (s == SOCKET_ERROR)
	{
#ifdef _WINDOWS
		fprintf(stderr, "Could not create socket (%d)\n", WSAGetLastError());
		return NULL;
#else
		perror("RapidPush: Could not create socket");
#endif
	}

	if (host == NULL)
	{
#ifdef _WINDOWS		
		fprintf(stderr, "Could not retrieve host by name (%d)\n", WSAGetLastError());
		return NULL;
#else
		perror("RapidPush: Could not retrieve host by name");
#endif
	}

	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(SSL_PORT);
	server.sin_addr = *(struct in_addr*)host->h_addr;

	if (connect(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
#ifdef _WINDOWS
		fprintf(stderr, "RapidPush connect error (%d)\n", WSAGetLastError());
		return NULL;
#else
		perror("RapidPush: connect error");
#endif
	}

	return s;
}

static char* rapidpush_ssl_read(rapidpush_connection* c)
{
	int r = 1;
	char buffer[BUFFERSIZE];
	int size = BUFFERSIZE + 1;
	char* retval = (char*)malloc(size);

	memset(retval, 0, size);

	while (r > 0)
	{
		r = SSL_read(c->ssl_handle, buffer, BUFFERSIZE);
		if (r > 0)
		{
			buffer[r] = '\0';

			retval = realloc(retval, size + r);
			strcat(retval, buffer);
		}
	}

	return retval;
}

static void rapidpush_ssl_disconnect(rapidpush_connection* c)
{
	if (c->socket) closesocket(c->socket);
	if (c->ssl_handle)
	{
		SSL_shutdown(c->ssl_handle);
		SSL_free(c->ssl_handle);
	}
	if (c->ssl_context) SSL_CTX_free(c->ssl_context);

	free(c);
}

static char rapidpush_int_to_hex(char code) 
{
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

static char* rapidpush_url_encode(char* str) 
{
	char* pstr = str;
	char* buf = (char*)malloc(strlen(str) * 3 + 1);
	char* pbuf = buf;

	while (*pstr)
	{
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
		{
			*pbuf++ = *pstr;
		}
		else
		{
			*pbuf++ = '%';
			*pbuf++ = rapidpush_int_to_hex(*pstr >> 4);
			*pbuf++ = rapidpush_int_to_hex(*pstr & 15);
		}

		pstr++;
	}
	*pbuf = '\0';

	return buf;
}