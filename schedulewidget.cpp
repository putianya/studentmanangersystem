#include "schedulewidget.h"
#include "ui_schedulewidget.h"
#include <QDate>
#include <QTableWidget>
#include <QComboBox>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QMessageBox>
#include <QFormLayout>
#include <QTimeEdit>
#include <QSqlError>
int customWeekNumber(const QDate& date) {
    QDate startOfYear(date.year(), 1, 1);
    int   dayOfWeek = startOfYear.dayOfWeek();
    int   days = startOfYear.daysTo(date);
    int   week = (days + dayOfWeek - 1) / 7 + 1;

    return week;
}

ScheduleWidget::ScheduleWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ScheduleWidget)
{
    ui->setupUi(this);
    setupUI();

    int currentYear = QDate::currentDate().year();
    int currentWeek = customWeekNumber(QDate::currentDate());

    yearComboBox->setCurrentText(QString::number(currentYear));
    weekComboBox->setCurrentText(QString("第 %1 周").arg(currentWeek));

    loadSchedule();
}

// 定义 setupTable 函数，用于设置表格内容和表头
void ScheduleWidget::setupTable() {
    // 定义一周七天的字符串列表
    QStringList days = { "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日" };

    // 定义一天内的时间段列表（假设times为QStringList类型，此处可能需检查变量定义）
    times = { "上午1", "上午2", "下午1", "下午2", "晚上1", "晚上2" };

    // 设置表格行数为7（一周七天）
    tableWidget->setRowCount(days.count());

    // 设置表格列数为6（六个时间段）
    tableWidget->setColumnCount(times.count());

    // 获取年份组合框当前选择的年份（整数）
    int year = yearComboBox->currentData().toInt();

    // 获取周数组合框当前选择的周数（整数）
    int week = weekComboBox->currentData().toInt();

    // 调用getWeekRange获取该周的起始和结束日期
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);

    // 提取起始日期
    QDate startDate = weekRange.first;

    // 定义垂直表头（行表头）的字符串列表
    QStringList verticalHeaders;

    // 遍历一周七天，生成每行的表头内容（星期几 + 日期）
    for (int i = 0; i < days.count(); ++i) {
        // 计算当前行对应的日期（起始日期 + i天，i=0到6对应一周七天）
        QDate currentDate = startDate.addDays(i);

        // 格式化表头内容："星期X\nMM/dd"，添加到垂直表头列表
        verticalHeaders.append(QString("%1\n%2").arg(days[i]).arg(currentDate.
                                                                  toString(
                                                                      "MM/dd")));
    }

    // 设置表格的垂直表头（行表头）
    tableWidget->setVerticalHeaderLabels(verticalHeaders);

    // 设置表格的水平表头（列表头，时间段）
    tableWidget->setHorizontalHeaderLabels(times);

    // 水平表头列宽自动拉伸以填满表格宽度
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 垂直表头行高自动拉伸以填满表格高度
    tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // 设置表格的编辑触发方式：双击或按下编辑键时可编辑
    tableWidget->setEditTriggers(
        QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
}

void ScheduleWidget::loadSchedule()
{
    // 防止加载数据时触发itemChanged信号
    tableWidget->blockSignals(true);

    // 清空表格内容但保留表头
    tableWidget->clearContents();

    // 获取当前选择的年份和周数
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();

    // 计算当前周的日期范围（周一到周日）
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate startDate = weekRange.first;
    QDate endDate = weekRange.second;

    // 更新日期范围显示
    dateRangeLabel->setText(startDate.toString(
                                "yyyy-MM-dd") + "到" +
                            endDate.toString("yyyy-MM-dd"));

    // 初始化课程数据结构，7天×多个时间段
    QVector<QVector<QString> > courses(7, QVector<QString>(times.count(), ""));

    // 准备SQL查询，获取指定日期范围内的所有课程
    QSqlQuery query;
    query.prepare(
        "SELECT date, time, course_name FROM schedule WHERE date BETWEEN ? AND ?");
    query.addBindValue(startDate.toString("yyyy-MM-dd"));
    query.addBindValue(endDate.toString("yyyy-MM-dd"));

    // 执行SQL查询
    if (query.exec()) {
        // 遍历查询结果
        while (query.next()) {
            // 获取课程日期、时间和名称
            QDate date = QDate::fromString(query.value(0).toString(),
                                           "yyyy-MM-dd");
            QString time = query.value(1).toString();

            // 计算课程在表格中的位置
            int dayIndex = startDate.daysTo(date);
            int timeIndex = times.indexOf(time);

            // 检查索引有效性并存储课程数据
            if ((dayIndex >= 0) && (dayIndex < 7) && (timeIndex != -1)) {
                courses[dayIndex][timeIndex] = query.value(2).toString();
            }
        }
    }

    // 填充表格数据
    for (int day = 0; day < 7; ++day) {
        for (int time = 0; time < times.count(); ++time) {
            // 创建表格项并设置居中对齐
            QTableWidgetItem *item = new QTableWidgetItem(courses[day][time]);
            item->setTextAlignment(Qt::AlignCenter);
            tableWidget->setItem(day, time, item);
        }
    }

    // 恢复信号处理
    tableWidget->blockSignals(false);
}

