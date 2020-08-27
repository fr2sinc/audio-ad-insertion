/*
  ==============================================================================

    DataPoint.h
    Created: 20 Aug 2020 12:54:43pm
    Author:  Throw

  ==============================================================================
*/

#pragma once
class DataPoint {

public: DataPoint(int songId, int time) {
		this->songId = songId;
		this->time = time;
	}

	int getTime() {
		return time;
	}

	int getSongId() {
		return songId;
	}

private:
	int time;
	int songId;

};
