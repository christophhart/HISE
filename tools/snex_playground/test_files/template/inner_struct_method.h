/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 13
  error: ""
  filename: "template/inner_struct_method"
END_TEST_DATA
*/

template <int NV> struct Outer
{
	struct Inner
	{
		float tick()
		{
			return 13.0f;
		}
	};
	
	float tick()
	{
		return obj.tick();
	}
	
	Inner obj;
};

int main(int input)
{
	Outer<1> x;
	
	return (int)x.tick();
}

