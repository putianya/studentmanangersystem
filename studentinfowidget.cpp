#include "studentinfowidget.h"
#include "ui_studentinfowidget.h"
#include <qsqldatabase.h>
#include <QSqlQuery>
#include <QDialog>
#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QDateEdit>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QBuffer>
#include <QMessageBox>
#include <QSqlError>
#include "tabledelegates.h"

StudentInfoWidget::StudentInfoWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::StudentInfoWidget)
{
    ui->setupUi(this);
    ui->tableWidget->verticalHeader()->setDefaultSectionSize(100);
    ui->tableWidget->setAlternatingRowColors(true);

    // ---- 使用自定义委托的代码 ----
    // 创建性别列的委托实例
    ComboBoxDelegate *genderDelegate = new ComboBoxDelegate(this);

    // 设置性别选项（男/女）
    genderDelegate->setItems(QStringList() << "男" << "女");

    // 将委托应用到表格的第2列（性别列）
    ui->tableWidget->setItemDelegateForColumn(2, genderDelegate);

    // 创建进度列的委托实例
    ComboBoxDelegate *progressDelegate = new ComboBoxDelegate(this);

    // 设置进度选项（百分比）
    progressDelegate->setItems(
        QStringList() << "0%" << "20%" << "40%" << "60%" << "80%" << "100%");

    // 将委托应用到表格的第6列（进度列）
    ui->tableWidget->setItemDelegateForColumn(6, progressDelegate);

    // 日期列代理
    ui->tableWidget->setItemDelegateForColumn(3, new DateEditDelegate(this));
    ui->tableWidget->setItemDelegateForColumn(4, new DateEditDelegate(this));

    // 图片列代理
    ui->tableWidget->setItemDelegateForColumn(7, new ImageDelegate(this));

    // 连接item修改信号
    connect(ui->tableWidget,
            &QTableWidget::itemChanged,
            this,
            &StudentInfoWidget::handleItemChanged);

    refreshTable();
}

StudentInfoWidget::~StudentInfoWidget()
{
    delete ui;
}

void StudentInfoWidget::refreshTable()
{
    // 防止表格刷新时触发信号（如单元格点击信号）
    ui->tableWidget->blockSignals(true);

    // 清空表格所有行，但保留列标题
    ui->tableWidget->setRowCount(0);

    // 执行SQL查询获取所有学生记录
    QSqlQuery query("SELECT * FROM studentInfo");

    // 遍历查询结果的每一行
    while (query.next()) {
        // 获取当前表格的行数，作为新行的索引
        int row = ui->tableWidget->rowCount();

        // 在表格末尾插入新行
        ui->tableWidget->insertRow(row);

        // 遍历表格的每一列
        for (int col = 0; col < ui->tableWidget->columnCount(); ++col) {
            // 创建新的表格项
            QTableWidgetItem *item = new QTableWidgetItem();

            // 设置单元格内容居中显示
            item->setTextAlignment(Qt::AlignCenter);

            // 处理最后一列（照片列）
            if (col == ui->tableWidget->columnCount() - 1) {
                // 从查询结果获取二进制照片数据
                QByteArray photoData = query.value(col).toByteArray();

                if (!photoData.isEmpty()) {
                    // 创建QPixmap对象
                    QPixmap photo;

                    // 从二进制数据加载图片
                    photo.loadFromData(photoData);

                    // 设置为单元格的装饰数据（显示图片），并缩放至100x100像素
                    item->setData(Qt::DecorationRole,
                                  photo.scaled(100, 100,
                                               Qt::KeepAspectRatio));

                    // 保存原始二进制数据到用户角色，便于后续使用
                    item->setData(Qt::UserRole, photoData);
                }
            }

            // 处理普通文本列
            else {
                // 将数据库字段值转换为文本并设置为单元格内容
                item->setText(query.value(col).toString());
            }

            // 将创建的单元格项添加到表格的指定行列位置
            ui->tableWidget->setItem(row, col, item);
        }
    }

    // 恢复表格信号触发
    ui->tableWidget->blockSignals(false);
}

