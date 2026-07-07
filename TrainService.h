#include "TrainService.h"

TrainService::TrainService(QSqlDatabase &db) : m_db(db) {}

bool TrainService::addTrain(const Train&, QString*) { return true; }
bool TrainService::updateTrain(const Train&, QString*) { return true; }
bool TrainService::deleteTrain(int, QString*) { return true; }
bool TrainService::setTrainStatus(int, const QString&, QString*) { return true; }
bool TrainService::configureSeats(int, const QString&, int, QString*) { return true; }
bool TrainService::isTrainBookable(int) { return true; }

QVector<Train> TrainService::searchTrains(const QString&, const QString&, const QDate&) { return {}; }
QVector<TrainTicketInfo> TrainService::queryRemainingTickets(const QString&, const QString&, const QDate&) { return {}; }

int TrainService::getRemainingSeats(int, const QString&) { return 100; } // 默认有100张票供E联调
std::optional<Train> TrainService::getTrainById(int) { return std::nullopt; }