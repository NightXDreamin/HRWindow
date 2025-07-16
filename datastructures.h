// datastructures.h
#ifndef DATASTRUCTURES_H
#define DATASTRUCTURES_H

#include <QString>
#include <QStringList>

// --- 这里是我们项目所有共享数据结构的定义中心 ---

// 1. 招聘职位的数据结构
struct Job {
    QString title;
    QString quota;
    QString salaryStart;
    QString salaryEnd;
    QString requirements;
};

// 2. 产品中心的数据结构
struct Product {
    QString name;
    QString category;
    QString description;
    QStringList imageUrls; // 存储一个或多个图片URL
};

// 3. 过往案例的数据结构
struct CaseStudy {
    QString title;
    QString description;
    QStringList imageUrls;
};

struct DashboardStats {
    int totalJobsCount = 0;
    int totalProductsCount = 0;
    int totalCasesCount = 0;
    int totalRecruitmentQuota = 0;
    QString serverTime;
};

// Q_DECLARE_METATYPE(Job);      // 如果您需要在QVariant中使用这些结构体，
// Q_DECLARE_METATYPE(Product);   // 就取消这些行的注释。目前我们还用不到。
// Q_DECLARE_METATYPE(CaseStudy);

#endif // DATASTRUCTURES_H
