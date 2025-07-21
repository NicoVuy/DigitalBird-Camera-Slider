class EncoderState
{
private:
  AS5600 encoder; //AMS AS5600 Encoder setups   The encoders keeps track of motor positiions even when powered down for positioning

  int resetEncoder; 
  uint8_t i2c_bus;
  bool should_reverse;
  bool encoder_available;
  double gear_ratio;
  char * id;

  void handle_reset_request(){
    if (ShouldResetEncoder()) {
        encoder.setZero();
        lastOutput = 0;
        S_position = 0;
        E_position = 0;
        revolutions = 0;
        ResetDone();
      }
  }
  void handle_reversal(){
    if (should_reverse){
        if (output < 0) {                               //Reverse the values for the encoder position
        S_position = abs(S_position);
      } else {
        S_position = S_position - (S_position * 2);

      }
    }
  }


public:
  long S_position;                          // Number of stepper steps at 16 microsteps/step interpolated by the driver to 256 microsteps/step (confusing!)
  long revolutions;     // number of revolutions the encoder has made
  long E_position;                        // the calculated value the encoder is at
  long output;                                // raw value from AS5600
  long lastOutput;                              // last output from AS5600
  long E_outputPos;                         // Ajustment value
  long E_lastOutput;                            // last output from AS5600
  long E_Trim;                              // A Trim value for the encoder effectively setting its 0 position to match Step 0 position
  long E_Current;
  long E_Turn;
  long E_outputTurn;
  long E_outputHold;
  long loopcount;
  long S_lastPosition;

public:
int getRawPosition(){
 return encoder.getPosition();
}
void checkEncoder(Logger & logger) {                                              //This function checks to see if the zoom and or Focus motors are available before running the encoder script for them
    TCA9548A(i2c_bus);
    Wire.beginTransmission(0x36); //connect to the sensor
    Wire.write(0x0B); //figure 21 - register map: Status: MD ML MH
    Wire.endTransmission(); //end transmission
    Wire.requestFrom(0x36, 1); //request from the sensor
    if (Wire.available() == 0) {
      encoder_available = false;
      logger.printf("\nEncoder %s unavailable", id);
    } else {
      encoder_available = true;
      int status=encoder.getStatus();
      bool magnet_detected=(status & 0b00100000)>0;
      bool magnet_low=(status & 0b00010000)>0;
      bool magnet_high=(status & 0b00001000)>0;
      int raw_position=getRawPosition();
      logger.printf("\nEncoder %s found, raw position %d, status magnet detected %d, magnet low %d, magnet high %d ", id, raw_position, magnet_detected, magnet_low, magnet_high);
    }
  }
  void ResetEncoder(){
    resetEncoder = 1;
  }
  bool ShouldResetEncoder(){
    return resetEncoder == 1;
  }
  void ResetDone(){
    resetEncoder = 0;
  }
  long get_sposition(){
    return S_position;
  }
  bool IsOperational(){
    return encoder_available;
  }
  EncoderState(char* anid, uint8_t abus, double aratio, bool reverse):id(anid),i2c_bus(abus),gear_ratio(aratio),should_reverse(reverse),resetEncoder(1), revolutions(0), E_position(0), E_outputPos(0), S_position(0), E_Trim(0), E_Current(0), E_Turn(0),
  E_outputTurn(0),E_outputHold(32728),loopcount(0), S_lastPosition(0), encoder_available(false)
  {
    
  }

  void Start_Encoder() {
    TCA9548A(i2c_bus);
    handle_reset_request();
    
    output = encoder.getPosition();           // get the raw value of the encoder

    if ((lastOutput - output) > 2047 ) {       // check if a full rotation has been made
      revolutions++;
    }
    if ((lastOutput - output) < -2047 ) {
      revolutions--;
    }
    E_position = revolutions * 4096 + output;   // calculate the position the the encoder is at based off of the number of revolutions
    lastOutput = output;                      // save the last raw value for the next loop
    E_outputPos = E_position;

    S_position = ((E_position / 2.56));       //Ajust encoder to stepper values the number of steps eqiv
    S_position = (S_position * gear_ratio);            //Ajust encoder to stepper values the number of steps eqiv
  }
};

EncoderState F_("Focus", 0,2, false);
EncoderState Z_("Zoom", 2,2, false);
EncoderState P_("Pan", 3,1, false);
EncoderState T_("Tilt", 1,5.18, true);



