#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>
#include <sys/stat.h>

#include <TSKPub/tskpub.hh>
#include <cfg.yml.hh>
#include <fkYAML/node.hpp>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace {
  struct Rate {
    size_t interval;
    std::chrono::time_point<std::chrono::system_clock> last_call;

    Rate(int rate)
        : interval(size_t(1000 / rate)),
          last_call(std::chrono::system_clock::now()) {}

    void sleep() {
      auto now = std::chrono::system_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now - last_call)
                         .count();
      if (elapsed < interval) {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(interval - elapsed));
      }
      last_call = now;
    }
  };

  struct Fixture {
    tskpub::TSKPub pub;
    fkyaml::node yml;

    Fixture() : pub{CFG_PATH} {
      std::ifstream ifs{CFG_PATH};
      yml = fkyaml::node::deserialize(ifs);
    }

    size_t read_streamly(const std::string &sensor_name, size_t n) const {
      size_t cnt = 0;
      Rate r(yml[sensor_name]["rate"].get_value<int>());
      for (size_t i = 0; i < n; i++) {
        if (pub.read(sensor_name)) cnt++;
        r.sleep();
      }
      return cnt;
    }
  };

  bool is_device_exist(const std::string &device_path) {
    struct stat buffer;
    return (stat(device_path.c_str(), &buffer) == 0);
  }
}  // namespace

TEST_CASE("TSKPub Read<Status> Test") {
  Fixture f;
  auto num = f.read_streamly("status", 5);
  CHECK(num == 5);
}

TEST_CASE("TSKPub Read<Imu> Test") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  auto port = f.yml["imu"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));
  size_t max_num = 100;
  auto num = f.read_streamly("imu", max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("TSKPub Read<Camera> Test") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  REQUIRE(is_device_exist("/dev/video0"));
  size_t max_num = 20;
  auto num = f.read_streamly("video0", max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("TSKPub Read<Lidar> Test") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  auto port = f.yml["laser"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));
  size_t max_num = 40;
  f.pub.read("laser");
  std::this_thread::sleep_for(std::chrono::seconds(10));
  auto num = f.read_streamly("laser", max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("TSKPub Read all") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;

  auto lport = f.yml["laser"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(lport));
  auto iport = f.yml["imu"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(iport));
  auto vport = "/dev/video0";
  REQUIRE(is_device_exist(vport));

  size_t max_snum = 10;
  size_t max_inum = 100;
  size_t max_cnum = 40;
  size_t max_lnum = 40;
  size_t snum = 0, inum = 0, cnum = 0, lnum = 0;
  auto sjob = std::thread([&] { snum = f.read_streamly("status", max_snum); });
  auto ijob = std::thread([&] { inum = f.read_streamly("imu", max_inum); });
  auto cjob = std::thread([&] { cnum = f.read_streamly("video0", max_cnum); });
  auto ljob = std::thread([&] {
    f.pub.read("laser");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    lnum = f.read_streamly("laser", max_lnum);
  });
  sjob.join();
  ijob.join();
  cjob.join();
  ljob.join();

  CHECK(snum == max_snum);
  CHECK(inum > max_inum * 0.5);
  CHECK(cnum > max_cnum * 0.5);
  CHECK(lnum > max_lnum * 0.5);
}
