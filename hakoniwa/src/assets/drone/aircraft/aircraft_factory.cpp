#include "aircraft_factory.hpp"
#include "utils/hako_utils.hpp"
#include "assets/drone/physics/body_frame/drone_dynamics_body_frame.hpp"
#include "assets/drone/physics/body_frame_rk4/drone_dynamics_body_frame_rk4.hpp"
#include "assets/drone/physics/ground_frame/drone_dynamics_ground_frame.hpp"
#include "assets/drone/physics/rotor/rotor_dynamics.hpp"
#include "assets/drone/physics/rotor/rotor_dynamics_jmavsim.hpp"
#include "assets/drone/physics/thruster/thrust_dynamics_linear.hpp"
#include "assets/drone/physics/thruster/thrust_dynamics_nonlinear.hpp"
#include "assets/drone/sensors/acc/sensor_acceleration.hpp"
#include "assets/drone/sensors/baro/sensor_baro.hpp"
#include "assets/drone/sensors/gps/sensor_gps.hpp"
#include "assets/drone/sensors/gyro/sensor_gyro.hpp"
#include "assets/drone/sensors/mag/sensor_mag.hpp"
#include "assets/drone/aircraft/aricraft.hpp"
#include "assets/drone/utils/sensor_noise.hpp"
#include "config/drone_config.hpp"
#include <math.h>

using hako::assets::drone::AirCraft;
using hako::assets::drone::DroneDynamicsBodyFrame;
using hako::assets::drone::DroneDynamicsGroundFrame;
using hako::assets::drone::SensorAcceleration;
using hako::assets::drone::SensorBaro;
using hako::assets::drone::SensorGps;
using hako::assets::drone::SensorMag;
using hako::assets::drone::SensorGyro;
using hako::assets::drone::RotorDynamics;
using hako::assets::drone::ThrustDynamicsLinear;
using hako::assets::drone::ThrustDynamicsNonLinear;
using hako::assets::drone::SensorNoise;

#define DELTA_TIME_SEC              drone_config.getSimTimeStep()
#define REFERENCE_LATITUDE          drone_config.getSimLatitude()
#define REFERENCE_LONGTITUDE        drone_config.getSimLongitude()
#define REFERENCE_ALTITUDE          drone_config.getSimAltitude()
#define PARAMS_MAG_F   drone_config.getSimMagneticField().intensity_nT
#define PARAMS_MAG_D    DEGREE2RADIAN(drone_config.getSimMagneticField().declination_deg)
#define PARAMS_MAG_I    DEGREE2RADIAN(drone_config.getSimMagneticField().inclination_deg)

#define ACC_SAMPLE_NUM              drone_config.getCompSensorSampleCount("acc")
#define GYRO_SAMPLE_NUM             drone_config.getCompSensorSampleCount("gyro")
#define BARO_SAMPLE_NUM             drone_config.getCompSensorSampleCount("baro")
#define GPS_SAMPLE_NUM              drone_config.getCompSensorSampleCount("gps")
#define MAG_SAMPLE_NUM              drone_config.getCompSensorSampleCount("mag")

#define RPM_MAX                     drone_config.getCompRotorRpmMax()
#define ROTOR_TAU                   drone_config.getCompRotorTr()
#define ROTOR_K                     drone_config.getCompRotorKr()

#define THRUST_PARAM_B              drone_config.getCompThrusterParameter("parameterB")
#define THRUST_PARAM_JR             drone_config.getCompThrusterParameter("parameterJr")

#define LOGPATH(name)               drone_config.getSimLogFullPath(name)

