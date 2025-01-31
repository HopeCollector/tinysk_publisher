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

  private:
    /// @brief 获取原始 Reader 指针，如果不清楚这么做的后果，请不要使用
    ///        它出现只是为了单元测试的需要
    /// @param sensor_name 传感器名称
    /// @return
    void* get_reader(const std::string& sensor_name) const;

    friend class TSKPubFixture;
  };
}  // namespace tskpub