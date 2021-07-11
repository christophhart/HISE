/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 0
  output: 28
  error: ""
  filename: "loop/for_loop1"
END_TEST_DATA
*/


span<int, 7> data = { 1, 2, 3, 4, 5, 6, 7};

index::unsafe<0> idx;

int main(int input)
{
    int sum = 0;

    for(int i = 0; i < data.size(); i++)
    {
        sum += data[i];
    }

    return sum;
}

