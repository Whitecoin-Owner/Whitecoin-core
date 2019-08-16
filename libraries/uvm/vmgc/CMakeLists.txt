cmake_minimum_required(VERSION 3.10)
project(vmgc)

set(CMAKE_CXX_STANDARD 14)

enable_testing()

include_directories(include)
add_library(
    vmgc
    src/vmgc.cpp 
    src/gcstate.cpp
    src/gcobject.cpp
    include/vmgc/vmgc.h 
    include/vmgc/gcstate.h
    include/vmgc/gcobject.h
	include/vmgc/exceptions.h
)

set(BOOST_ROOT $ENV{BOOST_ROOT})

# option
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread date_time system filesystem program_options signals serialization chrono unit_test_framework context locale regex iostreams)

find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_executable(vmgctest test/test_runner.cpp)
target_link_libraries(vmgctest vmgc ${Boost_LIBRARIES})

add_test(vmgctest vmgctest)
