#define FASTLED_INTERNAL
#include "FastLED.h"
#include "fl/singleton.h"
#include "fl/engine_events.h"
#include "fl/compiler_control.h"
#include "fl/int.h"

/// @file FastLED.cpp
/// Central source file for FastLED, implements the CFastLED class/object

#ifndef MAX_CLED_CONTROLLERS
#ifdef __AVR__
// if mega or leonardo, allow more controllers
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega32U4__)
#define MAX_CLED_CONTROLLERS 16
#else
#define MAX_CLED_CONTROLLERS 8
#endif
#else
#define MAX_CLED_CONTROLLERS 64
#endif  // __AVR__
#endif  // MAX_CLED_CONTROLLERS

#if defined(__SAM3X8E__)
volatile fl::u32 fuckit;
#endif

// Disable to fix build breakage.
// #ifndef FASTLED_DEFINE_WEAK_YEILD_FUNCTION
// #if defined(__AVR_ATtiny13__)
// // Arduino.h also defines this as a weak function on this platform.
// #define FASTLED_DEFINE_WEAK_YEILD_FUNCTION 0
// #else
// #define FASTLED_DEFINE_WEAK_YEILD_FUNCTION 1
// #endif
// #endif

/// Has to be declared outside of any namespaces.
/// Called at program exit when run in a desktop environment. 
/// Extra C definition that some environments may need. 
/// @returns 0 to indicate success
extern "C" __attribute__((weak)) int atexit(void (* /*func*/ )()) { return 0; }

#ifdef FASTLED_NEEDS_YIELD
extern "C" void yield(void) { }
#endif

FASTLED_NAMESPACE_BEGIN

fl::u16 cled_contoller_size() {
	return sizeof(CLEDController);
}

uint8_t get_brightness();

/// Pointer to the matrix object when using the Smart Matrix Library
/// @see https://github.com/pixelmatix/SmartMatrix
void *pSmartMatrix = NULL;

FL_DISABLE_WARNING_PUSH
FL_DISABLE_WARNING_GLOBAL_CONSTRUCTORS

CFastLED FastLED;  // global constructor allowed in this case.

FL_DISABLE_WARNING_POP

CLEDController *CLEDController::m_pHead = NULL;
CLEDController *CLEDController::m_pTail = NULL;
static fl::u32 lastshow = 0;

/// Global frame counter, used for debugging ESP implementations
/// @todo Include in FASTLED_DEBUG_COUNT_FRAME_RETRIES block?
fl::u32 _frame_cnt=0;

/// Global frame retry counter, used for debugging ESP implementations
/// @todo Include in FASTLED_DEBUG_COUNT_FRAME_RETRIES block?
fl::u32 _retry_cnt=0;

// uint32_t CRGB::Squant = ((uint32_t)((__TIME__[4]-'0') * 28))<<16 | ((__TIME__[6]-'0')*50)<<8 | ((__TIME__[7]-'0')*28);

CFastLED::CFastLED() {
	// clear out the array of led controllers
	// m_nControllers = 0;
	m_Scale = 255;
	m_nFPS = 0;
	m_pPowerFunc = NULL;
	m_nPowerData = 0xFFFFFFFF;
	m_nMinMicros = 0;
}

int CFastLED::size() {
	return (*this)[0].size();
}

CRGB* CFastLED::leds() {
	return (*this)[0].leds();
}

CLEDController &CFastLED::addLeds(CLEDController *pLed,
								  struct CRGB *data,
								  int nLedsOrOffset, int nLedsIfOffset) {
	int nOffset = (nLedsIfOffset > 0) ? nLedsOrOffset : 0;
	int nLeds = (nLedsIfOffset > 0) ? nLedsIfOffset : nLedsOrOffset;

	pLed->init();
	pLed->setLeds(data + nOffset, nLeds);
	FastLED.setMaxRefreshRate(pLed->getMaxRefreshRate(),true);
	fl::EngineEvents::onStripAdded(pLed, nLedsOrOffset - nOffset);
	return *pLed;
}

// This is bad code. But it produces the smallest binaries for reasons
// beyond mortal comprehensions.
// Instead of iterating through the link list for onBeginFrame(), beginShowLeds()
// and endShowLeds(): store the pointers in an array and iterate through that.
//
// static uninitialized gControllersData produces the smallest binary on attiny85.
static void* gControllersData[MAX_CLED_CONTROLLERS];

