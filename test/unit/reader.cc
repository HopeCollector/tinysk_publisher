#include "reader/reader.hh"

#include <TSKPub/msg/image.capnp.h>
#include <TSKPub/msg/imu.capnp.h>
#include <TSKPub/msg/status.capnp.h>
#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>
#include <sys/stat.h>

#include <string>

#include "common.hh"

namespace {
  const std::string config_file{"/ws/publisher/test/unit/cfg.yml"};
  struct Fixture {
    Fixture(const std::string &cfg_file) {
      tskpub::GlobalParams::get_instance().load_params(cfg_file);
      tskpub::Log::init();
    }

    ~Fixture() {
      tskpub::GlobalParams::get_instance().destroy();
      tskpub::Log::destory();
    }

    const fkyaml::node &yaml() const {
      return tskpub::GlobalParams::get_instance().yml;
    }

    template <typename T>
    typename T::Ptr create_reader(const std::string &sensor_name) {
      auto &params = yaml()[sensor_name];
      auto type = params["type"].get_value<std::string>();
      return std::dynamic_pointer_cast<T>(
          tskpub::ReaderFactory::create(type, sensor_name));
    }
  };

  template <class T> struct CapnpMsg {
    using Reader = typename T::Reader;
    kj::ArrayPtr<const capnp::byte> segment;
    kj::ArrayInputStream input_stream;
    capnp::PackedMessageReader reader;
    Reader root;

    CapnpMsg(const tskpub::MsgConstPtr &msg)
        : segment(msg->data(), msg->size()),
          input_stream(segment),
          reader(input_stream) {
      root = reader.getRoot<T>();
    }
  };

  bool does_camera_device_exist(const std::string &device_path) {
    struct stat buffer;
    return (stat(device_path.c_str(), &buffer) == 0);
  }

}  // namespace

TEST_CASE("Status Reader read test") {
  Fixture f{config_file};
  auto sreader = f.create_reader<tskpub::StatusReader>("status");
  auto msg = sreader->read();
  CHECK((msg != nullptr));

  CapnpMsg<Status> capnpmsg(msg->data);
  auto &status = capnpmsg.root;
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

TEST_CASE("IMU Reader encode test") {
  Fixture f{config_file};
  std::vector<double> test_data{0.0,  1.0,  2.0,  3.0,  4.0,  5.0,
                                6.0,  7.0,  8.0,  9.0,  10.0, 11.0,
                                12.0, 13.0, 14.0, 15.0, 16.0};
  auto ireader = f.create_reader<tskpub::IMUReader>("imu");
  auto msg = ireader->package_data(test_data);
  CHECK((msg != nullptr));
  CHECK(msg->size() > 0);

  CapnpMsg<Imu> capnpmsg(msg);
  auto &imu = capnpmsg.root;
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

TEST_CASE("Camera Reader read test") {
  Fixture f{config_file};
  REQUIRE(does_camera_device_exist("/dev/video0"));

  auto cam = f.create_reader<tskpub::CameraReader>("video0");
  auto msg = cam->read();
  CHECK((msg != nullptr));

  CapnpMsg<Image> capnpmsg(msg->data);
  auto &image = capnpmsg.root;
  CHECK(image.hasTopic());
  CHECK(image.getTopic().size() > 0);
  CHECK(std::string(image.getTopic().cStr()) == "/tinysk/video0");
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
