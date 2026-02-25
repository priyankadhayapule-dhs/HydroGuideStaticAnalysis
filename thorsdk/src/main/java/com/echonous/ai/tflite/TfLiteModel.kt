package com.echonous.ai.tflite

import com.echonous.ai.MLModel
import com.echonous.ai.Tensor
import com.echonous.util.LogUtils
import org.tensorflow.lite.InterpreterApi
import java.nio.FloatBuffer

class TfLiteModel(val interpreter: InterpreterApi, override val description: MLModel.Description): MLModel {
    private var cachedOutput: Output? = null
    class Output(features: List<MLModel.Feature>): MLModel.Output {
        val tensors: List<Tensor> = features.map { (_, shape) -> Tensor(shape) }
        val tfLiteMap: Map<Int, FloatBuffer> = makeTfLiteMap(tensors)

        fun makeTfLiteMap(tensors: List<Tensor>): Map<Int, FloatBuffer> {
            val map = HashMap<Int, FloatBuffer>()
            tensors.forEachIndexed { index, tensor ->
                map[index] = tensor.data
            }
            return map
        }
        override val size: Int get() = tensors.size
        override fun contains(element: Tensor): Boolean = tensors.contains(element)
        override fun containsAll(elements: Collection<Tensor>): Boolean = tensors.containsAll(elements)
        override fun get(index: Int): Tensor = tensors[index]
        override fun indexOf(element: Tensor): Int = tensors.indexOf(element)
        override fun isEmpty(): Boolean = tensors.isEmpty()
        override fun iterator(): Iterator<Tensor> = tensors.iterator()
        override fun lastIndexOf(element: Tensor): Int = tensors.lastIndexOf(element)
        override fun listIterator(): ListIterator<Tensor> = tensors.listIterator()
        override fun listIterator(index: Int): ListIterator<Tensor> = tensors.listIterator(index)
        override fun subList(fromIndex: Int, toIndex: Int): List<Tensor> = tensors.subList(fromIndex, toIndex)
        override fun close() {}

        fun rewind() {
            tensors.forEach { it.data.rewind() }
        }
    }

    override fun predict(inputs: List<Tensor>): MLModel.Output {
        val output = cachedOutput ?: Output(description.outputs).also { cachedOutput = it }

        // transform inputs into array
        val inputArray = Array(inputs.size) { inputs[it].data }
        output.rewind()
        interpreter.runForMultipleInputsOutputs(inputArray, output.tfLiteMap)
        output.rewind()
        return output
    }

    override fun close() {
        super.close()
        cachedOutput?.close()
        cachedOutput = null
        interpreter.close()
    }
}