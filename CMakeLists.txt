cmake_minimum_required(VERSION 3.10)
project(pyglu)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 14)

find_package(PythonInterp 3.5 REQUIRED)
find_package(PythonLibs 3.5 REQUIRED)
message("-- Using Python: ${PYTHON_EXECUTABLE}")
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "from distutils import sysconfig; print(sysconfig.get_config_var('EXT_SUFFIX'), end='')" OUTPUT_VARIABLE PYTHON_EXTENSION_SUFFIX)
message("-- Using Python extension suffix: ${PYTHON_EXTENSION_SUFFIX}")

add_library(pyglu MODULE pyglu.cpp)
target_include_directories(pyglu PRIVATE "/usr/include/python3.5m")

set_property(TARGET pyglu PROPERTY PREFIX "")
set_property(TARGET pyglu PROPERTY SUFFIX ${PYTHON_EXTENSION_SUFFIX})
