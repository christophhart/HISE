/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "vop/vop_1"
END_TEST_DATA
*/

span<float, 128> data = {0.0f};

dyn<float> d;

int main(int input)
{
	d.referTo(data, data.size());
	
  
	d *= 2.0f;

	return 12;
	
}

