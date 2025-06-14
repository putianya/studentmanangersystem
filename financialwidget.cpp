#include "financialwidget.h"
#include "ui_financialwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QChartView >
#include <QLabel>
#include <QComboBox>
#include <QDateEdit>
#include <Qdate>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>
#include <QSqlQuery>
#include <QHeaderView>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QSqlError>
#include <QPieSeries>
#include <QPieSlice>
#include <QLineSeries>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QMessageBox>
FinancialWidget::FinancialWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::FinancialWidget)
{
    ui->setupUi(this);
    setupUI();
    populateStudentComboBox();
}

FinancialWidget::~FinancialWidget()
{
    delete ui;
}

void FinancialWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *topLayout = new QHBoxLayout();
    QHBoxLayout *middleLayout = new QHBoxLayout();

    chartView = new QChartView();
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(middleLayout, 60); // 占60%高度
    mainLayout->addWidget(chartView, 40);    // 占40%高度

    // =============== 顶部筛选条件与按钮布局 ===============
    topLayout->addWidget(new QLabel("学生姓名:", this));
    studentComboBox = new QComboBox(this);
    topLayout->addWidget(studentComboBox);

    topLayout->addWidget(new QLabel("起始日期:", this));
    startDateEdit = new QDateEdit(QDate::currentDate().addMonths(-1));
    startDateEdit->setCalendarPopup(true);
    topLayout->addWidget(startDateEdit);

    topLayout->addWidget(new QLabel("结束日期:", this));
    endDateEdit = new QDateEdit(QDate::currentDate());
    endDateEdit->setCalendarPopup(true);
    topLayout->addWidget(endDateEdit);

    addButton = new QPushButton("添加");
    deleteButton = new QPushButton("删除");
    editButton = new QPushButton("修改");
    topLayout->addWidget(addButton);
    topLayout->addWidget(deleteButton);
    topLayout->addWidget(editButton);
    topLayout->addStretch();

    // =============== 主内容布局 ===============
    tableWidget = new QTableWidget();
    tableWidget->setFixedWidth(550);
    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tableWidget->setAlternatingRowColors(true);
    QStringList header = QStringList() << "ID" << "学生名字" << "缴费日期" << "金额" <<
                         "支付类型" << "备注";
    tableWidget->setColumnCount(header.count());
    tableWidget->setHorizontalHeaderLabels(header);
    tableWidget->setColumnHidden(0, true);
    middleLayout->addWidget(tableWidget);
    pieChartView = new QChartView();
    middleLayout->addWidget(pieChartView);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(200); // 最小高度保障
    // 连接
    connect(addButton,    &QPushButton::clicked, this,
            &FinancialWidget::addRecord);

    connect(deleteButton, &QPushButton::clicked, this,
            &FinancialWidget::deleteRecord);
    connect(editButton,   &QPushButton::clicked, this,
            &FinancialWidget::editRecord);
    connect(studentComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FinancialWidget::loadFinancialRecords);
    connect(startDateEdit, &QDateEdit::dateChanged, this,
            &FinancialWidget::loadFinancialRecords);
    connect(endDateEdit,   &QDateEdit::dateChanged, this,
            &FinancialWidget::loadFinancialRecords);
}

void FinancialWidget::loadFinancialRecords()
{
    tableWidget->setRowCount(0);

    QString studentId = studentComboBox->currentData().toString();
    QDate   startDate = startDateEdit->date();
    QDate   endDate = endDateEdit->date();

    QString queryStr = QString(
        "SELECT fr.id, s.name, fr.payment_date, fr.amount, fr.payment_type, fr.notes "
        "FROM financialRecords fr "
        "JOIN studentInfo s ON fr.student_id = s.id "
        "WHERE fr.payment_date BETWEEN '%1' AND '%2' %3"
        ).arg(startDate.toString("yyyy-MM-dd"),
              endDate.toString("yyyy-MM-dd"),
              (studentId !=
               "-1") ? QString("AND fr.student_id = '%1'").arg(studentId) : "");

    QSqlQuery query(queryStr);

    while (query.next()) {
        int row = tableWidget->rowCount();
        tableWidget->insertRow(row);

        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(
                                                              col).toString());
            item->setTextAlignment(Qt::AlignCenter);
            tableWidget->setItem(row, col, item);
        }
    }

    tableWidget->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);

    updateChart();    // 更新下方折线图
    updatePieChart(); // 更新右侧饼图
}

