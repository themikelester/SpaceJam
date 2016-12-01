#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include "Math.h"
#include <SDL.h>

typedef uint32_t ShipId;

const uint32_t kGameMaxShips = 32;
const uint32_t kGameMaxAsteroids = 64;
const uint32_t kGameMaxLasers = 128;
const int kGameWidth = 640;
const int kGameHeight = 480;
const float kGameScale = 50.0f; // Length of 1 unit in pixels(ish)

struct Ship
{
	void Update( double dt, float accel, float turn );

	ShipId id;
	bool alive;
	vec2 position;
	vec2 velocity;
	float rotation;
	float rotationVelocity;
	bool local;
};

struct Asteroid
{
	bool alive;
	vec2 position;
	float rotation;
	vec2 velocity;
	float size;
};

struct Laser
{
	void Update( double dt );

	bool alive;
	vec2 position;
	float rotation;
	double life;
};

struct GameState
{
	Ship ships[ kGameMaxShips ];
	Asteroid asteroids[ kGameMaxAsteroids ];
	Laser lasers[ kGameMaxLasers ];
};

struct Input
{
	int32_t accel;
	int32_t turn;
	int8_t fire;
};

struct Player
{
	int socket;
	Input input;
	double fireTimer;
};

class GameServer
{
public:
	void Initialize( int sock, GameState* gameState );
	void Update( float dt );

	void AddAsteroid();
	void AddLaser( vec2 position, float rotation );
	void SetInput( ShipId id, Input input );

private:
	int m_listener;
	ShipId m_currentShipId;
	double m_sendTimer;
	
	Player m_players[ kGameMaxShips ];
	GameState* m_gameState;
};

class GameClient
{
public:
	void Initialize( int sock, GameState* gameState );
	void Update( float dt );

	void SetInput( SDL_Keycode key, bool down );

private:
	int m_socket;
	Input m_input;
	GameState* m_gameState;
	double m_sendTimer;
};

#endif
