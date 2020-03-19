/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 13
  error: ""
  filename: "struct/span_struct_member_register"
END_TEST_DATA
*/

struct X
{
    int z = 12;
};

span<X, 4> d;

int main(int input)
{
	d[3].z = 13;
	
	return d[3].z;
}

