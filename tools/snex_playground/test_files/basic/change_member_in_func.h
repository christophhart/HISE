/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "basic/change_member_in_func"
END_TEST_DATA
*/

struct MyData
{
	int v2 = 18;
	int v1 = 90;
};

void changeData(MyData& d)
{
	d.v1 = 12;
}

int main(int input)
{
	MyData data;

	if(input == 12)
	{
		changeData(data);
		return data.v1;
	}
	
	return 9;
}

