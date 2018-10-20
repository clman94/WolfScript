#pragma once

#include "token.hpp"
#include "exception.hpp"
#include <memory>
#include <set>
#include <iostream>

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
		else if (mIter->type == token_type::kw_function
			&& can_peek() && peek()->type == token_type::identifier)
		{
			return parse_function_declaration(false);
		}
		else if (mIter->type == token_type::kw_for)
		{
			return parse_for_statement();
		}
		else if (mIter->type == token_type::kw_while)
		{
			return parse_while_statement();
		}
		else if (mIter->type == token_type::kw_break)
		{
			advance(); // Skip break
			expect(token_type::eol, "Expected ;");
			advance(); // Skip ;
			return std::make_unique<AST_node_break>();
		}
		else if (mIter->type == token_type::kw_continue)
		{
			advance(); // Skip continue
			expect(token_type::eol, "Expected ;");
			advance(); // Skip ;
			return std::make_unique<AST_node_continue>();
		}
		else if (mIter->type == token_type::eol)
		{
			auto node = std::make_unique<AST_node_empty>();
			advance(); // Skip ;
			return node;
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
		auto node = std::make_unique<AST_node_for>();

		advance(); // Skip for
		expect(token_type::l_parenthesis, "Expected ( for 'for' statement");
		advance(); // Skip (
		if (mIter->type == token_type::r_parenthesis)
			throw exception::parse_error("Missing 'for' statement expression", *mIter);

		// Var statement/expression
		if (mIter->type == token_type::eol)
		{
			node->children.emplace_back(std::make_unique<AST_node_empty>());
			advance(); // Skip ;
		}
		else if (mIter->type == token_type::kw_var)
		{
			node->children.emplace_back(parse_var()); // Already checks for ;
		}
		else
		{
			node->children.emplace_back(parse_expression());
			expect(token_type::eol, "Expected ;");
			advance(); // Skip ;
		}

		// Conditional
		if (mIter->type == token_type::eol)
		{
			node->children.emplace_back(std::make_unique<AST_node_empty>());
		}
		else
		{
			node->children.emplace_back(parse_expression());
		}
		expect(token_type::eol, "Expected ;");
		advance(); // Skip ;

		// Looped expression
		if (mIter->type == token_type::r_parenthesis)
		{
			node->children.emplace_back(std::make_unique<AST_node_empty>());
		}
		else
		{
			node->children.emplace_back(parse_expression());
		}
		expect(token_type::r_parenthesis, "Expected )");
		advance(); // Skip )

		node->children.emplace_back(parse_statement());

		return node;
	}

	std::unique_ptr<AST_node> parse_while_statement()
	{
		auto node = std::make_unique<AST_node_while>();

		advance(); // Skip while
		expect(token_type::l_parenthesis, "Expected ( for 'while' statement");
		advance(); // Skip (
		if (mIter->type == token_type::r_parenthesis)
			throw exception::parse_error("Missing 'while' statement expression", *mIter);

		node->children.emplace_back(parse_equality());
		expect(token_type::r_parenthesis, "Missing ) for 'while' statement");
		advance(); // Skip )

		node->children.emplace_back(parse_statement());

		return node;
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
		return parse_binary_expression({ token_type::assign,
			token_type::add_assign, token_type::sub_assign,
			token_type::mul_assign, token_type::div_assign },
			&parser::parse_logical_or);
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
		return parse_binary_expression({ token_type::mul, token_type::div }, &parser::parse_postfix_expression);
	}

	std::unique_ptr<AST_node> parse_postfix_expression()
	{
		auto node = parse_factor();

		bool has_postfix = false;
		do {
			has_postfix = false;
			if (mIter->type == token_type::period)
			{
				advance(); // Skip .
				expect(token_type::identifier, "Expected identifier");
				auto access_node = std::make_unique<AST_node_member_accessor>();
				access_node->identifier = mIter->text;
				access_node->children.emplace_back(std::move(node));
				node = std::move(access_node);
				advance(); // Skip identifier
				has_postfix = true;
			}
			else if (mIter->type == token_type::l_parenthesis)
			{
				node = parse_function_call(std::move(node));
				has_postfix = true;
			}
		} while (has_postfix);

		return node;
	}

	std::unique_ptr<AST_node> parse_function_call(std::unique_ptr<AST_node> pCaller)
	{
		auto node = std::make_unique<AST_node_function_call>();
		node->children.emplace_back(std::move(pCaller));
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
			return parse_function_declaration(true);
		}
		else
			throw exception::parse_error("Unexpected token", *mIter);
	}

	AST_node_function_declaration::param parse_parameter()
	{
		AST_node_function_declaration::param param;
		if (mIter->type == token_type::kw_const)
		{
			param.is_const = true;
			advance(); // Skip const
		}
		expect(token_type::identifier, "Expected identifier for parameter");
		param.identifier = mIter->text;
		advance(); // Skip identifier
		if (mIter->type == token_type::identifier)
		{
			param.has_type = true;
			param.type = *mIter;
			advance(); // Skip type identifier
		}
		return param;
	}

	std::unique_ptr<AST_node> parse_function_declaration(bool pAnonymous)
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
			node->parameters.emplace_back(parse_parameter());
			while (mIter->type == token_type::separator)
			{
				advance(); // Skip ,
				node->parameters.emplace_back(parse_parameter());
			}
		}
		expect(token_type::r_parenthesis, "Expected ) for function");
		advance(); // Skip )
		if (mIter->type == token_type::identifier)
		{
			node->has_return_type = true;
			node->return_type = *mIter;
			advance(); // Skip type identifier
		}

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

} // namespace wolfscript
