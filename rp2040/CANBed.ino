#include <EEPROM.h>
#include <ArduinoJson.h>

int led = 18;
String deviceName;

// Function to read the device name from EEPROM
void readDeviceName() {
  char name[100]; // Buffer to read the name
  int i = 0;

  // Read characters from EEPROM until the null terminator is found
  while (true) {
    char c = EEPROM.read(i);
    if (c == '\0' || i >= sizeof(name) - 1) break; // Stop at null terminator or buffer limit
    name[i] = c;
    i++;
  }
  name[i] = '\0'; // Null-terminate the string

  // Check if the name is empty or uninitialized
  if (i == 0 || name[0] == '\0') {
    deviceName = "Unknown"; // Default value
  } else {
    deviceName = String(name);
  }

  Serial.print("Read device name from EEPROM: ");
  Serial.println(deviceName);
}

// Function to write the device name to EEPROM
void writeDeviceName(const String& name) {
  Serial.print("Writing device name to EEPROM: ");
  Serial.println(name);

  // Clear EEPROM space for the name
  for (int i = 0; i < 100; i++) {
    EEPROM.write(i, 0); // Clear previous data
  }

  // Write the new name
  for (int i = 0; i < name.length() && i < 99; i++) {
    EEPROM.write(i, name[i]);
  }
  EEPROM.write(name.length(), '\0'); // Null-terminate the string
  EEPROM.commit(); // Commit to save changes on ESP boards
}

void handleCommand(String command) {
  command.trim(); // Remove any leading/trailing whitespace

  if (command.equalsIgnoreCase("identify")) {
    // Blink the LED for 3 seconds
    for (int i = 0; i < 6; i++) {
      digitalWrite(led, HIGH);  // Turn the LED on
      delay(250);               // Wait for 250 milliseconds
      digitalWrite(led, LOW);   // Turn the LED off
      delay(250);               // Wait for 250 milliseconds
    }
  } 
  else if (command.startsWith("set_name")) {
    // Extract the name from the command
    int index = command.indexOf(' ');
    if (index > 0) {
      String newName = command.substring(index + 1);
      writeDeviceName(newName); // Save the new name to EEPROM
      deviceName = newName; // Update the deviceName variable
      Serial.println("Name set to: " + deviceName);
    } else {
      Serial.println("Invalid set_name command");
    }
  } 
  else if (command.equalsIgnoreCase("get_name")) {
    // Send the current device name back to the serial
    // readDeviceName();
    Serial.println("Device name: " + deviceName);
  }
  else if (command.startsWith("upload:")) {
    // Handle upload
    handleUpload(command);
  }
}

void handleUpload(String command) {
  // Remove the "upload:" prefix and ":end" suffix
  command.replace("upload:", "");
  command.replace(":end", "");

  // Deserialize the JSON object
  StaticJsonDocument<512> doc;  // Adjust size if needed
  DeserializationError error = deserializeJson(doc, command);

  if (error) {
    Serial.print("Failed to parse JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Example: Accessing data from the JSON object
  if (doc.containsKey("key1")) {
    int value1 = doc["key1"];  // Replace "key1" with actual JSON keys
    Serial.print("Key1: ");
    Serial.println(value1);
  }
  
  if (doc.containsKey("key2")) {
    String value2 = doc["key2"];
    Serial.print("Key2: ");
    Serial.println(value2);
  }

  // Add additional processing of JSON data as needed
  Serial.println("JSON upload processed successfully.");
}

void setup() {
  // Initialize serial communication
  Serial.begin(115200);

  // Set LED to be an output pin
  pinMode(led, OUTPUT);

  // Initialize EEPROM with size 512 bytes for Arduino boards without commit requirement
  EEPROM.begin(512); // Uncomment for boards that need it

  // Read the device name from EEPROM
  readDeviceName();
}

void loop() {
  // Check if any serial data is available
  if (Serial.available() > 0) {
    // Read the incoming byte
    String command = Serial.readStringUntil('\n');
    handleCommand(command);
  }
}
