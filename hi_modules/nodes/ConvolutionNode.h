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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

#if 0
struct ConvolutionNode : public HiseDspBase
{
	void setProperty(NodeBase* parent, Identifier id, const var newValue)
	{
		auto mc = parent->getScriptProcessor()->getMainController_();

		if (newValue.isString())
		{
			PoolReference ref(mc, newValue, FileHandlerBase::SubDirectories::AudioFiles);
			reference = mc->getCurrentAudioSampleBufferPool()->loadFromReference(ref, PoolHelpers::LoadAndCacheWeak);
		}

	}

	void prepare(int numChannels, double sampleRate, int blockSize)
	{
		MultithreadedConvolver c2;

		for (auto c : convolvers)
		{
			c2.
		}
	}

	void process(ProcessData& d)
	{
		int channelIndex = 0;

		for (auto c : convolvers)
		{
			auto input = d.data[channelIndex];
			auto output = (float*)alloca(d.size * sizeof(float));

			c->process(input, output, d.size);
			FloatVectorOperations::copy(input, output, d.size);
			channelIndex++;
		}
	}

	AudioSampleBuffer impulse;

	OwnedArray<hise::MultithreadedConvolver> convolvers;

	AudioSampleBufferPool::ManagedPtr reference;
};
#endif

}
