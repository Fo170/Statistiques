#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QStatusBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>
#include <QVector>
#include <QPointF>
#include <vector>
#include <string>
#include <cmath>
#include <cstdio>

#include "statsengine.h"
#include "chartwidget.h"

static std::vector<double> xd, yd;
static StatsEngine engine;
static ChartWidget* chart = nullptr;
static QTextEdit* results = nullptr;
static QStatusBar* g_status = nullptr;

static void clearData()
{
    xd.clear();
    yd.clear();
    chart->setData(QVector<QPointF>());
    chart->setRegression(nullptr);
    results->clear();
}

static void updateChart()
{
    if (xd.empty()) return;
    engine.sortData(xd, yd);
    QVector<QPointF> pts;
    pts.reserve((int)xd.size());
    for (size_t i = 0; i < xd.size(); i++)
        pts.append(QPointF(xd[i], yd[i]));
    chart->setData(pts);
    chart->setRegression(&engine);
}

static void showResults()
{
    results->clear();
    results->setPlainText(QString::fromStdString(engine.resultsText()));
}

static void runAutoRegression();

static void inputManual()
{
    bool ok;
    int n = QInputDialog::getInt(nullptr, "Saisie", "Nombre de points :", 5, 1, 10000, 1, &ok);
    if (!ok || n <= 0) return;

    xd.clear();
    yd.clear();

    for (int i = 0; i < n; i++) {
        double x = QInputDialog::getDouble(nullptr, "Point " + QString::number(i+1),
                                           "X :", 0, -1e12, 1e12, 6, &ok);
        if (!ok) break;
        double y = QInputDialog::getDouble(nullptr, "Point " + QString::number(i+1),
                                           "Y :", 0, -1e12, 1e12, 6, &ok);
        if (!ok) break;
        xd.push_back(x);
        yd.push_back(y);
    }

    if (!xd.empty()) {
        engine.sortData(xd, yd);
        updateChart();
        runAutoRegression();
        QMessageBox::information(nullptr, "Saisie",
            QString("%1 points saisis.").arg(xd.size()));
    }
}

static void inputFile()
{
    QString fname = QFileDialog::getOpenFileName(nullptr, "Ouvrir un fichier",
                                                  QString(), "Fichiers texte (*.txt *.dat *.fch);;Tous (*)");
    if (fname.isEmpty()) return;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Erreur", "Impossible d'ouvrir le fichier.");
        return;
    }

    xd.clear();
    yd.clear();
    QTextStream in(&file);
    QString line = in.readLine().trimmed();
    int n = line.toInt();

    for (int i = 0; i < n; i++) {
        if (in.atEnd()) break;
        line = in.readLine().trimmed();
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            xd.push_back(parts[0].toDouble());
            yd.push_back(parts[1].toDouble());
        }
    }
    file.close();

    if (!xd.empty()) {
        engine.sortData(xd, yd);
        updateChart();
        runAutoRegression();
        QMessageBox::information(nullptr, "Fichier",
            QString("Lu %1 points depuis %2").arg(xd.size()).arg(fname));
    }
}

static void inputFunction()
{
    QStringList items;
    items << "sin(x)" << "cos(x)" << "exp(x)" << "log(x)"
          << "sqrt(x)" << "x^2" << "x^3" << "2*x+3";

    bool ok;
    QString item = QInputDialog::getItem(nullptr, "Fonction",
                                          "Choix de la fonction :", items, 0, false, &ok);
    if (!ok) return;
    int choice = items.indexOf(item);

    double xmin = QInputDialog::getDouble(nullptr, "X min", "X min :", 0, -1e12, 1e12, 6, &ok);
    if (!ok) return;
    double xmax = QInputDialog::getDouble(nullptr, "X max", "X max :", 10, -1e12, 1e12, 6, &ok);
    if (!ok) return;
    int n = QInputDialog::getInt(nullptr, "Points", "Nombre de points :", 100, 2, 100000, 1, &ok);
    if (!ok) return;

    xd.clear();
    yd.clear();
    double dx = (xmax - xmin) / (n - 1);
    for (int i = 0; i < n; i++) {
        double x = xmin + i * dx;
        double y = 0;
        switch (choice) {
        case 0: y = std::sin(x); break;
        case 1: y = std::cos(x); break;
        case 2: y = std::exp(x); break;
        case 3: y = (x > 0) ? std::log(x) : 0; break;
        case 4: y = (x >= 0) ? std::sqrt(x) : 0; break;
        case 5: y = x * x; break;
        case 6: y = x * x * x; break;
        case 7: y = 2 * x + 3; break;
        default: y = x; break;
        }
        xd.push_back(x);
        yd.push_back(y);
    }

    if (!xd.empty()) {
        updateChart();
        runAutoRegression();
        QMessageBox::information(nullptr, "Fonction",
            QString("Fonction %1 generee : %2 points.").arg(item).arg(xd.size()));
    }
}

