/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: int
  input: 12
  output: 1.4
  error: ""
  filename: "inheritance/inheritance_11"
END_TEST_DATA
*/

using instance = wrap::frame<2, wrap::fix<2, core::peak>>;

instance obj;
PrepareSpecs ps;

float main(int input)
{
	
	ps.blockSize = 1;

	obj.prepare(ps);

	return 0.0f;
}
