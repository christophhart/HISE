namespace hise { using namespace juce;

struct HiseJavascriptEngine::RootObject::ScopedBlockStatement: public Statement
{
	ScopedBlockStatement(const CodeLocation& l, ExpPtr condition_) noexcept:
	  Statement(l),
      condition(condition_)
	{}

	virtual bool isDebugStatement() const = 0;

	void writeLocation(dispatch::StringBuilder& n)
	{
		n << "goto " << location.externalFile << "@" << (int)(location.location - location.program.getCharPointer());
	}

	bool checkCondition(const Scope& s, bool before)
	{
		if(before && condition != nullptr)
			conditionValue = condition->getResult(s);

		return conditionValue;
	}

	virtual void cleanup(const Scope& s) const = 0;

	ExpPtr condition;
	bool conditionValue = true;
};

struct HiseJavascriptEngine::RootObject::ScopedSetter: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedSetter(CodeLocation l, ExpPtr c):
	  ScopedBlockStatement(l, c)
	{}

	SN_NODE_ID("set");

	bool isDebugStatement() const override { return false; }

	ResultCode perform(const Scope& s, var*) const override
	{
		prevValue = lhs->getResult(s);
		lhs->assign(s, rhs->getResult(s));
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		lhs->assign(s, prevValue); 
	}

	ExpPtr lhs, rhs;
	mutable var prevValue;
};

struct HiseJavascriptEngine::RootObject::ScopedBypasser: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedBypasser(CodeLocation l, ExpPtr c, ExpPtr broadcaster):
	  ScopedBlockStatement(l, c),
	  be(broadcaster)
	{}

	SN_NODE_ID("bypass");

	bool isDebugStatement() const override { return false; }

	ResultCode perform(const Scope& s, var*) const override
	{
		auto br = be->getResult(s);
        b = dynamic_cast<ScriptingObjects::ScriptBroadcaster*>(br.getObject());

		if(b != nullptr)
		{
			state = b->isBypassed();
		}
		else
		{
			location.throwError("expression is not a broadcaster");
		}

		if(!state)
		{
			dispatch::StringBuilder n;
			n << "bypass " << b->getMetadata().id;
			TRACE_EVENT_BEGIN("scripting", DYNAMIC_STRING_BUILDER(n));
		}

		b->setBypassed(true, false, false);
		
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		if(!state)
		{
			TRACE_EVENT_END("scripting");
		}

		if(b != nullptr)
			b->setBypassed(state, true, false);
	}

	mutable WeakReference<ScriptingObjects::ScriptBroadcaster> b;
	mutable bool state = false;

	ExpPtr be;
};

struct HiseJavascriptEngine::RootObject::ScopedLocker: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedLocker(CodeLocation l, ExpPtr c, LockHelpers::Type lockToAcquire):
	  ScopedBlockStatement(l, c),
	  lockType(lockToAcquire)
	{
		n << ".lock(Threads." << LockHelpers::getLockName(lockType) << ")";
		writeLocation(loc);
	}

	SN_NODE_ID("lock");

	bool isDebugStatement() const override { return false; }

	ResultCode perform(const Scope& s, var*) const override
	{
		try
		{
			{
				mc = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor)->getMainController();

				if(mc->getKillStateHandler().currentThreadHoldsLock(lockType))
					return ResultCode::ok;

#if PERFETTO
				dispatch::StringBuilder n2;
				n2 << "waiting for " << LockHelpers::getLockName(lockType);
				TRACE_EVENT("scripting", DYNAMIC_STRING_BUILDER(n2));
#endif

				auto& lock = LockHelpers::getLockChecked(mc, lockType);
				lock.enter();
				holdsLock = true;
			}

			TRACE_EVENT_BEGIN("scripting", DYNAMIC_STRING_BUILDER(n), "location", DYNAMIC_STRING_BUILDER(loc));
		}
		catch(const LockHelpers::BadLockException& e)
		{
			location.throwError(e.getErrorMessage());
		}
		
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		if(holdsLock)
		{
			auto& lock = LockHelpers::getLockUnchecked(mc, lockType);
			lock.exit();

			if(lockType == LockHelpers::Type::ScriptLock)
			{
				mc->getJavascriptThreadPool().notify();
			}

			TRACE_EVENT_END("scripting");
		}
	}

	mutable MainController* mc = nullptr;
	const LockHelpers::Type lockType;

	mutable bool holdsLock = false;

	dispatch::StringBuilder n, loc;
};

