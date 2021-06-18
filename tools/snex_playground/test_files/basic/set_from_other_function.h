/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "basic/set_from_other_function"
END_TEST_DATA
*/

span<int, 2> d = { 1, 5 };
int x = 9;

int main(int input)
{
    return d[0] + d[1];
}

