#pragma once

#include "common.hpp"
#include "value_type.hpp"
#include "exception.hpp"

namespace wolfscript
{

namespace exception
{

struct arithmetic_error :
	wolf_exception
{
	arithmetic_error(const char* pMsg) :
		wolf_exception(pMsg)
	{}
};

} // namespace exception

namespace detail
{

template<typename Tl, typename Tr>
value_type arithmetic_binary_operation_impl(token_type pOp, const value_type& pL, Tl& pLval, Tr& pRval)
{
	constexpr bool l_is_bool = std::is_same<std::decay_t<Tl>, bool>::value;
	constexpr bool r_is_bool = std::is_same<std::decay_t<Tr>, bool>::value;
	// Cast the right value to the type of the left
	const Tl r_casted = static_cast<Tl>(pRval);

	switch (pOp)
	{
	case token_type::equ: return pLval == r_casted; break;
	case token_type::not_equ: return pLval != r_casted; break;
	}

	// These operations are not allowed for bool.
	// TODO: Add helpful error messages for these.
	if constexpr (!l_is_bool && !r_is_bool)
	{
		switch (pOp)
		{
		case token_type::add: return pLval + r_casted; break;
		case token_type::sub: return pLval - r_casted; break;
		case token_type::mul: return pLval * r_casted; break;
		case token_type::div:
			if (r_casted == 0)
				throw exception::arithmetic_error("Divide by 0");
			return pLval / r_casted;
			break;

		case token_type::less_than: return pLval < r_casted; break;
		case token_type::less_than_equ_to: return pLval <= r_casted; break;
		case token_type::greater_than: return pLval > r_casted; break;
		case token_type::greater_than_equ_to: return pLval >= r_casted; break;
		}
	}

	if constexpr (std::is_const<Tl>::value)
	{
		switch (pOp)
		{
		case token_type::assign:
		case token_type::add_assign:
		case token_type::sub_assign:
		case token_type::mul_assign:
		case token_type::div_assign:
			throw exception::arithmetic_error("Cannot assign to a constant value");
		}
	}
	else
	{
		switch (pOp)
		{
		case token_type::assign: pLval = r_casted; return pL;
		}

		if constexpr (!l_is_bool && !r_is_bool)
		{
			switch (pOp)
			{
			case token_type::add_assign: pLval += r_casted; return pL;
			case token_type::sub_assign: pLval -= r_casted; return pL;
			case token_type::mul_assign: pLval *= r_casted; return pL;
			case token_type::div_assign: pLval /= r_casted; return pL;
			}
		}
	}

	throw exception::arithmetic_error("Unknown operation");
}

template<class Tcallable>
auto visit_arithmetic(const value_type& pVal, Tcallable&& pCallable)
{
	void* ptr = pVal.get_data().mPtr;
	const void* ptr_c = pVal.get_data().mPtr_c;
	if (pVal.is_const())
	{
		if (pVal.get_type_info().bare_equal(typeid(bool)))
			return pCallable(*static_cast<const bool*>(ptr_c));
		if (pVal.get_type_info().bare_equal(typeid(int)))
			return pCallable(*static_cast<const int*>(ptr_c));
		if (pVal.get_type_info().bare_equal(typeid(unsigned int)))
			return pCallable(*static_cast<const unsigned int*>(ptr_c));
		if (pVal.get_type_info().bare_equal(typeid(float)))
			return pCallable(*static_cast<const float*>(ptr_c));
	}
	else
	{
		if (pVal.get_type_info().bare_equal(typeid(bool)))
			return pCallable(*static_cast<bool*>(ptr));
		if (pVal.get_type_info().bare_equal(typeid(int)))
			return pCallable(*static_cast<int*>(ptr));
		if (pVal.get_type_info().bare_equal(typeid(unsigned int)))
			return pCallable(*static_cast<unsigned int*>(ptr));
		if (pVal.get_type_info().bare_equal(typeid(float)))
			return pCallable(*static_cast<float*>(ptr));
	}

	throw exception::arithmetic_error("Unknown arithmetic type");
}

} // namespace detail

// Perform a binary operation with 2 value_type object with arithmetic types.
value_type arithmetic_binary_operation(token_type pOp, const value_type& pL, const value_type& pR)
{
	assert(pL.is_arithmetic());
	assert(pR.is_arithmetic());
	auto lvisit = [pOp, &pL, &pR](auto& pLtype)
	{
		auto rvisit = [pOp, &pL, &pLtype](auto& pRtype)
		{
			return detail::arithmetic_binary_operation_impl(pOp, pL, pLtype, pRtype);
		};
		return detail::visit_arithmetic(pR, rvisit);
	};
	return detail::visit_arithmetic(pL, lvisit);
}

// Perform a unary operation with a single value_type object with an arithmetic type.
value_type arithmetic_unary_operation(token_type pOp, const value_type& pU)
{
	assert(pU.get_type_info().is_arithmetic);
	auto lvisit = [pOp, &pU](auto& pLtype) -> value_type
	{
		using ltype_t = std::remove_reference_t<decltype(pLtype)>;
		constexpr bool signed_value = std::is_signed<ltype_t>::value;

		// Do nothing if it is a boolean
		if constexpr (std::is_same_v<ltype_t, bool>)
			return pLtype;
		else
		{
			switch (pOp)
			{
			case token_type::add:
				return pLtype;
			case token_type::sub:
				if constexpr (signed_value)
					return -pLtype;
				else
					return pLtype; // Return value unchanged if unsigned
			}

			// TODO: Add helpful message for constant assignment
			if constexpr (!std::is_const<ltype_t>::value)
			{
				switch (pOp)
				{
				case token_type::increment:
					return (++pLtype, pU);
				case token_type::decrement:
					return (--pLtype, pU);
				}
			}

			throw exception::arithmetic_error("Unknown unary token");
		}
	};
	return detail::visit_arithmetic(pU, lvisit);
}

std::string arithmetic_to_string(const value_type& pVal)
{
	assert(pVal.is_arithmetic());
	auto visitor = [](const auto& pVal) -> std::string
	{
		return std::to_string(pVal);
	};
	return detail::visit_arithmetic(pVal, visitor);
}

} // namespace wolfscript
