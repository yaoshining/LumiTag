#ifndef __DFROBOT_BC20__
#define __DFROBOT_BC20__
#define OPENDBG    1
#include "Arduino.h"
#include "../../GsvParser.h"
extern GsvSnapshot gsvSnapshot;
#include "DFRobot_BC20AT.h"
#include "DFRobot_queue.h"
#define DBG(sth)   Serial.print("DBG:");Serial.print(__LINE__);Serial.println(sth)
#define ON   1
#define OFF  0
#define SN      0
#define IMEI    1
#define IMEISV  2
#define SVN     3

typedef struct
{
    uint8_t *IMEI_NUM;
}sSIMCard_t;
extern sSIMCard_t sSIMCard;

typedef struct
{
    uint8_t dataNum;
}sRecData_t;
extern sRecData_t sRecData;

typedef struct
{
    uint8_t *company;
    uint8_t *deviceType;
    uint8_t *Revision;
}sPII_t;
extern sPII_t sPII;

typedef struct
{
    uint8_t *company;
    uint8_t *deviceType;
    uint8_t *Revision;
}sMI_t;
extern sMI_t sMI;

typedef struct
{
    uint8_t rssi;
    uint8_t ber;
}sSQ_t;
extern sSQ_t sSQ;

typedef struct
{
    uint8_t rxlev;/*Received signal strength level。信号强度*/
    uint8_t ber;/*Channel bit error rate (in percent)。信道误码率*/
    uint8_t rscp;/*received signal code power .接收信号码功率*/
    uint8_t ecno;/* Ec/No*/
    uint8_t rsrq;/*Reference signal received quality。参考信号接收质量*/
    uint8_t rsrp;/*Reference signal received power。参考信号接收功率*/
}sESQ_t;
extern sESQ_t sESQ;

typedef struct
{
    String sn;
    String imei;
    String imeisv;
    String svn;
}sGSN_t;
extern sGSN_t sGSN;

typedef struct
{
    String LatitudeVal;
    String LatitudeDir;
    String LongitudeVal;
    String LongitudeDir;
    String UTC_Time;
    String DataStatus;
    String PositionMode;
}sGLL_t;
extern sGLL_t sGLL;

typedef struct
{
    String UTC_Time;
    String LatitudeVal;
    String LatitudeDir;
    String LongitudeVal;
    String LongitudeDir;
    String Status;
    String StatelliteNum;//卫星数量
    String HDOP;//水平精度因子
    String Altitude;//海拔高度
    String GeoID_Separation;//大地水平距离
    String DGPS_Age;//Differential revision time差分修订时间
    String DGPS_StationID;//差分参考基站ID号
}sGGA_t;
extern sGGA_t sGGA;

struct sGSAD
{
    String Mode;
    String FixStatus;
    String Statellite_CH[12];
	/*
    String Statellite_CH1;
    String Statellite_CH2;
    String Statellite_CH3;
    String Statellite_CH4;
    String Statellite_CH5;
    String Statellite_CH6;
    String Statellite_CH7;
    String Statellite_CH8;
    String Statellite_CH9;
    String Statellite_CH10;
    String Statellite_CH11;
    String Statellite_CH12;*/
    String PDOP;//位置精度因子
    String HDOP;//水平精度因子
    String VDOP;//垂直精度因子
    String GNSS_SystemID;//1-GPS,2-GLONASS,3-Galileo,4-BeiDou
};

typedef struct
{
    uint8_t NUM;
    sGSAD data[4];
}sGSA_t;
extern sGSA_t sGSA;

typedef struct
{
    String UTC_Time;
    String DataStatus;
    String LatitudeVal;
    String LatitudeDir;
    String LongitudeVal;
    String LongitudeDir;
    String GroundSpeed;
    String GroundHeading;
    String UTC_Date;
    String MagDeclination;//磁偏角
    String MagDeclinationDir;//磁偏角方向
    String PositioningMode;//位置模式
    String NaviStatus;//导航状态
}sRMC_t;
extern sRMC_t sRMC;

typedef struct
{
    String GroundCourse_True;
    String GroundCourse_Mag;
    String GroundCourse_Knots;//对地航速
    String GroundCourse_Kmh;//对地速度
    String PositioningMode;
}sVTG_t;
extern sVTG_t sVTG;

struct sGSV
{
    String PRN;
    String Elev;
    String Azim;
    String SNR;
    String Status;
    String SYS;
};

typedef struct
{
    uint8_t NUM;
    uint8_t USE;
    sGSV data[60];
}sSAT_t;
extern sSAT_t sSAT;

typedef struct
{
    uint16_t Year;
    uint8_t  Month;
    uint8_t  Day;
    int  Hour;
    uint8_t  Minute;
    uint8_t  Second;
    String   Mode;
    int TimeEquation;
}sCLK_t;
extern sCLK_t sCLK;

