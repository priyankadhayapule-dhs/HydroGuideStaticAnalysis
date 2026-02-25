package com.echonous.ai.ef

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor
import kotlinx.serialization.Serializable

class LVSegmentationPipeline(val models: List<MLModel>): AutoCloseable {

    fun process(frameWindow: Tensor, confMap: Tensor) {
        require(frameWindow.shape.contentEquals(intArrayOf(1, 5, 128, 128)))
        require(confMap.shape.contentEquals(intArrayOf(models.size, 3, 128, 128)))

        // Prepare to write into confMap backing FloatBuffer
        val outputBuffer = confMap.data
        outputBuffer.clear()

        // Run each model in our list, concatenating outputs into one buffer
        // Not trying to do any merging/averaging here in Kotlin, leave that until native side
        for (model in models) {
            model.predict(listOf(frameWindow)).use { output ->
                outputBuffer.put(output[0].data)
            }
        }
        outputBuffer.flip()
    }

    override fun close() {
        models.forEach { it.close() }
    }

    @Serializable
    data class Config(
        val modelName: String,
        val modelDescription: MLModel.Description,
    ) {
        companion object {
            private fun configForModel(name: String): Config {
                return Config(
                    modelName = String.format("%s.tflite", name),
                    modelDescription = MLModel.Description(
                        name = name,
                        version = "2024-05-07",
                        inputs = listOf(MLModel.Feature(name = "images", shape = intArrayOf(1, 5, 128, 128))),
                        outputs = listOf(MLModel.Feature(name = "segmentation", shape = intArrayOf(1, 3, 128, 128)))
                    )
                )
            }

            val ed_model_1 = configForModel("ED_MODEL_1")
            val ed_model_2 = configForModel("ED_MODEL_2")
            val ed_model_3 = configForModel("ED_MODEL_3")
            val es_model_1 = configForModel("ES_MODEL_1")
            val es_model_2 = configForModel("ES_MODEL_2")
            val es_model_3 = configForModel("ES_MODEL_3")
        }
    }

    companion object {

        private fun pipelineFromConfigs(configs: List<Config>, provider: MLProvider, context: Context): LVSegmentationPipeline {
            return LVSegmentationPipeline(configs.map {
                provider.createModelFromAsset(context.assets, it.modelName, it.modelDescription)
            })
        }

        fun edPipeline(provider: MLProvider, context: Context): LVSegmentationPipeline {
            return pipelineFromConfigs(listOf(
                Config.ed_model_1,
                Config.ed_model_2,
                Config.ed_model_3
            ), provider, context)
        }

        fun esPipeline(provider: MLProvider, context: Context): LVSegmentationPipeline {
            return pipelineFromConfigs(listOf(
                Config.es_model_1,
                Config.es_model_2,
                Config.es_model_3
            ), provider, context)
        }
    }

}