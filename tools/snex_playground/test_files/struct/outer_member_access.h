/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: "Line 21(11): Can't access outer member from inner class"
  filename: "struct/outer_member_access"
END_TEST_DATA
*/

struct Outer
{
	struct Inner
	{
		int x = 9;
		
		int getV()
		{
			return v;
		}
	};
	
	Inner data;
	
	int v = 12;
};

int main(int input)
{
	Outer obj;
	return obj.data.getV();
}

