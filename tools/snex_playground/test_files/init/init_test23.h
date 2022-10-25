/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 126
  error: ""
  filename: "init/init_test23"
END_TEST_DATA
*/

struct MyObject
{
	MyObject(int initValue)
	{
		v = initValue;
	}
	
	int v = 40;
};




int main(int input)
{
	auto o3 = MyObject(90);
		
	
	MyObject o2(input);
	
	
	MyObject o1 = { input * 2};
	
	return o3.v + o2.v + o1.v;
}

