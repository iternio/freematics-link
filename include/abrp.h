#ifndef INC_ABRP_H
#define INC_ABRP_H

// #include <string>
// #include <stdint.h>
#include <ArduinoJson.h>
// #include <HTTPClient.h>

#define ABRP_SERVER_HOST "api.iternio.com"
#define ABRP_SERVER_ENDPOINT "/1/tlm/send"
#define ABRP_SERVER_AUTHHEADER "Authorization"
#define ABRP_SERVER_AUTHTEXT "APIKEY "
#define ABRP_SERVER_AUTHVAR "api_key"
#define ABRP_SERVER_TOKENVAR "token"
#define ABRP_SERVER_TELEMVAR "tlm"

//TODO: Make these not configurable variables rather than #define's
#define ABRP_SERVER_APIKEY "------"
#define ABRP_SERVER_TOKEN "------"

template <typename T>
struct ABRPTelemetryValue {
public:
    ABRPTelemetryValue() : has_value { false }, value { } {};
    T operator()() { return value; };
    void operator=(T val) { value = val; has_value = true; };
    bool exists() { return has_value; };
private:
    bool has_value;
    T value;
};

struct ABRPTelemetry {
public:
    //Mandatory fields
    ABRPTelemetryValue<unsigned long> utc;      //UTC timestamp (s)
    ABRPTelemetryValue<float> soc;              //State of charge (%)
    ABRPTelemetryValue<float> speed;            //Speed (km/h)
    ABRPTelemetryValue<double> lat;              //Latitude (°)
    ABRPTelemetryValue<double> lon;              //Longitude (°)
    ABRPTelemetryValue<bool> is_charging;      //Charging state (boolean)
    //Optional fields
    ABRPTelemetryValue<float> power;            //Power input/output (kW)
    ABRPTelemetryValue<bool> is_dcfc;          //Chargin state is DC fast (boolean)
    ABRPTelemetryValue<float> battery_capacity; //Battery capacity (kWh)
    ABRPTelemetryValue<float> soh;              //State of health (%)
    ABRPTelemetryValue<float> elevation;        //Elevation (m)
    ABRPTelemetryValue<float> ext_temp;         //Outside temperature (°C)
    ABRPTelemetryValue<float> batt_temp;        //Battery temperature (°C)
    ABRPTelemetryValue<float> voltage;          //Battery voltage (V)
    ABRPTelemetryValue<float> current;          //Battery current (A)
    ABRPTelemetryValue<String> car_model;       //Car model identifier (string)

    ABRPTelemetry() : utc { }, soc { }, speed { }, lat { }, lon { }, is_charging { }, power { }, is_dcfc { },
        battery_capacity { }, soh { }, elevation { }, ext_temp { }, batt_temp { }, voltage { }, current { }, car_model { }
    {
        car_model = "freematics";
    };

    String toJSON() {
        String ret;
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

class ABRP {
public:
    // virtual bool start() = 0;
    // virtual bool connect() = 0;
    // virtual bool disconnect() = 0;
    // virtual bool end() = 0;
    bool send(ABRPTelemetry &telem);
protected:
    // virtual int write() = 0;
    // virtual int read() = 0;
    // virtual bool started() = 0;
    // virtual bool connected() = 0;

};

// class ABRPWiFi : public ABRP {
// public:
//     virtual bool begin();
//     virtual bool end();
// protected:
//     virtual int write();
//     virtual int read();
//     HTTPClient client;
// };

#endif
