/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "variadic/process_3channel"
END_TEST_DATA
*/

span<float, 48> s = { 1.0f };

ProcessData<3> d;

int main(int input)
{
    return d.data.size();
}

