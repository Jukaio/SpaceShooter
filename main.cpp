
#include<limits>
#include<array>
#include<tuple>
#include<type_traits>
#include<functional>

#include<SDL2/SDL.h>

#include"static_vector.hpp"

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
using Entity = size_t;
constexpr Entity InvalidEntity = std::numeric_limits<size_t>::max();
constexpr size_t MaxEntityCount = 1024;

template<size_t size>
using EntityRange = itlib::static_vector<Entity, size>;
#pragma endregion

#pragma region Component
template<typename T>
using ComponentArray = itlib::static_vector<T, MaxEntityCount>;
#pragma endregion

#pragma endregion

template<typename... Types>
using ComponentTable = std::tuple<ComponentArray<Types>...>;
//
//template<typename... types, typename currentType, typename... setTypes>
//void set(ComponentTable<types...>& table, currentType current, setTypes... setTo);
//
//
//template<typename... types, typename... setTypes>
//void set(ComponentTable<types...>& table, setTypes... setTo) {
//	
//}
//


template<typename GetterType, typename... Types>
ComponentArray<GetterType>& Get(ComponentTable<Types...>& table) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	return std::get<ComponentArray<GetterType>>(table);
}

template<typename SetterType, typename... Types>
void Set(ComponentTable<Types...>& table, Entity entity, SetterType setTo) {
	static_assert((std::is_same_v<SetterType, Types> || ...) && "Table does not contain Type");
	auto& column = std::get<ComponentArray<SetterType>>(table);
	column[entity] = setTo;
}

template<typename... Types>
Entity Create(ComponentTable<Types...>& table, std::tuple<Types...> data) {
	(std::get<ComponentArray<Types>>(table).push_back(std::get<Types>(data)), ...);
	return table._Myfirst._Val.size() - 1;
}

template<typename... Types, size_t size>
void ForEach(ComponentTable<Types...>& table, EntityRange<size> entityRange, std::function<void(Types...)> func) {
	const size_t max = table._Myfirst._Val.size();
	using TypesPlain = std::tuple<std::remove_reference_t<std::remove_const_t<Types>>...>;

	for (size_t entity = 0; entity < max, entity += 1) {
		
	}
}


template<size_t size, typename... Types>
void Test(EntityRange<size> entityRange, std::function<void(Types...)> tests) {

}

//template<typename... types>
//class ComponentTable {
//public:
//
//private:
//	std::tuple<ComponentArray<types>...> data{ };
//};

#pragma region Main

static ComponentTable<int, float> masterTable;

int main(int, char**) {
	//PushBack(std::get<ComponentArray<int>>(masterTable));
	Entity player = Create(masterTable, { 0, 0.0f});
	Set(masterTable, player, 5);
	auto& intColumn = Get<int>(masterTable);
	auto& floatColumn = Get<float>(masterTable);
	for (auto& bullshit : intColumn) {
		// Do stuff on each int
	}

	for (auto& bullshit : floatColumn) {
		// Do stuff on each float
	}

	EntityRange<3> range = {0, 1, 2 };

	std::function<void(int, float)> testy;
	ForEach(masterTable, range, testy);

	std::tuple<int, float, char, size_t> test;
	std::tuple<int, char> other;
	auto lhs = std::tuple_cat(test, other);
	std::tie(std::get<int>(test), std::get<int>(test), std::ignore, std::get<int>(test)) = test;

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
