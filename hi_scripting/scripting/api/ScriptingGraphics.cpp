/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;


ScriptingObjects::ScriptShader::FileParser::FileParser(ProcessorWithScriptingContent* p, bool addLineNumbers_, String& fileNameWithoutExtension_, FileList& listToUse):
	ControlledObject(p->getMainController_()),
	sp(p),
	includedFiles(listToUse),
	fileNameWithoutExtension(fileNameWithoutExtension_),
	addLineNumbers(addLineNumbers_)
{

}

StringArray ScriptingObjects::ScriptShader::FileParser::getLines()
{
	static const String incl = "#include";

	if(addLineNumbers)
		s << createLinePointer(0) << "\n";

	s << loadFileContent();

	if (s.contains(incl))
	{
		auto lines = StringArray::fromLines(s);

		for (int i = 0; i < lines.size(); i++)
		{
			auto s = lines[i];

			if (s.startsWith(incl))
			{
				auto fileNameToInclude = s.fromFirstOccurrenceOf(incl, false, false).trim().unquoted();
				FileParser includeParser(sp, addLineNumbers, fileNameToInclude, includedFiles);

				auto includedLines = includeParser.getLines();

				lines.remove(i);
				
				for (int j = includedLines.size(); j >= 0; --j)
					lines.insert(i, includedLines[j]);

				int lineToUse = i + 1;

				i += includedLines.size();

				if(addLineNumbers)
					lines.insert(i +1, createLinePointer(lineToUse-1));
			}
		}

		return lines;
	}
	
	return StringArray::fromLines(s);
}

String ScriptingObjects::ScriptShader::FileParser::createLinePointer(int i) const
{
	String l;
#if USE_BACKEND
	l << "#line " << String(i) <<  " \"" << fileNameWithoutExtension << "\"";
#endif
	return l;
}

String ScriptingObjects::ScriptShader::FileParser::loadFileContent()
{
#if USE_BACKEND

	auto glslFile = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts).getChildFile(fileNameWithoutExtension).withFileExtension("glsl");


	auto ef = getMainController()->getExternalScriptFile(glslFile, false);

	String content;

	if(ef != nullptr)
		content = ef->getFileDocument().getAllContent();
	else if (glslFile.existsAsFile())
		content = glslFile.loadFileAsString();
	else
    {
        String s;
        String nl = "\n";
        
        s << "void main()" << nl;
        s << "{" << nl;
        s << "    // Normalized pixel coordinates (from 0 to 1)" << nl;
        s << "    vec2 uv = fragCoord/iResolution.xy;" << nl;
        s << nl;
        s << "    // Time varying pixel color" << nl;
        s << "    vec3 col = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));" << nl;
        s << nl;
        s << "    // Output to screen" << nl;
        s << "    fragColor = pixelAlpha * vec4(col,1.0);" << nl;
        s << "}" << nl;
        
		content = s;
		glslFile.replaceWithText(s);
    }
	
	if (auto jp = dynamic_cast<JavascriptProcessor*>(sp))
	{
		ef = jp->addFileWatcher(glslFile);

		if (includedFiles.contains(ef))
			throw String("Trying to include " + fileNameWithoutExtension + " multiple times");

		includedFiles.add(ef);
		jp->getScriptEngine()->addShaderFile(glslFile);
	}

	return content;
#else

	if (!fileNameWithoutExtension.endsWith(".glsl"))
		fileNameWithoutExtension << ".glsl";

	return getMainController()->getExternalScriptFromCollection(fileNameWithoutExtension);
#endif
}

struct ScriptingObjects::ScriptShader::PreviewComponent: public Component,
													     public ComponentForDebugInformation,
														 public ApiProviderBase::Holder,
														 public PathFactory,
														 public ButtonListener,
														 public Timer
{

	PreviewComponent(ScriptShader* s) :
		ComponentForDebugInformation(s, dynamic_cast<ApiProviderBase::Holder*>(s->getScriptProcessor())),
		resizer(this, nullptr),
		statsButton("stats", this, *this),
		viewButton("view", this, *this),
		resetTimeButton("reset", this, *this)
	{
		addAndMakeVisible(statsButton);
		addAndMakeVisible(resetTimeButton);
		addAndMakeVisible(viewButton);

		statsButton.setToggleModeWithColourChange(true);
		viewButton.setToggleModeWithColourChange(true);

		addAndMakeVisible(uniformDataViewer = new ScriptWatchTable());
        uniformDataViewer->setOpaque(false);
        uniformDataViewer->bgColour = Colours::transparentBlack;
		uniformDataViewer->setHolder(this);
		addAndMakeVisible(resizer);
		setSize(600, 400);
		startTimer(15);
		setName("Shader preview");
	}

	void timerCallback() override
	{
		repaint();
	}

	void buttonClicked(Button* b) override
	{
		if (b == &resetTimeButton)
		{
			if (auto s = getObject<ScriptShader>())
			{
				s->iTime = 0.0;
			}
		}
		
		if (b == &viewButton)
		{
			uniformDataViewer->setVisible(b->getToggleState());
			resized();
		}
			
	}

	void refresh() override
	{
		provider = nullptr;
	}

	void paint(Graphics& g) override
	{
		using namespace gl;

		if (auto obj = getObject<ScriptShader>())
		{
            auto tc = TopLevelWindowWithOptionalOpenGL::findRoot(this);
            
            if(!dynamic_cast<TopLevelWindowWithOptionalOpenGL*>(tc)->isOpenGLEnabled())
            {
                String s;
                s << "### Open GL is not enabled  \n";
                s << "> Goto the Settings and tick the **Enable OpenGL** box, then restart HISE in order to use the OpenGL renderer that is required for painting this shader";

                MarkdownRenderer r(s);
                r.parse();
                r.getHeightForWidth(getWidth() - 20.0f);

                r.draw(g, getLocalBounds().toFloat().reduced(10.0f));
                return;
            }
                
            
			if (obj->shader == nullptr)
				return;

			if (obj->dirty)
			{
				auto r = obj->shader->checkCompilation(g.getInternalContext());
				obj->setCompileResult(r);
				obj->makeStatistics();
				obj->dirty = false;
			}

			if (obj->compiledOk())
			{
				auto localBounds = getLocalBounds();
                localBounds.removeFromTop(24);
                
				if (viewButton.getToggleState())
					localBounds.removeFromRight(uniformDataViewer->getWidth());

                UnblurryGraphics ug(g, *this);

                auto sf = ug.getTotalScaleFactor();
                auto st2 = AffineTransform::scale(sf);
                
				auto gb = getLocalArea(getTopLevelComponent(), getLocalBounds()).transformed(st2);

				obj->setGlobalBounds(gb, 1.0f);
				obj->localRect = localBounds.toFloat().transformed(st2);

                
				auto enabled = obj->enableBlending;
				auto wasEnabled = glIsEnabled(GL_BLEND);

				int blendSrc;
				int blendDst;

				

				glGetIntegerv(GL_BLEND_SRC, &blendSrc);
				glGetIntegerv(GL_BLEND_DST, &blendDst);

				if (enabled)
				{
					glEnable(GL_BLEND);
					glBlendFunc((int)obj->src, (int)obj->dst);
				}

				obj->shader->fillRect(g.getInternalContext(), localBounds);

				if(viewButton.getToggleState())
					rebuild();

				// reset it to default
				if (enabled)
				{
					if (!wasEnabled)
						glDisable(GL_BLEND);

					glBlendFunc(blendSrc, blendDst);
				}

				g.setColour(Colours::black.withAlpha(0.5f));

				if (statsButton.getToggleState())
				{
					String s;

					s << "`";
					s << JSON::toString(obj->getOpenGLStatistics(), true);
					s << "`";
					MarkdownRenderer r(s);
					r.parse();
					r.getHeightForWidth(getWidth() - 20.0f);
					r.draw(g, localBounds.toFloat().reduced(10.0f));
				}
			}
			else
			{
				String s;
				s << "### Compilation Error: \n";
				s << "`";
				s << obj->getErrorMessage(false);
				s << "`";

				MarkdownRenderer r(s);
				r.parse();
				r.getHeightForWidth(getWidth() - 20.0f);

				r.draw(g, getLocalBounds().toFloat().reduced(10.0f));
			}
		}
	}

	

	struct UniformProvider : public ApiProviderBase
	{
		UniformProvider(ScriptShader* s):
			shader(s)
		{

		}

		/** Override this method and return the number of all objects that you want to use. */
		virtual int getNumDebugObjects() const
		{
			if (shader != nullptr)
			{
				return shader->uniformData.size();
			}

			return 0;
		}

		void getColourAndLetterForType(int type, Colour& colour, char& letter)
		{
			auto t = (DebugInformation::Type)type;

			switch (t)
			{
			case DebugInformation::Type::RegisterVariable:
				colour = snex::Types::Helpers::getColourForType(Types::ID::Float);
				letter = 'F';
				break;
			case DebugInformation::Type::Constant:
				colour = snex::Types::Helpers::getColourForType(Types::ID::Integer);
				letter = 'I';
				break;
			case DebugInformation::Type::Callback:
				colour = Colours::red.withSaturation(0.6f);
				letter = 'A';
				break;
                default: jassertfalse; break;
			}
		}

		DebugInformationBase::Ptr getDebugInformation(int index) override
		{
			if (shader == nullptr)
				return nullptr;

			Identifier id = shader->uniformData.getName(index);

			auto v = shader->uniformData[id];

			DebugInformation::Type t = DebugInformation::Type::Variables;

			if (v.isDouble())
				t = DebugInformation::Type::RegisterVariable;

			if (v.isArray() || v.isBuffer())
				t = DebugInformation::Type::Callback;

			if (v.isInt() || v.isInt64())
				t = DebugInformation::Type::Constant;

			WeakReference<ScriptShader> s = shader;

			auto vf = [s, id]()
			{
				if (s != nullptr)
				{
					return s->uniformData[id];
				}

				return var();
			};
			
			return new LambdaValueInformation(vf, id, {}, t, {});
		}

		WeakReference<ScriptShader> shader;
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UniformProvider);
	};

	

	ApiProviderBase* getProviderBase() override
	{
		if(provider == nullptr)
			provider = new UniformProvider(getObject<ScriptShader>().obj);

		return provider;
	}

	Path createPath(const String& url) const override
	{
		Path p;

#if USE_BACKEND
		LOAD_EPATH_IF_URL("stats", BackendBinaryData::ToolbarIcons::debugPanel);
		LOAD_EPATH_IF_URL("view", BackendBinaryData::ToolbarIcons::viewPanel);
		LOAD_PATH_IF_URL("time", ColumnIcons::moveIcon);
#endif
		return p;
	}
	
	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(24);

		resetTimeButton.setBounds(top.removeFromLeft(24).reduced(2));
		statsButton.setBounds(top.removeFromLeft(24).reduced(2));
		viewButton.setBounds(top.removeFromLeft(24).reduced(2));
		
		if(viewButton.getToggleState())
			uniformDataViewer->setBounds(b.removeFromRight(350));

		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
	}

	HiseShapeButton viewButton;
	HiseShapeButton statsButton;
	HiseShapeButton resetTimeButton;

	ScopedPointer<ScriptWatchTable> uniformDataViewer;
	ScopedPointer<UniformProvider> provider;
	juce::ResizableCornerComponent resizer;
};


struct ScriptingObjects::ScriptShader::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptShader, setFragmentShader);
	API_VOID_METHOD_WRAPPER_2(ScriptShader, setUniformData);
	API_VOID_METHOD_WRAPPER_3(ScriptShader, setBlendFunc);
	API_VOID_METHOD_WRAPPER_1(ScriptShader, fromBase64);
	API_VOID_METHOD_WRAPPER_1(ScriptShader, setEnableCachedBuffer);
	API_VOID_METHOD_WRAPPER_2(ScriptShader, setPreprocessor);
	API_METHOD_WRAPPER_0(ScriptShader, toBase64);
	API_METHOD_WRAPPER_0(ScriptShader, getOpenGLStatistics);
};

