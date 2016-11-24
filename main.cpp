#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cassert>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <SDL.h>

#include <OpenGL/gl3.h>
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

//-----------
// Render
//-----------

bool CreateProgram( GLuint* program )
{
	assert(program != nullptr);
	
	const char* vertexShaderString = "\
	#version 330\n \
	layout(location = 0) in vec4 position; \
	uniform mat4 model; \
	void main() \
	{ \
	gl_Position = model * position; \
	}";
	
	const char* pixelShaderString = "\
	#version 330\n \
	out vec4 outputColor; \
	void main() \
	{ \
	outputColor = vec4( 1.0f ); \
	} ";
	
	GLint success;
	GLchar errorsBuf[256];
	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	GLuint ps = glCreateShader(GL_FRAGMENT_SHADER);
	GLuint prgm = glCreateProgram();
	
	glShaderSource(vs, 1, &vertexShaderString, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vs, sizeof(errorsBuf), 0, errorsBuf);
		printf("Vertex Shader Errors:\n%s", errorsBuf);
		return false;
	}
	
	glShaderSource(ps, 1, &pixelShaderString, NULL);
	glCompileShader(ps);
	glGetShaderiv(ps, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(ps, sizeof(errorsBuf), 0, errorsBuf);
		printf("Pixel Shader Errors:\n%s", errorsBuf);
		return false;
	}
	
	glAttachShader(prgm, vs);
	glAttachShader(prgm, ps);
	glLinkProgram(prgm);
	glGetProgramiv(prgm, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(prgm, sizeof(errorsBuf), 0, errorsBuf);
		printf("Program Link Errors:\n%s", errorsBuf);
		return false;
	}
	
	*program = prgm;
	
	return true;
}

bool RenderInit( SDL_Window* window )
{
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_CreateContext( window );
	
	// Enable VSync
	SDL_GL_SetSwapInterval( 1 );
	
	GLuint program = -1;
	CreateProgram( &program );
	assert( program != -1 );
	glUseProgram( program );
	
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
	
	GLuint shipVB;
	glGenBuffers( 1, &shipVB );
	glBindBuffer( GL_ARRAY_BUFFER, shipVB );
	
	glEnableVertexAttribArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, shipVB );
	glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0 );
	
	return true;
}

void Render()
{
	glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );
	
	static const GLfloat shipVerts[] = {
		0.0f, 1.0f, 0.0f,
		-0.3f, 0.0f, 0.0f,
		-0.3f, 0.0f, 0.0f,
		0.0f,  0.2f, 0.0f,
		0.0f,  0.2f, 0.0f,
		0.3f,  0.0f, 0.0f,
		0.3f,  0.0f, 0.0f,
		0.0f, 1.0f, 0.0f,
	};
	
	glBufferData( GL_ARRAY_BUFFER, sizeof( shipVerts ), shipVerts, GL_STREAM_DRAW );
	
	float modelMat[ 4 ][ 4 ] = {};
	modelMat[0][0] = 0.1f;
	modelMat[1][1] = 0.1f;
	modelMat[2][2] = 0.1f;
	modelMat[3][3] = 1.0f;
	glUniformMatrix4fv( 0, 1, GL_FALSE, &modelMat[0][0] );
	
	// Draw the triangle !
	glDrawArrays( GL_LINES, 0, 8 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

//-----------
// Main
//-----------

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

		server->AddShip();
	}
	else if ( strcmp( argv[ 1 ], "client" ) == 0 )
	{
		printf( "client start\n" );

		bool offlineMode = false;
		if( argc > 2 ) { offlineMode = (strcmp( "-o", argv[ 2 ] ) == 0); }

		client = new GameClient();
		client->Initialize( &gameState );

		if( !offlineMode )
		{
			int sock = client_connect();
			char msg[] = "this is a test\n";
			send( sock, msg, strlen(msg)+1, 0 );
		}
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
	window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL );
	if( !window )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}
	SDL_SetWindowTitle( window, argv[ 1 ] );
	screenSurface = SDL_GetWindowSurface( window );
	
	RenderInit( window );
	
	bool run = true;
	while ( run )
	{
		SDL_Event e;
		while ( SDL_PollEvent( &e ) != 0 )
		{
			if ( client )
			{
				if ( e.type == SDL_KEYDOWN )
				{
					client->SetInput( e.key.keysym.sym, true );
				}
				else if ( e.type == SDL_KEYUP )
				{
					client->SetInput( e.key.keysym.sym, false );
				}
			}

			if ( e.type == SDL_QUIT )
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
		Render();
		
		SDL_GL_SwapWindow( window );
	}
	
	SDL_DestroyWindow( window );
	SDL_Quit();
	
	return 0;
}



