#include <TSKPub/msg/status.capnp.h>
#include <capnp/serialize-packed.h>
#include <doctest/doctest.h>

#include <TSKPub/tskpub.hh>
#include <string>

const std::string config_file = "/ws/publisher/configs/test.yml";

TEST_CASE("Easy Test") {
  tskpub::TSKPub publisher(config_file);
  auto msg = publisher.read("status");
  CHECK((msg != nullptr));
  CHECK(msg->size() > 0);

  kj::ArrayPtr<const capnp::byte> segment{msg->data(), msg->size()};
  kj::ArrayInputStream input_stream(segment.asBytes());
  auto reader = capnp::PackedMessageReader(input_stream);
  auto status = reader.getRoot<Status>();
  CHECK(status.hasTopic());
  CHECK(status.getTopic().size() > 0);
  CHECK(std::string(status.getTopic().cStr()) == "/tinysk/status");
  CHECK(status.getTimestamp() > 0);
  CHECK(status.hasMessage());
  CHECK(status.getMessage().size() > 0);
  CHECK(std::string(status.getMessage().cStr()) == "Hello, World!");
}
