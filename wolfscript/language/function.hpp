#pragma once

#include "value_type.hpp"
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
	function_signature_types types;
	value_type::generic_function function;
	bool is_const_member;

	template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
	static generic_function_binding create(Tfunc&& pFunc, function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const>)
	{
		generic_function_binding binding;
		binding.function = pFunc;
		binding.types = function_signature_types::create<Tret, Tparams...>();
		binding.is_const_member = pIs_const;
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
auto get_param(const value_type& pValue)
{
	using type_bare = strip_bare_t<T>;
	auto ptr = std::is_const_v<std::remove_pointer_t<T>> ? pValue.get<const type_bare>() : pValue.get<type_bare>();
	if constexpr (std::is_pointer_v<T>)
		return ptr;
	else
		return std::ref(*ptr);
}

template <typename T>
auto get_param(const value_type& pValue, bool pIs_this_pointer)
{
	if (pIs_this_pointer)
	{
		// Get the app registered object from the object_behavior::object member
		auto obj = pValue.get<const value_type::object>();
		auto app_obj_iter = obj->members.find(object_behavior::object);
		if (app_obj_iter == obj->members.end())
			throw exception::interpretor_error("Cannot get object");
		return get_param<T>(*app_obj_iter);
	}
	else
		return get_param<T>(pValue);
}

template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams, std::size_t...pParams_index>
generic_function_binding make_proxy_function(Tfunc&& pFunc,
	function_signature<Tret, function_params<Tparams...>, pIs_member, pIs_const> pSig,
	std::index_sequence<pParams_index...>)
{
	auto proxy = [pFunc = std::forward<Tfunc>(pFunc)](const value_type::arg_list& pArgs) -> value_type
	{
		auto invoke = [&]()
		{
			return std::invoke(pFunc, std::forward<Tparams>(get_param<Tparams>(pArgs[pParams_index], pIs_member && pParams_index == 0))...);
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
	return generic_function_binding::create(std::move(proxy), pSig);
}

template <typename Tfunc, bool pIs_member, bool pIs_const, typename Tret, typename...Tparams>
generic_function_binding make_proxy_function(Tfunc&& pFunc,
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
detail::generic_function_binding function(this_first_t, T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using class_type = detail::first_param<traits::type::param_types>::type;

	static_assert(traits::type::param_types::size > 0, "You need to provide at least one parameter for method function");
	static_assert(std::is_pointer_v<class_type>, "First parameter of method function needs to be a pointer");

	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types, true, std::is_const_v<class_type>>;

	return detail::make_object_proxy(std::forward<T>(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
detail::generic_function_binding function(this_first_t, Tret(*pFunc)(Tparams...))
{
	static_assert(sizeof...(Tparams) > 0, "You need to provide at least one parameter for method function");
	using class_type = detail::first_param<Tparams...>::type;

	static_assert(std::is_pointer_v<class_type>, "First parameter of method function needs to be a pointer");

	using sig = detail::function_signature<Tret, detail::function_params<Tparams...>, true, std::is_const_v<class_type>>{};

	return detail::make_proxy_function(std::move(pFunc), sig{});
}

template <typename Tret, typename...Tparams>
detail::generic_function_binding function(Tret(*pFunc)(Tparams...))
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tparams...>>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...))
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<Tclass*, Tparams...>, true>{});
}

template <typename Tret, typename Tclass, typename...Tparams>
detail::generic_function_binding function(Tret(Tclass::*pFunc)(Tparams...) const)
{
	return detail::make_proxy_function(std::move(pFunc), detail::function_signature<Tret, detail::function_params<const Tclass*, Tparams...>, true, true>{});
}

template <typename T>
detail::generic_function_binding function(T&& pFunc)
{
	using traits = detail::function_signature_traits<typename decltype(&std::decay_t<T>::operator())>;
	using sig = detail::function_signature<traits::type::return_type, traits::type::param_types>;
	return detail::make_proxy_function(std::forward<T>(pFunc), sig{});
}

// Bind a member function
template <typename T, typename Tinstance>
detail::generic_function_binding function(T&& pFunc, Tinstance&& pInstance)
{
	return detail::make_proxy_function(bind_mem_fn(pFunc, std::forward<Tinstance>(pInstance)));
}

} // namespace wolfscript