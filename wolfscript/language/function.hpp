#pragma once

#include "value_type.hpp"
#include "callable.hpp"
#include "common.hpp"
#include <functional>

namespace wolfscript
{

namespace detail
{

template <typename...Tparams>
struct function_params
{
	constexpr static const std::size_t size = sizeof...(Tparams);
};

template <typename Tret, typename Tparams, bool Tis_member = false, bool Tis_const = false>
struct function_signature
{
	using return_type = Tret;
	using param_types = Tparams;
	constexpr static const bool is_member = Tis_member;
	constexpr static const bool is_const = Tis_const;
};

template <typename T, typename...Trest>
struct first_param
{
	using type = T;
	using rest = function_params<Trest...>;
};

template <typename T, typename...Trest>
struct first_param<function_params<T, Trest...>>
{
	using type = T;
	using rest = function_params<Trest...>;
};

template <typename T>
struct function_signature_traits {};

template <typename Tret, typename...Tparams>
struct function_signature_traits<Tret(*)(Tparams...)>
{
	using type = function_signature<Tret, function_params<Tparams...>>;
};

template <typename Tret, typename Tclass, typename...Tparams>
struct function_signature_traits<Tret(Tclass::*)(Tparams...)>
{
	using class_type = Tclass;
	using type = function_signature<Tret, function_params<Tparams...>, true>;
};


template <typename Tret, typename Tclass, typename...Tparams>
struct function_signature_traits<Tret(Tclass::*)(Tparams...) const>
{
	using class_type = std::add_const_t<Tclass>;
	using type = function_signature<Tret, function_params<Tparams...>, true, true>;
};

struct function_signature_types
{
	type_info return_type;
	std::vector<type_info> param_types;

	template <typename Tret, typename...Tparams>
	static function_signature_types create()
	{
		function_signature_types types;
		types.return_type = type_info::create<Tret>();
		types.param_types = { type_info::create<Tparams>()... };
		return types;
	}

	template <bool Tis_member, bool Tis_const, typename Tret, typename...Tparams>
	static function_signature_types create(function_signature<Tret, function_params<Tparams...>, Tis_member, Tis_const>)
	{
		return create<Tret, Tparams...>();
	}
};

struct generic_function_binding
{
	callable function;

