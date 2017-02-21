#pragma once
#include <linux/input.h>
#include "types.h"


struct EvdevControllerMapping
{
	const int Btn_A;
	const int Btn_B;
	const int Btn_X;
	const int Btn_Y;	
	const int Btn_Start;
                  /*AS BUTTONS*/	
	const int Btn_DPad_Left;
	const int Btn_DPad_Right;
	const int Btn_DPad_Up;
	const int Btn_DPad_Down; 
                  const int Btn_Trigger_Left;
	const int Btn_Trigger_Right;
                   /*AS ANALOG*/ 
                  const int Axis_DPad_X;
	const int Axis_DPad_Y;
	const int Axis_Analog_X;
	const int Axis_Analog_Y;
	const int Axis_Trigger_Left;
	const int Axis_Trigger_Right;
        
	const bool Axis_Analog_X_Inverted;
	const bool Axis_Analog_Y_Inverted;
	const bool Axis_Trigger_Left_Inverted;
	const bool Axis_Trigger_Right_Inverted;
};

struct EvdevAxisData
{
	s32 range; // smaller size than 32 bit might cause integer overflows
	s32 min;
	void init(int fd, int code, bool inverted);
	s8 convert(int value);
};

struct EvdevController
{
	int fd;
	EvdevControllerMapping* mapping;
	EvdevAxisData data_x;
	EvdevAxisData data_y;
	EvdevAxisData data_trigger_left;
	EvdevAxisData data_trigger_right;
	int rumble_effect_id;
	void init();
};

#define EVDEV_DEVICE_CONFIG_KEY "evdev_device_id_%d"
#define EVDEV_MAPPING_CONFIG_KEY "evdev_mapping_%d"
#define EVDEV_DEVICE_STRING "/dev/input/event%d"
#define EVDEV_MAPPING_PATH "/mappings/%s"

#ifdef TARGET_PANDORA
	#define EVDEV_DEFAULT_DEVICE_ID_1 4
#else
	#define EVDEV_DEFAULT_DEVICE_ID_1 0
#endif

#define EVDEV_DEFAULT_DEVICE_ID(port) (port == 1 ? EVDEV_DEFAULT_DEVICE_ID_1 : -1)

//extern int input_evdev_init(EvdevController* controller, const char* device, const char* mapping_fname);
extern int   input_evdev_init(EvdevController* controller, const char* device, struct DCPadKeymap &PadMap);  //обновлено
extern bool input_evdev_handle(EvdevController* controller, u32 port);
extern void input_evdev_rumble(EvdevController* controller, u16 pow_strong, u16 pow_weak);
