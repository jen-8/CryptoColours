#include "stm32f4xx_hal.h"

// Libraries to control the LCD display
#include <TFT_eSPI.h>
#include <SPI.h>

// Library to control the WS2812b LED strip
#include <Adafruit_NeoPixel.h>

// Define data pin and amount of LEDs to write to
#define LED_PIN        PIN_A0 
#define NUMPIXELS 3

// The ports seem mixed up
#define BUTTON_PIN_1  PA_10 // Pin D10 
#define BUTTON_PIN_2  PA_3 // Pin D3
#define BUTTON_PIN_3  PA_9 // Pin D9

#define SUBMIT USER_BTN // Blue onboard button

#define RAND PIN_A1 // Supplies the randomSeed()

// Define 8 distinct colours as constants. These will be used for setting the LED colours
#define RED pixels.Color(255, 0, 0)
#define ORANGE pixels.Color(255, 128, 0)
#define YELLOW pixels.Color(255, 255, 0)
#define GREEN pixels.Color(0, 255, 0)
#define CYAN pixels.Color(0, 255, 255)
#define BLUE pixels.Color(0, 0, 255)
#define PURPLE pixels.Color(127, 0, 255)
#define PINK pixels.Color(255, 20, 147)

// Define 8 distinct colours as constants (WRGB values). These will be used for getting the LED colours
// Best to never have magic numbers floating around.
enum Colour { red = 16711680, orange = 16744448, yellow = 16776960, green = 65280, cyan = 65535, blue = 255, purple = 8323327, pink = 16716947};

void generateColourCode();
void cycleColours(uint32_t buttonPin, uint16_t LEDnum);
void loseGame();
void winGame();

int validateCode();

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800); // Invoke library

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 20; 

int oldState = LOW;
int tries = 0, colour = 0;
int guessedColours[3] = {0, 0, 0}, code[3] = {0, 0, 0}, colours[8] = {RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, PURPLE, PINK};

void setup(void) {

  Serial.begin(115200);

  // Set the pins to input
  pinMode(BUTTON_PIN_1, INPUT_PULLDOWN);
  pinMode(BUTTON_PIN_2, INPUT_PULLDOWN);
  pinMode(BUTTON_PIN_3, INPUT_PULLDOWN);

  pinMode(SUBMIT, INPUT);

  // Setup random number generator
  int ar = analogRead(RAND);
  randomSeed(ar);

  // Initiate the LCD display
  tft.init();
  tft.setRotation(2);

  pixels.begin();

  generateColourCode();

  // Fill screen with black background
  tft.fillScreen(TFT_BLACK);
  
  // Set "cursor" at top left corner of display (0,0) and select font 2
  tft.setCursor(0, 0, 2);
  // Set the font colour to be white, set text size multiplier to 1
  tft.setTextColor(TFT_WHITE);  tft.setTextSize(1);

  tft.println("Welcome to");
  tft.println("CRYPTOCOLOURS!");
  delay(1000);
  tft.fillScreen(TFT_BLACK);

  tft.setCursor(0, 0, 2); tft.setTextSize(0.5);
  tft.println("Press the buttons to cycle through colours.");
  tft.println("When you think that you've guessed the combination, press the blue button.");
  delay(5000);
  tft.fillScreen(TFT_BLACK);
}

void loop() {

  if(tries <= 20)
  {
    // this works, for some reason
    cycleColours(BUTTON_PIN_2, 0);
    cycleColours(BUTTON_PIN_3, 1);
    cycleColours(BUTTON_PIN_1, 2); 

    if(digitalRead(SUBMIT) == LOW)
    {
      int b1_guess = pixels.getPixelColor(0);
      int b2_guess = pixels.getPixelColor(1);
      int b3_guess = pixels.getPixelColor(2);

      //checks to ensure no repeating colours are submitted. If there is, it keeps cycling through the colours until the player submites a valid combination
      if((b1_guess == b2_guess) || (b1_guess == b3_guess) || (b2_guess == b3_guess)) 
      { 
        tft.setCursor(0, 0, 2);
        tft.println("Error, no repeating colours.");

        cycleColours(BUTTON_PIN_2, 0);
        cycleColours(BUTTON_PIN_3, 1);
        cycleColours(BUTTON_PIN_1, 2); 
      }
  
      //clears the screen and checks to see how many colours are correct
      tft.fillScreen(TFT_BLACK);
      int count = validateCode();
      tries++;

      Serial.println(tries);

      //if the if you have repeating colours, it decreases the number of tries by 1, as an invalid input will not count towards the number of tries
      if(count == 4)
      {
        tries--;
      } //if you have all 3 correct colours, you win the game
      else if(count == 3)
      {
        winGame();
      }
    }
  }
  else //if you use all of your tries, the game is lost
  {
    loseGame();
  }
  
}

