#ifndef STUDENTINFOWIDGET_H
#define STUDENTINFOWIDGET_H

#include <QWidget>
#include <QByteArray>

namespace Ui {
class StudentInfoWidget;
}

class QGroupBox;
class QTableWidgetItem;

class StudentInfoWidget : public QWidget {
    Q_OBJECT

public:

    explicit StudentInfoWidget(QWidget *parent = nullptr);
    ~StudentInfoWidget();

private slots:

    void on_btnAdd_clicked();

    void on_btnDeleteItem_clicked();

    void on_btnDeleteLine_clicked();

    void handleItemChanged(QTableWidgetItem *item);

private:

    void       refreshTable();
    QGroupBox* createFormGroup();
    QGroupBox* createPhotoGroup();
    void       handleDialogAccepted(QGroupBox *formGroup,
                                    QGroupBox *photoGroup);

    QByteArray photoData;
    Ui::StudentInfoWidget *ui;
};

#endif // STUDENTINFOWIDGET_H
