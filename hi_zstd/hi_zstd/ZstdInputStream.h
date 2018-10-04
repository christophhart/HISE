/*  ===========================================================================

BSD License

For Zstandard software

Copyright (c) 2016-present, Facebook, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name Facebook nor the names of its contributors may be used to
endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*   ===========================================================================
*/

#ifndef ZSTD_INPUT_STREAM_H_INCLUDED
#define ZSTD_INPUT_STREAM_H_INCLUDED

namespace zstd {
using namespace juce;

/** A InputStream that decompresses another Input Stream using zstd.
*
*	You can use this like any other InputStream, however seeking is not allowed.
*
*	Note: This is not the most performant way of using the zstd compression.
*		  If performance is critical, use one of the other interfaces.
*
*/
class ZstdInputStream : public InputStream
{
public:

	// =========================================================================

	ZstdInputStream(InputStream* sourceStream);
	~ZstdInputStream();

	// =========================================================================

	virtual int read(void* destBuffer, int maxBytesToRead);
	int64 getTotalLength() override;
	bool isExhausted() override;
	int64 getPosition() override;
	bool setPosition(int64 newPosition) override;

	// =========================================================================

private:

	// =========================================================================

	struct Pimpl;
	ScopedPointer<Pimpl> pimpl;

	// =========================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZstdInputStream);
};

}

#endif