#ifndef HI_SCRIPTING_API_CUBE_API
#define HI_SCRIPTING_API_CUBE_API

namespace cube {

using namespace hise;

// The Cube Javascript API.
class CubeApi : public ApiClass
{
public:
    struct Orb {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };
    static Orb orb;

    CubeApi() : ApiClass(0) {
		ADD_API_METHOD_3(setOrbPosition);
	}

	struct Wrapper {
		API_METHOD_WRAPPER_3(CubeApi, setOrbPosition);
	};

    int setOrbPosition(float x, float y, float z);

	Identifier getName() const override { RETURN_STATIC_IDENTIFIER("CubeApi"); }
    ~CubeApi() {};

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CubeApi);
};

}

#endif
