#include <SPI.h>
#include <mcp_canbus.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

int led = 18;
String deviceName;

// Global variables for handling chunked uploads
bool isUploading = false;
String uploadBuffer = "";

// Structure to hold CAN message data
struct CANMessage
{
  unsigned long canId;
  unsigned char data[8];
  int dataLength;
  unsigned long timestamp; // Store the timestamp for simulation
};

CANMessage messageBuffer[100]; // Buffer to store up to 100 messages
int numMessages = 0;           // Number of stored messages
unsigned long startTime = 0;   // Store the start time for relative timestamps

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

void handleCommand(String command)
{
  command.trim();
  Serial.println(command);

  if (isUploading)
  {
    // If currently uploading, check for ":end" within the command
    int endIndex = command.indexOf(":end");
    if (endIndex != -1)
    {
      // Add up to the ":end" to the buffer
      uploadBuffer += command.substring(0, endIndex);

      // Process the complete upload
      processSerialMessages(uploadBuffer);
      uploadBuffer = "";   // Clear the buffer
      isUploading = false; // Reset the upload state
      Serial.println("Upload finished");

      // Process any remaining part after ":end"
      String remaining = command.substring(endIndex + 4);
      if (remaining.length() > 0)
      {
        handleCommand(remaining); // Recursively handle the remaining command
      }
    }
    else
    {
      // No ":end", continue accumulating
      uploadBuffer += command;
    }
  }
  else if (command.startsWith("upload:"))
  {
    Serial.println("Starting upload");
    isUploading = true;

    int endIndex = command.indexOf(":end");
    if (endIndex != -1)
    {
      // Handle the case where "upload:" and ":end" are in the same chunk
      uploadBuffer += command.substring(7, endIndex);

      // Process the complete upload
      processSerialMessages(uploadBuffer);
      uploadBuffer = "";   // Clear the buffer
      isUploading = false; // Reset the upload state
      Serial.println("Upload finished");

      // Process any remaining part after ":end"
      String remaining = command.substring(endIndex + 4);
      if (remaining.length() > 0)
      {
        handleCommand(remaining); // Recursively handle the remaining command
      }
    }
    else
    {
      // No ":end", start accumulating
      uploadBuffer += command.substring(7);
    }
  }
  else
  {
    // Handle other commands
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
    else if (command.equalsIgnoreCase("start_simulation"))
    {
      startSimulation(); // Start the simulation when the command is received
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(led, OUTPUT);
  EEPROM.begin(512);
  readDeviceName();

  // Initialize CAN bus at 500kbps
  if (CAN.begin(CAN_500KBPS) != CAN_OK)
  {
    Serial.println("CAN BUS FAIL!");
    while (1)
      ;
  }
  Serial.println("CAN BUS OK!");

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
    String command = Serial.readStringUntil('\n'); // Read the entire message
    handleCommand(command);
  }
}

void processSerialMessages(String input)
{
  Serial.println("Processing messages");
  input.trim();
  Serial.println(input);

  // Create a JsonDocument instead of StaticJsonDocument
  DynamicJsonDocument doc(1024); // Use DynamicJsonDocument for more flexibility

  DeserializationError error = deserializeJson(doc, input);

  if (error)
  {
    Serial.println("Failed to parse input as JSON.");
    return;
  }

  // Store the start time for relative timestamps (first message timestamp)
  if (numMessages == 0)
  {
    startTime = millis();
  }

  // Iterate through the array of messages and parse each one
  for (JsonObject message : doc.as<JsonArray>())
  {
    unsigned long timestamp = message["timestamp"];
    unsigned long canId = strtol(message["id"].as<const char *>(), NULL, 16); // Convert CAN ID from hex string
    String dataStr = message["data"].as<String>();
    unsigned char data[8];
    int dataLength = parseDataBytes(dataStr, data);

    if (dataLength > 0)
    {
      // Store the CAN message in the buffer for later replay
      storeMessage(canId, data, dataLength, timestamp);
    }
  }
}

void storeMessage(unsigned long canId, unsigned char *data, int dataLength, unsigned long timestamp)
{
  Serial.print("Storing message: ");
  Serial.println(canId, HEX);
  for (int i = 0; i < dataLength; i++)
  {
    Serial.print("0x");
    Serial.print(data[i], HEX);
    Serial.print("\t");
  }
  Serial.println();
  if (numMessages < 100)
  {
    messageBuffer[numMessages].canId = canId;
    for (int i = 0; i < 8; i++)
    {
      messageBuffer[numMessages].data[i] = data[i];
    }
    messageBuffer[numMessages].dataLength = dataLength;
    messageBuffer[numMessages].timestamp = timestamp;
    numMessages++;
  }
  else
  {
    Serial.println("Message buffer full, cannot store more messages.");
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

void startSimulation()
{
  if (numMessages == 0)
  {
    Serial.println("No messages to replay.");
    return;
  }

  Serial.println("Starting simulation...");

  // Replay the stored CAN messages at the specified timestamps
  unsigned long lastTimestamp = 0;
  unsigned long simulationStartTime = millis(); // Capture the time the simulation starts

  for (int i = 0; i < numMessages; i++)
  {
    // Calculate the relative wait time based on the difference between the current timestamp and the simulation start time
    unsigned long elapsedTime = millis() - simulationStartTime;
    unsigned long targetTime = messageBuffer[i].timestamp;

    if (elapsedTime >= targetTime)
    {
      // Send the CAN message
      unsigned long canId = messageBuffer[i].canId;
      unsigned char *data = messageBuffer[i].data;
      int dataLength = messageBuffer[i].dataLength;

      unsigned char transformedData[8];
      transformData(transformedData, data, dataLength); // Transform the data before sending

      byte sndStat = CAN.sendMsgBuf(canId, 0, dataLength, transformedData);

      if (sndStat == CAN_OK)
      {
        Serial.print("Replaying message with CAN ID 0x");
        Serial.print(canId, HEX);
        Serial.println(" sent successfully.");
      }
      else
      {
        Serial.println("Error replaying message.");
      }
    }
  }

  Serial.println("Simulation completed.");
}

// Helper function to parse data bytes from the 16-character hex string
int parseDataBytes(String dataBytesStr, unsigned char *data)
{
  int dataLength = dataBytesStr.length() / 2; // Calculate the number of bytes

  if (dataLength > 8)
  {
    Serial.println("Data too long, expected 8 bytes or less.");
    return 0;
  }

  // Parse each pair of hex characters into a byte
  for (int i = 0; i < dataLength; i++)
  {
    String byteStr = dataBytesStr.substring(i * 2, i * 2 + 2);  // Get two characters at a time
    data[i] = (unsigned char)strtol(byteStr.c_str(), NULL, 16); // Convert hex string to byte
  }

  return dataLength; // Return the actual number of bytes parsed
}
