/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "span/wrapped_initialiser"
END_TEST_DATA
*/



int main(int input)
{
	index::wrapped<5> s = { 7 };
	
	return (int)s;
}

