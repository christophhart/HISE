/*
 * This file is part of the HISE loris_library codebase (https://github.com/christophhart/loris-tools).
 * Copyright (c) 2023 Christoph Hart
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace hise
{
	using namespace juce;

	/** A minimal POD that can be used to check the thread state across DLL boundaries. */
	class ThreadController : public ReferenceCountedObject
	{
		struct Scaler
		{
			Scaler(bool isStep_ = false) :
				isStep(isStep_)
			{}

			double getScaledProgress(double input) const
			{
				if (isStep)
					return (v1 + input) / v2;
				else
					return v1 + (v2 - v1) * input;
			}

			bool isStep = false;
			double v1 = 0.0;
			double v2 = 0.0;
		};

		template <bool IsStep> struct ScopedScaler
		{
			template <typename T> ScopedScaler(ThreadController* parent_, T v1, T v2) : parent(parent_)
			{
				Scaler s(IsStep);
				s.v1 = (double)v1;
				s.v2 = (double)v2;

				if (parent != nullptr)
					parent->pushProgressScaler(s);
			};

			~ScopedScaler()
			{
				if (parent != nullptr)
					parent->popProgressScaler();
			};

			operator bool() const { return parent; }
			ThreadController* parent;
		};

	public:

		using Ptr = ReferenceCountedObjectPtr<ThreadController>;
		using ScopedRangeScaler = ScopedScaler<false>;
		using ScopedStepScaler = ScopedScaler<true>;

		ThreadController(Thread* t, double* p, int timeoutMs, uint32& lastTime_) :
			juceThreadPointer(t),
			progress(p),
			timeout(timeoutMs),
			lastTime(&lastTime_)
		{};

		ThreadController() :
			juceThreadPointer(nullptr),
			progress(nullptr),
			lastTime(nullptr)
		{};

		operator bool() const
		{
			if (juceThreadPointer == nullptr)
				return false;

			auto thisTime = Time::getMillisecondCounter();

			if (lastTime != nullptr && *lastTime != 0 && thisTime - *lastTime > timeout)
			{
				// prevent the jassert above to mess up subsequent timeouts...
				thisTime = Time::getMillisecondCounter();
			}

			if (lastTime != nullptr)
				*lastTime = thisTime;

			return !static_cast<Thread*>(juceThreadPointer)->threadShouldExit();
		}

		/** Allow a bigger time between calls. */
		void extendTimeout(uint32 milliSeconds)
		{
			if (lastTime != nullptr)
				*lastTime += milliSeconds;
		}


		/** Set a progress. If you want to add a scaler to the progress (for indicating a subprocess, use either ScopedStepScaler or ScopedRangeScalers). */
		bool setProgress(double p)
		{
			if (progress == nullptr)
				return true;

			for (int i = progressScalerIndex-1; i >= 0; i--)
			{
				p = jlimit(0.0, 1.0, progressScalers[i].getScaledProgress(p));
			}

			// If this hits, you might have forgot a scaler in the call stack...
			jassert(*progress <= p);

			*progress = p;

			return *this;
		}

	private:

		static constexpr int NumProgressScalers = 32;

		void pushProgressScaler(const Scaler& f)
		{
			progressScalers[progressScalerIndex++] = f;
			jassert(isPositiveAndBelow(progressScalerIndex, NumProgressScalers));
			setProgress(0.0);
		}

		void popProgressScaler()
		{
			progressScalers[progressScalerIndex--] = {};
			jassert(progressScalerIndex >= 0);
		}

		void* juceThreadPointer = nullptr;
		double* progress = nullptr;
		mutable uint32* lastTime = nullptr;
		uint32 timeout = 0;
		int progressScalerIndex = 0;
		Scaler progressScalers[NumProgressScalers];

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThreadController);
	};
}