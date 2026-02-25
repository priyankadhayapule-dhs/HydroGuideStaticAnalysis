package com.echonous.ai.autopreset

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor
import com.echonous.ai.autolabel.Label
import java.nio.ByteBuffer
import java.nio.ByteOrder
class Pipeline(val model: MLModel, val config: Config) : AutoCloseable {

    data class PipelineResult(
            val anatomy: Tensor
    )
    fun process(image: Tensor) : List<Float>
    {
        require(image.shape.contentEquals(intArrayOf(1, 5, 224, 224)))
        return model.predict(listOf(image))[0].data.array().asList() //autopreset only has one output tensor
    }

    override fun close() {
        model.close()
    }
    companion object {
        fun pipeline20240205(provider: MLProvider, context: Context) : Pipeline {
            return pipelineFromConfig(Config.multiframe, provider, context)
        }
        fun pipelineFromConfig(config: Config, provider: MLProvider, context: Context): Pipeline {
            val model = provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
            return Pipeline(model, config)
        }
    }
}