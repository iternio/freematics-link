/*
 * Structure to hold telemetry to send
 */

#pragma once

// #include <string.h>
#include <ArduinoJson.h>    //TODO: Do we need this? we use very little of it

namespace abrp {

    namespace telemetry {

        template <typename T>
        struct Value {
        public:
            Value() : has_value { false }, value { } {};
            T operator()() { return value; };
            void operator=(T val) { value = val; has_value = true; };
            bool exists() { return has_value; };
        private:
            bool has_value;
            T value;
        };

        struct Telemetry {
        public:
            //Mandatory fields
            Value<unsigned long> utc;      //UTC timestamp (s)
            Value<float> soc;              //State of charge (%)
            Value<float> speed;            //Speed (km/h)
            Value<double> lat;              //Latitude (°)
            Value<double> lon;              //Longitude (°)
            Value<bool> is_charging;      //Charging state (boolean)
            //Optional fields
            Value<float> power;            //Power input/output (kW)
            Value<bool> is_dcfc;          //Chargin state is DC fast (boolean)
            Value<float> battery_capacity; //Battery capacity (kWh)
            Value<float> soh;              //State of health (%)
            Value<float> elevation;        //Elevation (m)
            Value<float> ext_temp;         //Outside temperature (°C)
            Value<float> batt_temp;        //Battery temperature (°C)
            Value<float> voltage;          //Battery voltage (V)
            Value<float> current;          //Battery current (A)
            Value<String> car_model;       //Car model identifier (string)

            Telemetry() : utc { }, soc { }, speed { }, lat { }, lon { }, is_charging { }, power { }, is_dcfc { },
                battery_capacity { }, soh { }, elevation { }, ext_temp { }, batt_temp { }, voltage { }, current { }, car_model { }
            {
                car_model = "freematics";
            };

            String toJSON() {
                String ret;
                //TODO: temporarily commenting this out to resovle build issues
                // ret = "{}";
                StaticJsonDocument<JSON_OBJECT_SIZE(16)> doc;
                //Mandatory fields
                if (!utc.exists() || !soc.exists() || !speed.exists() || !lat.exists() || !lon.exists() || !is_charging.exists())
                    return String();
                doc["utc"] = utc();
                doc["soc"] = soc();
                doc["speed"] = speed();
                doc["lat"] = serialized(String(lat(),7));
                doc["lon"] = serialized(String(lon(),7));
                doc["is_charging"] = (is_charging() ? 1 : 0);
                //Optional fields
                if (power.exists())
                    doc["power"] = power();
                if (is_dcfc.exists())
                    doc["is_dcfc"] = (is_dcfc() ? 1 : 0);
                if (battery_capacity.exists())
                    doc["battery_capacity"] = battery_capacity();
                if (soh.exists())
                    doc["soh"] = soh();
                if (elevation.exists())
                    doc["elevation"] = elevation();
                if (ext_temp.exists())
                    doc["ext_temp"] = ext_temp();
                if (batt_temp.exists())
                    doc["batt_temp"] = batt_temp();
                if (voltage.exists())
                    doc["voltage"] = voltage();
                if (current.exists())
                    doc["current"] = current();
                if (car_model.exists())
                    doc["car_model"] = car_model();
                serializeJson(doc, ret);
                return ret;
            }
        };

    }

}
