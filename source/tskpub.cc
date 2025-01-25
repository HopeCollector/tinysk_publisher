#include "tskpub/tskpub.hh"

#include <spdlog/spdlog.h>

#include <fkYAML/node.hpp>
#include <fstream>
#include <unordered_map>

#include "common.hh"
#include "reader/reader.hh"

namespace {}  // namespace

namespace tskpub {
  TSKPub::TSKPub() {
    auto sensors = GlobalParams::get_instance()
                       .yml["sensors"]
                       .get_value<std::vector<std::string>>();
    for (auto &sensor_name : sensors) {
      auto &params = tskpub::GlobalParams::get_instance().yml[sensor_name];
      const auto &type = params["type"].get_value_ref<const std::string &>();
      sensor_reader_map_[sensor_name] = ReaderFactory::create(type);
    }
  }

  TSKPub::~TSKPub() {}

  Data::ConstPtr TSKPub::read(const std::string &sensor_name) const noexcept {
    auto it = sensor_reader_map_.find(sensor_name);
    if (it == sensor_reader_map_.end()) {
      Log::critical("No reader for sensor: " + sensor_name);
      return nullptr;
    }
    return it->second->read();
  }
}  // namespace tskpub
