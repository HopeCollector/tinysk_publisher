#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "common.hh"

#define READER_REGIST(msg_type, reader_type)                                  \
  static bool regist_success                                                  \
      = tskpub::ReaderFactory::regist(msg_type, [](std::string sensor_name) { \
          return std::make_shared<reader_type>(sensor_name);                  \
        });

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

  class StatusReader final : public Reader {
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
  };

  class ReaderFactory {
  public:
    using Creator = std::function<Reader::Ptr(std::string)>;
    static Reader::Ptr create(const std::string& msg_type,
                              const std::string& sensor_name);
    static bool regist(std::string msg_type, Creator creator);

  private:
    ReaderFactory() = delete;
    ReaderFactory(ReaderFactory&) = delete;
    ReaderFactory(const ReaderFactory&) = delete;
    ReaderFactory& operator=(ReaderFactory&) = delete;

    static std::unordered_map<std::string, Creator> creators_;
  };
}  // namespace tskpub