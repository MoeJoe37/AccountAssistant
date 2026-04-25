#include "mainwindow.h"
#include "chartselectiondialog.h"
#include "settingsdialog.h"
#include "pdfexporter.h"
#include "Accountswidget.h"
#include "Supplierswidget.h"
#include "themebox.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QFontDatabase>
#include <QStringConverter>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QVector>
#include <QMap>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QDataStream>
#include <QComboBox>
#include <QSignalBlocker>
#include <QByteArray>
#include <QBuffer>
#include <QSettings>
#include <zlib.h>

// ─────────────────────────────────────────────────────────────────────────────
static const char* kGlobalSS = R"(
* { font-family:"Segoe UI", Tahoma, Arial, sans-serif; color:#c8d0ed; }

QMainWindow, QWidget#centralWidget { background:#0d1020; }
QWidget#dataTab, QWidget#dataTableRoot, QWidget#suppliersRoot { background:#0d1020; }

/* ── Top header ── */
QWidget#header {
    background:#0a0d1a;
    border-bottom:2px solid #1e2650;
}

/* ── Tab widget ── */
QTabWidget::pane {
    border:none; background:#0d1020;
}
QTabWidget::tab-bar { alignment:left; }
QTabBar::tab {
    background:#111526;
    color:#5a6490;
    font-weight:700;
    padding:12px 28px;
    border-bottom:3px solid transparent;
    margin-right:1px;
}
QTabBar::tab:selected {
    color:#c8d0ed;
    background:#0d1020;
    border-bottom:3px solid #4f86f7;
}
QTabBar::tab:hover:!selected {
    color:#8892b8;
    background:#0f1223;
}

