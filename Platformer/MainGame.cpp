#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH{ 1280 };
int DISPLAY_HEIGHT{ 720 };
int DISPLAY_SCALE{ 1 };

int facing = 0;
Vector2D gravity{ 0,0 };

int PlayerSpeed = 0;
const Vector2D PLAYER_VELOCITY = { 0,0 };
Vector2D PLAYER_JUMP = { 0,-40 };

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_FLOOR,
};

enum PlayerState
{
	STATE_IDLE,
	STATE_WALKING,
	STATE_JUMPING,
	STATE_FALLING,
};

struct GameState
{
	int PlayerState{ STATE_IDLE };
};

GameState gamestate;

void Floor(int Tiles,int Height);
void TileAABB();
void PlayerMovement();
void Idle();
void WalkState();
void PlayerWalkingDirection();
void PlayerJump();
void PlayerJumpingDirection();
void PlayerFallingDirection();
void DrawTiles();
void Draw();

struct AABB
{
	Point2D pos{ 0.f, 0.f };
	Vector2D size{ 25.f, 25.f };

	Point2D Left() { return Point2D(pos.x - size.x, pos.y); }
	Point2D Right() { return Point2D(pos.x + size.x, pos.y); }
	Point2D Top() { return Point2D(pos.x, pos.y - size.y); }
	Point2D Bottom() { return Point2D(pos.x, pos.y + size.y); }

	Point2D TopLeft() { return pos - size; }
	Point2D BottomRight() { return pos + size; }
	Point2D TopRight() { return Point2D(pos.x + size.x, pos.y - size.y); }
	Point2D BottomLeft() { return Point2D(pos.x - size.x, pos.y + size.y); }
};
AABB aabb[2];

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager( DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE );
	Play::LoadBackground("Data\\Backgrounds\\background1.png");
	Play::CentreAllSpriteOrigins();
	
	Floor(20,1);
	Play::CreateGameObject(TYPE_PLAYER, { DISPLAY_WIDTH / 2, DISPLAY_HEIGHT}, 10, "walk_right_8");
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = PLAYER_VELOCITY;
	TileAABB();
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity += gravity;
	Draw();
	DrawTiles();
	PlayerMovement();
	

	switch (gamestate.PlayerState)
	{
	case STATE_IDLE:
		Idle();
		WalkState();
	break;
	  
	case STATE_WALKING:
		PlayerWalkingDirection();
		TileAABB();
	break;

	case STATE_JUMPING:
		PlayerJumpingDirection();
		PlayerJump();
	break;

	case STATE_FALLING:  
		PlayerFallingDirection();
		TileAABB();
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

void Floor(int xtiles, int ytiles)
{
	int widthspacing = 50;
	int heightspacing = 150;
	GameObject& floorObj{ Play::GetGameObjectByType(TYPE_FLOOR) };
	for (int n = 0; n < xtiles; n++)
	{
		for (int m = 0; m < ytiles; m++)
		{
			Play::CreateGameObject(TYPE_FLOOR, { n * widthspacing+10, 700 - m * heightspacing}, 10, "floor");
		}
	}
}

void DrawTiles()
{

	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj = Play::GetGameObject(floorId);
		Play::UpdateGameObject(floorIdObj);
		floorIdObj.scale = 0.5;
		Play::DrawObjectRotated(floorIdObj);


		aabb[TYPE_FLOOR].pos = Point2D(floorIdObj.pos);
		aabb[TYPE_FLOOR].size = Vector2D(25.f, 25.f);
		Play::DrawLine(aabb[TYPE_FLOOR].BottomLeft(), aabb[TYPE_FLOOR].TopLeft(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].TopRight(), aabb[TYPE_FLOOR].BottomRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].TopLeft(), aabb[TYPE_FLOOR].TopRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].BottomRight(), aabb[TYPE_FLOOR].BottomLeft(), Play::cGreen);

		GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
		aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos); 
		aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f); 
		Play::DrawLine(aabb[TYPE_PLAYER].BottomLeft(), aabb[TYPE_PLAYER].TopLeft(), Play::cGreen); 
		Play::DrawLine(aabb[TYPE_PLAYER].TopRight(), aabb[TYPE_PLAYER].BottomRight(), Play::cGreen); 
		Play::DrawLine(aabb[TYPE_PLAYER].TopLeft(), aabb[TYPE_PLAYER].TopRight(), Play::cGreen); 
		Play::DrawLine(aabb[TYPE_PLAYER].BottomRight(), aabb[TYPE_PLAYER].BottomLeft(), Play::cGreen); 

	}
}
   
