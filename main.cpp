
#include<limits>
#include<array>
#include<tuple>
#include<type_traits>
#include<functional>
#include<iostream>
#include<concepts>
#include<bitset>
#include<filesystem>

#include<glm/glm.hpp>

#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>

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
//constexpr size_t MaxEntityCount = 1024;


template<typename Context>
struct TypeSafeID {
	size_t id;

	operator size_t() { return id; }
};

template<size_t size>
using EntityRange = itlib::static_vector<Entity, size>;
#pragma endregion

#pragma region Component
template<typename T, size_t size>
using StaticDataArray = itlib::static_vector<T, size>;

template<typename T, size_t size>
using DynamicDataArray = std::vector<T>;

template<typename... Ts>
using DataEntry = std::tuple<Ts...>;
#pragma endregion

#pragma region System

#pragma endregion

#pragma endregion

template<size_t size, typename... Types>
using Table = std::tuple<StaticDataArray<Types, size>...>;

template<size_t size, typename... Types>
using SubTable = std::tuple<StaticDataArray<Types, size>*...>;

template <typename T>
struct function_traits
	: public function_traits<decltype(&T::operator())>
{};
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
	// we specialize for pointers to member function
{
	enum { arity = sizeof...(Args) };
	// arity is the number of arguments.

	typedef ReturnType result_type;

	using tuple_type = std::tuple<Args...>;
	using tuple_type_nocvref = std::tuple<std::remove_cvref_t<Args>...>;

	template <size_t i>
	struct arg
	{
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		// the i-th argument is equivalent to the i-th tuple element of a tuple
		// composed of those arguments.
	};
};

template<typename T, typename... Ts>
struct is_member_of_type_seq { static const bool value = false; };

template<typename T, typename U, typename... Ts>
struct is_member_of_type_seq<T, U, Ts...>
{
	static const bool value = std::conditional<
		std::is_same<T, U>::value,
		std::true_type,
		is_member_of_type_seq<T, Ts...>
	>::type::value;
};

template<typename, typename>
struct append_to_type_seq { };

template<typename T, typename... Ts>
struct append_to_type_seq<T, std::tuple<Ts...>>
{
	using type = std::tuple<Ts..., T>;
};

template<typename, typename>
struct prepend_to_type_seq { };

template<typename T, typename... Ts>
struct prepend_to_type_seq<T, std::tuple<Ts...>>
{
	using type = std::tuple<T, Ts...>;
};

template<typename, typename>
struct intersect_type_seq
{
	using type = std::tuple<>;
};

template<typename T, typename... Ts, typename... Us>
struct intersect_type_seq<std::tuple<T, Ts...>, std::tuple<Us...>>
{
	using type = typename std::conditional
	<
		!is_member_of_type_seq<T, Us...>::value,
		typename intersect_type_seq
		<
			std::tuple<Ts...>,
			std::tuple<Us...>
		>::type,
		typename prepend_to_type_seq
		<
			T,
			typename intersect_type_seq
			<
				std::tuple<Ts...>,
				std::tuple<Us...>
			>::type
		>::type
	>::type;
};

template<typename, typename>
struct union_type_seq;

template<typename... Dst, typename Current, typename... Rest>
struct union_type_seq<std::tuple<Dst...>, std::tuple<Current, Rest...>> {
	using type = 
	typename union_type_seq
	<
		std::conditional_t
		<
			is_member_of_type_seq<Current, Dst...>::value,
			std::tuple<Dst...>, 
			std::tuple<Current, Dst...>
		>,
		std::tuple<Rest...>
	>::type;
};

template<typename, typename>
struct make_unique_seq;

template<typename... Dst, typename Current, typename... Rest>
struct make_unique_seq<std::tuple<Dst...>, std::tuple<Current, Rest...>> {
	using type = 
	typename union_type_seq
	<
		std::conditional_t
		<
			is_member_of_type_seq<Current, Dst...>::value,
			std::tuple<Dst...>, 
			std::tuple<Current, Dst...>
		>,
		std::tuple<Rest...>
	>::type;
};

template<typename... Dst>
struct make_unique_seq<std::tuple<Dst...>, std::tuple<>> {
	using type = std::tuple<Dst...>;
};


template<typename... Dst>
struct union_type_seq<std::tuple<Dst...>, std::tuple<>> {
	using type = std::tuple<Dst...>;
};



template<typename GetterType, typename... Types, size_t size>
StaticDataArray<GetterType, size>& GetArray(Table<size, Types...>& table) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	return std::get<StaticDataArray<GetterType, size>>(table);
}

