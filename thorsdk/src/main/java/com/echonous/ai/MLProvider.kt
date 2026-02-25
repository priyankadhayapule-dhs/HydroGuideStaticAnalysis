package com.echonous.ai

import android.content.res.AssetManager
import android.os.Build

import com.echonous.IOUtility

interface MLProvider {
    val name: String

    fun createModelFromData(data: ByteArray, description: MLModel.Description): MLModel

    fun createModelFromAsset(assetManager: AssetManager, assetPath: String,description: MLModel.Description): MLModel {
        val data = assetManager.open(assetPath).use {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                it.readAllBytes()
            } else {
                IOUtility.readAllBytes(it)
            }
        }
        return createModelFromData(data, description)
    }
}