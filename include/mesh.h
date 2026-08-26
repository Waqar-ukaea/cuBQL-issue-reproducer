#ifndef CUBQL_DIST_STACK_REPRODUCER_MESH_H
#define CUBQL_DIST_STACK_REPRODUCER_MESH_H

#include <vector>

#include "cuBQL/queries/triangleData/Triangle.h"

struct HostTriangleMesh {
  std::vector<cuBQL::vec3f> vertices;
  std::vector<cuBQL::vec3i> indices;

  cuBQL::Triangle triangle(std::size_t triangle_id) const
  {
    const cuBQL::vec3i index = indices.at(triangle_id);

    return {
      vertices.at(index.x),
      vertices.at(index.y),
      vertices.at(index.z)
    };
  }

  std::vector<cuBQL::box3f> primitive_bounds() const 
  {
    std::vector<cuBQL::box3f> bounds(indices.size());
    
    for (std::size_t tri_id = 0; tri_id < indices.size(); ++tri_id)
    {
      bounds[tri_id] = triangle(tri_id).bounds();
    }

    return bounds;
  }

};

// hardcoded Host side unit cube triangle mesh
inline HostTriangleMesh make_cube()
{
  HostTriangleMesh mesh;

  mesh.vertices = {
    {-1.0f, -1.0f, -1.0f}, // 0
    { 1.0f, -1.0f, -1.0f}, // 1
    { 1.0f,  1.0f, -1.0f}, // 2
    {-1.0f,  1.0f, -1.0f}, // 3
    {-1.0f, -1.0f,  1.0f}, // 4
    { 1.0f, -1.0f,  1.0f}, // 5
    { 1.0f,  1.0f,  1.0f}, // 6
    {-1.0f,  1.0f,  1.0f}  // 7
  };

  mesh.indices = {
    {0, 2, 1}, {0, 3, 2}, // -Z
    {4, 5, 6}, {4, 6, 7}, // +Z
    {0, 1, 5}, {0, 5, 4}, // -Y
    {3, 7, 6}, {3, 6, 2}, // +Y
    {0, 4, 7}, {0, 7, 3}, // -X
    {1, 2, 6}, {1, 6, 5}  // +X
  };

  return mesh;
}

#endif // CUBQL_DIST_STACK_REPRODUCER_MESH_H