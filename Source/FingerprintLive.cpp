/*
  ==============================================================================

	FingerprintLive.cpp
	Created: 5 Sep 2020 4:26:50pm
	Author:  Throw

  ==============================================================================
*/

#include "FingerprintLive.h"

//==============================================================================
FingerprintLive::FingerprintLive() : forwardFFT(fftOrder),
window(fftSize, juce::dsp::WindowingFunction<float>::hann), m_sampleRate(44100.0), secondsToAnalyze(10)
{
	// In your constructor, you should add any child components, and
	// initialise any special settings that your component needs.
	formatManager.registerBasicFormats();
}

FingerprintLive::~FingerprintLive()
{
}

void FingerprintLive::matchHashes(int currentTime) {

	std::array<float, 2 * fftSize> outBuffer;

	memcpy(outBuffer.data(), fifo.data(), fftSize * sizeof(float));
	//writeWaveFormOnDisk(t, 987, fftSize, outBuffer.data());

	window.multiplyWithWindowingTable(outBuffer.data(), fftSize);
	forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation

	float magf1 = 0, magf2 = 0, magf3 = 0, magf4 = 0;
	int pt1 = 0, pt2 = 0, pt3 = 0, pt4 = 0;

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
	//writePeaksOnDisk(t, "stream", pt1, pt2, pt3, pt4, h);		

	if (h != 0) {//per evitare di matchare hash di silenzio (=0)
		std::list<DataPoint> listPoints;

		if (hashMap.find(h) != hashMap.end()) { //se ha trovato un hash esistente già in memoria uguale a quello attuale
			listPoints = hashMap.find(h)->second;

			for (DataPoint dP : listPoints) {

				//removed std::abs();
				int offset = dP.getTime() - currentTime;

				if ((matchMap.find(dP.getSongId())) == matchMap.end()) { //se è il primo match di hash di una canzone nuova
					std::unordered_map<int, int> tmpMap;
					tmpMap.insert(std::pair<int, int>(offset, 1));

					matchMap.insert(std::pair<int, std::unordered_map<int, int>>(dP.getSongId(), tmpMap));

				}
				else { //se esistenvano già dei match per quella canzone

					std::unordered_map<int, int> tmpMap;
					tmpMap = matchMap.find(dP.getSongId())->second;

					if ((tmpMap.find(offset)) == tmpMap.end()) {
						matchMap.find(dP.getSongId())->second.insert(std::pair<int, int>(offset, 1));
					}
					else {
						int count = tmpMap.find(offset)->second;
						matchMap.find(dP.getSongId())->second.find(offset)->second = count + 1;
					}
				}
			}
		}
	}
	
}

