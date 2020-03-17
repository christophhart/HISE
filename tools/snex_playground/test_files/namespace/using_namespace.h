/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 13
  error: ""
  filename: "namespace/using_namespace"
END_TEST_DATA
*/


namespace N1
{
    using Type = int;
}

namespace N2
{
	using namespace N1;
	
    Type x = 13;
}



int main(int input)
{
    using namespace N2;
	return x;
}

