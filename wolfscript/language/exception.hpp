#pragma once

#include "token.hpp"
#include <exception>

namespace wolfscript::exception
{

// Base class for all wolfscript exceptions.
struct wolf_exception :
	std::runtime_error
{
	wolf_exception(const char* pMsg) :
		std::runtime_error(pMsg)
	{}
};

struct tokenization_error :
	wolf_exception
{
	tokenization_error(const char* pMsg) :
		wolf_exception(pMsg)
	{}

	tokenization_error(const char* pMsg, text_position pPosition) :
		wolf_exception(pMsg),
		position(pPosition)
	{}

	text_position position;
};

struct parse_error :
	wolf_exception
{
	parse_error(const char* pMsg) :
		wolf_exception(pMsg)
	{}

	parse_error(const char* pMsg, token pToken) :
		wolf_exception(pMsg),
		current_token(pToken)
	{}

	token current_token;
};

struct interpretor_error :
	wolf_exception
{
	interpretor_error(const char* pMsg) :
		wolf_exception(pMsg)
	{}
};

} // namespace wolfscript::exception
