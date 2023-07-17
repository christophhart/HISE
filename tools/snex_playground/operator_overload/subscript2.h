/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 14
  error: ""
  filename: "operator_overload/subscript2"
END_TEST_DATA
*/

struct X
{
	int operator[](int i)
	{
		if(i == 0)
			return v;
		else
			return x;
	}
	
private:
	
	int v = 14;
	int x = 42;
};

int main(int input)
{
	X obj;
	
	return obj[input];
}

