#pragma once

#include <memory>
#include <vector>

namespace tskpub {
  using Msg = std::vector<uint8_t>;
  using MsgPtr = std::shared_ptr<Msg>;
  using MsgConstPtr = std::shared_ptr<const Msg>;

  class TSKPub {
  public:
    TSKPub(const std::string& config_file);
    ~TSKPub();
    MsgConstPtr read(const std::string& sensor_name) const;
  };
}  // namespace tskpub