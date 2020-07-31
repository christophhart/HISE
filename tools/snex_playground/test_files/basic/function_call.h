/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "basic/function_call"
END_TEST_DATA
*/

int other()
{
    return 2;
}

int main(int input)
{
    return other();
}

