/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"





using namespace hise;
using namespace scriptnode;
using namespace snex::Types;
using namespace snex::jit;
using namespace snex;

namespace choc
{
namespace ui
{
class WebView;
}
}

struct WebViewData: public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<WebViewData>;
    using CallbackType = std::function<var(const var&)>;
    using OpaqueResourceType = std::tuple<std::vector<uint8>, std::string>;
    
    struct CallbackItem;
    struct ExternalResource;
    
    struct ImportSource
    {
        ImportSource(const String& name_, const String& content_):
          name(name_),
          content(content_)
        {};
        
        String name;
        String content;
    };
    
    WebViewData() = default;
    
    OpaqueResourceType fetch(const std::string& path);
    
    void setErrorLogger(const std::function<void(const String& m)>& errorFunction)
    {
        errorLogger = errorFunction;
    }
    
    void setHtmlCode(const String& html)
    {
        htmlCode = html.toStdString();
    }
    
    void addScript(const String& script)
    {
        scripts.add(script);
    }
    
    void addCallback(const String& callbackName, const CallbackType& function);
    
    void addResource(const String& path, const String& mimeType, const String& content);
    
    void addPNGImage(const String& path, const Image& img);
    
    void addImportSource(const String& path, const String& content)
    {
        importSources.add(new ImportSource(path, content));
    }
    
    void createImportMap()
    {
        if(importSources.isEmpty() || importsCreatedAsResource)
            return;
        
        auto pf = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("polyfill.js");
        
        addResource("/polyfill.js", "text/javascript", pf.loadFileAsString());
        
        DynamicObject::Ptr p = new DynamicObject();
        
        for(auto i: importSources)
        {
            String path = "/import/" + i->name + ".js";
            
            addResource(path, "text/javascript", i->content);
            
            p->setProperty(Identifier(i->name), path);
        }
        
        DynamicObject::Ptr outer = new DynamicObject();
        
        outer->setProperty("imports", var(p.get()));
        
        String im;
        String nl = "\n";
        
        im << "<script type=\"text/javascript\" src=\"/polyfill.js\"/>" << nl;
        im << "<script type=\"importmap\">" << nl;
        im << JSON::toString(var(outer.get())) << nl;
        im << "</script>" << nl;
        
        DBG(im);
        
        htmlCode.append(im.toStdString());
        
        importsCreatedAsResource = true;
    }
    
private:
    
    bool importsCreatedAsResource = false;
    
    friend class WebViewWrapper;
    
    std::function<void(const String&)> errorLogger;
    
    std::string htmlCode;
    StringArray scripts;
    OwnedArray<ExternalResource> resources;
    Array<CallbackItem> callbacks;
    OwnedArray<ImportSource> importSources;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewData);
};

struct WebViewWrapper: public Component
{
#if JUCE_WINDOWS
    using NativeComponentType = juce::HWNDComponent;
#elif JUCE_MAC
    using NativeComponentType = juce::NSViewComponent;
#elif JUCE_LINUX
    using NativeComponentType = juce::XViewComponent; // or whatever, Dave, your job...
#endif
    
    WebViewWrapper(WebViewData::Ptr data);
    
    ~WebViewWrapper()
    {
        content.setView(nullptr);
        webView = nullptr;
        data = nullptr;
    }
    
    void resized()
    {
        content.setBounds(getLocalBounds());
    }
    
private:
    
    WebViewData::Ptr data;
    ScopedPointer<choc::ui::WebView> webView;
    NativeComponentType content;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewWrapper);
};

#include "choc/gui/choc_WebView.h"

// Implementation

struct WebViewData::CallbackItem
{
    CallbackItem() = default;
    
    CallbackItem(const String& name_, const CallbackType& f_):
      name(name_.toStdString()),
      callback(f_)
    {};
    
