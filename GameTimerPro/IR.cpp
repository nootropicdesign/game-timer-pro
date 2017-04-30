
#include <avr/interrupt.h>

#include "Hardware.h"
// Defining IR_GLOBAL here allows us to declare the instantiation of global variables
#define IR_GLOBAL
#	include "IR.h"
#undef IR_GLOBAL


ir_results results;


int  MATCH (int measured,  int desired)
{
 	DBG_PRINT(F("Testing: "));
 	DBG_PRINT(TICKS_LOW(desired), DEC);
 	DBG_PRINT(F(" <= "));
 	DBG_PRINT(measured, DEC);
 	DBG_PRINT(F(" <= "));
 	DBG_PRINT(TICKS_HIGH(desired), DEC);

  bool passed = ((measured >= TICKS_LOW(desired)) && (measured <= TICKS_HIGH(desired)));
  if (passed)
    DBG_PRINTLN(F("?; passed"));
  else
    DBG_PRINTLN(F("?; FAILED")); 
 	return passed;
}

//+========================================================
// Due to sensor lag, when received, Marks tend to be 100us too long
//
int  MATCH_MARK (int measured_ticks,  int desired_us)
{
	DBG_PRINT(F("Testing mark (actual vs desired): "));
	DBG_PRINT(measured_ticks * USECPERTICK, DEC);
	DBG_PRINT(F("us vs "));
	DBG_PRINT(desired_us, DEC);
	DBG_PRINT("us"); 
	DBG_PRINT(": ");
	DBG_PRINT(TICKS_LOW(desired_us + MARK_EXCESS) * USECPERTICK, DEC);
	DBG_PRINT(F(" <= "));
	DBG_PRINT(measured_ticks * USECPERTICK, DEC);
	DBG_PRINT(F(" <= "));
	DBG_PRINT(TICKS_HIGH(desired_us + MARK_EXCESS) * USECPERTICK, DEC);

  bool passed = ((measured_ticks >= TICKS_LOW (desired_us + MARK_EXCESS))
                && (measured_ticks <= TICKS_HIGH(desired_us + MARK_EXCESS)));
  if (passed)
    DBG_PRINTLN(F("?; passed"));
  else
    DBG_PRINTLN(F("?; FAILED")); 
 	return passed;
}

//+========================================================
// Due to sensor lag, when received, Spaces tend to be 100us too short
//
int  MATCH_SPACE (int measured_ticks,  int desired_us)
{
	DBG_PRINT(F("Testing space (actual vs desired): "));
	DBG_PRINT(measured_ticks * USECPERTICK, DEC);
	DBG_PRINT(F("us vs "));
	DBG_PRINT(desired_us, DEC);
	DBG_PRINT("us"); 
	DBG_PRINT(": ");
	DBG_PRINT(TICKS_LOW(desired_us - MARK_EXCESS) * USECPERTICK, DEC);
	DBG_PRINT(F(" <= "));
	DBG_PRINT(measured_ticks * USECPERTICK, DEC);
	DBG_PRINT(F(" <= "));
	DBG_PRINT(TICKS_HIGH(desired_us - MARK_EXCESS) * USECPERTICK, DEC);

  bool passed = ((measured_ticks >= TICKS_LOW (desired_us - MARK_EXCESS))
                && (measured_ticks <= TICKS_HIGH(desired_us - MARK_EXCESS)));
  if (passed)
    DBG_PRINTLN(F("?; passed"));
  else
    DBG_PRINTLN(F("?; FAILED")); 
 	return passed;
}

