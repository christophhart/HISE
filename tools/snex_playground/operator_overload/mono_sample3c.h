/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "operator_overload/mono_sample3c"
END_TEST_DATA
*/

using IndexType = index::unsafe<0>;

struct Obj
{
	Obj()
	{
		s.data[0].referTo(data1, data1.size());
		s.data[1].referTo(data2, data2.size());
		s.rootNote = 12;
	}

	span<float, 12> data1 = { 14.0f};
	span<float, 12> data2 = { 19.0f};

	IndexType x;
	StereoSample s;
	
	span<float, 2> getSample() const
	{
		return s[x];
	}
};


int main(int input)
{
	Obj obj;

	span<float, 2> ld = {1.0f};

	ld += obj.getSample();

	return (int)ld[0];
}

