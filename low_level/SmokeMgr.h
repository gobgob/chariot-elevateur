#ifndef SMOKE_MGR_h
#define SMOKE_MGR_h

#include <Arduino.h>
#include "Singleton.h"
#include "Config.h"
#include "MotionControlSystem.h"
#include "CommunicationServer.h"

#define SMOKE_MGR_UPDATE_PRIOD      50    // ms
#define P_PUMP_ON                   2000
#define P_PUMP_OFF                  0000
#define P_RES_ON                    1000
#define P_RES_OFF                   1000
#define SMOKE_OFF                   0       // pwm
#define SMOKE_ON                    32762   // pwm

class SmokeMgr : public Singleton<SmokeMgr>
{
public:
    SmokeMgr()
    {
        m_enabled = false;
        m_res_on = false;
        m_pump_on = false;
        m_res_timer = 0;
        m_pump_timer = 0;
        m_last_update_time = 0;
        pinMode(PIN_SMOKE_PUMP, OUTPUT);
        digitalWrite(PIN_SMOKE_PUMP, LOW);
        pinMode(PIN_SMOKE_RESISTOR, OUTPUT);
        digitalWrite(PIN_SMOKE_RESISTOR, LOW);
    }

    void update()
    {
        uint32_t now = millis();
        if (now - m_last_update_time > SMOKE_MGR_UPDATE_PRIOD)
        {
            m_last_update_time = now;

            if (m_res_on) {
                if (now - m_res_timer > P_RES_ON) {
                    m_res_timer = now;
                    m_res_on = false;
                }
            }
            else {
                if (now - m_res_timer > P_RES_OFF) {
                    m_res_timer = now;
                    m_res_on = true;
                }
            }

            if (m_pump_on) {
                if (now - m_pump_timer > P_PUMP_ON) {
                    m_pump_timer = now;
                    m_pump_on = false;
                }
            }
            else {
                if (now - m_pump_timer > P_PUMP_OFF) {
                    m_pump_timer = now;
                    m_pump_on = true;
                }
            }

            if (m_enabled && m_res_on) {
                analogWrite(PIN_SMOKE_RESISTOR, SMOKE_ON);
            }
            else {
                analogWrite(PIN_SMOKE_RESISTOR, SMOKE_OFF);
            }
            digitalWrite(PIN_SMOKE_PUMP, m_enabled && m_pump_on);
        }
    }

    void enableSmoke(bool enable)
    {
        m_enabled = enable;
        m_last_update_time = 0;
        m_res_on = false;
        m_pump_on = false;
        m_res_timer = millis();
        m_pump_timer = millis();
    }

private:
    bool m_enabled;
    bool m_res_on;
    bool m_pump_on;
    uint32_t m_last_update_time;
    uint32_t m_res_timer;
    uint32_t m_pump_timer;
};

#endif
