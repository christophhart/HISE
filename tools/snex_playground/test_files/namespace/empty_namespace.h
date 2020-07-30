/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "namespace/empty_namespace"
END_TEST_DATA
*/

namespace Empty
{
    namespace Sub
    {
        
    }
}

int z = 12;

int main(int input)
{
	return z;
}

