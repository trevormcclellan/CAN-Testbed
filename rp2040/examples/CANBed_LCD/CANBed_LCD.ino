#include <SPI.h>
#include <mcp_canbus.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <LiquidCrystal.h>

#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

// LCD pin connections: RS, E, D4, D5, D6, D7
LiquidCrystal lcd(10, 19, 21, 22, 23, 24);

const unsigned long CAN_ID_SAS11 = 0x2B0;  // CAN ID for SAS11 (688 decimal)
const float ANGLE_SCALE_FACTOR = 0.1;
const int MAX_ANGLE = 45; // Max visualized steering angle

// Custom characters for the bar display
byte borderMiddle[8] = {0b11111, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b11111};
byte borderLeft[8]   = {0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
byte borderRight[8]  = {0b11111, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b00001, 0b11111};
byte barFill[8]      = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};

int led = 18;
String deviceName;
const int MAX_HW_FILTERS = 6; // MCP2515 supports 6 filters
int selectedIds[20];          // Example size for selected IDs (can hold up to 20)
int numSelectedIds = 0;
int ignoredIds[20];           // Example size for ignored IDs (can hold up to 20)
int numIgnoredIds = 0;

// Replay configuration
String replayId;
String replayData;
int replayRepeat = 0;
int replayInterval = 0;
int replayStartTime = 0;
bool replayConfigured = false;
bool replayTriggered = false;
unsigned long firstCanMessageTime = 0;

// Function to read device name from EEPROM
void readDeviceName()
{
  char name[100];
  int i = 0;

  while (true)
  {
    char c = EEPROM.read(i);
    if (c == '\0' || i >= sizeof(name) - 1)
      break;
    name[i] = c;
    i++;
  }
  name[i] = '\0';

  deviceName = (i == 0 || name[0] == '\0') ? "Unknown" : String(name);
  Serial.print("Read device name from EEPROM: ");
  Serial.println(deviceName);
}

// Function to write device name to EEPROM
void writeDeviceName(const String &name)
{
  Serial.print("Writing device name to EEPROM: ");
  Serial.println(name);

  for (int i = 0; i < 100; i++)
  {
    EEPROM.write(i, 0);
  }

  for (int i = 0; i < name.length() && i < 99; i++)
  {
    EEPROM.write(i, name[i]);
  }
  EEPROM.write(name.length(), '\0');
  EEPROM.commit();
}

// Function to process the upload command with JSON data
void handleUpload(String command)
{
  command.replace("upload:", "");
  command.replace(":end", "");

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, command);

  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  JsonArray ids = obj["ids"];
  int mask = obj["mask"]; // Expecting mask as an integer

  numSelectedIds = ids.size();

  for (int i = 0; i < numSelectedIds; i++)
  {
    // Convert each hex string ID to an integer
    const char *hexIdStr = ids[i].as<const char *>();
    selectedIds[i] = strtol(hexIdStr, NULL, 16); // Convert hex string to integer
  }

  Serial.println("Uploaded selected CAN IDs and mask:");
  for (int i = 0; i < numSelectedIds; i++)
  {
    Serial.println(selectedIds[i], HEX);
  }
  Serial.print("Mask: 0x");
  Serial.println(mask, HEX);

  if (numSelectedIds <= MAX_HW_FILTERS)
  {
    setupHardwareFilters(mask);
  }
  else
  {
    Serial.println("Using software filtering due to more than 6 IDs.");
  }
}

void handleReplayAttack(String command)
{
  command.replace("replay:", "");
  command.replace(":endreplay", "");

  int firstColon = command.indexOf(':');
  int secondColon = command.indexOf(':', firstColon + 1);
  int thirdColon = command.indexOf(':', secondColon + 1);

  if (firstColon == -1 || secondColon == -1 || thirdColon == -1)
  {
    Serial.println("Invalid replay command format.");
    return;
  }

  String message = command.substring(0, firstColon);
  replayId = message.substring(0, message.indexOf('#'));
  replayData = message.substring(message.indexOf('#') + 1);
  replayRepeat = command.substring(firstColon + 1, secondColon).toInt();
  replayInterval = command.substring(secondColon + 1, thirdColon).toInt();
  replayStartTime = command.substring(thirdColon + 1, command.length()).toInt();
  replayConfigured = true;
  replayTriggered = false;

  Serial.println("Replay configuration set:");
  Serial.println("ID: " + replayId);
  Serial.println("Data: " + replayData);
  Serial.println("Repeat: " + String(replayRepeat));
  Serial.println("Interval: " + String(replayInterval) + " ms");
  Serial.println("Start Time: " + String(replayStartTime) + " ms");
}

