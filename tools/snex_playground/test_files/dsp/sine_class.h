/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "dsp/sine_class"
END_TEST_DATA
*/

struct SineOscillator
{
	void process()
	{
		for(auto& s: data.data)
		{
			s += Math.sin((float)data.uptime);
			data.uptime += data.delta;
		}
	}

	void init(block b)
	{
		data.data = b;
	}
	
private:
	OscProcessData data;
};

SineOscillator d;
span<float, 512> signal;

int main(int input)
{
	block tmp = signal;
	
	d.init(tmp);
	
	return 12;
}