void FinancialWidget::populateStudentComboBox()
{
    studentComboBox->clear();
    studentComboBox->addItem("所有学生", QVariant("-1")); // "-1" 表示所有学生

    QSqlQuery query("SELECT id, name FROM studentInfo");

    while (query.next()) {
        QString id = query.value(0).toString(); // id 是字符串类型
        QString name = query.value(1).toString();
        studentComboBox->addItem(name, QVariant(id));
    }
}

void FinancialWidget::addRecord()
{
    QDialog dialog(this);

    dialog.setWindowTitle("添加缴费记录");
    QFormLayout form(&dialog);

    // 学生名称下拉菜单
    QComboBox *studentNameComboBox = new QComboBox(&dialog);
    QSqlQuery  query("SELECT id, name FROM studentInfo");

    while (query.next()) {
        QString id = query.value(0).toString();
        QString name = query.value(1).toString();
        studentNameComboBox->addItem(name, QVariant(id)); // 将学生ID与名称关联
    }
    QDateEdit *paymentDateEdit = new QDateEdit(&dialog);
    paymentDateEdit->setDate(QDate::currentDate());       // 设置默认值为当前日期
    paymentDateEdit->setCalendarPopup(true);              // 允许弹出日历选择器

    QLineEdit *amountEdit = new QLineEdit(&dialog);
    QLineEdit *feeTypeEdit = new QLineEdit(&dialog);
    QLineEdit *remarkEdit = new QLineEdit(&dialog);

    form.addRow("学生名称:", studentNameComboBox);
    form.addRow("缴费日期:",     paymentDateEdit); // 修改为 QDateEdit
    form.addRow("金额:",            amountEdit);
    form.addRow("支付类型:",         feeTypeEdit);
    form.addRow("备注:",            remarkEdit);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal,
                               &dialog);
    buttonBox.button(QDialogButtonBox::Ok)->setText("确定");
    buttonBox.button(QDialogButtonBox::Cancel)->setText("取消");
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox,
                     &QDialogButtonBox::accepted,
                     &dialog,
                     &QDialog::accept);
    QObject::connect(&buttonBox,
                     &QDialogButtonBox::rejected,
                     &dialog,
                     &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString studentId = studentNameComboBox->currentData().toString();
        QString paymentDate = paymentDateEdit->date().toString("yyyy-MM-dd");
        double  amount = amountEdit->text().toDouble();
        QString feeType = feeTypeEdit->text();
        QString remark = remarkEdit->text();

        // 准备SQL查询
        QSqlQuery query;
        query.prepare("INSERT INTO financialRecords (student_id, payment_date, amount, payment_type, notes) "
                      "VALUES (:student_id, :payment_date, :amount, :payment_type, :notes)");
        query.bindValue(":student_id",     studentId); // 绑定学生ID
        query.bindValue(":payment_date", paymentDate);
        query.bindValue(":amount",            amount);
        query.bindValue(":payment_type",     feeType);
        query.bindValue(":notes",             remark);

        // 执行SQL查询
        if (query.exec()) {
            qDebug() << "记录添加成功！";
            loadFinancialRecords(); // 刷新表格
        }
        else qDebug() << "添加记录失败：" << query.lastError().text();
    }
}

