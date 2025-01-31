#pragma once

#include <spdlog/spdlog.h>

#include <fkYAML/node.hpp>
#include <memory>

#include "TSKPub/tskpub.hh"

namespace tskpub {
  struct Data {
    using Ptr = std::shared_ptr<Data>;
    using ConstPtr = std::shared_ptr<const Data>;
    Data(size_t size) : data(new Msg) { data->reserve(size); }
    Data(MsgPtr data) : data(data) {}

    void resize(size_t size) { data->resize(size); }

    void append(const uint8_t* src, size_t size) {
      data->insert(data->end(), src, src + size);
    }

    void append(const std::string& src) {
      data->insert(data->end(), src.begin(), src.end());
    }

    void append(const Data::ConstPtr src) {
      data->insert(data->end(), src->data->begin(), src->data->end());
    }

    uint8_t* raw() { return data->data(); }

    MsgConstPtr msg() const { return data; }

    const uint8_t* begin() { return data->data(); }
    size_t size() { return data->size(); }
    MsgPtr data;
  };

  uint64_t nano_now();

  class Log {
  public:
    static void init();
    static void destory();

    static std::shared_ptr<spdlog::logger> get_logger() { return logger_; }
    static void set_logger(std::shared_ptr<spdlog::logger> logger) {
      logger_ = logger;
    }

    static void trace(const std::string& msg) { logger_->trace(msg); }
    static void debug(const std::string& msg) { logger_->debug(msg); }
    static void info(const std::string& msg) { logger_->info(msg); }
    static void warn(const std::string& msg) { logger_->warn(msg); }
    static void error(const std::string& msg) { logger_->error(msg); }
    static void critical(const std::string& msg) { logger_->critical(msg); }

  private:
    Log() = delete;
    Log(const Log&) = delete;
    Log& operator=(Log&) = delete;
    static std::shared_ptr<spdlog::logger> logger_;
  };

  class GlobalParams {
  public:
    static GlobalParams& get_instance();
    void load_params(const std::string& cfg_filename);
    fkyaml::node yml;
    void destroy();

  private:
    GlobalParams();
    ~GlobalParams();
    GlobalParams(const GlobalParams&) = delete;
    GlobalParams& operator=(const GlobalParams&) = delete;
  };
}  // namespace tskpub