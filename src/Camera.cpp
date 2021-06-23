#include "Camera.hpp"

namespace
{
  float clamp(float value, float min, float max)
  {
    return std::min(max, std::max(value, min));
  }
}

namespace Akoylasar
{
  Camera::Camera(const Neon::Vec3f& origin,
                 const Neon::Vec3f& lookAt,
                 const Neon::Vec3f& up,
                 float fovy,
                 float aspect,
                 float near,
                 float far)
  : mOrigin(origin),
    mLookAt(lookAt),
    mUp(up),
    mView(1.0),
    mProjection(0.0)
  {
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
    const float fovyRad = Neon::degToRad * fovy;
    mProjection = Neon::makePerspective(fovyRad, aspect, near, far);
  }

  void Camera::setOrigin(const Neon::Vec3f& origin)
  {
    mOrigin = origin;
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
  }

  const Neon::Vec3f& Camera::getOrigin() const
  {
    return mOrigin;
  }
  
  void Camera::setLookAt(const Neon::Vec3f& lookAt, const Neon::Vec3f& up)
  {
    mLookAt = lookAt;
    mUp = up;
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
  }

  const Neon::Vec3f& Camera::getLookAt() const
  {
    return mLookAt;
  }

  Neon::Vec3f Camera::getDirection() const
  {
    return Neon::normalize(mOrigin - mLookAt);
  }
  
  void Camera::setProjection(float fovy, float aspect, float near, float far)
  {
    const float fovyRad = Neon::degToRad * fovy;
    mProjection = Neon::makePerspective(fovyRad, aspect, near, far);
  }
  
  const Neon::Mat4f& Camera::getView() const
  {
    return mView;
  }
  
  const Neon::Mat4f& Camera::getProjection() const
  {
    return mProjection;
  }

  void Camera::translate(float amount)
  {
    const auto delta = getDirection() * amount;
    mOrigin += delta;
    mLookAt += delta;
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
  }

  void Camera::pan(float deltaX, float deltaY)
  {
    const auto dir = getDirection();
    const auto right = normalize(cross(dir, mUp));
    const auto up = cross(right, dir);
    const auto delta = right * deltaX + up * deltaY;
    mOrigin += delta;
    mLookAt += delta;
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
  }

  void Camera::rotate(float pitch, float yaw)
  {
    // First change the frame of reference to that of the lookAt point.
    const auto transformedOrigin = mOrigin - mLookAt;

    // Calculate the radius (of the sphere camera position is located on).
    float r = Neon::mag(transformedOrigin);
    float r2 = r * r;

    // Calculate the projected (XZ plane) radius.
    float Rp = sqrtf(r2 - transformedOrigin.y * transformedOrigin.y);

    // Calculate the azimuth angle.
    float azimuth = std::acos(transformedOrigin.x / Rp);
    azimuth = transformedOrigin.z < 0.0 ? 2.0 * Neon::kPi - azimuth : azimuth;

    // Calculate the elevation angle.
    float elevation = std::asin(transformedOrigin.y / r);

    // Increase the elevation angle by adding delta of the vertical mouse movement.
    constexpr float minElevation = -Neon::kPi * 0.45f;
    constexpr float maxElevation = Neon::kPi * 0.45f;

    float newElevation = clamp(elevation + 0.1f * yaw, minElevation, maxElevation);

    // Calculate the new Y value.
    float newY = r * std::sin(newElevation);

    // Calculate the new size of the projected radius.
    float newRp = std::sqrtf(r2 - newY * newY);

    // Calculate new azimuth.
    float newAzimuth = azimuth + 0.1f * pitch;

    // Cacluate new X and Z values.
    float newX = newRp * std::cos(newAzimuth);
    float newZ = newRp * std::sin(newAzimuth);

    // Back to world transform.
    mOrigin.x = newX + mLookAt.x;
    mOrigin.y = newY + mLookAt.y;
    mOrigin.z = newZ + mLookAt.z;
    mView = Neon::makeLookAt(mOrigin, mLookAt, mUp);
  }
}