typedef struct
{
    float LatitudeVal;
    String LatitudeDir;
    float LongitudeVal;
    String LongitudeDir;
    float Altitude;
    float Speed;
    float Heading;
    String FixStatus;
    String PDOP;
    String HDOP;
    String VDOP;
    uint8_t StatelliteNum;
}sGGNS_t;
extern sGGNS_t sGGNS;

typedef struct
{
    uint8_t ActiveTime;
    String ActiveUint;
    uint8_t TAUTime;
    String TAUUint;
}sCEREG_t;
extern sCEREG_t sCEREG;

typedef enum {
    eTAUunit_10M,  //10 minutes
    eTAUunit_1H,   //1 hour
    eTAUunit_10H,  //10 hour
    eTAUunit_2S,   //2 seconds
    eTAUunit_30S,  //30 seconds
    eTAUunit_1M,   //1 minute
    eTAUunit_320H, //320 hours
    eTAUunit_stop  // stop
} eTAUunit_t;

typedef enum {
    eAcTunit_2S,         //2 seconds
    eAcTunit_1M,         //1 minute
    eAcTunit_decihours,   //decihours
    eAcTunit_stop        //stop
} eAcTunit_t;

typedef enum {
    eeDRXValue_20_48,    //20.48 seconds
    eeDRXValue_40_96,    //40.96 seconds
    eeDRXValue_81_92,    //81.92 seconds
    eeDRXValue_163_84,   //163.84 seconds
    eeDRXValue_327_68,   //327.68 seconds
    eeDRXValue_655_36,   //655.36 seconds
    eeDRXValue_1310_72,  //1310.72 seconds
    eeDRXValue_2621_44,  //2621.44 seconds
    eeDRXValue_5242_88,  //5242.88 seconds
    eeDRXValue_10485_76  //10485.76 seconds
} eeDRXValue_t;

typedef enum {
    eNWProvidedValue_20_48,    //20.48 seconds
    eNWProvidedValue_40_96,    //40.96 seconds
    eNWProvidedValue_81_92,    //81.92 seconds
    eNWProvidedValue_163_84,   //163.84 seconds
    eNWProvidedValue_327_68,   //327.68 seconds
    eNWProvidedValue_655_36,   //655.36 seconds
    eNWProvidedValue_1310_72,  //1310.72 seconds
    eNWProvidedValue_2621_44,  //2621.44 seconds
    eNWProvidedValue_5242_88,  //5242.88 seconds
    eNWProvidedValue_10485_76  //10485.76 seconds
} eNWProvidedValue_t;

typedef enum {
    ePTWValue_2_56,            //2.56  seconds
    ePTWValue_5_12,            //5.12  seconds
    ePTWValue_7_68,            //7.68  seconds
    ePTWValue_10_24,           //10.24 seconds
    ePTWValue_12_8,            //12.8  seconds
    ePTWValue_15_36,           //15.36 seconds
    ePTWValue_17_92,           //17.92 seconds
    ePTWValue_20_48,           //20.48 seconds
    ePTWValue_23_04,           //23.04 seconds
    ePTWValue_25_6,            //25.6  seconds
    ePTWValue_28_16,           //28.16 seconds
    ePTWValue_30_72,           //30.72 seconds
    ePTWValue_33_28,           //33.28 seconds
    ePTWValue_35_84,           //35.84 seconds
    ePTWValue_38_4,            //38.4  seconds
    ePTWValue_40_96            //40.96 seconds
} ePTWValue_t;

typedef enum {
    ePSM_OFF,
    ePSM_ON,
    ePSM_OFF_ResetParam
}ePSM_t;

typedef enum {
    eSleepMode_Disable,
    eSleepMode_DeepSleep,
    eSleepMode_Light
}eSleepMode_t;

