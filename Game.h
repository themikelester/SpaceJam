#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include "Math.h"
#include <SDL.h>

typedef uint32_t ShipId;

const uint32_t kGameMaxShips = 32;
const uint32_t kGameMaxAsteroids = 64;
const int kGameWidth = 640;
const int kGameHeight = 480;
const float kGameScale = 50.0f; // Length of 1 unit in pixels(ish)

struct Ship
{
	ShipId id;
	bool alive;
	vec2 position;
	vec2 velocity;
	float rotation;
	float rotationVelocity;
};

struct Asteroid
{
	bool alive;
	vec2 position;
	float rotation;
	vec2 velocity;
	float size;
};

struct GameState
{
	Ship ships[ kGameMaxShips ];
	Asteroid asteroids[ kGameMaxAsteroids ];
};

struct Input
{
	int32_t accel;
	int32_t turn;
};

class GameServer
{
public:
	void Initialize( GameState* gameState );
	void Update( float dt );

	ShipId AddShip();
	void AddAsteroid();
	void RemoveShip( ShipId id );
	void SetInput( ShipId id, Input input );

private:
	ShipId m_currentShipId;
	Input m_inputs[ kGameMaxShips ];

	GameState* m_gameState;
};

class GameClient
{
public:
	void Initialize( GameState* gameState );
	void Update( float dt );

	void SetInput( SDL_Keycode key, bool down );

private:
	Input m_input;
	GameState* m_gameState;
};

#endif
