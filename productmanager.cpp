// productmanager.cpp (生产级最终版)
#include "productmanager.h"
#include "ui_productmanager.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFileInfo>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QListWidgetItem>
#include <QDebug> // 用于调试

ProductManager::ProductManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProductManager),
    m_networkManager(new QNetworkAccessManager(this)),
    m_sessionKey(sessionKey),
    m_currentUploadingSlot(0)
{
    ui->setupUi(this);

    // --- [核心修正] 移除所有重复的手动connect，只保留一个统一的网络处理器 ---
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this,             &ProductManager::onNetworkReply);

    // 初始状态
    ui->uploadProgressBar->hide();
    ui->productBox->setEnabled(false);
    ui->saveProductButton->setEnabled(false);
}

ProductManager::~ProductManager()
{
    delete ui;
}

// 由主窗口调用，更新产品列表
void ProductManager::updateData(const QList<Product> &products)
{
    m_products = products;
    updateProductListWidget();
}


// --- UI 交互 (由Qt自动连接触发) ---
void ProductManager::on_addProduct_clicked()
{
    Product p;
    p.name = "新产品 - 请修改";
    p.category = ui->productCategoryComboBox->currentText();
    m_products.append(p);
    updateProductListWidget();
    ui->productListWidget->setCurrentRow(m_products.count()-1);
}

void ProductManager::on_deleteProduct_clicked()
{
    int row = ui->productListWidget->currentRow();
    if (row < 0) return;

    auto reply = QMessageBox::question(this, "确认删除",
                                       QString("确定删除产品 '%1' ?").arg(m_products[row].name),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_products.removeAt(row);
        updateProductListWidget();
    }
}

void ProductManager::on_productListWidget_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (previous) {
        int prevIdx = ui->productListWidget->row(previous);
        if (prevIdx >= 0) syncFormToData(prevIdx);
    }

    int idx = ui->productListWidget->row(current);
    bool isValidIndex = (idx >= 0);
    ui->productBox->setEnabled(isValidIndex);
    ui->saveProductButton->setEnabled(isValidIndex);

    if (isValidIndex) populateForm(idx);
    else clearForm();
}

void ProductManager::on_selectImageButton1_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "选择图片1", "", "Images (*.png *.jpg *.bmp)");
    if (file.isEmpty()) return;
    m_localImagePath1 = file;
    ui->productImgPath_1->setText(QFileInfo(file).fileName());
    ui->imagePreviewLabel1->setPixmap(QPixmap(file).scaled(ui->imagePreviewLabel1->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ProductManager::on_selectImageButton2_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "选择图片2", "", "Images (*.png *.jpg *.bmp)");
    if (file.isEmpty()) return;
    m_localImagePath2 = file;
    ui->productImgPath_2->setText(QFileInfo(file).fileName());
    ui->imagePreviewLabel2->setPixmap(QPixmap(file).scaled(ui->imagePreviewLabel2->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}


// --- 保存流程 (核心逻辑) ---
void ProductManager::on_saveProductButton_clicked()
{
    startSavingProcess();
}

void ProductManager::startSavingProcess()
{
    int currentIndex = ui->productListWidget->currentRow();
    if (currentIndex < 0) return;

    syncFormToData(currentIndex);
    m_currentUploadingSlot = 0;

    ui->saveProductButton->setEnabled(false);
    ui->statusbarLabel->setText("正在处理...");

    uploadNextImage();
}

void ProductManager::uploadNextImage()
{
    m_currentUploadingSlot++;

    QString imagePath;
    if (m_currentUploadingSlot == 1) imagePath = m_localImagePath1;
    else if (m_currentUploadingSlot == 2) imagePath = m_localImagePath2;
    else {
        saveProductData();
        return;
    }

    if (imagePath.isEmpty()) {
        uploadNextImage();
        return;
    }

    ui->statusbarLabel->setText(QString("正在上传图片 %1...").arg(m_currentUploadingSlot));
    ui->uploadProgressBar->setValue(0);
    ui->uploadProgressBar->show();

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart actionPart;
    actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"action\""));
    actionPart.setBody("upload_image");

    QHttpPart keyPart;
    keyPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"key\""));
    keyPart.setBody(m_sessionKey.toUtf8());

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("product");

    multiPart->append(actionPart);
    multiPart->append(keyPart);
    multiPart->append(typePart);

    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image_file\"; filename=\""+ QFileInfo(imagePath).fileName() +"\""));
    QFile *file = new QFile(imagePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "文件错误", "无法读取本地图片: " + imagePath);
        delete multiPart;
        return;
    }
    imagePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(imagePart);

    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::User, "upload_image");

    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this, &ProductManager::onUploadProgress);
}

