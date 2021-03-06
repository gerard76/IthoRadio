#include <Arduino.h>
#include "IthoSender.h"
#include "MqttCom.h"
#include "ESP8266WiFi.h"
#include <PubSubClient.h>
#include <WiFiManager.h>

#include <IthoReceive.h>
#include "StringSupport.h"

WiFiClient espClient;

void setupWifi(bool);


//print any message received for subscribed topic
void callback(char *topic, byte *payload, unsigned int length)
{
    payload[length] = 0;
    //printf("mqtt msg arrived: %s(%d) -> %s\n", topic, length, payload);

    // todo: check incoming topic
    String sPayload = String((char *)payload);
    String sTopic = String((char *)topic);

    String msg;
    msg += "mqtt msg arrived: ";
    msg += sTopic;
    msg += " -> ";
    msg += sPayload;
    MqttCom.logger(msg);

    String fw = sub(sTopic, '/', 0);
    //MqttCom.logger(String("firstword=")+fw);
    if (fw == "itho") {
        // new style command
        String command = sub(sTopic, '/', 2);
        if (command == "info") {
            MqttCom.logInfo();
            return;
        }
        if (command == "send") {
            String remote = sub(sPayload, '/', 0);
            String remoteCommand = sub(sPayload, '/', 1);
            MqttCom.logger(String("send: " + remote + " -> " + remoteCommand));
            IthoSender.sendCommand("mqtt", remote, remoteCommand);
            return;
        }
        MqttCom.logger("Itho responder, unknown command");
        return;
    }
    String localCommand = sTopic.substring(sTopic.indexOf('/')+1);
    // MqttCom.logger(String("lcmd/") + localCommand);

    String command = localCommand.substring(0, localCommand.indexOf('/'));
    // MqttCom.logger(String("cmd/") + command);
    if (command == "command") {
        IthoSender.sendCommand("mqtt", sPayload);
    }

    if (command == "info") {
        MqttCom.logInfo();
    }

    if (command == "reset") {
        MqttCom.logger("resetting");
        //setupWifi(true);
        Serial.println("start config panel");
        WiFiManager wifiManager;
        wifiManager.resetSettings();
        delay(3000);
        Serial.println("reset esp");
        ESP.reset();
        delay(3000);-
        //wifiManager.startConfigPortal("OnDemandAP");
        Serial.println("connected...yeey :)");
    }

    if (command == "set") {
        String variable = sPayload.substring(0, sPayload.indexOf('/'));
        String value = sPayload.substring(sPayload.indexOf('/')+1);
        MqttCom.logger(command + " " + sPayload + " " + variable + " = " + value);

        if (variable == "printAllPacket") {
            IthoReceive.printAllPacket = value == "true";
            MqttCom.logger(variable + "=" + IthoReceive.printAllPacket);
        }

        if (variable == "printNonRemote") {
            IthoReceive.printNonRemote = value == "true";
            MqttCom.logger(variable + "=" + IthoReceive.printNonRemote);
        }

    }
}

MqttComClass::MqttComClass(const String &t) : incomingTopic(t)
{
    _client = new PubSubClient(espClient);
}

void MqttComClass::setup()
{
    //connect to MQTT server
    _client->setServer("167.99.32.103", 1883);
    _client->setCallback(callback);

    // force connecting
    loop();
}

void MqttComClass::loop()
{
    // put your main code here, to run repeatedly:
    if (!_client->connected())
    {
        _reconnect();
    }
    if (_client->connected())
        _client->loop();
}

void MqttComClass::publish(const char *c, const char *m)
{
    _client->publish(c, m);
}

void MqttComClass::logInfo()
{
    char buf[20];
    IPAddress ip = WiFi.localIP();
    sprintf(buf, "ip=%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);

    String m = String("connected ip = ") + String(WiFi.localIP());
    logger(buf);
    logger(String("topic=") + incomingTopic);
    logger(String("printNonRemote=") + IthoReceive.printNonRemote);
    logger(String("printAllPacket=") + IthoReceive.printAllPacket);
    logger(String("printDebug=") + IthoReceive.printDebug);
}

void MqttComClass::_reconnect()
{
    // Loop until we're reconnected
    while (!_client->connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect, just a name to identify the client
        if (_client->connect(clientName.c_str(), "itho", "aapnootmies"))
        {
            if (incomingTopic != "")
            {
                _client->subscribe(incomingTopic.c_str(), 0);
                _client->subscribe(incomingTopic2.c_str(), 0);
            }
            logInfo();
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(_client->state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
        printf("connected = %d\n", _client->connected());
    }
}

void MqttComClass::logger(const String& m)
{
    publish((String("itho/log/") + clientName).c_str(), m.c_str());
}

MqttComClass MqttCom("ithoin");
