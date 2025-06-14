#ifndef SCHEDULEWIDGET_H
#define SCHEDULEWIDGET_H

#include <QWidget>

namespace Ui {
class ScheduleWidget;
}

class QTableWidget;
class QComboBox;
class QLabel;
class QPushButton;
class QTableWidgetItem;
class ScheduleWidget : public QWidget {
    Q_OBJECT

public:

    explicit ScheduleWidget(QWidget *parent = nullptr);
    ~ScheduleWidget();

private:

    void               setupUI();
    void               setupTable();
    void               loadSchedule();
    void               addCourse();
    void               handleItemChanged(QTableWidgetItem *item);
    void               deleteCourse();
    void               showPreviousWeek();
    void               showNextWeek();
    QPair<QDate, QDate>getWeekRange(int year,
                                    int week);
    QTableWidget *tableWidget;
    QComboBox *yearComboBox;
    QComboBox *weekComboBox;
    QLabel *dateRangeLabel; // 显示日期范围的标签
    QPushButton *addButton;
    QPushButton *deleteButton;
    QPushButton *prevWeekBtn;
    QPushButton *nextWeekBtn;

    // 课程数据存储结构：键为 (year, week)，值为课程表数据
    QMap<QPair<int, int>, QVector<QVector<QString> > >scheduleData;
    QStringList times; //上午1，上午2...


    Ui::ScheduleWidget *ui;
};

#endif // SCHEDULEWIDGET_H
