#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{
    int sockfd = 0;
    int len = 0;
    struct sockaddr_un address;
    int result = 0;
    char json_input[1024] = {0};
    char json_output[1024] = {0};
    char* tmp = 0;
   
    //scanf( "%s", json_input );
    read( 0, json_input, 1024 );

    tmp = json_input + 5;
    
    printf( "The input Json request command is:%s\n", tmp );
    
    sockfd = socket( AF_UNIX, SOCK_STREAM, 0 );
    address.sun_family = AF_UNIX;
    strcpy( address.sun_path, "p_share_socket" );
    len = sizeof(address);

    result = connect( sockfd, (struct sockaddr*)&address, len );
    if( -1 == result )
    {
        perror( "Connect to photo share server socket failed!\n" );
        exit(1);
    }

    printf( "Connect to p_share_socket successfully!\n" );

    write( sockfd, tmp, strlen( tmp ) );
    read( sockfd, json_output, 1024 );
    printf( "The Json response is:%s\n", json_output );
    close(sockfd);
    exit(0);
    
}