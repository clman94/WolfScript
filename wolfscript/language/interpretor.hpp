#pragma once

#include "parser.hpp"
#include "value_type.hpp"
#include "exception.hpp"
#include <iostream>

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
			if (mReturn_request)
				break;
			// Clear the result after each line.
			mResult_value.clear();
		}
		mSymbols.pop_scope();
	}

	// Declare variable
	virtual void dispatch(AST_node_variable* pNode) override
	{
		mSymbols.add(std::string(pNode->identifier), visit_for_value(pNode->children[0]).copy());
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

			// FIXME: Allow non-const access
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
					return func->function(args);
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
		
		std::vector<value_type> args;
		for (std::size_t i = 1; i < pNode->children.size(); i++)
			args.emplace_back(visit_for_value(pNode->children[i]));

		auto func = c.get<const value_type::callable>();
		mResult_value = func->function(args);
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
		func.function = [this, pNode](const std::vector<value_type>& pArgs)->value_type
		{
			if (pArgs.size() != pNode->parameters.size())
				throw exception::interpretor_error("Invalid argument count");
			mSymbols.push_scope();
			for (std::size_t i = 0; i < pArgs.size(); i++)
				mSymbols.add(std::string(pNode->parameters[i]), pArgs[i]);
			
			value_type retval = visit_for_value(pNode->children[0]);
			mReturn_request = false; // Clear the request
			mSymbols.pop_scope();
			return retval;
		};
		if (pNode->identifier.empty())
			mResult_value = func;
		else
			mSymbols.add(std::string(pNode->identifier), func);
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

} // namespace wolfscript