ScriptingObjects::ScriptShader::ScriptShader(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, (int)BlendMode::numBlendModes),
	r(Result::fail("uncompiled"))
{
	using namespace juce::gl;

	addConstant("GL_ZERO", GL_ZERO); 
	addConstant("GL_ONE", GL_ONE); 
	addConstant("GL_SRC_COLOR", GL_SRC_COLOR); 
	addConstant("GL_ONE_MINUS_SRC_COLOR", GL_ONE_MINUS_SRC_COLOR); 
	addConstant("GL_DST_COLOR", GL_DST_COLOR); 
	addConstant("GL_ONE_MINUS_DST_COLOR", GL_ONE_MINUS_DST_COLOR);
	addConstant("GL_SRC_ALPHA", GL_SRC_ALPHA);
	addConstant("GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA); 
	addConstant("GL_DST_ALPHA", GL_DST_ALPHA); 
	addConstant("GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA); 
	addConstant("GL_SRC_ALPHA_SATURATE", GL_SRC_ALPHA_SATURATE);

	ADD_API_METHOD_1(setFragmentShader);
	ADD_API_METHOD_2(setUniformData);
	ADD_API_METHOD_3(setBlendFunc);
	ADD_API_METHOD_1(fromBase64);
	ADD_API_METHOD_0(toBase64);
	ADD_API_METHOD_0(getOpenGLStatistics);
	ADD_API_METHOD_1(setEnableCachedBuffer);
	ADD_API_METHOD_2(setPreprocessor);
}

Identifier ScriptingObjects::ScriptShader::getObjectName() const
{ RETURN_STATIC_IDENTIFIER("ScriptShader"); }

void ScriptingObjects::ScriptShader::setEnableLineNumbers(bool shouldUseLineNumbers)
{
	useLineNumbers = shouldUseLineNumbers;
}

bool ScriptingObjects::ScriptShader::compiledOk() const
{ return r.wasOk(); }

void ScriptingObjects::ScriptShader::setCompileResult(Result compileResult)
{
	r = processErrorMessage(compileResult);

	for (auto f : includedFiles)
		f->setRuntimeErrors(r);
}

void ScriptingObjects::ScriptShader::setGlobalBounds(Rectangle<int> b, float sf)
{
	globalRect = b.toFloat();
	scaleFactor = sf;
}

bool ScriptingObjects::ScriptShader::shouldWriteToBuffer() const
{
	return enableCache || screenshotPending;
}

void ScriptingObjects::ScriptShader::renderWasFinished(ScreenshotListener::CachedImageBuffer::Ptr newData)
{
	if (screenshotPending)
	{
		DBG("REPAINT DONE");
		screenshotPending = false;
		lastScreenshot = newData;
	}
	else
		lastScreenshot = nullptr;
}

ScreenshotListener::CachedImageBuffer::Ptr ScriptingObjects::ScriptShader::getScreenshotBuffer()
{
	if (isRenderingScreenshot())
		return lastScreenshot;

	return nullptr;
}

bool ScriptingObjects::ScriptShader::isRenderingScreenshot()
{ return renderingScreenShot; }

ScriptingObjects::ScriptShader::ScopedScreenshotRenderer::ScopedScreenshotRenderer()
{
	renderingScreenShot = true;
}

ScriptingObjects::ScriptShader::ScopedScreenshotRenderer::~ScopedScreenshotRenderer()
{
	renderingScreenShot = false;
}

bool ScriptingObjects::ScriptShader::renderingScreenShot = false;

String ScriptingObjects::ScriptShader::getHeader()
{
	String s;

	s << "uniform float uScale;";
	s << "uniform float iTime;";
	s << "uniform vec2 iMouse;";
	s << "uniform vec2 uOffset;";
	s << "uniform vec3 iResolution;";
	s << "";
	s << "vec2 _gl_fc()";
	s << "{";
	s << "vec2 p = vec2(pixelPos.x + uOffset.x,";
	s << "	pixelPos.y + uOffset.y) / uScale;";
	s << "p.y = iResolution.y - p.y;";
	s << "return p;";
	s << "}";
	s << "\n#define fragCoord _gl_fc()\n";
	s << "#define fragColor gl_FragColor\n";

#if USE_BACKEND
	// We'll make this dynamic so everybody can adjust it to their graphics card
	if (GET_HISE_SETTING(dynamic_cast<Processor*>(getScriptProcessor()), HiseSettings::Other::EnableShaderLineNumbers))
	{
		s << "#line 0 \"" << shaderName << "\" \n";
	}
#endif

	return s;
}



void ScriptingObjects::ScriptShader::setFragmentShader(String shaderFile)
{
	shaderName = shaderFile;

	FileParser p(getScriptProcessor(), useLineNumbers, shaderFile, includedFiles);

	compileRawCode(p.getLines().joinIntoString("\n"));

	
}

void ScriptingObjects::ScriptShader::setUniformData(const String& id, var data)
{
	uniformData.set(Identifier(id), data);
}

void ScriptingObjects::ScriptShader::setBlendFunc(bool enabled, int sFactor, int dFactor)
{
	enableBlending = enabled;
	src = (BlendMode)sFactor;
	dst = (BlendMode)dFactor;
}

String ScriptingObjects::ScriptShader::toBase64()
{
	zstd::ZDefaultCompressor comp;
	
	MemoryBlock mb;
	comp.compress(compiledCode, mb);
	return mb.toBase64Encoding();
}

void ScriptingObjects::ScriptShader::fromBase64(String b64)
{
	zstd::ZDefaultCompressor comp;

	MemoryBlock mb;
	if (mb.fromBase64Encoding(b64))
	{
		String c;
		comp.expand(mb, c);
		compileRawCode(c);
	}
}

var ScriptingObjects::ScriptShader::getOpenGLStatistics()
{
	return openGLStats;
}

void ScriptingObjects::ScriptShader::setEnableCachedBuffer(bool shouldEnableBuffer)
{
	enableCache = shouldEnableBuffer;
}



void ScriptingObjects::ScriptShader::setPreprocessor(String preprocessorString, var value)
{
	if (preprocessorString.isEmpty())
		preprocessors.clear();
	else
		preprocessors.set(Identifier(preprocessorString), value);

	compileRawCode(compiledCode);
}

int ScriptingObjects::ScriptShader::blockWhileWaiting()
{
	if (screenshotPending)
	{
		DBG("BLOCK UNTIL REPAINTED");

		auto start = Time::getMillisecondCounter();

		int delta = 0;

		while (screenshotPending)
		{
			auto now = Time::getMillisecondCounter();

			delta = now - start;

			if (delta > 2000)
				break;

			Thread::getCurrentThread()->wait(200);
		}

		return delta;
	}

	return 0;
}

void ScriptingObjects::ScriptShader::prepareScreenshot()
{
	if (compiledOk() && enableCache)
	{
		screenshotPending = true;
	}
	else
		screenshotPending = false;
}

void ScriptingObjects::ScriptShader::makeStatistics()
{
	using namespace juce::gl;

	auto d = new DynamicObject();

	int major = 0, minor = 0;
	
    if(OpenGLContext::getCurrentContext() == nullptr)
    {
        d->setProperty("VersionString", "0.0");
        d->setProperty("Major", minor);
        d->setProperty("Minor", major);
        d->setProperty("Vendor", "Inactive");
        d->setProperty("Renderer", "Inactive");
        d->setProperty("GLSL Version", "0.0.0");
                       
        openGLStats = var(d);
        return;
    }
    
	auto vendor = String((const char*)glGetString(GL_VENDOR));
    jassert(glGetError() == GL_NO_ERROR);
    
	auto renderer = String((const char*)glGetString(GL_RENDERER));
    jassert(glGetError() == GL_NO_ERROR);
    
	auto version = String((const char*)glGetString(GL_VERSION));
    jassert(glGetError() == GL_NO_ERROR);
    
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    auto ok1 = glGetError();
    
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    auto ok2 = glGetError();
    
	auto shaderVersion = OpenGLShaderProgram::getLanguageVersion();
    jassert(glGetError() == GL_NO_ERROR);
    
    // Apparently this fails on older macs, so we need to parse the version integers from the string
    if(ok1 != GL_NO_ERROR || ok2 != GL_NO_ERROR)
    {
        const auto v = version.upToFirstOccurrenceOf(" ", false, false);
        
        major = v.upToFirstOccurrenceOf(".", false, false).getIntValue();
        minor = v.fromFirstOccurrenceOf(".", false, false).getIntValue();
    }
    
    
    
	d->setProperty("VersionString", version);
	d->setProperty("Major", major);
	d->setProperty("Minor", minor);
	d->setProperty("Vendor", vendor);
	d->setProperty("Renderer", renderer);
	d->setProperty("GLSL Version", shaderVersion);

	openGLStats = var(d);
}

Component* ScriptingObjects::ScriptShader::createPopupComponent(const MouseEvent& e, Component* componentToNotify)
{
#if USE_BACKEND
	return new PreviewComponent(this);
#else
	ignoreUnused(e, componentToNotify);
	return nullptr;
#endif
}

String ScriptingObjects::ScriptShader::getErrorMessage(bool verbose) const
{
	if (verbose)
	{
		String s;

		auto lines = StringArray::fromLines(r.getErrorMessage());
		lines.removeEmptyStrings();

		for (const auto& l : lines)
		{
			s << l;
			s << "{GLSL::";
			s << dynamic_cast<const Processor*>(getScriptProcessor())->getId() << "::" << shaderName << "}\n";
		}
		
		return s;
	}
	else
	return r.getErrorMessage();
}

void ScriptingObjects::ScriptShader::compileRawCode(const String& code)
{
	compiledCode = code;

	shaderCode = {};
	
	for (auto& p : preprocessors)
	{
		shaderCode << "#define " << p.name << " " << p.value.toString() << "\n";
	}

	shaderCode << getHeader();

	shaderCode << compiledCode;

	shader = new OpenGLGraphicsContextCustomShader(shaderCode);

	WeakReference<ScriptShader> safeShader(this);

	iTime = Time::getMillisecondCounterHiRes();

	shader->onShaderActivated = [safeShader](juce::OpenGLShaderProgram& pr)
	{
		if (safeShader == nullptr)
			return;

		auto thisTime = (float)(Time::getMillisecondCounterHiRes() - safeShader->iTime) * 0.001f;

		auto lr = safeShader->localRect;
		auto gr = safeShader->globalRect;

		auto scale = safeShader->scaleFactor;

		auto makeArray = [](var v1, var v2, var v3 = {})
		{
			Array<var> l;
			l.add(v1);
			l.add(v2);
			if (!v3.isVoid())
				l.add(v3);

			return var(l);
		};

		safeShader->uniformData.set("iTime", thisTime);
		safeShader->uniformData.set("uOffset", makeArray(gr.getX() - lr.getX() * scale, gr.getY() - lr.getY() * scale));
		safeShader->uniformData.set("iResolution", makeArray(lr.getWidth(), lr.getHeight(), 1.0f));
		safeShader->uniformData.set("uScale", scale);

		for (const auto& p : safeShader->uniformData)
		{
			const auto& v = p.value;

			auto name = p.name.toString().getCharPointer().getAddress();

			if (v.isArray())
			{
				if (v.getArray()->size() == 2) // vec2
					pr.setUniform(name, (float)v[0], (float)v[1]);
				if (v.getArray()->size() == 3) // vec3
					pr.setUniform(name, (float)v[0], (float)v[1], (float)v[2]);
				if (v.getArray()->size() == 4) // vec4
					pr.setUniform(name, (float)v[0], (float)v[1], (float)v[2], (float)v[3]);
			}
			if (v.isDouble()) // single value
				pr.setUniform(name, (float)v);
			if (v.isInt() || v.isInt64())
			{
				auto u = (int64)v;
				auto u_ = *reinterpret_cast<GLint*>(&u);
				pr.setUniform(name, u_);
			}
			if (v.isBuffer()) // static float array
				pr.setUniform(name, v.getBuffer()->buffer.getReadPointer(0), v.getBuffer()->size);
		}
	};

	dirty = true;
}

Result ScriptingObjects::ScriptShader::processErrorMessage(const Result& r)
{
	return r;
}

ScriptingObjects::SVGObject::SVGObject(ProcessorWithScriptingContent* p, const String& b64):
  ConstScriptingObject(p, 0)
{
    zstd::ZDefaultCompressor comp;
    
    MemoryBlock mb;
    mb.fromBase64Encoding(b64);
    
    String xmlText;
    
    comp.expand(mb, xmlText);
    
	WeakReference<SVGObject> safeThis(this);

	SafeAsyncCall::call<SVGObject>(*this, [xmlText](SVGObject& obj)
	{
		if (auto xml = XmlDocument::parse(xmlText))
			obj.svg = Drawable::createFromSVG(*xml);
	});
}


class PathPreviewComponent : public Component,
							 public ComponentForDebugInformation
{
public:

	PathPreviewComponent(ScriptingObjects::PathObject* o):
		ComponentForDebugInformation(o, dynamic_cast<ApiProviderBase::Holder*>(o->getScriptProcessor())),
		resizer(this, nullptr)
	{ 
		addAndMakeVisible(resizer);
		setName(getTitle());
		setSize(300, 300); 
	}

	void paint(Graphics &g) override
	{
		if (auto p = getObject<ScriptingObjects::PathObject>())
		{
			auto path = p->getPath();
			auto b = getLocalBounds().reduced(20).toFloat();

			auto pathBounds = path.getBounds();

			PathFactory::scalePath(path, b);

			g.setColour(Colours::white.withAlpha(0.5f));
			g.fillPath(path);
			g.setColour(Colours::white.withAlpha(0.8f));
			g.strokePath(path, PathStrokeType(2.0f));

			b = path.getBounds();

			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawRect(b.toFloat(), 1.0f);

			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.3f));

			b.expand(20.0f, 20.0f);

			g.drawText(pathBounds.getTopLeft().toString(), b, Justification::topLeft);
			g.drawText(pathBounds.getTopRight().toString(), b, Justification::topRight);
			g.drawText(pathBounds.getBottomLeft().toString(), b, Justification::bottomLeft);
			g.drawText(pathBounds.getBottomRight().toString(), b, Justification::bottomRight);
		}
		else
		{
			g.drawText("Path object " + getTitle() + " does not exist", getLocalBounds().toFloat(), Justification::centred);
		}
	}

	void resized() override
	{
		resizer.setBounds(getLocalBounds().removeFromRight(15).removeFromBottom(15));
	}

private:

	Path p;
	juce::ResizableCornerComponent resizer;
};

Component* ScriptingObjects::PathObject::createPopupComponent(const MouseEvent &e, Component* componentToNotify)
{
#if USE_BACKEND
	return new PathPreviewComponent(this);
#else
	ignoreUnused(e, componentToNotify);
	return nullptr;
#endif
}


struct ScriptingObjects::PathObject::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(PathObject, loadFromData);
	API_VOID_METHOD_WRAPPER_0(PathObject, closeSubPath);
	API_VOID_METHOD_WRAPPER_2(PathObject, startNewSubPath);
	API_VOID_METHOD_WRAPPER_2(PathObject, lineTo);
	API_VOID_METHOD_WRAPPER_0(PathObject, clear);
	API_VOID_METHOD_WRAPPER_4(PathObject, quadraticTo);
	API_VOID_METHOD_WRAPPER_4(PathObject, cubicTo);
	API_VOID_METHOD_WRAPPER_4(PathObject, addQuadrilateral);
	API_VOID_METHOD_WRAPPER_3(PathObject, addArc);
	API_VOID_METHOD_WRAPPER_1(PathObject, addEllipse);
	API_VOID_METHOD_WRAPPER_1(PathObject, addRectangle);
    API_VOID_METHOD_WRAPPER_2(PathObject, addRoundedRectangle);
    API_VOID_METHOD_WRAPPER_3(PathObject, addRoundedRectangleCustomisable);
	API_VOID_METHOD_WRAPPER_3(PathObject, addTriangle);
	API_VOID_METHOD_WRAPPER_4(PathObject, addPolygon);
	API_VOID_METHOD_WRAPPER_5(PathObject, addArrow);
	API_VOID_METHOD_WRAPPER_5(PathObject, addStar);
	API_VOID_METHOD_WRAPPER_5(PathObject, scaleToFit);
	API_VOID_METHOD_WRAPPER_1(PathObject, roundCorners);
	API_METHOD_WRAPPER_2(PathObject, createStrokedPath);
	API_METHOD_WRAPPER_3(PathObject, getIntersection);
	API_METHOD_WRAPPER_1(PathObject, getPointOnPath);
	API_METHOD_WRAPPER_1(PathObject, contains);
	API_METHOD_WRAPPER_1(PathObject, getBounds);
	API_METHOD_WRAPPER_0(PathObject, getLength);
	API_METHOD_WRAPPER_0(PathObject, toString);
	API_METHOD_WRAPPER_0(PathObject, toBase64);
	API_VOID_METHOD_WRAPPER_1(PathObject, fromString);
};

ScriptingObjects::PathObject::PathObject(ProcessorWithScriptingContent* p) :
	ConstScriptingObject(p, 0)
{
	ADD_API_METHOD_1(loadFromData);
	ADD_API_METHOD_0(closeSubPath);
	ADD_API_METHOD_0(clear);
	ADD_API_METHOD_2(startNewSubPath);
	ADD_API_METHOD_2(lineTo);
	ADD_API_METHOD_4(quadraticTo);
	ADD_API_METHOD_4(cubicTo);
	ADD_API_METHOD_4(addQuadrilateral);
	ADD_API_METHOD_3(addArc);
	ADD_API_METHOD_1(addEllipse);
	ADD_API_METHOD_1(addRectangle);
    ADD_API_METHOD_2(addRoundedRectangle);
    ADD_API_METHOD_3(addRoundedRectangleCustomisable);
	ADD_API_METHOD_3(addTriangle);
	ADD_API_METHOD_4(addPolygon);
	ADD_API_METHOD_5(addArrow);
	ADD_API_METHOD_5(addStar);
	ADD_API_METHOD_5(scaleToFit);
	ADD_API_METHOD_1(roundCorners);
	ADD_API_METHOD_1(getPointOnPath);
	ADD_API_METHOD_3(getIntersection);
	ADD_API_METHOD_1(contains);
	ADD_API_METHOD_1(getBounds);
	ADD_API_METHOD_0(getLength);
	ADD_API_METHOD_2(createStrokedPath);
	ADD_API_METHOD_0(toString);
	ADD_API_METHOD_0(toBase64);
	ADD_API_METHOD_1(fromString);
}

ScriptingObjects::PathObject::~PathObject()
{

}


void ScriptingObjects::PathObject::loadFromData(var data)
{
	ApiHelpers::loadPathFromData(p, data);

	
}

void ScriptingObjects::PathObject::clear()
{
	p.clear();
}

void ScriptingObjects::PathObject::startNewSubPath(var x, var y)
{
	auto x_ = (float)x;
	auto y_ = (float)y;

	p.startNewSubPath(SANITIZED(x_), SANITIZED(y_));
}

void ScriptingObjects::PathObject::closeSubPath()
{
	p.closeSubPath();
}

void ScriptingObjects::PathObject::lineTo(var x, var y)
{
	auto x_ = (float)x;
	auto y_ = (float)y;

	p.lineTo(SANITIZED(x_), SANITIZED(y_));
}

void ScriptingObjects::PathObject::quadraticTo(var cx, var cy, var x, var y)
{
	p.quadraticTo(cx, cy, x, y);
}

void ScriptingObjects::PathObject::cubicTo(var cxy1, var cxy2, var x, var y)
{
	p.cubicTo(cxy1[0], cxy1[1], cxy2[0], cxy2[1], x, y);
}

void ScriptingObjects::PathObject::addQuadrilateral(var xy1, var xy2, var xy3, var xy4)
{
	p.addQuadrilateral(xy1[0], xy1[1], xy2[0], xy2[1], xy3[0], xy3[1], xy4[0], xy4[1]);
}

void ScriptingObjects::PathObject::addArc(var area, var fromRadians, var toRadians)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);

	auto fr = (float)fromRadians;
	auto tr = (float)toRadians;

	p.addArc(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), SANITIZED(fr), SANITIZED(tr), true);
}

void ScriptingObjects::PathObject::addEllipse(var area)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);

	p.addEllipse(rect);
}

void ScriptingObjects::PathObject::addRectangle(var area)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);
	
	p.addRectangle(rect);
}

void ScriptingObjects::PathObject::addRoundedRectangle(var area, var cornerSize)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);
	
	p.addRoundedRectangle(rect, cornerSize);
}

void ScriptingObjects::PathObject::addRoundedRectangleCustomisable(var area, var cornerSizeXY, var boolCurves)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);
	
	p.addRoundedRectangle(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), cornerSizeXY[0], cornerSizeXY[1], boolCurves[0], boolCurves[1], boolCurves[2], boolCurves[3]);
}

void ScriptingObjects::PathObject::addTriangle(var xy1, var xy2, var xy3)
{
	p.addTriangle(xy1[0], xy1[1], xy2[0], xy2[1], xy3[0], xy3[1]);
}

void ScriptingObjects::PathObject::addPolygon(var center, var numSides, var radius, var angle)
{
    Point<float> c = ApiHelpers::getPointFromVar(center);

	p.addPolygon(c, numSides, radius, angle);
}

void ScriptingObjects::PathObject::addArrow(var start, var end, var thickness, var headWidth, var headLength)
{
    Point<float> p1 = ApiHelpers::getPointFromVar(start);
    Point<float> p2 = ApiHelpers::getPointFromVar(end);

    auto l = Line<float>(p1, p2);

	p.addArrow(l, thickness, headWidth, headLength);
}

void ScriptingObjects::PathObject::addStar(var center, var numPoints, var innerRadius, var outerRadius, var angle)
{
    Point<float> c = ApiHelpers::getPointFromVar(center);

	p.addStar(c, numPoints, innerRadius, outerRadius, angle);
}

void ScriptingObjects::PathObject::scaleToFit(var x, var y, var width, var height, bool preserveProportions)
{
	p.applyTransform(p.getTransformToScaleToFit(x, y, width, height, preserveProportions));
}

void ScriptingObjects::PathObject::roundCorners(var radius)
{
	p = p.createPathWithRoundedCorners(radius);
}

var ScriptingObjects::PathObject::getIntersection(var start, var end, bool keepSectionOutsidePath)
{
	Point<float> p1 = ApiHelpers::getPointFromVar(start);
	Point<float> p2 = ApiHelpers::getPointFromVar(end);

	Line<float> l(p1.getX(), p1.getY() - 0.001f, p2.getX(), p2.getY()); // -0.001f ensures the line starts inside the path boundaries... Hmmm...

	if (p.intersectsLine(l))
	{
		Line<float> clippedLine = p.getClippedLine(l, keepSectionOutsidePath);

		Array<var> a;

		if (keepSectionOutsidePath)
		{
			a.add(clippedLine.getStartX());
			a.add(clippedLine.getStartY());
		}
		else
		{
			a.add(clippedLine.getEndX());
			a.add(clippedLine.getEndY());
		}

		return var(a);
	}

	return false;
}

var ScriptingObjects::PathObject::getPointOnPath(var distanceFromStart)
{
	Point<float> point = p.getPointAlongPath((float)distanceFromStart);

	Array<var> a;

	a.add(point.getX());
	a.add(point.getY());

	return var(a);
}

var ScriptingObjects::PathObject::contains(var point)
{
	Point<float> pt = ApiHelpers::getPointFromVar(point);

	return p.contains(pt);
}

var ScriptingObjects::PathObject::getLength()
{
	return p.getLength(AffineTransform::scale(1.0f), 1.0f);
}

var ScriptingObjects::PathObject::getBounds(var scaleFactor)
{
	auto r = p.getBoundsTransformed(AffineTransform::scale(scaleFactor));

	Array<var> area;

	area.add(r.getX());
	area.add(r.getY());
	area.add(r.getWidth());
	area.add(r.getHeight());

	return var(area);
}

