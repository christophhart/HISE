/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 12
  error: ""
  filename: "complex/additive_synth1"
END_TEST_DATA
*/

class AdditiveSynth
{
public:

	// This function will be called once per sample
	void tick(float input)
	{
		for(auto& g: phaseBlock)
			input += phaseBlock[phaseIndex];
	}
	
	block phaseBlock;
};

index::unsafe<0> phaseIndex;
AdditiveSynth obj;
span<float, 16> b2;

int main(int input)
{

	obj.phaseBlock.referTo(b2);
	obj.tick(1.0f);

	return 12;
}

