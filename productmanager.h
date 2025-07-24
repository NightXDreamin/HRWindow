// productmanager.h (带进度条的最终生产版)
#ifndef PRODUCTMANAGER_H
#define PRODUCTMANAGER_H

#include <QWidget>
#include "datastructures.h"
#include <QList>

// 向前声明
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
    void on_addProduct_clicked();
    void on_deleteProduct_clicked();
    void on_productListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    void on_saveProductButton_clicked();

    void on_selectImageButton1_clicked();
    void on_selectImageButton2_clicked();

    void onNetworkReply(QNetworkReply *reply);

    // [新功能] 用于更新上传进度条的槽函数
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);

private:
    Ui::ProductManager *ui;
    QList<Product>     m_products;
    QNetworkAccessManager *m_networkManager;
    const QString      m_sessionKey;

    QString m_localImagePath1;
    QString m_localImagePath2;

    int m_currentUploadingSlot;

    void updateProductListWidget();
    void populateForm(int index);
    void clearForm();
    void syncFormToData();

    void startSavingProcess();
    void uploadNextImage();
    void saveProductData();
};

#endif // PRODUCTMANAGER_H
