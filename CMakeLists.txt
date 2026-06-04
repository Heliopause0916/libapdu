cmake_minimum_required(VERSION 3.10)
project(libapdu VERSION 1.0.0 LANGUAGES C)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Library sources
set(APDU_SOURCES
    src/apdu.c
)

set(APDU_PUBLIC_HEADERS
    include/libapdu/apdu.h
)

# Build shared or static library
option(BUILD_SHARED_LIBS "Build shared library" OFF)

# Create the library
add_library(${PROJECT_NAME} ${APDU_SOURCES})

# Copy public headers to build directory at configure time
file(COPY ${APDU_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_BINARY_DIR}/include/libapdu)

# Define export macro for shared library builds
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE LIBAPDU_BUILD_DLL)
endif()

# Specify include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Specify C99 for the target
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED ON
    POSITION_INDEPENDENT_CODE ON
)

# On Windows with shared library, ensure import library goes to lib directory
# and DLL goes to bin directory
if(BUILD_SHARED_LIBS AND WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${APDU_PUBLIC_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libapdu
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Testing
option(BUILD_TESTING "Build tests" ON)

if(BUILD_TESTING)
    enable_testing()

    add_executable(test_apdu
        tests/test_apdu.c
    )
    target_link_libraries(test_apdu PRIVATE ${PROJECT_NAME})
    target_include_directories(test_apdu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    # When linking against shared library, need to define LIBAPDU_USE_DLL
    if(BUILD_SHARED_LIBS)
        target_compile_definitions(test_apdu PRIVATE LIBAPDU_USE_DLL)
    endif()

    add_test(NAME test_apdu COMMAND test_apdu)
endif()
