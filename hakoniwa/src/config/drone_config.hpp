#ifndef _DRONE_CONFIG_HPP_
#define _DRONE_CONFIG_HPP_

#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>

using json = nlohmann::json;
//#define DRONE_PX4_RX_DEBUG_ENABLE
//DRONE_PX4_TX_DEBUG_ENABLE
//DRONE_PID_CONTROL_CPP
struct RotorPosition {
    std::vector<double> position;
    double rotationDirection;
};
#define DEGREE2RADIAN(v)    ( (v) * M_PI / (180.0) )
#define RADIAN2DEGREE(v)    ( (180.0 * (v)) / M_PI )

class DroneConfig {
private:
    json configJson;
    std::string config_filepath;
public:
    DroneConfig() {}
    bool init(const std::string& configFilePath) {
        config_filepath = configFilePath;
        std::ifstream configFile(config_filepath);
        if (configFile.is_open()) {
            try {
                configFile >> configJson;
            } catch (json::parse_error& e) {
                std::cerr << "JSON parsing error: " << e.what() << std::endl;
                return false;
            }
        } else {
            std::cerr << "Unable to open config file: " << config_filepath << std::endl;
            return false;
        }
        return true;
    }

    // Simulation parameters
    double getSimTimeStep() const {
        return configJson["simulation"]["timeStep"].get<double>();
    }
    bool getSimLockStep() const {
        return configJson["simulation"]["lockstep"].get<bool>();
    }
    std::string getSimLogOutputDirectory() const 
    {
        std::string directory = configJson["simulation"]["logOutputDirectory"].get<std::string>();
        // ディレクトリの存在を確認
        if (!std::filesystem::exists(directory)) {
            std::cerr << "Error: Log output directory '" << directory << "' does not exist." << std::endl;
            return "./"; // または適切なデフォルト値を返すか、ディレクトリを作成する
        }
        return directory;
    }
    std::string getSimLogFullPath(const std::string& filename) const
    {
        std::string logDirectory = getSimLogOutputDirectory();

        // パス区切り文字を確認して追加する（必要な場合のみ）
        if (logDirectory.back() != '/' && logDirectory.back() != '\\') {
            logDirectory += "/";
        }

        // 完全なログファイルパスを返す
        return logDirectory + filename;
    }

    // Log Output for Sensors
    bool isSimSensorLogEnabled(const std::string& sensorName) const {
        return configJson["simulation"]["logOutput"]["sensors"][sensorName].get<bool>();
    }

    // Log Output for MAVLINK
    bool isMSimavlinkLogEnabled(const std::string& mavlinkMessage) const {
        return configJson["simulation"]["logOutput"]["mavlink"][mavlinkMessage].get<bool>();
    }

    // MAVLINK Transmission Period
    int getSimMavlinkTransmissionPeriod(const std::string& mavlinkMessage) const {
        return configJson["simulation"]["mavlink_tx_period_msec"][mavlinkMessage].get<int>();
    }

    // Location parameters
    double getSimLatitude() const {
        return configJson["simulation"]["location"]["latitude"].get<double>();
    }

    double getSimLongitude() const {
        return configJson["simulation"]["location"]["longitude"].get<double>();
    }

    double getSimAltitude() const {
        return configJson["simulation"]["location"]["altitude"].get<double>();
    }

    struct MagneticField {
        double intensity_nT;
        double declination_deg;
        double inclination_deg;
    };

    MagneticField getSimMagneticField() const {
        MagneticField field;
        field.intensity_nT = configJson["simulation"]["location"]["magneticField"]["intensity_nT"].get<double>();
        field.declination_deg = configJson["simulation"]["location"]["magneticField"]["declination_deg"].get<double>();
        field.inclination_deg = configJson["simulation"]["location"]["magneticField"]["inclination_deg"].get<double>();
        return field;
    }

    // Drone Dynamics parameters
    std::string getCompDroneDynamicsPhysicsEquation() const {
        return configJson["components"]["droneDynamics"]["physicsEquation"].get<std::string>();
    }

