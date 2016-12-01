#include "Socket.h"
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int client_connect( uint16_t port )
{
	int optVal = 1;
	socklen_t optLen = sizeof(optVal);
	int result;

	addrinfo hints;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	char portStr[ 6 ];
	sprintf( portStr, "%hu", port );

	addrinfo* res;
	getaddrinfo( "localhost", portStr, &hints, &res );

	int sock = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if ( sock == -1 )
	{
		printf( "Error opening socket\n" );
		exit( -1 );
		return -1;
	}
	result = connect( sock, res->ai_addr, res->ai_addrlen );
	if ( result != 0 )
	{
		printf( "Error connecting to server: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	freeaddrinfo( res );

	int enable = 1;
	result = setsockopt( sock, SOL_SOCKET, TCP_NODELAY, &enable, sizeof(enable) );
	if ( result != 0 )
	{
		printf( "Could not set sockopt: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	result = setsockopt( sock, SOL_SOCKET, SO_NOSIGPIPE, &optVal, optLen );
	if ( result != 0 )
	{
		printf( "Could not set SO_NOSIGPIPE: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	fcntl( sock, F_SETFL, O_NONBLOCK );

	printf( "end connect\n" );

	return sock;
}

int server_create_listener( uint16_t port )
{
	int optVal = 1;
	socklen_t optLen = sizeof(optVal);
	int result;

	addrinfo hints;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	char portStr[ 6 ];
	sprintf( portStr, "%hu", port );

	addrinfo* res;
	getaddrinfo( nullptr, portStr, &hints, &res );

	int listener = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if ( listener < 0 )
	{
		printf( "Could not open listener: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	result = setsockopt( listener, SOL_SOCKET, SO_REUSEPORT, &optVal, optLen );
	if ( result != 0 )
	{
		printf( "Could not set SO_REUSEPORT: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	bind( listener, res->ai_addr, res->ai_addrlen );
	listen( listener, 16 );

	fcntl( listener, F_SETFL, O_NONBLOCK );

	freeaddrinfo( res );

	return listener;
}

int server_accept( int listener )
{
	int optVal = 1;
	socklen_t optLen = sizeof(optVal);
	int result;

	sockaddr_storage remoteAddr;
	socklen_t addrSize = sizeof(remoteAddr);
	int sock = accept( listener, (sockaddr*)&remoteAddr, &addrSize );
	if ( sock < 0 )
	{
		if ( errno != EAGAIN && errno != EWOULDBLOCK )
		{
			printf( "Could not open socket: %s\n", strerror( errno ) );
			exit( -1 );
		}
		return -1;
	}

	result = setsockopt( sock, SOL_SOCKET, TCP_NODELAY, &optVal, optLen );
	if ( result != 0 )
	{
		printf( "Could not set TCP_NODELAY: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	result = setsockopt( sock, SOL_SOCKET, SO_NOSIGPIPE, &optVal, optLen );
	if ( result != 0 )
	{
		printf( "Could not set SO_NOSIGPIPE: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	fcntl( sock, F_SETFL, O_NONBLOCK );

	return sock;
}

int socket_send( int sock, const void* data, uint32_t length )
{
	return -1;
}

int socket_recv( int sock, void* dataOut, uint32_t length )
{
	uint8_t temp[ length ];
	int recvd = recv( sock, temp, length, MSG_PEEK );
	if ( recvd == length )
	{
		recvd = recv( sock, dataOut, length, 0 );
		if ( recvd != length )
		{
			close( sock );
			return -1;
		}

		return length;
	}
	else if ( recvd == -1 && errno != EAGAIN && errno != EWOULDBLOCK )
	{
		close( sock );
		return -1;
	}
	else
	{
		return 0;
	}
}
