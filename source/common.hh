#pragma once

#include <spdlog/spdlog.h>

#include <fkYAML/node.hpp>

namespace tskpub {
  class GlobalParams {
  public:
    static GlobalParams& getInstance() {
      static GlobalParams instance;
      return instance;
    }

    std::string cfg_file;
    std::string log_pattern;

  private:
    GlobalParams() = default;
    ~GlobalParams() = default;
    GlobalParams(const GlobalParams&) = delete;
    GlobalParams& operator=(const GlobalParams&) = delete;
  };
}  // namespace tskpub