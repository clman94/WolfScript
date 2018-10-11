cmake_minimum_required(VERSION 3.0)

project(FunLang)

# Use latest standard in Visual Studio
if (MSVC_VERSION GREATER_EQUAL "1900")
	include(CheckCXXCompilerFlag)
	CHECK_CXX_COMPILER_FLAG("/std:c++latest" _cpp_latest_flag_supported)
	if (_cpp_latest_flag_supported)
		add_compile_options("/std:c++latest")
	endif()
endif()

add_executable(FunLang main.cpp wolfscript.hpp)