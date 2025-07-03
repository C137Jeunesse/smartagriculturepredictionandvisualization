#include "databasemanager.h"
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QSqlRecord>
#include <QMessageBox>
#include <QDebug>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QElapsedTimer>

DataBaseManager::DataBaseManager() {}

DataBaseManager::~DataBaseManager() {
    if (db.isOpen()) db.close();
}

bool DataBaseManager::InitDataBase() {
    if (QSqlDatabase::contains("smartagri_connection")) {
        db = QSqlDatabase::database("smartagri_connection");
    } else {
        db = QSqlDatabase::addDatabase("QODBC", "smartagri_connection");
        QString dsn = QString("DRIVER=MySQL ODBC 9.3 Unicode Driver;SERVER=rm-2ze58122pu228651soo.mysql.rds.aliyuncs.com;DATABASE=SAdb;USER=dashuai;PASSWORD=DaShuai123456;OPTION=3;");
        db.setDatabaseName(dsn);
    }
    if (db.open()) {
        qDebug("successful connect!");
        return true;
    } else {
        qDebug("connect failed!");
        return false;
    }
}

bool DataBaseManager::IsOpen() const {
    qDebug("检查数据库连接");
    return db.isOpen();
}

// 用户相关
int DataBaseManager::AddUser(const QString& name, const QString& phone, const QString& password, const QString& role, QString* errorOut) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO users (name, phone, passwd, role) VALUES (:name, :phone, :passwd, :role)");
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);
    query.bindValue(":passwd", password);
    query.bindValue(":role", role);
    bool ok = query.exec();
    if (!ok && errorOut) {
        *errorOut = query.lastError().text();
        return -1;
    }else{
        return query.lastInsertId().toInt();
    }
}
// 兼容原有接口
int DataBaseManager::AddUser(const QString& name, const QString& phone, const QString& password, const QString& role) {
    return AddUser(name,phone,password,role,nullptr);
}

bool DataBaseManager::UpdateUser(int userId, const QString& name, const QString& phone, const QString& password, const QString& role) {
    QSqlQuery query(db);
    query.prepare("UPDATE users SET name=:name, phone=:phone, passwd=:passwd, role=:role WHERE id=:id");
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);
    query.bindValue(":passwd", password);
    query.bindValue(":role", role);
    query.bindValue(":id", userId);
    return query.exec();
}

bool DataBaseManager::DeleteUser(int userId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM users WHERE id=:id");
    query.bindValue(":id", userId);
    return query.exec();
}

bool DataBaseManager::CheckUserLogin(const QString& name, const QString& phone, const QString& password, QString& role) {
    QSqlQuery query(db);
    query.prepare("SELECT role FROM users WHERE name=:name AND phone=:phone AND passwd=:passwd");
    query.bindValue(":name", name);
    query.bindValue(":phone", phone);
    query.bindValue(":passwd", password);
    if (query.exec() && query.next()) {
        role = query.value(0).toString();
        return true;
    }
    return false;
}

bool DataBaseManager::IsUserExists(const QString& phone){
    QSqlQuery query(db);
    query.prepare("SELECT COUNT(*) FROM users WHERE phone=:phone");
    query.bindValue(":phone", phone);
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

QVariantList DataBaseManager::GetUserInfo(int userId) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users WHERE id=:id");
    query.bindValue(":id", userId);
    if (query.exec() && query.next()) {
        for (int i = 0; i < query.record().count(); ++i)
            result << query.value(i);
    }
    return result;
}

QVariantList DataBaseManager::GetAllUsers() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM users");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}

int DataBaseManager::GetUserIdByPhone(const QString& phone) {
    QSqlQuery query(db);
    query.prepare("SELECT id FROM users WHERE phone=:phone");
    query.bindValue(":phone", phone);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();  // 返回对应的 userId
    }
    return -1;  // 如果没有找到该电话号码的用户，返回 -1
}

int DataBaseManager::GetUserIdByFarmerId(int FarmerId){
    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM farmer WHERE id = :farmerid");
    query.bindValue(":farmerid",FarmerId);
    if(query.exec()&&query.next()){
        return query.value(0).toInt();
    }
    return -1;
}

