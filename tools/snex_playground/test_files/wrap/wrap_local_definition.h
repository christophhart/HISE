/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "wrap/wrap_local_definition"
END_TEST_DATA
*/

int main(int input)
{
	wrap<5> d = input;
	return d;
}

