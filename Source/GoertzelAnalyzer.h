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
		window(GoertzelSize, juce::dsp::WindowingFunction<float>::hann) {
    }

    ~GoertzelAnalyzer() {
    }

	void pushSampleIntoFifo(const float& sample) {

		if (fifoIndex == GoertzelSize) {
			juce::zeromem(fftData.data(), sizeof(fftData));
			memcpy(fftData.data(), fifo.data(), sizeof(fifo));
			juce::zeromem(fq_levels, sizeof(fq_levels));
			window.multiplyWithWindowingTable(fftData.data(), GoertzelSize);

			detectGoertzelFrequencies(GoertzelSize, fftData.data());
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

	void detectGoertzelFrequencies(int size, float const *data) {
		for (int i = 0; i < fqSize; i++) {
			fq_levels[i] = goertzel(size, data, m_sampleRate, fq[i]);
		}
	}

	bool checkGoertzelFrequencies() {
		float threshold = 350.0;
		if(fqSize == 1){
			//DBG(fq_levels[0]);
			if (fq_levels[0] > threshold)
				return true;
		}
		//find simultaneous presence of two frequencies in one tone
		for (int i = 0; i < fqSize/2 + 1; i++) {			
			if (fq_levels[i] > threshold && fq_levels[i + fqSize/2] > threshold)//threshold of activation
				return true;
		}		
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

	//not used
	//N is number of time points and fft points
	void dft(float *in, float *out, int N) {

		for (int i = 0, k = 0; k < N; i += 2, k++) {
			out[i] = out[i + 1] = 0.0;

			for (int n = 0; n < N; n++) {
				out[i] += in[n] * cos(k*n*PI2 / N);
				out[i + 1] -= in[n] * sin(k*n*PI2 / N);
			}
			out[i] /= N; out[i + 1] /= N;
		}
	}

	enum {
		//FFT
		fftOrder = 9,
		fftSize = 1 << fftOrder,

		//Goertzel
		//fqSize può essere 1 o un numero pari, se è un numero dispari, l'ultima frequenza non è considerata
		fqSize = 1,
		GoertzelOrder = 12,
		GoertzelSize = 1 << GoertzelOrder,
	};	

private:
	juce::dsp::FFT forwardFFT;                      
	juce::dsp::WindowingFunction<float> window;     
	std::array<float, GoertzelSize> fifo;
	std::array<float, GoertzelSize * 2> fftData;   
	int fifoIndex = 0;   

	double m_sampleRate = 44100.0;
	//48000 / 2048 (2^11) = bins da 23.43 HZ
	//48000 / 4096 (2^12) = bins da 11.71 HZ
	//quindi goertzel settato sul riconoscimento di 16HZ, riconoscerà nell'intorno di +/- 5.85

	const float  PI2 = M_PI * 2;	
	//DTMF
	/*697, 770, 852, 941, 1209, 1336, 1477, 1633*/
	const int fq[fqSize] = {16};
	float fq_levels[fqSize] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GoertzelAnalyzer)
};

