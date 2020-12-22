/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 196
  error: ""
  filename: "wrap/get_self_object"
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
		d.value = 90;
	}
};

using C1Type = container::chain<parameter::empty, wrap::fix<1, MyClass>>;

using C2Type = container::chain<parameter::empty, wrap::fix<1, wrap::event<MyClass>>>;

using Wrapped = wrap::fix<2, wrap::init<MyClass, Initialiser>>;

using C3Type = container::chain<parameter::empty, Wrapped>;

C1Type c1;
C2Type c2;

Wrapped w;

C3Type c3;

using C4Type = container::chain<parameter::empty, wrap::fix<2, wrap::event<Wrapped>>>;

C4Type c4;

int main(int input)
{
	auto& x = c1.get<0>(); // MyClass
	auto& y = c2.get<0>(); // MyClass
	auto& z = w.getObject(); // wrap::init<MyClass, Initialiser>;
	auto& z2 = w.getWrappedObject(); // MyClass
	auto& t1 = c3.get<0>(); // wrap::init<MyClass, Initialiser>;
	auto& t2 = t1.getObject(); // wrap::init<MyClass, Initialiser>;
	auto& t3 = t2.getWrappedObject(); // MyClass
	auto& t4 = c4.get<0>().getWrappedObject();
	
	return x.value + y.value + t3.value + t4.value;
}

