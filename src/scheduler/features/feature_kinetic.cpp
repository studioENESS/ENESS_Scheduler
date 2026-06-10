#include "feature_kinetic.h"

#ifdef FEATURE_KINETIC

#include "nlohmann/json.hpp"

void DeserialiseKinetic(const std::string& str, SKineticData& data)
{
    nlohmann::json j = nlohmann::json::parse(str);

    data.bNeedsCalibration = j["bNeedsCalibration"];
    data.bCalibrated = j["bCalibrated"];
    data.iCalibrationHour = j["iCalibrationHour"];
    data.iCalibrationMinute = j["iCalibrationMinute"];
    data.iCalibrationSecond = j["iCalibrationSecond"];
}

void SerialiseKinetic(std::string& str, const SKineticData& data)
{
    nlohmann::json j;
    j["bNeedsCalibration"] = data.bNeedsCalibration;
    j["bCalibrated"] = data.bCalibrated;
    j["iCalibrationHour"] = data.iCalibrationHour;
    j["iCalibrationMinute"] = data.iCalibrationMinute;
    j["iCalibrationSecond"] = data.iCalibrationSecond;
    str = j.dump();
}

#endif // FEATURE_KINETIC
