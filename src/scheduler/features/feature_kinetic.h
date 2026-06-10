// FEATURE_KINETIC: kinetic installation calibration data (de)serialisation.
// Enable with -DFEATURE_KINETIC.
#pragma once

#ifdef FEATURE_KINETIC

#include <cstdint>
#include <string>

struct SKineticData
{
    bool bNeedsCalibration;
    bool bCalibrated;
    int32_t iCalibrationHour;
    int32_t iCalibrationMinute;
    int32_t iCalibrationSecond;
};

void DeserialiseKinetic(const std::string& str, SKineticData& data);
void SerialiseKinetic(std::string& str, const SKineticData& data);

#endif // FEATURE_KINETIC
