/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "namespace/static_struct_member"
END_TEST_DATA
*/


namespace N1
{
    struct X
    {
        static const int v = 12;
    };
}


int main(int input)
{
	return N1::X::v;
}

