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

static QByteArray makeWorksheetXml(const AppData& data)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter w(&buffer);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("worksheet");
    w.writeDefaultNamespace("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    w.writeNamespace("http://schemas.openxmlformats.org/officeDocument/2006/relationships", "r");
    w.writeStartElement("sheetData");

    const QStringList headers = {
        "Month", "Sales", "Sales Return", "Supplier Purchases", "Supplier Payments",
        "Expense Account", "Expense Amount", "Inventory First", "Inventory Last",
        "Supplier Name", "COGS Input"
    };

    auto writeTextCell = [&](const QString& ref, const QString& text) {
        w.writeStartElement("c");
        w.writeAttribute("r", ref);
        w.writeAttribute("t", "inlineStr");
        w.writeStartElement("is");
        w.writeTextElement("t", text);
        w.writeEndElement();
        w.writeEndElement();
    };
    auto writeNumCell = [&](const QString& ref, double value) {
        w.writeStartElement("c");
        w.writeAttribute("r", ref);
        w.writeStartElement("v");
        w.writeCharacters(QString::number(value, 'f', 2));
        w.writeEndElement();
        w.writeEndElement();
    };

    w.writeStartElement("row");
    w.writeAttribute("r", "1");
    for (int i = 0; i < headers.size(); ++i) {
        const QChar col = QLatin1Char(char('A' + i));
        writeTextCell(QString(col) + "1", headers.value(i));
    }
    w.writeEndElement();

    const auto months = monthNames();
    for (int i = 0; i < 12; ++i) {
        const auto& m = data.months[i];
        const int row = i + 2;
        w.writeStartElement("row");
        w.writeAttribute("r", QString::number(row));
        writeTextCell(QString("A%1").arg(row), months.value(i));
        writeNumCell(QString("B%1").arg(row), m.sales);
        writeNumCell(QString("C%1").arg(row), m.salesReturn);
        writeNumCell(QString("D%1").arg(row), m.supplierPurchases);
        writeNumCell(QString("E%1").arg(row), m.supplierPayments);
        writeTextCell(QString("F%1").arg(row), m.expenseAccount);
        writeNumCell(QString("G%1").arg(row), m.expenseAmount);
        writeNumCell(QString("H%1").arg(row), m.inventoryFirst);
        writeNumCell(QString("I%1").arg(row), m.inventoryLast);
        writeTextCell(QString("J%1").arg(row), m.supplierName);
        writeNumCell(QString("K%1").arg(row), m.cogsInput);
        w.writeEndElement();
    }

    w.writeEndElement(); // sheetData
    w.writeEndElement(); // worksheet
    w.writeEndDocument();
    return ba;
}

static QByteArray makeAccountsWorksheetXml(const AppData& data)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter w(&buffer);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("worksheet");
    w.writeDefaultNamespace("http://schemas.openxmlformats.org/spreadsheetml/2006/main");
    w.writeNamespace("http://schemas.openxmlformats.org/officeDocument/2006/relationships", "r");
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
    auto writeNumCell = [&](const QString& ref, double value) {
        w.writeStartElement("c");
        w.writeAttribute("r", ref);
        w.writeStartElement("v");
        w.writeCharacters(QString::number(value, 'f', 2));
        w.writeEndElement();
        w.writeEndElement();
    };

    w.writeStartElement("row");
    w.writeAttribute("r", "1");
    writeTextCell("A1", "Account");
    writeTextCell("B1", "Type");
    writeTextCell("C1", "Amount");
    w.writeEndElement();

    for (int i = 0; i < data.accounts.size(); ++i) {
        const int row = i + 2;
        w.writeStartElement("row");
        w.writeAttribute("r", QString::number(row));
        writeTextCell(QString("A%1").arg(row), data.accounts[i].name);
        writeTextCell(QString("B%1").arg(row), accountTypeDisplayName(data.accounts[i].type));
        writeNumCell(QString("C%1").arg(row), data.accounts[i].amount);
        w.writeEndElement();
    }

    w.writeEndElement();
    w.writeEndElement();
    w.writeEndDocument();
    return ba;
}

