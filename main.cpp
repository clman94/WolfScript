#include "funparsing.hpp"

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
	//try {
	std::string code = load_file_as_string("./script.txt");

	auto tokens = parser_fun::tokenize(code);

	parser_fun::parser parser;

	auto ast = parser.parse(tokens);

	parser_fun::AST_viewer viewer;
	ast->visit(&viewer);

	const int pie = 2;

	parser_fun::value_type_class2::callable myprint;
	myprint.func = [](const std::vector<parser_fun::value_type_class2>& pArgs) -> parser_fun::value_type_class2
	{
		std::cout << "print(" << pArgs[0].to_string() << ")\n";
		return{};
	};

	parser_fun::value_type_class2::object myobj;
	myobj.members["x"] = 23;
	myobj.members["y"] = 32;

	parser_fun::value_type_class2::callable mytostring;
	mytostring.func = [](const std::vector<parser_fun::value_type_class2>& pArgs) -> parser_fun::value_type_class2
	{
		auto obj = pArgs[0].get<parser_fun::value_type_class2::object>();
		return std::string("(" + obj->members["x"].to_string() + ", "
			+ obj->members["y"].to_string() + ")");
	};
	myobj.members["__to_string"] = mytostring;

	parser_fun::interpretor interp;
	interp["myobj"] = std::ref(myobj);
	interp["print"] = std::cref(myprint);
	interp["pie"] = &pie;
	interp["pie"] = interp["pie"] + interp["pie"];
	interp.interpret(ast.get());		
	/*}
	catch (parser_fun::exception::tokenization_error& e)
	{
		std::cout << "Error in tokenization : " << e.what() << " : " << e.position.to_string() << "\n";
	}
	catch (parser_fun::exception::parse_error& e)
	{
		std::cout << "Error in parser : " << e.what() << " : " << e.current_token.start_position.to_string() << "\n";
	}
	catch (parser_fun::exception::interpretor_error& e)
	{
		std::cout << "Error in interpretor : " << e.what() << "\n";
	}*/
}