/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: float
  input: 12.5f
  output: 4
  error: ""
  filename: "span/set"
END_TEST_DATA
*/

span<int, 5> data;

void setValue()
{
    data[2] = 4;
}


int main(float input)
{
    setValue();
	
    return data[2];
}