juce::var ScriptingObjects::PathObject::createStrokedPath(var strokeData, var dotData)
{
	auto s = ApiHelpers::createPathStrokeType(strokeData);

	auto np = new PathObject(getScriptProcessor());

	if (dotData.isArray())
	{
		auto& ar = *dotData.getArray();

		if (!ar.isEmpty())
		{
			Array<float> dashes;

			for (auto& a : ar)
				dashes.add((float)a);

			s.createDashedStroke(np->p, p, dashes.begin(), dashes.size());

			np->p.startNewSubPath(p.getBounds().getTopLeft());
			np->p.startNewSubPath(p.getBounds().getBottomRight());

			return var(np);
		}
	}
	
	s.createStrokedPath(np->p, p);
	
	np->p.startNewSubPath(p.getBounds().getTopLeft());
	np->p.startNewSubPath(p.getBounds().getBottomRight());

	return var(np);
}

String ScriptingObjects::PathObject::toString()
{
	return p.toString();
}

String ScriptingObjects::PathObject::toBase64()
{
	MemoryOutputStream mos;
	p.writePathToStream(mos);
	return mos.getMemoryBlock().toBase64Encoding();
}

void ScriptingObjects::PathObject::fromString(String stringPath)
{
	p.restoreFromString(stringPath);
}



class ScriptingObjects::MarkdownObject::Preview : public Component,
												  public ComponentForDebugInformation,
												  public PooledUIUpdater::SimpleTimer
{
public:

	Preview(ScriptingObjects::MarkdownObject* obj) :
		ComponentForDebugInformation(obj, dynamic_cast<ApiProviderBase::Holder*>(obj->getScriptProcessor())),
		SimpleTimer(obj->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
	{
		if (auto o = getObject<ScriptingObjects::MarkdownObject>())
		{
			auto b = o->obj->area.toNearestInt();

			if (b.isEmpty())
				setSize(200, 400);
			else
				setSize(b.getWidth(), b.getHeight());
		}
	}

	void timerCallback() override
	{
		auto b = getLocalBounds();

		if (auto o = getObject<ScriptingObjects::MarkdownObject>())
		{
			auto tb = o->obj->area.toNearestInt();

			tb.setPosition(0, 0);

			if (b != tb)
			{
				setSize(tb.getWidth(), tb.getHeight());
				repaint();
			}
		}
	}

	void paint(Graphics& g) override
	{
		if (auto o = getObject<ScriptingObjects::MarkdownObject>())
		{
			o->obj->perform(g);
		}
	}

	DrawActions::MarkdownAction::Ptr obj;
};

class ScriptingObjects::MarkdownObject::ScriptedImageProvider : public MarkdownParser::ImageProvider,
															    public ControlledObject
{
public:

	struct Entry
	{
		Entry(var data)
		{
			auto urlString = data.getProperty("URL", "").toString();

			if (urlString.isNotEmpty())
				url = MarkdownLink::createWithoutRoot(MarkdownLink::Helpers::getSanitizedURL(urlString), MarkdownLink::Image);
		}
			
		virtual ~Entry() {};

		Image getImage(const MarkdownLink& urlToResolve, float width)
		{
			if (url.toString(MarkdownLink::UrlWithoutAnchor) == urlToResolve.toString(MarkdownLink::UrlWithoutAnchor))
			{
				MarkdownParser::ImageProvider::updateWidthFromURL(urlToResolve, width);
				auto img = getImageInternal(width);
				return MarkdownParser::ImageProvider::resizeImageToFit(img, width);
			}

			return {};
		}

		virtual Image getImageInternal(float width) = 0;

		MarkdownLink url;
	};

	struct PathEntry : public Entry
	{
		PathEntry(var data):
			Entry(data)
		{
			jassert(data.getProperty("Type", "").toString() == "Path");

			var pathData = data.getProperty("Data", var());

			ApiHelpers::loadPathFromData(p, pathData);
			c = scriptnode::PropertyHelpers::getColourFromVar(data.getProperty("Colour", 0xFF888888));
		}

		Image getImageInternal(float width) override
		{
			Image img(Image::ARGB, (int)width, (int)width, true);
			Graphics g2(img);
			g2.setColour(c);
			PathFactory::scalePath(p, { 0.0f, 0.0f, width, width });
			g2.fillPath(p);
			return img;
		}

		Path p;
		Colour c;
	};

	struct ImageEntry: public ControlledObject,
					   public Entry
	{
		ImageEntry(MainController* mc, var data) :
			ControlledObject(mc),
			Entry(data)
		{
			auto link = data.getProperty("Reference", "").toString();

			if (link.isNotEmpty())
			{
				PoolReference ref(getMainController(), link, FileHandlerBase::Images);
				pooledImage = getMainController()->getCurrentImagePool()->loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
			}
		};

		Image getImageInternal(float width) override
		{
			if (pooledImage)
				return *pooledImage.getData();

			return {};
		}

		ImagePool::ManagedPtr pooledImage;
	};

	OwnedArray<Entry> entries;

	ScriptedImageProvider(MainController* mc, MarkdownParser* parent, var data_) :
		ImageProvider(parent),
		ControlledObject(mc),
		data(data_)
	{
		if (data.isArray())
		{
			for (auto v : *data.getArray())
			{
				auto isPath = v.getProperty("Type", "").toString() == "Path";

				if (isPath)
					entries.add(new PathEntry(v));
				else
					entries.add(new ImageEntry(mc, v));
			}
		}
	};

	MarkdownParser::ResolveType getPriority() const override { return MarkdownParser::ResolveType::EmbeddedPath; };

	ImageProvider* clone(MarkdownParser* newParser) const override { return new ScriptedImageProvider(const_cast<MainController*>(getMainController()), newParser, data); }
	Identifier getId() const override { RETURN_STATIC_IDENTIFIER("ScriptedImageProvider"); };

	Image getImage(const MarkdownLink& urlName, float width) override
	{
		for (auto e : entries)
		{
			auto img = e->getImage(urlName, width);

			if (img.isValid())
				return img;
		}

		jassertfalse;
		return {};
	}

	var data;
};

struct ScriptingObjects::MarkdownObject::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(MarkdownObject, setText);
	API_VOID_METHOD_WRAPPER_1(MarkdownObject, setStyleData);
	API_VOID_METHOD_WRAPPER_1(MarkdownObject, setImageProvider);
	API_METHOD_WRAPPER_1(MarkdownObject, setTextBounds);
	API_METHOD_WRAPPER_0(MarkdownObject, getStyleData);
};

ScriptingObjects::MarkdownObject::MarkdownObject(ProcessorWithScriptingContent* pwsc) :
	ConstScriptingObject(pwsc, 0),
	obj(new DrawActions::MarkdownAction(std::bind(&MainController::getStringWidthFloat, pwsc->getMainController_(), std::placeholders::_1, std::placeholders::_2)))
{
	ADD_API_METHOD_1(setText);
	ADD_API_METHOD_1(setStyleData);
	ADD_API_METHOD_1(setTextBounds);
	ADD_API_METHOD_0(getStyleData);
	ADD_API_METHOD_1(setImageProvider);
}



Component* ScriptingObjects::MarkdownObject::createPopupComponent(const MouseEvent& e, Component *c)
{
#if USE_BACKEND
	return new Preview(this);
#else
	ignoreUnused(e, c);
	return nullptr;
#endif
}

void ScriptingObjects::MarkdownObject::setText(const String& markdownText)
{
	ScopedLock sl(obj->lock);
	obj->renderer.setNewText(markdownText);
}

float ScriptingObjects::MarkdownObject::setTextBounds(var area)
{
	auto r = Result::ok();
	obj->area = ApiHelpers::getRectangleFromVar(area, &r);

	if (r.failed())
		reportScriptError(r.getErrorMessage());

	ScopedLock sl(obj->lock);
	return obj->renderer.getHeightForWidth(obj->area.getWidth());
}

void ScriptingObjects::MarkdownObject::setStyleData(var styleData)
{
	MarkdownLayout::StyleData s;

	auto mc = getScriptProcessor()->getMainController_();

	s.fromDynamicObject(styleData, [mc](const String& n)
	{
		return mc->getFontFromString(n, 14.0f);
	});

	ScopedLock sl(obj->lock);
	obj->renderer.setStyleData(s);
}

juce::var ScriptingObjects::MarkdownObject::getStyleData()
{
	ScopedLock sl(obj->lock);
	return obj->renderer.getStyleData().toDynamicObject();
}

void ScriptingObjects::MarkdownObject::setImageProvider(var data)
{
	auto newProvider = new ScriptedImageProvider(getScriptProcessor()->getMainController_(), &obj->renderer, data);

	ScopedLock sl(obj->lock);
	obj->renderer.clearResolvers();
	obj->renderer.setImageProvider(newProvider);
}

struct ScriptingObjects::GraphicsObject::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillAll);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setColour);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setOpacity);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillRect);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawRect);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawRoundedRectangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillRoundedRectangle);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawLine);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawHorizontalLine);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawVerticalLine);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, setFont);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, setFontWithSpacing);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawText);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawAlignedText);
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, drawAlignedTextShadow);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawFittedText);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawMultiLineText);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, drawMarkdownText);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setGradientFill);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawEllipse);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillEllipse);
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, drawImage);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, drawRepaintMarker);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawDropShadow);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawDropShadowFromPath);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, addDropShadowFromAlpha);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillPath);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawPath);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawFFTSpectrum);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, rotate);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, gaussianBlur);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, boxBlur);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, desaturate);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, addNoise);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyMask);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, beginLayer);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, endLayer);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, beginBlendLayer);
    API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawSVG);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyHSL);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, applyGamma);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, applyGradientMap);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, applySharpness);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, applySepia);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyVignette);
	API_METHOD_WRAPPER_2(GraphicsObject, applyShader);
    API_VOID_METHOD_WRAPPER_2(GraphicsObject, flip);
	API_METHOD_WRAPPER_1(GraphicsObject, getStringWidth);
};

ScriptingObjects::GraphicsObject::GraphicsObject(ProcessorWithScriptingContent *p, ConstScriptingObject* parent_) :
	ConstScriptingObject(p, 0),
	parent(parent_),
	rectangleResult(Result::ok())
{
	ADD_API_METHOD_1(fillAll);
	ADD_API_METHOD_1(setColour);
	ADD_API_METHOD_1(setOpacity);
	ADD_API_METHOD_2(drawRect);
	ADD_API_METHOD_1(fillRect);
	ADD_API_METHOD_3(drawRoundedRectangle);
	ADD_API_METHOD_2(fillRoundedRectangle);
	ADD_API_METHOD_5(drawLine);
	ADD_API_METHOD_3(drawHorizontalLine);
	ADD_API_METHOD_3(drawVerticalLine);
	ADD_API_METHOD_2(setFont);
	ADD_API_METHOD_3(setFontWithSpacing);
	ADD_API_METHOD_2(drawText);
	ADD_API_METHOD_3(drawAlignedText);
	ADD_API_METHOD_4(drawAlignedTextShadow);
	ADD_API_METHOD_5(drawFittedText);
	ADD_API_METHOD_5(drawMultiLineText);
	ADD_API_METHOD_1(drawMarkdownText);
    ADD_API_METHOD_3(drawSVG);
	ADD_API_METHOD_1(setGradientFill);
	ADD_API_METHOD_2(drawEllipse);
	ADD_API_METHOD_1(fillEllipse);
	ADD_API_METHOD_4(drawImage);
	ADD_API_METHOD_3(drawDropShadow);
	ADD_API_METHOD_5(drawDropShadowFromPath);
	ADD_API_METHOD_2(addDropShadowFromAlpha);
	ADD_API_METHOD_3(drawTriangle);
	ADD_API_METHOD_2(fillTriangle);
	ADD_API_METHOD_2(fillPath);
	
	ADD_API_METHOD_3(drawPath);
	ADD_API_METHOD_2(rotate);
	ADD_API_METHOD_2(drawFFTSpectrum);

	ADD_API_METHOD_1(beginLayer);
	ADD_API_METHOD_1(gaussianBlur);
	ADD_API_METHOD_1(boxBlur);
	ADD_API_METHOD_0(desaturate);
	ADD_API_METHOD_1(addNoise);
	ADD_API_METHOD_3(applyMask);
    ADD_API_METHOD_2(flip);

	ADD_API_METHOD_3(applyHSL);
	ADD_API_METHOD_1(applyGamma);
	ADD_API_METHOD_2(applyGradientMap);
	ADD_API_METHOD_1(applySharpness);
	ADD_API_METHOD_0(applySepia);
	ADD_API_METHOD_3(applyVignette);
	ADD_API_METHOD_2(applyShader);

	ADD_API_METHOD_0(endLayer);
	ADD_API_METHOD_2(beginBlendLayer);
	ADD_API_METHOD_1(getStringWidth);
	ADD_API_METHOD_1(drawRepaintMarker);

	WeakReference<Processor> safeP(dynamic_cast<Processor*>(p));

	drawActionHandler.errorLogger = [safeP](const String& m)
	{
		if (safeP.get() != nullptr)
			debugError(safeP.get(), m);
	};
}

ScriptingObjects::GraphicsObject::~GraphicsObject()
{
	parent = nullptr;
}

void ScriptingObjects::GraphicsObject::beginLayer(bool drawOnParent)
{
	drawActionHandler.beginLayer(drawOnParent);
}

void ScriptingObjects::GraphicsObject::beginBlendLayer(String blendMode, float alpha)
{
	drawActionHandler.beginBlendLayer(Identifier(blendMode), alpha);
}

void ScriptingObjects::GraphicsObject::endLayer()
{
	drawActionHandler.endLayer();
}

void ScriptingObjects::GraphicsObject::gaussianBlur(var blurAmount)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		cl->addPostAction(new ScriptedPostDrawActions::guassianBlur(jlimit(0, 100, (int)blurAmount)));
	}
	else
		reportScriptError("You need to create a layer for gaussian blur");
}

void ScriptingObjects::GraphicsObject::boxBlur(var blurAmount)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		cl->addPostAction(new ScriptedPostDrawActions::boxBlur(jlimit(0, 100, (int)blurAmount)));
	}
	else
		reportScriptError("You need to create a layer for box blur");
}

void ScriptingObjects::GraphicsObject::applyHSL(float hue, float saturation, float lightness)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applyHSL(hue, saturation, lightness));
	else
		reportScriptError("You need to create a layer for applying HSL");
}

void ScriptingObjects::GraphicsObject::applyGamma(float gamma)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applyGamma(gamma));
	else
		reportScriptError("You need to create a layer for applying gamma");
}

void ScriptingObjects::GraphicsObject::applyGradientMap(var darkColour, var brightColour)
{
	auto c1 = ScriptingApi::Content::Helpers::getCleanedObjectColour(darkColour);
	auto c2 = ScriptingApi::Content::Helpers::getCleanedObjectColour(brightColour);

	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applyGradientMap(c1, c2));
	else
		reportScriptError("You need to create a layer for applyGradientMap");
}

void ScriptingObjects::GraphicsObject::applySharpness(int delta)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applySharpness(delta));
	else
		reportScriptError("You need to create a layer for applySharpness");
}

void ScriptingObjects::GraphicsObject::applySepia()
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applySepia());
	else
		reportScriptError("You need to create a layer for applySepia");
}

void ScriptingObjects::GraphicsObject::applyVignette(float amount, float radius, float falloff)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::applyVignette(amount, radius, falloff));
	else
		reportScriptError("You need to create a layer for applySepia");
}

void ScriptingObjects::GraphicsObject::addNoise(var noiseAmount)
{
    auto m = drawActionHandler.getNoiseMapManager();

	Rectangle<int> ar;

	if (auto sc = dynamic_cast<ScriptComponent*>(parent))
	{
		ar = Rectangle<int>(0, 0, (int)sc->getScriptObjectProperty(ScriptComponent::Properties::width), (int)sc->getScriptObjectProperty(ScriptComponent::Properties::height));
	}

	if (noiseAmount.isDouble())
	{
		if (ar.isEmpty())
			reportScriptError("No valid area for noise map specified");
		else
			drawActionHandler.addDrawAction(new ScriptedPostDrawActions::addNoise(m, jlimit(0.0f, 1.0f, (float)noiseAmount), ar));
	}
	else if (auto obj = noiseAmount.getDynamicObject())
	{
		auto alpha = jlimit(0.0f, 1.0f, (float)noiseAmount["alpha"]);
		auto monochrom = (bool)noiseAmount["monochromatic"];

		auto sf = (float)noiseAmount.getProperty("scaleFactor", 1.0);

		auto customArea = noiseAmount.getProperty("area", var());

		if (customArea.isArray())
		{
			ar = ApiHelpers::getIntRectangleFromVar(customArea);
		}

		if(ar.isEmpty())
			reportScriptError("Invalid area for noise map");
		else
		{
			if (sf == -1.0f)
				sf = drawActionHandler.getScaleFactor();

			auto scale = jlimit(0.125, 2.0, (double)sf);

			drawActionHandler.addDrawAction(new ScriptedPostDrawActions::addNoise(m, jlimit(0.0f, 1.0f, (float)alpha), ar, monochrom, scale));
		}
	}
}

void ScriptingObjects::GraphicsObject::desaturate()
{
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		cl->addPostAction(new ScriptedPostDrawActions::desaturate());
	}
	else
		reportScriptError("You need to create a layer for desaturating");
}

void ScriptingObjects::GraphicsObject::applyMask(var path, var area, bool invert)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
		{
			Path p = pathObject->getPath();

			Rectangle<float> r = getRectangleFromVar(area);
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);

			cl->addPostAction(new ScriptedPostDrawActions::applyMask(p, invert));
		}
		else
			reportScriptError("No valid path object supplied");
	}
	else
		reportScriptError("You need to create a layer for applying a mask");
}


void ScriptingObjects::GraphicsObject::fillAll(var colour)
{
	Colour c = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);
	drawActionHandler.addDrawAction(new ScriptedDrawActions::fillAll(c));
}

void ScriptingObjects::GraphicsObject::fillRect(var area)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::fillRect(getRectangleFromVar(area)));
}

void ScriptingObjects::GraphicsObject::drawRect(var area, float borderSize)
{
	auto bs = (float)borderSize;
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawRect(getRectangleFromVar(area), SANITIZED(bs)));
}

