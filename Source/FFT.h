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

	//not used
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

	//not used
	bool checkTone() {		
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
			if (dtmf_levels[i] > 80.0)//threshold of activation
				firstTone = true;
		}
		for (int i = dtmfsSize / 2; i < dtmfsSize ; i++) {
			DBG(dtmf_levels[i]);
			if (dtmf_levels[i] > 350.0)
				secondTone = true;
		}
		//check secondTone if you need a two frequency match
		if (firstTone)
			return true;
		
		return false;
	}	

	void generateHashes(int songId, bool isMatching, juce::String input_file = "") {		
		
		juce::AudioBuffer<float> fileBuffer;
		int samples;
		File file(input_file);
		juce::String filename = file.getFileName();
		//String filename = input_file;

		if (!isMatching) {
			//increment totNumber of Songs
			nrSongs++;
			
			AudioFormatManager formatManager;
			formatManager.registerBasicFormats();

			std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file)); 			

			if (reader.get() != nullptr) {
				auto duration = (float)reader->lengthInSamples / reader->sampleRate;               

				fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);  
				reader->read(&fileBuffer,                                                      
					0,                                                                
					(int)reader->lengthInSamples,                                    
					0,                                                                
					true,                                                             
					true);																				  
			}						
		}
		else { //isMatching		
			fileBuffer = songMatchFifo;			
		}

		//set num of Samples (<= 10 seconds of samples)
		if (fileBuffer.getNumSamples() < (int)m_sampleRate * 10) {
			samples = fileBuffer.getNumSamples();
		}
		else {
			samples = (int)m_sampleRate * 10;
		}		
		
		int freqbandWidth = 40;		
		std::array<float, 2 * fftSize> outBuffer;		
		int mread = 0;

		//t = timeframe counter
		for (int t = 0; t < samples / fftSize; t++) {

			memcpy(outBuffer.data(), fileBuffer.getReadPointer(0, mread), fftSize * sizeof(float));
			mread += fftSize;			

			forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation
			
			float magf1 = 0, magf2 = 0, magf3 = 0, magf4 = 0;
			int pt1 = 0, pt2 = 0, pt3 = 0, pt4 = 0;

			int freqbandWidth2 = 80;
			int freqbandWidth3 = 120;
			int freqbandWidth4 = 180;
			int freqbandWidth5 = 256;
				

			for (int k = freqbandWidth; k < freqbandWidth5; k++) {

				float Magnitude = outBuffer[k];

				if (k >= freqbandWidth && k < freqbandWidth2 && Magnitude > magf1) {
					magf1 = Magnitude;
					pt1 = k;
				}
				else if (k >= freqbandWidth2 && k < freqbandWidth3 && Magnitude > magf2) {
					magf2 = Magnitude;
					pt2 = k;
				}
				else if (k >= freqbandWidth3 && k < freqbandWidth4 && Magnitude > magf3) {
					magf3 = Magnitude;
					pt3 = k;
				}
				else if (k >= freqbandWidth4 && k < freqbandWidth5 && Magnitude > magf4) {
					magf4 = Magnitude;
					pt4 = k;
				}
					
			}
						
			long long h = hash(pt1, pt2, pt3, pt4);
			writePeaksOnDisk(t, filename, pt1, pt2, pt3, pt4, h);

			if (isMatching) {

				std::list<DataPoint> listPoints;

				if (hashMap.find(h) != hashMap.end()) { //se ha trovato un hash esistente gi� in memoria uguale a quello attuale
					listPoints = hashMap.find(h)->second;

					for (DataPoint dP : listPoints) {

						int offset = std::abs(dP.getTime() - t);
							

						if ((matchMap.find(dP.getSongId())) == matchMap.end()) { //se � il primo match di hash di una canzone nuova
							std::unordered_map<int, int> tmpMap;
							tmpMap.insert(std::pair<int, int>(offset, 1));

							matchMap.insert(std::pair<int, std::unordered_map<int, int>>(dP.getSongId(), tmpMap));

						}
						else { //se esistenvano gi� dei match per quella canzone
								
							std::unordered_map<int, int> tmpMap;
							tmpMap = matchMap.find(dP.getSongId())->second;

							if ((tmpMap.find(offset)) == tmpMap.end()) {									
								matchMap.find(dP.getSongId())->second.insert(std::pair<int, int>(offset, 1));
							}
							else {
								int count = tmpMap.find(offset)->second;									
								matchMap.find(dP.getSongId())->second.find(offset)->second = count+1;
							}
						}
					}
				}

			}
			else { //nel caso in cui sta caricando in memoria gli hash di tutte le tracce/jingle
				std::list<DataPoint> listPoints;

				if (hashMap.find(h) == hashMap.end()) { //se non esiste quell'hash nella hasmap

					DataPoint point = DataPoint(songId, t);
					listPoints.push_back(point);
					hashMap.insert(std::pair<long long, std::list<DataPoint>>(h, listPoints));
				}
				else { //se esiste quell'hash inserisce nella lista corrispondente il DataPoint

					DataPoint point = DataPoint(songId, t);
					hashMap.find(h)->second.push_back(point);
				}
			}			
		}	
	}

	void writePeaksOnDisk(int& t, String& filename, int& pt1, int& pt2, int& pt3, int& pt4, long long& h){
		std::stringstream sstm;
		sstm << "audio-ad-insertion-data\\peaks-songId" << filename << ".txt";
		File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(sstm.str());
		f.create();
		if (t == 0) {
			//clear file only in the first timeframe
			f.replaceWithText("");
		}

		std::ostringstream oss;
		oss << "Song: " << filename << " pt1: " << pt1 << " pt2: " << pt2 << " pt3: " << pt3 << " pt4: " << pt4 << " h: " << h << std::endl;
		std::string var = oss.str();
		f.appendText(oss.str());
	}

	void pushSampleIntoSongMatchFifo(const juce::AudioBuffer<float>& tmpBuffer, const int bufferLength) {
		int totSamples = (int) m_sampleRate * 10; // up to 10 seconds of samples

		int samplesToCopy = bufferLength;

		if (totSamples - songMatchFifoIndex < bufferLength)
			samplesToCopy = (totSamples - songMatchFifoIndex);

		const float* bufferData = tmpBuffer.getReadPointer(0);
		songMatchFifo.copyFrom(0, songMatchFifoIndex, bufferData, samplesToCopy);

		if (totSamples - songMatchFifoIndex < bufferLength) { //allora il songMatchFifo buffer � pieno e pu� essere effettuato il fingerprint
			//Soluzione esterna: affida il match ad audfprint
			/*writeAudioFileOnDisk(songMatchFifo);

			std::thread t([]() {				
				//ShellExecute(0, "open", "cmd.exe", "/C ipconfig > out.txt", 0, SW_HIDE);
				//set your own paths
				std::system("cd C:\\Users\\Throw\\Documents\\JuceProjects\\FingerprintRepo\\audfprint-master && python audfprint.py match --dbase fpdbase.pklz C:\\Users\\Throw\\Documents\\audio-ad-insertion-data\\audioSampleToMatch\\sampleToMatch.wav >> myoutput.txt");
			});
			t.detach();*/

			//Soluzione interna: affida il match a generateHashes()
			//generateHashes(0, false);			
			generateHashes(100, true);
			int bestSong = bestMatch();

			songMatchFifo.clear();
			songMatchFifoIndex = 0;
		}
			songMatchFifoIndex += bufferLength;
	}

	int bestMatch() {

		std::list<DataPoint> listPoints;
		int bestCount = 0;
		int bestSong = -1;

		File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\debugMatch.txt");
		if (f.exists())
			f.deleteFile();
		f.create();

		for (int id = 0; id < nrSongs; id++) {

			std::unordered_map<int, int> tmpMap;
			tmpMap = matchMap.find(id)->second;
			int bestCountForSong = 0;

			auto it = tmpMap.begin();			
			while (it != tmpMap.end()) {
				if (it->second > bestCountForSong) {
					bestCountForSong = it->second;					
				}
				it++;
			}			

			std::ostringstream oss;
			oss << "Song: " << id << " bestCount: " << bestCountForSong << std::endl;
			std::string var = oss.str();
			f.appendText(oss.str());

			if (bestCountForSong > bestCount) {
				bestCount = bestCountForSong;
				bestSong = id;
			}			
		}
		return bestSong;
	}
	

	void writeAudioFileOnDisk(const juce::AudioBuffer<float>& tmpBuffer) {

		WavAudioFormat format;
		File file = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioSampleToMatch\\sampleToMatch.wav");

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
	//quindi goertzel settato sul riconoscimento di 16HZ, riconoscer� nell'intorno di +/- 5.85

	const float  PI2 = M_PI * 2;
	const int dtmf_fq[dtmfsSize] = {16, 697, 770, 852, 941, 1209, 1336, 1477, 1633};
	float dtmf_levels[dtmfsSize] = {};

	std::unordered_map<long long, std::list<DataPoint>> hashMap;
	std::unordered_map<int, std::unordered_map<int, int>> matchMap; // Map<SongId, Map<Offset, Count>>

	//necessario per mantenere uno stato
	juce::AudioBuffer<float> songMatchFifo;
	int songMatchFifoIndex = 0;
	int nrSongs = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFT)
};

