#include <capnp/serialize-packed.h>

#include "TSKPub/msg/imu.capnp.h"
#include "reader/reader.hh"

extern "C" {
#include <imu/hipnuc_dec.h>
#include <imu/serial_port.h>
}

namespace {
  constexpr static double Gravity = 9.8;

  struct IMU {
    using Ptr = std::shared_ptr<IMU>;

    IMU(std::string port, uint64_t baud_rate) : fd(-1) {
      if ((fd = serial_port_open(port.c_str())) < 0
          || serial_port_configure(fd, baud_rate) < 0) {
        throw std::runtime_error("Failed to open or configure port " + port
                                 + " with " + std::to_string(baud_rate));
      }

      // Enable data output
      serial_send_then_recv_str(fd, "AT+EOUT=1\r\n", "OK\r\n", buffer.data(),
                                buffer.size(), 200);
    }

    ~IMU() {
      if (fd >= 0) {
        serial_port_close(fd);
      }
    }

    std::vector<double> read() {
      hipnuc_raw_t hipnuc_raw{0};
      // Read data from serial port
      int len = serial_port_read(fd, buffer.data(), buffer.size());
      if (len <= 0) {
        return {};
      }

      for (int i = 0; i < len; i++) {
        if (hipnuc_input(&hipnuc_raw, buffer[i]) <= 0) {
          continue;
        }
        break;
      }

      auto& hi91 = hipnuc_raw.hi91;
      return {hi91.acc[0] * Gravity,  // 0
              hi91.acc[1] * Gravity,
              hi91.acc[2] * Gravity,
              hi91.gyr[0],  // 3
              hi91.gyr[1],
              hi91.gyr[2],
              hi91.mag[0],  // 6
              hi91.mag[1],
              hi91.mag[2],
              hi91.roll,  // 9
              hi91.pitch,
              hi91.yaw,
              hi91.quat[0],  // 12
              hi91.quat[1],
              hi91.quat[2],
              hi91.quat[3],
              hi91.air_pressure};
    };

    int fd;
    std::array<char, 1024> buffer;
  };

  IMU::Ptr imu{nullptr};

}  // namespace

namespace tskpub {
  IMUReader::IMUReader(std::string sensor_name) : Reader(sensor_name) {}

  IMUReader::~IMUReader() {
    if (imu) imu.reset();
  }

  void IMUReader::open_device() {
    auto params = GlobalParams::get_instance().yml[sensor_name_];
    imu = std::make_shared<IMU>(params["port"].get_value<std::string>(),
                                params["baud_rate"].get_value<uint64_t>());
  }

  Data::ConstPtr IMUReader::read() {
    if (!imu) open_device();
    std::vector<double> data;
    try {
      auto tmp = imu->read();
      data.swap(tmp);
    } catch (const std::exception& e) {
      data.resize(0);
    }

    if (data.empty()) {
      return nullptr;
    }
    return std::make_shared<Data>(package_data(data));
  }

  MsgPtr IMUReader::package_data(const std::vector<double>& data) {
    // build capnp message
    capnp::MallocMessageBuilder message{1024};
    auto imu = message.initRoot<Imu>();
    imu.setTopic(topic_);
    imu.setTimestamp(nano_now());
    auto linear_acceleration = imu.initLinearAcceleration();
    linear_acceleration.setX(data[0]);
    linear_acceleration.setY(data[1]);
    linear_acceleration.setZ(data[2]);
    auto angular_velocity = imu.initAngularVelocity();
    angular_velocity.setX(data[3]);
    angular_velocity.setY(data[4]);
    angular_velocity.setZ(data[5]);
    auto orientation = imu.initOrientation();
    orientation.setW(data[12]);
    orientation.setX(data[13]);
    orientation.setY(data[14]);
    orientation.setZ(data[15]);
    // serialize & package message to uint8_t*
    kj::VectorOutputStream output_stream;
    capnp::writePackedMessage(output_stream, message);
    auto buffer = output_stream.getArray();
    MsgPtr ret = std::make_shared<Msg>(buffer.size());
    ret->insert(ret->begin(), buffer.begin(), buffer.end());
    return ret;
  }
}  // namespace tskpub