/* ── Scroll bars ── */
QScrollBar:horizontal {
    background:#0d1020; height:8px; border-radius:4px;
}
QScrollBar::handle:horizontal {
    background:#2e3860; border-radius:4px; min-width:30px;
}
QScrollBar::handle:horizontal:hover { background:#4f86f7; }
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal { width:0; }

QScrollBar:vertical {
    background:#0d1020; width:8px; border-radius:4px;
}
QScrollBar::handle:vertical {
    background:#2e3860; border-radius:4px; min-height:30px;
}
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical { height:0; }
QScrollArea, QAbstractScrollArea { background:#0d1020; border:none; }
QScrollArea::viewport, QAbstractScrollArea::viewport { background:#0d1020; }
QMenu {
    background:#111526;
    color:#c8d0ed;
    border:1px solid #252b52;
}
QMenu::item:selected { background:#1e2445; }
QToolButton {
    background:#1a1f38;
}

/* ── Form / data panels ── */
QWidget#dataTab { background:#0d1020; }
QWidget#dataSubHeader { background:#111526; border-bottom:1px solid #1a1f38; }
QLabel#dataHint { color:#3a4470; background:transparent; }
QLineEdit, QDoubleSpinBox, QAbstractSpinBox {
    background:#252d4a;
    color:#c8d0ed;
    border:1px solid #3a4268;
    border-radius:5px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QAbstractSpinBox:focus { border-color:#4f86f7; }
)";

static const char* kGlobalSSLight = R"(
* { font-family:"Segoe UI", Tahoma, Arial, sans-serif; color:#1e2340; }

QMainWindow, QWidget#centralWidget { background:#f4f6fb; }
QWidget#dataTab, QWidget#dataTableRoot, QWidget#suppliersRoot { background:#f4f6fb; }

/* ── Top header ── */
QWidget#header {
    background:#ffffff;
    border-bottom:2px solid #dde2f0;
}

/* ── Tab widget ── */
QTabWidget::pane {
    border:none; background:#f4f6fb;
}
QTabWidget::tab-bar { alignment:left; }
QTabBar::tab {
    background:#eef0fa;
    color:#8892b8;
    font-weight:700;
    padding:12px 28px;
    border-bottom:3px solid transparent;
    margin-right:1px;
}
QTabBar::tab:selected {
    color:#1e2340;
    background:#f4f6fb;
    border-bottom:3px solid #4f86f7;
}
QTabBar::tab:hover:!selected {
    color:#5a6490;
    background:#e8ebf8;
}

/* ── Scroll bars ── */
QScrollBar:horizontal {
    background:#f4f6fb; height:8px; border-radius:4px;
}
QScrollBar::handle:horizontal {
    background:#c8d0ed; border-radius:4px; min-width:30px;
}
QScrollBar::handle:horizontal:hover { background:#4f86f7; }
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal { width:0; }

QScrollBar:vertical {
    background:#f4f6fb; width:8px; border-radius:4px;
}
QScrollBar::handle:vertical {
    background:#c8d0ed; border-radius:4px; min-height:30px;
}
QScrollBar::handle:vertical:hover { background:#4f86f7; }
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical { height:0; }
QScrollArea, QAbstractScrollArea { background:#f4f6fb; border:none; }
QScrollArea::viewport, QAbstractScrollArea::viewport { background:#f4f6fb; }
QMenu {
    background:#ffffff;
    color:#1e2340;
    border:1px solid #dde2f0;
}
QMenu::item:selected { background:#eef0fa; }
QToolButton {
    background:#ffffff;
}

/* ── Form / data panels ── */
QWidget#dataTab { background:#f4f6fb; }
QWidget#dataSubHeader { background:#ffffff; border-bottom:1px solid #dde2f0; }
QLabel#dataHint { color:#6b7280; background:transparent; }
QLineEdit, QDoubleSpinBox, QAbstractSpinBox {
    background:#ffffff;
    color:#1e2340;
    border:1px solid #cfd7ea;
    border-radius:5px;
}
QLineEdit:focus, QDoubleSpinBox:focus, QAbstractSpinBox:focus { border-color:#4f86f7; }
)";

namespace {

struct ZipEntry {
    QString name;
    QByteArray data;
    quint32 crc{0};
    quint32 offset{0};
};

static quint32 crc32TableValue(quint32 r)
{
    for (int i = 0; i < 8; ++i)
        r = (r & 1u) ? (0xEDB88320u ^ (r >> 1)) : (r >> 1);
    return r;
}

static quint32 crc32Bytes(const QByteArray& data)
{
    static quint32 table[256] = {0};
    static bool init = false;
    if (!init) {
        for (quint32 i = 0; i < 256; ++i)
            table[i] = crc32TableValue(i);
        init = true;
    }
    quint32 crc = 0xFFFFFFFFu;
    for (unsigned char ch : data)
        crc = table[(crc ^ ch) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

static QString xmlEscape(const QString& s)
{
    QString out = s;
    out.replace('&', "&amp;");
    out.replace('<', "&lt;");
    out.replace('>', "&gt;");
    out.replace('"', "&quot;");
    out.replace('\'', "&apos;");
    return out;
}

enum class XlsxSheetKind { DataEntry, Expenses, Suppliers, AllData };

static QByteArray makeWorksheetXml(const QStringList& headers, const QList<QStringList>& rows)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter w(&buffer);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("worksheet");
    w.writeDefaultNamespace("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    w.writeStartElement("sheetData");

    auto writeTextCell = [&](const QString& ref, const QString& text) {
        w.writeStartElement("c");
        w.writeAttribute("r", ref);
        w.writeAttribute("t", "inlineStr");
        w.writeStartElement("is");
        w.writeTextElement("t", text);
        w.writeEndElement();
        w.writeEndElement();
    };

    auto writeRow = [&](int rowNum, const QStringList& cols) {
        w.writeStartElement("row");
        w.writeAttribute("r", QString::number(rowNum));
        for (int i = 0; i < cols.size(); ++i) {
            QString col;
            int n = i;
            do {
                col.prepend(QChar('A' + (n % 26)));
                n = n / 26 - 1;
            } while (n >= 0);
            writeTextCell(QString("%1%2").arg(col).arg(rowNum), cols[i]);
        }
        w.writeEndElement();
    };

    writeRow(1, {"ACCOUNT_ASSISTANT_EXPORT", "6.0.0"});
    writeRow(2, headers);
    int row = 3;
    for (const auto& r : rows)
        writeRow(row++, r);

    w.writeEndElement();
    w.writeEndElement();
    w.writeEndDocument();
    return ba;
}

static QByteArray makeWorkbookXml(const QStringList& sheetNames)
{
    QString sheetsXml;
    for (int i = 0; i < sheetNames.size(); ++i)
        sheetsXml += QString("  <sheet name=\"%1\" sheetId=\"%2\" r:id=\"rId%2\"/>\n")
                         .arg(xmlEscape(sheetNames[i])).arg(i + 1);
    return QString(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <sheets>
%1  </sheets>
</workbook>)xml").arg(sheetsXml).toUtf8();
}

static QByteArray makeWorkbookRelsXml(const QStringList& sheetNames)
{
    QString rels;
    for (int i = 0; i < sheetNames.size(); ++i)
        rels += QString("  <Relationship Id=\"rId%1\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet\" Target=\"worksheets/sheet%1.xml\"/>\n").arg(i + 1);
    return QString(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
%1</Relationships>)xml").arg(rels).toUtf8();
}

static QByteArray makeRootRelsXml()
{
    return QByteArray(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)xml");
}

static QByteArray makeContentTypesXml(const QStringList& sheetNames)
{
    QString overrides;
    for (int i = 0; i < sheetNames.size(); ++i)
        overrides += QString("  <Override PartName=\"/xl/worksheets/sheet%1.xml\" ContentType=\"application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml\"/>\n").arg(i + 1);
    return QString(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
%1</Types>)xml").arg(overrides).toUtf8();
}

static void writeU16(QDataStream& out, quint16 v) { out << v; }
static void writeU32(QDataStream& out, quint32 v) { out << v; }

static bool writeStoredZip(const QString& path, const QVector<ZipEntry>& entries)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    QDataStream out(&f);
    out.setByteOrder(QDataStream::LittleEndian);

    QVector<ZipEntry> local = entries;
    for (auto& e : local) {
        e.crc = crc32Bytes(e.data);
        e.offset = quint32(f.pos());
        writeU32(out, 0x04034b50u);
        writeU16(out, 20);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0);
        writeU32(out, e.crc);
        writeU32(out, quint32(e.data.size()));
        writeU32(out, quint32(e.data.size()));
        const QByteArray name = e.name.toUtf8();
        writeU16(out, quint16(name.size()));
        writeU16(out, 0);
        if (f.write(name) != name.size()) return false;
        if (f.write(e.data) != e.data.size()) return false;
    }
    const quint32 centralOffset = quint32(f.pos());
    for (const auto& e : local) {
        const QByteArray name = e.name.toUtf8();
        writeU32(out, 0x02014b50u);
        writeU16(out, 20);
        writeU16(out, 20);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0); writeU16(out, 0);
        writeU32(out, e.crc);
        writeU32(out, quint32(e.data.size()));
        writeU32(out, quint32(e.data.size()));
        writeU16(out, quint16(name.size()));
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU16(out, 0);
        writeU32(out, 0);
        writeU32(out, e.offset);
        if (f.write(name) != name.size()) return false;
    }
    const quint32 centralSize = quint32(f.pos()) - centralOffset;
    writeU32(out, 0x06054b50u);
    writeU16(out, 0); writeU16(out, 0);
    writeU16(out, quint16(local.size()));
    writeU16(out, quint16(local.size()));
    writeU32(out, centralSize);
    writeU32(out, centralOffset);
    writeU16(out, 0);
    return true;
}

static bool saveWorkbookXlsx(const QString& path, const QList<QPair<QString, QByteArray>>& sheets)
{
    if (sheets.isEmpty()) return false;
    QStringList sheetNames;
    for (const auto& s : sheets) sheetNames << s.first;
    QVector<ZipEntry> entries;
    entries.push_back({"[Content_Types].xml", makeContentTypesXml(sheetNames)});
    entries.push_back({"_rels/.rels", makeRootRelsXml()});
    entries.push_back({"xl/workbook.xml", makeWorkbookXml(sheetNames)});
    entries.push_back({"xl/_rels/workbook.xml.rels", makeWorkbookRelsXml(sheetNames)});
    for (int i = 0; i < sheets.size(); ++i)
        entries.push_back({QString("xl/worksheets/sheet%1.xml").arg(i + 1), sheets[i].second});
    return writeStoredZip(path, entries);
}

static QList<QPair<QString, QByteArray>> buildSheetsForExport(const AppData& data, XlsxSheetKind kind)
{
    QList<QPair<QString, QByteArray>> sheets;
    auto addSheet = [&](const QString& sheetName, const QStringList& headers, const QList<QStringList>& rows) {
        sheets.push_back({sheetName, makeWorksheetXml(headers, rows)});
    };

    switch (kind) {
    case XlsxSheetKind::DataEntry: {
        const bool ongoing = (data.inventoryMode == InventoryMode::Ongoing);
        QStringList headers = ongoing
            ? QStringList{"DATA_ENTRY", "Month", "Sales", "Sales Return", "Supplier Purchases", "Supplier Payments", "COGS Input"}
            : QStringList{"DATA_ENTRY", "Month", "Sales", "Sales Return", "Supplier Purchases", "Supplier Payments", "Inventory First", "Inventory Last"};
        QList<QStringList> rows;
        const auto months = monthNames();
        for (int i = 0; i < 12; ++i) {
            const auto& m = data.months[i];
            if (ongoing) {
                rows.push_back({"DATA_ENTRY", months.value(i), QString::number(m.sales, 'f', 2), QString::number(m.salesReturn, 'f', 2), QString::number(m.supplierPurchases, 'f', 2), QString::number(m.supplierPayments, 'f', 2), QString::number(m.cogsInput, 'f', 2)});
            } else {
                rows.push_back({"DATA_ENTRY", months.value(i), QString::number(m.sales, 'f', 2), QString::number(m.salesReturn, 'f', 2), QString::number(m.supplierPurchases, 'f', 2), QString::number(m.supplierPayments, 'f', 2), QString::number(m.inventoryFirst, 'f', 2), QString::number(m.inventoryLast, 'f', 2)});
            }
        }
        addSheet("DATA_ENTRY", headers, rows);
        break;
    }
    case XlsxSheetKind::Expenses: {
        QStringList headers = {"EXPENSES", "Account Name", "Account Type", "Amount"};
        QList<QStringList> rows;
        for (const auto& a : data.accounts)
            rows.push_back({"EXPENSES", a.name, accountTypeDisplayName(a.type), QString::number(a.amount, 'f', 2)});
        addSheet("EXPENSES", headers, rows);
        break;
    }
    case XlsxSheetKind::Suppliers: {
        QStringList headers = {"SUPPLIERS", "Month", "Supplier Name", "Previous Balance", "Purchases", "Total Debt", "Payments", "Payment % of Purchases", "Payment % of Total Debt", "Supplier Balance"};
        QList<QStringList> rows;
        const auto months = monthNames();
        for (int i = 0; i < 12; ++i) {
            const auto entries = !data.supplierEntries[i].isEmpty() ? data.supplierEntries[i] : QList<SupplierEntry>{};
            if (entries.isEmpty()) {
                SupplierEntry e;
                e.name = data.suppliers[i].supplierName;
                e.purchases = data.suppliers[i].purchases;
                e.payments = data.suppliers[i].payments;
                e.totalDebt = e.previousBalance + e.purchases;
                rows.push_back({"SUPPLIERS", months.value(i), e.name, QString::number(e.previousBalance,'f',2), QString::number(e.purchases,'f',2), QString::number(e.totalDebt,'f',2), QString::number(e.payments,'f',2), QString::number(e.paymentPctOfPurchases(),'f',2), QString::number(e.paymentPctOfTotalDebt(),'f',2), QString::number(e.supplierBalance(),'f',2)});
            } else {
                for (const auto& e : entries)
                    rows.push_back({"SUPPLIERS", months.value(i), e.name, QString::number(e.previousBalance,'f',2), QString::number(e.purchases,'f',2), QString::number(e.totalDebt > 0.0 ? e.totalDebt : (e.previousBalance + e.purchases),'f',2), QString::number(e.payments,'f',2), QString::number(e.paymentPctOfPurchases(),'f',2), QString::number(e.paymentPctOfTotalDebt(),'f',2), QString::number(e.supplierBalance(),'f',2)});
            }
        }
        addSheet("SUPPLIERS", headers, rows);
        break;
    }
    case XlsxSheetKind::AllData:
        sheets = buildSheetsForExport(data, XlsxSheetKind::DataEntry);
        sheets.append(buildSheetsForExport(data, XlsxSheetKind::Expenses));
        sheets.append(buildSheetsForExport(data, XlsxSheetKind::Suppliers));
        break;
    }
    return sheets;
}

static bool saveSheetXlsx(const QString& path, const QString& sheetName, const QStringList& headers, const QList<QStringList>& rows)
{
    return saveWorkbookXlsx(path, {{sheetName, makeWorksheetXml(headers, rows)}});
}

static bool saveAppDataXlsx(const QString& path, const AppData& data, XlsxSheetKind kind = XlsxSheetKind::DataEntry)
{
    return saveWorkbookXlsx(path, buildSheetsForExport(data, kind));
}

// Decompress raw deflate (ZIP method 8) using zlib
static QByteArray inflateRawDeflate(const QByteArray& compressed, quint32 uncompSize)
{
    if (compressed.isEmpty())
        return {};

    z_stream strm{};
    strm.avail_in = uInt(compressed.size());
    strm.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.constData()));
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK)
        return {};

    QByteArray out;
    if (uncompSize > 0)
        out.reserve(int(uncompSize));

    char buffer[16384];
    int ret = Z_OK;
    while (ret == Z_OK) {
        strm.avail_out = sizeof(buffer);
        strm.next_out = reinterpret_cast<Bytef*>(buffer);
        ret = inflate(&strm, Z_NO_FLUSH);
        const int produced = int(sizeof(buffer) - strm.avail_out);
        if (produced > 0)
            out.append(buffer, produced);
    }

    inflateEnd(&strm);
    if (ret != Z_STREAM_END)
        return {};
    return out;
}

// Read all named entries from a ZIP file (supports stored and deflate)
static bool readZipEntries(const QString& path, QMap<QString, QByteArray>* out)
{
    if (!out) return false;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray all = f.readAll();
    if (all.size() < 22) return false;

    auto rd16 = [&](int off) -> quint16 {
        return quint16(quint8(all[off])) | (quint16(quint8(all[off+1])) << 8);
    };
    auto rd32 = [&](int off) -> quint32 {
        return quint32(quint8(all[off]))
             | (quint32(quint8(all[off+1])) << 8)
             | (quint32(quint8(all[off+2])) << 16)
             | (quint32(quint8(all[off+3])) << 24);
    };

    // Find End of Central Directory
    int eocd = -1;
    for (int i = all.size() - 22; i >= qMax(0, all.size() - 65558); --i) {
        if (rd32(i) == 0x06054b50u) { eocd = i; break; }
    }
    if (eocd < 0) return false;

    const quint16 numEntries   = rd16(eocd + 10);
    const quint32 centralOff   = rd32(eocd + 16);
    int pos = int(centralOff);

    for (quint16 idx = 0; idx < numEntries && pos + 46 <= int(all.size()); ++idx) {
        if (rd32(pos) != 0x02014b50u) return false;
        const quint16 method      = rd16(pos + 10);
        const quint32 compSize    = rd32(pos + 20);
        const quint32 uncompSize  = rd32(pos + 24);
        const quint16 nameLen     = rd16(pos + 28);
        const quint16 extraLen    = rd16(pos + 30);
        const quint16 commentLen  = rd16(pos + 32);
        const quint32 localOff    = rd32(pos + 42);

        const QString entryName = QString::fromUtf8(all.mid(pos + 46, nameLen));
        pos += 46 + nameLen + extraLen + commentLen;

        // Navigate to local file header
        const int localPos = int(localOff);
        if (localPos + 30 > int(all.size()) || rd32(localPos) != 0x04034b50u) continue;
        const quint16 localNameLen  = rd16(localPos + 26);
        const quint16 localExtraLen = rd16(localPos + 28);
        const int dataPos = localPos + 30 + localNameLen + localExtraLen;
        if (dataPos < 0 || dataPos + int(compSize) > int(all.size())) continue;

        QByteArray raw = all.mid(dataPos, int(compSize));

        if (method == 0) {
            // Stored – use as-is
            (*out)[entryName] = raw;
        } else if (method == 8) {
            // Deflate – decompress with raw zlib
            QByteArray decompressed = inflateRawDeflate(raw, uncompSize);
            if (!decompressed.isEmpty())
                (*out)[entryName] = decompressed;
        }
        // Other methods (e.g., bzip2) are rare in xlsx – skip silently
    }
    return true;
}

// Parse xl/sharedStrings.xml → list of strings indexed by position
static QList<QString> parseSharedStrings(const QByteArray& xml)
{
    QList<QString> result;
    if (xml.isEmpty()) return result;

    QXmlStreamReader xr(xml);
    bool inSharedString = false;
    QString current;
    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("si")) {
            inSharedString = true;
            current.clear();
        } else if (inSharedString && xr.isStartElement() && xr.name() == QLatin1String("t")) {
            current += xr.readElementText();
        } else if (xr.isEndElement() && xr.name() == QLatin1String("si")) {
            result.append(current);
            inSharedString = false;
        }
    }
    return result;
}

// Convert column letter(s) from a cell reference (e.g. "B3") to 0-based index
static int colLetterToIndex(const QString& ref)
{
    int col = 0;
    for (QChar ch : ref) {
        if (!ch.isLetter()) break;
        col = col * 26 + (ch.toUpper().unicode() - 'A' + 1);
    }
    return col - 1;  // 0-based
}

static QList<QString> g_sharedStringsXlsx;

static QList<QMap<int, QString>> parseWorksheetRows(const QByteArray& sheet)
{
    QList<QMap<int, QString>> rows;
    if (sheet.isEmpty()) return rows;
    QXmlStreamReader xr(sheet);
    int rowNum = 0;
    int sequentialRowNum = 0;
    QMap<int, QString> rowCells;
    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("row")) {
            rowCells.clear();
            rowNum = xr.attributes().value("r").toInt();
            if (rowNum <= 0)
                rowNum = ++sequentialRowNum;
            else
                sequentialRowNum = rowNum;
        } else if (xr.isStartElement() && xr.name() == QLatin1String("c")) {
            const QString ref = xr.attributes().value("r").toString();
            const QString cellType = xr.attributes().value("t").toString();
            int colIdx = colLetterToIndex(ref);
            if (colIdx < 0)
                colIdx = rowCells.isEmpty() ? 0 : (rowCells.lastKey() + 1);
            QString val;
            while (!(xr.isEndElement() && xr.name() == QLatin1String("c")) && !xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement()) {
                    if (xr.name() == QLatin1String("v")) {
                        QString raw = xr.readElementText();
                        if (cellType == QStringLiteral("s")) {
                            bool ok = false;
                            int si = raw.toInt(&ok);
                            val = (ok && si >= 0 && si < g_sharedStringsXlsx.size()) ? g_sharedStringsXlsx[si] : raw;
                        } else {
                            val = raw;
                        }
                    } else if (xr.name() == QLatin1String("t")) {
                        val += xr.readElementText();
                    }
                }
            }
            if (colIdx >= 0) rowCells[colIdx] = val;
        } else if (xr.isEndElement() && xr.name() == QLatin1String("row")) {
            while (rows.size() < rowNum) rows.append(QMap<int, QString>{});
            rows[rowNum - 1] = rowCells;
        }
    }
    return rows;
}

static bool strictToDouble(const QString& text, double* out)
{
    if (!out) return false;
    QString t = text.trimmed();
    if (t.isEmpty()) { *out = 0.0; return true; }

    t.replace(QChar(0x00A0), QChar(' '));
    t.replace(QChar(0x202F), QChar(' '));
    t.replace(QChar(0x2007), QChar(' '));
    t = t.trimmed();
    if (t.startsWith('='))
        t.remove(0, 1);
    if (t.startsWith(QChar('\'')))
        t.remove(0, 1);

    bool negativeByParens = false;
    if (t.startsWith('(') && t.endsWith(')')) {
        negativeByParens = true;
        t = t.mid(1, t.size() - 2).trimmed();
    }

    auto tryParse = [&](QString s, double* valueOut) -> bool {
        bool ok = false;
        double v = QLocale::c().toDouble(s, &ok);
        if (!ok) v = QLocale(QLocale::English, QLocale::UnitedStates).toDouble(s, &ok);
        if (!ok) v = QLocale().toDouble(s, &ok);
        if (!ok) {
            QString compact = s;
            compact.remove(' ');
            v = QLocale::c().toDouble(compact, &ok);
            if (!ok) {
                QString noCommas = compact;
                noCommas.remove(',');
                v = QLocale::c().toDouble(noCommas, &ok);
            }
            if (!ok) {
                QString decimalComma = compact;
                decimalComma.replace(',', '.');
                v = QLocale::c().toDouble(decimalComma, &ok);
            }
        }
        if (!ok)
            return false;
        *valueOut = v;
        return true;
    };

    double parsed = 0.0;
    if (!tryParse(t, &parsed))
        return false;
    *out = negativeByParens ? -parsed : parsed;
    return true;
}

static QString g_lastImportError;
static QString g_lastImportMarker;


static QString normalizedHeaderCell(QString text)
{
    text.replace(QChar(0x00A0), QChar(' '));
    text.replace(QChar(0x202F), QChar(' '));
    text = text.trimmed().toCaseFolded();
    QString collapsed;
    bool lastSpace = false;
    for (QChar ch : text) {
        if (ch.isSpace()) {
            if (!lastSpace)
                collapsed += QChar(' ');
            lastSpace = true;
        } else {
            collapsed += ch;
            lastSpace = false;
        }
    }
    return collapsed;
}

static bool headerMatches(const QMap<int, QString>& row, const QStringList& expected)
{
    for (int i = 0; i < expected.size(); ++i) {
        if (normalizedHeaderCell(row.value(i)) != normalizedHeaderCell(expected.value(i)))
            return false;
    }
    return true;
}
static QString normalizeWorkbookTargetPath(QString target)
{
    target = target.trimmed();
    if (target.isEmpty())
        return {};
    target.replace('\\', '/');
    while (target.startsWith('/'))
        target.remove(0, 1);
    if (target.startsWith(QStringLiteral("../"))) {
        while (target.startsWith(QStringLiteral("../")))
            target.remove(0, 3);
    }
    if (target.startsWith(QStringLiteral("worksheets/")))
        target.prepend(QStringLiteral("xl/"));
    else if (!target.startsWith(QStringLiteral("xl/")))
        target = QStringLiteral("xl/") + target;
    target.replace(QStringLiteral("xl/xl/"), QStringLiteral("xl/"));
    return target;
}

static QStringList workbookSheetPaths(const QMap<QString, QByteArray>& entries)
{
    QStringList result;
    const QByteArray relsXml = entries.value(QStringLiteral("xl/_rels/workbook.xml.rels"));
    if (!relsXml.isEmpty()) {
        QXmlStreamReader xr(relsXml);
        QMap<QString, QString> relTargets;
        while (!xr.atEnd()) {
            xr.readNext();
            if (xr.isStartElement() && xr.name() == QLatin1String("Relationship")) {
                const auto attrs = xr.attributes();
                relTargets[attrs.value("Id").toString()] = normalizeWorkbookTargetPath(attrs.value("Target").toString());
            }
        }
        const QByteArray workbookXml = entries.value(QStringLiteral("xl/workbook.xml"));
        if (!workbookXml.isEmpty()) {
            QXmlStreamReader wr(workbookXml);
            while (!wr.atEnd()) {
                wr.readNext();
                if (wr.isStartElement() && wr.name() == QLatin1String("sheet")) {
                    const QString rid = wr.attributes().value(QStringLiteral("http://schemas.openxmlformats.org/officeDocument/2006/relationships"), QStringLiteral("id")).toString();
                    const QString target = relTargets.value(rid);
                    if (!target.isEmpty() && entries.contains(target))
                        result << target;
                }
            }
        }
    }
    if (result.isEmpty()) {
        for (const QString& key : entries.keys())
            if (key.contains("worksheets/sheet") && key.endsWith(".xml")) result << key;
        result.sort();
    }
    return result;
}

static bool rowHasAnyData(const QMap<int, QString>& row)
{
    for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
        if (!it.value().trimmed().isEmpty()) return true;
    }
    return false;
}

static bool isKnownImportMarker(const QString& text)
{
    const QString marker = text.trimmed().toUpper();
    return marker == QStringLiteral("DATA_ENTRY") || marker == QStringLiteral("EXPENSES") || marker == QStringLiteral("SUPPLIERS") || marker == QStringLiteral("ALL_DATA");
}

static bool parseSingleSheetRows(const QList<QMap<int, QString>>& rows, AppData* data, QString* markerOut)
{
    if (!data) return false;

    auto findNextNonEmptyRow = [&](int startIndex) -> int {
        for (int i = qMax(0, startIndex); i < rows.size(); ++i) {
            if (rowHasAnyData(rows[i])) return i;
        }
        return -1;
    };

    auto rowContainsHeader = [](const QMap<int, QString>& row, const QString& header) -> bool {
        const QString wanted = normalizedHeaderCell(header);
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (normalizedHeaderCell(it.value()) == wanted)
                return true;
        }
        return false;
    };

    auto headerColumns = [](const QMap<int, QString>& row) -> QMap<QString, int> {
        QMap<QString, int> cols;
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            const QString key = normalizedHeaderCell(it.value());
            if (!key.isEmpty() && !cols.contains(key))
                cols[key] = it.key();
        }
        return cols;
    };

    auto findColumn = [](const QMap<QString, int>& cols, const QStringList& names) -> int {
        for (const QString& name : names) {
            const QString key = normalizedHeaderCell(name);
            if (cols.contains(key))
                return cols.value(key);
        }
        return -1;
    };

    auto cell = [](const QMap<int, QString>& row, int col) -> QString {
        return col >= 0 ? row.value(col).trimmed() : QString();
    };

    int signatureRowIndex = findNextNonEmptyRow(0);
    int searchStart = signatureRowIndex;
    if (signatureRowIndex >= 0 && rows[signatureRowIndex].value(0).trimmed().toUpper() == QStringLiteral("ACCOUNT_ASSISTANT_EXPORT"))
        searchStart = signatureRowIndex + 1;
    if (searchStart < 0) return false;

    int headerRowIndex = -1;
    QString marker;
    for (int i = searchStart; i < rows.size() && i < searchStart + 30; ++i) {
        if (!rowHasAnyData(rows[i]))
            continue;
        const QString first = rows[i].value(0).trimmed().toUpper();
        if (first == QStringLiteral("DATA_ENTRY") || first == QStringLiteral("EXPENSES") ||
            first == QStringLiteral("SUPPLIERS") || first == QStringLiteral("ALL_DATA")) {
            marker = first;
            headerRowIndex = i;
            break;
        }
        if (rowContainsHeader(rows[i], QStringLiteral("Account Name")) && rowContainsHeader(rows[i], QStringLiteral("Amount"))) {
            marker = QStringLiteral("EXPENSES");
            headerRowIndex = i;
            break;
        }
        if (rowContainsHeader(rows[i], QStringLiteral("Supplier Name")) && rowContainsHeader(rows[i], QStringLiteral("Previous Balance"))) {
            marker = QStringLiteral("SUPPLIERS");
            headerRowIndex = i;
            break;
        }
        if (rowContainsHeader(rows[i], QStringLiteral("Month")) && rowContainsHeader(rows[i], QStringLiteral("Sales")) &&
            rowContainsHeader(rows[i], QStringLiteral("Supplier Purchases"))) {
            marker = QStringLiteral("DATA_ENTRY");
            headerRowIndex = i;
            break;
        }
        if (rowContainsHeader(rows[i], QStringLiteral("Section"))) {
            marker = QStringLiteral("ALL_DATA");
            headerRowIndex = i;
            break;
        }
    }

    if (headerRowIndex < 0 || marker.isEmpty())
        return false;
    if (markerOut) *markerOut = marker;

    const QMap<QString, int> cols = headerColumns(rows[headerRowIndex]);
    const int dataStartRowIndex = headerRowIndex + 1;
    *data = AppData{};

    auto parseRequiredNumber = [&](const QMap<int, QString>& row, int col, double& target, const QString& fieldName) -> bool {
        if (col < 0) {
            g_lastImportError = tr_auto_import_failed_the_workbook_format_does_not_ac2d8f1c() + QStringLiteral(" ") + fieldName;
            return false;
        }
        double v = 0.0;
        if (!strictToDouble(row.value(col), &v)) {
            g_lastImportError = tr_auto_import_error_a_numeric_field_contains_text_ba89d1c8() + QStringLiteral(" ") + fieldName;
            return false;
        }
        target = v;
        return true;
    };

    if (marker == QStringLiteral("DATA_ENTRY")) {
        const int cMonth = findColumn(cols, {"Month"});
        const int cSales = findColumn(cols, {"Sales"});
        const int cSalesReturn = findColumn(cols, {"Sales Return"});
        const int cSupplierPurchases = findColumn(cols, {"Supplier Purchases"});
        const int cSupplierPayments = findColumn(cols, {"Supplier Payments"});
        const int cInventoryFirst = findColumn(cols, {"Inventory First"});
        const int cInventoryLast = findColumn(cols, {"Inventory Last"});
        const int cCogsInput = findColumn(cols, {"COGS Input"});
        const bool ongoingMode = (cCogsInput >= 0);

        if (cMonth < 0 || cSales < 0 || cSalesReturn < 0 || cSupplierPurchases < 0 || cSupplierPayments < 0 ||
            (ongoingMode && cCogsInput < 0) || (!ongoingMode && (cInventoryFirst < 0 || cInventoryLast < 0))) {
            g_lastImportError = tr_auto_import_failed_the_data_entry_sheet_does_no_57ac18f5();
            return false;
        }

        data->inventoryMode = ongoingMode ? InventoryMode::Ongoing : InventoryMode::Periodic;
        const auto months = monthNames();
        QMap<QString, int> monthLookup;
        for (int i = 0; i < months.size(); ++i)
            monthLookup[normalizedHeaderCell(months[i])] = i;

        for (int r = dataStartRowIndex; r < rows.size(); ++r) {
            const auto& row = rows[r];
            if (!rowHasAnyData(row)) continue;
            const QString rowMarker = row.value(0).trimmed().toUpper();
            if (isKnownImportMarker(rowMarker) && rowMarker != marker) continue;
            const int monthIndex = monthLookup.value(normalizedHeaderCell(cell(row, cMonth)), -1);
            if (monthIndex < 0 || monthIndex >= 12) continue;

            auto& m = data->months[monthIndex];
            if (!parseRequiredNumber(row, cSales, m.sales, QStringLiteral("Sales"))) return false;
            if (!parseRequiredNumber(row, cSalesReturn, m.salesReturn, QStringLiteral("Sales Return"))) return false;
            if (!parseRequiredNumber(row, cSupplierPurchases, m.supplierPurchases, QStringLiteral("Supplier Purchases"))) return false;
            if (!parseRequiredNumber(row, cSupplierPayments, m.supplierPayments, QStringLiteral("Supplier Payments"))) return false;
            m.expenseAccount.clear();
            m.expenseAmount = 0.0;
            m.supplierName.clear();
            if (ongoingMode) {
                if (!parseRequiredNumber(row, cCogsInput, m.cogsInput, QStringLiteral("COGS Input"))) return false;
                m.inventoryFirst = 0.0;
                m.inventoryLast = 0.0;
            } else {
                if (!parseRequiredNumber(row, cInventoryFirst, m.inventoryFirst, QStringLiteral("Inventory First"))) return false;
                if (!parseRequiredNumber(row, cInventoryLast, m.inventoryLast, QStringLiteral("Inventory Last"))) return false;
                m.cogsInput = 0.0;
            }
        }
    } else if (marker == QStringLiteral("EXPENSES")) {
        const int cName = findColumn(cols, {"Account Name", "Expense Account"});
        const int cType = findColumn(cols, {"Account Type", "Type"});
        const int cAmount = findColumn(cols, {"Amount", "Expense Amount"});
        if (cName < 0 || cType < 0 || cAmount < 0) {
            g_lastImportError = tr_auto_import_failed_the_expenses_sheet_does_not__1996b8cb();
            return false;
        }
        auto parseType = [](const QString& text) {
            const QString k = text.trimmed().toCaseFolded();
            if (k.contains(tr_import_account_type_receivable_ar_keyword()) || k.contains(QStringLiteral("receivable"))) return AccountType::Receivable;
            return AccountType::Payable;
        };
        for (int r = dataStartRowIndex; r < rows.size(); ++r) {
            const auto& row = rows[r];
            if (!rowHasAnyData(row)) continue;
            const QString rowMarker = row.value(0).trimmed().toUpper();
            if (isKnownImportMarker(rowMarker) && rowMarker != marker) continue;
            const QString name = cell(row, cName);
            if (name.isEmpty() && cell(row, cAmount).isEmpty()) continue;
            AccountItem a;
            a.name = name;
            a.type = parseType(cell(row, cType));
            if (!parseRequiredNumber(row, cAmount, a.amount, QStringLiteral("Amount"))) return false;
            if (!a.name.trimmed().isEmpty())
                data->accounts.append(a);
        }
    } else if (marker == QStringLiteral("SUPPLIERS")) {
        const int cMonth = findColumn(cols, {"Month"});
        const int cName = findColumn(cols, {"Supplier Name"});
        const int cPrevious = findColumn(cols, {"Previous Balance"});
        const int cPurchases = findColumn(cols, {"Purchases"});
        const int cTotalDebt = findColumn(cols, {"Total Debt"});
        const int cPayments = findColumn(cols, {"Payments"});
        if (cMonth < 0 || cName < 0 || cPrevious < 0 || cPurchases < 0 || cTotalDebt < 0 || cPayments < 0) {
            g_lastImportError = tr_auto_import_failed_the_suppliers_sheet_does_not_c5805d00();
            return false;
        }

        const auto months = monthNames();
        QMap<QString, int> monthLookup;
        for (int i = 0; i < months.size(); ++i)
            monthLookup[normalizedHeaderCell(months[i])] = i;
        for (int r = dataStartRowIndex; r < rows.size(); ++r) {
            const auto& row = rows[r];
            if (!rowHasAnyData(row)) continue;
            const QString rowMarker = row.value(0).trimmed().toUpper();
            if (isKnownImportMarker(rowMarker) && rowMarker != marker) continue;
            const int monthIndex = monthLookup.value(normalizedHeaderCell(cell(row, cMonth)), -1);
            if (monthIndex < 0 || monthIndex >= 12) continue;
            SupplierEntry e;
            e.name = cell(row, cName);
            if (e.name.trimmed().isEmpty() && cell(row, cPurchases).isEmpty() && cell(row, cPayments).isEmpty()) continue;
            if (!parseRequiredNumber(row, cPrevious, e.previousBalance, QStringLiteral("Previous Balance"))) return false;
            if (!parseRequiredNumber(row, cPurchases, e.purchases, QStringLiteral("Purchases"))) return false;
            if (!parseRequiredNumber(row, cTotalDebt, e.totalDebt, QStringLiteral("Total Debt"))) return false;
            if (!parseRequiredNumber(row, cPayments, e.payments, QStringLiteral("Payments"))) return false;
            if (!e.name.trimmed().isEmpty())
                data->supplierEntries[monthIndex].append(e);
        }
        for (int i = 0; i < 12; ++i) {
            double p = 0.0, pay = 0.0; QString first;
            for (const auto& e : data->supplierEntries[i]) {
                p += e.purchases;
                pay += e.payments;
                if (first.isEmpty() && !e.name.trimmed().isEmpty()) first = e.name.trimmed();
            }
            data->suppliers[i].supplierName = first;
            data->suppliers[i].purchases = p;
            data->suppliers[i].payments = pay;
        }
    } else if (marker == QStringLiteral("ALL_DATA")) {
        // Legacy single-sheet format. New ALL_DATA exports are multi-sheet, but this keeps old files readable.
        const int cSection = findColumn(cols, {"Section"});
        const int cKey1 = findColumn(cols, {"Key 1"});
        const int cValue1 = findColumn(cols, {"Value 1"});
        const int cValue2 = findColumn(cols, {"Value 2"});
        const int cValue3 = findColumn(cols, {"Value 3"});
        const int cValue4 = findColumn(cols, {"Value 4"});
        const int cValue5 = findColumn(cols, {"Value 5"});
        const int cValue6 = findColumn(cols, {"Value 6"});
        const int cValue7 = findColumn(cols, {"Value 7"});
        const int cValue8 = findColumn(cols, {"Value 8"});
        const int cValue9 = findColumn(cols, {"Value 9"});
        const int cValue10 = findColumn(cols, {"Value 10"});
        if (cSection < 0 || cKey1 < 0) {
            g_lastImportError = tr_auto_import_failed_the_workbook_format_does_not_ac2d8f1c();
            return false;
        }

        const auto months = monthNames();
        QMap<QString, int> monthLookup;
        for (int i = 0; i < months.size(); ++i) monthLookup[normalizedHeaderCell(months[i])] = i;
        auto parseType = [](const QString& text) {
            const QString k = text.trimmed().toCaseFolded();
            if (k.contains(tr_import_account_type_receivable_ar_keyword()) || k.contains(QStringLiteral("receivable"))) return AccountType::Receivable;
            return AccountType::Payable;
        };
        for (int r = dataStartRowIndex; r < rows.size(); ++r) {
            const auto& row = rows[r];
            if (!rowHasAnyData(row)) continue;
            const QString section = cell(row, cSection).toUpper();
            if (section == QStringLiteral("DATA_ENTRY")) {
                const int monthIndex = monthLookup.value(normalizedHeaderCell(cell(row, cKey1)), -1);
                if (monthIndex < 0 || monthIndex >= 12) continue;
                auto& m = data->months[monthIndex];
                if (!parseRequiredNumber(row, cValue1, m.sales, QStringLiteral("Sales"))) return false;
                if (!parseRequiredNumber(row, cValue2, m.salesReturn, QStringLiteral("Sales Return"))) return false;
                if (!parseRequiredNumber(row, cValue3, m.supplierPurchases, QStringLiteral("Supplier Purchases"))) return false;
                if (!parseRequiredNumber(row, cValue4, m.supplierPayments, QStringLiteral("Supplier Payments"))) return false;
                m.expenseAccount = cell(row, cValue5);
                if (!parseRequiredNumber(row, cValue6, m.expenseAmount, QStringLiteral("Expense Amount"))) return false;
                if (!parseRequiredNumber(row, cValue7, m.inventoryFirst, QStringLiteral("Inventory First"))) return false;
                if (!parseRequiredNumber(row, cValue8, m.inventoryLast, QStringLiteral("Inventory Last"))) return false;
                m.supplierName = cell(row, cValue9);
                if (!parseRequiredNumber(row, cValue10, m.cogsInput, QStringLiteral("COGS Input"))) return false;
            } else if (section == QStringLiteral("EXPENSES")) {
                if (cell(row, cKey1).isEmpty() && cell(row, cValue2).isEmpty()) continue;
                AccountItem a;
                a.name = cell(row, cKey1);
                a.type = parseType(cell(row, cValue1));
                if (!parseRequiredNumber(row, cValue2, a.amount, QStringLiteral("Amount"))) return false;
                if (!a.name.trimmed().isEmpty()) data->accounts.append(a);
            } else if (section == QStringLiteral("SUPPLIERS")) {
                const int monthIndex = monthLookup.value(normalizedHeaderCell(cell(row, cKey1)), -1);
                if (monthIndex < 0 || monthIndex >= 12) continue;
                SupplierEntry e;
                e.name = cell(row, cValue1);
                if (!parseRequiredNumber(row, cValue2, e.previousBalance, QStringLiteral("Previous Balance"))) return false;
                if (!parseRequiredNumber(row, cValue3, e.purchases, QStringLiteral("Purchases"))) return false;
                if (!parseRequiredNumber(row, cValue4, e.totalDebt, QStringLiteral("Total Debt"))) return false;
                if (!parseRequiredNumber(row, cValue5, e.payments, QStringLiteral("Payments"))) return false;
                if (!e.name.trimmed().isEmpty()) data->supplierEntries[monthIndex].append(e);
            }
        }
    }

    return true;
}

