

class MoogFilter
{
public:

	float frequency = 0.4f;
	float resonance = 0.9f;

	void calcCoefficients()
	{
		q = 1.0f - thisFreq;
  		p = thisFreq + 0.8f * thisFreq * q;
  		f = p + p - 1.0f;
  		q = thisRes * (1.0f + 0.5f * q * (1.0f - q + 5.6f * q * q));
	};

	void init()
	{
		calcCoefficients();
	};

	void updateCoefficients()
	{
		thisFreq = lastFreq * a + frequency * invA;
		thisRes = lastRes * a + resonance * invA;
		
		lastRes = thisRes;
		lastFreq = thisFreq;
		
		calcCoefficients();
	};
	
	void prepareToPlay(double sampleRate, int bs) {};

	float process(float input)
	{
		const float in = input - q * b4;                          
  		const float t1 = b1;  
		b1 = (in + b0) * p - b1 * f;
  		const float t2 = b2;
		b2 = (b1 + t1) * p - b2 * f;
  		t1 = b3;  
		b3 = (b2 + t2) * p - b3 * f;
        b4 = (b3 + t1) * p - b4 * f;
  		b4 = b4 - b4 * b4 * b4 * 0.166667f;    //clipping
  		b0 = in;

		//LP
		return b4;

		//HP
		//return in - b4;

		//BP
		 //return 3.0f * (b3 - b4) ; 

	};

private:
	
	const float a = 0.9f;
	const float invA = 0.1f;
	
	float thisFreq;
	float thisRes;
	
	float lastFreq;
	float lastRes;
	
	float f, p, q;             //filter coefficients
  	float b0, b1, b2, b3, b4;  //filter buffers (beware denormals!)
  	float t1;              //temporary buffers

};
