cmake_minimum_required(VERSION 3.0)

project(WolfScript)

# Use latest standard in Visual Studio
if (MSVC_VERSION GREATER_EQUAL "1900")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
	if (_cpp_latest_flag_supported)
		add_compile_options("/std:c++latest")
	endif()
endif()

add_executable(WolfScript
	main.cpp
	../wolfscript/wolfscript.hpp
	../wolfscript/language/value_type.hpp
	../wolfscript/language/arithmetic.hpp
	../wolfscript/language/cast.hpp
	../wolfscript/language/common.hpp
	../wolfscript/language/callable.hpp
	../wolfscript/language/type_info.hpp
	../wolfscript/language/exception.hpp
	../wolfscript/language/function.hpp
	../wolfscript/language/token.hpp
	../wolfscript/language/tokenizer.hpp
	../wolfscript/language/parser.hpp
	../wolfscript/language/interpreter.hpp)
