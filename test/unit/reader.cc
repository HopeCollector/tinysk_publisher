#include "reader/reader.hh"

#include <TSKPub/msg/image.capnp.h>
#include <TSKPub/msg/imu.capnp.h>
#include <TSKPub/msg/point_cloud.capnp.h>
#include <TSKPub/msg/status.capnp.h>
#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>
#include <sys/stat.h>

#include <optional>
#include <string>
#include <thread>

#include "cfg.yml.hh"
#include "common.hh"

namespace {
  const std::string config_file{CFG_PATH};
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
    std::string sensor_name;
    kj::ArrayPtr<const capnp::byte> segment;
    std::optional<kj::ArrayInputStream> input_stream{std::nullopt};
    std::optional<capnp::PackedMessageReader> reader{std::nullopt};
    std::optional<Reader> root{std::nullopt};

    CapnpMsg(const tskpub::MsgConstPtr &msg) {
      uint16_t name_len = msg->at(0) | (msg->at(1) << 8);
      sensor_name = std::string(msg->begin() + 2, msg->begin() + 2 + name_len);
      segment = kj::ArrayPtr<const kj::byte>(msg->data() + 2 + name_len,
                                             msg->size() - 2 - name_len);
      input_stream.emplace(segment);
      reader.emplace(*input_stream);
      root = reader->getRoot<T>();
    }
  };

  bool is_device_exist(const std::string &device_path) {
    struct stat buffer;
    return (stat(device_path.c_str(), &buffer) == 0);
  }

}  // namespace

TEST_CASE("Status Reader read test") {
  Fixture f{config_file};
  auto sreader = f.create_reader<tskpub::StatusReader>("status");
  auto msg = sreader->read();
  CHECK((msg != nullptr));

  CapnpMsg<Status> capnpmsg(msg);
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

TEST_CASE("Camera Reader read test") {
  Fixture f{config_file};
  REQUIRE(is_device_exist("/dev/video0"));

  auto cam = f.create_reader<tskpub::CameraReader>("video0");
  auto msg = cam->read();
  CHECK((msg != nullptr));

  CapnpMsg<Image> capnpmsg(msg);
  auto &image = capnpmsg.root.value();
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

TEST_CASE("Lidar Read Once Test") {
  Fixture f{config_file};
  REQUIRE(is_device_exist(f.yaml()["laser"]["port"].get_value<std::string>()));

  auto lreader = f.create_reader<tskpub::LidarReader>("laser");
  tskpub::MsgConstPtr msg{nullptr};
  while (!msg) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    msg = lreader->read();
  }
  CHECK((msg != nullptr));

  CapnpMsg<PointCloud> capnpmsg(msg);
  auto &cloud = capnpmsg.root.value();
  CHECK(cloud.hasTopic());
  CHECK(cloud.getTopic().size() > 0);
  CHECK(std::string(cloud.getTopic().cStr()) == "/tinysk/laser");
  CHECK(cloud.getTimestamp() > 0);

  auto points = cloud.getPoints();
  CHECK(points.size() > 1000);
}

TEST_CASE("Lidar Read Stream Test") {
  std::this_thread::sleep_for(std::chrono::seconds(5));
  Fixture f{config_file};
  REQUIRE(is_device_exist(f.yaml()["laser"]["port"].get_value<std::string>()));

  auto lreader = f.create_reader<tskpub::LidarReader>("laser");

  // wait for the first message
  tskpub::MsgConstPtr msg{nullptr};
  while (!msg) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    msg = lreader->read();
  }
  CHECK((msg != nullptr));

  // fast read next mybe empty
  lreader->read();
  CHECK((lreader->read() == nullptr));

  // read at 10hz for 20 times
  std::this_thread::sleep_for(std::chrono::seconds(5));
  int cnt = 0;
  for (int i = 0; i < 20; ++i) {
    msg = lreader->read();
    if (msg) {
      ++cnt;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  CHECK(cnt > 10);
}
