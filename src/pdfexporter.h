#pragma once
#include <QString>
#include <QList>
#include "appdata.h"

class PdfExporter {
public:
    static bool exportToPdf(const QString& path,
                            const AppData& data,
                            const QList<ChartRequest>& requests,
                            const QList<ResultFlowItem>& flowOrder = QList<ResultFlowItem>(),
                            bool landscape = true);
};
