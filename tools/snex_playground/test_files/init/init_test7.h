/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "init/init_test7"
END_TEST_DATA
*/


namespace X
{

	struct Inner
	{
		Inner(int z)
		{
			value = z;
		}
	
	
		int value = 0;
	};
}

X::Inner obj = { 2 };


int main(int input)
{
	return obj.value;
}

