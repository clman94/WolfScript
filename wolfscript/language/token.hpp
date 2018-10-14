#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace wolfscript
{

enum class token_type
{
	unknown,

	identifier,

	l_parenthesis,
	r_parenthesis,

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

	count,
};

constexpr const char* token_name[static_cast<unsigned int>(token_type::count)] =
{
	"Unknown",
	"Identifier",

	"Left Parenthesis",
	"Right Parenthesis",

	"Left Brace",
	"Right Brace",

	"Add",
	"Subtract",
	"Multiply",
	"Divide",
	"Equal",
	"Not Equal",
	"Assign",

	"Less Than",
	"Greater Than",
	"Less Than Equal To",
	"Greater Than Equal To",

	"Logical Or",
	"Logical And",

	"Separator",
	"Namespace Separator",

	"Period",

	"String Constant",
	"Integer Constant",
	"Floating-point Constant",

	"End of line",
	"End of file",

	"Keyword var",
	"Keyword const",
	"Keyword int",
	"Keyword uint",
	"Keyword float",
	"Keyword string",
	"Keyword if",
	"Keyword else",
	"Keyword for",
	"Keyword while",
	"Keyword class",
	"Keyword function",
	"Keyword return",
};

// Represents a position in text
struct text_position
{
	int line;
	int column;

	constexpr text_position(int pLine = 1, int pColumn = 0) :
		line(pLine),
		column(pColumn)
	{}
	constexpr text_position(const text_position&) = default;
	constexpr text_position& operator=(const text_position&) = default;


	std::string to_string() const
	{
		return "(" + std::to_string(line) + ", " + std::to_string(column) + ")";
	}
};

constexpr text_position unknown_position(-1, -1);

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

typedef std::vector<token> token_array;
typedef std::vector<token>::const_iterator token_iterator;

} // namespace wolfscript
