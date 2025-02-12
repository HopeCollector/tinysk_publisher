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

  struct Publisher {
    using Ptr = std::unique_ptr<Publisher>;
    zmq::context_t context;
    zmq::socket_t socket;
    std::string address;
    std::thread job;
    std::deque<tskpub::MsgConstPtr> queue;
    std::mutex qmtx;
    Publisher() = delete;
    Publisher(const std::string& address);
    ~Publisher();
    void publish(tskpub::MsgConstPtr msg);
    void work();
  };

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

Publisher::Publisher(const std::string& address)
    : context(),
      socket(context, zmq::socket_type::pub),
      address(address),
      job(&Publisher::work, this) {
  socket.set(zmq::sockopt::conflate, 1);
  socket.bind(address);
}

Publisher::~Publisher() {
  job.join();
  socket.close();
  context.close();
}

void Publisher::publish(tskpub::MsgConstPtr payload) {
  std::lock_guard<std::mutex> lock(qmtx);
  queue.push_back(payload);
}

void Publisher::work() {
  while (is_running) {
    while (queue.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::deque<tskpub::MsgConstPtr> buffer;
    {
      std::lock_guard<std::mutex> lock(qmtx);
      buffer.swap(queue);
    }

    std::for_each(buffer.begin(), buffer.end(), [&](tskpub::MsgConstPtr& msg) {
      zmq::const_buffer buf(msg->data(), msg->size());
      socket.send(buf, zmq::send_flags::none);
    });
  }
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
  socket = std::make_unique<Publisher>(
      params["app"]["address"].get_value<std::string>());
}

void Impl::run() {
  auto sensors = params["sensors"].get_value<std::vector<std::string>>();
  for (const auto& name : sensors) {
    threads.emplace_back([&]() {
      Rate r(params[name]["rate"].get_value<size_t>());
      while (is_running) {
        auto msg = pub->read(name);
        if (msg) {
          DEBUG("Read {} bytes from {}", msg->size(), name);
          socket->publish(msg);
        }
        r.sleep();
      }
    });
  }
  while (is_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
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