find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(codes_lexer ${CMAKE_CURRENT_SOURCE_DIR}/config/config-lex.l ${CMAKE_CURRENT_SOURCE_DIR}/config/config-lex.c DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config/config-lex.h)
bison_target(codes_parser ${CMAKE_CURRENT_SOURCE_DIR}/config/config-parser.y ${CMAKE_CURRENT_SOURCE_DIR}/config/config-parser.c DEFINES_FILE ${CMAKE_CURRENT_SOURCE_DIR}/config/config-parser.h)
ADD_FLEX_BISON_DEPENDENCY(codes_lexer codes_parser)

set(srcs
# config
  config/config-file.c
  config/config-glue.c
  config/config-lex.c
  config/config-parser.c
  config/config-store.c
  config/config-store-adapter.c
  config/configuration.c
  config/txt-config-file.c

# mapping
  mapping/codes-jobmap.c
  mapping/codes-mapping-context.c
  mapping/codes-mapping.c

# model-net
  model-net/lp-io.c
  model-net/lp-msg.c
  model-net/lp-type-lookup.c
  model-net/model-net-lp.c
  model-net/model-net-sched-impl.c
  model-net/model-net-sched.c
  model-net/model-net.c
  model-net/simplenet-upd.c
  model-net/simplep2p.c

# util
)

set(headers
# config
  config/config-file.h
  config/config-glue.h
  config/config-lex.h
  config/config-parser.h
  config/config-store.h
  config/config-store-adapter.h
  config/configuration.h
  config/txt-config-file.h

# mapping
  mapping/codes-jobmap-method-impl.h
  mapping/codes-jobmap.h
  mapping/codes-mapping-context.h
  mapping/codes-mapping.h

# model-net
  model-net/codes.h
  model-net/congestion-controller-core.h
  model-net/lp-io.h
  model-net/lp-msg.h
  model-net/lp-type-lookup.h
  model-net/model-net-lp.h
  model-net/model-net-method.h
  model-net/model-net-sched-impl.h
  model-net/model-net-sched.h
  model-net/model-net.h
  model-net/simplenet-upd.h
  model-net/simplep2p.h

# util
  util/jenkins-hash.h
  util/quicklist.h
)

# TODO set up different library modules that will get linked together
add_library(codes ${srcs})
add_library(CODES::CODES ALIAS codes)

target_link_libraries(codes PUBLIC ROSS::ROSS)

cmake_print_variables(CODES_SOURCE_DIR)
target_include_directories(codes
  PUBLIC
  $<BUILD_INTERFACE:${CODES_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CODES_BINARY_DIR}>
  $<INSTALL_INTERFACE:include>
)

cmake_print_variables(CMAKE_CURRENT_BINARY_DIR)
CONFIGURE_FILE(config/codes-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config/codes-config.h)

#-------------------------------------------------------------------------------
# installation

include (GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/codes)

install(TARGETS codes
  EXPORT CODES-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

#set_target_properties(codes PROPERTIES EXPORT_NAME CODES)

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

install(FILES ${headers} DESTINATION include/codes)
