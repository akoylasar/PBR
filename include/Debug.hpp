#pragma once

#include <iostream>
#include <cassert>

#define DEBUG_ASSERT(expr) assert(expr)
#ifndef NDEBUG
	#define DEBUG_ASSERT_MSG(expr, msg)\
	do\
	{\
  	if (!(expr))\
		{\
    	std::cerr << "Assertion failed with message: "<< msg << std::endl;\
      std::terminate();\
  	}\
	} while (false)
#else
	#define DEBUG_ASSERT_MSG(expr, msg) do {} while (false)
#endif
namespace Akoylasar
{
  void checkGLError(const char* cmdName, const char* file, int line);
  void clearGLErrors();
}

#ifndef NDEBUG
	#define CHECK_GL_ERROR(cmd)\
	[&]()\
	{\
		(cmd);\
		Akoylasar::checkGLError(#cmd, __FILE__, __LINE__);\
  }()
#else
	#define CHECK_GL_ERROR(cmd) (cmd)
#endif
