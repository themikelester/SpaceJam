#include "Game.h"
#include <cstring>
#include <sys/socket.h>

const float kShipAcceleration = 10.0f;
const float kShipRotateSpeed = 4.0f;

void GameServer::Initialize( int sock, GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	m_gameState = gameState;
	m_socket = sock;
	m_currentShipId = 1;
}

void GameServer::Update( float dt )
{
	if ( m_socket != -1 )
	{
		while ( true )
		{
			Input input;
			int bytes = recv( m_socket, &input, sizeof(input), MSG_PEEK );
			if ( bytes == sizeof(Input) )
			{
				recv( m_socket, &input, sizeof(input), 0 );
				m_inputs[ 0 ] = input;
			}
			else
			{
				break;
			}
		}
	}

	for ( uint32_t i = 0; i < kGameMaxShips; i++ )
	{
		Ship* ship = &m_gameState->ships[ i ];
		Input* input = &m_inputs[ i ];

		if ( !ship->alive )
		{
			continue;
		}

		vec2 accel( sinf(ship->rotation), cosf(ship->rotation) );
		accel *= input->accel;

		ship->velocity += accel * kShipAcceleration * dt;
		ship->velocity *= 0.99f;
		if ( length( ship->velocity ) < 0.001f )
		{
			ship->velocity = vec2( 0.0f, 0.0f );
		}

		ship->rotationVelocity += kShipRotateSpeed * input->turn * dt;
		ship->rotationVelocity *= 0.98f;
		if ( fabs( ship->rotationVelocity ) < 0.001f )
		{
			ship->rotationVelocity = 0.0f;
		}

		ship->position += ship->velocity * dt;
		ship->rotation += ship->rotationVelocity * dt;
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
		
		float widthUnits = kGameWidth / kGameScale;
		float heightUnits = kGameHeight / kGameScale;
		if( asteroid->position.x < -widthUnits ) { asteroid->position.x += widthUnits * 2; }
		if( asteroid->position.x > widthUnits ) { asteroid->position.x -= widthUnits * 2; }
		if( asteroid->position.y < -heightUnits ) { asteroid->position.y += heightUnits * 2; }
		if( asteroid->position.y > heightUnits ) { asteroid->position.y -= heightUnits * 2; }
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
}

void GameClient::Update( float dt )
{
	if ( m_socket != -1 )
	{
		send( m_socket, &m_input, sizeof(m_input), 0 );
	}

	m_gameState->asteroids[ 0 ].rotation -= dt;
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
