#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"
#include <algorithm>

int DISPLAY_WIDTH{ 1280 };
int DISPLAY_HEIGHT{ 720 };
int DISPLAY_SCALE{ 1 };
int INVENTORY_SPACES{ 3 };

std::string dialogue = "Press TAB to speak";
int dialogueCounter = 0;

int HEIGHT = -1;
int timer = 0;
int timer2 = 60;
int facing = 0;
int Fallen = 0;
int keysCollected = 0;

int whiteKeyCollected = 0;
int orangeKeyCollected = 0;
int greenKeyCollected = 0;


Vector2D gravity{ 0,0 };
Vector2D resistance = { -0.5f,0 };
Vector2D tileVelocity = {3,0};

int PlayerSpeed = 0;
const Vector2D PLAYER_VELOCITY = { 0,0 };
Vector2D PLAYER_JUMP = { 0,-40 };

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_FLOOR,
	TYPE_MOVINGFLOOR,
	TYPE_WHITEKEY,
	TYPE_ORANGEKEY,
	TYPE_GREENKEY,
	TYPE_DOOR,
	TYPE_INVENTORY,
	TYPE_BACKGROUND,
	TYPE_NPC,
};

enum PlayerState
{
	STATE_START,
	STATE_CONTROLS,
	STATE_WALKING,
	STATE_JUMPING,
	STATE_FALLING,
	STATE_END,
};

struct GameState
{
	int PlayerState{ STATE_START };
	int attachedTile{ -1 };
};

GameState gamestate;

bool HasCollided(Point2f pos1, Point2f pos2);
bool TileProximity(Point2f pos1, Point2f pos2); 
bool TileTooClose(Point2f pos1, Point2f pos2); 

void StartScreen();
void PlayerControls();
void EndScreen();

void NPC();
void Camera();
void Keys();
void CollectKeys();
void GameScreen();
void Platforms();
void MovingPlatforms();
void PlatformMovement();
void Grounded();
void OnMovingPlatform();

void PlayerMovement();
void PlayerOffScreen();

void IdleToWalking();
void WalkToFall();
void Falling();
void Umberela();
void PlayerJump();

void PlayerIdleDirection();
void PlayerWalkingDirection();
void PlayerJumpingDirection();
void PlayerFallingDirection();

void DrawTiles();
void DrawMovingTiles();
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
AABB aabb[4];



// The entry point for a PlayBuffer program
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();

	Platforms();
	MovingPlatforms();
	Play::CreateGameObject(TYPE_PLAYER, { DISPLAY_WIDTH / 2, 675 }, 10, "walk_right_8");
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = PLAYER_VELOCITY;

	Play::CreateGameObject(TYPE_NPC, { 1150,644 }, 10, "agent8_left_7");
	GameObject& npcObj(Play::GetGameObjectByType(TYPE_NPC));

	Play::CreateGameObject(TYPE_BACKGROUND, { DISPLAY_WIDTH / 2, -280 }, 10, "background");
	GameObject& backgroundObj(Play::GetGameObjectByType(TYPE_BACKGROUND));

	Play::CreateGameObject(TYPE_DOOR, { 1200,620 }, 10, "door");
	GameObject& doorObj(Play::GetGameObjectByType(TYPE_DOOR));

	Play::CreateGameObject(TYPE_WHITEKEY, { DISPLAY_WIDTH / 2, Play::RandomRollRange(-800,-1200) }, 10, "key_white_12"); 
	GameObject& keyWObj(Play::GetGameObjectByType(TYPE_WHITEKEY)); 

	Play::CreateGameObject(TYPE_ORANGEKEY, { Play::RandomRollRange(900,1250), Play::RandomRollRange(-800,-1200) }, 10, "key_orange_12"); 
	GameObject& keyOObj(Play::GetGameObjectByType(TYPE_ORANGEKEY)); 

	Play::CreateGameObject(TYPE_GREENKEY, { Play::RandomRollRange(30,380), Play::RandomRollRange(-800,-1200) }, 10, "key_green_12"); 
	GameObject& keyGObj(Play::GetGameObjectByType(TYPE_GREENKEY)); 

	GameObject& inventoryBox{ Play::GetGameObjectByType(TYPE_INVENTORY)};
	for (int b = 0; b < INVENTORY_SPACES ; b++)
	{
		Play::CreateGameObject(TYPE_INVENTORY, {50,260 + 100*b}, 10,"inventorybox");
	}
	 
	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
		movingFloorIdObj.velocity = tileVelocity;
	}

	Grounded();
	gamestate.PlayerState = STATE_START;
}
 
// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = playerObj.velocity + gravity;
	HEIGHT = playerObj.pos.y;
	playerObj.scale = 0.5;
	Play::DrawObjectRotated(playerObj);
	

	Play::SetCameraPosition({ 0 , 0 });

	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
		movingFloorIdObj.scale = 0.5;
		movingFloorIdObj.velocity = tileVelocity;
	}
	
	Camera();
	Draw();
	DrawTiles();
	PlayerMovement();
	PlayerOffScreen();
	PlatformMovement();
	--timer;
	--timer2;

	switch (gamestate.PlayerState)
	{
	case STATE_START:
		StartScreen();
		break;

	case STATE_CONTROLS:
		PlayerControls();
		break;

	case STATE_WALKING:
		
		DrawMovingTiles();
		PlayerWalkingDirection();
		WalkToFall();
		IdleToWalking();
		PlayerJump();
		Grounded();
		OnMovingPlatform();
		Falling();
		break;

	case STATE_JUMPING:
		DrawMovingTiles();
		PlayerJumpingDirection();
		Falling();
		break;

	case STATE_FALLING:
		
		DrawMovingTiles();
		PlayerFallingDirection();
		Umberela();
		Grounded();
		OnMovingPlatform();
		break;

	case STATE_END:
		EndScreen();
		break;
  	}

	GameScreen();
	Keys();
	CollectKeys();
	NPC();
	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void StartScreen()
{
	Play::DrawFontText("132px", "STICKMANS CLIMB", {DISPLAY_WIDTH / 2 ,200}, Play::CENTRE);
	Play::DrawFontText("105px", "Press SPACE To Start", { DISPLAY_WIDTH / 2 ,350}, Play::CENTRE);
	Play::DrawFontText("105px", "Press CTRL To see controls", { DISPLAY_WIDTH / 2 ,470}, Play::CENTRE);
	if (Play::KeyPressed(VK_SPACE))
	{
		gamestate.PlayerState = STATE_WALKING;
	}
	if (Play::KeyPressed(VK_CONTROL))
	{
		gamestate.PlayerState = STATE_CONTROLS;
	}
}

void PlayerControls()
{
	Play::DrawFontText("132px", "PLAYER CONTROLS", { DISPLAY_WIDTH / 2 ,150 }, Play::CENTRE);
	Play::DrawFontText("105px", "Use the left and right arrow keys to move", { DISPLAY_WIDTH / 2 ,230 }, Play::CENTRE);
	Play::DrawFontText("105px", "Press space to jump", { DISPLAY_WIDTH / 2 ,310 }, Play::CENTRE);
	Play::DrawFontText("105px", "When falling hold shift to use your unberela", { DISPLAY_WIDTH / 2 ,390 }, Play::CENTRE);
	Play::DrawFontText("105px", "to fall slower when needed", { DISPLAY_WIDTH / 2 ,470 }, Play::CENTRE);
	Play::DrawFontText("64px", "Press CTRL to return to the start menue", { DISPLAY_WIDTH / 2 ,550 }, Play::CENTRE);

	if (Play::KeyPressed(VK_CONTROL))
	{
		gamestate.PlayerState = STATE_START;
	}
}

void Camera()
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER)); 
	if (playerObj.pos.y <= 450) 
	{
		Play::SetCameraPosition({ 0 , playerObj.pos.y - 450 }); 
	}
	if (playerObj.pos.y <= -830) 
	{
		Play::SetCameraPosition({ 0 , -1280 }); 
	}
}

