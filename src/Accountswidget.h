#pragma once
#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QVector>
#include <QPair>
#include "appdata.h"

class Accountswidget : public QWidget
{
    Q_OBJECT
public:
    explicit Accountswidget(QWidget* parent = nullptr);

    AppData collectData() const;
    void setData(const AppData& data);
    void clearData();
    void applyTheme();
    void retranslate();

signals:
    void graphRequested(ChartKind kind);

private slots:
    void onAddAccount();
    void onSortChanged(int index);
    void onShowGraphs();
    void onRemoveRow();
    void onRowChanged();
    void onNameEdited();

private:
    struct RowWidgets {
        QWidget* row{};
        QLineEdit* name{};
        QDoubleSpinBox* amount{};
        QPushButton* removeBtn{};
        QString lastValidName;
    };

    void rebuildRows(const QList<AccountItem>& items);
    QList<AccountItem> currentItems() const;
    void sortAndRebuild();
    void addRow(const QString& name = QString(), double amount = 0.0);
    void updateGraphButtonMenu();
    void updatePrefixes();
    bool hasDuplicateName(const QString& name, const QLineEdit* except = nullptr) const;

    QScrollArea* m_scroll{};
    QWidget*     m_container{};
    QVBoxLayout* m_rowsLayout{};
    QLineEdit*   m_nameEdit{};
    QDoubleSpinBox* m_amountSpin{};
    QPushButton* m_addBtn{};
    QComboBox*   m_sortCombo{};
    QToolButton* m_graphBtn{};
    QLabel*      m_title{};
    QLabel*      m_subtitle{};
    QLabel*      m_empty{};
    QVector<RowWidgets> m_rows;
};
