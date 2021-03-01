/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 72
  error: ""
  filename: "loop/for_loop2"
END_TEST_DATA
*/


span<span<int, 4>, 2> data = { {1,2,3,4}, {5,6,7,8}};

index::unsafe<0> idx;

int main(int input)
{
    int sum = 0;

    for(int i = 0; i < data.size(); i++)
    {
	    for(int j = 0; j < data[i].size(); j++)
	    {
		    sum += data[i][j];
	    }
    }

	for(int i = 0; i < data.size(); i++)
	{
		for(int j = 0; j < data[i].size(); j++)
		{
			sum += data[i][j];
		}
	}

    return sum;
}

