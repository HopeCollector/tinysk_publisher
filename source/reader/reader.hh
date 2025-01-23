#pragma once

#include <functional>
#include <memory>
#include <string>

#include "tskpub/tskpub.hh"

namespace tskpub {
  class Reader {
  public:
    using Ptr = std::shared_ptr<Reader>;
    using ConstPtr = std::shared_ptr<const Reader>;
    virtual ~Reader() {}
    virtual DataConstPtr read() = 0;

  protected:
    Reader() {}
  };

  class StateReader final : public Reader {
  public:
    using Ptr = std::shared_ptr<StateReader>;
    using ConstPtr = std::shared_ptr<const StateReader>;
    static Ptr create(std::string topic);
    virtual ~StateReader();
    DataConstPtr read() override;
    void set_callback(std::function<std::string()> callback) { callback_ = callback; }

  private:
    StateReader() = delete;
    StateReader(StateReader&) = delete;
    StateReader(const StateReader&) = delete;
    StateReader& operator=(StateReader&) = delete;
    StateReader(std::string topic);

    std::string topic_;
    std::function<std::string()> callback_;
  };

  class IMUReader final : public Reader {
  public:
    using Ptr = std::shared_ptr<IMUReader>;
    using ConstPtr = std::shared_ptr<const IMUReader>;
    static Ptr create(std::string topic);
    virtual ~IMUReader();
    DataConstPtr read() override;

  private:
    IMUReader() = delete;
    IMUReader(IMUReader&) = delete;
    IMUReader(const IMUReader&) = delete;
    IMUReader& operator=(IMUReader&) = delete;
    IMUReader(std::string topic);

    std::string topic_;
  };
}  // namespace tskpub