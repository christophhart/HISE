#ifndef __mdaLimiter_H
#define __mdaLimiter_H



class mdaLimiter : public MdaEffect
{
public:

	static String getClassName() {	return "Limiter";	};

	mdaLimiter();
	~mdaLimiter();

	String getEffectName() const override;

	void process(float **inputs, float **outputs, int sampleFrames) override;
	void processReplacing(float **inputs, float **outputs, int sampleFrames) override;
	void setParameter(int index, float value) override;
	float getParameter(int index) const override;
	void getParameterLabel(int index, char *label) const override;
	void getParameterDisplay(int index, char *label) const override;
	void getParameterName(int index, char *name) const override;

	int getNumParameters() const override { return 5; };

	
private:

	float fParam1;
	float fParam2;
	float fParam3;
	float fParam4;
	float fParam5;
	float thresh, gain, att, rel, trim;
};

#endif
