#include <SPI.h>
#include <mcp_canbus.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <ArduinoQueue.h>

#define MAX_HW_FILTERS 6 // MCP2515 supports 6 filters
#define LED_PIN 18
#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

#define BUTTON_PIN 21

#define EEPROM_SIZE 128 // Emulated EEPROM Size
#define MAX_NAME_LENGTH 99
char deviceName[MAX_NAME_LENGTH + 1];

unsigned long selectedIds[20]; // Example size for selected IDs (can hold up to 20)
uint8_t numSelectedIds = 0;

unsigned long ignoredIds[20]; // Example size for ignored IDs (can hold up to 20)
uint8_t numIgnoredIds = 0;

// Added for button example
uint8_t buttonState = 0;
uint8_t lastButtonState = 0;
unsigned char hazardsOff[8] = {0x03, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x1C};
unsigned char hazardsOn[8] = {0x03, 0x01, 0x08, 0x00, 0x02, 0x00, 0x00, 0x5C};

// Replay configuration
unsigned long replayId;
unsigned char replayData[8];
uint16_t replayRepeat = 0;
uint32_t replayInterval = 0;
uint32_t replayStartTime = 0;
bool replayConfigured = false;
bool replayTriggered = false;
uint32_t firstCanMessageTime = 0;

// Queue to hold outgoing serial messages
ArduinoQueue<char*> messageQueue(1000);

// Function to read device name from EEPROM
void readDeviceName()
{
  char name[MAX_NAME_LENGTH + 1];
  int i = 0;

  while (true)
  {
    char c = EEPROM.read(i);
    if (c == '\0' || i >= MAX_NAME_LENGTH)
      break;
    name[i] = c;
    i++;
  }
  name[i] = '\0';

  if (i == 0 || name[0] == '\0')
  {
    snprintf(deviceName, sizeof(deviceName), "Unknown");
  }
  else
  {
    strncpy(deviceName, name, MAX_NAME_LENGTH);
  }

  Serial.print("Read device name from EEPROM: ");
  Serial.println(deviceName);
}

// Function to write device name to EEPROM
void writeDeviceName(const char *name)
{
  Serial.print("Writing device name to EEPROM: ");
  Serial.println(name);

  for (int i = 0; i < MAX_NAME_LENGTH; i++)
  {
    EEPROM.write(i, 0);
  }

  for (int i = 0; i < strlen(name) && i < MAX_NAME_LENGTH; i++)
  {
    EEPROM.write(i, name[i]);
  }
  EEPROM.write(strlen(name), '\0');
  EEPROM.commit();
}