int DataBaseManager::GetUserIdByExpertId(int ExpertId){
    QSqlQuery query(db);
    query.prepare("SELECT user_id FROM expert WHERE id = :expertId");
    query.bindValue(":expertId", ExpertId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();  // 返回对应的 userId
    }
    return -1;  // 如果没有找到该专家ID对应的用户，返回 -1
}


// 作物区相关
bool DataBaseManager::AddCropArea(int farmerId, const QString& cropType, float area, const QString& location, const QString& detail) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO crop_area (farmer_id, crop_type, area, location, detail) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(farmerId);
    query.addBindValue(cropType);
    query.addBindValue(area);
    query.addBindValue(location);
    query.addBindValue(detail);
    return query.exec();
}

bool DataBaseManager::UpdateCropArea(int cropAreaId, const QString& cropType, float area, const QString& location, const QString& detail) {
    QSqlQuery query(db);
    query.prepare("UPDATE crop_area SET crop_type=?, area=?, location=?, detail=? WHERE id=?");
    query.addBindValue(cropType);
    query.addBindValue(area);
    query.addBindValue(location);
    query.addBindValue(detail);
    query.addBindValue(cropAreaId);
    return query.exec();
}

bool DataBaseManager::DeleteCropArea(int cropAreaId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM crop_area WHERE id=?");
    query.addBindValue(cropAreaId);

    if (!query.exec()) {
        qDebug() << "DeleteCropArea error:" << query.lastError().text();
        qDebug() << "Query:" << query.lastQuery();
        qDebug() << "CropAreaID:" << cropAreaId;
        return false;
    }
    return true;
}

QVariantList DataBaseManager::GetCropAreas(int farmerId) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM crop_area WHERE farmer_id=?");
    query.addBindValue(farmerId);

    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            row << query.value("id");
            row << query.value("farmer_id");
            row << query.value("crop_type");
            row << query.value("area");
            row << query.value("location");
            row << query.value("detail");

            qDebug() << "查询结果：crop_type = " << query.value("crop_type").toString()
                     << ", location = " << query.value("location").toString(); // 打印调试信息

            result.append(row);
        }
    } else {
        qDebug() << "数据库查询失败：" << query.lastError().text();  // 输出错误信息
    }
    qDebug()<<"result:"<<result.size();
    return result;
}

QVariantList DataBaseManager::GetAllCropAreas() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM crop_area");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}

// 系统建议相关
// 已删除所有system_advice相关实现

// 专家建议相关
// 添加专家建议
bool DataBaseManager::AddExpertAdvice(int expertId, int cropAreaId, const QString& title, const QString& content, const QString& category) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO expert_advice (expert_id, crop_area_id, title, content, category) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(expertId);
    query.addBindValue(cropAreaId);
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(category);
    return query.exec();
}

// 更新专家建议
bool DataBaseManager::UpdateExpertAdvice(int adviceId, const QString& title, const QString& content, const QString& category) {
    QSqlQuery query(db);
    query.prepare("UPDATE expert_advice SET title=?, content=?, category=? WHERE id=?");
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(category);
    query.addBindValue(adviceId);
    return query.exec();
}

// 删除专家建议
bool DataBaseManager::DeleteExpertAdvice(int adviceId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM expert_advice WHERE id=?");
    query.addBindValue(adviceId);
    return query.exec();
}

// 获取特定种植区域的专家建议
QVariantList DataBaseManager::GetExpertAdvices(int cropAreaId) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT ea.id, ea.title, ea.content, ea.category, ea.created_at, u.name AS expert_name "
        "FROM expert_advice ea "
        "JOIN expert e ON ea.expert_id = e.id "
        "JOIN users u ON e.user_id = u.id "  // 通过用户表获取专家姓名
        "WHERE ea.crop_area_id = ? "
        "ORDER BY ea.created_at DESC"
        );
    query.addBindValue(cropAreaId);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap advice;
            advice["id"] = query.value("id");
            advice["title"] = query.value("title");
            advice["content"] = query.value("content");
            advice["category"] = query.value("category");
            advice["created_at"] = query.value("created_at");
            advice["expert_name"] = query.value("expert_name");
            result.append(advice);
        }
    } else {
        qWarning() << "GetExpertAdvices error:" << query.lastError().text();
    }
    return result;
}

