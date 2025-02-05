#include <fstream>
#include <iostream>
#include <vector>

#include "Camera/cam.hh"

// clang-format off
// const char* pipeline
//     = "v4l2src device=/dev/video0 !"
//       "video/x-raw width=640 height=480 framerate=30/1 !"
//       "videoconvert !"
//       "v4l2jpegenc extra-controls=\"encode,video_bitrate_mode=1,video_bitrate=2500000\" !"
//       "videorate ! image/jpeg framerate=10/1 !"
//       "jpegparse ! appsink name=s";

const char* pipeline
    = "v4l2src device=/dev/video0 !"
      "video/x-raw, width=640, height=480, framerate=30/1 !"
      "videoconvert ! jpegenc !"
      "videorate ! image/jpeg framerate=10/1 !"
      "jpegparse ! appsink name=s";
// clang-format on

int main() {
  camera::Camera camera{pipeline};
  if (!camera.connect()) {
    return -1;
  }

  int cnt = 0;
  while (true) {
    camera::Image::ConstPtr image = camera.capture();
    if (!image) {
      break;
    }

    cnt++;
    // Do something with the image data
    std::cout << "[" << image->stamp
              << "] Received image data of size: " << image->size << std::endl;
    if (cnt > 100) {
      std::ofstream outFile("/ws/test.jpeg", std::ios::binary);
      outFile.write(reinterpret_cast<const char*>(image->data.data()),
                    image->data.size());
      break;
    }
  }

  return 0;
}