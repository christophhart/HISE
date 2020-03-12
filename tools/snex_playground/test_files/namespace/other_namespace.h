/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 31
  error: ""
  filename: "namespace/other_namespace"
END_TEST_DATA
*/


namespace N1
{
    using Type = int;
}

namespace N2
{
    N1::Type x = 12;
    using OtherType = N1::Type;
}

N2::OtherType x = 19;

int main(int input)
{
	return N2::x + x;
}

