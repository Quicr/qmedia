# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake

# The command to remove a file.
RM = /Applications/CLion.app/Contents/bin/cmake/mac/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /Users/snandaku/repos/work/new-media/qmedia

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /Users/snandaku/repos/work/new-media/qmedia

# Include any dependencies generated for this target.
include src/extern/CMakeFiles/neo_media_client.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/extern/CMakeFiles/neo_media_client.dir/compiler_depend.make

# Include the progress variables for this target.
include src/extern/CMakeFiles/neo_media_client.dir/progress.make

# Include the compile flags for this target's objects.
include src/extern/CMakeFiles/neo_media_client.dir/flags.make

src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o: src/extern/CMakeFiles/neo_media_client.dir/flags.make
src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o: src/extern/neo_media_client.cc
src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o: src/extern/CMakeFiles/neo_media_client.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/Users/snandaku/repos/work/new-media/qmedia/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o"
	cd /Users/snandaku/repos/work/new-media/qmedia/src/extern && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o -MF CMakeFiles/neo_media_client.dir/neo_media_client.cc.o.d -o CMakeFiles/neo_media_client.dir/neo_media_client.cc.o -c /Users/snandaku/repos/work/new-media/qmedia/src/extern/neo_media_client.cc

src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/neo_media_client.dir/neo_media_client.cc.i"
	cd /Users/snandaku/repos/work/new-media/qmedia/src/extern && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /Users/snandaku/repos/work/new-media/qmedia/src/extern/neo_media_client.cc > CMakeFiles/neo_media_client.dir/neo_media_client.cc.i

src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/neo_media_client.dir/neo_media_client.cc.s"
	cd /Users/snandaku/repos/work/new-media/qmedia/src/extern && /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /Users/snandaku/repos/work/new-media/qmedia/src/extern/neo_media_client.cc -o CMakeFiles/neo_media_client.dir/neo_media_client.cc.s

# Object files for target neo_media_client
neo_media_client_OBJECTS = \
"CMakeFiles/neo_media_client.dir/neo_media_client.cc.o"

# External object files for target neo_media_client
neo_media_client_EXTERNAL_OBJECTS =

src/extern/libneo_media_client.dylib: src/extern/CMakeFiles/neo_media_client.dir/neo_media_client.cc.o
src/extern/libneo_media_client.dylib: src/extern/CMakeFiles/neo_media_client.dir/build.make
src/extern/libneo_media_client.dylib: libneoMedia.a
src/extern/libneo_media_client.dylib: proto/libmedia_api.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/lib/libprotobuf.a
src/extern/libneo_media_client.dylib: _deps/sframe-build/libsframe.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/debug/lib/libcrypto.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/debug/lib/libsamplerate.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/lib/libcurl.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/lib/libz.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/debug/lib/libopus.a
src/extern/libneo_media_client.dylib: /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX12.3.sdk/usr/lib/libm.tbd
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/debug/lib/libopenh264.a
src/extern/libneo_media_client.dylib: vcpkg_installed/x64-osx/debug/lib/libcrypto.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/quicrq/libquicrq-core.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picoquic/libpicoquic-core.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picoquic/libpicoquic-log.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picotls/libpicotls-core.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picotls/libpicotls-minicrypto.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picotls/libpicotls-openssl.a
src/extern/libneo_media_client.dylib: /Users/snandaku/repos/work/new-media/picotls/libpicotls-fusion.a
src/extern/libneo_media_client.dylib: src/extern/CMakeFiles/neo_media_client.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/Users/snandaku/repos/work/new-media/qmedia/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library libneo_media_client.dylib"
	cd /Users/snandaku/repos/work/new-media/qmedia/src/extern && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/neo_media_client.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/extern/CMakeFiles/neo_media_client.dir/build: src/extern/libneo_media_client.dylib
.PHONY : src/extern/CMakeFiles/neo_media_client.dir/build

src/extern/CMakeFiles/neo_media_client.dir/clean:
	cd /Users/snandaku/repos/work/new-media/qmedia/src/extern && $(CMAKE_COMMAND) -P CMakeFiles/neo_media_client.dir/cmake_clean.cmake
.PHONY : src/extern/CMakeFiles/neo_media_client.dir/clean

src/extern/CMakeFiles/neo_media_client.dir/depend:
	cd /Users/snandaku/repos/work/new-media/qmedia && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /Users/snandaku/repos/work/new-media/qmedia /Users/snandaku/repos/work/new-media/qmedia/src/extern /Users/snandaku/repos/work/new-media/qmedia /Users/snandaku/repos/work/new-media/qmedia/src/extern /Users/snandaku/repos/work/new-media/qmedia/src/extern/CMakeFiles/neo_media_client.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/extern/CMakeFiles/neo_media_client.dir/depend

