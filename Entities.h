#pragma once
#include<limits>
#include<array>
#include<tuple>
#include<type_traits>
#include<functional>
#include<iostream>
#include<concepts>
#include<chrono>
#include<typeinfo>
#include"bitfield.h"

#include"static_vector.hpp"

template<typename Container, typename ContentType>
concept ContainerType = requires (Container container, size_t n) {
	std::same_as<decltype(container.begin()), ContentType*>;
	std::same_as<decltype(container.end()), ContentType*>;
	std::convertible_to<decltype(container[n]), ContentType>;
};


#pragma endregion

#pragma region ECS

#pragma region Entity
using Entity = size_t;
constexpr Entity InvalidEntity = std::numeric_limits<size_t>::max();
//constexpr size_t MaxEntityCount = 1024;

using EntityContainer = std::vector<Entity>;

template<size_t size>
using EntitySignature = std::bitfield<size>;

template<typename Context>
struct TypeSafeID {
	size_t id;

	operator size_t() { return id; }
};

//template<size_t size>
//using EntityRange = itlib::static_vector<Entity, size>;

template<typename... Ts> requires (std::is_same_v<Ts, Ts>, ...)
itlib::static_vector<std::remove_cvref_t<std::tuple_element_t<0, std::tuple<Ts...>>>, sizeof...(Ts)> Range(Ts&&... elements) {
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
using Table = std::tuple<StaticDataArray<Entity, size>, StaticDataArray<EntitySignature<sizeof...(Types)>, size>, StaticDataArray<Types, size>...>;



template<typename>
struct Bitfield;

template<size_t size, typename... Types>
struct Bitfield<Table<size, Types...>> {
	using Type = EntitySignature<sizeof...(Types)>;
};

template<typename>
struct Signature;

template<size_t size, typename... Types>
struct Signature<Table <size, Types...>> {

private:
	using TableTuple = std::tuple<Types...>;

	template<size_t I>
	using TableTupleAt = std::remove_cvref_t<std::tuple_element_t<I, TableTuple>>;

	using SignatureType = EntitySignature<sizeof...(Types)>;

	template<typename... Ts>
	static consteval SignatureType Bits() {
		using namespace std;
		using Tuple = tuple<std::remove_cvref_t<Ts>...>;
		
		constexpr auto indeces = make_index_sequence<sizeof...(Types)>{};
		return [] <size_t... Is>(const index_sequence<Is...>) consteval {
			return ((SignatureType{ /* 0 or 1 */ (!!is_member_of_type_seq<TableTupleAt<Is>, Tuple>::value) } << Is) | ...);
		} (indeces);
	};

public:
	template<typename... Ts>
	struct Filter {
		using Outer = Signature<Table<size, Types...>>;
		static consteval SignatureType Bits() {
			return Outer::Bits<Ts...>();
		};
	};

	template<typename... Ts>
	struct Filter<std::tuple<Ts...>> {
		using Outer = Signature<Table<size, Types...>>;
		static consteval SignatureType Bits() {
			return Outer::Bits<Ts...>();
		};
	};
};



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

template<typename... NonExcludedTypes, typename Current, typename... Rest>
struct union_type_seq<std::tuple<NonExcludedTypes...>, std::tuple<Current, Rest...>> {
	using type =
		typename union_type_seq
		<
		std::conditional_t
		<
		is_member_of_type_seq<Current, NonExcludedTypes...>::value,
		std::tuple<NonExcludedTypes...>,
		std::tuple<Current, NonExcludedTypes...>
		>,
		std::tuple<Rest...>
		>::type;
};

template<typename, typename>
struct make_unique_seq;

template<typename... NonExcludedTypes, typename Current, typename... Rest>
struct make_unique_seq<std::tuple<NonExcludedTypes...>, std::tuple<Current, Rest...>> {
	using type =
		typename union_type_seq
		<
		std::conditional_t
		<
		is_member_of_type_seq<Current, NonExcludedTypes...>::value,
		std::tuple<NonExcludedTypes...>,
		std::tuple<Current, NonExcludedTypes...>
		>,
		std::tuple<Rest...>
		>::type;
};

template<typename... NonExcludedTypes>
struct make_unique_seq<std::tuple<NonExcludedTypes...>, std::tuple<>> {
	using type = std::tuple<NonExcludedTypes...>;
};


template<typename... NonExcludedTypes>
struct union_type_seq<std::tuple<NonExcludedTypes...>, std::tuple<>> {
	using type = std::tuple<NonExcludedTypes...>;
};

template<typename... Ts>
struct Exclude;

template<>
struct Exclude<> {
	using Type = std::tuple<>;
};

template<typename... Ts>
struct Exclude : Exclude<> {
	using Type = std::tuple<Ts...>;
};

template<typename>
struct HasExcludeList;

template<typename... Types>
struct HasExcludeList<std::tuple<Types...>> {
	static const bool value = (std::is_base_of<Exclude<>, Types>::value | ...);
};

template<typename, typename, typename>
struct RemoveExcludeLists;

template<typename... NonExcludedTypes, typename... ExcludedTypes>
struct RemoveExcludeLists<std::tuple<>, std::tuple<NonExcludedTypes...>, std::tuple<ExcludedTypes...>> {
	using Tuple = std::tuple<NonExcludedTypes...>;
	using ExcludedTuple = std::tuple<ExcludedTypes...>;
};

template<typename T, typename... Types, typename... NonExcludedTypes, typename... ExcludedTypes>
struct RemoveExcludeLists<std::tuple<T, Types...>, std::tuple<NonExcludedTypes...>, std::tuple<ExcludedTypes...>> :
	std::conditional<
	std::is_base_of<Exclude<>, T>::value,
	RemoveExcludeLists<std::tuple<Types...>, std::tuple<NonExcludedTypes...>, std::tuple<ExcludedTypes..., T>>,
	RemoveExcludeLists<std::tuple<Types...>, std::tuple<NonExcludedTypes..., T>, std::tuple<ExcludedTypes...>>
	>::type {
};


//template<typename... Ts, typename... Types, size_t size>
//constexpr EntitySignature<sizeof...(Types)> Signature(const Table<size, Types...>& table) {
//	using namespace std;
//	using TableTuple = tuple<Types...>;
//	using Tuple = tuple<Ts...>;
//
//	return [&] <size_t... Is>(std::index_sequence<Is...>) consteval {
//		return (
//		(
//			EntitySignature<sizeof...(Types)> { (!!is_member_of_type_seq<std::remove_cvref_t<std::tuple_element_t<Is, TableTuple>>, Tuple>::value) } << Is
//		) | ...);
//	} (std::make_index_sequence<sizeof...(Types)>{});
//}


template<typename GetterType, typename... Types, size_t size>
StaticDataArray<GetterType, size>& GetArray(Table<size, Types...>& table) {

	return std::get<StaticDataArray<GetterType, size>>(table);
}


template<typename GetterType, typename... Types, size_t size>
GetterType& Get(Table<size, Types...>& table, Entity entity) {

	auto& column = GetArray<GetterType>(table);
	return column[entity];
}

// Fix this....
template<typename... Types, size_t size>
EntitySignature<sizeof...(Types)>& GetSignature(Table<size, Types...>& table, Entity entity) {
	return GetArray<EntitySignature<sizeof...(Types)>>(table)[entity];
}

template<typename... HasTypes, typename... Types, size_t size>
bool Has(Table<size, Types...>& table, Entity entity) {
	auto signature = GetSignature(table, entity);
	auto testSignature = Signature<Table<size, Types...>>::template Filter<HasTypes...>::Bits();
	return (testSignature & signature) == testSignature;
}

template<typename... Types, size_t size>
void SetSignature(Table<size, Types...>& table, Entity entity, const EntitySignature<sizeof...(Types)>& signature) {
	GetArray<EntitySignature<sizeof...(Types)>>(table)[entity] = signature;
}


template<typename... Types, size_t size>
void Clear(Table<size, Types...>& table) {
	(GetArray<Types>(table).clear(), ...);
}

template<typename SetterType, typename... Types, size_t size >
void Set(Table<size, Types...>& table, Entity entity, SetterType setTo) {

	auto& column = GetArray<SetterType>(table);
	column[entity] = setTo;
}

template<typename SetterType, typename... Types, size_t size, ContainerType<Entity> Container>
void Set(Table<size, Types...>& table, Container entities, SetterType setTo) {

	auto& column = GetArray<SetterType>(table);
	for (const auto& entity : entities) {
		column[entity] = setTo;
	}
}

template<typename SetterType, typename... Types, size_t size>
void Add(Table<size, Types...>& table, Entity entity, SetterType setTo) {

	auto& column = GetArray<SetterType>(table);
	column[entity] = setTo;
	GetSignature(table, entity) |= Signature<Table<size, Types...>>::template Filter<SetterType>::Bits();
}

// TODO: Copy would be cool :) 

template<typename SetterType, typename... Types, size_t size, ContainerType<Entity> Container>
void Add(Table<size, Types...>& table, Container entities, SetterType setTo) {

	auto& column = GetArray<SetterType>(table);
	for (const auto& entity : entities) {
		column[entity] = setTo;
	}
	for (const auto& entity : entities) {
		GetSignature(table, entity) |= Signature<SetterType>(table);
	}
}

template<typename RemoveType, typename... Types, size_t size>
void Remove(Table<size, Types...>& table, Entity entity) {
	GetSignature(table, entity) &= ~Signature<RemoveType>(table);
}

template<typename RemoveType, typename... Types, size_t size, ContainerType<Entity> Container>
void Remove(Table<size, Types...>& table, Container entities) {
	for (const auto& entity : entities) {
		GetSignature(table, entity) &= ~Signature<RemoveType>(table);
	}
}

template<typename... Types, size_t size>
constexpr size_t Size(const Table<size, Types...>& table) {
	return std::get<0>(table).size();
}



template<typename... Types, size_t size>
Entity CreateSingle(Table<size, Types...>& table, const EntitySignature<sizeof...(Types)>& signature = { }) {

	[&] <size_t... Is>(std::index_sequence<Is...>) {
		(std::get<Is>(table).push_back({ }), ...);

	} (std::make_index_sequence<std::tuple_size<Table<size, Types...>>::value>{});
	const auto entity = Size(table) - 1;
	SetSignature(table, entity, signature);
	Set<Entity>(table, entity, entity);

	return entity;
}

template<typename... Types, size_t size>
void Destroy(Table<size, Types...>& table, Entity entity) {
	SetSignature(table, entity, { }); // Set 0 signature :D 
}

template<ContainerType<Entity> Container, typename... Types, size_t tableSize>
constexpr Container CreateMultiple(Table<tableSize, Types...>& table, size_t count, const EntitySignature<sizeof...(Types)>& signature = { }) {
	Container entities{ };
	for (size_t i = 0; i < count; i++) {
		entities.push_back(CreateSingle(table, signature));
	}
	return entities;
}


template<ContainerType<Entity> Container, typename... SignatureTypes, typename... Types, size_t size> 
Container Where(Table<size, Types...>& table) {
	using namespace std;
	Container container{};

	using TableTuple = tuple<Types...>;
	using PackTuple = tuple<remove_cvref_t<SignatureTypes>...>;
	using TypeFilter = RemoveExcludeLists<PackTuple, tuple<>, tuple<>>;

	using TableSignature = Signature<Table<size, Types...>>;
	constexpr auto signature = TableSignature::template Filter<typename TypeFilter::Tuple>::Bits();
	constexpr auto excludeSignature = TableSignature::template Filter<typename TypeFilter::ExcludedTuple>::Bits();

	const auto max = Size(table);
	for (size_t index = 0; index < max; index += 1) {
		auto entitySignature = GetSignature(table, index);

		if ((entitySignature & signature) == signature && !((entitySignature & excludeSignature) != 0)) {
			container.push_back(index);
		}
	}

	return container;
}

template<ContainerType<Entity> Container, typename... ExcludedTypes, typename... Types, size_t size, typename Func>
Container Where(Table<size, Types...>& table, Func func) {
	using namespace std;
	
	Container container{};
	
	using FunctionTraits = function_traits<Func>;
	using TypeFilter = RemoveExcludeLists<typename FunctionTraits::tuple_type_nocvref, tuple<>, tuple<>>;
	using FunctionTraitsTuple = typename TypeFilter::Tuple;
	using TableTuple = tuple<Types...>;
	using Result = intersect_type_seq<FunctionTraitsTuple, TableTuple>::type;

	using TableSignature = Signature<Table<size, Types...>>;
	constexpr auto signature = TableSignature::template Filter<FunctionTraitsTuple>::Bits();
	constexpr auto excludeSignature = TableSignature::template Filter<ExcludedTypes...>::Bits();
	
	// POD Vector could be even better tbh 
	itlib::static_vector<Entity, size> entities;

	const auto max = Size(table);
	for (size_t index = 0; index < max; index += 1) {
		auto entitySignature = GetSignature(table, index);

		if ((entitySignature & signature) == signature && !((entitySignature & excludeSignature) != 0)) {
			entities.push_back(index);
		}
	}

	[&] <size_t... Is> (std::index_sequence<Is...>) constexpr {
		const auto max = Size(table);
		// tuple_element_t<Is, typename FunctionTraits::tuple_type_nocvref> {}
		for (const auto& index : entities) {
			if 
			(
				func
				(
					(
						Get<tuple_element_t<Is, typename FunctionTraits::tuple_type_nocvref>>(table, index)
					)...
				)
			
			) {
				container.push_back(index);
			}
		}
	} (make_index_sequence<FunctionTraits::arity>{});
	return container;
}


template<typename... ExcludedTypes, typename... Types, size_t size, typename Func>
Entity Find(Table<size, Types...>& table, Func func) {
	using traits = function_traits<Func>;
	using A = traits::tuple_type_nocvref;
	using B = std::tuple<Types...>;
	using Result = intersect_type_seq<A, B>::type;

	// TODO: Implement signature check!
	constexpr EntitySignature<sizeof...(Types)> signature = [&] <size_t... Is>(std::index_sequence<Is...>) consteval {
		return(
			(
				EntitySignature<sizeof...(Types)> { } |
				(
					EntitySignature<sizeof...(Types)> {
			/* Condition */ (is_member_of_type_seq<std::remove_cvref_t<std::tuple_element_t<Is, B>>, A>::value)
				? 1ULL
				: 0ULL
		} << Is
					)
				) | ...);
	} (std::make_index_sequence<sizeof...(Types)>{});


	const auto excludeSignature = Signature<Table<size, Types...>>::template Filter<ExcludedTypes...>::Bits();

	itlib::static_vector<Entity, size> entities;

	const auto max = Size(table);
	for (size_t index = 0; index < max; index += 1) {
		auto entitySignature = GetSignature(table, index);

		if ((entitySignature & signature) == signature && !((entitySignature & excludeSignature) != 0)) {
			entities.push_back(index);
		}
	}

	return[&] <size_t... Is> (std::index_sequence<Is...>) constexpr {
		const auto max = Size(table);

		for (const auto& index : entities) {
			if (func((Get<std::remove_cvref_t<std::tuple_element_t<Is, typename traits::tuple_type>>>(table, index))...)) {
				return index;
			}
		}
		return InvalidEntity;
	} (std::make_index_sequence<traits::arity>{});

	return InvalidEntity;
}

template<typename... ExcludedTypes, typename... Types, size_t size, typename Func>
constexpr void For(Table<size, Types...>& table, Func func) {

	static_assert(sizeof...(Types) > 0, "No data in table");

	using FunctionTraits = function_traits<Func>;
	using FunctionTraitsTuple = FunctionTraits::tuple_type_nocvref;
	using TableTypesTuple = std::tuple<Types...>;
	using Result = intersect_type_seq<FunctionTraitsTuple, TableTypesTuple>::type;
	constexpr size_t tupleSize = std::tuple_size_v<Result>;


	using ExcludeTuple = std::tuple<ExcludedTypes...>;
	using TableSignature = Signature<Table<size, Types...>>;
	constexpr auto signature = TableSignature::template Filter<FunctionTraitsTuple>::Bits();
	constexpr auto excludeSignature = TableSignature::template Filter<ExcludedTypes...>::Bits();

	itlib::static_vector<Entity, size> entities;

	const auto count = Size(table);
	for (size_t index = 0; index < count; index += 1) {
	auto now = std::chrono::high_resolution_clock::now();
		auto entitySignature = Get<EntitySignature<sizeof...(Types)>>(table, index);

		if ((entitySignature & signature) == signature && !((entitySignature & excludeSignature) != 0)) {
			entities.push_back(index);
		}
	}

	[&] <size_t... Is> (std::index_sequence<Is...>) constexpr {
		for (auto index : entities) {
			func((Get<std::remove_cvref_t<std::tuple_element_t<Is, typename FunctionTraits::tuple_type>>>(table, index))...);
		}
	} (std::make_index_sequence<FunctionTraits::arity>{});
}


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
