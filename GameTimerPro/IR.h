
#ifndef IR_H
#define IR_H

#if defined(ARDUINO) && (ARDUINO >= 100)
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

// Sparkfun COM-11759
#define IR_POWER 0x10EFD827
#define IR_A 0x10EFF807
#define IR_B 0x10EF7887
#define IR_C 0x10EF58A7
#define IR_UP 0x10EFA05F
#define IR_DOWN 0x10EF00FF
#define IR_LEFT 0x10EF10EF
#define IR_RIGHT 0x10EF807F
#define IR_SELECT 0x10EF20DF

// Sparkfun COM-14865
#define IR_POWER2 0x00FF629D
#define IR_A2 0x00FF22DD
#define IR_B2 0x00FF02FD
#define IR_C2 0x00FFC23D
#define IR_UP2 0x00FF9867
#define IR_DOWN2 0x00FF38C7
#define IR_LEFT2 0x00FF30CF
#define IR_RIGHT2 0x00FF7A85
#define IR_SELECT2 0x00FF18E7

//------------------------------------------------------------------------------
// This handles definition and access to global variables
//
#ifdef IR_GLOBAL
#	define EXTERN
#else
#	define EXTERN extern
#endif

//------------------------------------------------------------------------------
// Information for the Interrupt Service Routine
//
#define RAWBUF  101  // Maximum length of raw duration buffer

typedef
	struct {
		// The fields are ordered to reduce memory over caused by struct-padding
		uint8_t       rcvstate;        // State Machine state
		uint8_t       recvpin;         // Pin connected to IR data from detector
		uint8_t       rawlen;          // counter of entries in rawbuf
		unsigned int  timer;           // State timer, counts 50uS ticks.
		unsigned int  rawbuf[RAWBUF];  // raw data
		uint8_t       overflow;        // Raw buffer overflow occurred
	}
irparams_t;

// ISR State-Machine : Receiver States
#define STATE_IDLE      2
#define STATE_MARK      3
#define STATE_SPACE     4
#define STATE_STOP      5
#define STATE_OVERFLOW  6

// Allow all parts of the code access to the ISR data
// NB. The data can be changed by the ISR at any time, even mid-function
// Therefore we declare it as "volatile" to stop the compiler/CPU caching it
EXTERN volatile irparams_t  irparams;

//------------------------------------------------------------------------------
// CPU Frequency
//
#ifdef F_CPU
#	define SYSCLOCK  F_CPU     // main Arduino clock
#else
#	define SYSCLOCK  16000000  // main Arduino clock
#endif

//------------------------------------------------------------------------------
// Defines for setting and clearing register bits
//
#ifndef cbi
#	define cbi(sfr, bit)  (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#	define sbi(sfr, bit)  (_SFR_BYTE(sfr) |= _BV(bit))
#endif

//------------------------------------------------------------------------------
// Pulse parms are ((X*50)-100) for the Mark and ((X*50)+100) for the Space.
// First MARK is the one after the long gap
// Pulse parameters in uSec
//

// Due to sensor lag, when received, Marks  tend to be 100us too long and
//                                   Spaces tend to be 100us too short
#define MARK_EXCESS    100

// microseconds per clock interrupt tick
#define USECPERTICK    50

// Upper and Lower percentage tolerances in measurements
#define TOLERANCE       25
#define LTOL            (1.0 - (TOLERANCE/100.))
#define UTOL            (1.0 + (TOLERANCE/100.))

// Minimum gap between IR transmissions
#define _GAP            5000
#define GAP_TICKS       (_GAP/USECPERTICK)

#define TICKS_LOW(us)   ((int)(((us)*LTOL/USECPERTICK)))
#define TICKS_HIGH(us)  ((int)(((us)*UTOL/USECPERTICK + 1)))

//------------------------------------------------------------------------------
// IR detector output is active low
//
#define MARK   0
#define SPACE  1

#define TIMER_RESET
#define TIMER_ENABLE_PWM    (TCCR2A |= _BV(COM2B1))
#define TIMER_DISABLE_PWM   (TCCR2A &= ~(_BV(COM2B1)))
#define TIMER_ENABLE_INTR   (TIMSK2 = _BV(OCIE2A))
#define TIMER_DISABLE_INTR  (TIMSK2 = 0)
#define TIMER_INTR_NAME     TIMER2_COMPA_vect

