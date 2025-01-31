#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "common.hh"

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
    virtual Data::ConstPtr read() = 0;

  protected:
    std::string topic_;
    std::string sensor_name_;
    std::string msg_type_;
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
    static bool regist() {
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
    Data::ConstPtr read() override;
    static const char* msg_type() noexcept { return "Status"; }
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
    Data::ConstPtr read() override;
    MsgPtr package_data(const std::vector<double>& data);
    static const char* msg_type() noexcept { return "Imu"; }
  };

}  // namespace tskpub