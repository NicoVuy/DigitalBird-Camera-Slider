#define NUM_LEDS 2
#define DATA_PIN 19
CRGB leds[NUM_LEDS];

int Tally;

void SetupTallyLed(){
    //Setup for Tally LED
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(90);
}

void SetTallyLed(int tally){
    switch (tally) {
      case 0:
        leds[0] = CRGB::Black;
        leds[1] = CRGB::Black;
        FastLED.show(); break;
      case 1:
        leds[0] = CRGB::Blue;
        leds[1] = CRGB::Blue;
        FastLED.show(); break;
      case 2:
        leds[0] = CRGB::Blue;
        leds[1] = CRGB::Blue;
        FastLED.show(); break;
      case 3:
        leds[0] = CRGB::Red;
        leds[1] = CRGB::Red;
        FastLED.show(); break;

      case 4:
        leds[0] = CRGB::Black;
        leds[1] = CRGB::Black;
        FastLED.show(); break;
    }

}

void TurnOffTallyLight(){
 Tally = 0;                                            //Turn off the tally light
        leds[0] = CRGB::Black;
        leds[1] = CRGB::Black;
        FastLED.show();
}

void TurnOnTallyLight(){
            Tally = 3;                                            //Turn on the tally light
        leds[0] = CRGB::Red;
        leds[1] = CRGB::Red;
        FastLED.show();

}

void ToggleTallyLight(){
    if (Tally==0) {
        TurnOnTallyLight();
    }else{
        TurnOffTallyLight();
    }
}

void LogTallyLightState(Logger & logger){
    switch (Tally) {
        case 0: logger.println("\nTally 0"); break; //Flashing (in prevue)
        case 1: logger.println("\nTally 1"); break; //Flashing (in prevue)
        case 2: logger.println("\nTally 2"); break; //Light Always on (Live)
        case 3: logger.println("\nTally 3"); break; //Normal or (off inactive)
        case 4: logger.println("\nTally 4"); break; //Normal or (off inactive)
      }
}