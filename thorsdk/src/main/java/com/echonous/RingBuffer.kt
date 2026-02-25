package com.echonous

class RingBuffer<T>(val capacity: Int) {
    var size = 0
    val isEmpty get() = size == 0
    val isFull get() = size == capacity
    private var head = 0
    private var tail = 0
    private val buffer = MutableList<T?>(capacity) { null }

    fun clear() {
        size = 0
        head = 0
        tail = 0
        buffer.forEachIndexed { index, _ -> buffer[index] = null }
    }

    fun add(element: T) {
        buffer[tail] = element
        tail = (tail + 1) % capacity
        if (size < capacity) {
            size++
        } else {
            head = (head + 1) % capacity
        }
    }

    operator fun get(index: Int): T {
        if (index < 0 || index >= size) {
            throw IndexOutOfBoundsException("index $index out of bounds (0 until $size)")
        }
        return buffer[(head + index) % capacity]!!
    }
}