void ScriptingObjects::GraphicsObject::fillRoundedRectangle(var area, var cornerData)
{
	if (cornerData.isObject())
	{
		auto cs = (float)cornerData["CornerSize"];
		cs = SANITIZED(cs);

		auto newAction = new ScriptedDrawActions::fillRoundedRect(getRectangleFromVar(area), cs);
		auto ra = cornerData["Rounded"];

		if (ra.isArray())
		{
			newAction->allRounded = false;
			newAction->rounded[0] = ra[0];
			newAction->rounded[1] = ra[1];
			newAction->rounded[2] = ra[2];
			newAction->rounded[3] = ra[3];
		}

		drawActionHandler.addDrawAction(newAction);
	}
	else
	{
		auto cs = (float)cornerData;
		cs = SANITIZED(cs);
		drawActionHandler.addDrawAction(new ScriptedDrawActions::fillRoundedRect(getRectangleFromVar(area), cs));
	}
}

void ScriptingObjects::GraphicsObject::drawRoundedRectangle(var area, var cornerData, float borderSize)
{
	auto bs = SANITIZED(borderSize);
	auto ar = getRectangleFromVar(area);

	if (cornerData.isObject())
	{
		auto cs = (float)cornerData["CornerSize"];
		cs = SANITIZED(cs);

		auto newAction = new ScriptedDrawActions::drawRoundedRectangle(getRectangleFromVar(area), borderSize, cs);
		auto ra = cornerData["Rounded"];

		if (ra.isArray())
		{
			newAction->allRounded = false;
			newAction->rounded[0] = ra[0];
			newAction->rounded[1] = ra[1];
			newAction->rounded[2] = ra[2];
			newAction->rounded[3] = ra[3];
		}

		drawActionHandler.addDrawAction(newAction);
	}
	else
	{
		auto cs = (float)cornerData;
		cs = SANITIZED(cs);
		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawRoundedRectangle(ar, bs, cs));
	}
}

void ScriptingObjects::GraphicsObject::drawHorizontalLine(int y, float x1, float x2)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawHorizontalLine(y, SANITIZED(x1), SANITIZED(x2)));
}

void ScriptingObjects::GraphicsObject::drawVerticalLine(int x, float y1, float y2)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawVerticalLine(x, SANITIZED(y1), SANITIZED(y2)));
}

void ScriptingObjects::GraphicsObject::setOpacity(float alphaValue)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::setOpacity(alphaValue));
}

void ScriptingObjects::GraphicsObject::drawLine(float x1, float x2, float y1, float y2, float lineThickness)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawLine(
		SANITIZED(x1), SANITIZED(y1), SANITIZED(x2), SANITIZED(y2), SANITIZED(lineThickness)));
}

void ScriptingObjects::GraphicsObject::setColour(var colour)
{
	auto c = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);
	drawActionHandler.addDrawAction(new ScriptedDrawActions::setColour(c));
}

void ScriptingObjects::GraphicsObject::setFont(String fontName, float fontSize)
{
	MainController *mc = getScriptProcessor()->getMainController_();
	auto f = mc->getFontFromString(fontName, SANITIZED(fontSize));
	currentFont = f;
	currentFontName = fontName;
	currentKerningFactor = 0.0f;
	currentFontHeight = fontSize;
	drawActionHandler.addDrawAction(new ScriptedDrawActions::setFont(f));
}

void ScriptingObjects::GraphicsObject::setFontWithSpacing(String fontName, float fontSize, float spacing)
{
	MainController *mc = getScriptProcessor()->getMainController_();
	auto f = mc->getFontFromString(fontName, SANITIZED(fontSize));

	f.setExtraKerningFactor(spacing);
	currentFont = f;
	currentFontName = fontName;
	currentFontHeight = fontSize;
	currentKerningFactor = spacing;
	drawActionHandler.addDrawAction(new ScriptedDrawActions::setFont(f));
}

void ScriptingObjects::GraphicsObject::drawText(String text, var area)
{
	Rectangle<float> r = getRectangleFromVar(area);
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawText(text, r));
}

void ScriptingObjects::GraphicsObject::drawAlignedText(String text, var area, String alignment)
{
	Rectangle<float> r = getRectangleFromVar(area);

	Result re = Result::ok();
	auto just = ApiHelpers::getJustification(alignment, &re);

	if (re.failed())
		reportScriptError(re.getErrorMessage());

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawText(text, r, just));
}

void ScriptingObjects::GraphicsObject::drawAlignedTextShadow(String text, var area, String alignment, var shadowData)
{
	Rectangle<float> r = getRectangleFromVar(area);

	Result re = Result::ok();
	auto just = ApiHelpers::getJustification(alignment, &re);

	

	if (re.failed())
		reportScriptError(re.getErrorMessage());

	auto sp = ApiHelpers::getShadowParameters(shadowData, &re);

	if (re.failed())
		reportScriptError(re.getErrorMessage());

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawTextShadow(text, r, just, sp));
}

void ScriptingObjects::GraphicsObject::drawFittedText(String text, var area, String alignment, int maxLines, float scale)
{
	Result re = Result::ok();
	auto just = ApiHelpers::getJustification(alignment, &re);

	if (re.failed())
		reportScriptError(re.getErrorMessage());

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawFittedText(text, area, just, maxLines, scale));
}

void ScriptingObjects::GraphicsObject::drawMultiLineText(String text, var xy, int maxWidth, String alignment, float leading)
{
	Result re = Result::ok();
	auto just = ApiHelpers::getJustification(alignment, &re);

	if (re.failed())
		reportScriptError(re.getErrorMessage());
    
    int startX = (int)xy[0];
    int baseLineY = (int)xy[1];
    
    drawActionHandler.addDrawAction(new ScriptedDrawActions::drawMultiLineText(text, startX, baseLineY, maxWidth, just, leading));
}

void ScriptingObjects::GraphicsObject::drawMarkdownText(var markdownRenderer)
{
	if (auto obj = dynamic_cast<MarkdownObject*>(markdownRenderer.getObject()))
	{
		if (obj->obj->area.isEmpty())
			reportScriptError("You have to call setTextBounds() before using this method");

		drawActionHandler.addDrawAction(obj->obj.get());
	}
	else
		reportScriptError("not a markdown renderer");
}

void ScriptingObjects::GraphicsObject::drawFFTSpectrum(var fftObject, var area)
{
	if (auto obj = dynamic_cast<ScriptingObjects::ScriptFFT*>(fftObject.getObject()))
	{
		auto b = ApiHelpers::getRectangleFromVar(area);
		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawFFTSpectrum(obj->getSpectrum(false), b, obj->getParameters()->quality));
	}
	else
        reportScriptError("not a SVG object");
}

void ScriptingObjects::GraphicsObject::drawSVG(var svgObject, var bounds, float opacity)
{
    if (auto obj = dynamic_cast<SVGObject*>(svgObject.getObject()))
    {
        auto b = ApiHelpers::getRectangleFromVar(bounds);
        drawActionHandler.addDrawAction(new ScriptedDrawActions::drawSVG(svgObject, b, opacity));
    }
    else
        reportScriptError("not a SVG object");
}

void ScriptingObjects::GraphicsObject::setGradientFill(var gradientData)
{
	if (gradientData.isArray())
	{
		Array<var>* data = gradientData.getArray();

		if (gradientData.getArray()->size() == 6)
		{
			auto c1 = ScriptingApi::Content::Helpers::getCleanedObjectColour(data->getUnchecked(0));
			auto c2 = ScriptingApi::Content::Helpers::getCleanedObjectColour(data->getUnchecked(3));

			auto grad = ColourGradient(c1, (float)data->getUnchecked(1), (float)data->getUnchecked(2),
				c2, (float)data->getUnchecked(4), (float)data->getUnchecked(5), false);


			drawActionHandler.addDrawAction(new ScriptedDrawActions::setGradientFill(grad));
		}
		else if (gradientData.getArray()->size() >= 7)
		{
			auto c1 = ScriptingApi::Content::Helpers::getCleanedObjectColour(data->getUnchecked(0));
			auto c2 = ScriptingApi::Content::Helpers::getCleanedObjectColour(data->getUnchecked(3));

			auto grad = ColourGradient(c1, (float)data->getUnchecked(1), (float)data->getUnchecked(2),
				c2, (float)data->getUnchecked(4), (float)data->getUnchecked(5), (bool)data->getUnchecked(6));

			auto& ar = *gradientData.getArray();

			if (ar.size() > 7)
			{
				for (int i = 7; i < ar.size(); i += 2)
				{
					auto c = ScriptingApi::Content::Helpers::getCleanedObjectColour(ar[i]);
					auto pos = (float)ar[i + 1];
					grad.addColour(pos, c);
				}
			}

			drawActionHandler.addDrawAction(new ScriptedDrawActions::setGradientFill(grad));
		}
	}
	else
		reportScriptError("Gradient Data is not sufficient");
}



void ScriptingObjects::GraphicsObject::drawEllipse(var area, float lineThickness)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawEllipse(getRectangleFromVar(area), lineThickness));
}



void ScriptingObjects::GraphicsObject::fillEllipse(var area)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::fillEllipse(getRectangleFromVar(area)));
}

void ScriptingObjects::GraphicsObject::drawImage(String imageName, var area, int /*xOffset*/, int yOffset)
{
	Image img;

	if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent))
	{
		img = sc->getLoadedImage(imageName);
	}
	else if (auto laf = dynamic_cast<ScriptingObjects::ScriptedLookAndFeel*>(parent))
	{
		img = laf->getLoadedImage(imageName);
	}
	else
	{
		reportScriptError("drawImage is only allowed in a panel's paint routine");
	}

	if (img.isValid())
	{
		Rectangle<float> r = getRectangleFromVar(area);

		if (r.getWidth() != 0)
		{
			const double scaleFactor = (double)img.getWidth() / (double)r.getWidth();
			drawActionHandler.addDrawAction(new ScriptedDrawActions::drawImage(img, r, (float)scaleFactor, yOffset));
		}
	}
	else
	{
		drawActionHandler.addDrawAction(new ScriptedDrawActions::setColour(Colours::grey));
		drawActionHandler.addDrawAction(new ScriptedDrawActions::fillRect(getRectangleFromVar(area)));
		drawActionHandler.addDrawAction(new ScriptedDrawActions::setColour(Colours::black));
		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawRect(getRectangleFromVar(area), 1.0f));
		drawActionHandler.addDrawAction(new ScriptedDrawActions::setFont(GLOBAL_BOLD_FONT()));
		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawText("XXX", getRectangleFromVar(area), Justification::centred));

		debugError(dynamic_cast<Processor*>(getScriptProcessor()), "Image " + imageName + " not found");
	}
}

void ScriptingObjects::GraphicsObject::drawDropShadow(var area, var colour, int radius)
{
	auto r = getIntRectangleFromVar(area);
	DropShadow shadow;

	shadow.colour = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);
	shadow.radius = radius;

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawDropShadow(r, shadow));
}

void ScriptingObjects::GraphicsObject::drawDropShadowFromPath(var path, var area, var colour, int radius, var offset)
{
	auto r = getIntRectangleFromVar(area);
	auto o = getPointFromVar(offset);
	auto c = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);



	if (auto p = dynamic_cast<ScriptingObjects::PathObject*>(path.getObject()))
	{
		Path sp = p->getPath();
		
		auto area = r.toFloat().translated(o.getX(), o.getY());

		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawDropShadowFromPath(sp, area, c, radius));
	}
}

void ScriptingObjects::GraphicsObject::drawTriangle(var area, float angle, float lineThickness)
{
	Path p;
	p.startNewSubPath(0.5f, 0.0f);
	p.lineTo(1.0f, 1.0f);
	p.lineTo(0.0f, 1.0f);
	p.closeSubPath();
	p.applyTransform(AffineTransform::rotation(angle));
	auto r = getRectangleFromVar(area);
	p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawPath(p, PathStrokeType(lineThickness)));
}

void ScriptingObjects::GraphicsObject::fillTriangle(var area, float angle)
{
	Path p;
	p.startNewSubPath(0.5f, 0.0f);
	p.lineTo(1.0f, 1.0f);
	p.lineTo(0.0f, 1.0f);
	p.closeSubPath();
	p.applyTransform(AffineTransform::rotation(angle));
	auto r = getRectangleFromVar(area);
	p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);

	drawActionHandler.addDrawAction(new ScriptedDrawActions::fillPath(p));
}

void ScriptingObjects::GraphicsObject::addDropShadowFromAlpha(var colour, int radius)
{
	DropShadow shadow;

	shadow.colour = ScriptingApi::Content::Helpers::getCleanedObjectColour(colour);
	shadow.radius = radius;

	drawActionHandler.addDrawAction(new ScriptedDrawActions::addDropShadowFromAlpha(shadow));
}

void ScriptingObjects::GraphicsObject::drawRepaintMarker(const String& label)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawRepaintMarker(label));
}

bool ScriptingObjects::GraphicsObject::applyShader(var shader, var area)
{
	if (auto obj = dynamic_cast<ScriptingObjects::ScriptShader*>(shader.getObject()))
	{
		Rectangle<int> b = getRectangleFromVar(area).toNearestInt();
		drawActionHandler.addDrawAction(new ScriptedDrawActions::addShader(&drawActionHandler, obj, b));
		return true;
	}

	return false;
}

float ScriptingObjects::GraphicsObject::getStringWidth(String text)
{
	auto mc = getScriptProcessor()->getMainController_();
	return mc->getStringWidthFromEmbeddedFont(text, currentFontName, currentFontHeight, currentKerningFactor);
}

void ScriptingObjects::GraphicsObject::fillPath(var path, var area)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();

		if (p.getBounds().isEmpty())
			return;

		if (area.isArray())
		{
			Rectangle<float> r = getRectangleFromVar(area);
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		drawActionHandler.addDrawAction(new ScriptedDrawActions::fillPath(p));
	}
}

void ScriptingObjects::GraphicsObject::drawPath(var path, var area, var strokeType)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();

		

		if (area.isArray())
		{
			Rectangle<float> r = getRectangleFromVar(area);

			if (p.getBounds().isEmpty() || r.isEmpty())
			{
				return;
			}

			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		auto s = ApiHelpers::createPathStrokeType(strokeType);

		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawPath(p, s));
	}
}

void ScriptingObjects::GraphicsObject::rotate(var angleInRadian, var center)
{
	Point<float> c = getPointFromVar(center);
	auto air = (float)angleInRadian;
	auto a = AffineTransform::rotation(SANITIZED(air), c.getX(), c.getY());

	drawActionHandler.addDrawAction(new ScriptedDrawActions::addTransform(a));
}

void ScriptingObjects::GraphicsObject::flip(bool horizontally, var area)
{
    AffineTransform a;
    auto r = getIntRectangleFromVar(area);
    
    if(horizontally)
    {
		a = AffineTransform(-1.0f,  0.0f, (float)r.getWidth(),
                            0.0f,   1.0f, 0.0f);
    }
    else
    {
        a = AffineTransform(1.0f,  0.0f, 0.0f,
                            0.0f, -1.0f, (float)r.getHeight());
    }
    
    drawActionHandler.addDrawAction(new ScriptedDrawActions::addTransform(a));
}


Point<float> ScriptingObjects::GraphicsObject::getPointFromVar(const var& data)
{
	Point<float>&& f = ApiHelpers::getPointFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
}

Rectangle<float> ScriptingObjects::GraphicsObject::getRectangleFromVar(const var &data)
{
	Rectangle<float>&& f = ApiHelpers::getRectangleFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
}

Rectangle<int> ScriptingObjects::GraphicsObject::getIntRectangleFromVar(const var &data)
{
	Rectangle<int>&& f = ApiHelpers::getIntRectangleFromVar(data, &rectangleResult);

	if (rectangleResult.failed()) reportScriptError(rectangleResult.getErrorMessage());

	return f;
}





struct ScriptingObjects::ScriptedLookAndFeel::Wrapper
{
	API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, registerFunction);
	API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, setGlobalFont);
	API_VOID_METHOD_WRAPPER_2(ScriptedLookAndFeel, loadImage);
	API_VOID_METHOD_WRAPPER_0(ScriptedLookAndFeel, unloadAllImages);
	API_METHOD_WRAPPER_1(ScriptedLookAndFeel, isImageLoaded);
	API_VOID_METHOD_WRAPPER_1(ScriptedLookAndFeel, setInlineStyleSheet);
	API_VOID_METHOD_WRAPPER_1(ScriptedLookAndFeel, setStyleSheet);
	API_VOID_METHOD_WRAPPER_3(ScriptedLookAndFeel, setStyleSheetProperty);
};


ScriptingObjects::ScriptedLookAndFeel::ScriptedLookAndFeel(ProcessorWithScriptingContent* sp, bool isGlobal) :
	ConstScriptingObject(sp, 0),
	ControlledObject(sp->getMainController_()),
	functions(new DynamicObject()),
	wasGlobal(isGlobal),
	lastResult(Result::ok())
{
	ADD_API_METHOD_2(registerFunction);
	ADD_API_METHOD_2(setGlobalFont);
	ADD_API_METHOD_2(loadImage);
	ADD_API_METHOD_0(unloadAllImages);
	ADD_API_METHOD_1(isImageLoaded);
	ADD_API_METHOD_1(setInlineStyleSheet);
	ADD_API_METHOD_1(setStyleSheet);
	ADD_API_METHOD_3(setStyleSheetProperty);
	
	if(isGlobal)
		getScriptProcessor()->getMainController_()->setCurrentScriptLookAndFeel(this);
}

ScriptingObjects::ScriptedLookAndFeel::~ScriptedLookAndFeel()
{
	SimpleReadWriteLock::ScopedWriteLock sl(getMainController()->getJavascriptThreadPool().getLookAndFeelRenderLock());
	clearScriptContext();
}

void ScriptingObjects::ScriptedLookAndFeel::registerFunction(var functionName, var function)
{
	if (HiseJavascriptEngine::isJavascriptFunction(function))
	{
		addOptimizableFunction(function);
		functions.getDynamicObject()->setProperty(Identifier(functionName.toString()), function);
	}
}

