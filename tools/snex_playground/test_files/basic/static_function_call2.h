/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 8
  error: ""
  filename: "basic/static_function_call2"
END_TEST_DATA
*/



span<int, 2> data = { 4 };

static void set(span<int, 2>& d)
  {
    d[0] = 8;
  }

struct X
{
	
};

int main(int input)
{
	set(data);
	return data[0];
}