struct HiseJavascriptEngine::RootObject::ScopedSuspender: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedSuspender(CodeLocation l, ExpPtr c, const dispatch::HashedPath& path_):
	  ScopedBlockStatement(l, c),
	  path(path_)
	{}

	dispatch::HashedPath path;

	SN_NODE_ID("defer");

	bool isDebugStatement() const override { return false; }

	ResultCode perform(const Scope& s, var*) const override
	{
		auto& root = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor)->getMainController()->getRootDispatcher();
		root.setState(path, dispatch::State::Paused);
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		auto& root = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor)->getMainController()->getRootDispatcher();
		root.setState(path, dispatch::State::Running);
	}
};

struct HiseJavascriptEngine::RootObject::ScopedTracer: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedTracer(CodeLocation l, ExpPtr c, const String& v):
	  ScopedBlockStatement(l, c)
	{
		n << v;
		writeLocation(loc);
	}
	
	SN_NODE_ID("trace");

#if PERFETTO
	bool isDebugStatement() const override { return false; }
#else
	bool isDebugStatement() const override { return true; }
#endif

	ResultCode perform(const Scope& s, var*) const override
	{
		TRACE_EVENT_BEGIN("scripting", DYNAMIC_STRING_BUILDER(n), "location", DYNAMIC_STRING_BUILDER(loc));
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		TRACE_EVENT_END("scripting");
	}

	dispatch::StringBuilder n, loc;

};

struct HiseJavascriptEngine::RootObject::ScopedDumper: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedDumper(CodeLocation l, ExpPtr c):
	  ScopedBlockStatement(l, c)
	{
		
	}

	SN_NODE_ID("dump");

	bool isDebugStatement() const override { return true; }

	OwnedArray<Expression> dumpObjects;

	void dump(const Scope& s, bool before) const
	{
		auto p = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor);
		auto loc = location.getEncodedLocationString(p->getId(), p->getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts));
		String m;
		m << "dump ";

		if(before) 
			m << "before: ";
		else
			m << "after: ";

		m << loc << "\n";

		int counter = 0;

		for(auto d: dumpObjects)
		{
			m << "> ";

			auto id = d->getVariableName();
			if(id.isNull())
				m << "args[" << String(counter) << "]";
			else
				m << id;
			
			m << " = " << JSON::toString(d->getResult(s), true) << "\n";

			counter++;
		}

		debugToConsole(p, m);
	}

	ResultCode perform(const Scope& s, var*) const override
	{
		dump(s, true);
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		dump(s, false);
	}
};

struct HiseJavascriptEngine::RootObject::ScopedNoop: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedNoop(CodeLocation l, ExpPtr c):
	  ScopedBlockStatement(l, c)
	{}

	SN_NODE_ID("noop");

	bool isDebugStatement() const override { return true; }

	ResultCode perform(const Scope& s, var*) const override
	{
		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
	}
};

struct HiseJavascriptEngine::RootObject::ScopedCounter: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedCounter(CodeLocation l, ExpPtr c, const String& name_):
	  ScopedBlockStatement(l, c),
	  name(name_)
	{}

	SN_NODE_ID("count");

	bool isDebugStatement() const override { return true; }

	ResultCode perform(const Scope& s, var*) const override
	{
		auto p = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor);

		String m;
		m << "counter " << name << ": " << String(counter++) << " - ";
		m << location.getEncodedLocationString(p->getId(), p->getMainController()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts));

		debugToConsole(p, m);

		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
	}

	const String name;
	mutable int counter = 0;
};


struct HiseJavascriptEngine::RootObject::ScopedProfiler: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedProfiler(CodeLocation l, ExpPtr c, const String& name_):
	  ScopedBlockStatement(l, c),
	  name(name_)
	{}

	SN_NODE_ID("profile");

	bool isDebugStatement() const override { return true; }

	ResultCode perform(const Scope& s, var*) const override
	{
		start = Time::getMillisecondCounterHiRes();

		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		auto delta = Time::getMillisecondCounterHiRes() - start;
		String m;
		m << "profile" << name << ": " << String(delta, 3) << " ms";
		debugToConsole(dynamic_cast<Processor*>(s.root->hiseSpecialData.processor), m);
	}

	const String name;
	mutable double start;
};

