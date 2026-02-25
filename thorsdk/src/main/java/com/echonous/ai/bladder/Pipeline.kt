package com.echonous.ai.bladder

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor
import com.echonous.util.LogUtils

data class PipelineResult(
    val viewOut: Tensor,
    val pMapOut: Tensor,
    val otherOut: Tensor
)

class Pipeline(val model: MLModel, val config: Config): AutoCloseable {
    fun process(image: Tensor): PipelineResult {
        model.predict(listOf( image)).use {
            val pMapOut = it[0]
            val viewOut = it[1]
            val otherOut = it[2]

            return PipelineResult(viewOut, pMapOut, otherOut)
        }

    }
    override fun close() {
        model.close()
    }
    companion object {
        fun pipelineFromConfig(config: Config, provider: MLProvider, context: Context): Pipeline{
            val model = provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
            return Pipeline(model, config)
        }
    }
}