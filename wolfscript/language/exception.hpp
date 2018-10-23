#pragma once

#include "token.hpp"
#include <exception>
#include <vector>

namespace wolfscript::exception
{

// Base class for all wolfscript exceptions.
struct wolf_exception :
	std::runtime_error
{
	wolf_exception(const std::string& pMsg) :
		std::runtime_error(pMsg)
	{}

	wolf_exception(const std::string& pMsg, text_position pPosition) :
		std::runtime_error(pMsg),
		position(pPosition)
	{}

	text_position position;
};

struct tokenization_error :
	wolf_exception
{
	tokenization_error(const std::string& pMsg) :
		wolf_exception(pMsg)
	{}

	tokenization_error(const std::string& pMsg, text_position pPosition) :
		wolf_exception(pMsg, pPosition)
	{}
};

struct parse_error :
	wolf_exception
{
	parse_error(const std::string& pMsg) :
		wolf_exception(pMsg)
	{}

	parse_error(const std::string& pMsg, token pToken) :
		wolf_exception(pMsg, pToken.position),
		current_token(pToken)
	{}

	token current_token;
};

struct interpretor_error :
	wolf_exception
{
	interpretor_error(const std::string& pMsg) :
		wolf_exception(pMsg)
	{}
	std::vector<std::string> stack;
};

} // namespace wolfscript::exception
