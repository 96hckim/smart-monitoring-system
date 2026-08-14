package com.hocheol.smartmonitoringsystem

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import com.hocheol.smartmonitoringsystem.ui.MonitoringScreen
import com.hocheol.smartmonitoringsystem.ui.theme.SmartMonitoringSystemTheme

/**
 * 앱의 메인 진입점 Activity
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Edge-to-Edge 디자인 적용
        enableEdgeToEdge()

        setContent {
            SmartMonitoringSystemTheme {
                // UI 메인 스크린 호출
                MonitoringScreen()
            }
        }
    }
}
