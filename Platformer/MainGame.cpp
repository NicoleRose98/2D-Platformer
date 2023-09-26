#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH{ 1280 };
int DISPLAY_HEIGHT{ 720 };
int DISPLAY_SCALE{ 1 };

int facing = 0;
int timer = 0;
const Vector2D gravity{ 0,5 };

int PlayerSpeed = 0;
const Vector2D PLAYER_VELOCITY = { 0,0 };
const Vector2D PLAYER_JUMP = { 0,-5 };

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_FLOOR,
};

enum PlayerState
{
	STATE_WALKING,
	STATE_JUMPING,
	STATE_FALLING,
};

struct GameState
{
	int PlayerState{ STATE_WALKING };
};

GameState gamestate;

bool HasCollided(Point2f pos1, Point2f pos2);
void Platform(int Tiles);
void PlayerOnPlatform();
void PlayerMovement();
void PlayerWalkingDirection();
void PlayerJump();
void PlayerJumpingDirection();
void PlayerFallingDirection();
void DrawTiles();
void Draw();

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::LoadBackground("Data\\Backgrounds\\background1.png");
	Play::CentreAllSpriteOrigins();

	Platform(64);
	Play::CreateGameObject(TYPE_PLAYER, { DISPLAY_WIDTH / 2, 690 }, 10, "walk_right_8");
	Play::SetSpriteOrigin("walk_right_8", 64, 370);
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = PLAYER_VELOCITY;
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.pos += playerObj.velocity;
	playerObj.animSpeed = 0.2;


	Draw();
	DrawTiles();
	PlayerMovement();

	switch (gamestate.PlayerState)
	{
	case STATE_WALKING:
		PlayerOnPlatform(); 
		PlayerWalkingDirection();
	break;

	case STATE_JUMPING:
		playerObj.velocity = PLAYER_JUMP;
		--timer;
		PlayerJump();
		PlayerJumpingDirection();
	break;

	case STATE_FALLING:
		PlayerOnPlatform();
		playerObj.velocity = gravity;
		PlayerFallingDirection();
	break;
	}

	Play::PresentDrawingBuffer();
	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{  
	Play::DestroyManager();
	return PLAY_OK;
}

void Draw()
{
	Play::ClearDrawingBuffer(Play::cGrey);
	Play::DrawBackground();
}
  
bool HasCollided(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 20;

	Vector2f separation = pos2 - pos1;
	float dist = sqrt((separation.x * separation.x) + (separation.y * separation.y));
	if (dist < DISTANCE_TEST)
	{
		return true;
	}
	else
		return false;
}

void Platform(int tiles)
{
	int spacing = 20;
	GameObject& floorObj{ Play::GetGameObjectByType(TYPE_FLOOR) };
	for (int n = 0; n < tiles; n++)
	{
		Play::CreateGameObject(TYPE_FLOOR, { n * spacing + 10, 700 }, 10, "floor");
	}
}

void DrawTiles()
{

	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj = Play::GetGameObject(floorId);
		Play::UpdateGameObject(floorIdObj);
		floorIdObj.scale = 0.2;
		Play::DrawObjectRotated(floorIdObj);
	}
}

void PlayerOnPlatform()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };

	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj = Play::GetGameObject(floorId);
		if ((HasCollided(playerObj.pos, floorIdObj.pos)) && (timer <= 0))
		{
			playerObj.velocity = PLAYER_VELOCITY;
			playerObj.pos.y = floorIdObj.pos.y - 15;
			gamestate.PlayerState = STATE_WALKING;
		}
	}
}

void PlayerMovement()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (Play::KeyDown(VK_RIGHT))
	{
		facing = 0;
		playerObj.pos.x += 3;
	}
	if (Play::KeyDown(VK_LEFT))
	{
		facing = 1;
		playerObj.pos.x -= 3;
	}
	if (Play::KeyPressed(VK_SPACE))
	{
		gamestate.PlayerState = STATE_JUMPING;
		timer = 20;
	}

	Play::DrawObjectRotated(playerObj);
 	playerObj.scale = 0.5;
}

void PlayerWalkingDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) }; 
	if (facing == 0) 
	{
		Play::SetSpriteOrigin("walk_right_8", 64, 370); 
		Play::SetSprite(playerObj, "walk_right_8", 1.0f); 
	}
	if (facing == 1)
	{ 
		Play::SetSpriteOrigin("walk_left_8", 64, 370);  
		Play::SetSprite(playerObj, "walk_left_8", 1.0f); 
	}
}

void PlayerJump()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	playerObj.scale = 0.5;

	if (timer <= 0)
	{
		gamestate.PlayerState = STATE_FALLING;
	}

}

void PlayerJumpingDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };

	if (facing == 0)
	{
		Play::SetSpriteOrigin("jump_right_3", 64, 370);
		Play::SetSprite(playerObj, "jump_right_3", 1.0f);
		
	}
	if (facing == 1)
	{
		Play::SetSpriteOrigin("jump_left_3", 64, 370);
		Play::SetSprite(playerObj, "jump_left_3", 1.0f);
	}
}

void PlayerFallingDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (facing == 0)
	{
		Play::SetSpriteOrigin("falling_right", 64, 370);
		Play::SetSprite(playerObj, "falling_right", 1.0f);
	}
	if (facing == 1)
	{
		Play::SetSpriteOrigin("falling_left", 64, 370);
		Play::SetSprite(playerObj, "falling_left", 1.0f);
	}
	Play::UpdateGameObject(playerObj);
}