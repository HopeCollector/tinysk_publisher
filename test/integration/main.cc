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

#ifndef CONFIG_FILE
#  error "CONFIG_FILE macro must be defined"
#endif

namespace {
  int64_t milli_now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  struct Rate {
    // 休眠周期
    int64_t interval;
    // 当前轮启动时间
    int64_t start;

    Rate(int rate) : interval(int64_t(1e3 / rate)), start(milli_now()) {}

    void sleep();
  };

  struct Fixture {
    tskpub::TSKPub pub;
    fkyaml::node yml;

    Fixture() : pub{CONFIG_FILE} {
      std::ifstream ifs{CONFIG_FILE};
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

void Rate::sleep() {
  // 理想结束时间 = 当前轮启动时间 + 休眠周期
  auto expected_end = start + interval;
  // 实际结束时间
  auto actual_end = milli_now();

  // 若实际结束时间因为某种原因比启动时间还小
  // 那么下一轮的启动时间 = 实际结束时间 + 休眠周期
  if (actual_end < start) {
    expected_end = actual_end + interval;
  }
  start = expected_end;  // 配置下一轮的启动时间

  // 若实际结束时间超出预计，则不应该休眠
  if (actual_end > expected_end) {
    // 若实际结束时间超出预计时间的一个周期以上，则应立即启动下一轮
    if (actual_end > expected_end + interval) {
      // 立即启动下一轮意味着：下一轮的启动时间 = 实际结束时间
      start = actual_end;
    }
  } else {
    // 否则，休眠到预计结束时间
    std::this_thread::sleep_for(
        std::chrono::milliseconds(expected_end - actual_end));
  }
}

TEST_CASE("read<Status>") {
  Fixture f;
  auto num = f.read_streamly("info", 5);
  CHECK(num == 5);
}

TEST_CASE("read<Imu>") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  std::string sensor_name{"imu0"};
  auto port = f.yml[sensor_name]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));
  size_t max_num = 100;
  auto num = f.read_streamly(sensor_name, max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("read<Camera>") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  std::string sensor_name{"video"};
  auto port = f.yml[sensor_name]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));
  size_t max_num = 20;
  auto num = f.read_streamly(sensor_name, max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("read<Lidar>") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;
  std::string sensor_name{"laser"};
  auto port = f.yml[sensor_name]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(port));
  size_t max_num = 40;
  f.pub.read(sensor_name);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  auto num = f.read_streamly(sensor_name, max_num);
  CHECK(num > max_num * 0.5);
}

TEST_CASE("read<All>") {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  Fixture f;

  auto lport = f.yml["laser"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(lport));
  auto iport = f.yml["imu0"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(iport));
  auto vport = f.yml["video"]["port"].get_value<std::string>();
  REQUIRE(is_device_exist(vport));

  size_t max_snum = 10;
  size_t max_inum = 100;
  size_t max_cnum = 40;
  size_t max_lnum = 40;
  size_t snum = 0, inum = 0, cnum = 0, lnum = 0;
  auto sjob = std::thread([&] { snum = f.read_streamly("info", max_snum); });
  auto ijob = std::thread([&] { inum = f.read_streamly("imu0", max_inum); });
  auto cjob = std::thread([&] { cnum = f.read_streamly("video", max_cnum); });
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
