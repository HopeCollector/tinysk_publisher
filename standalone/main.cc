#include <spdlog/spdlog.h>

#include <TSKPub/tskpub.hh>
#include <cxxopts.hpp>
#include <fkYAML/node.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <unordered_map>

#define INFO(...) SPDLOG_LOGGER_INFO(impl->logger, __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(impl->logger, __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(impl->logger, __VA_ARGS__)

struct Impl {
  using Ptr = std::unique_ptr<Impl>;
  using Callback = std::function<void(const std::string&)>;
  std::unordered_map<std::string, Callback> type_cb_map;
  std::unique_ptr<tskpub::TSKPub> pub;
  fkyaml::node params;
  std::shared_ptr<spdlog::logger> logger;
  std::string config_file_path;
  Impl() = delete;
  Impl(const std::string& config_path);
  void init();
  void status_cb(const std::string& sensor_name);
  void imu_cb(const std::string& sensor_name);
  void camera_cb(const std::string& sensor_name);
  void lidar_cb(const std::string& sensor_name);
};

Impl::Impl(const std::string& config_path)
    : pub(nullptr), logger(nullptr), config_file_path(config_path) {
  type_cb_map = std::unordered_map<std::string, Callback>{
      {"Status", std::bind(&Impl::status_cb, this, std::placeholders::_1)},
      {"Imu", std::bind(&Impl::imu_cb, this, std::placeholders::_1)},
      {"Camera", std::bind(&Impl::camera_cb, this, std::placeholders::_1)},
      {"Lidar", std::bind(&Impl::lidar_cb, this, std::placeholders::_1)}};
  std::ifstream config_file(config_path);
  params = fkyaml::node::deserialize(config_file);
}

void Impl::init() {
  pub = std::make_unique<tskpub::TSKPub>(config_file_path);
  auto dst = params["log"]["filename"].get_value<std::string>();
  if (dst == "stdout" || dst == "stderr") {
    logger = spdlog::get("console");
  } else {
    logger = spdlog::get("file_logger");
  }
  if (!logger) {
    throw std::runtime_error("Logger not initialized");
  }
}

void Impl::status_cb(const std::string& sensor_name) {
  logger->info("Status callback for sensor: {}", sensor_name);
}

void Impl::imu_cb(const std::string& sensor_name) {
  logger->info("Imu callback for sensor: {}", sensor_name);
}

void Impl::camera_cb(const std::string& sensor_name) {
  logger->info("Camera callback for sensor: {}", sensor_name);
}

void Impl::lidar_cb(const std::string& sensor_name) {
  logger->info("Lidar callback for sensor: {}", sensor_name);
}

auto parse_args(int argc, char** argv) {
  // clang-format off
  cxxopts::Options options("TSK Publisher",
                           "Sensor compress publisher for tiny snake robot");
  options.add_options()
    ("c,config", "Config file path", cxxopts::value<std::string>())
    ("h,help", "Print help");
  return options.parse(argc, argv);
  // clang-format on
}

int main(int argc, char** argv) {
  auto result = parse_args(argc, argv);
  Impl::Ptr impl{new Impl(result["config"].as<std::string>())};
  impl->init();
  INFO("Info log ok");
  WARN("Warn log ok");
  ERROR("Error log ok");
  return 0;
}