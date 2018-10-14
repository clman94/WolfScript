#pragma once

#include "type_info.hpp"
#include "exception.hpp"
#include <functional>
#include <map>
#include <any>
#include <memory>

namespace wolfscript
{

// These strings represent the names of the methods that
// define special behavior of an object.
namespace object_behavior
{

// Object
constexpr const char* copy = "__copy";
constexpr const char* object = "__object";
constexpr const char* to_string = "__to_string";

// Binary
constexpr const char* assign = "__assign";
constexpr const char* add_assign = "__add_assign";
constexpr const char* sub_assign = "__sub_assign";
constexpr const char* mul_assign = "__mul_assign";
constexpr const char* div_assign = "__div_assign";
constexpr const char* add = "__add";
constexpr const char* sub = "__sub";
constexpr const char* mul = "__mul";
constexpr const char* div = "__div";

// Unary
constexpr const char* negate = "__negate";
constexpr const char* positive = "__positive";

constexpr const char* from_token_type(token_type pType, bool pUnary = false)
{
	switch (pType)
	{
	case token_type::assign: return assign;
	case token_type::add: return pUnary ? positive : add;
	case token_type::sub: return pUnary ? negate : sub;
	case token_type::mul: return mul;
	case token_type::div: return div;
	}
	return nullptr;
}

} // namespace object_behavior

// A pretty large class the represents the entire type system
// of this scripting language.
class value_type
{
public:
	using cast_function = std::function<value_type(type_info, value_type)>;
	using arg_list = std::vector<value_type>;
	using generic_function = std::function<value_type(const arg_list&)>;

	// This type wraps a function type that can be called in-script
	struct callable
	{
		// For methods, the first parameter is the object.
		// If accessed directly by the application, you need to provide
		// the first parameter.
		generic_function function;
	};

	// Object type for script objects
	struct object
	{
		// Members are accessed through the period operator.
		std::map<std::string, value_type> members;
	};

	// A list that represents the casting rules
	class cast_list
	{
	public:
		void add(type_info pTo, type_info pFrom, cast_function pFunc)
		{
			cast_entry entry;
			entry.to = pTo;
			entry.from = pFrom;
			entry.func = pFunc;
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
			for (const auto& i : mEntries)
				if (i.to.stdtypeinfo == pTo.stdtypeinfo &&
					i.from.stdtypeinfo == pFrom.stdtypeinfo)
					return i.func;
			return {};
		}

		// Cast 2 values. This will throw if there is no such cast.
		value_type cast(const type_info& pTo, const value_type& pFrom) const
		{
			// Automatically cast arithmetic types
			if (pTo.is_arithmetic && pFrom.mData->mType_info.is_arithmetic)
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
				return pFrom.mData->visit_arithmetic(visit);
			}
			else
			{
				cast_function caster = find(pTo, pFrom.mData->mType_info);
				if (!caster)
					throw exception::interpretor_error("Cannot cast type");
				return caster(pTo, pFrom);
			}
		}

	private:
		struct cast_entry
		{
			type_info to, from;
			cast_function func;
		};
		std::vector<cast_entry> mEntries;
	};

private:
	struct data
	{
		data() :
			mType_info(type_info::create<void>()),
			mPtr_c(nullptr),
			mPtr(nullptr)
		{}

		data(const data&) = default;
		data(data&&) = default;

		template <typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, data>::value>>
		data(T&& pValue)
		{
			set(std::forward<T>(pValue));
		}

		template <typename T>
		void set(const std::shared_ptr<T>& pRef)
		{
			if constexpr (!std::is_const<T>::value)
				mPtr = static_cast<void*>(pRef.get());
			else
				mPtr = nullptr;
			mPtr_c = static_cast<const void*>(pRef.get());
			mData = std::any(std::move(pRef));
			mType_info = type_info::create<T>();
		}

		template <typename T>
		void set(const std::reference_wrapper<T>& pRef)
		{
			if constexpr (!std::is_const<T>::value)
				mPtr = static_cast<void*>(&pRef.get());
			else
				mPtr = nullptr;
			mPtr_c = static_cast<const void*>(&pRef.get());
			mData = std::any(pRef);
			mType_info = type_info::create<std::add_lvalue_reference_t<T>>();
		}

		// Store the pointer as a reference
		template <typename T>
		void set(T* pPtr)
		{
			if constexpr (!std::is_const<T>::value)
				mPtr = pPtr;
			else
				mPtr = nullptr;
			mPtr_c = pPtr;
			mData = std::any(std::ref(*pPtr));
			mType_info = type_info::create<std::add_pointer_t<T>>();
		}