// Function to generate a random colour code
void generateColourCode() 
{
  int button_col1 = 0, button_col2 = 0, button_col3 = 0;

  // Colours: 
    // 1 : red
    // 2 : orange
    // 3 : yellow
    // 4 : green
    // 5 : cyan
    // 6 : dark blue
    // 7 : purple
    // 8 : pink 

  // Generates a random number between 1 to 8 and assigns it to that button number
  button_col1 = random(1, 9); 
  button_col2 = random(1, 9);
  button_col3 = random(1, 9);

  // Checks to ensure no 2 buttons have the same colour
  while ((button_col1 == button_col2) || (button_col1 == button_col3) || (button_col2 == button_col3)) 
  { 
    button_col1 = random(1, 9); 
    button_col2 = random(1, 9);
    button_col3 = random(1, 9);
  }

  code[0] = button_col1;
  code[1] = button_col2;
  code[2] = button_col3;

  Serial.println(code[0]);
  Serial.println(code[1]);
  Serial.println(code[2]);

  for(int i = 0; i < 3; i++)
  { //assigns each value from 1-8 to a WRGB value so the LEDs display the correct colour
    switch(code[i])
    {
      case 1: code[i] = red;
      break;
      case 2: code[i] = orange;
      break;
      case 3: code[i] = yellow;
      break;
      case 4: code[i] = green;
      break;
      case 5: code[i] = cyan;
      break;
      case 6: code[i] = blue;
      break;
      case 7: code[i] = purple;
      break;
      case 8: code[i] = pink;
      break;
    }
  }

  for(int i = 0; i < 3; i++)
  {
    Serial.println(code[i]);
  }
}

// Function to cycle through 8 colours when a button is pressed. Colours should loop back after we hit 7.
// ** ADAPTED FROM AN EXAMPLE FROM THE ADAFRUIT NEOPIXEL LIBRARY **
void cycleColours(uint32_t buttonPin, uint16_t LEDnum)
{
  bool newState = digitalRead(buttonPin);

  // Check if the button has been pressed
  if((newState == LOW) && (oldState == HIGH)) {
    // Short delay to debounce button.
    delay(300);
    newState = digitalRead(buttonPin);

    if(newState == LOW) 
    {
      // Reset the counter once it reaches the last colour (pink)
      if(++colour > 7) colour = 0;
      
      // Cycle through the colours
      switch(colour) 
      {
        case 0: pixels.setPixelColor(LEDnum, RED);
        break;
        case 1:
        pixels.setPixelColor(LEDnum, ORANGE);
        break;
        case 2:
        pixels.setPixelColor(LEDnum, YELLOW);
        break;
        case 3:
        pixels.setPixelColor(LEDnum, GREEN);
        break;
        case 4:
        pixels.setPixelColor(LEDnum, CYAN);
        break;
        case 5:
        pixels.setPixelColor(LEDnum, BLUE);
        break;
        case 6:
        pixels.setPixelColor(LEDnum, PURPLE);
        break;
        case 7: pixels.setPixelColor(LEDnum, PINK);
        break;
      }
    }
  }
  pixels.show();
  // Set the last-read button state to the old state.
  oldState = newState;
}

// Check if submitted code is correct
int validateCode() 
{
    int count = 0; //keeps track of correct colours
    
    int colour1 = code[0];
    int colour2 = code[1];
    int colour3 = code[2];

    int guess1 = pixels.getPixelColor(0);
    int guess2 = pixels.getPixelColor(1);
    int guess3 = pixels.getPixelColor(2);

    tft.setCursor(0, 0, 2);

    //checks that no colours are repeating. if it is, it returns a 4 and breaks out of the function
    if((guess1 == guess2) || (guess1 == guess3) || (guess2 == guess3))
    {
        tft.setCursor(0, 0, 2);
        tft.println("Error, no repeating colours."); 

        return 4;
    }

    //each if statement checks each individual LED to see if they have the correct colour in the correct position 
    //and prints the correcponding message to the display screen
    if (guess1 == colour1) {
        tft.println("Button 1: correct colour");
        count++;
    } 
    else if ((guess1 == colour2) || (guess1 == colour3)) 
    {
        tft.println("Button 1: correct colour, wrong place");
    } 
    else 
    {
        tft.println("Button 1: wrong colour");
    }
    
    if (guess2 == colour2) {
        tft.println("Button 2: correct colour");
        count++;
    } 
    else if ((guess2 == colour1) || (guess2 == colour3)) 
    {
        tft.println("Button 2: correct colour, wrong place");
    } 
    else 
    {
        tft.println("Button 2: wrong colour");
    }
    
    if (guess3 == colour3) {
        tft.println("Button 3: correct colour");
        count++;
    } 
    else if ((guess3 == colour1) || (guess3 == colour2)) 
    {
        tft.println("Button 3: correct colour, wrong place");
    } 
    else 
    {
        tft.println("Button 3: wrong colour");
    }

    return count;
}

//if the game is won, the display screen displays the win message
void winGame()
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 2);
    tft.println("Congratulations! You have won the game!!");
}

// If the player has used 10 tries, the game ends and the LEDs will display the correct colours
void loseGame() 
{
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 2);
    tft.println("Sorry, you ran out of tries.");
    tft.println("The correct colours were: ");

    pixels.setPixelColor(0, code[0]);
    pixels.setPixelColor(1, code[1]);
    pixels.setPixelColor(2, code[2]);
    pixels.show();

    while(1) yield(); // Feed the watchdog timer
}


