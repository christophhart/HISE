/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 80
  error: ""
  filename: "vop/vop_5"
END_TEST_DATA
*/

span<float, 128> data = {20.0f};

dyn<float> d;



float main(float input)
{
  d.referTo(data, 64, 3);
  d *= 4.0f;
  
  return data[8];
  
}


