#ifndef FEATURETABLEVISIBILITYDIALOG_H
#define FEATURETABLEVISIBILITYDIALOG_H

#include <QBitArray>
#include <QDialog>

class QStandardItemModel;

namespace Ui {

class FeatureTableVisibilityDialogUi;

}

namespace qm {

class FeatureTableVisibilityDialog : public QDialog
{
    Q_OBJECT

public:
    FeatureTableVisibilityDialog(const QList<QPair<QString, bool> > &headers, QWidget *parent);
    ~FeatureTableVisibilityDialog();

    QBitArray getHeaderVisibility() const;

public slots:
    void accept();

private slots:
    void selectAllClicked();
    void clearClicked();

private:
    void initHeaderList(const QList<QPair<QString, bool> > &headers);
    void connectGui();
    void setListItemsCheckState(Qt::CheckState state);

    QStandardItemModel * getHeaderListModel() const;

    Ui::FeatureTableVisibilityDialogUi *ui;
};

} // namespace qm

#endif // FEATURETABLEVISIBILITYDIALOG_H
