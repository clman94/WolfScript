#pragma once

#include "parser.hpp"
#include <functional>
#include <any>
#include <iostream>
#include <type_traits>

namespace wolfscript
{

enum class type_flags : uint32_t
{
	none = 0,
	constant = 1,
	reference = 1 << 1,
	pod = 1 << 2,
	arithmic = 1 << 3,
};

type_flags operator | (const type_flags& l, const type_flags& r)
{
	using T = std::underlying_type<type_flags>::type;
	return static_cast<type_flags>(static_cast<T>(l) | static_cast<T>(r));
}

type_flags operator & (const type_flags& l, const type_flags& r)
{
	using T = std::underlying_type<type_flags>::type;
	return static_cast<type_flags>(static_cast<T>(l) & static_cast<T>(r));
}

type_flags& operator |= (type_flags& l, const type_flags& r)
{
	using T = std::underlying_type<type_flags>::type;
	return l = static_cast<type_flags>(static_cast<T>(l) | static_cast<T>(r));
}

struct type_info
{
	bool is_const{ false };
	bool is_reference{ false };
	bool is_pointer{ false };
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
		return stdtypeinfo == pOther.stdtypeinfo;
	}

	// Create a type_info from a C++ type
	template <typename T>
	static type_info create()
	{
		return type_info(
			std::is_const<T>::value,
			std::is_reference<T>::value,
			std::is_pointer<T>::value,
			std::is_arithmetic<std::decay_t<T>>::value,
			&typeid(std::decay_t<T>)
		);
	}
};

class class_descriptor
{
public:

};


// A pretty large class the represents the entire type system
// of this scripting language.
class value_type
{
public:
	using cast_function = std::function<value_type(type_info, value_type)>;
	using arg_list = std::vector<value_type>;
	struct callable
	{
		// For methods, the first parameter is the object
		std::function<value_type(const arg_list&)> func;
	};

	struct object
	{

		std::map<std::string, value_type> members;
	};

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

		bool can_cast(type_info pTo, type_info pFrom) const
		{
			if (pTo.is_arithmetic && pFrom.is_arithmetic)
				return true;
			return static_cast<bool>(find(pTo, pFrom));
		}

		cast_function find(type_info pTo, type_info pFrom) const
		{
			for (const auto& i : mEntries)
				if (i.to.stdtypeinfo == pTo.stdtypeinfo &&
					i.from.stdtypeinfo == pFrom.stdtypeinfo)
					return i.func;
			return {};
		}

