#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <chrono>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>

#include <SDL.h>

#include <OpenGL/gl3.h>
#include "Game.h"

const double kFrameTime = 1.0 / 60.0;
#define HACK_PORT "7777"

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

	fcntl( sock, F_SETFL, O_NONBLOCK );

	int enable = 1;
	result = setsockopt( sock, SOL_SOCKET, TCP_NODELAY, &enable, sizeof(enable) );
	if ( result != 0 )
	{
		printf( "Could not set sockopt: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	printf( "end connect\n" );

	return sock;
}

int server_listen()
{
	int optVal = 1;
	socklen_t optLen = sizeof(optVal);
	int result;

	addrinfo hints;
	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* res;
	getaddrinfo( nullptr, HACK_PORT, &hints, &res );

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

	freeaddrinfo( res );

	sockaddr_storage remoteAddr;
	socklen_t addrSize = sizeof(remoteAddr);
	int sock = accept( listener, (sockaddr*)&remoteAddr, &addrSize );
	if ( sock < 0 )
	{
		printf( "Could not open socket: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

	fcntl( sock, F_SETFL, O_NONBLOCK );

	result = setsockopt( sock, SOL_SOCKET, TCP_NODELAY, &optVal, optLen );
	if ( result != 0 )
	{
		printf( "Could not set TCP_NODELAY: %s\n", strerror( errno ) );
		exit( -1 );
		return -1;
	}

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
	layout(location = 0) in vec3 position; \
	uniform mat3 model; \
	uniform vec2 invFrameSize; \
	void main() \
	{ \
	vec2 pos = invFrameSize * ( model * vec3( position.xy, 1.0 ) ).xy; \
	gl_Position = vec4( pos, 0.0, 1.0 ); \
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
	
	static const GLfloat verts[] = {
		// Ship
		0.0f, 1.0f, 1.0f,
		-0.3f, 0.0f, 1.0f,
		-0.3f, 0.0f, 1.0f,
		0.0f,  0.2f, 1.0f,
		0.0f,  0.2f, 1.0f,
		0.3f,  0.0f, 1.0f,
		0.3f,  0.0f, 1.0f,
		0.0f, 1.0f, 1.0f,
		
		// Asteroid 0
		-0.5f, -0.5f, 1.0f,
		-0.5f,  0.5f, 1.0f,
		-0.5f,  0.5f, 1.0f,
		0.5f,  0.5f, 1.0f,
		0.5f,  0.5f, 1.0f,
		0.5f, -0.5f, 1.0f,
		0.5f, -0.5f, 1.0f,
		-0.5f, -0.5f, 1.0f,
	};
	
	glBufferData( GL_ARRAY_BUFFER, sizeof( verts ), verts, GL_STATIC_DRAW );
	
	return true;
}

void ToMatrix( float scale, float rotationRad, const vec2& translation, float outMatrix[3][3] )
{
	float cosA = cosf( rotationRad );
	float sinA = sinf( rotationRad );
	
	outMatrix[0][0] = cosA * scale; outMatrix[1][0] = sinA * scale; outMatrix[2][0] = translation.x;
	outMatrix[0][1] = -sinA * scale; outMatrix[1][1] = cosA * scale; outMatrix[2][1] = translation.y;
	outMatrix[0][2] = 0.0f; outMatrix[1][2] = 0.0f; outMatrix[2][2] = 1.0f;
}

void Render( const GameState& gameState, SDL_Window* window )
{
	glClearColor( 0.0, 0.0, 0.0, 1.0 );
	glClear( GL_COLOR_BUFFER_BIT );
	
	int width, height;
	SDL_GetWindowSize( window, &width, &height );
	vec2 invFrameSize( kGameScale/width, kGameScale/height );
	
	glUniform2fv( 0, 1, &invFrameSize.x );
	
	// Draw the ship
	for( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		const Ship& ship = gameState.ships[ i ];
		if( !ship.alive ) { continue; }
		
		float modelMat[ 3 ][ 3 ];
		ToMatrix( 1.0, ship.rotation, ship.position, modelMat );
		
		glUniformMatrix3fv( 1, 1, GL_FALSE, &modelMat[0][0] );
		glDrawArrays( GL_LINES, 0, 8 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	}
	
	// Draw asteriods
	for( uint32_t i = 0; i < kGameMaxAsteroids; i++ )
	{
		const Asteroid& asteroid = gameState.asteroids[ i ];
		if( !asteroid.alive ) { continue; }
		
		float modelMat[ 3 ][ 3 ];
		ToMatrix( asteroid.size, asteroid.rotation, asteroid.position, modelMat );
		
		glUniformMatrix3fv( 1, 1, GL_FALSE, &modelMat[0][0] );
		glDrawArrays( GL_LINES, 8, 8 );
	}
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

		bool offlineMode = false;
		if( argc > 2 ) { offlineMode = (strcmp( "-o", argv[ 2 ] ) == 0); }
		
		int sock = -1;
		if( !offlineMode )
		{
			sock = server_listen();
		}

		server = new GameServer();
		server->Initialize( sock, &gameState );
		
		server->AddShip();
		server->AddAsteroid();
		server->AddAsteroid();
		server->AddAsteroid();
	}
	else if ( strcmp( argv[ 1 ], "client" ) == 0 )
	{
		printf( "client start\n" );

		bool offlineMode = false;
		if( argc > 2 ) { offlineMode = (strcmp( "-o", argv[ 2 ] ) == 0); }

		int sock = -1;
		if( !offlineMode )
		{
			sock = client_connect();
		}

		client = new GameClient();
		client->Initialize( sock, &gameState );
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
	window = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kGameWidth, kGameHeight, SDL_WINDOW_OPENGL );
	if( !window )
	{
		printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		return -1;
	}
	SDL_SetWindowTitle( window, argv[ 1 ] );
	screenSurface = SDL_GetWindowSurface( window );
	
	RenderInit( window );

	std::chrono::steady_clock::time_point prevTime = std::chrono::steady_clock::now();
	double sleepTime = 0.0;

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

		std::chrono::steady_clock::time_point currentTime = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration frameTime = currentTime - prevTime;
		prevTime = currentTime;
		double dt = std::chrono::duration_cast<std::chrono::nanoseconds>( frameTime ).count() / 1000000000.0;
		
		if ( server )
		{
			server->Update( dt );
		}

		if ( client )
		{
			client->Update( dt );
		}

		// Draw game state
		Render( gameState, window );
		
		SDL_GL_SwapWindow( window );

		sleepTime += ( kFrameTime - dt );
		if ( sleepTime > 0.0 )
		{
			usleep( sleepTime * 1000000.0 );
		}
	}
	
	SDL_DestroyWindow( window );
	SDL_Quit();
	
	return 0;
}



