/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/global_variable"
END_TEST_DATA
*/

int x = 9;

int main(int input)
{
    return x;
}

