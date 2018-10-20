#pragma once

#include "token.hpp"
#include <iostream>
#include <set>
#include <memory>
#include <list>

namespace wolfscript
{

struct AST_node_empty;
struct AST_node_block;
struct AST_node_variable;
struct AST_node_unary_op;
struct AST_node_binary_op;
struct AST_node_member_accessor;
struct AST_node_constant;
struct AST_node_identifier;
struct AST_node_function_call;
struct AST_node_if;
struct AST_node_for;
struct AST_node_while;
struct AST_node_function_declaration;
struct AST_node_return;
struct AST_node_break;
struct AST_node_continue;

struct AST_visitor
{
	virtual ~AST_visitor() = default;
	virtual void dispatch(AST_node_empty*) {}
	virtual void dispatch(AST_node_block*) {}
	virtual void dispatch(AST_node_variable*) {}
	virtual void dispatch(AST_node_unary_op*) {}
	virtual void dispatch(AST_node_binary_op*) {}
	virtual void dispatch(AST_node_member_accessor*) {}
	virtual void dispatch(AST_node_constant*) {}
	virtual void dispatch(AST_node_identifier*) {}
	virtual void dispatch(AST_node_function_call*) {}
	virtual void dispatch(AST_node_if*) {}
	virtual void dispatch(AST_node_for*) {}
	virtual void dispatch(AST_node_while*) {}
	virtual void dispatch(AST_node_function_declaration*) {}
	virtual void dispatch(AST_node_return*) {}
	virtual void dispatch(AST_node_break*) {}
	virtual void dispatch(AST_node_continue*) {}
};

struct AST_node
{
	using ptr = std::unique_ptr<AST_node>;

	virtual ~AST_node() {}
	virtual void visit(AST_visitor*) = 0;
	virtual bool is_empty() const = 0;

