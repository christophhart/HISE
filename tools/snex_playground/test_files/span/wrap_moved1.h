/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "span/wrap_moved1"
END_TEST_DATA
*/

span<int, 8>::wrapped i = { 5 };

int main(int input)
{
    return i.moved(i);
}

