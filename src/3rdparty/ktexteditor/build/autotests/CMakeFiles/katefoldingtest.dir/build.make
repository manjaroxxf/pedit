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
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chungzh/Code/pedit/src/3rdparty/ktexteditor

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build

# Include any dependencies generated for this target.
include autotests/CMakeFiles/katefoldingtest.dir/depend.make

# Include the progress variables for this target.
include autotests/CMakeFiles/katefoldingtest.dir/progress.make

# Include the compile flags for this target's objects.
include autotests/CMakeFiles/katefoldingtest.dir/flags.make

autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o: autotests/CMakeFiles/katefoldingtest.dir/flags.make
autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o: autotests/katefoldingtest_autogen/mocs_compilation.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o -c /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/katefoldingtest_autogen/mocs_compilation.cpp

autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.i"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/katefoldingtest_autogen/mocs_compilation.cpp > CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.i

autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.s"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/katefoldingtest_autogen/mocs_compilation.cpp -o CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.s

autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o: autotests/CMakeFiles/katefoldingtest.dir/flags.make
autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o: ../autotests/src/katefoldingtest.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o -c /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/katefoldingtest.cpp

autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.i"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/katefoldingtest.cpp > CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.i

autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.s"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/katefoldingtest.cpp -o CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.s

# Object files for target katefoldingtest
katefoldingtest_OBJECTS = \
"CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o" \
"CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o"

# External object files for target katefoldingtest
katefoldingtest_EXTERNAL_OBJECTS =

bin/katefoldingtest: autotests/CMakeFiles/katefoldingtest.dir/katefoldingtest_autogen/mocs_compilation.cpp.o
bin/katefoldingtest: autotests/CMakeFiles/katefoldingtest.dir/src/katefoldingtest.cpp.o
bin/katefoldingtest: autotests/CMakeFiles/katefoldingtest.dir/build.make
bin/katefoldingtest: autotests/CMakeFiles/katefoldingtest.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable ../bin/katefoldingtest"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/katefoldingtest.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
autotests/CMakeFiles/katefoldingtest.dir/build: bin/katefoldingtest

.PHONY : autotests/CMakeFiles/katefoldingtest.dir/build

autotests/CMakeFiles/katefoldingtest.dir/clean:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && $(CMAKE_COMMAND) -P CMakeFiles/katefoldingtest.dir/cmake_clean.cmake
.PHONY : autotests/CMakeFiles/katefoldingtest.dir/clean

autotests/CMakeFiles/katefoldingtest.dir/depend:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chungzh/Code/pedit/src/3rdparty/ktexteditor /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/CMakeFiles/katefoldingtest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : autotests/CMakeFiles/katefoldingtest.dir/depend
