package com.echonous.hardware.ultrasound

/**
 * Returns the result of \c block if the original ThorError is OK, else returns the original
 * ThorError.
 */
inline fun ThorError.andThen(block: () -> ThorError): ThorError {
    return if (this.isOK) {
        block()
    } else {
        this
    }
}

/**
 * Returns the original ThorError if it is OK, else returns the result of \c block.
 */
inline fun ThorError.orElse(block: () -> ThorError): ThorError {
    return if (this.isOK) {
        this
    } else {
        block()
    }
}