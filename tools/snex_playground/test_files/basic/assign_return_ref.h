/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 36
  error: ""
  filename: "basic/assign_return_ref"
END_TEST_DATA
*/

float x = 90.0f;

PolyData<double, 8> pd;

float& getX()
{
	return x;
}

int main(int input)
{
	getX() = 24.0f;

  pd.get() = input;

  auto y = (int)pd.get();	

	return (int)getX() + y;
	
}

