#include <TSKPub/msg/point_cloud.capnp.h>
#include <capnp/serialize-packed.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <xtsdk/utils.h>
#include <xtsdk/xtsdk.h>

#include <memory>

#include "reader/reader.hh"

namespace {
  using Point = pcl::PointXYZI;
  using Cld = pcl::PointCloud<Point>;

  struct DeviceParams {
    int imgType;
    int HDR;
    int minLSB;
    int frequency_modulation;
    int cloud_coord;
    int int1, int2, int3, intgs;
    int cut_corner;
    int maxfps;

    bool start_stream;
    bool hmirror;
    bool vmirror;

    std::string connect_address;
  };

  struct FilterParams {
    int medianSize;
    bool kalmanEnable;
    float kalmanFactor;
    int kalmanThreshold;
    bool edgeEnable;
    int edgeThreshold;
    bool dustEnable;
    int dustThreshold;
    int dustFrames;
  };

  struct Params {
    DeviceParams device;
    FilterParams filter;
  };
}  // namespace

namespace tskpub {
  struct LidarReader::Impl {
    std::unique_ptr<XinTan::XtSdk> xtsdk{nullptr};
    Cld::ConstPtr cld{nullptr};
    std::string port;
    Params params;

    ~Impl() {
      if (xtsdk && xtsdk->isconnect()) {
        xtsdk->stop();
        xtsdk->setCallback();
        xtsdk->shutdown();
      }
    }

    // copy from sdk_example.cpp
    void eventCallback(const std::shared_ptr<XinTan::CBEventData> &event);
    void imgCallback(const std::shared_ptr<XinTan::Frame> &imgframe);
    void init();

    Cld::ConstPtr read() {
      if (!xtsdk) init();
      static Cld::ConstPtr prev_cld = nullptr;
      if (cld == prev_cld) {
        return nullptr;
      }
      prev_cld = cld;
      return cld;
    }
  };

  void LidarReader::Impl::eventCallback(
      const std::shared_ptr<XinTan::CBEventData> &event) {
    Log::debug("event: " + event->eventstr + " "
               + std::to_string(event->cmdid));
    if (event->eventstr == "sdkState") {
      // 端口打开后第一次连接上设备
      if (xtsdk->isconnect() && (event->cmdid == 0xfe)) {
        xtsdk->stop();
        XinTan::RespDevInfo devinfo;
        xtsdk->getDevInfo(devinfo);

        Log::debug(devinfo.fwVersion.c_str());
        Log::debug(devinfo.sn.c_str() + devinfo.chipidStr);
        Log::debug("DEV SN=" + devinfo.sn);

        xtsdk->setModFreq(
            (XinTan::ModulationFreq)params.device.frequency_modulation);
        xtsdk->setHdrMode((XinTan::HDRMode)params.device.HDR);
        xtsdk->setIntTimesus(params.device.intgs, params.device.int1,
                             params.device.int2, params.device.int3);
        xtsdk->setMinAmplitude(params.device.minLSB);
        xtsdk->setMaxFps(params.device.maxfps);
        xtsdk->setCutCorner(params.device.cut_corner);
        xtsdk->start((XinTan::ImageType)params.device.imgType);
      }
      Log::debug("sdkstate= " + xtsdk->getStateStr());
    } else if (event->eventstr == "devState") {
      Log::debug("devstate= " + xtsdk->getStateStr());
    } else {
      if (event->cmdid == XinTan::REPORT_LOG)  // log
      {
        std::string logdata;
        logdata.assign(event->data.begin(), event->data.end());
        Log::debug("log: " + logdata);
      }
      Log::debug("event: " + event->eventstr
                 + " cmd=" + std::to_string(event->cmdid));
    }
  }

  void LidarReader::Impl::imgCallback(
      const std::shared_ptr<XinTan::Frame> &imgframe) {
    if (imgframe->points.empty()) {
      cld.reset();
      return;
    }

    Cld::Ptr ret(new Cld);
    ret->resize(3000);
    ret->header.stamp = imgframe->timeStampS * 1e9 + imgframe->timeStampNS;
    for (size_t i = 0; i < ret->size(); i++) {
      ret->points[i].x = imgframe->points[i].x;
      ret->points[i].y = imgframe->points[i].y;
      ret->points[i].z = imgframe->points[i].z;
      ret->points[i].intensity = imgframe->points[i].intensity;
    }
    cld = ret;
  }

