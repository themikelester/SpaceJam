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

int socket_client_connect( uint16_t port )
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

	printf( "Connected!\n" );

	return sock;
}

int socket_server_create_listener( uint16_t port )
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

int socket_server_accept( int listener )
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

int socket_send_msg( int sock, const void* data, uint32_t length )
{
	uint16_t sendLength = length + sizeof(uint16_t);
	uint8_t msg[ sendLength ];

	*(uint16_t*)msg = length;
	memcpy( msg + sizeof(uint16_t), data, length );

	int bytes = send( sock, msg, sendLength, 0 );
	if ( bytes != sendLength && errno != EAGAIN && errno != EWOULDBLOCK )
	{
		printf( "Error sending, closing socket: %s\n", strerror( errno ) );
		close( sock );
		return -1;
	}

	return length;
}

int socket_recv_msg( int sock, void* dataOut, uint32_t maxLength )
{
	uint16_t bufLength = maxLength + sizeof(uint16_t);
	uint8_t buf[ bufLength ];
	
	int recvd = recv( sock, buf, bufLength, MSG_PEEK );
	if ( recvd >= (int)sizeof(uint16_t) )
	{
		uint32_t length = *(uint16_t*)buf;
		uint32_t msgLength = length + sizeof(uint16_t);

		if ( recvd < msgLength )
		{
			return 0;
		}

		recvd = recv( sock, buf, msgLength, 0 );
		if ( recvd != msgLength )
		{
			close( sock );
			return -1;
		}

		memcpy( dataOut, buf + sizeof(uint16_t), length );
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
