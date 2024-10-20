#define RTNEURAL_DEFAULT_ALIGNMENT 16
#define RTNEURAL_USE_XSIMD 1

#include "hi_neural.h"

#include "RTNeural/RTNeural/RTNeural.h"

namespace hise
{
using namespace juce;

#define DECLARE_ID(x) static const Identifier x(#x)
namespace PytorchIds
{
	// Containers
	DECLARE_ID(Sequential);

	// Layers
	DECLARE_ID(Linear);

	// Activations
	DECLARE_ID(Tanh);
	DECLARE_ID(ReLU);
	DECLARE_ID(Sigmoid);

	DECLARE_ID(in_features);
	DECLARE_ID(out_features);

	struct Helpers
	{
		static std::pair<Identifier, bool> getTypeIdAndIsActivation(RTNeural::Layer<float>* l)
		{
			if(dynamic_cast<RTNeural::Dense<float>*>(l))
				return { Linear, false };
			if(dynamic_cast<RTNeural::TanhActivation<float>*>(l))
				return { Tanh, true };
			if(dynamic_cast<RTNeural::ReLuActivation<float>*>(l))
				return { ReLU, true };
			if(dynamic_cast<RTNeural::SigmoidActivation<float>*>(l))
				return { Sigmoid, true };

			return { {}, false };
		}
	};
}

#undef DECLARE_ID

class PytorchParser
{
	struct LayerInfo
	{
		void parseArgs(const Identifier& id, const String& value)
		{
			if(id == PytorchIds::in_features)
				inputs = value.getIntValue();
			if(id == PytorchIds::out_features)
				outputs = value.getIntValue();
		}

		RTNeural::Layer<float>* createLayer() const
		{
			if(type == PytorchIds::Linear)
				return new RTNeural::Dense<float>(inputs, outputs);
			if(type == PytorchIds::Tanh)
				return new RTNeural::TanhActivation<float>(inputs);
			if(type == PytorchIds::ReLU)
				return new RTNeural::ReLuActivation<float>(inputs);
			if(type == PytorchIds::Sigmoid)
				return new RTNeural::SigmoidActivation<float>(inputs);
			
			throw Result::fail("Can't create layer with ID " + type.toString());
		}

		Identifier type;
		String name;
		int inputs = 0;
		int outputs = 0;
		bool isActivationFunction = false;

		var toJSON() const
		{
			DynamicObject* obj = new DynamicObject();

			obj->setProperty("type", type.toString());
			obj->setProperty("name", name);
			obj->setProperty("inputs", inputs);
			obj->setProperty("outputs", outputs);
			obj->setProperty("isActivation", isActivationFunction);

			return var(obj);
		}

		void fromJSON(const var& layerData)
		{
			type = layerData["type"].toString();
			name = layerData["name"];
			inputs = layerData["inputs"];
			outputs = layerData["outputs"];
			isActivationFunction = layerData["isActivation"];
		}
	};

	var toJSON() const
	{
		Array<var> list;

		for(const auto& l: layers)
			list.add(l.toJSON());

		return var(list);
	}

public:

	/** Creates a parser from a previously exported JSON list. */
	explicit PytorchParser(const var& layerList)
	{
		if(auto ar = layerList.getArray())
		{
			for(const auto& l: *ar)
			{
				LayerInfo newInfo;
				newInfo.fromJSON(l);
				layers.add(std::move(newInfo));
			}
		}

	}

	/** Creates a parser from a previously exported JSON list. */
	explicit PytorchParser(const String& modelLayout)
	{
		parseLayers(modelLayout);
	}

	static var createJSONModel(const String& modelLayout)
	{
		PytorchParser p(modelLayout);
		return p.toJSON();
	}

	using ModelPtr = std::unique_ptr<RTNeural::Model<float>>;

	std::pair<int, int> getNumConnections() const
	{
		return { layers.getFirst().inputs, layers.getLast().outputs };
	}

	ModelPtr createModel()
	{
		auto numInputs = layers.getFirst().inputs;

		auto model = std::make_unique<RTNeural::Model<float>>(numInputs);

		for(const auto& l: layers)
		{
			model->addLayer(l.createLayer());
		}

		return model;
	}


