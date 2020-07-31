/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 15
  error: ""
  filename: "struct/double_struct_member"
END_TEST_DATA
*/

struct X
{
    struct Y
    {
        int z = 12;
    };
    
    Y y;
};

X x;

int main(int input)
{
	x.y.z = 16;
	x.y.z = 15;
	
	return x.y.z;
}

