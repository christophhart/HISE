/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 27
  error: ""
  filename: "struct/local_struct_ref"
END_TEST_DATA
*/

struct X
{
    int v = 12;
    float d = 12.0f;
};

span<X, 12> data;

int main(int input)
{
	auto x = data[1];
	
	x.d = 15.0f;	
	return x.v + (int)x.d;
}

