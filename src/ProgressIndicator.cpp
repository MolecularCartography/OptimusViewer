#include "ui_ProgressIndicator.h"

#include "ProgressIndicator.h"

namespace qm {

ProgressIndicator::ProgressIndicator(QWidget *parent)
    : QDialog(parent), ui(new Ui::ProgressIndicatorUi)
{
    ui->setupUi(this);
    hide();
}

void ProgressIndicator::started()
{
    ui->progressBar->reset();
    show();
}

void ProgressIndicator::progress(int percents)
{
    ui->progressBar->setValue(percents);
}

void ProgressIndicator::finished()
{
    hide();
}

} // namespace qm
