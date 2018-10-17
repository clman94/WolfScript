#pragma once

#include "value_type.hpp"
#include "cast.hpp"

#include <vector>
#include <functional>
#include <queue>

namespace wolfscript
{

using arg_list = std::vector<value_type>;
using generic_function = std::function<value_type(const arg_list&)>;

// This type wraps a function type that can be called in-script
struct callable
{
	// Specifies the type of the return value.
	// If this is of type value_type, if can return any value.
	type_info return_type;
	// Specifies the type of each parameter.
	// If a parameter is of type value_type, it can take an argument of any type.
	std::vector<type_info> parameter_types;

	// If true, this callable can take any number of parameters
	bool generic_arity{ false };

	// Check if this function can be call with these parameters.
	// Returns 1 or greater if this function can be called with the specified
	// parameters.
	// The higher the value, the more "specific" the function is. This is helpful when
	// finding the best overload for a set of parameters.
	std::size_t match(const std::vector<type_info>& pParams, const cast_list& pCast_list) const
	{
		// This function can take any number of parameters
		if (generic_arity)
			return 1;

		// Check size
		if (parameter_types.size() != pParams.size())
			return 0;

		std::size_t score = 1;

		// Check each parameter
		for (std::size_t i = 0; i < parameter_types.size(); i++)
		{
			if (!pCast_list.can_cast(parameter_types[i], pParams[i]))
				return 0;
			else if (!parameter_types[i].bare_equal(type_info::create<value_type>())) // Generics are not counted in the score
				++score;
		}
		return score;
	}

	// For methods, the first parameter is the object.
	// If accessed directly by the application, you need to provide
	// the first parameter.
	generic_function function;

	// Stores the original function object converted to a generic function.
	// Allows a callable to be converted back into its original std::function
	// form without creating many more delegates affecting performance.
	value_type original_function;
};

class callable_overloader
{
public:
	// Append callables from another overloader
	void add(const callable_overloader& pOther) const
	{
		mCallables.insert(mCallables.end(), pOther.mCallables.begin(), pOther.mCallables.end());
	}

	// Add a callable to the overloader
	void add(value_type pCallable) const
	{
		assert(pCallable.get<const callable>());
		mCallables.push_back(pCallable);
	}

	// Find the best overload for this set of parameters
	const callable& find(const std::vector<type_info>& pParams, const cast_list& pCast_list) const
	{
		std::size_t max_score = 0;
		const callable* result = nullptr;
		for (auto& i : mCallables)
		{
			auto c = i.get<const callable>();
			const std::size_t score = c->match(pParams, pCast_list);
			if (score > 0 && max_score < score)
			{
				result = c;
			}
			else if (score > 0 && max_score == score)
				throw exception::interpretor_error("Ambiguous call");
		}
		if (!result)
			throw exception::interpretor_error("Cannot find overload");
		return *result;
	}

	const callable& find(const arg_list& pArgs, const cast_list& pCast_list) const
	{
		std::vector<type_info> types;
		types.reserve(pArgs.size());
		for (auto& i : pArgs)
			types.push_back(i.get_type_info());
		return find(types, pCast_list);
	}

private:
	mutable std::vector<value_type> mCallables;
};

} // namespace wolfscript
