package com.echonous.ai.autolabel


import com.echonous.RingBuffer
import com.echonous.ai.Tensor

class SmoothingBuffer(val numFrames: Int, val config: Config) {
    private val viewBuffer = RingBuffer<FloatArray>(numFrames)
    private val labelBuffer = RingBuffer<List<Label>>(numFrames)
    var view: Pair<Int, Float> = -1 to 0f
    val labels: List<Label> get() {
        val labels = mutableListOf<Label>()
        for (i in 0 until config.labels.size) {
            val label = labelForIndex(i)
            if (label.isValid) {
                labels.add(label)
            }
        }
        return labels
    }

    var smoothView: Tensor = Tensor(intArrayOf(1, config.views.size))

    fun clear() {
        viewBuffer.clear()
        labelBuffer.clear()
    }
    fun add(view: Tensor, labels: List<Label>) {
        require(view.size == config.views.size)
        // debug log view and confidence
        var bestView = 0
        var bestConf = 0f
        for (i in 0 until view.size) {
            if (view.data.get(i) > bestConf) {
                bestConf = view.data.get(i)
                bestView = i
            }
        }
        val viewName = config.views[bestView]

        viewBuffer.add(view.data.array())
        labelBuffer.add(labels)

        this.view = bestView()
    }

    /// Compute the location/confidence of the label with the given id,
    /// by averaging that label across all frames in the buffer
    /// Returns Label.invalid if the label is not present in at least config.labelBufferFraction
    /// of the buffer.
    fun labelForIndex(id: Int): Label {
        if (!config.isLabelInView(id, view.first)) {
            return Label.invalid
        }

        var count = 0
        var sumX = 0f
        var sumY = 0f
        var sumW = 0f
        var sumH = 0f
        var sumConf = 0f
        for (i in 0 until labelBuffer.size) {
            val single = labelBuffer[i].find { it.id == id }
            if (single != null) {
                count++
                sumX += single.x
                sumY += single.y
                sumW += single.w
                sumH += single.h
                sumConf += single.conf
            }
        }
        val fraction = count.toFloat() / labelBuffer.size.toFloat()
        return if (fraction > config.labelBufferFraction) {
            Label(
                id = id,
                x = sumX / count,
                y = sumY / count,
                w = sumW / count,
                h = sumH / count,
                conf = sumConf / count
            )
        } else {
            Label.invalid
        }
    }

    fun viewConfidence(index: Int): Float {
        var sumConfidence = 0f
        for (i in 0 until viewBuffer.size) {
            sumConfidence += viewBuffer[i][index]
        }
        return sumConfidence / viewBuffer.capacity
    }

    private fun bestView(): Pair<Int, Float> {
        var bestViewIndex = -1
        var bestViewScore = 0f
        for (view in 0 until config.views.size) {
            val viewScore = viewConfidence(view)
            smoothView.data.put(view, viewScore)
            if (viewScore > bestViewScore) {
                bestViewIndex = view
                bestViewScore = viewScore
            }
        }
        return bestViewIndex to bestViewScore
    }
}