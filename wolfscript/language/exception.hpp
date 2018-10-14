#pragma once

#include "token.hpp"
#include <exception>

namespace wolfscript::exception
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

struct interpretor_error :
	std::runtime_error
{
	interpretor_error(const char* pMsg) :
		std::runtime_error(pMsg)
	{}
};

} // namespace wolfscript::exception
