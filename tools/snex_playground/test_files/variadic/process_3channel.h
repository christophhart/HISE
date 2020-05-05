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

ProcessData<3> data;

int main(int input)
{
    return data.getNumChannels();
}