static QByteArray makeWorkbookXml()
{
    return QByteArray(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">
  <sheets>
    <sheet name="Data" sheetId="1" r:id="rId1"/>
    <sheet name="Accounts" sheetId="2" r:id="rId2"/>
  </sheets>
</workbook>)xml");
}

static QByteArray makeWorkbookRelsXml()
{
    return QByteArray(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet2.xml"/>
</Relationships>)xml");
}

static QByteArray makeRootRelsXml()
{
    return QByteArray(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>
</Relationships>)xml");
}

static QByteArray makeContentTypesXml()
{
    return QByteArray(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>
  <Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
  <Override PartName="/xl/worksheets/sheet2.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>
</Types>)xml");
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
        writeU16(out, 20); // version
        writeU16(out, 0);  // flags
        writeU16(out, 0);  // stored
        writeU16(out, 0); writeU16(out, 0); // time/date
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
        writeU16(out, 20); // made by
        writeU16(out, 20); // needed
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
    writeU16(out, 0);
    writeU16(out, 0);
    writeU16(out, quint16(local.size()));
    writeU16(out, quint16(local.size()));
    writeU32(out, centralSize);
    writeU32(out, centralOffset);
    writeU16(out, 0);
    return true;
}

static bool saveAppDataXlsx(const QString& path, const AppData& data)
{
    QVector<ZipEntry> entries;
    entries.push_back({"[Content_Types].xml", makeContentTypesXml()});
    entries.push_back({"_rels/.rels", makeRootRelsXml()});
    entries.push_back({"xl/workbook.xml", makeWorkbookXml()});
    entries.push_back({"xl/_rels/workbook.xml.rels", makeWorkbookRelsXml()});
    entries.push_back({"xl/worksheets/sheet1.xml", makeWorksheetXml(data)});
    entries.push_back({"xl/worksheets/sheet2.xml", makeAccountsWorksheetXml(data)});
    return writeStoredZip(path, entries);
}

// Decompress raw deflate (ZIP method 8) using zlib
static QByteArray inflateRawDeflate(const QByteArray& compressed, quint32 uncompSize)
{
    if (uncompSize == 0) return {};
    QByteArray out(int(uncompSize), '\0');
    z_stream strm{};
    strm.avail_in  = uInt(compressed.size());
    strm.next_in   = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.constData()));
    strm.avail_out = uInt(out.size());
    strm.next_out  = reinterpret_cast<Bytef*>(out.data());
    if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) return {};
    inflate(&strm, Z_FINISH);
    inflateEnd(&strm);
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
    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("si")) {
            // Collect all <t> text inside this <si>
            QString siText;
            int depth = 1;
            while (!xr.atEnd() && depth > 0) {
                xr.readNext();
                if (xr.isStartElement()) {
                    ++depth;
                    if (xr.name() == QLatin1String("t"))
                        siText += xr.readElementText();
                } else if (xr.isEndElement()) {
                    --depth;
                }
            }
            result.append(siText);
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

static QList<AccountItem> parseAccountsSheet(const QByteArray& sheet)
{
    QList<AccountItem> accounts;
    if (sheet.isEmpty()) return accounts;
    QXmlStreamReader xr(sheet);
    int rowNum = 0;
    QMap<int, QString> rowCells;

    auto parseType = [](const QString& text) {
        const QString k = text.trimmed().toCaseFolded();
        if (k.contains(QString::fromUtf8("مدين")) || k.contains(QStringLiteral("receivable")))
            return AccountType::Receivable;
        return AccountType::Payable;
    };

    auto flushRow = [&]() {
        if (rowNum < 2) return;
        AccountItem a;
        a.name = rowCells.value(0).trimmed();
        if (rowCells.contains(2)) {
            a.type = parseType(rowCells.value(1));
            a.amount = rowCells.value(2).trimmed().toDouble();
        } else {
            a.type = AccountType::Payable;
            a.amount = rowCells.value(1).trimmed().toDouble();
        }
        if (!a.name.isEmpty() || !qFuzzyIsNull(a.amount))
            accounts.append(a);
        rowCells.clear();
    };

    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("row")) {
            rowCells.clear();
            rowNum = xr.attributes().value("r").toInt();
        } else if (xr.isStartElement() && xr.name() == QLatin1String("c")) {
            const QString ref      = xr.attributes().value("r").toString();
            const QString cellType = xr.attributes().value("t").toString();
            const int colIdx = colLetterToIndex(ref);
            QString val;
            while (!(xr.isEndElement() && xr.name() == QLatin1String("c")) && !xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement()) {
                    if (xr.name() == QLatin1String("v")) {
                        QString raw = xr.readElementText();
                        val = raw;
                    } else if (xr.name() == QLatin1String("t") && cellType == QStringLiteral("inlineStr")) {
                        val = xr.readElementText();
                    }
                }
            }
            if (colIdx >= 0 && colIdx <= 2)
                rowCells[colIdx] = val;
        } else if (xr.isEndElement() && xr.name() == QLatin1String("row")) {
            flushRow();
        }
    }
    return accounts;
}

