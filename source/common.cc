#include "common.hh"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <chrono>
#include <fstream>
#include <string>

namespace tskpub {
  uint64_t nano_now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  std::shared_ptr<spdlog::logger> Log::logger_ = nullptr;

  void Log::init() {
    // get log params
    auto& params = GlobalParams::get_instance().yml["log"];

    // create logger according to where to log
    const auto& dst = params["filename"].get_value_ref<const std::string&>();
    if (dst == "stdout") {  // log to stdout
      logger_ = spdlog::stdout_color_mt("console");
    } else if (dst == "stderr") {  // log to stderr
      logger_ = spdlog::stderr_color_mt("console");
    } else {  // log to file
      logger_ = spdlog::basic_logger_mt("file_logger", dst);
    }

    // set log level and pattern
    logger_->set_level(static_cast<spdlog::level::level_enum>(
        params["level"].get_value<int>()));
    logger_->set_pattern(params["pattern"].get_value_ref<const std::string&>());
  }

  void Log::destory() {
    if (!logger_) return;
    // unreferece logger
    spdlog::drop(logger_->name());
    logger_ = nullptr;
  }

  GlobalParams::GlobalParams() {}
  GlobalParams::~GlobalParams() {}

  GlobalParams& GlobalParams::get_instance() {
    static GlobalParams instance;
    return instance;
  }

  void GlobalParams::load_params(const std::string& cfg_filename) {
    std::ifstream ifs(cfg_filename);
    yml = fkyaml::node::deserialize(ifs);
  }

  void GlobalParams::destroy() { GlobalParams::~GlobalParams(); }
};  // namespace tskpub