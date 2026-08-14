package com.hocheol.smartmonitoringsystem.ui

import androidx.camera.core.CameraSelector
import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.hocheol.smartmonitoringsystem.network.TcpSocketClient
import com.hocheol.smartmonitoringsystem.network.TcpSocketClient.SocketEvent
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * UI 상태 관리 및 네트워크 엔진 연동을 담당하는 ViewModel
 */
class MonitoringViewModel : ViewModel() {

    private val socketClient = TcpSocketClient(viewModelScope)

    private val _ip = MutableStateFlow("192.168.137.1")
    val ip = _ip.asStateFlow()

    private val _port = MutableStateFlow("8080")
    val port = _port.asStateFlow()

    private val _isConnected = MutableStateFlow(false)
    val isConnected = _isConnected.asStateFlow()

    private val _isStreaming = MutableStateFlow(false)
    val isStreaming = _isStreaming.asStateFlow()

    private val _cameraSelector = MutableStateFlow(CameraSelector.DEFAULT_BACK_CAMERA)
    val cameraSelector = _cameraSelector.asStateFlow()

    private val _isFlashEnabled = MutableStateFlow(false)
    val isFlashEnabled = _isFlashEnabled.asStateFlow()

    private val _gasValue = MutableStateFlow(0)
    val gasValue = _gasValue.asStateFlow()

    private val _threshold = MutableStateFlow(3000)
    val threshold = _threshold.asStateFlow()

    private val _logs = MutableStateFlow<List<String>>(emptyList())
    val logs = _logs.asStateFlow()

    init {
        observeSocketEvents()
    }

    private fun observeSocketEvents() {
        viewModelScope.launch {
            socketClient.events.collect { event ->
                when (event) {
                    is SocketEvent.Connected -> {
                        _isConnected.value = true
                        addLog("서버에 연결되었습니다: ${event.ip}:${event.port}")
                    }

                    is SocketEvent.Disconnected -> {
                        _isConnected.value = false
                        _isStreaming.value = false
                        addLog(event.message ?: "연결이 해제되었습니다.")
                    }

                    is SocketEvent.MessageReceived -> {
                        val text = event.text.trim()
                        if (text.startsWith("GAS:")) {
                            parseGasData(text)
                        } else {
                            addLog("수신: $text")
                        }
                    }

                    is SocketEvent.Error -> {
                        addLog("오류: ${event.message}")
                    }
                }
            }
        }
    }

    private fun parseGasData(text: String) {
        try {
            // 포맷: "GAS:<수치>:<임계값>"
            val parts = text.split(":")
            if (parts.size >= 3) {
                val value = parts[1].toIntOrNull() ?: 0
                val thresh = parts[2].toIntOrNull() ?: 3000
                _gasValue.value = value
                _threshold.value = thresh
            }
        } catch (e: Exception) {
            addLog("데이터 파싱 오류: $text")
        }
    }

    fun onIpChange(newIp: String) {
        _ip.value = newIp
    }

    fun onPortChange(newPort: String) {
        _port.value = newPort
    }

    fun toggleConnection() {
        if (_isConnected.value) {
            socketClient.disconnect()
        } else {
            val portInt = _port.value.toIntOrNull() ?: 8080
            socketClient.connect(_ip.value, portInt)
            addLog("연결 시도 중: ${_ip.value}:${portInt}")
        }
    }

    fun toggleStreaming() {
        _isStreaming.value = !_isStreaming.value
        addLog(if (_isStreaming.value) "스트리밍 시작" else "스트리밍 중지")
    }

    fun toggleCamera() {
        _cameraSelector.value = if (_cameraSelector.value == CameraSelector.DEFAULT_BACK_CAMERA) {
            CameraSelector.DEFAULT_FRONT_CAMERA
        } else {
            CameraSelector.DEFAULT_BACK_CAMERA
        }
        addLog("카메라 전환: ${if (_cameraSelector.value == CameraSelector.DEFAULT_BACK_CAMERA) "후면" else "전면"}")
    }

    fun toggleFlash() {
        _isFlashEnabled.value = !_isFlashEnabled.value
        addLog("플래시: ${if (_isFlashEnabled.value) "ON" else "OFF"}")
    }

    fun sendValveClose() {
        socketClient.sendText("1")
        addLog("송신: 1 (밸브 차단)")
    }

    fun sendValveOpen() {
        socketClient.sendText("0")
        addLog("송신: 0 (밸브 복구)")
    }

    fun sendVideoFrame(data: ByteArray) {
        if (_isConnected.value && _isStreaming.value) {
            socketClient.sendRaw(data)
        }
    }

    private fun addLog(text: String) {
        val timestamp = SimpleDateFormat("HH:mm:ss", Locale.getDefault()).format(Date())
        val formattedLog = "[$timestamp] $text"
        _logs.update { it + formattedLog }
    }

    override fun onCleared() {
        super.onCleared()
        socketClient.disconnect()
    }
}
