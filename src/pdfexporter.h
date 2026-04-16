#pragma once
#include <QString>
#include <QList>
#include "appdata.h"
class ResultsWidget;

class PdfExporter {
public:
    static bool exportToPdf(const QString& path,
                            const AppData& data,
                            const ResultsWidget* results,
                            bool landscape = true);
};
