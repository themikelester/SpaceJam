#include "Game.h"
#include <cstring>
#include "Socket.h"

#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

float kWidthUnits = kGameWidth / kGameScale;
float kHeightUnits = kGameHeight / kGameScale;

const double kServerSyncInterval = 0.1;
const float kShipAcceleration = 10.0f;
const float kShipRotateSpeed = 4.0f;

void Ship::Update( double dt, float accel, float turn )
{
	if ( !alive )
	{
		return;
	}

	vec2 dir( sinf(rotation), cosf(rotation) );

	velocity += dir * accel * kShipAcceleration * dt;
	velocity *= 0.99f;
	if ( length( velocity ) < 0.001f )
	{
		velocity = vec2( 0.0f, 0.0f );
	}

	rotationVelocity += kShipRotateSpeed * turn * dt;
	rotationVelocity *= 0.98f;
	if ( fabs( rotationVelocity ) < 0.001f )
	{
		rotationVelocity = 0.0f;
	}

	position += velocity * dt;
	rotation += rotationVelocity * dt;
	
	if( position.x < -kWidthUnits ) { position.x += kWidthUnits * 2; }
	if( position.x > kWidthUnits ) { position.x -= kWidthUnits * 2; }
	if( position.y < -kHeightUnits ) { position.y += kHeightUnits * 2; }
	if( position.y > kHeightUnits ) { position.y -= kHeightUnits * 2; }
}

void GameServer::Initialize( int listener, GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	m_gameState = gameState;
	m_currentShipId = 1;
	
	m_listener = listener;
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		m_clientSockets[ i ] = -1;
	}

	m_sendTimer = 0.0;

	AddAsteroid();
	AddAsteroid();
	AddAsteroid();
}

void GameServer::Update( float dt )
{
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_clientSockets[ i ] == -1 )
		{
			m_clientSockets[ i ] = server_accept( m_listener );
			if ( m_clientSockets[ i ] >= 0 )
			{
				printf( "Client connected, adding ship!\n" );
				AddShip();

				int bytes = send( m_clientSockets[ i ], m_gameState, sizeof(*m_gameState), 0 );
				if ( bytes < sizeof(*m_gameState) && errno != EAGAIN && errno != EWOULDBLOCK )
				{
					printf( "Error sending, closing socket: %s\n", strerror( errno ) );
					close( m_clientSockets[ i ] );
					m_clientSockets[ i ] = -1;
				}
			}
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		while ( m_clientSockets[ i ] != -1 )
		{
			Input* input = &m_inputs[ i ];
			int recvd = socket_recv( m_clientSockets[ i ], input, sizeof(*input) );
			if ( recvd == -1 )
			{
				printf( "Error receiving, closing socket: %s\n", strerror( errno ) );
				m_clientSockets[ i ] = -1;
			}
			else if ( recvd == 0 )
			{
				break;
			}
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_clientSockets[ i ] == -1 && m_gameState->ships[ i ].alive )
		{
			printf( "Removing ship\n" );
			RemoveShip( m_gameState->ships[ i ].id );
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		Ship* ship = &m_gameState->ships[ i ];
		ship->Update( dt, m_inputs[ i ].accel, m_inputs[ i ].turn );
	}
	
	for ( uint32_t i = 0; i < kGameMaxAsteroids; i++ )
	{
		Asteroid* asteroid = &m_gameState->asteroids[ i ];
		
		if ( !asteroid->alive )
		{
			continue;
		}
		
		asteroid->rotation += dt;
		asteroid->position += asteroid->velocity * dt;
		
		if( asteroid->position.x < -kWidthUnits ) { asteroid->position.x += kWidthUnits * 2; }
		if( asteroid->position.x > kWidthUnits ) { asteroid->position.x -= kWidthUnits * 2; }
		if( asteroid->position.y < -kHeightUnits ) { asteroid->position.y += kHeightUnits * 2; }
		if( asteroid->position.y > kHeightUnits ) { asteroid->position.y -= kHeightUnits * 2; }
	}

	m_sendTimer += dt;
	if ( m_sendTimer > kServerSyncInterval )
	{
		m_sendTimer -= kServerSyncInterval;
		for ( uint32_t i = 0; i < kGameMaxShips; i++ )
		{
			if ( m_clientSockets[ i ] != -1 )
			{
				int bytes = send( m_clientSockets[ i ], m_gameState, sizeof(*m_gameState), 0 );
				if ( bytes < sizeof(*m_gameState) && errno != EAGAIN && errno != EWOULDBLOCK )
				{
					printf( "Error sending, closing socket: %s\n", strerror( errno ) );
					close( m_clientSockets[ i ] );
					m_clientSockets[ i ] = -1;
				}
			}
		}
	}
}

