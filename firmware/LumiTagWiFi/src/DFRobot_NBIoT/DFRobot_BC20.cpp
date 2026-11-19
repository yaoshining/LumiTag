#include "DFRobot_BC20.h"
#include "DFRobot_queue.h"
#include "hardwareSerial.h"

#define POWERKEY    15  // io 15
#define SETKEY      13
#define PSW_EINT    5   // io 5
String A="";

sSIMCard_t sSIMCard;
sRecData_t sRecData;
sPII_t sPII;
sMI_t sMI;
sSQ_t sSQ;
sESQ_t sESQ;
sGSN_t sGSN;
sGLL_t sGLL;
sGGA_t sGGA;
sGSA_t sGSA;
sRMC_t sRMC;
sVTG_t sVTG;
sCLK_t sCLK;
sGGNS_t sGGNS;
sSAT_t sSAT;
GsvSnapshot gsvSnapshot;
sCEREG_t sCEREG;
static uint8_t getNum(String str){
    if(str.equals(NMEA_RMC)){
        return 0;
    }else if(str.equals(NMEA_VTG)){
        return 1;
    }else if(str.equals(NMEA_GGA)){
        return 2;
    }else if(str.equals(NMEA_GSA)){
        return 3;
    }else if(str.equals(NMEA_GSV)){
        return 4;
    }else if(str.equals(NMEA_GLL)){
        return 5;
    }else{
        return 255;
    }
}
static String RemoveSthString(String sth, String str){
    String ret = str;
    int tempInt = 0;
    tempInt = ret.indexOf(sth);
    if(tempInt != -1){
        ret = ret.substring(tempInt+sth.length(),ret.length());
    }
    return ret;
}

static String GetSthfrontString(String sth, String str){/*返回给定字符串前面的字符串*/
    String ret = str;
    String temp = sth;
    uint8_t tempInt = 0;
    tempInt = ret.indexOf(temp);
    if(tempInt != -1){
        ret = ret.substring(0,tempInt);
    }
    return ret;
}
static void SendATCMD(String str){
    if(str.length() > 0){
        BC20Serial.println("AT+"+str);
    }else{
        BC20Serial.println("AT"+str);
    }
}

static void SendATCMD(String str,uint8_t num){
    BC20Serial.print("AT+"+str+"=");
    BC20Serial.println(num);
}

static void SendATCMD(String str,String cmd){
    BC20Serial.print("AT+"+str+"=");
    BC20Serial.println(cmd);
}

static void ReceviceATCMD(uint32_t timeout){/*接收BC20返回的命令数据*/
    String tempData="";
    int tempInt;
    uint8_t tempNum = 0;
    uint32_t nowtime = millis();
    while(millis() - nowtime < timeout){
        while(BC20Serial.available() > 0){
            tempData += (char)BC20Serial.read();
            //delay(1);
        }
        //if((tempData.indexOf("OK") != -1) || (tempData.indexOf("ERROR") != -1)){
        if((tempData.indexOf("OK") != -1)){
            break;
        }
    }
    if(tempData.length() > 0){
        //DBG(tempData);
        if(tempData.indexOf("+QMTRECV: ")!=-1){
            A=tempData;
        }   
        do{
            tempInt = tempData.indexOf("\r\n");
            if(tempInt != -1){
                cuappEnqueue((uint8_t *)((tempData.substring(0,tempInt+2)).c_str()),tempInt+2,tempNum);
                tempData = tempData.substring(tempInt + 2, tempData.length());
                //DBG(tempData);
                //delay(100);
                tempNum ++;
            }else{
                if(tempData.length() > 0){/*显示出BC20返回的数据*/
                    //DBG("ERROR");
                    //DBG(tempData);
                    //Serial.println(tempData);
                }
            }
        }while(tempInt >= 0);
        sRecData.dataNum = tempNum-1;
    }
}

static bool CheckRecData(String str){/*检查接收到队列里的数据中是否有相应的字符串*/
    bool ret = false;
    static struct sQueueData *p = NULL;
    p = cuappDequeue(); 
    while(p != NULL){
       if(strncmp((const char* )p->data,str.c_str(),str.length()) == 0){
            ret = true;
       }
       free(p);
       p = cuappDequeue();
       
    }
    return ret;
}

static void flushBC20Serial(void){/*清除BC20串口未处理的数据和接收队列的数据*/
    ReceviceATCMD(1);
    CheckRecData("");
}

static void getRecData(uint8_t INFO){/*从返回数据中获取某一个参数信息。如SIM卡的CIMI号*/
    uint8_t *tempData;
    static struct sQueueData *p;
    tempData = (uint8_t *)malloc(1);
    do{
        p = cuappDequeue();
        if(strncmp((const char* )p->data,"\r\n",2) == 0){
            free(p);
            break;
        }
        if(tempData != NULL){
            free(tempData);
            tempData = NULL;
        }
        tempData = (uint8_t *)malloc((p->len)+1);
        memset(tempData,'\0',p->len+1);
        memcpy(tempData,p->data,p->len);
        free(p);
    }while(p != NULL);
    
    switch(INFO){
        case CIMI_INFO:
            if(sSIMCard.IMEI_NUM == NULL){
                sSIMCard.IMEI_NUM = (uint8_t *)malloc(15+1);
            }else{
                free(sSIMCard.IMEI_NUM);
                sSIMCard.IMEI_NUM = (uint8_t *)malloc(15+1);
            }
            memset((sSIMCard.IMEI_NUM),'\0',16);
            memcpy((sSIMCard.IMEI_NUM), tempData, 15);
            //DBG((const char *)(sSIMCard.IMEI_NUM));
            break;
        default:
            break;
    }
    free(tempData);
    flushBC20Serial();
}
static uint8_t getTailHandle(){
    static struct sQueueData *p = NULL;
    p = getQueueTail();
    if(p != NULL){
        return p->handle;
    }
    return 0;
}
static void getRecDataforNum(uint8_t num, uint8_t* &buf){/*获取接收数据的第几条数据*/
    static struct sQueueData *p = NULL;
    p = getQueueTail();
    if(p != NULL && strncmp((const char* )p->data,"OK\r\n",4) == 0){/*收到完整的AT回复*/
        p = getQueueHead();
        if(p == NULL){
            return;
        }
        /*
        if(strncmp((const char* )p->data,"AT",2) == 0){//设置了命令回显
            num = num + 1;
        }*/
        for(uint8_t i = 1; i < num; i++){
            p = p->next;
            if(p == NULL){
                return;
            }
        }
        if(p != NULL){
            //DBG((const char *)(p->data));
            if(buf != NULL){
                free(buf);
            }
            buf = (uint8_t *)malloc((p->len) +1);
            memset(buf,'\0',(p->len)+1);
            memcpy(buf,p->data,p->len);
        }
    }
}

static void getRecDataforNum_NoCheck(uint8_t num, uint8_t* &buf){/*获取接收数据的第几条数据*/
    static struct sQueueData *p = NULL;
    p = getQueueHead();
    if(p == NULL){
        return;
    }
    for(uint8_t i = 1; i < num; i++){
        p = p->next;
        if(p == NULL){
            return;
        }
    }
    if(p != NULL){
        if(buf != NULL){
            free(buf);
            buf = NULL;
        }
        buf = (uint8_t *)malloc((p->len) +1);
        memset(buf,'\0',(p->len)+1);
        memcpy(buf,p->data,p->len);
    }
}

