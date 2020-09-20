/*
  ==============================================================================

	FingerprintLive.h
	Created: 5 Sep 2020 4:26:50pm
	Author:  Throw

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <unordered_map> 
#include "DataPoint.h"
#include <iostream>
#include <fstream>
#include <thread>
#include "RecognizedJingle.h"
//==============================================================================
/*
*/
class FingerprintLive
{
public:
	enum {
		fftOrder = 9,
		fftSize = 1 << fftOrder,
		//più aumenti frameAnalysisAccumulator e più match spuri ci saranno, quindi di conseguenza la threshold deve essere più alta
		thresholdMatchCons = 4,
		frameAnalysisAccumulator = 128,
		matchMapFrame = 2,
	};

	FingerprintLive();

	~FingerprintLive();

	void matchHashes(int currentTime, int endMatchMap, int endMatchMap1);

	void loadHashes(int songId, bool isMatching, juce::String input_file);

	//std::pair<int, std::string> pushSampleIntoSongMatchFifoOverlap(const float & sample);

	//std::pair<int, std::string> pushSampleIntoSongMatchFifo(const float & sample);

	RecognizedJingle getRecognitionWithOverlap(const float & sample);	

	RecognizedJingle getRecognitionWithMatchMapOverlap(const float & sample);

	void setupFingerprintLive(double samplerate, double secToAnalyze);

	std::unordered_map<long long, std::list<DataPoint>>& getHashMap();

	std::unordered_map<int, std::pair<int, std::string>>& getJingleMap();

private:
	void resampleAudioBuffer(AudioBuffer<float>& buffer, unsigned int & numChannels, int64 & samples, double & sampleRateIn, double & sampleRateOut);

	void writeWaveFormOnDisk(int t, int idSong, int currentSize, float * outBuffer);

	void writePeaksOnDisk(int & t, String & filename, int & pt1, int & pt2, int & pt3, int & pt4, long long & h);
	
	RecognizedJingle calculateBestMatch(std::unordered_map<int, std::unordered_map<int, int>>& matchMapReceived);

	void writeAudioFileOnDisk(const juce::AudioBuffer<float>& tmpBuffer);

	long long hash(long long p1, long long p2, long long p3, long long p4);

	const int FUZ_FACTOR = 2;

	juce::dsp::FFT forwardFFT;
	juce::dsp::WindowingFunction<float> window;
	double m_sampleRate;
	double secondsToAnalyze;
	std::unordered_map<long long, std::list<DataPoint>> hashMap;	// Map<Hash, List<DataPoint>>
	std::unordered_map<int, std::unordered_map<int, int>> matchMap; // Map<SongId, Map<Offset, Count>>
	std::unordered_map<int, std::unordered_map<int, int>> matchMap2; // Map<SongId, Map<Offset, Count>>

	//analysis bands
	int freqbandWidth = 40;
	int freqbandWidth2 = 80;
	int freqbandWidth3 = 120;
	int freqbandWidth4 = 180;
	int freqbandWidth5 = 256;

	//needed to read files
	juce::AudioFormatManager formatManager;
	std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
	juce::AudioTransportSource transportSource;

	//needed to maintain a buffer state
	std::array<float, fftSize> fifo;
	int fifoIndex = 0;
	int increment = 0;
	bool lastOverlap = false;
	int nrSongs = 0;

	int windowAnalysisIndex = 0;
	std::unordered_map<int, std::pair<int, std::string>> jingleMap;	// Map<songId, (lengthInSamples, jingle name)>
	
	int offset1 = 0;
	int offset2 = 0;
	int offset3 = 0;


	int circularCounter = 0;
	bool firstFillMap = true;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FingerprintLive)
};
