#include "wolfscript/wolfscript.hpp"

#include <fstream>
#include <sstream>

std::string load_file_as_string(const std::string& pPath)
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
	using namespace wolfscript;

	// Try catch commented out so the debugger can catch the exceptions instead
	//try {
	std::string code = load_file_as_string("../script.txt");

	parser myparser;
	auto ast = myparser.parse(tokenize(code));

	AST_viewer viewer;
	ast->visit(&viewer);

	value_type::object myobj;
	myobj.members["x"] = 23;
	myobj.members["y"] = 32;
	value_type::callable mytostring;
	mytostring.func = [](const std::vector<value_type>& pArgs) -> value_type
	{
		auto obj = pArgs[0].get<value_type::object>();
		return std::string("(" + obj->members["x"].to_string() + ", "
			+ obj->members["y"].to_string() + ")");
	};
	myobj.members["__to_string"] = mytostring;
	value_type::callable myassign;
	myassign.func = [](const std::vector<value_type>& pArgs) -> value_type
	{
		auto obj = pArgs[0].get<value_type::object>();
		auto obj2 = pArgs[1].get<value_type::object>();
		value_type::binary_operation(token_type::assign, obj->members["x"], obj2->members["x"]);
		value_type::binary_operation(token_type::assign, obj->members["y"], obj2->members["y"]);
		return pArgs[0];
	};
	myobj.members["__assign"] = myassign;

	interpretor interp;

	// Register string factory
	// This will generate the object to access and modify the string
	interp.set_string_factory([](const std::string& pString) -> value_type
	{
		value_type::object obj;
		obj.members["__string"] = pString;
		obj.members["__to_string"] = value_type::callable{
			[](const value_type::arg_list& pArgs) -> value_type
		{
			auto obj = pArgs[0].get<const value_type::object>();
			auto str = obj->members.find("__string")->second.get<const std::string>();
			return *str;
		}};
		obj.members["length"] = value_type::callable{
			[](const value_type::arg_list& pArgs) -> value_type
		{
			auto obj = pArgs[0].get<const value_type::object>();
			auto str = obj->members.find("__string")->second.get<const std::string>();
			return str->length();
		}};
		return std::move(obj);
	});
	interp["myobj"] = std::ref(myobj);

	// Register print function for testing
	value_type::callable myprint;
	myprint.func = [](const std::vector<value_type>& pArgs) -> value_type
	{
		std::cout << "print(" << pArgs[0].to_string() << ")\n";
		return{};
	};
	interp["print"] = value_type::create_const(&myprint);

	const int pie = 2;
	interp["pie"] = &pie;
	interp["pie"] = interp["pie"] + interp["pie"];

	interp.interpret(ast.get());		
	/*}
	catch (exception::tokenization_error& e)
	{
		std::cout << "Error in tokenization : " << e.what() << " : " << e.position.to_string() << "\n";
	}
	catch (exception::parse_error& e)
	{
		std::cout << "Error in parser : " << e.what() << " : " << e.current_token.start_position.to_string() << "\n";
	}
	catch (exception::interpretor_error& e)
	{
		std::cout << "Error in interpretor : " << e.what() << "\n";
	}*/
}