void GameScreen()
{
	if (timer2 <= -60)
	{
		timer2 = 60;
	}

	Play::SetDrawingSpace(Play::SCREEN);

		/*if (timer2 >= 0)
		{
			Play::DrawFontText("105px", "Use arrow keys to walk", { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
		}
		else if (timer2 < 0)
		{
			Play::DrawFontText("105px", "Use space to jump", { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
		}*/

		Play::DrawFontText("64px", "Keys Collected:" + std::to_string(keysCollected) + "/3", { DISPLAY_WIDTH / 2, 700 }, Play::CENTRE);

		//std::vector<int>inventoryIds{ Play::CollectGameObjectIDsByType(TYPE_INVENTORY) };
		//for (int inventoryId : inventoryIds)
		//{
		//	GameObject& inventoryIdObj{ Play::GetGameObject(inventoryId) };
		//	Play::DrawObjectRotated(inventoryIdObj);
		//}

	Play::SetDrawingSpace(Play::WORLD);
	
}

void Keys()
{

	GameObject& keyWObj(Play::GetGameObjectByType(TYPE_WHITEKEY));
	Play::DrawObjectRotated(keyWObj);
	keyWObj.scale = 2;
	keyWObj.animSpeed = 2;
	Play::UpdateGameObject(keyWObj);

	GameObject& keyOObj(Play::GetGameObjectByType(TYPE_ORANGEKEY));
	Play::DrawObjectRotated(keyOObj);
	keyOObj.scale = 2;
	keyOObj.animSpeed = 2;
	Play::UpdateGameObject(keyOObj);

	GameObject& keyGObj(Play::GetGameObjectByType(TYPE_GREENKEY));
	Play::DrawObjectRotated(keyGObj);
	keyGObj.scale = 2;
	keyGObj.animSpeed = 2;
	Play::UpdateGameObject(keyGObj);

}

void Draw()
{
	Play::ClearDrawingBuffer(Play::cBlack); 

	GameObject& backgroundObj(Play::GetGameObjectByType(TYPE_BACKGROUND)); 
	Play::DrawObjectRotated(backgroundObj); 

	GameObject& doorObj(Play::GetGameObjectByType(TYPE_DOOR)); 
	Play::DrawObjectRotated(doorObj); 
	doorObj.scale = 2; 

	GameObject& npcObj(Play::GetGameObjectByType(TYPE_NPC));
	npcObj.animSpeed = 0;
	Play::DrawObjectRotated(npcObj);
}

bool HasCollided(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 100.0f;

	Vector2f separation = pos2 - pos1;
	float dist = sqrt((separation.x * separation.x) + (separation.y * separation.y));
	if (dist < DISTANCE_TEST)
	{
		return true;
	}
	else
		return false;
}

bool TileTooClose(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 200.0f;

	Vector2f separation = pos2 - pos1;
	float dist = sqrt((separation.x * separation.x) + (separation.y * separation.y));
	if (dist < DISTANCE_TEST)
	{
		return true;
	}
	else
		return false;
}

bool TileProximity(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 1000.f;

	Vector2f separation = pos2 - pos1;
	float dist = sqrt((separation.x * separation.x) + (separation.y * separation.y));
	if (dist < DISTANCE_TEST)
	{
		return true;
	}
	else
		return false;
}

void PlayerOffScreen()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (playerObj.pos.x > 1350)
	{
		playerObj.pos.x = -25;
	}
	else if (playerObj.pos.x < -25)
	{
		playerObj.pos.x = 1350;
	}
}

