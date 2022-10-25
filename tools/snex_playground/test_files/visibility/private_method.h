/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 27(17): private function X::getX is not accessible"
  filename: "visibility/private_method"
END_TEST_DATA
*/


class X
{
	int getX() const
	{
		return 12;
	}
};


X obj;

int main(int input)
{
	return obj.getX();
}

