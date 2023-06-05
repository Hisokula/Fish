cmake_minimum_required(VERSION 3.25)

set(PROJECT_NAME test)		 
project("${PROJECT_NAME}")

set(CMAKE_CXX_STANDARD 17)	
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

add_executable("${PROJECT_NAME}" WORK.cpp 
					       ext/sqlite/sqlite3.c)