// 获取当前农户的所有专家建议
QVariantList DataBaseManager::GetFarmerExpertAdvices(int farmerId) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare(
        "SELECT ea.id, ea.title, ea.content, ea.category, ea.created_at, u.name AS expert_name, "
        "       ca.crop_type, ca.location AS crop_location "
        "FROM expert_advice ea "
        "JOIN expert e ON ea.expert_id = e.id "
        "JOIN users u ON e.user_id = u.id "  // 通过用户表获取专家姓名
        "JOIN crop_area ca ON ea.crop_area_id = ca.id "
        "JOIN farmer f ON ca.farmer_id = f.id "
        "WHERE f.id = ? "  // 通过农户ID过滤
        "ORDER BY ea.created_at DESC"
        );
    query.addBindValue(farmerId);

    if (query.exec()) {
        while (query.next()) {
            QVariantMap advice;
            advice["id"] = query.value("id");
            advice["title"] = query.value("title");
            advice["content"] = query.value("content");
            advice["category"] = query.value("category");
            advice["created_at"] = query.value("created_at");
            advice["expert_name"] = query.value("expert_name");
            advice["crop_type"] = query.value("crop_type");
            advice["crop_location"] = query.value("crop_location");
            result.append(advice);
        }
    } else {
        qWarning() << "GetFarmerExpertAdvices error:" << query.lastError().text();
    }
    return result;
}

QVariantList DataBaseManager::GetAllExpertAdvices() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM expert_advice");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}

// 模型优化建议相关
bool DataBaseManager::AddModelOptimization(int userId, int role, const QString& content) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO model_optimization (user_id, role, content) VALUES (?, ?, ?)");
    query.addBindValue(userId);
    query.addBindValue(role);
    query.addBindValue(content);
    return query.exec();
}

bool DataBaseManager::UpdateModelOptimization(int optimizationId, const QString& content) {
    QSqlQuery query(db);
    query.prepare("UPDATE model_optimization SET content=? WHERE id=?");
    query.addBindValue(content);
    query.addBindValue(optimizationId);
    return query.exec();
}

bool DataBaseManager::DeleteModelOptimization(int optimizationId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM model_optimization WHERE id=?");
    query.addBindValue(optimizationId);
    return query.exec();
}

QVariantList DataBaseManager::GetModelOptimizations() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM model_optimization");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}

QVariantList DataBaseManager::GetAllModelOptimizations() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT * FROM model_optimization");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}

// 单例实现
DataBaseManager& DataBaseManager::instance() {
    static DataBaseManager instance;
    return instance;
}

//测试函数
// void DataBaseManager::TestPrintAllSensors() {
//     QSqlQuery query(db);
//     if (!query.exec("SELECT * FROM sensor")) {
//         qDebug() << "查询传感器数据失败:" << query.lastError().text();
//         return;
//     }
//     while (query.next()) {
//         QStringList row;
//         for (int i = 0; i < query.record().count(); ++i) {
//             row << query.record().fieldName(i) + "=" + query.value(i).toString();
//         }
//         qDebug() << row.join(", ");
//     }
// }

QVariantList DataBaseManager::GetSensorInfo(int sensorId) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT id, resource, event, content, event_time, receive_time FROM sensor WHERE id=:id");
    query.bindValue(":id", sensorId);
    if (query.exec() && query.next()) {
        for (int i = 0; i < query.record().count(); ++i) {
            // content字段直接返回字符串，交由上层解析JSON
            result << query.value(i);
        }
    }
    return result;
}

QVariantList DataBaseManager::GetAllSensors() {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT id, resource, event, content, event_time, receive_time FROM sensor");
    if (query.exec()) {
        while (query.next()) {
            QVariantList row;
            for (int i = 0; i < query.record().count(); ++i)
                row << query.value(i);
            result << QVariant(row);
        }
    }
    return result;
}


//农户相关
int DataBaseManager::AddFarmerUser(const int userId) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO farmer (user_id) VALUES (:userId)");
    query.bindValue(":userId", userId);

    if(query.exec()){
        return query.lastInsertId().toInt();
    } else {
        return -1;
    }
}

bool DataBaseManager::DeleteFarmerUser(const int farmerId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM farmer WHERE id=:farmerId");
    query.bindValue(":farmerId", farmerId);

    bool success = query.exec();
    return success;  // 返回是否删除成功
}

