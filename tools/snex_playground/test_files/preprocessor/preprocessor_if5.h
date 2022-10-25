/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 10
  error: "Line 20(0): missing #endif"
  filename: "preprocessor/preprocessor_if5"
END_TEST_DATA
*/

#if 1


int main(int input)
{
    return 12;
}
