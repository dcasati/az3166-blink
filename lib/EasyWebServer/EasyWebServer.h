#ifndef EASY_WEB_SERVER_H
#define EASY_WEB_SERVER_H

#include <Arduino.h>
#include "AZ3166WiFi.h"
#include "EasyDisplay.h"

/**
 * EasyWebServer - Control your board from a web browser!
 * 
 * Your son can control the LED colors and display messages
 * from any phone, tablet, or computer on the same WiFi network.
 */
class EasyWebServer {
public:
    /**
     * Start the web server
     * @param port - Port number (default: 80)
     */
    static void begin(int port = 80);

    /**
     * Check for web browser requests (call this in loop!)
     */
    static void handleRequests();

    /**
     * Get the web page URL
     * @return URL like "http://192.168.1.100"
     */
    static String getURL();

    /**
     * Show the URL on the display
     */
    static void showURL();

    /**
     * Is server running?
     */
    static bool isRunning();

    /**
     * Stop the server
     */
    static void stop();

private:
    static WiFiServer server;
    static bool running;
    static void handleClient(WiFiClient& client);
    static String getWebPage();
    static void processCommand(const String& command, const String& value);
};

#endif // EASY_WEB_SERVER_H
