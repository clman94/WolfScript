#pragma once

#include "value_type.hpp"
#include "arithmetic.hpp"

#include <vector>

namespace wolfscript
{

using cast_function = std::function<value_type(type_info, value_type)>;

// A list that represents the casting rules
class cast_list
{
public:
	void add(type_info pTo, type_info pFrom, cast_function pFunc, bool pExplicit = false)
	{
		cast_entry entry;
		entry.to = pTo;
		entry.from = pFrom;
		entry.func = pFunc;
		entry.explicit_cast = pExplicit;
		mEntries.push_back(entry);
	}

	// Check if this list supports the casting of 2 types
	bool can_cast(type_info pTo, type_info pFrom) const
	{
		if (pTo.is_arithmetic && pFrom.is_arithmetic)
			return true;
		return static_cast<bool>(find(pTo, pFrom));
	}

	// Find a function that can cast the 2 types. If none are found,
	// this function will return an empty function
	cast_function find(type_info pTo, type_info pFrom) const
	{
		// Check for const correctness
		bool correct_const = (pFrom.is_const && pTo.is_const) || (!pFrom.is_const && pTo.is_const) ||
			(!pFrom.is_const && !pTo.is_const);

		// Check for the same type or generic type
		if (pTo.bare_equal(type_info::create<value_type>()) ||
			(pTo.bare_equal(pFrom) && correct_const))
		{
			// Just return a mirroring function
			return [](type_info, value_type pVal) -> value_type
			{
				return pVal;
			};
		}

		if (!correct_const)
			return {};

		// Find the appropriate casting function
		for (const auto& i : mEntries)
			if (i.to.stdtypeinfo == pTo.stdtypeinfo &&
				i.from.stdtypeinfo == pFrom.stdtypeinfo)
				return i.func;
		return {};
	}

	template <typename T>
	T cast(const value_type& pFrom) const
	{
		return *cast(type_info::create<const T>(), pFrom).get<T>();
	}

	// Cast 2 values. This will throw if there is no such cast.
	value_type cast(const type_info& pTo, const value_type& pFrom) const
	{
		// Automatically cast arithmetic types
		if (pTo.is_arithmetic && pFrom.is_arithmetic())
		{
			auto visit = [&pTo](auto& pLtype) -> value_type
			{
				if (*pTo.stdtypeinfo == typeid(bool))
					return static_cast<bool>(pLtype);
				if (*pTo.stdtypeinfo == typeid(int))
					return static_cast<int>(pLtype);
				if (*pTo.stdtypeinfo == typeid(unsigned int))
					return static_cast<unsigned int>(pLtype);
				if (*pTo.stdtypeinfo == typeid(float))
					return static_cast<float>(pLtype);
			};
			return detail::visit_arithmetic(pFrom, visit);
		}
		else
		{
			cast_function caster = find(pTo, pFrom.get_type_info());
			if (!caster)
				throw exception::interpretor_error("Cannot cast type");
			return caster(pTo, pFrom);
		}
	}

private:
	struct cast_entry
	{
		bool explicit_cast{ false };
		type_info to, from;
		cast_function func;
	};
	std::vector<cast_entry> mEntries;
};

} // namespace wolfscript
