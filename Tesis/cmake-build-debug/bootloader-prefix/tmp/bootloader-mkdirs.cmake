# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/frameworks/esp-idf-v4.3.4/components/bootloader/subproject"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix/tmp"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix/src"
  "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Ramiro/Desktop/TpProfPrecursorSismico/Microcontrolador/Tesis-precursores-sismicos/Tesis/cmake-build-debug/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
