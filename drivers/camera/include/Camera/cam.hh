#pragma once

#include <memory>
#include <string>
#include <vector>

namespace camera {
  struct Image {
    using Ptr = std::shared_ptr<Image>;
    using ConstPtr = std::shared_ptr<const Image>;
    std::vector<uint8_t> data;
    uint64_t stamp;
    int width;
    int height;
    int size;
    int channels;
    std::string pix_fmt;
    std::string encoding;
    float fps;
  };

  class Camera {
  public:
    Camera(const std::string& pipeline);
    ~Camera();

    bool connect();
    bool disconnect();
    Image::ConstPtr capture() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };
}  // namespace camera
