/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "visibility/private_member_from_class"
END_TEST_DATA
*/

class X
{
	int getX()
	{
		return 12;
	}
	
	public:
	
	int getPublic()
	{
		return getX();
	}
};

int main(int input)
{
	X obj;
	
	return obj.getPublic();
}