void ScriptingObjects::ScriptedLookAndFeel::setGlobalFont(const String& fontName, float fontSize)
{
	f = getScriptProcessor()->getMainController_()->getFontFromString(fontName, fontSize);
}

void ScriptingObjects::ScriptedLookAndFeel::setInlineStyleSheet(const String& cssCode)
{
	currentStyleSheetFile = {};
	setStyleSheetInternal(cssCode);
}

String ScriptingObjects::ScriptedLookAndFeel::loadStyleSheetFile(const String& fileName)
{
	if(!fileName.endsWith(".css"))
		reportScriptError("the file must have the .css extension.");
	
#if true || USE_BACKEND

	auto cssFile = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts).getChildFile(fileName);
	
	auto ef = getMainController()->getExternalScriptFile(cssFile, false);

	String content;

	if(ef != nullptr)
		content = ef->getFileDocument().getAllContent();
	else if (cssFile.existsAsFile())
		content = cssFile.loadFileAsString();
	else
    {
        String s;
        String nl = "\n";
        
        s << "*" << nl;
        s << "{" << nl;
        s << "    color: white;" << nl;
        s << "}" << nl;
        
		content = s;
		cssFile.replaceWithText(s);
    }

	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		ef = jp->addFileWatcher(cssFile);
		
		jp->getScriptEngine()->addShaderFile(cssFile);
	}

	

	return content;
#else
	
	return getMainController()->getExternalScriptFromCollection(fileName);
#endif
}

void ScriptingObjects::ScriptedLookAndFeel::setStyleSheetInternal(const String& cssCode)
{
	debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "\tThe CSS renderer is still experimental, so use with precaution.");

	currentStyleSheet = cssCode;
	simple_css::Parser p(cssCode);

	additionalProperties = ValueTree("additionalProperties");

	auto ok = p.parse();

	if(!ok.wasOk())
		reportScriptError(ok.getErrorMessage());

	SimpleReadWriteLock::ScopedWriteLock sl(getMainController()->getJavascriptThreadPool().getLookAndFeelRenderLock());
	graphics.clear();
	css = p.getCSSValues();
}

void ScriptingObjects::ScriptedLookAndFeel::setStyleSheet(const String& fileName)
{
	

	currentStyleSheetFile = fileName;
	auto content = loadStyleSheetFile(fileName);
	setStyleSheetInternal(content);
}

void ScriptingObjects::ScriptedLookAndFeel::setStyleSheetProperty(const String& variableId, var value,
	const String& type)
{
	value = ApiHelpers::convertStyleSheetProperty(value, type);
	
	additionalProperties.setProperty(Identifier(variableId), value, nullptr);
}


Array<Identifier> ScriptingObjects::ScriptedLookAndFeel::getAllFunctionNames()
{
	static const Array<Identifier> sa =
	{
		"drawAlertWindow",
		"getAlertWindowMarkdownStyleData",
		"drawAlertWindowIcon",
		"drawPopupMenuBackground",
		"drawPopupMenuItem",
		"drawToggleButton",
		"drawRotarySlider",
		"drawLinearSlider",
		"drawDialogButton",
		"drawComboBox",
		"drawNumberTag",
		"createPresetBrowserIcons",
		"drawPresetBrowserBackground",
        "drawPresetBrowserDialog",
		"drawPresetBrowserColumnBackground",
		"drawPresetBrowserListItem",
		"drawPresetBrowserSearchBar",
		"drawPresetBrowserTag",
		"drawWavetableBackground",
		"drawWavetablePath",
		"drawTableBackground",
		"drawTablePath",
		"drawTablePoint",
		"drawTableRuler",
		"drawScrollbar",
		"drawMidiDropper",
		"drawThumbnailBackground",
		"drawThumbnailText",
		"drawThumbnailPath",
		"drawThumbnailRange",
		"drawThumbnailRuler",
        "getThumbnailRenderOptions",
		"drawAhdsrBackground",
		"drawAhdsrBall",
		"drawAhdsrPath",
		"drawKeyboardBackground",
		"drawWhiteNote",
		"drawBlackNote",
		"drawSliderPackBackground",
		"drawSliderPackFlashOverlay",
		"drawSliderPackRightClickLine",
		"drawSliderPackTextPopup",
        "getIdealPopupMenuItemSize",
		"drawTableRowBackground",
		"drawTableCell",
		"drawTableHeaderBackground",
		"drawTableHeaderColumn",
		"drawFilterDragHandle",
		"drawFilterBackground",
		"drawFilterPath",
		"drawFilterGridLines",
		"drawAnalyserBackground",
		"drawAnalyserPath",
		"drawAnalyserGrid",
        "drawMatrixPeakMeter"

	};

	return sa;
}

bool ScriptingObjects::ScriptedLookAndFeel::callWithGraphics(Graphics& g_, const Identifier& functionname, var argsObject, Component* c)
{
#if PERFETTO

	dispatch::StringBuilder n;

	if(c != nullptr)
		n << c->getName() << ".";

	n << functionname << "()";

	TRACE_SCRIPTING(DYNAMIC_STRING_BUILDER(n));

#endif

    // If this hits, you need to add that id to the array above.
	jassert(getAllFunctionNames().contains(functionname));


	if (!lastResult.wasOk())
		return false;
    
	auto f = functions.getProperty(functionname, {});

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
        ReferenceCountedObjectPtr<GraphicsObject> g;
        
        for(auto& gr: graphics)
        {
            if(gr.c == c && gr.functionName == functionname)
            {
                g = gr.g;
                break;
            }
        }
        
        if(g == nullptr)
        {
            GraphicsWithComponent gr;
            gr.g = new GraphicsObject(getScriptProcessor(), this);
            gr.c = c;
            gr.functionName = functionname;
            graphics.add(gr);
            g = gr.g;
        }
        
        var args[2];

        args[0] = var(g.get());
        args[1] = argsObject;
        
		var thisObject(this);

        
        
		{
			if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(getScriptProcessor()->getMainController_()->getJavascriptThreadPool().getLookAndFeelRenderLock()))
			{
				TRACE_SCRIPTING("executing script function");

				if (c != nullptr && c->getParentComponent() != nullptr)
				{
					var n = c->getParentComponent()->getName();
					argsObject.getDynamicObject()->setProperty("parentName", n);
				}
                
                static const StringArray hiddenProps = {"jcclr"};
                
                if(c != nullptr)
                {
                    for(auto& nv: c->getProperties())
                    {
                        if(!argsObject.hasProperty(nv.name))
                        {
                            bool hidden = false;
                            
                            for(const auto& hp: hiddenProps)
                            {
                                if(nv.name.toString().contains(hp))
                                {
                                    hidden = true;
                                    break;
                                }
                            }
                            
                            if(!hidden)
                                argsObject.getDynamicObject()->setProperty(nv.name, nv.value);
                        }
                    }
                }

				var::NativeFunctionArgs arg(thisObject, args, 2);
				auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();
				lastResult = Result::ok();

				engine->callExternalFunction(f, arg, &lastResult, true);

				if (lastResult.wasOk())
					g->getDrawHandler().flush(0);
				else
					debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), lastResult.getErrorMessage());
			}
		}

		TRACE_SCRIPTING("rendering draw actions");

		DrawActions::Handler::Iterator it(&g->getDrawHandler());

		if (c != nullptr)
		{
			it.render(g_, c);
		}
		else
		{
			while (auto action = it.getNextAction())
			{
#if PERFETTO
			dispatch::StringBuilder b;
			b << "g." << action->getDispatchId() << "()";
			TRACE_EVENT("drawactions", DYNAMIC_STRING_BUILDER(b));
#endif

				action->perform(g_);
			}
				
		}
        
		return true;
	}

	return false;
}

var ScriptingObjects::ScriptedLookAndFeel::callDefinedFunction(const Identifier& functionname, var* args, int numArgs)
{
	// If this hits, you need to add that id to the array above.
	jassert(getAllFunctionNames().contains(functionname));

	auto f = functions.getProperty(functionname, {});

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		SimpleReadWriteLock::ScopedReadLock sl(getScriptProcessor()->getMainController_()->getJavascriptThreadPool().getLookAndFeelRenderLock());

		var thisObject(this);
		var::NativeFunctionArgs arg(thisObject, args, numArgs);
		auto engine = dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine();
		Result r = Result::ok();

		try
		{
			return engine->callExternalFunctionRaw(f, arg);
		}
		catch (String& errorMessage)
		{
			debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), errorMessage);
		}
		catch (HiseJavascriptEngine::RootObject::Error&)
		{

		}
	}

	return {};
}



void ScriptingObjects::ScriptedLookAndFeel::clearScriptContext()
{
	functions = var();
	graphics.clear();
	loadedImages.clear();
}

hise::DebugableObjectBase::Location ScriptingObjects::ScriptedLookAndFeel::getLocation() const
{
	for (const auto& s : functions.getDynamicObject()->getProperties())
	{
		if (auto obj = dynamic_cast<DebugableObjectBase*>(s.value.getObject()))
		{
			return obj->getLocation();
		}
	}

	return Location();
}

Identifier ScriptingObjects::ScriptedLookAndFeel::Laf::getIdOfParentFloatingTile(Component& c)
{
	if (auto ft = c.findParentComponentOfClass<FloatingTile>())
	{
		return ft->getCurrentFloatingPanel()->getIdentifierForBaseClass();
	}

	return {};
}

bool ScriptingObjects::ScriptedLookAndFeel::Laf::addParentFloatingTile(Component& c, DynamicObject* obj)
{
	auto id = getIdOfParentFloatingTile(c);

	if (id.isValid())
	{
		obj->setProperty("parentType", id.toString());
		return true;
	}

	return false;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::setColourOrBlack(DynamicObject* obj, const Identifier& id, Component& c, int colourId)
{
	if (c.isColourSpecified(colourId))
		obj->setProperty(id, c.findColour(colourId).getARGB());
	else
		obj->setProperty(id, 0);
}

ScriptingObjects::ScriptedLookAndFeel::CSSLaf::CSSLaf(ScriptedLookAndFeel* parent_, ScriptContentComponent* content, Component* c, const ValueTree& data, const ValueTree& ad):
	LafBase(),
	StyleSheetLookAndFeel(*content),
	root(*content),
	parent(parent_)
{
	root.css.addIsolatedCollection(c, parent->currentStyleSheetFile, parent->css);
	
	simple_css::Selector id(simple_css::SelectorType::ID, data["id"].toString());

	StringArray initIds;
	initIds.add(id.toString());

	initIds.addArray(StringArray::fromTokens(ad["class"].toString(), " ", ""));

	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*c, initIds);
	
	
	if(auto ptr = root.css.getForComponent(c))
	{
		root.css.setAnimator(&root.animator);

		Component::SafePointer<Component> safe(c);

		auto updateProperty = [safe](Identifier v, var newValue)
		{
			if(safe.getComponent() != nullptr)
			{
				if(auto root = simple_css::CSSRootComponent::find(*safe.getComponent()))
				{
					if(auto ptr = root->css.getForComponent(safe.getComponent()))
					{
						if(v == Identifier("class"))
						{
							auto t = StringArray::fromTokens(newValue.toString(), " ", "");

							Array<var> classIds;

							for(auto& c: t)
								classIds.add(var(c));

							safe->getProperties().set(v, classIds);
							root->css.clearCache(safe);
						}
						else

							ptr->setPropertyVariable(v, newValue);
						
						safe->repaint();
					}
				}
			}
		};

		additionalPropertyUpdater.setCallback(parent->additionalProperties, {}, valuetree::AsyncMode::Asynchronously, updateProperty);
		additionalComponentPropertyUpdater.setCallback(ad, {}, valuetree::AsyncMode::Asynchronously, updateProperty);

		colourUpdater.setCallback(data, { Identifier("bgColour"), Identifier("itemColour"), Identifier("itemColour2"), Identifier("textColour")}, 
			valuetree::AsyncMode::Asynchronously, 
			[safe](Identifier v, var newValue)
		{
			if(safe.getComponent() != nullptr)
			{
				String c;
				c << "#" << ApiHelpers::getColourFromVar(newValue).toDisplayString(true);

				if(auto root = simple_css::CSSRootComponent::find(*safe.getComponent()))
				{
					if(auto ptr = root->css.getForComponent(safe.getComponent()))
					{
						ptr->setPropertyVariable(v, c);
						safe->repaint();
					}
				}
			}
		});
	}
}

ScriptingObjects::ScriptedLookAndFeel* ScriptingObjects::ScriptedLookAndFeel::CSSLaf::get()
{ return parent.get(); }

ScriptingObjects::ScriptedLookAndFeel::Laf::Laf(MainController* mc):
	ControlledObject(mc)
{}

ScriptingObjects::ScriptedLookAndFeel::Laf::~Laf()
{}

ScriptingObjects::ScriptedLookAndFeel* ScriptingObjects::ScriptedLookAndFeel::Laf::get()
{
	return dynamic_cast<ScriptedLookAndFeel*>(getMainController()->getCurrentScriptLookAndFeel());
}

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getFont()
{
	if (auto l = get())
		return l->f;
	else
		return GLOBAL_BOLD_FONT();
}

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getAlertWindowMessageFont()
{ return getFont(); }

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getAlertWindowTitleFont()
{ return getFont(); }

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getTextButtonFont(TextButton& textButton, int i)
{ return getFont(); }

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getComboBoxFont(ComboBox& comboBox)
{ return getFont(); }

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getPopupMenuFont()
{ return getFont(); }

Font ScriptingObjects::ScriptedLookAndFeel::Laf::getAlertWindowFont()
{ return getFont(); }

Identifier ScriptingObjects::ScriptedLookAndFeel::getObjectName() const
{ return "ScriptLookAndFeel"; }

int ScriptingObjects::ScriptedLookAndFeel::getNumChildElements() const
{
	if (auto dyn = functions.getDynamicObject())
		return dyn->getProperties().size();
            
	return 0;
}

DebugInformationBase* ScriptingObjects::ScriptedLookAndFeel::getChildElement(int index)
{
	WeakReference<ScriptedLookAndFeel> safeThis(this);

	auto vf = [safeThis, index]()
	{
		if (safeThis != nullptr)
		{
			if (auto dyn = safeThis->functions.getDynamicObject())
			{
				if(isPositiveAndBelow(index, dyn->getProperties().size()))
					return dyn->getProperties().getValueAt(index);
			}
		}

		return var();
	};

	String id = "%PARENT%.";

	auto mid = functions.getDynamicObject()->getProperties().getName(index);

	id << mid;

	Location l = getLocation();


	return new LambdaValueInformation(vf, id, {}, (DebugInformation::Type)getTypeNumber(), l);
}

