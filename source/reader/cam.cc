#include <capnp/serialize-packed.h>

#include <Camera/cam.hh>
#include <atomic>
#include <thread>

#include "TSKPub/msg/Image.capnp.h"
#include "reader/reader.hh"

namespace tskpub {
  struct CameraReader::Impl {
    // camera pipeline for GStreamer
    std::string pipeline;

    // camera object
    camera::Camera cam;

    // thread to read image
    std::thread job;

    // mutex to protect image
    std::mutex imtx;
    camera::Image::ConstPtr image{nullptr};

    // flag to control the thread
    std::atomic<bool> is_running{true};

    // max image size
    size_t max_sz;
    Impl() = delete;
    Impl(const std::string& pipeline) : pipeline(pipeline), cam(pipeline) {}
    void read_cb();
  };

  void CameraReader::Impl::read_cb() {
    while (is_running) {
      auto tmp = cam.capture();
      if (!tmp) {
        continue;
      }
      std::lock_guard<std::mutex> lock(imtx);
      image.swap(tmp);
    }
  }

  CameraReader::CameraReader(std::string sensor_name) : Reader(sensor_name) {
    auto params = GlobalParams::get_instance().yml[sensor_name_];
    std::stringstream ss;
    auto width = params["width"].get_value<int>();
    auto height = params["height"].get_value<int>();

    // create camera pipeline
    // clang-format off
    ss << "v4l2src device=" << params["port"].get_value<std::string>() << " !"
       << " video/x-raw, width=" << width << ", height=" << height << " !"
       << " videoconvert ! " << params["enc_pipeline"].get_value<std::string>()
       << " videorate ! image/jpeg framerate=" << params["fps"].get_value<int>() << "/1 !"
       << " jpegparse ! appsink name=s";
    // clang-format on
    Log::info("Camera pipeline: " + ss.str());

    // create camera object
    impl_ = std::make_unique<Impl>(ss.str());
    impl_->max_sz = width * height * 3;
    if (!impl_->cam.connect()) {
      Log::critical("Failed to connect to camera");
      throw std::runtime_error("Failed to connect to camera");
    }

    // launch thread to read image
    impl_->job = std::thread(&Impl::read_cb, impl_.get());
  }

  CameraReader::~CameraReader() {
    // stop the read thread
    impl_->is_running = false;
    if (impl_->job.joinable()) {
      impl_->job.join();
    }
  }

  MsgConstPtr CameraReader::read() {
    camera::Image::ConstPtr img{nullptr};

    // get image from Impl::image
    {
      std::lock_guard<std::mutex> lock(impl_->imtx);
      img.swap(impl_->image);
    }
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
    // set data with zero copy
    kj::ArrayPtr<const kj::byte> data_ptr{
        reinterpret_cast<const kj::byte*>(img->data.data()), img->data.size()};
    image.setData(data_ptr);
    return to_msg(builder, impl_->max_sz);
  }

}  // namespace tskpub