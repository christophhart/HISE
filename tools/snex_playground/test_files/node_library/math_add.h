/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 0.5
  output: 0.5
  error: ""
  filename: "node_library/math_add"
END_TEST_DATA
*/

float main(int input)
{
	math::add a;
	
	a.setParameter<0>(0.5);
	
	span<float, 1> data = {0.0f};
	
	a.processFrame(data);
	
	return data[0];
}

