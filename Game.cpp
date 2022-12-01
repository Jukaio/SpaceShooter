#include "Game.h"

#include<filesystem>
#include<glm/glm.hpp>

#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>

#include "Entities.h"


struct Speed {
	float value;
};

struct Position {
	float x;
	float y;
};

struct Dimension {
	float w;
	float h;
};

struct Color {
	SDL_Color value;
};

struct Velocity {
	float x;
	float y;
};

struct LookDirection {
	float x;
	float y;
};

struct Input {

};

struct EnemyAI {

};

struct TextureID {
	Entity id;
};

struct EnemyRow {

};

struct Child {
	Entity parentID;
};

using TextureTable = Table<64, std::filesystem::path, SDL_Texture*>;
static TextureTable textureTable;

struct ColliderID {
	Entity id;
};

// Make table from signatures would be cool to keep this part slim
using GameplayTable = Table<
	1024, 
	TextureID,
	Child,
	EnemyAI,
	EnemyRow,
	ColliderID, 
	Position,
	LookDirection,
	Dimension, 
	Input, 
	Speed, 
	Velocity, 
	Color
>;

using GameplayBitfield = Bitfield<GameplayTable>::Type;
static GameplayTable gameplayTable{};

struct RectProperty {
	float width;
	float height;
};

struct CircleProperty {
	float radius;
};

struct LineProperty {
	float x;
	float y;
};

using ColliderTable = Table<1024, Position, RectProperty, CircleProperty, LineProperty>;
ColliderTable colliderTable{};

void LoadTextures(Game* game) {
	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "Assets" / "Sprites")) {
		SDL_Texture* texture = game->LoadTexture(entry.path());
		auto entity = CreateSingle(textureTable, Signature<SDL_Texture*, std::filesystem::path>(textureTable));
		Set(textureTable, entity, texture);
		Set(textureTable, entity, entry.path());
	}
}

void SetupPlayer() {
	constexpr auto signature = Signature<Input, Velocity>(gameplayTable);
	auto player = CreateSingle(gameplayTable, signature);

	// Add after signature is set
	auto shipTextures = Where<EntityContainer>(textureTable, [](SDL_Texture* texture) {
		return texture != nullptr;
	});

	Add(gameplayTable, player, TextureID{ shipTextures[0] });
	Add(gameplayTable, player, Speed{ 240.0f });
	Add(gameplayTable, player, Color{ 0, 255, 0, 255 });
	Add(gameplayTable, player, Position{ 400.0f, 500.0f });
	Add(gameplayTable, player, Dimension{ 64.0f, 64.0f });
	Add(gameplayTable, player, LookDirection{ 0.0f, -1.0f });
}

void SetupEnemy(Game* game) {
	auto shipTextures = Where<EntityContainer>(textureTable, [](SDL_Texture* texture) {
		return texture != nullptr;
	});
	auto enemyTextures = shipTextures[1];

	constexpr auto signature = Signature<EnemyAI>(gameplayTable);
	int windowWidth, windowHeight;
	const int width = 12;
	const int height = 6;
	game->GetWindowSize(&windowWidth, &windowHeight);

	const Dimension dim{ 64.0f, 64.0f };
	const TextureID textureID{ enemyTextures };

	const float rowWidth = (width * dim.w);
	const float offsetX = (windowWidth - (width * dim.w)) * 0.5f;

	for (int y = 0; y < height; y++) {
		float yPos = y * dim.h;
		Velocity velocity{ y % 2 == 0 ? 1.0f : -1.0f, 0.0f };


		auto parent = CreateSingle(gameplayTable, Signature<EnemyRow>(gameplayTable));
		for (int x = 0; x < width; x++) {
			auto enemy = CreateSingle(gameplayTable, signature);
			Add(gameplayTable, enemy, textureID);
			Add(gameplayTable, enemy, dim);
			Add(gameplayTable, enemy, Color{ 255, 0, 0, 255 });
			Add(gameplayTable, enemy, Position{ (x * dim.w), 0 });
			Add(gameplayTable, enemy, LookDirection{ 0.0f, 1.0f });
			Add(gameplayTable, enemy, Child { parent });
		}

		Add(gameplayTable, parent, Position{ offsetX, yPos});
		Add(gameplayTable, parent, Dimension{ rowWidth, dim.h });
		Add(gameplayTable, parent, Color{ 255, 0, 255, 255 });
		Add(gameplayTable, parent, Speed{ 20.0f });
		Add(gameplayTable, parent, velocity);
	}
}

void ApplyInputToVelocity(Game* game) {
	For(gameplayTable, [game](const Input& input, Velocity& velocity) {
		velocity = Velocity{ 0.0f, 0.0f };
		glm::vec2 direction{ 0.0f, 0.0f };
		if (game->isDown(SDL_SCANCODE_S)) {
			direction.y = 1.0f;
		}
		if (game->isDown(SDL_SCANCODE_W)) {
			direction.y = -1.0f;
		}
		if (game->isDown(SDL_SCANCODE_A)) {
			direction.x = -1.0f;
		}
		if (game->isDown(SDL_SCANCODE_D)) {
			direction.x = 1.0f;
		}
		velocity = { direction.x, direction.y };
	});
}