static uint8_t getIntforString(String CMD,String basic,uint8_t n){/*发送命令并获取返回的命令中指定的数字*/
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(CMD);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    if (data == NULL) return 255;
    tempStr  = (char *)data;
    free(data);
    if (tempStr.indexOf("+"+basic+": ") < 0) return 255;
    if( n == 1){
        tempStr  = RemoveSthString("+"+basic+": ",tempStr);
        String value = GetSthfrontString("\r\n",tempStr);
        if (value.length() == 0) return 255;
        for (unsigned int i = 0; i < value.length(); ++i) {
            if (!isDigit(value[i])) return 255;
        }
        return value.toInt();
    }else{
        return 0;
    }
}

static void DBGPrintRecData(void){
    static struct sQueueData *p = NULL;
    uint8_t tempNum = 0;
}


DFRobot_BC20::DFRobot_BC20(void){
    BC20Serial.begin(115200);
    pinMode(PSW_EINT,OUTPUT);
    pinMode(POWERKEY,OUTPUT);
    digitalWrite(POWERKEY, LOW);
    //digitalWrite(PSW_EINT,LOW);
}

DFRobot_BC20::~DFRobot_BC20(void){
    
}

bool DFRobot_BC20::powerOn(void){
    flushBC20Serial();
    if(checkBC20()){
        return true;
    }
    //pinMode(PSW_EINT,OUTPUT);
    //pinMode(POWERKEY,OUTPUT);
    digitalWrite(POWERKEY, LOW);
    digitalWrite(POWERKEY, HIGH);
    delay(800);
    digitalWrite(POWERKEY, LOW);
    delay(100);
    //BC20Serial.begin(115200);
    /*if(checkBC20()){
        return true;
    }*/
    return false;
}

bool DFRobot_BC20::configKeepalive(char* alivetime){
	BC20Serial.print("AT+QMTCFG=\"KEEPALIVE\",0,");
	BC20Serial.println(alivetime);
	ReceviceATCMD(300);
	return CheckRecData(AT_OK);
}

bool DFRobot_BC20::checkBC20(void){
    SendATCMD(CHECK_BC20);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::checkNBCard(void){
    bool ret = false;
    SendATCMD(CHECK_NB_CARD);
    ReceviceATCMD(300);
    ret = CheckRecData(AT_OK);
    if(!ret){
        Rest();
        delay(1000);
    }
    return ret;
}

bool DFRobot_BC20::setCmdEcho(String str){/*设置AT命令是否回显*/
    BC20Serial.println(str);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: setBaudrate(unsigned long baud){/*设置BC20的UART的波特率*/
    BC20Serial.print("AT+IPR=");
    BC20Serial.println(baud);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

unsigned long DFRobot_BC20 :: readBaudrate(){/*读取BC20的UART的波特率*/
    unsigned long ret=0;
    SendATCMD(READ_BAUD);
    ReceviceATCMD(300);
    uint8_t *baud;
    baud = (uint8_t *)malloc(1);
    getRecDataforNum(2,baud);
    //DBG((const char *)baud);
    uint8_t i = 0,k = 0;
    while((baud[i] != '\r') && (i < 15)){
        i++;
    }
    for(uint8_t j = i-1; j > 0; j--){
        if((baud[j] > '9') || (baud[j] < '0')){
            break;
        }
        ret = ret + ((baud[j] - '0')*(pow(10,k)) );
        k++;
        
    }
    free(baud);
    return ret;
}

bool DFRobot_BC20 :: saveConfig(){/*掉电保存当前配置*/
    BC20Serial.println("AT&W0");
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getPII(void){
    BC20Serial.println(GET_PII);
    ReceviceATCMD(300);
    getRecDataforNum(2,sPII.company);
    getRecDataforNum(3,sPII.deviceType);
    getRecDataforNum(4,sPII.Revision);
    flushBC20Serial();
    /*
    DBG((const char *)sPII.company);
    DBG((const char *)sPII.deviceType);
    DBG((const char *)sPII.Revision);*/
}

void DFRobot_BC20 :: getMI(void){
    SendATCMD(GET_MI);
    ReceviceATCMD(300);
    getRecDataforNum(2,sMI.company);
    getRecDataforNum(3,sMI.deviceType);
    getRecDataforNum(4,sMI.Revision);
    flushBC20Serial();
    /*
    DBG((const char *)sMI.company);
    DBG((const char *)sMI.deviceType);
    DBG((const char *)sMI.Revision);*/
}

void DFRobot_BC20 :: getMM(void){
    SendATCMD(GET_MM);
    ReceviceATCMD(300);
    getRecDataforNum(2,sPII.deviceType);
    flushBC20Serial();
    //DBG((const char *)sPII.deviceType);
}

void DFRobot_BC20 :: getMR(void){
    SendATCMD(GET_MR);
    ReceviceATCMD(300);
    getRecDataforNum(2,sPII.Revision);
    flushBC20Serial();
    //DBG((const char *)sPII.Revision);
}

void DFRobot_BC20 :: getSQ(void){
    uint8_t *data=NULL;
    int tempInt = 0;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_SQ);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }else{
        return;
    }
    tempStr  = RemoveSthString("+CSQ: ",tempStr);
    sSQ.rssi = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);
    sSQ.ber  = GetSthfrontString("\r\n",tempStr).toInt();
    //DBG(sSQ.rssi);
    //DBG(sSQ.ber);
    flushBC20Serial();
}

void DFRobot_BC20 :: getESQ(void){
    uint8_t *data=NULL;
    int tempInt = 0;
    String tempStr="";
    SendATCMD(GET_ESQ);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+CESQ: ",tempStr);

    sESQ.rxlev = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    sESQ.ber = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    sESQ.rscp = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    sESQ.ecno = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    sESQ.rsrq = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    sESQ.rsrp = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);

    //DBG(sESQ.rxlev);
    //DBG(sESQ.ber);
    //DBG(sESQ.rscp);
    //DBG(sESQ.ecno);
    //DBG(sESQ.rsrq);
    //DBG(sESQ.rsrp);
    flushBC20Serial();
}

void DFRobot_BC20 :: getGSN(uint8_t cmd){
    uint8_t *data=NULL;
    int tempInt = 0;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_GSN,cmd);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    switch(cmd){
        case 0:
            sGSN.sn = tempStr;
            //DBG(sGSN.sn);
            break;
        case 1:
            tempStr  = RemoveSthString("+CGSN: ",tempStr);
            tempStr = GetSthfrontString("\r\n",tempStr);
            sGSN.imei = tempStr;
            //DBG(sGSN.imei);
            break;
        case 2:
            tempStr  = RemoveSthString("+CGSN: ",tempStr);
            tempStr = GetSthfrontString("\r\n",tempStr);
            sGSN.imeisv = tempStr;
            //DBG(sGSN.imeisv);
            break;
        case 3:
            tempStr  = RemoveSthString("+CGSN: ",tempStr);
            tempStr = GetSthfrontString("\r\n",tempStr);
            sGSN.svn = tempStr;
            //DBG(sGSN.svn);
        default:
            break;
    }
}

static uint8_t ConvertUint8(String num, uint8_t bit){
    uint8_t ret = 0;
    String temp = "";
    for(uint8_t i = bit; i > 0; i--){
        temp = num.substring(i-1,i);
        if(temp.toInt() == 1){
            ret |= 1<<(bit-i);
        }else{
        }
    }
    return ret;
}

