#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "common.hh"

namespace capnp {
  class MallocMessageBuilder;
}

namespace tskpub {
  class Reader {
  public:
    using Ptr = std::shared_ptr<Reader>;
    using ConstPtr = std::shared_ptr<const Reader>;
    Reader() = delete;
    Reader(Reader&) = delete;
    Reader(const Reader&) = delete;
    Reader& operator=(Reader&) = delete;
    Reader(std::string sensor_name);
    virtual ~Reader() = default;
    virtual MsgConstPtr read() = 0;

  protected:
    std::string topic_;
    std::string sensor_name_;
    std::string msg_type_;

    MsgPtr to_msg(capnp::MallocMessageBuilder& builder, size_t max_sz);
  };

  class ReaderFactory {
  public:
    using Creator = std::function<Reader::Ptr(std::string)>;
    static Reader::Ptr create(const std::string& msg_type,
                              const std::string& sensor_name);
    static bool regist(std::string msg_type, Creator creator);
    static std::unordered_map<std::string, Creator>& creaters();

  private:
    ReaderFactory() = delete;
    ReaderFactory(ReaderFactory&) = delete;
    ReaderFactory(const ReaderFactory&) = delete;
    ReaderFactory& operator=(ReaderFactory&) = delete;
  };

  template <typename T> struct ReaderRegistor {
    static bool regist() __attribute__((used)) {
      return ReaderFactory::regist(T::msg_type(), [](std::string sensor_name) {
        return std::make_shared<T>(sensor_name);
      });
    }
    static bool registed;
    ReaderRegistor() { (void)registed; }
  };
  template <typename T> bool ReaderRegistor<T>::registed
      = ReaderRegistor<T>::regist();

  class StatusReader final : public Reader,
                             public ReaderRegistor<StatusReader> {
  public:
    using Ptr = std::shared_ptr<StatusReader>;
    using ConstPtr = std::shared_ptr<const StatusReader>;
    StatusReader() = delete;
    StatusReader(StatusReader&) = delete;
    StatusReader(const StatusReader&) = delete;
    StatusReader& operator=(StatusReader&) = delete;
    StatusReader(std::string sensor_name);
    virtual ~StatusReader();
    MsgConstPtr read() override;
    static const char* msg_type() noexcept { return "Status"; }

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class IMUReader final : public Reader, public ReaderRegistor<IMUReader> {
  public:
    using Ptr = std::shared_ptr<IMUReader>;
    using ConstPtr = std::shared_ptr<const IMUReader>;
    IMUReader() = delete;
    IMUReader(IMUReader&) = delete;
    IMUReader(const IMUReader&) = delete;
    IMUReader& operator=(IMUReader&) = delete;
    IMUReader(std::string sensor_name);
    virtual ~IMUReader();
    void open_device();
    MsgConstPtr read() override;
    MsgPtr package_data(const std::vector<double>& data);
    static const char* msg_type() noexcept { return "Imu"; }
  };

  class CameraReader final : public Reader,
                             public ReaderRegistor<CameraReader> {
  public:
    using Ptr = std::shared_ptr<CameraReader>;
    using ConstPtr = std::shared_ptr<const CameraReader>;
    CameraReader() = delete;
    CameraReader(CameraReader&) = delete;
    CameraReader(const CameraReader&) = delete;
    CameraReader& operator=(CameraReader&) = delete;
    CameraReader(std::string sensor_name);
    virtual ~CameraReader();
    MsgConstPtr read() override;
    MsgPtr package_data(const void* data);
    static const char* msg_type() noexcept { return "Image"; }

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class LidarReader final : public Reader, public ReaderRegistor<LidarReader> {
  public:
    using Ptr = std::shared_ptr<LidarReader>;
    using ConstPtr = std::shared_ptr<const LidarReader>;
    LidarReader() = delete;
    LidarReader(LidarReader&) = delete;
    LidarReader(const LidarReader&) = delete;
    LidarReader& operator=(LidarReader&) = delete;
    LidarReader(std::string sensor_name);
    virtual ~LidarReader();
    MsgConstPtr read() override;
    MsgPtr package_data(const void* data);
    static const char* msg_type() noexcept { return "PointCloud"; }

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

}  // namespace tskpub