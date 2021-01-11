/*
BEGIN_TEST_DATA
  f: {processor}
  ret: int
  args: int
  input: "zero.wav"
  output: "node/node_frame_osc.wav"
  error: ""
  events: [{"Type": "NoteOn", "Channel": 1, "Value1": 64, "Value2": 64, "Timestamp": 256}]
  filename: "node/node_frame_osc"
END_TEST_DATA
*/

using ctype = container::chain<parameter::empty, core::oscillator>;

using processor = container::chain<parameter::empty, ctype, core::oscillator>;

