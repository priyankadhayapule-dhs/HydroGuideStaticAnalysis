package com.echonous.ai.autopreset

import com.echonous.ai.MLModel
import kotlinx.serialization.Serializable

@Serializable
data class Config(
        val modelName: String,
        val modelDescription: MLModel.Description,
        val imageWidth: Int,
        val imageHeight: Int,
        val inferenceIntervalFrames: Int,
        val inputBufferSize: Int,
        val smoothingFramesCount: Int,
        val decisionBufferSize: Int,
        val anatomyLabelCount: Int,
        val confidenceThreshold: Float,
        val decisionBufferFraction: Float,
        val top2ConfidenceThreshold: Float,
        val anatomyClasses: List<String>,
) {

    companion object {
        val multiframe = Config(
                modelName = "multiframe_2_05_2024.tflite",
                modelDescription = MLModel.Description(
                        name = "multiframe",
                        version = "2_05_2024",
                        inputs = listOf(MLModel.Feature(name="input_frames", shape = intArrayOf(1,5,224,224))),
                        outputs = listOf(MLModel.Feature(name="anatomy", shape = intArrayOf(1,5) )),
                ),
                imageWidth = 224,
                imageHeight = 224,
                inferenceIntervalFrames = 4,
                inputBufferSize = 5,
                smoothingFramesCount = 16,
                decisionBufferSize = 16,
                anatomyLabelCount = 5,
                confidenceThreshold = 0.65f,
                decisionBufferFraction = 0.8f,
                top2ConfidenceThreshold = 0.2f,
                anatomyClasses = listOf("Cardiac", "Abdominal", "Lung", "Noise", "IVC")
                )
    }
}