void executeReplay()
{
  Serial.println("Starting replay...");
  unsigned char buf[8];
  int messageLen = replayData.length() / 2; // Each byte is two hex chars

  // Parse the replay data into the buffer
  for (int i = 0; i < messageLen && i < 8; i++)
  {
    String byteStr = replayData.substring(i * 2, i * 2 + 2);
    buf[i] = strtol(byteStr.c_str(), NULL, 16);
  }

  unsigned long replayIdValue = strtol(replayId.c_str(), NULL, 16);

  for (int i = 0; i < replayRepeat; i++)
  {
    // Send CAN message
    if (CAN.sendMsgBuf(replayIdValue, 0, messageLen, buf) == CAN_OK)
    {
      Serial.println("Replay message sent.");
    }
    else
    {
      Serial.println("Error sending replay message.");
    }

    delay(replayInterval);
  }
  Serial.println("Replay complete.");
}

// Function to set hardware filters
void setupHardwareFilters(unsigned long mask)
{
  Serial.println("Setting up hardware filters:");
  CAN.init_Mask(0, 0, mask); // Set both masks to the same value
  CAN.init_Mask(1, 0, mask);

  for (int i = 0; i < 6; i++)
  {
    if (i < numSelectedIds)
    {
      CAN.init_Filt(i, 0, selectedIds[i]);
      Serial.print("Hardware filter set for ID: 0x");
      Serial.println(selectedIds[i], HEX);
    }
    else
    {
      CAN.init_Filt(i, 0, 0x00); // Initialize with dummy values if no ID
    }
  }
}

void handleCommand(String command)
{
  command.trim();

  if (command.equalsIgnoreCase("identify"))
  {
    for (int i = 0; i < 6; i++)
    {
      digitalWrite(led, HIGH);
      delay(250);
      digitalWrite(led, LOW);
      delay(250);
    }
  }
  else if (command.startsWith("set_name"))
  {
    int index = command.indexOf(' ');
    if (index > 0)
    {
      String newName = command.substring(index + 1);
      writeDeviceName(newName);
      deviceName = newName;
      Serial.println("Name set to: " + deviceName);
    }
    else
    {
      Serial.println("Invalid set_name command");
    }
  }
  else if (command.equalsIgnoreCase("get_name"))
  {
    Serial.println("Device name: " + deviceName);
  }
  else if (command.startsWith("upload:"))
  {
    handleUpload(command);
  }
  else if (command.startsWith("replay:"))
  {
    handleReplayAttack(command);
  }
  else if (command.startsWith("ignore:"))
  {
    command.replace("ignore:", "");
    command.replace(":end", "");

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, command);

    if (error)
    {
      Serial.print("Failed to parse JSON: ");
      Serial.println(error.c_str());
      return;
    }

    JsonArray arr = doc.as<JsonArray>();
    numIgnoredIds = arr.size();

    for (int i = 0; i < numIgnoredIds; i++)
    {
      // Convert each hex string ID to an integer
      const char *hexIdStr = arr[i].as<const char *>();
      ignoredIds[i] = strtol(hexIdStr, NULL, 16); // Convert hex string to integer
    }
    Serial.println("Uploaded and set ignored CAN IDs:");
    for (int i = 0; i < numIgnoredIds; i++)
    {
      Serial.println(ignoredIds[i], HEX);
    }
  }
  else
  {
    Serial.println("Unknown command: " + command);
  }
}