  void LidarReader::Impl::init() {
    xtsdk = std::make_unique<XinTan::XtSdk>();
    auto &dev = params.device;
    auto &flt = params.filter;
    if (XinTan::Utils::isComport(port)) {
      xtsdk->setConnectSerialportName(port);
    }
    xtsdk->setSdkCloudCoordType(
        static_cast<XinTan::ClOUDCOORD_TYPE>(dev.cloud_coord));
    xtsdk->setCallback([this](auto event) { eventCallback(event); },
                       [this](auto frame) { imgCallback(frame); });
    if (flt.edgeEnable) xtsdk->setSdkEdgeFilter(flt.edgeThreshold);
    if (flt.kalmanEnable)  // 卡尔曼滤波
      xtsdk->setSdkKalmanFilter(flt.kalmanFactor * 1000, flt.kalmanThreshold,
                                2000);
    if (flt.medianSize > 0)  // 中值滤波
      xtsdk->setSdkMedianFilter(flt.medianSize);
    if (flt.dustEnable)  // 尘点滤波
      xtsdk->setSdkDustFilter(flt.dustThreshold, flt.dustFrames);
    xtsdk->startup();
  }

  LidarReader::LidarReader(std::string sensor_name)
      : Reader(sensor_name), impl_(std::make_unique<Impl>()) {
    auto &dev = impl_->params.device;
    const auto &dcfg = GlobalParams::get_instance().yml[sensor_name]["device"];
    dev.frequency_modulation = dcfg["frequency_modulation"].get_value<int>();
    dev.HDR = dcfg["HDR"].get_value<int>();
    dev.imgType = dcfg["imgType"].get_value<int>();
    dev.cloud_coord = dcfg["cloud_coord"].get_value<int>();
    dev.int1 = dcfg["int1"].get_value<int>();
    dev.int2 = dcfg["int2"].get_value<int>();
    dev.int3 = dcfg["int3"].get_value<int>();
    dev.intgs = dcfg["intgs"].get_value<int>();
    dev.minLSB = dcfg["minLSB"].get_value<int>();
    dev.cut_corner = dcfg["cut_corner"].get_value<int>();
    dev.start_stream = dcfg["start_stream"].get_value<bool>();
    // dev.connect_address = dcfg["connect_address"].get_value<std::string>();
    dev.maxfps = dcfg["maxfps"].get_value<int>();
    dev.hmirror = dcfg["hmirror"].get_value<bool>();
    dev.vmirror = dcfg["vmirror"].get_value<bool>();

    auto &flt = impl_->params.filter;
    const auto &fcfg = GlobalParams::get_instance().yml[sensor_name]["filter"];
    flt.medianSize = fcfg["medianSize"].get_value<int>();
    flt.kalmanEnable = fcfg["kalmanEnable"].get_value<bool>();
    flt.kalmanFactor = fcfg["kalmanFactor"].get_value<float>();
    flt.kalmanThreshold = fcfg["kalmanThreshold"].get_value<int>();
    flt.edgeEnable = fcfg["edgeEnable"].get_value<bool>();
    flt.edgeThreshold = fcfg["edgeThreshold"].get_value<int>();
    flt.dustEnable = fcfg["dustEnable"].get_value<bool>();
    flt.dustThreshold = fcfg["dustThreshold"].get_value<int>();
    flt.dustFrames = fcfg["dustFrames"].get_value<int>();

    impl_->port = GlobalParams::get_instance()
                      .yml[sensor_name]["port"]
                      .get_value<std::string>();
  }

  LidarReader::~LidarReader() {}

  Data::ConstPtr LidarReader::read() {
    auto cld = impl_->read();
    if (!cld) {
      return nullptr;
    }
    auto msg = package_data(reinterpret_cast<const void *>(cld.get()));
    return std::make_shared<Data>(msg);
  }

  MsgPtr LidarReader::package_data(const void *cld_ptr) {
    auto cld = reinterpret_cast<const Cld *>(cld_ptr);
    auto builder = capnp::MallocMessageBuilder();
    auto msg = builder.initRoot<PointCloud>();
    msg.setTopic(topic_);
    msg.setTimestamp(cld->header.stamp);
    auto points = msg.initPoints(cld->size());
    for (size_t i = 0; i < cld->size(); i++) {
      points[i].setX(cld->points[i].x);
      points[i].setY(cld->points[i].y);
      points[i].setZ(cld->points[i].z);
      points[i].setI(cld->points[i].intensity);
    }
    kj::VectorOutputStream output_stream;
    capnp::writePackedMessage(output_stream, builder);
    auto buffer = output_stream.getArray();
    MsgPtr ret = std::make_shared<Msg>(buffer.size());
    ret->insert(ret->begin(), buffer.begin(), buffer.end());
    return ret;
  }
}  // namespace tskpub