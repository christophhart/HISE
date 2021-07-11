/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "struct/inner_struct"
END_TEST_DATA
*/

struct X
{
    struct Y
    {
        int z = 5;
    };
};

X::Y obj;

int main(int input)
{
	return obj.z;
}