void transformData(unsigned char *transformedData, unsigned char *data, int dataLength)
{
  // Add code here to transform the data before sending
  for (int i = 0; i < dataLength; i++)
  {
    transformedData[i] = data[i];
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  EEPROM.begin(512);
  readDeviceName();

  // Initialize LCD and load custom characters
  lcd.begin(16, 2);
  lcd.createChar(0, borderLeft);
  lcd.createChar(1, borderRight);
  lcd.createChar(2, borderMiddle);
  lcd.createChar(3, barFill);

  lcd.setCursor(0, 0);
  lcd.print(" Steering Angle");
  lcd.setCursor(0, 1);
  lcd.write(byte(0));

  for (uint8_t i = 1; i < 15; i++) {
    lcd.setCursor(i, 1);
    lcd.write(byte(2));
  }
  lcd.setCursor(15, 1);
  lcd.write(byte(1));

  if (CAN.begin(CAN_500KBPS) != CAN_OK)
  {
    Serial.println("CAN BUS FAIL!");
    while (1)
      ;
  }
  Serial.println("CAN BUS OK!");

  CAN.init_Mask(0, 0, 0x0);
  CAN.init_Mask(1, 0, 0x0);

  for (int i = 0; i < 6; i++)
  {
    digitalWrite(led, HIGH);
    delay(250);
    digitalWrite(led, LOW);
    delay(250);
  }
}

void loop()
{
  if (Serial.available() > 0)
  {
    String command = Serial.readStringUntil('\n');
    handleCommand(command);
  }

  if (CAN_MSGAVAIL == CAN.checkReceive())
  {
    unsigned char len = 0;
    unsigned char buf[8];
    CAN.readMsgBuf(&len, buf);
    unsigned long canId = CAN.getCanId();

    // Transform the data before processing
    unsigned char transformedData[8];
    transformData(transformedData, buf, len);

    // Check if ID is in the ignored list
    for (int i = 0; i < numIgnoredIds; i++)
    {
      if (canId == ignoredIds[i])
        return; // Ignore if ID is in the ignored list
    }

    // Check CAN ID against selected IDs
    if (numSelectedIds > MAX_HW_FILTERS)
    {
      // Software filtering
      bool idMatch = false;
      for (int i = 0; i < numSelectedIds; i++)
      {
        if (canId == selectedIds[i])
        {
          idMatch = true;
          break;
        }
      }
      if (!idMatch)
        return; // Ignore if ID doesn't match any in the array
    }

    unsigned long recvTime = millis();

    // If using hardware filters or ID matches, process the message
    Serial.print("recv:");
    Serial.print(canId, HEX);
    Serial.print("#"); // Add the '#' separator
    for (int i = 0; i < len; i++)
    {
      if (transformedData[i] < 0x10)
        Serial.print("0");                   // Zero-pad single-digit hex values
      Serial.print(transformedData[i], HEX); // Print the data in hexadecimal
    }
    Serial.print("#");
    Serial.print(recvTime);
    Serial.println(":endrecv");

    if (canId == CAN_ID_SAS11) {
      int16_t rawAngle = ((int16_t)buf[1] << 8) | buf[0];  // Extract 16-bit signed steering angle
      float steeringAngle = rawAngle * ANGLE_SCALE_FACTOR;

      // Display numeric angle value
      lcd.setCursor(0, 0);
      lcd.print(" Angle: ");
      lcd.print(steeringAngle, 1);
      lcd.print("   ");

      // Clear bar and update visualization
      int filledBlocks = map(abs(steeringAngle), 0, MAX_ANGLE, 0, 7);

      if (steeringAngle > 0) {
        for (int i = 8; i <= 15; i++) {
          lcd.setCursor(i, 1);
          lcd.write((i <= 7 + filledBlocks) ? byte(3) : ((i == 15) ? byte(1) : byte(2)));
        }
      } else if (steeringAngle < 0) {
        for (int i = 7; i >= 0; i--) {
          lcd.setCursor(i, 1);
          lcd.write((i >= 8 - filledBlocks) ? byte(3) : ((i == 0) ? byte(0) : byte(2)));
        }
      }
    }
    // Handle replay logic
    if (replayConfigured && !replayTriggered)
    {
      replayTriggered = true;
      firstCanMessageTime = millis();
    }

    if (replayTriggered && (millis() - firstCanMessageTime >= replayStartTime))
    {
      replayTriggered = false; // Reset after replay
      replayConfigured = false;
      executeReplay();
    }
  }
}
