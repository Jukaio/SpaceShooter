#include<filesystem>
#include<glm/glm.hpp>

#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>

#include "Entities.h"


#pragma region Main

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

struct Input
{
	bool isDown(SDL_Scancode key) const {
		return currState[key];
	}

	bool isUp(SDL_Scancode key) const {
		return !currState[key];
	}

	bool justDown(SDL_Scancode key) const {
		return currState[key] && !prevState[key];
	}

	bool justUp(SDL_Scancode key) const {
		return !currState[key] && prevState[key];
	}

	uint8_t currState[SDL_NUM_SCANCODES];
	uint8_t prevState[SDL_NUM_SCANCODES];
};

struct TextureID {
	Entity id;
};


using TextureTable = Table<64, std::filesystem::path, SDL_Texture*>;
static TextureTable textureTable;

using GameplayTable = Table<1024, TextureID, Position, Dimension, Input, Speed, Velocity, Color>;
using GameplayBitfield = Bitfield<GameplayTable>::Type;
static GameplayTable gameplayTable;


std::unordered_map<SDL_Texture*, std::vector<Entity>> map;



#pragma region ApplicationQuit
static bool isAppRunning = true;
static bool ApplicationIsRunning() {
	return isAppRunning;
}

static void ApplicationQuit() {
	isAppRunning = false;
}

int main(int, char**) {
	IMG_Init(IMG_INIT_PNG);

	//SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Basic Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

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
	std::cout << "=====================" << '\n' << '\n';;

	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "Assets" / "Sprites")) {
		SDL_Texture* texture = IMG_LoadTexture(renderer, (const char*)entry.path().u8string().c_str());
		auto entity = CreateSingle(textureTable, Signature<SDL_Texture*, std::filesystem::path>(textureTable));
		Set(textureTable, entity, texture);
		Set(textureTable, entity, entry.path());
	}

	auto shipTextures = Where<EntityContainer>(textureTable, [](SDL_Texture* texture){
		return texture != nullptr;
	});


	{
		constexpr auto signature = Signature<Input, Velocity>(gameplayTable);
		auto player = CreateSingle(gameplayTable, signature);
 
		// Add after signature is set
		Add(gameplayTable, player, TextureID{ shipTextures[0] });
		Add(gameplayTable, player, Speed{ 240.0f });
		Add(gameplayTable, player, Color{ 0, 255, 0, 255 });
		Add(gameplayTable, player, Position{ 400.0f, 500.0f });
		Add(gameplayTable, player, Dimension{ 128.0f, 128.0f });
	}

	auto tooFash = Where<std::vector<Entity>>(textureTable, [](SDL_Texture* texture){
		return texture != nullptr;
	});

	auto quitWatcher = [](void* data, SDL_Event* e) { 
		if (e->type == SDL_QUIT) {
			ApplicationQuit();
		}
		return 0; 
	};
	SDL_AddEventWatch(quitWatcher, nullptr);
	auto tp = SDL_GetTicks64();
	auto dt = 0.0f;
	while (ApplicationIsRunning()) {
		SDL_PumpEvents();

		{
			auto newTp = SDL_GetTicks64();
			auto difference = newTp - tp;
			dt = static_cast<float>(difference) / 1000.0f;
			tp = newTp;
		}

		{ // Update Keyboard states
			int arrSize = 0;
			auto* keyboardState = SDL_GetKeyboardState(&arrSize);
			For(gameplayTable, [arrSize, keyboardState](Input& input) {
				std::ignore = memcpy_s(input.prevState, sizeof(input.prevState), input.currState, sizeof(input.currState));

				auto err = memcpy_s(input.currState, sizeof(input.currState), keyboardState, arrSize);
				assert(err == 0 && "Not enough memory space in input state type");
			});
		}

		For(gameplayTable, [&shipTextures](const Input& input, Velocity& velocity, Speed speed, TextureID& texture) {
			velocity = Velocity{ 0.0f, 0.0f };
			glm::vec2 direction{ 0.0f, 0.0f };
			if (input.isDown(SDL_SCANCODE_S)) {
				direction.y = 1.0f;
			}
			if (input.isDown(SDL_SCANCODE_W)) {
				direction.y = -1.0f;
			}
			if (input.isDown(SDL_SCANCODE_A)) {
				direction.x = -1.0f;
			}
			if (input.isDown(SDL_SCANCODE_D)) {
				direction.x = 1.0f;
			}

			if (input.justDown(SDL_SCANCODE_Q)) {
				texture.id--;
			}
			if (input.justDown(SDL_SCANCODE_E)) {
				texture.id++;
			}

			texture.id = (texture.id + shipTextures.size()) % shipTextures.size();

			if (glm::length(direction) > 0.0f) {
				direction = glm::normalize(direction);
			}
			
			velocity.x = direction.x * speed.value;
			velocity.y = direction.y * speed.value;
		});

		For(gameplayTable, [dt](Position& pos, Velocity vel) {
			pos.x += vel.x * dt;
			pos.y += vel.y * dt;
		});

		For(gameplayTable, [](Position& pos, Dimension size) {
			if(pos.x < 0.0f) {
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
		

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);


		For(gameplayTable, [&renderer](Position pos, Dimension dim, Color color) {
			SDL_FRect rect{ pos.x, pos.y, dim.w, dim.h };
			auto c = color.value;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_RenderDrawRectF(renderer, &rect);
		});

		For(gameplayTable, [&renderer](Position pos, Dimension dim, TextureID texture) {
			auto tex = Get<SDL_Texture*>(textureTable, texture.id);
			int width;
			int height;
			SDL_QueryTexture(tex, nullptr, nullptr, &width, &height);
			SDL_Rect src{ 0, 0, width, height };
			SDL_FRect dst{ pos.x, pos.y, dim.w, dim.h };
			SDL_RenderCopyF(renderer, tex, &src, &dst);
		});

		SDL_RenderPresent(renderer);
	}

	For(textureTable, [](SDL_Texture* texture) {
		SDL_DestroyTexture(texture);
	});

	SDL_DelEventWatch(quitWatcher, nullptr);

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
#pragma endregion
