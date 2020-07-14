#ifndef HI_SCRIPTING_API_CUBE_API
#define HI_SCRIPTING_API_CUBE_API

namespace cube {

struct Orb {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Orbit {
    struct Path {
        struct Keyframe {
            float time = 0;
            float pos = 0;
            bool easeIn = false;
            bool easeOut = false;
        };
        std::vector<Keyframe> keyframes;
    };

    struct Lfo {
        enum WaveType { Sin, Triangle, Saw, Square };
        WaveType waveType = Sin;
        float frequency = 1;
        float phaseOffset = 0;
        float min = -1;
        float max = 1;
    };

    struct Axis {
        enum Type { Path, Lfo };
        Type type = Path;
        struct Path path;
        struct Lfo lfo;
    };

    bool visible = false;
    Axis x;
    Axis y;
    Axis z;
    hise::Vector3D<float> rotation;
    hise::Vector3D<bool> mirror;
    float intensity = 1;
};

using namespace hise;

// The Cube Javascript API.
class CubeApi : public ApiClass
{
public:
    static Orb orb;
    static Orbit orbit;

    CubeApi() : ApiClass(0) {
		ADD_API_METHOD_3(setOrbPosition);
        ADD_API_METHOD_0(showOrbit);
        ADD_API_METHOD_0(hideOrbit);
        ADD_API_METHOD_4(setLfo);
        ADD_API_METHOD_3(setLfoRange);
        ADD_API_METHOD_1(setEmptyPath);
        ADD_API_METHOD_5(addPathKeyframe);
        ADD_API_METHOD_3(setOrbitRotation);
        ADD_API_METHOD_3(setOrbitMirror);
        ADD_API_METHOD_1(setOrbitIntensity);
	}

	struct Wrapper {
		API_VOID_METHOD_WRAPPER_3(CubeApi, setOrbPosition);
        API_VOID_METHOD_WRAPPER_0(CubeApi, showOrbit);
        API_VOID_METHOD_WRAPPER_0(CubeApi, hideOrbit);
        API_VOID_METHOD_WRAPPER_4(CubeApi, setLfo);
        API_VOID_METHOD_WRAPPER_3(CubeApi, setLfoRange);
        API_VOID_METHOD_WRAPPER_1(CubeApi, setEmptyPath);
        API_VOID_METHOD_WRAPPER_5(CubeApi, addPathKeyframe);
        API_VOID_METHOD_WRAPPER_3(CubeApi, setOrbitRotation);
        API_VOID_METHOD_WRAPPER_3(CubeApi, setOrbitMirror);
        API_VOID_METHOD_WRAPPER_1(CubeApi, setOrbitIntensity);
	};

    void setOrbPosition(float x, float y, float z);
    void showOrbit();
    void hideOrbit();
    void setLfo(int axis, String waveType, float frequency, float phaseOffset);
    void setLfoRange(int axis, float min, float max);
    void setEmptyPath(int axis);
    void addPathKeyframe(int axis, float time, float pos, bool easeIn, bool easeOut);
    void setOrbitRotation(float x, float y, float z);
    void setOrbitMirror(bool x, bool y, bool z);
    void setOrbitIntensity(float intensity);

	Identifier getName() const override { RETURN_STATIC_IDENTIFIER("CubeApi"); }
    ~CubeApi() {};

private:
    Orbit::Axis* getAxis(int axis);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CubeApi);
};

}

#endif