void DFRobot_BC20 :: getEREG(void){/*待完善*/
    uint8_t *data=NULL;
    String tempStr="";
    String ActTime="";
    String PerTau="";
    flushBC20Serial();
    SendATCMD(GET_EREG);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+CEREG: ",tempStr);
    uint8_t temp  = GetSthfrontString(",",tempStr).toInt();
    if(temp == 4){
        tempStr  = RemoveSthString(",",tempStr);/*4*/
        tempStr  = RemoveSthString(",",tempStr);/*1*/
        tempStr  = RemoveSthString(",",tempStr);/*"5A10"*/
        tempStr  = RemoveSthString(",",tempStr);/*"0CFD33A0"*/
        tempStr  = RemoveSthString(",",tempStr);/*9*/
        tempStr  = RemoveSthString(",",tempStr);/* */
        tempStr  = RemoveSthString(",",tempStr);/* */
        ActTime = tempStr;
        ActTime = RemoveSthString("\"",ActTime);
        ActTime = GetSthfrontString("\"",ActTime);
        sCEREG.ActiveTime = ConvertUint8(ActTime.substring(3,ActTime.length()),5);
        temp = ConvertUint8(ActTime.substring(0,3),3);
        if(temp == 0){
            sCEREG.ActiveUint = "2 Seconds";
        }else if(temp == 0x01){
            sCEREG.ActiveUint = "1 Minute";
        }else if(temp == 0x02){
            sCEREG.ActiveUint = "Decihours";
        }else{
            sCEREG.ActiveUint = "Deactivated";
        }
        tempStr  = RemoveSthString(",",tempStr);/*"11100000"*/
        PerTau = tempStr;
        PerTau = RemoveSthString("\"",PerTau);
        PerTau = GetSthfrontString("\"",PerTau);
        sCEREG.TAUTime = ConvertUint8(PerTau.substring(3,ActTime.length()),5);
        temp = ConvertUint8(PerTau.substring(0,3),3);
        switch(temp){
            case 0x00:
                sCEREG.TAUUint = "10 Minutes";
                break;
            case 0x01:
                sCEREG.TAUUint = "1 Hour";
                break;
            case 0x02:
                sCEREG.TAUUint = "10 Hour";
                break;
            case 0x03:
                sCEREG.TAUUint = "2 Seconds";
                break;
            case 0x04:
                sCEREG.TAUUint = "30 Seconds";
                break;
            case 0x05:
                sCEREG.TAUUint = "1 Minute";
                break;
            case 0x06:
                sCEREG.TAUUint = "320 Hours";
                break;
            case 0x07:
                sCEREG.TAUUint = "Deactivated";
                break;
            default:
                break;
        }
    }
}

