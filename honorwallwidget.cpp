#include "honorwallwidget.h"
#include "ui_honorwallwidget.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QMessageBox>
#include <QFileDialog>
#include <QBuffer>
#include <QSqlError>
HonorWallWidget::HonorWallWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HonorWallWidget)
{
    ui->setupUi(this);
    setupUI();
    loadImagesFromDatabase();
}

HonorWallWidget::~HonorWallWidget()
{
    delete ui;
}

void HonorWallWidget::setupUI()
{
    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout;

    // 添加按钮
    addButton = new QPushButton("添加图片", this);

    connect(addButton, &QPushButton::clicked, this,
            &HonorWallWidget::addImage);
    buttonLayout->addWidget(addButton);

    // 修改按钮
    modifyButton = new QPushButton("修改图片", this);

    connect(modifyButton,
            &QPushButton::clicked,
            this,
            &HonorWallWidget::modifyImage);
    buttonLayout->addWidget(modifyButton);

    // 删除按钮
    deleteButton = new QPushButton("删除图片", this);

    connect(deleteButton,
            &QPushButton::clicked,
            this,
            &HonorWallWidget::deleteImage);
    buttonLayout->addWidget(deleteButton);
    mainLayout->addLayout(buttonLayout);

    // 滚动区域
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 允许内容区域调整大小
    // 内容区域
    contentWidget = new QWidget(scrollArea);
    gridLayout = new QGridLayout(contentWidget);
    contentWidget->setLayout(gridLayout);

    // 设置滚动区域的内容
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    // 设置主布局
    setLayout(mainLayout);
}

void HonorWallWidget::loadImagesFromDatabase()
{
    // 清空布局中的所有内容
    QLayoutItem *item;

    while ((item = gridLayout->takeAt(0)) != nullptr) {
        delete item; // 删除布局项
    }

    // 从数据库中加载图片
    QSqlQuery query("SELECT id, image_data FROM honorWall");

    while (query.next()) {
        int id = query.value(0).toInt();
        QByteArray imageData = query.value(1).toByteArray();

        // 将二进制数据转换为 QPixmap
        QPixmap pixmap;
        pixmap.loadFromData(imageData);

        if (!pixmap.isNull()) {
            // 将图片显示在界面上
            ClickableLabel *imageLabel = new ClickableLabel(contentWidget);
            QPixmap scaledPixmap = pixmap.scaled(imgW,
                                                 imgH,
                                                 Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation);
            imageLabel->setPixmap(scaledPixmap);
            imageLabel->setAlignment(Qt::AlignCenter);
            imageLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");
            imageLabel->setProperty("id", id); // 设置 id 属性

            connect(imageLabel, &ClickableLabel::clicked, this,
                    &HonorWallWidget::onImageClicked);

            // 动态添加到网格布局
            int row = gridLayout->count() / 3; // 每行 3 张图片
            int col = gridLayout->count() % 3;
            gridLayout->addWidget(imageLabel, row, col);
        }
        else {
            qWarning() << "无法加载图片数据！";
        }
    }
}

void HonorWallWidget::addImage()
{
    // 打开文件对话框选择图片
    QString imagePath = QFileDialog::getOpenFileName(this,
                                                     "选择图片",
                                                     "",
                                                     "图片文件 (*.png *.jpg *.jpeg *.bmp)");

    if (!imagePath.isEmpty()) addImageToWall(imagePath);
}

void HonorWallWidget::addImageToWall(const QString& imagePath)
{
    // 加载图片
    QPixmap pixmap(imagePath);

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片！");
        return;
    }

    // 将图片转换为二进制数据
    QByteArray imageData;
    QBuffer    buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG"); // 保存为 PNG 格式
    // 将图片信息插入数据库
    QSqlQuery query;
    query.prepare(
        "INSERT INTO honorWall (image_data, description,added_date) VALUES(:image_data, :description,:added_date)");
    query.bindValue(":image_data",  imageData);
    query.bindValue(":description", "未填写描述");                         // 默认描述
    query.bindValue(":added_date",  QDate::currentDate().toString()); // 默认描述

    if (!query.exec()) {
        qWarning() << "插入数据失败：" << query.lastError().text();
        return;
    }

    // 将图片显示在界面上
    addImageToUI(pixmap);
}