    choc::value::Value operator()(const choc::value::ValueView& args)
    {
        auto x = choc::json::toString(args);
        auto obj = JSON::parse(String(x.data()));
        auto ret = callback(obj);
        
        if(ret.isString())
        {
            auto s = ret.toString().toStdString();
            return choc::value::createString(s);
        }
        if(ret.isDouble())
            return choc::value::createFloat64((double)ret);
        if(ret.isInt() || ret.isInt64())
            return choc::value::createInt64((int64)ret);
        
        return choc::json::parse(JSON::toString(ret).toStdString());
    }
    
    void registerToWebView(choc::ui::WebView* wv) const
    {
        if(callback)
            wv->bind(name, *this);
    }
    
    std::string name;
    CallbackType callback;
};

void WebViewData::addCallback(const String& callbackName, const CallbackType& function)
{
    callbacks.add(CallbackItem(callbackName, function));
}

struct WebViewData::ExternalResource
{
    ExternalResource(const String& path_, const String& mimeType, const String& data):
      path(path_.toStdString())
    {
        auto& dataVector = std::get<0>(resource);
        dataVector.reserve(data.length());
        
        auto b = data.begin();
        auto e = data.end();
        
        while(b != e)
        {
            dataVector.push_back(*b++);
        }
        
        std::get<1>(resource) = mimeType.toStdString();
    }
    
    ExternalResource(const String& path_):
      path(path_.toStdString())
    {};
    
    virtual ~ExternalResource() {};
    
    const std::string path;
    
    OpaqueResourceType resource;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExternalResource);
};

void WebViewData::addResource(const String& path, const String& mimeType, const String& content)
{
    auto pstring = path.toStdString();
    for(auto r: resources)
    {
        if(r->path == pstring && errorLogger)
        {
            String m;
            m << "!WebView ERROR: Duplicate resource " << path;
        }
    }
    
    resources.add(new ExternalResource(path, mimeType, content));
}

void WebViewData::addPNGImage(const String& path, const Image& img)
{
    MemoryOutputStream mos;
    PNGImageFormat png;
    png.writeImageToStream(img, mos);
    
    mos.flush();
    auto mb = mos.getMemoryBlock();
    
    auto nr = new ExternalResource(path);
    
    auto& dv = std::get<0>(nr->resource);
    dv.resize(mb.getSize());
    memcpy(dv.data(), mb.getData(), mb.getSize());
    resources.add(nr);
}

WebViewData::OpaqueResourceType WebViewData::fetch(const std::string& path)
{
    for(auto r: resources)
    {
        if(r->path == path)
            return r->resource;
    }
    
    if(errorLogger)
    {
        String m;
        m << "!WebView Error: Resource for " << path << " not found";
        errorLogger(m);
    }
    
    jassertfalse;
    return {};
};

WebViewWrapper::WebViewWrapper(WebViewData::Ptr data)
{
    choc::ui::WebView::Options options;
    options.enableDebugMode = true;
    options.fetchResource = [data](const std::string& path)
    {
        choc::ui::WebView::Options::Resource s;
        
        if(path == "/")
        {
            s.data.reserve(data->htmlCode.length());
            
            for(const auto& c: data->htmlCode)
                s.data.push_back(c);
            
            s.mimeType = "text/html";
            return s;
        };
            
        auto type = data->fetch(path);
        
        s.data = std::move(std::get<0>(type));
        s.mimeType = std::move(std::get<1>(type));
        
        return s;
    };
    
    
    webView = new choc::ui::WebView(options);
    
    addAndMakeVisible(content);
    
    data->createImportMap();
    
    //webView->setHTML(data->htmlCode);
    
    for(auto& x: data->scripts)
        webView->addInitScript(x.toStdString());
    
    for(const auto& c: data->callbacks)
        c.registerToWebView(webView);
    
    content.setView(webView->getViewHandle());
}

