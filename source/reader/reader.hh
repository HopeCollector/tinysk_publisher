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

    /// @brief Read data from sensor
    /// @return Byte vector
    virtual MsgConstPtr read() = 0;

  protected:
    /// @brief topic name
    std::string topic_;
    /// @brief sensor name
    std::string sensor_name_;
    /// @brief message type in capnp
    std::string msg_type_;

    /// @brief Package data into a message
    /// @param builder Message builder
    /// @param max_sz Maximum size of the message
    /// @return Byte vector
    MsgPtr to_msg(capnp::MallocMessageBuilder& builder, size_t max_sz);
  };

  /// @brief Factory class for creating readers
  class ReaderFactory {
  public:
    /// @brief Creator function type
    using Creator = std::function<Reader::Ptr(std::string)>;

    /// @brief Create a reader
    /// @param msg_type
    /// @param sensor_name
    /// @return Reader::Ptr
    static Reader::Ptr create(const std::string& msg_type,
                              const std::string& sensor_name);

    /// @brief Register a creator function
    /// @param msg_type Message types processed by Reader
    /// @param creator Creator function for Reader
    /// @return
    static bool regist(std::string msg_type, Creator creator);

    /// @brief Get all registered creators.
    ///        Use a delayed construction method to ensure that the map is
    ///        constructed first
    /// @return msg_type -> Creator map
    static std::unordered_map<std::string, Creator>& creaters();

  private:
    ReaderFactory() = delete;
    ReaderFactory(ReaderFactory&) = delete;
    ReaderFactory(const ReaderFactory&) = delete;
    ReaderFactory& operator=(ReaderFactory&) = delete;
  };

  /// @brief Registration aids
  /// @tparam T Reader type
  template <typename T> struct ReaderRegistor {
    /// @brief Registration of the constructor with the factory will be
    ///        completed before the main function runs.
    ///        The __attribute__((used)) attribute is used to ensure that the
    ///        registration function is not optimized out by the compiler.
    /// @return
    static bool regist() __attribute__((used)) {
      return ReaderFactory::regist(T::msg_type(), [](std::string sensor_name) {
        return std::make_shared<T>(sensor_name);
      });
    }

    /// @brief Registration flag
    static bool registed;

    ReaderRegistor() {
      // Ensure that the registration function is called before the main
      // function runs
      (void)registed;
    }
  };

  // Static members of the template class are initialized and no actual code is
  // produced until actual instantiation Therefore, you need to add
  // the__attribute__(used)) attribute to the above register () function to
  // prevent it from being optimized by the compiler
  template <typename T> bool ReaderRegistor<T>::registed
      = ReaderRegistor<T>::regist();

  // Use CRTP pattern to register your own constructor in ReaderFactory.
  // The core idea of CRTP is to have a derived class inherit from a template
  // class and pass itself as a template parameter to the template class
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

    // Use the PIMPL pattern to hide implementation details in the
    // implementation file of the class
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