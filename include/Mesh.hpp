/*
 * Copyright (c) Fouad Valadbeigi (akoylasar@gmail.com) */
#pragma once

#include <vector>
#include <cstdint>

#include <Neon.hpp>

#include <GL/gl3w.h>

namespace Akoylasar
{
  struct Vertex
  {
    Neon::Vec3f position;
    Neon::Vec3f normal;
    Neon::Vec2f uv;
  };

  struct Mesh
  {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    static std::unique_ptr<Mesh> buildSphere(double radius = 1.0,
                                             unsigned int hSegments = 32,
                                             unsigned int vSegments = 32);
    static std::unique_ptr<Mesh> buildQuad();
    static std::unique_ptr<Mesh> buildCube();
  };

  struct GpuMesh
  {
    GLuint vbo;
    GLuint ebo;
    GLuint vao;
    GLenum drawMode = 0;
    GLsizei indexCount = 0;
    static GpuMesh createGpuMesh(const Mesh& mesh,
                                 GLuint positionAttribuIndex = 0, // layout (location = 0) in shader.
                                 GLuint normalAttribuIndex = 1, // layout (location = 1) in shader.
                                 GLuint uvAttribuIndex = 2); // layout (location = 2) in shader.
    static void releaseGpuMesh(GpuMesh& gpuMesh);
    void draw() const;
    // @todo(Fouad): Add overload for adding GLB model.
  };
}
