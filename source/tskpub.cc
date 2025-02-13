#include "TSKPub/tskpub.hh"

#include <spdlog/spdlog.h>

#include <fkYAML/node.hpp>
#include <fstream>
#include <unordered_map>

#include "common.hh"
#include "reader/reader.hh"

namespace {
  std::unordered_map<std::string, tskpub::Reader::Ptr> sensor_reader_map;
}  // namespace

namespace tskpub {
  TSKPub::TSKPub(const std::string &config_file) {
    GlobalParams::get_instance().load_params(config_file);
    Log::init();
    auto sensors = GlobalParams::get_instance()
                       .yml["sensors"]
                       .get_value<std::vector<std::string>>();
    for (auto &sensor_name : sensors) {
      auto &params = tskpub::GlobalParams::get_instance().yml[sensor_name];
      const auto &type = params["type"].get_value_ref<const std::string &>();
      sensor_reader_map[sensor_name] = ReaderFactory::create(type, sensor_name);
    }
  }

  TSKPub::~TSKPub() {
    GlobalParams::get_instance().destroy();
    Log::destory();
  }

  MsgConstPtr TSKPub::read(const std::string &sensor_name) const {
    auto it = sensor_reader_map.find(sensor_name);
    if (it == sensor_reader_map.end()) {
      Log::critical("No reader for sensor: " + sensor_name);
      return nullptr;
    }
    auto msg = it->second->read();
    GlobalParams::get_instance().total_read_bytes += msg ? msg->size() : 0;
    return msg;
  }
}  // namespace tskpub
