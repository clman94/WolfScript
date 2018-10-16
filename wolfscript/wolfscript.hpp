#pragma once

#include "language/tokenizer.hpp"
#include "language/parser.hpp"
#include "language/interpretor.hpp"
#include "language/function.hpp"

#include <fstream>
#include <sstream>

namespace wolfscript
{

class script
{
public:
	script()
	{
		register_string_factory();
	}

	void load_file(const std::string& pFilepath)
	{
		mScript = load_file_as_string(pFilepath);

		parser p;
		mRoot_node = p.parse(tokenize(mScript));
	}

	// Execute all code in the global scope
	void execute()
	{
		mInterpretor.interpret(mRoot_node.get());
	}

	value_type& operator[](const std::string& pIdentifier)
	{
		return mInterpretor[pIdentifier];
	}

	AST_node* get_ast()
	{
		return mRoot_node.get();
	}

	const AST_node* get_ast() const
	{
		return mRoot_node.get();
	}

private:
	static std::string load_file_as_string(const std::string& pPath)
	{
		std::ifstream stream(pPath.c_str());
		if (!stream.good())
		{
			std::cout << "Could not load file \"" << pPath << "\"\n";
			std::getchar();
			return{};
		}
		std::stringstream sstr;
		sstr << stream.rdbuf();
		return sstr.str();
	}

private:
	void register_string_factory()
	{
		// Register the string factory
		// This will generate the object to access and modify the string
		mInterpretor.set_string_factory([](const std::string& pString) -> value_type
		{
			value_type::object obj;
			obj.members[object_behavior::object] = pString;
			obj.members[object_behavior::to_string] = value_type::callable{
				[](const value_type::arg_list& pArgs) -> value_type
			{
				auto obj = pArgs[0].get<const value_type::object>();
				auto str = obj->members.find(object_behavior::object)->second.get<const std::string>();
				return *str;
			} };
			obj.members[object_behavior::add_assign]
				= value_type::callable{ function(this_first, [](std::string* This, const std::string& pString)
			{
				*This = pString;
			}).function };
			obj.members[object_behavior::copy]
				= value_type::callable{ function(this_first, [](std::string* This, const std::string& pString)
			{

			}).function };
			obj.members["length"] = value_type::callable{
				[](const value_type::arg_list& pArgs) -> value_type
			{
				auto obj = pArgs[0].get<const value_type::object>();
				auto str = obj->members.find(object_behavior::object)->second.get<const std::string>();
				return str->length();
			} };
			return std::move(obj);
		});
	}

private:
	std::string mScript;
	std::unique_ptr<AST_node> mRoot_node;
	interpretor mInterpretor;
};

}
