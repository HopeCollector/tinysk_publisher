#pragma once

#include <spdlog/spdlog.h>

#include <fkYAML/node.hpp>
#include <memory>

#include "TSKPub/tskpub.hh"

namespace tskpub {
  /// @brief Get current time in nanoseconds
  /// @return
  uint64_t nano_now();

  /// @brief Spdlog wrapper
  class Log {
  public:
    static void init();
    static void destory();

    static std::shared_ptr<spdlog::logger> get_logger() { return logger_; }
    static void set_logger(std::shared_ptr<spdlog::logger> logger) {
      logger_ = logger;
    }

    static void trace(const std::string& msg) {
      if (logger_) logger_->trace(msg);
    }
    static void debug(const std::string& msg) {
      if (logger_) logger_->debug(msg);
    }
    static void info(const std::string& msg) {
      if (logger_) logger_->info(msg);
    }
    static void warn(const std::string& msg) {
      if (logger_) logger_->warn(msg);
    }
    static void error(const std::string& msg) {
      if (logger_) logger_->error(msg);
    }
    static void critical(const std::string& msg) {
      if (logger_) logger_->critical(msg);
    }

  private:
    Log() = delete;
    Log(const Log&) = delete;
    Log& operator=(Log&) = delete;
    static std::shared_ptr<spdlog::logger> logger_;
  };

  /// @brief Global parameters
  class GlobalParams {
  public:
    /// @brief Get the singleton instance
    /// @return GlobalParams&
    static GlobalParams& get_instance();

    /// @brief Load parameters from a YAML file
    /// @param cfg_filename YAML file name
    void load_params(const std::string& cfg_filename);

    /// @brief Destroy the singleton instance
    void destroy();

    /// @brief YAML node
    fkyaml::node yml;

    /// @brief Total read bytes
    std::atomic<uint64_t> total_read_bytes;

  private:
    GlobalParams();
    ~GlobalParams();
    GlobalParams(const GlobalParams&) = delete;
    GlobalParams& operator=(const GlobalParams&) = delete;
  };
}  // namespace tskpub