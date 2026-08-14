#ifndef CHARTMANAGER_H
#define CHARTMANAGER_H

#include <QObject>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE // Qt5 / Qt6 차트 네임스페이스 선언

    class ChartManager : public QObject {
    Q_OBJECT

public:
    explicit ChartManager(QObject* parent = nullptr);
    ~ChartManager();

    // 1. 차트 초기화 (UI의 QChartView와 연결)
    void initChart(QChartView* chartView);

    // 2. 실시간 가스 ADC 수치 데이터 추가
    void addGasData(int value);

    // 3. 임계값(Threshold) 가로 기준선 수치 변경
    void setThreshold(int threshold);

private:
    void updateThresholdLine(); // 임계선 X축 범위 갱신 헬퍼

    QChart* m_chart;
    QLineSeries* m_gasSeries; // 실시간 가스 수치 곡선 (파란색)
    QLineSeries* m_thresholdSeries; // 임계값 기준선 (붉은 점선)
    QValueAxis* m_axisX; // X축 (시간/데이터 수)
    QValueAxis* m_axisY; // Y축 (ADC 수치: 0 ~ 4095)

    int m_maxDataPoints; // 화면에 표시할 최대 데이터 개수 (예: 50개)
    int m_dataIndex; // 데이터 시퀀스 번호
    int m_currentThreshold; // 현재 설정된 임계값
};

#endif // CHARTMANAGER_H