ShipId GameServer::AddShip()
{
	Ship* ship = nullptr;
	Input* input = nullptr;
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_gameState->ships[ i ].id == 0 )
		{
			ship = &m_gameState->ships[ i ];
			input = &m_inputs[ i ];
			break;
		}
	}

	if ( ship )
	{
		memset( ship, 0, sizeof(*ship) );
		memset( input, 0, sizeof(*input) );
		ship->id = m_currentShipId;
		ship->alive = true;
		
		m_currentShipId++;

		return ship->id;
	}

	return 0;
}

void GameServer::RemoveShip( ShipId id )
{
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_gameState->ships[ i ].id == id )
		{
			Ship* ship = &m_gameState->ships[ i ];
			Input* input = &m_inputs[ i ];
			memset( ship, 0, sizeof(*ship) );
			memset( input, 0, sizeof(*input) );
			m_clientSockets[ i ] = -1;
			return;
		}
	}
}

void GameServer::AddAsteroid()
{
	Asteroid* asteroid = nullptr;
	for ( uint32_t i = 0; i < kGameMaxAsteroids; i++ )
	{
		if ( !m_gameState->asteroids[ i ].alive )
		{
			asteroid = &m_gameState->asteroids[ i ];
			break;
		}
	}
	
	if ( asteroid )
	{
		memset( asteroid, 0, sizeof(*asteroid) );
		asteroid->alive = true;
		
		asteroid->position.x = 0.5 * kGameWidth / kGameScale * ((rand() / (float)RAND_MAX) * 2.0 - 1.0);
		asteroid->position.y = 0.5 * kGameWidth / kGameScale * ((rand() / (float)RAND_MAX) * 2.0 - 1.0);
		
		asteroid->rotation = 2.0 * M_PI * (rand() / (float)RAND_MAX);
		
		const float kAsteroidSizeMin = 0.5;
		const float kAsteroidSizeRange = 3.0;
		asteroid->size = kAsteroidSizeMin + kAsteroidSizeRange * (rand() / (float)RAND_MAX);
		
		const float kAsteroidSpeedMin = 0.1;
		const float kAsteroidSpeedRange = 3.0;
		asteroid->velocity.x = kAsteroidSpeedMin + kAsteroidSpeedRange * (rand() / (float)RAND_MAX);
		asteroid->velocity.y = kAsteroidSpeedMin + kAsteroidSpeedRange * (rand() / (float)RAND_MAX);
	}
}

void GameServer::SetInput( ShipId id, Input input )
{
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_gameState->ships[ i ].id == id )
		{
			memcpy( &m_inputs[ i ], &input, sizeof(input) );
			return;
		}
	}
}

void GameClient::Initialize( int sock, GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	m_gameState = gameState;
	m_socket = sock;

	m_gameState->asteroids[ 0 ].alive = true;
	m_gameState->asteroids[ 0 ].size = 1.0;

	m_sendTimer = 0.0;
}

void GameClient::Update( float dt )
{
	if ( m_socket != -1 )
	{
		m_sendTimer += dt;
		if ( m_sendTimer > kServerSyncInterval )
		{
			send( m_socket, &m_input, sizeof(m_input), 0 );
			m_sendTimer -= kServerSyncInterval;
		}
	}

	while ( m_socket != -1 )
	{
		int recvd = socket_recv( m_socket, m_gameState, sizeof(*m_gameState) );
		if ( recvd == -1 )
		{
			printf( "Error receiving, closing socket: %s\n", strerror( errno ) );
			m_socket = -1;
		}
		else if ( recvd == 0 )
		{
			break;
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		m_gameState->ships[ i ].Update( dt, 0.0f, 0.0f );
	}
}

void GameClient::SetInput( SDL_Keycode key, bool down )
{
	if ( key == SDLK_UP )
	{
		m_input.accel = ( down ? 1 : 0 );
	}
	if ( key == SDLK_DOWN )
	{
		m_input.accel = ( down ? -1 : 0 );
	}
	if ( key == SDLK_LEFT )
	{
		m_input.turn = ( down ? -1 : 0 );
	}
	if ( key == SDLK_RIGHT )
	{
		m_input.turn = ( down ? 1 : 0 );
	}
}
