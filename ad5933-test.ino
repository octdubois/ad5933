/*
ad5933-test
    Reads impedance values from the AD5933 over I2C and prints them serially.
*/

#include <Wire.h>
#include "AD5933.h"

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include <SPI.h>

#define START_FREQ  (55000)
#define FREQ_INCR   (1000)
#define NUM_INCR    (10)
#define REF_RESIST  (200000)
#define SD_CS 5

double gain[NUM_INCR+1];
int phase[NUM_INCR+1];
String dataMessage;

double impedance;
double Gain;
char filename[16] = {'\0'};
char filename1[16] = {'\0'};

RTC_DATA_ATTR  int num= 0;

void setup(void)
{
  // Begin I2C
  Wire.begin();

  // Begin serial at 9600 baud for output
  Serial.begin(112500);
  Serial.println("AD5933 Test Started!");

  // Perform initial configuration. Fail if any one of these fail.
  if (!(AD5933::reset() &&
        AD5933::setInternalClock(true) &&
        AD5933::setStartFrequency(START_FREQ) &&
        AD5933::setIncrementFrequency(FREQ_INCR) &&
        AD5933::setNumberIncrements(NUM_INCR) &&
        AD5933::setPGAGain(PGA_GAIN_X1)))
        {
            Serial.println("FAILED in initialization!");
            while (true) ;
        }

   delay(5000);
  // Perform calibration sweep
  if (AD5933::calibrate(gain, phase, REF_RESIST, NUM_INCR+1))
    Serial.println("Calibrated!");
  else
    Serial.println("Calibration failed...");

   delay(5000);
   Serial.println("plug sensor");
   delay(10000);
   
  initSd();

  frequencySweepRaw();

  initSd1();

  frequencySweepRaw1();


}

void loop(void)
{
  // Easy to use method for frequency sweep
  //frequencySweepEasy();

  // Delay
  //delay(5000);

  // Complex but more robust method for frequency sweep
  //frequencySweepRaw();

  // Delay
  delay(5000);
}

// Easy way to do a frequency sweep. Does an entire frequency sweep at once and
// stores the data into arrays for processing afterwards. This is easy-to-use,
// but doesn't allow you to process data in real time.

