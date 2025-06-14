#ifndef TABLEDELEGATES_H
#define TABLEDELEGATES_H
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QDateEdit>
#include <QBuffer>
#include <QEvent>
#include <QLabel>
#include <Qpainter>
#include <QMouseEvent>
#include <QFileDialog>

// 自定义组合框委托类，继承自 QStyledItemDelegate
class ComboBoxDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:

    // 构造函数，接收父对象指针
    ComboBoxDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}

    // 设置下拉菜单的选项列表
    void setItems(const QStringList& items)
    {
        m_items = items; // 将传入的选项列表保存到成员变量
    }

    // 创建用于编辑的控件（当用户双击表格单元格时调用）
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override
    {
        Q_UNUSED(option); // 忽略样式选项参数
        Q_UNUSED(index);  // 忽略索引参数（所有列使用相同的选项列表）

        // 创建组合框控件
        QComboBox *editor = new QComboBox(parent);
        editor->addItems(m_items); // 添加预定义选项到组合框
        return editor;             // 返回组合框作为编辑器
    }

    // 将编辑器中的数据保存到模型中（用户完成编辑时调用）
    void setModelData(QWidget            *editor,
                      QAbstractItemModel *model,
                      const QModelIndex & index) const override
    {
        // 将编辑器转换为组合框类型
        QComboBox *comboBox = static_cast<QComboBox *>(editor);

        // 获取当前选中的文本
        QString value = comboBox->currentText();

        // 将选中的值写入模型（使用 EditRole 指定编辑角色）
        model->setData(index, value, Qt::EditRole);
    }

private:

    QStringList m_items; // 存储下拉菜单的选项列表
};

class DateEditDelegate : public QStyledItemDelegate {
public:

    explicit DateEditDelegate(QObject *parent = nullptr) : QStyledItemDelegate(
            parent) {}

    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        QDateEdit *editor = new QDateEdit(parent);

        editor->setDisplayFormat("yyyy-MM-dd"); // 设置日期格式
        editor->setCalendarPopup(true);         // 启用日历弹出
        return editor;
    }

    void setModelData(QWidget            *editor,
                      QAbstractItemModel *model,
                      const QModelIndex & index) const override {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);

        if (dateEdit) model->setData(index,
                                     dateEdit->date().toString("yyyy-MM-dd"),
                                     Qt::EditRole);
    }
};

class ImageDelegate : public QStyledItemDelegate {
public:

    // 构造函数，初始化基类
    explicit ImageDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent)
    {}

    // 创建编辑器控件 - 这里使用QLabel显示图片
    QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem& option,
                          const QModelIndex& index) const override {
        Q_UNUSED(option); Q_UNUSED(index); // 标记参数未使用

        // 创建QLabel作为编辑器
        QLabel *editor = new QLabel(parent);
        return editor;
    }

    // 将编辑器中的数据保存到模型
    void setModelData(QWidget            *editor,
                      QAbstractItemModel *model,
                      const QModelIndex & index) const override {
        QLabel *label = qobject_cast<QLabel *>(editor); // 将编辑器转换为QLabel

        if (label) {
            QByteArray imageData;
            QPixmap    pixmap = label->pixmap(); // 获取QLabel中的QPixmap对象

            if (!pixmap.isNull()) {
                // 创建缓冲区并将QPixmap保存为PNG格式的二进制数据
                QBuffer buffer(&imageData);
                buffer.open(QIODevice::WriteOnly);
                pixmap.save(&buffer, "PNG");
            }

            // 将二进制数据保存到模型的UserRole中
            model->setData(index, imageData, Qt::UserRole);
        }
    }

    // 绘制项的内容 - 显示图片
    void paint(QPainter                   *painter,
               const QStyleOptionViewItem& option,
               const QModelIndex         & index) const override {
        // 从模型获取图片二进制数据
        QByteArray imageData = index.data(Qt::UserRole).toByteArray();

        // 如果数据为空，使用默认绘制
        if (imageData.isEmpty()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        // 加载图片数据到QPixmap
        QPixmap pixmap;
        pixmap.loadFromData(imageData);

        // 如果加载失败，使用默认绘制
        if (pixmap.isNull()) {
            QStyledItemDelegate::paint(painter, option, index);
            return;
        }

        // 绘制图片
        QRect rect = option.rect; // 获取项的矩形区域

        // 缩放图片到100x100并保持宽高比
        QPixmap scaledPixmap = pixmap.scaled(100, 100, Qt::KeepAspectRatio);

        // 在项的矩形区域绘制图片
        painter->drawPixmap(rect, scaledPixmap);
    }

    // 处理编辑器事件 - 双击时允许用户选择新图片
    bool editorEvent(QEvent                     *event,
                     QAbstractItemModel         *model,
                     const QStyleOptionViewItem& option,
                     const QModelIndex         & index) override {
        // 检查是否为鼠标双击事件
        if (event->type() == QEvent::MouseButtonDblClick) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

            // 检查是否为左键双击
            if (mouseEvent->button() == Qt::LeftButton) {
                // 弹出文件对话框选择新图片
                QString imagePath = QFileDialog::getOpenFileName(
                    nullptr,                   // 父窗口
                    "选择图片",                    // 对话框标题
                    "",                        // 默认路径
                    "图片文件 (*.png *.jpg *.bmp)" // 文件过滤器
                    );

                // 如果用户选择了有效路径
                if (!imagePath.isEmpty()) {
                    // 打开文件并读取二进制数据
                    QFile file(imagePath);

                    if (file.open(QIODevice::ReadOnly)) {
                        QByteArray imageData = file.readAll();
                        file.close();

                        // 将二进制数据保存到模型的UserRole中
                        model->setData(index, imageData, Qt::UserRole);
                    }
                }

                // 事件已处理
                return true;
            }
        }

        // 其他事件交给基类处理
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};


#endif // TABLEDELEGATES_H
