@0xb1e0169055193689;

struct Status {
  topic @0 :Text;
  timestamp @1 :UInt64;
  cpuUsage @2 :Float32;
  cpuTemp @3 :Float32;
  memUsage @4 :Float32;
  totalReadBytes @5 :UInt64;
  batteryVoltage @6 :Float32;
  batteryCurrent @7 :Float32;
  ip @8 :Text;
}
