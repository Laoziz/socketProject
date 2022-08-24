#ifndef _CELLTimestamp_hpp_
#define _CELLTimestamp_hpp_

#include<chrono>
using namespace std;
using namespace std::chrono;

class CELLTimestamp {
public:
	CELLTimestamp() {
		update();
	}
	~CELLTimestamp() {

	}

	void update() {
		_begin = high_resolution_clock::now();
	}
	/** 
	* ��ȡ��ǰ��
	*/
	double getElapsedSecond() {
		return getElapsedTimeInMicroSec() * 0.000001;
	}
	/**
	* ��ȡ��ǰ����
	*/
	double getElapsedTimeInMilliSec() {
		return this->getElapsedTimeInMicroSec() * 0.001;
	}
	/**
	* ��ȡ��ǰ΢��
	*/
	long long getElapsedTimeInMicroSec() {
		/*
        LARGE_INTEGER endCount;
        QueryPerformanceCounter(&endCount);

        double  startTimeInMicroSec =   _startCount.QuadPart * (1000000.0 / _frequency.QuadPart);
        double  endTimeInMicroSec   =   endCount.QuadPart * (1000000.0 / _frequency.QuadPart);

        return  endTimeInMicroSec - startTimeInMicroSec;
		*/
		return duration_cast<microseconds>(high_resolution_clock::now() - _begin).count();
	}
protected:
	time_point<high_resolution_clock> _begin;
};
#endif // !_CELLTimestamp_hpp_