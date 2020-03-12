/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "namespace/namespace_struct"
END_TEST_DATA
*/


namespace Objects
{
    struct MyObject
    {
        int x = 5;
    };
}


Objects::MyObject s;

int main(int input)
{
	return s.x;
}

