package com.echonous.ai

import kotlinx.serialization.Serializable

interface MLModel: AutoCloseable {
    @Serializable
    class Description(
        val name: String,
        val version: String,
        val inputs: List<Feature>,
        val outputs: List<Feature>,
    )

    @Serializable
    data class Feature(val name: String, val shape: IntArray) {
        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (javaClass != other?.javaClass) return false

            other as Feature

            if (name != other.name) return false
            if (!shape.contentEquals(other.shape)) return false

            return true
        }

        override fun hashCode(): Int {
            var result = name.hashCode()
            result = 31 * result + shape.contentHashCode()
            return result
        }

    }

    interface Output: List<Tensor>, AutoCloseable

    val description: Description

    fun predict(inputs: List<Tensor>): Output
    override fun close() {}
}