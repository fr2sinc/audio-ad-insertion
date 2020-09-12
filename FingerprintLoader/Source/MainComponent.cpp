#include "MainComponent.h"

//==============================================================================
MainComponent::MainComponent()
{
	addAndMakeVisible(&loadButton);
	loadButton.setButtonText("Load Database of Jingles");
	loadButton.onClick = [this] { loadButtonClicked(); };   // [8]

	addAndMakeVisible(sampleRateMenu);
	sampleRateMenu.addItem("44100 Hz", 1);
	sampleRateMenu.addItem("48000 Hz", 2);
	sampleRateMenu.addItem("96000 Hz", 3);

	sampleRateMenu.onChange = [this] { sampleRateMenuChanged(); };
	sampleRateMenu.setSelectedId(2);

	setSize(600, 300);
}

MainComponent::~MainComponent()
{
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    /*g.setFont (juce::Font (16.0f));
    g.setColour (juce::Colours::white);
    g.drawText ("Hello World!", getLocalBounds(), juce::Justification::centred, true);*/
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
	loadButton.setBounds(10, 10, getWidth() - 20, 40);
	sampleRateMenu.setBounds(10, 80, getWidth() - 20, 40);
}

void MainComponent::loadButtonClicked() {
	FingerprintLive fprintLive;
	//first of all setup fingerprint
	//set your host samplerate manually
	fprintLive.setupFingerprintLive(sampleRate, 0.5);

	File f = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile("audio-ad-insertion-data\\audioDatabase");
	DirectoryIterator dir_iterator(f, false);

	int songId = 0;
	while (dir_iterator.next()) {

		auto file = File(dir_iterator.getFile());
		fprintLive.loadHashes(songId, false, file.getFullPathName());
		songId++;
	}

	JsonUtility::writeHashMap(fprintLive.getHashMap());
	JsonUtility::writeJingleMap(fprintLive.getJingleMap());

	//clear data structures in fprintLive
}

void MainComponent::sampleRateMenuChanged () {

	switch (sampleRateMenu.getSelectedId())
		{
			case 1: sampleRate = 44100.0;	break;
			case 2: sampleRate = 48000.0;   break;
			case 3: sampleRate = 96000.0;	break;
			default: break;
		}

}
