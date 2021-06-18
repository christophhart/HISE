/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "preprocessor/macro_define_type"
END_TEST_DATA
*/

#define MyType int

int main(MyType input)
{
	
	return input / 2;
}

