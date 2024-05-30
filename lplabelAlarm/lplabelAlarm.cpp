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
		>> alarm.isResident;
	return in;
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
	m_pAlarmRH->sendFlawDefectAlarmInfo(alarms.alarmType, alarms.evt_name, alarms.evt_detail, alarms.bOverlay, alarms.bShow, alarms.level, alarms.isResident);

	// 显示弹窗
	QMessageBox::information(nullptr, "Alarm Received",
		QString("Type: %1\nEvent Name: %2\nDetail: %3\nOverlay: %4\nShow: %5\nLevel: %6\nResident: %7")
		.arg(alarms.alarmType)
		.arg(alarms.evt_name)
		.arg(alarms.evt_detail)
		.arg(alarms.bOverlay)
		.arg(alarms.bShow)
		.arg(alarms.level)
		.arg(alarms.isResident));
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