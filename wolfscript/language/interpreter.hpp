#pragma once

#include "parser.hpp"
#include "value_type.hpp"
#include "exception.hpp"
#include "callable.hpp"
#include "common.hpp"
#include "arithmetic.hpp"

#include <iostream>
#include <bitset>

namespace wolfscript
{

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
		auto iter = mScope_stack.front().find(pName);
		if (iter != mScope_stack.front().end())
		{
			if (pValue.get<const callable>())
			{
				if (auto overloader = iter->second.get<const callable_overloader>())
				{
					// Add to a current overloader in scope
					overloader->add(pValue);
				}
				else if (iter->second.get<const callable>())
				{
					// Swap it out for an overloader
					callable_overloader overloader;
					overloader.add(pValue);
					overloader.add(iter->second);
					iter->second = overloader;
				}
			}
		}
		else
		{
			return mScope_stack.front()[pName] = pValue;
		}
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

	value_type* lookup_current_scope(const std::string& pName)
	{
		auto iter = mScope_stack.front().find(pName);
		if (iter != mScope_stack.front().end())
			return &iter->second;
		return nullptr;
	}

	std::vector<value_type*> get_all_matches(const std::string& pName)
	{
		std::vector<value_type*> result;
		for (auto& i : mScope_stack)
		{
			auto iter = i.find(pName);
			if (iter != i.end())
				result.push_back(&iter->second);
		}
		return result;
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

class interpreter :
	private AST_visitor
{
public:
	using string_factory = std::function<value_type(const std::string&)>;

	void interpret(AST_node* mRoot)
	{
		mRoot->visit(this);
		mControl_flags.reset();
	}

	void interpret(const AST_node::ptr& mRoot)
	{
		interpret(mRoot.get());
	}

	void add(const std::string& pName, value_type pVal)
	{
		mSymbols.add(pName, pVal);
	}

	value_type& operator[](const std::string& pIdentifier)
	{
		return mSymbols[pIdentifier];
	}

	void set_string_factory(string_factory pFactory)
	{
		mString_factory = pFactory;
	}

	template <typename T>
	void add_type(const std::string& pStr)
	{
		mTypes.emplace_back(std::make_pair(pStr, type_info::create<T>()));
	}

private:
	value_type visit_for_value(AST_node* pNode)
	{
		mResult_value.clear();
		pNode->visit(this);
		value_type result = mResult_value;
		mResult_value.clear();
		return result;
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

			// The return request breaks all scopes.
			// The result value is retained because it contains the return value.
			if (mControl_flags.any())
				break;
			// Clear the result after each line.
			mResult_value.clear();
		}
		mSymbols.pop_scope();
	}

	// Declare variable
	virtual void dispatch(AST_node_variable* pNode) override
	{
		mSymbols.add(std::string(pNode->identifier), copy_value(visit_for_value(pNode->children[0])));
	}

	virtual void dispatch(AST_node_unary_op* pNode)
	{
		value_type val = visit_for_value(pNode->children[0]);
		mResult_value = arithmetic_unary_operation(pNode->type, val);
	}

	virtual void dispatch(AST_node_binary_op* pNode) override
	{
		value_type l = visit_for_value(pNode->children[0]);
		value_type r = visit_for_value(pNode->children[1]);
			
		if (l.is_arithmetic() && r.is_arithmetic())
			mResult_value = arithmetic_binary_operation(pNode->type, l, r);
		else
			mResult_value = call_function(object_behavior::from_token_type(pNode->type), { l, r });
	}

	virtual void dispatch(AST_node_member_accessor* pNode) override
	{
		value_type l = visit_for_value(pNode->children[0]);

		// Call function to access the member
		mResult_value = call_function(std::string(pNode->identifier), { l });
	}

	virtual void dispatch(AST_node_constant* pNode) override
	{
		switch (pNode->related_token.type)
		{
		case token_type::integer:
			mResult_value = std::cref(std::get<int>(pNode->related_token.value));
			break;
		case token_type::string:
			mResult_value = std::cref(std::get<std::string>(pNode->related_token.value));
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
		
		arg_list args;
		std::vector<type_info> arg_types;
		for (std::size_t i = 1; i < pNode->children.size(); i++)
		{
			args.emplace_back(visit_for_value(pNode->children[i]));
			arg_types.push_back(args.back().get_type_info());
		}

		if (auto overloader = c.get<const callable_overloader>())
		{
			auto& func = overloader->find(arg_types, mCaster);
			mResult_value = func.function(args);
		}
		else if (auto func = c.get<const callable>())
		{
			if (!func->match(arg_types, mCaster))
				throw exception::interpretor_error("Cannot find function");
			mResult_value = func->function(args);
		}
	}

	virtual void dispatch(AST_node_if* pNode) override
	{
		// If
		if (mCaster.cast<bool>(visit_for_value(pNode->children[0])))
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
				if (mCaster.cast<bool>(visit_for_value(pNode->children[condition_idx])))
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

	virtual void dispatch(AST_node_for* pNode) override
	{
		// Scope for the var statement
		mSymbols.push_scope();

		const bool empty_conditional = pNode->children[1]->is_empty();
		for (
			pNode->children[0]->visit(this);
			empty_conditional || mCaster.cast<bool>(visit_for_value(pNode->children[1]));
			pNode->children[2]->visit(this)
			)
		{
			// New scope for each iteration
			mSymbols.push_scope();

			pNode->children[3]->visit(this);

			// "continue" just causes all scopes to unwind in the loop
			// and it loops again like nothing happened
			mControl_flags[ctrl_continue] = false;

			// Break loop
			if (mControl_flags[ctrl_break]
				|| mControl_flags[ctrl_return])
			{
				mControl_flags[ctrl_break] = false;
				break;
			}

			mSymbols.pop_scope();
		}

		mSymbols.pop_scope();
	}

	virtual void dispatch(AST_node_while* pNode) override
	{
		while (mCaster.cast<bool>(visit_for_value(pNode->children[0])))
		{
			// New scope for each iteration
			mSymbols.push_scope();

			pNode->children[1]->visit(this);

			// Loop again like nothing happened
			mControl_flags[ctrl_continue] = false;

			// Break loop
			if (mControl_flags[ctrl_break]
				|| mControl_flags[ctrl_return])
			{
				mControl_flags[ctrl_break] = false;
				break;
			}

			mSymbols.pop_scope();
		}
	}

	virtual void dispatch(AST_node_function_declaration* pNode) override
	{
		callable func;

		if (pNode->has_return_type)
		{
			auto type = get_type(pNode->return_type.text);
			if (!type)
				throw exception::interpretor_error("Invalid return type");
			func.return_type = *type;
		}
		else
		{
			func.return_type = type_info::create<value_type>();
		}

		for (auto& i : pNode->parameters)
		{
			if (i.has_type)
			{
				auto type = get_type(i.type.text);
				if (!type)
					throw exception::interpretor_error("Invalid parameter type");
				if (i.is_const)
					func.parameter_types.push_back(const_type(*type));
				else
					func.parameter_types.push_back(*type);
			}
			else
			{
				func.parameter_types.push_back(type_info::create<value_type>());
			}
		}


		func.function = [this, pNode](const std::vector<value_type>& pArgs)->value_type
		{
			mSymbols.push_scope();
			// Add the parameters to the scope
			for (std::size_t i = 0; i < pArgs.size(); i++)
				mSymbols.add(std::string(pNode->parameters[i].identifier), pArgs[i]);
			
			// Interpret the functions body nodes
			value_type retval = visit_for_value(pNode->children[0]);
			mControl_flags.reset();
			mSymbols.pop_scope();
			return retval;
		};

		if (pNode->identifier.empty())
		{
			// Return the object for this anonymous function
			mResult_value = func;
		}
		else
		{
			// Add the function as its own constant variable
			mSymbols.add(std::string(pNode->identifier), const_value(func));
		}
	}

	virtual void dispatch(AST_node_return* pNode) override
	{
		mResult_value = visit_for_value(pNode->children[0]);
		mControl_flags[ctrl_return] = true;
	}

	virtual void dispatch(AST_node_break* pNode) override
	{
		mControl_flags[ctrl_break] = true;
	}

	virtual void dispatch(AST_node_continue* pNode) override
	{
		mControl_flags[ctrl_continue] = true;
	}

private:
	value_type call_function(const std::string& pName, const arg_list& pArgs)
	{
		auto matches = mSymbols.get_all_matches(pName);
		if (matches.empty())
			throw exception::interpretor_error("No function matches");

		callable_overloader overloader;
		for (auto i : matches)
		{
			if (i->get<const callable>())
			{
				overloader.add(*i);
			}
			else if (auto other = i->get<const callable_overloader>())
			{
				overloader.add(*other);
			}
		}

		const callable& c = overloader.find(pArgs, mCaster);
		return c.function(pArgs);
	}

	value_type copy_value(const value_type& pVal)
	{
		if (pVal.is_arithmetic())
		{
			auto visitor = [](const auto& pVal) -> value_type
			{
				return pVal;
			};
			return detail::visit_arithmetic(pVal, visitor);
		}
		else
		{
			return call_function("copy", { pVal });
		}
	}

	const type_info* get_type(const std::string_view& pStr) const
	{
		for (auto& i : mTypes)
			if (i.first == pStr)
				return &i.second;
		return nullptr;
	}

	const std::string* get_type(const type_info& pType) const
	{
		for (auto& i : mTypes)
			if (i.second.bare_equal(pType))
				return &i.first;
		return nullptr;
	}

private:
	enum control_flags : std::size_t
	{
		ctrl_return,
		ctrl_break,
		ctrl_continue,

		ctrl_count
	};
	std::bitset<ctrl_count> mControl_flags;

	std::vector<std::pair<std::string, type_info>> mTypes;
	string_factory mString_factory;
	value_type mResult_value;
	cast_list mCaster;
	symbol_table mSymbols;
};

} // namespace wolfscript