void Platforms()
{
	int levelY = 700;
	int numLevels = 7;
	int widthSpacing = 50;
	int heightSpacing = 300;
	int successfullCollision = 0;

	for (int n = 0; n < numLevels; n++)
	{

		if (n == 0)
		{
			for (int m = 0; m < 26; m++)
			{
				Play::CreateGameObject(TYPE_FLOOR, { m * widthSpacing + 25, levelY }, 10, "floor");
			}
		}
		else if (n == 1)
		{
			levelY = 550;
			for (int m = 1; m < 4; m++)
			{
				Play::CreateGameObject(TYPE_FLOOR, { m * (DISPLAY_WIDTH/4), levelY}, 10, "floor");
			}
		}
		else
		{
			int numPlatforms = Play::RandomRollRange(5, 7);
			std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };

			for (int m = 2; m < numPlatforms; m++)
			{
				int successfullCollision = 0;
				int tooClose = 0;
				int randomSpacing = Play::RandomRollRange(50, 1230);

				for (int floorId : floorIds)
				{
					GameObject& floorIdObj = Play::GetGameObject(floorId);

					if (TileTooClose({ randomSpacing,levelY }, floorIdObj.pos))
					{
						++tooClose;
					}

				}
				
				if (tooClose != 0)
				{
					do
					{
						int xPosition = Play::RandomRollRange(50, 1230);
						int yPosition = Play::RandomRollRange(-2000, 600);
						successfullCollision = 0;

						std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
						for (int floorId : floorIds)
						{
							GameObject& floorIdObj = Play::GetGameObject(floorId);
							tooClose = 0;
							if (TileTooClose({ xPosition,yPosition }, floorIdObj.pos))
							{
								++tooClose;
							}
							if (TileProximity({ xPosition,yPosition }, floorIdObj.pos))
							{
								++successfullCollision;
							}

						}
					} while (successfullCollision = 0 || tooClose != 0);

				}
				else
				{
					Play::CreateGameObject(TYPE_FLOOR, { randomSpacing, levelY }, 10, "floor");
				}
			}
		}
        
		levelY -= heightSpacing;
	}
}

void MovingPlatforms()
{
	int levelY = 400;
	int numLevels = 6;
	int heightSpacing = 300;
	int successfullCollision = 0;

	for (int n = 0; n < numLevels; n++)
	{
		int numPlatforms = Play::RandomRollRange(3, 5);

		for (int m = 1; m < numPlatforms; m++)
		{
			int randomSpacing = DISPLAY_WIDTH / numPlatforms;
			Play::CreateGameObject(TYPE_MOVINGFLOOR, { m * randomSpacing, levelY }, 10, "movingfloor");

		}

		levelY -= heightSpacing;
	}
}

void DrawMovingTiles()
{
	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
		movingFloorIdObj.scale = 0.5;

		Play::UpdateGameObject(movingFloorIdObj);
		Play::DrawObjectRotated(movingFloorIdObj);
	}
}

void PlatformMovement()
{
	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);

		if (timer == 0)
		{
			timer = 40;
			tileVelocity *= -1;
		}

		Play::UpdateGameObject(movingFloorIdObj);

	}
}

void DrawTiles()
{

	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj = Play::GetGameObject(floorId);
		floorIdObj.scale = 0.5;
		Play::UpdateGameObject(floorIdObj);
		Play::DrawObjectRotated(floorIdObj);
	}
}

void Grounded()
{
	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj{ Play::GetGameObject(floorId) };
		aabb[TYPE_FLOOR].pos = Point2D(floorIdObj.pos);
		aabb[TYPE_FLOOR].size = Vector2D(25.f, 25.f);

		GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
		aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos);
		aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);

		bool left = (aabb[TYPE_FLOOR].Left().x < aabb[TYPE_PLAYER].Right().x);
		bool right = (aabb[TYPE_FLOOR].Right().x > aabb[TYPE_PLAYER].Left().x);
		bool up = (aabb[TYPE_FLOOR].Top().y < aabb[TYPE_PLAYER].Bottom().y);
		bool down = (aabb[TYPE_FLOOR].Bottom().y > aabb[TYPE_PLAYER].Bottom().y);

		if (up && down && left && right)
		{
			playerObj.velocity = { 0,0 };
			gravity = { 0,0 };
			playerObj.pos.y = floorIdObj.pos.y - 64;
			gamestate.PlayerState = STATE_WALKING;
		}
	}
}

