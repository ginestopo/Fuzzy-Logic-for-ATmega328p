/**
 * fuzzy_controller.ino
 **/

// eFLL includes
#include <Fuzzy.h>
#include <FuzzyComposition.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzyOutput.h>
#include <FuzzyRule.h>
#include <FuzzyRuleAntecedent.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzySet.h>

// pins
#define LDR A0
#define TRIGGER 4
#define ECHO 5
#define LED 6

// LDR Characteristics
const float GAMMA = 0.7;
const float RL10 = 50;

#include <Servo.h>

Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards

int pos = 0;    // variable to store the servo position

const int slidePotPin = A1; // Slide potentiometer connected to analog pin A1

// object library
Fuzzy *fuzzy = new Fuzzy();

void setup() {
  // set console and pins
  Serial.begin(9600);
  pinMode(LDR, INPUT);
  pinMode(TRIGGER, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(LED, OUTPUT);

  myservo.attach(9);  // attaches the servo on pin 9 to the servo object

  // fuzzy sets
  // Water depth (0-400)
  FuzzySet *lowDepth = new FuzzySet(0, 0, 0, 120); 
  FuzzySet *medDepth = new FuzzySet(60, 120, 120, 180);
  FuzzySet *bigDepth = new FuzzySet(120, 180, 180, 240);
  FuzzySet *verybigDepth = new FuzzySet(180, 240, 400, 400);
  
  // ldr (Potentiometer)
  // Illumination ldr (0-100000 lux after conversion)
  FuzzySet *lowldr = new FuzzySet(0, 0, 0, 100);
  FuzzySet *medldr = new FuzzySet(60, 300, 500, 700);
  FuzzySet *highldr = new FuzzySet(500 ,700, 10000, 10000);

  // Water quality
  FuzzySet *badQ = new FuzzySet(0, 0, 0, 100);
  FuzzySet *medQ = new FuzzySet(60, 200, 500, 700);
  FuzzySet *highQ = new FuzzySet(400 ,700, 1015, 10000);

  // Adequacy of water conditions (index)
  FuzzySet *bad = new FuzzySet(0, 0, 0, 0);
  FuzzySet *medium = new FuzzySet(0, 0, 10, 40);
  FuzzySet *good = new FuzzySet(22, 40, 62, 80);
  FuzzySet *verygood = new FuzzySet(62, 92, 600, 600);

  // variables
  // variable water depth with universe 0-400 as input
  FuzzyInput *depth = new FuzzyInput(1);
  depth->addFuzzySet(lowDepth);
  depth->addFuzzySet(medDepth);
  depth->addFuzzySet(bigDepth);
  depth->addFuzzySet(verybigDepth);
  fuzzy->addFuzzyInput(depth);

  // variable ldr with universe 0-1015 as input
  FuzzyInput *ldr = new FuzzyInput(2);
  ldr->addFuzzySet(lowldr);
  ldr->addFuzzySet(medldr);
  ldr->addFuzzySet(highldr);
  fuzzy->addFuzzyInput(ldr); 

  // variable water quality with universe 0-1015 as input
  FuzzyInput *waterQ = new FuzzyInput(3);
  waterQ->addFuzzySet(badQ);
  waterQ->addFuzzySet(medQ);
  waterQ->addFuzzySet(highQ);
  fuzzy->addFuzzyInput(waterQ);


  // variable adequacy with universe 0-255 as output
  FuzzyOutput *adequacy = new FuzzyOutput(1);
  adequacy->addFuzzySet(bad);
  adequacy->addFuzzySet(medium);
  adequacy->addFuzzySet(good);
  adequacy->addFuzzySet(verygood);
  fuzzy->addFuzzyOutput(adequacy);

  // Rule 1
  // IF depth is medium & ldr is high & waterQ is high THEN adequacy is VERY GOOD
  FuzzyRuleAntecedent *ifDepthMedANDLdrHighANDWaterQHigh1 = new FuzzyRuleAntecedent();
  ifDepthMedANDLdrHighANDWaterQHigh1->joinWithAND(medDepth, highldr);
  FuzzyRuleAntecedent *ifDepthMedANDLdrHighANDWaterQHigh2 = new FuzzyRuleAntecedent();
  ifDepthMedANDLdrHighANDWaterQHigh2->joinWithAND(ifDepthMedANDLdrHighANDWaterQHigh1, highQ);
  FuzzyRuleConsequent *thenAdequacyVeryGood = new FuzzyRuleConsequent();
  thenAdequacyVeryGood->addOutput(verygood);
  FuzzyRule *fuzzyRule1 = new FuzzyRule(1, ifDepthMedANDLdrHighANDWaterQHigh2, thenAdequacyVeryGood);
  fuzzy->addFuzzyRule(fuzzyRule1);

  // Rule 2
  // IF depth is medium & ldr is med & waterQ is high THEN adequacy is GOOD
  FuzzyRuleAntecedent *ifDepthMedANDLdrMedANDWaterQHigh1 = new FuzzyRuleAntecedent();
  ifDepthMedANDLdrMedANDWaterQHigh1->joinWithAND(medDepth, medldr);
  FuzzyRuleAntecedent *ifDepthMedANDLdrMedANDWaterQHigh2 = new FuzzyRuleAntecedent();
  ifDepthMedANDLdrMedANDWaterQHigh2->joinWithAND(ifDepthMedANDLdrMedANDWaterQHigh1, highQ);
  FuzzyRuleConsequent *thenAdequacyGood = new FuzzyRuleConsequent();
  thenAdequacyGood->addOutput(good);
  FuzzyRule *fuzzyRule2 = new FuzzyRule(2, ifDepthMedANDLdrMedANDWaterQHigh2, thenAdequacyGood);
  fuzzy->addFuzzyRule(fuzzyRule2);

  // Rule 3
  // IF waterQ is bad THEN adequacy is BAD
  FuzzyRuleAntecedent *ifWaterQBad= new FuzzyRuleAntecedent();
  ifWaterQBad->joinSingle(badQ);
  FuzzyRuleConsequent *thenAdequacyBad = new FuzzyRuleConsequent();
  thenAdequacyBad->addOutput(bad);
  FuzzyRule *fuzzyRule3 = new FuzzyRule(3, ifWaterQBad, thenAdequacyBad);
  fuzzy->addFuzzyRule(fuzzyRule3);

  // Rule 4
  // IF depth is low THEN adequacy is BAD
  FuzzyRuleAntecedent *ifDepthIsLow= new FuzzyRuleAntecedent();
  ifDepthIsLow->joinSingle(lowDepth);
  FuzzyRule *fuzzyRule4 = new FuzzyRule(4, ifDepthIsLow, thenAdequacyBad);
  fuzzy->addFuzzyRule(fuzzyRule4);

  // Rule 5
  // IF depth is big & ldr is high & waterQ is medium THEN adequacy is GOOD
  FuzzyRuleAntecedent *ifDepthBigANDLdrHighANDWaterQMed1 = new FuzzyRuleAntecedent();
  ifDepthBigANDLdrHighANDWaterQMed1->joinWithAND(bigDepth, highldr);
  FuzzyRuleAntecedent *ifDepthBigANDLdrHighANDWaterQMed2 = new FuzzyRuleAntecedent();
  ifDepthBigANDLdrHighANDWaterQMed2->joinWithAND(ifDepthBigANDLdrHighANDWaterQMed1, medQ);
  FuzzyRule *fuzzyRule5 = new FuzzyRule(5, ifDepthMedANDLdrMedANDWaterQHigh2, thenAdequacyGood);
  fuzzy->addFuzzyRule(fuzzyRule5);

}

// returns the distance
int distance() {
  digitalWrite(TRIGGER, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER, LOW);
  long pulse = pulseIn(ECHO, HIGH) / 2;
  return pulse * 10 / 292;
}

// returns the brightness by making a conversion to luxes
int brightness() {
  int analogValue = analogRead(LDR);
  float voltage = analogValue / 1024. * 5;
  float resistance = 2000 * voltage / (1 - voltage / 5);
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA) / resistance, (1 / GAMMA));
  return lux;
}


void loop() {
  // get depth, light and waterQ
  int depth = distance();
  int ldr = brightness();
  int waterQ = analogRead(slidePotPin);

  // fuzzyfication
  fuzzy->setInput(1, depth); // water depth as fuzzy input 1
  fuzzy->setInput(2, ldr); // light as fuzzy input 2 (ldr)
  fuzzy->setInput(3, waterQ); // waterQ as fuzzy input 3
  fuzzy->fuzzify();

  // defuzzyfication
  int output = fuzzy->defuzzify(1); // defuzzify fuzzy output 1 (adequacy)
  
  
  Serial.print("Depth: ");
  Serial.print(depth);
  Serial.print(" Light: ");
  Serial.print(ldr);
  Serial.print(" WaterQ: ");
  Serial.print(waterQ);
  Serial.print(" => Adequacy Index: ");
  Serial.print(output);
  Serial.println();

  pos = map(output, 0, 100, 0, 180);
  myservo.write(pos);

  delay(100);
  
}