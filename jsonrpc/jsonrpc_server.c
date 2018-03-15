//#include <czmq.h>
#include "jsonrpc.h"
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

static int method_echo(json_t *json_params, json_t **result)
{
	json_incref(json_params);
	*result = json_params;
	return 0;
}

static int method_subtract(json_t *json_params, json_t **result)
{
	size_t flags = 0;
	json_error_t error;
	double x, y;
	int rc;
	
	if (json_is_array(json_params)) {
		rc = json_unpack_ex(json_params, &error, flags, "[FF!]", &x, &y);
	} else if (json_is_object(json_params)) {
		rc = json_unpack_ex(json_params, &error, flags, "{s:F,s:F}",
			"minuend", &x, "subtrahend", &y
		);
	} else {
		//assert(0);
	}

	if (rc==-1) {
		*result = jsonrpc_error_object(JSONRPC_INVALID_PARAMS, json_string(error.text));
		return JSONRPC_INVALID_PARAMS;
	}
	
	*result = json_real(x - y);
	return 0;
}

static int method_sum(json_t *json_params, json_t **result)
{
	double total = 0;
	size_t len = json_array_size(json_params);
	int k;
	for (k=0; k < len; k++) {
		double value = json_number_value(json_array_get(json_params, k));
		total += value;
	}
	*result = json_real(total);
	return 0;
}

static struct jsonrpc_method_entry_t method_table[] = {
	{ "echo", method_echo, "o" }, 
	{ "subtract", method_subtract, "o" }, 
	{ "sum", method_sum, "[]" }, 
	{ NULL },
};

int main()
{
#if 0    
	zctx_t *ctx = zctx_new();
	void *sock = zsocket_new(ctx, ZMQ_REP);
	int port = zsocket_bind(sock, "tcp://127.0.0.1:*");
	printf("bound to port %d\n", port);
	
	while (1) {
		zmsg_t *msg = zmsg_recv(sock);
		zframe_t *frame = zmsg_first(msg);
		
		char *output = jsonrpc_handler((char *)zframe_data(frame), zframe_size(frame), method_table);
		if (output) {
			zstr_send(sock, output);
			free(output);
		} else {
			zstr_send(sock, "");
		}
		
		zmsg_destroy(&msg);
	}
	
	zctx_destroy(&ctx);
#endif

    char input[1024] = {0};
    int server_sockfd = 0, client_sockfd = 0;
    int server_len = 0, client_len = 0;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;

    unlink( "p_share_socket" );
    server_sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );
    server_address.sun_family = AF_UNIX;
    strcpy( server_address.sun_path, "p_share_socket" );
    server_len = sizeof( server_address );
    bind( server_sockfd, (struct sockaddr*)&server_address, server_len );
    listen( server_sockfd, 5 );
    
    while(1)
    {
        printf( "Photo share server is waiting...\n" );
        memset( input, 0, 1024 );

        client_len = sizeof( client_address );
        client_sockfd = accept( server_sockfd, (struct sockaddr*)&client_address, &client_len );
        printf( "A Photo share client connects to this server.\n" );
        read( client_sockfd, input, 1024 );
        printf( "Input from client=%s\n", input );
        char *output = jsonrpc_handler( input, strlen(input), method_table );
        printf( "Server generated output=%s\n", output );
        write( client_sockfd, output, strlen(output) );
        free( output );
    }
	
	return 0;
}
