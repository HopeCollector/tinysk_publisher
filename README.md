# 🐍Tiny Snake Publisher

本项目为微型生命探测机器人项目的子项目，目的在于驱动传感器并将其通过网络打包发送会远程地面站。项目设计的机器人外观为蛇型机器人，蛇头位置配备摄像头、激光雷达、IMU三种传感器设备。

通信的总体架构为发布者-订阅者架构，机器人为发布者，采集数据后进行发布，地面站是订阅者，负责接收消息。由于项目整体限制传输带宽，因此本项目除 IMU 之外的所有数据都进行了压缩处理。点云进行了一定比例的降采样，视频进行使用硬件加速的 jpeg 编码。

机器人使用的开发板型号为 **RaspberryPi Zero 2W**，使用基于 aarch64(armv8) Debian12(bookwarm) 的树莓派定制操作系统

## 目录结构

```
tinysk_publisher 
|-- all             # 项目完整 CMake 工程的位置，适合 IDE 使用
|-- cmake           # cmake 脚本
|-- configs         # 配置文件位置
|   `-- test          # 测试用配置文件，测试程序会直接读这里的文件
|-- documentation   # doxygen 文档相关项目
|-- drivers         # 传感器设备驱动
|-- include         # 暴露给用户的头文件
|-- messages        # capnp 消息
|-- source          # TSKPub 库的源代码
|   `-- reader        # 读取传感器用的子模块，新增传感器类型的话需要在这里增加新的读取器
|-- standalone      # 可执行文件所在的项目
`-- test            # 测试项目
    |-- integration   # 黑盒测试
    `-- unit          # 白盒测试
```

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
    make install
    ```


## 构建方法

```bash
mkdir -p workspace/build
cd workspace
git clone --depth 1 https://github.com/HopeCollector/tinysk_publisher.git
cmake -S tinysk_publisher/all -B build
cmake --build build --config Release
```

## 测试

测试需要在设备连接好之后进行，在进行测试前请修改 `tinysk_publisher/configs/test` 中的配置文件，保证测试程序能正确找到设备

```bash
cd build
ctest
```

## 运行

程序运行依赖配置文件，使用前请将配置文件拷贝一份到本地

```bash
cp tinysk_publisher/configs/template.yml ./cfg.yml
```

修改 `cfg.yml` 后使用下面的指令运行发布者

```bash
./build/standalone/TSKPubStandalone -c ./cfg.yml
```

## 二次开发

### IDE 使用

建议将 IDE 的根项目文件选为 `all/CMakeLists.txt`，这样 IDE 可以找到所有可编译的对象

### 新增读取器

1. 将新读取器的声明写在 `source/reader/reader.hh`
2. 在 `source/reader` 中新建一个 `.cc` 用于实现新的读取器
3. 在 `source/CMakeLists.txt` 中增加新的源文件。
4. 若需要新增传感器的驱动程序，请将驱动程序打包放入 `drivers` 文件夹中
5. 在 `CMakeLists.txt` 中导入新的驱动程序

## FAQ

### `CMakeLists.txt` 中新增了依赖，为什么编译还是不通过？

使用 `cmake --build build --target TSKPub` 尝试是否能过编译，若不能过编译说明写的有
问题。若编译通过，则需要将新增的依赖在 `test/unit/CMakeLists.txt` 中再添加一次，因为
单元测试相当于把 `TSKPub` 的源代码编译为了新的东西，而不是将其作为依赖，所有依赖都需要
与编译 `TSKPub` 是保持一致
