#include "../wolfscript/wolfscript.hpp"
#include "../wolfscript/language/function.hpp"

#include <fstream>
#include <sstream>

int main()
{
	using namespace wolfscript;

	// Try catch commented out so the debugger can catch the exceptions instead
	//try {
	script myscript;
	myscript.load_file("../script.wolf");

	AST_viewer viewer;
	myscript.get_ast()->visit(&viewer);

	value_type::object myobj;
	myobj.members["x"] = 23;
	myobj.members["y"] = 32;
	value_type::callable mytostring;
	mytostring.function = [](const std::vector<value_type>& pArgs) -> value_type
	{
		auto obj = pArgs[0].get<value_type::object>();
		return std::string("(" + obj->members["x"].to_string() + ", "
			+ obj->members["y"].to_string() + ")");
	};
	myobj.members[object_behavior::to_string] = mytostring;
	value_type::callable myassign;
	myassign.function = [](const value_type::arg_list& pArgs) -> value_type
	{
		auto obj = pArgs[0].get<value_type::object>();
		auto obj2 = pArgs[1].get<value_type::object>();
		value_type::binary_operation(token_type::assign, obj->members["x"], obj2->members["x"]);
		value_type::binary_operation(token_type::assign, obj->members["y"], obj2->members["y"]);
		return pArgs[0];
	};
	myobj.members[object_behavior::assign] = myassign;

	myscript["myobj"] = std::ref(myobj);

	auto printfun = [](const std::string& pStr)
	{
		std::cout << pStr;
	};
	auto printfunbinding = wolfscript::function(printfun);
	printfunbinding.function({ std::string("pie is great") });

	// Register print function for testing
	value_type::callable myprint;
	myprint.function = [](const std::vector<value_type>& pArgs) -> value_type
	{
		std::cout << "print(" << pArgs[0].to_string() << ")\n";
		return{};
	};
	myscript["print"] = value_type::create_const(&myprint);

	myscript.execute();
		
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