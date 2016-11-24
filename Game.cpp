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
}

void GameClient::Update( float dt )
{

}

void GameClient::SetInput( SDL_Keycode key, bool down )
{
	
}
