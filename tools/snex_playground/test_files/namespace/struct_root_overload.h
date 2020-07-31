/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 50
  error: ""
  filename: "namespace/struct_root_overload"
END_TEST_DATA
*/


namespace N1
{
    struct X
    {
        int x = 20;  
    };
}

using namespace N1;

struct X
{
    int x = 30;
};

X x1;
N1::X x2;


int main(int input)
{
	return x1.x + x2.x;
}

