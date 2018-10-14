#pragma once

#include <typeinfo>
#include <type_traits>

namespace wolfscript
{

struct type_info
{
	bool is_const{ false };

	// Reference and pointer types passed to the value_type object
	// will be passed around as non-owning references. If both are false,
	// the script will create a shared_ptr around the value.
	bool is_reference{ false };
	bool is_pointer{ false };

	// True when the type is an arithmetic (bool, int, float, etcetera...)
	bool is_arithmetic{ false };

	const std::type_info* stdtypeinfo{ &typeid(void) };

	type_info() = default;
	type_info(bool pIs_const,
		bool pIs_reference,
		bool pIs_pointer,
		bool pIs_arithmic,
		const std::type_info* pType) :
		is_const(pIs_const),
		is_reference(pIs_reference),
		is_pointer(pIs_pointer),
		is_arithmetic(pIs_arithmic),
		stdtypeinfo(pType)
	{}
	type_info(const type_info&) = default;
	type_info(type_info&&) = default;
	type_info& operator=(const type_info&) = default;

	bool bare_equal(const type_info& pOther) const
	{
		return *stdtypeinfo == *pOther.stdtypeinfo;
	}

	bool owning() const
	{
		return !is_reference && !is_pointer;
	}

	// Create a type_info from a C++ type
	template <typename T>
	static type_info create()
	{
		using type_bare = std::remove_pointer_t<std::decay_t<T>>;
		return type_info(
			std::is_const<std::remove_pointer_t<T>>::value,
			std::is_reference<T>::value,
			std::is_pointer<T>::value,
			std::is_arithmetic<type_bare>::value,
			&typeid(type_bare)
		);
	}
};

} // namespace wolfscript
