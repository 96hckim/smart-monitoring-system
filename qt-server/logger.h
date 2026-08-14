#ifndef LOGGER_H
#define LOGGER_H

#include <QDateTime>
#include <QString>

// 1. 통신/모듈 카테고리 열거형
enum class LogCategory {
    TCP,
    Serial,
    System
};

// 2. 로그 상태 레벨 열거형
enum class LogLevel {
    Info,
    Warn,
    Error,
    Tx,
    Rx
};

class Logger {
public:
    // ⭐ 메인 로깅 함수 (단하나의 대표 함수)
    static QString format(LogCategory category, LogLevel level, const QString& message)
    {
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss");
        return QString("[%1] [%2] [%3] %4")
            .arg(timeStr)
            .arg(categoryToString(category))
            .arg(levelToString(level))
            .arg(message);
    }

    // ⭐ 카테고리를 생략하면 System으로 처리하는 오버로딩 함수
    static QString format(LogLevel level, const QString& message)
    {
        return format(LogCategory::System, level, message);
    }

private:
    // Enum ➔ QString 변환 헬퍼 함수
    static QString categoryToString(LogCategory category)
    {
        switch (category) {
        case LogCategory::TCP:
            return "TCP";
        case LogCategory::Serial:
            return "SERIAL";
        case LogCategory::System:
            return "SYSTEM";
        }
        return "UNKNOWN";
    }

    static QString levelToString(LogLevel level)
    {
        switch (level) {
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Tx:
            return "TX";
        case LogLevel::Rx:
            return "RX";
        }
        return "INFO";
    }
};

#endif // LOGGER_H