class DFRobot_BC20{
  public:
    DFRobot_BC20(void);
    ~DFRobot_BC20(void);
    String Echo_OFF = SET_ECHO_0;
    String Echo_ON = SET_ECHO_1;
    bool mqttConneced = false;
    bool powerOn(void);
	bool configKeepalive(char* alivetime);
    bool checkBC20(void);
    bool checkNBCard(void);
    void getGPS(void);
    void getPII(void);
    void getMI(void);
    void getMM(void);
    void getMR(void);
    void getSQ(void);
    void getESQ(void);
    void getGSN(uint8_t cmd=0);
    void getEREG(void);
    bool setEREG(uint8_t cmd=4);
    void getSCON(void);
    bool setSCON(uint8_t cmd);
    void getOPS(void);
    bool setOPS(uint8_t cmd);
    uint8_t getGATT(void);
    bool setGATT(uint8_t state);
    void getGDCONT(void);
    bool setGDCONT(uint8_t state);
    void getGACT(void);
    bool setGACT(uint8_t state);
    void getGPADDR(void);
    bool setGPADDR(uint8_t state);
    uint8_t *getIMI(void);
    uint8_t getFUN(void);
    bool setFUN(uint8_t fun);
    uint8_t getMEE(void);
    bool setMEE(uint8_t fun);
    String getCLK(void);
    void getBC(void);
    void getPSMS(void);
    bool setPSMS(uint8_t mode);
    void getEDRXS(void);
    bool setEDRXS(uint8_t mode);
    void getEDRXRDP(void);
    uint8_t getTZR(void);
    bool setTZR(uint8_t onoff);
    void getIPCA(void);
    bool setIPCA(uint8_t n);
    void getCIOTOPT(void);
    bool setCIOTOPT(uint8_t n);
    bool setGAPNRC(uint8_t cid);
    String getEER(void);
    bool setQGACT(uint8_t op);
    uint8_t getQBAND(void);
    bool setQBAND(uint8_t band);
    bool Rest(uint8_t mode = 1);
    uint8_t getQSPCHSC(void);
    bool setQSPCHSC(uint8_t mode);
    void getQLOCKF(void);
    bool setQLOCKF(uint8_t mode);
    uint8_t getQCSEARFCN(void);
    void getQCGDEFCONT(void);
    bool setQCGDEFCONT(uint8_t type);
    uint8_t getQNBIOTRAI(void);
    bool setQNBIOTRAI(uint8_t rai);
    void getQNBIOTEVENT(void);
    bool setQNBIOTEVENT(uint8_t enable);
    uint8_t getQATWAKEUP(void);
    bool setQATWAKEUP(uint8_t enable);
    String getQENG(uint8_t mode);
    String getQCCID(void);
    uint8_t getQPOWD(void);
    bool setQPOWD(uint8_t op);
    uint8_t getQSCLK(void);
    bool setQSCLK(uint8_t mode);
    bool setQFOTADL(String url);
    uint8_t getQGNSSC(void);
    bool setQGNSSC(uint8_t mode);
    uint8_t getQGNSSRD(void);
    bool getQGNSSRD(String sth);

    bool setCmdEcho(String str);
    bool saveConfig(void);
    bool setBaudrate(unsigned long baud);
    unsigned long readBaudrate(void);
    
    
    void ConfigMQTT(void);
    bool setMQTTVersion(uint8_t version);
    void getMQTTParameter(void);
    bool OpenMQTT(uint8_t ConnectID, String Server, uint16_t Port);
    bool ReadMQTT(void);
    bool CheckMQTT(uint8_t ConnectID);
    bool CloseMQTT(uint8_t ConnectID);
    bool setServer(String IPAddress,uint16_t port,uint8_t ConnectID = 0);
    bool connected();
    bool ConnectServer(uint8_t ConnectID, String ClientID, String UserName, String PassWord);
    bool connect(String ClientID, String username,String password,uint8_t ConnectID = 0);
    bool connect(char * ClientID, char * username,char * password,uint8_t ConnectID = 0);
    bool getQMTCONN(void);
    bool DisConnectServer(String ConnectID);
    bool SubTopic(uint8_t ConnectID, uint8_t msgID, String topic, uint8_t qos);
    bool UnSubTopic(String ConnectID, String msgID, String topic);
    void publish(String topic,String msg);
    bool PubMessage(uint8_t ConnectID,uint8_t msgID, uint8_t qos, uint8_t retain, String topic, String msg);
    void setCallback(void (*call)(char*, uint8_t*, unsigned int));
    void loop();
    bool ConfigSleepMode(eSleepMode_t mode);
    bool BC20WakeUp();
    void setTAUTime(uint8_t time, eTAUunit_t unit);
    void setActiveTime(uint8_t time, eAcTunit_t unit);
    bool setPSMMode(ePSM_t status);
    bool seteDRX(uint8_t mode);// OFF = 0 // ON = 1
    bool seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested);
    bool seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested, uint8_t NW);
    bool seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested, uint8_t NW,uint8_t PTW);
    bool setNBEventReport(bool mode);
    void setPSMCallback(void (*call)(void));
	
    void publish(String topic,uint8_t *msg,uint16_t len);
	bool PubMessage(uint8_t ConnectID, uint8_t msgID, uint8_t qos, uint8_t retain, String topic, uint8_t *msg, uint16_t len);	
	
  private:
    void (*callback)(char*, uint8_t*, unsigned int);
    void (*PSMcallback)(void);
    String _TAUValue = "";
    String _AcTValue = "";
};

#endif
