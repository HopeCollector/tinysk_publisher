@0xf5af2cc3c1f08880;

struct Image {
  topic @0 :Text;
  timestamp @1 :UInt64;
  width @2 :UInt32;
  height @3 :UInt32;
  encoding @4 :Text;
  fps @5 :Float32;
  data @6 :List(UInt8);
}