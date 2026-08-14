package com.hocheol.smartmonitoringsystem.network

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.BufferedReader
import java.io.InputStreamReader
import java.io.OutputStream
import java.io.PrintWriter
import java.net.InetSocketAddress
import java.net.Socket

/**
 * TCP 소켓 통신을 담당하는 클라이언트 엔진
 */
class TcpSocketClient(
    private val scope: CoroutineScope
) {
    private var socket: Socket? = null
    private var writer: PrintWriter? = null
    private var reader: BufferedReader? = null
    private var outputStream: OutputStream? = null
    private var receiveJob: Job? = null

    private val _events = MutableSharedFlow<SocketEvent>()
    val events = _events.asSharedFlow()

    sealed class SocketEvent {
        data class Connected(val ip: String, val port: Int) : SocketEvent()
        data class Disconnected(val message: String? = null) : SocketEvent()
        data class MessageReceived(val text: String) : SocketEvent()
        data class Error(val message: String) : SocketEvent()
    }

    fun connect(ip: String, port: Int) {
        scope.launch(Dispatchers.IO) {
            try {
                socket = Socket()
                socket?.connect(InetSocketAddress(ip, port), 5000)

                val currentSocket = socket ?: throw Exception("Socket creation failed")
                outputStream = currentSocket.getOutputStream()
                writer = PrintWriter(outputStream, true)
                reader = BufferedReader(InputStreamReader(currentSocket.getInputStream()))

                _events.emit(SocketEvent.Connected(ip, port))
                startReceiveLoop()
            } catch (e: Exception) {
                _events.emit(SocketEvent.Error(e.message ?: "Unknown connection error"))
                disconnect()
            }
        }
    }

    private fun startReceiveLoop() {
        receiveJob = scope.launch(Dispatchers.IO) {
            try {
                while (isActive && socket?.isConnected == true) {
                    val receivedMessage = reader?.readLine()
                    if (receivedMessage != null) {
                        _events.emit(SocketEvent.MessageReceived(receivedMessage))
                    } else {
                        _events.emit(SocketEvent.Disconnected("Server closed connection"))
                        disconnect()
                        break
                    }
                }
            } catch (e: Exception) {
                if (isActive) {
                    _events.emit(SocketEvent.Error("Receive error: ${e.message}"))
                    disconnect()
                }
            }
        }
    }

    fun sendText(text: String) {
        scope.launch(Dispatchers.IO) {
            try {
                writer?.println(text)
            } catch (e: Exception) {
                _events.emit(SocketEvent.Error("Send failed: ${e.message}"))
            }
        }
    }

    fun sendRaw(data: ByteArray) {
        scope.launch(Dispatchers.IO) {
            try {
                outputStream?.let { os ->
                    val size = data.size
                    os.write(
                        byteArrayOf(
                            (size ushr 24).toByte(),
                            (size ushr 16).toByte(),
                            (size ushr 8).toByte(),
                            size.toByte()
                        )
                    )
                    os.write(data)
                    os.flush()
                }
            } catch (e: Exception) {
                // Raw data errors are often too frequent to log as critical events
            }
        }
    }

    fun disconnect() {
        scope.launch(Dispatchers.IO) {
            try {
                receiveJob?.cancel()
                socket?.close()
                socket = null
                writer = null
                reader = null
                outputStream = null
                _events.emit(SocketEvent.Disconnected())
            } catch (e: Exception) {
                _events.emit(SocketEvent.Error("Disconnect error: ${e.message}"))
            }
        }
    }
}
