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

ProductManager::ProductManager(const QString &sessionKey, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ProductManager),
    m_networkManager(new QNetworkAccessManager(this)),
    m_sessionKey(sessionKey),
    m_currentUploadingSlot(0)
{
    ui->setupUi(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &ProductManager::onNetworkReply);

    // 初始状态
    ui->uploadProgressBar->hide(); // 默认隐藏进度条
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

// --- 图片选择与预览 ---
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

// --- 保存流程的核心 ---
void ProductManager::on_saveProductButton_clicked()
{
    startSavingProcess();
}

void ProductManager::startSavingProcess()
{
    int currentIndex = ui->productListWidget->currentRow();
    if (currentIndex < 0) return;

    // 1. 将UI表单的当前文本内容，同步到 m_products 对应的条目中
    syncFormToData();

    // 2. 重置上传状态
    m_currentUploadingSlot = 0;

    ui->saveProductButton->setEnabled(false);
    ui->statusbarLabel->setText("正在处理...");

    // 3. 从第一个图片槽开始，启动上传流程
    uploadNextImage();
}

void ProductManager::uploadNextImage()
{
    m_currentUploadingSlot++;

    QString imagePath;
    int currentIndex = ui->productListWidget->currentRow();
    if (currentIndex < 0) return;

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
    ui->uploadProgressBar->setValue(0); // 重置进度条
    ui->uploadProgressBar->show();     // 显示进度条

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 准备 action, key, type 等文本字段
    QHttpPart actionPart;
    actionPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"action\""));
    actionPart.setBody("upload_image");
    multiPart->append(actionPart);

    QHttpPart keyPart;
    keyPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"key\""));
    keyPart.setBody(m_sessionKey.toUtf8());
    multiPart->append(keyPart);

    QHttpPart typePart;
    typePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"type\""));
    typePart.setBody("product");
    multiPart->append(typePart);

    // 准备图片文件
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg")); // 可以根据文件后缀动态设置
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image_file\"; filename=\""+ QFileInfo(imagePath).fileName() +"\""));
    QFile *file = new QFile(imagePath);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "文件错误", "无法读取本地图片文件: " + imagePath);
        delete multiPart;
        return;
    }
    imagePart.setBodyDevice(file);
    file->setParent(multiPart); // 让multiPart来管理file的生命周期
    multiPart->append(imagePart);

    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);

    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // 让reply来管理multiPart的生命周期

    connect(reply, &QNetworkReply::uploadProgress, this, &ProductManager::onUploadProgress);
}

void ProductManager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (bytesTotal > 0) {
        ui->uploadProgressBar->setMaximum(bytesTotal);
        ui->uploadProgressBar->setValue(bytesSent);
    }
}

void ProductManager::saveProductData()
{
    ui->statusbarLabel->setText("正在保存产品信息...");

    // 将 m_products 整个列表转换为JSON
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

    // 发送 action=save_products 的POST请求
    QUrl url("https://tianyuhuanbao.com/api.php");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery postData;
    postData.addQueryItem("action", "save_products");
    postData.addQueryItem("key", m_sessionKey);
    postData.addQueryItem("data", jsonDataString);

    m_networkManager->post(request, postData.query(QUrl::FullyEncoded).toUtf8());
}

// --- 统一的网络响应处理器 ---
void ProductManager::onNetworkReply(QNetworkReply *reply)
{
    disconnect(reply, &QNetworkReply::uploadProgress, this, &ProductManager::onUploadProgress);
    ui->uploadProgressBar->hide(); // 无论成功失败，隐藏进度条

    // 根据请求的URL中的action来区分是哪个操作的响应
    QUrlQuery query(reply->request().url());
    QString action = query.queryItemValue("action");
    if(action.isEmpty()){ // POST请求的action在body里，这里做一个兼容判断
        QByteArray requestBody = reply->request().attribute(QNetworkRequest::SourceIsFromCacheAttribute).toByteArray();
        QUrlQuery postData(requestBody);
        action = postData.queryItemValue("action");
    }

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "网络错误", "操作失败: " + reply->errorString());
        ui->saveProductButton->setEnabled(true);
        ui->statusbarLabel->setText("操作失败！");
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();

        if (action == "upload_image") {
            if (obj["status"].toString() == "success") {
                // 图片上传成功，将其返回的URL更新到对应的数据条目中
                int idx = ui->productListWidget->currentRow();
                if (idx >= 0) {
                    QString newUrl = obj["url"].toString();
                    if (m_currentUploadingSlot == 1) {
                        if (m_currentUploadingSlot == 1) m_localImagePath1.clear(); // 清空本地路径，表示已处理
                    } else if (m_currentUploadingSlot == 2) {
                        if (m_currentUploadingSlot == 2) m_localImagePath2.clear();
                    }
                }
                // 继续处理下一张图片
                uploadNextImage();
            } else {
                QMessageBox::critical(this, "上传失败", "图片上传失败，整个保存操作已取消。");
                ui->saveProductButton->setEnabled(true);
                ui->statusbarLabel->setText("操作已取消。");
            }
        }
        else if (action == "save_products") {
            if (obj["status"].toString() == "success") {
                QMessageBox::information(this, "保存成功", "产品信息已成功更新到服务器。");
                ui->statusbarLabel->setText("保存成功！");
            } else {
                QMessageBox::critical(this, "保存失败", "服务器返回错误: " + obj["message"].toString());
            }
            ui->saveProductButton->setEnabled(true);
        }
    }

    reply->deleteLater();
}