void initSd(){
  
     // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
   File file = SD.open(filename);
   if(!file) {
    num++;
    sprintf(filename, "/data%d.csv", num);
   Serial.println(filename);
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
     writeFile(SD, filename, "frequence,real,imag,impd,mag,gain\r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();
  
  
  
  
  }


void initSd1(){
  
     // Initialize SD card
  SD.begin(SD_CS);  
  if(!SD.begin(SD_CS)) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("ERROR - SD card initialization failed!");
    return;    // init failed
  }

  // If the data.txt file doesn't exist
  // Create a file on the SD card and write the data labels
   File file = SD.open(filename1);
   if(!file) {
    
    sprintf(filename1, "/datav%d.csv", num);
    Serial.println(filename1);
    Serial.println("File doens't exist");
    Serial.println("Creating file...");
     writeFile(SD, filename1, "frequence,real,imag,impd,mag,gain\r\n");
  }
  else {
    Serial.println("File already exists");  
  }
  file.close();
  
  
  
  
  }




void frequencySweepEasy() {
    // Create arrays to hold the data
    int real[NUM_INCR+1], imag[NUM_INCR+1];
     Serial.print("E");
    // Perform the frequency sweep
    if (AD5933::frequencySweep(real, imag, NUM_INCR+1)) {
      // Print the frequency data
    int  cfreq = START_FREQ/1000;
      for (int i = 0; i < NUM_INCR+1; i++, cfreq += FREQ_INCR/1000) {
        // Print raw frequency data
        Serial.print(cfreq);
        Serial.print(" ");
        Serial.print(real[i]);
        Serial.print(" ");
        Serial.print(imag[i]);

        // Compute impedance
        double magnitude = sqrt(pow(real[i], 2) + pow(imag[i], 2));
        double impedance = 1/(magnitude*gain[i]);
        Serial.print(" ");
        Serial.println(impedance);

      }
      Serial.println("Frequency sweep complete!");
    } else {
      Serial.println("Frequency sweep failed...");
    }
}

// Removes the frequencySweep abstraction from above. This saves memory and
// allows for data to be processed in real time. However, it's more complex.


void frequencySweepRaw() {
    // Create variables to hold the impedance data and track frequency
    int real, imag, i = 0, cfreq = START_FREQ/1000;
    Serial.print('R');
    // Initialize the frequency sweep
    if (!(AD5933::setPowerMode(POWER_STANDBY) &&          // place in standby
          AD5933::setControlMode(CTRL_INIT_START_FREQ) && // init start freq
          AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) // begin frequency sweep
         {
             Serial.println("Could not initialize frequency sweep...");
         }

    // Perform the actual sweep
    while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
        // Get the frequency data for this frequency point
        if (!AD5933::getComplexData(&real, &imag)) {
            Serial.println("Could not get raw frequency data...");
        }

        // Print out the frequency data
        Serial.print(cfreq);
        Serial.print(" ");
        Serial.print(real);
        Serial.print(" ");
        Serial.print(imag);

        // Compute impedance
        double magnitude = sqrt(pow(real, 2) + pow(imag, 2));
        impedance = 1/(magnitude*gain[i]);
        Serial.print(" ");
        Serial.println(impedance);
        Gain = (1/100000)/magnitude;
        
         dataMessage = String(cfreq) + "," + String(real) + "," + String(imag) + "," + 
                String(impedance) + "," + String(magnitude) + "," + String(Gain)+  "\r\n";
        Serial.print("Save data: ");
        Serial.println(dataMessage);
        appendFile(SD, filename, dataMessage.c_str());

        // Increment the frequency
        i++;
        cfreq += FREQ_INCR/1000;
        AD5933::setControlMode(CTRL_INCREMENT_FREQ);
    }

    Serial.println("Frequency sweep complete!");

    // Set AD5933 power mode to standby when finished
    if (!AD5933::setPowerMode(POWER_STANDBY))
        Serial.println("Could not set to standby...");
}

void frequencySweepRaw1() {
    // Create variables to hold the impedance data and track frequency
    int real, imag, i = 0, cfreq = START_FREQ/1000;
    Serial.print('R');
    // Initialize the frequency sweep
    if (!(AD5933::setPowerMode(POWER_STANDBY) &&          // place in standby
          AD5933::setControlMode(CTRL_INIT_START_FREQ) && // init start freq
          AD5933::setControlMode(CTRL_START_FREQ_SWEEP))) // begin frequency sweep
         {
             Serial.println("Could not initialize frequency sweep...");
         }

    // Perform the actual sweep
    while ((AD5933::readStatusRegister() & STATUS_SWEEP_DONE) != STATUS_SWEEP_DONE) {
        // Get the frequency data for this frequency point
        if (!AD5933::getComplexData(&real, &imag)) {
            Serial.println("Could not get raw frequency data...");
        }

        // Print out the frequency data
        Serial.print(cfreq);
        Serial.print(" ");
        Serial.print(real);
        Serial.print(" ");
        Serial.print(imag);

        // Compute impedance
        double magnitude = sqrt(pow(real, 2) + pow(imag, 2));
        impedance = 1/(magnitude*gain[i]);
        Serial.print(" ");
        Serial.println(impedance);
        Gain = (1/100000)/magnitude;
        
         dataMessage = String(cfreq) + "," + String(real) + "," + String(imag) + "," + 
                String(impedance) + "," + String(magnitude) + "," + String(Gain)+  "\r\n";
        Serial.print("Save data: ");
        Serial.println(dataMessage);
        appendFile(SD, filename1, dataMessage.c_str());

        // Increment the frequency
        i++;
        cfreq += FREQ_INCR/1000;
        AD5933::setControlMode(CTRL_INCREMENT_FREQ);
    }

    Serial.println("Frequency sweep complete!");

    // Set AD5933 power mode to standby when finished
    if (!AD5933::setPowerMode(POWER_STANDBY))
        Serial.println("Could not set to standby...");
}




// Write to the SD card (DON'T MODIFY THIS FUNCTION)
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

// Append data to the SD card (DON'T MODIFY THIS FUNCTION)
void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
