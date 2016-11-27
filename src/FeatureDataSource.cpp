#include <QApplication>
#include <QDataStream>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QVariant>

#include "FeatureDataSource.h"

const int QUERY_PARAMS_LIMIT = 999;

namespace ov {

FeatureDataSource::FeatureDataSource()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
}

bool FeatureDataSource::isValid() const
{
    return db.isOpen();
}

QMultiHash<SampleId, FeatureId> FeatureDataSource::getFeaturesToExtract(const QMultiHash<SampleId, FeatureId> &featuresBySample,
    QHash<SampleId, QHash<FeatureId, FeatureData> > &presentFeatures, QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > &presentMs2Scans)
{
    QMultiHash<SampleId, FeatureId> featuresToExtract;
    foreach(const SampleId &sampleId, featuresBySample.uniqueKeys()) {
        foreach(const FeatureId &featureId, featuresBySample.values(sampleId)) {
            if (currentFeatures.contains(sampleId) && currentFeatures[sampleId].contains(featureId)) {
                presentFeatures[sampleId].insert(featureId, currentFeatures[sampleId][featureId]);
                presentMs2Scans[sampleId].insert(featureId, currentMs2Scans[sampleId][featureId]);
            } else {
                featuresToExtract.insert(sampleId, featureId);
            }
        }
    }
    return featuresToExtract;
}

void FeatureDataSource::fetchFeatures(const QMultiHash<SampleId, FeatureId> &featureIdsToExtract, QHash<SampleId, QHash<FeatureId, FeatureData> > &features)
{
    if (featureIdsToExtract.isEmpty()) {
        return;
    }

    QString queryStr = "SELECT sample_id, feature_id, data, rt_start, rt_end FROM FeatureMassTrace WHERE ";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach(const SampleId &sampleId, featureIdsToExtract.uniqueKeys()) {
        foreach(const FeatureId &featureId, featureIdsToExtract.values(sampleId)) {
            queryStr.append("(sample_id = ? AND feature_id = ?)");
            bindParameters.append(sampleId);
            bindParameters.append(featureId);
            queryStr.append(queryConjunction);
        }
    }
    queryStr.chop(queryConjunction.length());

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        const SampleId sampleId = query.value(0).value<SampleId>();
        const SampleId featureId = query.value(1).value<FeatureId>();
        QByteArray massTraceData = query.value(2).toByteArray();
        QDataStream binaryStream(&massTraceData, QIODevice::ReadOnly);
        binaryStream.setByteOrder(QDataStream::LittleEndian);
        QList<QVector3D> massTrace;
        while (!binaryStream.atEnd()) {
            double mz = 0.0;
            float rt = 0.0;
            float intensity = 0.0;
            int bytesRead = binaryStream.readRawData(reinterpret_cast<char *>(&mz), sizeof(mz));
            Q_ASSERT(bytesRead == sizeof(mz));
            bytesRead = binaryStream.readRawData(reinterpret_cast<char *>(&rt), sizeof(rt));
            Q_ASSERT(bytesRead == sizeof(rt));
            bytesRead = binaryStream.readRawData(reinterpret_cast<char *>(&intensity), sizeof(intensity));
            Q_ASSERT(bytesRead == sizeof(intensity));
            massTrace.append(QVector3D(mz, rt, intensity));
        }
        const qreal massTraceStart = query.value(3).toReal();
        const qreal massTraceEnd = query.value(4).toReal();

        if (!features.contains(sampleId) || !features[sampleId].contains(featureId)) {
            features[sampleId][featureId] = FeatureData(sampleId, featureId, QList<QList<QVector3D> >() << massTrace, massTraceStart, massTraceEnd);
        } else {
            FeatureData &feature = features[sampleId][featureId];
            feature.massTraces.append(massTrace);
            feature.featureStart = qMin(feature.featureStart, massTraceStart);
            feature.featureEnd = qMax(feature.featureEnd, massTraceEnd);
        }
    }
}

void FeatureDataSource::fetchMs2Scans(const QMultiHash<SampleId, FeatureId> &featuresToExtract, QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > &ms2Scans)
{
    if (featuresToExtract.isEmpty()) {
        return;
    }

    QString queryStr = "SELECT FMT.sample_id, FMT.feature_id, FS.scan_time, FS.precursor_mz, FS.precursor_intensity, FS.id "
        "FROM FeatureMassTrace AS FMT, MassTraceFragmentationSpectrum AS MSFS, FragmentationSpectrum AS FS "
        "WHERE FMT.id = MSFS.mt_id AND MSFS.spectrum_id = FS.id AND (";
    QVariantList bindParameters;
    const QString queryConjunction = " OR ";
    foreach(const SampleId &sampleId, featuresToExtract.uniqueKeys()) {
        foreach(const FeatureId &featureId, featuresToExtract.values(sampleId)) {
            queryStr.append("(FMT.sample_id = ? AND FMT.feature_id = ?)");
            bindParameters.append(sampleId);
            bindParameters.append(featureId);
            queryStr.append(queryConjunction);
        }
    }
    queryStr.chop(queryConjunction.length());
    queryStr.append(") ORDER BY FS.scan_time");

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const QVariant &value, bindParameters) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        const SampleId sampleId = query.value(0).value<SampleId>();
        const SampleId featureId = query.value(1).value<FeatureId>();
        ms2Scans[sampleId][featureId].append(Ms2ScanInfo(query.value(2).toReal(), query.value(3).toReal(),
            query.value(4).toReal(), query.value(5).value<FragmentationSpectrumId>()));
    }
}

