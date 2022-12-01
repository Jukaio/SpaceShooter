#pragma once

#include <SDL2/SDL_scancode.h>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

namespace std {
	namespace filesystem {
		class path;
	}
}

class Application {
public:
	Application(const char* name);

	virtual ~Application();

	void Run();

	void Quit();

	bool IsRunning();

	virtual void OnUpdate(float dt) = 0;
	virtual void OnRender(float dt, SDL_Renderer* renderer) = 0;

	SDL_Texture* LoadTexture(const std::filesystem::path& path);

	bool isDown(SDL_Scancode key) const;
	bool isUp(SDL_Scancode key) const;
	bool justDown(SDL_Scancode key) const;
	bool justUp(SDL_Scancode key) const;

	void GetWindowSize(int* width, int* height);

private:
	void UpdateKeyboardStates();

	SDL_Window* window{ nullptr };
	SDL_Renderer* renderer{ nullptr };
	uint8_t currState[SDL_NUM_SCANCODES]{ };
	uint8_t prevState[SDL_NUM_SCANCODES]{ };
	bool isRunning{ false };
};