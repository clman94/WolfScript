#include "../wolfscript/wolfscript.hpp"
#include "../wolfscript/language/function.hpp"

#include <fstream>
#include <sstream>

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

int main()
{
	wolfscript::parser parser;
	const std::string code = load_file_as_string("../script.wolf");
	wolfscript::AST_node::ptr ast = parser.parse(wolfscript::tokenize(code));

	wolfscript::AST_viewer viewer;
	ast->visit(&viewer);

	wolfscript::interpreter interpreter;
	interpreter.add_type<int>("int");
	interpreter.add_type<std::string>("string");
	interpreter.add("string", wolfscript::function([](const std::string& pStr)
	{
		return std::string(pStr);
	}));
	interpreter.add("string", wolfscript::function([]()
	{
		return std::string();
	}));
	interpreter.add("copy", wolfscript::function([](const std::string& pStr)
	{
		return std::string(pStr);
	}));
	interpreter.add(wolfscript::object_behavior::add, wolfscript::function([](const std::string& l, const std::string& r)
	{
		return l + r;
	}));
	interpreter.add(wolfscript::object_behavior::assign, wolfscript::function([](std::string& l, const std::string& r)
	{
		l = r;
	}));
	interpreter.add("myfunc", wolfscript::function([](const std::string& pThis)
	{
		return wolfscript::function([pThis]()
		{
			std::cout << "myfunc" << pThis << "\n";
		});
	}));
	interpreter.add("print", wolfscript::function([](const std::string& pStr)
	{
		std::cout << pStr;
	}));
	interpreter.add("print", wolfscript::function([](int pInt)
	{
		std::cout << pInt;
	}));
	interpreter.interpret(ast);
}