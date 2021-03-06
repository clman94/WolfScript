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
	mod,

	equ,
	not_equ,

	increment,
	decrement,

	assign,
	add_assign,
	sub_assign,
	mul_assign,
	div_assign,

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

	// Type Keywords
	kw_int,
	kw_uint,
	kw_float,
	kw_string,

	// Keywords
	kw_var,
	kw_const,
	kw_if,
	kw_else,
	kw_for,
	kw_while,
	kw_class,
	kw_function,
	kw_return,
	kw_break,
	kw_continue,

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
	"Modulus",

	"Equal",
	"Not Equal",

	"Increment",
	"Decrement",

	"Assign",
	"Add Assign",
	"Subtract Assign",
	"Multiply Assign",
	"Divide Assign",

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

	"Keyword int",
	"Keyword uint",
	"Keyword float",
	"Keyword string",

	"Keyword var",
	"Keyword const",
	"Keyword if",
	"Keyword else",
	"Keyword for",
	"Keyword while",
	"Keyword class",
	"Keyword function",
	"Keyword return",
	"Keyword break",
	"Keyword continue",
};

// Represents a position in text
struct text_position
{
	// Starts at 1, -1 is an unknown position
	int line;
	// Starts at 0, -1 is an unknown position
	int column;

	constexpr text_position(int pLine = 1, int pColumn = 0) :
		line(pLine),
		column(pColumn)
	{}
	constexpr text_position(const text_position&) = default;
	constexpr text_position& operator=(const text_position&) = default;

	constexpr bool operator==(const text_position& pOther) const
	{
		return line == pOther.line && column == pOther.column;
	}

	// Gets the line position as an index (starting at 0)
	constexpr int line_index() const
	{
		return line - 1;
	}

	constexpr void new_line()
	{
		++line;
		column = 0;
	}

	// Returns a string, "([line], [column])"
	std::string to_string() const
	{
		return "(" + std::to_string(line) + ", " + std::to_string(column) + ")";
	}
};

constexpr text_position unknown_position(-1, -1);

struct token
{
	// The representation of this token
	token_type type{ token_type::unknown };
	// Reference to the source for this token
	std::string_view text;
	// Value associated with string and arithmetic types
	std::variant<int, float, std::string> value;
	// Position in the source
	text_position position;

	token() = default;
	token(const token&) = default;
	token(token&&) = default;
	token(token_type pType) :
		type(pType)
	{}

	token& operator=(const token&) = default;
	token& operator=(token&&) = default;
};

typedef std::vector<token> token_array;
typedef std::vector<token>::const_iterator token_iterator;

} // namespace wolfscript
