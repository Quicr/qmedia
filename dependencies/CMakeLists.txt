include(FetchContent)
include(ExternalProject)

set(OPENSSL_SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl-src) # default path by CMake
set(OPENSSL_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/openssl)
set(OPENSSL_INCLUDE_DIR ${OPENSSL_INSTALL_DIR}/include)
set(OPENSSL_CONFIGURE_COMMAND ${OPENSSL_SOURCE_DIR}/config)
set(OPENSSL_ROOT_DIR ${OPENSSL_SOURCE_DIR})
ExternalProject_Add(
        OpenSSL
        SOURCE_DIR ${OPENSSL_SOURCE_DIR}
        GIT_REPOSITORY https://github.com/openssl/openssl.git
        GIT_TAG OpenSSL_1_1_1n
        USES_TERMINAL_DOWNLOAD TRUE
        CONFIGURE_COMMAND
        ${OPENSSL_CONFIGURE_COMMAND}
        --prefix=${OPENSSL_INSTALL_DIR}
        --openssldir=${OPENSSL_INSTALL_DIR}
        BUILD_COMMAND make
        TEST_COMMAND ""
        INSTALL_COMMAND make install
        INSTALL_DIR ${OPENSSL_INSTALL_DIR}
)
# We cannot use find_library because ExternalProject_Add() is performed at build time.
# And to please the property INTERFACE_INCLUDE_DIRECTORIES,
# we make the include directory in advance.
file(MAKE_DIRECTORY ${OPENSSL_INCLUDE_DIR})

add_library(OpenSSL::SSL STATIC IMPORTED GLOBAL)
set_property(TARGET OpenSSL::SSL PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libssl.${OPENSSL_LIBRARY_SUFFIX})
set_property(TARGET OpenSSL::SSL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
add_dependencies(OpenSSL::SSL OpenSSL)

add_library(OpenSSL::Crypto STATIC IMPORTED GLOBAL)
set_property(TARGET OpenSSL::Crypto PROPERTY IMPORTED_LOCATION ${OPENSSL_INSTALL_DIR}/lib/libcrypto.${OPENSSL_LIBRARY_SUFFIX})
set_property(TARGET OpenSSL::Crypto PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OPENSSL_INCLUDE_DIR})
add_dependencies(OpenSSL::Crypto OpenSSL)


#Would be nice if "downstream" packages could pick up on the lovely OpenSSL library we intend to compile above
#TODO: This is the end for now... (until fixed..)
find_package(OpenSSL 1.1 REQUIRED)


################################################################################
# Project URL: https://github.com/cisco/sframe
#     Git URL: https://github.com/cisco/sframe.git
#      Branch: master
#     Targets: sframe
FetchContent_Declare(   sframe
    GIT_REPOSITORY      https://github.com/cisco/sframe.git
    GIT_TAG             master)


FetchContent_Declare(quicr
        GIT_REPOSITORY      https://github.com/palerikm/libquicr.git
        GIT_TAG             main)
set(quicr_BUILD_TESTS OFF)
set(quicrq_BUILD_TESTS OFF)


FetchContent_Declare(   samplerate
        GIT_REPOSITORY      https://github.com/libsndfile/libsamplerate.git
        GIT_TAG             master)

FetchContent_Declare(   idn2
        GIT_REPOSITORY      https://gitlab.com/libidn/libidn2.git
        GIT_TAG             master)



FetchContent_Declare(   curl
        GIT_REPOSITORY      https://github.com/curl/curl.git
        GIT_TAG             master)

FetchContent_Declare(   opus
        GIT_REPOSITORY     https://github.com/xiph/opus.git
        GIT_TAG             master)

FetchContent_MakeAvailable(sframe quicr samplerate idn2 curl opus)
