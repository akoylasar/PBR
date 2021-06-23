#pragma once

#include <Neon.hpp>

namespace Akoylasar
{
  class Camera
  {
  public:
    Camera(const Neon::Vec3f& origin,
           const Neon::Vec3f& lookAt,
           const Neon::Vec3f& up,
           float fovy,
           float aspect,
           float near,
           float far);
    
    ~Camera() = default;
    
    void setOrigin(const Neon::Vec3f& origin);
    const Neon::Vec3f& getOrigin() const;

    void setLookAt(const Neon::Vec3f& lookAt, const Neon::Vec3f& up);
    const Neon::Vec3f& getLookAt() const;

    Neon::Vec3f getDirection() const;

    void setProjection(float fovy, float aspect, float near, float far);
    const Neon::Mat4f& getView() const;
    const Neon::Mat4f& getProjection() const;

    void translate(float amount);
    void pan(float deltaX, float deltaY);
    void rotate(float pitch, float yaw);

  private:
    Neon::Vec3f mOrigin;
    Neon::Vec3f mLookAt;
    Neon::Vec3f mUp;
    Neon::Mat4f mView;
    Neon::Mat4f mProjection;
  };
}
