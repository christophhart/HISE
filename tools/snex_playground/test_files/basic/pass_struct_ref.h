/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 5
  error: ""
  filename: "basic/pass_struct_ref"
END_TEST_DATA
*/

void init(OscProcessData& d)
{
	d.uptime = 5.0;
}



int main(int input)
{
	OscProcessData f;
	
	init(f);
	
	return (int)f.uptime;
}

