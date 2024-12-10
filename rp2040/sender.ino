#include <SPI.h>
#include <mcp_canbus.h>

#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

void setup() {
  Serial.begin(115200);

  // Initialize CAN bus at 500kbps
  if (CAN.begin(CAN_500KBPS) != CAN_OK) {
    Serial.println("CAN BUS FAIL!");
    while (1);
  }
  Serial.println("CAN BUS OK!");
}

void loop() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n'); // Read the entire message
    input.trim(); // Remove extra spaces or newlines
    
    if (input.length() > 0) {
      processSerialMessage(input); // Parse and send CAN message
    }
  }
}

void processSerialMessage(String message) {
  message.trim();

  // Split the input using the '#' character to separate CAN ID and data bytes
  int separatorIndex = message.indexOf('#');
  
  if (separatorIndex == -1) {
    Serial.println("Invalid message format. Use: <CAN_ID>#<DATA_BYTES>");
    return;
  }

  // Extract CAN ID part
  String canIdStr = message.substring(0, separatorIndex);
  unsigned long canId = strtol(canIdStr.c_str(), NULL, 16); // Convert CAN ID from hex string to unsigned long
  
  // Extract data bytes part
  String dataBytesStr = message.substring(separatorIndex + 1);
  unsigned char data[8];
  int dataLength = parseDataBytes(dataBytesStr, data); // Parse data bytes into the array
  
  if (dataLength > 0) {
    // Send the CAN message
    byte sndStat = CAN.sendMsgBuf(canId, 0, dataLength, data);
    
    if (sndStat == CAN_OK) {
      Serial.println("Message sent successfully.");
    } else {
      Serial.println("Error sending message.");
    }
  }
}

// Helper function to parse data bytes from the 16-character hex string
int parseDataBytes(String dataBytesStr, unsigned char* data) {
  int dataLength = 0;

  // Ensure the data string is the right length for 8 bytes (16 hex characters)
  if (dataBytesStr.length() != 16) {
    Serial.println("Invalid data byte length. Expected 16 characters.");
    return 0;
  }
  
  // Parse each pair of hex characters into a byte
  for (int i = 0; i < 8; i++) {
    String byteStr = dataBytesStr.substring(i * 2, i * 2 + 2); // Get two characters at a time
    data[i] = (unsigned char) strtol(byteStr.c_str(), NULL, 16); // Convert hex string to byte
    dataLength++;
  }

  return dataLength; // Return the number of bytes parsed
}
