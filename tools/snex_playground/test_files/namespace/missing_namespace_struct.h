/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: "Line 23: Can't parse statement"
  filename: "namespace/missing_namespace_struct"
END_TEST_DATA
*/


namespace Objects
{
    struct MyObject
    {
        int x = 5;
    };
}


MyObject s;

int main(int input)
{
	return s.x;
}