QGroupBox * StudentInfoWidget::createFormGroup()
{
    QGroupBox   *formGroup = new QGroupBox("基本信息");
    QFormLayout *formLayout = new QFormLayout(formGroup);

    // 初始化控件
    QLineEdit *idEdit = new QLineEdit();

    idEdit->setObjectName("idEdit");
    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setObjectName("nameEdit");
    QComboBox *genderCombo = new QComboBox();
    genderCombo->setObjectName("genderCombo");
    QDateEdit *birthdayEdit = new QDateEdit(QDate::currentDate());
    birthdayEdit->setObjectName("birthdayEdit");
    QDateEdit *joinDateEdit = new QDateEdit(QDate::currentDate());
    joinDateEdit->setObjectName("joinDateEdit");
    QLineEdit *goalEdit = new QLineEdit();
    goalEdit->setObjectName("goalEdit");
    QComboBox *progressCombo = new QComboBox();
    progressCombo->setObjectName("progressCombo");

    // 配置控件
    genderCombo->addItems({ tr("男"), tr("女") });
    progressCombo->addItems({ tr("0%"), tr("20%"), tr("40%"), tr("60%"), tr(
                                  "80%"), tr("100%") });
    birthdayEdit->setDisplayFormat("yyyy-MM-dd");
    joinDateEdit->setDisplayFormat("yyyy-MM-dd");
    birthdayEdit->setCalendarPopup(true);
    joinDateEdit->setCalendarPopup(true);

    // 添加控件到表单
    formLayout->addRow(tr("编号："),   idEdit);
    formLayout->addRow(tr("姓名："),   nameEdit);
    formLayout->addRow(tr("性别："),   genderCombo);
    formLayout->addRow(tr("出生日期："), birthdayEdit);
    formLayout->addRow(tr("入学日期："), joinDateEdit);
    formLayout->addRow(tr("学习目标："), goalEdit);
    formLayout->addRow(tr("当前进度："), progressCombo);

    return formGroup;
}

// 创建照片区域
QGroupBox * StudentInfoWidget::createPhotoGroup()
{
    // 创建一个名为"照片上传"的分组框
    QGroupBox *photoGroup = new QGroupBox(tr("照片上传"));

    // 为分组框设置垂直布局
    QVBoxLayout *photoLayout = new QVBoxLayout(photoGroup);

    // 初始化控件
    // 用于显示照片预览的标签
    QLabel *lblPhotoPreview = new QLabel();

    // 用于选择照片的按钮
    QPushButton *btnSelectPhoto = new QPushButton(tr("选择照片"));

    // 配置控件
    // 设置预览标签内容居中显示
    lblPhotoPreview->setAlignment(Qt::AlignCenter);

    // 设置预览标签最小尺寸为200x200像素
    lblPhotoPreview->setMinimumSize(200, 200);

    // 设置选择按钮固定大小为100x40像素
    btnSelectPhoto->setFixedSize(100, 40);

    // 添加控件到布局
    // 将预览标签添加到垂直布局顶部
    photoLayout->addWidget(lblPhotoPreview);

    // 将选择按钮添加到垂直布局底部并水平居中
    photoLayout->addWidget(btnSelectPhoto, 0, Qt::AlignHCenter);

    // 连接照片选择功能
    // 当点击选择按钮时触发文件选择对话框
    connect(btnSelectPhoto, &QPushButton::clicked, [this, lblPhotoPreview]() {
        // 打开文件选择对话框，初始路径为系统图片文件夹
        QString fileName = QFileDialog::getOpenFileName(
            this, tr("选择学生照片"),
            QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
            tr("图片文件 (*.png *.jpg *.jpeg)"));

        // 如果用户选择了文件
        if (!fileName.isEmpty()) {
            // 加载选中的图片
            QPixmap pixmap(fileName);

            // 检查图片是否成功加载
            if (!pixmap.isNull()) {
// 智能缩放图片以适应预览标签
// 缩放时保持原始宽高比，并留出15像素边距
                pixmap = pixmap.scaled(
                    lblPhotoPreview->width() - 30,
                    lblPhotoPreview->height() - 30,
                    Qt::KeepAspectRatio
                    );

                                             // 在预览标签中显示缩放后的图片
                lblPhotoPreview->setPixmap(pixmap);

                                             // 将图片转换为字节数组以便存储
                QBuffer buffer(&photoData);  // photoData应为类成员变量
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG"); // 以PNG格式保存到缓冲区
            }
            else {
                                             // 图片加载失败时显示警告对话框
                QMessageBox::warning(this, tr("错误"), tr("无法加载图片文件！"));
            }
        }
    });

    return photoGroup;
}

