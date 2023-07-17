/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: double
  input: 12
  output: 2
  error: ""
  filename: "index/index_constructor_with_cast"
END_TEST_DATA
*/



int main(double input)
{
	index::wrapped<5> idx((int)input);
	
	return (int)idx;
}

