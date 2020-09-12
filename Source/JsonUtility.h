/*
  ==============================================================================

    JsonUtility.h
    Created: 12 Sep 2020 11:10:06am
    Author:  Throw

  ==============================================================================
*/

#pragma once

#include "json.hpp"
#include "DataPoint.h"
#include <JuceHeader.h>

using nlohmann::json;

//==============================================================================
/*
*/
class JsonUtility 
{
public:
    JsonUtility();
    ~JsonUtility();

	static void writeHashMap(std::unordered_map<long long, std::list<DataPoint>>& hashMap);

	static void readHashMap(std::unordered_map<long long, std::list<DataPoint>>& hashMap);

	static void writeJingleMap(std::unordered_map<int, std::pair<int, std::string>>& jingleMap);

	static void readJingleMap(std::unordered_map<int, std::pair<int, std::string>>& jingleMap);


private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JsonUtility)
};
