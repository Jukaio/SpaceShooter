#include "Application.h"
#include <filesystem>
#include <string>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <assert.h>

static int QuitListener(void* data, SDL_Event* e) {
	if (e->type == SDL_QUIT) {
		Application* app = reinterpret_cast<Application*>(data);
		app->Quit();
	}
	return 0;
}

void Application::UpdateKeyboardStates() {
	int arrSize = 0;
	auto* keyboardState = SDL_GetKeyboardState(&arrSize);
	std::ignore = memcpy_s(prevState, sizeof(prevState), currState, sizeof(currState));

	auto err = memcpy_s(currState, sizeof(currState), keyboardState, arrSize);
	assert(err == 0 && "Not enough memory space in input state type");
}

Application::Application(const char* name) {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);
	window = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_AddEventWatch(QuitListener, this);
}

Application::~Application() {
	SDL_DelEventWatch(QuitListener, this);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();
}

void Application::GetWindowSize(int* width, int* height) {
	SDL_GetWindowSize(window, width, height);
}

void Application::Run() {
	auto tp = SDL_GetTicks64();
	auto dt = 0.0f;

	isRunning = true;
	while (IsRunning()) {
		SDL_PumpEvents();
		UpdateKeyboardStates();

		{ // Calculate DeltaTime
			auto newTp = SDL_GetTicks64();
			auto difference = newTp - tp;
			dt = static_cast<float>(difference) / 1000.0f;
			tp = newTp;
		}


		OnUpdate(dt);

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		OnRender(dt, renderer);

		SDL_RenderPresent(renderer);
	}

}

void Application::Quit() {
	isRunning = false;
}

bool Application::IsRunning() {
	return isRunning;
}

SDL_Texture* Application::LoadTexture(const std::filesystem::path& path) {
	return IMG_LoadTexture(renderer, (const char*)path.u8string().c_str());
}

bool Application::isDown(SDL_Scancode key) const {
	return currState[key];
}

bool Application::isUp(SDL_Scancode key) const {
	return !currState[key];
}

bool Application::justDown(SDL_Scancode key) const {
	return currState[key] && !prevState[key];
}

bool Application::justUp(SDL_Scancode key) const {
	return !currState[key] && prevState[key];
}