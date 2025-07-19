
#ifndef ARDUINOPROJECTMODULE_IOEXPANDER_H
#define ARDUINOPROJECTMODULE_IOEXPANDER_H

#include <avr/interrupt.h>
#include "CIOExpander.h"
#include "StepperRPM.h"

class IOExpander : protected CIOExpander {
protected:
    uint16_t pendingSteps;

#ifdef INCLUDE_STP_MODULE
    StepperRPM stepperRpm;
#endif

public:
    explicit IOExpander(uint8_t addrOption, uint8_t configOption = 0xff, uint8_t dataOption = 0x00) {
        // set config + unused config
        iox_init(XL9535_BASE_ADDRESS + (addrOption & XL9535_OPTIONAL_ADDRESS_MASK), TILT_CONFIGURATION | ((uint16_t) configOption << 8) & TILT_UNUSED_IO, TILT_INVERT_OUT | ((uint16_t) dataOption << 8) & TILT_UNUSED_IO);
        fStepCallback = iox_step_done;
    }

#ifdef INCLUDE_STP_MODULE

    /**
     * Step number of steps
     * @param steps  steps, >0 cw, <0 ccw
     */
    void step(int8_t steps) {
        if (steps) {
            CLI();
            pendingSteps += steps;
            uint8_t ccw = pendingSteps < 0;
            uint8_t startStepping = !isStepping() && pendingSteps;
            SEI();

            if (startStepping) {
                // start stepping
                ciox_step(this, ccw);
            }
        }
    }

    NO_DISCARD inline uint8_t isStepping() const {
        return flags & IOX_FLAGS_STEPPING;
    }

    /**
     * Callback to schedule next step after completion of current step, if still have pending steps
     * or just handle last step sent, possibly turn off motor en after a delay
     *
     * @param havePendingSteps   0 if no more pending steps, else have pending steps
     */
    virtual void stepDone(uint8_t havePendingSteps) = 0;

    void cancelStepping() {
        CLI();
        pendingSteps = 0;
        SEI();
    }

protected:

    // IMPORTANT: called from TWI interrupt via request done
    virtual void ioxStepDone(const CByteStream *pStream) {
        if (pendingSteps < 0) {
            pendingSteps++;
        } else if (pendingSteps > 0) {
            pendingSteps--;
        }
        if (pendingSteps) {
            flags |= IOX_FLAGS_STEPPING;
        }

        stepDone(flags & IOX_FLAGS_STEPPING);
    }

    static void iox_step_done(const CByteStream_t *pStream, CIOExpander_t *pIox) {
        ((IOExpander *) pIox)->ioxStepDone(pStream);
    }

#endif
};

#endif //ARDUINOPROJECTMODULE_IOEXPANDER_H
