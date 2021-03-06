#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <chrono>
#include <unistd.h>

#include <SDL.h>
#include <OpenGL/gl3.h>

#include "Game.h"
#include "Socket.h"

const double kFrameTime = 1.0 / 60.0;
#define DEFAULT_PORT 7777

GLint g_colorLocation = -1;

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
	uniform vec3 color; \
	void main() \
	{ \
	outputColor = vec4( color, 1.0f ); \
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

	g_colorLocation = glGetUniformLocation(	prgm, "color" );
	
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
		
		if ( ship.local )
		{
			float color[ 3 ] = { 0.0f, 1.0f, 0.0f };
			glUniform3fv( g_colorLocation, 1, color );
		}
		else
		{
			float color[ 3 ] = { 1.0f, 1.0f, 1.0f };
			glUniform3fv( g_colorLocation, 1, color );
		}

		float modelMat[ 3 ][ 3 ];
		ToMatrix( 1.0, ship.rotation, ship.position, modelMat );
		
		glUniformMatrix3fv( 1, 1, GL_FALSE, &modelMat[0][0] );
		glDrawArrays( GL_LINES, 0, 8 ); // Starting from vertex 0; 3 vertices total -> 1 triangle
	}

	// Draw asteriods
	float asteroidColor[ 3 ] = { 1.0f, 1.0f, 1.0f };
	glUniform3fv( g_colorLocation, 1, asteroidColor );
	for( uint32_t i = 0; i < kGameMaxAsteroids; i++ )
	{
		const Asteroid& asteroid = gameState.asteroids[ i ];
		if( !asteroid.alive ) { continue; }
		
		float modelMat[ 3 ][ 3 ];
		ToMatrix( asteroid.size, asteroid.rotation, asteroid.position, modelMat );
		
		glUniformMatrix3fv( 1, 1, GL_FALSE, &modelMat[0][0] );
		glDrawArrays( GL_LINES, 8, 8 );
	}

	// Draw lasers
	float laserColor[ 3 ] = { 1.0f, 1.0f, 1.0f };
	glUniform3fv( g_colorLocation, 1, laserColor );
	for( uint32_t i = 0; i < kGameMaxLasers; i++ )
	{
		const Laser& laser = gameState.lasers[ i ];
		if( !laser.alive ) { continue; }
		
		float modelMat[ 3 ][ 3 ];
		ToMatrix( 0.5f, 0.5f, laser.position, modelMat );
		
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

	bool serverMode = false;
	bool headlessMode = true;
	const char* hostname = "localhost";
	uint16_t port = DEFAULT_PORT;
	for ( uint32_t i = 1; i < argc; i++ )
	{
		if ( strcmp( "-s", argv[ i ] ) == 0 )
		{
			serverMode = true;
		}
		else if ( strcmp( "-h", argv[ i ] ) == 0 )
		{
			headlessMode = false;
		}
		else if ( strcmp( "-a", argv[ i ] ) == 0 )
		{
			if ( i + 1 >= argc )
			{
				printf( "Specify a hostname\n" );
				return -1;
			}
			hostname = argv[ i + 1 ];
		}
		else if ( strcmp( "-p", argv[ i ] ) == 0 )
		{
			if ( i + 1 >= argc )
			{
				printf( "Specify a port number\n" );
				return -1;
			}
			port = atoi( argv[ i + 1 ] );
			if ( port == 0 )
			{
				printf( "Invalid port specified\n" );
				return -1;
			}
		}
	}
	
	if ( serverMode )
	{
		printf( "server start on port %hu\n", port );

		int listener = socket_server_create_listener( port );
		server = new GameServer();
		server->Initialize( listener, &gameState );
	}
	else
	{
		printf( "client start on %s:%hu\n", hostname, port );

		int sock = socket_client_connect( hostname, port );
		client = new GameClient();
		client->Initialize( sock, &gameState );

		headlessMode = false;
	}
	
	SDL_Window* window = nullptr;
	SDL_Surface* screenSurface = nullptr;
	if ( !headlessMode )
	{
		if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
		{
			printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		}

		window = SDL_CreateWindow( "Asteroids", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kGameWidth, kGameHeight, SDL_WINDOW_OPENGL );
		if( !window )
		{
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
			return -1;
		}

		const char* name = serverMode ? "Asteroids Server" : "Asteroids";
		SDL_SetWindowTitle( window, name );
		screenSurface = SDL_GetWindowSurface( window );

		RenderInit( window );
	}
	
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

			if ( e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE )
			{
				run = false;
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
		if ( !headlessMode )
		{
			Render( gameState, window );
			SDL_GL_SwapWindow( window );
		}

		sleepTime += ( kFrameTime - dt );
		if ( sleepTime > 0.0 )
		{
			usleep( sleepTime * 1000000.0 );
		}
	}
	
	if ( !headlessMode )
	{
		SDL_DestroyWindow( window );
		SDL_Quit();
	}
	
	return 0;
}
