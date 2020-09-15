/*
  ==============================================================================

    RecognizedJingle.h
    Created: 15 Sep 2020 9:41:07pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
#include <string>
//==============================================================================
/*
*/
class RecognizedJingle 
{
public:
    RecognizedJingle();

	RecognizedJingle(int duration, int offset, int remaining, int id, std::string title);

	RecognizedJingle(const RecognizedJingle& source);

    ~RecognizedJingle();

	RecognizedJingle& operator=(const RecognizedJingle& source);

	void setDurationInSamples(int duration);

	void setOffsetInSamples(int offset);

	void setRemainingInSamples(int remaining);

	void setId(int id);

	void setTitle(std::string title);

	int getDurationInSamples() const;

	int getOffsetInSamples() const;

	int getRemainingInSamples() const;

	int getId() const;

	std::string getTitle() const;

	bool isEmpty();


private:
	int durationInSamples;
	int offsetInSamples;
	int remainingInSamples;
	int id;
	std::string title;

	//Questo non consente l'overload degli operatori, quindi va eliminato in questo file
    //JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RecognizedJingle)
};
