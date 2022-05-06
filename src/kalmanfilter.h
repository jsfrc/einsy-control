#pragma once
#include <math.h>


class KalmanFilter {
public:
	KalmanFilter();
	KalmanFilter(float mea_e, float est_e, float q);
	float updateEstimate(float mea);
	void setMeasurementError(float mea_e);
	void setEstimateError(float est_e);
	void setProcessNoise(float q);
	float getKalmanGain();
	float getEstimateError();
private:
	float _err_measure;
	float _err_estimate;
	float _q;
	float _current_estimate = 0;
	float _last_estimate = 0;
	float _kalman_gain = 0;
};
