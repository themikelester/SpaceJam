#include "Game.h"
#include <cstring>
#include "Socket.h"

#include <cassert>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>

#include "lz4.h"

float kWidthUnits = kGameWidth / kGameScale;
float kHeightUnits = kGameHeight / kGameScale;

const double kServerSyncInterval = 0.1;
const float kShipAcceleration = 10.0f;
const float kShipRotateSpeed = 4.0f;

const float kShipFireInterval = 0.3f;
const float kLaserSpeed = 15.0f;
const float kLaserLifeTime = 3.0f;

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

void Laser::Update( double dt )
{
	if ( !alive )
	{
		return;
	}

	life -= dt;
	if ( life <= 0.0 )
	{
		alive = false;
		return;
	}
	
	vec2 direction( sinf(rotation), cosf(rotation) ); 
	position += direction * kLaserSpeed * dt;
	
	if( position.x < -kWidthUnits ) { alive = false; }
	if( position.x > kWidthUnits ) { alive = false; }
	if( position.y < -kHeightUnits ) { alive = false; }
	if( position.y > kHeightUnits ) { alive = false; }
}

void GameServer::Initialize( int listener, GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	memset( gameState, 0, sizeof(*gameState) );
	m_gameState = gameState;
	m_currentShipId = 1;
	
	m_listener = listener;
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		m_players[ i ].socket = -1;
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
		if ( m_players[ i ].socket == -1 )
		{
			int sock = socket_server_accept( m_listener );
			if ( sock >= 0 )
			{
				printf( "Client connected, adding ship!\n" );

				Player* player = &m_players[ i ];
				Ship* ship = &m_gameState->ships[ i ];
				
				memset( player, 0, sizeof(*player) );
				memset( ship, 0, sizeof(*ship) );

				player->socket = sock;
				ship->id = m_currentShipId;
				ship->alive = true;
				
				m_currentShipId++;
			}
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		while ( m_players[ i ].socket != -1 )
		{
			Input* input = &m_players[ i ].input;
			int recvd = socket_recv_msg( m_players[ i ].socket, input, sizeof(*input) );
			if ( recvd == -1 )
			{
				printf( "Error receiving, closing socket: %s\n", strerror( errno ) );
				m_players[ i ].socket = -1;
			}
			else if ( recvd == 0 )
			{
				break;
			}
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_players[ i ].socket == -1 && m_gameState->ships[ i ].alive )
		{
			printf( "Removing ship\n" );
			
			Player* player = &m_players[ i ];
			Ship* ship = &m_gameState->ships[ i ];
			
			memset( player, 0, sizeof(*player) );
			memset( ship, 0, sizeof(*ship) );

			player->socket = -1;
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		Ship* ship = &m_gameState->ships[ i ];
		ship->Update( dt, m_players[ i ].input.accel, m_players[ i ].input.turn );

		Player* player = &m_players[ i ];
		if ( player->input.fire )
		{
			player->fireTimer -= dt;
			if ( player->fireTimer < 0.0f )
			{
				player->fireTimer += kShipFireInterval;
				AddLaser( ship->position, ship->rotation );
			}
		}
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

	for ( uint32_t i = 0; i < kGameMaxLasers; i++ )
	{
		m_gameState->lasers[ i ].Update( dt );
	}

	m_sendTimer += dt;
	if ( m_sendTimer > kServerSyncInterval )
	{
		m_sendTimer -= kServerSyncInterval;
		for ( uint32_t i = 0; i < kGameMaxShips; i++ )
		{
			if ( m_players[ i ].socket != -1 )
			{
				m_gameState->ships[ i ].local = true;

				uint8_t diff[ sizeof(GameState) ];
				uint8_t* current = (uint8_t*)m_gameState;
				for ( uint32_t b = 0; b < sizeof(GameState); b++ )
				{
					diff[ b ] = current[ b ] ^ m_players[ i ].prev[ b ];
				}
				memcpy( m_players[ i ].prev, current, sizeof(GameState) );

				uint8_t compressed[ sizeof(GameState) ];
				int32_t result = LZ4_compress_default( (const char*)diff, (char*)compressed, sizeof(GameState), sizeof(compressed) );
				int bytes = socket_send_msg( m_players[ i ].socket, compressed, result );
				if ( bytes != result )
				{
					printf( "Error sending, closing socket: %s\n", strerror( errno ) );
					m_players[ i ].socket = -1;
				}

				m_gameState->ships[ i ].local = false;
			}
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

void GameServer::AddLaser( vec2 position, float rotation )
{
	Laser* laser = nullptr;
	for ( uint32_t i = 0; i < kGameMaxLasers; i++ )
	{
		if ( !m_gameState->lasers[ i ].alive )
		{
			laser = &m_gameState->lasers[ i ];
			break;
		}
	}

	if ( laser )
	{
		laser->alive = true;
		laser->position = position;
		laser->rotation = rotation;
		laser->life = kLaserLifeTime;
	}
}

void GameServer::SetInput( ShipId id, Input input )
{
	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		if ( m_gameState->ships[ i ].id == id )
		{
			memcpy( &m_players[ i ].input, &input, sizeof(input) );
			return;
		}
	}
}

void GameClient::Initialize( int sock, GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	memset( gameState, 0, sizeof(*gameState) );
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
			int bytes = socket_send_msg( m_socket, &m_input, sizeof(m_input) );
			if ( bytes < sizeof(m_input) )
			{
				printf( "Error sending, closing socket: %s\n", strerror( errno ) );
				m_socket = -1;
			}
			m_sendTimer -= kServerSyncInterval;
		}
	}

	while ( m_socket != -1 )
	{
		uint8_t compressed[ sizeof(GameState) ];
		int recvd = socket_recv_msg( m_socket, compressed, sizeof(compressed) );
		if ( recvd == -1 )
		{
			printf( "Error receiving, closing socket: %s\n", strerror( errno ) );
			m_socket = -1;
		}
		else if ( recvd == 0 )
		{
			break;
		}

		uint8_t diff[ sizeof(GameState) ];
		int result = LZ4_decompress_fast( (const char*)compressed, (char*)diff, sizeof(GameState) );
		if ( result != recvd )
		{
			printf( "Error decompressing received state\n" );
			m_socket = -1;
		}

		uint8_t* current = (uint8_t*)m_gameState;
		for ( uint32_t b = 0; b < sizeof(GameState); b++ )
		{
			current[ b ] = diff[ b ] ^ m_prev[ b ];
		}
		memcpy( m_prev, current, sizeof(GameState) );
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		float accel = 0.0f;
		float turn = 0.0f;
		if ( m_gameState->ships[ i ].local )
		{
			accel = m_input.accel;
			turn = m_input.turn;
		}
		m_gameState->ships[ i ].Update( dt, accel, turn );
	}

	for ( uint32_t i = 0; i < kGameMaxLasers; i++ )
	{
		m_gameState->lasers[ i ].Update( dt );
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
	if ( key == SDLK_SPACE )
	{
		m_input.fire = ( down ? 1 : 0 );
	}
}
