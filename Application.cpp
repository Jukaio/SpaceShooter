#include "Application.h"
#include <filesystem>
#include <string>
#include<SDL2/SDL_ttf.h>

#include <iostream>

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
	std::ignore = memcpy_s(prevState, sizeof(prevState), currState, sizeof(currState));

	if (SDL_GetKeyboardFocus() != window) {
		memset(currState, 0, sizeof(currState));
		return;
	}
	int arrSize = 0;
	auto* keyboardState = SDL_GetKeyboardState(&arrSize); 
	auto err = memcpy_s(currState, sizeof(currState), keyboardState, arrSize);
	assert(err == 0 && "Not enough memory space in input state type");
}

Application::Application(const char* name) {
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();

	window = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC */);

	SDL_AddEventWatch(QuitListener, this);
}

Application::~Application() {
	SDL_DelEventWatch(QuitListener, this);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

void Application::GetWindowSize(int* width, int* height) {
	SDL_GetWindowSize(window, width, height);
}

void Application::Run() {
	auto tp = SDL_GetTicks64();
	auto dt = 0.0f;
	auto start = std::chrono::high_resolution_clock::now();
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

		std::chrono::duration<float> dtChrono = std::chrono::high_resolution_clock::now() - start; //  std::ratio<1, 1>
		start = std::chrono::high_resolution_clock::now();
		OnUpdate(dtChrono.count());

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		OnRender(dtChrono.count(), renderer);

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

bool Application::IsDown(SDL_Scancode key) const {
	return currState[key];
}

bool Application::IsUp(SDL_Scancode key) const {
	return !currState[key];
}

bool Application::JustDown(SDL_Scancode key) const {
	return currState[key] && !prevState[key];
}

bool Application::JustUp(SDL_Scancode key) const {
	return !currState[key] && prevState[key];
}