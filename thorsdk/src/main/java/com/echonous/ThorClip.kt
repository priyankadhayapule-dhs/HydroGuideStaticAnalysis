package com.echonous


import com.echonous.ai.Tensor
import com.echonous.hardware.ultrasound.ThorError
import java.nio.ByteBuffer
import java.nio.ByteOrder

object ThorClip {
    fun getScanConvertedBModeFrames(file: String, width: Int, height: Int): List<Tensor> {
        val (statusCode, buffer) = nativeGetScanConvertedBModeFrames(file, width, height)
        val status = ThorError.fromCode(statusCode)
        if (!status.isOK) {
            throw RuntimeException(status.description)
        }
        buffer.order(ByteOrder.nativeOrder())
        buffer.position(0)
        buffer.limit(buffer.capacity())
        val floatBuffer = buffer.asFloatBuffer()
        val frameSize = width * height
        check(floatBuffer.remaining() % frameSize == 0) { "Buffer size is not a multiple of frame size" }
        val frameCount = floatBuffer.remaining() / frameSize
        val frames = (0 until frameCount).map {
            // Unfortunately, ByteBuffer.slice(Int, Int) is API 34, so use slice based off position/capacity
            floatBuffer.position(it * frameSize)
            floatBuffer.limit(it * frameSize + frameSize)
            val frameData = floatBuffer.slice()
            Tensor(intArrayOf(1, 1, height, width), frameData)
        }
        return frames
    }

    init {
        System.loadLibrary("Ultrasound")
        nativeInit()
    }

    private external fun nativeInit()

    private data class ConvertResult(val status: Int, val buffer: ByteBuffer)
    private external fun nativeGetScanConvertedBModeFrames(file: String, width: Int, height: Int): ConvertResult
}