// productmanager.h (生产级最终版)
#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <QWidget>
#include "datastructures.h"
#include <QList>

class QNetworkAccessManager;
class QNetworkReply;
class QListWidgetItem;

namespace Ui {
class ProductManager;
}

class ProductManager : public QWidget
{
    Q_OBJECT

public:
    explicit ProductManager(const QString &sessionKey, QWidget *parent = nullptr);
    ~ProductManager();

public slots:
    void updateData(const QList<Product> &products);

private slots:
    // --- 所有槽函数都将由Qt根据objectName自动连接 ---
    void on_addProduct_clicked();
    void on_deleteProduct_clicked();
    void on_productListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_saveProductButton_clicked();

    // 图片选择按钮
    void on_selectImageButton1_clicked();
    void on_selectImageButton2_clicked();

    // 唯一的网络响应处理器
    void onNetworkReply(QNetworkReply *reply);

    // 上传进度
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    Ui::ProductManager *ui;
    QList<Product>     m_products;
    QNetworkAccessManager *m_networkManager;
    const QString      m_sessionKey;

    // 存储待上传的本地图片路径
    QString m_localImagePath1;
    QString m_localImagePath2;

    // 管理保存流程的状态变量
    int m_currentUploadingSlot;

    // 私有函数
    void updateProductListWidget();
    void populateForm(int index);
    void clearForm();
    void syncFormToData(int index); // 将表单的文本内容同步到数据结构

    // 图片上传流程
    void startSavingProcess();
    void uploadNextImage();
    void saveProductData();
};

#endif // PRODUCTMANAGER_H