void OnMovingPlatform()
{
	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj{ Play::GetGameObject(movingFloorId) }; 
		aabb[TYPE_MOVINGFLOOR].pos = Point2D(movingFloorIdObj.pos); 
		aabb[TYPE_MOVINGFLOOR].size = Vector2D(25.f, 25.f);

		GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) }; 
		aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos); 
		aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);

		bool left = (aabb[TYPE_MOVINGFLOOR].Left().x < aabb[TYPE_PLAYER].Right().x); 
		bool right = (aabb[TYPE_MOVINGFLOOR].Right().x > aabb[TYPE_PLAYER].Left().x); 
		bool up = (aabb[TYPE_MOVINGFLOOR].Top().y < aabb[TYPE_PLAYER].Bottom().y); 
		bool down = (aabb[TYPE_MOVINGFLOOR].Bottom().y > aabb[TYPE_PLAYER].Bottom().y); 

		if (up && down && left && right)
		{
			playerObj.velocity = { 0,0 };
			gravity = { 0,0 };

			gamestate.attachedTile = movingFloorIdObj.GetId(); 
			GameObject& attachedTileIdObj = Play::GetGameObject(gamestate.attachedTile);

			playerObj.pos.y = attachedTileIdObj.pos.y - 63;

			if (playerObj.pos.x < attachedTileIdObj.pos.x - 25)
			{
				playerObj.pos.x = attachedTileIdObj.pos.x - 25;
			}
			else  if (playerObj.pos.x > attachedTileIdObj.pos.x + 25)
			{
				playerObj.pos.x = attachedTileIdObj.pos.x + 25;
			}

			gamestate.PlayerState = STATE_WALKING; 
		}
	}
}

void WalkToFall()
{
	std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
	for (int floorId : floorIds)
	{
		GameObject& floorIdObj{ Play::GetGameObject(floorId) };
		aabb[TYPE_FLOOR].pos = Point2D(floorIdObj.pos);
		aabb[TYPE_FLOOR].size = Vector2D(25.f, 25.f);

		GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
		aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos);
		aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);

		bool left = (aabb[TYPE_FLOOR].Left().x < aabb[TYPE_PLAYER].Right().x);
		bool right = (aabb[TYPE_FLOOR].Right().x > aabb[TYPE_PLAYER].Left().x);
		bool up = (aabb[TYPE_FLOOR].Top().y < aabb[TYPE_PLAYER].Bottom().y);
		bool down = (aabb[TYPE_FLOOR].Bottom().y > aabb[TYPE_PLAYER].Bottom().y);

		if (!left || !right)
		{
			gravity = { 0,1 };
		}
	}
}

void PlayerMovement()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };

	if (Play::KeyDown(VK_RIGHT))
	{
		facing = 0;
		playerObj.pos.x += 5;
	}
	if (Play::KeyDown(VK_LEFT))
	{
		facing = 1;
		playerObj.pos.x -= 5;
	}
	Play::UpdateGameObject(playerObj);

	Play::DrawObjectRotated(playerObj);
	playerObj.scale = 0.5;
}

void IdleToWalking()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (Play::KeyPressed(VK_SPACE))
	{
		gamestate.PlayerState = STATE_JUMPING;
	}
	else
	{
		gamestate.PlayerState = STATE_WALKING;
	}
	Play::UpdateGameObject(playerObj);
}

void PlayerJump()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (Play::KeyPressed(VK_SPACE))
	{
		playerObj.velocity = { 0,-20 };
		gravity = { 0 , 1.f };
	}
	Play::UpdateGameObject(playerObj);
}

void Falling()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	if (playerObj.velocity.y > 0)
	{
		playerObj.velocity = { 0,0 };
		gravity = { 0 , 1.f };
		gamestate.PlayerState = STATE_FALLING;
	}
}

void Umberela()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
		
		if (Play::KeyDown(VK_SHIFT))
		{
			gravity = { 0, 1.f };
			playerObj.velocity = { 0,0 };

			if (facing == 0)
			{
				Play::SetSprite(playerObj, "slow_falling_right", 1.0f);
			}
			if (facing == 1)
			{
				Play::SetSprite(playerObj, "slow_falling_left", 1.0f);
			}
			Play::UpdateGameObject(playerObj);
		}
		else
		{
			gravity = { 0,1.f };
			if (facing == 0)
			{
				Play::SetSprite(playerObj, "falling_right", 1.0f);
			}
			if (facing == 1)
			{
				Play::SetSprite(playerObj, "falling_left", 1.0f);
			}
		}
}