// 处理对话框确认
void StudentInfoWidget::handleDialogAccepted(QGroupBox *formGroup,
                                             QGroupBox *photoGroup)
{
    // 通过对象名称查找表单控件
    QLineEdit *idEdit = formGroup->findChild<QLineEdit *>("idEdit");
    QLineEdit *nameEdit = formGroup->findChild<QLineEdit *>("nameEdit");
    QComboBox *genderCombo = formGroup->findChild<QComboBox *>("genderCombo");
    QDateEdit *birthdayEdit = formGroup->findChild<QDateEdit *>("birthdayEdit");
    QDateEdit *joinDateEdit = formGroup->findChild<QDateEdit *>("joinDateEdit");
    QLineEdit *goalEdit = formGroup->findChild<QLineEdit *>("goalEdit");
    QComboBox *progressCombo = formGroup->findChild<QComboBox *>("progressCombo");

    // 基础数据校验：确保学号和姓名不为空
    if (idEdit->text().isEmpty() || nameEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("错误"), tr("学号和姓名不能为空！"));
        return; // 验证失败则终止操作
    }

    // 检查学号唯一性（数据库中是否已存在该学号）
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT id FROM studentInfo WHERE id = ?");
    checkQuery.addBindValue(idEdit->text()); // 绑定学号参数

    // 执行查询并检查结果
    if (checkQuery.exec() && checkQuery.next()) {
        QMessageBox::warning(this, tr("错误"),
                             tr("学号 %1 已存在！").arg(idEdit->text()));
        return; // 学号重复则终止操作
    }

    // 开启数据库事务（确保数据一致性）
    QSqlDatabase::database().transaction();

    // 准备插入SQL语句
    QSqlQuery insertQuery;
    insertQuery.prepare(
        "INSERT INTO studentInfo "
        "(id, name, gender, birthday, join_date, study_goal, progress, photo) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

    // 绑定表单数据到SQL参数（按顺序对应字段）
    insertQuery.addBindValue(idEdit->text());                              // 学号
    insertQuery.addBindValue(nameEdit->text());                            // 姓名
    insertQuery.addBindValue(genderCombo->currentText());                  // 性别
    insertQuery.addBindValue(birthdayEdit->date().toString("yyyy-MM-dd")); // 出生日期
    insertQuery.addBindValue(joinDateEdit->date().toString("yyyy-MM-dd")); // 入学日期
    insertQuery.addBindValue(goalEdit->text());                            // 学习目标
    insertQuery.addBindValue(progressCombo->currentText());                // 学习进度
    // 照片数据：如果为空则存储NULL，否则存储二进制数据
    insertQuery.addBindValue(photoData.isEmpty() ? QVariant() : photoData);

    // 执行插入操作
    if (!insertQuery.exec()) {
        // 插入失败时回滚事务
        QSqlDatabase::database().rollback();
        QMessageBox::critical(this,
                              tr("错误"),
                              tr("添加失败：") + insertQuery.lastError().text());
    }
    else {
        // 插入成功时提交事务
        QSqlDatabase::database().commit();
        refreshTable(); // 刷新表格显示最新数据
        QMessageBox::information(this, tr("成功"),
                                 tr("已成功添加学生：%1").arg(nameEdit->text()));
    }
}

void StudentInfoWidget::on_btnAdd_clicked()
{
    QDialog dlg(this);

    dlg.setWindowTitle(tr("添加学生信息"));
    dlg.setMinimumSize(600, 400);

    // 初始化对话框布局
    QVBoxLayout *mainLayout = new QVBoxLayout(&dlg);
    QHBoxLayout *contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout);

    // 添加表单和照片区域
    QGroupBox *formGroup = createFormGroup();
    QGroupBox *photoGroup = createPhotoGroup();
    contentLayout->addWidget(formGroup,  1);
    contentLayout->addWidget(photoGroup, 1);

    // 添加按钮区域
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnConfirm = new QPushButton(tr("确认"));
    QPushButton *btnCancel = new QPushButton(tr("取消"));

    // 配置按钮
    btnConfirm->setFixedWidth(150);
    btnCancel->setFixedWidth(150);

    // 添加按钮到布局
    btnLayout->addStretch();
    btnLayout->addWidget(btnConfirm);
    btnLayout->addWidget(btnCancel);
    btnLayout->addStretch();

    // 连接按钮信号
    connect(btnConfirm, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(btnCancel,  &QPushButton::clicked, &dlg, &QDialog::reject);
    mainLayout->addLayout(btnLayout);

    // 执行对话框
    if (dlg.exec() == QDialog::Accepted) handleDialogAccepted(formGroup,
                                                              photoGroup);
}

