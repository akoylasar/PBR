#pragma once

#include <filesystem>
#include <array>
#include <fstream>
#include <sstream>
#include <utility>

using ShaderInfo = std::pair<std::filesystem::path, std::string>;
using ProgramInfo = std::array<ShaderInfo, 2>;

namespace Akoylasar
{
  class Common
  {
  public:
    static bool readToString(const std::filesystem::path& file, std::string& output)
    {
      std::ifstream strm {file};
      if (!strm.is_open())
        return true;
      std::stringstream buffer;
      buffer << strm.rdbuf();
      output = buffer.str();
      return false;
    }
  };
}
