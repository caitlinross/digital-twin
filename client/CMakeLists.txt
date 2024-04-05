set(srcs
    DigitalTwinClient.cxx
)

set(headers)

add_executable(codes-digital-twin ${srcs})
target_link_libraries(codes-digital-twin PRIVATE CODES::CODES)

#TODO add install rules
