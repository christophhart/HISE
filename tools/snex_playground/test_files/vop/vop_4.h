/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 80
  error: ""
  filename: "vop/vop_4"
END_TEST_DATA
*/

span<float, 128> data = {20.0f};

dyn<float> d;
dyn<float> d2;

int main(int input)
{
  d.referTo(data, data.size());
  d *= 4.0f;
  
  return (int)data[1];
  
  
}