struct HiseJavascriptEngine::RootObject::ScopedPrinter: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedPrinter(CodeLocation l, ExpPtr c, const String& v):
	  ScopedBlockStatement(l, c)
	{
		b1 << "enter " << v;
		b2 << "exit " << v;
	}

	SN_NODE_ID("print");

	bool isDebugStatement() const override { return true; }

	ResultCode perform(const Scope& s, var*) const override
	{
		auto p = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor);
		debugToConsole(p, b1.toString());

		return ResultCode::ok;
	}

	void cleanup(const Scope& s) const override
	{
		auto p = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor);
		debugToConsole(p, b2.toString());
	}

	dispatch::StringBuilder b1, b2;
};

template <bool CheckBefore> struct ScopedAssert: public HiseJavascriptEngine::RootObject::ScopedBlockStatement
{
	ScopedAssert(HiseJavascriptEngine::RootObject::CodeLocation l, HiseJavascriptEngine::RootObject::ExpPtr c):
	  ScopedBlockStatement(l, c)
	{}

	~ScopedAssert() override
	{
		expected = nullptr;
		actual = nullptr;
	}

	SN_NODE_ID(CheckBefore ? "before" : "after");

	bool isDebugStatement() const override { return true; }

	ResultCode perform(const HiseJavascriptEngine::RootObject::Scope& s, var*) const override
	{
		if constexpr(CheckBefore)
			checkOrThrow(s);

		return ResultCode::ok;
	}

	void checkOrThrow(const HiseJavascriptEngine::RootObject::Scope& s) const
	{
		auto v1 = expected->getResult(s);
		auto v2 = actual->getResult(s);

		if(v1 != v2)
		{
			dispatch::StringBuilder n;
			n << "assert before failed. Expected: " << v1.toString() << ", actual: " << v2.toString();
			location.throwError(n.toString());
		}
	}

	void cleanup(const HiseJavascriptEngine::RootObject::Scope& s) const override
	{
		if constexpr (!CheckBefore)
			checkOrThrow(s);
	}

	HiseJavascriptEngine::RootObject::ExpPtr expected, actual;
};

struct HiseJavascriptEngine::RootObject::ScopedBefore: public ScopedAssert<true> { ScopedBefore(CodeLocation l, ExpPtr c): ScopedAssert<true>(l, c) {}; };
struct HiseJavascriptEngine::RootObject::ScopedAfter:  public ScopedAssert<false> { ScopedAfter(CodeLocation l, ExpPtr c): ScopedAssert<false>(l, c) {}; };

struct HiseJavascriptEngine::RootObject::BlockStatement : public Statement
{
	BlockStatement(const CodeLocation& l) noexcept : Statement(l) 
	{
		
	}

	void cleanup(const Scope& s) const
	{
		auto reportLocation = [&](ScopedBlockStatement * sbs, const String& message)
		{
			String m;
			m << sbs->location.getLocationString() << " - Error at scope cleanup: " << message;
			auto p = dynamic_cast<Processor*>(s.root->hiseSpecialData.processor);
			debugError(p, message);
		};

		for(int i = scopedBlockCounter ; i >= 0; i--)
		{
			auto sbs = scopedBlockStatements[i];

			try
			{
				if(sbs->checkCondition(s, false))
					sbs->cleanup(s);
			}
			catch(const String& e)
			{
				reportLocation(sbs, e);
			}
			catch(const Result& r)
			{
				reportLocation(sbs, r.getErrorMessage());
			}
			catch(const Breakpoint&)
			{
				reportLocation(sbs, "BREAKPOINT");
			}
			catch(const HiseJavascriptEngine::RootObject::Error& e)
			{
				reportLocation(sbs, e.errorMessage);
			}
		}
	}