#define TIMER_CONFIG_KHZ(val) ({ \
	const uint8_t pwmval = SYSCLOCK / 2000 / (val); \
	TCCR2A               = _BV(WGM20); \
	TCCR2B               = _BV(WGM22) | _BV(CS20); \
	OCR2A                = pwmval; \
	OCR2B                = pwmval / 3; \
})

#define TIMER_COUNT_TOP  (SYSCLOCK * USECPERTICK / 1000000)

//-----------------
#if (TIMER_COUNT_TOP < 256)
#define TIMER_CONFIG_NORMAL() ({ \
		TCCR2A = _BV(WGM21); \
		TCCR2B = _BV(CS20); \
		OCR2A  = TIMER_COUNT_TOP; \
		TCNT2  = 0; \
	})
#else
#define TIMER_CONFIG_NORMAL() ({ \
		TCCR2A = _BV(WGM21); \
		TCCR2B = _BV(CS21); \
		OCR2A  = TIMER_COUNT_TOP / 8; \
		TCNT2  = 0; \
	})
#endif

#define TIMER_PWM_PIN  3



//------------------------------------------------------------------------------
// An enumerated list of all supported formats
// You do NOT need to remove entries from this list when disabling protocols!
//
typedef
	enum {
		UNKNOWN      = -1,
		UNUSED       =  0,
		RC5,
		RC6,
		NEC,
		SONY,
		PANASONIC,
		JVC,
		SAMSUNG,
		WHYNTER,
		AIWA_RC_T501,
		LG,
		SANYO,
		MITSUBISHI,
		DISH,
		SHARP,
		DENON,
		PRONTO,
	}
decode_type_t;

//------------------------------------------------------------------------------
// Set DEBUG to 1 for lots of lovely debug output
//
#define DEBUG  0

//------------------------------------------------------------------------------
// Debug directives
//
#if DEBUG
#	define DBG_PRINT(...)    Serial.print(__VA_ARGS__)
#	define DBG_PRINTLN(...)  Serial.println(__VA_ARGS__)
#else
#	define DBG_PRINT(...)
#	define DBG_PRINTLN(...)
#endif

//------------------------------------------------------------------------------
// Mark & Space matching functions
//
int  MATCH       (int measured, int desired) ;
int  MATCH_MARK  (int measured_ticks, int desired_us) ;
int  MATCH_SPACE (int measured_ticks, int desired_us) ;

// NEC definitions
#define NEC_BITS          32
#define NEC_HDR_MARK    9000
#define NEC_HDR_SPACE   4500
#define NEC_BIT_MARK     560
#define NEC_ONE_SPACE   1690
#define NEC_ZERO_SPACE   560
#define NEC_RPT_SPACE   2250

//------------------------------------------------------------------------------
// Results returned from the decoder
//
class ir_results
{
	public:
		decode_type_t          decode_type;  // UNKNOWN, NEC, SONY, RC5, ...
		unsigned int           address;      // Used by Panasonic & Sharp [16-bits]
		unsigned long          value;        // Decoded value [max 32-bits]
		int                    bits;         // Number of bits in decoded value
		volatile unsigned int  *rawbuf;      // Raw intervals in 50uS ticks
		int                    rawlen;       // Number of records in rawbuf
		int                    overflow;     // true iff IR raw code too long
};


//------------------------------------------------------------------------------
// Decoded value for NEC when a repeat code is received
//
#define REPEAT 0xFFFFFFFF

//------------------------------------------------------------------------------
// Main class for receiving IR
//
class IR
{
	public:
		IR();

		unsigned long getIRCommand();
		int   decode(ir_results *results) ;
		void  begin() ;
		bool  isIdle() ;
		void  resume() ;

	private:
		long  decodeHash (ir_results *results) ;
		int   compare    (unsigned int oldval, unsigned int newval) ;

		//......................................................................
		bool  decodeNEC        (ir_results *results) ;
} ;

#endif
