#ifndef _DATA_h
#define _DATA_h

#include "arduino.h"

#include <dataTable.h>

/*    datos    */
struct Config :public Item 
{	
	char wifi_ssid[32] = "Zyxel_E49C";
	char wifi_pass[64] = "^t!pcm774K";
	char www_user[32] = "admin";
	char www_pass[64] = "admin";
	float screw_pitch = 2.0;
	uint8_t micro_step= 8; // 1 / 8
	float speed= 8.0;// mm/s
	float acc_desc= 4.0;// mm/s²
	
	bool setAdmin(const char * www_user, const char * www_pass) 
	{
		if (strlen(www_user) > 32 || strlen(www_pass) > 64) 
			return false;

		strcpy(this->www_user, www_user);
		strcpy(this->www_pass, www_pass);
		return true;
	};

	bool setWifi(const char * wifi_ssid, const char * wifi_pass) 
	{
		if (strlen(wifi_ssid) > 32 || strlen(wifi_pass) > 64)
			return false;

		strcpy(this->wifi_ssid, wifi_ssid);
		strcpy(this->wifi_pass, wifi_pass);
		return true;
	};
	
	bool setSpeed(float speed, float acc_desc) 
	{
		this->acc_desc = acc_desc;
		this->speed = speed;
		return true;
	};

	bool setMotor(float screw_pitch, uint8_t micro_step) 
	{
		this->screw_pitch = screw_pitch;
		this->micro_step = micro_step;
		return true;
	};

	// item implementation
	void serializeItem(JsonObject &obj, bool extra) {
		if (!extra) {
			obj["wifi_pass"] = this->wifi_pass;
			obj["www_pass"] = this->www_pass;
			obj["www_user"] = this->www_user;
		}
		obj["wifi_ssid"] = this->wifi_ssid;

		obj["acc_desc"] = this->acc_desc;
		obj["speed"] = this->speed;

		obj["screw_pitch"] = this->screw_pitch;
		obj["micro_step"] = this->micro_step;
	};
	
	bool deserializeItem(JsonObject &obj) {
		if (!obj["wifi_ssid"].is<const char*>() || !obj["wifi_pass"].is<const char*>() || 
			!obj["www_user"].is<const char*>() || !obj["www_pass"].is<const char*>()|| 
			!obj["micro_step"].is<uint8_t>() || !obj["speed"].is<float>()||
			!obj["screw_pitch"].is<float>() || !obj["acc_desc"].is<float>())
		{
			Serial.println("faill deserializeItem Config");
			return false;
		}

		setAdmin(obj["www_user"],obj["www_pass"]);
		setWifi(obj["wifi_ssid"],obj["wifi_pass"]);
		setSpeed(obj["speed"],obj["acc_desc"]);
		setMotor(obj["screw_pitch"],obj["micro_step"]);
		
		return true;
	};
};

struct SensorItem :public Item
{
	float distance; float force;
	uint32_t time;

	void set(float distance, float force, uint32_t time) 
	{
		this->distance = distance; this->force = force;
		 this->time = time;
	};
	void serializeItem(JsonObject & obj, bool extra = false) 
	{
		obj["d"] = this->distance; obj["f"] = this->force;
		obj["t"] = this->time;
	};
	bool deserializeItem(JsonObject & obj)
	{
		if (!obj["d"].is<float>() || !obj["f"].is<float>() ||
			!obj["t"].is<uint>() )
		{
			Serial.println("faill deserializeItem SensorItem");
			return false;
		}
		set( obj["d"], obj["f"], obj["t"] );
		return true;
	};
};

struct ResultItem : public Item
{
	char pathData[55] = "";
	char date[40] = "";
	char name[40] = "";
	char description[200] = "";

	void set( const char * path, const char * name, const char * date, 
		const char * description)
	{
		strcpy(this->pathData, path);
		strcpy(this->date, date);
		strcpy(this->name, name);
		strcpy(this->description,description );
	};
	
	bool isValide( const char * path, const char * name, const char * date, 
		const char * description)
	{
		return strlen(path) < 53 && 
			strlen(name) < 38 && strlen(date) < 38 && strlen(description) < 198;
	};
	void serializeItem(JsonObject &obj, bool extra) 
	{
		obj["path"] = this->pathData;
		obj["name"] = this->name;
		obj["date"] = this->date;
		obj["description"] = this->description;
	};
	bool deserializeItem(JsonObject &obj) {
		if(!obj["path"].is<const char*>() ||
			!obj["name"].is<const char*>() ||
			!obj["date"].is<const char*>() ||
			!obj["description"].is<const char*>()
		) {
			Serial.println("faill deserializeItem ResultItem");
			return false;
		}
		set(
			obj["path"],obj["name"],obj["date"],obj["description"]
		);
		return true;
	};
};
#endif