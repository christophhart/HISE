/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 80
  error: ""
  filename: "operator_overload/type_cast2"
END_TEST_DATA
*/

struct X
{
	operator int() const
	{
		return v;
	}
		
	int v = 80;
};



int main(int input)
{
	X i;
	
	
	
	
	return (int)i;
}

