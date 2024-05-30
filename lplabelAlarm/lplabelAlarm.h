#pragma once

#include "lplabelalarm_global.h"
#include "lpservicestatusmsg.h"
//#include "lpwavedatamsg.h"
#include "lpmqservice.h"
#include "redhandapi_global.h"
#include <QString>
#include <QMessageBox>
#pragma execution_character_set("utf-8")


struct Lithium_alarm
{
	int alarmType;
	QString evt_name;
	QString evt_detail;
	bool bOverlay;
	bool bShow;
	int level;
	bool isResident;
	double length;
};




QDataStream &operator>>(QDataStream &in, Lithium_alarm &alarm);

class LPRedHandAlarm
{
public:
	LPRedHandAlarm();
	~LPRedHandAlarm();
	void sendFlawDefectAlarmInfo(int alarmType, const QString& evt_name,
		const QString& evt_detail, bool bOverlay,
		bool bShow, int level, bool isResidents, double length);

};

class LPMQSERVICE_API lplabelAlarm : public LPMQService
{
public:
    lplabelAlarm(QString strName);

	virtual void recvMsg(QByteArray& ba, const MsgReceiverInfo& receiverInfo);
	void handleLithium(const Lithium_alarm &alarms);
	QMap<QString, QPair<double, double>> parseWarningValues();

private:
	LPRedHandAlarm* m_pAlarmRH;

};


