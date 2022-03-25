#include <MozziGuts.h>   // at the top of your sketch
#include <Oscil.h>
#include <tables/SIN2048_int8.h>
#include <tables/cos2048_int8.h>
#include <Smooth.h>
#include <AutoMap.h>
#include <LowPassFilter.h>
#include <mozzi_rand.h>
#include <mozzi_midi.h>

#define CONTROL_RATE 64

// globais para implementação midi
int upButtonState;
int lastUpState = LOW;
int downButtonState;
int lastDownState = LOW;
unsigned long lastUpDebounceTime = 0;    
unsigned long lastDownDebounceTime = 0;  
unsigned long debounceDelay = 50; 

//constantes de divisão
const float DIV1023 = 1.0 / 1023.0; // division constant for pot value range
const float DIV10 = 1.0 / 10.0;  

// globais do appegiator
int sequence = 0;               // current sequence playing
int note = 0;                   // current note playing
const int numSequences = 21;    // total number of sequences (must match number of sequences in NOTES array)
const int numNotes = 8;  

//ESCALAS
const int NOTES[numSequences][numNotes] = {
                        {36,48,36,48,36,48,36,48}};                    // up on two



// Declarando o range maximo das frequencias (notas)
const int MIN_NOTA = 0;
const int MAX_NOTA = 440;


// Declarando o range maximo e minimo da sintese fm(modulação de frequencias)
const int MIN_MOD = 10000;
const int MAX_MOD = 1;

//Declarando a intensidade para o automap (efeito inverso: mais luz menos vibração)
const int MIN_INTEN = 1000;
const int MAX_INTEN = 10;

//Low pass filter (Cut off)
int LPF_CUTOFF_MIN = 10;      
int LPF_CUTOFF_MAX = 600; 

AutoMap kMapCarrierFreq(0, 1023, MIN_NOTA, MAX_NOTA);
AutoMap kMapIntensity(0, 1023, MIN_INTEN, MAX_INTEN);
AutoMap kMapMod(0, 1023, MIN_MOD, MAX_MOD);

//Oscilador
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aCarrier(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aCarrier2(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aModulator(SIN2048_DATA);
Oscil<SIN2048_NUM_CELLS, AUDIO_RATE> aModulator2(COS2048_DATA);
Oscil<SIN2048_NUM_CELLS, CONTROL_RATE> kIntensityMod(SIN2048_DATA);
Oscil<COS2048_NUM_CELLS, CONTROL_RATE> kFilterMod(COS2048_DATA);

//Entrada do potenciometro e sensor ldr
const int POT1_PIN = 0; // Volume
const int POT2_PIN = 1;// Frequencia
const int POT4_PIN = 2;
const int LDR1_PIN = 3; // modulação 1
const int LDR2_PIN = 4; // modulação 2
const int POT3_PIN = 5; //Cut off

int mod_ratio = 5; //harmoniSIN
long fm_intensity; // sintese fm provindo do updateControl para updateAudio
LowPassFilter lpf; // Filtro
uint8_t resonance = 10;


// smooth para remover clicks na transição de frequencias
float smoothness = 0.95f;
Smooth<long> aSmoothIntensity(smoothness);


byte volume;


//Controle de rate
 //Hz, powers of 2 are most reliable

void setup() {
  //botoes
  Serial.begin(115200); 
  startMozzi();
  //aCrunchySound.setFreq(2.f);
  kFilterMod.setFreq(1.3f);
}

void updateControl(){
  
  int pot1_value = mozziAnalogRead(POT1_PIN); //leitura do potenciometro volume
  int pot2_value = mozziAnalogRead(POT2_PIN); //leitura da freq
  int pot3_value = mozziAnalogRead(POT4_PIN);
  //Cut off
  //byte cut_off = mozziAnalogRead(POT_PIN)/5;
  lpf.setResonance(128u);
  
  //mapeador carrier Freq
  int carrier_freq = kMapCarrierFreq(pot2_value);
  int carrier_freq2 = kMapCarrierFreq(pot3_value);

  //calculador da modulação
  int mod_freq = carrier_freq * mod_ratio;
  int mod_freq2 = carrier_freq2 * mod_ratio;

  //fm oscilattor freq
  aCarrier.setFreq(carrier_freq);
  aCarrier2.setFreq(carrier_freq2);
  aModulator.setFreq(mod_freq);
  aModulator2.setFreq(mod_freq2);

  volume = pot1_value >> 2; 
  //imprimindo o volume na porta serial POTENCIOMETRO
  Serial.print("volume= ");
  Serial.println((int)volume);
  Serial.print("\t");

  int LDR1_value = mozziAnalogRead(LDR1_PIN); //leitor de luminosidade
  Serial.print("LDR1 = ");
  Serial.print(LDR1_value);
  Serial.print("\t");

  int LDR1_calibrated = kMapIntensity(LDR1_value);
  Serial.print("LDR1_calibrated = ");
  Serial.print(LDR1_calibrated);
  Serial.print("\t"); // prints a tab

  
  // calcula a intensidade do sintese fm
  fm_intensity = ((long)LDR1_calibrated *(kIntensityMod.next()+128))>>8;
  Serial.print("fm_intensity = ");
  Serial.print(fm_intensity);
  Serial.print("\t"); // prints a tab


  // leitor do ldr para ver a volocidade analogica do input
  int LDR2_value = mozziAnalogRead(LDR2_PIN);
  Serial.print("LDR2 = ");
  Serial.print(LDR2_value);
  Serial.print("\t");

  float mod_speed = (float)kMapMod(LDR2_value)/1000;
  Serial.print("   mod_speed = ");
  Serial.print(mod_speed);
  kIntensityMod.setFreq(mod_speed);

  
  int modC_val = mozziAnalogRead(POT3_PIN);
  int modC_freq = LPF_CUTOFF_MIN + (LPF_CUTOFF_MAX *(modC_val * DIV1023));
  
  Serial.println();
}

int updateAudio(){
  long modulation = aSmoothIntensity.next(fm_intensity) * aModulator.next();
  long modulation2 = aSmoothIntensity.next(fm_intensity) * aModulator2.next();
  return (((aCarrier.phMod(modulation)+ aCarrier2.phMod(modulation))* (volume/2)>>8));
  
}

void loop() {
  audioHook();
} 
