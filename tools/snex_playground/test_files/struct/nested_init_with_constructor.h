/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 3
  output: 31
  error: ""
  filename: "struct/nested_init_with_constructor"
END_TEST_DATA
*/


struct MyFirstStruct
{
	struct InnerStruct
	{
		int i1 = 0;
		int i2 = 1;
	};

	MyFirstStruct(int initValue)
	{
		is1.i1 = initValue * 6;
		is2.i2 = initValue * 4;
	}
	
	int get() const
	{
		return is1.i1 + is1.i2 + is2.i2 + is2.i1;
	}

	InnerStruct is1;
	InnerStruct is2;	

}; 


int main(int input)
{
	MyFirstStruct localObject = { 3 };	
	
	return localObject.get();
}

