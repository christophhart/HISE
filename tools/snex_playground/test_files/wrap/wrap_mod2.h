/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "wrap/wrap_mod2"
END_TEST_DATA
*/

using mod_type = wrap::mod<parameter::plain<math::add, 0>, core::peak>;

mod_type peak;
math::add adder;

span<float, 1> data = { 124.0f };

int main(int input)
{
	peak.getParameter().connect<0>(adder);

	peak.processFrame(data);
	
	return 12;
}

