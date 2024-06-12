#include "lplabelAlarm.h"

lplabelAlarm::lplabelAlarm(QString strName)
	:LPMQService(strName)
{

	m_pAlarmRH = new LPRedHandAlarm();
	parseWarningValues();
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

void lplabelAlarm::parseWarningValues() {
	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/Config/Alarm_Lithium.json";
	QFile file(jsonFilePath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open file:" << jsonFilePath;
		return;
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (doc.isNull() || !doc.isObject()) {
		qWarning() << "Invalid JSON document.";
		return;
	}

	QMap<QString, QMap<QString, QPair<double, double>>> newWarningValues;
	QJsonObject obj = doc.object();
	QJsonObject channels = obj["channels"].toObject();

	for (auto channelId : channels.keys()) {
		QJsonObject channel = channels[channelId].toObject();
		QJsonArray categories = channel["categories"].toArray();
		QMap<QString, QPair<double, double>> categoryWarnings;

		for (const QJsonValue& category : categories) {
			QJsonObject categoryObj = category.toObject();
			QString name = categoryObj["name"].toString();
			QJsonArray warningValue = categoryObj["settings"].toObject()["warningValue"].toArray();
			double max = warningValue[0].toDouble();
			double min = warningValue[1].toDouble();

			categoryWarnings.insert(name, qMakePair(max, min));
		}

		newWarningValues.insert(channelId, categoryWarnings);
	}

	// 更新类成员变量
	this->warningValues = newWarningValues;
}


QMap<int, QString> lplabelAlarm::loadTypeDescriptions() {
	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/Config/typeDescriptions.json";
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
	case LP_UPDATA_CONFIG:
	{
		QString jsonConfig;
		in >> jsonConfig;
		updateConfig(jsonConfig);
		break;
	}
	default:
		break;
	}
}


void lplabelAlarm::updateConfig(const QString &jsonConfig) {
	QString jsonFilePath = QCoreApplication::applicationDirPath() + "/Config/Alarm_Lithium.json";
	QFile file(jsonFilePath);

	// 读取现有配置
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning() << "Failed to open file for reading:" << jsonFilePath;
		return;
	}
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
	file.close();

	if (!doc.isObject()) {
		qWarning() << "Invalid JSON document structure.";
		return;
	}

	// 解析传入的 JSON 配置
	QJsonDocument newDoc = QJsonDocument::fromJson(jsonConfig.toUtf8());
	if (!newDoc.isObject()) {
		qWarning() << "Invalid new JSON config structure.";
		return;
	}

	QJsonObject rootObj = doc.object();
	QJsonObject channelsObj = rootObj["channels"].toObject();
	QJsonObject newChannelsObj = newDoc.object()["channels"].toObject();
	QJsonObject channelsNumObj = rootObj["channels_num"].toObject();
	QJsonObject newChannelsNumObj = newDoc.object()["channels_num"].toObject();

	// 更新 channels
	for (const QString &channelId : newChannelsObj.keys()) {
		if (channelsObj.contains(channelId)) {
			QJsonObject channelObj = channelsObj[channelId].toObject();
			QJsonObject newChannelObj = newChannelsObj[channelId].toObject();
			QJsonArray categoriesArray = channelObj["categories"].toArray();
			QJsonArray newCategoriesArray = newChannelObj["categories"].toArray();

			for (const QJsonValue &newCategoryValue : newCategoriesArray) {
				QJsonObject newCategoryObj = newCategoryValue.toObject();
				bool found = false;

				for (int i = 0; i < categoriesArray.size(); ++i) {
					QJsonObject categoryObj = categoriesArray[i].toObject();
					if (categoryObj["name"].toString() == newCategoryObj["name"].toString()) {
						categoryObj["settings"] = newCategoryObj["settings"];
						categoriesArray[i] = categoryObj;
						found = true;
						break;
					}
				}

				if (!found) {
					qWarning() << "Category not found and not added:" << newCategoryObj["name"].toString();
				}
			}

			channelObj["categories"] = categoriesArray;
			channelsObj[channelId] = channelObj;
		}
		else {
			qWarning() << "Channel not found and not updated:" << channelId;
		}
	}

	// 更新 channels_num
	for (const QString &channelId : newChannelsNumObj.keys()) {
		if (channelsNumObj.contains(channelId)) {
			channelsNumObj[channelId] = newChannelsNumObj[channelId];
		}
		else {
			qWarning() << "Channel number not found and not updated:" << channelId;
		}
	}

	rootObj["channels"] = channelsObj;
	rootObj["channels_num"] = channelsNumObj;
	doc.setObject(rootObj);

	// 写回修改后的配置
	if (!file.open(QIODevice::WriteOnly)) {
		qWarning() << "Failed to open file for writing:" << jsonFilePath;
		return;
	}
	file.write(QJsonDocument(rootObj).toJson());
	file.close();

	// 重新加载配置到内存
	parseWarningValues();
}


void lplabelAlarm::handleLithiumWidth(const LithiumTypeInfoPub_Tag & Widthalarms)
{


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