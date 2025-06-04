#include "esp8266.h"
#include "onenet.h"
#include "mqttkit.h"
#include "usart.h"
#include "dht11.h"
#include <string.h>
#include <stdio.h>

//#define PROID		"q7scNNdMtO"  //��ƷID
#define PROID		"yjVuZZ3Bcb"  //��ƷID

//#define AUTH_INFO	"version=2018-10-31&res=products%2Fq7scNNdMtO%2Fdevices%2Ftest&et=2810353764&method=md5&sign=%2FKplfgsznQJlTOYA%2Bf1pdA%3D%3D"  //��Ȩ��Ϣ
#define AUTH_INFO	"version=2018-10-31&res=products%2FyjVuZZ3Bcb%2Fdevices%2Fyixi&et=2810353764&method=md5&sign=6c4TPhHnkxxAqaQW8JtYZw%3D%3D"  //��Ȩ��Ϣ

//#define DEVID		"test"   //�豸����
#define DEVID		"yixi"   //�豸����

extern unsigned char esp8266_buf[128];

//==========================================================
//	�������ƣ�	OneNet_DevLink
//
//	�������ܣ�	��onenet��������
//
//	��ڲ�����	��
//
//	���ز�����	1-�ɹ�	0-ʧ��
//
//	˵����		��onenetƽ̨��������
//==========================================================
_Bool OneNet_DevLink(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};					//Э���

	unsigned char *dataPtr;
	
	_Bool status = 1;
	
	if(MQTT_PacketConnect(PROID, AUTH_INFO, DEVID, 256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)  //�޸�clean_session=1
	{
		
		ESP8266_SendData(mqttPacket._data, mqttPacket._len);	//�ϴ�ƽ̨
		
		dataPtr = ESP8266_GetIPD(250);	//�ȴ�ƽ̨��Ӧ
		if(dataPtr != NULL)
		{
			if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
			{
				switch(MQTT_UnPacketConnectAck(dataPtr))
				{
					case 0:status = 0;break;
					
					case 1:break;
					case 2:break;
					case 3:break;
					case 4:break;
					case 5:break;
					
					default:break;
				}
			}
		}
		
		MQTT_DeleteBuffer(&mqttPacket);		//ɾ��
	}
	
	return status;
	
}


//����ONENET��Ҫ�ύJSON���ݣ��ͻ�ȡ������ʪ��ת��ΪJSON������ʽ
unsigned char MqttOnenet_Savedata(char *t_payload)
{
//	extern double temperature;
	
	char json[] = "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{\"temperature\":{\"value\":%d},\"humidity\":{\"value\":%d}}}";  //������JSON������ʽ������OneNET����
    char t_json[200];
    unsigned short json_len;
	sprintf(t_json, json, temperature,humidity);
    json_len = strlen(t_json)/sizeof(char);
  	memcpy(t_payload, t_json, json_len);
	
    return json_len;	
}

//==========================================================
//	�������ƣ�	OneNet_SendData
//
//	�������ܣ�	�ϴ����ݵ�ƽ̨
//
//	��ڲ�����	type���������ݵĸ�ʽ
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_SendData(void)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};		//Э���
	
	char buf[128];
	short body_len = 0, i = 0;
	memset(buf, 0, sizeof(buf));    //���buff
	body_len=MqttOnenet_Savedata(buf);	
	
	if(body_len)
	{
		
		if(MQTT_PacketSaveData(DEVID, body_len, NULL, 5, &mqttPacket) == 0)							//���
		{
			
			for(; i < body_len; i++)
				mqttPacket._data[mqttPacket._len++] = buf[i];
			
			ESP8266_SendData(mqttPacket._data, mqttPacket._len);									//�ϴ����ݵ�ƽ̨
			
			MQTT_DeleteBuffer(&mqttPacket);															//ɾ��
		}
	}
	
}

//==========================================================
//	�������ƣ�	OneNet_RevPro
//
//	�������ܣ�	ƽ̨�������ݼ��
//
//	��ڲ�����	dataPtr��ƽ̨���ص�����
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void OneNet_RevPro(unsigned char *cmd)
{
	
	MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};								//Э���
	
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	
	unsigned short req_len = 0;
	
	unsigned char type = 0;
	
	short result = 0;

	char *dataPtr = NULL;
	char numBuf[10];
	int num = 0;
	
	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_CMD:		//�����·�
			
			result = MQTT_UnPacketCmd(cmd, &cmdid_topic, &req_payload, &req_len);	//���topic����Ϣ��
			if(result == 0)
			{
				if(MQTT_PacketCmdResp(cmdid_topic, req_payload, &mqttPacket) == 0)	//����ظ����
				{	
					ESP8266_SendData(mqttPacket._data, mqttPacket._len);			//�ظ�����
					MQTT_DeleteBuffer(&mqttPacket);			//ɾ��
				}
			}
		
			break;
			
		case MQTT_PKT_PUBACK:			//����Publish��Ϣ��ƽ̨�ظ���Ack
			
			break;
		
		default:
			result = -1;
			break;
	}
	
	ESP8266_Clear();									//��ջ���
	
	if(result == -1)
		return;
	
	dataPtr = strchr(req_payload, '}');					//����'}'

	if(dataPtr != NULL && result != -1)					//����ҵ���
	{
		dataPtr++;
		
		while(*dataPtr >= '0' && *dataPtr <= '9')		//�ж��Ƿ����·��������������
		{
			numBuf[num++] = *dataPtr++;
		}
		
		num = atoi((const char *)numBuf);				//תΪ��ֵ��ʽ
	}

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}