    std::vector<double> getCompDroneDynamicsAirFrictionCoefficient() const {
        std::vector<double> frictions;
        for (const auto& item : configJson["components"]["droneDynamics"]["airFrictionCoefficient"]) {
            frictions.push_back(item);
        }
        return frictions;
    }
    bool getCompDroneDynamicsCollisionDetection() const {
        return configJson["components"]["droneDynamics"]["collision_detection"].get<bool>();
    }
    bool getCompDroneDynamicsManualControl() const {
        return configJson["components"]["droneDynamics"]["manual_control"].get<bool>();
    }
    std::vector<double> getCompDroneDynamicsBodySize() const {
        std::vector<double> body_size;
        for (const auto& item : configJson["components"]["droneDynamics"]["body_size"]) {
            body_size.push_back(item);
        }
        return body_size;
    }
    std::vector<double> getCompDroneDynamicsInertia() const {
        std::vector<double> inertia;
        for (const auto& item : configJson["components"]["droneDynamics"]["inertia"]) {
            inertia.push_back(item);
        }
        return inertia;
    }
    std::vector<double> getCompDroneDynamicsPosition() const {
        std::vector<double> inertia;
        for (const auto& item : configJson["components"]["droneDynamics"]["position_meter"]) {
            inertia.push_back(item);
        }
        return inertia;
    }
    std::vector<double> getCompDroneDynamicsAngle() const {
        std::vector<double> inertia;
        for (const auto& item : configJson["components"]["droneDynamics"]["angle_degree"]) {
            inertia.push_back(item);
        }
        return inertia;
    }
    double getCompDroneDynamicsMass() const {
        return configJson["components"]["droneDynamics"]["mass_kg"].get<double>();
    }
    std::string getCompRotorVendor() const {
        // 指定されたパスにパラメータが存在するかチェック
        if (configJson["components"]["rotor"].contains("vendor")) {
            return configJson["components"]["rotor"]["vendor"].get<std::string>();
        } else {
            // パラメータが存在しない場合は 0 を返す
            return "None";
        }
    }

    // Rotor parameters
    double getCompRotorTr() const {
        return configJson["components"]["rotor"]["Tr"].get<double>();
    }

    double getCompRotorKr() const {
        return configJson["components"]["rotor"]["Kr"].get<double>();
    }

    int getCompRotorRpmMax() const {
        return configJson["components"]["rotor"]["rpmMax"].get<int>();
    }

    // Thruster parameters
    std::vector<RotorPosition> getCompThrusterRotorPositions() const {
        std::vector<RotorPosition> positions;
        for (const auto& item : configJson["components"]["thruster"]["rotorPositions"]) {
            RotorPosition pos;
            pos.position = item["position"].get<std::vector<double>>();
            pos.rotationDirection = item["rotationDirection"].get<double>();
            positions.push_back(pos);
        }
        return positions;
    }
    double getCompThrusterParameter(const std::string& param_name) const {
        // 指定されたパスにパラメータが存在するかチェック
        if (configJson["components"]["thruster"].contains(param_name)) {
            return configJson["components"]["thruster"][param_name].get<double>();
        } else {
            // パラメータが存在しない場合は 0 を返す
            return 0.0;
        }
    }
    std::string getCompThrusterVendor() const {
        // 指定されたパスにパラメータが存在するかチェック
        if (configJson["components"]["thruster"].contains("vendor")) {
            return configJson["components"]["thruster"]["vendor"].get<std::string>();
        } else {
            // パラメータが存在しない場合は 0 を返す
            return "None";
        }
    }
    double getCompSensorSampleCount(const std::string& sensor_name) const {
        return configJson["components"]["sensors"][sensor_name]["sampleCount"].get<double>();
    }
    double getCompSensorNoise(const std::string& sensor_name) const {
        return configJson["components"]["sensors"][sensor_name]["noise"].get<double>();
    }
    double getControllerPid(const std::string& param1, const std::string& param2, const std::string& param3)
    {
        return configJson["controller"]["pid"][param1][param2][param3].get<double>();
    }
};

extern class DroneConfig drone_config;

#endif /* _DRONE_CONFIG_HPP_ */