#pragma once
#include <Arduino.h>
#include "../Logging/Logging.hpp"

#include "esp_adc_cal.h"

typedef enum BatteryLevel
{
    BATTERY_LEVEL_CRITICAL = 5,
    BATTERY_LEVEL_LOW = 25,
    BATTERY_LEVEL_MEDIUM = 50,
    BATTERY_LEVEL_HIGH = 75,
    BATTERY_LEVEL_FULL = 100,
} BatteryLevel_t;

#define ADC_MEASURE_CNT 5
#define ADC_DIVIDER (2.3 / 1.3) // resistor divider R1 = 1M and R2 = 1.3M on PCB
#define BATT_PIN (34)

class Battery
{
public:
    Battery()
    {
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
            ADC_UNIT_2,
            ADC_ATTEN_DB_12,
            ADC_WIDTH_BIT_12,
            1100,
            &adc_chars);
    }

    bool setup()
    {
        esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
            ADC_UNIT_2,
            ADC_ATTEN_DB_12,
            ADC_WIDTH_BIT_12,
            1100,
            &adc_chars);

        return true;
    }

    int getBatteryPercent()
    {
        return convertVoltageToPercent(getVBatVoltage());
    }

private:
    esp_adc_cal_characteristics_t adc_chars;

    float getVBatVoltage()
    {
        uint32_t voltageRaw = 0;
        for (uint8_t i = 0; i < ADC_MEASURE_CNT; i++)
        {
            voltageRaw += analogRead(BATT_PIN);
        }
        voltageRaw /= ADC_MEASURE_CNT;

        return (float)esp_adc_cal_raw_to_voltage(voltageRaw, &adc_chars) * ADC_DIVIDER / 1000.0;
    }

    int convertVoltageToPercent(float voltage)
    {
        float voltageToPercent[21] = {3.27, 3.61, 3.69, 3.71, 3.73, 3.75, 3.77, 3.79, 3.80, 3.82, 3.84, 3.85, 3.87, 3.91, 3.95, 3.98, 4.02, 4.08, 4.11, 4.15, 4.20};
        int voltageCent = (int)(voltage * 100);
        for (int i = 0; i < 21; i++)
        {
            int limitCent = (int)(voltageToPercent[i] * 100);
            if (voltageCent <= limitCent)
            {
                return 5 * i;
            }
        }
        return 100;
    }
};
