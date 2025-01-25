#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "reader/reader.hh"

namespace tskpub {
  class TSKPub {
  public:
    TSKPub();
    ~TSKPub();
    Data::ConstPtr read(const std::string& sensor_name) const noexcept;

  private:
    std::unordered_map<std::string, Reader::Ptr> sensor_reader_map_;
  };
}  // namespace tskpub