static void runRegression(int mode)
{
    if (xd.empty()) {
        QMessageBox::warning(nullptr, "Regression", "Aucune donnee. Saisissez des points d'abord.");
        return;
    }
    engine.setMode(mode);
    engine.compute(xd, yd);
    showResults();
    chart->setRegression(&engine);
    chart->update();
}

static void runAutoRegression()
{
    if (xd.empty()) {
        QMessageBox::warning(nullptr, "Regression", "Aucune donnee.");
        return;
    }
    int best = engine.autoMode(xd, yd);
    showResults();
    chart->setRegression(&engine);
    chart->update();
    if (g_status) g_status->showMessage(
        QString("Meilleure regression : mode %1").arg(best));
}

static void saveResults()
{
    if (xd.empty()) {
        QMessageBox::warning(nullptr, "Sauvegarde", "Aucune donnee a sauvegarder.");
        return;
    }
    QString fname = QFileDialog::getSaveFileName(nullptr, "Sauvegarder",
                                                  "resultats.txt",
                                                  "Fichiers texte (*.txt);;Tous (*)");
    if (fname.isEmpty()) return;

    QFile file(fname);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(nullptr, "Erreur", "Impossible d'ecrire.");
        return;
    }
    QTextStream out(&file);
    out << "=== STATISTIQUES - Resultats de regression ===\n\n";
    out << "Nombre de points : " << xd.size() << "\n\n";
    out << "Donnees originales :\n";
    for (size_t i = 0; i < xd.size(); i++)
        out << xd[i] << "\t" << yd[i] << "\n";
    out << "\n";
    out << QString::fromStdString(engine.resultsText()) << "\n";
    out << "Valeurs estimees (Ycal) :\n";
    for (size_t i = 0; i < xd.size(); i++) {
        double yc = (double)engine.regFY(xd[i]);
        out << xd[i] << "\t" << yd[i] << "\t" << yc << "\n";
    }
    file.close();
    if (g_status) g_status->showMessage("Resultats sauvegardes dans " + fname);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Statistiques");
    app.setApplicationVersion("2.0");

    QMainWindow win;
    win.setWindowTitle("Statistiques - Regression Analysis");
    win.resize(1000, 700);

    QSplitter* split = new QSplitter(Qt::Vertical, &win);
    chart = new ChartWidget();
    split->addWidget(chart);
    results = new QTextEdit();
    results->setReadOnly(true);
    results->setFont(QFont("Consolas", 10));
    results->setMaximumHeight(200);
    split->addWidget(results);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);
    win.setCentralWidget(split);

    QMenuBar* mb = win.menuBar();

    QMenu* mnuFile = mb->addMenu("&Fichier");
    mnuFile->addAction("&Ouvrir...", &inputFile);
    mnuFile->addAction("&Sauvegarder...", &saveResults);
    mnuFile->addSeparator();
    mnuFile->addAction("&Quitter", &win, &QMainWindow::close);

    QMenu* mnuData = mb->addMenu("&Donnees");
    mnuData->addAction("Saisie &manuelle...", &inputManual);
    mnuData->addAction("&Fonction mathematique...", &inputFunction);
    mnuData->addSeparator();
    mnuData->addAction("&Effacer", &clearData);

    QMenu* mnuReg = mb->addMenu("&Regression");
    mnuReg->addAction("&Lineaire (mode 0)", [&](){ runRegression(0); });
    mnuReg->addAction("&Logarithmique (mode 1)", [&](){ runRegression(1); });
    mnuReg->addAction("&Exponentielle (mode 2)", [&](){ runRegression(2); });
    mnuReg->addAction("&Puissance (mode 3)", [&](){ runRegression(3); });
    mnuReg->addAction("Puissance &NLS (mode 4)", [&](){ runRegression(4); });
    mnuReg->addAction("&Reciproque (mode 5)", [&](){ runRegression(5); });
    mnuReg->addAction("&Polynomial deg 2 (mode 6)", [&](){ runRegression(6); });
    mnuReg->addAction("&Sinusoidal (mode 7)", [&](){ runRegression(7); });
    mnuReg->addAction("&Logistique (mode 8)", [&](){ runRegression(8); });
    mnuReg->addSeparator();
    mnuReg->addAction("&Auto (best fit)", &runAutoRegression);

    mb->addAction("&Reinitialiser le zoom", chart, &ChartWidget::resetView);

    g_status = win.statusBar();
    g_status->showMessage("Pret");

    win.show();
    return app.exec();
}
