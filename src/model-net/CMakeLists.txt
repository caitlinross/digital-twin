find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(codes_lexer ${CMAKE_CURRENT_SOURCE_DIR}/config-lex.l ${CMAKE_CURRENT_SOURCE_DIR}/config-lex.c DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config-lex.h)
bison_target(codes_parser ${CMAKE_CURRENT_SOURCE_DIR}/config-parser.y ${CMAKE_CURRENT_SOURCE_DIR}/config-parser.c DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config-parser.h)
ADD_FLEX_BISON_DEPENDENCY(codes_lexer codes_parser)

cmake_print_variables(FLEX_codes_lexer_OUTPUTS)
cmake_print_variables(BISON_codes_parser_OUTPUTS)

set(srcs
  codes-jobmap.c
  codes-mapping-context.c
  codes-mapping.c
  config-file.c
  config-glue.c
  config-lex.c
  config-parser.c
  config-store.c
  config-store-adapter.c
  configuration.c
  lp-io.c
  lp-msg.c
  lp-type-lookup.c
  model-net-lp.c
  model-net-sched-impl.c
  model-net-sched.c
  model-net.c
  simplenet-upd.c
  simplep2p.c
  txt-config-file.c
)

set(headers
  codes-jobmap-method-impl.h
  codes-jobmap.h
  codes-mapping-context.h
  codes-mapping.h
  codes.h
  config-file.h
  config-glue.h
  config-lex.h
  config-parser.h
  config-store.h
  config-store-adapter.h
  configuration.h
  congestion-controller-core.h
  jenkins-hash.h
  lp-io.h
  lp-msg.h
  lp-type-lookup.h
  model-net-lp.h
  model-net-method.h
  model-net-sched-impl.h
  model-net-sched.h
  model-net.h
  quicklist.h
  simplenet-upd.h
  simplep2p.h
  txt-config-file.h
)

# TODO set up different library modules that will get linked together
add_library(codes ${srcs})
add_library(CODES::CODES ALIAS codes)

target_link_libraries(codes PUBLIC ROSS::ROSS)

target_include_directories(codes
  PUBLIC
  $<BUILD_INTERFACE:${CODES_SOURCE_DIR}/src/model-net>
  $<BUILD_INTERFACE:${CODES_BINARY_DIR}/src/model-net>
  $<INSTALL_INTERFACE:include>
)

CONFIGURE_FILE(codes-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/codes-config.h)

#-------------------------------------------------------------------------------
# installation

include (GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/codes)

install(TARGETS codes
  EXPORT CODES-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

set_target_properties(codes PROPERTIES EXPORT_NAME CODES)

install(EXPORT CODES-targets
  FILE CODES-targets.cmake
  NAMESPACE CODES::
  DESTINATION ${INSTALL_CONFIGDIR})

# generate the config file that includes the exports
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CODES_BINARY_DIR}/CODESConfigVersion.cmake
    VERSION ${CODES_VERSION_SHORT}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(${CODES_SOURCE_DIR}/cmake/CODESConfig.cmake.in
  "${CODES_BINARY_DIR}/CODESConfig.cmake"
  INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
  )
install(FILES
  "${CODES_BINARY_DIR}/CODESConfig.cmake"
  "${CODES_BINARY_DIR}/CODESConfigVersion.cmake"
  DESTINATION ${INSTALL_CONFIGDIR})

export(EXPORT CODES-targets
    FILE ${CMAKE_CURRENT_BINARY_DIR}/CODES-targets.cmake
    NAMESPACE CODES::)

install(FILES ${headers} DESTINATION include)
