/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 3
  error: ""
  filename: "preprocessor/macro_struct"
END_TEST_DATA
*/

#define VALUE_CLASS(i) struct i { int value = 3; }



VALUE_CLASS(Object);


int main(int input)
{
	Object x;
	
	return x.value;
}

