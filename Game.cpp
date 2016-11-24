#include "Game.h"
#include <cstring>

void GameServer::Initialize( GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	m_gameState = gameState;
}

void GameServer::Update( float dt )
{

}

void GameClient::Initialize( GameState* gameState )
{
	memset( this, 0, sizeof(*this) );
	m_gameState = gameState;
	
	m_gameState->ships[ 0 ].alive = true;
	m_gameState->asteroids[ 0 ].alive = true;
	m_gameState->asteroids[ 0 ].size = 1.0;
}

void GameClient::Update( float dt )
{
	m_gameState->ships[ 0 ].rotation += dt;
	m_gameState->asteroids[ 0 ].rotation -= dt;
}
