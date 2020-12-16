/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "init/init_test22"
END_TEST_DATA
*/

struct e 
{
	

	void reset()
	{
		
	}
	
	int v = 90;
};


struct i
{
	i(e& o)
	{
		o.v = 18;
	}
};

container::chain<parameter::empty, wrap::init<wrap::event<e>, i>> obj;

int main(int input)
{
	return obj.get<0>().x;
}

