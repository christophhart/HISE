

#include <JuceHeader.h>




#if JUCE_WINDOWS
/** \internal */
#define DLL_EXPORT extern "C" __declspec(dllexport)
#else
/** \internal */
#define DLL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#if JUCE_DEBUG
#include "include/onnxruntime_cxx_api.h"
#else
#include "include_rel/onnxruntime_cxx_api.h"
#endif

using namespace juce;

struct ONNXRuntime: public ReferenceCountedObject
{
    static void testImage(const Image& test, bool invertAxis, const String& m)
	{
		Image::BitmapData cd(test, 0, 0, test.getWidth(), test.getHeight());

		String t;
		t << m << ": ";
		int i = 0;

		if(invertAxis)
		{
			for(int x = 0; x < cd.width; x++)
	        {
	            auto c = cd.getPixelColour(x, cd.height - 1);
	            
	            auto r = (int)c.getRed();
	            auto g = (int)c.getGreen();
	            auto b = (int)c.getBlue();
	            auto a = c.getAlpha();

				t << "p[" << String(i++) << "]: " << String(r) << ", ";

				if(i >= 5)
				{
					DBG(t);
					return;
				}
					
	            int funky = 5;
	        }
		}
		else
		{
			for(int y = 0; y < cd.height; y++)
	        {
	            auto c = cd.getPixelColour(0, y);
	            
	            auto r = (int)c.getRed();
	            auto g = (int)c.getGreen();
	            auto b = (int)c.getBlue();
	            auto a = c.getAlpha();

				t << "p[" << String(i++) << "]: " << String(r) << ", ";

				if(i >= 5)
				{
					DBG(t);
					return;
				}
					
	            int funky = 5;
	        }
		}
	}

	using Ptr = ReferenceCountedObjectPtr<ONNXRuntime>;

	ONNXRuntime();;

	~ONNXRuntime();

	/** Load a ONNX runtime model from a binary data blob. */
	Result load(const MemoryBlock& mb);

	Result load(const File& f);

	/** Run inference on the image. outputValues must be have the size of float parameters for the output. The inputFormat will be used
	 *  to convert the image before passing it into the neural network.
	 */
	Result run(const juce::Image& img, std::vector<float>& outputValues, bool isGreyScale);

private:

	struct Pimpl;

	Pimpl* pimpl = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ONNXRuntime);
};

struct ONNXRuntime::Pimpl
{
	Pimpl():
      env(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntime"),
	  memory_info(Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU))
	{}

	Result load(const MemoryBlock& mb)
	{
		Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

		try
		{
			ScopedLock sl(lock);
			session.reset(new Ort::Session(env, mb.getData(), mb.getSize(), session_options));
			return Result::ok();
		}
		catch(std::exception& e)
		{
			return Result::fail(e.what());
		}
	}
	
	Result run(const Image& image, std::vector<float>& outputData, bool isGreyscale)
	{
		ScopedLock sl(lock);

		{
			const char* const input = "input";
			const char* const output = "output";

	        std::vector<int64_t> input_shape = 
			{
				1,
				isGreyscale ? 1 : 3,
				static_cast<int64_t>(image.getHeight()),
				static_cast<int64_t>(image.getWidth())
			};

	        std::vector<float> input_data(input_shape[1] * input_shape[2] * input_shape[3], 0.0f); 
	        
	        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
	            memory_info, input_data.data(), input_data.size(), input_shape.data(), input_shape.size());

			Image::BitmapData bp(image, 0, 0, image.getWidth(), image.getHeight());

			auto v = input_data.data();

			if(!isGreyscale)
			{
				return Result::fail("Colour images are not supported");
			}
			else
			{
				for(int y = 0; y < bp.height; y++)
				{
					for(int x = 0; x < bp.width; x++)
					{
                        auto c = bp.getPixelColour(x, y);
						*v++ = c.getBrightness();
					}
				}
			}

			try
			{
				// Run inference
		        auto output_tensors = session->Run(Ort::RunOptions{nullptr},
		                                          &input, &input_tensor, 1,
		                                          &output, outputData.size());

		        // Process output
		        float* out = output_tensors[0].GetTensorMutableData<float>();

				memcpy(outputData.data(), out, outputData.size() * sizeof(float));
			}
			catch(std::exception& e)
			{
				return Result::fail(String(e.what()));
			}

			return Result::ok();
		}

		return Result::fail("Model is currently rebuilding");
	}

	CriticalSection lock;

	Ort::Env env;
	std::unique_ptr<Ort::Session> session;
	Ort::MemoryInfo memory_info;
};

#if 0
struct ONNXRuntime::Pimpl
{
	ScopedPointer<juce::DynamicLibrary> dll;

	onnxf::FuncCollection f;

