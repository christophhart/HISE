/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "preprocessor/preprocessor_multiline"
END_TEST_DATA
*/


#define X(name) \
int name() { \
  return 12; \
}

X(test)


int main(int input)
{
    return test();
}
