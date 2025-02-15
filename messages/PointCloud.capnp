@0xb56ab2085c576ce4;

struct Point {
  x @0 :Float32;
  y @1 :Float32;
  z @2 :Float32;
  i @3 :Float32;
}

struct PointCloud{
  topic @0 :Text;
  timestamp @1 :UInt64;
  points @2 :List(Point);
}