@0x8d9f5c49dadaf089;

struct Vector3 {
  x @0 :Float32;
  y @1 :Float32;
  z @2 :Float32;
}

struct Orientation {
  w @0 :Float32;
  x @1 :Float32;
  y @2 :Float32;
  z @3 :Float32;
}

struct Imu {
  topic @0 :Text;
  timestamp @1 :UInt64;
  orientation @2 :Orientation;
  angularVelocity @3 :Vector3;
  linearAcceleration @4 :Vector3;
}