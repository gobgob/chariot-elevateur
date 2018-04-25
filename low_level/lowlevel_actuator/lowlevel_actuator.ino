#include "config.h"
#include "InternalCom.h"
#include "MotorSensor.h"
#include "ArmController.h"
#include "ArmPosition.h"
#include "Communication.h"
#include "SensorsMgr.h"
#include "UserInputControler.h"
#include <Wire.h>


void setup()
{
    pinMode(PIN_EN_TOF_G, OUTPUT);
    digitalWrite(PIN_EN_TOF_G, LOW);
    pinMode(PIN_EN_TOF_D, OUTPUT);
    digitalWrite(PIN_EN_TOF_D, LOW);
    delay(1000);
}

void errorLoop()
{
    pinMode(PIN_DEL_GYRO_G, OUTPUT);
    pinMode(PIN_DEL_GYRO_D, OUTPUT);
    while (true)
    {
        for (int i = 0; i < 4; i++)
        {
            digitalWrite(PIN_DEL_GYRO_G, HIGH);
            digitalWrite(PIN_DEL_GYRO_D, HIGH);
            delay(100);
            digitalWrite(PIN_DEL_GYRO_G, LOW);
            digitalWrite(PIN_DEL_GYRO_D, LOW);
            delay(100);
        }
        delay(400);
    }
}

void loop()
{
    Serial.println("Coucou");
    pinMode(PIN_DEL_GYRO_G, OUTPUT);
    pinMode(PIN_DEL_GYRO_D, OUTPUT);
    digitalWrite(PIN_DEL_GYRO_G, HIGH);
    digitalWrite(PIN_DEL_GYRO_D, LOW);
    Wire.begin();
    delay(500);

    Communication communication;
    SensorsMgr sensorsMgr;
    UserInputControler userInputControler;

    if (communication.init() != 0)
    {
        Serial.println("Communication init failed");
        errorLoop();
    }

    if (sensorsMgr.init() != 0)
    {
        Serial.println("Sensors init failed");
        errorLoop();
    }

    userInputControler.init();

    IntervalTimer timer;
    timer.priority(253);
    timer.begin(armControllerInterrupt, PERIOD_ASSERV);

    uint32_t reportTimer = 0;
    while (true)
    {
        communication.listenAndExecute();

        if (communication.newScoreAvailable())
        {
            userInputControler.setScore(communication.getScore());
        }

        if (communication.newSensorsAngle())
        {
            sensorsMgr.setAimAngles(communication.getLeftSensorAngle(), communication.getRightSensorAngle());
        }

        if (millis() - reportTimer > REPORT_PERIOD)
        {
            reportTimer = millis();
            int32_t tofG = 0, tofD = 0;
            float angleG = 0, angleD = 0;
            sensorsMgr.getSensorsData(tofG, tofD, angleG, angleD);
            communication.sendReport(tofG, tofD, angleG, angleD);
        }

        sensorsMgr.updateServos();

        userInputControler.control();
    }
}

void armControllerInterrupt()
{
    static ArmController & armController = ArmController::Instance();
    armController.controlCCMotors();
}


/* Ce bout de code permet de compiler avec std::vector */
namespace std {
    void __throw_bad_alloc()
    {
        while (true)
        {
            Serial.printf("Unable to allocate memory\n");
            delay(500);
        }
    }

    void __throw_length_error(char const*e)
    {
        while (true)
        {
            Serial.println(e);
            delay(500);
        }
    }

    void __throw_out_of_range(char const*e)
    {
        while (true)
        {
            Serial.println(e);
            delay(500);
        }
    }

    void __throw_out_of_range_fmt(char const*e, ...)
    {
        while (true)
        {
            Serial.println(e);
            delay(500);
        }
    }
}
