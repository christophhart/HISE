/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "visibility/private_member_by_method"
END_TEST_DATA
*/


class X
{
	int x = 9;

public:
	
	int getX() const
	{
		return x;
	}
};


X obj;

int main(int input)
{
	return obj.getX();
}

