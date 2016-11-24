#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <SDL.h>

#include "Game.h"

#define HACK_PORT "7777"
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const uint32_t kBoardSize = 12;

int client_connect()
{
	addrinfo hints;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* res;
	getaddrinfo( "localhost", HACK_PORT, &hints, &res );

	int sock = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	if ( sock == -1 )
	{
		printf( "Error opening socket\n" );
		exit( -1 );
		return -1;
	}
	int result = connect( sock, res->ai_addr, res->ai_addrlen );
	if ( result != 0 )
	{
		printf( "Error connecting to server: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	freeaddrinfo( res );

	printf( "end connect\n" );

	return sock;
}

int server_listen()
{
	addrinfo hints;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* res;
	getaddrinfo( nullptr, HACK_PORT, &hints, &res );

	int listener = socket( res->ai_family, res->ai_socktype, res->ai_protocol );
	bind( listener, res->ai_addr, res->ai_addrlen );
	listen( listener, 16 );

	freeaddrinfo( res );

	sockaddr_storage remoteAddr;
	socklen_t addrSize = sizeof(remoteAddr);
	int sock = accept( listener, (sockaddr*)&remoteAddr, &addrSize );

	printf( "client connect!\n" );

	return sock;
}

int main( int argc, char* argv[] )
{
	GameClient* client = nullptr;
	GameServer* server = nullptr;

	GameState gameState;
	memset( &gameState, 0, sizeof(gameState) );

	if ( argc < 2 )
	{
		printf( "specify server/client\n" );
		return -1;
	}
	else if ( strcmp( argv[ 1 ], "server" ) == 0 )
	{
		printf( "server start\n" );

		server = new GameServer();
		server->Initialize( &gameState );

		server_listen();
	}
	else if ( strcmp( argv[ 1 ], "client" ) == 0 )
	{
		printf( "client start\n" );

		client = new GameClient();
		client->Initialize( &gameState );

		int sock = client_connect();
		char msg[] = "this is a test\n";
		send( sock, msg, strlen(msg)+1, 0 );
	}
	else
	{
		printf( "invalid parameter\n" );
		return -1;
	}

	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}

	SDL_Window* window = nullptr;
	SDL_Surface* screenSurface = nullptr;
	window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
	if( !window )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}
	SDL_SetWindowTitle( window, argv[ 1 ] );
	screenSurface = SDL_GetWindowSurface( window );

	bool run = true;
	while ( run )
	{
		SDL_Event e;
		while( SDL_PollEvent( &e ) != 0 )
		{
			if( e.type == SDL_QUIT )
			{
				run = false;
			}
		}

		float dt = 0.016666f;

		if ( server )
		{
			server->Update( dt );
		}

		if ( client )
		{
			client->Update( dt );
		}

		// Draw game state

		SDL_FillRect( screenSurface, nullptr, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) );
		SDL_UpdateWindowSurface( window );
	}
	
	SDL_DestroyWindow( window );
	SDL_Quit();

	return 0;
}