template<typename GetterType, typename... Types, size_t size>
StaticDataArray<GetterType, size>*& GetArray(SubTable<size, Types...>& table) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	return std::get<StaticDataArray<GetterType, size>*>(table);
}

template<typename GetterType, typename... Types, size_t size>
GetterType& Get(Table<size, Types...>& table, Entity entity) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	auto& column = GetArray<GetterType>(table);
	return column[entity];
}

template<typename GetterType, typename... Types, size_t size>
GetterType& Get(SubTable<size, Types...>& table, Entity entity) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	auto& column = *GetArray<GetterType>(table);
	return column[entity];
}

template<typename... Types, size_t size>
void Clear(Table<size, Types...>& table) {
	(GetArray<Types>(table).clear(), ...);
}

template<typename SetterType, typename... Types, size_t size >
void Set(Table<size, Types...>& table, Entity entity, SetterType setTo) {
	static_assert((std::is_same_v<SetterType, Types> || ...) && "Table does not contain Type");
	auto& column = GetArray<SetterType>(table);
	column[entity] = setTo;
}

template<typename SetterType, typename... Types, size_t size>
void Set(SubTable<size, Types...>& table, Entity entity, SetterType setTo) {
	static_assert((std::is_same_v<SetterType, Types> || ...) && "Table does not contain Type");
	auto& column = *GetArray<SetterType>(table);
	column[entity] = setTo;
}

template<typename... Types, size_t size>
constexpr size_t Size(const Table<size, Types...>& table) {
	return std::get<0>(table).size(); // If this fails you do not have a table
}

template<typename... Types, size_t size>
constexpr size_t Size(const SubTable<size, Types...>& span) {
	return std::get<0>(span)->size(); // If this fails you do not have a table
}

template<typename... Types, size_t size>
Entity CreateSingle(Table<size, Types...>& table, std::tuple<Types...> data = { }) {
	(std::get<StaticDataArray<Types, size>>(table).push_back(std::get<Types>(data)), ...);
	return Size(table) - 1;
}

//template<typename... Types>
//Entity CreateSingle(ComponentTable<Types...>& table) {
//	(std::get<ComponentArray<Types>>(table).push_back({ }), ...);
//	return Size(table) - 1;
//}

template<size_t size, typename... Types, size_t tableSize>
constexpr EntityRange<size> CreateMultiple(Table<tableSize, Types...>& table, size_t count, std::tuple<Types...> data = { }) {
	EntityRange<size> entities { };
	for (size_t i = 0; i < count; i++) {
		entities.push_back(CreateSingle(table, data));
	}
	return entities;
}

//template<size_t size, typename... Types>
//void For(ComponentTable<Types...>& table, EntityRange<size> entityRange, std::function<void(Types...)> func) {
//	const size_t max = Size(table);
//
//	for (size_t entity = 0; entity < max; entity += 1) {
//		(func(std::get<ComponentArray<std::remove_cvref_t<Types>>>(table)[entity]...));
//	}
//}

