package com.echonous.ai.tflite

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.MLProvider
import org.tensorflow.lite.InterpreterApi
import org.tensorflow.lite.TensorFlowLite
import org.tensorflow.lite.gpu.CompatibilityList
import org.tensorflow.lite.gpu.GpuDelegateFactory
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.concurrent.Executor

class TfLiteProvider(val compatList: CompatibilityList, val runtime: InterpreterApi.Options.TfLiteRuntime): MLProvider {
    override val name: String = "TensorFlowLite version ${TensorFlowLite.runtimeVersion(runtime)}"
    override fun createModelFromData(data: ByteArray, description: MLModel.Description): MLModel {
        val options = InterpreterApi.Options()
        options.runtime = runtime
        if (compatList.isDelegateSupportedOnThisDevice) {
            val gpuOptions = compatList.bestOptionsForThisDevice
            gpuOptions.isPrecisionLossAllowed = false
            options.addDelegateFactory(GpuDelegateFactory(gpuOptions))
        }
        val buffer = ByteBuffer.allocateDirect(data.size).order(ByteOrder.nativeOrder())
        buffer.put(data)
        buffer.flip()
        val interpreter = InterpreterApi.create(buffer, options)
        return TfLiteModel(interpreter, description)
    }

    companion object {
        private val providers: MutableMap<Context, TfLiteProvider> = HashMap()

        private object DirectExecutor: Executor {
            override fun execute(r: Runnable) {
                r.run()
            }
        }

        /**
         * Create a TfLiteProvider that uses the google play services TfLite runtime
         */
        fun createGMSProvider(context: Context): TfLiteProvider {
            synchronized(providers) {
                return providers.computeIfAbsent(context) {
                    val compatList = CompatibilityList()
                    TfLiteProvider(
                        compatList,
                        InterpreterApi.Options.TfLiteRuntime.FROM_APPLICATION_ONLY
                    )
                }
            }
        }
    }
}