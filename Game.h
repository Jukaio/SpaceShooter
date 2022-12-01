#pragma once

#include <vector>
#include "Application.h"
using Entity = size_t;

class Game : public Application {
public:
	Game(const char* name);

	~Game();

	virtual void OnUpdate(float dt) final;
	virtual void OnRender(float dt, SDL_Renderer* renderer) final;

};