	ResultCode performWithinScope(const Scope& s, var* returnedValue) const
	{
		for (int i = 0; i < statements.size(); ++i)
		{
#if ENABLE_SCRIPTING_BREAKPOINTS
			ScriptAudioThreadGuard guard(statements[i]->location);

			if (statements.getUnchecked(i)->breakpointReference.index != -1)
			{
				Statement* st = statements.getUnchecked(i);
				const auto& loc = st->location;
				int col, line;

				loc.fillColumnAndLines(col, line);
				Breakpoint bp = Breakpoint(st->breakpointReference.localScopeId, loc.externalFile, line, col, loc.getCharIndex(), st->breakpointReference.index);

				const bool hasRootScope = s.root.get() == s.scope.get();

				if(!hasRootScope)
					bp.localScope = s.scope.get();

				throw bp;
			}
#endif
			if (ResultCode r = statements.getUnchecked(i)->perform(s, returnedValue))
				return r;
		}
		
		return ok;
	}

	ResultCode perform(const Scope& s, var* returnedValue) const override
	{
		if(scopedBlockStatements.isEmpty())
		{
			return performWithinScope(s, returnedValue);
		}
		else try
		{
			scopedBlockCounter = 0;

			for(int i = 0; i < scopedBlockStatements.size(); i++)
			{
				scopedBlockCounter = i;

				if(scopedBlockStatements[i]->checkCondition(s, true))
					scopedBlockStatements[i]->perform(s, returnedValue);
			}

			auto rv = performWithinScope(s, returnedValue);

			cleanup(s);

			return rv;
		}
		catch(const String& e)
		{
			cleanup(s);
			throw e;
		}
		catch(const Result& r)
		{
			cleanup(s);
			throw r;
		}
		catch(const Breakpoint& bp)
		{
			cleanup(s);
			throw bp;
		}
		catch(const HiseJavascriptEngine::RootObject::Error& e)
		{
			cleanup(s);
			throw e;
		}
	}

	bool replaceChildStatement(ScopedPointer<Statement>& newStatement, Statement* childToReplace) override
	{
		if (newStatement == nullptr)
		{
			statements.removeObject(childToReplace);
			return true;
		}
		else
		{
			auto idx = statements.indexOf(childToReplace);
			statements.set(idx, newStatement.release(), true);
			return true;
		}
	}

	Statement* getChildStatement(int index) override 
	{
		if (isPositiveAndBelow(index, statements.size()))
			return statements[index];

		index -= statements.size();

		if (isPositiveAndBelow(index, scopedBlockStatements.size()))
			return scopedBlockStatements[index];

		return nullptr;
	};

	OwnedArray<Statement> statements;

	OwnedArray<ScopedBlockStatement> scopedBlockStatements;

	mutable int scopedBlockCounter = 0;
};

struct HiseJavascriptEngine::RootObject::IfStatement : public Statement
{
	IfStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var* returnedValue) const override
	{
		return (condition->getResult(s) ? trueBranch : falseBranch)->perform(s, returnedValue);
	}

	Statement* getChildStatement(int index) override
	{
		if (index == 0) return condition;
		if (index == 1) return trueBranch.get();
		if (index == 2) return falseBranch.get();
		return nullptr;
	};

	bool replaceChildStatement(Ptr& s, Statement* n) override
	{
		return swapIf(s, n, condition) || swapIf(s, n, trueBranch) || swapIf(s, n, falseBranch);
	}

	ExpPtr condition;
	ScopedPointer<Statement> trueBranch, falseBranch;
};

struct HiseJavascriptEngine::RootObject::CaseStatement : public Statement
{
	CaseStatement(const CodeLocation &l, bool isNotDefault_) noexcept: Statement(l), isNotDefault(isNotDefault_), initialized(false) {};

	ResultCode perform(const Scope &s, var *returnValue) const override
	{
		return body->perform(s, returnValue);
	}

	void initValues(const Scope &s)
	{
		if (!initialized)
		{
			initialized = true;
			values.ensureStorageAllocated(conditions.size());

			for (int i = 0; i < conditions.size(); i++)
				values.add(conditions[i]->getResult(s));
		}
	}

	Statement* getChildStatement(int index) override
	{
		if (isPositiveAndBelow(index, conditions.size()))
		{
			return conditions[index];
		}

		index -= conditions.size();

		if (index == 0)
			return body.get();

		return nullptr;
	};

