cmake_minimum_required(VERSION 3.18)

project(cubql-issue-reproducers
  VERSION 0.1.0
  LANGUAGES CXX
)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
endif()

find_package(OpenMP REQUIRED COMPONENTS CXX)

# These reproducers only exercises cuBQL's OpenMP pathway
set(CUBQL_OMP ON CACHE BOOL "Build cuBQL for OpenMP" FORCE)
add_subdirectory(cuBQL EXCLUDE_FROM_ALL)

# Get compute architecture
set(CUBQL_OMP_GPU_ARCH "89" CACHE STRING
  "NVIDIA GPU compute capability without a separator (for example 80, 89, or 90)"
)

if(NOT CUBQL_OMP_GPU_ARCH MATCHES "^[0-9]+$")
  message(FATAL_ERROR
    "CUBQL_OMP_GPU_ARCH must be a numeric compute capability, such as 80, 89, or 90"
  )
endif()

# List of all reproducers to be compiled
set(REPRODUCER_NAMES
  dist_stack_fail
)

# Create an interface library to store shared config between executables
add_library(reproducer_config INTERFACE)

target_link_libraries(reproducer_config INTERFACE
  cuBQL
  OpenMP::OpenMP_CXX
)

target_compile_features(reproducer_config INTERFACE cxx_std_17)

target_include_directories(reproducer_config INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(reproducer_config INTERFACE
    -fopenmp-cuda-mode
    -fopenmp-targets=nvptx64
    -Xopenmp-target
    -march=sm_${CUBQL_OMP_GPU_ARCH}
  )

  target_link_options(reproducer_config INTERFACE
    -fopenmp=libomp
    -fopenmp-targets=nvptx64
    -Xopenmp-target
    -march=sm_${CUBQL_OMP_GPU_ARCH}
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
  target_compile_options(reproducer_config INTERFACE
    -mp=gpu
    -Minfo=mp
    -gpu=cc${CUBQL_OMP_GPU_ARCH}
  )

  target_link_options(reproducer_config INTERFACE
    -mp=gpu
    -gpu=cc${CUBQL_OMP_GPU_ARCH}
  )
else()
  message(FATAL_ERROR "Unsupported OpenMP offload compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# For each reproducer add an executable and peform any specific things required
foreach(reproducer IN LISTS REPRODUCER_NAMES)
  add_executable(${reproducer} reproducers/${reproducer}.cpp)

  target_link_libraries(${reproducer} PRIVATE reproducer_config)
endforeach()
