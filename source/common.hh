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

    template<typename... Args>
    static void trace(Args&&... args) {
      if (logger_) logger_->trace(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void debug(Args&&... args) {
      if (logger_) logger_->debug(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void info(Args&&... args) {
      if (logger_) logger_->info(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void warn(Args&&... args) {
      if (logger_) logger_->warn(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void error(Args&&... args) {
      if (logger_) logger_->error(std::forward<Args>(args)...);
    }
    template<typename... Args>
    static void critical(Args&&... args) {
      if (logger_) logger_->critical(std::forward<Args>(args)...);
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