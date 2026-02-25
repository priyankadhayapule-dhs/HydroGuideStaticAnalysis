package com.echonous.ai.ef

import com.echonous.ai.MLModel
import kotlinx.serialization.Serializable

@Serializable
data class PhaseDetectConfig (
    val modelName: String,
    val modelDescription: MLModel.Description,
    val imageWidth: Int,
    val imageHeight: Int,
)
{
    companion object {
        val areaRegressionPhaseDetect = PhaseDetectConfig(
                modelName = "phase_detect_non_opt.tflite",
                modelDescription= MLModel.Description(
                        name="area_regression",
                        version = "128_0.0001_300_64",
                        inputs = listOf(MLModel.Feature(name="image", shape=intArrayOf(1,1,128,128))),
                        outputs = listOf(MLModel.Feature(name="estimate", shape=intArrayOf(1,1)))
                ),
                imageWidth = 128,
                imageHeight = 128
        )
    }
}