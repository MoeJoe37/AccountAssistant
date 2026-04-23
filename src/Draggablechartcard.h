#pragma once
#include <QFrame>
#include <QLabel>
#include <QChartView>
#include <QPoint>
#include <QObject>
#include <QEvent>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QContextMenuEvent>

class DraggableChartCard : public QFrame
{
    Q_OBJECT
public:
    explicit DraggableChartCard(const QString& title,
                                QChartView*    view,
                                QWidget*       parent = nullptr);

    QChartView* chartView() const { return m_view; }
    QString title() const { return m_title; }
    int cardIndex() const { return m_index; }
    int flowIndex() const { return m_flowIndex; }
    void setCardIndex(int i) { m_index = i; }
    void setFlowIndex(int i) { m_flowIndex = i; }
    void setHighlight(bool on);

signals:
    void swapRequested(int fromIdx, int toIdx);
    void hideRequested(int cardIndex);
    void removeRequested(int cardIndex);
    void insertSeparatorRequested(int afterFlowIndex);
    void editRequested(int cardIndex);
    void duplicateRequested(int cardIndex);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dragLeaveEvent(QDragLeaveEvent*) override;
    void dropEvent(QDropEvent*) override;
    void contextMenuEvent(QContextMenuEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    int         m_index{0};
    int         m_flowIndex{0};
    QPoint      m_dragStart;
    bool        m_onHandle{false};
    QWidget*    m_handle{nullptr};
    QLabel*     m_titleLabel{nullptr};
    QChartView* m_view{nullptr};
    QString     m_title;
};
