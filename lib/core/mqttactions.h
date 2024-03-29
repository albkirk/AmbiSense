// Adding function with project's customized MQTT actions
#include <hassio.h>
#include <custommqtt.h>



// Handling of received message
void on_message(const char* topic, byte* payload, unsigned int msg_length) {

    telnet_println("New message received from Broker");

    char msg[msg_length + 1];
    strncpy (msg, (char*)payload, msg_length);
    msg[msg_length] = '\0';

    //telnet_println("Topic: " + String(topic));
    //telnet_println("Payload: " + String((char*)msg));

    // Check requested command
    String command = String(topic);
    command.replace(mqtt_pathconf, "");
    String cmd_value = String((char*)msg);
    telnet_print("Requested Command: " + command);
    telnet_println("\tCommand Value: " + cmd_value);

    // System Configuration 
    if ( command == "DeviceName") strcpy(config.DeviceName, cmd_value.c_str());
    if ( command == "Location") strcpy(config.Location, cmd_value.c_str());
    if ( command == "ClientID") strcpy(config.ClientID, cmd_value.c_str());
    if ( command == "DEEPSLEEP") { config.DEEPSLEEP = bool(cmd_value.toInt());storage_write(); }
    if ( command == "SLEEPTime") { config.SLEEPTime = byte(cmd_value.toInt());storage_write(); }
    if ( command == "ONTime") { config.ONTime = byte(cmd_value.toInt());storage_write(); }
    if ( command == "ExtendONTime") if (bool(cmd_value.toInt()) == true) Extend_time = 60;
    if ( command == "LED") {config.LED = bool(cmd_value.toInt()); mqtt_publish(mqtt_pathtele, "LED", String(config.LED));}
    if ( command == "TELNET") { config.TELNET = bool(cmd_value.toInt()); storage_write(); telnet_setup(); }
    if ( command == "OTA") { config.OTA = bool(cmd_value.toInt()); storage_write(); ESPRestart(); }
    if ( command == "WEB") { config.WEB = bool(cmd_value.toInt()); storage_write(); web_setup(); }
    if ( command == "DHCP") { config.DHCP = bool(cmd_value.toInt()); storage_write(); wifi_connect(); }
    if ( command == "STAMode") { config.STAMode = bool(cmd_value.toInt()); storage_write(); }
    if ( command == "APMode") { config.APMode = bool(cmd_value.toInt()); storage_write(); }
    if ( command == "SSID") strcpy(config.SSID, cmd_value.c_str());
    if ( command == "WiFiKey") strcpy(config.WiFiKey, cmd_value.c_str());
    if ( command == "NTPServerName") strcpy(config.NTPServerName, cmd_value.c_str());
    if ( command == "MQTT_Server") { strcpy(config.MQTT_Server, cmd_value.c_str()); storage_write(); }
    if ( command == "MQTT_Port") { config.MQTT_Port = (long)cmd_value.toInt();storage_write(); }
    if ( command == "MQTT_Secure") { config.MQTT_Secure = bool(cmd_value.toInt()); storage_write(); }
    if ( command == "MQTT_User") { strcpy(config.MQTT_User, cmd_value.c_str()); storage_write(); }
    if ( command == "MQTT_Password") { strcpy(config.MQTT_Password, cmd_value.c_str()); storage_write(); }   
    if ( command == "Update_Time_Via_NTP_Every") config.Update_Time_Via_NTP_Every = (ulong)abs(atol(cmd_value.c_str()));
    if ( command == "TimeZone") config.TimeZone = (long)cmd_value.toInt();
    if ( command == "isDayLightSaving") config.isDayLightSaving = bool(cmd_value.toInt());
    if ( command == "Store") if (bool(cmd_value.toInt()) == true) storage_write();
    if ( command == "Boot")  if (bool(cmd_value.toInt()) == true) {mqtt_publish(mqtt_pathconf, "Boot", "", true); mqtt_restart();}
    if ( command == "Reset") if (bool(cmd_value.toInt()) == true) {mqtt_publish(mqtt_pathconf, "Reset", "", true); mqtt_reset();}
    if ( command == "Switch_Def") { 
            config.SWITCH_Default = bool(cmd_value.toInt());
            storage_write();
            mqtt_publish(mqtt_pathtele, "Switch", String(SWITCH));
       }
    if ( command == "Temp_Corr") { 
            config.Temp_Corr = cmd_value.toFloat();
            storage_write();
            mqtt_publish(mqtt_pathtele, "Temperatura", String(getTemperature()));
       }
    if ( command == "LDO_Corr") { 
            config.LDO_Corr = cmd_value.toFloat();
            storage_write();
            mqtt_publish(mqtt_pathtele, "Battery", String(getBattLevel(),0));
       }

    // Standard Actuators/Actions 
    if ( command == "Level") LEVEL = (uint)abs(cmd_value.toInt());
    if ( command == "Position") POSITION = cmd_value.toInt();
    if ( command == "Switch") { SWITCH = bool(cmd_value.toInt()); mqtt_publish(mqtt_pathtele, "Switch", String(SWITCH));}
    if ( command == "Timer") TIMER = (ulong)abs(atol(cmd_value.c_str()));
    if ( command == "Counter") COUNTER = (ulong)abs(atol(cmd_value.c_str()));
    if ( command == "Calibrate") { CALIBRATE = cmd_value.toFloat(); }

    mqtt_custom(command, cmd_value);

    storage_print();
}