void ApplyVelocity(float dt) {
	For(gameplayTable, [dt](Position& pos, Velocity vel) {
		pos.x += vel.x * dt;
		pos.y += vel.y * dt;
	});
}

void ClampToScreen() {

	For<Child>(gameplayTable, [](Input, Entity entity, Position& pos, Dimension size) {
		if (pos.x < 0.0f) {
			pos.x = 0.0f;
		}
		else if (pos.x + size.w > 800.0f) {
			pos.x = 800 - size.w;
		}

		if (pos.y < 0.0f) {
			pos.y = 0.0f;
		}
		else if (pos.y + size.h > 640.0f) {
			pos.y = 640.0f - size.h;
		}
	});
}

void DrawRect(SDL_Renderer* renderer) {
	For(gameplayTable, [&renderer](Entity entity, Position pos, Dimension dim, Color color) {
		int width;
		int height;

		int x = pos.x;
		int y = pos.y;

		if (Has<Child>(gameplayTable, entity)) {
			auto parent = Get<Child>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<Child>(gameplayTable, parent)) {
					parent = Get<Child>(gameplayTable, parent).parentID;
				}
				else {
					break;
				}
			}
		}

		SDL_FRect rect{ x, y, dim.w, dim.h };
		auto c = color.value;
		SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
		SDL_RenderDrawRectF(renderer, &rect);
	});
}

void DrawSprite(SDL_Renderer* renderer) {
	For(gameplayTable, [&renderer](Entity entity, Position pos, Dimension dim, TextureID texture) {
		auto tex = Get<SDL_Texture*>(textureTable, texture.id);
		int width;
		int height;
		SDL_QueryTexture(tex, nullptr, nullptr, &width, &height);
		SDL_Rect src{ 0, 0, width, height };
		int x = pos.x;
		int y = pos.y;

		if (Has<Child>(gameplayTable, entity)) {
			auto parent = Get<Child>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<Child>(gameplayTable, parent)) {
					parent = Get<Child>(gameplayTable, parent).parentID;
				}
				else {
					break;
				}
			}
		}


		SDL_FRect dst{ x, y, dim.w, dim.h };
		SDL_RenderCopyF(renderer, tex, &src, &dst);
	});
}

Game::Game(const char* name) : Application(name) {
	// Fun debug info
	std::cout << "=== DATA ===" << '\n' << '\n';
	std::cout << "=== GAMEPLAY TABLE ===" << '\n';
	std::cout << "Allocated Memory: " << sizeof(gameplayTable) << " Byte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(gameplayTable) / 1024.0f << " Kibibyte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(gameplayTable) / 1024.0f / 1024.0f << " Mebibyte(s)" << '\n';
	std::cout << "=====================" << '\n';
	std::cout << '\n';
	std::cout << "=== TEXTURE TABLE ===" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) << " Byte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) / 1024.0f << " Kibibyte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) / 1024.0f / 1024.0f << " Mebibyte(s)" << '\n';
	std::cout << "=====================" << '\n';
	std::cout << "=====================" << '\n' << '\n';


	LoadTextures(this);
	SetupPlayer();
	SetupEnemy(this);
}

Game::~Game() {
	For(textureTable, [](SDL_Texture* texture) {
		SDL_DestroyTexture(texture);
	});
}

void Game::OnUpdate(float dt) {
	ApplyInputToVelocity(this);

	For(gameplayTable, [](Speed speed, Velocity& velocity) {
		glm::vec2 direction{ velocity.x, velocity.y };
		if (glm::length(direction) > 0.0f) {
			direction = glm::normalize(direction);
		}

		velocity.x = direction.x * speed.value;
		velocity.y = direction.y * speed.value;
	});

	ApplyVelocity(dt);

	ClampToScreen();

	For(gameplayTable, [](EnemyRow, Position pos, Dimension size, Speed speed, Velocity& velocity) {
		if (pos.x < 0.0f) {
			pos.x = 0.0f;
			velocity.x = 1.0f * speed.value;
		}
		else if (pos.x + size.w > 800.0f) {
			pos.x = 800 - size.w;
			velocity.x = -1.0f * speed.value;
		}
	});
}

void Game::OnRender(float dt, SDL_Renderer* renderer) {
	DrawRect(renderer);
	DrawSprite(renderer);

	SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
	For(gameplayTable, [renderer](Entity entity, Position pos, Dimension size, LookDirection dir) {
		float x = pos.x + (size.w * 0.5f);
		float y = pos.y + (size.h * 0.5f);

		if (Has<Child>(gameplayTable, entity)) {
			auto parent = Get<Child>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<Child>(gameplayTable, parent)) {
					parent = Get<Child>(gameplayTable, parent).parentID;
				}
				else {
					break;
				}
			}
		}
		SDL_RenderDrawLineF(renderer, x, y, x + (dir.x * size.w), y + (dir.y * size.h));
	});
}
