#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include "Math.h"
#include <SDL.h>

typedef uint32_t ShipId;

const uint32_t kGameMaxShips = 32;
const uint32_t kGameMaxAsteroids = 64;

struct Ship
{
	ShipId id;
	bool alive;
	vec2 position;
	float rotation;
};

struct Asteroid
{
	bool alive;
	vec2 position;
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

	void AddShip();
	void RemoveShip( ShipId ship );
	void SetInput( ShipId ship, Input input );

private:
	ShipId m_currentShipId;
	Input m_input[ kGameMaxShips ];

	GameState* m_gameState;
};

class GameClient
{
public:
	void Initialize( GameState* gameState );
	void Update( float dt );

	void SetInput( SDL_Keycode key, bool down );

private:
	GameState* m_gameState;
};

#endif
