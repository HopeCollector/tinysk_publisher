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
    auto& params = GlobalParams::get_instance().yml["log"];
    const auto& dst = params["filename"].get_value_ref<const std::string&>();
    if (dst == "stdout") {
      logger_ = spdlog::stdout_color_mt("console");
    } else if (dst == "stderr") {
      logger_ = spdlog::stderr_color_mt("console");
    } else {
      logger_ = spdlog::basic_logger_mt("file_logger", dst);
    }
    logger_->set_level(static_cast<spdlog::level::level_enum>(
        params["level"].get_value<int>()));
    logger_->set_pattern(params["pattern"].get_value_ref<const std::string&>());
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
};  // namespace tskpub