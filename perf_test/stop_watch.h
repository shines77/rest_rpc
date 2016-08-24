#pragma once

#include <chrono>

#ifndef COMPILER_BARRIER
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define COMPILER_BARRIER()		_ReadWriteBarrier()
#else
#define COMPILER_BARRIER()		asm volatile ("" : : : "memory")
#endif
#endif

using namespace std::chrono;

class StopWatch {
public:
    typedef std::chrono::time_point<high_resolution_clock> time_clock;
    typedef std::chrono::duration<double> time_elapsed;

private:
    time_clock start_time_;
    time_clock stop_time_;
    time_elapsed interval_time_;
	double total_elapsed_time_;

public:
    StopWatch() : interval_time_{0}, total_elapsed_time_(0.0) {};
    ~StopWatch() {};

	void reset() {
		total_elapsed_time_ = 0.0;
        start_time_ = std::chrono::high_resolution_clock::now();
        interval_time_ = std::chrono::duration_cast<time_elapsed>(start_time_ - start_time_);
	}

    void restart() {
        reset();
        start();
    }

    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
		COMPILER_BARRIER();
    }

    void stop() {
		COMPILER_BARRIER();
        stop_time_ = std::chrono::high_resolution_clock::now();
    }

    void mark_start() {
        start();
    }

    void mark_stop() {
        stop();
    }

	void again() {
		double elapsed_time = getIntervalTime();
		total_elapsed_time_ += elapsed_time;
	}

    double now() {
        time_elapsed now_ = std::chrono::duration_cast< time_elapsed >(std::chrono::high_resolution_clock::now() - start_time_);
        return now_.count();
    }
   
    double getIntervalTime() {
        interval_time_ = std::chrono::duration_cast< time_elapsed >(stop_time_ - start_time_);
        return interval_time_.count();
    }

    double getSecond() {
        return getIntervalTime();
    }

    double getMillisec() {
        return getIntervalTime() * 1000.0;
    }

    double getElapsedTime() {
        stop();
        return getIntervalTime();
    }

    double getElapsedSecond() {
        return getElapsedTime();
    }

    double getElapsedMillisec() {
        return getElapsedTime() * 1000.0;
    }

    double getTotalSecond() const {
        return total_elapsed_time_;
    }

    double getTotalMillisec() const {
        return getTotalSecond() * 1000.0;
    }
};

#undef COMPILER_BARRIRER
