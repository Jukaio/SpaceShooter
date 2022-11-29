
#include<limits>
#include<array>
#include<tuple>
#include<type_traits>
#include<functional>
#include<iostream>
#include<concepts>

#include"bitfield.h"
#include<filesystem>
#include<glm/glm.hpp>

#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>

#include"static_vector.hpp"


// Compile time Counter https://stackoverflow.com/questions/6166337/does-c-support-compile-time-counters
template< size_t n > // This type returns a number through function lookup.
struct cn // The function returns cn<n>.
{
	char data[n + 1];
}; // The caller uses (sizeof fn() - 1).

template< typename id, size_t n, size_t acc >
cn< acc > seen(id, cn< n >, cn< acc >); // Default fallback case.

/* Evaluate the counter by finding the last defined overload.
   Each function, when defined, alters the lookup sequence for lower-order
   functions. */
#define counter_read( id ) \
( sizeof seen( id(), cn< 1 >(), cn< \
( sizeof seen( id(), cn< 2 >(), cn< \
( sizeof seen( id(), cn< 4 >(), cn< \
( sizeof seen( id(), cn< 8 >(), cn< \
( sizeof seen( id(), cn< 16 >(), cn< \
( sizeof seen( id(), cn< 32 >(), cn< 0 \
/* Add more as desired; trimmed for Stack Overflow code block. */ \
                      >() ).data - 1 ) \
                      >() ).data - 1 ) \
                      >() ).data - 1 ) \
                      >() ).data - 1 ) \
                      >() ).data - 1 ) \
                      >() ).data - 1 )

   /* Define a single new function with place-value equal to the bit flipped to 1
	  by the increment operation.
	  This is the lowest-magnitude function yet undefined in the current context
	  of defined higher-magnitude functions. */
#define counter_inc( id ) \
cn< counter_read( id ) + 1 > \
seen( id, cn< ( counter_read( id ) + 1 ) & ~ counter_read( id ) >, \
          cn< ( counter_read( id ) + 1 ) & counter_read( id ) > )

struct my_cnt {};

//int const a = counter_read(my_cnt);
//counter_inc(my_cnt);
//counter_inc(my_cnt);
//counter_inc(my_cnt);
//counter_inc(my_cnt);
//counter_inc(my_cnt);
//
//int const b = counter_read(my_cnt);
//
//counter_inc(my_cnt);

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

template<size_t size>
using EntitySignature = std::bitfield<size>;

template<typename Context>
struct TypeSafeID {
	size_t id;

	operator size_t() { return id; }
};

template<size_t size>
using EntityRange = itlib::static_vector<Entity, size>;

template<typename... Ts> requires (std::is_same_v<Ts, Ts>, ...)
itlib::static_vector<std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Ts...>>>, sizeof...(Ts)> MakeRange(Ts&&... elements) {
	//EntityRange<sizeof...(T)> range{ elements... };
	//(range.push_back(indeces), ...);
	using namespace std;
	using namespace itlib;
	return static_vector<remove_cvref_t<tuple_element_t<0, tuple<Ts...>>>, sizeof...(Ts)> { elements... };
}
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
//using Table = std::tuple<StaticDataArray<Types, size>...>;
using Table = std::tuple<StaticDataArray<EntitySignature<sizeof...(Types)>, size>, StaticDataArray<Types, size>...>;
// Requires TABLE as context for signature :I 
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

