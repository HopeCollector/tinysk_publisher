# 🐍Tiny Snake Publisher

本项目为微型生命探测机器人项目的子项目，目的在于驱动传感器并将其通过网络打包发送会远程地面站。项目设计的机器人外观为蛇型机器人，蛇头位置配备摄像头、激光雷达、IMU三种传感器设备。

通信的总体架构为发布者-订阅者架构，机器人为发布者，采集数据后进行发布，地面站是订阅者，负责接收消息。由于项目整体限制传输带宽，因此本项目除 IMU 之外的所有数据都进行了压缩处理。点云进行了一定比例的降采样，视频进行 h264 编码。

机器人使用的开发板型号为 **RaspberryPi Zero 2W**，使用基于 aarch64(armv8) Debian12(bookwarm) 的树莓派定制操作系统

## 目录结构

## 项目依赖

1. zeromq 一个实现多种通信模式的基础库

    ```bash
    # install libsodium
    cd /path/to/build/dir
    curl -OL https://download.libsodium.org/libsodium/releases/libsodium-1.0.20-stable.tar.gz
    tar -zxvf libsodium-1.0.20-stable.tar.gz
    cd libsodium-stable
    ./configure
    make -j10
    make check
    make install

    # install libzmq
    cd /path/to/build/dir
    git clone https://github.com/zeromq/libzmq.git
    cd libzmq
    ./autogen.sh
    ./configure --with-libsodium
    make -j10
    make install
    ldconfig
    ```

2. capnproto 消息序列化库

    ```bash
    cd /path/to/build/dir
    curl -O https://capnproto.org/capnproto-c++-1.1.0.tar.gz
    tar zxf capnproto-c++-1.1.0.tar.gz
    cd capnproto-c++-1.1.0
    ./configure
    make -j6 check
    sudo make install
    ```


## 构建方法
