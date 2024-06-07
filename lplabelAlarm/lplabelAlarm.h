#pragma once

#include "lplabelalarm_global.h"
#include "lpservicestatusmsg.h"
//#include "lpwavedatamsg.h"
#include "lpmqservice.h"
#include "redhandapi_global.h"
#include <QString>
#include <QMessageBox>
#pragma execution_character_set("utf-8")


struct SimpleFlawInfo
{
	int camId{ -1 };    //所属相机id
	QString coorNodeName;//工位名
	double RightEdgeMm{ -1 };
	double LeftEdgeMm{ -1 };
	double centerXMm{ -1 };
	double centerYMm{ -1 };
	double FlawWidthMm{ -1 };
	int chanleId;//识别通道序号

	friend QDataStream &operator>>(QDataStream &stream, SimpleFlawInfo &info) {
		stream >> info.camId
			>> info.coorNodeName
			>> info.RightEdgeMm
			>> info.LeftEdgeMm
			>> info.centerXMm
			>> info.centerYMm
			>> info.FlawWidthMm
			>> info.chanleId;
		return stream;
	}
	
};
struct  LithiumTypeInfoPub_Tag
{
	//9001~9004使用同一个结构体
	//同一个纵向米数的所有工位信息
	int msgId;//消息id
	int detectType;//缺陷类型9001~9004
	qreal  doffY100PosMm{ -1 }; //局部y坐标精确mm,每隔100mm中取一个
	qint64 doffId{ -1 };
	QList<SimpleFlawInfo> simpleFlawInfoList;

	//friend QDataStream& operator<<(QDataStream& stream, const LithiumTypeInfoPub_Tag& info) {
	//	stream << info.msgId << info.detectType << info.doffY100PosMm << info.doffId << info.simpleFlawInfoList;
	//	return stream;
	//}
	friend QDataStream& operator>>(QDataStream& stream, LithiumTypeInfoPub_Tag& info) {
		stream >> info.detectType >> info.doffY100PosMm >> info.doffId >> info.simpleFlawInfoList;
		return stream;
	}
};
struct  LithiumCalculatebyInfoPub_Tag
{
	//通过9001-9004的对齐信息，
	int msgId;//消息id
	qreal  doffY100PosMm{ -1 }; //局部y坐标精确mm,每隔100mm中取一个
	qint64 doffId{ -1 };
	int detectType;//缺陷类型
	QMap<int, QMap<QString, double>> contactNameAndAlignment;//<通道，<工位A_工位B,对齐度>>

	//使用centerX标识9001在每一个图片上位置
	//QString ContactCoorName;//工位A_工位B，哪两个元素进行对比
	//double contactNameAndAlignment{-1};
/*		对齐度
		centerX，*/

		//QMap<QString, double> contactNameAndWidth;//<工位名，宽度>
		//QList<SimpleFlawInfo> simpleFlawInfoList;

	//friend QDataStream& operator<<(QDataStream& stream, const LithiumCalculatebyInfoPub_Tag& info) {
	//	stream << info.msgId << info.doffY100PosMm << info.doffId << info.detectType << info.contactNameAndAlignment;
	//	return stream;
	//}
	friend QDataStream& operator>>(QDataStream& stream, LithiumCalculatebyInfoPub_Tag& info) {
		stream >> info.msgId >> info.doffY100PosMm >> info.doffId >> info.detectType >> info.contactNameAndAlignment;
		return stream;
	}
};







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
	void handleLithiumWidth(const LithiumTypeInfoPub_Tag &Widthalarms);

	void handleLithiumAlign(const LithiumCalculatebyInfoPub_Tag &Alignalarms);
	QMap<QString, QMap<QString, QPair<double, double>>> lplabelAlarm::parseWarningValues();

	QMap<int, QString> loadTypeDescriptions();

private:
	LPRedHandAlarm* m_pAlarmRH;

};


