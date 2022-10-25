/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 24
  error: ""
  filename: "preprocessor/preprocessor_sum"
END_TEST_DATA
*/

#define SUM(a, b) a + b

int main(int input)
{
	return SUM(input, 12);
}