static bool loadAppDataXlsx(const QString& path, AppData* data)
{
    g_lastImportError.clear();
    g_lastImportMarker.clear();
    if (!data) return false;
    QMap<QString, QByteArray> entries;
    if (!readZipEntries(path, &entries)) return false;

    g_sharedStringsXlsx.clear();
    for (const QString& key : entries.keys()) {
        if (key.contains("sharedStrings")) {
            g_sharedStringsXlsx = parseSharedStrings(entries.value(key));
            break;
        }
    }

    const QStringList sheetPaths = workbookSheetPaths(entries);
    if (sheetPaths.isEmpty()) { g_lastImportError = tr_auto_missing_sheet_marker_or_headers_b00ed7c6(); return false; }

    *data = AppData{};
    bool loadedAny = false;
    QStringList loadedMarkers;
    for (const QString& sheetPath : sheetPaths) {
        const QByteArray sheet = entries.value(sheetPath);
        if (sheet.isEmpty()) continue;
        const auto rows = parseWorksheetRows(sheet);
        if (rows.isEmpty()) continue;

        AppData part;
        QString marker;
        const QString oldError = g_lastImportError;
        g_lastImportError.clear();
        if (parseSingleSheetRows(rows, &part, &marker)) {
            loadedAny = true;
            if (!marker.isEmpty()) loadedMarkers << marker;
            if (marker == QStringLiteral("DATA_ENTRY")) {
                data->months = part.months;
                data->inventoryMode = part.inventoryMode;
            } else if (marker == QStringLiteral("EXPENSES")) {
                data->accounts = part.accounts;
            } else if (marker == QStringLiteral("SUPPLIERS")) {
                data->suppliers = part.suppliers;
                data->supplierEntries = part.supplierEntries;
            } else if (marker == QStringLiteral("ALL_DATA")) {
                *data = part;
            }
        } else if (!g_lastImportError.isEmpty()) {
            return false;
        } else {
            g_lastImportError = oldError;
        }
    }

    if (!loadedAny) {
        g_lastImportError = tr_auto_import_failed_the_workbook_format_does_not_ac2d8f1c();
        return false;
    }

    g_lastImportMarker = loadedMarkers.join(",");
    data->calculate();
    return true;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
static void applyFontRecursive(QWidget* widget, const QFont& font)
{
    if (!widget)
        return;
    widget->setFont(font);
    const auto children = widget->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children)
        applyFontRecursive(child, font);
}

