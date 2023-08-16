/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "basic/mod_value"
END_TEST_DATA
*/

template <int NV> struct MyClass
{	
	ModValue mv;
	
	int handleModulation(double& v)
	{
		return mv.getChangedValue(v);
	}
};


double d = 1.0;

int main(int input)
{
	MyClass<1> x;

	x.mv.setModValue(0.5);
	
	return x.handleModulation(d) * (int)((double)input * d);
}

