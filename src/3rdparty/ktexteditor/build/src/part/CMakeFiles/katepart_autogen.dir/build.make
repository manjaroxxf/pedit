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

# Utility rule file for katepart_autogen.

# Include the progress variables for this target.
include src/part/CMakeFiles/katepart_autogen.dir/progress.make

src/part/CMakeFiles/katepart_autogen: src/part/katepart.json
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Automatic MOC for target katepart"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part && /usr/bin/cmake -E cmake_autogen /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part/CMakeFiles/katepart_autogen.dir/AutogenInfo.json Debug

src/part/katepart.json: ../src/part/katepart.desktop
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Generating katepart.json"
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/part && /usr/bin/desktoptojson -i katepart.desktop -o /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part/katepart.json -s /usr/share/kservicetypes5/kpart.desktop

katepart_autogen: src/part/CMakeFiles/katepart_autogen
katepart_autogen: src/part/katepart.json
katepart_autogen: src/part/CMakeFiles/katepart_autogen.dir/build.make

.PHONY : katepart_autogen

# Rule to build all files generated by this target.
src/part/CMakeFiles/katepart_autogen.dir/build: katepart_autogen

.PHONY : src/part/CMakeFiles/katepart_autogen.dir/build

src/part/CMakeFiles/katepart_autogen.dir/clean:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part && $(CMAKE_COMMAND) -P CMakeFiles/katepart_autogen.dir/cmake_clean.cmake
.PHONY : src/part/CMakeFiles/katepart_autogen.dir/clean

src/part/CMakeFiles/katepart_autogen.dir/depend:
	cd /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chungzh/Code/pedit/src/3rdparty/ktexteditor /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/src/part /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part /home/chungzh/Code/pedit/src/3rdparty/ktexteditor/build/src/part/CMakeFiles/katepart_autogen.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : src/part/CMakeFiles/katepart_autogen.dir/depend

