/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 40
  error: ""
  filename: "vop/vop_2"
END_TEST_DATA
*/

span<float, 129> data = {20.0f};

dyn<float> d;

float main(float input)
{
	d.referTo(data, data.size());
	
	d *= 2.0f;

	return d[128];
	
}

