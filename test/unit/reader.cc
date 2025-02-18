#include "reader/reader.hh"

#include <TSKPub/msg/Image.capnp.h>
#include <TSKPub/msg/Imu.capnp.h>
#include <TSKPub/msg/PointCloud.capnp.h>
#include <TSKPub/msg/Status.capnp.h>
#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>
#include <sys/stat.h>

#include <optional>
#include <string>
#include <thread>

#include "common.hh"

#ifndef CONFIG_FILE
#  error "CONFIG_FILE macro must be defined"
#endif

namespace {
  const std::string config_file{CONFIG_FILE};

  /// @brief Check if the device exists
  /// @param device_path path of the device
  /// @return true if the device exists
  bool is_device_exist(const std::string &device_path) {
    struct stat buffer;
    return (stat(device_path.c_str(), &buffer) == 0);
  }

  /// @brief Test fixture for simple setup a test case and create reader
  struct Fixture {
    Fixture(const std::string &cfg_file) {
      // load config file
      tskpub::GlobalParams::get_instance().load_params(cfg_file);
      // init logger
      tskpub::Log::init();
    }

    ~Fixture() {
      tskpub::GlobalParams::get_instance().destroy();
      tskpub::Log::destory();
    }

    const fkyaml::node &yaml() const {
      return tskpub::GlobalParams::get_instance().yml;
    }

    /// @brief Create reader for sensor_name
    /// @tparam T Reader type
    /// @param sensor_name name in the config file
    /// @return pointer to the reader
    template <typename T>
    typename T::Ptr create_reader(const std::string &sensor_name) {
      auto &params = yaml()[sensor_name];
      auto type = params["type"].get_value<std::string>();
      return std::dynamic_pointer_cast<T>(
          tskpub::ReaderFactory::create(type, sensor_name));
    }
  };

  /// @brief decoding capnp message
  /// @tparam T capnp message type
  template <class T> struct CapnpMsg {
    using Reader = typename T::Reader;
    std::string sensor_name;
    kj::ArrayPtr<const capnp::byte> segment;
    std::optional<kj::ArrayInputStream> input_stream{std::nullopt};
    std::optional<capnp::PackedMessageReader> reader{std::nullopt};
    std::optional<Reader> root{std::nullopt};

    CapnpMsg(const tskpub::MsgConstPtr &msg, const std::string &sensor_name)
        : sensor_name(sensor_name) {
      segment = kj::ArrayPtr<const kj::byte>(msg->data() + sensor_name.size(),
                                             msg->size() - sensor_name.size());
      input_stream.emplace(segment);
      reader.emplace(*input_stream);
      root = reader->getRoot<T>();
    }
  };
}  // namespace

// test if auto regist factory works
TEST_CASE("AutoRegistFactory") {
  Fixture f{config_file};
  auto sreader = f.create_reader<tskpub::StatusReader>("info");
  CHECK((sreader != nullptr));
  auto ireader = f.create_reader<tskpub::IMUReader>("imu0");
  CHECK((ireader != nullptr));
  auto lreader = f.create_reader<tskpub::LidarReader>("laser");
  CHECK((lreader != nullptr));
  auto creader = f.create_reader<tskpub::CameraReader>("video");
  CHECK((creader != nullptr));
}

// read once from StatusReader
TEST_CASE("Status.read") {
  Fixture f{config_file};
  auto sreader = f.create_reader<tskpub::StatusReader>("info");
  auto msg = sreader->read();
  CHECK((msg != nullptr));

  // decode the message and check the content
  CapnpMsg<Status> capnpmsg(msg, "info");
  auto &status = capnpmsg.root.value();
  CHECK(status.hasTopic());
  CHECK(status.getTopic().size() > 0);
  CHECK(std::string(status.getTopic().cStr()) == "/tinysk/status");
  CHECK(status.getTimestamp() > 0);

  CHECK(status.getCpuUsage() >= 0.0);
  CHECK(status.getCpuTemp() > 0.0);
  CHECK(status.getMemUsage() > 0.0);
  CHECK(status.getBatteryVoltage() > 0.0);
  CHECK(status.getBatteryCurrent() > 0.0);
  CHECK(status.getIp().size() > 0);
  CHECK(status.getTotalReadBytes() >= 0);
}

