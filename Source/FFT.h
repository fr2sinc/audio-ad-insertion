/*
  ==============================================================================

    FFT.h
    Created: 27 Jul 2020 9:23:42pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
#define _USE_MATH_DEFINES
#include <JuceHeader.h>
#include <math.h>
#include <cmath>
#include <vector>
#include <list>
#include <unordered_map> 
#include "DataPoint.h"
#include <iostream>
#include <algorithm> 
#include <limits>
#include <thread>
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
		m_sampleRate = float(sampleRate);		
	}

	const float& getPeakFrequency() {
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
		float freqFound = getPeakFrequency();
		if (freqFound >= 421.74f && freqFound <= 446.17f)
			return true;
		else
			return false;
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
			DBG(dtmf_levels[i]);
			if (dtmf_levels[i] > 350.0)//threshold of activation
				firstTone = true;
		}
		for (int i = dtmfsSize / 2; i < dtmfsSize ; i++) {
			DBG(dtmf_levels[i]);
			if (dtmf_levels[i] > 350.0)
				secondTone = true;
		}
		if (firstTone)
			return true;
		
		return false;
	}	

	void generateHashes(int songId, bool isMatching, juce::String input_file = "") {

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
		
		const float* bufferData = fileBuffer.getReadPointer(0, 0);

		std::array<float, fftSize> inputBuffer;
		std::array<float, 2*fftSize> outBuffer;
		int sect = 0;
		
		int freqbandWidth = 40;
		int t = 0; //timeFrame Counter


		for (int i = 0; i < samples; i++) {
			if (sect < fftSize) {//fill the inputBuffer
				
				inputBuffer[sect] = bufferData[i];
				sect++;
			}
			else {
				sect = 0;
				i -= 1;
				
				//fft_plan plan = fft_plan_dft_r2c_1d(N, inputBuffer, outBuffer, 0);

				juce::zeromem(outBuffer.data(), sizeof(outBuffer));
				memcpy(outBuffer.data(), inputBuffer.data(), sizeof(inputBuffer));

				forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation
				//fft_execute(plan);
				//fft_destroy_plan(plan);

				float freq1 = 0, freq2 = 0, freq3 = 0, freq4 = 0;
				int pt1 = 0, pt2 = 0, pt3 = 0, pt4 = 0;

				int freqbandWidth2 = freqbandWidth * 2; //80
				int freqbandWidth3 = freqbandWidth * 3; //120
				int freqbandWidth4 = freqbandWidth3 + 60; //180
				int freqbandWidth5 = freqbandWidth4 + 120; //300
				

				for (int k = freqbandWidth; k < freqbandWidth5; k++) {

					//int freq = (outBuffer[k] > 0) ? (int)outBuffer[k] : (int)(0 - outBuffer[k]);

					//int Magnitude = (int)(log10f((freq + 1)) * 1000);

					float Magnitude = outBuffer[k];

					if (k >= freqbandWidth && k < freqbandWidth2 && Magnitude > freq1) {
						freq1 = Magnitude;
						pt1 = k;
					}
					else if (k >= freqbandWidth2 && k < freqbandWidth3 && Magnitude > freq2) {
						freq2 = Magnitude;
						pt2 = k;
					}
					else if (k >= freqbandWidth3 && k < freqbandWidth4 && Magnitude > freq3) {
						freq3 = Magnitude;
						pt3 = k;
					}
					else if (k >= freqbandWidth4 && k < freqbandWidth5 && Magnitude > freq4) {
						freq4 = Magnitude;
						pt4 = k;
					}
					
				}
				
				long long h = hash(freqbandWidth, pt1, pt2, pt3);

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
				}
				t++;
			}
		}	
	}
	
	void generateHashesOverlap(int songId, bool isMatching, juce::String input_file = "") {		

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
		int sect = 0;

		int freqbandWidth = 40;
		int mread = 0;

		for (int t = 0; t < samples/(fftSize/2); t++) {			

			memcpy(outBuffer.data(), fileBuffer.getReadPointer(0, mread), fftSize * sizeof(float));								
			mread += fftSize / 2;

			window.multiplyWithWindowingTable(outBuffer.data(), fftSize);
			forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation

			float freq1 = 0, freq2 = 0, freq3 = 0, freq4 = 0;
			int pt1 = 0, pt2 = 0, pt3 = 0, pt4 = 0;

			int freqbandWidth2 = freqbandWidth * 2; //80
			int freqbandWidth3 = freqbandWidth * 3; //120
			int freqbandWidth4 = freqbandWidth3 + 60; //180
			int freqbandWidth5 = freqbandWidth4 + 120; //300


			for (int k = freqbandWidth; k < freqbandWidth5; k++) {

				//int freq = (outBuffer[k] > 0) ? (int)outBuffer[k] : (int)(0 - outBuffer[k]);

				//int Magnitude = (int)(log10f((freq + 1)) * 1000);
				float Magnitude = outBuffer[k];

				if (k >= freqbandWidth && k < freqbandWidth2 && Magnitude > freq1) {
					freq1 = Magnitude;
					pt1 = k;
				}
				else if (k >= freqbandWidth2 && k < freqbandWidth3 && Magnitude > freq2) {
					freq2 = Magnitude;
					pt2 = k;
				}
				else if (k >= freqbandWidth3 && k < freqbandWidth4 && Magnitude > freq3) {
					freq3 = Magnitude;
					pt3 = k;
				}
				else if (k >= freqbandWidth4 && k < freqbandWidth5 && Magnitude > freq4) {
					freq4 = Magnitude;
					pt4 = k;
				}

			}			

			long long h = hash(freqbandWidth, pt1, pt2, pt3);

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
			}			
			
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



	void pushSampleIntoSongMatchFifo(const juce::AudioBuffer<float>& tmpBuffer, const int bufferLength) {
		int totSamples = (int) m_sampleRate * 15; // up to 15 seconds of samples

		int samplesToCopy = bufferLength;

		if (totSamples - songMatchFifoIndex < bufferLength)
			samplesToCopy = (totSamples - songMatchFifoIndex);

		const float* bufferData = tmpBuffer.getReadPointer(0);
		songMatchFifo.copyFrom(0, songMatchFifoIndex, bufferData, samplesToCopy);

		if (totSamples - songMatchFifoIndex < bufferLength) {
			//Soluzione esterna: affida il match ad audfprint
			/*writeAudioFileOnDisk(songMatchFifo);

			std::thread t([]() {				
				//ShellExecute(0, "open", "cmd.exe", "/C ipconfig > out.txt", 0, SW_HIDE);
				std::system("cd C:\\Users\\Throw\\Documents\\JuceProjects\\FingerprintRepo\\audfprint-master && python audfprint.py match --dbase fpdbase.pklz C:\\Users\\Throw\\Documents\\JuceProjects\\audio-ad-insertion\\Builds\\VisualStudio2017\\sampleToMatch.wav >> myoutput.txt");
			});
			t.detach();*/

			//Soluzione interna : affida il match a generateHashes
			generateHashes(100, true);
			songMatchFifo.clear();
			songMatchFifoIndex = 0;
		}

			songMatchFifoIndex += bufferLength;
	}

	

	void writeAudioFileOnDisk(const juce::AudioBuffer<float>& tmpBuffer) {

		WavAudioFormat format;
		File file = File::getCurrentWorkingDirectory().getChildFile("sampleToMatch.wav");
		if (file.exists())
			file.deleteFile();
		file.create();
		std::unique_ptr<AudioFormatWriter> writer;
		writer.reset(format.createWriterFor(new FileOutputStream(file),
			m_sampleRate,
			tmpBuffer.getNumChannels(),
			16,
			{},
			0));
		if (writer != nullptr)
			writer->writeFromAudioSampleBuffer(tmpBuffer, 0, tmpBuffer.getNumSamples());

	}

	void setSongMatchBuffer() {
		songMatchFifo.setSize(1, (int)m_sampleRate * 15);
	}

	enum {
		fftOrder = 9,
		fftSize = 1 << fftOrder,
		dtmfsSize = 9,
		N = 512,
		GoertzelFFTOrder = 12,
		GoertzelFFTSize = 1 << GoertzelFFTOrder,
	};

