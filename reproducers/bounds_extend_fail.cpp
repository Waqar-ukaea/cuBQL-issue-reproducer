#include <cstdint>
#include <iostream>

#include <omp.h>

// Clang defines __CUDA_ARCH__ when compiling an OpenMP NVPTX device image,
// even though CUDA language keywords and runtime constants are unavailable.
// This doesn't seem to affect nvc++ builds.
#if defined(__CUDA_ARCH__) && !defined(__CUDACC__)
#undef __CUDA_ARCH__
#endif

#include "cuBQL/math/box.h"

int main()
{
  const int gpu_id = omp_get_default_device();

  if (omp_get_num_devices() == 0) {
    std::cerr << "No OpenMP target devices found\n";
    return 1;
  }

  cuBQL::vec3d vertices[] = {
    {-1.0, -1.0, 0.0},
    { 1.0, -1.0, 0.0},
    { 0.0,  1.0, 0.0}
  };

  cuBQL::box3d bounds[1];

  std::cout << "Running box3d::extend() on OpenMP target device " << gpu_id << std::endl;

  #pragma omp target teams distribute parallel for device(gpu_id) \
    map(to: vertices[0:3]) \
    map(from: bounds[0:1])
  for (std::uint32_t primitive_id = 0; primitive_id < 1; ++primitive_id) {
    cuBQL::box3d aabb;
    aabb.extend(vertices[0]);
    aabb.extend(vertices[1]);
    aabb.extend(vertices[2]);
    bounds[primitive_id] = aabb;
  }

  const cuBQL::vec3d expected_lower(-1.0, -1.0, 0.0);
  const cuBQL::vec3d expected_upper( 1.0,  1.0, 0.0);

  const bool correct_bounds = bounds[0].lower == expected_lower
                           && bounds[0].upper == expected_upper;

  std::cout << "\n--------------------------------------\n";
  std::cout << "Computed lower   : " << bounds[0].lower << '\n';
  std::cout << "Computed upper   : " << bounds[0].upper << '\n';
  std::cout << "Expected lower   : " << expected_lower << '\n';
  std::cout << "Expected upper   : " << expected_upper << '\n';
  std::cout << "Bounds extension : "
            << (correct_bounds ? "PASS" : "FAIL") << '\n';
  std::cout << "--------------------------------------\n";

  return correct_bounds ? 0 : 1;
}
