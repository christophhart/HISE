/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 5
  error: ""
  filename: "basic/return_inc"
END_TEST_DATA
*/

struct Object
{
	
	float x = 5.0f;
	
	float decX()
	{
		auto copy = x;
		
		x += 1.0f;
		
		return copy;
	}
};

Object obj;

int main(int input)
{	
	return (int)obj.decX();
}