void CFastLED::show(uint8_t scale) {
#ifndef FASTLED_MANUAL_ENGINE_EVENTS
	fl::EngineEvents::onBeginFrame();
#endif
	while(m_nMinMicros && ((micros()-lastshow) < m_nMinMicros));
	lastshow = micros();

	// If we have a function for computing power, use it!
	if(m_pPowerFunc) {
		scale = (*m_pPowerFunc)(scale, m_nPowerData);
	}


	int length = 0;
	CLEDController *pCur = CLEDController::head();

	while(pCur && length < MAX_CLED_CONTROLLERS) {
		if (pCur->getEnabled()) {
			gControllersData[length] = pCur->beginShowLeds(pCur->size());
		} else {
			gControllersData[length] = nullptr;
		}
		length++;
		if (m_nFPS < 100) { pCur->setDither(0); }
		pCur = pCur->next();
	}

	pCur = CLEDController::head();
	for (length = 0; length < MAX_CLED_CONTROLLERS && pCur; length++) {
		if (pCur->getEnabled()) {
			pCur->showLedsInternal(scale);
		}
		pCur = pCur->next();

	}

	length = 0;  // Reset length to 0 and iterate again.
	pCur = CLEDController::head();
	while(pCur && length < MAX_CLED_CONTROLLERS) {
		if (pCur->getEnabled()) {
			pCur->endShowLeds(gControllersData[length]);
		}
		length++;
		pCur = pCur->next();
	}
	countFPS();
	onEndFrame();
#ifndef FASTLED_MANUAL_ENGINE_EVENTS
	fl::EngineEvents::onEndShowLeds();
#endif
}

void CFastLED::onEndFrame() {
	fl::EngineEvents::onEndFrame();
}

int CFastLED::count() {
    int x = 0;
	CLEDController *pCur = CLEDController::head();
	while( pCur) {
        ++x;
		pCur = pCur->next();
	}
    return x;
}

CLEDController & CFastLED::operator[](int x) {
	CLEDController *pCur = CLEDController::head();
	while(x-- && pCur) {
		pCur = pCur->next();
	}
	if(pCur == NULL) {
		return *(CLEDController::head());
	} else {
		return *pCur;
	}
}

void CFastLED::showColor(const struct CRGB & color, uint8_t scale) {
	while(m_nMinMicros && ((micros()-lastshow) < m_nMinMicros));
	lastshow = micros();

	// If we have a function for computing power, use it!
	if(m_pPowerFunc) {
		scale = (*m_pPowerFunc)(scale, m_nPowerData);
	}

	int length = 0;
	CLEDController *pCur = CLEDController::head();
	while(pCur && length < MAX_CLED_CONTROLLERS) {
		if (pCur->getEnabled()) {
			gControllersData[length] = pCur->beginShowLeds(pCur->size());
		} else {
			gControllersData[length] = nullptr;
		}
		length++;
		pCur = pCur->next();
	}

	pCur = CLEDController::head();
	while(pCur && length < MAX_CLED_CONTROLLERS) {
		if(m_nFPS < 100) { pCur->setDither(0); }
		if (pCur->getEnabled()) {
			pCur->showColorInternal(color, scale);
		}
		pCur = pCur->next();
	}

	pCur = CLEDController::head();
	length = 0;  // Reset length to 0 and iterate again.
	while(pCur && length < MAX_CLED_CONTROLLERS) {
		if (pCur->getEnabled()) {
			pCur->endShowLeds(gControllersData[length]);
		}
		length++;
		pCur = pCur->next();
	}
	countFPS();
	onEndFrame();
}

void CFastLED::clear(bool writeData) {
	if(writeData) {
		showColor(CRGB(0,0,0), 0);
	}
    clearData();
}

void CFastLED::clearData() {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->clearLedDataInternal();
		pCur = pCur->next();
	}
}

void CFastLED::delay(unsigned long ms) {
	unsigned long start = millis();
        do {
#ifndef FASTLED_ACCURATE_CLOCK
		// make sure to allow at least one ms to pass to ensure the clock moves
		// forward
		::delay(1);
#endif
		show();
		yield();
	}
	while((millis()-start) < ms);
}

void CFastLED::setTemperature(const struct CRGB & temp) {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setTemperature(temp);
		pCur = pCur->next();
	}
}

void CFastLED::setCorrection(const struct CRGB & correction) {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setCorrection(correction);
		pCur = pCur->next();
	}
}

