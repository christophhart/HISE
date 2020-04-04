/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 125
  error: ""
  filename: "preprocessor/preprocessor_constant"
END_TEST_DATA
*/

#define MY_CONSTANT 125

int main(int input)
{
	return MY_CONSTANT;
}

