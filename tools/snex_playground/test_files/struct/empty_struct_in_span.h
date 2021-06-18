/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 24
  error: ""
  filename: "struct/empty_struct_in_span"
END_TEST_DATA
*/

struct X
{
	int multiply(int input)
	{
		return input + 3;
	}
};

span<X, 8> data;

int main(int input)
{
	for(auto& x: data)
	    input = x.multiply(input);
	    
	return input;
}

