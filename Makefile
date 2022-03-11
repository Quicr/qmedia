# This is just a convenience Makefile to avoid having to remember
# all the CMake commands and their arguments.

# Set CMAKE_GENERATOR in the environment to select how you build, e.g.:
#   CMAKE_GENERATOR=Ninja

BUILD_DIR=build
CLANG_FORMAT=clang-format -i

.PHONY: all tidy test libs test-libs test-all gen example everything clean cclean format

all: ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

${BUILD_DIR}: CMakeLists.txt 
	cmake -B${BUILD_DIR} -DBUILD_EXTERN=ON -DCMAKE_BUILD_TYPE=Debug .

enable-quic:
        cmake -B${BUILD_DIR} -DBUILD_WITH_QUIC=ON -DCMAKE_BUILD_TYPE=Debug .

test: ${BUILD_DIR} test/*

	cmake --build ${BUILD_DIR} --target neoMedia_test

vcpkg-status:
	less build/vcpkg_installed/vcpkg/status

everything: ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

clean:
	cmake --build ${BUILD_DIR} --target clean

cclean:
	rm -rf ${BUILD_DIR}

format:
	find include -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find src -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find cmd -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}
	find test -iname "*.hh" -or -iname "*.cc" | xargs ${CLANG_FORMAT}