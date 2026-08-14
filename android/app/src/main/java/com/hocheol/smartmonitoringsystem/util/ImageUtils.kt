package com.hocheol.smartmonitoringsystem.util

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.ImageFormat
import android.graphics.Matrix
import android.graphics.Rect
import android.graphics.YuvImage
import androidx.camera.core.ImageProxy
import java.io.ByteArrayOutputStream

/**
 * 카메라 프레임 처리를 위한 유틸리티 클래스
 */
object ImageUtils {

    /**
     * CameraX의 [ImageProxy]를 JPEG 바이트 배열로 변환합니다.
     * 센서의 회전 각도([ImageProxy.getImageInfo.getRotationDegrees])를 자동으로 반영합니다.
     */
    fun imageProxyToJpeg(imageProxy: ImageProxy, quality: Int = 50): ByteArray? {
        if (imageProxy.format != ImageFormat.YUV_420_888) return null

        val yBuffer = imageProxy.planes[0].buffer
        val uBuffer = imageProxy.planes[1].buffer
        val vBuffer = imageProxy.planes[2].buffer

        val ySize = yBuffer.remaining()
        val uSize = uBuffer.remaining()
        val vSize = vBuffer.remaining()

        val nv21 = ByteArray(ySize + uSize + vSize)

        yBuffer.get(nv21, 0, ySize)
        vBuffer.get(nv21, ySize, vSize)
        uBuffer.get(nv21, ySize + vSize, uSize)

        val yuvImage = YuvImage(nv21, ImageFormat.NV21, imageProxy.width, imageProxy.height, null)
        val out = ByteArrayOutputStream()
        yuvImage.compressToJpeg(Rect(0, 0, imageProxy.width, imageProxy.height), quality, out)
        val imageBytes = out.toByteArray()

        val rotation = imageProxy.imageInfo.rotationDegrees
        if (rotation == 0) return imageBytes

        // 회전 처리
        val bitmap = BitmapFactory.decodeByteArray(imageBytes, 0, imageBytes.size)
        val matrix = Matrix().apply { postRotate(rotation.toFloat()) }
        val rotatedBitmap = Bitmap.createBitmap(
            bitmap, 0, 0, bitmap.width, bitmap.height, matrix, true
        )

        val rotatedOut = ByteArrayOutputStream()
        rotatedBitmap.compress(Bitmap.CompressFormat.JPEG, quality, rotatedOut)

        bitmap.recycle()
        rotatedBitmap.recycle()

        return rotatedOut.toByteArray()
    }
}