void FingerprintLive::loadHashes(int songId, bool isMatching, juce::String input_file = "") {

	juce::File file(input_file);
	juce::String filename = file.getFileName();

	//increment totNumber of Songs
	nrSongs++;

	auto* reader = formatManager.createReaderFor(file);

	if (reader != nullptr) {
		std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
		transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
		readerSource.reset(newSource.release());
	}

	transportSource.start();
	//leggi tutto il file
	int samples = transportSource.getTotalLength();
	jingleMap.insert(std::pair<int, std::pair<int, std::string>>(songId, std::pair<int, std::string>(samples, filename.toStdString())));

	//int samples = juce::jmin((int)(m_sampleRate * secondsToAnalyze), (int)reader->lengthInSamples);	
	std::array<float, 2 * fftSize> outBuffer;

	AudioBuffer<float> tmp;
	tmp.setSize(1, fftSize);
	//t = timeframe counter

	for (int t = 0; t < samples / fftSize; t++) {
		transportSource.getNextAudioBlock(AudioSourceChannelInfo(tmp));
		memcpy(outBuffer.data(), tmp.getReadPointer(0), fftSize * sizeof(float));
		//writeWaveFormOnDisk(t, songId, fftSize, outBuffer.data());

		window.multiplyWithWindowingTable(outBuffer.data(), fftSize);
		forwardFFT.performFrequencyOnlyForwardTransform(outBuffer.data()); //FFT computation

		float magf1 = 0, magf2 = 0, magf3 = 0, magf4 = 0;
		int pt1 = 0, pt2 = 0, pt3 = 0, pt4 = 0;


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
		//writePeaksOnDisk(t, filename, pt1, pt2, pt3, pt4, h);

		//per evitare di salvare hash di silenzio (=0)
		if (h != 0) {
			//sta caricando in memoria gli hash di tutti i jingle
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
	transportSource.stop();
}

void FingerprintLive::resampleAudioBuffer(AudioBuffer<float>& buffer, unsigned int& numChannels, int64& samples, double& sampleRateIn, double& sampleRateOut) {
	if (sampleRateIn != sampleRateOut) {
		//resample AudioSampleBuffer with lagrange interpolator		
		juce::AudioBuffer<float> resampledBuffer;
		double ratio = sampleRateIn / sampleRateOut;
		resampledBuffer.setSize((int)numChannels, (int)(((double)samples) / ratio));
		LagrangeInterpolator resampler;
		resampler.reset();
		resampler.process(ratio, buffer.getReadPointer(0), resampledBuffer.getWritePointer(0), resampledBuffer.getNumSamples());
		buffer = std::move(resampledBuffer);
	}
}

void FingerprintLive::writeWaveFormOnDisk(int t, int idSong, int currentSize, float* outBuffer) {
	//write waveform on disk
	std::stringstream sstm;
	sstm << "audio-ad-insertion-data\\waveform" << idSong << ".txt";
	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(sstm.str());
	f.create();
	if (t == 0) {
		//clear file only in the first timeframe
		f.replaceWithText("");
	}

	std::ostringstream oss;
	for (int count = 0; count < currentSize; count++) {
		oss << outBuffer[count] << std::endl;
	}
	std::string var = oss.str();
	f.appendText(oss.str());
}

void FingerprintLive::writePeaksOnDisk(int& t, String& filename, int& pt1, int& pt2, int& pt3, int& pt4, long long& h) {
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

/*std::pair<int, std::string> FingerprintLive::pushSampleIntoSongMatchFifoOverlap(const float& sample) {
	int samplesRemaining = -1;
	std::string jingleName = "";

	if (fifoIndex == fftSize) {
		
		matchHashes(windowAnalysisIndex);
		memcpy(fifo.data(), &fifo[fftSize / 2], sizeof(float) * fftSize / 2);
		fifoIndex = fftSize/2;

		increment++;
		windowAnalysisIndex += increment % 2;

		if (windowAnalysisIndex == frameAnalysisAccumulator && !lastOverlap) {
			lastOverlap = true;
		}
		else if (windowAnalysisIndex == frameAnalysisAccumulator && lastOverlap) {
			windowAnalysisIndex = 0;
			increment = 0;
			std::pair<int, int> tuple = calculateBestMatch();
			if (tuple.first > 0) {
				samplesRemaining = jingleMap.find(tuple.second)->second.first - tuple.first;
				jingleName = jingleMap.find(tuple.second)->second.second;
			}
			lastOverlap = false;
			matchMap.clear();
		}
	}
	fifo[fifoIndex] = sample;
	++fifoIndex;
	return std::pair<int, std::string>(samplesRemaining, jingleName);
}*/

/*std::pair<int, std::string> FingerprintLive::pushSampleIntoSongMatchFifo(const float& sample) {
	int samplesRemaining = -1;
	std::string jingleName = "";
	if (fifoIndex == fftSize) {

		matchHashes(windowAnalysisIndex);
		fifoIndex = 0;
		windowAnalysisIndex++;

		if (windowAnalysisIndex == frameAnalysisAccumulator) {
			windowAnalysisIndex = 0;

			std::pair<int, int> tuple = calculateBestMatch();
			if (tuple.first > 0) {
				samplesRemaining = jingleMap.find(tuple.second)->second.first - tuple.first;
				jingleName = jingleMap.find(tuple.second)->second.second;
			}
			matchMap.clear();
		}
	}
	fifo[fifoIndex] = sample;
	++fifoIndex;
	return std::pair<int, std::string>(samplesRemaining, jingleName);
}*/

RecognizedJingle FingerprintLive::getRecognitionWithOverlap(const float& sample) {
	RecognizedJingle rj;

	if (fifoIndex == fftSize) {

		matchHashes(windowAnalysisIndex);
		memcpy(fifo.data(), &fifo[fftSize / 2], sizeof(float) * fftSize / 2);
		fifoIndex = fftSize / 2;

		increment++;
		windowAnalysisIndex += increment % 2;

		if (windowAnalysisIndex == frameAnalysisAccumulator && !lastOverlap) {
			lastOverlap = true;
		}
		else if (windowAnalysisIndex == frameAnalysisAccumulator && lastOverlap) {
			windowAnalysisIndex = 0;
			increment = 0;
			rj = calculateBestMatch();
			
			/*std::pair<int, int> tuple = calculateBestMatch();
			if (tuple.first > 0) {
				samplesOffset = tuple.first;
				//jingleName = jingleMap.find(tuple.second)->second.second;
				totalDuration = jingleMap.find(tuple.second)->second.first;
			}*/
			lastOverlap = false;
			matchMap.clear();
		}
	}
	fifo[fifoIndex] = sample;
	++fifoIndex;
	return rj;
}



//vitale bestOffset > 0
RecognizedJingle FingerprintLive::calculateBestMatch() {

	int bestCount = 0;
	int bestSong = -1;
	int bestOffset = -1;

	std::ostringstream oss;
	//juce::Time::getCurrentTime().formatted("%Y.%m.%d_%H.%M.%S") //user format
	oss << "audio-ad-insertion-data\\logFrameAnalysisWindow\\" << juce::Time::getCurrentTime().toString(true, false) << ".txt";

	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(oss.str());
	/*if (f.exists()) {
		f.deleteFile();
	}*/
	auto result = f.create();

	int bestCountForSong = 0;
	int bestOffsetForSong = 0;

	for (int id = 0; id < jingleMap.size(); id++) {

		auto tmpMapIt = matchMap.find(id);
		if (tmpMapIt != matchMap.end()) {//se ha trovato dei match per quell'id di jingle
			std::unordered_map<int, int> tmpMap;
			tmpMap = tmpMapIt->second;

			auto it = tmpMap.begin();
			while (it != tmpMap.end()) {
				if (it->second > bestCountForSong) {
					bestCountForSong = it->second;
					bestOffsetForSong = it->first;
				}
				it++;
			}
		}

		oss.str(std::string());
		oss << "Jingle: " << id << " bestCount: " << bestCountForSong << " bestOffset: " << bestOffsetForSong << std::endl;
		std::string var = oss.str();
		f.appendText(oss.str());
		
		if (bestCountForSong > bestCount) {
			bestCount = bestCountForSong;
			bestSong = id;
			bestOffset = bestOffsetForSong;
		}
		bestCountForSong = 0;
		bestOffsetForSong = 0;
	}
	
	int duration = -1;
	int nSamplesOffset = -1;
	int remaining = -1;	
	std::string jingleTitle = "";

	offset1 = offset2;
	//offset2 = offset3;
	offset2 = bestOffset;

	oss.str(std::string());
	oss << std::endl;
	std::string var = oss.str();
	f.appendText(oss.str());

	if (bestCount > thresholdMatchCons /*&& bestOffset > 0 && ((offset2 - offset1) == frameAnalysisAccumulator * 1)*/) {
		
		auto it = jingleMap.find(bestSong);
		if (it != jingleMap.end()) {
			duration = it->second.first;
			nSamplesOffset = (frameAnalysisAccumulator * fftSize) + (bestOffset * fftSize);
			remaining = duration - nSamplesOffset;
			jingleTitle = it->second.second;
		}
		oss.str(std::string());
		oss << "bestJingle: " << bestSong << " Jingle name: " << jingleMap.find(bestSong)->second.second
			<< " duration: " << jingleMap.find(bestSong)->second.first << " bestOffset:"<< bestOffset << std::endl << std::endl;
		std::string var = oss.str();
		f.appendText(oss.str());
	}
	RecognizedJingle rj(duration, nSamplesOffset, remaining, bestSong, jingleTitle);
	return rj;
}

void FingerprintLive::writeAudioFileOnDisk(const juce::AudioBuffer<float>& tmpBuffer) {

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

void FingerprintLive::setupFingerprintLive(double samplerate, double secToAnalyze) {
	m_sampleRate = samplerate;
	secondsToAnalyze = secToAnalyze;
	transportSource.prepareToPlay(fftSize, m_sampleRate);
	//songMatchFifo.setSize(1, (int)(m_sampleRate * secToAnalyze));
	//songMatchFifo.setSize(1, (int)(512));

}

long long FingerprintLive::hash(long long p1, long long p2, long long p3, long long p4) {
	return (p4 - (p4 % FUZ_FACTOR)) * 100000000 + (p3 - (p3 % FUZ_FACTOR))
		* 100000 + (p2 - (p2 % FUZ_FACTOR)) * 100
		+ (p1 - (p1 % FUZ_FACTOR));
}

std::unordered_map<long long, std::list<DataPoint>>& FingerprintLive::getHashMap() {
	return hashMap;
}

std::unordered_map<int, std::pair<int, std::string>>& FingerprintLive::getJingleMap() {
	return jingleMap;
}
