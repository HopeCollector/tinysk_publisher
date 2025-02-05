#include "Camera/cam.hh"

#include <chrono>
#include <sstream>
#include <string>

extern "C" {
#include <gst/app/gstappsink.h>
#include <gst/gst.h>
}

namespace {
  uint64_t nano_now() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
  }
}  // namespace

namespace camera {
  struct Camera::Impl {
    GstElement* pipelineElement;
    GstElement* appsink;
    std::string pipeline;

    Impl() : pipelineElement(nullptr), appsink(nullptr) {}
    ~Impl() {
      if (appsink) {
        gst_object_unref(appsink);
      }
      if (pipelineElement) {
        gst_element_set_state(pipelineElement, GST_STATE_NULL);
        gst_object_unref(pipelineElement);
      }
    }
  };

  Camera::Camera(const std::string& pipeline) : impl_(new Impl) {
    impl_->pipeline = pipeline;
  }

  Camera::~Camera() { disconnect(); }

  bool Camera::connect() {
    gst_init(nullptr, nullptr);

    GError* error = nullptr;
    impl_->pipelineElement = gst_parse_launch(impl_->pipeline.c_str(), &error);
    if (!impl_->pipelineElement) {
      std::stringstream ss;
      ss << "Failed to create pipeline: " << error->message << std::endl;
      g_clear_error(&error);
      throw std::runtime_error(ss.str());
    }

    impl_->appsink = gst_bin_get_by_name(GST_BIN(impl_->pipelineElement), "s");
    if (!impl_->appsink) {
      throw std::runtime_error("Failed to get appsink from pipeline");
    }

    gst_element_set_state(impl_->pipelineElement, GST_STATE_PLAYING);
    return true;
  }

  bool Camera::disconnect() {
    impl_.reset(new Impl);
    gst_deinit();
    return true;
  }

  Image::ConstPtr Camera::capture() const {
    GstSample* sample = gst_app_sink_pull_sample(GST_APP_SINK(impl_->appsink));
    if (!sample) {
      throw std::runtime_error("Failed to get sample from appsink");
    }

    GstBuffer* buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
      gst_sample_unref(sample);
      throw std::runtime_error("Failed to get buffer from sample");
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
      gst_sample_unref(sample);
      throw std::runtime_error("Failed to map buffer");
    }

    Image::Ptr image(new Image);
    image->data.resize(map.size);
    std::copy(map.data, map.data + map.size, image->data.begin());
    image->stamp = nano_now();
    image->width = 640;
    image->height = 480;
    image->size = map.size;
    image->channels = 3;
    image->pix_fmt = "jpeg";
    image->encoding = "jpeg";
    image->fps = 10.0;

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return image;
  }
}  // namespace camera
