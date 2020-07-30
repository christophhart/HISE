/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 3
  error: ""
  filename: "span/get"
END_TEST_DATA
*/

span<int, 2> data = { 1, 3};

int main(int input)
{
    return data[1];
}