void ScheduleWidget::addCourse() {
    // 获取当前选中的表格单元格位置
    int dayIndex = tableWidget->currentRow();
    int timeIndex = tableWidget->currentColumn();

    // 验证是否选中了有效单元格
    if ((dayIndex == -1) || (timeIndex == -1)) {
        QMessageBox::warning(this, "错误", "请先选择一个时间段！");
        return;
    }

    // 检查该时间段是否已被占用
    if (!tableWidget->item(dayIndex, timeIndex)->text().isEmpty()) {
        QMessageBox::warning(this, "错误", "该时间段已被占用！");
        return;
    }

    // 创建添加课程的对话框
    QDialog dialog(this);
    dialog.setWindowTitle("添加课程");
    QFormLayout layout(&dialog);

    // 创建学生姓名选择下拉框
    QComboBox nameCombo;
    QSqlQuery nameQuery("SELECT name FROM studentInfo");

    while (nameQuery.next()) nameCombo.addItem(nameQuery.value(0).toString());

    // 定义时间预设映射表（列索引 -> 默认时间）
    QMap<int, QTime> timePresets = {
        { 0, QTime(9,  0)         },  { 1, QTime(11, 0) }, { 2, QTime(14, 0) },
        { 3, QTime(16, 0)         },  { 4, QTime(19, 0) }, { 5, QTime(21, 0) }
    };

    // 创建时间选择控件
    QTimeEdit timeEdit;
    timeEdit.setDisplayFormat("HH:mm");
    timeEdit.setTime(timePresets.value(timeIndex)); // 设置为对应列的默认时间

    // 添加控件到对话框布局
    layout.addRow("学生姓名:", &nameCombo);
    layout.addRow("课程时间:", &timeEdit);

    // 添加确定/取消按钮
    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons.button(QDialogButtonBox::Ok)->setText("确定");
    buttons.button(QDialogButtonBox::Cancel)->setText("取消");
    layout.addRow(&buttons);

    // 连接按钮信号到对话框槽函数
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 显示对话框并等待用户操作
    if (dialog.exec() != QDialog::Accepted) return;

    // 生成课程名称字符串（格式："姓名,HH:mm"）
    QString courseName = QString("%1,%2").arg(nameCombo.currentText())
                         .arg(timeEdit.time().toString("HH:mm"));

    // 获取当前选择的年份和周数
    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);

    // 计算当前选择单元格对应的日期（周起始日期 + 天偏移）
    QDate currentDate = weekRange.first.addDays(dayIndex);

    // 获取原始时间段标识（如"上午1"、"下午2"等）
    QString timeSlot = times[timeIndex];

    // 准备SQL插入语句
    QSqlQuery query;
    query.prepare(
        "INSERT INTO schedule (date, time, course_name) VALUES (?, ?, ?)");
    query.addBindValue(currentDate.toString("yyyy-MM-dd"));
    query.addBindValue(timeSlot);   // 存储原始时间段标识
    query.addBindValue(courseName); // 存储"姓名,HH:mm"格式的字符串

    // 执行SQL插入
    if (!query.exec()) QMessageBox::critical(this,
                                             "错误",
                                             "添加失败：" + query.lastError().text());
    else loadSchedule();  // 插入成功后刷新课程表显示
}

