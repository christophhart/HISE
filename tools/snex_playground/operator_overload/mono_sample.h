/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 14
  error: ""
  filename: "operator_overload/mono_sample"
END_TEST_DATA
*/


span<float, 12> data1 = { 14.0f};
span<float, 12> data2 = { 19.0f};
StereoSample s;
index::unsafe<0> x;

int main(int input)
{
	s.data[0].referTo(data1, data1.size());
	s.data[1].referTo(data2, data2.size());
	s.rootNote = 12;

  	auto ml = s[1];

	return (int)ml[0];

	//return (int)s.data[idx][0];
}