bool DFRobot_BC20 :: setEREG(uint8_t cmd){/*待完善*/
    flushBC20Serial();
    SendATCMD(SET_EREG,cmd);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getSCON(void){
    flushBC20Serial();
    SendATCMD(GET_SCON);
    ReceviceATCMD(300);
}

/*!
 * @param cmd
 *     0 Disable unsolicited result code
 *     1 Enable unsolicited result code: “+CSCON:<mode>”
 * return true or false
 */
bool DFRobot_BC20 :: setSCON(uint8_t cmd){
    flushBC20Serial();
    SendATCMD(SET_SCON,cmd);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getOPS(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_OPS);
    ReceviceATCMD(630000);
}

bool DFRobot_BC20 :: setOPS(uint8_t cmd){/*待定*/
    flushBC20Serial();
    SendATCMD(SET_OPS,cmd);
    ReceviceATCMD(630000);
    return CheckRecData(AT_OK);
}

/*!
 * @return Indicates the state of PDP context activation.
 *     0 Detached
 *     1 Attached
 */
uint8_t DFRobot_BC20 :: getGATT(void){
    return getIntforString(GET_GATT,SET_GATT,1);
}

/*!
 * @param state
 *     0 Detached
 *     1 Attached
 * return true or false
 */
bool DFRobot_BC20 :: setGATT(uint8_t state){
    flushBC20Serial();
    SendATCMD(SET_GATT,state);
    ReceviceATCMD(75000);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getGDCONT(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_GDCONT);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setGDCONT(uint8_t state){/*待定*/
    flushBC20Serial();
    SendATCMD(SET_GDCONT,state);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getGACT(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_GACT);
    ReceviceATCMD(150000);
}

bool DFRobot_BC20 :: setGACT(uint8_t state){/*待定*/
    flushBC20Serial();
    SendATCMD(SET_GACT,state);
    ReceviceATCMD(150000);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getGPADDR(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_GPADDR);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setGPADDR(uint8_t state){
    return CheckRecData(AT_OK);
}

uint8_t * DFRobot_BC20 :: getIMI(void){
    flushBC20Serial();
    SendATCMD(GET_IMI);
    ReceviceATCMD(300);
    getRecData(CIMI_INFO);
    return (uint8_t *)sSIMCard.IMEI_NUM;
}

/*!
 * @return
 *     0 Minimum functionality
 *     1 Full functionality
 *     4 Disable the UE from both transmitting and receiving RF signals
 */
uint8_t DFRobot_BC20 :: getFUN(void){
    return getIntforString(GET_FUN,SET_FUN,1);
}

/*!
 * @param fun
 *     0 Minimum functionality
 *     1 Full functionality
 *     4 Disable the UE from both transmitting and receiving RF signals
 * @return true or false
 */
bool DFRobot_BC20 :: setFUN(uint8_t fun){
    flushBC20Serial();
    SendATCMD(SET_FUN,fun);
    ReceviceATCMD(15000);
    return CheckRecData(AT_OK);
}

/*!
 * @return
 *     0 Disable result code
 *     1 Enable result code and use numeric values
 *     2 Enable result code and use verbose values
 */
uint8_t DFRobot_BC20 :: getMEE(void){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_MEE);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+CMEE: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

/*!
 * @param n
 *     0 Disable result code
 *     1 Enable result code and use numeric values
 *     2 Enable result code and use verbose values
 * @return true or false
 */
bool DFRobot_BC20 :: setMEE(uint8_t n){
    flushBC20Serial();
    SendATCMD(SET_MEE,n);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

/*!
 * @return time
 * The format is "yy/MM/dd,hh:mm:ss±zz",
 */
String DFRobot_BC20 :: getCLK(void){
    uint8_t *data=NULL;
    String tempStr="";
    String retStr = "";
    flushBC20Serial();
    SendATCMD(GET_CLK);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+CCLK: ",tempStr);
    retStr = tempStr;
    sCLK.Year = GetSthfrontString("/",tempStr).toInt();
    tempStr  = RemoveSthString("/",tempStr);
    sCLK.Month = GetSthfrontString("/",tempStr).toInt();
    tempStr  = RemoveSthString("/",tempStr);
    sCLK.Day = GetSthfrontString(",",tempStr).toInt();
    tempStr  = RemoveSthString(",",tempStr);
    sCLK.Hour = GetSthfrontString(":",tempStr).toInt();
    tempStr  = RemoveSthString(":",tempStr);
    sCLK.Minute = GetSthfrontString(":",tempStr).toInt();
    tempStr  = RemoveSthString(":",tempStr);
    sCLK.Second = GetSthfrontString("GMT",tempStr).toInt();
    tempStr  = RemoveSthString("GMT",tempStr);
    sCLK.TimeEquation = tempStr.toInt();
    sCLK.Hour = sCLK.Hour+sCLK.TimeEquation;
    tempStr = GetSthfrontString("\r\n",tempStr);
    sCLK.Mode = "GMT" + tempStr;
    struct tm timeinfo;
    timeinfo.tm_hour = sCLK.Hour;
    timeinfo.tm_min  = sCLK.Minute;
    timeinfo.tm_sec  = sCLK.Second;
    timeinfo.tm_year = sCLK.Year;
    timeinfo.tm_mon  = sCLK.Month;
    timeinfo.tm_mday = sCLK.Day;
    mktime(&timeinfo);
    sCLK.Year = timeinfo.tm_year;
    sCLK.Month = timeinfo.tm_mon;
    sCLK.Day = timeinfo.tm_mday;
    sCLK.Hour = timeinfo.tm_hour;
    sCLK.Minute = timeinfo.tm_min;
    sCLK.Second = timeinfo.tm_sec;
    return retStr;
}

void DFRobot_BC20 :: getBC(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_BC);
    ReceviceATCMD(300);
}

void DFRobot_BC20 :: getPSMS(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_PSMS);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setPSMS(uint8_t mode){/*待定*/
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getEDRXS(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_EDRXS);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setEDRXS(uint8_t mode){/*待定*/
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getEDRXRDP(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_EDRXRDP);
    ReceviceATCMD(300);
}

/*!
 * @return
 *     0 Disable time zone change event reporting
 *     1 Enable time zone change event reporting by unsolicited result code “+CTZV: <tz>”.
 */
uint8_t DFRobot_BC20 :: getTZR(void){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_TZR);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+CTZR: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

/*!
 * @param onoff
 *     0 Disable time zone change event reporting
 *     1 Enable time zone change event reporting by unsolicited result code “+CTZV: <tz>”.
 * @return true or false.
 */
bool DFRobot_BC20 :: setTZR(uint8_t onoff){
    flushBC20Serial();
    SendATCMD(SET_TZR,onoff);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getIPCA(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_IPCA);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setIPCA(uint8_t n){/*待定*/
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getCIOTOPT(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_CIOTOPT);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setCIOTOPT(uint8_t n){/*待定*/
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: setGAPNRC(uint8_t n){/*待定*/
    return CheckRecData(AT_OK);
}

String DFRobot_BC20 :: getEER(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_EER);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+CEER: ",tempStr);
    return tempStr;
}

bool DFRobot_BC20 :: setQGACT(uint8_t op){/*待定*/
    return CheckRecData(AT_OK);
}

/*!
 * @return Operating Band.
 * Integer value indicating the band(s) being used.
 * Valid values: 1, 2, 3, 5, 8, 12, 13, 17, 18, 19, 20, 25, 26, 28, 66
 */
uint8_t DFRobot_BC20 :: getQBAND(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_QBAND);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+QBAND: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

bool DFRobot_BC20 :: setQBAND(uint8_t n){/*待定*/
    return CheckRecData(AT_OK);
}
/*!
 * @return true or false.
 */
bool DFRobot_BC20 :: Rest(uint8_t mode){
    flushBC20Serial();
    //BC20Serial.println("AT+QRST=1");
    SendATCMD(SET_QRST,mode);
    ReceviceATCMD(300);
    delay(1000);
    return true;
    //return CheckRecData(AT_OK);
}
/*!
 * @return Integer
 *     0 Old algorithm
 *     1 New algorithm
 */
uint8_t DFRobot_BC20 :: getQSPCHSC(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_QSPCHSC);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+QSPCHSC: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}
/*!
 * @param mode
 *     0 Old algorithm
 *     1 New algorithm
 * @return true or false.
 */
bool DFRobot_BC20 :: setQSPCHSC(uint8_t mode){
    flushBC20Serial();
    SendATCMD(SET_QSPCHSC,mode);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getQLOCKF(void){/*待定*/

}

bool DFRobot_BC20 :: setQLOCKF(uint8_t mode){/*待定*/
    return CheckRecData(AT_OK);
}

/*!
 * @return Integer value indicating the EARFCN has been cleared
 *     0 Successful
 *     2 Failure
 *     3 Not found
 */
uint8_t DFRobot_BC20 :: getQCSEARFCN(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_QCSEARFCN);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+QCSEARFCN: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

void DFRobot_BC20 :: getQCGDEFCONT(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_QCGDEFCONT);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setQCGDEFCONT(uint8_t type){/*待定*/
    return CheckRecData(AT_OK);
}

/*!
 * @return
 *     0 No information available (or none of the other options apply)
 *     1 TE will send only 1 UL packet and no DL packets expected
 *     2 TE will send only 1 UL packet and only 1 DL packet expected
 */
uint8_t DFRobot_BC20 :: getQNBIOTRAI(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_QNBIOTRAI);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+QNBIOTRAI: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

/*!
 * @param rai
 *     0 No information available (or none of the other options apply)
 *     1 TE will send only 1 UL packet and no DL packets expected
 *     2 TE will send only 1 UL packet and only 1 DL packet expected
 * @return true or false.
 */
bool DFRobot_BC20 :: setQNBIOTRAI(uint8_t rai){
    flushBC20Serial();
    SendATCMD(SET_QNBIOTRAI,rai);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getQNBIOTEVENT(void){/*待定*/
    flushBC20Serial();
    SendATCMD(GET_QNBIOTEVENT);
    ReceviceATCMD(300);
}

bool DFRobot_BC20 :: setQNBIOTEVENT(uint8_t enable){/*待定*/
    return CheckRecData(AT_OK);
}

/*!
 * @return
 *     0 Disable indication on this channel when modem wakes up from deep sleep
 *     1 Enable indication on this channel when modem wakes up from deep sleep
 */
uint8_t DFRobot_BC20 :: getQATWAKEUP(void){
    uint8_t *data=NULL;
    String tempStr = "";
    flushBC20Serial();
    SendATCMD(GET_QATWAKEUP);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr = RemoveSthString("+QATWAKEUP: ",tempStr);
    return GetSthfrontString("\r\n",tempStr).toInt();
}

/*!
 * @param enable
 *     0 Disable indication on this channel when modem wakes up from deep sleep
 *     1 Enable indication on this channel when modem wakes up from deep sleep
 * @return true or false.
 */
bool DFRobot_BC20 :: setQATWAKEUP(uint8_t enable){
    flushBC20Serial();
    SendATCMD(SET_QATWAKEUP,enable);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

/*!
 * @param mode
 *     0 Display radio information for serving and neighbor cells
 *     1 Display data transfer information only if modem in RRC-CONNECTED state
 * @return String.
 */
String DFRobot_BC20 :: getQENG(uint8_t mode){
    uint8_t *data0=NULL,*data1=NULL;
    String tempStr0 = "",tempStr1 = "";
    flushBC20Serial();
    SendATCMD(GET_QENG,mode);
    ReceviceATCMD(300);
    getRecDataforNum(2,data0);
    tempStr0 = (char *)data0;
    tempStr0 = RemoveSthString("+QENG: ",tempStr0);
    if(data0 != NULL){
        free(data0);
    }
    getRecDataforNum(3,data1);
    tempStr1 = (char *)data1;
    tempStr1 = RemoveSthString("+QENG: ",tempStr1);
    if(data1 != NULL){
        free(data1);
    }
    return tempStr0+tempStr1;
}

/*!
 * @return USIM card identification number
 */
String DFRobot_BC20 :: getQCCID(void){
    uint8_t *data=NULL;
    String CCID = "";
    flushBC20Serial();
    SendATCMD(GET_QCCID);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    CCID = (char *)data;
    CCID = RemoveSthString("+QCCID: ",CCID);
    CCID = GetSthfrontString("\r\n",CCID);
    if(data != NULL){
        free(data);
    }
    return CCID;
}

/*!
 * @return
 *     0 Power off the module
 */
uint8_t DFRobot_BC20 :: getQPOWD(void){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_QPOWD);
    ReceviceATCMD(1000);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+QPOWD: (",tempStr);
    return GetSthfrontString(")",tempStr).toInt();
}

/*!
 * @param op
 *     0 Power off the module.
 * @return true or false.
 */
bool DFRobot_BC20 :: setQPOWD(uint8_t op){
    flushBC20Serial();
    SendATCMD(SET_QPOWD,op);
    ReceviceATCMD(1000);
    return CheckRecData(AT_OK);
}

/*!
 * @return
 *     0 Disable Sleep Mode
 *     1 Enable light sleep and deep sleep, wakeup by PSM_EINT (Falling Edge)
 *     2 Enable light sleep only, wakeup by Main UART
 */
uint8_t DFRobot_BC20 :: getQSCLK(void){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_QSCLK);
    ReceviceATCMD(300);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    if(data != NULL){
        free(data);
    }
    tempStr  = RemoveSthString("+QSCLK: ",tempStr);
    
    return GetSthfrontString("\r\n",tempStr).toInt();
}

/*!
 * @param mode
 *     0 Disable Sleep Mode.
 *     1 Enable light sleep and deep sleep, wakeup by PSM_EINT (Falling Edge).
 *     2 Enable light sleep only, wakeup by Main UART.
 * @return true or false.
 */
bool DFRobot_BC20 :: setQSCLK(uint8_t mode){
    flushBC20Serial();
    SendATCMD(SET_QSCLK,mode);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: setQFOTADL(String url){
    flushBC20Serial();
    BC20Serial.println("AT+QFOTADL=\""+url+"\"");
    return CheckRecData(AT_OK);
}


uint8_t DFRobot_BC20 :: getQGNSSC(void){
    return getIntforString(GET_QGNSSC,SET_QGNSSC,1);
}

bool DFRobot_BC20 :: setQGNSSC(uint8_t mode){
    flushBC20Serial();
    SendATCMD(SET_QGNSSC,mode);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}
static float Longitude_conversion(String str){
    float temp = 0;
    String tempStr = str;
    if(str.length() > 0){
        if(str == sRMC.LatitudeVal){
            temp = tempStr.substring(0,2).toInt();
            tempStr = tempStr.substring(2,tempStr.length());
            temp += (tempStr.substring(0,2).toInt())*10.0/60.0/10.0;
            tempStr = tempStr.substring(3,tempStr.length());
            temp += (tempStr.substring(0,4).toInt())*1.0/60.0/10000.0;
        }else if(str == sRMC.LongitudeVal){
            temp = tempStr.substring(0,3).toInt();
            tempStr = tempStr.substring(3,tempStr.length());
            temp += (tempStr.substring(0,2).toInt())*10.0/60.0/10.0;
            tempStr = tempStr.substring(3,tempStr.length());
            temp += (tempStr.substring(0,4).toInt())*1.0/60.0/10000.0;
        }else{
            
        }
    }
    return temp;
}
uint8_t DFRobot_BC20 :: getQGNSSRD(void){
    getQGNSSRD(NMEA_RMC);
    sGGNS.LatitudeVal = Longitude_conversion(sRMC.LatitudeVal);
    sGGNS.LatitudeDir = sRMC.LatitudeDir;
    sGGNS.LongitudeVal = Longitude_conversion(sRMC.LongitudeVal);
    sGGNS.LongitudeDir = sRMC.LongitudeDir;
    getQGNSSRD(NMEA_VTG);
    //sGGNS.Speed = GetSthfrontString(".",sVTG.GroundCourse_Kmh).toInt();
    sGGNS.Speed = atof(sVTG.GroundCourse_Kmh.c_str());
    sGGNS.Heading = atof(sVTG.GroundCourse_True.c_str());
    getQGNSSRD(NMEA_GGA);
    sGGNS.Altitude = atof(sGGA.Altitude.c_str());
    getQGNSSRD(NMEA_GSA);
    sGGNS.PDOP = sGSA.data[0].PDOP;
    sGGNS.HDOP = sGSA.data[0].HDOP;
    sGGNS.VDOP = sGSA.data[0].VDOP;
    if(sGSA.data[0].FixStatus == "2"){
        sGGNS.FixStatus = "2D fixed";
    }else if(sGSA.data[0].FixStatus == "3"){
        sGGNS.FixStatus = "3D fixed";
    }else{
        sGGNS.FixStatus = "No fix";
    }
    getQGNSSRD(NMEA_GSV);
    return 1;
}

static void CheckSatelliteUse(uint8_t num){
    uint8_t temp = 0;
    if(sSAT.data[num].SYS.equals("GPS")){//
        temp = 0;
    }else if(sSAT.data[num].SYS.equals("BeiDou")){
        temp = 1;
    }else{
        
    }
    for(uint8_t i =0; i < 12; i++){
        if(sSAT.data[num].PRN.equals(sGSA.data[temp].Statellite_CH[i])){
            sSAT.data[num].Status = "Y";
            return;
        }
    }
    sSAT.data[num].Status = "N";
    /*
    if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH1)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH2)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH3)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH4)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH5)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH6)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH7)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH8)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH9)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH10)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH11)){
        sSAT.data[num].Status = "Y";
    }else if(sSAT.data[num].PRN.equals(sGSA.data[num].Statellite_CH12)){
        sSAT.data[num].Status = "Y";
    }else{
        sSAT.data[num].Status = "N";
    }
    */
    /*
    if(sGSA.data[num].GNSS_SystemID.equals("1")){
        sSAT.data[num].SYS = "GPS";
    }else if(sGSA.data[num].GNSS_SystemID.equals("2")){
        sSAT.data[num].SYS = "GLONASS";
    }else if(sGSA.data[num].GNSS_SystemID.equals("3")){
        sSAT.data[num].SYS = "Galileo";
    }else if(sGSA.data[num].GNSS_SystemID.equals("4")){
        sSAT.data[num].SYS = "BeiDou";
    }else{
    }*/
}
static void GetSatelliteInformation(uint8_t start, uint8_t num, String str, String sys){//给定一条卫星数据，这条数据里是从第几个卫星开始的，有几个卫星数据
    String tempStr = str;
    for(uint8_t i = 0; i < num;i++){
        if(GetSthfrontString(",",tempStr).length() == 0){
            sSAT.data[i+start].PRN = "N/A";
        }else{
            sSAT.data[i+start].PRN = GetSthfrontString(",",tempStr);//卫星PRN码编号i+start
        }
        tempStr = RemoveSthString(",",tempStr);
        if(GetSthfrontString(",",tempStr).length() == 0){
            sSAT.data[i+start].Elev = "N/A";
        }else{
            sSAT.data[i+start].Elev = GetSthfrontString(",",tempStr);////卫星仰角i+start
        }
        tempStr = RemoveSthString(",",tempStr);
        if(GetSthfrontString(",",tempStr).length() == 0){
            sSAT.data[i+start].Azim = "N/A";
        }else{
            sSAT.data[i+start].Azim = GetSthfrontString(",",tempStr);//卫星方位角i+start
        }
        tempStr = RemoveSthString(",",tempStr);
        if(GetSthfrontString(",",tempStr).length() == 0){
            sSAT.data[i+start].SNR = "N/A";
        }else{
            sSAT.data[i+start].SNR = GetSthfrontString(",",tempStr);//信噪比i+start
        }
        sSAT.data[i+start].SYS = sys;
        tempStr = RemoveSthString(",",tempStr);
        CheckSatelliteUse(i+start);
    }
}

static void CheckUse(){//计算正在使用的数量
    sSAT.USE = 0x00;
    for(uint8_t i =0; i < sGSA.NUM;i++){
        for(uint8_t j =0; j < 12; j++){
            if((sGSA.data[i].Statellite_CH[j].length() > 0) &&(!sGSA.data[i].Statellite_CH[j].equals("N/A"))){
                sSAT.USE++;
            }
        }
    }
}

bool DFRobot_BC20 :: getQGNSSRD(String cmd){
    uint8_t *data=NULL;
    uint8_t NumSAT,NumSEN,StartNum,lastTotSEN,TotSEN = 0;
    uint8_t readnum = 2;
    String tempStr="";
    String timeStr = "";
    flushBC20Serial();
    SendATCMD(SET_QGNSSRD,cmd);
    ReceviceATCMD(5000);
    if(cmd.equals(NMEA_GSV)){
        gsvSnapshot = GsvSnapshot();
        struct sQueueData *tail = getQueueTail();
        if (tail == NULL || strncmp((const char *)tail->data, "OK\r\n", 4) != 0) return false;
        struct sQueueData *entry = getQueueHead();
        while (entry != NULL) {
            gsvSnapshot.add((const char *)entry->data);
            entry = entry->next;
        }
        return gsvSnapshot.groupCount > 0;
    }else{
        getRecDataforNum(2,data);
        if (data == NULL) return false;
        tempStr  = (char *)data;
        free(data);
        data = NULL;
        if (tempStr.indexOf("+QGNSSRD: ") < 0) return false;
        tempStr  = RemoveSthString("+QGNSSRD: ",tempStr);
        //DBG(tempStr);
        switch(getNum(cmd)){
            case 0:/*NMEA_RMC*/
                tempStr = RemoveSthString("$GNRMC,",tempStr);
                sRMC.UTC_Time = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.DataStatus = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.LatitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.LatitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.LongitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.LongitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.GroundSpeed = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.GroundHeading = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.UTC_Date = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.MagDeclination = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.MagDeclinationDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.PositioningMode = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sRMC.NaviStatus = GetSthfrontString("*",tempStr);
                timeStr = sRMC.UTC_Time;
                sCLK.Hour = timeStr.substring(0,2).toInt();
                timeStr = timeStr.substring(2,timeStr.length());
                sCLK.Minute = timeStr.substring(0,2).toInt();
                timeStr = timeStr.substring(2,timeStr.length());
                sCLK.Second = timeStr.substring(0,2).toInt();
                timeStr = sRMC.UTC_Date;
                sCLK.Day = timeStr.substring(0,2).toInt() ;
                timeStr = timeStr.substring(2,timeStr.length());
                sCLK.Month = timeStr.substring(0,2).toInt();
                timeStr = timeStr.substring(2,timeStr.length());
                sCLK.Year = timeStr.substring(0,2).toInt() + 2000;
                sCLK.Mode = "UTC+0";
                break;
            case 1:/*NMEA_VTG*/
                tempStr = RemoveSthString("$GNVTG,",tempStr);
                sVTG.GroundCourse_True = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("T,",tempStr);
                //tempStr = RemoveSthString(",",tempStr);
                sVTG.GroundCourse_Mag = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("M,",tempStr);
                //tempStr = RemoveSthString(",",tempStr);
                sVTG.GroundCourse_Knots = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("N,",tempStr);
                //tempStr = RemoveSthString(",",tempStr);
                sVTG.GroundCourse_Kmh = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("K,",tempStr);
                //tempStr = RemoveSthString(",",tempStr);
                sVTG.PositioningMode = GetSthfrontString("*",tempStr);
                break;
            case 2:/*NMEA_GGA*//*待定*/
                tempStr = RemoveSthString("$GNGGA,",tempStr);
                sGGA.UTC_Time= GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.LatitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.LatitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.LongitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.LongitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.Status = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.StatelliteNum = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.HDOP = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.Altitude = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("M,",tempStr);
                sGGA.GeoID_Separation = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString("M,",tempStr);
                sGGA.DGPS_Age = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGGA.DGPS_StationID = GetSthfrontString("*",tempStr);
                break;
            case 3:/*NMEA_GSA*/
                sGSA.NUM = 0;
                for(uint8_t i = 0; i < 4; i++){
                    if(tempStr.indexOf("$GNGSA,") != -1){
                        sGSA.NUM = i+1;
                        tempStr = RemoveSthString("$GNGSA,",tempStr);
                        sGSA.data[i].Mode = GetSthfrontString(",",tempStr);
                        tempStr = RemoveSthString(",",tempStr);
                        sGSA.data[i].FixStatus = GetSthfrontString(",",tempStr);
                        tempStr = RemoveSthString(",",tempStr);
                        for(uint8_t j = 0; j < 12; j++){
                            sGSA.data[i].Statellite_CH[j] = GetSthfrontString(",",tempStr);
                            tempStr = RemoveSthString(",",tempStr);
                        }
                        sGSA.data[i].PDOP = GetSthfrontString(",",tempStr);
                        tempStr = RemoveSthString(",",tempStr);
                        sGSA.data[i].HDOP = GetSthfrontString(",",tempStr);
                        tempStr = RemoveSthString(",",tempStr);
                        sGSA.data[i].VDOP = GetSthfrontString(",",tempStr);
                        tempStr = RemoveSthString(",",tempStr);
                        sGSA.data[i].GNSS_SystemID = GetSthfrontString("*",tempStr);
                        for(uint8_t j = 0; j < 12; j++){
                            if(sGSA.data[i].Statellite_CH[j].length() == 0){
                                sGSA.data[i].Statellite_CH[j] = "N/A";
                            }
                        }
                        getRecDataforNum(3+i,data);
                        tempStr  = (char *)data;
                        if(data != NULL){
                            free(data);
                            data = NULL;
                        }else{
                            break;
                        }
                    }else{
                        break;
                    }
                }
                CheckUse();
                break;
            /*case 4://NMEA_GSV//待定
                tempStr = RemoveSthString("$GNGSV,",tempStr);
                DBG(tempStr);
                sGLL.LatitudeVal = GetSthfrontString(",",tempStr);
                DBG(sGLL.LatitudeVal);
                break;*/
            case 5:/*NMEA_GLL*/
                tempStr = RemoveSthString("$GNGLL,",tempStr);
                sGLL.LatitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.LatitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.LongitudeVal = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.LongitudeDir = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.UTC_Time = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.DataStatus = GetSthfrontString(",",tempStr);
                tempStr = RemoveSthString(",",tempStr);
                sGLL.PositionMode = GetSthfrontString("*",tempStr);
                break;
            default:
                break;
        }
        
    }
    
    return 1;
}

void DFRobot_BC20 :: getGPS(void){
    BC20Serial.println("Hello World!");
}

void DFRobot_BC20 :: ConfigMQTT(void){
    flushBC20Serial();
}

bool DFRobot_BC20::setMQTTVersion(uint8_t version){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)GET_QMTCFG+"=");
    BC20Serial.print("\"version\",0,");
    BC20Serial.println(version);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: getMQTTParameter(void){
    flushBC20Serial();
    SendATCMD(TEST_QMTCFG);
    ReceviceATCMD(300);
    
}

bool DFRobot_BC20 :: OpenMQTT(uint8_t ConnectID, String Server, uint16_t Port){
    uint8_t *data=NULL;
    String tempStr="";
    uint16_t temptime = 0;
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTOPEN+"=");
    BC20Serial.print(ConnectID);
    BC20Serial.print(",\""+Server+"\",");
    BC20Serial.println(Port);
    ReceviceATCMD(300);
    if(CheckRecData(AT_OK)){
        while(1){
            tempStr = "";
            if(temptime > 250){
                return false;
            }
            ReceviceATCMD(300);
            temptime++;
            getRecDataforNum_NoCheck(2,data);
            CheckRecData("");
            tempStr  = (char *)data;
            //DBG(tempStr);
            if(data != NULL){
                free(data);
                data = NULL;
            }
            if(tempStr.equals("+QMTOPEN: 0,0\r\n")){
                return true;
            }
        }
    }
    return false;
}

bool DFRobot_BC20 :: ReadMQTT(void){
    flushBC20Serial();
    SendATCMD(GET_QMTOPEN);
    ReceviceATCMD(75000);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: CheckMQTT(uint8_t ConnectID){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    SendATCMD(GET_QMTOPEN);
    ReceviceATCMD(75000);
    getRecDataforNum(2,data);
    tempStr  = (char *)data;
    //DBG(tempStr);
    if(data != NULL){
        free(data);
        data = NULL;
    }
    if(tempStr.length() < 5){
        ReceviceATCMD(75000);
        getRecDataforNum(2,data);
        tempStr  = (char *)data;
        //DBG(tempStr);
        tempStr  = RemoveSthString("+QMTOPEN: ",tempStr);
        
    }
    return false;
}

bool DFRobot_BC20 :: CloseMQTT(uint8_t ConnectID){
    flushBC20Serial();
    SendATCMD(SET_QMTCLOSE,ConnectID);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}
bool DFRobot_BC20::connected(){
    return this->mqttConneced;
}
bool DFRobot_BC20 :: ConnectServer(uint8_t ConnectID, String ClientID, String UserName, String PassWord){
    uint8_t *data=NULL;
    String tempStr="";
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTCONN+"=");
    BC20Serial.print(ConnectID);
    BC20Serial.print(",\""+ClientID+"\"");
    BC20Serial.print(",\""+UserName+"\"");
    BC20Serial.println(",\""+PassWord+"\"");
    ReceviceATCMD(300);
    if(CheckRecData(AT_OK)){
        ReceviceATCMD(10000);
        getRecDataforNum_NoCheck(2,data);
        tempStr  = (char *)data;
        //DBG(tempStr);
        if(data != NULL){
            free(data);
            data = NULL;
        }
        tempStr = RemoveSthString(",",tempStr);
        tempStr = RemoveSthString(",",tempStr);
        if(tempStr.equals("0\r\n")){
            this->mqttConneced = true;
            return true;
        }
    }
    return false;
}

bool DFRobot_BC20 :: connect(String ClientID, String username,String password,uint8_t ConnectID){
    return ConnectServer(ConnectID,ClientID,username,password);

}

bool DFRobot_BC20 :: connect(char * ClientID, char * username,char * password,uint8_t ConnectID){
    String ClientID_str = "", username_str = "", password_str = "";
    ClientID_str = ClientID;
    username_str = username;
    password_str = password;
    return connect(ClientID_str,username_str,password_str,ConnectID);
}

bool DFRobot_BC20 :: setServer(String IPAddress, uint16_t port, uint8_t ConnectID){
    while(!OpenMQTT(ConnectID,IPAddress,port)){
        //Serial.println("Open MQTT Failed!");
        //Serial.println("ReadMQTT");
        if(ReadMQTT()){
            //Serial.println("Close MQTT!");
            CloseMQTT(0);
        }
        delay(1000);
    }
    return true;
}

bool DFRobot_BC20 :: getQMTCONN(){
    flushBC20Serial();
    SendATCMD(GET_QMTCONN);
    ReceviceATCMD(10000);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: DisConnectServer(String ConnectID){
    flushBC20Serial();
    SendATCMD(SET_QMTDISC,ConnectID);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: SubTopic(uint8_t ConnectID, uint8_t msgID, String topic, uint8_t qos){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTSUB+"=");
    BC20Serial.print(ConnectID);
    BC20Serial.print(",");
    BC20Serial.print(msgID);
    BC20Serial.print(",\""+topic+"\",");
    BC20Serial.println(qos);
    ReceviceATCMD(40000);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20 :: UnSubTopic(String ConnectID, String msgID, String topic){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTUNS+"=");
    BC20Serial.print(ConnectID+",");
    BC20Serial.print(msgID+",");
    BC20Serial.println("\""+topic+"\"");
    ReceviceATCMD(40000);
    return CheckRecData(AT_OK);
}
void DFRobot_BC20::publish(String topic,uint8_t *msg,uint16_t len){
    PubMessage(0,1,1,0,topic,msg,len);
}

bool DFRobot_BC20 :: PubMessage(uint8_t ConnectID, uint8_t msgID, uint8_t qos, uint8_t retain, String topic, uint8_t *msg, uint16_t len){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTPUB+"=");
    BC20Serial.print(ConnectID);
    BC20Serial.print(",");
    BC20Serial.print(msgID);
    BC20Serial.print(",");
    BC20Serial.print(qos);
    BC20Serial.print(",");
    BC20Serial.print(retain);
    BC20Serial.print(",");
    BC20Serial.println("\"" + topic + "\"");
    ReceviceATCMD(1000);
    for(int i = 0; i < len; i++){
        BC20Serial.print((char)msg[i]);
    }
    BC20Serial.print((char)0x1A);
    ReceviceATCMD(4000);
    delay(1000);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20::publish(String topic,String msg){
    PubMessage(0,1,1,0,topic,msg);
}
bool DFRobot_BC20 :: PubMessage(uint8_t ConnectID, uint8_t msgID, uint8_t qos, uint8_t retain, String topic, String msg){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_QMTPUB+"=");
    BC20Serial.print(ConnectID);
    BC20Serial.print(",");
    BC20Serial.print(msgID);
    BC20Serial.print(",");
    BC20Serial.print(qos);
    BC20Serial.print(",");
    BC20Serial.print(retain);
    BC20Serial.print(",");
    BC20Serial.print("\""+topic+"\",");
    BC20Serial.println("\""+msg+"\"");
    ReceviceATCMD(40000);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20 :: setCallback(void (*call)(char*, uint8_t*, unsigned int)){
    this->callback = call;
}

void DFRobot_BC20 :: loop(){
    String tempStr ="";
    uint8_t ConnectID,msgID;
    String Topic,RecData;    
    if(A!=""){
        tempStr =A;
        tempStr = RemoveSthString("+QMTRECV: ",tempStr);
        uint8_t ConnectID,msgID;
        String Topic,RecData;
        ConnectID = GetSthfrontString(",",tempStr).toInt();
        tempStr = RemoveSthString(",",tempStr);
        msgID = GetSthfrontString(",",tempStr).toInt();
        tempStr = RemoveSthString(",\"",tempStr);
        Topic = GetSthfrontString("\",",tempStr);
        tempStr = RemoveSthString("\",\"",tempStr);
        RecData = GetSthfrontString("\"\r\n",tempStr);
        if(callback){
            callback((char *)(Topic.c_str()),(uint8_t *)(RecData.c_str()),RecData.length());
        }
        A="";
    }
    if(BC20Serial.available() > 0){
        do{
            tempStr += char(BC20Serial.read());
            delay(1);
        }while(BC20Serial.available() > 0);
        if(tempStr.indexOf("+QMTRECV: ") != -1){
            tempStr = RemoveSthString("+QMTRECV: ",tempStr);
            ConnectID = GetSthfrontString(",",tempStr).toInt();
            tempStr = RemoveSthString(",",tempStr);
            msgID = GetSthfrontString(",",tempStr).toInt();
            tempStr = RemoveSthString(",\"",tempStr);
            Topic = GetSthfrontString("\",",tempStr);
            tempStr = RemoveSthString("\",\"",tempStr);
            RecData = GetSthfrontString("\"\r\n",tempStr);
            if(callback){
                callback((char *)(Topic.c_str()),(uint8_t *)(RecData.c_str()),RecData.length());
            }
        }else if(tempStr.indexOf("ENTER PSM") != -1){
            if(PSMcallback){
                PSMcallback();
            }
        }
    }
}

bool DFRobot_BC20::ConfigSleepMode(eSleepMode_t mode){
    return setQSCLK(mode);
}

bool DFRobot_BC20::BC20WakeUp(){
    flushBC20Serial();
    if(checkBC20()){
        return true;
    }else{
        digitalWrite(PSW_EINT, HIGH);
        delay(100);
        digitalWrite(PSW_EINT, LOW);
        delay(100);
    }
    return false;
}

static String ConvertString(uint8_t num, uint8_t bit){
    String ret = "";
    for(int i = bit; i > 0; i--){
        if(num & (0x01<<i-1)){
            ret += '1';
        }else{
            ret += '0';
        }
    }
    return ret;
}


void DFRobot_BC20::setTAUTime(uint8_t time, eTAUunit_t unit){
    this->_TAUValue = ConvertString(unit,3) + ConvertString(time,5);
    //Serial.print("_TAUValue=");
    //Serial.println(this->_TAUValue);
}

void DFRobot_BC20::setActiveTime(uint8_t time, eAcTunit_t unit){
    this->_AcTValue = ConvertString(unit,3) + ConvertString(time,5);
    //Serial.print("_AcTValue=");
    //Serial.println(this->_AcTValue);
}

bool DFRobot_BC20::setPSMMode(ePSM_t status){
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_PSMS+"=");
    BC20Serial.print(status);
    BC20Serial.print(",,,\"");
    BC20Serial.print(this->_TAUValue);
    BC20Serial.print("\",\"");
    BC20Serial.print(this->_AcTValue);
    BC20Serial.println("\"");
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::seteDRX(uint8_t mode){
    String val = "";
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_EDRXS+"=");
    BC20Serial.println(mode);
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested){
    String val = "";
    switch(Requested){
        case eeDRXValue_20_48:
            val = "0010";
            break;
        case eeDRXValue_40_96:
            val = "0011";
            break;
        case eeDRXValue_81_92:
            val = "0101";
            break;
        case eeDRXValue_163_84:
            val = "1001";
            break;
        case eeDRXValue_327_68:
            val = "1010";
            break;
        case eeDRXValue_655_36:
            val = "1011";
            break;
        case eeDRXValue_1310_72:
            val = "1100";
            break;
        case eeDRXValue_2621_44:
            val = "1101";
            break;
        case eeDRXValue_5242_88:
            val = "1110";
            break;
        case eeDRXValue_10485_76:
            val = "1111";
            break;
        default:
            val = "0101";
            break;
    }
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_EDRXS+"=");
    BC20Serial.print(mode);
    BC20Serial.print(",");
    BC20Serial.print(ActType);
    BC20Serial.print(",\"");
    BC20Serial.print(val);
    BC20Serial.println("\"");
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested, uint8_t NW){
    String val = "";
    String nw = "";
    switch(Requested){
        case eeDRXValue_20_48:
            val = "0010";
            break;
        case eeDRXValue_40_96:
            val = "0011";
            break;
        case eeDRXValue_81_92:
            val = "0101";
            break;
        case eeDRXValue_163_84:
            val = "1001";
            break;
        case eeDRXValue_327_68:
            val = "1010";
            break;
        case eeDRXValue_655_36:
            val = "1011";
            break;
        case eeDRXValue_1310_72:
            val = "1100";
            break;
        case eeDRXValue_2621_44:
            val = "1101";
            break;
        case eeDRXValue_5242_88:
            val = "1110";
            break;
        case eeDRXValue_10485_76:
            val = "1111";
            break;
        default:
            val = "0101";
            break;
    }
    switch(NW){
        case eNWProvidedValue_20_48:
            nw = "0010";
            break;
        case eNWProvidedValue_40_96:
            nw = "0011";
            break;
        case eNWProvidedValue_81_92:
            nw = "0101";
            break;
        case eNWProvidedValue_163_84:
            nw = "1001";
            break;
        case eNWProvidedValue_327_68:
            nw = "1010";
            break;
        case eNWProvidedValue_655_36:
            nw = "1011";
            break;
        case eNWProvidedValue_1310_72:
            nw = "1100";
            break;
        case eNWProvidedValue_2621_44:
            nw = "1101";
            break;
        case eNWProvidedValue_5242_88:
            nw = "1110";
            break;
        case eNWProvidedValue_10485_76:
            nw = "1111";
            break;
        default:
            nw = "0101";
            break;
    }
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_EDRXS+"=");
    BC20Serial.print(mode);
    BC20Serial.print(",");
    BC20Serial.print(ActType);
    BC20Serial.print(",\"");
    BC20Serial.print(val);
    BC20Serial.print("\"");
    BC20Serial.print(",\"");
    BC20Serial.print(nw);
    BC20Serial.println("\"");
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::seteDRX(uint8_t mode,uint8_t ActType, uint8_t Requested, uint8_t NW, uint8_t PTW){
    String val = "";
    String nw = "";
    String ptw = "";
    switch(Requested){
        case eeDRXValue_20_48:
            val = "0010";
            break;
        case eeDRXValue_40_96:
            val = "0011";
            break;
        case eeDRXValue_81_92:
            val = "0101";
            break;
        case eeDRXValue_163_84:
            val = "1001";
            break;
        case eeDRXValue_327_68:
            val = "1010";
            break;
        case eeDRXValue_655_36:
            val = "1011";
            break;
        case eeDRXValue_1310_72:
            val = "1100";
            break;
        case eeDRXValue_2621_44:
            val = "1101";
            break;
        case eeDRXValue_5242_88:
            val = "1110";
            break;
        case eeDRXValue_10485_76:
            val = "1111";
            break;
        default:
            val = "0101";
            break;
    }
    switch(NW){
        case eNWProvidedValue_20_48:
            nw = "0010";
            break;
        case eNWProvidedValue_40_96:
            nw = "0011";
            break;
        case eNWProvidedValue_81_92:
            nw = "0101";
            break;
        case eNWProvidedValue_163_84:
            nw = "1001";
            break;
        case eNWProvidedValue_327_68:
            nw = "1010";
            break;
        case eNWProvidedValue_655_36:
            nw = "1011";
            break;
        case eNWProvidedValue_1310_72:
            nw = "1100";
            break;
        case eNWProvidedValue_2621_44:
            nw = "1101";
            break;
        case eNWProvidedValue_5242_88:
            nw = "1110";
            break;
        case eNWProvidedValue_10485_76:
            nw = "1111";
            break;
        default:
            nw = "0101";
            break;
    }
    switch(PTW){
        case ePTWValue_2_56:
            ptw = "0000";
            break;
        case ePTWValue_5_12:
            ptw = "0001";
            break;
        case ePTWValue_7_68:
            ptw = "0010";
            break;
        case ePTWValue_10_24:
            ptw = "0011";
            break;
        case ePTWValue_12_8:
            ptw = "0100";
            break;
        case ePTWValue_15_36:
            ptw = "0101";
            break;
        case ePTWValue_17_92:
            ptw = "0110";
            break;
        case ePTWValue_20_48:
            ptw = "0111";
            break;
        case ePTWValue_23_04:
            ptw = "1000";
            break;
        case ePTWValue_25_6:
            ptw = "1001";
            break;
        case ePTWValue_28_16:
            ptw = "1010";
            break;
        case ePTWValue_30_72:
            ptw = "1011";
            break;
        case ePTWValue_33_28:
            ptw = "1100";
            break;
        case ePTWValue_35_84:
            ptw = "1101";
            break;
        case ePTWValue_38_4:
            ptw = "1110";
            break;
        case ePTWValue_40_96:
            ptw = "1111";
            break;
        default:
            ptw = "0101";
            break;
    }
    flushBC20Serial();
    BC20Serial.print("AT+"+(String)SET_EDRXS+"=");
    BC20Serial.print(mode);
    BC20Serial.print(",");
    BC20Serial.print(ActType);
    BC20Serial.print(",\"");
    BC20Serial.print(val);
    BC20Serial.print("\"");
    BC20Serial.print(",\"");
    BC20Serial.print(nw);
    BC20Serial.print("\"");
    BC20Serial.print(",\"");
    BC20Serial.print(ptw);
    BC20Serial.println("\"");
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

bool DFRobot_BC20::setNBEventReport(bool mode){
    if(mode){
        BC20Serial.println("AT+QNBIOTEVENT=1,1");
    }else{
        BC20Serial.println("AT+QNBIOTEVENT=0,1");
    }
    ReceviceATCMD(300);
    return CheckRecData(AT_OK);
}

void DFRobot_BC20::setPSMCallback(void (*call)(void)){
    this->PSMcallback = call;
}
