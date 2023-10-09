#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH{ 1280 };
int DISPLAY_HEIGHT{ 720 };
int DISPLAY_SCALE{ 1 };

int HEIGHT = -1;
int timer = 0;
int facing = 0;
int Fallen = 0;
int HasKey = 0;

Vector2D gravity{ 0,0 };
Vector2D resistance = { -0.5f,0 };
Vector2D tileVelocity = {3,0};

int PlayerSpeed = 0;
const Vector2D PLAYER_VELOCITY = { 0,0 };
Vector2D PLAYER_JUMP = { 0,-40 };

enum GameObjectType
{
	TYPE_FLOOR = 0,
	TYPE_MOVINGFLOOR,
	TYPE_KEY,
	TYPE_FINISHLINE,
	TYPE_PLAYER,
	TYPE_BACKGROUND,
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
	int attachedTile{ -1 };
};

GameState gamestate;

bool HasCollided(Point2f pos1, Point2f pos2);
bool TileProximity(Point2f pos1, Point2f pos2); 
bool TileTooClose(Point2f pos1, Point2f pos2); 

void Platforms();
void MovingPlatforms();
void PlatformMovement();
void Grounded();
void OnMovingPlatform();
void OnFinishline();

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
void DrawKeyAndFinishline();

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

	Play::CreateGameObject(TYPE_BACKGROUND, { DISPLAY_WIDTH / 2, -280 }, 10, "background");
	GameObject& backgroundObj(Play::GetGameObjectByType(TYPE_BACKGROUND));

	Play::CreateGameObject(TYPE_KEY, { DISPLAY_WIDTH / 2, - 1250 }, 10, "key_white_12");
	GameObject& keyObj(Play::GetGameObjectByType(TYPE_KEY));

	Play::CreateGameObject(TYPE_FINISHLINE, { DISPLAY_WIDTH / 2,-1200 }, 10, "finish");
	GameObject& finishlineObj(Play::GetGameObjectByType(TYPE_FINISHLINE));

	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
		movingFloorIdObj.velocity = tileVelocity;
	}

	Grounded();
}
 
// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = playerObj.velocity + gravity;
	HEIGHT = playerObj.pos.y;

	Play::SetCameraPosition({ 0 , 0 });

	if (playerObj.pos.y <= 450)
	{
		Play::SetCameraPosition({ 0 , playerObj.pos.y - 450 });
	}
	if (playerObj.pos.y <= -920)
	{
		Play::SetCameraPosition({ 0 , -1280 });
	}


	std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_MOVINGFLOOR) };
	for (int movingFloorId : movingFloorIds)
	{
		GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
		movingFloorIdObj.scale = 0.5;
		movingFloorIdObj.velocity = tileVelocity;
	}

	Draw();
	DrawKeyAndFinishline();
	DrawTiles();
	DrawMovingTiles();
	PlayerMovement();
	PlayerOffScreen();
	PlatformMovement();
	--timer;

	switch (gamestate.PlayerState)
	{

	case STATE_WALKING:
		PlayerWalkingDirection();
		WalkToFall();
		IdleToWalking();
		PlayerJump();
		Grounded();
		OnMovingPlatform();
		OnFinishline();
		Falling();
		break;

	case STATE_JUMPING:
		PlayerJumpingDirection();
		Falling();
		break;

	case STATE_FALLING:
		PlayerFallingDirection();
		Umberela();
		Grounded();
		OnMovingPlatform();
		OnFinishline();
		break;
	}
	
	Play::PresentDrawingBuffer();
	return Play::KeyDown(VK_ESCAPE);
}

// Gets called once when the player quits the game 
int MainGameExit(void)
{
	Play::DestroyManager();
	return PLAY_OK;
}

