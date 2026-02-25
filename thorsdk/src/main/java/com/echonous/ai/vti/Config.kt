package com.echonous.ai.vti

import com.echonous.ai.MLModel
import com.echonous.ai.fast.Config
import kotlinx.serialization.Serializable

@Serializable
data class Config(
        val modelName: String,
        val modelDescription: MLModel.Description,
        val imageWidth: Int,
        val imageHeight: Int,
        val uncertaintyThreshold: List<Float>
) {
    companion object {
        val vtiConfig36 = Config(
                modelName = "vti_non_opt_kernels36_51_50_167.tflite",
                modelDescription = MLModel.Description(
                        name = "VTI 36",
                        version = "TFLITE",
                        inputs = listOf(MLModel.Feature(name = "input_frames", shape = intArrayOf(1, 256, 256)),
                                MLModel.Feature(name = "kernel", shape = intArrayOf(1, 9))),
                        outputs = listOf(
                                MLModel.Feature(name = "output", shape = intArrayOf(2, 256, 256)))
                ),
                imageWidth = 256,
                imageHeight = 256,
                uncertaintyThreshold = listOf(0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f)
        )

        val vtiConfig38 = Config(
                modelName = "vti_non_opt_kernels38_38_88_170.tflite",
                modelDescription = MLModel.Description(
                        name = "VTI 38",
                        version = "TFLITE",
                        inputs = listOf(MLModel.Feature(name = "input_frames", shape = intArrayOf(1, 256, 256)),
                                MLModel.Feature(name = "kernel", shape = intArrayOf(1, 9))),
                        outputs = listOf(
                                MLModel.Feature(name = "output", shape = intArrayOf(2, 256, 256)))
                ),
                imageWidth = 256,
                imageHeight = 256,
                uncertaintyThreshold = listOf(0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f)
        )

        val vtiConfig50 = Config(
                modelName = "vti_non_opt_kernels50_33_50_154.tflite",
                modelDescription = MLModel.Description(
                        name = "VTI 50",
                        version = "TFLITE",
                        inputs = listOf(MLModel.Feature(name = "input_frames", shape = intArrayOf(1, 256, 256)),
                                MLModel.Feature(name = "kernel", shape = intArrayOf(1, 9))),
                        outputs = listOf(
                                MLModel.Feature(name = "output", shape = intArrayOf(2, 256, 256)))
                ),
                imageWidth = 256,
                imageHeight = 256,
                uncertaintyThreshold = listOf(0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f, 0.6f)
        )
    }
}