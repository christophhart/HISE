/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 19
  error: ""
  filename: "ra_pass/skip_float"
END_TEST_DATA
*/



int main(int input)
{

	float x = 12.0f;
	x = 19.0f;
	
	return (int)x;
}

