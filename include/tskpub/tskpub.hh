#pragma once

#include <memory>
#include <vector>

namespace tskpub {

  using Data = std::vector<uint8_t>;
  using DataPtr = std::shared_ptr<Data>;
  using DataConstPtr = std::shared_ptr<const Data>;

  enum class DataType {
    IMU,
    IMAGE,
    LIDAR,
  };

  class TSKPub {
  public:
    TSKPub();
    ~TSKPub();
    DataConstPtr read() const noexcept;
  };
}  // namespace tskpub