	Array<ExpPtr> conditions;
	Array<var> values;
	const bool isNotDefault;
	bool initialized;
	ScopedPointer<BlockStatement> body;
};


struct HiseJavascriptEngine::RootObject::SwitchStatement : public Statement
{
	SwitchStatement(const CodeLocation &l) noexcept: Statement(l) {};

	ResultCode perform(const Scope &s, var *returnValue) const override
	{
		var selectedCase = condition->getResult(s);

		bool caseFound = false;

		for (int i = 0; i < cases.size(); i++)
		{
			cases[i]->initValues(s);

			if (cases[i]->values.contains(selectedCase))
			{
				ResultCode caseResult = cases[i]->body->perform(s, returnValue);

				if (caseResult == Statement::breakWasHit)
				{
					caseFound = true;
					break;
				}
				else if (caseResult == Statement::returnWasHit)
				{
					return Statement::returnWasHit;
				}
			}
		}

		if (!caseFound && defaultCase != nullptr) defaultCase->perform(s, returnValue);

		return Statement::ok;
	}

	Statement* getChildStatement(int index) override
	{
		// too lazy LOL
		return nullptr;
	};

	OwnedArray<CaseStatement> cases;

	ScopedPointer<CaseStatement> defaultCase;
	ExpPtr condition;
};


struct HiseJavascriptEngine::RootObject::VarStatement : public Expression
{
	VarStatement(const CodeLocation& l) noexcept : Expression(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		s.scope->setProperty(name, initialiser->getResult(s));
		return ok;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; }

	bool replaceChildStatement(Ptr& s, Statement* n) override
	{
		return swapIf(s, n, initialiser);
	}

	Identifier name;
	ExpPtr initialiser;
};



struct HiseJavascriptEngine::RootObject::ConstVarStatement : public Statement
{
	ConstVarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		ns->constObjects.set(name, initialiser->getResult(s));

		return ok;
		
	}

	Statement* getChildStatement(int index) override { return index == 0 ? initialiser.get() : nullptr; }

	bool replaceChildStatement(Ptr& s, Statement* n) override
	{
		return swapIf(s, n, initialiser);
	}

	JavascriptNamespace* ns = nullptr;

	Identifier name;
	ExpPtr initialiser;
};



struct HiseJavascriptEngine::RootObject::LoopStatement : public Statement
{
	struct IteratorName : public Expression
	{
		IteratorName(const CodeLocation& l, LoopStatement* loop, Identifier id_) noexcept : Expression(l), loopToUse(loop), id(id_) {}

		var getResult(const Scope& s) const override
		{
			//LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);

			if (auto loop = loopToUse)
			{
				var *data = &loop->currentObject;
				CHECK_CONDITION_WITH_LOCATION(data != nullptr, "data does not exist");

				if (data->isArray())
				{
					if (loop->index >= data->size())
						location.throwError("Loop iterator index invalid. Do not change the array in a for...in loop");

					return data->getArray()->getUnchecked(loop->index);
				}
				else if (data->isBuffer())
					return data->getBuffer()->getSample(loop->index);
				else if (auto obj = data->getDynamicObject())
					return obj->getProperties().getName(loop->index).toString();
				else if (auto fo = dynamic_cast<fixobj::Array*>(data->getObject()))
					return fo->getAssignedValue(loop->index);
				else location.throwError("Illegal iterator target");
			}
			
			return var();
		}

		Statement* getChildStatement(int) override { return nullptr; }

		void assign(const Scope& s, const var& newValue) const override
		{
			LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);
			var *data = &loop->currentObject;
			CHECK_CONDITION_WITH_LOCATION(data != nullptr, "data does not exist");

			if (data->isArray())		data->getArray()->set(loop->index, newValue);
			else if (data->isBuffer())	data->getBuffer()->setSample(loop->index, newValue);
			else if (auto fo = dynamic_cast<fixobj::Array*>(data->getObject()))
			{
				auto v = dynamic_cast<fixobj::ObjectReference*>(fo->getAssignedValue(loop->index).getObject());
				auto s = dynamic_cast<fixobj::ObjectReference*>(newValue.getObject());

				*v = *s;
			}
			else if (auto obj = data->getDynamicObject())	
				*obj->getProperties().getVarPointerAt(loop->index) = newValue;	   
		}

