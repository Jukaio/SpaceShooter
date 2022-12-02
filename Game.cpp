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

struct Bullet {
	Entity owner{ 0 };
};

struct EnemyGroupMember {
	Entity parentID;
};

struct TextureName {
	std::string value; // Just stay short string pls
};

struct TileProperty {

};

struct SpriteProperty {

};

struct SingleShot {
	uint64_t fireRate { 0 };
	uint64_t nextTimeAvailable{ 0 };
	float bulletSpeed { 0.0f };
};

struct DoubleShot {
	uint64_t fireRate{ 0 };
	uint64_t nextTimeAvailable{ 0 };
	float bulletSpeed{ 0.0f };
};

using TextureTable = Table<256, TextureName, std::filesystem::path, SDL_Texture*, TileProperty, SpriteProperty>;
static TextureTable textureTable;

struct ColliderID {
	Entity id;
};

// Make table from signatures would be cool to keep this part slim
using GameplayTable = Table<10240,
	TextureID,
	EnemyGroupMember,
	EnemyAI,
	EnemyRow,
	SingleShot,
	DoubleShot,
	ColliderID, 
	Position,
	LookDirection,
	Dimension, 
	Input, 
	Bullet,
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
		auto entity = CreateSingle(textureTable, Signature<SpriteProperty>(textureTable));
		Add(textureTable, entity, TextureName{ entry.path().filename().replace_extension("").string() });
		Add(textureTable, entity, texture);
		Add(textureTable, entity, entry.path());
	}

	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "Assets" / "Tiles")) {
		SDL_Texture* texture = game->LoadTexture(entry.path());
		auto entity = CreateSingle(textureTable, Signature<TileProperty>(textureTable));
		Add(textureTable, entity, TextureName{ entry.path().filename().replace_extension("").string() });
		Add(textureTable, entity, texture);
		Add(textureTable, entity, entry.path());
	}
}

Entity SetupBullet(Entity owner, Position pos, Dimension size, LookDirection lookDirection, float speed, Entity recycle = InvalidEntity) {


	Dimension bulletSize { 16.0f, 16.0f };

	float x = pos.x + (size.w * 0.5f) - (bulletSize.w * 0.5f);
	float y = pos.y + (size.h * 0.5f) - (bulletSize.h * 0.5f);

	auto bullet = recycle == InvalidEntity ? CreateSingle(gameplayTable, Signature<Bullet>(gameplayTable)) : recycle;
	Add(gameplayTable, bullet, Bullet{ owner });
	Add(gameplayTable, bullet, TextureID{ Find(textureTable, [](TextureName& name) { return name.value == "tile_0000"; }) }); // Might be heavy, but right now O(0)
	Add(gameplayTable, bullet, Color{ 0, 255, 0, 255 });
	Add(gameplayTable, bullet, Position{ x, y });
	Add(gameplayTable, bullet, bulletSize);
	Add(gameplayTable, bullet, lookDirection);
	Add(gameplayTable, bullet, Velocity{ lookDirection.x, lookDirection.y });
	Add(gameplayTable, bullet, Speed{ speed });
	return bullet;
}


void SetupPlayer() {
	constexpr auto signature = Signature<Input, Velocity>(gameplayTable);
	auto player = CreateSingle(gameplayTable, signature);

	Add(gameplayTable, player, TextureID{ Find(textureTable, [](TextureName& name) { return name.value == "ship_0000"; }) });
	Add(gameplayTable, player, Speed{ 240.0f });
	Add(gameplayTable, player, Color{ 0, 255, 0, 255 });
	Add(gameplayTable, player, Position{ 400.0f, 500.0f });
	Add(gameplayTable, player, Dimension{ 64.0f, 64.0f });
	Add(gameplayTable, player, LookDirection{ 0.0f, -1.0f });
	Add(gameplayTable, player, SingleShot { 500, 0, 360.0f });
}

void SetupEnemy(Game* game) {

	constexpr auto signature = Signature<EnemyAI>(gameplayTable);
	int windowWidth, windowHeight;
	const int width = 12;
	const int height = 6;
	game->GetWindowSize(&windowWidth, &windowHeight);

	const Dimension dim{ 64.0f, 64.0f };
	const TextureID textureID{ Find(textureTable, [](TextureName& name) { return name.value == "ship_0001"; }) };

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
			Add(gameplayTable, enemy, EnemyGroupMember { parent });
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
		if (game->IsDown(SDL_SCANCODE_S)) {
			direction.y = 1.0f;
		}
		if (game->IsDown(SDL_SCANCODE_W)) {
			direction.y = -1.0f;
		}
		if (game->IsDown(SDL_SCANCODE_A)) {
			direction.x = -1.0f;
		}
		if (game->IsDown(SDL_SCANCODE_D)) {
			direction.x = 1.0f;
		}
		velocity = { direction.x, direction.y };
	});
}

void ApplySpeedToVelocity() {
	For(gameplayTable, [](Speed speed, Velocity& velocity) {
		glm::vec2 direction{ velocity.x, velocity.y };
		if (glm::length(direction) > 0.0f) {
			direction = glm::normalize(direction);
		}

		velocity.x = direction.x * speed.value;
		velocity.y = direction.y * speed.value;
	});
}

void ApplyVelocity(float dt) {
	For(gameplayTable, [dt](Position& pos, Velocity vel) {
		pos.x += vel.x * dt;
		pos.y += vel.y * dt;
	});
}

