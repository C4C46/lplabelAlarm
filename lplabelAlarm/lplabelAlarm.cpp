#include "lplabelAlarm.h"

lplabelAlarm::lplabelAlarm(QString strName)
	:LPMQService(strName)
{

	m_pAlarmRH = new LPRedHandAlarm();
}


//QDataStream &operator>>(QDataStream &stream, SimpleFlawInfo &info) {
//	stream >> info.camId
//		>> info.coorNodeName
//		>> info.RightEdgeMm
//		>> info.LeftEdgeMm
//		>> info.centerXMm
//		>> info.centerYMm
//		>> info.FlawWidthMm
//		>> info.chanleId;
//	return stream;
//}
//
//QDataStream &operator>>(QDataStream &stream, LithiumTypeInfoPub_Tag &Widthalarms)
//{
//	stream >> Widthalarms.detectType
//		>> Widthalarms.doffY100PosMm 
//		>> Widthalarms.doffId
//		>> Widthalarms.simpleFlawInfoList;
//	return stream;
//
//}



QMap<QString, QMap<QString, QPair<double, double>>> lplabelAlarm::parseWarningValues()
{

	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/Alarm_Lithium.json";
	QFile file(jsonFilePath);
	QMap<QString, QMap<QString, QPair<double, double>>> warningValues;;

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open file:" << jsonFilePath;
		return warningValues; // 返回空的 QMap
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (doc.isNull()) {
		qWarning() << "Failed to create JSON doc.";
		return warningValues;
	}
	if (!doc.isObject()) {
		qWarning() << "JSON document is not an object.";
		return warningValues;
	}

	QJsonObject obj = doc.object();
	QJsonObject channels = obj["channels"].toObject();
	if (channels.isEmpty()) {
		qWarning() << "No 'channels' object in JSON file or 'channels' is empty.";
		return warningValues;
	}

	for (auto channelId : channels.keys()) {
		QJsonObject channel = channels[channelId].toObject();
		QJsonArray categories = channel["categories"].toArray();
		QMap<QString, QPair<double, double>> categoryWarnings;

		for (const QJsonValue& category : categories) {
			QJsonObject categoryObj = category.toObject();
			QString name = categoryObj["name"].toString();
			QJsonObject settings = categoryObj["settings"].toObject();
			QJsonArray warningValue = settings["warningValue"].toArray();
			double max = warningValue[0].toDouble();
			double min = warningValue[1].toDouble();

			categoryWarnings.insert(name, qMakePair(max, min));
		}

		warningValues.insert(channelId, categoryWarnings);
	}

	qDebug() << "Loaded warning values successfully from:" << jsonFilePath;
	file.close();
	return warningValues;
}

QMap<int, QString> lplabelAlarm::loadTypeDescriptions() {
	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/typeDescriptions.json";
	QFile file(jsonFilePath);
	QMap<int, QString> typeDescriptions;

	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open type descriptions file:" << jsonFilePath;
		return typeDescriptions;
	}

	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	if (doc.isNull()) {
		qWarning() << "Failed to create JSON doc from type descriptions.";
		return typeDescriptions;
	}

	QJsonObject obj = doc.object();
	for (auto key : obj.keys()) {
		typeDescriptions.insert(key.toInt(), obj[key].toString());
	}

	return typeDescriptions;
}


//接收锂电信息
void lplabelAlarm::recvMsg(QByteArray & ba, const MsgReceiverInfo & receiverInfo)
{
	QDataStream in(ba);
	int type;
	in >> type;

	switch (type)
	{
		
	case LP_SEND_WIDTH:
	{
		LithiumTypeInfoPub_Tag Widthalarms;
		in >> Widthalarms;
		handleLithiumWidth(Widthalarms);
		break;

	}
	case LP_SEND_ALIGN:
	{
		LithiumCalculatebyInfoPub_Tag  Alignalarms;
		in >> Alignalarms;
		handleLithiumAlign(Alignalarms);
		break;
	}
	case LP_SEND_CENTRALIZER:
	{
		LithiumElectrodeRegionCentralizePub_Tag Centeralarms;
		in >> Centeralarms;
		handleLithiumCentralizer(Centeralarms);
		break;
	}
	default:
		break;
	}
}

