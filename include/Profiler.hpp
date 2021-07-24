#pragma once

#include <memory>
#include <cstdint>
#include <vector>

#include <GL/gl3w.h>

#include "Debug.hpp"

namespace Akoylasar
{
  constexpr double fromNsToMs = 1.0 / 1000000.0;
  class Profiler;
  class TimeStamp
  {
  public:
    friend class Profiler;

    TimeStamp()
    : mBackQuery(0), mFrontQuery(0)
    {
      CHECK_GL_ERROR(glGenQueries(1, &mBackQuery));
      CHECK_GL_ERROR(glGenQueries(1, &mFrontQuery));
    }

    ~TimeStamp()
    {
      CHECK_GL_ERROR(glDeleteQueries(1, &mBackQuery));
      CHECK_GL_ERROR(glDeleteQueries(1, &mFrontQuery));
    }

    void begin()
    {
      CHECK_GL_ERROR(glBeginQuery(GL_TIME_ELAPSED, mBackQuery));
    }

    void end()
    {
      CHECK_GL_ERROR(glEndQuery(GL_TIME_ELAPSED));
    }

    std::uint64_t getElapsedTime()
    {
      GLuint64 elapsedTime = 0;
      static bool clearErrorsOnFirstCall = true;
      if (clearErrorsOnFirstCall)
      {
        glGetQueryObjectui64v(mFrontQuery, GL_QUERY_RESULT, &elapsedTime);
        clearGLErrors();
        clearErrorsOnFirstCall = false;
      }
      else
      // @todo: Fix INVALID_OPERATION error thrown on first invocation.
      CHECK_GL_ERROR(glGetQueryObjectui64v(mFrontQuery, GL_QUERY_RESULT, &elapsedTime));
      return elapsedTime;
    }

  private:
    void swapBuffers()
    {
      std::swap(mBackQuery, mFrontQuery);
    }

  private:
    TimeStamp(const TimeStamp&) = delete;
    TimeStamp& operator=(const TimeStamp&) = delete;

  private:
    GLuint mBackQuery;
    GLuint mFrontQuery;
  };

  class Profiler
  {
  public:
    Profiler() = default;

    TimeStamp& createTimeStamp()
    {
      auto ts = std::make_shared<TimeStamp>();
      mTimeStamps.push_back(ts);
      return *ts;
    }

    void swapBuffers()
    {
      for (auto& ts : mTimeStamps)
        ts->swapBuffers();
    }

  private:
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;

  private:
    using TimeStampPtr = std::shared_ptr<TimeStamp>;
    std::vector<TimeStampPtr> mTimeStamps;
  };
}