void TileAABB()
{
	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj{ Play::GetGameObject(floorId)};
		aabb[TYPE_FLOOR].pos = Point2D(floorIdObj.pos);
		aabb[TYPE_FLOOR].size = Vector2D(26.f, 26.f);

		GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) }; 
		aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos);
		aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);

		bool left = (aabb[TYPE_FLOOR].Left().x < aabb[TYPE_PLAYER].Right().x);
		bool right = (aabb[TYPE_FLOOR].Right().x > aabb[TYPE_PLAYER].Left().x);
		bool up = (aabb[TYPE_FLOOR].Top().y < aabb[TYPE_PLAYER].Bottom().y);
		bool down = (aabb[TYPE_FLOOR].Bottom().y > aabb[TYPE_PLAYER].Top().y);

		if (up && left && right)
		{     
			playerObj.velocity = { 0,0 };
			gravity = { 0,0 };
			playerObj.pos.y = floorIdObj.pos.y - 66;  
			gamestate.PlayerState = STATE_IDLE;    
		}
	}
}

void WalkState()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (playerObj.velocity.y == 0)
	{
		gamestate.PlayerState = STATE_WALKING;
	}
}

void PlayerMovement()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (Play::KeyDown(VK_RIGHT))
	{
		facing = 0;
		playerObj.pos.x += 5;
		playerObj.animSpeed = 0.2;
		Play::UpdateGameObject(playerObj); 
	}
	else if (Play::KeyDown(VK_LEFT))
	{
		facing = 1;
		playerObj.pos.x -= 5;
		playerObj.animSpeed = 0.2;
		Play::UpdateGameObject(playerObj); 
	}
	else
	{
		playerObj.animSpeed = 0;
	}
	if (Play::KeyPressed(VK_SPACE))
	{
		playerObj.velocity = { 0, -15 };       
		gravity = { 0 , 0.5f };
		gamestate.PlayerState = STATE_JUMPING;
		
	}
	
	Play::DrawObjectRotated(playerObj);
 	playerObj.scale = 0.5;    
}

void Idle()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	playerObj.animSpeed = 0.1;
	playerObj.velocity = { 0,0 };
	gravity = { 0,0 };
	if (facing == 0)
	{
		Play::SetSprite(playerObj, "idle_right_7", 1.0f);
	}
	if (facing == 1)
	{
		Play::SetSprite(playerObj, "idle_left_7", 1.0f);
	}
	Play::UpdateGameObject(playerObj);
}

void PlayerWalkingDirection()
{	
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) }; 
	playerObj.animSpeed = 0.2;
	if (facing == 0) 
	{
		Play::SetSprite(playerObj, "walk_right_8", 1.0f); 
	}
	if (facing == 1)
	{
		Play::SetSprite(playerObj, "walk_left_8", 1.0f); 
	}
} 

void PlayerJumpingDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (facing == 0)
	{
		Play::SetSprite(playerObj, "jump_right", 1.0f);
	}
	if (facing == 1)
	{
		Play::SetSprite(playerObj, "jump_left", 1.0f);
	}
}

void PlayerJump()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (playerObj.velocity.y >= 0)
	{
		playerObj.velocity = { 0, 0 };
		gravity = { 0 , 0.5f };
		gamestate.PlayerState = STATE_FALLING;
	}
	Play::UpdateGameObject(playerObj);
}

void PlayerFallingDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (facing == 0)
	{
		Play::SetSprite(playerObj, "falling_right", 1.0f);
	}
	if (facing == 1)
	{
		Play::SetSprite(playerObj, "falling_left", 1.0f);
	}
	Play::UpdateGameObject(playerObj);
}