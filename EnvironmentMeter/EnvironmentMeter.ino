/**
 * SPL and environment meter. Made to be executed on STM32F4XX "Black Pill" devices.
 * 
 * @file EnvironmentMeter.ino
 * @brief Reads and display sound level, temperature, humidity, barometric pressure, and luminosity information.
 * @author Jean-Philippe Lesage
 * @version 1.0
 * @since 2022-05-16
 * 
 **********
 * 
 * Required libraries:
 *  Adafruit ST7789 v1.9.3
 *  Adafruit GFX    v1.11.1
 *  BlueDot BME280  v1.0.9
 * 
 **********
 * 
 * NOTE FOR UNO R3:
 *  It might not work since this uses quite a bit of memory. Libraries were changed so it *might* just be small enough to run
 *  on such a limited device.
 * 
 * NOTE FOR MEGA2560:
 *  SCLK, MOSI, and MISO pins are 52, 51 and 50 respectively. Or use the ICSP header.
 *
 * NOTE FOR STM32F4XX:
 *  CS, RES, and DC absolutely need to be on pins PA13, PA14, and PA15 respectively. No other pins seems to work. Any of the SPI 
 *  pins should work for the other connections. I used pins PA5, PA6, and PA7 for SCLK, MISO, and MOSI connections.
 * 
 *  Note that the STM32F4xx is 5V-compatible, but has a *3.3V logic*. Reference voltage is -->3.3V<--, not 5V.
 * 
 * STM32F411CEU6 "Black Pill" wiring diagram (w/ board pin labels):
 *  ST7789 TFT screen:
 *      VCC   ->  5V
 *      GND   ->  GND
 *      SCLK  ->  A5
 *      MOSI  ->  A7
 *      MISO  ->  A6
 *      CS    ->  A13
 *      RES   ->  A14
 *      DC    ->  A15
 *      BLK   ->  Not connected
 *      SDCS  ->  Not connected
 * 
 *  Gravity Analog Sound Level Meter
 *      VCC   ->  3V3
 *      GND   ->  GND
 *      SIG   ->  A1
 * 
 *  BME280
 *      VCC       ->  3V3
 *      GND       ->  GND
 *      SDA/MOSI  ->  B7
 *      SCL/SCK   ->  B6
 *      ADDR/MISO ->  Not connected
 *      CS        ->  Not connected
 * 
 *  Grove Light Sensor
 *      VCC       ->  3V3
 *      GND       ->  GND
 *      SIG       ->  A2
 * 
 **********
 * R.I.P. Saphira. Sunrise: 2007-02-07 - Sunset: 2022-05-16. You were the best cat one could dream of having. I miss you <3
 **********
 */

#include <Adafruit_ST7789.h>
#include <Adafruit_GFX.h>
#include <BlueDot_BME280.h>

// Some RGB565 colors.
#define COLOR_RGB565_BLACK     0x0000   // Black
#define COLOR_RGB565_NAVY      0x000F   // Navy blue
#define COLOR_RGB565_DGREEN    0x03E0   // Dark green
#define COLOR_RGB565_DCYAN     0x03EF   // Teal
#define COLOR_RGB565_MAROON    0x7800   // Deep red-ish?
#define COLOR_RGB565_PURPLE    0x780F   // Purple
#define COLOR_RGB565_OLIVE     0x7BE0   // Olive green
#define COLOR_RGB565_LGRAY     0xC618   // Light gray
#define COLOR_RGB565_DGRAY     0x7BEF   // Dark gray
#define COLOR_RGB565_BLUE      0x001F   // Blue
#define COLOR_RGB565_GREEN     0x07E0   // Green
#define COLOR_RGB565_CYAN      0x07FF   // Cyan
#define COLOR_RGB565_RED       0xF800   // Red
#define COLOR_RGB565_MAGENTA   0xF81F   // Magenta
#define COLOR_RGB565_YELLOW    0xFFE0   // Yellow
#define COLOR_RGB565_ORANGE    0xFD20   // Orange
#define COLOR_RGB565_WHITE     0xFFFF   // White
#define COLOR_RGB565_PINK      0xFE19   // Pink
#define COLOR_RGB565_SKYBLUE   0x867D   // Sky blue

