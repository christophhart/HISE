/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 23
  error: ""
  filename: "namespace/struct_with_same_name2"
END_TEST_DATA
*/


namespace N1
{
    struct X
    {
        int x = 15;  
    };
}

namespace N2
{
    struct X
    {
        int x = 8;
    };
}

using namespace N1;

X x1;
N2::X x2;


int main(int input)
{
	return x1.x + x2.x;
}