		// Create a unique copy of the value
		template <typename T>
		void set(T pCopy)
		{
			auto ptr = std::make_shared<T>(pCopy);
			mPtr = ptr.get();
			mPtr_c = mPtr;
			mData = std::any(std::move(ptr));
			mType_info = type_info::create<T>();
		}

		template <typename T>
		bool has_type() const
		{
			return mType_info.bare_equal(type_info::create<T>());
		}

		template<class Tcallable>
		auto visit_arithmetic(Tcallable&& pCallable) const
		{
			if (mType_info.is_const)
			{
				if (*mType_info.stdtypeinfo == typeid(bool))
					return pCallable(*static_cast<const bool*>(mPtr_c));
				if (*mType_info.stdtypeinfo == typeid(int))
					return pCallable(*static_cast<const int*>(mPtr_c));
				if (*mType_info.stdtypeinfo == typeid(unsigned int))
					return pCallable(*static_cast<const unsigned int*>(mPtr_c));
				if (*mType_info.stdtypeinfo == typeid(float))
					return pCallable(*static_cast<const float*>(mPtr_c));
			}
			else
			{
				if (*mType_info.stdtypeinfo == typeid(bool))
					return pCallable(*static_cast<bool*>(mPtr));
				if (*mType_info.stdtypeinfo == typeid(int))
					return pCallable(*static_cast<int*>(mPtr));
				if (*mType_info.stdtypeinfo == typeid(unsigned int))
					return pCallable(*static_cast<unsigned int*>(mPtr));
				if (*mType_info.stdtypeinfo == typeid(float))
					return pCallable(*static_cast<float*>(mPtr));
			}

			throw exception::interpretor_error("Cannot cast arithmetic");
		}
		type_info mType_info;
		void* mPtr;
		const void* mPtr_c;
		std::any mData;
	};

public:
	value_type()
	{
		mData = std::make_shared<data>();
	}

	value_type(const value_type& pCopy) = default;
	value_type(value_type&& pCopy) = default;

	template <typename T, typename = std::enable_if_t<!std::is_same<std::decay_t<T>, value_type>::value>>
	value_type(T&& pValue)
	{
		mData = std::make_shared<data>(std::forward<T>(pValue));
	}

	value_type& operator=(value_type pCopy)
	{
		std::swap(mData, pCopy.mData);
		return *this;
	}

	template <typename T>
	static value_type create_const(const T&& pValue)
	{
		value_type result(std::forward<std::add_const_t<T>>(pValue));
		result.mData->mType_info.is_const = true;
		return result;
	}

	const type_info& get_type_info() const
	{
		return mData->mType_info;
	}

	std::string to_string() const
	{
		if (*mData->mType_info.stdtypeinfo == typeid(object))
		{
			const object* obj = get<const object>();
			auto f = obj->members.find(object_behavior::to_string);
			auto c = f->second.get<const callable>();
			return *c->function({ *this }).get<const std::string>();
		}
		else if (mData->mType_info.is_arithmetic)
		{
			auto arithmeticvisit = [](const auto& pVal) -> std::string
			{
				return std::to_string(pVal);
			};
			return mData->visit_arithmetic(arithmeticvisit);
		}
		return{};
	}

	value_type operator+(const value_type& pR) const
	{
		return binary_operation(token_type::add, *this, pR);
	}

	value_type operator-(const value_type& pR) const
	{
		return binary_operation(token_type::sub, *this, pR);
	}

	value_type operator*(const value_type& pR) const
	{
		return binary_operation(token_type::mul, *this, pR);
	}

	value_type operator/(const value_type& pR) const
	{
		return binary_operation(token_type::div, *this, pR);
	}

	bool operator==(const value_type& pR) const
	{
		return *binary_operation(token_type::equ, *this, pR).get<const bool>();
	}

	template <typename T>
	T* get() const
	{
		if (!mData->has_type<T>())
			return nullptr;
		if (!std::is_const<T>::value && mData->mType_info.is_const)
			return nullptr;
		if constexpr (std::is_const<T>::value)
			return static_cast<const T*>(mData->mPtr_c);
		else
			return static_cast<T*>(mData->mPtr);
	}

	template <typename T>
	T get(const cast_list& pCaster) const
	{
		return *pCaster.cast(type_info::create<T>(), *this).get<const T>();
	}

	void clear()
	{
		std::swap(*this, value_type{});
	}

