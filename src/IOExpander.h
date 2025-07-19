
#ifndef ARDUINOPROJECTMODULE_IOEXPANDER_H
#define ARDUINOPROJECTMODULE_IOEXPANDER_H

#include <avr/interrupt.h>
#include <Arduino.h>
#include "CIOExpander.h"
#include "ByteStream.h"

class IOExpander : protected CIOExpander {

public:
    explicit IOExpander() {
        // set config + unused config
        fStepCallback = iox_step_done;
    }

    void init(uint8_t addrOption, uint8_t configOption = 0xff, uint8_t dataOption = 0x00) {
        iox_init(XL9535_BASE_ADDRESS + (addrOption & XL9535_OPTIONAL_ADDRESS_MASK), TILT_CONFIGURATION | ((uint16_t) configOption << 8) & TILT_UNUSED_IO, TILT_INVERT_OUT | ((uint16_t) dataOption << 8) & TILT_UNUSED_IO);
    }

    inline void setLED(uint8_t ledColor) {
        ciox_led_color(this, ledColor);
    }

    inline ByteStream *requestInputs() {
        return (ByteStream *) ciox_in(this);
    }

    NO_DISCARD inline uint8_t haveChangedInputs() const {
        return lastInputs != inputs && (flags & IOX_FLAGS_VALID_INPUTS);
    }

    NO_DISCARD inline uint8_t haveLatestInputs() const {
        return flags & IOX_FLAGS_LATEST_INPUTS;
    }

    NO_DISCARD inline uint16_t getInputs() const {
        return inputs;
    }

    NO_DISCARD inline uint16_t getLastInputs() const {
        return inputs;
    }

    inline void clearChangedInputs() {
        lastInputs = inputs;
    }

    /**
     * Step
     * @param ccw  step ccw, else cw
     */
    inline ByteStream *step(int8_t ccw) {
        return (ByteStream *)ciox_step(this, ccw);
    }

    NO_DISCARD inline uint8_t isStepping() const {
        return flags & IOX_FLAGS_STEPPING;
    }

    /**
     * Callback to schedule next step after completion of current step, if still have pending steps
     * or just handle last step sent, possibly turn off motor en after a delay
     *
     * NOTE: request start time can be obtained from twiint_request_start_time, end time is micros().
     */
    virtual void stepDone(ByteStream *pStream) = 0;

protected:

    // IMPORTANT: called from TWI interrupt via request done
    static void iox_step_done(const CByteStream_t *pStream, CIOExpander_t *pIox) {
        ((IOExpander *) pIox)->flags |= IOX_FLAGS_STEPPING;
        ((IOExpander *) pIox)->stepDone((ByteStream *)pStream);
    }
};

#endif //ARDUINOPROJECTMODULE_IOEXPANDER_H
