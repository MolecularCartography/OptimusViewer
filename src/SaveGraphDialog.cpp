#include <QLabel>
#include <QGridLayout>
#include <QSlider>
#include <QDoubleSpinBox>

#include "SaveGraphDialog.h"

namespace ov {

SaveGraphDialog::SaveGraphDialog(QWidget *parent, const FormatId &selectedFormat)
    : QFileDialog(parent, tr("Save File")), selectedScale(1), selectedQuality(100)
{
    initFilters();
    setOption(QFileDialog::DontUseNativeDialog);
    setFileMode(QFileDialog::AnyFile);
    setAcceptMode(QFileDialog::AcceptSave);
    setViewMode(QFileDialog::Detail);
    setupUi();

    const QString activeFilter = formatByFilter.key(selectedFormat);
    selectNameFilter(activeFilter);
    filterSelected(activeFilter);
}

void SaveGraphDialog::initFilters()
{
    const QStringList formats = QStringList() << ExportFormats::lossyImageFormats << ExportFormats::losslessImageFormats
        << ExportFormats::vectorImageFormats << ExportFormats::dataFormats;
    QStringList filters;
    foreach (const QString &format, formats) {
        const char *filterStr = NULL;
        if (ExportFormats::lossyImageFormats.contains(format) || ExportFormats::losslessImageFormats.contains(format)) {
            filterStr = "%1 Image (*.%2)";
        } else if (ExportFormats::vectorImageFormats.contains(format)) {
            filterStr = "%1 Document (*.%2)";
        } else if (ExportFormats::dataFormats.contains(format)) {
            filterStr = "%1 File (*.%2)";
        } else {
            Q_ASSERT(false);
        }
        const QString filter = tr(filterStr).arg(format, format.toLower());
        filters.append(filter); // use list to keep the order of formats
        formatByFilter[filter] = format;
    }

    setNameFilters(filters);
}

void SaveGraphDialog::setupUi()
{
    QGridLayout *l = dynamic_cast<QGridLayout *>(layout());
    Q_ASSERT(NULL != l);

    int currentRow = l->rowCount();
    QLabel *scaleLabel = new QLabel(tr("Scale:"));
    l->addWidget(scaleLabel, currentRow, 0);
    scaleControllers.append(scaleLabel);

    QDoubleSpinBox *scaleSetter = new QDoubleSpinBox();
    scaleSetter->setSingleStep(0.1);
    scaleSetter->setMinimum(1);
    scaleSetter->setMaximum(10);
    scaleSetter->setValue(1);

    connect(scaleSetter, SIGNAL(valueChanged(double)), SLOT(scaleChanged(double)));

    l->addWidget(scaleSetter, currentRow, 1);
    scaleControllers.append(scaleSetter);

    ++currentRow;

    QLabel *qualityLabel = new QLabel(tr("Quality:"));
    l->addWidget(qualityLabel, currentRow, 0);
    qualityControllers.append(qualityLabel);

    QSlider *qualitySlider = new QSlider(Qt::Horizontal);
    qualitySlider->setMinimum(0);
    qualitySlider->setMaximum(100);
    qualitySlider->setValue(100);

    l->addWidget(qualitySlider, currentRow, 1);
    qualityControllers.append(qualitySlider);

    QSpinBox *qualitySpinBox = new QSpinBox();
    qualitySpinBox->setSingleStep(1);
    qualitySpinBox->setMinimum(qualitySlider->minimum());
    qualitySpinBox->setMaximum(qualitySlider->maximum());
    qualitySpinBox->setValue(qualitySlider->value());

    connect(qualitySlider, &QSlider::valueChanged, qualitySpinBox, &QSpinBox::setValue);
    connect(qualitySlider, &QSlider::valueChanged, this, &SaveGraphDialog::qualityChanged);
    connect(qualitySpinBox, SIGNAL(valueChanged(int)), qualitySlider, SLOT(setValue(int)));

    l->addWidget(qualitySpinBox, currentRow, 2);
    qualityControllers.append(qualitySpinBox);

    connect(this, &QFileDialog::filterSelected, this, &SaveGraphDialog::filterSelected);
}

int SaveGraphDialog::getQuality() const
{
    return selectedQuality;
}

double SaveGraphDialog::getScale() const
{
    return selectedScale;
}

QString SaveGraphDialog::getSelectedFormat() const
{
    return formatByFilter[selectedNameFilter()];
}

void SaveGraphDialog::qualityChanged(int value)
{
    selectedQuality = value;
}

void SaveGraphDialog::scaleChanged(double value)
{
    selectedScale = value;
}

namespace {

void setWidgetsVisibility(const QList<QWidget *> &widgets, bool visibility)
{
    std::for_each(widgets.constBegin(), widgets.constEnd(), [visibility](QWidget *w){ w->setVisible(visibility); });
}

}

void SaveGraphDialog::filterSelected(const QString &filter)
{
    const QString selectedFormat = formatByFilter[filter];
    if (ExportFormats::lossyImageFormats.contains(selectedFormat)) {
        setWidgetsVisibility(scaleControllers, true);
        setWidgetsVisibility(qualityControllers, true);
    } else if (ExportFormats::vectorImageFormats.contains(selectedFormat) || ExportFormats::losslessImageFormats.contains(selectedFormat)) {
        setWidgetsVisibility(scaleControllers, true);
        setWidgetsVisibility(qualityControllers, false);
    } else if (ExportFormats::dataFormats.contains(selectedFormat)) {
        setWidgetsVisibility(scaleControllers, false);
        setWidgetsVisibility(qualityControllers, false);
    } else {
        Q_ASSERT(false);
    }
}

} // namespace ov
