/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 0
  error: ""
  filename: "polydata/polydata_1"
END_TEST_DATA
*/

PolyData<int, 18> data;

int main(int input)
{
	return data.get();
}

void prepare(PrepareSpecs ps)
{
	data.prepare(ps);
}