void lplabelAlarm::handleLithiumWidth(const LithiumTypeInfoPub_Tag & Widthalarms)
{
	static QMap<QString, QMap<QString, QPair<double, double>>> warningValues = parseWarningValues();

	static const QMap<int, QString> typeDescriptions = loadTypeDescriptions();


	for (const SimpleFlawInfo &flawInfo : Widthalarms.simpleFlawInfoList)
	{
		//QString chanleId = QString("%1").arg(flawInfo.chanleId);
		QString description = typeDescriptions.value(Widthalarms.detectType, "未知类型");
		QString coorNodeName = flawInfo.coorNodeName;
		double flawWidth = flawInfo.FlawWidthMm;
		QString eventName = QString("%1_%2异常").arg(description).arg(coorNodeName);
		QString eventDetail = QString("宽度异常值: %1").arg(flawWidth);

		if (warningValues.contains(QString::number(Widthalarms.detectType)) && warningValues[QString::number(Widthalarms.detectType)].contains(coorNodeName))
		{
			double max = warningValues[QString::number(Widthalarms.detectType)][coorNodeName].first;
			double min = warningValues[QString::number(Widthalarms.detectType)][coorNodeName].second;
			if (flawWidth < min || flawWidth > max)
			{
				m_pAlarmRH->sendFlawDefectAlarmInfo(Widthalarms.detectType, eventName, eventDetail, false, true, 3, false);
			}
		}
	}

	
}

void lplabelAlarm::handleLithiumAlign(const LithiumCalculatebyInfoPub_Tag &Alignalarms)
{
	static QMap<QString, QMap<QString, QPair<double, double>>> warningValues = parseWarningValues();
	static const QMap<int, QString> typeDescriptions = loadTypeDescriptions();

	// 遍历所有通道
	for (auto channelIter = Alignalarms.contactNameAndAlignment.begin(); channelIter != Alignalarms.contactNameAndAlignment.end(); ++channelIter) {
		int channelId = channelIter.key();  // 通道ID
		const QMap<QString, double> &alignmentMap = channelIter.value();  // 获取工位对齐度映射

		// 遍历每个工位对的对齐度
		for (auto alignmentIter = alignmentMap.begin(); alignmentIter != alignmentMap.end(); ++alignmentIter) {

			QString description = typeDescriptions.value(Alignalarms.detectType, "未知类型");
			QString contactName = alignmentIter.key();  // 工位名
			double alignmentValue = alignmentIter.value();  // 对齐度
			QString eventName = QString("%1_%2异常").arg(description).arg(contactName);//缺陷类型_工位名
			QString eventDetail = QString("对齐度异常值: %1").arg(alignmentValue);

			if (warningValues.contains(QString::number(Alignalarms.detectType)) && warningValues[QString::number(Alignalarms.detectType)].contains(contactName))
			{
				double max = warningValues[QString::number(Alignalarms.detectType)][contactName].first;
				double min = warningValues[QString::number(Alignalarms.detectType)][contactName].second;
				if (alignmentValue < min || alignmentValue > max)
				{
					m_pAlarmRH->sendFlawDefectAlarmInfo(Alignalarms.detectType, eventName, eventDetail, false, true, 3, false);
				}
			}
		}
	}

}

void lplabelAlarm::handleLithiumCentralizer(const LithiumElectrodeRegionCentralizePub_Tag & Centeralarms)
{
	static QMap<QString, QMap<QString, QPair<double, double>>> warningValues = parseWarningValues();
	static const QMap<int, QString> typeDescriptions = loadTypeDescriptions();

	QString description = typeDescriptions.value(Centeralarms.detectType, "未知类型");
	QString coorNodeName = Centeralarms.coorNodeName;
	double centralizeValue = Centeralarms.electrodeRegionCentralize;
	QString eventName = QString("%1_%2异常").arg(description).arg(coorNodeName);
	QString eventDetail = QString("居中度异常值: %1").arg(centralizeValue);

	if (warningValues.contains(QString::number(Centeralarms.detectType)) &&
		warningValues[QString::number(Centeralarms.detectType)].contains(coorNodeName))
	{
		double max = warningValues[QString::number(Centeralarms.detectType)][coorNodeName].first;
		double min = warningValues[QString::number(Centeralarms.detectType)][coorNodeName].second;
		if (centralizeValue < min || centralizeValue > max)
		{
			m_pAlarmRH->sendFlawDefectAlarmInfo(Centeralarms.detectType, eventName, eventDetail, false, true, 3, false);
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
void LPRedHandAlarm::sendFlawDefectAlarmInfo(int alarmType, const QString & evt_name, const QString & evt_detail, bool bOverlay, bool bShow, int level, bool isResidents)
{

	QSharedPointer<lpRHCustomEvent> eventInfo = QSharedPointer<lpRHCustomEvent>(new lpRHCustomEvent);
	eventInfo->alarmType = (emAlarmType)alarmType;
	eventInfo->eventName = evt_name;
	eventInfo->eventCotent = evt_detail;
	eventInfo->overlay = bOverlay;
	eventInfo->show = bShow;
	eventInfo->level = level;
	eventInfo->isResident = isResidents;
	LPRedHand::GetRedHandManager()->SendCustomEvent(eventInfo);


}


LPMQService* LpMQServiceNewInstance(const QString& name)
{
	return new lplabelAlarm(name);
}