int DataBaseManager::GetFarmerId(const int userId) {
    QSqlQuery query(db);
    query.prepare("SELECT id FROM farmer WHERE user_id=:userId");
    query.bindValue(":userId", userId);

    if (query.exec() && query.next()) {
        int farmerId = query.value(0).toInt();
        return farmerId;  // 返回对应的 farmerId
    }
    return -1;  // 如果没有找到农户信息，返回 -1
}

//专家相关
bool DataBaseManager::AddExpertUser(const int userId, const QString& field) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO expert (user_id, field) VALUES (:userId, :field)");
    query.bindValue(":userId", userId);
    query.bindValue(":field", field);

    return query.exec();
}

bool DataBaseManager::DeleteExpertUser(const int expertId) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM expert WHERE id=:expertId");
    query.bindValue(":expertId", expertId);

    bool success = query.exec();
    return success;  // 返回是否删除成功
}

int DataBaseManager::GetExpertId(const int userId) {
    QSqlQuery query(db);
    query.prepare("SELECT id FROM expert WHERE user_id=:userId");
    query.bindValue(":userId", userId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();  // 返回对应的 expertId
    }
    return -1;  // 如果没有找到专家信息，返回 -1
}

QString DataBaseManager::GetExpertField(const int expertId){
    QSqlQuery query(db);
    query.prepare("SELECT field FROM expert WHERE id=:expertId");
    query.bindValue(":expertId", expertId);
    if (query.exec() && query.next()) {
        return query.value(0).toString();  // 返回对应的 field
    }
    return QString();  // 如果没有找到专家信息，返回空字符串
}

QString DataBaseManager::GetLastError() const {
    QSqlQuery query(db);
    if (query.lastError().isValid()) {
        return query.lastError().text();
    }
    return QString();  // 如果没有错误，返回空字符串
}

bool DataBaseManager::BeginTransaction() {
    return db.transaction();
}

bool DataBaseManager::CommitTransaction() {
    return db.commit();
}

bool DataBaseManager::RollbackTransaction() {
    return db.rollback();
}

bool DataBaseManager::TestConnection() {
    QSqlQuery testQuery(db);
    testQuery.prepare("SELECT 1"); // 执行简单查询测试连接

    QElapsedTimer timer;
    timer.start();

    if (!testQuery.exec()) {
        return false; // 查询失败，连接不可用
    }

    // 检查查询响应时间，避免长时间卡顿
    if (timer.elapsed() > 1000) { // 2秒超时
        return false;
    }

    return testQuery.next() && testQuery.value(0).toInt() == 1;
}

bool DataBaseManager::UpdateExpert(int expertId, const QString& field) {
    QSqlQuery query(db);
    query.prepare("UPDATE expert SET field = :field WHERE id = :id");
    query.bindValue(":field", field);
    query.bindValue(":id", expertId);
    return query.exec();
}

QVariantList DataBaseManager::GetExpertAdviceList(int expertId, int cropAreaId)
{
    QVariantList adviceList;
    QSqlQuery query(db);
    query.prepare("SELECT id, title, content, created_at, category FROM expert_advice WHERE expert_id = :expertId AND crop_area_id = :cropAreaId ORDER BY created_at DESC");
    query.bindValue(":expertId", expertId);
    query.bindValue(":cropAreaId", cropAreaId);

    if (query.exec()) {
        while (query.next()) {
            QVariantList advice;
            advice << query.value("id")           // 0: 建议ID
                   << query.value("title")        // 1: 标题
                   << query.value("content")      // 2: 内容
                   << query.value("created_at")   // 3: 时间
                   << query.value("category");    // 4: 类别
            adviceList << QVariant(advice);
        }
    }

    qDebug() << "GetExpertAdviceList adviceList:" << adviceList;
    return adviceList;
}

// 传感器相关扩展函数实现
QVariantList DataBaseManager::GetSAThData(int cropAreaId, const QString& startTime, const QString& endTime) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT content, receive_time FROM sensor WHERE service_id='SA_th' AND crop_id_virtual=:cropId AND receive_time >= :start AND receive_time <= :end");
    query.bindValue(":cropId", cropAreaId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    if (query.exec()) {
        while (query.next()) {
            QString contentStr = query.value(0).toString();
            QString receiveTime = query.value(1).toString();
            QJsonDocument doc = QJsonDocument::fromJson(contentStr.toUtf8());
            if (!doc.isObject()) continue;
            QJsonObject props = doc.object();
            QVariantList row;
            row << props["temp"].toDouble();
            row << props["stemp"].toDouble();
            row << props["hum"].toDouble();
            row << props["shum"].toDouble();
            row << receiveTime;
            result << QVariant(row);
        }
    }
    return result;
}

