/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "operator_overload/mono_sample3"
END_TEST_DATA
*/


span<float, 12> data1 = { 14.0f};
span<float, 12> data2 = { 19.0f};
StereoSample s;

using IndexType = index::unsafe<0>;

IndexType x;

span<float, 2> getSample()
{
	return s[x];
}

int main(int input)
{
	s.data[0].referTo(data1, data1.size());
	s.data[1].referTo(data2, data2.size());
	s.rootNote = 12;

	span<float, 2> ld = {1.0f};

	ld += getSample();

	return (int)ld[0];
}