void StudentInfoWidget::on_btnDeleteItem_clicked()
{
    // 获取表格中被选中的单元格
    auto selected = ui->tableWidget->selectedItems();

    // 检查是否有选中的单元格
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要删除的单元格！");
        return;
    }

    // 开始数据库事务，确保所有更新操作要么全部成功，要么全部失败
    QSqlDatabase::database().transaction();

    // 遍历所有选中的单元格
    foreach(QTableWidgetItem * item, selected) {
        // 获取当前单元格所在的行和列
        int row = item->row();
        int col = item->column();

        // 获取当前行的ID（假设ID在第0列）
        QString id = ui->tableWidget->item(row, 0)->text();

        // 定义表格列名与数据库字段的映射关系
        const QStringList columns =
        { "id",        "name",        "gender",
          "birthday",
          "join_date", "study_goal",  "progress",
          "photo" };

        // 创建SQL查询对象
        QSqlQuery query;

        // 准备SQL语句：将指定ID记录的对应字段置为空
        query.prepare(QString("UPDATE studentInfo SET %1 = ? WHERE id = ?").arg(
                          columns[col]));

        // 绑定要更新的值（空字符串）和ID条件
        query.addBindValue("");
        query.addBindValue(id);

        // 执行SQL语句
        if (!query.exec()) {
            // 若执行失败，回滚整个事务
            QSqlDatabase::database().rollback();

            // 显示错误消息
            QMessageBox::critical(this, "错误", "更新失败：" + query.lastError().text());
            return;
        }
    }

    // 所有更新操作成功后提交事务
    QSqlDatabase::database().commit();

    // 刷新表格显示，反映数据库的最新状态
    refreshTable();
}

void StudentInfoWidget::on_btnDeleteLine_clicked()
{
    auto selected = ui->tableWidget->selectionModel()->selectedRows();

    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要删除的行！");
        return;
    }
    QSqlDatabase::database().transaction(); // 启动一个数据库事务直到commit()或者rollback()
    foreach(const QModelIndex& index, selected) {
        QString   id = ui->tableWidget->item(index.row(), 0)->text();
        QSqlQuery query;

        query.prepare("DELETE FROM studentInfo WHERE id = ?");
        query.addBindValue(id);

        if (!query.exec()) {
            QSqlDatabase::database().rollback();
            QMessageBox::critical(this, "错误", "删除失败：" + query.lastError().text());
            return;
        }
    }
    QSqlDatabase::database().commit();
    refreshTable();
}

void StudentInfoWidget::handleItemChanged(QTableWidgetItem *item)
{
    // 获取当前修改项信息
    const int row = item->row();
    const int col = item->column();

    // 如果尝试修改 id 列（主键），恢复原始值并提示用户
    if (col == 0) {
        QMessageBox::warning(this, "警告", "学号是主键，不能修改！");
        refreshTable(); // 刷新表格恢复原始数据
        return;
    }

    // 获取当前行的原始学号（作为更新条件）
    const QString originalId = ui->tableWidget->item(row, 0)->text();

    // 映射列索引到数据库字段名
    const QString columnName = QStringList{ "id", "name", "gender", "birthday",
                                            "join_date", "study_goal", "progress",
                                            "photo" }[col];

    // 开始数据库事务（确保操作原子性）
    QSqlDatabase::database().transaction();

    try {
        // 准备SQL更新语句（动态构建要更新的字段）
        QSqlQuery updateQuery;
        updateQuery.prepare(QString(
                                "UPDATE studentInfo SET %1 = ? WHERE id = ?").arg(
                                columnName));

        // 根据列类型绑定不同的数据
        if (columnName == "photo") { // 处理图片列（存储二进制数据）
            updateQuery.addBindValue(item->data(Qt::UserRole).toByteArray());
        }
        else {                       // 处理普通文本列（去除首尾空格）
            updateQuery.addBindValue(item->text().trimmed());
        }

        // 绑定WHERE子句中的主键条件
        updateQuery.addBindValue(originalId);

        // 执行SQL语句
        if (!updateQuery.exec()) {
            // 抛出异常并附带错误信息
            throw std::runtime_error(
                      "更新失败: " + updateQuery.lastError().text().toStdString());
        }

        // 更新成功，提交事务
        QSqlDatabase::database().commit();
    }
    catch (const std::exception& e) {
        // 发生异常时回滚事务
        QSqlDatabase::database().rollback();

        // 刷新表格恢复原始数据
        refreshTable();

        // 显示错误对话框
        QMessageBox::critical(this, "操作失败", QString::fromUtf8(e.what()));
    }
}
