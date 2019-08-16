cmake_minimum_required(VERSION 3.5)

project(uvm)

set(CMAKE_CXX_STANDARD 11)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "-std=c++11 -D_DARWIN_C_SOURCE -Wno-invalid-partial-specialization")
endif()

file(GLOB HEADERS "include/uvm/*.h" "include/cborcpp/*.h" "include/cbor_diff/*.h" "include/safenumber/*.h" "include/native_contract/*.h" "../jsondiff/jsondiff-cpp/include/jsondiff/*.h" "vmgc/include/vmgc/*.h")

set(SOURCES src/uvm/lapi.cpp
    src/uvm/lauxlib.cpp
    src/uvm/lbaselib.cpp
    src/uvm/lbitlib.cpp
    src/uvm/lcode.cpp
    src/uvm/lcorolib.cpp
    src/uvm/lctype.cpp
    src/uvm/ldblib.cpp
    src/uvm/ldebug.cpp
    src/uvm/ldo.cpp
    src/uvm/ldump.cpp
    src/uvm/lfunc.cpp
    src/uvm/lgc.cpp
    src/uvm/lhashmap.cpp
    src/uvm/lhttplib.cpp
    src/uvm/linit.cpp
    src/uvm/liolib.cpp
    src/uvm/ljsonlib.cpp
    src/uvm/ljsonlib2.cpp
    src/uvm/json_reader.cpp
    src/uvm/ljsonrpclib.cpp
    src/uvm/llex.cpp
    src/uvm/lmathlib.cpp
    src/uvm/lsafemathlib.cpp
    src/uvm/lmem.cpp
    src/uvm/lnetlib.cpp
    src/uvm/loadlib.cpp
    src/uvm/lobject.cpp
    src/uvm/lopcodes.cpp
    src/uvm/loslib.cpp
    src/uvm/lparser.cpp
    src/uvm/lprefix.cpp
    src/uvm/lstate.cpp
    src/uvm/lstring.cpp
    src/uvm/lstrlib.cpp
    src/uvm/ltable.cpp
    src/uvm/ltablib.cpp
    src/uvm/ltimelib.cpp
    src/uvm/ltm.cpp
    src/uvm/lundump.cpp
    src/uvm/lutf8lib.cpp
    src/uvm/lvm.cpp
    src/uvm/lzio.cpp
    src/uvm/uvm_api_types.cpp
    src/uvm/uvm_lib.cpp
    src/uvm/uvm_lutil.cpp
    src/uvm/uvm_state_scope.cpp
    src/uvm/uvm_storage.cpp
    src/uvm/uvm_tokenparser.cpp

	src/cborcpp/cbor_object.cpp
	src/cborcpp/decoder.cpp
	src/cborcpp/encoder.cpp
	src/cborcpp/input.cpp
	src/cborcpp/output_dynamic.cpp
	src/cborcpp/output_static.cpp
	src/cborcpp/output.cpp
	
	vmgc/src/gcobject.cpp
	vmgc/src/gcstate.cpp
	vmgc/src/vmgc.cpp

	src/cbor_diff/cbor_diff.cpp
	src/cbor_diff/cbor_diff_tests.cpp
	src/cbor_diff/helper.cpp

	src/safenumber/safenumber.cpp

	src/native_contract/native_token_contract.cpp
	src/native_contract/native_exchange_contract.cpp
	src/native_contract/native_uniswap_contract.cpp
)

find_package(OpenSSL REQUIRED)

SET(BOOST_COMPONENTS)
LIST(APPEND BOOST_COMPONENTS thread iostreams date_time system filesystem program_options  signals       serialization  chrono unit_test_framework context locale regex  )
				SET( Boost_USE_STATIC_LIBS ON CACHE STRING "ON or OFF" )

	IF( WIN32 )
	SET(BOOST_ROOT $ENV{BOOST_ROOT})
	set(Boost_USE_MULTITHREADED ON)
	set(BOOST_ALL_DYN_LINK OFF) # force dynamic linking for all libraries
	  ENDIF(WIN32)
	  FIND_PACKAGE(Boost 1.57 REQUIRED COMPONENTS ${BOOST_COMPONENTS})
	# For Boost 1.53 on windows, coroutine was not in BOOST_LIBRARYDIR and do not need it to build,  but if boost versin >= 1.54, find coroutine otherwise will cause link errors
	IF(NOT "${Boost_VERSION}" MATCHES "1.53(.*)")
	  SET(BOOST_LIBRARIES_TEMP ${Boost_LIBRARIES})
	  FIND_PACKAGE(Boost 1.54 REQUIRED COMPONENTS coroutine)
	  LIST(APPEND BOOST_COMPONENTS coroutine)
	  SET(Boost_LIBRARIES ${BOOST_LIBRARIES_TEMP} ${Boost_LIBRARIES})
	ENDIF()


add_library( uvm ${SOURCES} ${HEADERS} )

target_link_libraries( uvm 
  PUBLIC fc)
target_include_directories( uvm 
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../jsondiff/jsondiff-cpp/include" "${CMAKE_CURRENT_SOURCE_DIR}/vmgc/include"
)

#aux_source_directory(./simplechain/src simplechain_src1)
#aux_source_directory(./simplechain/src/simplechain simplechain_src2)
#
#include_directories(simplechain/include)
#
#link_directories(../fc /usr/local/lib/boost/lib)
#link_libraries(fc boost_system boost_system boost_chrono boost_date_time boost_coroutine boost_context boost_thread boost_filesystem pthread)
#
#add_executable(simplechain_runner ${simplechain_src1} ${simplechain_src2})
#
#target_include_directories( simplechain_runner 
  #PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/simplechain/include" "${CMAKE_CURRENT_SOURCE_DIR}/../jsondiff/jsondiff-cpp/include"
#)
#
# target_link_libraries(simplechain_runner PUBLIC uvm fc OpenSSL::SSL ${CMAKE_SOURCE_DIR}/../jsondiff/libjsondiff_cpp.a)

# add_executable(uvm_single_exec uvm_single/main.cpp uvm_single/Keccak.cpp uvm_single/uvm_api.demo.cpp)
# target_link_libraries(uvm_single_exec PUBLIC uvm fc OpenSSL::SSL ${CMAKE_SOURCE_DIR}/../jsondiff/libjsondiff_cpp.a)

if (USE_PCH)
  set_target_properties(uvm PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(uvm)
endif(USE_PCH)

install( TARGETS
   uvm

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/uvm" )

