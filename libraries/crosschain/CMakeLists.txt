# list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
# include( GetGitRevisionDescription )
# get_git_head_revision(GIT_REFSPEC GRAPHENE_GIT_REVISION_SHA)
get_git_unix_timestamp(GRAPHENE_GIT_REVISION_UNIX_TIMESTAMP)
# git_describe(GRAPHENE_GIT_REVISION_DESCRIPTION --tags)
if(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)
    set(GRAPHENE_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)

file(GLOB HEADERS "include/graphene/crosschain/*.hpp")

set(sources
   crosschain.cpp
   crosschain_interface_emu.cpp
   crosschain_interface_btc.cpp
   crosschain_interface_ltc.cpp
   crosschain_interface_eth.cpp
   crosschain_interface_erc.cpp
   crosschain_interface_usdt.cpp
   crosschain_interface_bch.cpp
   crosschain_interface_doge.cpp
   crosschain_transaction_record_plugin.cpp
   ${HEADERS})

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")

add_library( crosschain
             ${sources}
             ${HEADERS} )
target_link_libraries( crosschain fc graphene_chain graphene_wallet graphene_app crosschain_privatekey_management graphene_utilities )
target_include_directories( crosschain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
if (USE_PCH)
  set_target_properties(crosschain PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(crosschain)
endif(USE_PCH)

install( TARGETS
   crosschain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/crosschain" )
