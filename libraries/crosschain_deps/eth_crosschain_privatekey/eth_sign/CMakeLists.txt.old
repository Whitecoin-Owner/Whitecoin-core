AUX_SOURCE_DIRECTORY(cryptopp CRYPTOPP_SRC)
AUX_SOURCE_DIRECTORY(libdevcore DEVCORE_SRC)
AUX_SOURCE_DIRECTORY(libethcore ETHCORE_SRC)
AUX_SOURCE_DIRECTORY(libdevcrypto DEVCRYPTO_SRC)

LIST(REMOVE_ITEM ETHCORE_SRC "libethcore/KeyManager.cpp" "libethcore/BasicAuthority.cpp")
#LIST(REMOVE_ITEM GLUA_SRC "libraries/glua/main.cpp" "libraries/glua/cdemo1.cpp" "libraries/glua/hsrcore_lua_api.demo.cpp")
ADD_DEFINITIONS("-DUSE_NUM_NONE -DUSE_SCALAR_8X32 -DUSE_FIELD_10X26 -DUSE_FIELD_INV_BUILTIN -DUSE_SCALAR_INV_BUILTIN -DHAVE_ROUND -D_CRT_SECURE_NO_WARNINGS")
cmake_minimum_required(VERSION 2.8)
SET (CMAKE_CXX_COMPILER     "/usr/bin/g++")

# Boost settings
#set(Boost_USE_STATIC_LIBS     ON)
#set(Boost_USE_MULTITHREADED   ON)
#set(Boost_USE_STATIC_RUNTIME  ON)

# Specify Boost libraries that will use.
#FIND_PACKAGE( Boost 1.54.0 REQUIRED COMPONENTS system program_options chrono context coroutine date_time exception filesystem iostreams locale thread )

#set(CMAKE_CXX_FLAGS "-std=c++11 -DHSR_GLOBAL_API_LOG=1 -DHSR_TEST_NETWORK -D_DARWIN_C_SOURCE" )
set(CMAKE_CXX_FLAGS "-std=c++11")
set(Boost_INCLUDE_DIR　"/home/ted/boost/boost_1_64_0")
#set(Boost_LIBRARY_DIR  "/usr/lib/x86_64-linux-gnu")

INCLUDE_DIRECTORIES(
#	"/home/ted/ethlib/eth_sign" 
	"secp256k1"
	"secp256k1/include"
	"secp256k1/src"
	"./"
#        "/home/ted/ethlib/cryptopp"
 #       "/home/ted/ethlib/libdevcore"
  #      "/home/ted/ethlib/libethcore"
   #     "/home/ted/ethlib/libdevcrypto"
)

LINK_DIRECTORIES( 
        "libdevcore"
        "libethcore"
        "libdevcrypto"
	"cryptopp"
)

set(SOURCE_FILES
	main.cpp
)

add_library(eth_sign ${DEVCORE_SRC}  ${ETHCORE_SRC} ${DEVCRYPTO_SRC} secp256k1/src/eth_secp256k1.c)
#add_executable(eth_sign ${DEVCORE_SRC}  ${ETHCORE_SRC} ${DEVCRYPTO_SRC})

target_link_libraries( eth_sign libcryptopp.a)
INSTALL(TARGETS
	eth_sign

	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
