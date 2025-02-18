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
  fkyaml::node params;

  /// @brief Publisher class with ZMQ
  struct Publisher {
    using Ptr = std::unique_ptr<Publisher>;
    // ZMQ socket for publishing
    zmq::socket_t socket;
    // ZMQ socket for inproc communication (message queue)
    zmq::socket_t queue;
    // Address to bind
    std::string address;
    Publisher() = delete;
    Publisher(const std::string& address, int max_msg_size);
    ~Publisher();

    /// @brief Recv message from queue and send it to socket
    void work();
  };

  /// @brief Get current time in milliseconds
  /// @return ms in int64_t
  int64_t milli_now() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }

  struct Rate {
    // sleep interval in ms
    int64_t interval;
    // start time of current round
    int64_t start;

    Rate(int rate) : interval(int64_t(1e3 / rate)), start(milli_now()) {}

    void sleep();
  };

  struct Freq {
    // create time of current round
    int64_t start;
    // last update time
    int64_t last;
    // count of messages
    size_t cnt;
    // name of the frequency counter
    std::string name;
    Freq() = delete;
    Freq(const std::string& name);

    /// @brief cnt++ and print frequency if time up
    void update();
  };

  struct Impl {
    using Ptr = std::unique_ptr<Impl>;

    // TSKPub object
    std::unique_ptr<tskpub::TSKPub> pub;

    // config file path
    std::string config_file_path;

    // reader threads
    std::deque<std::thread> threads;

    // Publisher object
    Publisher::Ptr socket;
    Impl() = delete;
    Impl(const std::string& config_path);
    ~Impl();

    // Initialization
    void init();

    // Main loop
    void run();
  };
}  // namespace

Publisher::Publisher(const std::string& address, int max_msg_size)
    : socket(*context, zmq::socket_type::pub),
      queue(*context, zmq::socket_type::pull),
      address(address) {
  // always send the latest message
  socket.set(zmq::sockopt::conflate, 1);
  socket.bind(address);

  // innner process communication
  queue.bind("inproc://tinysk");
}

Publisher::~Publisher() {
  // stop the queue first to stop message receiving
  queue.close();
  // close the socket
  socket.close();
}

void Publisher::work() {
  Freq f("Publisher");
  Rate r(params["app"]["checking_rate"].get_value<int>());
  while (is_running) {
    zmq::message_t msg;
    if (!queue.recv(msg, zmq::recv_flags::dontwait)) {
      // no message in queue, sleep for a while
      r.sleep();
      continue;
    }
    socket.send(msg, zmq::send_flags::none);
    f.update();
  }
}

void Rate::sleep() {
  // expected end time = current round start time + sleep interval
  auto expected_end = start + interval;
  auto actual_end = milli_now();

  // If the actual end time is earlier than the start time for some reason, then
  // the next round of start time = actual end time + sleep period
  if (actual_end < start) {
    expected_end = actual_end + interval;
  }
  start = expected_end;  // this is the start time for the next round

  // If the actual end time exceeds the forecast, you should not sleep
  if (actual_end > expected_end) {
    // If the actual end time exceeds the expected time by more than one cycle,
    // the next round should be started immediately
    if (actual_end > expected_end + interval) {
      // Starting the next round immediately means: starting time of the next
      // round = actual ending time
      start = actual_end;
    }
  } else {
    // Otherwise, sleep until the estimated end time
    std::this_thread::sleep_for(
        std::chrono::milliseconds(expected_end - actual_end));
  }
}

Freq::Freq(const std::string& name)
    : start(milli_now()), last(start), cnt(0), name(name) {}

void Freq::update() {
  auto curt = milli_now();

  // print frequency every 10 seconds
  if (curt - last > 10 * 1e3) {
    std::stringstream ss;
    ss << name << " rate: " << std::fixed << std::setprecision(2)
       << cnt * 1e3 / (curt - last) << " Hz";
    INFO(ss.str());
    // reset counter to prevent overflow
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
  // destroy Publisher first to stop the message sending
  socket.reset();

  // wait all threads to stop
  std::for_each(threads.begin(), threads.end(),
                std::mem_fn(&std::thread::join));

  // unrefence the logger
  spdlog::drop(logger->name());

  // clear the ZMQ Context
  context.reset();
}

void Impl::init() {
  // init TSKPub object
  pub = std::make_unique<tskpub::TSKPub>(config_file_path);

  // get logger instance because TSKPub has already initialized the logger
  auto dst = params["log"]["filename"].get_value<std::string>();
  if (dst == "stdout" || dst == "stderr") {
    logger = spdlog::get("console");
  } else {
    logger = spdlog::get("file_logger");
  }
  if (!logger) {
    throw std::runtime_error("Logger not initialized");
  }

  // init ZMQ context and Publisher
  context = std::make_optional<zmq::context_t>(3);
  std::string address{"tcp://*:"};
  address += std::to_string(params["app"]["port"].get_value<int>());
  socket = std::make_unique<Publisher>(
      address, params["app"]["max_message_size"].get_value<int>());
  INFO("App Start");
}

void Impl::run() {
  // get all sensor names from config file
  auto sensors = params["sensors"].get_value<std::vector<std::string>>();

  // create read thread for each sensor
  for (const auto& name : sensors) {
    threads.emplace_back([&]() {
      Rate r(params[name]["rate"].get_value<size_t>());
      Freq f(name);

      // create the zmq inproc socket to send message to Publisher
      zmq::socket_t queue(*context, zmq::socket_type::push);
      queue.connect("inproc://tinysk");

      // keep running until stop
      while (is_running) {
        auto msg = pub->read(name);
        if (!msg) {
          DEBUG("Failed to read from {}", name);
          continue;
        }
        DEBUG("Read {} bytes from {}", msg->size(), name);

        // zero copy to transfer the message to Publisher
        zmq::const_buffer buf(msg->data(), msg->size());
        queue.send(buf, zmq::send_flags::none);

        // update frequency
        f.update();

        // sleep for a while
        r.sleep();
      }

      // close the queue socket
      queue.close();
    });
  }

  // start recv message from queue and send it to socket
  socket->work();
}

/// @brief Parse command line arguments
/// @param argc
/// @param argv
/// @return cxxopts::ParseResult
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

/// @brief Signal handler for SIGINT and SIGTERM
/// @param signal
void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    WARN("Received signal {}", strsignal(signal));
    // stop the main loop
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