void ClampToScreen() {

	For<EnemyGroupMember, EnemyRow, Bullet>(gameplayTable, [](Input, Entity entity, Position& pos, Dimension size) {
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

void BounceVelocityOfEnemyRows() {
	For(gameplayTable, [](EnemyRow, Position& pos, Dimension size, Speed speed, Velocity& velocity) {
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

void DrawRect(SDL_Renderer* renderer) {
	For(gameplayTable, [&renderer](Entity entity, Position pos, Dimension dim, Color color) {
		int width;
		int height;

		int x = pos.x;
		int y = pos.y;

		if (Has<EnemyGroupMember>(gameplayTable, entity)) {
			auto parent = Get<EnemyGroupMember>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<EnemyGroupMember>(gameplayTable, parent)) {
					parent = Get<EnemyGroupMember>(gameplayTable, parent).parentID;
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

void DrawLookDirection(SDL_Renderer* renderer) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

	// Something like this might be better
	//EntityContainer nonEnemyGroupMembers{ };
	//auto enemyGroupMembers = Where<EntityContainer>(gameplayTable, [&nonEnemyGroupMembers](Entity entity, Position pos, Dimension size, LookDirection dir) {
	//	if (!Has<EnemyGroupMember>(gameplayTable, entity)) {
	//		nonEnemyGroupMembers.push_back(entity);
	//		return false;
	//	}
	//	return true; 
	//});


	For(gameplayTable, [renderer](Entity entity, Position pos, Dimension size, LookDirection dir) {
		float x = pos.x + (size.w * 0.5f);
		float y = pos.y + (size.h * 0.5f);

		if (Has<EnemyGroupMember>(gameplayTable, entity)) {
			auto parent = Get<EnemyGroupMember>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<EnemyGroupMember>(gameplayTable, parent)) {
					parent = Get<EnemyGroupMember>(gameplayTable, parent).parentID;
				}
				else {
					break;
				}
			}
		}
		SDL_RenderDrawLineF(renderer, x, y, x + (dir.x * size.w), y + (dir.y * size.h));
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

		if (Has<EnemyGroupMember>(gameplayTable, entity)) {
			auto parent = Get<EnemyGroupMember>(gameplayTable, entity).parentID;
			while (true) {
				auto p = Get<Position>(gameplayTable, parent);
				x += p.x;
				y += p.y;
				if (Has<EnemyGroupMember>(gameplayTable, parent)) {
					parent = Get<EnemyGroupMember>(gameplayTable, parent).parentID;
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

	For(gameplayTable, [this](Input, SingleShot& weapon, Entity entity, Position pos, Dimension size, LookDirection dir) {
		const auto ticks = SDL_GetTicks64();
		if (ticks > weapon.nextTimeAvailable && IsDown(SDL_SCANCODE_SPACE)) {
			auto bullet = Find(gameplayTable, [](GameplayBitfield signature) {
				return signature.none();
			});
			pos.y += (dir.y * (size.h * 0.25f));
			SetupBullet(entity, pos, size, dir, weapon.bulletSpeed, bullet);
			weapon.nextTimeAvailable = ticks + weapon.fireRate;
		}
	});

	For(gameplayTable, [this](Input, DoubleShot& weapon, Entity entity, Position pos, Dimension size, LookDirection dir) {
		const auto ticks = SDL_GetTicks64();
		if (ticks > weapon.nextTimeAvailable && IsDown(SDL_SCANCODE_SPACE)) {
			auto bullet = Find(gameplayTable, [](GameplayBitfield signature) {
				return signature.none();
			});
			float x = pos.x - (size.w * 0.25f);
			SetupBullet(entity, { x, pos.y }, size, dir, bullet);
			bullet = Find(gameplayTable, [](GameplayBitfield signature) {
				return signature.none();
			});
			x = pos.x + (size.w * 0.25f);
			SetupBullet(entity, { x, pos.y }, size, dir, weapon.bulletSpeed, bullet);
			weapon.nextTimeAvailable = ticks + weapon.fireRate;
		}
	});
	ApplySpeedToVelocity();

	ApplyVelocity(dt);

	auto enemies = Where<EntityContainer>(gameplayTable, [](EnemyAI, Position, Dimension){ return true; });

	For(gameplayTable, [&enemies](Entity entity, Bullet bullet, Position pos, Dimension dim) {
		for (auto enemy : enemies) {
			if (enemy != bullet.owner) {
				auto enemyPos = Get<Position>(gameplayTable, enemy);
				const auto enemyDim = Get<Dimension>(gameplayTable, enemy);

				if (Has<EnemyGroupMember>(gameplayTable, enemy)) {
					auto parent = Get<EnemyGroupMember>(gameplayTable, enemy).parentID;
					while (true) {
						auto p = Get<Position>(gameplayTable, parent);
						enemyPos.x += p.x;
						enemyPos.y += p.y;
						if (Has<EnemyGroupMember>(gameplayTable, parent)) {
							parent = Get<EnemyGroupMember>(gameplayTable, parent).parentID;
						}
						else {
							break;
						}
					}
				}

				SDL_FRect enemyRect{ enemyPos.x, enemyPos.y, enemyDim.w, enemyDim.h };
				SDL_FRect bulletRect{ pos.x, pos.y, dim.w, dim.h };
				SDL_FRect result;
				if (SDL_IntersectFRect(&enemyRect, &bulletRect, &result)) {
					Destroy(gameplayTable, enemy);
					Destroy(gameplayTable, entity);
					return;
				}
			}
		}
	});

	ClampToScreen();
	
	For(gameplayTable, [this](Bullet, Entity entity, Position& pos, Dimension size) {
		if (pos.y < 0.0f || pos.y + size.h > 640.0f) {
			Destroy(gameplayTable, entity);
		}
		else if (pos.y + size.h > 640.0f) {
			Destroy(gameplayTable, entity);
		}
	});

	BounceVelocityOfEnemyRows();
}

void Game::OnRender(float dt, SDL_Renderer* renderer) {
	DrawRect(renderer);
	DrawSprite(renderer);
	DrawLookDirection(renderer);
}

