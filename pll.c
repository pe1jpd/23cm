/*
 * Routines for setting the PLL frequency. And controlling the 
 * Receiver/transmitter.
 * 
 * The PLL is working as follows:  fvco = (P*B + A)*REFin/R
 * 
 * where
 *  fvco = output frequency of external voltage controlled oscillator (VCO).
 *     P = preset modulus of dual-modulus prescaler. (16 in our case)
 *     B = preset divide ratio of binary 13-bit counter (3 to 8191).
 *     A = preset divide ratio of binary 6-bit swallow counter (0 to 63).
 * REFin = output frequency of the external reference frequency oscillator (12.8 MHz)
 *     R = preset divide ratio of binary 14-bit programmable reference counter (1 to 16,383).
 *         The devide ratio is chosen to match the step/raster size e.g 25000, but this is arbitrary.
 * 
 * If P and R are known, we need to do the following calculations to get B and A from fvco:
 * B = (fvco/R) / P (note: int, so truncated at decimal point)
 * A = (fvco/R) % P (the remainer to do without prescaler P)
 * 
 * To set fvco to 1298.375 MHz, having P = 16 and R = 25000 we need to set A and B to:
 * B = (1298375000 / 25000) / 16 = 3245
 * A = (1298375000 / 25000) % 16 = 15
 */

const unsigned long IF = 69300000; // Intermediate Frequency of receiver in Hz.
const unsigned long minTxFreq = 1240000000; // Lower band limit
const unsigned long maxTxFreq = 1300000000; // Upper band limit

const int repeaterShift[] = {-28, -6, 0, 6, 28 };

/*
 * Initialize the PLL using the "Counter Reset Method" as
 * described on page 14 of the datasheet, ADF4113HV.pdf
 */
void setupPLL() {
  
  pinMode(PLL_DATA,OUTPUT);
  pinMode(PLL_CLK, OUTPUT);
  pinMode(PLL_LE,  OUTPUT);

  digitalWrite(PLL_DATA,LOW);
  digitalWrite(PLL_CLK, LOW);
  digitalWrite(PLL_LE,  LOW);

  // Step 2, Conduct a function latch load with F1 bit set.
  //
  // Set function latch (bits 0,1):
  // - Hold R/A/B counters on reset (F1, bit 2)
  // - Normal power                 (bit 3)
  // - Three state muxout           (bit 4,5,6)
  // - Positive Phase detection     (bit 7)
  // - Three-state Charge pump      (bit 8)
  // - High current Charge pump     (bits 15,16,17)
  // - Prescaler 16/17              (bits 22,23)
  unsigned long reg = 0x438086; 
  writePLL(reg);
    
  // Step 3, Conduct an R counter load.
  //
  // Set reference counter latch     (bit 0,1):
  // 7.2nS anti-backlash pulse width (bit 16,17)
  // Devide ratio                    (bits 2 to 15)
  reg  = 0x020000; 
  reg += ((tcxoRefHz/rasterHz) << 2) & 0x00fffa; // Calculate devide ratio, mask those bits for safety
  writePLL(reg);

  // Step 4, Conduct an AB counter load (set any frequency)
  setVCOFreq(1299000000);

  // Step 5, conduct a function latch load with F1 bit cleared
  //
  // Set function latch (bits 0,1):
  // - Normal R/A/B counters on reset (F1, bit 2)
  // - Normal power                   (bit 3)
  // - Three state muxout             (bit 4,5,6)
  // - Positive Phase detection       (bit 7)
  // - Three-state Charge pump        (bit 8)
  // - High current Charge pump       (bits 15,16,17)
  // - Prescaler 16/17                (bits 22,23)
  reg = 0x438082;
  writePLL(reg);
}

void transmit(){
  digitalWrite(MUTE, true);
  setTxFreq(rxFreqHz + repeaterShift[repeaterShiftIndex] * 1000000);
  ctcssTx();

  // Done, clean up display and switch to Rx.
  lcd.setCursor(0,1);
  lcd.print("                ");
  setRxFreq(rxFreqHz);
}

/*
 * Sets the transmission frequency.
 * Call this when switching from Rx to Tx
 */
void setTxFreq(unsigned long txfreq){
  
  displayFrequency(txfreq);
  setVCOFreq(txfreq);
  digitalWrite(TX_ON, HIGH);

  lcd.setCursor(14,1);
  lcd.print("TX");
  lcd.setCursor(0,1);
  lcd.print("Tone ");
  printCTCSS();  
}

/*
 * Sets the reception frequency.
 * Call this when switching from Tx to Rx
 */
void setRxFreq(unsigned long rxfreq){
  digitalWrite(TX_ON, LOW); 
  displayFrequency(rxfreq);
  setVCOFreq(rxfreq - IF);
}

/*
 * Sets the PLL A/B counters so that the PLL sets the output frequency
 * of the VCO to the given frequency.
 */
void setVCOFreq(unsigned long freq) {
  
  // At which raster slot is the given frequency
  unsigned long channel = freq/rasterHz;

  unsigned long B = (channel / 16) & 0x1fff; // Calculate the 13 bits in B
  unsigned long A = (channel % 16) & 0x3f;   // Calculate the  7 bits in A

  unsigned long reg = 0x000001; // Load AB Counter Latch
  reg += (B << 8); // Load B in bits 8-13
  reg += (A << 2); // Load A in bits 2-7
    
  writePLL(reg);
}

/*
 * Sends the lower 3 bytes in the given long,
 * most significant bit first.
 * 
 * No delays, as the CPU clock is 16Mhz. 
 * See page 15: The maximum allowable serial clock rate is 20 MHz. 
 * This means that the maximum update rate possible for the device 
 * is 833 kHz, or one update every 1.2 µs. This rate is more than 
 * adequate for systems that have typical lock times in the 
 * hundreds of microseconds.
 */
void writePLL(unsigned long pll_word) {
  for (int i = 0; i < 24; i++) {
    boolean msb = (pll_word & 0x800000) ? true : false;
    digitalWrite(PLL_DATA, msb);
    digitalWrite(PLL_CLK,HIGH);
    digitalWrite(PLL_CLK,LOW);
    pll_word <<= 1;
  }

  digitalWrite(PLL_LE,HIGH);
  digitalWrite(PLL_LE,LOW);
}

void printRepeaterShift() {
  int shift = repeaterShift[repeaterShiftIndex];
  if (shift < 10 && shift > -10) {
    lcd.print(" "); 
  }
  if (repeaterShift[repeaterShiftIndex] == 0) {
    lcd.print(" ");
  }
  if (repeaterShift[repeaterShiftIndex] > 0) {
    lcd.print("+");
  }
  lcd.print(shift);
  lcd.print(" Hz");
}
