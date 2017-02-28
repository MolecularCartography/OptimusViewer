#ifndef FEATURE_TABLE_EXPORTER_H
#define FEATURE_TABLE_EXPORTER_H

#include <QObject>

namespace ov {

class AppView;

class FeatureTableExporter: public QObject
{
    Q_OBJECT
public:
    FeatureTableExporter(const AppView &appView);

public slots:
    void exportFeatures(const QVector<int> &visibleColumns);

private:
    const AppView &appView;
};

} // namespace ov

#endif // FEATURE_TABLE_EXPORTER_H
