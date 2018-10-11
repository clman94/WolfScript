#pragma once

#include "tokenizer.hpp"
#include <memory>
#include <set>
#include <iostream>

namespace wolfscript
{

struct AST_node_block;
struct AST_node_variable;
struct AST_node_unary_op;
struct AST_node_binary_op;
struct AST_node_constant;
struct AST_node_identifier;
struct AST_node_function_call;
struct AST_node_if;
struct AST_node_function_declaration;
struct AST_node_return;

struct AST_visitor
{
	virtual void dispatch(AST_node_block*) {}
	virtual void dispatch(AST_node_variable*) {}
	virtual void dispatch(AST_node_unary_op*) {}
	virtual void dispatch(AST_node_binary_op*) {}
	virtual void dispatch(AST_node_constant*) {}
	virtual void dispatch(AST_node_identifier*) {}
	virtual void dispatch(AST_node_function_call*) {}
	virtual void dispatch(AST_node_if*) {}
	virtual void dispatch(AST_node_function_declaration*) {}
	virtual void dispatch(AST_node_return*) {}
};

struct AST_node
{
	virtual ~AST_node() {}
	virtual void visit(AST_visitor*) = 0;

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

	token related_token;
};

struct AST_node_block :
	AST_node_impl<AST_node_block>
{};

struct AST_node_variable :
	AST_node_impl<AST_node_variable>
{
	bool is_const;
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

struct AST_node_constant :
	AST_node_impl<AST_node_constant>
{
};

struct AST_node_if :
	AST_node_impl<AST_node_if>
{
	std::size_t elseif_count{ 0 };
	bool has_else{ false };
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
	std::string_view identifier;
	std::vector<std::string_view> parameters;
};

struct AST_node_return :
	AST_node_impl<AST_node_return>
{
};

namespace exception
{
struct parse_error :
	std::runtime_error
{
	parse_error(const char* pMsg) :
		std::runtime_error(pMsg)
	{}

	parse_error(const char* pMsg, token pToken) :
		std::runtime_error(pMsg),
		current_token(pToken)
	{}

	token current_token;
};
}

template<typename Tto, typename Tfrom>
inline std::unique_ptr<Tto> static_unique_pointer_cast(std::unique_ptr<Tfrom>&& pOld)
{
	return std::unique_ptr<Tto>{ static_cast<Tto*>(pOld.release()) };
}

class parser
{
public:
	std::unique_ptr<AST_node> parse(const token_array& pTokens)
	{
		mTokens = pTokens;
		mIter = mTokens.begin();
		return parse_file();
	}

private:
	std::unique_ptr<AST_node> parse_file()
	{
		auto node = std::make_unique<AST_node_block>();
		while (can_peek())
			node->children.emplace_back(std::move(parse_statement()));
		return node;
	}

	std::unique_ptr<AST_node> parse_statement()
	{
		if (mIter->type == token_type::l_brace)
		{
			return parse_compound_statement();
		}
		else if (mIter->type == token_type::kw_var ||
			mIter->type == token_type::kw_const)
		{
			return parse_var();
		}
		else if (mIter->type == token_type::kw_if)
		{
			return parse_if_statement();
		}
		else if (mIter->type == token_type::kw_return)
		{
			return parse_return_statement();
		}
		else if (mIter->type == token_type::kw_function)
		{
			return function_declaration(false);
		}
		else
		{
			auto node = parse_expression();

			expect(token_type::eol, "Expected ;");
			advance(); // Skip ;

			return node;
		}
	}

	std::unique_ptr<AST_node> parse_compound_statement()
	{
		advance(); // Skip {
		auto node = std::make_unique<AST_node_block>();
		while (mIter->type != token_type::r_brace)
			node->children.emplace_back(std::move(parse_statement()));
		advance(); // Skip }
		return node;
	}

	std::unique_ptr<AST_node> parse_return_statement()
	{
		advance(); // Skip return
		auto node = std::make_unique<AST_node_return>();
		node->children.emplace_back(parse_expression());
		expect(token_type::eol, "Expected ;");
		advance(); // Skip ;
		return node;
	}

	// TODO: Add node type
	std::unique_ptr<AST_node> parse_if_statement()
	{
		auto node = std::make_unique<AST_node_if>();

		advance(); // Skip if
		expect(token_type::l_parenthesis, "Expected ( for if statement conditional expression");
		advance(); // Skip (
		if (mIter->type == token_type::r_parenthesis)
			throw exception::parse_error("Missing if statement conditional expression", *mIter);

		node->children.emplace_back(parse_expression());

		expect(token_type::r_parenthesis, "Expected )");
		advance(); // Skip )

		node->children.emplace_back(parse_statement());

		while (can_peek()
			&& mIter->type == token_type::kw_else
			&& peek()->type == token_type::kw_if)
		{
			advance(2); // Skip else if
			expect(token_type::l_parenthesis, "Expected ( for else if statement conditional expression");
			advance(); // Skip (
			if (mIter->type == token_type::r_parenthesis)
				throw exception::parse_error("Missing if statement conditional expression", *mIter);

			node->children.emplace_back(parse_expression());

			expect(token_type::r_parenthesis, "Expected )");
			advance(); // Skip )

			node->children.emplace_back(parse_statement());

			++node->elseif_count;
		}

		if (mIter->type == token_type::kw_else)
		{
			advance(); // Skip else
			node->has_else = true;
			node->children.emplace_back(parse_statement());
		}

		return node;
	}

	std::unique_ptr<AST_node> parse_for_statement()
	{
		advance(); // Skip for
		expect(token_type::l_parenthesis, "Expected ( for 'for' statement");
		advance(); // Skip (
		if (mIter->type == token_type::r_parenthesis)
			throw exception::parse_error("Missing 'for' statement expression", *mIter);

		// First segment
		if (mIter->type != token_type::eol)
		{
			if (mIter->type == token_type::kw_var)
				auto first_node = parse_var(); // Already checks for ;
			else
			{
				auto first_node = parse_expression();
				expect(token_type::eol, "Expected ;");
				advance(); // Skip ;
			}
		}

		// Second segment
		if (mIter->type != token_type::eol)
		{
			auto second_node = parse_expression();

			expect(token_type::eol, "Expected ;");
			advance(); // Skip ;
		}

		// Third segment
		if (mIter->type != token_type::r_parenthesis)
		{
			auto third_node = parse_expression();

			expect(token_type::r_parenthesis, "Expected ;");
			advance(); // Skip ;
		}

		return{};
	}

	std::unique_ptr<AST_node> parse_var()
	{
		auto node = std::make_unique<AST_node_variable>();

		node->is_const = mIter->type == token_type::kw_const;
		advance(); // Skip var/const

		// Get the identifier
		expect(token_type::identifier, "Expected identifier for variable");
		node->identifier = mIter->text;
		advance(); // Skip identifier

		// Check for =
		expect(token_type::assign, "Expected =");
		advance(); // Skip =

		node->children.emplace_back(std::move(parse_expression()));

		expect(token_type::eol, "Expected ;");
		advance(); // Skip ;

		return node;
	}

	// Helper function for binary expressions since they are defined in mostly the same way.
	std::unique_ptr<AST_node> parse_binary_expression(const std::set<token_type>& pOps,
		std::unique_ptr<AST_node>(parser::*pChild_func)())
	{
		auto node = (this->*pChild_func)();
		while (pOps.find(mIter->type) != pOps.end())
		{
			auto op_node = std::make_unique<AST_node_binary_op>();
			op_node->related_token = *mIter;
			op_node->type = mIter->type;
			op_node->children.emplace_back(std::move(node));
			advance(); // Skip op
			op_node->children.emplace_back((this->*pChild_func)());
			node = std::move(static_unique_pointer_cast<AST_node>(std::move(op_node)));
		}
		return node;
	}

	std::unique_ptr<AST_node> parse_expression()
	{
		return parse_binary_expression({ token_type::add, token_type::sub }, &parser::parse_assignment);
	}

	std::unique_ptr<AST_node> parse_assignment()
	{
		return parse_binary_expression({ token_type::assign }, &parser::parse_logical_or);
	}

	std::unique_ptr<AST_node> parse_logical_or()
	{
		return parse_binary_expression({ token_type::logical_or }, &parser::parse_logical_and);
	}

	std::unique_ptr<AST_node> parse_logical_and()
	{
		return parse_binary_expression({ token_type::logical_and }, &parser::parse_equality);
	}

	std::unique_ptr<AST_node> parse_equality()
	{
		return parse_binary_expression({ token_type::equ, token_type::not_equ }, &parser::parse_relational);
	}

	std::unique_ptr<AST_node> parse_relational()
	{
		return parse_binary_expression(
			{
				token_type::less_than,
				token_type::less_than_equ_to,
				token_type::greater_than,
				token_type::greater_than_equ_to
			}, &parser::parse_additive_expression);
	}

	std::unique_ptr<AST_node> parse_additive_expression()
	{
		return parse_binary_expression({ token_type::add, token_type::sub }, &parser::parse_multiplicative_expression);
	}

	std::unique_ptr<AST_node> parse_multiplicative_expression()
	{
		return parse_binary_expression({ token_type::mul, token_type::div }, &parser::parse_function_call);
	}


	std::unique_ptr<AST_node> parse_function_call()
	{
		auto factor = parse_member_accessor();
		if (mIter->type == token_type::l_parenthesis)
		{
			auto node = std::make_unique<AST_node_function_call>();
			node->children.emplace_back(std::move(factor));
			advance(); // Skip (

			// No arguments
			if (mIter->type == token_type::r_parenthesis)
			{
				advance(); // Skip )
				return node;
			}

			// First parameter
			node->children.emplace_back(parse_expression());

			while (mIter->type == token_type::separator)
			{
				advance(); // Skip ,
				node->children.emplace_back(parse_expression());
			}

			expect(token_type::r_parenthesis, "Expected )");
			advance(); // Skip )

			return node;
		}
		else
			return factor;
	}

	std::unique_ptr<AST_node> parse_member_accessor()
	{
		return parse_binary_expression({ token_type::period }, &parser::parse_factor);
	}

	std::unique_ptr<AST_node> parse_factor()
	{
		if (mIter->type == token_type::add
			|| mIter->type == token_type::sub)
		{
			auto node = std::make_unique<AST_node_unary_op>();
			node->related_token = *mIter;
			node->type = mIter->type;
			advance(); // Skip +/-
			node->children.emplace_back(parse_factor());
			return node;
		}
		else if (mIter->type == token_type::l_parenthesis)
		{
			advance(); // Skip (
			auto node = parse_expression();
			expect(token_type::r_parenthesis, "Expected )");
			advance(); // Skip )
			return node;
		}
		else if (mIter->type == token_type::integer ||
			mIter->type == token_type::string)
		{
			auto node = std::make_unique<AST_node_constant>();
			node->related_token = *mIter;
			advance();
			return node;
		}
		else if (mIter->type == token_type::identifier)
		{
			auto node = std::make_unique<AST_node_identifier>();
			node->related_token = *mIter;
			node->identifier = mIter->text;
			advance();
			return node;
		}
		else if (mIter->type == token_type::kw_function)
		{
			return function_declaration(true);
		}
		else
			throw exception::parse_error("Unexpected token", *mIter);
	}

	std::unique_ptr<AST_node> function_declaration(bool pAnonymous)
	{
		auto node = std::make_unique<AST_node_function_declaration>();
		node->related_token = *mIter;
		advance(); // Skip function
		if (!pAnonymous)
		{
			expect(token_type::identifier, "Expected function identifier");
			node->identifier = mIter->text;
			advance(); // Skip identifier
		}
		expect(token_type::l_parenthesis, "Expected (");
		advance(); // Skip (
		if (mIter->type != token_type::r_parenthesis)
		{
			expect(token_type::identifier, "Expected identifier for parameter");
			node->parameters.push_back(mIter->text);
			advance(); // Skip identifier
			while (mIter->type == token_type::separator)
			{
				advance(); // Skip ,
				expect(token_type::identifier, "Expected identifier for parameter");
				node->parameters.push_back(mIter->text);
				advance(); // Skip identifier
			}
		}
		expect(token_type::r_parenthesis, "Expected ) for function");
		advance(); // Skip )
		expect(token_type::l_brace, "Expected { for function");
		node->children.emplace_back(parse_compound_statement());
		return node;
	}

	void expect(token_type pToken, const char* pMsg) const
	{
		if (mIter->type != pToken)
			throw exception::parse_error(pMsg, *mIter);
	}

	void advance(std::size_t count = 1)
	{
		mIter += count;
		if (mIter == mTokens.end())
			throw exception::parse_error("Unexpected end of file", *mIter);
	}

	bool can_peek() const
	{
		return mIter != mTokens.end() &&
			++token_iterator(mIter) != mTokens.end();
	}

	token_iterator peek() const
	{
		return ++token_iterator(mIter);
	}

private:
	token_array mTokens;
	token_iterator mIter;
};


namespace exception
{
struct interpretor_error :
	std::runtime_error
{
	interpretor_error(const char* pMsg) :
		std::runtime_error(pMsg)
	{}
};
}


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
		std::cout << get_indent() << "Var\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_unary_op* pNode)
	{
		std::cout << get_indent() << "Unary Operation\n";
		++mIndent;
		pNode->children[0]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_binary_op* pNode)
	{
		std::cout << get_indent() << "Binary Operation\n";
		++mIndent;
		pNode->children[0]->visit(this);
		pNode->children[1]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_if* pNode)
	{
		std::cout << get_indent() << "If Statement\n";
		++mIndent;
		pNode->children[0]->visit(this);
		pNode->children[1]->visit(this);
		--mIndent;
	}

	virtual void dispatch(AST_node_constant* pNode)
	{
		std::cout << get_indent() << "Constant\n";
	}

	virtual void dispatch(AST_node_identifier* pNode)
	{
		std::cout << get_indent() << "Identifier\n";
	}

	virtual void dispatch(AST_node_function_call* pNode)
	{
		std::cout << get_indent() << "Function Call\n";
		++mIndent;
		for (auto& i : pNode->children)
			i->visit(this);
		--mIndent;
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

}