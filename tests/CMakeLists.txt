add_executable(lp-io-test lp-io-test.c)
target_link_libraries(lp-io-test PRIVATE CODES::CODES)
add_test(NAME lp-io-test
    COMMAND lp-io-test --sync=1)

add_test(NAME simplep2p-test
      COMMAND codes-digital-twin --sync=1
      --config-file=${CMAKE_CURRENT_SOURCE_DIR}/conf/simplep2p-test.yaml
)

# TODO add comparison for output
add_executable(mapping-test mapper-test.cxx)
target_link_libraries(mapping-test PUBLIC CODES::CODES)
add_test(NAME mapping-test
      COMMAND mapping-test --sync=1
      --codes-config=${CMAKE_CURRENT_SOURCE_DIR}/conf/mapping-test.yaml
)

#add_executable(job-map-test jobmap-test.c)
#target_link_libraries(job-map-test PUBLIC CODES::CODES)
#add_test(NAME job-map-test
#      COMMAND job-map-test
#        ${CMAKE_CURRENT_SOURCE_DIR}/conf/jobmap-test-list.conf
#)
#
#add_executable(map-ctx-test map-ctx-test.c)
#target_link_libraries(map-ctx-test PUBLIC CODES::CODES)
#add_test(NAME map-ctx-test
#      COMMAND map-ctx-test
#        ${CMAKE_CURRENT_SOURCE_DIR}/conf/map-ctx-test.conf
#)