QVariantList DataBaseManager::GetSASoilData(int cropAreaId, const QString& startTime, const QString& endTime) {
    QVariantList result;
    QSqlQuery query(db);
    query.prepare("SELECT content, receive_time FROM sensor WHERE service_id='SA_soil' AND crop_id_virtual=:cropId AND receive_time >= :start AND receive_time <= :end");
    query.bindValue(":cropId", cropAreaId);
    query.bindValue(":start", startTime);
    query.bindValue(":end", endTime);
    if (query.exec()) {
        while (query.next()) {
            QString contentStr = query.value(0).toString();
            QString receiveTime = query.value(1).toString();
            QJsonDocument doc = QJsonDocument::fromJson(contentStr.toUtf8());
            if (!doc.isObject()) continue;
            QJsonObject props = doc.object();
            QVariantList row;
            row << props["N"].toDouble();
            row << props["P"].toDouble();
            row << props["K"].toDouble();
            row << receiveTime;
            result << QVariant(row);
        }
    }
    return result;
}

QVariantMap DataBaseManager::GetLatestSoilAndThData(int cropAreaId) {
    QVariantMap result;
    // soil
    QSqlQuery querySoil(db);
    querySoil.prepare("SELECT content FROM sensor WHERE service_id='SA_soil' AND crop_id_virtual=:cropId ORDER BY receive_time DESC LIMIT 1");
    querySoil.bindValue(":cropId", cropAreaId);
    if (querySoil.exec() && querySoil.next()) {
        QString contentStr = querySoil.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(contentStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject props = doc.object();
            result["N"] = props["N"].toDouble();
            result["P"] = props["P"].toDouble();
            result["K"] = props["K"].toDouble();
        }
    }
    // th
    QSqlQuery queryTh(db);
    queryTh.prepare("SELECT content FROM sensor WHERE service_id='SA_th' AND crop_id_virtual=:cropId ORDER BY receive_time DESC LIMIT 1");
    queryTh.bindValue(":cropId", cropAreaId);
    if (queryTh.exec() && queryTh.next()) {
        QString contentStr = queryTh.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(contentStr.toUtf8());
        if (doc.isObject()) {
            QJsonObject props = doc.object();
            result["temp"] = props["temp"].toDouble();
            result["stemp"] = props["stemp"].toDouble();
            result["hum"] = props["hum"].toDouble();
            result["shum"] = props["shum"].toDouble();
        }
    }
    return result;
}

// 测试函数：测试新加的sensor相关接口
void DataBaseManager::TestPrintSensorFunctions(int cropAreaId, const QString& startTime, const QString& endTime) {
    qDebug() << "--- 测试GetSAThData ---";
    QVariantList saThList = GetSAThData(cropAreaId, startTime, endTime);
    for (const QVariant& rowVar : saThList) {
        QVariantList row = rowVar.toList();
        qDebug() << "temp:" << row.value(0).toDouble()
                 << "stemp:" << row.value(1).toDouble()
                 << "hum:" << row.value(2).toDouble()
                 << "shum:" << row.value(3).toDouble()
                 << "event_time:" << row.value(4).toString();
    }
    qDebug() << "--- 测试GetSASoilData ---";
    QVariantList saSoilList = GetSASoilData(cropAreaId, startTime, endTime);
    for (const QVariant& rowVar : saSoilList) {
        QVariantList row = rowVar.toList();
        qDebug() << "N:" << row.value(0).toDouble()
                 << "P:" << row.value(1).toDouble()
                 << "K:" << row.value(2).toDouble()
                 << "event_time:" << row.value(3).toString();
    }
    qDebug() << "--- 测试GetLatestSoilAndThData ---";
    QVariantMap latest = GetLatestSoilAndThData(cropAreaId);
    qDebug() << "N:" << latest.value("N").toDouble()
             << "P:" << latest.value("P").toDouble()
             << "K:" << latest.value("K").toDouble()
             << "temp:" << latest.value("temp").toDouble()
             << "stemp:" << latest.value("stemp").toDouble()
             << "hum:" << latest.value("hum").toDouble()
             << "shum:" << latest.value("shum").toDouble();
}
