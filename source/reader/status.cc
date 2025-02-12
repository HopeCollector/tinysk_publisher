#include <TSKPub/msg/status.capnp.h>
#include <capnp/serialize-packed.h>

#include "reader/reader.hh"

namespace {
  // run command in terminal then return the result
  std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
      throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
    }
    return result;
  }

  std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
    }
    return tokens;
  }
}  // namespace

namespace tskpub {
  struct StatusReader::Impl {
    std::string cmd;
    std::atomic<uint64_t>* total_read_bytes;
  };

  StatusReader::StatusReader(std::string sensor_name)
      : Reader(sensor_name), impl_(std::make_unique<Impl>()) {
    auto params = GlobalParams::get_instance().yml[sensor_name_];
    impl_->cmd = params["cmd"].get_value<std::string>();
    impl_->total_read_bytes = &GlobalParams::get_instance().total_read_bytes;
  }

  StatusReader::~StatusReader() {}

  Data::ConstPtr StatusReader::read() {
    capnp::MallocMessageBuilder message{1024};
    auto status = message.initRoot<Status>();
    status.setTopic(topic_);
    status.setTimestamp(nano_now());

    // get the status of the system
    auto results = split(exec(impl_->cmd.c_str()), ';');
    if (results.size() != 6) {
      return nullptr;
    }
    status.setCpuUsage(std::stod(results[0]));
    status.setCpuTemp(std::stod(results[1]));
    status.setMemUsage(std::stod(results[2]));
    status.setBatteryVoltage(std::stod(results[3]));
    status.setBatteryCurrent(std::stod(results[4]));
    status.setIp(results[5]);
    status.setTotalReadBytes(impl_->total_read_bytes->load());
    impl_->total_read_bytes->store(0);
    kj::VectorOutputStream output_stream;
    capnp::writePackedMessage(output_stream, message);
    auto buffer = output_stream.getArray();
    Data::Ptr data = std::make_shared<Data>(buffer.size());
    data->append(static_cast<uint8_t*>(buffer.begin()), buffer.size());
    return data;
  }
}  // namespace tskpub