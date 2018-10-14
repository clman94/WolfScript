#pragma once

#include "token.hpp"
#include "exception.hpp"
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <algorithm>
#include <exception>

namespace wolfscript
{

namespace detail
{

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
		if (is_letter(i) || is_digit(i) || i == '_')
			++length;
		else
			break;
	}

	token t;
	t.text = pView.substr(0, length);

	// Lookup table for keywords and their respective token_type value
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

} // namespace detail

// Convert a string into an array of tokens for the parser.
// To ensure efficiency, the tokenizer does not store strings copied
// from the original. It only keeps references to sections of it, so
// you MUST keep the string alive as long as you are using the tokens
// generated.
// This will throw a tokenization_error exception on an error.
token_array tokenize(std::string_view pView)
{
	using namespace detail;

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
		else if (query_multichar(pView, "+="))
			result.push_back(tokenize_char(pView, current_position, token_type::add_assign, 2));
		else if (query_multichar(pView, "-="))
			result.push_back(tokenize_char(pView, current_position, token_type::sub_assign, 2));
		else if (query_multichar(pView, "*="))
			result.push_back(tokenize_char(pView, current_position, token_type::mul_assign, 2));
		else if (query_multichar(pView, "/="))
			result.push_back(tokenize_char(pView, current_position, token_type::div_assign, 2));
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
			result.push_back(tokenize_char(pView, current_position, token_type::l_parenthesis));
		else if (c == ')')
			result.push_back(tokenize_char(pView, current_position, token_type::r_parenthesis));
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

} // namespace wolfscript
