# CMAKE generated file: DO NOT EDIT!
# Generated by "MinGW Makefiles" Generator, CMake Version 3.23

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

SHELL = cmd.exe

# The CMake executable.
CMAKE_COMMAND = "C:\Program Files\JetBrains\CLion 2022.2.4\bin\cmake\win\bin\cmake.exe"

# The command to remove a file.
RM = "C:\Program Files\JetBrains\CLion 2022.2.4\bin\cmake\win\bin\cmake.exe" -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug

# Include any dependencies generated for this target.
include esp-idf/cmock/CMakeFiles/__idf_cmock.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include esp-idf/cmock/CMakeFiles/__idf_cmock.dir/compiler_depend.make

# Include the progress variables for this target.
include esp-idf/cmock/CMakeFiles/__idf_cmock.dir/progress.make

# Include the compile flags for this target's objects.
include esp-idf/cmock/CMakeFiles/__idf_cmock.dir/flags.make

esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj: esp-idf/cmock/CMakeFiles/__idf_cmock.dir/flags.make
esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj: C:/Espressif/frameworks/esp-idf-v4.3.4/components/cmock/CMock/src/cmock.c
esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj: esp-idf/cmock/CMakeFiles/__idf_cmock.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj"
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && C:\Espressif\tools\xtensa-esp32-elf\esp-2021r2-patch3-8.4.0\xtensa-esp32-elf\bin\xtensa-esp32-elf-gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -MD -MT esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj -MF CMakeFiles\__idf_cmock.dir\CMock\src\cmock.c.obj.d -o CMakeFiles\__idf_cmock.dir\CMock\src\cmock.c.obj -c C:\Espressif\frameworks\esp-idf-v4.3.4\components\cmock\CMock\src\cmock.c

esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.i"
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && C:\Espressif\tools\xtensa-esp32-elf\esp-2021r2-patch3-8.4.0\xtensa-esp32-elf\bin\xtensa-esp32-elf-gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E C:\Espressif\frameworks\esp-idf-v4.3.4\components\cmock\CMock\src\cmock.c > CMakeFiles\__idf_cmock.dir\CMock\src\cmock.c.i

esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.s"
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && C:\Espressif\tools\xtensa-esp32-elf\esp-2021r2-patch3-8.4.0\xtensa-esp32-elf\bin\xtensa-esp32-elf-gcc.exe $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S C:\Espressif\frameworks\esp-idf-v4.3.4\components\cmock\CMock\src\cmock.c -o CMakeFiles\__idf_cmock.dir\CMock\src\cmock.c.s

# Object files for target __idf_cmock
__idf_cmock_OBJECTS = \
"CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj"

# External object files for target __idf_cmock
__idf_cmock_EXTERNAL_OBJECTS =

esp-idf/cmock/libcmock.a: esp-idf/cmock/CMakeFiles/__idf_cmock.dir/CMock/src/cmock.c.obj
esp-idf/cmock/libcmock.a: esp-idf/cmock/CMakeFiles/__idf_cmock.dir/build.make
esp-idf/cmock/libcmock.a: esp-idf/cmock/CMakeFiles/__idf_cmock.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libcmock.a"
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && $(CMAKE_COMMAND) -P CMakeFiles\__idf_cmock.dir\cmake_clean_target.cmake
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles\__idf_cmock.dir\link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
esp-idf/cmock/CMakeFiles/__idf_cmock.dir/build: esp-idf/cmock/libcmock.a
.PHONY : esp-idf/cmock/CMakeFiles/__idf_cmock.dir/build

esp-idf/cmock/CMakeFiles/__idf_cmock.dir/clean:
	cd /d C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock && $(CMAKE_COMMAND) -P CMakeFiles\__idf_cmock.dir\cmake_clean.cmake
.PHONY : esp-idf/cmock/CMakeFiles/__idf_cmock.dir/clean

esp-idf/cmock/CMakeFiles/__idf_cmock.dir/depend:
	$(CMAKE_COMMAND) -E cmake_depends "MinGW Makefiles" C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis C:\Espressif\frameworks\esp-idf-v4.3.4\components\cmock C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock C:\Users\Ramiro\Desktop\TpProfPrecursorSismico\Microcontrolador\Tesis-precursores-sismicos\Tesis\cmake-build-debug\esp-idf\cmock\CMakeFiles\__idf_cmock.dir\DependInfo.cmake --color=$(COLOR)
.PHONY : esp-idf/cmock/CMakeFiles/__idf_cmock.dir/depend

