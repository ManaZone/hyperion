// Qt includes
#include <QDateTime>

#include "LinearColorSmoothing.h"

LinearColorSmoothing::LinearColorSmoothing(LedDevice *ledDevice, double ledUpdateFrequency, double settlingTime) :
	QObject(),
	LedDevice(),
	_ledDevice(ledDevice),
	_updateInterval(1000.0 / ledUpdateFrequency),
	_settlingTime(1000 * settlingTime),
	_timer()
{
	_timer.setSingleShot(false);
	_timer.setInterval(_updateInterval);

	connect(&_timer, SIGNAL(timeout()), this, SLOT(updateLeds()));
}

LinearColorSmoothing::~LinearColorSmoothing()
{
	delete _ledDevice;
}

int LinearColorSmoothing::write(const std::vector<RgbColor> &ledValues)
{
	// received a new target color
	if (_previousValues.size() == 0)
	{
		// not initialized yet
		_targetTime = QDateTime::currentMSecsSinceEpoch() + _settlingTime;
		_targetValues = ledValues;

		_previousTime = QDateTime::currentMSecsSinceEpoch();
		_previousValues = ledValues;
		_timer.start();
	}
	else
	{
		_targetTime = QDateTime::currentMSecsSinceEpoch() + _settlingTime;
		memcpy(_targetValues.data(), ledValues.data(), ledValues.size() * sizeof(RgbColor));
	}

	return 0;
}

int LinearColorSmoothing::switchOff()
{
	// stop smoothing filter
	_timer.stop();

	// return to uninitialized state
	_previousValues.clear();
	_previousTime = 0;
	_targetValues.clear();
	_targetTime = 0;

	// finally switch off all leds
	return _ledDevice->switchOff();
}

void LinearColorSmoothing::updateLeds()
{
	int64_t now = QDateTime::currentMSecsSinceEpoch();
	int deltaTime = _targetTime - now;

	if (deltaTime < 0)
	{
		memcpy(_previousValues.data(), _targetValues.data(), _targetValues.size() * sizeof(RgbColor));
		_previousTime = now;

		_ledDevice->write(_previousValues);
	}
	else
	{
		float k = 1.0f - 1.0f * deltaTime / (_targetTime - _previousTime);

		for (size_t i = 0; i < _previousValues.size(); ++i)
		{
			RgbColor & prev = _previousValues[i];
			RgbColor & target = _targetValues[i];

			prev.red   += k * (target.red   - prev.red);
			prev.green += k * (target.green - prev.green);
			prev.blue  += k * (target.blue  - prev.blue);
		}
		_previousTime = now;

		_ledDevice->write(_previousValues);
	}
}
