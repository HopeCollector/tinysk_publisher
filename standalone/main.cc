#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>

#include <TSKPub/tskpub.hh>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cxxopts.hpp>
#include <deque>
#include <fkYAML/node.hpp>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <zmq.hpp>

#define DEBUG(...) SPDLOG_LOGGER_DEBUG(logger, __VA_ARGS__)
#define INFO(...) SPDLOG_LOGGER_INFO(logger, __VA_ARGS__)
#define WARN(...) SPDLOG_LOGGER_WARN(logger, __VA_ARGS__)
#define ERROR(...) SPDLOG_LOGGER_ERROR(logger, __VA_ARGS__)

namespace {
  std::shared_ptr<spdlog::logger> logger{nullptr};
  std::atomic<bool> is_running{true};
  std::optional<zmq::context_t> context{std::nullopt};

  struct Publisher {
    using Ptr = std::unique_ptr<Publisher>;
    zmq::socket_t socket;
    zmq::socket_t queue;
    std::string address;
    Publisher() = delete;
    Publisher(const std::string& address, int max_msg_size);
    ~Publisher();
    void work();
  };

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

  struct Freq {
    int64_t start;
    int64_t last;
    size_t cnt;
    std::string name;
    Freq() = delete;
    Freq(const std::string& name);
    void update();
  };

  struct Impl {
    using Ptr = std::unique_ptr<Impl>;
    std::unique_ptr<tskpub::TSKPub> pub;
    fkyaml::node params;
    std::string config_file_path;
    std::deque<std::thread> threads;
    Publisher::Ptr socket;
    Impl() = delete;
    Impl(const std::string& config_path);
    ~Impl();
    void init();
    void run();
  };
}  // namespace

Publisher::Publisher(const std::string& address, int max_msg_size)
    : socket(*context, zmq::socket_type::pub),
      queue(*context, zmq::socket_type::pull),
      address(address) {
  // socket.set(zmq::sockopt::conflate, 1);
  socket.set(zmq::sockopt::sndhwm, max_msg_size);
  socket.bind(address);
  queue.bind("inproc://tinysk");
}

Publisher::~Publisher() {
  queue.close();
  socket.close();
}

void Publisher::work() {
  Freq f("Publisher");
  while (is_running) {
    zmq::message_t msg;
    if (!queue.recv(msg)) {
      WARN("Failed to receive message");
      continue;
    }
    socket.send(msg, zmq::send_flags::none);
    f.update();
  }
}

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

Freq::Freq(const std::string& name)
    : start(milli_now()), last(start), cnt(0), name(name) {}

void Freq::update() {
  auto curt = milli_now();
  if (curt - last > 10 * 1e3) {
    std::stringstream ss;
    ss << name << " rate: " << std::fixed << std::setprecision(2)
       << cnt * 1e3 / (curt - last) << " Hz";
    INFO(ss.str());
    cnt = 0;
    last = curt;
  }
  cnt++;
}

Impl::Impl(const std::string& config_path)
    : pub(nullptr), config_file_path(config_path), socket(nullptr) {
  std::ifstream config_file(config_path);
  params = fkyaml::node::deserialize(config_file);
}

Impl::~Impl() {
  WARN("Destroying Impl");
  is_running = false;
  std::for_each(threads.begin(), threads.end(),
                std::mem_fn(&std::thread::join));
  spdlog::drop(logger->name());
  context.reset();
}

void Impl::init() {
  pub = std::make_unique<tskpub::TSKPub>(config_file_path);
  auto dst = params["log"]["filename"].get_value<std::string>();
  if (dst == "stdout" || dst == "stderr") {
    logger = spdlog::get("console");
  } else {
    logger = spdlog::get("file_logger");
  }
  if (!logger) {
    throw std::runtime_error("Logger not initialized");
  }

  context = std::make_optional<zmq::context_t>(3);
  socket = std::make_unique<Publisher>(
      params["app"]["address"].get_value<std::string>(),
      params["app"]["max_message_size"].get_value<int>());
  INFO("App Start");
}

void Impl::run() {
  auto sensors = params["sensors"].get_value<std::vector<std::string>>();
  for (const auto& name : sensors) {
    threads.emplace_back([&]() {
      Rate r(params[name]["rate"].get_value<size_t>());
      Freq f(name);
      size_t cnt = 0;
      auto prvt = milli_now();
      zmq::socket_t queue(*context, zmq::socket_type::push);
      queue.connect("inproc://tinysk");
      while (is_running) {
        auto msg = pub->read(name);
        if (!msg) {
          WARN("Failed to read from {}", name);
        }
        DEBUG("Read {} bytes from {}", msg->size(), name);
        zmq::const_buffer buf(msg->data(), msg->size());
        queue.send(buf, zmq::send_flags::none);
        f.update();
        r.sleep();
      }
      queue.close();
    });
  }
  socket->work();
}

auto parse_args(int argc, char** argv) {
  // clang-format off
  cxxopts::Options options("TSK Publisher",
                           "Sensor compress publisher for tiny snake robot");
  options.add_options()
    ("c,config", "Config file path", cxxopts::value<std::string>())
    ("h,help", "Print help");
  return options.parse(argc, argv);
  // clang-format on
}

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    WARN("Received signal {}", strsignal(signal));
    is_running = false;
  }
}

void setup_signal_handlers() {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
}

int main(int argc, char** argv) {
  auto result = parse_args(argc, argv);
  Impl::Ptr impl{new Impl(result["config"].as<std::string>())};
  impl->init();
  setup_signal_handlers();
  impl->run();
  return 0;
}