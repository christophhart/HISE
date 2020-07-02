/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "span/span_size"
END_TEST_DATA
*/

span<int, 5> d;

int main(int input)
{
    return d.size();
}