	std::vector<std::unique_ptr<AST_node>> children;
};

template<class T>
struct AST_node_impl :
	AST_node
{
	virtual ~AST_node_impl() {}
	void visit(AST_visitor* pVisitor) override
	{
		pVisitor->dispatch(static_cast<T*>(this));
	}

	virtual bool is_empty() const override
	{
		return false;
	}

	token related_token;
};

struct AST_node_empty :
	AST_node_impl<AST_node_empty>
{
	virtual bool is_empty() const override
	{
		return true;
	}
};

struct AST_node_block :
	AST_node_impl<AST_node_block>
{};

struct AST_node_variable :
	AST_node_impl<AST_node_variable>
{
	bool is_const{ false };
	std::string_view identifier;
};

struct AST_node_unary_op :
	AST_node_impl<AST_node_unary_op>
{
	token_type type;
};

struct AST_node_binary_op :
	AST_node_impl<AST_node_binary_op>
{
	token_type type;
};

struct AST_node_member_accessor :
	AST_node_impl<AST_node_member_accessor>
{
	std::string_view identifier;
};

struct AST_node_constant :
	AST_node_impl<AST_node_constant>
{
};

struct AST_node_if :
	AST_node_impl<AST_node_if>
{
	std::size_t elseif_count{ 0 };
	// If true, the last child is the else statement
	bool has_else{ false };
};

struct AST_node_for :
	AST_node_impl<AST_node_for>
{
};

struct AST_node_while :
	AST_node_impl<AST_node_while>
{
};

struct AST_node_identifier :
	AST_node_impl<AST_node_identifier>
{
	std::string_view identifier;
};

struct AST_node_function_call :
	AST_node_impl<AST_node_function_call>
{
};

struct AST_node_function_declaration :
	AST_node_impl<AST_node_function_declaration>
{
	// This is empty if this is an anonymous function
	std::string_view identifier;

	struct param
	{
		std::string_view identifier;
		bool has_type{ false };
		bool is_const{ false };
		token type;
	};
	std::vector<param> parameters;

	bool has_return_type{ false };
	token return_type;
};

struct AST_node_return :
	AST_node_impl<AST_node_return>
{
};

struct AST_node_break :
	AST_node_impl<AST_node_break>
{
};

struct AST_node_continue :
	AST_node_impl<AST_node_continue>
{
};

// This visitor prints out the AST for debugging
class AST_viewer :
	public AST_visitor
{
public:
	virtual void dispatch(AST_node_block* pNode)
	{
		std::cout << get_indent() << "Block\n";
		++mIndent;
		for (const auto& i : pNode->children)
			i->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_variable* pNode)
	{
		std::cout << get_indent() << "Var <Identifier: " << pNode->identifier << ">\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_unary_op* pNode)
	{
		std::cout << get_indent() << "Unary Operation <Op: " << token_name[static_cast<unsigned int>(pNode->type)] << ">\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_binary_op* pNode)
	{
		std::cout << get_indent() << "Binary Operation <Op: " << token_name[static_cast<unsigned int>(pNode->type)] << ">\n";
		++mIndent;
		pNode->children[0]->visit(this);
		pNode->children[1]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_member_accessor* pNode)
	{
		std::cout << get_indent() << "Member Accessor <Identifier: " << pNode->identifier << ">\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_if* pNode)
	{
		std::cout << get_indent() << "If Statement\n";
		++mIndent;
		pNode->children[0]->visit(this);
		pNode->children[1]->visit(this);
		--mIndent;
		if (pNode->elseif_count > 0)
			std::cout << get_indent() << "Else If Statement\n";
		for (std::size_t i = 0; i < pNode->elseif_count; i++)
		{
			++mIndent;
			pNode->children[i * 2 + 2]->visit(this);
			pNode->children[i * 2 + 3]->visit(this);
			--mIndent;
		}
		if (pNode->has_else)
		{
			std::cout << get_indent() << "Else Statement\n";
			++mIndent;
			pNode->children.back()->visit(this);
			--mIndent;
		}
	}


	virtual void dispatch(AST_node_for* pNode)
	{
		std::cout << get_indent() << "For Statement\n";

		std::cout << get_indent() << "Var/Expression\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;

		std::cout << get_indent() << "Conditional\n";
		++mIndent;
		pNode->children[1]->visit(this);
		--mIndent;

		std::cout << get_indent() << "Expression\n";
		++mIndent;
		pNode->children[2]->visit(this);
		--mIndent;

		std::cout << get_indent() << "Body\n";
		++mIndent;
		pNode->children[3]->visit(this);
		--mIndent;
	}
	virtual void dispatch(AST_node_while* pNode)
	{
		std::cout << get_indent() << "While Statement\n";

		std::cout << get_indent() << "Conditional\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;

		std::cout << get_indent() << "Body\n";
		++mIndent;
		pNode->children[1]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_constant* pNode)
	{
		std::cout << get_indent() << "Constant <";
		if (pNode->related_token.type == token_type::string)
			std::cout << std::get<std::string>(pNode->related_token.value);
		else if (pNode->related_token.type == token_type::integer)
			std::cout << std::get<int>(pNode->related_token.value);
		else if (pNode->related_token.type == token_type::floating)
			std::cout << std::get<float>(pNode->related_token.value);
		std::cout << ">\n";
	}

	virtual void dispatch(AST_node_identifier* pNode)
	{
		std::cout << get_indent() << "Identifier <" << pNode->identifier << ">\n";
	}

	virtual void dispatch(AST_node_function_call* pNode)
	{
		std::cout << get_indent() << "Function Call\n";
		++mIndent;
		for (auto& i : pNode->children)
			i->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_function_declaration* pNode)
	{
		std::cout << get_indent() << "Function Declaration <";
		if (!pNode->identifier.empty())
			std::cout << "Identifier: " << pNode->identifier << " ";
		std::cout << "Parameters: ";
		for (const auto& i : pNode->parameters)
			std::cout << i.identifier << ", ";
		std::cout << ">\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_return* pNode)
	{
		std::cout << get_indent() << "Return\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_break* pNode)
	{
		std::cout << get_indent() << "Break\n";
	}

	virtual void dispatch(AST_node_continue* pNode)
	{
		std::cout << get_indent() << "Continue\n";
	}

private:
	std::string get_indent() const
	{
		std::string result;
		for (int i = 0; i < mIndent; i++)
			result += "  ";
		return result;
	}

	int mIndent{ 0 };
};


class AST_walker :
	public AST_visitor
{
public:
	virtual void dispatch(AST_node_block* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_variable* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_unary_op* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_binary_op* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_member_accessor* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_if* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_for* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_while* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_function_call* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_function_declaration* pNode)
	{
		visit_all(pNode);
	}

	virtual void dispatch(AST_node_return* pNode)
	{
		visit_all(pNode);
	}

private:
	void visit_all(AST_node* pNode)
	{
		for (auto& i : pNode->children)
			i->visit(this);
	}
};

// Traverses the AST to find symbols not defined locally.
class AST_nonlocal_symbols_finder :
	public AST_walker
{
public:
	virtual void dispatch(AST_node_block* pNode)
	{
		mLocals.emplace_front();
		AST_walker::dispatch(pNode);
		mLocals.pop_front();
	}

	virtual void dispatch(AST_node_variable* pNode)
	{
		mLocals.front().insert(pNode->identifier);
		AST_walker::dispatch(pNode);
	}

	virtual void dispatch(AST_node_identifier* pNode)
	{
		if (!query_local(pNode->identifier))
			mUnknown_symbols.insert(pNode->identifier);
	}

	const std::set<std::string_view>& get_symbols() const
	{
		return mUnknown_symbols;
	}

private:
	bool query_local(std::string_view pName) const
	{
		for (auto& i : mLocals)
			if (i.count(pName) > 0)
				return true;
		return false;
	}

private:
	std::set<std::string_view> mUnknown_symbols;
	std::list<std::set<std::string_view>> mLocals;
};

} // namespace wolfscript
