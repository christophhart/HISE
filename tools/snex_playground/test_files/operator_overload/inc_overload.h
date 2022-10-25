/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "operator_overload/inc_overload"
END_TEST_DATA
*/


template <int UpperLimit> struct Counter
{
	operator bool() const
	{
		return value < UpperLimit;
	}
	
	auto& operator++()
	{
		++value;
		
		return *this;
	}
	
private:
	int value = 0;
};

int main(int input)
{
	Counter<125> c;
	
	while((bool)c)
	{
		++c;
	}
	

	return 15;
}

