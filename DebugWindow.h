#pragma once

#include "Application.h"
#include <thread>

class DebugWindow : public Application {
	DebugWindow() 
		: Application("Debug")
	{

	}

private:

	// Inherited via Application
	virtual void OnUpdate(float dt) override;
	virtual void OnRender(float dt, SDL_Renderer* renderer) override;

	std::thread thread{ };
};