/*
  ==============================================================================

    Fingerprint.cpp
    Created: 5 Sep 2020 4:26:50pm
    Author:  Throw

  ==============================================================================
*/

#include "Fingerprint.h"

//==============================================================================
Fingerprint::Fingerprint() : forwardFFT(fftOrder),
	window(fftSize, juce::dsp::WindowingFunction<float>::hann), m_sampleRate(44100.0), secondsToAnalyze(10)
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
	formatManager.registerBasicFormats();
}

Fingerprint::~Fingerprint()
{
}

void Fingerprint::matchHashes() {

	std::array<float, 2 * fftSize> outBuffer;
	int mread = 0;

	//t = timeframe counter
	int windowIndex = 0;
	for (int t = 0; t < (songMatchFifoCopy.getNumSamples() / fftSizeIncrement) - 1; t++) {
		
		memcpy(outBuffer.data(), songMatchFifoCopy.getReadPointer(0, mread), fftSize * sizeof(float));
		//writeWaveFormOnDisk(t, 987, fftSize, outBuffer.data());

		mread += fftSizeIncrement;		

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

					int offset = std::abs(dP.getTime() - windowIndex);

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
#ifdef Overlap
		if (t % 2 == 1) {
			windowIndex++;
		}
#else
		windowIndex++;
#endif // 

		
	}
}