	Result loadWeights(const ModelPtr& model, const nlohmann::json& modelJson)
	{
		try
		{
			for(int i = 0; i < layers.size(); i++)
			{
				auto l = layers[i];

				if(l.isActivationFunction)
					continue;

				std::string prefix; prefix.append(l.name.toStdString()); prefix.append(".");

				auto thisLayer = model->layers[i];
				
				if(l.type == PytorchIds::Linear)
				{
					RTNeural::torch_helpers::loadDense<float> (modelJson, prefix, *dynamic_cast<RTNeural::Dense<float>*>(thisLayer));
				}
				else
				{
					throw Result::fail("Unknown type " + l.type.toString());
				}
			}
		}
		catch(Result& r)
		{
			return r;
		}

		return Result::ok();
	}

	/** Creates a JSON representation of all the layers that can be fed into the C++ code generator. */
	static var toJSON(const ModelPtr& model)
	{
		Array<var> list;

		for(auto l: model->layers)
		{
			LayerInfo newInfo;

			newInfo.name = l->getName();
			newInfo.inputs = l->in_size;
			newInfo.outputs = l->out_size;

			auto i2 = PytorchIds::Helpers::getTypeIdAndIsActivation(l);

			newInfo.type = i2.first;
			newInfo.isActivationFunction = i2.second;

			list.add(newInfo.toJSON());
		}

		return var(list);
	}

private:

	void parseLayers(const String& modelLayout)
	{
		auto lines = StringArray::fromLines(modelLayout);

		layers.ensureStorageAllocated(lines.size() - 2);

		String currentSequentialId;

		for(auto l: lines)
		{
			auto t = l.trim();

			if(t.startsWithChar(')') && currentSequentialId.isNotEmpty())
			{
				currentSequentialId = {};
			}

			if(!t.startsWithChar('('))
				continue;

			

			auto t1 = StringArray::fromTokens(t, ":", "");

			LayerInfo info;

			if(currentSequentialId.isNotEmpty())
				info.name << currentSequentialId << ".";

			info.name << t1[0].removeCharacters("()");

			info.type = Identifier(t1[1].upToFirstOccurrenceOf("(", false, false).trim());

			

			auto t2 = t1[1].fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false).trim();
			auto t3 = StringArray::fromTokens(t2, ",", "");
			t3.trim();

			for(auto arg: t3)
			{
				auto t4 = StringArray::fromTokens(arg, "=", "");
				auto key = Identifier(t4[0]);
				auto value = t4[1];
				info.parseArgs(key, value);
			}

			if(info.type == PytorchIds::Sequential)
				currentSequentialId = info.name;

			layers.add(std::move(info));
		}

		for(int i = 0; i < layers.size(); i++)
			if(layers[i].type == PytorchIds::Sequential)
				layers.remove(i--);

		for(int i = 1; i < layers.size(); i++)
		{
			auto& ref = layers.getReference(i);

			if(ref.inputs == 0)
			{
				ref.isActivationFunction = true;
				ref.inputs = layers[i-1].outputs;;
				ref.outputs = ref.inputs;
			}
		}
	}

	Array<LayerInfo> layers;
};

struct EmptyModel: public NeuralNetwork::ModelBase
{
	ModelBase* clone() { return new EmptyModel(); }

	void process(const float*, float*) override {};
	void reset() override {};
	int getNumInputs() const override { return 0; }
	int getNumOutputs() const override { return 0; };
	Result loadWeights(const String& jsonData) override
	{
		return Result::fail("network is not initialised");
	}
};

struct TensorFlowModel: public NeuralNetwork::ModelBase
{
	TensorFlowModel(const nlohmann::json& jsonData):
	  modelData(jsonData)
	{
		model = RTNeural::json_parser::parseJson<float>(modelData);
		numInputs = model->getInSize();
		numOutputs = model->getOutSize();
		model->reset();
	}

