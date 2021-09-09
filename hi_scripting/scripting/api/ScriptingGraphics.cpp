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
	l << "#line " << String(i) <<  " \"" << fileNameWithoutExtension << "\"";
	return l;
}

String ScriptingObjects::ScriptShader::FileParser::loadFileContent()
{
#if USE_BACKEND
	auto f = getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts).getChildFile(fileNameWithoutExtension).withFileExtension("glsl");

    if(!f.existsAsFile())
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
        
        f.replaceWithText(s);
    }
    
	if (auto jp = dynamic_cast<JavascriptProcessor*>(sp))
	{
		auto ef = jp->addFileWatcher(f);

		if (includedFiles.contains(ef))
			throw String("Trying to include " + fileNameWithoutExtension + " multiple times");

		includedFiles.add(ef);
		jp->getScriptEngine()->addShaderFile(f);
	}

	return f.loadFileAsString();
#else
	return getMainController()->getExternalScriptFromCollection(fileNameWithoutExtension + ".glsl");
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
		if (auto obj = getObject<ScriptShader>())
		{
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
				
				if (viewButton.getToggleState())
					localBounds.removeFromRight(uniformDataViewer->getWidth());

				auto gb = getLocalArea(getTopLevelComponent(), localBounds);

				obj->setGlobalBounds(gb, 1.0f);
				obj->localRect = localBounds.toFloat();

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

				auto time = Time::getMillisecondCounterHiRes();

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

		LOAD_PATH_IF_URL("stats", BackendBinaryData::ToolbarIcons::debugPanel);
		LOAD_PATH_IF_URL("view", BackendBinaryData::ToolbarIcons::viewPanel);
		LOAD_PATH_IF_URL("time", ColumnIcons::moveIcon);
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
	API_METHOD_WRAPPER_0(ScriptShader, toBase64);
	API_METHOD_WRAPPER_0(ScriptShader, getOpenGLStatistics);
};

ScriptingObjects::ScriptShader::ScriptShader(ProcessorWithScriptingContent* sp) :
	ConstScriptingObject(sp, (int)BlendMode::numBlendModes),
	r(Result::fail("uncompiled"))
{
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

#if JUCE_WINDOWS
	// The #line directive does not work on macOS apparently...
	s << "#line 0 \"" << shaderName << "\" \n";
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

void ScriptingObjects::ScriptShader::makeStatistics()
{
	auto d = new DynamicObject();

	int major = 0, minor = 0;
	glGetIntegerv(GL_MAJOR_VERSION, &major);
	glGetIntegerv(GL_MINOR_VERSION, &minor);

	auto vendor = String((const char*)glGetString(GL_VENDOR));

	auto renderer = String((const char*)glGetString(GL_RENDERER));
	auto version = String((const char*)glGetString(GL_VERSION));
	auto shaderVersion = OpenGLShaderProgram::getLanguageVersion();

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

	shaderCode = getHeader();

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

struct Result ScriptingObjects::ScriptShader::processErrorMessage(const Result& r)
{
	return r;
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
	API_VOID_METHOD_WRAPPER_3(PathObject, addArc);
	API_METHOD_WRAPPER_1(PathObject, getBounds);
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
	ADD_API_METHOD_3(addArc);
	ADD_API_METHOD_1(getBounds);
}

ScriptingObjects::PathObject::~PathObject()
{

}


void ScriptingObjects::PathObject::loadFromData(var data)
{
	if (data.isArray())
	{
		p.clear();

		Array<unsigned char> pathData;

		Array<var> *varData = data.getArray();

		const int numElements = varData->size();

		pathData.ensureStorageAllocated(numElements);

		for (int i = 0; i < numElements; i++)
		{
			pathData.add(static_cast<unsigned char>((int)varData->getUnchecked(i)));
		}

		p.loadPathFromData(pathData.getRawDataPointer(), numElements);
	}
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

void ScriptingObjects::PathObject::addArc(var area, var fromRadians, var toRadians)
{
	auto rect = ApiHelpers::getRectangleFromVar(area);

	auto fr = (float)fromRadians;
	auto tr = (float)toRadians;

	p.addArc(rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight(), SANITIZED(fr), SANITIZED(tr), true);
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
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, setFont);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawText);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawAlignedText);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, setGradientFill);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, drawEllipse);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, fillEllipse);
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, drawImage);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawDropShadow);
	API_VOID_METHOD_WRAPPER_5(GraphicsObject, drawDropShadowFromPath);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, addDropShadowFromAlpha);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillTriangle);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, fillPath);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, drawPath);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, rotate);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, gaussianBlur);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, boxBlur);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, desaturate);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, addNoise);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyMask);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, beginLayer);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, endLayer);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, beginBlendLayer);

	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyHSL);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, applyGamma);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, applyGradientMap);
	API_VOID_METHOD_WRAPPER_1(GraphicsObject, applySharpness);
	API_VOID_METHOD_WRAPPER_0(GraphicsObject, applySepia);
	API_VOID_METHOD_WRAPPER_3(GraphicsObject, applyVignette);
	API_VOID_METHOD_WRAPPER_2(GraphicsObject, applyShader);
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
	ADD_API_METHOD_2(setFont);
	ADD_API_METHOD_2(drawText);
	ADD_API_METHOD_3(drawAlignedText);
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

	ADD_API_METHOD_1(beginLayer);
	ADD_API_METHOD_1(gaussianBlur);
	ADD_API_METHOD_1(boxBlur);
	ADD_API_METHOD_0(desaturate);
	ADD_API_METHOD_1(addNoise);
	ADD_API_METHOD_3(applyMask);

	ADD_API_METHOD_3(applyHSL);
	ADD_API_METHOD_1(applyGamma);
	ADD_API_METHOD_2(applyGradientMap);
	ADD_API_METHOD_1(applySharpness);
	ADD_API_METHOD_0(applySepia);
	ADD_API_METHOD_3(applyVignette);
	ADD_API_METHOD_2(applyShader);

	ADD_API_METHOD_0(endLayer);
	ADD_API_METHOD_2(beginBlendLayer);

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
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		cl->addPostAction(new ScriptedPostDrawActions::addNoise(jlimit(0.0f, 1.0f, (float)noiseAmount)));
	}
	else
		reportScriptError("You need to create a layer for adding noise");
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

