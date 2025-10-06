#include "EasyWebServer.h"

// Static member initialization
WiFiServer EasyWebServer::server(80);
bool EasyWebServer::running = false;

void EasyWebServer::begin(int port) {
    server = WiFiServer(port);
    server.begin();
    running = true;
    
    Serial.println("[EasyWebServer] Web server started!");
    Serial.print("[EasyWebServer] Go to: http://");
    Serial.println(WiFi.localIP());
    
    showURL();
}

void EasyWebServer::handleRequests() {
    if (!running) return;
    
    WiFiClient client = server.available();
    if (client) {
        Serial.println("[EasyWebServer] New visitor!");
        handleClient(client);
    }
}

String EasyWebServer::getURL() {
    IPAddress ip = WiFi.localIP();
    char buffer[20];
    sprintf(buffer, "http://%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return String(buffer);
}

void EasyWebServer::showURL() {
    IPAddress ip = WiFi.localIP();
    char buffer[20];
    sprintf(buffer, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    EasyDisplay::write(4, buffer);
}

bool EasyWebServer::isRunning() {
    return running;
}

void EasyWebServer::stop() {
    running = false;
    Serial.println("[EasyWebServer] Server stopped");
}

void EasyWebServer::handleClient(WiFiClient& client) {
    String request = "";
    unsigned long timeout = millis() + 1000; // 1 second timeout
    
    // Read the HTTP request with timeout protection
    while (client.connected() && millis() < timeout) {
        if (client.available()) {
            char c = client.read();
            request += c;
            
            // Check if we got the end of the request
            if (request.endsWith("\r\n\r\n") || request.endsWith("\n\n")) {
                break;
            }
            
            // Prevent buffer overflow
            if (request.length() > 512) {
                break;
            }
        }
        delay(1); // Small delay to prevent tight loop
    }
    
    // Process commands from the request
    if (request.indexOf("GET /?") >= 0) {
        // Extract command and value
        int cmdStart = request.indexOf("cmd=") + 4;
        int cmdEnd = request.indexOf("&", cmdStart);
        if (cmdEnd == -1) cmdEnd = request.indexOf(" ", cmdStart);
        
        String command = request.substring(cmdStart, cmdEnd);
        
        int valStart = request.indexOf("val=") + 4;
        int valEnd = request.indexOf(" ", valStart);
        String value = request.substring(valStart, valEnd);
        
        // Decode URL encoding (replace + with space, %20 with space)
        value.replace("+", " ");
        value.replace("%20", " ");
        
        Serial.print("[EasyWebServer] Command: ");
        Serial.print(command);
        Serial.print(", Value: ");
        Serial.println(value);
        
        processCommand(command, value);
    }
    
    // Send the web page - send directly to save memory
    client.print("HTTP/1.1 200 OK\r\n");
    client.print("Content-Type: text/html; charset=UTF-8\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(getWebPage());
    
    delay(10); // Give time for data to be sent
    client.stop();
    
    Serial.println("[EasyWebServer] Visitor left!");
}

void EasyWebServer::processCommand(const String& command, const String& value) {
    if (command == "led") {
        Serial.print("[EasyWebServer] Setting LED to: ");
        Serial.println(value);
        EasyDisplay::setLED(value.c_str());
    }
    else if (command == "msg1") {
        Serial.print("[EasyWebServer] Line 1: ");
        Serial.println(value);
        EasyDisplay::write(1, value.c_str());
    }
    else if (command == "msg2") {
        Serial.print("[EasyWebServer] Line 2: ");
        Serial.println(value);
        EasyDisplay::write(2, value.c_str());
    }
    else if (command == "msg3") {
        Serial.print("[EasyWebServer] Line 3: ");
        Serial.println(value);
        EasyDisplay::write(3, value.c_str());
    }
    else if (command == "rainbow") {
        Serial.println("[EasyWebServer] Rainbow effect!");
        EasyDisplay::rainbow();
    }
    else if (command == "blink") {
        Serial.print("[EasyWebServer] Blinking: ");
        Serial.println(value);
        EasyDisplay::blink(value.c_str(), 3);
    }
    else if (command == "clear") {
        Serial.println("[EasyWebServer] Clearing display!");
        EasyDisplay::clear();
    }
}

String EasyWebServer::getWebPage() {
    String page = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AZ3166 Control</title>
<style>
body{font-family:Arial;max-width:600px;margin:20px auto;padding:20px;background:#667eea;color:#fff}
h1{text-align:center;font-size:24px}
.section{background:rgba(255,255,255,0.1);border-radius:10px;padding:15px;margin:15px 0}
.section h2{margin-top:0;font-size:18px;border-bottom:2px solid rgba(255,255,255,0.3);padding-bottom:8px}
.color-btn{display:inline-block;width:60px;height:60px;margin:4px;border:2px solid #fff;border-radius:50%;cursor:pointer}
input[type="text"]{width:calc(100% - 20px);padding:10px;margin:8px 0;border-radius:6px;border:none;font-size:14px}
.btn{background:#fff;color:#667eea;border:none;padding:10px 20px;border-radius:6px;font-size:14px;font-weight:bold;cursor:pointer;margin:4px}
.fun-btn{background:#f093fb;color:#fff}
</style>
</head>
<body>
<h1>AZ3166 Control Panel</h1>
<div class="section">
<h2>LED Colors</h2>
<div style="text-align:center">
<div class="color-btn" style="background:red" onclick="setLED('red')"></div>
<div class="color-btn" style="background:green" onclick="setLED('green')"></div>
<div class="color-btn" style="background:blue" onclick="setLED('blue')"></div>
<div class="color-btn" style="background:yellow" onclick="setLED('yellow')"></div>
<div class="color-btn" style="background:cyan" onclick="setLED('cyan')"></div>
<div class="color-btn" style="background:purple" onclick="setLED('purple')"></div>
<div class="color-btn" style="background:#fff" onclick="setLED('white')"></div>
<div class="color-btn" style="background:#333" onclick="setLED('off')"></div>
</div>
</div>
<div class="section">
<h2>Write Messages</h2>
<input type="text" id="msg1" placeholder="Message for Line 1" maxlength="20">
<button class="btn" onclick="sendMsg(1)">Send to Line 1</button>
<input type="text" id="msg2" placeholder="Message for Line 2" maxlength="20">
<button class="btn" onclick="sendMsg(2)">Send to Line 2</button>
<input type="text" id="msg3" placeholder="Message for Line 3" maxlength="20">
<button class="btn" onclick="sendMsg(3)">Send to Line 3</button>
</div>
<div class="section">
<h2>Fun Effects</h2>
<button class="btn fun-btn" onclick="rainbow()">Rainbow</button>
<button class="btn fun-btn" onclick="blinkLED('red')">Blink Red</button>
<button class="btn fun-btn" onclick="blinkLED('green')">Blink Green</button>
<button class="btn fun-btn" onclick="blinkLED('blue')">Blink Blue</button>
<button class="btn" onclick="clearDisplay()">Clear Display</button>
</div>
<script>
function setLED(c){fetch('/?cmd=led&val='+c)}
function sendMsg(l){var m=document.getElementById('msg'+l).value;fetch('/?cmd=msg'+l+'&val='+encodeURIComponent(m));document.getElementById('msg'+l).value=''}
function rainbow(){fetch('/?cmd=rainbow&val=1')}
function blinkLED(c){fetch('/?cmd=blink&val='+c)}
function clearDisplay(){fetch('/?cmd=clear&val=1')}
</script>
</body>
</html>
)rawliteral";
    
    return page;
}