	template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
	static generic_function_binding create(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const>)
	{
		generic_function_binding binding;
		binding.function.function = pFunc;
		const auto types = function_signature_types::create<Tret, Tparams...>();
		binding.function.return_type = types.return_type;
		binding.function.parameter_types = types.param_types;
		return binding;
	}
};
struct generic_constructor_binding :
	generic_function_binding
{
	generic_constructor_binding() = default;
	generic_constructor_binding(const generic_constructor_binding&) = default;
	generic_constructor_binding(generic_constructor_binding&&) = default;
	generic_constructor_binding(const generic_function_binding& pB) :
		generic_function_binding(pB)
	{}
	generic_constructor_binding(generic_function_binding&& pB) :
		generic_function_binding(pB)
	{}
};
struct generic_destructor_binding :
	generic_function_binding
{
	generic_destructor_binding() = default;
	generic_destructor_binding(const generic_destructor_binding&) = default;
	generic_destructor_binding(generic_destructor_binding&&) = default;
	generic_destructor_binding(const generic_function_binding& pB) :
		generic_function_binding(pB)
	{}
	generic_destructor_binding(generic_function_binding&& pB) :
		generic_function_binding(pB)
	{}
};

template <typename T>
auto get_arg(const value_type& pValue)
{
	if constexpr (std::is_same_v<strip_bare_t<T>, value_type>)
	{
		return std::forward<decltype(pValue)>(pValue);
	}
	else
	{
		constexpr bool is_const_value = std::is_const_v<std::remove_reference_t<std::remove_pointer_t<T>>>
			|| (!std::is_reference_v<T> && !std::is_pointer_v<T>);
		using type = add_const_cond_t<is_const_value, strip_bare_t<T>>;

		type* ptr = pValue.get<type>();
		if (!ptr)
			throw exception::interpretor_error("Invalid argument");

		if constexpr (std::is_pointer_v<T>)
			return ptr;
		else if constexpr (std::is_lvalue_reference_v<T>)
			return std::ref(*ptr);
		else
			return *pValue.get<const strip_bare_t<T>>();
	}
}

template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams, std::size_t...pParams_index>
callable make_proxy_function(Tfunc&& pFunc,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig,
	std::index_sequence<pParams_index...>)
{
	auto proxy = [pFunc = std::forward<Tfunc>(pFunc)](const arg_list& pArgs) -> value_type
	{
		auto invoke = [&]()
		{
			return std::invoke(pFunc, std::forward<Tparams>(get_arg<Tparams>(pArgs[pParams_index]))...);
		};

		if constexpr (std::is_same_v<Tret, void>)
		{
			invoke();
			return{}; // Return void type
		}
		else
		{
			return invoke();
		}
	};
	return generic_function_binding::create(std::move(proxy), pSig).function;
}

template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
callable make_proxy_function(Tfunc&& pFunc,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig)
{
	return make_proxy_function(std::forward<Tfunc>(pFunc), pSig, std::index_sequence_for<Tparams...>{});
}

} // namespace detail

// The first parameter will recieve the "this" pointer.
// First parameter must be pointer. If the first parameter
// is const, the function will be registered as a "const" method.
struct this_first_t {};
constexpr const this_first_t this_first;

template <typename T>
callable function(this_first_t, T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using class_type = detail::first_param<traits::type::param_types>::type;

	static_assert(traits::type::param_types::size > 0, "You need to provide at least one parameter for a standalone method");
	static_assert(std::is_pointer_v<class_type>, "First parameter of a standalone method needs to be a pointer");

	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types, true, std::is_const_v<class_type>>;

	return detail::make_proxy_function(std::forward<T>(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
callable function(this_first_t, Tret(*pFunc)(Tparams...))
{
	static_assert(sizeof...(Tparams) > 0, "You need to provide at least one parameter for a standalone method");
	using class_type = detail::first_param<Tparams...>::type;

	static_assert(std::is_pointer_v<class_type>, "First parameter of a standalone method needs to be a pointer");

	using sig = detail::function_signature<Tret, detail::function_params<Tparams...>, true, std::is_const_v<class_type>>{};

	return detail::make_proxy_function(std::move(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
callable function(Tret(*pFunc)(Tparams...))
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tparams...>>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
callable function(Tret(Tclass::*pFunc)(Tparams...))
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tclass*, Tparams...>, true>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
callable function(Tret(Tclass::*pFunc)(Tparams...) const)
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<const Tclass*, Tparams...>, true, true>{});
}

template <typename T>
callable function(T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types>;
	return detail::make_proxy_function(std::forward<T>(pFunc), sig{});
}

namespace detail
{

template <typename Tfuncsig, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams, std::size_t...pParams_index>
std::function<Tfuncsig> make_function(
	const callable& pCallable,
	const cast_list& pCaster,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig,
	std::index_sequence<pParams_index...>)
{
	return [pCallable, &pCaster](Tparams...pArgs) -> Tret
	{
		return pCallable.function({ pCaster.cast(pCallable.parameter_types[pParams_index], pArgs)... });
	};
}

template <typename Tfuncsig, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
std::function<Tfuncsig> make_function(
	const callable& pCallable,
	const cast_list& pCaster,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig)
{
	return make_function(pCallable, pCaster, pSig, std::index_sequence_for<Tparams...>{});
}

} // namespace detail

template <typename T>
std::function<T> from_callable(const callable& pCallable, const cast_list& pCaster)
{
	// Function was originally constructed with this signature
	if (auto orig = pCallable.original_function.get<std::function<T>>())
		return *orig;

	// Check types
	if (!pCallable.match(types.param_types, pCaster))
		return{};

	// Create a proxy function
	using traits = detail::function_signature_traits<T*>;
	auto types = detail::function_signature_types::create(traits{});
	return detail::make_function<T>(pCallable, pCaster, traits::type);
}

} // namespace wolfscript
