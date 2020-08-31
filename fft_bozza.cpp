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


	void generateLandmark(int songId, bool isMatching, juce::String input_file = "") {

		const int NFFT = fftSize;
		// frequency window to generate landmark pairs, in units of DF = SAMPLING_RATE / NFFT. Values between 0 and NFFT/2
		const int IF_MIN = 0; // you can increase this to avoid having fingerprints for low frequencies
		const int IF_MAX = NFFT / 2; // you don't really want to decrease this, better reduce SAMPLING_RATE instead for faster computation.
		std::array<float, NFFT / 2> threshold;

		const int MNLM = 5;
		// maximum number of local maxima for each spectrum. useful to tune the amount of fingerprints at output

		// prepare the values of exponential masks.
		const int MASK_DF = 3; // mask decay scale in DF units on the frequency axis.	

		float EWW[NFFT / 2][NFFT / 2];
		for (int i = 0; i < NFFT / 2; i++) {
			for (int j = 0; j < NFFT / 2; j++) {
				EWW[i][j] = -0.5* pow((j - i) / MASK_DF / sqrt(i + 3), 2); // gaussian mask is a polynom when working on the log-spectrum. log(exp()) = Id()
				// MASK_DF is multiplied by Math.sqrt(i+3) to have wider masks at higher frequencies
				// see the visualization out-thr.png for better insight of what is happening
			}
		}

		struct Triplet
		{
			double t;
			std::vector<double> i, v;
		};
		std::vector<Triplet> marks;
		const int PRUNING_DT = 24;
		const double MASK_DECAY_LOG = log(0.995); // threshold decay factor between frames.
		const int WINDOW_DT = 96; // a little more than 1 sec.
		const int WINDOW_DF = 60; // we set this to avoid getting fingerprints linking very different frequencies.

		std::vector<double> tcodes;
		std::vector<double> hcodes;

		const int MPPP = 3;


		File file(input_file);
		AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		juce::AudioBuffer<float> fileBuffer;
		int samples;
		if (!isMatching) {

			std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file)); // [2]			

			if (reader.get() != nullptr) {
				auto duration = (float)reader->lengthInSamples / reader->sampleRate;               // [3]

				fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);  // [4]
				reader->read(&fileBuffer,                                                      // [5]
					0,                                                                //  [5.1]
					(int)reader->lengthInSamples,                                    //  [5.2]
					0,                                                                //  [5.3]
					true,                                                             //  [5.4]
					true);                                                            //  [5.5]
																				  // [6]
			}
			//samples = fileBuffer.getNumSamples();
			samples = (int)m_sampleRate * 15;
		}
		else {
			fileBuffer = songMatchFifo;
			samples = (int)m_sampleRate * 15;
		}

		

		
		std::array<float, 2 * fftSize> outBuffer;	
		int mread = 0;
		int freqbandWidth = 40;	


		for (int t = 0; t < samples / (fftSize / 2); t++) {

			memcpy(outBuffer.data(), fileBuffer.getReadPointer(0, mread), fftSize * sizeof(float));
			mread += fftSize / 2;

			window.multiplyWithWindowingTable(outBuffer.data(), fftSize);
			forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation

			// log-normal surface
			for (int i = IF_MIN; i < IF_MAX; i++) {
				// the lower part of the spectrum is damped, the higher part is boosted, leading to a better peaks detection.
				outBuffer[i] = abs(outBuffer[i])* sqrt(i + 16);
			}

			// positive values of the difference between log spectrum and threshold				
			std::array<double, NFFT / 2> diff;
			for (int i = IF_MIN; i < IF_MAX; i++) {
				diff[i] = std::max(log(std::max(1e-6, (double)outBuffer[i])) - threshold[i], 0.0);
			}

			// find at most MNLM local maxima in the spectrum at this timestamp.
			std::vector<double> iLocMax(MNLM);
			std::vector<double> vLocMax(MNLM);

			for (int i = 0; i < MNLM; i++) {
				iLocMax[i] = NAN;
				vLocMax[i] = std::numeric_limits<double>::lowest();
			}
			for (int i = IF_MIN + 1; i < IF_MAX - 1; i++) {
				//console.log("checking local maximum at i=" + i + " data[i]=" + data[i] + " vLoc[last]=" + vLocMax[MNLM-1] );
				if (diff[i] > diff[i - 1] && diff[i] > diff[i + 1] && outBuffer[i] > vLocMax[MNLM - 1]) { // if local maximum big enough
					// insert the newly found local maximum in the ordered list of maxima
					for (int j = MNLM - 1; j >= 0; j--) {
						// navigate the table of previously saved maxima
						if (j >= 1 && outBuffer[i] > vLocMax[j - 1]) continue;
						for (int k = MNLM - 1; k >= j + 1; k--) {
							iLocMax[k] = iLocMax[k - 1];	// offset the bottom values
							vLocMax[k] = vLocMax[k - 1];
						}
						iLocMax[j] = i;
						vLocMax[j] = outBuffer[i];
						break;
					}
				}
			}

			// now that we have the MNLM highest local maxima of the spectrum,
			// update the local maximum threshold so that only major peaks are taken into account.
			for (int i = 0; i < MNLM; i++) {
				if (vLocMax[i] > std::numeric_limits<double>::lowest()) {
					for (int j = IF_MIN; j < IF_MAX; j++) {
						int tmp = (int)iLocMax[i];
						threshold[j] = std::max(threshold[j], log(outBuffer[iLocMax[i]]) + EWW[tmp][j]);
					}
				}
				else {
					vLocMax.erase(vLocMax.begin()+ i, vLocMax.begin() + i + MNLM - i); // remove the last elements.
					iLocMax.erase(iLocMax.begin() + i, iLocMax.begin() + i + MNLM - i);
					break;
				}
			}

			
			// array that stores local maxima for each time step
			//marks.push({ "t": Math.round(this.stepIndex / STEP), "i" : iLocMax, "v" : vLocMax });
			marks.push_back({(double)t, iLocMax, vLocMax});
			// remove previous (in time) maxima that would be too close and/or too low.
			int nm = marks.size() ;
			int t0 = nm - PRUNING_DT - 1;
			for (int  i = nm - 1; i >= std::max(t0 + 1, 0); i--) {
				//console.log("pruning ntests=" + this.marks[i].v.length);
				for (int j = 0; j < marks[i].v.size(); j++) {
					//console.log("pruning " + this.marks[i].v[j] + " <? " + this.threshold[this.marks[i].i[j]] + " * " + Math.pow(this.mask_decay, lenMarks-1-i));
					if (marks[i].i[j] != 0 && log(marks[i].v[j]) < threshold[marks[i].i[j]] + MASK_DECAY_LOG * (nm - 1 - i)) {

						marks[i].v[j] = std::numeric_limits<double>::lowest();
						marks[i].i[j] = std::numeric_limits<double>::lowest();
					}
				}
			}

			// generate hashes for peaks that can no longer be pruned. stepIndex:{f1:f2:deltaindex}
			int nFingersTotal = 0;
			if (t0 >= 0) {
				struct Triplet m = marks[t0];

			loopCurrentPeaks:
				for (int i = 0; i < m.i.size(); i++) {
					int nFingers = 0;

				loopPastTime:
					for (int j = t0; j >= std::max(0, t0 - WINDOW_DT); j--) {

						struct Triplet m2 = marks[j];

					loopPastPeaks:
						for (int k = 0; k < m2.i.size(); k++) {
							if (m2.i[k] != m.i[i] && abs(m2.i[k] - m.i[i]) < WINDOW_DF) {
								tcodes.push_back(m.t); //Math.round(this.stepIndex/STEP));
								// in the hash: dt=(t0-j) has values between 0 and WINDOW_DT, so for <65 6 bits each
								//				f1=m2.i[k] , f2=m.i[i] between 0 and NFFT/2-1, so for <255 8 bits each.
								hcodes.push_back(m2.i[k] + NFFT / 2 * (m.i[i] + NFFT / 2 * (t0 - j)));
								nFingers += 1;
								nFingersTotal += 1;

								if (nFingers >= MPPP) goto loopCurrentPeaks;
							}
						}
					}
				}
			}

			// decrease the threshold for the next iteration
			for (int j = 0; j < threshold.size(); j++) {
				threshold[j] += MASK_DECAY_LOG;
			}

			

			/*long long h = hash(freqbandWidth, pt1, pt2, pt3);

			if (isMatching) {
				std::list<DataPoint> listPoints;

				if (hashMap.find(h) != hashMap.end()) {
					listPoints = hashMap.find(h)->second;

					for (DataPoint dP : listPoints) {

						int offset = std::abs(dP.getTime() - t);

						std::unordered_map<int, int> tmpMap;

						if ((matchMap.find(dP.getSongId())) == matchMap.end()) {

							tmpMap.insert(std::pair<int, int>(offset, 1));

							matchMap.insert(std::pair<long long, std::unordered_map<int, int>>(dP.getSongId(), tmpMap));

						}
						else {
							tmpMap = matchMap.find(dP.getSongId())->second;

							if ((tmpMap.find(offset)) == tmpMap.end()) {
								tmpMap.insert(std::pair<int, int>(offset, 1));
							}
							else {
								int count = tmpMap.find(offset)->second;
								tmpMap.insert(std::pair<int, int>(offset, count + 1));
							}
						}
					}
				}

			}
			else {
				std::list<DataPoint> listPoints;

				if (hashMap.find(h) == hashMap.end()) {

					DataPoint point = DataPoint((int)songId, t);
					listPoints.push_back(point);
					hashMap.insert(std::pair<long long, std::list<DataPoint>>(h, listPoints));
				}
				else {

					DataPoint point = DataPoint(songId, t);
					hashMap.find(h)->second.push_back(point);
				}
			}*/
				
		}
	}
