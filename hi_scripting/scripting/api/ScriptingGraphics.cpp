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



struct ScriptingObjects::ScriptShader::Wrapper
{
	API_VOID_METHOD_WRAPPER_1(ScriptShader, setFragmentShader);
	API_VOID_METHOD_WRAPPER_2(ScriptShader, setUniformData);
	API_VOID_METHOD_WRAPPER_3(ScriptShader, setBlendFunc);
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
}

void ScriptingObjects::ScriptShader::setFragmentShader(String shaderFile)
{
#if USE_BACKEND
	auto f = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(getScriptProcessor())).getSubDirectory(FileHandlerBase::Scripts).getChildFile(shaderFile).withFileExtension("glsl");

	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		currentShaderFile = jp->addFileWatcher(f).get();
		jp->getScriptEngine()->addShaderFile(f);
	}

	auto codeToUse = f.loadFileAsString();

#else
	
	auto codeToUse = getScriptProcessor()->getMainController_()->getExternalScriptFromCollection(shaderFile + ".glsl");


#endif

	

	shaderCode = {};

	shaderCode << "uniform float uScale;";
	shaderCode << "uniform float iTime;";
	shaderCode << "uniform vec2 iMouse;";
	shaderCode << "uniform vec2 uOffset;";
	shaderCode << "uniform vec3 iResolution;";
	shaderCode << "";
	shaderCode << "vec2 gl_fc()";
	shaderCode << "{";
	shaderCode << "vec2 p = vec2(pixelPos.x + uOffset.x,";
	shaderCode << "	pixelPos.y + uOffset.y) / uScale;";
	shaderCode << "p.y = iResolution.y - p.y;";
	shaderCode << "return p;";
	shaderCode << "}";

	shaderCode << "\n#define fragCoord gl_fc()\n";
	shaderCode << "#define fragColor gl_FragColor\n";
	shaderCode << "\n#line 0 \"" << shaderFile << "\"\n";

	shaderCode << codeToUse;

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

		auto wScale = lr.getWidth() / gr.getWidth();

		auto scale = safeShader->scaleFactor;

		pr.setUniform("iTime", thisTime);
		pr.setUniform("uOffset", gr.getX() - lr.getX() * scale, gr.getY() - lr.getY() * scale);
		pr.setUniform("iResolution", lr.getWidth(), lr.getHeight(), 1.0f);
		//pr.setUniform("iResolution", safeShader->pos[2], safeShader->pos[3], 1.0f);
		pr.setUniform("uScale", scale);

		for (const auto& p : safeShader->uniformData)
		{
			const auto& v = p.value;

			auto name = p.name.toString().getCharPointer().getAddress();

			if (v.isArray())
			{
				if (v.getArray()->size() == 2) // vec2
					pr.setUniform(name, (float)v[0], (float)v[1]);
				if (v.getArray()->size() == 3) // vec3
				{
					pr.setUniform(name, (float)v[0], (float)v[1], (float)v[2]);
				}
				if (v.getArray()->size() == 4) // vec4
				{
					pr.setUniform(name, (float)v[0], (float)v[1], (float)v[2], (float)v[3]);
				}
			}
			if (v.isDouble()) // single value
			{
				pr.setUniform(name, (float)v);
			}
			if (v.isBuffer()) // static float array
			{
				pr.setUniform(name, v.getBuffer()->buffer.getReadPointer(0), v.getBuffer()->size);
			}
		}
	};

	dirty = true;
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


struct Result ScriptingObjects::ScriptShader::processErrorMessage(const Result& r)
{
	return r;
}

class PathPreviewComponent : public Component
{
public:

	PathPreviewComponent(Path &p_) : p(p_) { setSize(300, 300); }

	void paint(Graphics &g) override
	{
		g.setColour(Colours::white);
		p.scaleToFit(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), true);
		g.fillPath(p);
	}

private:

	Path p;
};

void ScriptingObjects::PathObject::rightClickCallback(const MouseEvent &e, Component* componentToNotify)
{
#if USE_BACKEND

	auto *editor = GET_BACKEND_ROOT_WINDOW(componentToNotify);

	PathPreviewComponent* content = new PathPreviewComponent(p);

	MouseEvent ee = e.getEventRelativeTo(editor);

	editor->getRootFloatingTile()->showComponentInRootPopup(content, editor, ee.getMouseDownPosition());

#else

	ignoreUnused(e, componentToNotify);

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
	API_VOID_METHOD_WRAPPER_4(GraphicsObject, addPerlinNoise);
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

	ADD_API_METHOD_4(addPerlinNoise);
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
		cl->addPostAction(new ScriptedPostDrawActions::guassianBlur(jlimit(1, 100, (int)blurAmount)));
	}
	else
		reportScriptError("You need to create a layer for gaussian blur");
}

void ScriptingObjects::GraphicsObject::boxBlur(var blurAmount)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
	{
		cl->addPostAction(new ScriptedPostDrawActions::boxBlur(jlimit(1, 100, (int)blurAmount)));
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

void ScriptingObjects::GraphicsObject::addPerlinNoise(double freq, double octave, double z, float amount)
{
	if (auto cl = drawActionHandler.getCurrentLayer())
		cl->addPostAction(new ScriptedPostDrawActions::addPerlinNoise(freq, octave, z, amount));
	else
		reportScriptError("You need to create a layer for addPerlinNoise");
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

	drawActionHandler.addDrawAction(new ScriptedDrawActions::drawPath(p, lineThickness));
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

void ScriptingObjects::GraphicsObject::drawPath(var path, var area, var thickness)
{
	if (PathObject* pathObject = dynamic_cast<PathObject*>(path.getObject()))
	{
		Path p = pathObject->getPath();

		if (area.isArray())
		{
			Rectangle<float> r = getRectangleFromVar(area);
			p.scaleToFit(r.getX(), r.getY(), r.getWidth(), r.getHeight(), false);
		}

		auto t = (float)thickness;
		drawActionHandler.addDrawAction(new ScriptedDrawActions::drawPath(p, SANITIZED(t)));
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
