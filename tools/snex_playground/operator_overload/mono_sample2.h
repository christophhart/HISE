/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "operator_overload/mono_sample2"
END_TEST_DATA
*/


span<float, 12> data1 = { 14.0f};
span<float, 12> data2 = { 19.0f};
StereoSample s;
index::unsafe<0> x;

int main(int input)
{
	s.data[0].referTo(data1, data1.size());
	s.data[1].referTo(data2, data1.size());
	

	span<float, 2> ld = {1.0f};

	ld += s[1];

	return (int)ld[0];
}

