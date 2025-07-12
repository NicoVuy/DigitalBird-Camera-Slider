#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET 4
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   10
#define LOGO_WIDTH    25



// 'Bat25', 25x10px
const unsigned char logo_bmp1 [] PROGMEM = {
  0x3f, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00, 0x80, 0x40, 0x00, 0x1e, 0x80, 0xc0, 0x00, 0x1e, 0x80,
  0xc0, 0x00, 0x1e, 0x80, 0xc0, 0x00, 0x1e, 0x80, 0xc0, 0x00, 0x1e, 0x80, 0x40, 0x00, 0x1e, 0x80,
  0x40, 0x00, 0x00, 0x80, 0x3f, 0xff, 0xff, 0x00
};
// 'Bat50', 25x10px
const unsigned char logo_bmp2 [] PROGMEM = {
  0x3f, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00, 0x80, 0x40, 0x03, 0xde, 0x80, 0xc0, 0x03, 0xde, 0x80,
  0xc0, 0x03, 0xde, 0x80, 0xc0, 0x03, 0xde, 0x80, 0xc0, 0x03, 0xde, 0x80, 0x40, 0x03, 0xde, 0x80,
  0x40, 0x00, 0x00, 0x80, 0x3f, 0xff, 0xff, 0x00
};
// 'Bat75', 25x10px
const unsigned char logo_bmp3 [] PROGMEM = {
  0x3f, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00, 0x80, 0x40, 0x7b, 0xde, 0x80, 0xc0, 0x7b, 0xde, 0x80,
  0xc0, 0x7b, 0xde, 0x80, 0xc0, 0x7b, 0xde, 0x80, 0xc0, 0x7b, 0xde, 0x80, 0x40, 0x7b, 0xde, 0x80,
  0x40, 0x00, 0x00, 0x80, 0x3f, 0xff, 0xff, 0x00
};
// 'Bat100', 25x10px
const unsigned char logo_bmp4 [] PROGMEM = {
  0x3f, 0xff, 0xff, 0x00, 0x40, 0x00, 0x00, 0x80, 0x4f, 0x7b, 0xde, 0x80, 0xcf, 0x7b, 0xde, 0x80,
  0xcf, 0x7b, 0xde, 0x80, 0xcf, 0x7b, 0xde, 0x80, 0xcf, 0x7b, 0xde, 0x80, 0x4f, 0x7b, 0xde, 0x80,
  0x40, 0x00, 0x00, 0x80, 0x3f, 0xff, 0xff, 0x00
};

int OLED_ID;

void SetupOled(Logger & logger){
  //*************************************Setup OLED****************************
  TCA9548A(4);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    logger.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.clearDisplay();
}



void Percent_100(int id) {
  TCA9548A(4);                          //OLED on bus 4
  display.clearDisplay();
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(35, 16);            // Start at top-left corner
  display.cp437(true);                  // Use full 256 char 'Code Page 437' font
  if (id == 5) {
    OLED_ID = 0;
  } else {
    OLED_ID = id;
  }
  display.printf("ID:%d\n", OLED_ID);
  display.setTextSize(1);               // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.setCursor(35, 25);            // Start at top-left corner
  display.cp437(true);                  // Use full 256 char 'Code Page 437' font
  display.write("100%");
  display.drawBitmap(
    (display.width()  - 60 ),
    (display.height() - 10),
    logo_bmp4, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(100);
}

void Percent_75(int id) {
  TCA9548A(4);                          //OLED on bus 3
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 16);
  display.cp437(true);
  if (id == 5) {                 //PTZ ID 0 is actually 5!
    OLED_ID = 0;
  } else {
    OLED_ID = id;
  }
  display.printf("ID:%d\n", OLED_ID);


  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 25);
  display.cp437(true);
  display.write("75%");
  //display.println(BatV);
  display.drawBitmap(
    (display.width()  - 60 ),
    (display.height() - 10),
    logo_bmp3, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(100);
}

void Percent_50(int id) {
  TCA9548A(4);                          //OLED on bus 3
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 16);
  display.cp437(true);
  if (id == 5) {
    OLED_ID = 0;
  } else {
    OLED_ID = id;
  }
  display.printf("ID:%d\n", OLED_ID);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 25);
  display.cp437(true);
  display.write("50%");
  display.drawBitmap(
    (display.width()  - 60 ),
    (display.height() - 10),
    logo_bmp2, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(100);
}

void Percent_25(int id) {
  TCA9548A(4);                          //OLED on bus 3
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 16);
  display.cp437(true);
  if (id == 5) {
    OLED_ID = 0;
  } else {
    OLED_ID = id;
  }
  display.printf("ID:%d\n", OLED_ID);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 25);
  display.cp437(true);
  display.write("25%");
  display.drawBitmap(
    (display.width()  - 60 ),
    (display.height() - 10),
    logo_bmp1, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(100);
}

void Percent_0(int id) {
  TCA9548A(4);                          //OLED on bus 3
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 16);
  display.cp437(true);
  if (id == 5) {
    OLED_ID = 0;
  } else {
    OLED_ID = id;
  }
  display.printf("ID:%d\n", OLED_ID);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 25);
  display.cp437(true);
  display.write("0%");
  display.drawBitmap(
    (display.width()  - 60 ),
    (display.height() - 10),
    logo_bmp1, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(100);
}

void Oledmessage(int sld) {
  TCA9548A(4);                          //OLED on bus 3
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 16);
  display.cp437(true);
  display.printf("Sld:%d\n", sld);
  display.display();
  delay(2000);
}

