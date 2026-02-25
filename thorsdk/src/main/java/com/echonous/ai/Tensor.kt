package com.echonous.ai

import java.io.FileOutputStream
import java.nio.FloatBuffer

/// An N-Dimensional Tensor of floats. \c data can be either direct or indirect.
class Tensor(val shape: IntArray, val data: FloatBuffer = makeFloatBuffer(shape)) {

    val size = shape.fold(1, Int::times)

    init {
        require(shape.isNotEmpty()) { "Shape must not be empty" }
        require(shape.all { it > 0 }) { "Shape cannot contain negative values" }
        val size = shape.fold(1, Int::times)
        require(size == data.capacity()) { "Shape and data size mismatch" }
    }

    fun getRangeForDebug(): Pair<Float, Float> {
        var min = Float.MAX_VALUE
        var max = Float.MIN_VALUE
        for (i in 0 until size) {
            val value = data.get(i)
            min = minOf(min, value)
            max = maxOf(max, value)
        }
        return min to max
    }

    fun savePGM(path: String) {
        val width = shape[2]
        val height = shape[3]
        FileOutputStream(path).use { output ->
            output.write("P5\n".toByteArray())
            output.write("$width $height\n".toByteArray())
            output.write("255\n".toByteArray())
            for (y in 0 until height) {
                for (x in 0 until width) {
                    val value = data.get(y * width + x)
                    output.write((value * 255).toInt())
                }
            }
        }
    }

    companion object {
        fun makeFloatBuffer(shape: IntArray): FloatBuffer {
            require(shape.isNotEmpty()) { "Shape must not be empty" }
            require(shape.all { it > 0 }) { "Shape cannot contain negative values" }
            val size = shape.fold(1, Int::times)
            return FloatBuffer.allocate(size)
        }
    }
}