bool FeatureDataSource::setActiveFeatures(const QMultiHash<SampleId, FeatureId> &featuresBySample)
{
    if (featuresBySample.isEmpty()) {
        currentFeatures.clear();
        currentMs2Scans.clear();
        return true;
    }

    // Limit on number of SQLite query parameters
    // TODO: consider splitting the query into multiple ones.
    if (featuresBySample.values().size() * 2 > QUERY_PARAMS_LIMIT) {
        currentFeatures.clear();
        currentMs2Scans.clear();
        return false;
    }

    QHash<SampleId, QHash<FeatureId, FeatureData> > newFeatures;
    QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > newMs2Scans;
    const QMultiHash<SampleId, FeatureId> featuresToExtract = getFeaturesToExtract(featuresBySample, newFeatures, newMs2Scans);

    fetchFeatures(featuresToExtract, newFeatures);
    currentFeatures = newFeatures;

    fetchMs2Scans(featuresToExtract, newMs2Scans);
    currentMs2Scans = newMs2Scans;

    return true;
}

QList<FeatureData> FeatureDataSource::getMs1Data() const
{
    QList<FeatureData> result;
    foreach (const SampleId &sampleId, currentFeatures.keys()) {
        result.append(currentFeatures[sampleId].values());
    }
    return result;
}

QHash<SampleId, QHash<FeatureId, QList<Ms2ScanInfo> > > FeatureDataSource::getMs2ScanData() const
{
    Q_ASSERT(isValid());
    return currentMs2Scans;
}

QHash<FragmentationSpectrumId, QList<QPointF> > FeatureDataSource::getMs2SpectraData(const QList<FragmentationSpectrumId> &spectrumIds) const
{
    Q_ASSERT(isValid());
    QHash<FragmentationSpectrumId, QList<QPointF> > result;

    if (spectrumIds.size() > QUERY_PARAMS_LIMIT) {
        return result;
    }

    const QString queryStr = QString("SELECT FS.id, FS.data FROM FragmentationSpectrum AS FS WHERE FS.id IN (%1)").arg(QStringList(QVector<QString>(spectrumIds.size(), "?").toList()).join(","));

    QSqlQuery query;
    query.prepare(queryStr);
    foreach(const FragmentationSpectrumId &value, spectrumIds) {
        query.addBindValue(value);
    }
    const bool ok = query.exec();
    Q_ASSERT(ok);

    while (query.next()) {
        const FragmentationSpectrumId spectrumId = query.value(0).value<FragmentationSpectrumId>();
        QByteArray spectrumData = query.value(1).toByteArray();
        QDataStream binaryStream(&spectrumData, QIODevice::ReadOnly);
        binaryStream.setByteOrder(QDataStream::LittleEndian);
        QList<QPointF> spectrum;
        while (!binaryStream.atEnd()) {
            double mz = 0.0;
            float intensity = 0.0;
            int bytesRead = binaryStream.readRawData(reinterpret_cast<char *>(&mz), sizeof(mz));
            Q_ASSERT(bytesRead == sizeof(mz));
            bytesRead = binaryStream.readRawData(reinterpret_cast<char *>(&intensity), sizeof(intensity));
            Q_ASSERT(bytesRead == sizeof(intensity));
            spectrum.append(QPointF(mz, intensity));
        }
        result[spectrumId] = spectrum;
    }
    return result;
}

void FeatureDataSource::selectDataSource()
{
    DataSourceId dataSourceId = QFileDialog::getOpenFileName(QApplication::activeWindow(), QObject::tr("Open File"), QString(), getInputFileFilter());
    if (dataSourceId.isEmpty()) {
        return;
    } else if (setDataSource(dataSourceId)) {
        updateSamplesInfo();
        updateFeaturesInfo();
        currentFeatures.clear();
        emit samplesChanged();
    }
}

SampleId FeatureDataSource::getSampleIdByNumber(int number) const
{
    if (0 <= number && number < sampleIds.size()) {
        return sampleIds[number];
    } else {
        Q_ASSERT(false);
        return -1;
    }
}

QString FeatureDataSource::getSampleNameById(const SampleId &id) const
{
    if (sampleNameById.contains(id)) {
        return sampleNameById[id];
    } else {
        Q_ASSERT(false);
        return QString();
    }
}

qint64 FeatureDataSource::getSampleCount() const
{
    return sampleIds.size();
}

FeatureId FeatureDataSource::getFeatureIdByNumber(int number) const
{
    if (0 <= number && number < featureIds.size()) {
        return featureIds[number];
    } else {
        Q_ASSERT(false);
        return -1;
    }
}