static bool loadAppDataXlsx(const QString& path, AppData* data)
{
    if (!data) return false;
    QMap<QString, QByteArray> entries;
    if (!readZipEntries(path, &entries)) return false;

    // Try to find the worksheets – check common paths
    QByteArray sheet;
    QByteArray accountsSheet;
    for (const QString& key : entries.keys()) {
        if (key.endsWith("worksheets/sheet1.xml")) sheet = entries.value(key);
        else if (key.endsWith("worksheets/sheet2.xml")) accountsSheet = entries.value(key);
        else if (key.contains("worksheets/sheet") && key.endsWith(".xml") && sheet.isEmpty()) sheet = entries.value(key);
    }
    if (sheet.isEmpty()) return false;

    // Build shared strings table (present when Excel saves strings)
    QList<QString> sharedStrings;
    for (const QString& key : entries.keys()) {
        if (key.contains("sharedStrings")) {
            sharedStrings = parseSharedStrings(entries.value(key));
            break;
        }
    }

    // Map of column index → column name in our data (0=Month, 1=Sales, …, 8=InvLast)
    // We'll parse using cell references (e.g. "B3") so skipped cells don't break alignment.
    QXmlStreamReader xr(sheet);
    int rowNum = 0;

    // Per-row data: col index (0-based) → string value
    QMap<int, QString> rowCells;

    auto flushRow = [&]() {
        if (rowNum < 2) return;                    // row 1 is header
        const int i = rowNum - 2;
        if (i < 0 || i >= 12) return;
        auto& m = data->months[i];
        auto toDouble = [&](int col) {
            return rowCells.value(col).trimmed().toDouble();
        };
        // Col A(0)=Month name, B(1)=Sales, C(2)=SalesReturn, D(3)=SupPurch,
        // E(4)=SupPayments, F(5)=ExpAccount, G(6)=ExpAmount, H(7)=InvFirst, I(8)=InvLast,
        // J(9)=SupplierName, K(10)=COGSInput
        m.sales             = toDouble(1);
        m.salesReturn       = toDouble(2);
        m.supplierPurchases = toDouble(3);
        m.supplierPayments  = toDouble(4);
        m.expenseAccount    = rowCells.value(5);
        m.expenseAmount     = toDouble(6);
        m.inventoryFirst    = toDouble(7);
        m.inventoryLast     = toDouble(8);
        m.supplierName      = rowCells.value(9);
        m.cogsInput         = toDouble(10);
        rowCells.clear();
    };

    while (!xr.atEnd()) {
        xr.readNext();
        if (xr.isStartElement() && xr.name() == QLatin1String("row")) {
            rowCells.clear();
            rowNum = xr.attributes().value("r").toInt();
        } else if (xr.isStartElement() && xr.name() == QLatin1String("c")) {
            const QString ref      = xr.attributes().value("r").toString();
            const QString cellType = xr.attributes().value("t").toString();
            const int colIdx = colLetterToIndex(ref);

            QString val;
            while (!(xr.isEndElement() && xr.name() == QLatin1String("c")) && !xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement()) {
                    if (xr.name() == QLatin1String("v")) {
                        QString raw = xr.readElementText();
                        if (cellType == QStringLiteral("s")) {
                            // Shared string index
                            bool ok = false;
                            int si = raw.toInt(&ok);
                            val = (ok && si < sharedStrings.size()) ? sharedStrings[si] : raw;
                        } else {
                            val = raw;
                        }
                    } else if (xr.name() == QLatin1String("t") && cellType == QStringLiteral("inlineStr")) {
                        val = xr.readElementText();
                    }
                }
            }
            if (colIdx >= 0 && colIdx <= 10)
                rowCells[colIdx] = val;
        } else if (xr.isEndElement() && xr.name() == QLatin1String("row")) {
            flushRow();
        }
    }
    if (xr.hasError()) return false;
    data->accounts = parseAccountsSheet(accountsSheet);
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
        m_tabs->addTab(m_suppliers, "");
    }

    // Tab 3: Results
    {
        m_results = new ResultsWidget;
        connect(m_results, &ResultsWidget::editChartsRequested, this, &MainWindow::onEditCharts);
        connect(m_results, &ResultsWidget::resultsStateChanged, this, &MainWindow::syncResultsState);
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
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;

    ChartSelectionDialog dlg(working, this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    working.sel = dlg.selections();
    working.chartRequests = dlg.chartRequests();
    m_data = working;
    m_hasResults = true;

    if (m_results) {
        m_results->buildResults(m_data);
        syncResultsState();
    }
    m_tabs->setCurrentIndex(3);
}


void MainWindow::onEditCharts()
{
    if (!m_hasResults)
        return;

    if (auto* popup = QApplication::activePopupWidget())
        popup->hide();

    AppData working = collectAllData();
    working.chartRequests = m_lastChartRequests;
    working.hiddenChartRequests = m_lastHiddenChartRequests;
    working.resultFlowOrder = m_lastFlowOrder;

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
    req.accountFilter = accountFilter;
    req.title = metricDisplayName(M_EXPENSES);
    switch (kind) {
    case ChartKind::Pie:
        req.title += QStringLiteral(" — ") + tr_pie_97ce50();
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

    m_results->appendChart(m_data, req);
    syncResultsState();
    m_tabs->setCurrentIndex(3);
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
    if (m_table)
        m_table->setInventoryMode(mode);

    AppData d = collectAllData();
    d.inventoryMode = mode;
    d.calculate();
    setTableData(d);

    m_data = d;
    if (m_hasResults) {
        // Preserve the selected charts and visible flow when refreshing the mode.
        m_data.chartRequests = m_results ? m_results->chartRequests() : m_data.chartRequests;
        m_data.hiddenChartRequests = m_results ? m_results->hiddenChartRequests() : m_data.hiddenChartRequests;
        m_data.resultFlowOrder = m_results ? m_results->flowOrder() : m_data.resultFlowOrder;
    }

    if (m_results)
        m_results->buildResults(m_data);
}

void MainWindow::onSaveData()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr_save_data_ee42b8(),
        tr_default_account_data_filename_0aa2f1().arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmm")),
        tr_excel_workbook_xlsx_all_files_aa27cc());
    if (path.isEmpty()) return;

    const AppData data = collectAllData();
    if (!saveAppDataXlsx(path, data)) {
        ThemeBox::critical(this,
            tr_save_data_ee42b8(),
            tr_unable_to_write_the_xlsx_file_da2b9b());
        return;
    }

    ThemeBox::info(this,
        tr_save_data_ee42b8(),
        tr_data_saved_successfully_f941c7());
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
            tr_unable_to_read_the_xlsx_file_cd99e7());
        return;
    }

    setTableData(imported);
    setAccountData(imported.accounts);
    m_data = imported;
    m_hasResults = false;
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
    if (g_classicView && m_classicTable) {
        AppData d = m_classicTable->collectData();
        if (m_table)
            d.inventoryMode = m_table->inventoryMode();
        return d;
    }
    return m_table ? m_table->collectData() : AppData{};
}
void MainWindow::setTableData(const AppData& d)
{
    if (m_table) {
        m_table->setData(d);
        m_table->setInventoryMode(d.inventoryMode);
    }
    if (m_classicTable) m_classicTable->setData(d);
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
    if (m_suppliers)
        d.suppliers = m_suppliers->collectData().suppliers;
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
