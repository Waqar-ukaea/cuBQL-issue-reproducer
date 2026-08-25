#include <iostream>
#include <vector>

#include <omp.h>

// I'm not sure why exactly this is necessary but it is needed to get things 
// to compile when I use llvm/clang. According to ChatGPT/Codex: 
  /* 
     Clang defines __CUDA_ARCH__ when compiling an OpenMP NVPTX device image,
     even though CUDA language keywords and runtime constants are unavailable.
  */

#if defined(__CUDA_ARCH__) && !defined(__CUDACC__)
#undef __CUDA_ARCH__
#endif

#include "mesh.h"

#include "cuBQL/bvh.h"
#include "cuBQL/builder/omp.h"

int main(int argc, char** argv) {

  const int host_id = omp_get_initial_device();
  const int gpu_id = omp_get_default_device();

  if (omp_get_num_devices() == 0) {
    std::cerr << "No OpenMP target devices found\n";
    return 1;
  }

  const HostTriangleMesh mesh = make_cube(); // Instantiate cube mesh

  std::cout << "Vertices  : " << mesh.vertices.size() << '\n';
  std::cout << "Triangles : " << mesh.indices.size() << '\n';

  // Transfer vertices to OpenMP device
  const std::size_t vertex_bytes = mesh.vertices.size() * sizeof(cuBQL::vec3f);
  auto* device_vertices = static_cast<cuBQL::vec3f*>(omp_target_alloc(vertex_bytes, gpu_id));
  omp_target_memcpy(device_vertices,
                    mesh.vertices.data(),
                    vertex_bytes,
                    0,
                    0, 
                    gpu_id,
                    host_id);

  // Transfer indices to OpenMP device
  const std::size_t index_bytes = mesh.indices.size() * sizeof(cuBQL::vec3i);
  auto* device_indices = static_cast<cuBQL::vec3i*>(omp_target_alloc(index_bytes, gpu_id));
  omp_target_memcpy(device_vertices,
                    mesh.vertices.data(),
                    index_bytes,
                    0,
                    0, 
                    gpu_id,
                    host_id);

  // Transfer bounds data to OpenMP device
  std::vector<cuBQL::box3f> host_bounds = mesh.primitive_bounds();
  const std::size_t bounds_bytes = host_bounds.size() * sizeof(cuBQL::box3f);
  auto* device_bounds = static_cast<cuBQL::box3f*>(omp_target_alloc(bounds_bytes, gpu_id));
  omp_target_memcpy(device_bounds,
                    host_bounds.data(),
                    bounds_bytes,
                    0,
                    0, 
                    gpu_id,
                    host_id);
  
  // Build OpenMP bvh
  const int num_prims = host_bounds.size();
  cuBQL::BuildConfig build_params;
  cuBQL::bvh3f bvh;
  cuBQL::build_omp_target(bvh,
                          device_bounds,
                          num_prims,
                          build_params,
                          gpu_id);


  

  omp_target_free(device_vertices, gpu_id);
  omp_target_free(device_indices, gpu_id);
  omp_target_free(device_bounds, gpu_id);
  omp_target_free(bvh.nodes, gpu_id);
  omp_target_free(bvh.primIDs, gpu_id);
  
  return 0;
}
