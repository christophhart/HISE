

struct HiseJavascriptEngine::RootObject::LockStatement : public Statement
{
	LockStatement(CodeLocation l, bool isReadLock_) : Statement(l), isReadLock(isReadLock_), currentLock(nullptr) {};

	ResultCode perform(const Scope& s, var*) const override;

	ExpPtr lockedObj;

	mutable ReadWriteLock* currentLock;

	const bool isReadLock;
};

struct HiseJavascriptEngine::RootObject::BlockStatement : public Statement
{
	BlockStatement(const CodeLocation& l) noexcept : Statement(l) 
	{
		
	}

	ResultCode perform(const Scope& s, var* returnedValue) const override
	{
		ScopedBlockLock sl(s, lockStatements);

		for (int i = 0; i < statements.size(); ++i)
		{
#if ENABLE_SCRIPTING_BREAKPOINTS
			if (statements.getUnchecked(i)->breakpointReference.index != -1)
			{
				Statement* st = statements.getUnchecked(i);
				const auto& loc = st->location;
				int col, line;

				loc.fillColumnAndLines(col, line);
				Breakpoint bp = Breakpoint(st->breakpointReference.localScopeId, loc.externalFile, line, col, loc.getCharIndex(), st->breakpointReference.index);

				const bool hasRootScope = s.root == s.scope.getObject();

				if(!hasRootScope)
					bp.localScope = s.scope.getObject();

				throw bp;
			}
#endif

			if (ResultCode r = statements.getUnchecked(i)->perform(s, returnedValue))
				return r;
		}
			

		return ok;
	}

	struct ScopedBlockLock
	{
		ScopedBlockLock(const Scope& s, const OwnedArray<LockStatement> &statements_):
			statements(statements_),
			numLocks(statements_.size())
		{
			if (numLocks == 0) return;

			SpinLock::ScopedLockType sl(accessLock);

			for (int i = 0; i < numLocks; i++)
			{
				const LockStatement* lock = statements.getUnchecked(i);
				lock->perform(s, nullptr);

				if (lock->currentLock != nullptr)
					lock->isReadLock ? lock->currentLock->enterRead() : lock->currentLock->enterWrite();
			}
		}

		~ScopedBlockLock()
		{
			if (numLocks == 0) return;

			SpinLock::ScopedLockType sl(accessLock);

			for (int i = 0; i < numLocks; i++)
			{
				const LockStatement* lock = statements.getUnchecked(i);

				if (lock->currentLock != nullptr)
					lock->isReadLock ? lock->currentLock->exitRead() : lock->currentLock->exitWrite();
			}
		}

		const OwnedArray<LockStatement>& statements;
		const int numLocks;

		SpinLock accessLock;
	};

	OwnedArray<Statement> statements;
	OwnedArray<LockStatement> lockStatements;
};

struct HiseJavascriptEngine::RootObject::IfStatement : public Statement
{
	IfStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var* returnedValue) const override
	{
		return (condition->getResult(s) ? trueBranch : falseBranch)->perform(s, returnedValue);
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

	OwnedArray<CaseStatement> cases;

	ScopedPointer<CaseStatement> defaultCase;
	ExpPtr condition;
};


struct HiseJavascriptEngine::RootObject::VarStatement : public Statement
{
	VarStatement(const CodeLocation& l) noexcept : Statement(l) {}

	ResultCode perform(const Scope& s, var*) const override
	{
		s.scope->setProperty(name, initialiser->getResult(s));
		return ok;
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

	JavascriptNamespace* ns = nullptr;

	Identifier name;
	ExpPtr initialiser;
};



struct HiseJavascriptEngine::RootObject::LoopStatement : public Statement
{
	struct IteratorName : public Expression
	{
		IteratorName(const CodeLocation& l, Identifier id_) noexcept : Expression(l), id(id_) {}

		var getResult(const Scope& s) const override
		{
			LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);
			var *data = &loop->currentObject;
			CHECK_CONDITION_WITH_LOCATION(data != nullptr, "data does not exist");

			if (data->isArray())
			{
				if (loop->index >= data->size())
					location.throwError("Loop iterator index invalid. Do not change the array in a for...in loop");

				return data->getArray()->getUnchecked(loop->index);
			}
			else if (data->isBuffer())		return data->getBuffer()->getSample(loop->index);
			else if (data->isObject())		return data->getDynamicObject()->getProperties().getName(loop->index).toString();
			else location.throwError("Illegal iterator target");

			return var();
		}

		void assign(const Scope& s, const var& newValue) const override
		{
			LoopStatement *loop = (LoopStatement*)(s.currentLoopStatement);
			var *data = &loop->currentObject;
			CHECK_CONDITION_WITH_LOCATION(data != nullptr, "data does not exist");

			if (data->isArray())		data->getArray()->set(loop->index, newValue);
			else if (data->isBuffer())	data->getBuffer()->setSample(loop->index, newValue);
			else if (data->isObject())	*data->getDynamicObject()->getProperties().getVarPointerAt(loop->index) = newValue;	   
		}

		bool isArray;
		bool isBuffer;

		const Identifier id;
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

			const int size = currentObject.isArray() ? currentObject.getArray()->size() :
							 currentObject.isBuffer() ? currentObject.getBuffer()->size :
							 currentObject.isObject() ? currentObject.getDynamicObject()->getProperties().size() : 0;

			while (index < size)
			{
				ResultCode r = body->perform(s, returnedValue);

				index++;

				if (r == returnWasHit)   return r;
				if (r == breakWasHit)    break;
			}

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

	ExpPtr returnValue;
};


struct HiseJavascriptEngine::RootObject::BreakStatement : public Statement
{
	BreakStatement(const CodeLocation& l) noexcept : Statement(l) {}
	ResultCode perform(const Scope&, var*) const override  { return breakWasHit; }
};


struct HiseJavascriptEngine::RootObject::ContinueStatement : public Statement
{
	ContinueStatement(const CodeLocation& l) noexcept : Statement(l) {}
	ResultCode perform(const Scope&, var*) const override  { return continueWasHit; }
};
