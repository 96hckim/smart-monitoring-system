#include "chartmanager.h"
#include <QPen>

ChartManager::ChartManager(QObject* parent)
    : QObject(parent)
    , m_chart(new QChart())
    , m_gasSeries(new QLineSeries())
    , m_thresholdSeries(new QLineSeries())
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_maxDataPoints(50) // 화면에 최신 50개 데이터만 유지 (스프레드시트 스크롤 효과)
    , m_dataIndex(0)
    , m_currentThreshold(3000) // 기본 임계값 3000
{
}

ChartManager::~ChartManager()
{
}

void ChartManager::initChart(QChartView* chartView)
{
    if (!chartView)
        return;

    // ----------------------------------------------------
    // [1. 범례 및 타이틀 설정]
    // ----------------------------------------------------
    m_chart->setTitle("실시간 가스 수치 추이");
    m_chart->legend()->setVisible(true);
    m_chart->legend()->setAlignment(Qt::AlignBottom);

    // ----------------------------------------------------
    // [2. 시리즈 스타일링 (선 색상 및 형태)]
    // ----------------------------------------------------
    // ① 가스 수치 실선 (파란색, 두께 2)
    m_gasSeries->setName("가스 수치");
    QPen gasPen(QColor(0, 122, 255));
    gasPen.setWidth(2);
    m_gasSeries->setPen(gasPen);

    // ② 임계값 가로선 (붉은색 점선, 두께 2)
    m_thresholdSeries->setName("위험 임계값");
    QPen thresholdPen(Qt::red);
    thresholdPen.setWidth(2);
    thresholdPen.setStyle(Qt::DashLine); // ⭐ 붉은 점선 스타일
    m_thresholdSeries->setPen(thresholdPen);

    m_chart->addSeries(m_gasSeries);
    m_chart->addSeries(m_thresholdSeries);

    // ----------------------------------------------------
    // [3. X / Y 축 범주 설정]
    // ----------------------------------------------------
    m_axisX->setRange(0, m_maxDataPoints);
    m_axisX->setLabelFormat("%d");
    m_axisX->setTitleText("샘플 번호");

    m_axisY->setRange(0, 4095); // 12-bit ADC 범위 (0 ~ 4095)
    m_axisY->setTitleText("Gas Value");

    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    // 시리즈를 축에 연결
    m_gasSeries->attachAxis(m_axisX);
    m_gasSeries->attachAxis(m_axisY);
    m_thresholdSeries->attachAxis(m_axisX);
    m_thresholdSeries->attachAxis(m_axisY);

    // ----------------------------------------------------
    // [4. QChartView 설정]
    // ----------------------------------------------------
    chartView->setChart(m_chart);
    chartView->setRenderHint(QPainter::Antialiasing); // 부드러운 선 표현

    // 최초 임계선 긋기
    updateThresholdLine();
}

// ⭐ STM32 데이터 수신 시 실시간 추가
void ChartManager::addGasData(int value)
{
    // 1. 신규 데이터 좌표 추가
    m_gasSeries->append(m_dataIndex, value);

    // 2. 데이터가 maxDataPoints를 초과하면 좌측(오래된 데이터) 삭제 및 X축 이동 (Slide)
    if (m_dataIndex > m_maxDataPoints) {
        m_gasSeries->remove(0); // 가장 오래된 점 삭제
        m_axisX->setRange(m_dataIndex - m_maxDataPoints, m_dataIndex);
    } else {
        m_axisX->setRange(0, m_maxDataPoints);
    }

    m_dataIndex++;

    // 3. X축 범위 변경에 맞춰 임계값 점선도 같이 길이 연장
    updateThresholdLine();
}

// ⭐ UI(SpinBox 등)에서 임계값이 바뀔 때 호출
void ChartManager::setThreshold(int threshold)
{
    m_currentThreshold = threshold;
    updateThresholdLine();
}

// 임계값 가로 점선 좌/우 끝 좌표 업데이트
void ChartManager::updateThresholdLine()
{
    m_thresholdSeries->clear();

    qreal minX = m_axisX->min();
    qreal maxX = m_axisX->max();

    // X축 시작점부터 끝점까지 동일한 Y(임계값) 높이로 직선 긋기
    m_thresholdSeries->append(minX, m_currentThreshold);
    m_thresholdSeries->append(maxX, m_currentThreshold);
}