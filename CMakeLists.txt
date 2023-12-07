file(READ version.txt WIZCHIP_VERSION)
message("WIZCHIP_VERSION : ${WIZCHIP_VERSION}")

# ethernet
file(GLOB_RECURSE ETHERNET_SOURCES 
    ioLibrary_Driver/Ethernet/*.*
    wizchip/*.*
)
add_library(wizchip ${ETHERNET_SOURCES})

target_include_directories(wizchip PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/ioLibrary_Driver
    ${CMAKE_CURRENT_SOURCE_DIR}/ioLibrary_Driver/Ethernet
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# defines
target_compile_definitions(wizchip PUBLIC -DWIZCHIP_VERSION="${WIZCHIP_VERSION}")

# depends
target_link_libraries(wizchip etl)
target_link_libraries(wizchip periph)