void Fingerprint::loadHashes(int songId, bool isMatching, juce::String input_file = "") {

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

void Fingerprint::resampleAudioBuffer(AudioBuffer<float>& buffer, unsigned int& numChannels, int64& samples, double& sampleRateIn, double& sampleRateOut) {
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

void Fingerprint::writeWaveFormOnDisk(int t, int idSong, int currentSize, float* outBuffer) {
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

void Fingerprint::writePeaksOnDisk(int& t, String& filename, int& pt1, int& pt2, int& pt3, int& pt4, long long& h) {
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

void Fingerprint::pushSampleIntoSongMatchFifo(const juce::AudioBuffer<float>& tmpBuffer, const int bufferLength) {
	int totSamples = (int)(m_sampleRate * secondsToAnalyze); // up to secondsToAnalyze of samples
	/*int samplesToCopy = bufferLength;
	if (totSamples - songMatchFifoIndex < bufferLength)
		samplesToCopy = (totSamples - songMatchFifoIndex);*/
	int samplesToCopy = juce::jmin(totSamples - songMatchFifoIndex, bufferLength);

	const float* bufferData = tmpBuffer.getReadPointer(0);
	songMatchFifo.copyFrom(0, songMatchFifoIndex, bufferData, samplesToCopy);

	if (totSamples - songMatchFifoIndex < bufferLength) { //allora il songMatchFifo buffer è pieno e può essere effettuato il fingerprint
		
		//Soluzione interna: affida il match a generateHashes()
		songMatchFifoCopy = songMatchFifo;
		std::thread t([this]() {
			matchHashes();
			int bestSong = calculateBestMatch();

		});
		t.detach();

		songMatchFifo.clear();
		songMatchFifoIndex = 0;
	}
	songMatchFifoIndex += bufferLength;
}


int Fingerprint::calculateBestMatch() {

	std::list<DataPoint> listPoints;
	int bestCount = 0;
	int bestSong = -1;

	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\logLevel0BufferingFrame\\debugMatch.txt");
	if (f.exists()) {
		f.deleteFile();
		//f.replaceWithText("");//clear
	}
	f.create();

	for (int id = 0; id < jingleMap.size(); id++) {

		int bestCountForSong = 0;

		if (matchMap.find(id) != matchMap.end()) {//se ha trovato dei match per quell'id di jingle
			std::unordered_map<int, int> tmpMap;
			tmpMap = matchMap.find(id)->second;


			auto it = tmpMap.begin();
			while (it != tmpMap.end()) {
				if (it->second > bestCountForSong) {
					bestCountForSong = it->second;
				}
				it++;
			}
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

void Fingerprint::writeAudioFileOnDisk(const juce::AudioBuffer<float>& tmpBuffer) {

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

void Fingerprint::setupFingerprint(double samplerate, double secToAnalyze) {
	m_sampleRate = samplerate;	
	secondsToAnalyze = secToAnalyze;
	transportSource.prepareToPlay(fftSize, m_sampleRate);
	songMatchFifo.setSize(1, (int)(m_sampleRate * secToAnalyze));
}

long long Fingerprint::hash(long long p1, long long p2, long long p3, long long p4) {
	return (p4 - (p4 % FUZ_FACTOR)) * 100000000 + (p3 - (p3 % FUZ_FACTOR))
		* 100000 + (p2 - (p2 % FUZ_FACTOR)) * 100
		+ (p1 - (p1 % FUZ_FACTOR));
}

std::unordered_map<long long, std::list<DataPoint>>& Fingerprint::getHashMap() {
	return hashMap;
}

std::unordered_map<int, std::pair<int, std::string>>& Fingerprint::getJingleMap() {
	return jingleMap;
}

//generateHashes() old, not used
/*void generateHash(int songId, bool isMatching, juce::String input_file = "") {

	juce::AudioBuffer<float> fileBuffer;
	int samples;
	File file(input_file);
	juce::String filename = file.getFileName();
	juce::AudioFormatManager formatManager;
	formatManager.registerBasicFormats();
	if (!isMatching) {
		//increment totNumber of Songs
		nrSongs++;

		std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));

		if (reader.get() != nullptr) {
			fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
			reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
			resampleAudioBuffer(fileBuffer, reader->numChannels, reader->lengthInSamples, reader->sampleRate, m_sampleRate);
		}
		//if (reader->sampleRate != m_sampleRate) {
			//resample AudioSampleBuffer
		//	juce::AudioBuffer<float> resampledBuffer;
		//	double ratio = reader->sampleRate / m_sampleRate;s
		//	fileBuffer.setSize((int)reader->numChannels, (int)(((double)reader->lengthInSamples) / ratio));
		//	LagrangeInterpolator resampler;
		//	resampler.reset();
		//	resampler.process(ratio, fileBuffer.getReadPointer(0), resampledBuffer.getWritePointer(0), resampledBuffer.getNumSamples());
		//	fileBuffer = std::move(resampledBuffer);
		//}

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

		//write waveform on disk
		std::stringstream sstm;
		sstm << "audio-ad-insertion-data\\waveform" << songId << ".txt";
		File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(sstm.str());
		f.create();
		if (t == 0) {
			//clear file only in the first timeframe
			f.replaceWithText("");
		}

		std::ostringstream oss;
		for (int count = 0; count < fftSize; count++) {
			oss << outBuffer[count] << std::endl;
		}
		std::string var = oss.str();
		f.appendText(oss.str());

		mread += fftSize;

		window.multiplyWithWindowingTable(outBuffer.data(), fftSize);
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
		//writePeaksOnDisk(t, filename, pt1, pt2, pt3, pt4, h);

		if (isMatching) {

			std::list<DataPoint> listPoints;

			if (hashMap.find(h) != hashMap.end()) { //se ha trovato un hash esistente già in memoria uguale a quello attuale
				listPoints = hashMap.find(h)->second;

				for (DataPoint dP : listPoints) {

					int offset = std::abs(dP.getTime() - t);


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
}*/

//Soluzione esterna: affida il match ad audfprint
/*writeAudioFileOnDisk(songMatchFifo);

std::thread t([]() {
	//ShellExecute(0, "open", "cmd.exe", "/C ipconfig > out.txt", 0, SW_HIDE);
	//set your own paths
	std::system("cd C:\\Users\\Throw\\Documents\\JuceProjects\\FingerprintRepo\\audfprint-master && python audfprint.py match --dbase fpdbase.pklz C:\\Users\\Throw\\Documents\\audio-ad-insertion-data\\audioSampleToMatch\\sampleToMatch.wav >> myoutput.txt");
});
t.detach();*/