	TensorFlowModel(const var& obj)
	{
		auto s = JSON::toString(obj, false).toStdString();
		modelData = nlohmann::json::parse(s);
		model = RTNeural::json_parser::parseJson<float>(modelData);

		numInputs = model->getInSize();
		numOutputs = model->getOutSize();
		model->reset();
	}

	ModelBase* clone() { return new TensorFlowModel(modelData); }

	Result loadWeights(const String& jsonData)
	{
		return Result::fail("Tensor Flow models will initialise their weights with the model JSON");
	}

	void reset() final
	{
		 model->reset();
	}

	void process(const float* input, float* output) final
	{
		model->forward(input);
		memcpy(output, model->getOutputs(), sizeof(float) * numOutputs);
	}

	int getNumInputs() const final { return numInputs; }
	int getNumOutputs() const final { return numOutputs; }

	int numInputs = 0;
	int numOutputs = 0;

	PytorchParser::ModelPtr model;

	nlohmann::json modelData;
};

struct DynamicModel: public NeuralNetwork::ModelBase
{
	DynamicModel(const var& modelJSON):
	  p(modelJSON),
	  layoutData(modelJSON)
	{
		auto c = p.getNumConnections();
		numInputs = c.first;
		numOutputs = c.second;
		model = p.createModel();
	}

	DynamicModel* clone()
	{
		auto nt = new DynamicModel(layoutData);
		nt->loadWeightsInternal(weights);
		return nt;
	}

	Result loadWeightsInternal(const nlohmann::json& weights_)
	{
		weights = weights_;
		return p.loadWeights(model, weights);
	}

	Result loadWeights(const String& jsonData) final
	{
		return loadWeightsInternal(nlohmann::json::parse(jsonData.toStdString()));
	}

	void reset() final
	{
		 model->reset();
	}

	void process(const float* input, float* output) final
	{
		model->forward(input);
		memcpy(output, model->getOutputs(), sizeof(float) * numOutputs);
	}

	int getNumInputs() const final { return numInputs; }
	int getNumOutputs() const final { return numOutputs; }

	nlohmann::json weights;

	PytorchParser p;
	PytorchParser::ModelPtr model;

	int numInputs = 0;
	int numOutputs = 0;

	var layoutData;
	String weightData;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicModel);
};


NeuralNetwork::Ptr NeuralNetwork::Holder::getOrCreate(const Identifier& id)
{
	for(auto nn: networks)
		if(id == nn->getId())
			return nn;

	auto nn = new NeuralNetwork(id, getFactory());

	networks.add(nn);
	return Ptr(nn);
}

StringArray NeuralNetwork::Holder::getIdList() const
{
	StringArray sa;

	for(auto nn: networks)
		sa.add(nn->getId().toString());

	return sa;
}

NeuralNetwork::NeuralNetwork(const Identifier& id_, Factory* f):
	id(id_),
    factory(f)
{
    jassert(f != nullptr);
	currentModels.add(f->create(id));
}

NeuralNetwork::~NeuralNetwork()
{
    SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
    currentModels.clear();
}

NeuralNetwork::Ptr NeuralNetwork::clone(int numNetworks)
{
    auto nn = new NeuralNetwork(getId(), factory);
    
    nn->currentModels.clear();
    
    nn->context = context;
    
    for(int i = 0; i < numNetworks; i++)
        nn->currentModels.add(currentModels.getFirst()->clone());
    
    return nn;
}

var NeuralNetwork::parseModelJSON(const File& modelFile)
{
	return PytorchParser::createJSONModel(modelFile.loadFileAsString());
}

Result NeuralNetwork::loadPytorchModel(const var& fullJson)
{
	auto layerData = fullJson["layers"].toString();
	auto weightData = JSON::toString(fullJson["weights"]);

	auto modelJson = PytorchParser::createJSONModel(layerData);

	auto ok = build(modelJson);

	if(!ok.wasOk())
		return ok;

	return loadWeights(weightData);
}

void NeuralNetwork::clearModel()
{
	OwnedArray<ModelBase> nm;

	for(int i = 0; i < getNumNetworks(); i++)
		nm.add(new EmptyModel());

	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
		currentModels.swapWith(nm);
	}
}