// The callback to handle the MQTT PUBLISH messages received from Broker.
void mqtt_setcallback() {
    MQTTclient.setCallback(on_message);
}


// MQTT commands to run on setup function.
void mqtt_setup() {
    //mqtt_pathtele = "/" + String(config.ClientID) + "/" + String(config.Location) + "/" + String(config.DeviceName) + "/telemetry/";
    //mqtt_pathconf = "/" + String(config.ClientID) + "/" + String(config.Location) + "/" + String(config.DeviceName) + "/configure/";
    mqtt_pathtele = String(config.ClientID) + "/" + String(ChipID) + "/" + String(config.DeviceName) + "/inform/";
    mqtt_pathconf = String(config.ClientID) + "/" + String(ChipID) + "/" + String(config.DeviceName) + "/command/";
    
    hassio_device();

    mqtt_connect();
    if (MQTT_state == MQTT_CONNECTED) {
        if (ESPWakeUpReason() == "Deep-Sleep Wake") {
            mqtt_publish(mqtt_pathtele, "Status", "WakeUp");
            mqtt_publish(mqtt_pathtele, "Battery", String(getBattLevel(),0));
        }
        else {
            config_entity("sensor","none", "Status");
            if (BattPowered) config_entity("sensor","Battery");
            else delete_entity("sensor","Battery");
            config_entity("switch","LED");
            config_entity("sensor","signal_strength", "RSSI");
            custo_hassio();

            mqtt_publish(mqtt_pathtele, "Boot", ESPWakeUpReason());
            //mqtt_publish(mqtt_pathtele, "Brand", BRANDName);
            //mqtt_publish(mqtt_pathtele, "Model", MODELName);
            //mqtt_publish(mqtt_pathtele, "ChipID", ChipID);
            //mqtt_publish(mqtt_pathtele, "SWVer", SWVer);
            status_report();
            mqtt_publish(mqtt_pathtele, "IP", WiFi.localIP().toString());
            mqtt_publish(mqtt_pathtele, "LED", String(config.LED));
        }
        mqtt_publish(mqtt_pathtele, "RSSI", String(getRSSI()));
        mqtt_dump_data(mqtt_pathtele, "Telemetry");
    }
    mqtt_setcallback();
}


// MQTT commands to run on loop function.
void mqtt_loop() {
    if (!MQTTclient.loop()) {
        if ( millis() - MQTT_LastTime > (MQTT_Retry * 1000)) {
            MQTT_errors ++;
            Serial.println( "in loop function MQTT ERROR! #: " + String(MQTT_errors) + "  ==> " + MQTT_state_string(MQTTclient.state()) );
            MQTT_LastTime = millis();
            mqtt_connect();
            if(MQTT_state == MQTT_CONNECTED ) {
                status_report();
                mqtt_dump_data(mqtt_pathtele, "Telemetry");
            }
        }
    }
    yield();
}