void CFastLED::setDither(uint8_t ditherMode)  {
	CLEDController *pCur = CLEDController::head();
	while(pCur) {
		pCur->setDither(ditherMode);
		pCur = pCur->next();
	}
}

//
// template<int m, int n> void transpose8(unsigned char A[8], unsigned char B[8]) {
// 	uint32_t x, y, t;
//
// 	// Load the array and pack it into x and y.
//   	y = *(unsigned int*)(A);
// 	x = *(unsigned int*)(A+4);
//
// 	// x = (A[0]<<24)   | (A[m]<<16)   | (A[2*m]<<8) | A[3*m];
// 	// y = (A[4*m]<<24) | (A[5*m]<<16) | (A[6*m]<<8) | A[7*m];
//
        // // pre-transform x
        // t = (x ^ (x >> 7)) & 0x00AA00AA;  x = x ^ t ^ (t << 7);
        // t = (x ^ (x >>14)) & 0x0000CCCC;  x = x ^ t ^ (t <<14);
				//
        // // pre-transform y
        // t = (y ^ (y >> 7)) & 0x00AA00AA;  y = y ^ t ^ (t << 7);
        // t = (y ^ (y >>14)) & 0x0000CCCC;  y = y ^ t ^ (t <<14);
				//
        // // final transform
        // t = (x & 0xF0F0F0F0) | ((y >> 4) & 0x0F0F0F0F);
        // y = ((x << 4) & 0xF0F0F0F0) | (y & 0x0F0F0F0F);
        // x = t;
//
// 	B[7*n] = y; y >>= 8;
// 	B[6*n] = y; y >>= 8;
// 	B[5*n] = y; y >>= 8;
// 	B[4*n] = y;
//
//   B[3*n] = x; x >>= 8;
// 	B[2*n] = x; x >>= 8;
// 	B[n] = x; x >>= 8;
// 	B[0] = x;
// 	// B[0]=x>>24;    B[n]=x>>16;    B[2*n]=x>>8;  B[3*n]=x>>0;
// 	// B[4*n]=y>>24;  B[5*n]=y>>16;  B[6*n]=y>>8;  B[7*n]=y>>0;
// }
//
// void transposeLines(Lines & out, Lines & in) {
// 	transpose8<1,2>(in.bytes, out.bytes);
// 	transpose8<1,2>(in.bytes + 8, out.bytes + 1);
// }


/// Unused value
/// @todo Remove?
extern int noise_min;

/// Unused value
/// @todo Remove?
extern int noise_max;

void CFastLED::countFPS(int nFrames) {
	static int br = 0;
	static fl::u32 lastframe = 0; // millis();

	if(br++ >= nFrames) {
		fl::u32 now = millis();
		now -= lastframe;
		if(now == 0) {
			now = 1; // prevent division by zero below
		}
		m_nFPS = (br * 1000) / now;
		br = 0;
		lastframe = millis();
	}
}

void CFastLED::setMaxRefreshRate(fl::u16 refresh, bool constrain) {
	if(constrain) {
		// if we're constraining, the new value of m_nMinMicros _must_ be higher than previously (because we're only
		// allowed to slow things down if constraining)
		if(refresh > 0) {
			m_nMinMicros = ((1000000 / refresh) > m_nMinMicros) ? (1000000 / refresh) : m_nMinMicros;
		}
	} else if(refresh > 0) {
		m_nMinMicros = 1000000 / refresh;
	} else {
		m_nMinMicros = 0;
	}
}


uint8_t get_brightness() {
	return FastLED.getBrightness();
}



#ifdef NEED_CXX_BITS
namespace __cxxabiv1
{
	#if !defined(ESP8266) && !defined(ESP32)
	extern "C" void __cxa_pure_virtual (void) {}
	#endif

	/* guard variables */

	/* The ABI requires a 64-bit type.  */
	__extension__ typedef int __guard __attribute__((mode(__DI__)));

	extern "C" int __cxa_guard_acquire (__guard *) __attribute__((weak));
	extern "C" void __cxa_guard_release (__guard *) __attribute__((weak));
	extern "C" void __cxa_guard_abort (__guard *) __attribute__((weak));

	extern "C" int __cxa_guard_acquire (__guard *g)
	{
		return !*(char *)(g);
	}

	extern "C" void __cxa_guard_release (__guard *g)
	{
		*(char *)g = 1;
	}

	extern "C" void __cxa_guard_abort (__guard *)
	{

	}
}
#endif

FASTLED_NAMESPACE_END
