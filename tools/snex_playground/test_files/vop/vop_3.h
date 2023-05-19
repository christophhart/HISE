/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 60
  error: ""
  filename: "vop/vop_3"
END_TEST_DATA
*/

span<float, 128> data = {20.0f};

dyn<float> d;

span<float, 128> data2 = {3.0f};

dyn<float> d2;

float main(float input)
{
	d.referTo(data, 64);
	d2.referTo(data2, 64);
	
	d *= d2;

	return d[32];
	
}

