/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 90
  error: ""
  filename: "operator_overload/subscript1"
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
	
	int v = 0;
	int x = 90;
};

int main(int input)
{
	X obj;
	
	return obj[input];
}

