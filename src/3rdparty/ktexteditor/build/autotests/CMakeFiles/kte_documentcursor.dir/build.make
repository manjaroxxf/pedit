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
include autotests/CMakeFiles/kte_documentcursor.dir/depend.make

# Include the progress variables for this target.
include autotests/CMakeFiles/kte_documentcursor.dir/progress.make

# Include the compile flags for this target's objects.
include autotests/CMakeFiles/kte_documentcursor.dir/flags.make

autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o: autotests/CMakeFiles/kte_documentcursor.dir/flags.make
autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o: autotests/kte_documentcursor_autogen/mocs_compilation.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o -c /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/kte_documentcursor_autogen/mocs_compilation.cpp

autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.i"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/kte_documentcursor_autogen/mocs_compilation.cpp > CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.i

autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.s"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/kte_documentcursor_autogen/mocs_compilation.cpp -o CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.s

autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o: autotests/CMakeFiles/kte_documentcursor.dir/flags.make
autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o: ../autotests/src/kte_documentcursor.cpp
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++  $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -o CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o -c /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/kte_documentcursor.cpp

autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.i"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/kte_documentcursor.cpp > CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.i

autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.s"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests/src/kte_documentcursor.cpp -o CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.s

# Object files for target kte_documentcursor
kte_documentcursor_OBJECTS = \
"CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o" \
"CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o"

# External object files for target kte_documentcursor
kte_documentcursor_EXTERNAL_OBJECTS =

bin/kte_documentcursor: autotests/CMakeFiles/kte_documentcursor.dir/kte_documentcursor_autogen/mocs_compilation.cpp.o
bin/kte_documentcursor: autotests/CMakeFiles/kte_documentcursor.dir/src/kte_documentcursor.cpp.o
bin/kte_documentcursor: autotests/CMakeFiles/kte_documentcursor.dir/build.make
bin/kte_documentcursor: autotests/CMakeFiles/kte_documentcursor.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX executable ../bin/kte_documentcursor"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/kte_documentcursor.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
autotests/CMakeFiles/kte_documentcursor.dir/build: bin/kte_documentcursor

.PHONY : autotests/CMakeFiles/kte_documentcursor.dir/build

autotests/CMakeFiles/kte_documentcursor.dir/clean:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests && $(CMAKE_COMMAND) -P CMakeFiles/kte_documentcursor.dir/cmake_clean.cmake
.PHONY : autotests/CMakeFiles/kte_documentcursor.dir/clean

autotests/CMakeFiles/kte_documentcursor.dir/depend:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chungzh/Code/pedit/src/3rdparty/ktexteditor /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/autotests /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/autotests/CMakeFiles/kte_documentcursor.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : autotests/CMakeFiles/kte_documentcursor.dir/depend

