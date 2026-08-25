#include <cstdint>
#include <iostream>
#include <vector>

#include <omp.h>

// Clang defines __CUDA_ARCH__ when compiling an OpenMP NVPTX device image,
// even though CUDA language keywords and runtime constants are unavailable.
#if defined(__CUDA_ARCH__) && !defined(__CUDACC__)
#undef __CUDA_ARCH__
#endif

#include "mesh.h"

#include "cuBQL/bvh.h"
#include "cuBQL/builder/omp.h"

int main() {

  const int gpu_id = omp_get_default_device();

  if (omp_get_num_devices() == 0) {
    std::cerr << "No OpenMP target devices found\n";
    return 1;
  }

  cuBQL::omp::Context context(gpu_id);

  const HostTriangleMesh mesh = make_cube(); // Instantiate cube mesh
  const std::vector<cuBQL::box3f> host_bounds = mesh.primitive_bounds();

  std::cout << "Vertices  : " << mesh.vertices.size() << '\n';
  std::cout << "Triangles : " << mesh.indices.size() << '\n';

  cuBQL::vec3f* device_vertices = nullptr;
  cuBQL::vec3i* device_indices = nullptr;
  cuBQL::box3f* device_bounds = nullptr;

  context.alloc_and_upload(device_vertices, mesh.vertices);
  context.alloc_and_upload(device_indices, mesh.indices);
  context.alloc_and_upload(device_bounds, host_bounds);
  
  // Build the OpenMP BVH. The convenience wrapper constructs its own context
  // for the same device internally.
  const auto num_primitives = static_cast<std::uint32_t>(host_bounds.size());
  cuBQL::bvh3f bvh;
  cuBQL::BuildConfig build_config {};
  cuBQL::build_omp_target(bvh,
                          device_bounds,
                          num_primitives,
                          build_config,
                          gpu_id);

  // The builder input bounds are not needed for traversal.
  context.free(device_bounds);
  device_bounds = nullptr;

  std::cout << "BVH nodes      : " << bvh.numNodes << '\n';
  std::cout << "BVH primitives : " << bvh.numPrims << '\n';

  // TODO: perform ray traversal while the BVH and device mesh remain alive.
  
  // Release device memory
  cuBQL::omp::freeBVH(bvh, &context);
  bvh = {};

  context.free(device_vertices);
  context.free(device_indices);
  device_vertices = nullptr;
  device_indices = nullptr;
  
  return 0;
}
