project(eth_crosschain_privatekey)
cmake_minimum_required(VERSION 2.8)
message(STATUS "Configuring project eth_crosschain_privatekey located in: ${CMAKE_CURRENT_SOURCE_DIR}")

AUX_SOURCE_DIRECTORY(cryptopp CRYPTOPP_SRC)
AUX_SOURCE_DIRECTORY(libdevcore DEVCORE_SRC)
AUX_SOURCE_DIRECTORY(libdevcrypto DEVCRYPTO_SRC)

AUX_SOURCE_DIRECTORY(libethcore ETHCORE_SRC)
LIST(REMOVE_ITEM ETHCORE_SRC "libethcore/KeyManager.cpp" "libethcore/BasicAuthority.cpp")

#SET(CMAKE_C_COMPILER   "/usr/bin/gcc")
#SET(CMAKE_CXX_COMPILER "/usr/bin/g++")
set(CMAKE_CXX_FLAGS "-std=c++11")

ADD_DEFINITIONS("-DUSE_NUM_NONE -DUSE_SCALAR_8X32 -DUSE_FIELD_10X26 -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DHAVE_ROUND -D_CRT_SECURE_NO_WARNINGS")

# Specify Boost libraries that will use.
FIND_PACKAGE(Boost REQUIRED COMPONENTS system program_options chrono context coroutine date_time exception filesystem iostreams locale thread)
if (Boost_FOUND)
  message(STATUS "found boost library!")
else (Boost_FOUND)
  message(FATAL_ERROR "cannot found Boost library!")
endif()

INCLUDE_DIRECTORIES(
  "secp256k1"
  "secp256k1/include"
  "secp256k1/src"
  "."
  "${Boost_INCLUDE_DIR}"
)

LINK_DIRECTORIES(
  "${CMAKE_CURRENT_SOURCE_DIR}/libdevcore"
  "${CMAKE_CURRENT_SOURCE_DIR}/libethcore"
  "${CMAKE_CURRENT_SOURCE_DIR}/libdevcrypto"
  "${CMAKE_CURRENT_SOURCE_DIR}/cryptopp"
)

add_library(eth_sign 
  ${DEVCORE_SRC} 
  ${ETHCORE_SRC} 
  ${DEVCRYPTO_SRC} 
  secp256k1/src/eth_secp256k1.c
)

target_link_libraries(eth_sign libcryptopp.a)

INSTALL(TARGETS
    eth_sign

    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
