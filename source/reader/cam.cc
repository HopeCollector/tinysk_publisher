#include <capnp/serialize-packed.h>

#include <Camera/cam.hh>

#include "TSKPub/msg/image.capnp.h"
#include "reader/reader.hh"

namespace tskpub {
  struct CameraReader::Impl {
    std::string pipeline;
    camera::Camera cam;
    Impl() = delete;
    Impl(const std::string& pipeline) : pipeline(pipeline), cam(pipeline) {}
  };

  CameraReader::CameraReader(std::string sensor_name) : Reader(sensor_name) {
    auto params = GlobalParams::get_instance().yml[sensor_name_];
    auto pipeline = params["pipeline"].get_value<std::string>();
    impl_ = std::make_unique<Impl>(pipeline);
    if (!impl_->cam.connect()) {
      Log::critical("Failed to connect to camera");
      throw std::runtime_error("Failed to connect to camera");
    }
  }

  CameraReader::~CameraReader() {}

  MsgConstPtr CameraReader::read() {
    auto img = impl_->cam.capture();
    if (!img) {
      return nullptr;
    }
    return package_data(reinterpret_cast<const void*>(img.get()));
  }

  MsgPtr CameraReader::package_data(const void* data) {
    auto img = reinterpret_cast<const camera::Image*>(data);
    auto builder
        = capnp::MallocMessageBuilder(img->size + sizeof(camera::Image));
    auto image = builder.initRoot<Image>();
    image.setTopic(topic_);
    image.setTimestamp(nano_now());
    image.setWidth(img->width);
    image.setHeight(img->height);
    image.setEncoding(img->encoding);
    image.setFps(img->fps);
    kj::ArrayPtr<const kj::byte> data_ptr{
        reinterpret_cast<const kj::byte*>(img->data.data()), img->data.size()};
    image.setData(data_ptr);
    return to_msg(builder, img->size + sizeof(camera::Image));
  }

}  // namespace tskpub