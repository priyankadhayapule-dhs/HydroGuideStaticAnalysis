package com.accessvascularinc.hydroguide.mma.ultrasound

import com.echonous.hardware.ultrasound.DauException
import com.echonous.hardware.ultrasound.ThorError

interface ErrorCallback {
    suspend fun onThorError(error: ThorError)
    suspend fun onDauException(dauException: DauException)
    suspend fun onUPSError()
    suspend fun onViewerError(error: ThorError)
    suspend fun onDauOpenError(error: ThorError)
}