// 定义getWeekRange函数，计算指定年份和周数的起始和结束日期（周一到周日）
QPair<QDate, QDate>ScheduleWidget::getWeekRange(int year, int week) {
    // 初始化起始日期为该年1月1日
    QDate startDate(year, 1, 1);

    // 计算1月1日距离星期一的天数差（若为星期一，daysToSubtract=0；否则为正数，需调整到周一）
    int daysToSubtract = startDate.dayOfWeek() - Qt::Monday;

    // 如果1月1日不是周一，调整到该年第一个周一（例如1月1日是周二，daysToSubtract=1，减去1天到周一）
    if (daysToSubtract > 0) startDate = startDate.addDays(-daysToSubtract);

    // 计算第week周的起始日期：第1周从第一个周一开始，每周加7天
    QDate weekStart = startDate.addDays((week - 1) * 7);

    // 计算第week周的结束日期：起始日期 + 6天（周一到周日共7天，索引0到6）
    QDate weekEnd = weekStart.addDays(6);

    // 返回该周的起始和结束日期对
    return qMakePair(weekStart, weekEnd);
}

ScheduleWidget::~ScheduleWidget()
{
    delete ui;
}

void ScheduleWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *dateLayout = new QHBoxLayout();

    yearComboBox = new QComboBox(this);
    weekComboBox = new QComboBox(this);

    int currentYear = QDate::currentDate().year();

    for (int year = 2020; year <= currentYear + 5; ++year) yearComboBox->addItem(QString::number(
                                                                                     year),
                                                                                 year);

    for (int week = 1; week <= 52; ++week) weekComboBox->addItem(QString(
                                                                     "第 %1 周").arg(
                                                                     week),
                                                                 week);

    dateRangeLabel = new QLabel(this);

    // 添加周导航按钮
    QPushButton *prevWeekBtn = new QPushButton("上一周", this);
    QPushButton *nextWeekBtn = new QPushButton("下一周", this);
    prevWeekBtn->setFixedWidth(200);
    nextWeekBtn->setFixedWidth(200);

    dateLayout->addWidget(new QLabel("年份：", this));
    dateLayout->addWidget(yearComboBox);
    dateLayout->addWidget(new QLabel("周数：", this));
    dateLayout->addWidget(weekComboBox);
    dateLayout->addWidget(dateRangeLabel);
    dateLayout->addStretch();

    tableWidget = new QTableWidget(this);
    tableWidget->setAlternatingRowColors(true);
    tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    setupTable();

    addButton = new QPushButton("添加课程", this);
    deleteButton = new QPushButton("删除课程", this);
    addButton->setFixedWidth(200);
    deleteButton->setFixedWidth(200);

    connect(yearComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScheduleWidget::loadSchedule);
    connect(weekComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScheduleWidget::loadSchedule);
    connect(addButton,    &QPushButton::clicked,      this,
            &ScheduleWidget::addCourse);

    connect(deleteButton, &QPushButton::clicked,      this,
            &ScheduleWidget::deleteCourse);

    connect(prevWeekBtn,  &QPushButton::clicked,      this,
            &ScheduleWidget::showPreviousWeek);
    connect(nextWeekBtn,  &QPushButton::clicked,      this,
            &ScheduleWidget::showNextWeek);
    connect(tableWidget,  &QTableWidget::itemChanged, this,
            &ScheduleWidget::handleItemChanged);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(prevWeekBtn);
    buttonLayout->addWidget(nextWeekBtn);
    buttonLayout->addWidget(addButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(dateLayout);
    mainLayout->addWidget(tableWidget);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void ScheduleWidget::handleItemChanged(QTableWidgetItem *item)
{
    int day = item->row();
    int timeSlot = item->column();
    QString newCourse = item->text().trimmed();

    int year = yearComboBox->currentData().toInt();
    int week = weekComboBox->currentData().toInt();
    QPair<QDate, QDate> weekRange = getWeekRange(year, week);
    QDate   date = weekRange.first.addDays(day);
    QString time = times[timeSlot];

    QSqlQuery query;

    if (newCourse.isEmpty()) {
        // 删除课程
        query.prepare("DELETE FROM schedule WHERE date = ? AND time = ?");
        query.addBindValue(date.toString("yyyy-MM-dd"));
        query.addBindValue(time);
    }
    else {
        // 使用REPLACE语句更新或插入
        query.prepare(
            "INSERT OR REPLACE INTO schedule (date, time, course_name) VALUES (?, ?, ?)");
        query.addBindValue(date.toString("yyyy-MM-dd"));
        query.addBindValue(time);
        query.addBindValue(newCourse);
    }

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "操作失败：" + query.lastError().text());
        loadSchedule(); // 恢复数据
    }
}

