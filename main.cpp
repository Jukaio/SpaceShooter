
#include<limits>
#include<array>

#include<SDL2/SDL.h>

#pragma region ApplicationQuit
static bool isAppRunning = true;
static bool ApplicationIsRunning() {
	return isAppRunning;
}

static void ApplicationQuit() {
	isAppRunning = false;
}
#pragma endregion

#pragma region ECS

#pragma region Entity
using Entity = uint64_t;
constexpr Entity InvalidEntity = std::numeric_limits<uint64_t>::max();
constexpr size_t MaxEntityCount = 1024;
#pragma endregion

#pragma region Component
template<typename T>
struct Component {
	T test;
};

template<typename T, size_t size>
struct ComponentArray
{
public:
	void push_back(const T& element) {
		push_back(std::move(element));
	}
	void push_back(T&& element) {
		_data[_pivot] = std::move(element);
		_pivot += 1;
	}

	T& operator[](size_t index) {
		return _data[index];
	}

private:
	std::array<T, size> _data{ };
	size_t _pivot{ 0 };
};

template<typename T, size_t size>
struct ComponentArraySlice {
public:
	using iterator = typename std::array<T, size>::iterator;
	using const_iterator = typename std::array<T, size>::const_iterator;


	_NODISCARD _CONSTEXPR17 const_iterator begin() const noexcept {
		return _begin;
	}

	_NODISCARD _CONSTEXPR17 const_iterator end() const noexcept {
		return _end;
	}

private:
	const_iterator _begin;
	const_iterator _end;
};

#pragma endregion

#pragma endregion

#pragma region Main
int main(int, char**) {
	ComponentArray<size_t, 1024> IDs;
	IDs.push_back(5);

	SDL_Window* window = SDL_CreateWindow("Basic Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	
	auto quitWatcher = [](void* data, SDL_Event* e) { 
		if (e->type == SDL_QUIT) {
			ApplicationQuit();
		}
		return 0; 
	};

	SDL_AddEventWatch(quitWatcher, nullptr);

	while (ApplicationIsRunning()) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {

		}

		SDL_RenderClear(renderer);

		// Drawing here

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
#pragma endregion
