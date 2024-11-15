/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "struct/inner_getter"
END_TEST_DATA
*/

struct Outer
{
	struct Inner
	{
		int getX()
		{
			return x;
		}
		
		int x = 12;
	};
	
	int getFromInner()
	{
		return data.getX();
	}
	
	Inner data;
};

int main(int input)
{
	Outer obj;
	
	return obj.getFromInner();
}

