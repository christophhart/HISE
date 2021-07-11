/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 25
  error: ""
  filename: "variadic/split_test"
END_TEST_DATA
*/

struct Test
{
	DECLARE_NODE(Test);

	template <int P> void setParameter(double v)
	{
		
	}

	void reset()
	{
		
	}
};

container::split<parameter::empty, wrap::fix<1, Test>, Test> s;

int main(int input)
{
	s.reset();
	

	return 25;
}