// Function to process the upload command with JSON data
void handleUpload(const char *command)
{
  char tempCommand[MAX_NAME_LENGTH + 1];
  strncpy(tempCommand, command, MAX_NAME_LENGTH);

  char *commandPtr = tempCommand;
  const char *uploadPrefix = "upload:";
  if (strncmp(commandPtr, uploadPrefix, strlen(uploadPrefix)) == 0)
  {
    commandPtr += strlen(uploadPrefix);
  }
  else
  {
    Serial.println("Invalid upload command format.");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, commandPtr);

  if (error)
  {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  JsonArray ids = obj["ids"];
  unsigned long mask = obj["mask"];

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

void handleReplayAttack(const char *command)
{
  char tempCommand[MAX_NAME_LENGTH + 1];
  strncpy(tempCommand, command, MAX_NAME_LENGTH);
  tempCommand[MAX_NAME_LENGTH] = '\0';  // Ensure null termination

  char *commandPtr = tempCommand;
  const char *replayPrefix = "replay:";

  if (strncmp(commandPtr, replayPrefix, strlen(replayPrefix)) == 0)
  {
    commandPtr += strlen(replayPrefix);
  }
  else
  {
    Serial.println("Invalid replay command format.");
    return;
  }

  // Find delimiters (`#` and `:`)
  char *idSeparator = strchr(commandPtr, '#');  // # separates ID and DATA
  if (!idSeparator) { Serial.println("Invalid replay format."); return; }
  *idSeparator = '\0';  // Null-terminate ID

  char *firstColon = strchr(idSeparator + 1, ':');
  if (!firstColon) { Serial.println("Invalid replay format."); return; }
  *firstColon = '\0';  // Null-terminate DATA

  char *secondColon = strchr(firstColon + 1, ':');
  if (!secondColon) { Serial.println("Invalid replay format."); return; }
  *secondColon = '\0';  // Null-terminate repeat

  char *thirdColon = strchr(secondColon + 1, ':');
  if (!thirdColon) { Serial.println("Invalid replay format."); return; }
  *thirdColon = '\0';  // Null-terminate interval

  char *fourthColon = strchr(thirdColon + 1, ':');
  if (!fourthColon) { Serial.println("Invalid replay format."); return; }
  *fourthColon = '\0';  // Null-terminate start time

  // Convert replay ID (Hex)
  replayId = strtoul(commandPtr, NULL, 16);

  // Parse replayData (Hex Bytes)
  int messageLen = strlen(idSeparator + 1) / 2;  // Each byte = 2 hex chars
  for (int i = 0; i < messageLen && i < 8; i++)
  {
    char byteStr[3] = {idSeparator[1 + i * 2], idSeparator[2 + i * 2], '\0'};
    replayData[i] = strtol(byteStr, NULL, 16);
  }

  // Parse other parameters
  replayRepeat = atoi(firstColon + 1);
  replayInterval = atoi(secondColon + 1);
  replayStartTime = atoi(thirdColon + 1);
  replayConfigured = true;
  replayTriggered = false;

  // Debugging output
  Serial.println("Replay configuration set:");
  Serial.print("ID: ");
  Serial.println(replayId, HEX);  // Print replayId as hexadecimal
  Serial.print("Data: ");
  for (int i = 0; i < messageLen; i++)  // Print each byte of replayData
  {
    Serial.print(replayData[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  Serial.print("Repeat: ");
  Serial.println(replayRepeat);
  Serial.print("Interval: ");
  Serial.println(replayInterval);
  Serial.print("Start Time: ");
  Serial.println(replayStartTime);
}

void executeReplay()
{
  Serial.println("Starting replay...");

  for (int i = 0; i < replayRepeat; i++)
  {
    if (CAN.sendMsgBuf(replayId, 0, 8, replayData) == CAN_OK)
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

void handleCommand(const char *command)
{
  if (strcmp(command, "identify") == 0)
  {
    for (int i = 0; i < 6; i++)
    {
      digitalWrite(LED_PIN, HIGH);
      delay(250);
      digitalWrite(LED_PIN, LOW);
      delay(250);
    }
  }
  else if (strncmp(command, "set_name", 8) == 0)
  {
    const char *newName = command + 9; // Skip 'set_name '
    writeDeviceName(newName);
    strncpy(deviceName, newName, MAX_NAME_LENGTH);
    Serial.print("Name set to: ");
    Serial.println(deviceName);
  }
  else if (strcmp(command, "get_name") == 0)
  {
    Serial.print("Device name: ");
    Serial.println(deviceName);
  }
  else if (strncmp(command, "upload:", 7) == 0)
  {
    handleUpload(command);
  }
  else if (strcmp(command, "sync") == 0)
  {
    Serial.print("sync:");
    Serial.print(millis());
    Serial.println(":endsync");
  }
  else if (strncmp(command, "replay:", 7) == 0)
  {
    handleReplayAttack(command);
  }
  else if (strncmp(command, "ignore:", 7) == 0)
  {
    char tempCommand[MAX_NAME_LENGTH + 1];
    strncpy(tempCommand, command, MAX_NAME_LENGTH);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, tempCommand + 7);

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
  }
  else
  {
    Serial.println("Unknown command.");
  }
}

void transformData(unsigned char *transformedData, unsigned char *data, int dataLength)
{
  // Add code here to transform the data before processing
  for (int i = 0; i < dataLength; i++)
  {
    transformedData[i] = data[i];
  }
}

void enqueueMessage(const char* message)
{
  // Check if message queue is full before enqueuing
  if (!messageQueue.isFull())
  {
    char* copy = strdup(message); // Allocate memory and copy string
    messageQueue.enqueue(copy);
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);  // Turn on LED if queue is full
  }
}

void processSerialQueue()
{
  // Non-blocking serial output
  while (!messageQueue.isEmpty())
  {
    char* message = messageQueue.getHead();  // Retrieve the message from the queue

    if (message != nullptr)
    {
      // Check if there's enough space in the serial buffer to send the message
      if (Serial.availableForWrite() >= strlen(message) + 2)
      {
        Serial.println(message);  // Print the message to serial
        messageQueue.dequeue();  // Remove the message from the queue
        free(message);
      }
      else
      {
        break;  // Stop if there's not enough space in the serial buffer
      }
    }
  }
}


void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  EEPROM.begin(EEPROM_SIZE);
  readDeviceName();

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
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
    delay(250);
  }
}

void flashHazards()
{
  unsigned long startTime = millis();
  while (millis() - startTime < 5000)
  { // Run for 5 seconds
    // Send message 1 five times (every 40 ms)
    for (int i = 0; i < 5; i++)
    {
      CAN.sendMsgBuf(0x541, 0, 8, hazardsOff);
      delay(60);
    }

    // Send message 2 six times (every 40 ms)
    for (int i = 0; i < 6; i++)
    {
      CAN.sendMsgBuf(0x541, 0, 8, hazardsOn);
      delay(60);
    }
  }
}

void loop()
{
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH)
  {
    flashHazards();
  }

  lastButtonState = buttonState;

  if (Serial.available() > 0)
  {
    char command[256];
    int i = 0;
    while (Serial.available() > 0 && i < sizeof(command) - 1)
    {
      char c = Serial.read();
      if (c == '\n' || c == '\r')
        break;

      command[i++] = c;
    }
    command[i] = '\0';  // Null-terminate the string
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
    char msg[64];
    snprintf(msg, sizeof(msg), "%03lX#", canId);

    for (int i = 0; i < len; i++)
    {
      char hexBuf[3];  // Buffer to store the hex string for a byte
      snprintf(hexBuf, sizeof(hexBuf), "%02X", transformedData[i]);
      strncat(msg, hexBuf, sizeof(msg) - strlen(msg) - 1);
    }

    // Append the receive time
    snprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), "#%lu", recvTime);

    // Convert to uppercase
    for (int i = 0; msg[i] != '\0'; i++)
    {
      msg[i] = toupper(msg[i]);
    }

    // Enqueue message
    char finalMsg[64];  // Final buffer for "recv:" and ":endrecv"
    snprintf(finalMsg, sizeof(finalMsg), "recv:%s:endrecv", msg);
    enqueueMessage(finalMsg);

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
  
  processSerialQueue();
}