//+=============================================================================
// Interrupt Service Routine - Fires every 50uS
// TIMER2 interrupt code to collect raw data.
// Widths of alternating SPACE, MARK are recorded in rawbuf.
// Recorded in ticks of 50uS [microseconds, 0.000050 seconds]
// 'rawlen' counts the number of entries recorded so far.
// First entry is the SPACE between transmissions.
// As soon as a the first [SPACE] entry gets long:
//   Ready is set; State switches to IDLE; Timing of SPACE continues.
// As soon as first MARK arrives:
//   Gap width is recorded; Ready is cleared; New logging starts
//
ISR (TIMER_INTR_NAME)
{
	TIMER_RESET;

	// Read if IR Receiver -> SPACE [xmt LED off] or a MARK [xmt LED on]
	// digitalRead() is very slow. Optimisation is possible, but makes the code unportable
	uint8_t  irdata = (uint8_t)digitalRead(irparams.recvpin);

	irparams.timer++;  // One more 50uS tick
	if (irparams.rawlen >= RAWBUF)  irparams.rcvstate = STATE_OVERFLOW ;  // Buffer overflow

	switch(irparams.rcvstate) {
		//......................................................................
		case STATE_IDLE: // In the middle of a gap
			if (irdata == MARK) {
				if (irparams.timer < GAP_TICKS)  {  // Not big enough to be a gap.
					irparams.timer = 0;

				} else {
					// Gap just ended; Record duration; Start recording transmission
					irparams.overflow                  = false;
					irparams.rawlen                    = 0;
					irparams.rawbuf[irparams.rawlen++] = irparams.timer;
					irparams.timer                     = 0;
					irparams.rcvstate                  = STATE_MARK;
				}
			}
			break;
		//......................................................................
		case STATE_MARK:  // Timing Mark
			if (irdata == SPACE) {   // Mark ended; Record time
				irparams.rawbuf[irparams.rawlen++] = irparams.timer;
				irparams.timer                     = 0;
				irparams.rcvstate                  = STATE_SPACE;
			}
			break;
		//......................................................................
		case STATE_SPACE:  // Timing Space
			if (irdata == MARK) {  // Space just ended; Record time
				irparams.rawbuf[irparams.rawlen++] = irparams.timer;
				irparams.timer                     = 0;
				irparams.rcvstate                  = STATE_MARK;

			} else if (irparams.timer > GAP_TICKS) {  // Space
					// A long Space, indicates gap between codes
					// Flag the current code as ready for processing
					// Switch to STOP
					// Don't reset timer; keep counting Space width
					irparams.rcvstate = STATE_STOP;
			}
			break;
		//......................................................................
		case STATE_STOP:  // Waiting; Measuring Gap
		 	if (irdata == MARK)  irparams.timer = 0 ;  // Reset gap timer
		 	break;
		//......................................................................
		case STATE_OVERFLOW:  // Flag up a read overflow; Stop the State Machine
			irparams.overflow = true;
			irparams.rcvstate = STATE_STOP;
		 	break;
	}

}

//+=============================================================================
// Decodes the received IR message
// Returns 0 if no data ready, 1 if data ready.
// Results of decoding are stored in results
//
int  IR::decode (ir_results *results)
{
	results->rawbuf   = irparams.rawbuf;
	results->rawlen   = irparams.rawlen;

	results->overflow = irparams.overflow;

	if (irparams.rcvstate != STATE_STOP)  return false ;

	DBG_PRINTLN("Attempting NEC decode");
	if (decodeNEC(results))  return true ;

	// decodeHash returns a hash on any input.
	// Thus, it needs to be last in the list.
	// If you add any decodes, add them before this.
	if (decodeHash(results))  return true ;

	// Throw away and start over
	resume();
	return false;
}

//+=============================================================================
IR::IR()
{
	irparams.recvpin = IR_PIN;
}

void  IR::begin()
{
	cli();
	// Setup pulse clock timer interrupt
	// Prescale /8 (16M/8 = 0.5 microseconds per tick)
	// Therefore, the timer interval can range from 0.5 to 128 microseconds
	// Depending on the reset value (255 to 0)
	TIMER_CONFIG_NORMAL();

	// Timer2 Overflow Interrupt Enable
	TIMER_ENABLE_INTR;

	TIMER_RESET;

	sei();  // enable interrupts

	// Initialize state machine variables
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;

	// Set pin modes
	pinMode(irparams.recvpin, INPUT);
}

unsigned long IR::getIRCommand() {
  if (decode(&results)) {
  	resume();
  	return results.value;
  } else {
  	return 0;
  }
}

