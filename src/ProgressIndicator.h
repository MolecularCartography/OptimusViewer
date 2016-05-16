#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H

#include <QDialog>

namespace Ui {

class ProgressIndicatorUi;

}

namespace ov {

class ProgressIndicator : public QDialog
{
    Q_OBJECT
public:
    ProgressIndicator(QWidget *parent = NULL);

public slots:
    void started();
    void progress(int percents);
    void finished();

private:
    Ui::ProgressIndicatorUi *ui;
};

} // namespace qm

#endif // PROGRESSINDICATOR_H