static void applyGlobalAppFont(int pointSize)
{
    QFont f = qApp->font();
    f.setPointSize(pointSize);
    qApp->setFont(f);
    for (QWidget* w : qApp->topLevelWidgets())
        applyFontRecursive(w, f);
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    if (auto* scr = QGuiApplication::primaryScreen()) {
        QRect g = scr->availableGeometry();
        resize(int(g.width()*0.88), int(g.height()*0.88));
        move(int(g.width()*0.06), int(g.height()*0.06));
    }

    loadSettings();   // restore globals before building UI
    buildUI();
    applyTheme();

    // Re-apply language direction after building UI
    qApp->setLayoutDirection(g_lang == AppLanguage::Arabic ? Qt::RightToLeft : Qt::LeftToRight);

    // Re-apply saved font size everywhere
    applyGlobalAppFont(g_fontSize);

    loadTableDataLocally(); // restore entered data
}

// ─────────────────────────────────────────────────────────────────────────────
//  Persistence: Settings
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::loadSettings()
{
    QSettings s(QStringLiteral("AccountAssistant"), QStringLiteral("AccountAssistant"));
    g_lang        = static_cast<AppLanguage>(s.value(QStringLiteral("language"),    0).toInt());
    g_lightMode   = s.value(QStringLiteral("lightMode"),   false).toBool();
    g_currency    = static_cast<AppCurrency>(s.value(QStringLiteral("currency"),    0).toInt());
    g_fontSize    = s.value(QStringLiteral("fontSize"),    12).toInt();
    g_classicView = s.value(QStringLiteral("classicView"), false).toBool();
}

