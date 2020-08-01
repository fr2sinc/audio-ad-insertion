/*
  ==============================================================================

    FFT.h
    Created: 27 Jul 2020 5:19:20pm
    Author:  Throw

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class FFT  : public juce::Component
{
public:
    FFT(int order = 10):
		m_order(order),
		m_size(1<<m_order),
		m_size2(2 * m_size),
		m_sizeHalf(m_size/2),
		m_sizeHalfF(float(m_sizeHalf)),
		m_sizeDiv(1.f / float(m_size)),
		m_sizeMaxInv(1.f / float(m_size -1)),
		m_sizeMaxInvPi(m_sizeMaxInv* MathConstants<float>::pi),
		m_fifo(m_size),
		m_fftData(m_size2),
		m_fifoIdx(0),
		m_forwardFFT(order),
		m_sampleRate(1.f), //don't know the sample rate ad this point
		m_sampleRateSizeMaxInv(1.f),
		window(m_size, juce::dsp::WindowingFunction<float>::hann)
    {
    }

    ~FFT() override
    {
    }

	void setSampleRate(const double& sampleRate) {
		m_sampleRate = float(sampleRate);
		m_sampleRateSizeMaxInv = m_sampleRate * m_sizeMaxInv;
	}

	void pushSampleIntoFifo(const float& sample) {
		if (m_fifoIdx == m_size) {
			std::fill(std::next(m_fftData.begin(), m_size), m_fftData.end(), 0.f);
			std::copy(m_fifo.begin(), m_fifo.end(), m_fftData.begin());

			window.multiplyWithWindowingTable(m_fftData.data(), m_size);
			m_forwardFFT.performFrequencyOnlyForwardTransform(m_fftData.data());
			m_fifoIdx = 0;
		}

			m_fifo[m_fifoIdx] = sample;
			++m_fifoIdx;		
	}
	const float& getFundamentalFrequency() {
		float index = 0.f;
		float max = 0.f;
		float absSample;
		for (auto i = 0; i < m_sizeHalf; ++i) {
			absSample = abs(m_fftData[i]);
			if (max < absSample) {
				max = absSample;
				index = i;
			}
		}
		return m_sampleRateSizeMaxInv * index;
	}

    void paint (juce::Graphics& g) override
    {
        /* This demo code just fills the component's background and
           draws some placeholder text to get you started.

           You should replace everything in this method with your own
           drawing code..
        */

        g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));   // clear the background

        g.setColour (juce::Colours::grey);
        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

        g.setColour (juce::Colours::white);
        g.setFont (14.0f);
        g.drawText ("FFT", getLocalBounds(),
                    juce::Justification::centred, true);   // draw some placeholder text
    }

    void resized() override
    {
        // This method is where you should set the bounds of any child
        // components that your component contains..

    }


private:

	int m_order, m_size, m_size2, m_sizeHalf;
	float m_sizeHalfF, m_sizeDiv, m_sizeMaxInv, m_sizeMaxInvPi;
	std::vector<float> m_fifo, m_fftData;
	int m_fifoIdx;

	dsp::FFT m_forwardFFT;
	juce::dsp::WindowingFunction<float> window;

	int m_windowType;
	float m_sampleRate, m_sampleRateSizeMaxInv;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFT)
};

class FFTOsc {
private:
	struct Osc {
		Osc() : m_sampleRate2Inv(0.f), m_inc(0.f), m_idx(0.f) {}
		void setSampleRate(const float& sampleRate) { m_sampleRate2Inv = 2.f / sampleRate; }
		void setFreq(const float& freq) { m_inc = freq * m_sampleRate2Inv; }
		void process() {
			m_idx += m_inc;
			if (m_idx > 1.f)
				m_idx -= 2.f;
		}
		const float& getSin() { return std::sin(m_idx * MathConstants<float>::pi); }
		const float& getSaw() { return m_idx; }
		const float& getSqr() { return m_idx < 0.f ? -1.f : 1.f; }
		float m_sampleRate2Inv, m_inc, m_idx;
	};

	struct EnvelopeFollower {
		EnvelopeFollower(float attack = .7f, float release = .7f) : 
			m_idx(0.f),
			m_cycle(0.f),
			m_attack(attack),
			m_release(release),
			m_sampleRate(0.f),
			m_envelope(0.f)
		{
			setAttack(attack); setRelease(release);
		}
		void setAttack(const float& attack) { m_attack = 1.f - attack; }
		void setRelease(const float& release) { m_release = 1.f - release; }
		void setSampleRate(const float& sampleRate) { m_sampleRate = sampleRate; }
		void setFreq(const float& freq) { m_cycle = m_sampleRate / freq; }
		const float& getEnvelope(float sample, bool saturated= false) {
			sample = std::abs(sample);
			if (m_envelope < sample) {
				m_envelope += m_attack * (sample - m_envelope);
				m_idx = 0.f;
			}
			else {
				if (m_idx < m_cycle) {
					++m_idx;
				}
				else {
					m_envelope += m_release * (sample - m_envelope);
					m_idx -= m_cycle;
				}
				if (saturated)
					return std::tanh(2.f * m_envelope);
				return m_envelope;
			}
		}

