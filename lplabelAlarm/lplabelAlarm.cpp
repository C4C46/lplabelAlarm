#include "lplabelAlarm.h"

lplabelAlarm::lplabelAlarm(QString strName)
	:LPMQService(strName)
{

	m_pAlarmRH = new LPRedHandAlarm();
}


QDataStream &operator>>(QDataStream &in, Lithium_alarm &alarm)
{
	in >> alarm.alarmType
		>> alarm.evt_name
		>> alarm.evt_detail
		>> alarm.bOverlay
		>> alarm.bShow
		>> alarm.level
		>> alarm.isResident
		>> alarm.length;
	return in;
}


QMap<QString, QPair<double, double>> lplabelAlarm::parseWarningValues()
{

	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/Alarm_Lithium.json";
	QFile file(jsonFilePath);
	QMap<QString, QPair<double, double>> warningValues;

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open file:" << jsonFilePath;
		return warningValues; // 返回空的 QMap
	}


	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	QJsonObject obj = doc.object();
	QJsonArray categories = obj["categories"].toArray();

	for (const QJsonValue& category : categories) {
		QJsonObject categoryObj = category.toObject();
		QString name = categoryObj["name"].toString();
		QJsonObject settings = categoryObj["settings"].toObject();
		QJsonArray warningValue = settings["warningValue"].toArray();
		double max = warningValue[0].toDouble();
		double min = warningValue[1].toDouble();

		warningValues.insert(name, qMakePair(max, min));
	}

	qDebug() << "Loaded warning values successfully from:" << jsonFilePath;
	file.close();
	return warningValues;
}

//接收锂电信息
void lplabelAlarm::recvMsg(QByteArray & ba, const MsgReceiverInfo & receiverInfo)
{
	QDataStream in(ba);
	int type;
	in >> type;

	switch (type)
	{
	case LP_ALARMMSG_PUB_ALARM_INFO:
	{
		Lithium_alarm alarms;
		in >> alarms;
		handleLithium(alarms);

	}
	default:
		break;
	}
}

void lplabelAlarm::handleLithium(const Lithium_alarm & alarms)
{
	static QMap<QString, QPair<double, double>> warningValues = parseWarningValues();

	if (warningValues.contains(alarms.evt_name))
	{
		double length = alarms.length;
		double max = warningValues[alarms.evt_name].first;
		double min = warningValues[alarms.evt_name].second;
		if (length <min || length > max)
		{
			QMessageBox::warning(nullptr, "警告", QString("长度超出范围: %1\n允许的范围: [%2, %3]").arg(length).arg(min).arg(max));
			m_pAlarmRH->sendFlawDefectAlarmInfo(alarms.alarmType, alarms.evt_name, alarms.evt_detail, alarms.bOverlay, alarms.bShow, alarms.level, alarms.isResident, alarms.length);
		}
	}

	
}





LPRedHandAlarm::LPRedHandAlarm()
{

	LPRedHand::SetAppName("AOI");
	LPRedHand::GetRedHandManager();
}

LPRedHandAlarm::~LPRedHandAlarm()
{
	LPRedHand::DestroyRedHandManager();
}


//发送报警
void LPRedHandAlarm::sendFlawDefectAlarmInfo(int alarmType, const QString & evt_name, const QString & evt_detail, bool bOverlay, bool bShow, int level, bool isResidents, double length)
{

	QSharedPointer<lpRHCustomEvent> eventInfo = QSharedPointer<lpRHCustomEvent>(new lpRHCustomEvent);
	eventInfo->alarmType = (emAlarmType)alarmType;
	eventInfo->eventName = evt_name;
	eventInfo->eventCotent = evt_detail;
	eventInfo->overlay = bOverlay;
	eventInfo->show = bShow;
	eventInfo->level = level;
	eventInfo->isResident = isResidents;
	eventInfo->length = length;
	LPRedHand::GetRedHandManager()->SendCustomEvent(eventInfo);


}


LPMQService* LpMQServiceNewInstance(const QString& name)
{
	return new lplabelAlarm(name);
}