template<typename T, typename... Ts>
struct is_member_of_type_seq<T, std::tuple<Ts...>>
{
	static const bool value = is_member_of_type_seq<T, Ts...>::value;
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

// Fix this....
template<typename... Types, size_t size>
EntitySignature<sizeof...(Types)>& GetSignature(Table<size, Types...>& table, Entity entity) {
	return std::get<0>(table)[entity];
}

template<typename... Types, size_t size>
void SetSignature(Table<size, Types...>& table, Entity entity, const EntitySignature<sizeof...(Types)>& signature) {
	std::get<0>(table)[entity] = signature;
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

template<typename... Ts, typename... Types, size_t size>
consteval EntitySignature<sizeof...(Types)> Signature(const Table<size, Types...>& table) {
	using namespace std;
	using TableTuple = tuple<Types...>;
	using Tuple = tuple<Ts...>;

	static_assert(sizeof...(Types) > 0, "No data in table");

	return [&] <size_t... Is>(std::index_sequence<Is...>) consteval {
		return	(
		(EntitySignature<sizeof...(Types)> { } |
			(
				EntitySignature<sizeof...(Types)> {
					/* Condition */ (is_member_of_type_seq<std::remove_cvref_t<std::tuple_element_t<Is, TableTuple>>, Tuple>::value)
					? 1ULL 
					: 0ULL 
				} << Is
			)
		) | ...);
	} (std::make_index_sequence<sizeof...(Types)>{});
}


template<typename... Types, size_t size>
Entity CreateSingle(Table<size, Types...>& table, const EntitySignature<sizeof...(Types)>& signature = { }) {
	
	// Iterate and create for all
	// [0] == signatures 
	[&] <size_t... Is>(std::index_sequence<Is...>) {
		(std::get<Is>(table).push_back({ }), ...);

	} (std::make_index_sequence<std::tuple_size<Table<size, Types...>>::value>{});
	const auto entity = Size(table) - 1;
	SetSignature(table, entity, signature);
	//(std::get<StaticDataArray<Types, size>>(table).push_back(std::get<Types>(data)), ...);
	return entity;
}

//template<typename... Types>
//Entity CreateSingle(ComponentTable<Types...>& table) {
//	(std::get<ComponentArray<Types>>(table).push_back({ }), ...);
//	return Size(table) - 1;
//}

template<size_t size, typename... Types, size_t tableSize>
constexpr EntityRange<size> CreateMultiple(Table<tableSize, Types...>& table, size_t count, const EntitySignature<sizeof...(Types)>& signature = { }) {
	EntityRange<size> entities { };
	for (size_t i = 0; i < count; i++) {
		entities.push_back(CreateSingle(table, signature));
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


	// TODO: Implement signature check!
	constexpr EntitySignature<sizeof...(Types)> signature = [&] <size_t... Is>(std::index_sequence<Is...>) consteval {
		return	(
			(EntitySignature<sizeof...(Types)> { } |
				(
					EntitySignature<sizeof...(Types)> {
			/* Condition */ (is_member_of_type_seq<std::remove_cvref_t<std::tuple_element_t<Is, B>>, A>::value)
				? 1ULL
				: 0ULL
		} << Is
					)
				) | ...);
	} (std::make_index_sequence<sizeof...(Types)>{});

	std::cout << signature.to_string() << '\n';
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

//using EntitySignature = std::bitset<128>;

using TextureTable = Table<64, SDL_Texture*>;
static TextureTable textureTable;

using GameplayTable = Table<1024, TextureID, Position, Input, Speed, Velocity, Color>;
static GameplayTable gameplayTable;


int main(int, char**) {
	IMG_Init(IMG_INIT_PNG);

	//SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
	SDL_Window* window = SDL_CreateWindow("Basic Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	constexpr auto TextureIDID = Signature<TextureID>(gameplayTable);
	constexpr auto PositionID = Signature<Position>(gameplayTable);
	constexpr auto InputID = Signature<Input>(gameplayTable);
	constexpr auto SpeedID = Signature<Speed>(gameplayTable);
	constexpr auto VelocityID = Signature<Velocity>(gameplayTable);

	constexpr auto playerSignature = Signature<TextureID, Position, Input, Speed, Velocity, Color>(gameplayTable);
	constexpr auto staticObjectSignature = Signature<Position, TextureID>(gameplayTable);
	constexpr auto dynamicObjectSignature = Signature<Position, TextureID, Velocity>(gameplayTable);


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

	EntityRange<64> shipTextures{ };
	for (auto const& entry : std::filesystem::directory_iterator(std::filesystem::current_path() / "Assets" / "Sprites")) {
		SDL_Texture* texture = IMG_LoadTexture(renderer, (const char*)entry.path().u8string().c_str());
		auto entity = CreateSingle(textureTable);
		Set(textureTable, entity, texture);
		shipTextures.push_back(entity);
	}

	for (Entity i = 0; i < shipTextures.size(); i++) {
		float x = (i * 64) % (800 - 64);
		float y = ((i * 64) / (800 - 64)) * 64;
	
	}

	auto player = CreateSingle(gameplayTable, playerSignature);

	auto physicsTable = Slice<Position, Speed, Velocity>(gameplayTable);
	auto inputTable = Slice<Input>(gameplayTable);


	auto renderTable = Slice<Position, Color>(gameplayTable);
	auto spriteRenderingTable = Slice<Position, TextureID>(gameplayTable);

	Set(gameplayTable, player, TextureID{ shipTextures[0] });
	Set(physicsTable, player, Speed{ 0.016f });
	Set(renderTable, player, Color{ 0, 255, 0, 255 });

	auto players = MakeRange(player);

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
			For(inputTable, players, [arrSize, keyboardState](Input& input) {
				std::ignore = memcpy_s(input.prevState, sizeof(input.prevState), input.currState, sizeof(input.currState));

				auto err = memcpy_s(input.currState, sizeof(input.currState), keyboardState, arrSize);
				assert(err == 0 && "Not enough memory space in input state type");
			});
		}

		For(gameplayTable, players, [&shipTextures](const Input& input, Velocity& velocity, Speed speed, TextureID& texture) {
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

		For(gameplayTable, [](Position& pos, Velocity vel) {
			pos.x += vel.x;
			pos.y += vel.y;
		});

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);


		For(gameplayTable, [&renderer](Position pos, Color color) {
			SDL_FRect rect{ pos.x, pos.y, 64.0f, 64.0f };
			auto c = color.value;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_RenderDrawRectF(renderer, &rect);
		});

		For(gameplayTable, [&renderer](Position pos, TextureID texture) {
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
