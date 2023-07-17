/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "operator_overload/mono_sample3b"
END_TEST_DATA
*/

using IndexType = index::unsafe<0>;

struct Obj
{
	

	span<float, 12> data1 = { 14.0f};
	span<float, 12> data2 = { 19.0f};
	StereoSample s;
	
	span<float, 2> getSample() const
	{
		return s[x];
	}
	
	IndexType x;
	
};

Obj obj;


int main(int input)
{
	obj.s.data[0].referTo(obj.data1, obj.data1.size());
	obj.s.data[1].referTo(obj.data2, obj.data2.size());
	obj.s.rootNote = 12;

	span<float, 2> ld = {1.0f};

	ld += obj.getSample();

	return (int)ld[0];
}