IAirCraft* hako::assets::drone::create_aircraft(const char* drone_type)
{
    (void)drone_type;

    auto drone = new AirCraft();
    HAKO_ASSERT(drone != nullptr);

    //drone dynamics
    IDroneDynamics *drone_dynamics = nullptr;
    if (drone_config.getCompDroneDynamicsPhysicsEquation() == "BodyFrame") {
        drone_dynamics = new DroneDynamicsBodyFrame(DELTA_TIME_SEC);
    }
    else if (drone_config.getCompDroneDynamicsPhysicsEquation() == "BodyFrameRK4") {
        drone_dynamics = new DroneDynamicsBodyFrameRK4(DELTA_TIME_SEC);
    }
    else {
        drone_dynamics = new DroneDynamicsGroundFrame(DELTA_TIME_SEC);
    }
    //auto drone_dynamics = new DroneDynamicsGroundFrame(DELTA_TIME_SEC);
    HAKO_ASSERT(drone_dynamics != nullptr);
    auto drags = drone_config.getCompDroneDynamicsAirFrictionCoefficient();
    drone_dynamics->set_drag(drags[0], drags[1]);
    drone_dynamics->set_mass(drone_config.getCompDroneDynamicsMass());
    drone_dynamics->set_collision_detection(drone_config.getCompDroneDynamicsCollisionDetection());
    drone_dynamics->set_manual_control(drone_config.getCompDroneDynamicsManualControl());
    auto body_size = drone_config.getCompDroneDynamicsBodySize();
    drone_dynamics->set_body_size(body_size[0], body_size[1], body_size[2]);
    auto inertia = drone_config.getCompDroneDynamicsInertia();
    drone_dynamics->set_torque_constants(inertia[0], inertia[1], inertia[2]);
    auto position = drone_config.getCompDroneDynamicsPosition();
    DronePositionType drone_pos;
    drone_pos.data = { position[0], position[1], position[2] }; 
    drone_dynamics->set_pos(drone_pos);
    auto angle = drone_config.getCompDroneDynamicsAngle();
    DroneEulerType rot;
    rot.data = { DEGREE2RADIAN(angle[0]), DEGREE2RADIAN(angle[1]), DEGREE2RADIAN(angle[2]) };
    drone_dynamics->set_angle(rot);
    drone->set_drone_dynamics(drone_dynamics);
    drone->get_logger().add_entry(*drone_dynamics, LOGPATH("drone_dynamics.csv"));

    //rotor dynamics
    IRotorDynamics* rotors[hako::assets::drone::ROTOR_NUM];
    auto rotor_vendor = drone_config.getCompRotorVendor();
    std::cout<< "Rotor vendor: " << rotor_vendor << std::endl;
    for (int i = 0; i < hako::assets::drone::ROTOR_NUM; i++) {
        IRotorDynamics *rotor = nullptr;
        std::string logfilename= "log_rotor_" + std::to_string(i) + ".csv";
        if (rotor_vendor == "jmavsim") {
            rotor = new RotorDynamicsJmavsim(DELTA_TIME_SEC);
            HAKO_ASSERT(rotor != nullptr);
            static_cast<RotorDynamicsJmavsim*>(rotor)->set_params(RPM_MAX, ROTOR_TAU, ROTOR_K);
            drone->get_logger().add_entry(*static_cast<RotorDynamicsJmavsim*>(rotor), LOGPATH(logfilename));
        }
        else {
            rotor = new RotorDynamics(DELTA_TIME_SEC);
            HAKO_ASSERT(rotor != nullptr);
            static_cast<RotorDynamics*>(rotor)->set_params(RPM_MAX, ROTOR_TAU, ROTOR_K);
            drone->get_logger().add_entry(*static_cast<RotorDynamics*>(rotor), LOGPATH(logfilename));
        }
        rotors[i] = rotor;
    }
    drone->set_rotor_dynamics(rotors);

    //thrust dynamics
    IThrustDynamics *thrust = nullptr;
    auto thrust_vendor = drone_config.getCompThrusterVendor();
    std::cout<< "Thruster vendor: " << thrust_vendor << std::endl;
    if (thrust_vendor == "linear") {
        thrust = new ThrustDynamicsLinear(DELTA_TIME_SEC);
        HAKO_ASSERT(thrust != nullptr);
        double HoveringRpm = drone_config.getCompThrusterParameter("HoveringRpm");
        HAKO_ASSERT(HoveringRpm != 0);
        double mass = drone_dynamics->get_mass();
        double param_A = (mass * GRAVITY / (HoveringRpm * ROTOR_NUM));
        double param_B = drone_config.getCompThrusterParameter("parameterB_linear");
        static_cast<ThrustDynamicsLinear*>(thrust)->set_params(
            param_A,
            param_B
        );
        std::cout << "param_A_linear: " << param_A << std::endl;
        std::cout << "param_B_linear: " << param_B << std::endl;
        drone->get_logger().add_entry(*static_cast<ThrustDynamicsLinear*>(thrust), LOGPATH("log_thrust.csv"));
    }
    else {
        thrust = new ThrustDynamicsNonLinear(DELTA_TIME_SEC);
        HAKO_ASSERT(thrust != nullptr);
        double HoveringRpm = drone_config.getCompThrusterParameter("HoveringRpm");
        HAKO_ASSERT(HoveringRpm != 0);
        double mass = drone_dynamics->get_mass();
        double param_A = ( 
                            mass * GRAVITY / 
                            (
                                pow(HoveringRpm, 2) * ROTOR_NUM
                            )
                        );
        std::cout << "param_A: " << param_A << std::endl;
        std::cout << "param_B: " << THRUST_PARAM_B << std::endl;
        std::cout << "param_Jr: " << THRUST_PARAM_JR << std::endl;
        static_cast<ThrustDynamicsNonLinear*>(thrust)->set_params(param_A, THRUST_PARAM_B, THRUST_PARAM_JR);
        drone->get_logger().add_entry(*static_cast<ThrustDynamicsNonLinear*>(thrust), LOGPATH("log_thrust.csv"));
    }
    drone->set_thrus_dynamics(thrust);

    RotorConfigType rotor_config[ROTOR_NUM];
    std::vector<RotorPosition> pos = drone_config.getCompThrusterRotorPositions();
    HAKO_ASSERT(pos.size() == ROTOR_NUM);
    for (size_t i = 0; i < pos.size(); ++i) {
        rotor_config[i].ccw = pos[i].rotationDirection;
        rotor_config[i].data.x = pos[i].position[0];
        rotor_config[i].data.y = pos[i].position[1];
        rotor_config[i].data.z = pos[i].position[2];
    }    

    thrust->set_rotor_config(rotor_config);

    //sensor acc
    auto acc = new SensorAcceleration(DELTA_TIME_SEC, ACC_SAMPLE_NUM);
    HAKO_ASSERT(acc != nullptr);
    double variance = drone_config.getCompSensorNoise("acc");
    if (variance > 0) {
        auto noise = new SensorNoise(variance);
        HAKO_ASSERT(noise != nullptr);
        acc->set_noise(noise);
    }
    drone->set_acc(acc);
    drone->get_logger().add_entry(*acc, LOGPATH("log_acc.csv"));

    //sensor gyro
    auto gyro = new SensorGyro(DELTA_TIME_SEC, ACC_SAMPLE_NUM);
    HAKO_ASSERT(gyro != nullptr);
    variance = drone_config.getCompSensorNoise("gyro");
    if (variance > 0) {
        auto noise = new SensorNoise(variance);
        HAKO_ASSERT(noise != nullptr);
        gyro->set_noise(noise);
    }
    drone->set_gyro(gyro);
    drone->get_logger().add_entry(*gyro, LOGPATH("log_gyro.csv"));

    //sensor mag
    auto mag = new SensorMag(DELTA_TIME_SEC, ACC_SAMPLE_NUM);
    HAKO_ASSERT(mag != nullptr);
    variance = drone_config.getCompSensorNoise("mag");
    if (variance > 0) {
        auto noise = new SensorNoise(variance);
        HAKO_ASSERT(noise != nullptr);
        mag->set_noise(noise);
    }
    mag->set_params(PARAMS_MAG_F, PARAMS_MAG_I, PARAMS_MAG_D);
    drone->set_mag(mag);
    drone->get_logger().add_entry(*mag, LOGPATH("log_mag.csv"));

    //sensor baro
    auto baro = new SensorBaro(DELTA_TIME_SEC, ACC_SAMPLE_NUM);
    HAKO_ASSERT(baro != nullptr);
    baro->init_pos(REFERENCE_LATITUDE, REFERENCE_LONGTITUDE, REFERENCE_ALTITUDE);
    variance = drone_config.getCompSensorNoise("baro");
    if (variance > 0) {
        auto noise = new SensorNoise(variance);
        HAKO_ASSERT(noise != nullptr);
        baro->set_noise(noise);
    }
    drone->set_baro(baro);
    drone->get_logger().add_entry(*baro, LOGPATH("log_baro.csv"));

    //sensor gps
    auto gps = new SensorGps(DELTA_TIME_SEC, ACC_SAMPLE_NUM);
    HAKO_ASSERT(gps != nullptr);
    variance = drone_config.getCompSensorNoise("gps");
    if (variance > 0) {
        auto noise = new SensorNoise(variance);
        HAKO_ASSERT(noise != nullptr);
        gps->set_noise(noise);
    }
    gps->init_pos(REFERENCE_LATITUDE, REFERENCE_LONGTITUDE, REFERENCE_ALTITUDE);
    drone->set_gps(gps);
    drone->get_logger().add_entry(*gps, LOGPATH("log_gps.csv"));

    return drone;
}