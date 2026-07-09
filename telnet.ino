WiFiServer telnetServer(23);
WiFiClient telnetClient;
HardwareSerial HardwareDebugSerial(0);
TelnetDebugConsole DebugSerial;

static bool telnetStarted = false;

void TelnetDebugConsole::begin(unsigned long baud) {
  HardwareDebugSerial.begin(baud);
}

int TelnetDebugConsole::available() {
  return HardwareDebugSerial.available();
}

int TelnetDebugConsole::read() {
  return HardwareDebugSerial.read();
}

int TelnetDebugConsole::peek() {
  return HardwareDebugSerial.peek();
}

void TelnetDebugConsole::flush() {
  HardwareDebugSerial.flush();
}

size_t TelnetDebugConsole::write(uint8_t data) {
  HardwareDebugSerial.write(data);
  if (WiFi.status() == WL_CONNECTED && telnetClient && telnetClient.connected()) {
    telnetClient.write(data);
  }
  return 1;
}

size_t TelnetDebugConsole::write(const uint8_t *buffer, size_t size) {
  HardwareDebugSerial.write(buffer, size);
  if (WiFi.status() == WL_CONNECTED && telnetClient && telnetClient.connected()) {
    telnetClient.write(buffer, size);
  }
  return size;
}

void TelnetInit() {
  if (telnetStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }

  telnetServer.begin();
  telnetServer.setNoDelay(true);
  telnetStarted = true;

  Serial.print("Telnet ready: ");
  Serial.print(WiFi.localIP());
  Serial.println(":23");
}

void TelnetRun() {
  if (WiFi.status() != WL_CONNECTED) {
    if (telnetClient) {
      telnetClient.stop();
    }
    return;
  }

  if (!telnetStarted) {
    TelnetInit();
    return;
  }

  if (telnetServer.hasClient()) {
    WiFiClient newClient = telnetServer.available();

    if (telnetClient && telnetClient.connected()) {
      newClient.println("Telnet already connected.");
      newClient.stop();
      return;
    }

    telnetClient = newClient;
    telnetClient.setNoDelay(true);
    Serial.println("Telnet client connected");
  }

  if (telnetClient && !telnetClient.connected()) {
    telnetClient.stop();
    Serial.println("Telnet client disconnected");
  }

  while (telnetClient && telnetClient.connected() && telnetClient.available()) {
    HardwareDebugSerial.write(telnetClient.read());
  }
}
