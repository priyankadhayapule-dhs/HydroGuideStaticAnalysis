package com.echonous.ai.fast

import com.echonous.ai.Tensor

@Suppress("KotlinConstantConditions")
data class Prediction(val yolo: Tensor, val col: Int, val row: Int, val anchor: Int, val config: com.echonous.ai.autolabel.Config) {
    private val xStride = 1
    private val yStride = yolo.shape[3]
    private val elementStride = yolo.shape[3] * yolo.shape[2]
    private val aStride = elementStride * config.yoloPredictionSize

    val baseOffset = col * xStride + row * yStride + anchor * aStride
    val x: Float get() = yolo.data.get(baseOffset + 0 * elementStride)
    val y: Float get() = yolo.data.get(baseOffset + 1 * elementStride)
    val w: Float get() = yolo.data.get(baseOffset + 2 * elementStride)
    val h: Float get() = yolo.data.get(baseOffset + 3 * elementStride)
    val objConf: Float get() = yolo.data.get(baseOffset + 4 * elementStride)
    fun labelConf(label: Int) = yolo.data.get(baseOffset + (5 + label) * elementStride)
}
data class FASTPrediction(val view: List<Float>, val yolo: List<Float>)
{
    companion object {
        fun default() : FASTPrediction {
            val view = listOf(0.0f)
            val yolo = listOf(0.0f)
            return FASTPrediction(view, yolo)
        }
    }
}