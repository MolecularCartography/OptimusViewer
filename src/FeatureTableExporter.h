#ifndef FEATURE_TABLE_EXPORTER_H
#define FEATURE_TABLE_EXPORTER_H

#include <QObject>

namespace qm {

class FeatureTableModel;

class FeatureTableExporter: public QObject
{
    Q_OBJECT
public:
    FeatureTableExporter(const FeatureTableModel &model);

public slots:
    void exportFeatures(const QVector<int> &visibleColumns);

private:
    const FeatureTableModel &model;
};

} // namespace qm

#endif // FEATURE_TABLE_EXPORTER_H
