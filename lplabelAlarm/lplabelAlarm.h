#pragma once

#include "lplabelalarm_global.h"
#include "lpservicestatusmsg.h"
//#include "lpwavedatamsg.h"
#include "lpmqservice.h"
#include "redhandapi_global.h"
#include <QString>
#include <QMessageBox>



struct Lithium_alarm
{
	int alarmType;
	QString evt_name;
	QString evt_detail;
	bool bOverlay;
	bool bShow;
	int level;
	bool isResident;
};


//QDataStream &operator<<(QDataStream &out, const Lithium_alarm &alarm)
//{
//	out << alarm.alarmType
//		<< alarm.evt_name
//		<< alarm.evt_detail
//		<< alarm.bOverlay
//		<< alarm.bShow
//		<< alarm.level
//		<< alarm.isResident;
//	return out;
//}

QDataStream &operator>>(QDataStream &in, Lithium_alarm &alarm);

class LPRedHandAlarm
{
public:
	LPRedHandAlarm();
	~LPRedHandAlarm();
	void sendFlawDefectAlarmInfo(int alarmType, const QString& evt_name,
		const QString& evt_detail, bool bOverlay,
		bool bShow, int level, bool isResidents);
};

class LPMQSERVICE_API lplabelAlarm : public LPMQService
{
public:
    lplabelAlarm(QString strName);

	virtual void recvMsg(QByteArray& ba, const MsgReceiverInfo& receiverInfo);
	void handleLithium(const Lithium_alarm &alarms);


private:
	LPRedHandAlarm* m_pAlarmRH;

};


