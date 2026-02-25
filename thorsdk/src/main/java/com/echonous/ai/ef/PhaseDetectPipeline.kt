package com.echonous.ai.ef
import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor

class PhaseDetectPipeline(val model: MLModel, val config: com.echonous.ai.ef.PhaseDetectConfig) : AutoCloseable{

    fun process(image: Tensor): Float {
        model.predict(listOf(image)).use {
            val estimate = it[0]
            return estimate.data[0]
        }
    }

    override fun close() {
        model.close()
    }

    companion object {
        fun phaseDetectPipelineDefault(provider: MLProvider, context: Context): PhaseDetectPipeline {
            val model = provider.createModelFromAsset(context.assets, com.echonous.ai.ef.PhaseDetectConfig.areaRegressionPhaseDetect.modelName, com.echonous.ai.ef.PhaseDetectConfig.areaRegressionPhaseDetect.modelDescription)
            return PhaseDetectPipeline(model, com.echonous.ai.ef.PhaseDetectConfig.areaRegressionPhaseDetect)
        }
    }
}