Result NeuralNetwork::build(const var& modelJSON)
{
	OwnedArray<ModelBase> nm;

	try
	{
		nm.add(new DynamicModel(modelJSON));

		for(int i = 1; i < getNumNetworks(); i++)
			nm.add(nm.getFirst()->clone());
	}
	catch(Result& r)
	{
		return r;
	}

	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
		currentModels.swapWith(nm);
	}
	
	return Result::ok();
}

Result NeuralNetwork::loadWeights(const String& jsonData)
{
	auto ok = Result::ok();

	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);

		for(auto l: currentModels)
			ok = l->loadWeights(jsonData);
	}
	
	reset(-1);
	return ok;
}

var NeuralNetwork::getModelJSON() const
{
	if(auto d = dynamic_cast<DynamicModel*>(currentModels.getFirst()))
	{
		return PytorchParser::toJSON(d->model);
	}
	if(auto d = dynamic_cast<TensorFlowModel*>(currentModels.getFirst()))
	{
		return PytorchParser::toJSON(d->model);
	}

	return {};
}

void NeuralNetwork::setNumNetworks(int numNetworks, bool forceClone)
{
	if(numNetworks == 0)
		return;

	if(!forceClone && context.fixChannelSize > 0)
		return;

	if(getNumNetworks() != numNetworks)
	{
		OwnedArray<ModelBase> newModels;

		auto toCopy = currentModels.getFirst();

		newModels.ensureStorageAllocated(numNetworks);

		for(int i = 0; i < numNetworks; i++)
		{
			newModels.add(toCopy->clone());
			newModels.getLast()->reset();
		}

		{
			SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
			newModels.swapWith(currentModels);
		}
	}
}

int NeuralNetwork::getNumInputs() const
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	return currentModels.getFirst()->getNumInputs();
}

int NeuralNetwork::getNumOutputs() const
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	return currentModels.getFirst()->getNumOutputs();
}

void NeuralNetwork::reset(int networkIndex)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if(networkIndex == -1)
	{
		for(auto m: currentModels)
			m->reset();
	}
	else if(auto cm = currentModels[networkIndex])
		cm->reset();
}

void NeuralNetwork::process(int networkIndex, const float* input, float* output)
{
	if(auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
	{
		if(auto cm = currentModels[networkIndex])
			cm->process(input, output);
	}
}

Result NeuralNetwork::loadTensorFlowModel(const var& jsonData)
{
	OwnedArray<ModelBase> nt;

	nt.add(new TensorFlowModel(jsonData));

	for(int i = 1; i < getNumNetworks(); i++)
		nt.add(nt.getFirst()->clone());
		

	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
		currentModels.swapWith(nt);
	}
	
	return Result::ok();
}

#define RTN_PARSE_MODEL_JSON(x) auto modelJson = nlohmann::json::parse(x.toStdString());
#define RTN_MODEL_ID(x) SN_NODE_ID(#x); static NeuralNetwork::ModelBase* create() { return new x(); }
#define RTN_LOAD_MODEL_LAYER(index, name) RTNeural::torch_helpers::loadDense<float>(modelJson, name, this->obj.get<index>());

template <typename ModelType> struct CompiledModel: public NeuralNetwork::ModelBase
{
	void reset() final { obj.reset(); }

	int getNumInputs() const final { return ModelType::input_size; }
	int getNumOutputs() const final { return ModelType::output_size; }

protected:

	template <int Idx> void loadLayer(const nlohmann::json& modelJson, std::string name)
	{
		name.append(".");
		RTNeural::torch_helpers::loadDense<float>(modelJson, name, this->obj.template get<Idx>());
	}

	void process(const float* input, float* output) final
	{
		this->obj.forward(input);
		memcpy(output, obj.getOutputs(), sizeof(float) * getNumOutputs());
	}

	ModelType obj;
};


