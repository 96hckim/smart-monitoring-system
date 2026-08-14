package com.hocheol.smartmonitoringsystem.ui

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

    private val _cameraSelector = MutableStateFlow(0) // 0: Back, 1: Front
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
                        addLog("서버 연결 성공: ${event.ip}:${event.port}")
                    }

                    is SocketEvent.Disconnected -> {
                        _isConnected.value = false
                        _isStreaming.value = false
                        addLog(event.message ?: "서버 연결 해제됨")
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
            val parts = text.split(":")
            if (parts.size >= 3) {
                _gasValue.value = parts[1].toIntOrNull() ?: 0
                _threshold.value = parts[2].toIntOrNull() ?: 3000
            }
        } catch (e: Exception) {
            // 파싱 오류 무시
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
            addLog("서버 연결 시도 중...")
        }
    }

    fun toggleStreaming() {
        if (!_isConnected.value) {
            addLog("먼저 서버에 연결해 주세요.")
            return
        }
        _isStreaming.value = !_isStreaming.value
        addLog(if (_isStreaming.value) "스트리밍 시작" else "스트리밍 중지")
    }

    fun toggleCamera() {
        _cameraSelector.value = if (_cameraSelector.value == 0) 1 else 0
        addLog("카메라 전환: ${if (_cameraSelector.value == 0) "후면" else "전면"}")
    }

    fun toggleFlash() {
        _isFlashEnabled.value = !_isFlashEnabled.value
        addLog("플래시: ${if (_isFlashEnabled.value) "ON" else "OFF"}")
    }

    fun sendValveClose() {
        socketClient.sendText("1")
        addLog("명령 송신: 1 (밸브 차단)")
    }

    fun sendValveOpen() {
        socketClient.sendText("0")
        addLog("명령 송신: 0 (밸브 복구)")
    }

    fun clearLogs() {
        _logs.value = emptyList()
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
