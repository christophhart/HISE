/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 182
  error: ""
  filename: "wrap/wrap_data3"
END_TEST_DATA
*/


/** A dummy object that expects a table. */
struct X
{
	DECLARE_NODE(X);

	template <int P> void setParameter(double v) {}
	
	void setExternalData(const ExternalData& d, int index)
	{
		//Console.print(125);
		d.referBlockTo(f, 0);
	}
	
	block f;
};

struct LookupTableEmpty
{
	span<float, 512> data = { 0.0f };
};

struct LookupTable
{
	span<float, 512> data = { 182.0f };
};

LookupTableEmpty lut;

ExternalData o = {lut};

wrap::data<core::table, data::embedded::table<LookupTable>> mainObject;

span<float, 1> d;

int main(int input)
{
	mainObject.setExternalData(o, 0);
	mainObject.processFrame(d);
	return (int)d[0];
}

