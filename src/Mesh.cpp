#include "Mesh.hpp"

#include <cmath>

#include "Debug.hpp"

namespace Akoylasar
{
  constexpr double kTwoPi = Neon::kPi * 2.0;
  std::unique_ptr<Mesh> Mesh::buildSphere(double radius,
                                          unsigned int hSegments,
                                          unsigned int vSegments)
  {
    std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
    const int numVerts = (hSegments + 1) * (vSegments + 1);
    mesh->vertices.reserve(numVerts);
    for (int h = 0; h <= hSegments; ++h)
    {
      const double phi = static_cast<double>(h) / hSegments;
      const double y = radius * std::cos(phi * Neon::kPi);
      const double scale = radius * std::sin(phi * Neon::kPi);
      for (int v = 0; v <= vSegments; ++v)
      {
        Vertex vert;
        const double theta = static_cast<double>(v) / vSegments;
        const double x = std::cos(theta * kTwoPi) * scale;
        const double z = std::sin(theta * kTwoPi) * scale;
        vert.position.x = x;
        vert.position.y = y;
        vert.position.z = z;
        vert.normal = Neon::normalize(vert.position);
        vert.uv.x = theta;
        vert.uv.y = phi;
        mesh->vertices.push_back(vert);
      }
    }
    // Generate indices. TRIANGLE topology.
    for (int h = 0; h < hSegments; ++h)
    {
      for (int v = 0; v < vSegments; ++v)
      {
        const unsigned int a = h * vSegments + (v + 1);
        const unsigned int b = h * vSegments + v;
        const unsigned int c = (h + 1) * vSegments + v;
        const unsigned int d = (h + 1) * vSegments + (v + 1);
        // Avoid degenerate triangles in the caps.
        if (h != 0)
        {
          mesh->indices.push_back(a);
          mesh->indices.push_back(b);
          mesh->indices.push_back(d);
        }
        if (h != hSegments - 1)
        {
          mesh->indices.push_back(b);
          mesh->indices.push_back(c);
          mesh->indices.push_back(d);
        }
      }
    }
    return mesh;
  }

  GpuMesh GpuMesh::createGpuMesh(const Mesh& mesh,
                                 GLuint positionAttribuIndex,
                                 GLuint normalAttribuIndex,
                                 GLuint uvAttribuIndex)
  {
    GpuMesh gpuMesh;
    
    // Vao setup.
    CHECK_GL_ERROR(glGenVertexArrays(1, &gpuMesh.vao));
    CHECK_GL_ERROR(glBindVertexArray(gpuMesh.vao));
    
    // Setup vertex buffer.
    CHECK_GL_ERROR(glGenBuffers(1, &gpuMesh.vbo));
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo));
    const auto vertexBufferSize = mesh.vertices.size() * sizeof(Vertex);
    CHECK_GL_ERROR(glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, &mesh.vertices.at(0), GL_STATIC_DRAW));
    
    // Setup index buffer.
    CHECK_GL_ERROR(glGenBuffers(1, &gpuMesh.ebo));
    CHECK_GL_ERROR(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpuMesh.ebo));
    const auto indexBufferSize = mesh.indices.size() * sizeof(std::uint32_t);
    CHECK_GL_ERROR(glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, &mesh.indices.at(0), GL_STATIC_DRAW));
    
    // Specify vertex format.
    CHECK_GL_ERROR(glBindBuffer(GL_ARRAY_BUFFER, gpuMesh.vbo));
    CHECK_GL_ERROR(glVertexAttribPointer(positionAttribuIndex, 3, GL_FLOAT, false, sizeof(Vertex), (void*)(offsetof(Vertex, position))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(0));
    CHECK_GL_ERROR(glVertexAttribPointer(normalAttribuIndex, 3, GL_FLOAT, false, sizeof(Vertex), (void*)(offsetof(Vertex, normal))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(1));
    CHECK_GL_ERROR(glVertexAttribPointer(uvAttribuIndex, 2, GL_FLOAT, false, sizeof(Vertex), (void*)(offsetof(Vertex, uv))));
    CHECK_GL_ERROR(glEnableVertexAttribArray(2));

    CHECK_GL_ERROR(glBindVertexArray(0));
    
    gpuMesh.indexCount = mesh.indices.size();
    gpuMesh.drawMode = GL_TRIANGLES;
    
    return gpuMesh;
  }

  void GpuMesh::releaseGpuMesh(GpuMesh& gpuMesh)
  {
    CHECK_GL_ERROR(glDeleteBuffers(1, &gpuMesh.vbo));
    CHECK_GL_ERROR(glDeleteBuffers(1, &gpuMesh.ebo));
    CHECK_GL_ERROR(glDeleteVertexArrays(1, &gpuMesh.vao));
    gpuMesh.vbo = 0;
    gpuMesh.ebo = 0;
    gpuMesh.vao = 0;
  }
}
