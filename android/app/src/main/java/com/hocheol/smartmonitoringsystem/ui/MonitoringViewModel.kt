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

    private val _message = MutableStateFlow("")
    val message = _message.asStateFlow()

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
                        addLog("수신: ${event.text}")
                    }

                    is SocketEvent.Error -> {
                        addLog("오류: ${event.message}")
                    }
                }
            }
        }
    }

    fun onIpChange(newIp: String) {
        _ip.value = newIp
    }

    fun onPortChange(newPort: String) {
        _port.value = newPort
    }

    fun onMessageChange(newMessage: String) {
        _message.value = newMessage
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

    fun sendMessage() {
        val msg = _message.value
        if (msg.isBlank()) return
        socketClient.sendText(msg)
        addLog("송신: $msg")
        _message.value = ""
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
