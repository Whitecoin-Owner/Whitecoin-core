add_executable( cli_wallet main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
  message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
  IF(WIN32)
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc)
  ELSE()
    list( APPEND PLATFORM_SPECIFIC_LIBS libtcmalloc.a)
  ENDIF()
endif()

IF(WIN32)
  target_link_libraries( cli_wallet PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet fc leveldb
    ${PLATFORM_SPECIFIC_LIBS} 
    ${CMAKE_DL_LIBS}        
  )
ELSE()
  target_link_libraries( cli_wallet PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet fc crosschain_privatekey_management leveldb 
    $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a 
    $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a
    ${PLATFORM_SPECIFIC_LIBS}
    ${CMAKE_DL_LIBS}    
  )
ENDIF()

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
  cli_wallet

  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
