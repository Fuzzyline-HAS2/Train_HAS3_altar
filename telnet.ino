WiFiServer telnetServer(23);
WiFiClient telnetClient;
HardwareSerial HardwareDebugSerial(0);
TelnetDebugConsole DebugSerial;

static bool telnetStarted = false;
static const size_t TELNET_LOG_BUFFER_SIZE = 4096;
static uint8_t telnetLogBuffer[TELNET_LOG_BUFFER_SIZE];
static size_t telnetLogHead = 0;
static size_t telnetLogCount = 0;

static void TelnetRemember(uint8_t data) {
  telnetLogBuffer[telnetLogHead] = data;
  telnetLogHead = (telnetLogHead + 1) % TELNET_LOG_BUFFER_SIZE;
  if (telnetLogCount < TELNET_LOG_BUFFER_SIZE) {
    telnetLogCount++;
  }
}

static void TelnetRemember(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    TelnetRemember(buffer[i]);
  }
}

static void TelnetReplayBufferedLog() {
  if (!telnetClient || !telnetClient.connected() || telnetLogCount == 0) {
    return;
  }

  telnetClient.println();
  telnetClient.println("----- recent serial log -----");

  size_t start = (telnetLogHead + TELNET_LOG_BUFFER_SIZE - telnetLogCount) % TELNET_LOG_BUFFER_SIZE;
  for (size_t i = 0; i < telnetLogCount; ++i) {
    telnetClient.write(telnetLogBuffer[(start + i) % TELNET_LOG_BUFFER_SIZE]);
  }

  telnetClient.println();
  telnetClient.println("----- live log -----");
}

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
  TelnetRemember(data);
  if (WiFi.status() == WL_CONNECTED && telnetClient && telnetClient.connected()) {
    telnetClient.write(data);
  }
  return 1;
}

size_t TelnetDebugConsole::write(const uint8_t *buffer, size_t size) {
  HardwareDebugSerial.write(buffer, size);
  TelnetRemember(buffer, size);
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
    TelnetReplayBufferedLog();
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