void Draw()
{
	Play::ClearDrawingBuffer(Play::cBlack);

	GameObject& backgroundObj(Play::GetGameObjectByType(TYPE_BACKGROUND));
	Play::DrawObjectRotated(backgroundObj);

	Play::SetDrawingSpace(Play::SCREEN);
	Play::DrawFontText("132px", std::to_string(HEIGHT), { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
	Play::SetDrawingSpace(Play::WORLD);
}

bool HasCollided(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 40.0f;

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
	int numLevels = 12;
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
	int numLevels = 11;
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

		aabb[TYPE_MOVINGFLOOR].pos = Point2D(movingFloorIdObj.pos);
		aabb[TYPE_MOVINGFLOOR].size = Vector2D(25.f, 25.f);
		Play::DrawLine(aabb[TYPE_MOVINGFLOOR].BottomLeft(), aabb[TYPE_MOVINGFLOOR].TopLeft(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_MOVINGFLOOR].TopRight(), aabb[TYPE_MOVINGFLOOR].BottomRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_MOVINGFLOOR].TopLeft(), aabb[TYPE_MOVINGFLOOR].TopRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_MOVINGFLOOR].BottomRight(), aabb[TYPE_MOVINGFLOOR].BottomLeft(), Play::cGreen);
	}
}

//Working on better random tile generation

//void Platforms()
//{
//	int levelY = 700;
//	int numPlatforms = 70;
//	int widthSpacing = 50;
//	int heightSpacing = 300;
//	int successfullCollision = 0;
//	int xPosition = 0;
//	int yPosition = 0;
//
//	for (int n = 0; n < 2; n++)
//	{
//
//		if (n == 0)
//		{
//			for (int m = 0; m < 26; m++)
//			{
//				Play::CreateGameObject(TYPE_FLOOR, { m * widthSpacing + 25, levelY }, 10, "floor");
//			}
//		}
//		else
//		{
//
//			for (int m = 2; m < numPlatforms; m++)
//			{
//				int xPosition = Play::RandomRollRange(50, 1230);
//				int yPosition = Play::RandomRollRange(-2000, 600);
//				int successfullCollision = 0;
//				int tooClose = 1;
//
//				if (tooClose != 0)
//				{
//					do
//					{
//						int xPosition = Play::RandomRollRange(50, 1230);
//						int yPosition = Play::RandomRollRange(-2000, 600);
//						successfullCollision = 0;
//
//						std::vector<int> floorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
//						for (int floorId : floorIds)
//						{
//							GameObject& floorIdObj = Play::GetGameObject(floorId);
//							tooClose = 0;
//							if (TileTooClose({ xPosition,yPosition }, floorIdObj.pos))
//							{
//								++tooClose;
//							}
//							if (TileProximity({ xPosition,yPosition }, floorIdObj.pos))
//							{
//								++successfullCollision;
//							}
//
//						}
//					} while (successfullCollision = 0 || tooClose != 0);
//
//				}
//				Play::CreateGameObject(TYPE_FLOOR, { xPosition,yPosition }, 10, "floor");
//			}
//		}
//		levelY -= heightSpacing;
//	}
//}

