/*
  ==============================================================================

    FFT.h
    Created: 27 Jul 2020 9:23:42pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

//==============================================================================
/*
*/
class FFT  
{
public:
    FFT(): forwardFFT(fftOrder),
		window(fftSize, juce::dsp::WindowingFunction<float>::hann) {

    }

    ~FFT() {
		
    }

	void pushSampleIntoFifo(const float& sample) {
		if (fifoIndex == fftSize) {

			juce::zeromem(fftData.data(), sizeof(fftData));
			memcpy(fftData.data(), fifo.data(), sizeof(fifo));

			window.multiplyWithWindowingTable(fftData.data(), fftSize);
			forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
			fifoIndex = 0;
		}

		fifo[fifoIndex] = sample;
		++fifoIndex;
	}

	void setSampleRate(const double& sampleRate) {
		m_sampleRate = float(sampleRate);		
	}

	const float& getFundamentalFrequency() {
		float index = 0.0f;
		float max = 0.0f;
		float absSample;

		for (auto i = 0; i < (fftSize/2); ++i) {

			absSample = std::abs(fftData[i]);
			if (max < absSample) {
				max = absSample;
				index = i;
			}
		}
		float outFreq = float(index) * m_sampleRate / forwardFFT.getSize(); // Freq = (sampleRate * fftBufferIdx) / bufferSize
		DBG(outFreq);

		return outFreq;

		/*
		For example, assume you are looking at index number 256 and you are running at a sample rate of 44100 Hz.
		Then the formula says:
		Freq = 44100 * 256 / 1024= 11025
		In other words: the frequeny for the index 256 is 11025 Hz.
		*/
	}

	bool checkTone() {
		//range: 421.74 - 445.17
		float freqFound = getFundamentalFrequency();
		if (freqFound >= 421.74f && freqFound <= 446.17f)
			return true;
		else
			return false;
	}

	enum {
		fftOrder = 11,            
		fftSize = 1 << fftOrder
	};

private:
	juce::dsp::FFT forwardFFT;                      
	juce::dsp::WindowingFunction<float> window;     
	std::array<float, fftSize> fifo;
	std::array<float, fftSize * 2> fftData;   
	int fifoIndex = 0;   

	float m_sampleRate = 48000.0;
	//48000 / 2048 = bins da 23.43 HZ

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFT)
};


