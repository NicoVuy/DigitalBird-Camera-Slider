AS5600 F_encoder;                                           //AMS AS5600 Encoder setups   The encoders keeps track of motor positiions even when powered down for positioning
AS5600 Z_encoder;
AS5600 P_encoder;
AS5600 T_encoder;

class EncoderState
{
private:
  int resetEncoder;  

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
  EncoderState():resetEncoder(1), revolutions(0), E_position(0), E_outputPos(0), S_position(0), E_Trim(0), E_Current(0), E_Turn(0),
  E_outputTurn(0),E_outputHold(32728),loopcount(0), S_lastPosition(0)
  {
    
  }
};

EncoderState F_;
EncoderState Z_;
EncoderState P_;
EncoderState T_;

//*****Focus Encoder
void Start_F_Encoder() {
  TCA9548A(0);
  if (F_.ShouldResetEncoder()) {
    //    F_encoder.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
    //    F_encoder.resetCumulativePosition(0);
    F_encoder.setZero();
    //F_encoder.resetPosition();
    F_.lastOutput = 0;
    F_.S_position = 0;
    F_.E_position = 0;
    F_.revolutions = 0;
    F_.ResetDone();

  }
  //F_output = F_encoder.getCumulativePosition() ;
  F_.output = F_encoder.getPosition();           // get the raw value of the encoder

  if ((F_.lastOutput - F_.output) > 2047 ) {       // check if a full rotation has been made
    F_.revolutions++;
  }
  if ((F_.lastOutput - F_.output) < -2047 ) {
    F_.revolutions--;
  }
  F_.E_position = F_.revolutions * 4096 + F_.output;   // calculate the position the the encoder is at based off of the number of revolutions

  //logger.println("Encoder E_Position=");
  //logger.println(E_position);

  F_.lastOutput = F_.output;                      // save the last raw value for the next loop
  F_.E_outputPos = F_.E_position;

  F_.S_position = ((F_.E_position / 2.56));       //Ajust encoder to stepper values the number of steps eqiv
  F_.S_position = (F_.S_position * 2);            //Ajust encoder to stepper values the number of steps eqiv
  //logger.println(F_S_position);
}

//*****Zoom Encoder
void Start_Z_Encoder() {
  TCA9548A(2);
  if (Z_.ShouldResetEncoder()) {
    //    Z_encoder.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
    //    Z_encoder.resetCumulativePosition(0);
    Z_encoder.setZero();
    //Z_encoder.resetPosition();
    Z_.lastOutput = 0;
    Z_.S_position = 0;
    Z_.E_position = 0;
    Z_.revolutions = 0;
    Z_.ResetDone();

  }
  //Z_output = Z_encoder.getCumulativePosition() ;
  Z_.output = Z_encoder.getPosition();           // get the raw value of the encoder

  if ((Z_.lastOutput - Z_.output) > 2047 ) {       // check if a full rotation has been made
    Z_.revolutions++;
  }
  if ((Z_.lastOutput - Z_.output) < -2047 ) {
    Z_.revolutions--;
  }
  Z_.E_position = Z_.revolutions * 4096 + Z_.output;   // calculate the position the the encoder is at based off of the number of revolutions

  //logger.println("Encoder E_Position=");
  //logger.println(E_position);

  Z_.lastOutput = Z_.output;                      // save the last raw value for the next loop
  Z_.E_outputPos = Z_.E_position;

  Z_.S_position = ((Z_.E_position / 2.56));       //Ajust encoder to stepper values the number of steps eqiv
  Z_.S_position = (Z_.S_position * 2);            //Ajust encoder to stepper values the number of steps eqiv
  //logger.println(Z_S_position);
}


//*******Tilt Encoder
void Start_T_Encoder() {
  TCA9548A(1);
  if (T_.ShouldResetEncoder()) {
    //    T_encoder.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
    //    T_encoder.resetCumulativePosition(0);
    T_encoder.setZero();
    //T_encoder.resetPosition();
    T_.lastOutput = 0;
    T_.S_position = 0;
    T_.E_position = 0;
    T_.revolutions = 0;
    T_.ResetDone();

  }
  //T_output = T_encoder.getCumulativePosition() ;
  T_.output = T_encoder.getPosition();           // get the raw value of the encoder

  if ((T_.lastOutput - T_.output) > 2047 ) {       // check if a full rotation has been made
    T_.revolutions++;
  }
  if ((T_.lastOutput - T_.output) < -2047 ) {
    T_.revolutions--;
  }
  T_.E_position = T_.revolutions * 4096 + T_.output;   // calculate the position the the encoder is at based off of the number of revolutions

  //logger.println("Encoder E_Position=");
  //logger.println(E_position);

  T_.lastOutput = T_.output;                      // save the last raw value for the next loop
  T_.E_outputPos = T_.E_position;

  T_.S_position = ((T_.E_position / 2.56));       //Ajust encoder to stepper values the number of steps eqiv


  //logger.println(P_S_position);
  T_.S_position = (T_.S_position * 5.18);            //Ajust for gear ratio 5.18:1 which is to fast for the encoder on the!
  //******This is required if the motor is geared and the encoder is NOT on the back of the motor. (The AS5600 canot keep up with the fast moving motor) *****************

  if (T_.output < 0) {                               //Reverse the values for the encoder position
    T_.S_position = abs(T_.S_position);
  } else {
    T_.S_position = T_.S_position - (T_.S_position * 2);

  }
  //***************************************************************************************************************

}

//*******Pan Encoder
void Start_P_Encoder() {
  TCA9548A(3);
  if (P_.ShouldResetEncoder()) {
    //    P_encoder.setDirection(AS5600_CLOCK_WISE);  //  default, just be explicit.
    //    P_encoder.resetCumulativePosition(0);
    P_encoder.setZero();
    //P_encoder.resetPosition();
    P_.lastOutput = 0;
    P_.S_position = 0;
    P_.E_position = 0;
    P_.revolutions = 0;
    P_.ResetDone();

  }
  //P_output = P_encoder.getCumulativePosition() ;
  //P_output = P_encoder.rawAngle() ;
  P_.output = P_encoder.getPosition();           // get the raw value of the encoder

  if ((P_.lastOutput - P_.output) > 2047 ) {       // check if a full rotation has been made
    P_.revolutions++;
  }
  if ((P_.lastOutput - P_.output) < -2047 ) {
    P_.revolutions--;
  }
  P_.E_position = P_.revolutions * 4096 + P_.output;   // calculate the position the the encoder is at based off of the number of revolutions

  //logger.println("Encoder E_Position=");
  //logger.println(E_position);

  P_.lastOutput = P_.output;                      // save the last raw value for the next loop
  P_.E_outputPos = P_.E_position;

  P_.S_position = ((P_.E_position / 2.56));       //Ajust encoder to stepper values the number of steps eqiv
  // P_S_position = (P_S_position * 2);            //Ajust encoder to stepper values the number of steps eqiv
  //logger.println(P_S_position);
  // if (P_output < 0) {                               //Reverse the values for the encoder position
  //    P_S_position = abs(P_S_position);
  //  } else {
  //    P_S_position = P_S_position - (P_S_position * 2);
  //
  //  }
}

