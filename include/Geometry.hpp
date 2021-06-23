#include <vector>

#include <Neon.hpp>

namespace Akoylasar
{
  struct Vertex
  {
    Neon::Vec3f position;
    Neon::Vec3f normal;
    Neon::Vec3f uv;
  }

  struct Mesh
  {
    std::vector<Neon::Vec3d> positions;
    std::vector<Neon::Vec3d> normals;
    std::vector<Neon::Vec2d> uvs;
    std::vector<unsigned int> indices;
  };

  std::unique_ptr<Mesh> buildSphere(double radius = 1.0,
                                        unsigned int hSegments = 32,
                                        unsigned int vSegments = 32);
  std::unique_ptr<Mesh> buildBox();
}