void HonorWallWidget::addImageToUI(const QPixmap& pixmap)
{
    if (pixmap.isNull()) {
        qWarning() << "图片无效！";
        return;
    }
    QPixmap scaledPixmap = pixmap.scaled(imgW,
                                         imgH,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);

    // 创建可点击的 QLabel 显示图片
    ClickableLabel *imageLabel = new ClickableLabel(contentWidget);

    if (!imageLabel) {
        qWarning() << "无法创建 QLabel！";
        return;
    }
    imageLabel->setPixmap(scaledPixmap);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");

    connect(imageLabel,
            &ClickableLabel::clicked,
            this,
            &HonorWallWidget::onImageClicked);

    // 动态添加到网格布局
    int row = gridLayout->count() / 3; // 每行 3 张图片
    int col = gridLayout->count() % 3;
    gridLayout->addWidget(imageLabel, row, col);
}

void HonorWallWidget::onImageClicked()
{
    if (selectedLabel) { // 取消之前选中的图片样式
        selectedLabel->setStyleSheet("border: 1px solid #ccc; padding: 5px;");
    }
    selectedLabel = qobject_cast<ClickableLabel *>(sender());

    if (selectedLabel) { // 更新选中的图片
        selectedLabel->setStyleSheet("border: 2px solid red; padding: 5px;");
    }
}

void HonorWallWidget::deleteImage()
{
    if (!selectedLabel) {
        QMessageBox::warning(this, "错误", "请先选择一张图片！");
        return;
    }

    // 确认删除
    if (QMessageBox::question(this, "确认删除",
                              "确定要删除这张图片吗？") != QMessageBox::Yes) return;

    // 获取当前选中图片对应的数据库 id
    int id = selectedLabel->property("id").toInt();

    // 从数据库中删除记录
    QSqlQuery query;
    query.prepare("DELETE FROM honorWall WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qWarning() << "删除数据失败：" << query.lastError().text();
        return;
    }

    // 从界面上删除图片
    gridLayout->removeWidget(selectedLabel);
    delete selectedLabel;
    selectedLabel = nullptr;
    reorderImages(); // 重新排列剩余的图片
}

void HonorWallWidget::reorderImages()
{
    // 清空布局中的所有内容
    QLayoutItem *item;

    while ((item = gridLayout->takeAt(0)) != nullptr) {
        delete item; // 删除布局项
    }

    // 重新加载图片;
    loadImagesFromDatabase();
}

void HonorWallWidget::modifyImage()
{
    if (!selectedLabel) {
        QMessageBox::warning(this, "错误", "请先选择一张图片！");
        return;
    }

    // 打开文件对话框选择新图片
    QString imagePath = QFileDialog::getOpenFileName(this,
                                                     "选择图片",
                                                     "",
                                                     "图片文件 (*.png *.jpg *.jpeg *.bmp)");

    if (imagePath.isEmpty()) return;

    // 加载新图片
    QPixmap pixmap(imagePath);

    if (pixmap.isNull()) {
        QMessageBox::warning(this, "错误", "无法加载图片！");
        return;
    }

    // 将图片转换为二进制数据
    QByteArray imageData;
    QBuffer    buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG"); // 保存为 PNG 格式
    // 获取当前选中图片对应的数据库 id
    int id = selectedLabel->property("id").toInt();

    // 更新数据库
    QSqlQuery query;
    query.prepare("UPDATE honorWall SET image_data = :image_data WHERE id = :id");
    query.bindValue(":image_data", imageData);
    query.bindValue(":id",         id);

    if (!query.exec()) {
        qWarning() << "更新数据失败：" << query.lastError().text();
        return;
    }

    // 更新界面上的图片
    QPixmap scaledPixmap = pixmap.scaled(imgW,
                                         imgH,
                                         Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    selectedLabel->setPixmap(scaledPixmap);
}