// SPL meter defines
#define SoundSensorPin PA1  // Analog pin the sensor is plugged in.
#define VREF  3.3 // Voltage on AREF pin. There's no AREF pin on STM32 devices, so let's assume the documentation is correct and it's 3.3V logic.

//IPS TFT screen defines
#if defined ARDUINO_SAM_ZERO
#define TFT_DC  7
#define TFT_CS  5
#define TFT_RST 6
#elif defined(ESP32) || defined(ESP8266)
/*ESP32 and ESP8266*/
#define TFT_DC  D2
#define TFT_CS  D6
#define TFT_RST D3
#elif defined(STM32F4)
/*STM32F4XX boards*/
#define TFT_DC  PA15
#define TFT_CS  PA13
#define TFT_RST PA14
#else
/*AVR series mainboard*/
#define TFT_DC  2
#define TFT_CS  3
#define TFT_RST 4
#endif

// For the voltage-to-decibel value conversion, we need to know the ADC resolution.
#if defined(ESP32) || defined(STM32F4)
// 12-bit ADC
#define ADCResolution 4096.0
#else
// 10-bit ADC
#define ADCResolution 1024.0
#endif

// Constants for the max DB value for the different background colors.
const float THRESHOLD_MAX_GREEN = 75.0;
const float THRESHOLD_MAX_YELLOW = 120.0;

// The BlueDot BME280 sensor returns this value when properly initialized.
const int BME_SENSOR_DETECTED_SENTINEL = 0x60;

// Font sizes constants.
const int DETAIL_TEXT_FONT_SIZE = 2;
const int EASTER_EGG_TEXT_FONT_SIZE = 1;
const int DB_TEXT_FONT_SIZE = 8;

// Screen object. Dropped the DFRobot libraries like a hot potato because they are huge.
Adafruit_ST7789 screen = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// BME280 sensor library
BlueDot_BME280 g_bmeSensor;
int g_bmeDetected = 0;

/**
 * @brief Setups all the sensors and device settings.
 * @since 2022-05-16
 */
void setup()
{
    Serial.begin(115200);
    screen.init(240, 320, SPI_MODE_0);

    #if defined(ESP32) || defined(STM32F4)
    // Forcing the analog read resolution to use the full 12-bits of the ADC (only for devices that supports it). We want all the precisions.
    analogReadResolution(12);
    #endif

    g_bmeSensor.parameter.communication = 0;
    g_bmeSensor.parameter.I2CAddress = 0x77;
    g_bmeSensor.parameter.sensorMode = 0b11;
    g_bmeSensor.parameter.IIRfilter = 0b100;
    g_bmeSensor.parameter.humidOversampling = 0b101;
    g_bmeSensor.parameter.tempOversampling = 0b101;
    g_bmeSensor.parameter.pressOversampling = 0b101;
    g_bmeSensor.parameter.pressureSeaLevel = 1014.80;
    g_bmeSensor.parameter.tempOutsideCelsius = 15;
    g_bmeSensor.parameter.tempOutsideFahrenheit = 59;

    g_bmeDetected = g_bmeSensor.init();

    if (g_bmeDetected != BME_SENSOR_DETECTED_SENTINEL)
    {
        Serial.println(F("BME280 Sensor not found!"));
    }
    else
    {
        Serial.println(F("BME280 Sensor detected!"));
    }

    screen.setRotation(3); // Rotates the display 90 degrees clockwise (320x240)
    screen.fillScreen(COLOR_RGB565_BLACK); // Blacking screen out.
}

/**
 * @brief Gets the decibel value from the analog voltage at the sensor output.
 * @note Value depends on Vref and ADC resolution. There's no way to get either of these from the board and they need to be hardcoded.
 * @return float, decibel value.
 * @since 2022-05-16
 */
float getDecibelFromVoltage()
{
    const float DB_CONV_MULTIPLIER = 50.0;
    return (analogRead(SoundSensorPin) / ADCResolution * VREF) * DB_CONV_MULTIPLIER;
}

/**
 * @brief Pads a string to be a given minimum length.
 * @param p_string:  Pointer to a String to modify.
 * @param p_length:  Minimum string length.
 * @param p_paddingChar: Padding character to use
 * @since 2022-05-16
 */
void padString(String *p_string, int p_length, char p_paddingChar) {
    while (p_string->length() < p_length)
    {
        p_string->concat(p_paddingChar);
    }
}