    Pimpl(const String& libraryPath):
	  dll(new DynamicLibrary())
    {
		if(dll->open(libraryPath))
		{
			f.init(dll.get());
		}

        // Initialize the ONNX Runtime environment
        OrtApi = OrtGetApiBase()->GetApi(ORT_API_VERSION);
        OrtApi->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "ONNXRuntime", &env);
        OrtApi->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info);
    }

    ~Pimpl()
    {
        // Clean up resources
        if (session) OrtApi->ReleaseSession(session);
        if (memory_info) OrtApi->ReleaseMemoryInfo(memory_info);
        if (env) OrtApi->ReleaseEnv(env);
    }

    Result load(const MemoryBlock& mb)
    {
        OrtSessionOptions* session_options;
        OrtApi->CreateSessionOptions(&session_options);
        OrtApi->SetIntraOpNumThreads(session_options, 1);
        OrtApi->SetSessionGraphOptimizationLevel(session_options, ORT_ENABLE_BASIC);

        try
        {
            ScopedLock sl(lock);

            if (session) OrtApi->ReleaseSession(session); // Release previous session if it exists

            // Load model from memory block
            OrtApi->CreateSessionFromArray(env, mb.getData(), mb.getSize(), session_options, &session);
            OrtApi->ReleaseSessionOptions(session_options);

            return Result::ok();
        }
        catch (const std::exception& e)
        {
            OrtApi->ReleaseSessionOptions(session_options);
            return Result::fail(e.what());
        }
    }

    Result run(const Image& image, std::vector<float>& outputData, bool isGreyscale)
    {
        ScopedLock sl(lock);

        const char* input_name = "input";
        const char* output_name = "output";

        std::vector<int64_t> input_shape = {
            1,
            isGreyscale ? 1 : 3,
            static_cast<int64_t>(image.getHeight()),
            static_cast<int64_t>(image.getWidth())
        };

        std::vector<float> input_data(input_shape[1] * input_shape[2] * input_shape[3], 0.0f);

        OrtValue* input_tensor = nullptr;
        OrtApi->CreateTensorWithDataAsOrtValue(memory_info, input_data.data(), 
                                               input_data.size() * sizeof(float), 
                                               input_shape.data(), input_shape.size(), 
                                               ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, 
                                               &input_tensor);

        auto copy = image;
        Image::BitmapData bp(copy, 0, 0, image.getWidth(), image.getHeight(), Image::BitmapData::ReadWriteMode::readOnly);

        auto v = input_data.data();

        if (!isGreyscale)
        {
            OrtApi->ReleaseValue(input_tensor);
            return Result::fail("Colour images are not supported");
        }
        else
        {
            for (int y = 0; y < bp.height; y++)
            {
                for (int x = 0; x < bp.width; x++)
                {
                    *v++ = bp.getPixelColour(x, y).getBrightness();
                }
            }
        }

        try
        {
            // Run inference
            OrtValue* output_tensor = nullptr;
            OrtApi->Run(session, nullptr, &input_name, &input_tensor, 1, &output_name, 1, &output_tensor);

            // Process output
            float* out = nullptr;
            OrtApi->GetTensorMutableData(output_tensor, reinterpret_cast<void**>(&out));
            std::copy(out, out + outputData.size(), outputData.data());

            OrtApi->ReleaseValue(output_tensor);
            OrtApi->ReleaseValue(input_tensor);
        }
        catch (const std::exception& e)
        {
            OrtApi->ReleaseValue(input_tensor);
            return Result::fail(e.what());
        }

        return Result::ok();

        return Result::fail("Model is currently rebuilding");
    }

private:
    const OrtApi* OrtApi = nullptr;
    OrtEnv* env = nullptr;
    OrtSession* session = nullptr;
    OrtMemoryInfo* memory_info = nullptr;
    CriticalSection lock;
};
#endif

ONNXRuntime::ONNXRuntime():
  pimpl(new Pimpl())
{}

ONNXRuntime::~ONNXRuntime()
{
	delete pimpl;
}

Result ONNXRuntime::load(const MemoryBlock& mb)
{
	return pimpl->load(mb);
}

Result ONNXRuntime::load(const File& f)
{
	MemoryBlock mb;
	f.loadFileAsData(mb);
	return load(mb);
}

Result ONNXRuntime::run(const Image& img, std::vector<float>& outputValues, bool isGreyScale)
{
	testImage(img, true, "inside DLL");

	return pimpl->run(img, outputValues, isGreyScale);
}

struct Data
{
    
    Data(const void* data, size_t numBytes):
      ok(Result::ok()),
      model(data, numBytes)
    {
		ok = rt.load(model);
    };

    bool run(const void* imageData, size_t numBytes, int numOutputs, bool isGreyScale)
    {
        outputValues.resize(numOutputs);
        auto image = ImageFileFormat::loadFrom(imageData, numBytes);

        if(image.getFormat() == Image::PixelFormat::ARGB)
            ok = rt.run(image, outputValues, isGreyScale);
        else
            ok = Result::fail("Image must have pixel format ARGB");
	    
        return ok.wasOk();
    }
    
    ONNXRuntime rt;
    MemoryBlock model;
	Result ok;
    std::vector<float> outputValues;
};

DLL_EXPORT bool getError(void* model, char* buffer, size_t& length)
{
	auto typed = static_cast<Data*>(model);

    if(typed->ok.wasOk())
        return true;

    auto msg = typed->ok.getErrorMessage();

    memset(buffer, 0, 2048);

    length = msg.length();

    memcpy(buffer, msg.getCharPointer().getAddress(), length);
    return false;
}

/** Destroy the model from the given data. */
DLL_EXPORT void destroyModel(void* model)
{
	auto typed = static_cast<Data*>(model);
    delete typed;
}

DLL_EXPORT void getOutput(void* model, float* data)
{
	auto typed = static_cast<Data*>(model);
    memcpy(data, typed->outputValues.data(), typed->outputValues.size() * sizeof(float));
}

DLL_EXPORT bool runModel(void* model, const void* imageData, size_t numBytes, int numOutputs, bool isGreyscale)
{
	auto typed = static_cast<Data*>(model);
    return typed->run(imageData, numBytes, numOutputs, isGreyscale);
}

/** Load the model from the given data. */
DLL_EXPORT void* loadModel(const void* data, size_t numBytes)
{
    ScopedPointer<Data> d = new Data(data, numBytes);

    return d.release();
}

