#pragma once

#include <vector>
#include "Application.h"

#include <thread>
using Entity = size_t;

class Game : public Application {
public:
	Game(const char* name);

	~Game();

private:
	virtual void OnUpdate(float dt) final;
	virtual void OnRender(float dt, SDL_Renderer* renderer) final;
};
