#include "reader/reader.hh"

#include "common.hh"

namespace tskpub {
  Reader::Reader(std::string sensor_name) : sensor_name_(sensor_name) {
    auto &params = GlobalParams::get_instance().yml[sensor_name];
    if (params.empty()) {
      Log::critical("No params for sensor: " + sensor_name);
      return;
    }
    params["topic"].get_value_inplace(topic_);
    params["msg_type"].get_value_inplace(msg_type_);
  }

  // *****************
  // * ReaderFactory *
  // *****************
  Reader::Ptr ReaderFactory::create(std::string msg_type) {
    auto it = creators_.find(msg_type);
    if (it == creators_.end()) {
      Log::critical("No creater for message type: " + msg_type);
      return nullptr;
    }
    return it->second(msg_type);
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
  READER_REGIST("status", StatusReader);
  StatusReader::StatusReader(std::string sensor_name) : Reader(sensor_name) {}
  StatusReader::~StatusReader() {}
  Data::ConstPtr StatusReader::read() {
    Data::Ptr data = std::make_shared<Data>(1024);
    data->append(topic_);
    data->append(std::to_string(nano_now()));
    data->append(callback_());
    return data;
  }

  void StatusReader::set_callback(std::function<std::string()> callback) {
    callback_ = callback;
  }
}  // namespace tskpub