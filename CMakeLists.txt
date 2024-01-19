cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)

add_compile_options(-Wall -Wextra -O3)

project(cygnoana VERSION 0.0.1)


link_directories("$ENV{ROOTANASYS}/lib"
                 "$ENV{OPENCVSYS}/lib64" #opencv
                )

add_executable(cygnoana "${PROJECT_SOURCE_DIR}/main.cxx")
target_link_libraries(cygnoana PUBLIC cygnolib s3 rootana z opencv_imgcodecs opencv_core)


# -------- cygnolib --------
add_library(cygnolib
           "${PROJECT_SOURCE_DIR}/src/cygnolib.cxx"
           )
target_include_directories(cygnolib PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/include"
                          "$ENV{ROOTANASYS}/include"
                          "$ENV{OPENCVSYS}/include"
                          )
                          
# --------    s3  --------
add_library(s3
           "${PROJECT_SOURCE_DIR}/src/s3.cxx"
           )
target_include_directories(s3 PUBLIC
                          "${PROJECT_BINARY_DIR}"
                          "${PROJECT_SOURCE_DIR}/include"
                          )