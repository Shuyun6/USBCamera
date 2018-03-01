cmake_minimum_required(VERSION 3.4.1)

add_library( # Sets the name of the library.
             load-lib

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             src/main/cpp/load.cpp )


find_library(
              log-lib

              log )

target_link_libraries(
                       load-lib

                       ${log-lib} )