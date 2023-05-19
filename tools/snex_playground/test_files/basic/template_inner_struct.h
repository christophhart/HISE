/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/template_inner_struct"
END_TEST_DATA
*/



template <int NV> struct Outer
{
	struct Inner
	{
		double uptime = 0.0;
		double delta = 0.0;
	};
	
	PolyData<Inner, NV> states;
};

Outer<18> instance;

int main(int input)
{
	return 12;
}

