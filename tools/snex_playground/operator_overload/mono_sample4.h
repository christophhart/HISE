/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 1
  error: ""
  filename: "operator_overload/mono_sample4"
END_TEST_DATA
*/


span<float, 4> data1 = { 14.0f};
span<float, 4> data2 = { 19.0f};
StereoSample s;

using IndexType = index::unsafe<0>;

IndexType getIndex(int i)
{
	IndexType x(i);
	return x;
}

span<float, 2> getSample()
{
	return s[getIndex(2)];
}

int main(int input)
{
	s.data[0].referTo(data1, data1.size());
	s.data[1].referTo(data2, data2.size());
	
	data1[2] = 0.0f;
	
	s.rootNote = 12;

	span<float, 2> ld = {1.0f};

	ld += getSample();

	return (int)ld[0];
}