private:
	long long hash(long long p1, long long p2, long long p3, long long p4) {
		return (p4 - (p4 % FUZ_FACTOR)) * 100000000 + (p3 - (p3 % FUZ_FACTOR))
			* 100000 + (p2 - (p2 % FUZ_FACTOR)) * 100
			+ (p1 - (p1 % FUZ_FACTOR));
	}
	const int FUZ_FACTOR = 2;
	

private:
	juce::dsp::FFT forwardFFT;                      
	juce::dsp::WindowingFunction<float> window;     
	std::array<float, GoertzelFFTSize> fifo;
	std::array<float, GoertzelFFTSize * 2> fftData;   
	int fifoIndex = 0;   

	float m_sampleRate = 48000.0;
	//48000 / 2048 (2^11) = bins da 23.43 HZ
	//48000 / 4096 (2^12) = bins da 11.71 HZ
	//quindi goertzel settato sul riconoscimento di 16HZ, riconoscerà nell'intorno di +/- 5.85

	const float  PI2 = M_PI * 2;
	const int dtmf_fq[dtmfsSize] = {16, 697, 770, 852, 941, 1209, 1336, 1477, 1633};
	float dtmf_levels[dtmfsSize] = {};

	std::unordered_map<long long, std::list<DataPoint>> hashMap;
	std::unordered_map<int, std::unordered_map<int, int>> matchMap; // Map<SongId, Map<Offset, Count>>

	//necessario per mantenere uno stato
	juce::AudioBuffer<float> songMatchFifo;
	int songMatchFifoIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFT)
};

