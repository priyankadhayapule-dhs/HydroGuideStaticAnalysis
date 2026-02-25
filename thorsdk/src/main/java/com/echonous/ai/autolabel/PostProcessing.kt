package com.echonous.ai.autolabel

import com.echonous.ai.Tensor
import kotlin.math.exp

// Suppress warning about multiplication by 0: the multiplication is there so it matches the
// lines that use non zero values
@Suppress("KotlinConstantConditions")
data class Prediction(val yolo: Tensor, val col: Int, val row: Int, val anchor: Int, val config: Config) {
    private val xStride = 1
    private val yStride = yolo.shape[3]
    private val elementStride = yolo.shape[3] * yolo.shape[2]
    private val aStride = elementStride * config.yoloPredictionSize

    val baseOffset = col * xStride + row * yStride + anchor * aStride
    val x: Float get() = yolo.data.get(baseOffset + 0 * elementStride)
    val y: Float get() = yolo.data.get(baseOffset + 1 * elementStride)
    val w: Float get() = yolo.data.get(baseOffset + 2 * elementStride)
    val h: Float get() = yolo.data.get(baseOffset + 3 * elementStride)
    val objConf: Float get() = yolo.data.get(baseOffset + 4 * elementStride)
    fun labelConf(label: Int) = yolo.data.get(baseOffset + (5 + label) * elementStride)
}
data class YoloVals(val proc: List<SimplePrediction>, val raw : List<SimplePrediction>) {

}
data class SimplePrediction(val x : Float, val y: Float, val w: Float, val h: Float, val objConf: Float, val labelConf: List<Float>) {

}
fun processYoloRaw(prediction: Prediction, config: Config): SimplePrediction {
    fun sigmoid(x: Float) = 1.0f / (1.0f + exp(-x))
    var procLabelConf : MutableList<Float> = arrayListOf()
    for (labelIndex in 0 until config.labels.size) {
        procLabelConf.add(sigmoid(prediction.labelConf(labelIndex)))
    }
    val tx = sigmoid(prediction.x)
    val ty = sigmoid(prediction.y)
    val tw = sigmoid(prediction.w)
    val th = sigmoid(prediction.h)

    val bx = (prediction.col.toFloat() + tx * 2.0f - 0.5f) * config.columnPixels
    val by = (prediction.row.toFloat() + ty * 2.0f - 0.5f) * config.rowPixels
    val bw = 4.0f * tw * tw * config.anchors[prediction.anchor].width
    val bh = 4.0f * th * th * config.anchors[prediction.anchor].height
    return SimplePrediction(bx, by, bw, bh,
            sigmoid(prediction.objConf), procLabelConf)
}
fun processSinglePrediction(prediction: Prediction, config: Config): Label
{
    fun sigmoid(x: Float) = 1.0f / (1.0f + exp(-x))

    // Step 1: Apply sigmoid and transform x, y, w, h from local (grid cell) coords into image space coords
    val tx = sigmoid(prediction.x)
    val ty = sigmoid(prediction.y)
    val tw = sigmoid(prediction.w)
    val th = sigmoid(prediction.h)

    val bx = (prediction.col.toFloat() + tx * 2.0f - 0.5f) * config.columnPixels
    val by = (prediction.row.toFloat() + ty * 2.0f - 0.5f) * config.rowPixels
    val bw = 4.0f * tw * tw * config.anchors[prediction.anchor].width
    val bh = 4.0f * th * th * config.anchors[prediction.anchor].height

    // Step 2: Find object and label confidence, and multiply to get total confidence
    val objConf = sigmoid(prediction.objConf)
    var maxLabelIndex = 0
    var maxLabelConf = sigmoid(prediction.labelConf(0))
    for (labelIndex in 1 until config.labels.size) {
        val labelConf = sigmoid(prediction.labelConf(labelIndex))
        if (labelConf > maxLabelConf) {
            maxLabelIndex = labelIndex
            maxLabelConf = labelConf
        }
    }
    val conf = objConf * maxLabelConf
    val label = Label(id = maxLabelIndex, x = bx, y = by, w = bw, h = bh, conf = conf)
    check(label.isValid)
    check(label.id < config.labels.size)

    return label
}

fun processYolo(yolo: Tensor, config: Config): List<Label> {
    val labels = MutableList(config.labels.size) { Label.invalid }
    // TODO: Sanity check shape of input tensor is as expected
    for (row in 0 until config.gridHeight) {
        for (col in 0 until config.gridWidth) {
            for (anchor in 0 until config.anchors.size) {
                val prediction = Prediction(yolo, col, row, anchor, config)
                val label = processSinglePrediction(prediction, config)
                require(label.isValid)
                require(label.id < config.labels.size)
                if (label.conf >= config.labels[label.id].threshold && label.conf > labels[label.id].conf) {
                    labels[label.id] = label
                }
            }
        }
    }

    // Remove invalid labels (occurs when no labels for that class pass the threshold check)
    labels.removeAll { !it.isValid }

    return labels
}

fun processYoloWithIntermediates(yolo: Tensor, config: Config): YoloVals {
    var yoloProc : MutableList<SimplePrediction> = arrayListOf()
    var yoloRaw : MutableList<SimplePrediction> = arrayListOf()
    // TODO: Sanity check shape of input tensor is as expected
    var count = 0
    for (row in 0 until config.gridHeight) {
        for (col in 0 until config.gridWidth) {
            for (anchor in 0 until config.anchors.size) {
                val prediction = Prediction(yolo, col, row, anchor, config)
                // should have x/y/w/h/obj[l]/labelConf[l]
                var rawLabelConf : MutableList<Float> = arrayListOf()
                for (labelIndex in 0 until config.labels.size) {
                    rawLabelConf.add(prediction.labelConf(labelIndex))

                }

                val rawPred = SimplePrediction(prediction.x, prediction.y, prediction.w, prediction.h, prediction.objConf, rawLabelConf)
                yoloRaw.add(rawPred)
                yoloProc.add(processYoloRaw(prediction, config))
                count = count+1
            }
        }
    }
    return YoloVals(yoloProc, yoloRaw)
}

fun processView(view: Tensor) {
    // apply softmax to view
    var maxValue = 0.0f
    for (i in 0 until view.data.capacity()) {
        if (view.data.get(i) > maxValue) {
            maxValue = view.data.get(i)
        }
    }
    var sum = 0.0f
    for (i in 0 until view.data.capacity()) {
        val difference = view.data.get(i) - maxValue
        val e = exp(difference)
        view.data.put(i, e)
        sum += e
    }
    for (i in 0 until view.data.capacity()) {
        view.data.put(i, view.data.get(i) / sum)
    }
}