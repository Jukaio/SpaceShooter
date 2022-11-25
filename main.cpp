
#include<limits>
#include<array>
#include<tuple>
#include<type_traits>
#include<functional>
#include<iostream>
#include<concepts>
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

template<typename... Types>
using ComponentSpan = std::tuple<ComponentArray<Types>*...>;

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
	using type = typename std::conditional<
		!is_member_of_type_seq<T, Us...>::value,
		typename intersect_type_seq<
		std::tuple<Ts...>,
		std::tuple<Us...>>
		::type,
		typename prepend_to_type_seq<
		T,
		typename intersect_type_seq<
		std::tuple<Ts...>,
		std::tuple<Us...>
		>::type
		>::type
		>::type;
};

template<typename GetterType, typename... Types>
ComponentArray<GetterType>& Get(ComponentTable<Types...>& table) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	return std::get<ComponentArray<GetterType>>(table);
}

template<typename GetterType, typename... Types>
ComponentArray<GetterType>*& Get(ComponentSpan<Types...>& table) {
	static_assert((std::is_same_v<GetterType, Types> || ...) && "Table does not contain Type");
	return std::get<ComponentArray<GetterType>*>(table);
}


template<typename SetterType, typename... Types>
void Set(ComponentTable<Types...>& table, Entity entity, SetterType setTo) {
	static_assert((std::is_same_v<SetterType, Types> || ...) && "Table does not contain Type");
	auto& column = std::get<ComponentArray<SetterType>>(table);
	column[entity] = setTo;
}

template<typename... Types>
constexpr size_t Size(const ComponentTable<Types...>& table) {
	return std::get<0>(table).size(); // If this fails you do not have a table
}

template<typename... Types>
constexpr size_t Size(const ComponentSpan<Types...>& span) {
	return std::get<0>(span)->size(); // If this fails you do not have a table
}

template<typename... Types>
Entity Create(ComponentTable<Types...>& table, std::tuple<Types...> data) {
	(std::get<ComponentArray<Types>>(table).push_back(std::get<Types>(data)), ...);

	return Size(table) - 1;
}

template<typename... Types>
Entity Create(ComponentTable<Types...>& table) {
	const std::tuple<Types...> tuple { };
	(std::get<ComponentArray<Types>>(table).push_back(tuple), ...);
	return Size(table) - 1;
}

//template<size_t size, typename... Types>
//void For(ComponentTable<Types...>& table, EntityRange<size> entityRange, std::function<void(Types...)> func) {
//	const size_t max = Size(table);
//
//	for (size_t entity = 0; entity < max; entity += 1) {
//		(func(std::get<ComponentArray<std::remove_cvref_t<Types>>>(table)[entity]...));
//	}
//}

template<typename... Types, typename Func> 
constexpr void For(ComponentTable<Types...>& table, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");

	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t size = std::tuple_size_v<Result>;

	static_assert(size > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(auto&& t, std::index_sequence<Is...>) {
		const auto max = Size(table);

		for (size_t index = 0; index < max; index += 1) {
			func(std::get<ComponentArray<std::remove_cvref_t<std::tuple_element<Is, typename traits::tuple_type>::type>>>(t)[index]...);
		}
	} (table, std::make_index_sequence<traits::arity>{});
}

template<typename... Types, typename Func>
constexpr void For(ComponentSpan<Types...>& table, Func func) {
	static_assert(sizeof...(Types) > 0, "No data in table");

	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;
	constexpr size_t size = std::tuple_size_v<Result>;

	static_assert(size > 0, "No Valid parameters in Function");

	[&] <size_t... Is>(auto && t, std::index_sequence<Is...>) {
		const auto max = Size(table);

		for (size_t index = 0; index < max; index += 1) {
			func((*std::get<ComponentArray<std::remove_cvref_t<std::tuple_element<Is, typename traits::tuple_type>::type>>*>(t))[index]...);
		}
	} (table, std::make_index_sequence<traits::arity>{});
}

//
//template<const size_t size, typename... Types, typename Func>
//constexpr void For(EntityRange<size> entityContainer, ComponentTable<Types...>& table, Func func) {
//	static_assert(sizeof...(Types) > 0, "No data in table");
//
//	using traits = function_traits<Func>;
//	using A = traits::tuple_type_nocvref;
//	using B = std::tuple<Types...>;
//	using Result = intersect_type_seq<A, B>::type;
//	constexpr size_t size = std::tuple_size_v<Result>;
//
//	static_assert(size > 0, "No Valid parameters in Function");
//
//	[&] <size_t... Is>(auto && t, std::index_sequence<Is...>) {
//		for (const auto& entity : entityContainer) {
//			func(std::get<ComponentArray<std::remove_cvref_t<std::tuple_element<Is, typename traits::tuple_type>::type>>>(t)[entity]...);
//		}
//
//	} (table, std::make_index_sequence<traits::arity>{});
//}

template<typename... WhereTypes, typename... Types>
constexpr auto Where(ComponentTable<Types...>& table) -> ComponentSpan<WhereTypes...> {
	//std::tuple<ComponentArray<WhereTypes>*...> returnTuple;
	//(std::get<ComponentArray<WhereTypes>*>(returnTuple) = std::get<ComponentArray<WhereTypes>>(table), ...);
	return ComponentSpan<WhereTypes...>{ &std::get<ComponentArray<WhereTypes>>(table)... };
	//return std::tuple(&std::get<ComponentArray<WhereTypes>>(table)...);
}

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

static ComponentTable<Position, Speed, Velocity, Color> masterTable;

int main(int, char**) {
	SDL_Window* window = SDL_CreateWindow("Basic Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 640, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);

	// Just in case we need player later, let's store it : ) 
	Entity player = Create(masterTable, { });
	Create(masterTable, { });
	Create(masterTable, { });
	Create(masterTable, { });
	Create(masterTable, { });

	auto physicsTable = Where<Position, Speed, Velocity>(masterTable);
	auto renderTable = Where<Position, Color>(masterTable);

	// Setup colours
	For(masterTable, [](Color& color) {
		color.value = {255, 255, 255, 255};
	});
	
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

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		For(physicsTable, [](Speed speed, Velocity& vel) {
			vel.x += speed.value;
			vel.y += speed.value;
			});


		For(physicsTable, [](Position& pos, Velocity vel) {
			pos.x += vel.x;
			pos.y += vel.y;
			});

		For(renderTable, [&renderer](Position pos, Color color) {
			SDL_FRect rect{ pos.x, pos.y, 32.0f, 32.0f };
			auto c = color.value;
			SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, c.a);
			SDL_RenderFillRectF(renderer, &rect);
			});

		// Drawing here

		SDL_RenderPresent(renderer);
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	return 0;
}
#pragma endregion
