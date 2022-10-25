/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 91
  error: ""
  filename: "index/index6"
END_TEST_DATA
*/

index::unsafe<0, false> i;

int main(int input)
{
	i = 90;

	return (int)++i;
}

