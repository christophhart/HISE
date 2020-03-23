/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 6
  error: ""
  filename: "dsp/interleave_span"
END_TEST_DATA
*/

span<float, 16> c1 = { 2.0f };
int x = 12;
span<float, 16> c2 = { 1.0f };

ChannelData d;

void prepare()
{
	 d[0] = c1;
	 d[1] = c2;

}

int main(int input)
{
	prepare();
	
	auto fd = interleave(d);

	for(auto& f: fd)
	{
		f[0] = 5.0f;
	}

	interleave(fd);
	
	return (int)(c1[0] + c2[0]);
}



//span<dyn<float>, 2>    =>    dyn<span<float, 2>>