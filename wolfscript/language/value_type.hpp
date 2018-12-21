#pragma once

#include "type_info.hpp"
#include "exception.hpp"
#include <functional>
#include <map>
#include <any>
#include <memory>

namespace wolfscript
{

// These strings represent the names of the functions that
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
	case token_type::add_assign: return add_assign;
	case token_type::sub_assign: return sub_assign;
	case token_type::mul_assign: return mul_assign;
	case token_type::div_assign: return div_assign;
	}
	return nullptr;
}

} // namespace object_behavior

// A pretty large class that represents the entire type system
// of this scripting language.
class value_type
{
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

	bool is_void() const
	{
		return mData->mType_info.is_void;
	}

	bool is_const() const
	{
		return mData->mType_info.is_const;
	}

	bool is_arithmetic() const
	{
		return mData->mType_info.is_arithmetic;
	}

	const type_info& get_type_info() const
	{
		return mData->mType_info;
	}

	template <typename T>
	T* get() const
	{
		if (!mData->has_type<T>())
			return nullptr;
		if (!std::is_const<T>::value && is_const())
			return nullptr;
		if constexpr (std::is_const<T>::value)
			return static_cast<const T*>(mData->mPtr_c);
		else
			return static_cast<T*>(mData->mPtr);
	}

	// Reset this object to a void type
	void clear()
	{
		std::swap(*this, value_type{});
	}

	data& get_data() const
	{
		return *mData;
	}

	// Creates a new value_type pointing to the same stored value
	// but in a seperate data instance. Optional parameter to make
	// the new reference constant.
	value_type create_unique_reference(bool mMake_const = false) const
	{
		auto new_data = std::make_shared<data>(*mData);
		new_data->mType_info.is_const |= mMake_const;

		value_type new_value_type;
		new_value_type.mData = new_data;
		return new_value_type;
	}

private:
	std::shared_ptr<data> mData;
};

template <typename T>
value_type const_value(T&& pVal)
{
	return value_type(std::forward<T>(pVal)).create_unique_reference(true);
}


} // namespace wolfscript
