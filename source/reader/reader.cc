#include "reader/reader.hh"

#include <capnp/common.h>
#include <capnp/serialize-packed.h>

#include <iomanip>

#include "TSKPub/msg/status.capnp.h"

namespace tskpub {
  Reader::Reader(std::string sensor_name) : sensor_name_(sensor_name) {
    auto& params = GlobalParams::get_instance().yml[sensor_name];
    if (params.empty()) {
      Log::critical("No params for sensor: " + sensor_name);
      return;
    }
    params["topic"].get_value_inplace(topic_);
    params["type"].get_value_inplace(msg_type_);
  }

  MsgPtr Reader::to_msg(capnp::MallocMessageBuilder& builder, size_t max_sz) {
    size_t prefix_len = sensor_name_.size();
    size_t capacity = prefix_len + max_sz;

    // write sensor name
    auto ret = std::make_shared<Msg>(capacity);
    ret->insert(ret->begin(), sensor_name_.begin(), sensor_name_.end());

    // write message body
    kj::ArrayPtr<kj::byte> array(&ret->at(prefix_len),
                                 ret->size() - prefix_len);
    kj::ArrayOutputStream out(array);
    capnp::writePackedMessage(out, builder);
    auto pkgsz = out.getArray().size();
    ret->resize(prefix_len + pkgsz);
    return ret;
  }

  // *****************
  // * ReaderFactory *
  // *****************
  Reader::Ptr ReaderFactory::create(const std::string& msg_type,
                                    const std::string& sensor_name) {
    const auto& map = creaters();
    auto it = map.find(msg_type);
    if (it == map.end()) {
      Log::critical("No creater for message type: " + msg_type);
      return nullptr;
    }
    return it->second(sensor_name);
  }

  bool ReaderFactory::regist(std::string msg_type, Creator creator) {
    auto& map = creaters();
    auto it = map.find(msg_type);
    bool ret = false;
    if (it != map.end()) {
      Log::critical("Creater for message type: " + msg_type
                    + " already exists");
      ret = false;
    } else {
      map[msg_type] = creator;
      ret = true;
    }
    return ret;
  }

  std::unordered_map<std::string, ReaderFactory::Creator>&
  ReaderFactory::creaters() {
    static std::unordered_map<std::string, Creator> creaters;
    return creaters;
  }
}  // namespace tskpub