void PlayerIdleDirection()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
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
	if (facing == 0)
	{
		Play::SetSprite(playerObj, "walk_right_8", 1.0f);
		playerObj.animSpeed = 0.1;
	}
	if (facing == 1)
	{
		Play::SetSprite(playerObj, "walk_left_8", 1.0f);
		playerObj.animSpeed = 0.1;
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

void CollectKeys()
{
	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };

	GameObject& keyWObj(Play::GetGameObjectByType(TYPE_WHITEKEY));
	GameObject& keyOObj(Play::GetGameObjectByType(TYPE_ORANGEKEY));
	GameObject& keyGObj(Play::GetGameObjectByType(TYPE_GREENKEY));

		if (HasCollided(keyWObj.pos, playerObj.pos) && !whiteKeyCollected)
		{
			Play::DestroyGameObjectsByType(TYPE_WHITEKEY);
			whiteKeyCollected = 1;
			++keysCollected;
		}

		if (HasCollided(keyOObj.pos, playerObj.pos) && !orangeKeyCollected)
		{
			Play::DestroyGameObjectsByType(TYPE_ORANGEKEY);
			orangeKeyCollected = 1;
			++keysCollected;
		}

		if (HasCollided(keyGObj.pos, playerObj.pos) && !greenKeyCollected)
		{
			Play::DestroyGameObjectsByType(TYPE_GREENKEY);
			greenKeyCollected = 1;
			++keysCollected;
		}
}

void NPC()
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	GameObject& npcObj(Play::GetGameObjectByType(TYPE_NPC));

	if ((HasCollided(playerObj.pos, npcObj.pos)) && (keysCollected <= 2))
	{

		if (Play::KeyPressed(VK_TAB) && dialogueCounter == 0)
		{
			++dialogueCounter;
			dialogue = "Hello Stick Man";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter ==1)
		{
			++dialogueCounter;
			dialogue = "I need your help!";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 2)
		{
			++dialogueCounter;
			dialogue = "I lost my keys";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 3)
		{
			++dialogueCounter;
			dialogue = "Can you find them?";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 4)
		{
			++dialogueCounter;
			dialogue = "Press TAB to speak";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 5)
		{
			++dialogueCounter;
			dialogue = "Did you find them?";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 6)
		{
			++dialogueCounter;
			dialogue = "Oh no!";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 7)
		{
			++dialogueCounter;
			dialogue = "you dont have them all";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 8)
		{
			++dialogueCounter;
			dialogue = "keep looking";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 9)
		{
			++dialogueCounter;
			dialogue = "I need 3 for this door";
		}
		else if (Play::KeyPressed(VK_TAB) && dialogueCounter == 10)
		{
			dialogueCounter = 5;
			dialogue = "Press TAB to speak";
		}
	}

	else if ((HasCollided(playerObj.pos, npcObj.pos)) && (keysCollected == 3))
	{
		if (Play::KeyPressed(VK_TAB) && (dialogueCounter == 5 || dialogueCounter == 0))
		{
			++dialogueCounter;
			dialogue = "Did you find them?";
		}
		else if (Play::KeyPressed(VK_TAB) && (dialogueCounter == 6 || dialogueCounter == 1))
		{
			++dialogueCounter;
			dialogue = "WOW";
		}
		else if (Play::KeyPressed(VK_TAB) && (dialogueCounter == 7 || dialogueCounter == 2))
		{
			++dialogueCounter;
			dialogue = "YOU FOUND ALL 3!";
		}
		else if (Play::KeyPressed(VK_TAB) && (dialogueCounter == 8|| dialogueCounter == 3))
		{
			++dialogueCounter;
			dialogue = "Thank you!";
		}
		else if (Play::KeyPressed(VK_TAB) && (dialogueCounter == 9 || dialogueCounter == 4))
		{
			dialogueCounter = 50;
			dialogue = "Lets get out of here.";
		}
		else if (Play::KeyPressed(VK_TAB) && (dialogueCounter  == 50))
		{
			gamestate.PlayerState = STATE_END;
		}
	}
	else
	{
		dialogue = "Press TAB to speak";
	}
	Play::DrawFontText("64px", dialogue, { npcObj.pos.x - 50, npcObj.pos.y - 100 }, Play::CENTRE);

}

void EndScreen()
{
	Play::DrawFontText("132px", "CONGRATULATIONS", { DISPLAY_WIDTH / 2, 300 }, Play::CENTRE);
	Play::DrawFontText("105px", "You completed the game", { DISPLAY_WIDTH / 2, 400}, Play::CENTRE);
	Play::DrawFontText("105px", "Press ESC to leave the game.", { DISPLAY_WIDTH / 2, 500 }, Play::CENTRE);
}