void FinancialWidget::updatePieChart()
{
    QString studentId = studentComboBox->currentData().toString();
    QDate   startDate = startDateEdit->date();
    QDate   endDate = endDateEdit->date();

    QString queryStr = QString(
        "SELECT payment_type, SUM(amount) "
        "FROM financialRecords "
        "WHERE payment_date BETWEEN '%1' AND '%2' %3 "
        "GROUP BY payment_type")
                       .arg(startDate.toString("yyyy-MM-dd"))
                       .arg(endDate.toString("yyyy-MM-dd"))
                       .arg(studentId != "-1" ? QString(
                                "AND student_id = '%1'").arg(studentId) : "");

    // 执行SQL查询获取统计数据
    QSqlQuery query(queryStr);

    // 创建饼图数据系列
    QPieSeries *series = new QPieSeries();

    // 遍历查询结果，为每个数据项创建饼图切片
    while (query.next()) {
        // 获取分类名称和对应数值
        QString type = query.value(0).toString();  // 第一列：分类名称
        qreal   value = query.value(1).toDouble(); // 第二列：数值

        // 仅添加数值大于0的数据（排除无效数据）
        if (value > 0) {
            // 构建图例标签，包含分类名称和数值
            QString legendLabel = QString("%1 %2元").arg(type).arg(value);

            // 创建饼图切片，设置标签和数值
            QPieSlice *slice = new QPieSlice(legendLabel, value);

            // 隐藏饼图上的标签（避免拥挤），仅在图例中显示
            slice->setLabelVisible(false);

            // 将切片添加到数据系列
            series->append(slice);
        }
    }
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("支付类型分布");

    // 图例设置
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setBackgroundVisible(true);
    chart->legend()->setBrush(QBrush(Qt::white));
    chart->legend()->setLabelColor(Qt::black);
    chart->legend()->setContentsMargins(10, 10, 10, 10);

    // 饼图尺寸
    series->setPieSize(0.75);

    if (pieChartView->chart()) {
        delete pieChartView->chart();
    }
    pieChartView->setChart(chart);

    // 强制重绘
    pieChartView->repaint();
}

void FinancialWidget::updateChart()
{ // ================== 1. 获取并验证日期范围 ==================
    QDate startDate = startDateEdit->date();
    QDate endDate = endDateEdit->date();

    if (startDate > endDate) {
        std::swap(startDate, endDate);
        startDateEdit->setDate(startDate);
        endDateEdit->setDate(endDate);
    }

    // ================== 2. 构建安全SQL查询 ==================
    QString studentId = studentComboBox->currentData().toString();
    QString queryStr = QString("SELECT DATE(payment_date) AS day, SUM(amount) AS total "
                               "FROM financialRecords "
                               "WHERE payment_date BETWEEN :startDate AND :endDate "
                               "%1 GROUP BY day ORDER BY day"
                               ).arg(
        studentId != "-1" ? "AND student_id = :studentId" : "");
    QSqlQuery query;
    query.prepare(queryStr);
    query.bindValue(":startDate", startDate.toString("yyyy-MM-dd"));
    query.bindValue(":endDate",     endDate.toString("yyyy-MM-dd"));

    if (studentId != "-1") query.bindValue(":studentId", studentId);

    if (!query.exec()) qCritical() << "[SQL错误]" << query.lastError().text();

    // ================== 3. 处理查询数据 ==================
    QMap<QDate, qreal> dayData;
    qreal maxAmount = 0;

    while (query.next()) {
        QDate day = QDate::fromString(query.value(0).toString(), "yyyy-MM-dd");

        if (!day.isValid()) continue;
        qreal amount = query.value(1).toDouble();
        dayData[day] = amount;

        if (amount > maxAmount) maxAmount = amount;
    }

    // ================== 4. 创建图表系列 ==================
    QLineSeries *series = new QLineSeries();
    series->setName("销售额");
    QPen pen(Qt::blue);
    series->setPen(pen);
    QDate currentDate = startDate;

    while (currentDate <= endDate) {
        qreal value = dayData.value(currentDate, 0.0);
        series->append(currentDate.startOfDay().toMSecsSinceEpoch(), value);
        currentDate = currentDate.addDays(1);
    }

    // ================== 5. 配置坐标轴 ==================
    QChart *chart = new QChart();
    chart->addSeries(series);
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("yyyy-MM-dd");
    axisX->setTitleText("日期");
    axisX->setRange(startDate.startOfDay(), endDate.startOfDay());
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("金额 (元)");
    axisY->setLabelFormat("%.0f");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // ================== 6. 应用图表 ==================
    if (chartView->chart()) delete chartView->chart();
    chartView->setChart(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chart->legend()->setVisible(false);
}

void FinancialWidget::editRecord()
{
    int currentRow = tableWidget->currentRow();

    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请选择要修改的记录！");
        return;
    }

    // 获取当前行的数据
    QString id = tableWidget->item(currentRow, 0)->text();          // ID 是字符串类型
    QString studentName = tableWidget->item(currentRow, 1)->text(); // 学生名称
    QString paymentDate = tableWidget->item(currentRow, 2)->text();
    QString amount = tableWidget->item(currentRow, 3)->text();
    QString feeType = tableWidget->item(currentRow, 4)->text();
    QString remark = tableWidget->item(currentRow, 5)->text();
    QDialog dialog(this);
    dialog.setWindowTitle("修改缴费记录");
    QFormLayout form(&dialog);

    // 学生名称下拉菜单
    QComboBox *studentNameComboBox = new QComboBox(&dialog);
    QSqlQuery  query("SELECT id, name FROM studentInfo");

    while (query.next()) {
        QString id = query.value(0).toString(); // id 是字符串类型
        QString name = query.value(1).toString();
        studentNameComboBox->addItem(name, QVariant(id));
    }
    studentNameComboBox->setCurrentText(studentName); // 设置当前学生名称
    QLineEdit *paymentDateEdit = new QLineEdit(paymentDate, &dialog);
    QLineEdit *amountEdit = new QLineEdit(amount, &dialog);
    QLineEdit *feeTypeEdit = new QLineEdit(feeType, &dialog);
    QLineEdit *remarkEdit = new QLineEdit(remark, &dialog);
    form.addRow("学生名称:", studentNameComboBox);
    form.addRow("缴费日期:",     paymentDateEdit);
    form.addRow("金额:",            amountEdit);
    form.addRow("支付类型:",         feeTypeEdit);
    form.addRow("备注:",            remarkEdit);
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal,
                               &dialog);
    buttonBox.button(QDialogButtonBox::Ok)->setText("确定");
    buttonBox.button(QDialogButtonBox::Cancel)->setText("取消");
    form.addRow(&buttonBox);
    QObject::connect(&buttonBox,
                     &QDialogButtonBox::accepted,
                     &dialog,
                     &QDialog::accept);
    QObject::connect(&buttonBox,
                     &QDialogButtonBox::rejected,
                     &dialog,
                     &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QString studentId = studentNameComboBox->currentData().toString(); // studentId
                                                                           // 是字符串类型
        QString paymentDate = paymentDateEdit->text();
        double  amount = amountEdit->text().toDouble();
        QString feeType = feeTypeEdit->text();
        QString remark = remarkEdit->text();

        // 准备 SQL 查询
        QSqlQuery query;
        query.prepare("UPDATE financialRecords SET student_id = :student_id, payment_date = :payment_date, "
                      "amount = :amount, payment_type = :payment_type, notes = :notes WHERE id = :id");
        query.bindValue(":student_id",     studentId); // studentId 是字符串类型
        query.bindValue(":payment_date", paymentDate);
        query.bindValue(":amount",            amount);
        query.bindValue(":payment_type",     feeType);
        query.bindValue(":notes",             remark);
        query.bindValue(":id",                    id);

        // 执行 SQL 查询
        if (query.exec()) {
            qDebug() << "记录修改成功！";
            loadFinancialRecords(); // 刷新表格
        }
        else qDebug() << "修改记录失败：" << query.lastError().text();
    }
}