//+=============================================================================
// Return if receiving new IR signals
// 
bool  IR::isIdle ( ) 
{
 return (irparams.rcvstate == STATE_IDLE || irparams.rcvstate == STATE_STOP) ? true : false;
}
//+=============================================================================
// Restart the ISR state machine
//
void  IR::resume ( )
{
	irparams.rcvstate = STATE_IDLE;
	irparams.rawlen = 0;
}

//+=============================================================================
// hashdecode - decode an arbitrary IR code.
// Instead of decoding using a standard encoding scheme
// (e.g. Sony, NEC, RC5), the code is hashed to a 32-bit value.
//
// The algorithm: look at the sequence of MARK signals, and see if each one
// is shorter (0), the same length (1), or longer (2) than the previous.
// Do the same with the SPACE signals.  Hash the resulting sequence of 0's,
// 1's, and 2's to a 32-bit value.  This will give a unique value for each
// different code (probably), for most code systems.
//
// http://arcfn.com/2010/01/using-arbitrary-remotes-with-arduino.html
//
// Compare two tick values, returning 0 if newval is shorter,
// 1 if newval is equal, and 2 if newval is longer
// Use a tolerance of 20%
//
int  IR::compare (unsigned int oldval,  unsigned int newval)
{
	if      (newval < oldval * .8)  return 0 ;
	else if (oldval < newval * .8)  return 2 ;
	else                            return 1 ;
}

//+=============================================================================
// Use FNV hash algorithm: http://isthe.com/chongo/tech/comp/fnv/#FNV-param
// Converts the raw code values into a 32-bit hash code.
// Hopefully this code is unique for each button.
// This isn't a "real" decoding, just an arbitrary value.
//
#define FNV_PRIME_32 16777619
#define FNV_BASIS_32 2166136261

long  IR::decodeHash (ir_results *results)
{
	long  hash = FNV_BASIS_32;

	// Require at least 6 samples to prevent triggering on noise
	if (results->rawlen < 6)  return false ;

	for (int i = 1;  (i + 2) < results->rawlen;  i++) {
		int value =  compare(results->rawbuf[i], results->rawbuf[i+2]);
		// Add value into the hash
		hash = (hash * FNV_PRIME_32) ^ value;
	}

	results->value       = hash;
	results->bits        = 32;
	results->decode_type = UNKNOWN;

	return true;
}

bool  IR::decodeNEC (ir_results *results)
{
	long  data   = 0;  // We decode in to here; Start with nothing
	int   offset = 1;  // Index in to results; Skip first entry!?

	// Check header "mark"
	if (!MATCH_MARK(results->rawbuf[offset], NEC_HDR_MARK))  return false ;
	offset++;

	// Check for repeat
	if ( (irparams.rawlen == 4)
	    && MATCH_SPACE(results->rawbuf[offset  ], NEC_RPT_SPACE)
	    && MATCH_MARK (results->rawbuf[offset+1], NEC_BIT_MARK )
	   ) {
		results->bits        = 0;
		results->value       = REPEAT;
		results->decode_type = NEC;
		return true;
	}

	// Check we have enough data
	if (irparams.rawlen < (2 * NEC_BITS) + 4)  return false ;

	// Check header "space"
	if (!MATCH_SPACE(results->rawbuf[offset], NEC_HDR_SPACE))  return false ;
	offset++;

	// Build the data
	for (int i = 0;  i < NEC_BITS;  i++) {
		// Check data "mark"
		if (!MATCH_MARK(results->rawbuf[offset], NEC_BIT_MARK))  return false ;
		offset++;
        // Suppend this bit
		if      (MATCH_SPACE(results->rawbuf[offset], NEC_ONE_SPACE ))  data = (data << 1) | 1 ;
		else if (MATCH_SPACE(results->rawbuf[offset], NEC_ZERO_SPACE))  data = (data << 1) | 0 ;
		else                                                            return false ;
		offset++;
	}

	// Success
	results->bits        = NEC_BITS;
	results->value       = data;
	results->decode_type = NEC;

	return true;
}


