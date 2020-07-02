/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: float
  input: 12.5f
  output: 12
  error: ""
  filename: "basic/simple cast"
END_TEST_DATA
*/

int main(float input)
{
	return (int)input;
}