		bool isArray;
		bool isBuffer;

		const Identifier id;
		LoopStatement* loopToUse = nullptr;
	};

	LoopStatement(const CodeLocation& l, bool isDo, bool isIterator_ = false) noexcept : Statement(l), isDoLoop(isDo), isIterator(isIterator_) {}

	ResultCode perform(const Scope& s, var* returnedValue) const override
	{
		if (isIterator)
		{
			CHECK_CONDITION_WITH_LOCATION((currentIterator != nullptr), "Iterator does not exist");
			currentObject = currentIterator->getResult(s);

			ScopedValueSetter<void*> loopScoper(s.currentLoopStatement, (void*)this);
			index = 0;

			int size = 0;

			if (auto ar = currentObject.getArray())
				size = ar->size();
			else if (auto b = currentObject.getBuffer())
				size = b->size;
			else if (auto dynObj = currentObject.getDynamicObject())
				size = dynObj->getProperties().size();
			else if (auto fixStack = dynamic_cast<fixobj::Stack*>(currentObject.getObject()))
				size = fixStack->size();
			else if (auto fixArray = dynamic_cast<fixobj::Array*>(currentObject.getObject()))
				size = fixArray->getConstantValue(0);
			else
			{
				location.throwError("no iterable type");
			}
				
			while (index < size)
			{
				ResultCode r = body->perform(s, returnedValue);

				index++;

				if (r == returnWasHit)
				{
					currentObject = var();
					return r;
				}
				if (r == breakWasHit)    break;
			}

			currentObject = var();

			return ok;
		}
		else
		{
			initialiser->perform(s, nullptr);

			while (isDoLoop || condition->getResult(s))
			{
				s.checkTimeOut(location);
				ResultCode r = body->perform(s, returnedValue);

				if (r == returnWasHit)   return r;
				if (r == breakWasHit)    break;

				iterator->perform(s, nullptr);

				if (isDoLoop && r != continueWasHit && !condition->getResult(s))
					break;
			}

			return ok;
		}
	}

	Statement* getChildStatement(int index) override
	{
		if (isIterator)
		{
			if (index == 0) return currentIterator.get();
			if (index == 1) return body.get();
		}
		else
		{
			if (index == 0) return initialiser.get();
			if (index == 1) return iterator.get();
			if (index == 2) return condition.get();
			if (index == 3) return body.get();
		}
		
		return nullptr;
	}

	bool replaceChildStatement(Ptr& s, Statement* n) override
	{
		return	swapIf(s, n, body) ||
				swapIf(s, n, condition) ||
				swapIf(s, n, currentIterator) ||
				swapIf(s, n, initialiser) ||
				swapIf(s, n, iterator);
	}

	ScopedPointer<Statement> initialiser, iterator, body;

	ExpPtr condition;

	ExpPtr currentIterator;

	bool isDoLoop;

	bool isIterator;

	mutable int index;

	mutable var currentObject;
};


struct HiseJavascriptEngine::RootObject::ReturnStatement : public Statement
{
	ReturnStatement(const CodeLocation& l, Expression* v) noexcept : Statement(l), returnValue(v) {}

	ResultCode perform(const Scope& s, var* ret) const override
	{
		if (ret != nullptr)  *ret = returnValue->getResult(s);
		return returnWasHit;
	}

	Statement* getChildStatement(int index) override { return index == 0 ? returnValue.get() : nullptr; }

	bool replaceChildStatement(Ptr& newChild, Statement* oldChild) override
	{ 
		return swapIf(newChild, oldChild, returnValue);
	};

	ExpPtr returnValue;
};


struct HiseJavascriptEngine::RootObject::BreakStatement : public Statement
{
	BreakStatement(const CodeLocation& l) noexcept : Statement(l) {}
	ResultCode perform(const Scope&, var*) const override  { return breakWasHit; }

	Statement* getChildStatement(int) override { return nullptr; }
};


struct HiseJavascriptEngine::RootObject::ContinueStatement : public Statement
{
	ContinueStatement(const CodeLocation& l) noexcept : Statement(l) {}
	ResultCode perform(const Scope&, var*) const override  { return continueWasHit; }

	Statement* getChildStatement(int) override { return nullptr; }
};

} // namespace hise