void ScheduleWidget::deleteCourse()
{
    // 创建确认对话框，防止误删除
    QMessageBox confirmBox(this);

    confirmBox.setWindowTitle("确认删除");
    confirmBox.setText("确定要删除该记录吗？");

    // 将按钮文本设置为中文，提升用户体验
    QPushButton *yesButton = confirmBox.addButton("确定", QMessageBox::YesRole);
    QPushButton *noButton = confirmBox.addButton("取消", QMessageBox::NoRole);

    // 设置取消为默认按钮，减少误操作
    confirmBox.setDefaultButton(noButton);

    // 显示对话框并等待用户选择
    confirmBox.exec();

    // 仅当用户点击"确定"按钮时执行删除逻辑
    if (confirmBox.clickedButton() == yesButton) {
        // 获取当前选中的表格行和列索引
        int dayIndex = tableWidget->currentRow();
        int timeIndex = tableWidget->currentColumn();

        // 验证用户是否选择了有效单元格
        if ((dayIndex == -1) || (timeIndex == -1)) {
            QMessageBox::warning(this, "错误", "请先选择一个时间段！");
            return;
        }

        // 获取选中单元格的内容并检查是否有课程
        QTableWidgetItem *item = tableWidget->item(dayIndex, timeIndex);

        if (!item || item->text().isEmpty()) {
            QMessageBox::warning(this, "错误", "该时间段没有课程！");
            return;
        }

        // 获取当前选择的年份和周数
        int year = yearComboBox->currentData().toInt();
        int week = weekComboBox->currentData().toInt();

        // 计算当前周的日期范围（周一到周日）
        QPair<QDate, QDate> weekRange = getWeekRange(year, week);

        // 根据表格行索引计算具体日期（dayIndex=0表示周一）
        QDate currentDate = weekRange.first.addDays(dayIndex);

        // 获取时间段标识（如"上午1"、"下午2"等）
        QString time = times[timeIndex];

        // 准备SQL删除语句，通过日期和时间段唯一确定一条记录
        QSqlQuery query;
        query.prepare("DELETE FROM schedule WHERE date = ? AND time = ?");
        query.addBindValue(currentDate.toString("yyyy-MM-dd"));
        query.addBindValue(time);

        // 执行SQL删除操作
        if (!query.exec()) {
            // 删除失败时显示错误信息
            QMessageBox::critical(this, "错误", "删除失败：" + query.lastError().text());
        }
        else {
            // 删除成功后刷新课程表显示
            loadSchedule();
        }
    }
}

void ScheduleWidget::showPreviousWeek()
{
    int currentWeek = weekComboBox->currentIndex();
    int currentYear = yearComboBox->currentIndex();

    if (currentWeek > 0) {
        weekComboBox->setCurrentIndex(currentWeek - 1);
    }
    else {
        if (yearComboBox->currentIndex() > 0) {
            yearComboBox->setCurrentIndex(currentYear - 1);

            // 跳转到上一年最后一周（第52周）
            weekComboBox->setCurrentIndex(51);
        }
    }
}

void ScheduleWidget::showNextWeek()
{
    int currentWeek = weekComboBox->currentIndex();
    int currentYear = yearComboBox->currentIndex();

    if (currentWeek < 51) {
        weekComboBox->setCurrentIndex(currentWeek + 1);
    }
    else {
        if (yearComboBox->currentIndex() < yearComboBox->count() - 1) {
            yearComboBox->setCurrentIndex(currentYear + 1);
            weekComboBox->setCurrentIndex(0);
        }
    }
}
