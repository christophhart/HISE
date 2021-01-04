/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "node/node_frame_osc.wav"
  error: ""
  filename: "node/node_frame_osc"
END_TEST_DATA
*/



using processor_ = container::chain<parameter::empty, core::oscillator, core::oscillator, core::oscillator>;

using processor = wrap::frame<1, processor_>;


