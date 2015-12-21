#ifndef SAVEGRAPHDIALOG_H
#define SAVEGRAPHDIALOG_H

#include <QMap>
#include <QFileDialog>

#include "Globals.h"

namespace ov {

class SaveGraphDialog : public QFileDialog
{
    Q_OBJECT

public:
    SaveGraphDialog(QWidget *parent, const FormatId &selectedFormat);

    double getScale() const;
    int getQuality() const;
    QString getSelectedFormat() const;

private slots:
    void filterSelected(const QString &filter);
    void qualityChanged(int value);
    void scaleChanged(double value);

private:
    void setupUi();
    void initFilters();

    double selectedScale;
    int selectedQuality;
    QMap<QString, QString> formatByFilter;

    QList<QWidget *> qualityControllers;
    QList<QWidget *> scaleControllers;
};

} // namespace ov

#endif // SAVEGRAPHDIALOG_H