void FinancialWidget::deleteRecord()
{
    int currentRow = tableWidget->currentRow();

    if (currentRow < 0) {
        QMessageBox::warning(this, "警告", "请选择要删除的记录！");
        return;
    }

    // 获取 ID 列的值
    int id = tableWidget->item(currentRow, 0)->text().toInt(); // ID 列是第一列

    // 确认删除操作
    QMessageBox confirmBox(this);
    confirmBox.setWindowTitle("确认删除");
    confirmBox.setText("确定要删除该记录吗？");

    // 设置按钮为中文
    QPushButton *yesButton = confirmBox.addButton("确定", QMessageBox::YesRole);
    QPushButton *noButton = confirmBox.addButton("取消", QMessageBox::NoRole);

    // 设置默认按钮
    confirmBox.setDefaultButton(noButton);

    // 显示对话框并等待用户选择
    confirmBox.exec();

    if (confirmBox.clickedButton() == yesButton) {
        // 用户点击了“确定”
        QSqlQuery query;
        query.prepare("DELETE FROM financialRecords WHERE id = :id");
        query.bindValue(":id", id);

        if (query.exec()) {
            qDebug() << "记录删除成功！";
            loadFinancialRecords(); // 刷新表格
        }
        else {
            QMessageBox::warning(this, "错误", "删除记录失败！");
        }
    }
}