void ProductManager::saveProductData()
{
    ui->statusbarLabel->setText("正在保存产品信息...");

    QJsonArray productsArray;
    for (const auto &p : m_products) {
        QJsonObject o;
        o["name"] = p.name;
        o["category"] = p.category;
        o["description"] = p.description;
        o["imageUrls"] = QJsonArray::fromStringList(p.imageUrls);
        productsArray.append(o);
    }
    QJsonDocument doc(productsArray);
    QString jsonDataString = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));

    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setAttribute(QNetworkRequest::User, "save_products");

    QUrlQuery postData;
    postData.addQueryItem("action", "save_products");
    postData.addQueryItem("key", m_sessionKey);
    postData.addQueryItem("data", jsonDataString);

    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
}

void ProductManager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->uploadProgressBar->setMaximum(bytesTotal);
        ui->uploadProgressBar->setValue(bytesSent);
    }
}

void ProductManager::onNetworkReply(QNetworkReply *reply)
{
    disconnect(reply, &QNetworkReply::uploadProgress, this, &ProductManager::onUploadProgress);
    ui->uploadProgressBar->hide();

    QString requestType = reply->request().attribute(QNetworkRequest::User).toString();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "操作失败: " + reply->errorString());
        ui->saveProductButton->setEnabled(true);
        ui->statusbarLabel->setText("操作失败！");
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        if (requestType == "upload_image") {
            if (obj["status"].toString() == "success") {
                int idx = ui->productListWidget->currentRow();
                if (idx >= 0) {
                    QString newUrl = obj["url"].toString();
                    if (m_currentUploadingSlot == 1) {
                        if (m_products[idx].imageUrls.size() < 1) m_products[idx].imageUrls.append(newUrl);
                        else m_products[idx].imageUrls[0] = newUrl;
                        m_localImagePath1.clear();
                    } else if (m_currentUploadingSlot == 2) {
                        if (m_products[idx].imageUrls.size() < 2) m_products[idx].imageUrls.append(newUrl);
                        else m_products[idx].imageUrls[1] = newUrl;
                        m_localImagePath2.clear();
                    }
                }
                uploadNextImage();
            } else {
                QMessageBox::critical(this, "图片上传失败", obj["message"].toString());
                ui->saveProductButton->setEnabled(true);
                ui->statusbarLabel->setText("图片上传失败！");
            }
        }
        else if (requestType == "save_products") {
            if (obj["status"].toString() == "success") {
                QMessageBox::information(this, "保存成功", obj["message"].toString());
                ui->statusbarLabel->setText("保存成功！");
            } else {
                QMessageBox::critical(this, "保存失败", "服务器返回错误: " + obj["message"].toString());
            }
            ui->saveProductButton->setEnabled(true);
        }
    }

    reply->deleteLater();
}

void ProductManager::updateProductListWidget()
{
    ui->productListWidget->blockSignals(true);
    ui->productListWidget->clear();
    for (const auto &p : m_products)
        ui->productListWidget->addItem(p.name);
    ui->productListWidget->blockSignals(false);

    bool hasProducts = !m_products.isEmpty();
    ui->deleteProduct->setEnabled(hasProducts);
    ui->productBox->setEnabled(hasProducts);
    ui->saveProductButton->setEnabled(hasProducts);

    if (hasProducts) ui->productListWidget->setCurrentRow(0);
    else clearForm();
}

void ProductManager::populateForm(int index)
{
    if (index < 0 || index >= m_products.count()) return;

    const Product &p = m_products[index];
    ui->productNameEdit->setText(p.name);
    ui->productCategoryComboBox->setCurrentText(p.category);
    ui->productDescriptionEdit->setPlainText(p.description);

    ui->imagePreviewLabel1->clear();
    ui->productImgPath_1->setText("尚未选择新图片");
    m_localImagePath1.clear();
    ui->imagePreviewLabel2->clear();
    ui->productImgPath_2->setText("尚未选择新图片");
    m_localImagePath2.clear();

    // [新功能] 从网络加载并显示已有的图片
    if (!p.imageUrls.isEmpty()) {
        ui->productImgPath_1->setText(p.imageUrls.value(0));
        // (加载网络图片的具体实现，需要额外发送GET请求，我们可以在下一步细化)
    }
    if (p.imageUrls.size() > 1) {
        ui->productImgPath_2->setText(p.imageUrls.value(1));
    }
}

void ProductManager::clearForm()
{
    ui->productNameEdit->clear();
    ui->productCategoryComboBox->setCurrentIndex(0);
    ui->productDescriptionEdit->clear();
    ui->imagePreviewLabel1->clear();
    ui->productImgPath_1->setText("尚未选择");
    m_localImagePath1.clear();
    ui->imagePreviewLabel2->clear();
    ui->productImgPath_2->setText("尚未选择");
    m_localImagePath2.clear();
}

void ProductManager::syncFormToData(int index)
{
    if (index < 0 || index >= m_products.size()) return;
    m_products[index].name        = ui->productNameEdit->text();
    m_products[index].category    = ui->productCategoryComboBox->currentText();
    m_products[index].description = ui->productDescriptionEdit->toPlainText();
}