QHash<FeatureId, QStringList> FeatureDataSource::getFeatureCompoundIds(const QSet<FeatureId> &ids) const
{
    // Limit on number of SQLite query parameters
    // TODO: consider splitting the query into multiple ones.
    if (ids.isEmpty() || ids.size() > QUERY_PARAMS_LIMIT) {
        return QHash<FeatureId, QStringList>();
    }

    QSqlQuery annotationsQuery;
    QString queryText("SELECT feature_id, compound_id FROM Annotation, FeatureAnnotation WHERE annotation_id = id AND feature_id IN (");
    queryText.append(QString("?,").repeated(ids.size()));
    queryText.replace(queryText.length() - 1, 1, ")");
    annotationsQuery.prepare(queryText);
    foreach (const FeatureId &id, ids) {
        annotationsQuery.addBindValue(id);
    }
    const bool ok = annotationsQuery.exec();
    Q_ASSERT(ok);
    QHash<FeatureId, QStringList> result;
    foreach (const FeatureId &fId, ids) {
        result[fId] = QStringList();
    }
    while (annotationsQuery.next()) {
        result[annotationsQuery.value(0).value<FeatureId>()].append(annotationsQuery.value(1).toString());
    }
    return result;
}

qint64 FeatureDataSource::getFeatureCount() const
{
    return featureIds.size();
}

void FeatureDataSource::updateSamplesInfo()
{
    sampleIds.clear();

    QSqlQuery samplesQuery("SELECT id, name FROM Sample ORDER BY id");
    while (samplesQuery.next()) {
        const SampleId id = samplesQuery.value(0).value<SampleId>();
        sampleNameById[id] = samplesQuery.value(1).toString();
        sampleIds.append(id);
    }
}

void FeatureDataSource::updateFeaturesInfo()
{
    featureIds.clear();

    QSqlQuery featuresQuery("SELECT id FROM Feature ORDER BY id");
    while (featuresQuery.next()) {
        featureIds.append(featuresQuery.value(0).value<FeatureId>());
    }
}

DataSourceId FeatureDataSource::currentDataSourceId() const
{
    return db.databaseName();
}

namespace {

int versionToInt(const QString &strVersion)
{
    return QString(strVersion).replace(".", "").toInt();
}

QString getMetaInfoValue(const QString &key)
{
    QSqlQuery query;
    query.prepare("SELECT value FROM MetaInfo WHERE key = ?");
    query.addBindValue(key);
    const bool ok = query.exec();
    Q_ASSERT(ok);
    query.next();
    return query.value(0).toString();
}

}

bool FeatureDataSource::isDataSourceVersionSupported()
{
    const QString minOptimusVersionStr = getMetaInfoValue("min_compatible_optimus_version");
    const int dataSourceMinOptimusVersion = versionToInt(minOptimusVersionStr);

    const QString curOptimusVersionStr = getMetaInfoValue("optimus_version");
    const int dataSourceCurOptimusVersion = versionToInt(curOptimusVersionStr);

    const int curSupportedVersion = versionToInt(CURRENT_OPTIMUS_VERSION);
    if (dataSourceMinOptimusVersion > curSupportedVersion) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"),
            tr("This Optimus database was created by Optimus version %1 "
               "which is not compatible with the current version of OptimusViewer. "
               "Use newer versions of OptimusViewer to open this file.").arg(curOptimusVersionStr));
        return false;
    }

    const int minSupportedVersion = versionToInt(MIN_COMPATIBLE_OPTIMUS_VERSION);
    if (dataSourceCurOptimusVersion < minSupportedVersion) {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"),
            tr("This Optimus database was created by Optimus version %1 "
            "which is not compatible with the current version of OptimusViewer. "
            "Use previous versions of OptimusViewer to open this file.").arg(curOptimusVersionStr));
        return false;
    }

    if (dataSourceCurOptimusVersion > curSupportedVersion) {
        QMessageBox::warning(QApplication::activeWindow(), tr("Warning"),
            tr("This Optimus database was created by Optimus version %1 "
            "which is newer than expected by the current version of OptimusViewer. "
            "Some features might be not available.").arg(curOptimusVersionStr));
    }
    return true;
}

bool FeatureDataSource::setDataSource(const DataSourceId &dataSourceId)
{
    Q_ASSERT(!dataSourceId.isEmpty());

    if (isValid()) {
        db.close();
    }

    db.setDatabaseName(dataSourceId);
    bool storageAvailable = db.open();

    if (storageAvailable) {
        QSqlQuery prepQuery(
            "PRAGMA synchronous = OFF;"
            "PRAGMA main.locking_mode = EXCLUSIVE;"
            "PRAGMA temp_store = MEMORY;"
            "PRAGMA journal_mode = MEMORY;"
            "PRAGMA cache_size = 50000;"
            "PRAGMA foreign_keys = ON;"
        );
        if (!isDataSourceVersionSupported()) {
            db.close();
            storageAvailable = false;
        }
    } else {
        QMessageBox::critical(QApplication::activeWindow(), tr("Error"), tr("Unable to read the Optimus database."));
    }

    return storageAvailable;
}

QString FeatureDataSource::getInputFileFilter()
{
    return QObject::tr("Optimus database (*.db)");
}

} // namespace ov
