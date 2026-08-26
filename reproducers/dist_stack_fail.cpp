#include <cmath>
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
#include "cuBQL/math/Ray.h"
#include "cuBQL/queries/triangleData/math/rayTriangleIntersections.h"
#include "cuBQL/traversal/rayQueries.h"


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


  std::cout << "BVH nodes      : " << bvh.numNodes << '\n';
  std::cout << "BVH primitives : " << bvh.numPrims << '\n';

  // Trace one ray from near the centre of the cube towards its +X face. The
  // expected hit should be at a distance of 1
  cuBQL::Ray ray;
  ray.origin = {0.0f, 0.25f, 0.1f}; // origin off centre to avoid intersection with diagonal
  ray.direction = {1.0f, 0.0f, 0.1f};
  ray.tMin = 0.0f;
  ray.tMax = CUBQL_INF;

  int hit_primitive = -1;
  float hit_distance = CUBQL_INF;

  const int num_vertices = static_cast<int>(mesh.vertices.size());
  const int num_triangles = static_cast<int>(mesh.indices.size());
  
  // omp region for a single ray
  #pragma omp target device(gpu_id) \
    is_device_ptr(device_vertices, device_indices) \
    map(to: ray) map(from: hit_primitive, hit_distance)
  {
    hit_primitive = -1;
    hit_distance = CUBQL_INF;

    cuBQL::TriangleMesh device_mesh {
      device_vertices,
      device_indices,
      num_vertices,
      num_triangles
    };

    cuBQL::Ray traversal_ray = ray;

    auto intersect_primitive = [&](std::uint32_t primitive_id) -> float {
      const cuBQL::Triangle triangle = device_mesh.getTriangle(primitive_id);
      cuBQL::RayTriangleIntersection intersection;

      if (intersection.compute(traversal_ray, triangle)) {
        hit_primitive = static_cast<int>(primitive_id);
        hit_distance = intersection.t;
        traversal_ray.tMax = intersection.t;
      }

      return traversal_ray.tMax;
    };

    cuBQL::shrinkingRayQuery::forEachPrim(intersect_primitive,
                                          bvh,
                                          traversal_ray);
  }

  constexpr int expected_primitive = 10;
  constexpr float expected_distance = 1.0f;

  const bool correct_hit = hit_primitive == expected_primitive;

  std::cout << "\n--------------------------------------\n";
  std::cout << "Hit primitive     : " << hit_primitive << '\n';
  std::cout << "Hit distance      : " << hit_distance << '\n';
  std::cout << "Expected t        : " << expected_distance << '\n';
  std::cout << "Expected hit prim : " << expected_primitive << '\n';
  std::cout << "Traversal         : "
            << (correct_hit ? "PASS" : "FAIL") << '\n';
  std::cout << "--------------------------------------\n";
  // Release device memory
  cuBQL::omp::freeBVH(bvh, &context);
  bvh = {};

  context.free(device_bounds);
  context.free(device_vertices);
  context.free(device_indices);
  device_vertices = nullptr;
  device_indices = nullptr;
  device_bounds = nullptr;
  
  return correct_hit ? 0 : 1; // Return 0 on successful hit and 1 on failed
}
