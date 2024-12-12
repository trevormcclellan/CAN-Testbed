#include <SPI.h>
#include <mcp_canbus.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <Crypto.h>
#include <ChaCha.h>

#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

// ChaCha20 key and nonce
byte key[32] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
                0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20};
byte nonce[12] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00};

ChaCha chacha;

int led = 18;
String deviceName;
const int MAX_HW_FILTERS = 6; // MCP2515 supports 6 filters
int selectedIds[20];          // Example size for selected IDs (can hold up to 20)
int numSelectedIds = 0;

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

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, command);

    if (error)
    {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    JsonArray arr = doc.as<JsonArray>();
    numSelectedIds = arr.size();

    for (int i = 0; i < numSelectedIds; i++)
    {
        // Convert each hex string ID to an integer
        const char *hexIdStr = arr[i].as<const char *>();
        selectedIds[i] = strtol(hexIdStr, NULL, 16); // Convert hex string to integer
    }
    Serial.println("Uploaded and set selected CAN IDs:");
    for (int i = 0; i < numSelectedIds; i++)
    {
        Serial.println(selectedIds[i], HEX);
    }

    if (numSelectedIds <= MAX_HW_FILTERS)
    {
        setupHardwareFilters();
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
void setupHardwareFilters()
{
    Serial.println("Setting up hardware filters:");

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
}

void decryptData(unsigned char *decryptedData, unsigned char *data, int dataLength)
{
    chacha.setKey(key, 32);
    chacha.setIV(nonce, 12);
    chacha.decrypt(decryptedData, data, dataLength);
}

void transformData(unsigned char *transformedData, unsigned char *data, int dataLength)
{
    // Add code here to transform the data before processing
    decryptData(transformedData, data, dataLength);
}

void setup()
{
    Serial.begin(115200);
    pinMode(led, OUTPUT);
    EEPROM.begin(512);
    readDeviceName();

    if (CAN.begin(CAN_500KBPS) != CAN_OK)
    {
        Serial.println("CAN BUS FAIL!");
        while (1)
            ;
    }
    Serial.println("CAN BUS OK!");

    CAN.init_Mask(0, 0, 0x3ff);
    CAN.init_Mask(1, 0, 0x3ff);
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

        // If using hardware filters or ID matches, process the message
        Serial.print("recv:");
        Serial.print(canId, HEX);
        Serial.print("#"); // Add the '#' separator
        for (int i = 0; i < len; i++)
        {
            if (transformedData[i] < 0x10)
                Serial.print("0");                 // Zero-pad single-digit hex values
            Serial.print(transformedData[i], HEX); // Print the data in hexadecimal
        }
        Serial.println(":endrecv");

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
