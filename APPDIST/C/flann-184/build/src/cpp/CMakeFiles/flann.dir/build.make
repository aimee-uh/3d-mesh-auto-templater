# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.17

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

# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake3

# The command to remove a file.
RM = /usr/bin/cmake3 -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build

# Include any dependencies generated for this target.
include src/cpp/CMakeFiles/flann.dir/depend.make

# Include the progress variables for this target.
include src/cpp/CMakeFiles/flann.dir/progress.make

# Include the compile flags for this target's objects.
include src/cpp/CMakeFiles/flann.dir/flags.make

src/cpp/CMakeFiles/flann.dir/empty.cpp.o: src/cpp/CMakeFiles/flann.dir/flags.make
src/cpp/CMakeFiles/flann.dir/empty.cpp.o: ../src/cpp/empty.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/cpp/CMakeFiles/flann.dir/empty.cpp.o"
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && /opt/rh/devtoolset-7/root/usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/flann.dir/empty.cpp.o -c /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/src/cpp/empty.cpp

src/cpp/CMakeFiles/flann.dir/empty.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/flann.dir/empty.cpp.i"
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && /opt/rh/devtoolset-7/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/src/cpp/empty.cpp > CMakeFiles/flann.dir/empty.cpp.i

src/cpp/CMakeFiles/flann.dir/empty.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/flann.dir/empty.cpp.s"
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && /opt/rh/devtoolset-7/root/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/src/cpp/empty.cpp -o CMakeFiles/flann.dir/empty.cpp.s

# Object files for target flann
flann_OBJECTS = \
"CMakeFiles/flann.dir/empty.cpp.o"

# External object files for target flann
flann_EXTERNAL_OBJECTS =

lib/libflann.so.1.8.4: src/cpp/CMakeFiles/flann.dir/empty.cpp.o
lib/libflann.so.1.8.4: src/cpp/CMakeFiles/flann.dir/build.make
lib/libflann.so.1.8.4: lib/libflann_s.a
lib/libflann.so.1.8.4: src/cpp/CMakeFiles/flann.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX shared library ../../lib/libflann.so"
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/flann.dir/link.txt --verbose=$(VERBOSE)
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && $(CMAKE_COMMAND) -E cmake_symlink_library ../../lib/libflann.so.1.8.4 ../../lib/libflann.so.1.8 ../../lib/libflann.so

lib/libflann.so.1.8: lib/libflann.so.1.8.4
	@$(CMAKE_COMMAND) -E touch_nocreate lib/libflann.so.1.8

lib/libflann.so: lib/libflann.so.1.8.4
	@$(CMAKE_COMMAND) -E touch_nocreate lib/libflann.so

# Rule to build all files generated by this target.
src/cpp/CMakeFiles/flann.dir/build: lib/libflann.so

.PHONY : src/cpp/CMakeFiles/flann.dir/build

src/cpp/CMakeFiles/flann.dir/clean:
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp && $(CMAKE_COMMAND) -P CMakeFiles/flann.dir/cmake_clean.cmake
.PHONY : src/cpp/CMakeFiles/flann.dir/clean

src/cpp/CMakeFiles/flann.dir/depend:
	cd /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184 /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/src/cpp /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp /homes/grail/iytian/iytian/styku-ss-fit3d-super/C/flann-184/build/src/cpp/CMakeFiles/flann.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/cpp/CMakeFiles/flann.dir/depend
