/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 1
  output: 1.0
  error: ""
  filename: "node_library/core_peak"
END_TEST_DATA
*/

using parameter_t = parameter::plain<math::add, 0>;

using mod_t = wrap::mod<parameter_t, core::peak>;

math::add target;

span<float, 1> data = {0.5f};

mod_t source;

float main(int input)
{
	
	
	source.getParameter().connect<0>(target);
	
	
	
	source.processFrame(data);
	
	target.processFrame(data);
	
	return data[0];
}