void MainWindow::saveSettings()
{
    QSettings s(QStringLiteral("AccountAssistant"), QStringLiteral("AccountAssistant"));
    s.setValue(QStringLiteral("language"),    static_cast<int>(g_lang));
    s.setValue(QStringLiteral("lightMode"),   g_lightMode);
    s.setValue(QStringLiteral("currency"),    static_cast<int>(g_currency));
    s.setValue(QStringLiteral("fontSize"),    g_fontSize);
    s.setValue(QStringLiteral("classicView"), g_classicView);
    s.sync();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Persistence: Table data
// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::saveTableDataLocally()
{
    const AppData data = collectAllData();
    QSettings s(QStringLiteral("AccountAssistant"), QStringLiteral("AccountAssistant"));
    s.beginGroup(QStringLiteral("tableData"));
    s.setValue(QStringLiteral("hasData"), true);
    s.setValue(QStringLiteral("inventoryMode"), int(data.inventoryMode));
    for (int i = 0; i < 12; ++i) {
        const auto& m = data.months[i];
        s.beginGroup(QString::number(i));
        s.setValue(QStringLiteral("sales"),             m.sales);
        s.setValue(QStringLiteral("salesReturn"),       m.salesReturn);
        s.setValue(QStringLiteral("supplierPurchases"), m.supplierPurchases);
        s.setValue(QStringLiteral("supplierPayments"),  m.supplierPayments);
        s.setValue(QStringLiteral("supplierName"),      m.supplierName);
        s.setValue(QStringLiteral("expenseAccount"),    m.expenseAccount);
        s.setValue(QStringLiteral("expenseAmount"),     m.expenseAmount);
        s.setValue(QStringLiteral("inventoryFirst"),    m.inventoryFirst);
        s.setValue(QStringLiteral("inventoryLast"),     m.inventoryLast);
        s.setValue(QStringLiteral("cogsInput"),         m.cogsInput);
        s.endGroup();
    }
    s.endGroup();

    const QList<AccountItem> accounts = m_accounts ? m_accounts->collectData().accounts : QList<AccountItem>{};
    s.beginGroup(QStringLiteral("accountsData"));
    s.setValue(QStringLiteral("hasData"), !accounts.isEmpty());
    s.setValue(QStringLiteral("count"), accounts.size());
    for (int i = 0; i < accounts.size(); ++i) {
        s.beginGroup(QString::number(i));
        s.setValue(QStringLiteral("name"), accounts[i].name);
        s.setValue(QStringLiteral("type"), int(accounts[i].type));
        s.setValue(QStringLiteral("amount"), accounts[i].amount);
        s.endGroup();
    }
    s.endGroup();

    s.beginGroup(QStringLiteral("suppliersData"));
    s.setValue(QStringLiteral("hasData"), m_suppliers != nullptr);
    for (int i = 0; i < 12; ++i) {
        const auto& sm = data.suppliers[i];
        s.beginGroup(QString::number(i));
        s.setValue(QStringLiteral("supplierName"), sm.supplierName);
        s.setValue(QStringLiteral("purchases"), sm.purchases);
        s.setValue(QStringLiteral("payments"), sm.payments);
        const auto entries = data.supplierEntries[i];
        s.setValue(QStringLiteral("entryCount"), entries.size());
        for (int j = 0; j < entries.size(); ++j) {
            s.beginGroup(QStringLiteral("entry_") + QString::number(j));
            s.setValue(QStringLiteral("name"), entries[j].name);
            s.setValue(QStringLiteral("previousBalance"), entries[j].previousBalance);
            s.setValue(QStringLiteral("purchases"), entries[j].purchases);
            s.setValue(QStringLiteral("totalDebt"), entries[j].totalDebt);
            s.setValue(QStringLiteral("payments"), entries[j].payments);
            s.endGroup();
        }
        s.endGroup();
    }
    s.endGroup();
    s.sync();
}

void MainWindow::loadTableDataLocally()
{
    QSettings s(QStringLiteral("AccountAssistant"), QStringLiteral("AccountAssistant"));
    s.beginGroup(QStringLiteral("tableData"));
    const bool hasData = s.value(QStringLiteral("hasData"), false).toBool();

    AppData data;
    if (hasData) {
        data.inventoryMode = static_cast<InventoryMode>(s.value(QStringLiteral("inventoryMode"), 0).toInt());
        for (int i = 0; i < 12; ++i) {
            auto& m = data.months[i];
            s.beginGroup(QString::number(i));
            m.sales             = s.value(QStringLiteral("sales"),             0.0).toDouble();
            m.salesReturn       = s.value(QStringLiteral("salesReturn"),       0.0).toDouble();
            m.supplierPurchases = s.value(QStringLiteral("supplierPurchases"), 0.0).toDouble();
            m.supplierPayments  = s.value(QStringLiteral("supplierPayments"),  0.0).toDouble();
            m.supplierName      = s.value(QStringLiteral("supplierName"),      QString()).toString();
            m.expenseAccount    = s.value(QStringLiteral("expenseAccount"),    QString()).toString();
            m.expenseAmount     = s.value(QStringLiteral("expenseAmount"),     0.0).toDouble();
            m.inventoryFirst    = s.value(QStringLiteral("inventoryFirst"),    0.0).toDouble();
            m.inventoryLast     = s.value(QStringLiteral("inventoryLast"),     0.0).toDouble();
            m.cogsInput         = s.value(QStringLiteral("cogsInput"),         0.0).toDouble();
            s.endGroup();
        }
        setTableData(data);
    }
    s.endGroup();

    s.beginGroup(QStringLiteral("accountsData"));
    const bool hasAccounts = s.value(QStringLiteral("hasData"), false).toBool();
    QList<AccountItem> accounts;
    if (hasAccounts) {
        const int count = s.value(QStringLiteral("count"), 0).toInt();
        for (int i = 0; i < count; ++i) {
            s.beginGroup(QString::number(i));
            AccountItem a;
            a.name = s.value(QStringLiteral("name"), QString()).toString();
            a.type = static_cast<AccountType>(s.value(QStringLiteral("type"), 0).toInt());
            a.amount = s.value(QStringLiteral("amount"), 0.0).toDouble();
            accounts.append(a);
            s.endGroup();
        }
    }
    s.endGroup();

    AppData supData;
    s.beginGroup(QStringLiteral("suppliersData"));
    if (s.value(QStringLiteral("hasData"), false).toBool()) {
        for (int i = 0; i < 12; ++i) {
            s.beginGroup(QString::number(i));
            supData.suppliers[i].supplierName = s.value(QStringLiteral("supplierName"), QString()).toString();
            supData.suppliers[i].purchases    = s.value(QStringLiteral("purchases"), 0.0).toDouble();
            supData.suppliers[i].payments     = s.value(QStringLiteral("payments"), 0.0).toDouble();
            const int entryCount = s.value(QStringLiteral("entryCount"), 0).toInt();
            for (int j = 0; j < entryCount; ++j) {
                s.beginGroup(QStringLiteral("entry_") + QString::number(j));
                SupplierEntry e;
                e.name = s.value(QStringLiteral("name"), QString()).toString();
                e.previousBalance = s.value(QStringLiteral("previousBalance"), 0.0).toDouble();
                e.purchases = s.value(QStringLiteral("purchases"), 0.0).toDouble();
                e.totalDebt = s.value(QStringLiteral("totalDebt"), 0.0).toDouble();
                e.payments = s.value(QStringLiteral("payments"), 0.0).toDouble();
                supData.supplierEntries[i].append(e);
                s.endGroup();
            }
            s.endGroup();
        }
    }
    s.endGroup();

    if (m_accounts) { AppData accData; accData.accounts = accounts; m_accounts->setData(accData); }
    if (m_suppliers) { m_suppliers->setData(supData); }
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::closeEvent(QCloseEvent* event)
{
    saveSettings();
    saveTableDataLocally();
    QMainWindow::closeEvent(event);
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::buildUI()
{
    auto* central = new QWidget;
    central->setObjectName("centralWidget");
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0,0,0,0);
    root->setSpacing(0);

    // ── Header bar ────────────────────────────────────────────────────────
    {
        auto* hdr = new QWidget;
        hdr->setObjectName("header");
        hdr->setFixedHeight(60);
        auto* hl = new QHBoxLayout(hdr);
        hl->setContentsMargins(24,0,20,0);
        hl->setSpacing(0);

        // App logo/title
        m_titleLabel = new QLabel;
        m_titleLabel->setObjectName("appTitle");
        m_titleLabel->setStyleSheet("font-weight:900; letter-spacing:1px; background:transparent;");
        hl->addWidget(m_titleLabel);
        hl->addStretch();

        // Buttons
        auto makeBtn = [&](QPushButton*& btn,
                           const QString& ss = QString()) -> QPushButton* {
            btn = new QPushButton;
            btn->setFixedHeight(36);
            btn->setCursor(Qt::PointingHandCursor);
            if (!ss.isEmpty()) btn->setStyleSheet(ss);
            hl->addWidget(btn);
            hl->addSpacing(8);
            return btn;
        };

        const char* kBtnBase =
            "QPushButton{"
            "  border:none; border-radius:7px;"
            "  font-weight:700;"
            "  padding:0 18px; min-width:120px;"
            "}";

        QString calcSS = QString(kBtnBase) +
            "QPushButton{ background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
            "stop:0 #4f86f7, stop:1 #2a5cc4); color:white; }"
            "QPushButton:hover{ background:#5e91f8; }"
            "QPushButton:pressed{ background:#3a6fe0; }";

        QString secSS = QString(kBtnBase) +
            "QPushButton{ background:#1a1f38; color:#8892b8;"
            "border:1px solid #252b52; }"
            "QPushButton:hover{ background:#1e2445; color:#c8d0ed; }";

        makeBtn(m_calcBtn,    calcSS);
        makeBtn(m_saveBtn,    secSS);
        makeBtn(m_importBtn,  secSS);
        makeBtn(m_exportBtn,  secSS);
        makeBtn(m_settingsBtn,secSS);
        m_calcBtn->setObjectName("calcBtn");
        m_saveBtn->setObjectName("saveBtn");
        m_importBtn->setObjectName("importBtn");
        m_exportBtn->setObjectName("exportBtn");
        m_settingsBtn->setObjectName("settingsBtn");

        connect(m_calcBtn,     &QPushButton::clicked, this, &MainWindow::onCalculate);
        connect(m_saveBtn,     &QPushButton::clicked, this, &MainWindow::onSaveData);
        connect(m_importBtn,   &QPushButton::clicked, this, &MainWindow::onImportData);
        connect(m_exportBtn,   &QPushButton::clicked, this, &MainWindow::onExportPdf);
        connect(m_settingsBtn, &QPushButton::clicked, this, &MainWindow::onSettings);

        root->addWidget(hdr);
    }

    // ── Tab widget ────────────────────────────────────────────────────────
    m_tabs = new QTabWidget;

    // Tab 0: Data entry
    {
        auto* dataTab = new QWidget;
        dataTab->setObjectName("dataTab");
        auto* vl = new QVBoxLayout(dataTab);
        vl->setContentsMargins(0,0,0,0);
        vl->setSpacing(0);

        // Thin top bar just for the Clear button (title/subtitle live inside the table)
        auto* subHdr = new QWidget;
        subHdr->setObjectName("dataSubHeader");
        subHdr->setFixedHeight(44);
        auto* shl = new QHBoxLayout(subHdr);
        shl->setContentsMargins(24, 0, 20, 0);
        shl->setSpacing(10);
        shl->addStretch();

        m_inventoryModeCombo = new QComboBox;
        m_inventoryModeCombo->addItem(tr_periodic_inventory_8a4f19());
        m_inventoryModeCombo->addItem(tr_ongoing_inventory_4f9f2c());
        m_inventoryModeCombo->setCursor(Qt::PointingHandCursor);
        m_inventoryModeCombo->setMinimumWidth(180);
        m_inventoryModeCombo->setFixedHeight(30);
        m_inventoryModeCombo->setStyleSheet(g_lightMode
            ? "QComboBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:0 10px;font-weight:700;}"
              " QComboBox::drop-down{border:none;width:24px;}"
              " QComboBox QAbstractItemView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;}"
            : "QComboBox{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;border-radius:6px;padding:0 10px;font-weight:700;}"
              " QComboBox::drop-down{border:none;width:24px;}"
              " QComboBox QAbstractItemView{background:#111526;color:#c8d0ed;selection-background-color:#1e2445;}"
        );
        connect(m_inventoryModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onInventoryModeChanged);

        m_clearBtn = new QPushButton(tr_clear_data_4fcd0d());
        m_clearBtn->setCursor(Qt::PointingHandCursor);
        m_clearBtn->setFixedHeight(30);
        m_clearBtn->setStyleSheet(g_lightMode
            ? "QPushButton{border:1px solid #e74c3c; border-radius:6px; font-weight:700;"
              " padding:0 16px; background:#fff5f5; color:#c0392b;}"
              "QPushButton:hover{background:#fde8e8; color:#e74c3c;}"
              "QPushButton:pressed{background:#f5d0d0;}"
            : "QPushButton{border:1px solid #c0392b; border-radius:6px; font-weight:700;"
              " padding:0 16px; background:#1e1010; color:#e74c3c;}"
              "QPushButton:hover{background:#2c1515; color:#ff6b6b;}"
              "QPushButton:pressed{background:#3a1a1a;}");
        connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::onClearData);
        shl->addWidget(m_inventoryModeCombo);
        shl->addWidget(m_clearBtn);

        vl->addWidget(subHdr);

        m_tableStack = new QStackedWidget(dataTab);
        m_table        = new DataTableWidget(m_tableStack);
        m_classicTable = new ClassicDataTableWidget(m_tableStack);
        m_tableStack->addWidget(m_table);          // index 0 – card view
        m_tableStack->addWidget(m_classicTable);   // index 1 – classic view
        m_tableStack->setCurrentIndex(g_classicView ? 1 : 0);
        vl->addWidget(m_tableStack, 1);

        m_tabs->addTab(dataTab, "");
    }

    // Tab 1: Expenses
    {
        m_accounts = new Accountswidget;
        connect(m_accounts, &Accountswidget::graphRequested, this, &MainWindow::onAccountGraphRequested);
        m_tabs->addTab(m_accounts, "");
    }

    // Tab 2: Suppliers
    {
        m_suppliers = new SuppliersWidget;
        connect(m_suppliers, &SuppliersWidget::graphRequested, this, &MainWindow::onSupplierGraphRequested);
        m_tabs->addTab(m_suppliers, "");
    }

    // Tab 3: Results
    {
        m_results = new ResultsWidget;
        connect(m_results, &ResultsWidget::editChartsRequested, this, &MainWindow::onEditCharts);
        connect(m_results, &ResultsWidget::duplicateChartRequested, this, &MainWindow::onDuplicateChart);
        connect(m_results, &ResultsWidget::chartRemoved, this, &MainWindow::onChartRemoved);
        connect(m_results, &ResultsWidget::resultsStateChanged, this, &MainWindow::syncResultsState);
        connect(m_results, &ResultsWidget::clearResultsRequested, this, &MainWindow::onClearResultsTab);
        m_tabs->addTab(m_results, "");
    }

    root->addWidget(m_tabs, 1);
    retranslate();
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::onCalculate()
{
    if (auto* popup = QApplication::activePopupWidget())
        popup->hide();
    AppData working = collectAllData();
    working.calculate();

    QList<ChartRequest> preservedExternalCharts;
    QList<ChartRequest> preservedExternalHiddenCharts;
    QList<ChartRequest> editableCalculationCharts;
    for (const ChartRequest& req : m_lastChartRequests) {
        if (req.origin == ChartOrigin::Accounts || req.origin == ChartOrigin::Suppliers)
            preservedExternalCharts << req;
        else
            editableCalculationCharts << req;
    }
    for (const ChartRequest& req : m_lastHiddenChartRequests) {
        if (req.origin == ChartOrigin::Accounts || req.origin == ChartOrigin::Suppliers)
            preservedExternalHiddenCharts << req;
    }

    working.chartRequests = editableCalculationCharts;
    working.hiddenChartRequests.clear();
    working.resultFlowOrder = m_lastFlowOrder;

    ChartSelectionDialog dlg(working, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    working.sel = dlg.selections();
    working.chartRequests = dlg.chartRequests();
    for (const ChartRequest& req : preservedExternalCharts)
        working.chartRequests << req;
    working.hiddenChartRequests = preservedExternalHiddenCharts;
    m_data = working;
    m_hasResults = true;

    if (m_results) {
        m_results->buildResults(m_data);
        syncResultsState();
    }
    m_tabs->setCurrentIndex(3);
}


void MainWindow::onEditCharts(int cardIndex)
{
    if (!m_hasResults)
        return;

    if (auto* popup = QApplication::activePopupWidget())
        popup->hide();

    AppData working = collectAllData();
    working.calculate();
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;

    if (cardIndex >= 0 && m_results) {
        const ChartRequest focused = m_results->requestForCard(cardIndex);
        if (focused.origin == ChartOrigin::Accounts || focused.origin == ChartOrigin::Suppliers) {
            m_pendingGraphReplaceCardIndex = cardIndex;
            m_pendingGraphReplaceOrigin = focused.origin;
            m_tabs->setCurrentIndex(focused.origin == ChartOrigin::Accounts ? 1 : 2);
            if (focused.origin == ChartOrigin::Suppliers && m_suppliers) {
                const bool accepted = m_suppliers->showGraphSelectionForRequest(focused);
                if (!accepted) {
                    m_pendingGraphReplaceCardIndex = -1;
                    m_pendingGraphReplaceOrigin = ChartOrigin::Custom;
                }
            }
            return;
        }
        if (focused.metricA != M_COUNT || !focused.title.isEmpty()) {
            auto sameReq = [&](const ChartRequest& req) {
                return req.kind == focused.kind
                    && req.metricA == focused.metricA
                    && req.metricB == focused.metricB
                    && req.compareMetrics == focused.compareMetrics
                    && req.months == focused.months
                    && req.title == focused.title;
            };
            for (int i = 0; i < working.chartRequests.size(); ++i) {
                if (sameReq(working.chartRequests[i])) {
                    working.chartRequests.move(i, 0);
                    break;
                }
            }
        }
    }

    ChartSelectionDialog dlg(working, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    working.sel = dlg.selections();
    working.chartRequests = dlg.chartRequests();
    m_data = working;

    if (m_results) {
        m_results->buildResults(m_data);
        syncResultsState();
    }
    m_tabs->setCurrentIndex(3);
}


void MainWindow::onDuplicateChart(int cardIndex)
{
    if (!m_hasResults || !m_results)
        return;

    AppData working = collectAllData();
    working.calculate();
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;

    const ChartRequest req = m_results->requestForCard(cardIndex);
    if (req.metricA == M_COUNT && req.title.isEmpty())
        return;

    m_data = working;
    m_results->appendChart(m_data, req);
    syncResultsState();
    m_tabs->setCurrentIndex(3);
}


void MainWindow::onAccountGraphRequested(ChartKind kind, AccountTypeFilter accountFilter)
{
    AppData working = collectAllData();
    working.calculate();
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;
    m_data = working;
    m_hasResults = true;

    if (!m_results)
        return;

    if (m_results->flowOrder().isEmpty()) {
        m_results->buildResults(m_data);
    }

    ChartRequest req;
    req.kind = kind;
    req.metricA = M_EXPENSES;
    req.origin = ChartOrigin::Accounts;
    req.accountFilter = accountFilter;
    req.title = metricDisplayName(M_EXPENSES);
    switch (kind) {
    case ChartKind::Pie:
        req.title += QStringLiteral(" — ") + tr_pie_97ce50();
        break;
    case ChartKind::Candle:
        req.title += QStringLiteral(" — ") + tr_candle_77e8b9();
        break;
    case ChartKind::RankedBar:
        req.title += QStringLiteral(" — ") + tr_bar_6dda02();
        break;
    case ChartKind::MetricLine:
    default:
        req.kind = ChartKind::MetricLine;
        req.title += QStringLiteral(" — ") + tr_line_a566e8();
        break;
    }

    if (m_pendingGraphReplaceOrigin == ChartOrigin::Accounts && m_pendingGraphReplaceCardIndex >= 0) {
        m_results->removeCardByIndex(m_pendingGraphReplaceCardIndex);
        m_pendingGraphReplaceCardIndex = -1;
        m_pendingGraphReplaceOrigin = ChartOrigin::Custom;
    }

    m_results->appendChart(m_data, req);
    syncResultsState();
    m_tabs->setCurrentIndex(3);
}

void MainWindow::onSupplierGraphRequested(const ChartRequest& request)
{
    AppData working = collectAllData();
    working.calculate();
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;
    m_data = working;
    m_hasResults = true;

    if (!m_results)
        return;

    if (m_results->flowOrder().isEmpty())
        m_results->buildResults(m_data);

    ChartRequest req = request;
    req.origin = ChartOrigin::Suppliers;
    if (req.compareMetrics.isEmpty())
        req.compareMetrics << M_SUPPLIER_BALANCE;
    req.compareMetrics.removeAll(M_SUPPLIER_NAME);
    req.metricA = req.compareMetrics.value(0, M_SUPPLIER_BALANCE);
    req.metricB = req.compareMetrics.value(1, req.metricA);

    QStringList metricNames;
    for (MetricId id : req.compareMetrics)
        metricNames << metricDisplayName(id);
    const QString metricsTitle = metricNames.isEmpty() ? metricDisplayName(M_SUPPLIER_BALANCE) : metricNames.join(QStringLiteral(" vs "));
    req.title = metricsTitle + QStringLiteral(" — ") + tr_suppliers_7beff3();
    switch (req.kind) {
    case ChartKind::Pie:
    case ChartKind::ComparePie:
        req.kind = ChartKind::ComparePie;
        req.title += QStringLiteral(" — ") + tr_pie_97ce50();
        break;
    case ChartKind::MetricLine:
    case ChartKind::CompareLine:
        req.kind = ChartKind::CompareLine;
        req.title += QStringLiteral(" — ") + tr_line_a566e8();
        break;
    case ChartKind::Candle:
        req.kind = ChartKind::Candle;
        req.title += QStringLiteral(" — ") + tr_candle_77e8b9();
        break;
    case ChartKind::RankedBar:
    case ChartKind::MetricBar:
    case ChartKind::CompareBar:
    default:
        req.kind = ChartKind::CompareBar;
        req.title += QStringLiteral(" — ") + tr_bar_6dda02();
        break;
    }

    if (m_pendingGraphReplaceOrigin == ChartOrigin::Suppliers && m_pendingGraphReplaceCardIndex >= 0) {
        m_results->removeCardByIndex(m_pendingGraphReplaceCardIndex);
        m_pendingGraphReplaceCardIndex = -1;
        m_pendingGraphReplaceOrigin = ChartOrigin::Custom;
    }

    m_results->appendChart(m_data, req);
    syncResultsState();
    m_tabs->setCurrentIndex(3);
}


static bool sameChartRequestForRemoval(const ChartRequest& a, const ChartRequest& b)
{
    return a.origin == b.origin
        && a.kind == b.kind
        && a.metricA == b.metricA
        && a.metricB == b.metricB
        && a.compareMetrics == b.compareMetrics
        && a.months == b.months
        && a.accountFilter == b.accountFilter
        && a.comparePieBaseMetric == b.comparePieBaseMetric
        && a.includeSummaryPoint == b.includeSummaryPoint
        && a.title == b.title;
}

void MainWindow::onChartRemoved(const ChartRequest& removed)
{
    if (removed.origin != ChartOrigin::Custom)
        return;

    auto removeOne = [&](QList<ChartRequest>& list) {
        for (int i = 0; i < list.size(); ++i) {
            if (sameChartRequestForRemoval(list[i], removed)) {
                list.removeAt(i);
                return true;
            }
        }
        return false;
    };

    bool changed = removeOne(m_lastChartRequests);
    changed = removeOne(m_lastHiddenChartRequests) || changed;
    changed = removeOne(m_data.chartRequests) || changed;
    changed = removeOne(m_data.hiddenChartRequests) || changed;

    if (changed && m_results) {
        m_lastChartRequests = m_results->chartRequests();
        m_lastHiddenChartRequests = m_results->hiddenChartRequests();
        m_lastFlowOrder = m_results->flowOrder();
        m_data.chartRequests = m_lastChartRequests;
        m_data.hiddenChartRequests = m_lastHiddenChartRequests;
        m_data.resultFlowOrder = m_lastFlowOrder;
    }
}

void MainWindow::onClearResultsTab()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr_auto_clear_results_118d7c());
    box.setText(tr_auto_clear_results_warning_778aa1());
    QPushButton* clearBtn = box.addButton(tr_auto_clear_results_118d7c(), QMessageBox::AcceptRole);
    QPushButton* cancelBtn = box.addButton(tr_cancel_8d40ef(), QMessageBox::RejectRole);
    box.setDefaultButton(cancelBtn);
    box.exec();
    if (box.clickedButton() != clearBtn)
        return;

    m_hasResults = false;
    m_data.chartRequests.clear();
    m_data.hiddenChartRequests.clear();
    m_data.resultFlowOrder.clear();
    m_lastChartRequests.clear();
    m_lastHiddenChartRequests.clear();
    m_lastFlowOrder.clear();
    m_pendingGraphReplaceCardIndex = -1;
    m_pendingGraphReplaceOrigin = ChartOrigin::Custom;
    if (m_results)
        m_results->resetAllResults();
}

void MainWindow::syncResultsState()
{
    if (!m_results)
        return;
    m_data.chartRequests = m_results->chartRequests();
    m_data.hiddenChartRequests = m_results->hiddenChartRequests();
    m_data.resultFlowOrder = m_results->flowOrder();
    m_lastChartRequests = m_data.chartRequests;
    m_lastHiddenChartRequests = m_data.hiddenChartRequests;
    m_lastFlowOrder = m_data.resultFlowOrder;
}

void MainWindow::onInventoryModeChanged(int index)
{
    const InventoryMode mode = index == 1 ? InventoryMode::Ongoing : InventoryMode::Periodic;
    const AppData current = collectAllData();
    if (current.inventoryMode == mode)
        return;

    if (appDataHasUserEntries(current)) {
        QDialog dialog(this);
        dialog.setWindowTitle(tr_auto_switch_inventory_mode_1fb086f6());
        dialog.setModal(true);
        dialog.setSizeGripEnabled(false);
        dialog.setWindowFlag(Qt::MSWindowsFixedSizeDialogHint, true);
        dialog.setStyleSheet(g_lightMode
            ? QStringLiteral(R"(
                QDialog { background:#f4f6fb; }
                QLabel { color:#1e2340; background:transparent; }
                QPushButton {
                    background:#4f86f7;
                    color:white;
                    font-weight:700;
                    border:none;
                    border-radius:7px;
                    padding:8px 18px;
                }
                QPushButton:hover { background:#6a9df9; }
                QPushButton:pressed { background:#3a6fe0; }
            )")
            : QStringLiteral(R"(
                QDialog { background:#12152a; }
                QLabel { color:#e6ebff; background:transparent; }
                QPushButton {
                    background:#4f86f7;
                    color:white;
                    font-weight:700;
                    border:none;
                    border-radius:7px;
                    padding:8px 18px;
                }
                QPushButton:hover { background:#6a9df9; }
                QPushButton:pressed { background:#3a6fe0; }
            )"));
        dialog.setMinimumWidth(g_lang == AppLanguage::Arabic ? 760 : 560);

        auto* root = new QVBoxLayout(&dialog);
        root->setContentsMargins(18, 18, 18, 18);
        root->setSpacing(14);

        auto* topRow = new QHBoxLayout;
        topRow->setSpacing(12);

        auto* iconLabel = new QLabel(&dialog);
        const QPixmap iconPm = style()->standardIcon(QStyle::SP_MessageBoxWarning).pixmap(40, 40);
        iconLabel->setPixmap(iconPm);
        iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        auto* textCol = new QVBoxLayout;
        textCol->setSpacing(8);

        auto* mainText = new QLabel(tr_auto_switching_the_inventory_mode_will_clear_th_9988b06e(), &dialog);
        mainText->setWordWrap(true);
        mainText->setTextFormat(Qt::PlainText);
        mainText->setAlignment(g_lang == AppLanguage::Arabic ? Qt::AlignRight : Qt::AlignLeft);
        QFont mainFont = mainText->font();
        mainFont.setPointSize(mainFont.pointSize() + 2);
        mainText->setFont(mainFont);

        auto* infoText = new QLabel(tr_auto_choose_how_to_continue_1e9d6832(), &dialog);
        infoText->setWordWrap(true);
        infoText->setTextFormat(Qt::PlainText);
        infoText->setAlignment(g_lang == AppLanguage::Arabic ? Qt::AlignRight : Qt::AlignLeft);

        textCol->addWidget(mainText);
        textCol->addWidget(infoText);
        topRow->addLayout(textCol, 1);
        topRow->addWidget(iconLabel, 0, Qt::AlignTop);
        root->addLayout(topRow);

        auto* buttons = new QHBoxLayout;
        buttons->setSpacing(12);
        if (g_lang == AppLanguage::Arabic)
            buttons->setDirection(QBoxLayout::RightToLeft);

        auto* clearBtn = new QPushButton(tr_auto_clear_them_c4858318(), &dialog);
        auto* saveClearBtn = new QPushButton(tr_auto_save_data_and_clear_91f861f8(), &dialog);
        auto* cancelBtn = new QPushButton(tr_auto_cancel_8d40ef3e(), &dialog);

        const QFontMetrics btnFm(dialog.font());
        const auto buttonWidthForText = [&](const QString& s, int minWidth) {
            return std::max(minWidth, btnFm.horizontalAdvance(s) + 96);
        };
        clearBtn->setMinimumWidth(buttonWidthForText(clearBtn->text(), 150));
        saveClearBtn->setMinimumWidth(buttonWidthForText(saveClearBtn->text(), g_lang == AppLanguage::Arabic ? 280 : 210));
        cancelBtn->setMinimumWidth(buttonWidthForText(cancelBtn->text(), 120));
        clearBtn->setMinimumHeight(38);
        saveClearBtn->setMinimumHeight(38);
        cancelBtn->setMinimumHeight(38);
        clearBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        saveClearBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        cancelBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

        buttons->addWidget(cancelBtn);
        buttons->addWidget(clearBtn);
        buttons->addWidget(saveClearBtn);
        root->addLayout(buttons);

        dialog.adjustSize();
        dialog.setFixedSize(dialog.sizeHint());

        enum class SwitchChoice { Cancel, Clear, SaveAndClear };
        SwitchChoice choice = SwitchChoice::Cancel;
        QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, [&]() { choice = SwitchChoice::Cancel; dialog.reject(); });
        QObject::connect(clearBtn, &QPushButton::clicked, &dialog, [&]() { choice = SwitchChoice::Clear; dialog.accept(); });
        QObject::connect(saveClearBtn, &QPushButton::clicked, &dialog, [&]() { choice = SwitchChoice::SaveAndClear; dialog.accept(); });
        dialog.exec();

        if (choice == SwitchChoice::Cancel) {
            if (m_inventoryModeCombo) {
                QSignalBlocker blocker(m_inventoryModeCombo);
                m_inventoryModeCombo->setCurrentIndex(int(current.inventoryMode));
            }
            return;
        }

        if (choice == SwitchChoice::SaveAndClear) {
            const QString path = QFileDialog::getSaveFileName(
                this,
                tr_save_data_ee42b8(),
                tr_default_account_data_filename_0aa2f1().arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm")),
                tr_excel_workbook_xlsx_all_files_aa27cc());
            if (path.isEmpty()) {
                if (m_inventoryModeCombo) {
                    QSignalBlocker blocker(m_inventoryModeCombo);
                    m_inventoryModeCombo->setCurrentIndex(int(current.inventoryMode));
                }
                return;
            }
            if (!saveAppDataXlsx(path, current)) {
                ThemeBox::critical(this, tr_save_data_ee42b8(), tr_unable_to_write_the_xlsx_file_da2b9b());
                if (m_inventoryModeCombo) {
                    QSignalBlocker blocker(m_inventoryModeCombo);
                    m_inventoryModeCombo->setCurrentIndex(int(current.inventoryMode));
                }
                return;
            }
        }

        clearTableData();
        m_hasResults = false;
        if (m_results)
            m_results->clearResults();
    }

    AppData d;
    d.inventoryMode = mode;
    d.calculate();
    setTableData(d);
    m_data = d;
}

void MainWindow::onSaveData()
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Question);
    box.setWindowTitle(tr_save_data_ee42b8());
    box.setText(tr_auto_choose_what_to_export_a3c74f3f());
    auto* currentBtn = box.addButton(tr_auto_current_tab_341e1745(), QMessageBox::AcceptRole);
    auto* allBtn = box.addButton(tr_auto_all_data_2e629e38(), QMessageBox::ActionRole);
    currentBtn->setMinimumWidth(isArabic() ? 220 : 150);
    allBtn->setMinimumWidth(isArabic() ? 180 : 120);
    box.setMinimumWidth(isArabic() ? 520 : 420);
    box.addButton(tr_auto_cancel_8d40ef3e(), QMessageBox::RejectRole);
    ThemeBox::applyDialogTheme(&box);
    box.exec();
    if (box.clickedButton() != currentBtn && box.clickedButton() != allBtn) return;

    const QString path = QFileDialog::getSaveFileName(
        this,
        tr_save_data_ee42b8(),
        tr_default_account_data_filename_0aa2f1().arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm")),
        tr_excel_workbook_xlsx_all_files_aa27cc());
    if (path.isEmpty()) return;

    const AppData data = collectAllData();
    XlsxSheetKind kind = XlsxSheetKind::DataEntry;
    if (box.clickedButton() == allBtn) kind = XlsxSheetKind::AllData;
    else if (m_tabs && m_tabs->currentIndex() == 1) kind = XlsxSheetKind::Expenses;
    else if (m_tabs && m_tabs->currentIndex() == 2) kind = XlsxSheetKind::Suppliers;
    if (!saveAppDataXlsx(path, data, kind)) {
        ThemeBox::critical(this, tr_save_data_ee42b8(), tr_unable_to_write_the_xlsx_file_da2b9b());
        return;
    }

    ThemeBox::info(this, tr_save_data_ee42b8(), tr_data_saved_successfully_f941c7());
}

void MainWindow::onImportData()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr_import_data_8de4db(),
        QString(),
        tr_excel_workbook_xlsx_all_files_aa27cc());
    if (path.isEmpty()) return;

    AppData imported;
    if (!loadAppDataXlsx(path, &imported)) {
        ThemeBox::critical(this,
            tr_import_data_8de4db(),
            !g_lastImportError.isEmpty() ? g_lastImportError : tr_unable_to_read_the_xlsx_file_cd99e7());
        return;
    }

    if (!appDataHasUserEntries(imported) && g_lastImportMarker.trimmed().isEmpty() && m_tabs) {
        ThemeBox::critical(this, tr_import_data_8de4db(), tr_auto_the_workbook_did_not_contain_any_importabl_5d9e29f7());
        return;
    }

    AppData merged = collectAllData();
    QStringList markers;
    for (QString m : g_lastImportMarker.split(',', Qt::SkipEmptyParts)) {
        m = m.trimmed().toUpper();
        if (!m.isEmpty() && !markers.contains(m))
            markers << m;
    }
    if (markers.isEmpty())
        markers << g_lastImportMarker.trimmed().toUpper();

    auto accountKey = [](const QString& name) -> QString {
        return normalizedHeaderCell(name);
    };

    auto sameAccountData = [](const AccountItem& a, const AccountItem& b) -> bool {
        return a.type == b.type && qAbs(a.amount - b.amount) < 0.005;
    };

    auto mergeAccounts = [&]() -> bool {
        QList<AccountItem> importedAccounts;
        QMap<QString, int> importedIndex;
        for (const auto& a : imported.accounts) {
            const QString key = accountKey(a.name);
            if (key.isEmpty())
                continue;
            if (importedIndex.contains(key))
                importedAccounts[importedIndex.value(key)] = a;   // last duplicate in the workbook wins
            else {
                importedIndex[key] = importedAccounts.size();
                importedAccounts.append(a);
            }
        }

        QMap<QString, int> existingIndex;
        for (int i = 0; i < merged.accounts.size(); ++i) {
            const QString key = accountKey(merged.accounts[i].name);
            if (!key.isEmpty() && !existingIndex.contains(key))
                existingIndex[key] = i;
        }

        struct AccountReplacement { int index; AccountItem item; };
        QList<AccountReplacement> replacements;
        QList<AccountItem> additions;
        QStringList conflicts;

        for (const auto& a : importedAccounts) {
            const QString key = accountKey(a.name);
            if (existingIndex.contains(key)) {
                const int idx = existingIndex.value(key);
                if (!sameAccountData(merged.accounts[idx], a)) {
                    replacements.append(AccountReplacement{idx, a});
                    conflicts << a.name.trimmed();
                }
            } else {
                additions.append(a);
            }
        }

        if (!conflicts.isEmpty()) {
            conflicts.removeDuplicates();
            QMessageBox box(QMessageBox::Warning,
                            tr_import_data_8de4db(),
                            tr_auto_one_or_more_expense_accounts_already_exist_ecc8b7fa(),
                            QMessageBox::NoButton,
                            this);
            box.setInformativeText(conflicts.join(QStringLiteral("\n")));
            QPushButton* knowBtn = box.addButton(tr_auto_i_know_96d277a3(), QMessageBox::AcceptRole);
            QPushButton* cancelBtn = box.addButton(tr_auto_cancel_8d40ef3e(), QMessageBox::RejectRole);
            box.setDefaultButton(cancelBtn);
            ThemeBox::applyDialogTheme(&box);
            box.exec();
            if (box.clickedButton() != knowBtn)
                return false;

            for (const auto& r : replacements)
                if (r.index >= 0 && r.index < merged.accounts.size())
                    merged.accounts[r.index] = r.item;
        }

        for (const auto& a : additions)
            merged.accounts.append(a);
        return true;
    };

    auto supplierEntryIsBlank = [](const SupplierEntry& e) -> bool {
        return e.name.trimmed().isEmpty() && e.previousBalance == 0.0 && e.purchases == 0.0 &&
               e.totalDebt == 0.0 && e.payments == 0.0;
    };

    auto refreshSupplierSummary = [](AppData& d) {
        for (int i = 0; i < 12; ++i) {
            double purchases = 0.0;
            double payments = 0.0;
            QString firstName;
            for (const auto& e : d.supplierEntries[i]) {
                if (e.name.trimmed().isEmpty())
                    continue;
                purchases += e.purchases;
                payments += e.payments;
                if (firstName.isEmpty())
                    firstName = e.name.trimmed();
            }
            d.suppliers[i].supplierName = firstName;
            d.suppliers[i].purchases = purchases;
            d.suppliers[i].payments = payments;
        }
    };

    auto mergeSuppliers = [&]() {
        for (int month = 0; month < 12; ++month) {
            QList<SupplierEntry>& current = merged.supplierEntries[month];
            for (int i = current.size() - 1; i >= 0; --i) {
                if (supplierEntryIsBlank(current[i]))
                    current.removeAt(i);
            }

            QMap<QString, int> currentIndex;
            for (int i = 0; i < current.size(); ++i) {
                const QString key = normalizedHeaderCell(current[i].name);
                if (!key.isEmpty() && !currentIndex.contains(key))
                    currentIndex[key] = i;
            }

            for (const auto& importedEntry : imported.supplierEntries[month]) {
                const QString key = normalizedHeaderCell(importedEntry.name);
                if (key.isEmpty())
                    continue;
                if (currentIndex.contains(key)) {
                    current[currentIndex.value(key)] = importedEntry;   // existing supplier row: replace
                } else {
                    currentIndex[key] = current.size();
                    current.append(importedEntry);                       // new supplier row: merge/add
                }
            }
        }
        refreshSupplierSummary(merged);
    };

    if (markers.contains(QStringLiteral("ALL_DATA")) && markers.size() == 1) {
        // Legacy single-sheet ALL_DATA files are full-state imports.
        merged = imported;
    } else {
        if (markers.contains(QStringLiteral("DATA_ENTRY"))) {
            merged.months = imported.months;          // DATA_ENTRY is a replace import
            merged.inventoryMode = imported.inventoryMode;
        }
        if (markers.contains(QStringLiteral("EXPENSES"))) {
            if (!mergeAccounts())
                return;
        }
        if (markers.contains(QStringLiteral("SUPPLIERS"))) {
            mergeSuppliers();                         // SUPPLIERS is add-new + replace-existing
        }
    }

    setAccountData(merged.accounts);
    setTableData(merged);
    m_data = collectAllData();
    m_hasResults = false;
    if (m_results) m_results->clearResults();
    ThemeBox::info(this,
        tr_import_data_8de4db(),
        tr_data_imported_successfully_c05a52());
}

void MainWindow::onExportPdf()
{
    if (!m_hasResults) {
        ThemeBox::info(this,
            tr_export_pdf_2cc36e(),
            tr_please_calculate_first_then_ex_3d96fc());
        return;
    }
    QString path = QFileDialog::getSaveFileName(this,
        tr_export_to_pdf_bc0791(),
        tr_default_account_report_filename_6c5a9b().arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm")),
        tr_pdf_file_filter_8e3d1c());
    if (path.isEmpty()) return;

    bool ok = PdfExporter::exportToPdf(path, m_data, m_results, m_results->pageLandscape());
    if (ok) {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(tr_export_complete_fc6d0a());
        box.setText(tr_the_pdf_report_was_exported_su_37f5d4());
        box.setInformativeText(QFileInfo(path).fileName());
        box.setTextFormat(Qt::PlainText);
        box.setStyleSheet(ThemeBox::style());
        box.exec();
    } else {
        ThemeBox::critical(this,
            tr_error_5a0bc4(),
            tr_failed_to_export_pdf_e10045());
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Table view helpers — route calls to whichever widget is active
// ─────────────────────────────────────────────────────────────────────────────
AppData MainWindow::collectTableData() const
{
    if (g_classicView && m_classicTable)
        return m_classicTable->collectData();
    return m_table ? m_table->collectData() : AppData{};
}
void MainWindow::setTableData(const AppData& d)
{
    if (m_table) {
        m_table->setData(d);
        m_table->setInventoryMode(d.inventoryMode);
    }
    if (m_classicTable) {
        m_classicTable->setInventoryMode(d.inventoryMode);
        m_classicTable->setData(d);
    }
    if (m_suppliers)    m_suppliers->setData(d);
    if (m_inventoryModeCombo) {
        QSignalBlocker blocker(m_inventoryModeCombo);
        m_inventoryModeCombo->setCurrentIndex(int(d.inventoryMode));
    }
}

void MainWindow::setAccountData(const QList<AccountItem>& accounts)
{
    AppData d;
    d.accounts = accounts;
    if (m_accounts) m_accounts->setData(d);
}
AppData MainWindow::collectAllData() const
{
    AppData d = collectTableData();
    if (m_suppliers) {
        AppData sup = m_suppliers->collectData();
        d.suppliers = sup.suppliers;
        d.supplierEntries = sup.supplierEntries;
    }
    if (m_accounts) {
        AppData acc = m_accounts->collectData();
        d.accounts = acc.accounts;
    }
    return d;
}

void MainWindow::clearTableData()
{
    if (m_table)        m_table->clearData();
    if (m_classicTable) m_classicTable->clearData();
    if (m_suppliers)    m_suppliers->clearData();
    if (m_accounts)     m_accounts->clearData();
}
void MainWindow::updateTableCurrency()
{
    if (m_table)        m_table->updateCurrency();
    if (m_classicTable) m_classicTable->updateCurrency();
    if (m_suppliers)    m_suppliers->updateCurrencyPrefix();
    if (m_accounts)     m_accounts->retranslate();
}
void MainWindow::applyTableTheme()
{
    if (m_table)        m_table->applyTheme();
    if (m_classicTable) m_classicTable->applyTheme();
    if (m_suppliers)    m_suppliers->applyTheme();
    if (m_accounts)     m_accounts->applyTheme();
}
void MainWindow::retranslateTable()
{
    if (m_table)        m_table->retranslate();
    if (m_classicTable) m_classicTable->retranslate();
    if (m_suppliers)    m_suppliers->retranslate();
    if (m_accounts)     m_accounts->retranslate();
}
void MainWindow::switchTableView(bool classic)
{
    if (m_tableStack)
        m_tableStack->setCurrentIndex(classic ? 1 : 0);
}

void MainWindow::onClearData()
{
    if (ThemeBox::confirm(this,
            tr_clear_all_data_491f5d(),
            tr_this_will_erase_all_entered_da_382bea()) == QMessageBox::Yes)
    {
        clearTableData();
        m_hasResults = false;
        if (m_results) m_results->clearResults();
    }
}

void MainWindow::onSettings()
{
    SettingsDialog dlg(g_lang, g_lightMode, g_currency, g_fontSize, g_classicView, this);
    if (dlg.exec() == QDialog::Accepted) {
        bool currencyChanged    = (dlg.selectedCurrency() != g_currency);
        bool fontChanged        = (dlg.selectedFontSize() != g_fontSize);
        bool classicViewChanged = (dlg.isClassicView()    != g_classicView);

        g_lightMode = dlg.isLightMode();
        g_currency  = dlg.selectedCurrency();
        g_fontSize  = dlg.selectedFontSize();

        if (classicViewChanged) {
            AppData current = collectAllData();
            g_classicView = dlg.isClassicView();
            switchTableView(g_classicView);
            setTableData(current);
        }

        applyLanguage(dlg.selectedLanguage());
        applyTheme();

        if (currencyChanged) {
            updateTableCurrency();
            if (m_accounts) m_accounts->retranslate();
        }

        if (fontChanged) {
            applyGlobalAppFont(g_fontSize);
        }

        saveSettings(); // persist immediately
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyTheme()
{
    qApp->setStyleSheet(g_lightMode ? kGlobalSSLight : kGlobalSS);
    applyGlobalAppFont(g_fontSize);

    if (m_titleLabel) {
        m_titleLabel->setStyleSheet(g_lightMode
            ? "font-weight:900; letter-spacing:1px; color:#1e2340; background:transparent;"
            : "font-weight:900; letter-spacing:1px; color:#4f86f7; background:transparent;");
    }

    const QString primaryBtn = g_lightMode
        ? "QPushButton{border:none; border-radius:7px; font-weight:700; padding:0 18px; min-width:120px; background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4f86f7, stop:1 #2a5cc4); color:white;}"
          "QPushButton:hover{background:#5e91f8;} QPushButton:pressed{background:#3a6fe0;}"
        : "QPushButton{border:none; border-radius:7px; font-weight:700; padding:0 18px; min-width:120px; background:qlineargradient(x1:0,y1:0,x2:1,y2:0, stop:0 #4f86f7, stop:1 #2a5cc4); color:white;}"
          "QPushButton:hover{background:#5e91f8;} QPushButton:pressed{background:#3a6fe0;}";
    const QString secondaryBtn = g_lightMode
        ? "QPushButton{border:1px solid #d9e0ef; border-radius:7px; font-weight:700; padding:0 18px; min-width:120px; background:#ffffff; color:#1e2340;}"
          "QPushButton:hover{background:#f2f4fb;} QPushButton:pressed{background:#e9edf7;}"
        : "QPushButton{border:1px solid #252b52; border-radius:7px; font-weight:700; padding:0 18px; min-width:120px; background:#1a1f38; color:#8892b8;}"
          "QPushButton:hover{background:#1e2445; color:#c8d0ed;} QPushButton:pressed{background:#171b31;}";

    if (m_calcBtn)     m_calcBtn->setStyleSheet(primaryBtn);
    if (m_saveBtn)     m_saveBtn->setStyleSheet(secondaryBtn);
    if (m_importBtn)   m_importBtn->setStyleSheet(secondaryBtn);
    if (m_exportBtn)   m_exportBtn->setStyleSheet(secondaryBtn);
    if (m_settingsBtn) m_settingsBtn->setStyleSheet(secondaryBtn);
    if (m_clearBtn) {
        m_clearBtn->setStyleSheet(g_lightMode
            ? "QPushButton{border:1px solid #e74c3c; border-radius:5px; font-weight:700;"
              " padding:0 14px; background:#fff5f5; color:#c0392b;}"
              "QPushButton:hover{background:#fde8e8; color:#e74c3c;}"
              "QPushButton:pressed{background:#f5d0d0;}"
            : "QPushButton{border:1px solid #c0392b; border-radius:5px; font-weight:700;"
              " padding:0 14px; background:#1e1010; color:#e74c3c;}"
              "QPushButton:hover{background:#2c1515; color:#ff6b6b;}"
              "QPushButton:pressed{background:#3a1a1a;}");
    }
    if (m_inventoryModeCombo) {
        m_inventoryModeCombo->setStyleSheet(g_lightMode
            ? "QComboBox{background:#ffffff;color:#1e2340;border:1px solid #cfd7ea;border-radius:6px;padding:0 10px;font-weight:700;}"
              " QComboBox::drop-down{border:none;width:24px;}"
              " QComboBox QAbstractItemView{background:#ffffff;color:#1e2340;selection-background-color:#eef0fa;}"
            : "QComboBox{background:#1a1f38;color:#c8d0ed;border:1px solid #252b52;border-radius:6px;padding:0 10px;font-weight:700;}"
              " QComboBox::drop-down{border:none;width:24px;}"
              " QComboBox QAbstractItemView{background:#111526;color:#c8d0ed;selection-background-color:#1e2445;}");
    }

    applyTableTheme();
    if (m_accounts) m_accounts->applyTheme();
    if (m_results) m_results->applyTheme();
    if (m_hasResults)
        m_results->buildResults(m_data);
}

// ─────────────────────────────────────────────────────────────────────────────
void MainWindow::applyLanguage(AppLanguage lang)
{
    g_lang = lang;
    qApp->setLayoutDirection(lang == AppLanguage::Arabic
                             ? Qt::RightToLeft : Qt::LeftToRight);
    retranslate();
    retranslateTable();
}

void MainWindow::retranslate()
{
    setWindowTitle(tr_account_assistant_edcbbf());
    m_titleLabel->setText(
        "◆  " + tr_account_assistant_edcbbf());

    m_calcBtn->setText(
        tr_calculate_36f437());
    m_saveBtn->setText(
        tr_save_data_e6059e());
    m_clearBtn->setText(
        tr_clear_data_4fcd0d());
    m_importBtn->setText(
        tr_import_data_fbe7a5());
    m_exportBtn->setText(
        tr_export_pdf_b87c01());
    m_settingsBtn->setText(
        tr_settings_b7a402());

    if (m_inventoryModeCombo) {
        QSignalBlocker blocker(m_inventoryModeCombo);
        m_inventoryModeCombo->setItemText(0, tr_periodic_inventory_8a4f19());
        m_inventoryModeCombo->setItemText(1, tr_ongoing_inventory_4f9f2c());
    }
    m_tabs->setTabText(0,
        tr_data_entry_a353ce());
    m_tabs->setTabText(1,
        tr_expenses_13597e());
    m_tabs->setTabText(2,
        tr_suppliers_7beff3());
    m_tabs->setTabText(3,
        tr_results_87ae7f());
    if (m_results) m_results->retranslate();
}
