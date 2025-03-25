#include <SPI.h>
#include <mcp_canbus.h>

#define SPI_CS_PIN 9
MCP_CAN CAN(SPI_CS_PIN);

void setup()
{
  if (CAN.begin(CAN_1000KBPS) != CAN_OK)
  {
    while (1)
      ;
  }
}

void loop()
{
  
}