//void MovingPlatforms()
//{
//	int levelY = 400;
//	int numPlatforms = 20;
//	int heightSpacing = 300;
//	int successfullCollision = 0;
//
//	for (int n = 0; n < numPlatforms; n++)
//	{
//		int xPosition = Play::RandomRollRange(100,1200);
//		int yPosition = Play::RandomRollRange(-2000, 600);
//
//		std::vector<int> movingFloorIds{ Play::CollectGameObjectIDsByType(TYPE_FLOOR) };
//		for (int movingFloorId : movingFloorIds)
//		{
//			GameObject& movingFloorIdObj = Play::GetGameObject(movingFloorId);
//
//			if (TileTooClose({ xPosition,yPosition }, movingFloorIdObj.pos))
//			{
//				int xPosition = Play::RandomRollRange(100, 1200);
//				int yPosition = Play::RandomRollRange(-2000, 550);
//			}
//		}
//		Play::CreateGameObject(TYPE_MOVINGFLOOR, { xPosition, yPosition }, 10, "movingfloor");
//		levelY -= heightSpacing;
//	}
//}

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


		aabb[TYPE_FLOOR].pos = Point2D(floorIdObj.pos);
		aabb[TYPE_FLOOR].size = Vector2D(25.f, 25.f);
		Play::DrawLine(aabb[TYPE_FLOOR].BottomLeft(), aabb[TYPE_FLOOR].TopLeft(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].TopRight(), aabb[TYPE_FLOOR].BottomRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].TopLeft(), aabb[TYPE_FLOOR].TopRight(), Play::cGreen);
		Play::DrawLine(aabb[TYPE_FLOOR].BottomRight(), aabb[TYPE_FLOOR].BottomLeft(), Play::cGreen);
	}

	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos);
	aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);
	Play::DrawLine(aabb[TYPE_PLAYER].BottomLeft(), aabb[TYPE_PLAYER].TopLeft(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_PLAYER].TopRight(), aabb[TYPE_PLAYER].BottomRight(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_PLAYER].TopLeft(), aabb[TYPE_PLAYER].TopRight(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_PLAYER].BottomRight(), aabb[TYPE_PLAYER].BottomLeft(), Play::cGreen);

	GameObject& finishlineObj{ Play::GetGameObjectByType(TYPE_FINISHLINE) };
	aabb[TYPE_FINISHLINE].pos = Point2D(finishlineObj.pos);
	aabb[TYPE_FINISHLINE].size = Vector2D(100.f, 25.f);
	Play::DrawLine(aabb[TYPE_FINISHLINE].BottomLeft(), aabb[TYPE_FINISHLINE].TopLeft(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_FINISHLINE].TopRight(), aabb[TYPE_FINISHLINE].BottomRight(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_FINISHLINE].TopLeft(), aabb[TYPE_FINISHLINE].TopRight(), Play::cGreen);
	Play::DrawLine(aabb[TYPE_FINISHLINE].BottomRight(), aabb[TYPE_FINISHLINE].BottomLeft(), Play::cGreen);

}

void DrawKeyAndFinishline()
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	GameObject& keyObj(Play::GetGameObjectByType(TYPE_KEY));
	GameObject& finishlineObj(Play::GetGameObjectByType(TYPE_FINISHLINE));

	keyObj.scale = 2;
	keyObj.animSpeed = 1;

	finishlineObj.scale = 2;

	if (HasCollided(playerObj.pos, keyObj.pos))
	{
	keyObj.pos = playerObj.pos;
	HasKey = 1;
	}
	
	Play::DrawObjectRotated(finishlineObj);
	Play::DrawObjectRotated(keyObj);
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

			playerObj.pos.x = attachedTileIdObj.pos.x ; 
			playerObj.pos.y = attachedTileIdObj.pos.y - 63;

			gamestate.PlayerState = STATE_WALKING; 
		}
	}
}

void OnFinishline()
{
	GameObject& finishlineObj(Play::GetGameObjectByType(TYPE_FINISHLINE));
	aabb[TYPE_FINISHLINE].pos = Point2D(finishlineObj.pos);
	aabb[TYPE_FINISHLINE].size = Vector2D(100.f, 25.f);

	GameObject& playerObj{ Play::GetGameObjectByType(TYPE_PLAYER) };
	aabb[TYPE_PLAYER].pos = Point2D(playerObj.pos);
	aabb[TYPE_PLAYER].size = Vector2D(20.f, 40.f);

	bool left = (aabb[TYPE_FINISHLINE].Left().x < aabb[TYPE_PLAYER].Right().x);
	bool right = (aabb[TYPE_FINISHLINE].Right().x > aabb[TYPE_PLAYER].Left().x);
	bool up = (aabb[TYPE_FINISHLINE].Top().y < aabb[TYPE_PLAYER].Bottom().y);
	bool down = (aabb[TYPE_FINISHLINE].Bottom().y > aabb[TYPE_PLAYER].Bottom().y);

	if (up && down && left && right)
	{
		playerObj.velocity = { 0,0 };
		gravity = { 0,0 };
		playerObj.pos.y = finishlineObj.pos.y - 64;
		gamestate.PlayerState = STATE_WALKING;
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
		
	if (HasKey == 1)
	{
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