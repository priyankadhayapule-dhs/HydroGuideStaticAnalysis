package com.echonous.ai.vti
import android.content.Context

import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import com.echonous.ai.Tensor


class Pipeline(val model: MLModel, val config: com.echonous.ai.vti.Config) : AutoCloseable {

    override fun close() {
        model.close()
    }
    fun process(image: Tensor, kernel: Tensor ) : List<Tensor>{
        return model.predict(listOf(kernel, image))
    }
    companion object {

        fun pipelineFromConfig(config: com.echonous.ai.vti.Config, provider: MLProvider, context: Context): Pipeline {
            val model = provider.createModelFromAsset(context.assets, config.modelName, config.modelDescription)
            return Pipeline(model, config)
        }
    }
}