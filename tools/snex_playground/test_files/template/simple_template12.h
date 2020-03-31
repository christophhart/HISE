/*
BEGIN_TEST_DATA
  f: main
  ret: int
  args: int
  input: 12
  output: 2
  error: ""
  filename: "template/simple_template12"
END_TEST_DATA
*/


template <typename T, int NumElements> struct PolyData
{
    void setVoiceIndex(int newVoiceIndex)
    {
        index = newVoiceIndex;
    }
    
    span<T, NumElements>::wrapped index;
};

PolyData<int, 256> d;

int main(int input)
{
	d.setVoiceIndex(258);
	
	return d.index;
}

