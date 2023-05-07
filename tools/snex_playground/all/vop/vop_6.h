/*
BEGIN_TEST_DATA
  f: main
  ret: float
  args: float
  input: 12
  output: 12
  error: ""
  filename: "vop/vop_6"
END_TEST_DATA
*/

span<float, 16> data = {1.0f, 2.0f, 3.0f, 4.0f,
                        5.0f, 6.0f, 7.0f, 8.0f,
                        9.0f, 10.0f, 11.0f, 12.0f,
                        13.0f, 14.0f, 15.0f, 16.0f };




float main(float input)
{
	block d;
  block d2;

  	d.referTo(data, 8);
  	d2.referTo(data, 8, 8);
  	d += d2;
  
  	return d[1];
  
}