/**
 * @brief Displays a string on a display device.
 * @param p_string:      String to print.
 * @param p_xLocation:   Horizontal location of the text, in pixels.
 * @param p_yLocation:   Vertical location of the text, in pixels.
 * @param p_foreColor:   Text color.
 * @param p_backColor:   Background color.
 * @param p_textSize:    Size of the Text.
 * @since 2022-05-30
 */
void displayString(String p_string, int p_xLocation, int p_yLocation, unsigned int p_foreColor, unsigned int p_backColor, unsigned int p_textSize)
{
    screen.setCursor(p_xLocation, p_yLocation);
    screen.setTextColor(p_foreColor, p_backColor);
    screen.setTextSize(p_textSize);
    screen.print(p_string);
}

/**
 * @brief Main program loop.
 */
void loop()
{
    float dbValue = getDecibelFromVoltage();
    int luminosity = analogRead(PA2);

    String outputTemp = "  Temp:  ";
    String outputHumidity = "  RH:    ";
    String outputPressure = "  Pres.: ";
    String outputAltitude = "  Alt.:  ";
    String outputLum = "  Lum.:  ";

    // This is the relative luminosity based on the output of the sensor. Raw ADC value is displayed, not the lumen value.
    outputLum += luminosity;

    if (g_bmeDetected == BME_SENSOR_DETECTED_SENTINEL)
    {
        outputTemp += g_bmeSensor.readTempC();
        outputTemp += "C";

        outputHumidity += g_bmeSensor.readHumidity();
        outputHumidity += "%";

        outputPressure += g_bmeSensor.readPressure();
        outputPressure += "hPa";

        outputAltitude += g_bmeSensor.readAltitudeMeter();
        outputAltitude += "m";
    }
    else
    {
        outputTemp = "Sensor error! " + g_bmeDetected;
        outputHumidity = "";
        outputPressure = "";
        outputAltitude = "";
    }

    // Determining the background color. <74.99 is green, between 75 and 119.99 is yellow, >=120 is red.
    int backColor = COLOR_RGB565_GREEN;

    if (dbValue >= THRESHOLD_MAX_GREEN && dbValue < THRESHOLD_MAX_YELLOW)
    {
        backColor = COLOR_RGB565_ORANGE;
    }
    else if (dbValue >= THRESHOLD_MAX_YELLOW)
    {
        backColor = COLOR_RGB565_RED;
    }

    /**
    * Text is printed with a background color. It's less flickering when refreshing text that doesn't change.
    */

    String dbValueString = "";
    dbValueString+= dbValue;
    padString(&dbValueString, 6, ' '); // Padding the output string to make sure it's 6 characters and fills the entire width of the screen.
    displayString(dbValueString, 20, 20, COLOR_RGB565_PURPLE, backColor, DB_TEXT_FONT_SIZE);
    
    // Padding the strings so they are all 22 character long (and gets cleared on the TFT).
    padString(&outputTemp, 22, ' ');
    padString(&outputHumidity, 22, ' ');
    padString(&outputPressure, 22, ' ');
    padString(&outputAltitude, 22, ' ');
    padString(&outputLum, 22, ' ');

    // Altitude depends on the current sea-level adjusted barometric pressure, meaning it cannot be calculated unless that value is fed separately.
    //  I don't have that value, the average is "meh" at best and doesn't represent the actual value, so this is going to be disabled for now.
    displayString(outputTemp + "\r\n" + outputHumidity + "\r\n" + outputPressure + "\r\n" + outputLum, 0, 100, COLOR_RGB565_PURPLE, COLOR_RGB565_BLACK, DETAIL_TEXT_FONT_SIZE);

    // Saphira says hi.
    displayString("Meow! <3", 260, 230, COLOR_RGB565_DGRAY, COLOR_RGB565_BLACK, EASTER_EGG_TEXT_FONT_SIZE);

    // Outputing values over serial for plotting. That could be skipped with an #if defined() statement.
    Serial.print("DB: ");
    Serial.println(dbValue);
    Serial.println(outputTemp);
    Serial.println(outputHumidity);
    Serial.println(outputPressure);
    Serial.println(outputAltitude);
    Serial.println(outputLum);

    // Waiting half a second before next reading.
    delay(500);
}