		value_type cast(const type_info& pTo, const value_type& pFrom) const
		{
			// Automatically cast arithmetic types
			if (pTo.is_arithmetic && pFrom.mData->mType_info.is_arithmetic)
			{
				auto visit = [&pTo](auto& pLtype)->value_type
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

		template <typename T>
		data(T&& pValue)
		{
			set(std::forward<T>(pValue));
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
			mType_info = type_info::create<T>();
		}

		template <typename T>
		void set(T* pPtr)
		{
			mPtr_c = pPtr;
			if constexpr (!std::is_const<T>::value)
				mPtr = pPtr;
			else
				mPtr = nullptr;
			mData = std::any(std::ref(*pPtr));
			mType_info = type_info::create<T>();
		}

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
			return mType_info.stdtypeinfo == type_info::create<T>().stdtypeinfo;
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

	template <typename T, typename = std::enable_if<!std::is_same<std::decay_t<T>, value_type>::value>::type>
	value_type(T&& pValue)
	{
		mData = std::make_shared<data>(std::forward<T>(pValue));
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

	value_type& operator=(value_type pCopy)
	{
		std::swap(mData, pCopy.mData);
		return *this;
	}

	std::string to_string() const
	{
		if (*mData->mType_info.stdtypeinfo == typeid(object))
		{
			const object* obj = get<const object>();
			auto f = obj->members.find("__to_string");
			auto c = f->second.get<const callable>();
			return *c->func({ *this }).get<const std::string>();
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

	value_type operator()(const std::initializer_list<value_type>& pList)
	{

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
			std::string op_name;
			switch (pOp)
			{
			case token_type::assign: op_name = "__assign"; break;
			default:
				throw exception::interpretor_error("Unknown operation");
			}
			auto member_iter = obj->members.find(op_name);
			if (member_iter == obj->members.end())
				throw exception::interpretor_error("Cannot find operator overload for object");
			auto member_callable = member_iter->second.get<const callable>();
			member_callable->func({ pL, pR });
		}
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
				throw exception::interpretor_error("Cannot assign to a constant value");
			}
		}
		else
		{
			switch (pOp)
			{
			case token_type::assign:
				pLval = r_casted;
				return pL;
			}
		}

		throw exception::interpretor_error("Unknown operation");
	}

private:
	std::shared_ptr<data> mData;
};

class symbol_table
{
public:
	symbol_table()
	{
		mScope_stack.emplace_front();
	}

	void push_scope()
	{
		mScope_stack.emplace_front();
	}

	void pop_scope()
	{
		mScope_stack.pop_front();
	}

	value_type& add(const std::string& pName, const value_type& pValue)
	{
		return mScope_stack.front()[pName] = pValue;
	}

	value_type* lookup(const std::string& pName)
	{
		for (auto& i : mScope_stack)
		{
			auto iter = i.find(pName);
			if (iter != i.end())
				return &iter->second;
		}
		return nullptr;
	}

	bool exists(const std::string& pName) const
	{
		for (const auto& i : mScope_stack)
			if (i.find(pName) != i.end())
				return true;
		return false;
	}

	value_type& operator[](const std::string& pName)
	{
		if (auto found = lookup(pName))
			return *found;
		else
			return add(pName, value_type{});
	}

private:
	typedef std::map<std::string, value_type> scope_t;
	std::list<scope_t> mScope_stack;
};

class interpretor :
	private AST_visitor
{
public:
	using string_factory = std::function<value_type(const std::string&)>;

	void interpret(AST_node* mRoot)
	{
		mRoot->visit(this);
		mReturn_request = false;
	}

	value_type& operator[](const std::string& pIdentifier)
	{
		return mSymbols[pIdentifier];
	}

	void set_string_factory(string_factory pFactory)
	{
		mString_factory = pFactory;
	}

private:
	value_type visit_for_value(AST_node* pNode)
	{
		pNode->visit(this);
		return mResult_value;
	}

	value_type visit_for_value(const std::unique_ptr<AST_node>& pNode)
	{
		return visit_for_value(pNode.get());
	}

	virtual void dispatch(AST_node_block* pNode) override
	{
		mSymbols.push_scope();
		for (const auto& i : pNode->children)
		{
			i->visit(this);

			// The return request breaks all scopes
			if (mReturn_request)
				break;

			// Clear the result after each line.
			std::swap(mResult_value, value_type{});
		}
		mSymbols.pop_scope();
	}

	// Declare variable
	virtual void dispatch(AST_node_variable* pNode) override
	{
		std::swap(mResult_value, value_type{});
		mSymbols[std::string(pNode->identifier)] = visit_for_value(pNode->children[0]);
		std::cout << "Defined variable " << pNode->identifier << " with " << mResult_value.to_string() << "\n";
	}

	virtual void dispatch(AST_node_unary_op* pNode)
	{
		value_type val = visit_for_value(pNode->children[0]);
		mResult_value = val.unary_operation(pNode->type, val);
	}

	virtual void dispatch(AST_node_binary_op* pNode) override
	{
		if (pNode->type == token_type::period)
		{
			value_type l = visit_for_value(pNode->children[0]);

			// Cast the r to an identifier node. 
			// FIXME: Move this to the parser instead
			AST_node_identifier* r_id
				= dynamic_cast<AST_node_identifier*>(pNode->children[1].get());
			if (!r_id)
				throw exception::interpretor_error("Expected identifier");

			auto obj = l.get<const value_type::object>();

			// Find the member object and return it
			auto member = obj->members.find(std::string(r_id->identifier));
			if (member == obj->members.end())
				throw exception::interpretor_error("Could not find member");

			// If it's a callable, create a delegate callable for calling the method of a specific instance
			if (auto func = member->second.get<const value_type::callable>())
			{
				// The capture will keep the object alive during the lifetime of this return value 
				mResult_value = value_type::callable{
					[l, func](const value_type::arg_list& pArgs)->value_type
				{
					value_type::arg_list args;
					args.push_back(l);
					args.insert(args.end(), pArgs.begin(), pArgs.end());
					return func->func(args);
				}
				};
			}
			else
			{
				// Return the reference to the member
				mResult_value = member->second;
			}
		}
		else
		{
			value_type l = visit_for_value(pNode->children[0]);
			value_type r = visit_for_value(pNode->children[1]);
			if (!l.get_type_info().is_arithmetic || !r.get_type_info().is_arithmetic)
				throw exception::interpretor_error("These types are not arithmic types");
			mResult_value = value_type::binary_operation(pNode->type, l, r);
		}
	}

	virtual void dispatch(AST_node_constant* pNode) override
	{
		switch (pNode->related_token.type)
		{
		case token_type::integer:
			mResult_value = value_type::create_const(std::forward<int>(std::get<int>(pNode->related_token.value)));
			break;
		case token_type::string:
			mResult_value = value_type::create_const(mString_factory(std::get<std::string>(pNode->related_token.value)));
			break;
		default:
			throw exception::interpretor_error("Unsupported constant type");
		}
		return;
	}

	virtual void dispatch(AST_node_identifier* pNode) override
	{
		if (auto value = mSymbols.lookup(std::string(pNode->identifier)))
			mResult_value = *value;
		else
			throw exception::interpretor_error("Variable does not exist");
	}

	virtual void dispatch(AST_node_function_call* pNode) override
	{
		value_type c = visit_for_value(pNode->children[0]);
		//std::cout << "Function call to " << pNode->identifier << "\n";
		const value_type::callable* func = c.get<const value_type::callable>();

		std::vector<value_type> args;
		for (std::size_t i = 1; i < pNode->children.size(); i++)
			args.emplace_back(visit_for_value(pNode->children[i]));
		mResult_value = func->func(args);
	}

	virtual void dispatch(AST_node_if* pNode) override
	{
		// If
		if (visit_for_value(pNode->children[0]).get<bool>(mCaster))
		{
			pNode->children[1]->visit(this);
			return;
		}

		// ...Else if...
		else if (pNode->elseif_count > 0)
		{
			for (std::size_t i = 0; i < pNode->elseif_count; i++)
			{
				std::size_t condition_idx = i * 2 + 2;
				std::size_t body_idx = i * 2 + 3;
				if (visit_for_value(pNode->children[condition_idx]).get<bool>(mCaster))
				{
					pNode->children[body_idx]->visit(this);
					return;
				}
			}
		}

		// Else
		if (pNode->has_else)
			pNode->children.back()->visit(this);
	}

	virtual void dispatch(AST_node_function_declaration* pNode) override
	{
		value_type::callable func;
		func.func = [this, pNode](const std::vector<value_type>& pArgs)->value_type
		{
			if (pArgs.size() != pNode->parameters.size())
				throw exception::interpretor_error("Invalid argument count");
			mSymbols.push_scope();
			for (std::size_t i = 0; i < pArgs.size(); i++)
				mSymbols[std::string(pNode->parameters[i])] = pArgs[i];
			pNode->children[0]->visit(this);
			mReturn_request = false; // Clear the request
			mSymbols.pop_scope();
			return mResult_value;
		};
		if (pNode->identifier.empty())
			mResult_value = func;
		else
		{
			mSymbols[std::string(pNode->identifier)] = func;
		}
	}

	virtual void dispatch(AST_node_return* pNode) override
	{
		mResult_value = visit_for_value(pNode->children[0]);
		mReturn_request = true;
	}

private:
	bool mReturn_request{ false };
	string_factory mString_factory;
	value_type mResult_value;
	value_type::cast_list mCaster;
	symbol_table mSymbols;
};


}