	// Copy this object's value
	value_type copy() const
	{
		if (auto obj = get<const object>())
		{
			// Use the copy overload to copy the value of the object
			auto copy_overload = obj->members.find(object_behavior::copy);
			if (copy_overload != obj->members.end())
			{
				auto func = copy_overload->second.get<const callable>();
				return func->function({ *this });
			}
			else
			{
				// Recursively copy all members of this object
				object new_object;
				for (const auto& i : obj->members)
					new_object.members[i.first] = i.second.copy();
				return std::move(new_object);
			}
		}
		else if (auto func = get<const callable>())
		{
			// Just copy the callable
			return *func;
		}
		else if (mData->mType_info.is_arithmetic)
		{
			// Copy the value of the arithmetic type
			auto visit = [](const auto& pVal) -> value_type
			{
				return pVal;
			};
			return mData->visit_arithmetic(visit);
		}
		else
		{
			// Unknown data type
			throw exception::interpretor_error("Cannot copy value");
		}
	}

	// Cast the value to a type
	template <typename T>
	value_type cast(const cast_list& pCaster) const
	{
		return pCaster.cast(type_info::create<T>(), *this);
	}

	// Creates a new value_type pointing to the same stored value
	// but in a seperate data instance. Optional parameter to make
	// the new reference constant. This is helpful when casting
	// an object, too.
	value_type create_unique_reference(bool mMake_const = false) const
	{
		auto new_data = std::make_shared<data>(*mData);
		new_data->mType_info.is_const |= mMake_const;

		value_type new_value_type;
		new_value_type.mData = new_data;
		return new_value_type;
	}

public:
	static value_type binary_operation(token_type pOp, const value_type& pL, const value_type& pR)
	{
		bool l_is_arithmetic = pL.mData->mType_info.is_arithmetic;
		bool r_is_arithmetic = pR.mData->mType_info.is_arithmetic;
		if (l_is_arithmetic)
		{
			auto lvisit = [pOp, &pL, &pR, r_is_arithmetic](auto& pLtype)
			{
				// Do built-in arithmetic operation
				if (r_is_arithmetic)
				{
					auto rvisit = [pOp, &pL, &pLtype](auto& pRtype)
					{
						return arithmetic_binary_operation_impl(pOp, pL, pLtype, pRtype);
					};
					return pR.mData->visit_arithmetic(rvisit);
				}
			};
			return pL.mData->visit_arithmetic(lvisit);
		}
		else if (auto obj = pL.get<object>())
		{
			const char* behavior_name = object_behavior::from_token_type(pOp);
			if (!behavior_name)
				throw exception::interpretor_error("Unsupported object behavior");

			auto member_iter = obj->members.find(behavior_name);
			if (member_iter == obj->members.end())
				throw exception::interpretor_error("Cannot find behavior method for object");

			auto member_callable = member_iter->second.get<const callable>();
			return member_callable->function({ pL, pR });
		}
		else
			throw exception::interpretor_error("No implementation to perform binary operation");
	}

	static value_type unary_operation(token_type pOp, const value_type& pU)
	{
		if (pU.mData->mType_info.is_arithmetic)
		{
			auto lvisit = [pOp, &pU](const auto& pLtype) -> value_type
			{
				typedef decltype(pLtype) ltype_t;
				if constexpr (!std::is_unsigned<std::decay<ltype_t>::type>::value)
				{
					switch (pOp)
					{
					case token_type::add:
						return +pLtype;
					case token_type::sub:
						return -pLtype;
					default:
						throw exception::interpretor_error("Unknown unary token");
					}
				}
				else
					return pLtype;

			};
			return pU.mData->visit_arithmetic(lvisit);
		}
		else if (auto obj = pU.get<object>())
		{
			const char* behavior_name = object_behavior::from_token_type(pOp);
			if (!behavior_name)
				throw exception::interpretor_error("Unsupported object behavior");

			auto member_iter = obj->members.find(behavior_name);
			if (member_iter == obj->members.end())
				throw exception::interpretor_error("Cannot find behavior method for object");

			auto member_callable = member_iter->second.get<const callable>();
			return member_callable->function({ pU });
		}
		else
			throw exception::interpretor_error("No implementation to perform unary operation");
	}

private:
	template<typename Tl, typename Tr>
	static value_type arithmetic_binary_operation_impl(token_type pOp, const value_type& pL, Tl& pLval, Tr& pRval)
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
		// TODO: Add helpful error messages for these (probably when the semantic analyzer is implemented).
		if constexpr (!l_is_bool && !r_is_bool)
		{
			switch (pOp)
			{
			case token_type::add: return pLval + r_casted; break;
			case token_type::sub: return pLval - r_casted; break;
			case token_type::mul: return pLval * r_casted; break;
			case token_type::div:
				if (r_casted == 0)
					throw exception::interpretor_error("Divide by 0");
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
				throw exception::interpretor_error("Cannot assign to a constant value");
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

		throw exception::interpretor_error("Unknown operation");
	}

private:
	std::shared_ptr<data> mData;
};

} // namespace wolfscript