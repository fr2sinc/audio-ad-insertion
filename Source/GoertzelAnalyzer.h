/*
  ==============================================================================

    GoertzelAnalyzer.h
    Created: 27 Jul 2020 9:23:42pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
#define _USE_MATH_DEFINES
#include <JuceHeader.h>
#include <math.h>
#include <cmath>
//==============================================================================
/*
*/


class GoertzelAnalyzer  
{
public:
	GoertzelAnalyzer(): forwardFFT(fftOrder),
		window(GoertzelFFTSize, juce::dsp::WindowingFunction<float>::hann) {
    }

    ~GoertzelAnalyzer() {
    }

	void pushSampleIntoFifo(const float& sample) {

		if (fifoIndex == GoertzelFFTSize) {
			juce::zeromem(fftData.data(), sizeof(fftData));
			memcpy(fftData.data(), fifo.data(), sizeof(fifo));
			juce::zeromem(dtmf_levels, sizeof(dtmf_levels));			

			window.multiplyWithWindowingTable(fftData.data(), GoertzelFFTSize);

			detectDTMF(GoertzelFFTSize, fftData.data());
			fifoIndex = 0;
		}
		fifo[fifoIndex] = sample;
		++fifoIndex;
	}

	void setSampleRate(const double& sampleRate) {

		m_sampleRate = sampleRate;		
	}

	float goertzel(int size, const float *data, int sample_fq, int detect_fq) {

		float omega = static_cast<float> (PI2 * detect_fq / sample_fq);
		float sine = sin(omega);
		float cosine = cos(omega);
		float coeff = cosine * 2;
		float q0 = 0;
		float q1 = 0;
		float q2 = 0;

		for (int i = 0; i < size; i++) {
			q0 = coeff * q1 - q2 + data[i];
			q2 = q1;
			q1 = q0;
		}

		float real = (q1 - q2 * cosine) / (size / 2.0);
		float imag = (q2 * sine) / (size / 2.0);

		float out = sqrt(real * real + imag * imag);
		return out * 1000;
	}

	void detectDTMF(int size, float const *data) {

		for (int i = 0; i < dtmfsSize; i++) {
			dtmf_levels[i] = goertzel(size, data, m_sampleRate, dtmf_fq[i]);
		}
	}

	bool checkDetectDTMF() {

		bool firstTone = false;
		bool secondTone = false;

		//to find the simultaneous presence of two frequencies in one tone
		for (int i = 0; i < dtmfsSize/2; i++) {
			//DBG(dtmf_levels[i]);
			if (dtmf_levels[i] > 350.0)//threshold of activation
				firstTone = true;
		}
		for (int i = dtmfsSize / 2; i < dtmfsSize ; i++) {
			//DBG(dtmf_levels[i]);
			if (dtmf_levels[i] > 350.0)
				secondTone = true;
		}
		//check secondTone if you need a two frequency match
		if (firstTone)
			return true;
		
		return false;
	}

	//not used
	const float& getPeakFrequency() {

		float index = 0.0f;
		float max = 0.0f;
		float absSample;

		for (auto i = 0; i < (fftSize / 2); ++i) {

			absSample = std::abs(fftData[i]);
			if (max < absSample) {
				max = absSample;
				index = i;
			}
		}
		float outFreq = float(index) * m_sampleRate / forwardFFT.getSize(); // Freq = (sampleRate * fftBufferIdx) / bufferSize
		//DBG(outFreq);

		return outFreq;
		/*
		For example, assume you are looking at index number 256 and you are running at a sample rate of 44100 Hz.
		Then the formula says:
		Freq = 44100 * 256 / 1024= 11025
		In other words: the frequeny for the index 256 is 11025 Hz.
		*/
	}

	//not used
	bool checkTone() {
		float freqFound = getPeakFrequency();
		if (freqFound >= 421.74f && freqFound <= 446.17f)
			return true;
		else
			return false;
	}

	enum {
		fftOrder = 9,
		fftSize = 1 << fftOrder,
		dtmfsSize = 9,
		GoertzelFFTOrder = 12,
		GoertzelFFTSize = 1 << GoertzelFFTOrder,
	};


private:
	juce::dsp::FFT forwardFFT;                      
	juce::dsp::WindowingFunction<float> window;     
	std::array<float, GoertzelFFTSize> fifo;
	std::array<float, GoertzelFFTSize * 2> fftData;   
	int fifoIndex = 0;   

	double m_sampleRate = 44100.0;
	//48000 / 2048 (2^11) = bins da 23.43 HZ
	//48000 / 4096 (2^12) = bins da 11.71 HZ
	//quindi goertzel settato sul riconoscimento di 16HZ, riconoscerà nell'intorno di +/- 5.85

	const float  PI2 = M_PI * 2;
	const int dtmf_fq[dtmfsSize] = {16, 697, 770, 852, 941, 1209, 1336, 1477, 1633};
	float dtmf_levels[dtmfsSize] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoertzelAnalyzer)
};

