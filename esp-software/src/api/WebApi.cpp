#include "WebApi.h"
#include "../state/LightingState.h"
#include "../wled/WledClient.h"
#include "../teensy/TeensyProtocol.h"
#include "../config/Config.h"
#include "WebUI.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

AsyncEventSource events("/events");
WebApi webApi;
AsyncWebServer server(80);

void WebApi::begin() {
    server.addHandler(&events);

    server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<512> doc;
        JsonObject root = doc.to<JsonObject>();
        globalState.toJson(root);
        serializeJson(doc, *response);
        request->send(response);
    });

    server.on("/api/state", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if(!error) {
            if(globalState.updateFromJson(doc.as<JsonObject>())) {
                events.send("Sending updated state to Teensy via UART", "log");
                teensyProtocol.sendState(globalState);
                wledClient.syncState(globalState);
            }
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\"}");
        }
    });

    server.on("/api/triggerRipple", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        if(!error) {
            uint16_t bush = doc["bush"] | 0;
            int16_t row = doc["row"] | 0;
            int16_t col = doc["col"] | 0;
            
            events.send(String("Sending ripple trigger to Teensy (Row: " + String(row) + ", Col: " + String(col) + ")").c_str(), "log");
            teensyProtocol.sendTrigger(bush, row, col);
            wledClient.triggerRipple();
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\"}");
        }
    });

    server.on("/api/triggerNode", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("id")) {
            uint16_t nodeId = request->getParam("id")->value().toInt();
            events.send(String("Sending manual trigger to Teensy for Node " + String(nodeId)).c_str(), "log");
            teensyProtocol.sendNodeTrigger(nodeId);
            wledClient.triggerRipple();
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"missing id parameter\"}");
        }
    });

    server.on("/api/wled/status", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<256> doc;
        doc["enabled"] = Config::WLED_ENABLED;
        doc["host"] = Config::WLED_HOST;
        doc["connected"] = wledClient.isConnected();
        doc["lastError"] = wledClient.getLastError();
        doc["lastSyncMs"] = wledClient.getLastSyncMs();
        serializeJson(doc, *response);
        request->send(response);
    });

    server.on("/api/map", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"ok\"}");
    }, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        if (index == 0) {
            LittleFS.remove("/graph.json");
            request->_tempFile = LittleFS.open("/graph.json", "w");
        }
        if (request->_tempFile) {
            request->_tempFile.write(data, len);
        }
        if (index + len == total) {
            if (request->_tempFile) {
                request->_tempFile.close();
            }
        }
    });

    server.on("/api/reboot", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"status\":\"rebooting\"}");
        delay(500);
        ESP.restart();
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", WEB_UI_HTML);
    });

    // Serve static files from LittleFS
    server.serveStatic("/", LittleFS, "/");

    server.begin();
}