void ScriptingObjects::GraphicsObject::fillRoundedRectangle(var area, float cornerSize)
{
	auto cs = (float)cornerSize;
	drawActionHandler.addDrawAction(new ScriptedDrawActions::fillRoundedRect(getRectangleFromVar(area), SANITIZED(cs)));
}

void ScriptingObjects::GraphicsObject::drawRoundedRectangle(var area, float cornerSize, float borderSize)
{
	auto cs = SANITIZED(cornerSize);
	auto bs = SANITIZED(borderSize);
	auto ar = getRectangleFromVar(area);

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawRoundedRectangle(ar, bs, cs));
}

void ScriptingObjects::GraphicsObject::drawHorizontalLine(int y, float x1, float x2)
{
	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawHorizontalLine(y, SANITIZED(x1), SANITIZED(x2)));
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
		else
			reportScriptError("Gradient Data must have six elements");
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
	if (auto sc = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(parent))
	{
		const Image img = sc->getLoadedImage(imageName);

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
	else
	{
		reportScriptError("drawImage is only allowed in a panel's paint routine");
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

void ScriptingObjects::GraphicsObject::applyShader(var shader, var area)
{
	if (auto obj = dynamic_cast<ScriptingObjects::ScriptShader*>(shader.getObject()))
	{
		Rectangle<int> b = getRectangleFromVar(area).toNearestInt();
		drawActionHandler.addDrawAction(new ScriptedDrawActions::addShader(&drawActionHandler, obj, b));
	}
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
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		PathStrokeType s(1.0f);

		if (auto obj = strokeType.getDynamicObject())
		{
			static const StringArray endcaps = { "butt", "square", "rounded" };
			static const StringArray jointStyles = { "mitered", "curved","beveled" };

			auto endCap = (PathStrokeType::EndCapStyle)endcaps.indexOf(obj->getProperty("EndCapStyle").toString());
			auto jointStyle = (PathStrokeType::JointStyle)jointStyles.indexOf(obj->getProperty("JointStyle").toString());
			auto thickness = (float)obj->getProperty("Thickness");

			s = PathStrokeType(SANITIZED(thickness), jointStyle, endCap);
		}
		else
		{
			auto t = (float)strokeType;
			s = PathStrokeType(SANITIZED(t));
		}

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




} 
