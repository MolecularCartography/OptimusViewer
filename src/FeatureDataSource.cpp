#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QVariant>

#include "FeatureDataSource.h"

namespace ov {

FeatureDataSource::FeatureDataSource(const DataSourceId &dataSourceId)
{
    db = QSqlDatabase::addDatabase("QSQLITE");
}

bool FeatureDataSource::isValid() const
{
    return db.isOpen();
}

QList<GraphPoint> FeatureDataSource::getXicData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const
{
    Q_ASSERT(isValid());
    QList<GraphPoint> result;
    if (featuresBySample.isEmpty()) {
        return result;
    }

    QString queryStr = "SELECT sample_id, feature_id, rt, intensity FROM FeatureXIC WHERE ";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach (const SampleId &sampleId, featuresBySample.keys()) {
        foreach (const FeatureId &featureId, featuresBySample.values(sampleId)) {
            queryStr.append("(sample_id = ? AND feature_id = ?)");
            bindParameters.append(sampleId);
            bindParameters.append(featureId);
            queryStr.append(queryConjunction);
        }
    }
    queryStr.chop(queryConjunction.length());
    queryStr.append(" ORDER BY rt");

    QSqlQuery query;
    query.prepare(queryStr);
    foreach (const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        result.append(GraphPoint(query.value(0).value<SampleId>(),
            query.value(1).value<FeatureId>(), query.value(2).toReal(), query.value(3).toReal()));
    }
    return result;
}

QList<GraphPoint> FeatureDataSource::getIsotopicPatternData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const
{
    Q_ASSERT(isValid());
    QList<GraphPoint> result;
    if (featuresBySample.isEmpty()) {
        return result;
    }

    QString queryStr = "SELECT F.sample_id, F.feature_id, M.mz, M.intensity FROM FeatureMs1Peak AS F, MassPeak AS M WHERE ";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach(const SampleId &sampleId, featuresBySample.keys()) {
        foreach(const FeatureId &featureId, featuresBySample.values(sampleId)) {
            queryStr.append("(F.sample_id = ? AND F.feature_id = ? AND F.mass_peak_id = M.id)");
            bindParameters.append(sampleId);
            bindParameters.append(featureId);
            queryStr.append(queryConjunction);
        }
    }
    queryStr.chop(queryConjunction.length());
    queryStr.append(" ORDER BY M.mz");

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        result.append(GraphPoint(query.value(0).value<SampleId>(),
            query.value(1).value<FeatureId>(), query.value(2).toReal(), query.value(3).toReal()));
    }
    return result;
}

QList<GraphPoint> FeatureDataSource::getMs2SpectraData(const QHash<SampleId, QMultiHash<FeatureId, qreal> > &featureScansBySample) const
{
    Q_ASSERT(isValid());
    QList<GraphPoint> result;
    if (featureScansBySample.isEmpty()) {
        return result;
    }

    QString queryStr = "SELECT F.sample_id, F.feature_id, F.scan_time, F.precursor_mz, M.mz, M.intensity FROM FeatureMs2Peak AS F, MassPeak AS M WHERE ";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach(const SampleId &sampleId, featureScansBySample.keys()) {
        foreach(const FeatureId &featureId, featureScansBySample[sampleId].keys()) {
            foreach (qreal scanStartTime, featureScansBySample[sampleId].values(featureId)) {
                queryStr.append("(F.sample_id = ? AND F.feature_id = ? AND F.scan_time = ? AND F.mass_peak_id = M.id)");
                bindParameters.append(sampleId);
                bindParameters.append(featureId);
                bindParameters.append(scanStartTime);
                queryStr.append(queryConjunction);
            }
        }
    }
    queryStr.chop(queryConjunction.length());
    queryStr.append(" ORDER BY M.mz");

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        QMap<GraphPoint::Attribute, QVariant> attrs;
        attrs[GraphPoint::SCAN_START_TIME] = query.value(2);
        attrs[GraphPoint::PRECURSOR_MZ_ATTR] = query.value(3);
        result.append(GraphPoint(query.value(0).value<SampleId>(),
            query.value(1).value<FeatureId>(), query.value(4).toReal(), query.value(5).toReal(), attrs));
    }
    return result;
}

QList<GraphPoint> FeatureDataSource::getMs2ScanData(const QMultiHash<SampleId, FeatureId> &featuresBySample) const
{
    Q_ASSERT(isValid());
    QList<GraphPoint> result;
    if (featuresBySample.isEmpty()) {
        return result;
    }

    QString queryStr = "SELECT F.sample_id, F.feature_id, F.scan_time, F.precursor_mz FROM FeatureMs2Peak AS F, MassPeak AS M WHERE ";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach(const SampleId &sampleId, featuresBySample.keys()) {
        foreach(const FeatureId &featureId, featuresBySample.values(sampleId)) {
            queryStr.append("(F.sample_id = ? AND F.feature_id = ? AND F.mass_peak_id = M.id)");
            bindParameters.append(sampleId);
            bindParameters.append(featureId);
            queryStr.append(queryConjunction);
        }
    }
    queryStr.chop(queryConjunction.length());
    queryStr.append(" ORDER BY F.scan_time");

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        QMap<GraphPoint::Attribute, QVariant> attrs;
        attrs[GraphPoint::PRECURSOR_MZ_ATTR] = query.value(3);
        result.append(GraphPoint(query.value(0).value<SampleId>(), query.value(1).value<FeatureId>(),
            query.value(2).toReal(), 0.0, attrs));
    }
    return result;
}

void FeatureDataSource::selectDataSource()
{
    DataSourceId dataSourceId = QFileDialog::getOpenFileName(QApplication::activeWindow(), QObject::tr("Open file"), QString(), getInputFileFilter());
    if (dataSourceId.isEmpty()) {
        return;
    } else if (currentDataSourceId() == dataSourceId) {
        QMessageBox::information(QApplication::activeWindow(), tr("Info"), tr("Selected database is currently opened."));
    } else if (!setDataSource(dataSourceId)) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to read the OptimusFD database."));
    } else {
        emit samplesChanged(getSampleNameById());
    }
}

QMap<SampleId, QString> FeatureDataSource::getSampleNameById()
{
    Q_ASSERT(isValid());

    QMap<SampleId, QString> nameById;
    QSqlQuery query("SELECT id, name FROM Sample");
    while (query.next()) {
        nameById[query.value(0).toLongLong()] = query.value(1).toString();
    }
    return nameById;
}

DataSourceId FeatureDataSource::currentDataSourceId() const
{
    return db.databaseName();
}

bool FeatureDataSource::setDataSource(const DataSourceId &dataSourceId)
{
    Q_ASSERT(!dataSourceId.isEmpty());

    if (isValid()) {
        db.close();
    }

    db.setDatabaseName(dataSourceId);
    return db.open();
}

QString FeatureDataSource::getInputFileFilter()
{
    return QObject::tr("OptimusFD database (*.db)");
}

} // namespace ov
