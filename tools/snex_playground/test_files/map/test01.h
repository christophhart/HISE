/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "map/test01"
END_TEST_DATA
*/

struct ExternalFunctionMap
{
	int value = 9000;
};

struct ExternalFunction
{
	dyn<ExternalFunctionMap> m;

	void setMap(dyn<ExternalFunctionMap> map)
	{
		m.referTo(map, 0, 1);
	}
	
	
};

struct Base
{
	

	int value = 90;
	int value2 = 100;
};

span<ExternalFunctionMap, 1> map;
dyn<ExternalFunctionMap> m;

struct Derived: public Base
{
	ExternalFunction f1;
	
	void init(dyn<ExternalFunctionMap> m)
	{
		f1.setMap(m);
	}

	void ping0(int input)
	{
		value = input;
	}
};

int main(int input)
{
	m.referTo(map, 0, 1);

	
	
	Derived obj;
	
	obj.init(m);
	
	obj.ping0(input);

	return obj.value + obj.value2;
}

