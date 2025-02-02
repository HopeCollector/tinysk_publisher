#include <TSKPub/msg/imu.capnp.h>
#include <TSKPub/msg/status.capnp.h>
#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>

#include <TSKPub/tskpub.hh>
#include <iostream>
#include <string>

#include "common.hh"
#include "reader/reader.hh"

const std::string config_file = "/ws/publisher/configs/test.yml";

class TSKPubFixture {
public:
  TSKPubFixture() : publisher(config_file) {}

  template <class T> T* get_reader(const std::string& sensor_name) {
    return reinterpret_cast<T*>(publisher.get_reader(sensor_name));
  }

  tskpub::TSKPub publisher;
};

template <class T> struct CapnpMsg {
  using Reader = typename T::Reader;
  kj::ArrayPtr<const capnp::byte> segment;
  kj::ArrayInputStream input_stream;
  capnp::PackedMessageReader reader;
  Reader root;

  CapnpMsg(const tskpub::MsgConstPtr& msg)
      : segment(msg->data(), msg->size()),
        input_stream(segment),
        reader(input_stream) {
    root = reader.getRoot<T>();
  }
};

TEST_CASE("Fixure Test") {
  TSKPubFixture fixture;

  // status
  auto sreader = fixture.get_reader<tskpub::StatusReader>("status");
  CHECK((sreader != nullptr));
  CHECK(sreader->msg_type() == "Status");

  // imu
  auto ireader = fixture.get_reader<tskpub::IMUReader>("imu");
  CHECK((ireader != nullptr));
  CHECK(ireader->msg_type() == "Imu");
}

TEST_CASE("Easy Test") {
  TSKPubFixture fixture;
  auto msg = fixture.publisher.read("status");
  CHECK((msg != nullptr));
  CHECK(msg->size() > 0);

  CapnpMsg<Status> capnpmsg(msg);
  auto& status = capnpmsg.root;
  CHECK(status.hasTopic());
  CHECK(status.getTopic().size() > 0);
  CHECK(std::string(status.getTopic().cStr()) == "/tinysk/status");
  CHECK(status.getTimestamp() > 0);
  CHECK(status.hasMessage());
  CHECK(status.getMessage().size() > 0);
  CHECK(std::string(status.getMessage().cStr()) == "Hello, World!");
}

TEST_CASE("IMU Reader encode test") {
  TSKPubFixture fixture;
  std::vector<double> test_data{0.0,  1.0,  2.0,  3.0,  4.0,  5.0,
                                6.0,  7.0,  8.0,  9.0,  10.0, 11.0,
                                12.0, 13.0, 14.0, 15.0, 16.0};
  auto ireader = fixture.get_reader<tskpub::IMUReader>("imu");
  auto msg = ireader->package_data(test_data);
  CHECK((msg != nullptr));
  CHECK(msg->size() > 0);

  CapnpMsg<Imu> capnpmsg(msg);
  auto& imu = capnpmsg.root;
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