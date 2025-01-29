#include "reader/reader.hh"

#include <capnp/common.h>
#include <capnp/serialize-packed.h>

#include <iomanip>

#include "TSKPub/msg/status.capnp.h"
#include "common.hh"

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

  // *****************
  // * ReaderFactory *
  // *****************
  std::unordered_map<std::string, ReaderFactory::Creator>
      ReaderFactory::creators_;

  Reader::Ptr ReaderFactory::create(const std::string& msg_type,
                                    const std::string& sensor_name) {
    auto it = creators_.find(msg_type);
    if (it == creators_.end()) {
      Log::critical("No creater for message type: " + msg_type);
      return nullptr;
    }
    return it->second(sensor_name);
  }

  bool ReaderFactory::regist(std::string msg_type, Creator creator) {
    auto it = creators_.find(msg_type);
    bool ret = false;
    if (it != creators_.end()) {
      Log::critical("Creater for message type: " + msg_type
                    + " already exists");
      ret = false;
    } else {
      creators_[msg_type] = creator;
      ret = true;
    }
    return ret;
  }

  // ****************
  // * StatusReader *
  // ****************
  READER_REGIST("Status", StatusReader);
  StatusReader::StatusReader(std::string sensor_name) : Reader(sensor_name) {}
  StatusReader::~StatusReader() {}
  Data::ConstPtr StatusReader::read() {
    capnp::MallocMessageBuilder message{1024};
    auto status = message.initRoot<Status>();
    status.setTopic(topic_);
    status.setTimestamp(nano_now());
    status.setMessage("Hello, World!");
    kj::VectorOutputStream output_stream;
    capnp::writePackedMessage(output_stream, message);
    auto buffer = output_stream.getArray();
    Data::Ptr data = std::make_shared<Data>(buffer.size());
    data->append(static_cast<uint8_t*>(buffer.begin()), buffer.size());
    return data;
  }
}  // namespace tskpub