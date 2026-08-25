cmake_minimum_required(VERSION 3.18)

project(cubql-dist-stack-reproducer
  VERSION 0.1.0
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

find_package(OpenMP REQUIRED COMPONENTS CXX)

# This reproducer only exercises cuBQL's OpenMP pathway
set(CUBQL_OMP ON CACHE BOOL "Build cuBQL for OpenMP" FORCE)
add_subdirectory(cuBQL EXCLUDE_FROM_ALL)

add_executable(cubql-dist-stack-reproducer
  main.cpp
)

target_compile_features(cubql-dist-stack-reproducer
  PRIVATE cxx_std_17
)

target_link_libraries(cubql-dist-stack-reproducer
  PRIVATE
    cuBQL
    OpenMP::OpenMP_CXX
)

set(CUBQL_OMP_GPU_ARCH "89" CACHE STRING
  "NVIDIA GPU compute capability without a separator (for example 80, 89, or 90)"
)

if(NOT CUBQL_OMP_GPU_ARCH MATCHES "^[0-9]+$")
  message(FATAL_ERROR
    "CUBQL_OMP_GPU_ARCH must be a numeric compute capability, such as 80, 89, or 90"
  )
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(cubql-dist-stack-reproducer PRIVATE
    -fopenmp-cuda-mode
    -fopenmp-targets=nvptx64
    -Xopenmp-target
    -march=sm_${CUBQL_OMP_GPU_ARCH}
  )

  target_link_options(cubql-dist-stack-reproducer PRIVATE
    -fopenmp=libomp
    -fopenmp-targets=nvptx64
    -Xopenmp-target
    -march=sm_${CUBQL_OMP_GPU_ARCH}
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
  target_compile_options(cubql-dist-stack-reproducer PRIVATE
    -mp=gpu
    -Minfo=mp
    -gpu=cc${CUBQL_OMP_GPU_ARCH}
  )

  target_link_options(cubql-dist-stack-reproducer PRIVATE
    -mp=gpu
    -gpu=cc${CUBQL_OMP_GPU_ARCH}
  )
else()
  message(FATAL_ERROR
    "Unsupported OpenMP offload compiler: ${CMAKE_CXX_COMPILER_ID}"
  )
endif()
