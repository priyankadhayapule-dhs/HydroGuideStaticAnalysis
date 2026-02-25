package com.echonous.ai.fast

import android.content.Context
import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.ai.autolabel.Label
import com.echonous.ai.autolabel.Prediction
import com.echonous.ai.autolabel.processSinglePrediction
import com.echonous.ai.fast.Pipeline
import com.echonous.ai.tflite.TfLiteProvider
import java.nio.ByteBuffer
import java.nio.ByteOrder

fun YoloMap(x: Int, y: Int, a: Int, ch: Int, config: Config):Int
{
    val FAST_OBJ_DETECT_NUM_CHANNELS = config.yoloPredictionSize
    val GRID_DIM = config.gridHeight
    val X_STRIDE = 1;
    val Y_STRIDE = config.gridWidth;
    val ELEMENT_STRIDE = GRID_DIM * GRID_DIM;
    val A_STRIDE = ELEMENT_STRIDE * FAST_OBJ_DETECT_NUM_CHANNELS;
    val baseOffset = x * X_STRIDE + y * Y_STRIDE + a * A_STRIDE;
    val retval = baseOffset + ch * ELEMENT_STRIDE;
    return retval
}

class NativeInterface(val context: Context) : AutoCloseable {

    var pipeline: Pipeline? = null
    var pipelineFast_142704: Pipeline? = null
    var pipelineFast_142730: Pipeline? = null
    var pipelineFast_142755: Pipeline? = null
    var pipelineFast_142805: Pipeline? = null
    var hybridModel: List<Pipeline?> = listOf()
    fun load() {
        pipeline = createTfLitePipeline(context)
        pipelineFast_142704 = createTfLitePipelineWithConfig(context, Config.fast_142704)
        pipelineFast_142730 = createTfLitePipelineWithConfig(context, Config.fast_142730)
        pipelineFast_142755 = createTfLitePipelineWithConfig(context, Config.fast_142755)
        pipelineFast_142805 = createTfLitePipelineWithConfig(context, Config.fast_142805)
        hybridModel = listOf(pipelineFast_142704, pipelineFast_142730, pipelineFast_142755, pipelineFast_142805)
    }

    fun process(image: ByteBuffer, labels: ByteBuffer, views: ByteBuffer) : Int {
        val startTime = System.currentTimeMillis()
        val pipeline = pipeline ?: return -1
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val modelPreds = pipeline.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        views.order(ByteOrder.nativeOrder())
        views.clear()
        for (pred in modelPreds.yolo) {
            labels.putFloat(pred.toFloat())
        }

        for (temp in modelPreds.view){
            views.putFloat(temp.toFloat())
        }
        labels.flip()
        views.flip()
        val elapsedTime = System.currentTimeMillis() - startTime
        //return modelPreds
        return modelPreds.yolo.size
    }

    fun processOrdered(image: ByteBuffer, labels: ByteBuffer, views: ByteBuffer, index: Int) : Int {
        val startTime = System.currentTimeMillis()
        //println("[ FAST ] Processing index $index")
        val pipeline = hybridModel[index] ?: return 0
        image.order(ByteOrder.nativeOrder())
        val imageTensor = Tensor(pipeline.config.modelDescription.inputs[0].shape, image.asFloatBuffer())
        val modelPreds = pipeline.process(imageTensor)
        labels.order(ByteOrder.nativeOrder())
        labels.clear()
        views.order(ByteOrder.nativeOrder())
        views.clear()
            for (y in 0..<pipeline.config.gridHeight){
                for (x in 0..<pipeline.config.gridWidth){
                    for(a in 0..<pipeline.config.anchorCount)
                    {
                    for(ch in 0..<pipeline.config.yoloPredictionSize){
                        labels.putFloat(modelPreds.yolo[YoloMap(x,y,a,ch,pipeline.config)])
                    }
                }
            }
        }

        for (temp in modelPreds.view){
            views.putFloat(temp.toFloat())
        }
        labels.flip()
        views.flip()
        val elapsedTime = System.currentTimeMillis() - startTime
        //return modelPreds
        return modelPreds.yolo.size
    }

    fun clear() {
    }

    override fun close() {
    }

    companion object {
        fun createTfLitePipeline(context: Context): com.echonous.ai.fast.Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.default(provider, context)

        }
        fun createTfLitePipelineWithConfig(context: Context, config: Config): com.echonous.ai.fast.Pipeline {
            val provider = TfLiteProvider.createGMSProvider(context)
            return Pipeline.pipelineFromConfig(config, provider, context)
        }
    }
}