//==============================================================================
MainComponent::MainComponent() :
	data(new ui::WorkbenchData())
{
	/** TODO:
	
	- split the entire snex compiler into parser & code generator, then test everything

	- chew threw unit tests with MIR
	- implement dyn
	- add all library objects (polydata, osc process data, etc)

	- remove all unnecessary things:
	  - inlining (?)
	  - indexes (?)
	  - 
	

	*/


	bool useValueTrees = false;
	
	laf.setDefaultColours(funkSlider);

    
    
	if (useValueTrees)
	{
		data->getTestData().setUpdater(&updater);

		auto compileThread = new snex::jit::JitNodeCompileThread(data);
		data->setCompileHandler(compileThread);

		provider = new snex::ui::ValueTreeCodeProvider(data, 2);
		data->setCodeProvider(provider);

		playground = new snex::ui::SnexPlayground(data, true);
		playground->setReadOnly(true);
		
		addAndMakeVisible(parameters = new snex::ui::ParameterList(data));

		addAndMakeVisible(testData = new snex::ui::TestDataComponent(data));

		addAndMakeVisible(graph1 = new snex::ui::Graph());
		addAndMakeVisible(graph2 = new snex::ui::Graph());
		addAndMakeVisible(complexData = new snex::ui::TestComplexDataManager(data));
	}
	else
	{
		auto compileThread = new snex::jit::TestCompileThread(data);
		data->setCompileHandler(compileThread);

		for (auto o : OptimizationIds::getDefaultIds())
			data->getGlobalScope().addOptimization(o);

		playground = new snex::ui::SnexPlayground(data, true);
		provider = new snex::ui::SnexPlayground::TestCodeProvider(*playground, {});
		data->setCodeProvider(provider, sendNotification);

		
	}

	
	
	

    //context.attachTo(*playground);
    
    WebViewData::Ptr data = new WebViewData();
    data->setHtmlCode(R"xxx(
      <!DOCTYPE html>
      <html>
          <head lang="en">
              <meta charset="utf-8">
              <title>My first three.js app</title>
              <style>
                  body { margin: 0; }
              </style>
          </head>
          <body>
              <script type="module" src="/main.js"></script>
          </body>
      </html>
    )xxx");
    
    auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("three.js");
    auto s = f.loadFileAsString();
    
    //data->addImportSource("three", s);
    
    data->addResource("/three.js", "text/javascript", s);
    
    data->addResource("/main.js", "text/javascript", R"xxx(
        
        import * as THREE from "/three.js"
    
        const scene = new THREE.Scene();
        const camera = new THREE.PerspectiveCamera( 75, window.innerWidth / window.innerHeight, 0.1, 100 );

        const renderer = new THREE.WebGLRenderer();
        renderer.setSize( 400, 400 );
        document.body.appendChild( renderer.domElement );

        const geometry = new THREE.BoxGeometry( 1, 1, 1 );
        const material = new THREE.MeshBasicMaterial( { color: 0x0088FF } );
        const cube = new THREE.Mesh( geometry, material );
        scene.add( cube );

        camera.position.z = 5;

        function animate() {
            requestAnimationFrame( animate );

            getX(1.0).then ((result) => { cube.rotation.x += result; });
            cube.rotation.y += 0.01;

            renderer.render( scene, camera );
        }

        animate();
    )xxx");
    
    data->addCallback("getX", [](const var& v)
    {
        return JUCE_LIVE_CONSTANT(0.01);
    });
    
    data->setErrorLogger([](const String& d)
    {
        DBG(d);
    });
    
    addAndMakeVisible(webViewWrapper = new WebViewWrapper(data));
    
    addAndMakeVisible(playground);
    
    //playground->toFront(true);
    //context.attachTo(*playground);
    
    setSize (1024, 768);
}

MainComponent::~MainComponent()
{
	data = nullptr;

	//context.detach();

    

}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	//g.fillAll(Colour(0xFF333336));
}

void MainComponent::resized()
{
	auto b = getLocalBounds();

	if (parameters != nullptr)
		parameters->setBounds(b.removeFromTop(50));

	if (graph1 != nullptr)
	{
		//auto gb = b.removeFromTop(400);
		testData->setBounds(b.removeFromTop(150));
		graph1->setBounds(b.removeFromTop(150));// .removeFromLeft(400));
		//graph2->setBounds(b);// .removeFromLeft(400));
		
		
	}
	
    
    
    webViewWrapper->setBounds(b);
    playground->setBounds(b.removeFromBottom(200));
    
    
	
}
