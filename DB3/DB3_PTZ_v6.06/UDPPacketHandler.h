#ifndef UDP_PACKET_HANDLER_H
#define UDP_PACKET_HANDLER_H

#include <WiFiUdp.h>

class UDPPacketHandler {
private:
  WiFiUDP udp;
  Logger& logger;
  const uint16_t port = 1259;
  // Pointers to global variables
  int* pJoy_Pan_Speed;
  int* pJoy_Pan_Accel;
  int* pJoy_Tilt_Speed;
  int* pJoy_Tilt_Accel;
  void (*pHome)();
  void (*pStop)();
  FastAccelStepper *stepper1;
  FastAccelStepper *stepper2;
  // Buffer for incoming packets
  uint8_t packetBuffer[255];
  
  // Process command based on the two bytes before 0xFF
  void processCommand(uint8_t* buffer, int packetSize) {
    if (packetSize < 5 || buffer[0] != 0x81 || buffer[packetSize - 1] != 0xFF) {
      logger.printf("\nUDP Received invalid packet with length %d", packetSize);
      return; // Invalid packet
    }
    
    uint8_t cmd1 = buffer[packetSize - 3];
    uint8_t cmd2 = buffer[packetSize - 2];
    
    if (cmd1 == 0x06 && cmd2 == 0x04) {
      logger.printf("\nUDP Received Home command");
      pHome();
    }
    else if (cmd1 == 0x06 && cmd2 == 0x12) {
      logger.printf("\nUDP Received position request");
      // Get absolute position command
      sendPositionPacket();
    }
    else if ((cmd1 == 0x01 || cmd1 == 0x02 || cmd1 == 0x03) &&
             (cmd2 == 0x01 || cmd2 == 0x02 || cmd2 == 0x03)) {
      // Movement commands
      logger.printf("\nUDP Received move command %X %X", cmd1, cmd2);
      processMovementCommand(cmd1, cmd2, buffer, packetSize);
    }
  }
  
  // Process movement commands
  void processMovementCommand(uint8_t cmd1, uint8_t cmd2, uint8_t* buffer, int packetSize) {
    if (packetSize < 6) return; // Ensure enough bytes for speeds
    
    int8_t VPanSpeed = buffer[4]; 
    int pan_factor = 0; // Default stop
    // Tilt direction
    if (cmd1== 0x01) { // Negative direction
      pan_factor=-1;
    } else if (cmd1 == 0x02) { // Positive direction
      pan_factor=1;
    }
    *pJoy_Pan_Accel = 2000;
    if (VPanSpeed < 3) {
      *pJoy_Pan_Speed = pan_factor * (VPanSpeed * 200);

    } else {
      *pJoy_Pan_Speed = pan_factor * (VPanSpeed * 150);

    }
    int8_t VTiltSpeed = buffer[5]; // 5th byte (index 4)
    int tilt_factor = 0; // Default stop
    // Tilt direction
    if (cmd2== 0x01) { // Negative direction
      tilt_factor=1;
    } else if (cmd2 == 0x02) { // Positive direction
      tilt_factor=-1;
    }

    if (VTiltSpeed < 3) {
      *pJoy_Tilt_Speed = VTiltSpeed * tilt_factor * 200 * 2;
    } 
    else {
      *pJoy_Tilt_Speed = VTiltSpeed * tilt_factor * 150 * 2;
    }


    logger.printf("\nUDP PanTilt to %d , %d (%d,%d)", *pJoy_Pan_Speed, *pJoy_Tilt_Speed, VPanSpeed, VTiltSpeed);
  }
  
  // Send 11-byte position packet
  void sendPositionPacket() {
    uint8_t response[11] = {
      0x90, 0x50, // Start markers
      0x00, 0x00, 0x00, 0x00, // Placeholder for pan position
      0x00, 0x00, 0x00, 0x00, // Placeholder for tilt position
      0xFF  // End marker
    };
    
    int16_t pan=stepper2->getCurrentPosition();
    int16_t tilt=stepper1->getCurrentPosition();

    // Encode panPosition (range -7000 to +7000) into 4 bytes, big-endian, lower nibble
    response[2] = (pan >> 12) & 0x0F; // Most significant nibble
    response[3] = (pan >> 8) & 0x0F;
    response[4] = (pan >> 4) & 0x0F;
    response[5] = pan & 0x0F;
    
    // Encode tiltPosition (assuming same range) into 4 bytes, big-endian, lower nibble
    response[6] = (tilt >> 12) & 0x0F;
    response[7] = (tilt >> 8) & 0x0F;
    response[8] = (tilt >> 4) & 0x0F;
    response[9] = tilt & 0x0F;

    logger.printf("\nUDP Pan %d (%x): 0x%02X 0x%02X 0x%02X 0x%02X", pan, pan, response[2],response[3],response[4],response[5]);
    logger.printf("\nUDP Tilt %d (%x): 0x%02X 0x%02X 0x%02X 0x%02X", tilt, tilt, response[6],response[7],response[8],response[9]);

    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(response, 11);
    udp.endPacket();
  }

public:
  UDPPacketHandler(int* panspeed, int* panaccel, int* tiltspeed, int* tiltaccel, Logger & alogger, void (*apHome)(),  void (*apStop)()) 
    : logger(alogger), pJoy_Pan_Speed(panspeed), pJoy_Pan_Accel(panaccel), 
      pJoy_Tilt_Speed(tiltspeed), pJoy_Tilt_Accel(tiltaccel), pHome(apHome), pStop(apStop) {
        stepper1 = NULL;
        stepper2 = NULL;
      }
  
  // Initialize UDP
  void begin(FastAccelStepper *astepper1, FastAccelStepper *astepper2) {
    stepper1=astepper1;
    stepper2=astepper2;
    udp.begin(port);
  }
  
  // Check for and process incoming packets
  void processPackets() {
    int packetSize = udp.parsePacket();
    if (packetSize) {
      int len = udp.read(packetBuffer, sizeof(packetBuffer));
      logger.printf("\nUDP Received packet len %d", len);    
      processCommand(packetBuffer, len);
    }
  }
};

#endif