// --- UI 交互 ---
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

void ProductManager::on_productListWidget_currentItemChanged(
    QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    int idx = ui->productListWidget->row(current);
    ui->productBox->setEnabled(idx >= 0);
    ui->saveProductButton->setEnabled(idx >= 0);
    if (idx >= 0) populateForm(idx);
    else clearForm();
}

void ProductManager::updateProductListWidget()
{
    ui->productListWidget->blockSignals(true);
    ui->productListWidget->clear();
    for (const auto &p : m_products)
        ui->productListWidget->addItem(p.name);
    ui->productListWidget->blockSignals(false);
    bool ok = !m_products.isEmpty();
    ui->productBox->setEnabled(ok);
    ui->saveProductButton->setEnabled(ok);
    if (ok) ui->productListWidget->setCurrentRow(0);
    else clearForm();
}

void ProductManager::populateForm(int index)
{
    if (index < 0 || index >= m_products.count()) return;
    // 断开文本编辑信号
    disconnect(ui->productNameEdit,        &QLineEdit::textChanged, this, nullptr);
    disconnect(ui->productCategoryComboBox,&QComboBox::currentTextChanged, this, nullptr);
    disconnect(ui->productDescriptionEdit, &QPlainTextEdit::textChanged, this, nullptr);

    const Product &p = m_products[index];
    ui->productTitleTxt->setText(p.name);
    ui->productNameEdit->setText(p.name);
    ui->productCategoryComboBox->setCurrentText(p.category);
    ui->productDescriptionEdit->setPlainText(p.description);
    // 图片预览
    if (!p.imageUrls.isEmpty()) {
        // 假设两张
        ui->imagePreviewLabel1->setPixmap(QPixmap(p.imageUrls.value(0)).scaled(ui->imagePreviewLabel1->size(), Qt::KeepAspectRatio));
        if (p.imageUrls.size() > 1)
            ui->imagePreviewLabel2->setPixmap(QPixmap(p.imageUrls.value(1)).scaled(ui->imagePreviewLabel2->size(), Qt::KeepAspectRatio));
    }

    // 重新连接
    connect(ui->productNameEdit,        &QLineEdit::textChanged, this, &ProductManager::syncFormToData);
    connect(ui->productCategoryComboBox,&QComboBox::currentTextChanged, this, &ProductManager::syncFormToData);
    connect(ui->productDescriptionEdit, &QPlainTextEdit::textChanged, this, &ProductManager::syncFormToData);
}

void ProductManager::clearForm()
{
    ui->productTitleTxt->clear();
    ui->productNameEdit->clear();
    ui->productCategoryComboBox->setCurrentIndex(0);
    ui->productDescriptionEdit->clear();
    ui->imagePreviewLabel1->clear();
    ui->imagePreviewLabel2->clear();
}

void ProductManager::syncFormToData()
{
    int idx = ui->productListWidget->currentRow();
    if (idx < 0 || idx >= m_products.size()) return;

    // 基本字段
    m_products[idx].name        = ui->productNameEdit->text();
    m_products[idx].category    = ui->productCategoryComboBox->currentText();
    m_products[idx].description = ui->productDescriptionEdit->toPlainText();

    // 如果你用 m_localImagePathN 存储了刚选的本地路径，就更新 imageUrls
    if (!m_localImagePath1.isEmpty()) {
        if (m_products[idx].imageUrls.size() < 1)
            m_products[idx].imageUrls.append(m_localImagePath1);
        else
            m_products[idx].imageUrls[0] = m_localImagePath1;
    }
    if (!m_localImagePath2.isEmpty()) {
        if (m_products[idx].imageUrls.size() < 2)
            m_products[idx].imageUrls.append(m_localImagePath2);
        else
            m_products[idx].imageUrls[1] = m_localImagePath2;
    }
}