// package data from IMUReader
TEST_CASE("IMU.package_data") {
  Fixture f{config_file};
  std::string sensor_name{"imu0"};
  std::vector<double> test_data{0.0,  1.0,  2.0,  3.0,  4.0,  5.0,
                                6.0,  7.0,  8.0,  9.0,  10.0, 11.0,
                                12.0, 13.0, 14.0, 15.0, 16.0};
  auto ireader = f.create_reader<tskpub::IMUReader>(sensor_name);
  auto msg = ireader->package_data(test_data);
  CHECK((msg != nullptr));
  CHECK(msg->size() > 0);

  CapnpMsg<Imu> capnpmsg(msg, sensor_name);
  auto &imu = capnpmsg.root.value();
  CHECK(imu.hasTopic());
  CHECK(imu.getTopic().size() > 0);
  CHECK(std::string(imu.getTopic().cStr()) == "/tinysk/imu");
  CHECK(imu.getTimestamp() > 0);

  REQUIRE(imu.hasOrientation());
  auto ori = imu.getOrientation();
  CHECK(ori.getW() == test_data[12]);
  CHECK(ori.getX() == test_data[13]);
  CHECK(ori.getY() == test_data[14]);
  CHECK(ori.getZ() == test_data[15]);

  REQUIRE(imu.hasLinearAcceleration());
  auto lacc = imu.getLinearAcceleration();
  CHECK(lacc.getX() == test_data[0]);
  CHECK(lacc.getY() == test_data[1]);
  CHECK(lacc.getZ() == test_data[2]);

  REQUIRE(imu.hasAngularVelocity());
  auto avel = imu.getAngularVelocity();
  CHECK(avel.getX() == test_data[3]);
  CHECK(avel.getY() == test_data[4]);
  CHECK(avel.getZ() == test_data[5]);
}

// read once from IMUReader
TEST_CASE("IMU.read") {
  Fixture f{config_file};
  std::string sensor_name{"imu0"};
  auto port = f.yaml()[sensor_name]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));

  auto ireader = f.create_reader<tskpub::IMUReader>(sensor_name);
  auto msg = ireader->read();
  CHECK((msg != nullptr));

  CapnpMsg<Imu> capnpmsg(msg, sensor_name);
  auto &imu = capnpmsg.root.value();
  CHECK(imu.hasTopic());
  CHECK(imu.getTopic().size() > 0);
  CHECK(std::string(imu.getTopic().cStr()) == "/tinysk/imu");
  CHECK(imu.getTimestamp() > 0);
  CHECK(imu.hasOrientation());
  CHECK(imu.hasLinearAcceleration());
  CHECK(imu.hasAngularVelocity());
}

// read once from CameraReader
TEST_CASE("Camera.read") {
  Fixture f{config_file};
  std::string sensor_name{"video"};
  auto port = f.yaml()[sensor_name]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));

  auto cam = f.create_reader<tskpub::CameraReader>(sensor_name);
  auto msg = cam->read();
  CHECK((msg != nullptr));

  CapnpMsg<Image> capnpmsg(msg, sensor_name);
  auto &image = capnpmsg.root.value();
  CHECK(image.hasTopic());
  CHECK(image.getTopic().size() > 0);
  CHECK(std::string(image.getTopic().cStr()) == "/tinysk/video");
  CHECK(image.getTimestamp() > 0);

  CHECK(image.getWidth() == 640);
  CHECK(image.getHeight() == 480);
  CHECK(std::string(image.getEncoding().cStr()) == "jpeg");
  CHECK(image.getFps() == 10.);
  CHECK(image.getData().size() > 0);

  // check if the image data is valid
  const auto &data = image.getData();
  CHECK(data[0] == 0xff);
  CHECK(data[1] == 0xd8);
}

// read once from LidarReader
TEST_CASE("Lidar.read") {
  Fixture f{config_file};
  std::string sensor_name{"laser"};
  REQUIRE(
      is_device_exist(f.yaml()[sensor_name]["port"].get_value<std::string>()));

  auto lreader = f.create_reader<tskpub::LidarReader>(sensor_name);
  tskpub::MsgConstPtr msg{nullptr};
  // lidar may take some time to start
  while (!msg) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    msg = lreader->read();
  }
  CHECK((msg != nullptr));

  CapnpMsg<PointCloud> capnpmsg(msg, sensor_name);
  auto &cloud = capnpmsg.root.value();
  CHECK(cloud.hasTopic());
  CHECK(cloud.getTopic().size() > 0);
  CHECK(std::string(cloud.getTopic().cStr()) == "/tinysk/laser");
  CHECK(cloud.getTimestamp() > 0);

  auto points = cloud.getPoints();
  CHECK(points.size() > 1000);
}
