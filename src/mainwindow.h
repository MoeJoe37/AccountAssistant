#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>
#include <QCloseEvent>
#include "appdata.h"
#include "datatablewidget.h"
#include "ClassicDataTableWidget.h"
#include "resultswidget.h"
#include "translations.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onCalculate();
    void onSaveData();
    void onImportData();
    void onExportPdf();
    void onSettings();
    void onClearData();
    void onEditCharts();

private:
    void buildUI();
    void applyLanguage(AppLanguage lang);
    void applyTheme();
    void retranslate();
    void switchTableView(bool classic);

    // Persistence helpers
    void loadSettings();
    void saveSettings();
    void loadTableDataLocally();
    void saveTableDataLocally();

    // Helper: active table interface wrappers
    AppData       collectTableData() const;
    void          setTableData(const AppData& d);
    void          clearTableData();
    void          updateTableCurrency();
    void          applyTableTheme();
    void          retranslateTable();

    QTabWidget*            m_tabs{};
    QStackedWidget*        m_tableStack{};
    DataTableWidget*       m_table{};        // card view (v3)
    ClassicDataTableWidget* m_classicTable{}; // spreadsheet view
    ResultsWidget*         m_results{};

    QPushButton* m_calcBtn{};
    QPushButton* m_saveBtn{};
    QPushButton* m_importBtn{};
    QPushButton* m_exportBtn{};
    QPushButton* m_settingsBtn{};
    QPushButton* m_clearBtn{};
    QLabel*      m_titleLabel{};

    AppData m_data;
    bool    m_hasResults{false};
    QList<ChartRequest>   m_lastChartRequests;
    QList<ResultFlowItem> m_lastFlowOrder;
};