Image ScriptingObjects::ScriptedLookAndFeel::getLoadedImage(const String& prettyName)
{
	for (auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
		{
			return img.image ? *img.image.getData() : Image();
		}
	}

	return Image();
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAlertBox(Graphics& g_, AlertWindow& w, const Rectangle<int>& ta, TextLayout& tl)
{
	if (functionDefined("drawAlertWindow"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(w.getLocalBounds().toFloat()));
		obj->setProperty("title", w.getName());

		addParentFloatingTile(w, obj);

		if (get()->callWithGraphics(g_, "drawAlertWindow", var(obj), &w))
			return;
	}

	GlobalHiseLookAndFeel::drawAlertBox(g_, w, ta, tl);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight)
{
    if (functionDefined("getIdealPopupMenuItemSize"))
    {
        auto obj = new DynamicObject();

        obj->setProperty("text", text);
        obj->setProperty("isSeparator", isSeparator);
        obj->setProperty("standardMenuHeight", standardMenuItemHeight);
        
        var x = var(obj);

        auto nObj = get()->callDefinedFunction("getIdealPopupMenuItemSize", &x, 1);

        if (nObj.isArray())
        {
            idealWidth = (int)nObj[0];
            idealHeight = (int)nObj[1];
            return;
        }
        if(nObj.isInt() || nObj.isInt64() || nObj.isDouble())
        {
            idealHeight = (int)nObj;
            return;
        }
    }
    
    GlobalHiseLookAndFeel::getIdealPopupMenuItemSize(text, isSeparator, standardMenuItemHeight, idealWidth, idealHeight);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawFilterDragHandle(Graphics& g_, FilterDragOverlay& o, int index, Rectangle<float> handleBounds, const FilterDragOverlay::DragData& d)
{
	if (functionDefined("drawFilterDragHandle"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(o.getLocalBounds().toFloat()));
		obj->setProperty("index", index);
		obj->setProperty("handle", ApiHelpers::getVarRectangle(handleBounds));
		obj->setProperty("selected", d.selected);
		obj->setProperty("enabled", d.enabled);
		obj->setProperty("drag", d.dragging);
		obj->setProperty("hover", d.hover);
		obj->setProperty("frequency", d.frequency);
		obj->setProperty("Q", d.q);
		obj->setProperty("gain", d.gain);
		obj->setProperty("type", d.type);

		setColourOrBlack(obj, "bgColour", o, FilterGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour1", o, FilterGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour2", o, FilterGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour3", o, FilterGraph::ColourIds::gridColour);
		setColourOrBlack(obj, "textColour", o, FilterGraph::ColourIds::textColour);

		if (get()->callWithGraphics(g_, "drawFilterDragHandle", var(obj), &o))
			return;
	}

	FilterDragOverlay::LookAndFeelMethods::drawFilterDragHandle(g_, o, index, handleBounds, d);
}



void ScriptingObjects::ScriptedLookAndFeel::Laf::drawFilterBackground(Graphics &g_, FilterGraph& fg)
{
	if (functionDefined("drawFilterBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(fg.getLocalBounds().toFloat()));

		setColourOrBlack(obj, "bgColour", fg, FilterGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour1", fg, FilterGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour2", fg, FilterGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour3", fg, FilterGraph::ColourIds::gridColour);
		setColourOrBlack(obj, "textColour", fg, FilterGraph::ColourIds::textColour);

		if (get()->callWithGraphics(g_, "drawFilterBackground", var(obj), &fg))
			return;
	}

	FilterGraph::LookAndFeelMethods::drawFilterBackground(g_, fg);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawFilterPath(Graphics& g_, FilterGraph& fg, const Path& p)
{
	if (functionDefined("drawFilterPath"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(fg.getLocalBounds().toFloat()));

		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);
		sp->getPath() = p;
		obj->setProperty("path", keeper);

		obj->setProperty("pathArea", ApiHelpers::getVarRectangle(p.getBounds()));

		setColourOrBlack(obj, "bgColour", fg, FilterGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour1", fg, FilterGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour2", fg, FilterGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour3", fg, FilterGraph::ColourIds::gridColour);
		setColourOrBlack(obj, "textColour", fg, FilterGraph::ColourIds::textColour);

		if (get()->callWithGraphics(g_, "drawFilterPath", var(obj), &fg))
			return;
	}

	FilterGraph::LookAndFeelMethods::drawFilterPath(g_, fg, p);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawFilterGridLines(Graphics &g_, FilterGraph& fg, const Path& gridPath)
{
	if (functionDefined("drawFilterGridLines"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(fg.getLocalBounds().toFloat()));

		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);
		sp->getPath() = gridPath;
		obj->setProperty("grid", keeper);

		setColourOrBlack(obj, "bgColour", fg, FilterGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour1", fg, FilterGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour2", fg, FilterGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour3", fg, FilterGraph::ColourIds::gridColour);
		setColourOrBlack(obj, "textColour", fg, FilterGraph::ColourIds::textColour);

		if (get()->callWithGraphics(g_, "drawFilterGridLines", var(obj), &fg))
			return;
	}

	FilterGraph::LookAndFeelMethods::drawFilterGridLines(g_, fg, gridPath);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawOscilloscopeBackground(Graphics& g_, RingBufferComponentBase& ac, Rectangle<float> areaToFill)
{
	if (functionDefined("drawAnalyserBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(areaToFill));

		auto c = dynamic_cast<Component*>(&ac);
		setColourOrBlack(obj, "bgColour", *c, RingBufferComponentBase::ColourId::bgColour);
		setColourOrBlack(obj, "itemColour1", *c, RingBufferComponentBase::ColourId::fillColour);
		setColourOrBlack(obj, "itemColour2", *c, RingBufferComponentBase::ColourId::lineColour);

		if (get()->callWithGraphics(g_, "drawAnalyserBackground", var(obj), c))
			return;
	}

	RingBufferComponentBase::LookAndFeelMethods::drawOscilloscopeBackground(g_, ac, areaToFill);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawOscilloscopePath(Graphics& g_, RingBufferComponentBase& ac, const Path& p)
{
	if (functionDefined("drawAnalyserPath"))
	{
		auto obj = new DynamicObject();
		auto c = dynamic_cast<Component*>(&ac);
		obj->setProperty("area", ApiHelpers::getVarRectangle(c->getLocalBounds().toFloat()));
		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());
		

		var keeper(sp);
		sp->getPath() = p;
		obj->setProperty("path", keeper);
		obj->setProperty("pathArea", ApiHelpers::getVarRectangle(p.getBounds()));

		setColourOrBlack(obj, "bgColour", *c, RingBufferComponentBase::ColourId::bgColour);
		setColourOrBlack(obj, "itemColour1", *c, RingBufferComponentBase::ColourId::fillColour);
		setColourOrBlack(obj, "itemColour2", *c, RingBufferComponentBase::ColourId::lineColour);

		if (get()->callWithGraphics(g_, "drawAnalyserPath", var(obj), c))
			return;
	}

	RingBufferComponentBase::LookAndFeelMethods::drawOscilloscopePath(g_, ac, p);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawGonioMeterDots(Graphics& g_, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index)
{
	RingBufferComponentBase::LookAndFeelMethods::drawGonioMeterDots(g_, ac, dots, index);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAnalyserGrid(Graphics& g_, RingBufferComponentBase& ac, const Path& p)
{
	if (functionDefined("drawAnalyserGrid"))
	{
		auto obj = new DynamicObject();
		auto c = dynamic_cast<Component*>(&ac);
		obj->setProperty("area", ApiHelpers::getVarRectangle(c->getLocalBounds().toFloat()));
		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);
		sp->getPath() = p;
		obj->setProperty("grid", keeper);

		setColourOrBlack(obj, "bgColour", *c, RingBufferComponentBase::ColourId::bgColour);
		setColourOrBlack(obj, "itemColour1", *c, RingBufferComponentBase::ColourId::fillColour);
		setColourOrBlack(obj, "itemColour2", *c, RingBufferComponentBase::ColourId::lineColour);

		if (get()->callWithGraphics(g_, "drawAnalyserGrid", var(obj), c))
			return;
	}

	RingBufferComponentBase::LookAndFeelMethods::drawAnalyserGrid(g_, ac, p);
}

hise::MarkdownLayout::StyleData ScriptingObjects::ScriptedLookAndFeel::Laf::getAlertWindowMarkdownStyleData()
{
	auto s = MessageWithIcon::LookAndFeelMethods::getAlertWindowMarkdownStyleData();

	if (functionDefined("getAlertWindowMarkdownStyleData"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("textColour", s.textColour.getARGB());
		obj->setProperty("codeColour", s.codeColour.getARGB());
		obj->setProperty("linkColour", s.linkColour.getARGB());
		obj->setProperty("headlineColour", s.headlineColour.getARGB());

		obj->setProperty("headlineFont", s.boldFont.getTypefaceName());
		obj->setProperty("font", s.f.getTypefaceName());
		obj->setProperty("fontSize", s.fontSize);

		var x = var(obj);

		auto nObj = get()->callDefinedFunction("getAlertWindowMarkdownStyleData", &x, 1);

		if (nObj.getDynamicObject() != nullptr)
		{
			s.textColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["textColour"]);
			s.linkColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["linkColour"]);
			s.codeColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["codeColour"]);
			s.headlineColour = ScriptingApi::Content::Helpers::getCleanedObjectColour(nObj["headlineColour"]);

			s.boldFont = getMainController()->getFontFromString(nObj.getProperty("headlineFont", "Default"), s.boldFont.getHeight());

			s.fontSize = nObj["fontSize"];
			s.f = getMainController()->getFontFromString(nObj.getProperty("font", "Default"), s.boldFont.getHeight());
		}
	}

	return s;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPopupMenuBackground(Graphics& g_, int width, int height)
{
	if (functionDefined("drawPopupMenuBackground"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("width", width);
		obj->setProperty("height", height);

		if (get()->callWithGraphics(g_, "drawPopupMenuBackground", var(obj), nullptr))
			return;
	}

	GlobalHiseLookAndFeel::drawPopupMenuBackground(g_, width, height);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPopupMenuItem(Graphics& g_, const Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked, bool hasSubMenu, const String& text, const String& shortcutKeyText, const Drawable* icon, const Colour* textColourToUse)
{
	if (functionDefined("drawPopupMenuItem"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("isSeparator", isSeparator);
        obj->setProperty("isSectionHeader", false);
		obj->setProperty("isActive", isActive);
		obj->setProperty("isHighlighted", isHighlighted);
		obj->setProperty("isTicked", isTicked);
		obj->setProperty("hasSubMenu", hasSubMenu);
		obj->setProperty("text", text);

		var keeper;

		if (auto p = dynamic_cast<const DrawablePath*>(icon))
		{
			auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());
			sp->getPath() = p->getPath();
			keeper = var(sp);
		}

		obj->setProperty("path", keeper);

		if (get()->callWithGraphics(g_, "drawPopupMenuItem", var(obj), nullptr))
			return;
	}

	GlobalHiseLookAndFeel::drawPopupMenuItem(g_, area, isSeparator, isActive, isHighlighted, isTicked, hasSubMenu, text, shortcutKeyText, icon, textColourToUse);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPopupMenuSectionHeader (Graphics& g_, const Rectangle<int>& area, const String& sectionName)
{
    if (functionDefined("drawPopupMenuItem"))
    {
        auto obj = new DynamicObject();
        obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
        obj->setProperty("isSeparator", false);
        obj->setProperty("isSectionHeader", true);
        obj->setProperty("isActive", false);
        obj->setProperty("isHighlighted", false);
        obj->setProperty("isTicked", false);
        obj->setProperty("hasSubMenu", false);
        obj->setProperty("text", sectionName);

        if (get()->callWithGraphics(g_, "drawPopupMenuItem", var(obj), nullptr))
            return;
    }

    GlobalHiseLookAndFeel::drawPopupMenuSectionHeader(g_, area, sectionName);
}

void setColourOrBlack(DynamicObject* obj, const Identifier& id, Component& c, int colourId)
{
	if (c.isColourSpecified(colourId))
		obj->setProperty(id, c.findColour(colourId).getARGB());
	else
		obj->setProperty(id, 0);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawToggleButton(Graphics &g_, ToggleButton &b, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawToggleButton"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("id", b.getComponentID());
		obj->setProperty("area", ApiHelpers::getVarRectangle(b.getLocalBounds().toFloat()));
		obj->setProperty("enabled", b.isEnabled());
		obj->setProperty("text", b.getButtonText());
		obj->setProperty("over", isMouseOverButton);
		obj->setProperty("down", isButtonDown);
		obj->setProperty("value", b.getToggleState());

		setColourOrBlack(obj, "bgColour", b, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "itemColour1", b, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", b, HiseColourScheme::ComponentFillBottomColourId);
		setColourOrBlack(obj, "textColour", b, HiseColourScheme::ComponentTextColourId);

		addParentFloatingTile(b, obj);

		if (get()->callWithGraphics(g_, "drawToggleButton", var(obj), &b))
			return;
	}

	GlobalHiseLookAndFeel::drawToggleButton(g_, b, isMouseOverButton, isButtonDown);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawRotarySlider(Graphics &g_, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s)
{
	if (functionDefined("drawRotarySlider"))
	{
		auto obj = new DynamicObject();

		s.setTextBoxStyle(Slider::NoTextBox, false, -1, -1);

		obj->setProperty("id", s.getComponentID());
		obj->setProperty("enabled", s.isEnabled());
		obj->setProperty("text", s.getName());
		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getLocalBounds().toFloat()));

		obj->setProperty("valueAsText", s.getTextFromValue(s.getValue()));
		obj->setProperty("value", s.getValue());

		NormalisableRange<double> range = NormalisableRange<double>(s.getMinimum(), s.getMaximum(), s.getInterval(), s.getSkewFactor());
		obj->setProperty("valueNormalized", range.convertTo0to1(s.getValue()));

		obj->setProperty("valueSuffixString", s.getTextFromValue(s.getValue()));
		obj->setProperty("suffix", s.getTextValueSuffix());
		obj->setProperty("skew", s.getSkewFactor());
		obj->setProperty("min", s.getMinimum());
		obj->setProperty("max", s.getMaximum());

		obj->setProperty("clicked", s.isMouseButtonDown());
		obj->setProperty("hover", s.isMouseOver());

		setColourOrBlack(obj, "bgColour", s, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "itemColour1", s, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", s, HiseColourScheme::ComponentFillBottomColourId);
		setColourOrBlack(obj, "textColour", s, HiseColourScheme::ComponentTextColourId);

		addParentFloatingTile(s, obj);

		if (get()->callWithGraphics(g_, "drawRotarySlider", var(obj), &s))
			return;
	}

	GlobalHiseLookAndFeel::drawRotarySlider(g_, -1, -1, width, height, -1, -1, -1, s);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawLinearSlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPos*/, float /*minSliderPos*/, float /*maxSliderPos*/, const Slider::SliderStyle style, Slider &slider)
{
	if (functionDefined("drawLinearSlider"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("id", slider.getComponentID());
		obj->setProperty("enabled", slider.isEnabled());
		obj->setProperty("text", slider.getName());

		auto parentPack = slider.findParentComponentOfClass<SliderPack>();

		

		obj->setProperty("area", ApiHelpers::getVarRectangle(slider.getLocalBounds().toFloat()));

		obj->setProperty("valueAsText", slider.getTextFromValue(slider.getValue()));

		obj->setProperty("valueSuffixString", slider.getTextFromValue(slider.getValue()));
		obj->setProperty("suffix", slider.getTextValueSuffix());
		obj->setProperty("skew", slider.getSkewFactor());

		obj->setProperty("style", style);	// Horizontal:2, Vertical:3, Range:9

		// Vertical & Horizontal style slider
		obj->setProperty("min", slider.getMinimum());
		obj->setProperty("max", slider.getMaximum());
		obj->setProperty("value", slider.getValue());

		NormalisableRange<double> range = NormalisableRange<double>(slider.getMinimum(), slider.getMaximum(), slider.getInterval(), slider.getSkewFactor());
		obj->setProperty("valueNormalized", range.convertTo0to1(slider.getValue()));

		// Range style slider
		double minv = 0.0;
		double maxv = 1.0;

		if (slider.isTwoValue())
		{
			minv = slider.getMinValue();
			maxv = slider.getMaxValue();
		}

		obj->setProperty("valueRangeStyleMin", minv);
		obj->setProperty("valueRangeStyleMax", maxv);

		obj->setProperty("valueRangeStyleMinNormalized", range.convertTo0to1(minv));
		obj->setProperty("valueRangeStyleMaxNormalized", range.convertTo0to1(maxv));

		obj->setProperty("clicked", slider.isMouseButtonDown());
		obj->setProperty("hover", slider.isMouseOver());

		setColourOrBlack(obj, "bgColour",    slider, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "itemColour1", slider, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", slider, HiseColourScheme::ComponentFillBottomColourId);
		setColourOrBlack(obj, "textColour",  slider, HiseColourScheme::ComponentTextColourId);

		if (parentPack != nullptr)
		{
			obj->setProperty("text", parentPack->getName());

			setColourOrBlack(obj, "bgColour", *parentPack, Slider::ColourIds::backgroundColourId);
			setColourOrBlack(obj, "itemColour1", *parentPack, Slider::thumbColourId);
			setColourOrBlack(obj, "itemColour2", *parentPack, Slider::textBoxOutlineColourId);
			setColourOrBlack(obj, "textColour", *parentPack, Slider::trackColourId);
		}

		addParentFloatingTile(slider, obj);

		if (get()->callWithGraphics(g, "drawLinearSlider", var(obj), &slider))
			return;
	}

	GlobalHiseLookAndFeel::drawLinearSlider(g, -1, -1, width, height, -1, -1, -1, style, slider);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawButtonText(Graphics &g_, TextButton &button, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawDialogButton"))
		return;

	static const Identifier pb("PresetBrowser");

	if (getIdOfParentFloatingTile(button) == pb)
		PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonText(g_, button, isMouseOverButton, isButtonDown);
	else
		GlobalHiseLookAndFeel::drawButtonText(g_, button, isMouseOverButton, isButtonDown);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawComboBox(Graphics& g_, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb)
{
	if (functionDefined("drawComboBox"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(cb.getLocalBounds().toFloat()));

		auto text = cb.getText();

		if (text.isEmpty())
		{
			if (cb.getNumItems() == 0)
				text = cb.getTextWhenNoChoicesAvailable();
			else
				text = cb.getTextWhenNothingSelected();
		}

		obj->setProperty("text", text);
		obj->setProperty("active", cb.getSelectedId() != 0);
		obj->setProperty("enabled", cb.isEnabled() && cb.getNumItems() > 0);
		obj->setProperty("hover", cb.isMouseOver(true) || cb.isMouseButtonDown(true) || cb.isPopupActive());

		setColourOrBlack(obj, "bgColour",    cb, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "itemColour1", cb, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", cb, HiseColourScheme::ComponentFillBottomColourId);
		setColourOrBlack(obj, "textColour",  cb, HiseColourScheme::ComponentTextColourId);

		addParentFloatingTile(cb, obj);

		if (get()->callWithGraphics(g_, "drawComboBox", var(obj), &cb))
			return;
	}

	GlobalHiseLookAndFeel::drawComboBox(g_, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, cb);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::positionComboBoxText(ComboBox &c, Label &labelToPosition)
{
	if (functionDefined("drawComboBox"))
	{
		labelToPosition.setVisible(false);
		return;
	}

	GlobalHiseLookAndFeel::positionComboBoxText(c, labelToPosition);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label)
{
	if (functionDefined("drawComboBox"))
	{
		label.setVisible(false);
		return;
	}

	GlobalHiseLookAndFeel::drawComboBoxTextWhenNothingSelected(g, box, label);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawButtonBackground(Graphics& g_, Button& button, const Colour& bg, bool isMouseOverButton, bool isButtonDown)
{
	if (functionDefined("drawDialogButton"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(button.getLocalBounds().toFloat()));
		obj->setProperty("text", button.getButtonText());
		obj->setProperty("enabled", button.isEnabled());
		obj->setProperty("over", isMouseOverButton);
		obj->setProperty("down", isButtonDown);
		obj->setProperty("value", button.getToggleState());
		obj->setProperty("bgColour", bg.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		addParentFloatingTile(button, obj);

		if (get()->callWithGraphics(g_, "drawDialogButton", var(obj), &button))
			return;
	}

	static const Identifier pb("PresetBrowser");

	if (getIdOfParentFloatingTile(button) == pb)
		PresetBrowserLookAndFeelMethods::drawPresetBrowserButtonBackground(g_, button, bg, isMouseOverButton, isButtonDown);
	else
		GlobalHiseLookAndFeel::drawButtonBackground(g_, button, bg, isMouseOverButton, isButtonDown);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawNumberTag(Graphics& g_, Component& comp, Colour& c, Rectangle<int> area, int offset, int size, int number)
{
	if (auto l = get())
	{
		if (number != -1)
		{
			auto obj = new DynamicObject();
			obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
			obj->setProperty("macroIndex", number - 1);

            setColourOrBlack(obj, "bgColour",    comp, HiseColourScheme::ComponentOutlineColourId);
            setColourOrBlack(obj, "itemColour1", comp, HiseColourScheme::ComponentFillTopColourId);
            setColourOrBlack(obj, "itemColour2", comp, HiseColourScheme::ComponentFillBottomColourId);
            setColourOrBlack(obj, "textColour",  comp, HiseColourScheme::ComponentTextColourId);
            
			if (l->callWithGraphics(g_, "drawNumberTag", var(obj), nullptr))
				return;
		}
	}

	NumberTag::LookAndFeelMethods::drawNumberTag(g_, comp, c, area, offset, size, number);
}

juce::Path ScriptingObjects::ScriptedLookAndFeel::Laf::createPresetBrowserIcons(const String& id)
{
	if (functionDefined("createPresetBrowserIcons"))
	{
		if (auto l = get())
		{
			var args = var(id);
			auto returnPath = l->callDefinedFunction("createPresetBrowserIcons", &args, 1);

			if (auto sg = dynamic_cast<PathObject*>(returnPath.getObject()))
			{
				return sg->getPath();
			}
		}
	}

	return PresetBrowserLookAndFeelMethods::createPresetBrowserIcons(id);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawPresetBrowserBackground(Graphics& g_, Component* p)
{
	if (functionDefined("drawPresetBrowserBackground"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(p->getLocalBounds().toFloat()));
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserBackground", var(obj), p))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawPresetBrowserBackground(g_, p);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawColumnBackground(Graphics& g_, int columnIndex, Rectangle<int> listArea, const String& emptyText)
{
	if (functionDefined("drawPresetBrowserColumnBackground"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(listArea.toFloat()));
		obj->setProperty("columnIndex", columnIndex);
		obj->setProperty("text", emptyText);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserColumnBackground", var(obj), nullptr))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawColumnBackground(g_, columnIndex, listArea, emptyText);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawListItem(Graphics& g_, int columnIndex, int rowIndex, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover)
{
	if (functionDefined("drawPresetBrowserListItem"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(position.toFloat()));
		obj->setProperty("columnIndex", columnIndex);
		obj->setProperty("rowIndex", rowIndex);
		obj->setProperty("text", itemName);
		obj->setProperty("selected", rowIsSelected);
		obj->setProperty("hover", hover);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserListItem", var(obj), nullptr))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawListItem(g_, columnIndex, rowIndex, itemName, position, rowIsSelected, deleteMode, hover);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSearchBar(Graphics& g_, Rectangle<int> area)
{
	if (functionDefined("drawPresetBrowserSearchBar"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		auto p = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(p);

		static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
	103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
	191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
	218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
	95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

		p->getPath().loadPathFromData(searchIcon, sizeof(searchIcon));
		p->getPath().applyTransform(AffineTransform::rotation(float_Pi));
		p->getPath().scaleToFit(6.0f, 5.0f, 18.0f, 18.0f, true);

		obj->setProperty("icon", var(p));

		if (get()->callWithGraphics(g_, "drawPresetBrowserSearchBar", var(obj), nullptr))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawSearchBar(g_, area);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableBackground(Graphics& g_, TableEditor& te, Rectangle<float> area, double rulerPosition)
{
	if (functionDefined("drawTableBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("id", te.getName());
		obj->setProperty("position", rulerPosition);
		obj->setProperty("enabled", te.isEnabled());
		
		setColourOrBlack(obj, "bgColour",    te, TableEditor::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour",  te, TableEditor::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", te, TableEditor::ColourIds::lineColour);
		setColourOrBlack(obj, "textColour",  te, TableEditor::ColourIds::rulerColour);

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTableBackground", var(obj), &te))
			return;		
	}
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTablePath(Graphics& g_, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness)
{
	if (functionDefined("drawTablePath"))
	{
		auto obj = new DynamicObject();

		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);

		sp->getPath() = p;

		obj->setProperty("path", var(sp));

		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("lineThickness", lineThickness);
		obj->setProperty("enabled", te.isEnabled());
		
		setColourOrBlack(obj, "bgColour", te, TableEditor::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", te, TableEditor::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", te, TableEditor::ColourIds::lineColour);
		setColourOrBlack(obj, "textColour", te, TableEditor::ColourIds::rulerColour);

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTablePath", var(obj), &te))
			return;
	}

	TableEditor::LookAndFeelMethods::drawTablePath(g_, te, p, area, lineThickness);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTablePoint(Graphics& g_, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged)
{
	if (functionDefined("drawTablePoint"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("tablePoint", ApiHelpers::getVarRectangle(tablePoint));
		obj->setProperty("isEdge", isEdge);
		obj->setProperty("hover", isHover);
		obj->setProperty("clicked", isDragged);
		obj->setProperty("enabled", te.isEnabled());
		
		setColourOrBlack(obj, "bgColour", te, TableEditor::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", te, TableEditor::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", te, TableEditor::ColourIds::lineColour);
		setColourOrBlack(obj, "textColour", te, TableEditor::ColourIds::rulerColour);

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTablePoint", var(obj), &te))
			return;
	}

	TableEditor::LookAndFeelMethods::drawTablePoint(g_, te, tablePoint, isEdge, isHover, isDragged);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableRuler(Graphics& g_, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition)
{
	if (functionDefined("drawTableRuler"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("position", rulerPosition);
		obj->setProperty("lineThickness", lineThickness);
		obj->setProperty("enabled", te.isEnabled());
		
		setColourOrBlack(obj, "bgColour",    te, TableEditor::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour",  te, TableEditor::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", te, TableEditor::ColourIds::lineColour);
		setColourOrBlack(obj, "textColour",  te, TableEditor::ColourIds::rulerColour);

		addParentFloatingTile(te, obj);

		if (get()->callWithGraphics(g_, "drawTableRuler", var(obj), &te))
			return;
	}

	TableEditor::LookAndFeelMethods::drawTableRuler(g_, te, area, lineThickness, rulerPosition);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawScrollbar(Graphics& g_, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
	if (functionDefined("drawScrollbar"))
	{
		auto obj = new DynamicObject();

		auto fullArea = Rectangle<int>(x, y, width, height).toFloat();

		Rectangle<float> thumbArea;

		if (isScrollbarVertical)
			thumbArea = Rectangle<int>(x, y + thumbStartPosition, width, thumbSize).toFloat();
		else
			thumbArea = Rectangle<int>(x + thumbStartPosition, y, thumbSize, height).toFloat();

		obj->setProperty("area", ApiHelpers::getVarRectangle(fullArea));
		obj->setProperty("handle", ApiHelpers::getVarRectangle(thumbArea));
		obj->setProperty("vertical", isScrollbarVertical);
		obj->setProperty("over", isMouseOver);
		obj->setProperty("down", isMouseDown);
		setColourOrBlack(obj, "bgColour",    scrollbar, ScrollBar::ColourIds::backgroundColourId);
		setColourOrBlack(obj, "itemColour",  scrollbar, ScrollBar::ColourIds::thumbColourId);
		setColourOrBlack(obj, "itemColour2", scrollbar, ScrollBar::ColourIds::trackColourId);

		addParentFloatingTile(scrollbar, obj);

		if (get()->callWithGraphics(g_, "drawScrollbar", var(obj), &scrollbar))
			return;
	}

	GlobalHiseLookAndFeel::drawScrollbar(g_, scrollbar, x, y, width, height, isScrollbarVertical, thumbStartPosition, thumbSize, isMouseOver, isMouseDown);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAhdsrBackground(Graphics& g, AhdsrGraph& graph)
{
	if (functionDefined("drawAhdsrBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("enabled", graph.isEnabled());
		obj->setProperty("area", ApiHelpers::getVarRectangle(graph.getBounds().toFloat()));
		
		setColourOrBlack(obj, "bgColour", graph, AhdsrGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", graph, AhdsrGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", graph, AhdsrGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour3", graph, AhdsrGraph::ColourIds::outlineColour);

		addParentFloatingTile(graph, obj);

		if (get()->callWithGraphics(g, "drawAhdsrBackground", var(obj), &graph))
			return;
	}

	AhdsrGraph::LookAndFeelMethods::drawAhdsrBackground(g, graph);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
{
	if (functionDefined("drawAhdsrPath"))
	{
		auto obj = new DynamicObject();

		auto p = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(p);

		p->getPath() = s;

		obj->setProperty("enabled", graph.isEnabled());
		obj->setProperty("isActive", isActive);
		obj->setProperty("path", keeper);
		obj->setProperty("currentState", graph.getCurrentStateIndex());
		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getBounds().toFloat()));
		
		setColourOrBlack(obj, "bgColour", graph, AhdsrGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", graph, AhdsrGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", graph, AhdsrGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour3", graph, AhdsrGraph::ColourIds::outlineColour);

		addParentFloatingTile(graph, obj);

		if (get()->callWithGraphics(g, "drawAhdsrPath", var(obj), &graph))
			return;
	}

	AhdsrGraph::LookAndFeelMethods::drawAhdsrPathSection(g, graph, s, isActive);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> pos)
{
	if (functionDefined("drawAhdsrBall"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(graph.getLocalBounds().toFloat()));
		obj->setProperty("position", ApiHelpers::getVarFromPoint(pos));
		obj->setProperty("currentState", graph.getCurrentStateIndex());
		obj->setProperty("enabled", graph.isEnabled());

		setColourOrBlack(obj, "bgColour",	 graph, AhdsrGraph::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour",  graph, AhdsrGraph::ColourIds::fillColour);
		setColourOrBlack(obj, "itemColour2", graph, AhdsrGraph::ColourIds::lineColour);
		setColourOrBlack(obj, "itemColour3", graph, AhdsrGraph::ColourIds::outlineColour);

		addParentFloatingTile(graph, obj);

		if (get()->callWithGraphics(g, "drawAhdsrBall", var(obj), &graph))
			return;
	}

	AhdsrGraph::LookAndFeelMethods::drawAhdsrBallPosition(g, graph, pos);
}


void ScriptingObjects::ScriptedLookAndFeel::Laf::drawMidiDropper(Graphics& g_, Rectangle<float> area, const String& text, MidiFileDragAndDropper& d)
{
	if (functionDefined("drawMidiDropper"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("hover", d.hover);
		obj->setProperty("active", d.isActive());
		obj->setProperty("externalDrag", d.externalDrag);

		setColourOrBlack(obj, "bgColour", d, HiseColourScheme::ComponentBackgroundColour);
		setColourOrBlack(obj, "itemColour", d, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "textColour", d, HiseColourScheme::ComponentTextColourId);

		obj->setProperty("text", text);

		if (get()->callWithGraphics(g_, "drawMidiDropper", var(obj), &d))
			return;
	}

	MidiFileDragAndDropper::LookAndFeelMethods::drawMidiDropper(g_, area, text, d);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawHiseThumbnailBackground(Graphics& g_, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area)
{
	if (functionDefined("drawThumbnailBackground"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("enabled", areaIsEnabled);

		setColourOrBlack(obj, "bgColour", th, AudioDisplayComponent::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", th, AudioDisplayComponent::ColourIds::fillColour);
		setColourOrBlack(obj, "textColour", th, AudioDisplayComponent::ColourIds::outlineColour);

		if (get()->callWithGraphics(g_, "drawThumbnailBackground", var(obj), &th))
			return;
	}

	HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailBackground(g_, th, areaIsEnabled, area);
}
void ScriptingObjects::ScriptedLookAndFeel::Laf::drawHiseThumbnailPath(Graphics& g_, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path)
{
	if (functionDefined("drawThumbnailPath"))
	{
		auto obj = new DynamicObject();
		auto area = path.getBounds();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("enabled", areaIsEnabled);

		auto sp = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(sp);

		sp->getPath() = path;

		obj->setProperty("path", keeper);

		setColourOrBlack(obj, "bgColour", th, AudioDisplayComponent::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", th, AudioDisplayComponent::ColourIds::fillColour);
		setColourOrBlack(obj, "textColour", th, AudioDisplayComponent::ColourIds::outlineColour);

		if (get()->callWithGraphics(g_, "drawThumbnailPath", var(obj), &th))
			return;
	}

	HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailPath(g_, th, areaIsEnabled, path);

}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const HiseAudioThumbnail::RectangleListType& rectList)
{
    HiseAudioThumbnail::LookAndFeelMethods::drawHiseThumbnailRectList(g, th, areaIsEnabled, rectList);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawThumbnailRuler(Graphics& g_, HiseAudioThumbnail& th, int xPosition)
{
	if (functionDefined("drawThumbnailRuler"))
	{
		auto obj = new DynamicObject();
		auto area = th.getLocalBounds();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		
		obj->setProperty("xPosition", xPosition);

		setColourOrBlack(obj, "bgColour", th, AudioDisplayComponent::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", th, AudioDisplayComponent::ColourIds::fillColour);
		setColourOrBlack(obj, "textColour", th, AudioDisplayComponent::ColourIds::outlineColour);

		if (get()->callWithGraphics(g_, "drawThumbnailRuler", var(obj), &th))
			return;
	}

	HiseAudioThumbnail::LookAndFeelMethods::drawThumbnailRuler(g_, th, xPosition);
}

HiseAudioThumbnail::RenderOptions ScriptingObjects::ScriptedLookAndFeel::Laf::getThumbnailRenderOptions(HiseAudioThumbnail& th, const HiseAudioThumbnail::RenderOptions& defaultOptions)
{
    if (functionDefined("getThumbnailRenderOptions"))
    {
        auto obj = new DynamicObject();

        obj->setProperty("displayMode", (int)defaultOptions.displayMode);
        obj->setProperty("manualDownSampleFactor", defaultOptions.manualDownSampleFactor);
        obj->setProperty("drawHorizontalLines", defaultOptions.drawHorizontalLines);
        obj->setProperty("scaleVertically", defaultOptions.scaleVertically);
        obj->setProperty("displayGain", defaultOptions.displayGain);
        obj->setProperty("useRectList", defaultOptions.useRectList);
        obj->setProperty("forceSymmetry", defaultOptions.forceSymmetry);
		obj->setProperty("multithreadThreshold", defaultOptions.multithreadThreshold);
		obj->setProperty("dynamicOptions", defaultOptions.dynamicOptions);

        var x = var(obj);

        auto nObj = get()->callDefinedFunction("getThumbnailRenderOptions", &x, 1);

        if (auto no = nObj.getDynamicObject() != nullptr)
        {
            auto newOptions = defaultOptions;
            
            newOptions.displayMode = (HiseAudioThumbnail::DisplayMode)(int)nObj.getProperty("displayMode", (int)defaultOptions.displayMode);
            newOptions.manualDownSampleFactor = nObj.getProperty("manualDownSampleFactor", defaultOptions.manualDownSampleFactor);
            newOptions.drawHorizontalLines = nObj.getProperty("drawHorizontalLines", defaultOptions.drawHorizontalLines);
            newOptions.scaleVertically = nObj.getProperty("scaleVertically", defaultOptions.scaleVertically);
            newOptions.displayGain = nObj.getProperty("displayGain", defaultOptions.displayGain);
            newOptions.useRectList = nObj.getProperty("useRectList", defaultOptions.useRectList);
            newOptions.forceSymmetry = nObj.getProperty("forceSymmetry", defaultOptions.forceSymmetry);
			newOptions.multithreadThreshold = (int)nObj.getProperty("multithreadThreshold", defaultOptions.multithreadThreshold);
			newOptions.dynamicOptions = nObj.getProperty("dynamicOptions", defaultOptions.dynamicOptions);

            FloatSanitizers::sanitizeFloatNumber(newOptions.manualDownSampleFactor);
            FloatSanitizers::sanitizeFloatNumber(newOptions.displayGain);
            
            return newOptions;
        }
    }

    return defaultOptions;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawThumbnailRange(Graphics& g_, HiseAudioThumbnail& th, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled)
{
	if (functionDefined("drawThumbnailRange"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("rangeIndex", areaIndex);
		obj->setProperty("rangeColour", c.getARGB());
		obj->setProperty("enabled", areaEnabled);

		setColourOrBlack(obj, "bgColour", th, AudioDisplayComponent::ColourIds::bgColour);
		setColourOrBlack(obj, "itemColour", th, AudioDisplayComponent::ColourIds::fillColour);
		setColourOrBlack(obj, "textColour", th, AudioDisplayComponent::ColourIds::outlineColour);

		if (get()->callWithGraphics(g_, "drawThumbnailRange", var(obj), &th))
			return;
	}

	HiseAudioThumbnail::LookAndFeelMethods::drawThumbnailRange(g_, th, area, areaIndex, c, areaEnabled);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTextOverlay(Graphics& g_, HiseAudioThumbnail& th, const String& text, Rectangle<float> area)
{
	if (functionDefined("drawThumbnailText"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area));
		obj->setProperty("text", text);

		

		if (get()->callWithGraphics(g_, "drawThumbnailText", var(obj), &th))
			return;
	}

	HiseAudioThumbnail::LookAndFeelMethods::drawTextOverlay(g_, th, text, area);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawKeyboardBackground(Graphics &g_, Component* c, int width, int height)
{
	if (functionDefined("drawKeyboardBackground"))
	{
		auto obj = new DynamicObject();

		Rectangle<int> a(0, 0, width, height);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));

		if (get()->callWithGraphics(g_, "drawKeyboardBackground", var(obj), c))
			return;
	}

	CustomKeyboardLookAndFeelBase::drawKeyboardBackground(g_, c, width, height);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawWhiteNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g_, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour)
{
	if (functionDefined("drawWhiteNote"))
	{
		auto obj = new DynamicObject();

		Rectangle<int> a(x, y, w, h);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));
		obj->setProperty("noteNumber", midiNoteNumber);
		obj->setProperty("hover", isOver);
		obj->setProperty("down", isDown);
		obj->setProperty("keyColour", state->getColourForSingleKey(midiNoteNumber).getARGB());

		if (get()->callWithGraphics(g_, "drawWhiteNote", var(obj), c))
			return;
	}

	CustomKeyboardLookAndFeelBase::drawWhiteNote(state, c, midiNoteNumber, g_, x, y, w, h, isDown, isOver, lineColour, textColour);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawBlackNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g_, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour)
{
	if (functionDefined("drawBlackNote"))
	{
		auto obj = new DynamicObject();

		Rectangle<int> a(x, y, w, h);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));
		obj->setProperty("noteNumber", midiNoteNumber);
		obj->setProperty("hover", isOver);
		obj->setProperty("down", isDown);
		obj->setProperty("keyColour", state->getColourForSingleKey(midiNoteNumber).getARGB());

		if (get()->callWithGraphics(g_, "drawBlackNote", var(obj), c))
			return;
	}

	CustomKeyboardLookAndFeelBase::drawBlackNote(state, c, midiNoteNumber, g_, x, y, w, h, isDown, isOver, noteFillColour);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSliderPackBackground(Graphics& g_, SliderPack& s)
{
	if (functionDefined("drawSliderPackBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("id", s.getName());

		setColourOrBlack(obj, "bgColour", s, Slider::ColourIds::backgroundColourId);
		setColourOrBlack(obj, "itemColour", s, Slider::thumbColourId);
		setColourOrBlack(obj, "itemColour2", s, Slider::textBoxOutlineColourId);
		setColourOrBlack(obj, "textColour", s, Slider::trackColourId);
		

		obj->setProperty("numSliders", s.getNumSliders());
		obj->setProperty("displayIndex", s.getData()->getNextIndexToDisplay());

		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getLocalBounds().toFloat()));

		if(get()->callWithGraphics(g_, "drawSliderPackBackground", var(obj), &s))
			return;
	}

	SliderPack::LookAndFeelMethods::drawSliderPackBackground(g_, s);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSliderPackFlashOverlay(Graphics& g_, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity)
{
	if (functionDefined("drawSliderPackFlashOverlay"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("id", s.getName());

		setColourOrBlack(obj, "bgColour", s, Slider::ColourIds::backgroundColourId);
		setColourOrBlack(obj, "itemColour", s, Slider::thumbColourId);
		setColourOrBlack(obj, "itemColour2", s, Slider::textBoxOutlineColourId);
		setColourOrBlack(obj, "textColour", s, Slider::trackColourId);

		obj->setProperty("numSliders", s.getNumSliders());
		obj->setProperty("displayIndex", sliderIndex);
		obj->setProperty("value", s.getValue(sliderIndex));
		obj->setProperty("intensity", intensity);

		auto sBounds = sliderBounds;
		sBounds.setY(0);
		sBounds.setHeight(s.getHeight()); s.getValue(sliderIndex);

		obj->setProperty("area", ApiHelpers::getVarRectangle(sBounds.toFloat()));

		if (get()->callWithGraphics(g_, "drawSliderPackFlashOverlay", var(obj), &s))
			return;
	}

	SliderPack::LookAndFeelMethods::drawSliderPackFlashOverlay(g_, s, sliderIndex, sliderBounds, intensity);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSliderPackRightClickLine(Graphics& g_, SliderPack& s, Line<float> lineToDraw)
{
	if (functionDefined("drawSliderPackRightClickLine"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("id", s.getName());

		setColourOrBlack(obj, "bgColour", s, Slider::ColourIds::backgroundColourId);
		setColourOrBlack(obj, "itemColour", s, Slider::thumbColourId);
		setColourOrBlack(obj, "itemColour2", s, Slider::textBoxOutlineColourId);
		setColourOrBlack(obj, "textColour", s, Slider::trackColourId);

		obj->setProperty("x1", lineToDraw.getStartX());
		obj->setProperty("x2", lineToDraw.getEndX());
		obj->setProperty("y1", lineToDraw.getStartY());
		obj->setProperty("y2", lineToDraw.getEndY());

		if (get()->callWithGraphics(g_, "drawSliderPackRightClickLine", var(obj), &s))
			return;
	}

	SliderPack::LookAndFeelMethods::drawSliderPackRightClickLine(g_, s, lineToDraw);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawSliderPackTextPopup(Graphics& g_, SliderPack& s, const String& textToDraw)
{
	if (functionDefined("drawSliderPackTextPopup"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("id", s.getName());

		setColourOrBlack(obj, "bgColour", s, Slider::ColourIds::backgroundColourId);
		setColourOrBlack(obj, "itemColour", s, Slider::thumbColourId);
		setColourOrBlack(obj, "itemColour2", s, Slider::textBoxOutlineColourId);
		setColourOrBlack(obj, "textColour", s, Slider::trackColourId);

		auto index = s.getCurrentlyDraggedSliderIndex();
		auto value = s.getCurrentlyDraggedSliderValue();

		obj->setProperty("index", index);
		obj->setProperty("value", value);

		obj->setProperty("area", ApiHelpers::getVarRectangle(s.getLocalBounds().toFloat()));
		
		obj->setProperty("text", textToDraw);

		if (get()->callWithGraphics(g_, "drawSliderPackTextPopup", var(obj), &s))
			return;
	}

	SliderPack::LookAndFeelMethods::drawSliderPackTextPopup(g_, s, textToDraw);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableRowBackground(Graphics& g_, const ScriptTableListModel::LookAndFeelData& d, int rowNumber, int width, int height, bool rowIsSelected, bool rowIsHovered)
{
	if (functionDefined("drawTableRowBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("bgColour", d.bgColour.getARGB());
		obj->setProperty("itemColour", d.itemColour1.getARGB());
		obj->setProperty("itemColour2", d.itemColour2.getARGB());
		obj->setProperty("textColour", d.textColour.getARGB());

		obj->setProperty("rowIndex", rowNumber);
		obj->setProperty("selected", rowIsSelected);
		obj->setProperty("hover", rowIsHovered);

		Rectangle<int> a(0, 0, width, height);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));

		if (get()->callWithGraphics(g_, "drawTableRowBackground", var(obj), nullptr))
			return;
	}

	ScriptTableListModel::LookAndFeelMethods::drawTableRowBackground(g_, d, rowNumber, width, height, rowIsSelected, rowIsHovered);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableCell(Graphics& g_, const ScriptTableListModel::LookAndFeelData& d, const String& text, int rowNumber, int columnId, int width, int height, bool rowIsSelected, bool cellIsClicked, bool cellIsHovered)
{
	if (functionDefined("drawTableCell"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("bgColour", d.bgColour.getARGB());
		obj->setProperty("itemColour", d.itemColour1.getARGB());
		obj->setProperty("itemColour2", d.itemColour2.getARGB());
		obj->setProperty("textColour", d.textColour.getARGB());

		obj->setProperty("text", text);
		obj->setProperty("rowIndex", rowNumber);
		obj->setProperty("columnIndex", columnId - 1);
		obj->setProperty("selected", rowIsSelected);
		obj->setProperty("clicked", cellIsClicked);
		obj->setProperty("hover", cellIsHovered);

		Rectangle<int> a(0, 0, width, height);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));

		if (get()->callWithGraphics(g_, "drawTableCell", var(obj), nullptr))
			return;
	}

	ScriptTableListModel::LookAndFeelMethods::drawTableCell(g_, d, text, rowNumber, columnId, width, height, rowIsSelected, cellIsClicked, cellIsHovered);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableHeaderBackground(Graphics& g_, TableHeaderComponent& h)
{
	if (functionDefined("drawTableHeaderBackground"))
	{
		auto obj = new DynamicObject();

		auto d = getDataFromTableHeader(h);

		obj->setProperty("bgColour", d.bgColour.getARGB());
		obj->setProperty("itemColour", d.itemColour1.getARGB());
		obj->setProperty("itemColour2", d.itemColour2.getARGB());
		obj->setProperty("textColour", d.textColour.getARGB());

		auto a = h.getLocalBounds();
		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));

		if (get()->callWithGraphics(g_, "drawTableHeaderBackground", var(obj), &h))
			return;
	}

	drawDefaultTableHeaderBackground(g_, h);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTableHeaderColumn(Graphics& g_, TableHeaderComponent& h, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags)
{
	if (functionDefined("drawTableHeaderColumn"))
	{
		auto obj = new DynamicObject();

		auto d = getDataFromTableHeader(h);

		obj->setProperty("bgColour", d.bgColour.getARGB());
		obj->setProperty("itemColour", d.itemColour1.getARGB());
		obj->setProperty("itemColour2", d.itemColour2.getARGB());
		obj->setProperty("textColour", d.textColour.getARGB());

		obj->setProperty("text", columnName);
		obj->setProperty("columnIndex", columnId - 1);
		obj->setProperty("hover", isMouseOver);
		obj->setProperty("down", isMouseDown);

		obj->setProperty("sortColumnId", d.sortColumnId);
		obj->setProperty("sortForwards", d.sortForward);

		Rectangle<int> a(0, 0, width, height);

		obj->setProperty("area", ApiHelpers::getVarRectangle(a.toFloat()));

		if (get()->callWithGraphics(g_, "drawTableHeaderColumn", var(obj), &h))
			return;
	}

	drawDefaultTableHeaderColumn(g_, h, columnName, columnId, width, height, isMouseOver, isMouseDown, columnFlags);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawMatrixPeakMeter(Graphics& g_, float* peakValues, float* maxPeaks, int numChannels, bool isVertical, float segmentSize, float paddingSize, Component* c)
{
    if (functionDefined("drawMatrixPeakMeter"))
    {
        auto obj = new DynamicObject();
 
        Array<var> peaks, maxPeakArray;
        
        for(int i = 0; i < numChannels; i++)
        {
            peaks.add(peakValues[i]);
            
            if(maxPeaks != nullptr)
                maxPeakArray.add(maxPeaks[numChannels]);
        }
        
        obj->setProperty("area", ApiHelpers::getVarRectangle(c->getLocalBounds().toFloat()));
        
        obj->setProperty("numChannels", numChannels);
        obj->setProperty("peaks", var(peaks));
        obj->setProperty("maxPeaks", var(maxPeakArray));
        
        obj->setProperty("isVertical", isVertical);
        obj->setProperty("segmentSize", segmentSize);
        obj->setProperty("paddingSize", paddingSize);
        
        if(auto pc = c->findParentComponentOfClass<PanelWithProcessorConnection>())
        {
            obj->setProperty("processorId", pc->getConnectedProcessor()->getId());
        }
                                 
        setColourOrBlack(obj, "bgColour", *c, MatrixPeakMeter::ColourIds::bgColour);
        setColourOrBlack(obj, "itemColour", *c, MatrixPeakMeter::ColourIds::peakColour);
        setColourOrBlack(obj, "itemColour2", *c, MatrixPeakMeter::ColourIds::trackColour);
        setColourOrBlack(obj, "textColour", *c, MatrixPeakMeter::ColourIds::maxPeakColour);

        if (get()->callWithGraphics(g_, "drawMatrixPeakMeter", var(obj), c))
            return;
    }

    MatrixPeakMeter::LookAndFeelMethods::drawMatrixPeakMeter(g_, peakValues, maxPeaks, numChannels, isVertical, segmentSize, paddingSize, c);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawWavetableBackground(Graphics& g_, WaterfallComponent& wc, bool isEmpty)
{
	if (functionDefined("drawWavetableBackground"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(wc.getLocalBounds().toFloat()));

		obj->setProperty("isEmpty", isEmpty);
		
		if (auto pc = wc.findParentComponentOfClass<PanelWithProcessorConnection>())
			obj->setProperty("processorId", pc->getConnectedProcessor()->getId());

		addParentFloatingTile(wc, obj);

		setColourOrBlack(obj, "bgColour", wc, HiseColourScheme::ComponentBackgroundColour);
		setColourOrBlack(obj, "itemColour", wc, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", wc, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "textColour", wc, HiseColourScheme::ComponentTextColourId);

		if (get()->callWithGraphics(g_, "drawWavetableBackground", var(obj), &wc))
			return;
	}

	WaterfallComponent::LookAndFeelMethods::drawWavetableBackground(g_, wc, isEmpty);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawWavetablePath(Graphics& g_, WaterfallComponent& wc, const Path& p, int tableIndex, bool isStereo, int currentTableIndex, int numTables)
{
	if (functionDefined("drawWavetablePath"))
	{
		auto obj = new DynamicObject();

		obj->setProperty("area", ApiHelpers::getVarRectangle(p.getBounds().toFloat()));

		auto pat = new ScriptingObjects::PathObject(get()->getScriptProcessor());

		var keeper(pat);

		pat->getPath() = p;

		obj->setProperty("path", keeper);

		obj->setProperty("tableIndex", tableIndex);
		obj->setProperty("isStereo", isStereo);
		obj->setProperty("currentTableIndex", currentTableIndex);
		obj->setProperty("numTables", numTables);

		if (auto pc = wc.findParentComponentOfClass<PanelWithProcessorConnection>())
			obj->setProperty("processorId", pc->getConnectedProcessor()->getId());

		addParentFloatingTile(wc, obj);

		setColourOrBlack(obj, "bgColour", wc, HiseColourScheme::ComponentBackgroundColour);
		setColourOrBlack(obj, "itemColour", wc, HiseColourScheme::ComponentFillTopColourId);
		setColourOrBlack(obj, "itemColour2", wc, HiseColourScheme::ComponentOutlineColourId);
		setColourOrBlack(obj, "textColour", wc, HiseColourScheme::ComponentTextColourId);

		if (get()->callWithGraphics(g_, "drawWavetablePath", var(obj), &wc))
			return;
	}

	WaterfallComponent::LookAndFeelMethods::drawWavetablePath(g_, wc, p, tableIndex, isStereo, currentTableIndex, numTables);
}

juce::Image ScriptingObjects::ScriptedLookAndFeel::Laf::createIcon(PresetHandler::IconType type)
{
	auto img = MessageWithIcon::LookAndFeelMethods::createIcon(type);

	if (auto l = get())
	{
		DynamicObject::Ptr obj = new DynamicObject();

		String s;

		switch (type)
		{
		case PresetHandler::IconType::Error:	s = "Error"; break;
		case PresetHandler::IconType::Info:		s = "Info"; break;
		case PresetHandler::IconType::Question: s = "Question"; break;
		case PresetHandler::IconType::Warning:	s = "Warning"; break;
		default: jassertfalse; break;
		}

		obj->setProperty("type", s);
		obj->setProperty("area", ApiHelpers::getVarRectangle({ 0.0f, 0.0f, (float)img.getWidth(), (float)img.getHeight() }));

		Image img2(Image::ARGB, img.getWidth(), img.getHeight(), true);
		Graphics g(img2);

		if (l->callWithGraphics(g, "drawAlertWindowIcon", var(obj.get()), nullptr))
		{
			if ((int)obj->getProperty("type") == -1)
				return {};

			return img2;
		}

	}

	return img;
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawTag(Graphics& g_, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position)
{
	if (functionDefined("drawPresetBrowserTag"))
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(position.toFloat()));
		obj->setProperty("text", name);
		obj->setProperty("blinking", blinking);
		obj->setProperty("value", active);
		obj->setProperty("selected", selected);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (get()->callWithGraphics(g_, "drawPresetBrowserTag", var(obj), nullptr))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawTag(g_, blinking, active, selected, name, position);
}

void ScriptingObjects::ScriptedLookAndFeel::Laf::drawModalOverlay(Graphics& g_, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command)
{
	if (auto l = get())
	{
		auto obj = new DynamicObject();
		obj->setProperty("area", ApiHelpers::getVarRectangle(area.toFloat()));
		obj->setProperty("labelArea", ApiHelpers::getVarRectangle(labelArea.toFloat()));
		obj->setProperty("title", title);
		obj->setProperty("text", command);
		obj->setProperty("bgColour", backgroundColour.getARGB());
		obj->setProperty("itemColour", highlightColour.getARGB());
		obj->setProperty("itemColour2", modalBackgroundColour.getARGB());
		obj->setProperty("textColour", textColour.getARGB());

		if (l->callWithGraphics(g_, "drawPresetBrowserDialog", var(obj), nullptr))
			return;
	}

	PresetBrowserLookAndFeelMethods::drawModalOverlay(g_, area, labelArea, title, command);
}



bool ScriptingObjects::ScriptedLookAndFeel::Laf::functionDefined(const String& s)
{
	return get() != nullptr && HiseJavascriptEngine::isJavascriptFunction(get()->functions.getProperty(Identifier(s), {}));
}

#if !HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL
LookAndFeel* HiseColourScheme::createAlertWindowLookAndFeel(void* mainController)
{
	if (auto mc = reinterpret_cast<MainController*>(mainController))
	{
		if (mc->getCurrentScriptLookAndFeel() != nullptr)
			return new ScriptingObjects::ScriptedLookAndFeel::Laf(mc);
	}

	return new hise::AlertWindowLookAndFeel();
}
#endif


void ScriptingObjects::ScriptedLookAndFeel::loadImage(String imageName, String prettyName)
{
	// It's a bit ugly to just copy that code from the script panel...
	PoolReference ref(getProcessor()->getMainController(), imageName, ProjectHandler::SubDirectories::Images);

	for (auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
		{
			if (img.image.getRef() != ref)
			{
				HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());
				img.image = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref);
			}

			return;
		}
	}

	HiseJavascriptEngine::TimeoutExtender xt(dynamic_cast<JavascriptProcessor*>(getScriptProcessor())->getScriptEngine());

	if (auto newImage = getProcessor()->getMainController()->getExpansionHandler().loadImageReference(ref))
		loadedImages.add({ newImage, prettyName });
	else
	{
		debugToConsole(dynamic_cast<Processor*>(getScriptProcessor()), "Image " + imageName + " not found. ");
	}
}

void ScriptingObjects::ScriptedLookAndFeel::unloadAllImages()
{
	loadedImages.clear();
}

bool ScriptingObjects::ScriptedLookAndFeel::isImageLoaded(String prettyName)
{
	for (auto& img : loadedImages)
	{
		if (img.prettyName == prettyName)
			return true;
	}
	
	return false;
}

ScriptingObjects::ScriptedLookAndFeel::LocalLaf::LocalLaf(ScriptedLookAndFeel* l) :
	Laf(l->getScriptProcessor()->getMainController_()),
	weakLaf(l)
{

}

hise::ScriptingObjects::ScriptedLookAndFeel* ScriptingObjects::ScriptedLookAndFeel::LocalLaf::get()
{
	return weakLaf.get();
}



} 
