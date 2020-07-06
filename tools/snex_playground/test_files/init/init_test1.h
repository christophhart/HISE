/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "init/init_test1"
END_TEST_DATA
*/


struct X
{
	void processData(ProcessData<2>& d) {};
	void processFrame(span<float, 2>& d) {};
	void reset() {}
	void prepare(PrepareSpecs ps) {}
	void handleEvent(HiseEvent& e) {};
	
	double d1 = 6.0;
	double d2 = 0.0;
};

struct XInit
{
	void init(X& x)
	{
		x.d1 = 5.0;
	}
};

int main(int input)
{
	wrap::init<X> d;

	return (int)d.obj.d1;
}

