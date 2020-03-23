/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 4
  error: ""
  filename: "span/wrap_moved3"
END_TEST_DATA
*/

span<int, 4> d = { 1, 2, 3, 4 };

span<int, 4>::wrapped i;

int main(int input)
{
    return d[i.moved(-1)];
}

