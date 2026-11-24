#pragma once

#include "AZ3166WiFi.h"
#include "HttpRequest.h"

bool authorizeAdmin(WiFiClient& client, const Http::Request& request);
bool authorizeMutation(WiFiClient& client, const Http::Request& request);
bool handleOtaRequest(WiFiClient& client, const Http::Request& request);
void pollOta();
bool otaMaintenance();
void pollOperatorSerial();
const char* adminCsrfToken();
bool httpReply(WiFiClient& client, int code, const char* body,
               const char* type = "text/plain");
