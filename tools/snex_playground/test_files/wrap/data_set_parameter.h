/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 11
  error: ""
  filename: "wrap/data_set_parameter"
END_TEST_DATA
*/

struct MyClass
{
	DECLARE_NODE(MyClass);

	template <int P> void setParameter(double d)
	{
		value = (int)d;
	}
	

	int value = 8;
};


struct Initialiser
{
	Initialiser(MyClass& d)
	{
		
	}
};


using Wrapped = wrap::init<MyClass, Initialiser>;

using ParameterType = parameter::plain<Wrapped, 0>;

//using CType = container::chain<parameter::plain<MyClass, 0>, MyClass>;




int main(int input)
{
	//CType c;
	
	
	Wrapped obj;
	
	ParameterType p;
	
	p.connect<0>(obj);
	
	obj.setParameter<0>(9.0);
	
	int v1 = obj.getWrappedObject().value;
	
	p.call(2.0);
	
	int v2 = obj.getWrappedObject().value;
	
	return v1 + v2;
}