template<typename... Types, size_t size, typename Func>
constexpr void For(Table<size, Types...>& table, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");

	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t tupleSize = std::tuple_size_v<Result>;

	static_assert(tupleSize > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(std::index_sequence<Is...>) {
		const auto max = Size(table);

		for (size_t index = 0; index < max; index += 1) {
			func(Get<typename std::remove_cvref_t <std::tuple_element<Is, typename traits::tuple_type>::type>>(table, index)...);
		}
	} (std::make_index_sequence<traits::arity>{});
}

template<typename... Types, size_t size, typename Func>
constexpr void For(SubTable<size, Types...>& table, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");

	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t tupleSize = std::tuple_size_v<Result>;

	static_assert(tupleSize > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(std::index_sequence<Is...>) {
		const auto max = Size(table);

		for (size_t index = 0; index < max; index += 1) {
			func((Get<typename std::remove_cvref_t <std::tuple_element<Is, typename traits::tuple_type>::type>>(table, index))...);
		}
	} (std::make_index_sequence<traits::arity>{});
}

//template<typename... Types, size_t size, typename Func>
//constexpr Table<size, Types...> Where(Table<size, Types...>& table, Func func) {
//	static_assert(sizeof...(Types) > 0, "No data in table");
//
//	using traits = function_traits<Func>;
//	using A = traits::tuple_type_nocvref;
//	using B = std::tuple<Types...>;
//	using Result = intersect_type_seq<A, B>::type;
//	constexpr size_t tupleSize = std::tuple_size_v<Result>;
//
//	static_assert(tupleSize > 0, "No Valid parameters in Function");
//
//	Table<size, Types...> newTable { };
//
//	[&] <size_t... Is>(std::index_sequence<Is...>) {
//		const auto max = Size(table);
//
//		for (size_t index = 0; index < max; index += 1) {
//
//			[&](size_t I) constexpr {
//				//(func((Get<typename std::remove_cvref_t <typename std::tuple_element<I, typename traits::tuple_type>::type>>(table, index))));
//			}(Is...);
//		}
//	} (std::make_index_sequence<traits::arity>{});
//	return newTable;
//}


template<typename Container, typename ContentType>
concept ContainerType = requires (Container container, size_t n) {
	std::same_as<decltype(container.begin()), ContentType*>;
	std::same_as<decltype(container.end()), ContentType*>;
	std::convertible_to<decltype(container[n]), ContentType>;
};

template<typename Container, typename... Types, size_t size, typename Func>
constexpr void For(Table<size, Types...>& table, const Container& entities, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");
	
	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t tupleSize = std::tuple_size_v<Result>;
	
	static_assert(tupleSize > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(std::index_sequence<Is...>) {
		const auto max = Size(table);
		for (const auto& entity : entities) {
			func(Get<typename std::remove_cvref_t < std::tuple_element<Is, typename traits::tuple_type>::type>>(table, entity)...);
		}
	} (std::make_index_sequence<traits::arity>{});
}

template<typename Container, typename... Types, size_t size, typename Func>
constexpr void For(SubTable<size, Types...>& table, const Container& entities, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");

	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t tupleSize = std::tuple_size_v<Result>;

	static_assert(tupleSize > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(std::index_sequence<Is...>) {
		const auto max = Size(table);
		for (const auto& entity : entities) {
			func((Get<typename std::remove_cvref_t<std::tuple_element<Is, typename traits::tuple_type>::type>>(table, entity))...);
		}
	} (std::make_index_sequence<traits::arity>{});
}

template<typename... WhereTypes, typename... Types, size_t size>
constexpr auto Slice(Table<size, Types...>& table) -> SubTable<size, WhereTypes...> {
	return SubTable<size, WhereTypes...>{ &GetArray<WhereTypes>(table)... };
}

template<typename... WhereTypes, typename... Types, size_t size>
constexpr auto Slice(SubTable<size, Types...>& table) -> SubTable<size, WhereTypes...> {
	return SubTable<size, WhereTypes...>{ GetArray<WhereTypes>(table)... };
}

template<typename... As, typename... Bs, size_t size>
constexpr auto Merge(SubTable<size, As...>& a, SubTable<size, Bs...> b) {
	using A = std::tuple<As...>;
	using B = std::tuple<Bs...>;
	using R = union_type_seq<A, B>::type;
	using UniqueResult = union_type_seq<std::tuple<>, R>::type; 

	return [&] <typename... Result>(std::tuple<Result...>) -> SubTable<Result...> {
		auto returnTable = SubTable<Result...>{ };
		// Duplicates get assigned TWICE, what is not a big problem, 
		// but what if their pointers are different? Ye... That will fuck things up LMAO 
		// TODO: Fix it
		((GetArray<As>(returnTable) = GetArray<As>(a)), ...);
		((GetArray<Bs>(returnTable) = GetArray<Bs>(b)), ...);
		return returnTable;
	} (UniqueResult{ });
}

template<typename>
struct AsTuple;

template<size_t size, typename... Ts>
struct AsTuple<Table<size, Ts...>> {
	using tuple = std::tuple<Ts...>;
};

#pragma region Main
//
//template<size_t IDValue>
//struct Trait {
//	constexpr size_t ID() { return IDValue; }
//};

struct Speed {
	float value;
};

struct Position {
	float x;
	float y;
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

using EntitySignature = std::bitset<128>;

using TextureTable = Table<64, SDL_Texture*>;
static TextureTable textureTable;

using MasterTable = Table<1024, TextureID, Position, Input, Speed, Velocity, Color>;
static MasterTable masterTable;

int main(int, char**) {
	IMG_Init(IMG_INIT_PNG);

	//using test = AsTuple<MasterTable>::tuple;

	//SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Basic Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	
	std::cout << "===GAMEPLAY DATA===" << '\n' << '\n';
	std::cout << "===MASTER TABLE===" << '\n';
	std::cout << "Allocated Memory: " << sizeof(masterTable) << " Byte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(masterTable) / 1024.0f << " Kibibyte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(masterTable) / 1024.0f / 1024.0f << " Mebibyte(s)" << '\n';
	std::cout << "===================" << '\n';
	std::cout << '\n';
	std::cout << "===TEXTURE TABLE===" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) << " Byte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) / 1024.0f << " Kibibyte(s)" << '\n';
	std::cout << "Allocated Memory: " << sizeof(textureTable) / 1024.0f / 1024.0f << " Mebibyte(s)" << '\n';
	std::cout << "===================" << '\n';
	std::cout << "===================" << '\n' << '\n';;

	EntityRange<64> shipTextures{ };
	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "Assets" / "Sprites")) {
		SDL_Texture* texture = IMG_LoadTexture(renderer, (const char*)entry.path().u8string().c_str());
		shipTextures.push_back(CreateSingle(textureTable, { texture }));
	}

	for (Entity i = 0; i < shipTextures.size(); i++) {
		float x = (i * 64) % (800 - 64);
		float y = ((i * 64) / (800 - 64)) * 64;
	
	}

	Entity player = CreateSingle(masterTable);

	auto physicsTable = Slice<Position, Speed, Velocity>(masterTable);
	auto inputTable = Slice<Input>(masterTable);

	auto renderTable = Slice<Position, Color>(masterTable);
	auto spriteRenderingTable = Slice<Position, TextureID>(masterTable);

	Set(masterTable, player, TextureID{ shipTextures[0] });
	Set(physicsTable, player, Speed{ 0.016f });
	Set(renderTable, player, Color{ 0, 255, 0, 255 });

	EntityRange<1> inputReceiver{ player };
	auto quitWatcher = [](void* data, SDL_Event* e) { 
		if (e->type == SDL_QUIT) {
			ApplicationQuit();
		}
		return 0; 
	};
	SDL_AddEventWatch(quitWatcher, nullptr);

	while (ApplicationIsRunning()) {
		SDL_PumpEvents();

		{ // Update Keyboard states
			int arrSize = 0;
			auto* keyboardState = SDL_GetKeyboardState(&arrSize);
			For(inputTable, inputReceiver, [arrSize, keyboardState](Input& input) {
				std::ignore = memcpy_s(input.prevState, sizeof(input.prevState), input.currState, sizeof(input.currState));

				auto err = memcpy_s(input.currState, sizeof(input.currState), keyboardState, arrSize);
				assert(err == 0 && "Not enough memory space in input state type");
			});
		}

		For(masterTable, inputReceiver, [&shipTextures](const Input& input, Velocity& velocity, Speed speed, TextureID& texture) {
			velocity = Velocity{ 0.0f, 0.0f };
			glm::vec2 direction{ 0.0f, 0.0f };
			if (input.isDown(SDL_SCANCODE_S)) {
				direction.y = 0.016f;
			}
			if (input.isDown(SDL_SCANCODE_W)) {
				direction.y = -0.016f;
			}
			if (input.isDown(SDL_SCANCODE_A)) {
				direction.x = -0.016f;
			}
			if (input.isDown(SDL_SCANCODE_D)) {
				direction.x = 0.016f;
			}

			if (input.justDown(SDL_SCANCODE_Q)) {
				texture.id--;
			}
			if (input.justDown(SDL_SCANCODE_E)) {
				texture.id++;
			}
			std::cout << texture.id << '\n';
			texture.id = (texture.id + shipTextures.size()) % shipTextures.size();

			if (glm::length(direction) > 0.0f) {
				direction = glm::normalize(direction);
			}
			
			velocity.x = direction.x * speed.value;
			velocity.y = direction.y * speed.value;
		});

		For(physicsTable, [](Position& pos, Velocity vel) {
			pos.x += vel.x;
			pos.y += vel.y;
		});

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);


		For(renderTable, [&renderer](Position pos, Color color) {
			SDL_FRect rect{ pos.x, pos.y, 64.0f, 64.0f };
			auto c = color.value;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_RenderDrawRectF(renderer, &rect);
		});

		For(spriteRenderingTable, [&renderer](Position pos, TextureID texture) {
			auto tex = Get<SDL_Texture*>(textureTable, texture.id);
			SDL_Rect src{ 0, 0, 64, 64 };
			SDL_FRect dst{ pos.x, pos.y, 64.0f, 64.0f };
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
