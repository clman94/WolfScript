#pragma once

#include <string>
#include <string_view>
#include <exception>
#include <any>
#include <memory>
#include <algorithm>
#include <variant>
#include <map>
#include <vector>
#include <functional>
#include <set>
#include <variant>
#include <iostream>
#include <type_traits>

#define STRINGIFY(A) #A

namespace wolfscript
{

/*

Typing

  This language is mainly statically typed meaning that a variable declared with an int will always be an int.
However, soft typing through implicit variable declarations will allow basic generics.

Variables

  Variable declaration is inspired by Go in that they start with 'var', proceeded
by an identifier and an optional type.

// Defines a variable of type int
var myvariable int;

// A variable of type float initialized with the value 34
var myvariable2 float = 34;

// Defines a variable that is implicitly of type int.
// Its very simple deduction based on the result of the expression.
// This could also mean that the type of the expression can change
// at different points during runtime hence the soft typing mentioned before.
var myimplicitvariables = 34;


Functions

  Functions are fairly standard and lambda/anonymous functions are very similar in syntax.
Due to the nature of the language, you can define a function at any scope.

// A declaration of a function with an int and string parameter
// and a return type of int.
function myfunction(arg1 int, arg2 string) int
{
	return 5; // Return is very C-like
}

// Same function but as an anonymous function.
// Note: this example takes advantage of implicit typing.
var myfunction = function(arg1 int, arg2 string) int
{
	return 5;
};

// Same as example above but with explicit typing
var myfunction function(int, string) int = function(arg1 int, arg2 string) int
{
	return 5;
};

Classes

  Classes are shared objects meaning they are always passed around as a reference and
are destroyed when they are no longer referenced. If you want to create a value type,
create a struct instead.

class myclass
{
	// All methods and members are public by default
	
	// Contructor
	this()
	{
	}

	// Deconstructor
	~this()
	{
	}

	function mypublicmethod()
	{
	}

	private function myprivatemethod()
	{
	}

	var mypublicmember int;
	private var myprivatemember int;
}

// Initialization of a class object is just
// like calling a function.
// Note: This is using implicit typing
//   for convenience.
var myvar = myclass();

// myvar and myvar2 now reference the same object
var myvar2 myclass = myvar;

// Initializes this reference to null
// instead of creating a new myclass object.
var myvar3 myclass;

Structs

  Structs have all the features (members, methods, etc...) of classes except they are
treated like values and passed by value.

struct mystruct
{
}

// Creates a struct object
var myvar mystruct;

// myvar is copied into myvar2
var myvar2 = myvar;


Referencing value types

  You can refernce values by placing 'ref' after the type name
in a variable declaration. This can also make the lifetime of a
value go beyond its scope.

var myvar int = 23;

// myvar and myvarref now point to the same object
var myvarref int ref = myvar;

// Implicit typing
var myvarref ref = myref;

// refparam references an int with read-write access
// while crefparam has read-only access
function myfunction(refparam int ref, const crefparam int ref)
{
	
}

A rough sketch of the language's grammer (Note: This may be out of data):

<program> : <statement-list>

<statement-list> :  ( <statement> )*

<compound-statement> : '{' ( <statement> )* '}'

<statement> : <compound-statement> 
		    | <var-definition>
		    | <if-statement>
		    | <function-definition>
		    | <expression> <eol>
			| <eol>

<namespace-statement> : 'namespace' <namespace-identifier> <compound-statement>

<function-definition> : 'function' <identifier> '(' [ <param> { ',' <param> } ] ')' <compound-statement>

<if-statement> : 'if' '(' <expression> ')' <statement> [ 'else if' '(' <expression> ')' <statement> ]* [ 'else' <statement> ]

<try-statement> : 'try' block { catchstatment }
<catch-statment> : 'catch' '(' [ param { ',' param } ] ')' '{' block '}'

<param> : <identifier> [<typename>]

<class-definition> : 'class' <identifier> [ ':' <identifier> [ ',' <identifier> ]* ] '{' <class-body>* '}'

<class-body> : [ <class-access-modifiers> ] [ 'const' ] <function-definition>
             | [ <class-access-modifiers> ] <var-definition>

<class-access-modifiers> : 'public'
                         | 'private'
						 | 'protected'

<var-definition> : ('var' | 'const') <identifier> '=' <expr> <eol>
                 | ('var' | 'const') <identifier> <typename> [ '=' <expr> ] <eol>

<expression> : <assignment-expression>

<assignment-expression> : <ternay-expression> ( <assignment-operators> <ternay-expression> )*

<ternay-expression> : <logical-or-expression> ( '?' <logical-or-expression> ':' <ternay-expression> )

<logical-or-expression> : <logical-and-expression> ( '||' <logical-and-expression> )*

<logical-and-expression> : <assignment-expression> ( '&&' <assignment-expression> )*

<inclusive-or-expression> : <exclusive-or-expression> ( '|' <exclusive-or-expression> )*

<exclusive-or-expression> : <and-expression> ( '^' <and-expression> )*

<and-expression> : <equality-expression> ( '&' <equality-expression> )*

<equality-expression> : <relational-expression> ( ('==' | '!=') <relational-expression> )*

<relational-expression> :: <shift-expression> ( ('<' | '>' | '<=' | '>=') <shift-expression> )*

<shift-expression> ::= <additive-expression> ( ('<<' | '>>') <additive-expression>  )*

<additive-expression> : <multiplicative-expression> ( ('+' | '-') <multiplicative-expression> )*

<multiplicative-expression> : <member-accessor> ( ('*' | '/') <member-accessor> )*

<member-accessor> : <cast-expression> ( '.' <cast-expression> )*

<cast-expression> : <unary-expression>
                  | '(' <namespace-identifier> ')' <cast-expression>

<unary-expression> : <postfix-expression>
                   | ('++' | --') <unary-expression>

<postfix-expression> : <factor> ( '.' <identifier> )*

<factor> : '(' <expression> ')'
	     | <constant>
	     | <namespace-identifier>
	     | <function-call>
		 | <anonymous-function>

(* This will actually generate a class object with the '()' operator overloaded and private members as captures. *)
<anonymous-function> : 'function' (' [ <param> ( ',' <param> )* ] ')' <compound-statement>

<function-call> : <namespace-identifier> '(' [ <expr> [ ',' <expr> ]* ] ')'
                | <anonymous-function> '(' [ <expr> [ ',' <expr> ]* ] ')'

<namespace-identifier> : <identifier> ( '::' <identifier> )*

<identifier> : (* C-like *)

<assignment-operators> : '='
                       | '+='
					   | '-='
					   | '*='
					   | '/='
					   | '%='
					   | '>>='
					   | '<<='
					   | '&='
					   | '|='
					   | '^='

<prefix-unary-operators> : '+'
                         | '-'

<typename> : int
           | uint
		   | float
		   | double
		   | string
		   | <namespace-identifier>

<constant> : integer
           | float
		   | string

<eol> : ';'
*/

enum class token_type
{
	unknown,

