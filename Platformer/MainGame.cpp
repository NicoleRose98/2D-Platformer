#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH{ 1280 };
int DISPLAY_HEIGHT{ 720 };
int DISPLAY_SCALE{ 1 };

int STATE = -1;

int facing = 0;
Vector2D gravity{ 0,0 };

int PlayerSpeed = 0;
const Vector2D PLAYER_VELOCITY = { 0,0 };
Vector2D PLAYER_JUMP = { 0,-40 };

enum GameObjectType
{
	TYPE_PLAYER = 0,
	TYPE_FLOOR,
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
};

GameState gamestate;

bool TileProximity(Point2f pos1, Point2f pos2);
void Platforms();
void Grounded();
void PlayerMovement();
void PlayerIdleDirection();
void IdleToWalking();
void WalkToFall();
void PlayerWalkingDirection();
void Falling();
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
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CentreAllSpriteOrigins();

	Platforms();
	Play::CreateGameObject(TYPE_PLAYER, { DISPLAY_WIDTH / 2, 675 }, 10, "walk_right_8");
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = PLAYER_VELOCITY;

	Play::CreateGameObject(TYPE_BACKGROUND, { DISPLAY_WIDTH / 2, -280 }, 10, "background");
	GameObject& backgroundObj(Play::GetGameObjectByType(TYPE_BACKGROUND));

	Grounded();
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	GameObject& playerObj(Play::GetGameObjectByType(TYPE_PLAYER));
	playerObj.velocity = playerObj.velocity + gravity;
	Play::SetCameraPosition({ 0 , 0 });

	if (playerObj.pos.y <= 450)
	{
		Play::SetCameraPosition({ 0 , playerObj.pos.y - 450 });
	}

	Draw();
	DrawTiles();
	PlayerMovement();


	switch (gamestate.PlayerState)
	{

	case STATE_WALKING:
		STATE = 1;
		PlayerWalkingDirection();
		WalkToFall();
		IdleToWalking();
		PlayerJump();
		Grounded();
		Falling();
		break;

	case STATE_JUMPING:
		STATE = 2;
		PlayerJumpingDirection();
		Falling();
		break;

	case STATE_FALLING:
		STATE = 3;
		PlayerFallingDirection();
		Grounded();
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
	Play::DrawFontText("132px", std::to_string(STATE), { DISPLAY_WIDTH / 2, 50 }, Play::CENTRE);
}

void Platforms()
{
	int levelY = 700;
	int numLevels = 20;
	int widthSpacing = 50;
	int heightSpacing = 150;

	for (int n = 0; n < numLevels; n++)
	{

		if (n == 0)
		{
			for (int m = 0; m < 26; m++)
			{
				Play::CreateGameObject(TYPE_FLOOR, { m * widthSpacing + 25, levelY }, 10, "floor");
			}
		}
		else
		{
			int numPlatforms = Play::RandomRollRange(3, 5);

			for (int m = 1; m < numPlatforms; m++)
			{

				int randomSpacing = Play::RandomRollRange(50, 1230);

				Play::CreateGameObject(TYPE_FLOOR, { randomSpacing, levelY }, 10, "floor");
			}
		}

		levelY -= heightSpacing;
	}

}

bool TileProximity(Point2f pos1, Point2f pos2)
{
	const float DISTANCE_TEST = 90.0f;

	Vector2f separation = pos2 - pos1;
	float dist = sqrt((separation.x * separation.x) + (separation.y * separation.y));
	if (dist < DISTANCE_TEST)
	{
		return true;
	}
	else
		return false;
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