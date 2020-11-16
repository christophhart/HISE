/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 9
  error: ""
  filename: "basic/function_return_ref"
END_TEST_DATA
*/



struct X
{
    span<int, 2> data = { 1, 2 };
    
    int& getData()
    {
        return data[0];
    }
};


X obj;

int main(int input)
{
	auto& x = obj.getData();
	x = 9;
	return obj.data[0];
}

