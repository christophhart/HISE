/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 0
  output: 2.0
  error: ""
  filename: "basic/if_false"
END_TEST_DATA
*/

float value = 2.0f;

float main(int input)
{
	if (input == 0)
	{
		return value;
	}
	else
	{
		return value;
	}
}