	identifier,

	l_paranthesis,
	r_paranthesis,

	l_brace,
	r_brace,

	// Operations
	add,
	sub,
	mul,
	div,
	equ,
	not_equ,
	assign,

	less_than,
	greater_than,
	less_than_equ_to,
	greater_than_equ_to,

	logical_or,
	logical_and,

	separator,
	namespace_separator,

	period,

	// Types
	string,
	integer,
	floating,

	eol, // End of line
	eof, // End of file

	// Keywords
	kw_var,
	kw_const,
	kw_int,
	kw_uint,
	kw_float,
	kw_string,
	kw_if,
	kw_else,
	kw_for,
	kw_while,
	kw_class,
	kw_function,
	kw_return,
};

struct text_position
{
	int line;
	int column;
	text_position(int pLine = 1, int pColumn = 0) :
		line(pLine),
		column(pColumn)
	{}

	std::string to_string() const
	{
		return "(" + std::to_string(line) + ", " + std::to_string(column) + ")";
	}
};

struct token
{
	token_type type;
	std::string_view text;
	std::variant<int, float, std::string> value;
	text_position start_position;

	token() :
		type(token_type::unknown)
	{}
	token(token_type pType) :
		type(pType)
	{}

	bool is_operation() const
	{
		return type >= token_type::add &&
			type <= token_type::equ;
	}
};

namespace exception
{
struct tokenization_error :
	std::runtime_error
{
	tokenization_error(const char* pMsg) :
		std::runtime_error(pMsg)
	{}

	tokenization_error(const char* pMsg, text_position text_position) :
		std::runtime_error(pMsg),
		position(text_position)
	{}

	text_position position;
};
}

typedef std::vector<token> token_array;
typedef std::vector<token>::const_iterator token_iterator;

bool is_whitespace(char c)
{
	return
		c == ' ' ||
		c == '\t' ||
		c == '\n' ||
		c == '\r';
}

bool is_digit(char c)
{
	return c >= '0' &&
		c <= '9';
}

bool is_letter(char c)
{
	return (c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z');
}

std::string_view trim_whitespace_prefix(std::string_view pView, text_position& pPosition)
{
	auto iter = std::find_if(pView.begin(), pView.end(), [&pPosition](char c)
	{
		if (c == '\n')
		{
			++pPosition.line;
			pPosition.column = 0;
		}
		else
			++pPosition.column;
		return !is_whitespace(c);
	});
	pView.remove_prefix(std::distance(pView.begin(), iter));
	--pPosition.column; // Do not include the last character which was not a whitespace
	return pView;
}

token tokenize_identifier(std::string_view& pView, text_position& pPosition)
{
	int length = 0;
	for (auto i : pView)
	{
		if (is_letter(i) || is_digit(i))
			++length;
		else
			break;
	}
	token t;
	t.text = pView.substr(0, length);

	std::map<std::string_view, token_type> keywords =
	{
		{"var" , token_type::kw_var},
		{"const", token_type::kw_const},
		{"int" , token_type::kw_int},
		{"uint" , token_type::kw_uint},
		{"float" , token_type::kw_float},
		{"if" , token_type::kw_if},
		{"else" , token_type::kw_else},
		{"for" , token_type::kw_for},
		{"function" , token_type::kw_function},
		{"return" , token_type::kw_return}
	};
	if (keywords.find(t.text) != keywords.end())
		t.type = keywords[t.text];
	else
		t.type = token_type::identifier;
	t.start_position = pPosition;

	pPosition.column += length;
	pView.remove_prefix(length);

	return t;
}

token tokenize_number(std::string_view& pView, text_position& pPosition)
{
	int length = 0;
	bool is_float = false;
	for (char i : pView)
	{
		if (is_digit(i))
			++length;
		else if (i == '.')
		{
			is_float = true;
			++length;
		}
		else if (i == 'f')
		{
			is_float = true;
			++length;
			break;
		}
		else if (is_letter(i))
			throw exception::tokenization_error("This identifier should not start with a digit", pPosition);
		else
			break;
	}

	token t;
	t.text = pView.substr(0, length);
	t.start_position = pPosition;
	if (is_float)
	{
		t.type = token_type::floating;
		t.value = std::stof(std::string(t.text.begin(), t.text.end()));
	}
	else
	{
		t.type = token_type::integer;
		t.value = std::stoi(std::string(t.text.begin(), t.text.end()));
	}
	pPosition.column += length;
	pView.remove_prefix(length);

	return t;
}

token tokenize_char(std::string_view& pView, text_position& pPosition, token_type pType, std::size_t pLength = 1)
{
	token t;
	t.text = pView.substr(0, pLength);
	t.type = pType;
	t.start_position = pPosition;

	pPosition.column += pLength;
	pView.remove_prefix(pLength);

	return t;
}

token tokenize_string(std::string_view& pView, text_position& pPosition)
{
	pView.remove_prefix(1); // Skip "
	std::string result;
	int length = 0;
	for (; length < pView.length() && pView[length] != '\"'; length++)
	{
		// Parse escape sequence
		if (pView[length] == '\\')
		{
			// There should be room for one more character
			if (pView.length() - length < 2)
				throw exception::tokenization_error("Escape sequence at end of file", pPosition);
			++length; // Skip '\'
			switch (pView[length])
			{
			case 'n': result += '\n'; break;
			case 't': result += '\t'; break;
				// TODO: Add the rest of these escape sequences
			default:
				throw exception::tokenization_error("Invalid escape sequence", pPosition);
			}
		}
		else if (pView[length] != '\"')
			result += pView[length];
	}

	token t;
	if (length > 0)
		t.text = pView.substr(0, length - 1);
	t.type = token_type::string;
	t.start_position = pPosition;
	t.value = std::move(result);

	pPosition.column += length + 1;

	pView.remove_prefix(length + 1);
	return t;
}

bool query_multichar(const std::string_view& pView, const char* pComp)
{
	const std::size_t length = std::strlen(pComp);
	return pView.length() >= length && pView.substr(0, length) == pComp;
}

void skip_comment(std::string_view& pView, text_position& pPosition)
{
	auto i = pView.begin();
	i += 2; // Skip //
	for (; i != pView.end(); i++)
	{
		if (*i == '\n')
			break;
		++pPosition.column;
	}
	++pPosition.line;
	pView.remove_prefix(std::distance(pView.begin(), i));
}

void skip_multiline_comment(std::string_view& pView, text_position& pPosition)
{
	auto i = pView.begin();
	i += 2; // Skip /*
	for (; i != pView.end(); i++)
	{
		if (*i == '\n')
			++pPosition.line;
		++pPosition.column;
		if (*i == '*')
		{
			auto peek = i + 1;
			if (peek != pView.end() && *peek == '/')
			{
				i += 2; // Skip */
				++pPosition.column;
				break;
			}
		}
	}
	pView.remove_prefix(std::distance(pView.begin(), i));
}

token_array tokenize(std::string_view pView)
{
	token_array result;
	text_position current_position;

	pView = trim_whitespace_prefix(pView, current_position);
	while (!pView.empty())
	{
		const char c = pView.front();
		if (is_letter(c))
			result.push_back(tokenize_identifier(pView, current_position));
		else if (is_digit(c))
			result.push_back(tokenize_number(pView, current_position));
		else if (query_multichar(pView, "//"))
			skip_comment(pView, current_position);
		else if (query_multichar(pView, "/*"))
			skip_multiline_comment(pView, current_position);
		else if (query_multichar(pView, "=="))
			result.push_back(tokenize_char(pView, current_position, token_type::equ, 2));
		else if (query_multichar(pView, "!="))
			result.push_back(tokenize_char(pView, current_position, token_type::not_equ, 2));
		else if (query_multichar(pView, "||"))
			result.push_back(tokenize_char(pView, current_position, token_type::logical_or, 2));
		else if (query_multichar(pView, "<="))
			result.push_back(tokenize_char(pView, current_position, token_type::less_than_equ_to, 2));
		else if (query_multichar(pView, ">="))
			result.push_back(tokenize_char(pView, current_position, token_type::greater_than_equ_to, 2));
		else if (query_multichar(pView, "::"))
			result.push_back(tokenize_char(pView, current_position, token_type::namespace_separator, 2));
		else if (c == '<')
			result.push_back(tokenize_char(pView, current_position, token_type::less_than));
		else if (c == '>')
			result.push_back(tokenize_char(pView, current_position, token_type::greater_than));
		else if (c == '(')
			result.push_back(tokenize_char(pView, current_position, token_type::l_paranthesis));
		else if (c == ')')
			result.push_back(tokenize_char(pView, current_position, token_type::r_paranthesis));
		else if (c == '+')
			result.push_back(tokenize_char(pView, current_position, token_type::add));
		else if (c == '-')
			result.push_back(tokenize_char(pView, current_position, token_type::sub));
		else if (c == '*')
			result.push_back(tokenize_char(pView, current_position, token_type::mul));
		else if (c == '/')
			result.push_back(tokenize_char(pView, current_position, token_type::div));
		else if (c == '=')
			result.push_back(tokenize_char(pView, current_position, token_type::assign));
		else if (c == ';')
			result.push_back(tokenize_char(pView, current_position, token_type::eol));
		else if (c == ',')
			result.push_back(tokenize_char(pView, current_position, token_type::separator));
		else if (c == '{')
			result.push_back(tokenize_char(pView, current_position, token_type::l_brace));
		else if (c == '}')
			result.push_back(tokenize_char(pView, current_position, token_type::r_brace));
		else if (c == '.')
			result.push_back(tokenize_char(pView, current_position, token_type::period));
		else if (c == '\"')
			result.push_back(tokenize_string(pView, current_position));
		else
			throw exception::tokenization_error("Unknown character", current_position);
		pView = trim_whitespace_prefix(pView, current_position);
	}
	result.emplace_back(token_type::eof);
	return result;
}

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
		expect(token_type::l_paranthesis, "Expected ( for if statement conditional expression");
		advance(); // Skip (
		if (mIter->type == token_type::r_paranthesis)
			throw exception::parse_error("Missing if statement conditional expression", *mIter);

		node->children.emplace_back(parse_expression());

		expect(token_type::r_paranthesis, "Expected )");
		advance(); // Skip )

		node->children.emplace_back(parse_statement());

		while (can_peek()
			&& mIter->type == token_type::kw_else
			&& peek()->type == token_type::kw_if)
		{
			advance(2); // Skip else if
			expect(token_type::l_paranthesis, "Expected ( for else if statement conditional expression");
			advance(); // Skip (
			if (mIter->type == token_type::r_paranthesis)
				throw exception::parse_error("Missing if statement conditional expression", *mIter);

			node->children.emplace_back(parse_expression());

			expect(token_type::r_paranthesis, "Expected )");
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
		expect(token_type::l_paranthesis, "Expected ( for 'for' statement");
		advance(); // Skip (
		if (mIter->type == token_type::r_paranthesis)
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
		if (mIter->type != token_type::r_paranthesis)
		{
			auto third_node = parse_expression();

			expect(token_type::r_paranthesis, "Expected ;");
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
		if (mIter->type == token_type::l_paranthesis)
		{
			auto node = std::make_unique<AST_node_function_call>();
			node->children.emplace_back(std::move(factor));
			advance(); // Skip (

			// No arguments
			if (mIter->type == token_type::r_paranthesis)
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

			expect(token_type::r_paranthesis, "Expected )");
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
		else if (mIter->type == token_type::l_paranthesis)
		{
			advance(); // Skip (
			auto node = parse_expression();
			expect(token_type::r_paranthesis, "Expected )");
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
		expect(token_type::l_paranthesis, "Expected (");
		advance(); // Skip (
		if (mIter->type != token_type::r_paranthesis)
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
		expect(token_type::r_paranthesis, "Expected ) for function");
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