		float m_idx, m_cycle, m_attack, m_release, m_sampleRate, m_envelope;
	};
public:
	enum WaveForms {
		WFSine,
		WFSaw,
		WFSqr
	};
	FFTOsc(float filter = .5f, Range<float> frequencyRange = Range<float>(80.f, 19000.f)) :
		m_filter(filter),
		m_freq(0.f),
		m_envelope(0.f),
		m_frequencyRange(frequencyRange) {
		setFilter(filter);
	}
	void setFilter(float filter ) { m_filter = filter; }
		void setSampleRate(const double& sampleRate) {
			auto m_sampleRate = float(sampleRate);
			osc.setSampleRate(m_sampleRate);
			env.setSampleRate(m_sampleRate);
	}
	void setFreq(const float& freq) {
		if (!freq > m_frequencyRange.getEnd() || freq < m_frequencyRange.getStart())
			m_freq = m_freq + m_filter * (freq - m_freq);
		osc.setFreq(m_freq);
		env.setFreq(m_freq);
	}
	void process(const float& sample = 1.f) {
		osc.process();
		m_envelope = env.getEnvelope(sample);
	}
	const float& getWave(WaveForms wf = WFSine) {
		switch (wf) {
			case WFSine: return osc.getSin() * m_envelope;
			case WFSaw: return osc.getSaw() * m_envelope;
			case WFSqr: return osc.getSqr() * m_envelope;
		}
	}
private:
	float m_filter, m_freq, m_envelope;
	Range<float> m_frequencyRange;
	Osc osc;
	EnvelopeFollower env;
};

/*float goertzel(int numSamples, float const* data, float SAMPLING_RATE, int TARGET_FREQUENCY)
{
	int     k, i;
	float   floatnumSamples;
	float   omega, sine, cosine, coeff, q0, q1, q2, magnitude, real, imag;

	float   scalingFactor = numSamples / 2.0f;

	floatnumSamples = static_cast<float> (numSamples);
	k = static_cast<int>((0.5 + ((floatnumSamples * TARGET_FREQUENCY) / SAMPLING_RATE)));
	omega = (2.0 * M_PI * k) / floatnumSamples;
	sine = sin(omega);
	cosine = cos(omega);
	coeff = 2.0 * cosine;
	q0 = 0;
	q1 = 0;
	q2 = 0;

	for (i = 0; i < numSamples; i++)
	{
		q0 = coeff * q1 - q2 + data[i];
		q2 = q1;
		q1 = q0;
	}

	// calculate the real and imaginary results
	// scaling appropriately
	real = (q1 - q2 * cosine) / scalingFactor;
	imag = (q2 * sine) / scalingFactor;

	magnitude = sqrtf(real * real + imag * imag);
	return magnitude;
}*/



//How to convert two channels in one monoChannel

auto chInv = 1.0f / float(buffer.getNumChannels());
DBG(fft.getPeakFrequency());
for (auto s = 0; s < buffer.getNumSamples(); ++s) {
	auto sample = 0.f;
	for (auto ch = 0; ch < buffer.getNumChannels(); ++ch) {
		auto * channelData = buffer.getReadPointer(ch, s);
		sample += *channelData;
	}
	sample *= chInv;
	fft.pushSampleIntoFifo(sample);
}	

//on / off sound
auto* channelData = buffer.getWritePointer(channel);

if (toneOn)//shutDown sound
	for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
		channelData[sample] = channelData[sample] * Decibels::decibelsToGain(-60.0f);
	}


if (fft.checkDetectDTMF() && timeCounter > 150) {
				if (!toneOn) {
					toneOn = true;
					newTonoState = On;
					timeCounter = 0;
				}
				else {
					toneOn = false;
					newTonoState = Off;
					timeCounter = 0;
				}
}

//old injection
if (toneOn) {

		audioInjection = true;

		//changeState(Starting);
		if (state != Starting) {

			auto file = File("C:\\Users\\Throw\\Music\\Changes\\Changes CD 1 TRACK 1 (320).mp3");
			auto* reader = formatManager.createReaderFor(file);                    // [10]

			if (reader != nullptr)
			{
				std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true)); // [11]
				transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);                                 // [12]

				readerSource.reset(newSource.release());                                                                    // [14]
			}

			transportSource.getNextAudioBlock(AudioSourceChannelInfo(buffer));

			changeState(Starting);
		}

	}