#if 0 // Example output of the CPP builder
namespace pimpl
{
using l1_t = RTNeural::DenseT<float, 1, 16>;
using t1_t = RTNeural::ReLuActivationT<float, 16>;
using l2_t = RTNeural::DenseT<float, 16, 4>;
using t2_t = RTNeural::ReLuActivationT<float, 4>;
using l3_t = RTNeural::DenseT<float, 4, 1>;
using MyFunk_t = RTNeural::ModelT<float, 1, 1, l1_t, t1_t, 
                                  l2_t, t2_t, l3_t>;
}

// ===============| Class definition |===============

struct MyFunk: public CompiledModel<pimpl::MyFunk_t>
{
	RTN_MODEL_ID(MyFunk);
	
	Result loadWeights(const String& jsonData) final
	{
		RTN_PARSE_MODEL_JSON(jsonData);
		
		RTN_LOAD_MODEL_LAYER(0, "l1.");
		RTN_LOAD_MODEL_LAYER(2, "l2.");
		RTN_LOAD_MODEL_LAYER(4, "l3.");
		
		return Result::ok();
	};
};
#endif


NeuralNetwork::Factory::Factory()
{
	defaultFunction = [](){ return new EmptyModel(); };
}

NeuralNetwork::ModelBase* NeuralNetwork::Factory::create(const Identifier& id)
{
	for(const auto& entry: registeredModels)
	{
		if(entry.first == id)
		{
			return entry.second();
		}
	}

	return defaultFunction();
}

NeuralNetwork::CppBuilder::CppBuilder(const Identifier& id_, const var& modelJson):
	id(id_)
{
	if(modelJson.isArray())
		layers = *modelJson.getArray();
}

String NeuralNetwork::CppBuilder::createCppModel() const
{
#if HISE_INCLUDE_SNEX
	HashMap<String, NamespacedIdentifier> layerClasses;
	NamespacedIdentifier rt("RTNeural");
	layerClasses.set("Linear", rt.getChildId("DenseT"));
	layerClasses.set("ReLU", rt.getChildId("ReLuActivationT"));

	using namespace snex::cppgen;
	Base b(Base::OutputType::AddTabs);
	UsingTemplate mt(b, Identifier(id + "_t"), rt.getChildId("ModelT"));
	b.addComment("Type definitions for the model layers", Base::CommentType::FillTo80);
	{
		Namespace n(b, "pimpl", false);

		StringArray list;

		for(const auto& l: layers)
		{
			auto type = l["type"].toString();
			auto name = l["name"].toString();
			auto inputs = (int)l["inputs"];
			auto outputs = (int)l["outputs"];
			auto isActivationFunction = (bool)l["isActivation"];
			auto typeName = name + "_t";

			UsingTemplate ut(b, Identifier(typeName), layerClasses[type]);
			ut << "float";
			ut << inputs;

			if(!isActivationFunction)
				ut << outputs;

			ut.flushIfNot();
			list.add(typeName);
		}

		mt << "float";
		mt << (int)layers.getFirst()["inputs"];
		mt << (int)layers.getLast()["outputs"];

		for(auto& l: list)
			mt << l;

		mt.flushIfNot();
	}

	b.addComment("Class definition", Base::CommentType::FillTo80);
	Struct s(b, id, { NamespacedIdentifier::fromString("CompiledModel<pimpl::" + id.toString() + "_t>") }, {}, true);
	{
		b << String("RTN_MODEL_ID(" + id.toString() + ");");
		b.addEmptyLine();
		b << "Result loadWeights(const String& jsonData) final";
		{
			StatementBlock sb(b, true);
			b << "RTN_PARSE_MODEL_JSON(jsonData);";
			b.addEmptyLine();
			int idx = 0;

			for(const auto& l: layers)
			{
				if(l["isActivation"])
				{
					idx++;
					continue;
				}
					
				String s;
				s << "RTN_LOAD_MODEL_LAYER(" << String(idx) << ", \"" << l["name"].toString() << ".\");";
				b << s;
				idx++;
			}

			b.addEmptyLine();
			b << "return Result::ok();";
		}
	}

	s.flushIfNot();
	return b.toString();
#else
	jassertfalse;
	return {};
#endif
}



}
