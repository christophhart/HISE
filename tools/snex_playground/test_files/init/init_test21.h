/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 18
  error: ""
  filename: "init/init_test21"
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

wrap::init<wrap::event<e>, i> obj;

int main(int input)
{
	return obj.getObject().v;
}

