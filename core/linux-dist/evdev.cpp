#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include "linux-dist/evdev.h"
#include "linux-dist/main.h"
#include "cfg/ini.h"
#include <vector>
#include <map>
#include <dlfcn.h>

#if defined(USE_EVDEV)
	bool libevdev_tried = false;
	bool libevdev_available = false;
	typedef int (*libevdev_func1_t)(int, const char*);
	typedef const char* (*libevdev_func2_t)(int, int);
	libevdev_func1_t libevdev_event_code_from_name;
	libevdev_func2_t libevdev_event_code_get_name;

	void load_libevdev()
	{
		if (libevdev_tried)
		{
			return;
		}

		libevdev_tried = true;
		void* lib_handle = dlopen("libevdev.so", RTLD_NOW);

		bool failed = false;

		if (!lib_handle)
		{
			fprintf(stderr, "%s\n", dlerror());
			failed = true;
		}
		else
		{
			libevdev_event_code_from_name = reinterpret_cast<libevdev_func1_t>(dlsym(lib_handle, "libevdev_event_code_from_name"));

			const char* error1 = dlerror();
			if (error1 != NULL)
			{
				fprintf(stderr, "%s\n", error1);
				failed = true;
			}

			libevdev_event_code_get_name = reinterpret_cast<libevdev_func2_t>(dlsym(lib_handle, "libevdev_event_code_get_name"));

			const char* error2 = dlerror();
			if (error2 != NULL)
			{
				fprintf(stderr, "%s\n", error2);
				failed = true;
			}
		}

		if(failed)
		{
			puts("WARNING: libevdev is not available. You'll not be able to use button names instead of numeric codes in your controller mappings!\n");
			return;
		}

		libevdev_available = true;
	}

	s8 EvdevAxisData::convert(s32 value)
	{
		return (((value - min) * 255) / range);
	}

	void EvdevAxisData::init(int fd, int code, bool inverted)
	{
		struct input_absinfo abs;
		if(code < 0 || ioctl(fd, EVIOCGABS(code), &abs))
		{
			if(code >= 0)
			{
				perror("evdev ioctl");
			}
			this->range = 255;
			this->min = 0;
			return;
		}
		s32 min = abs.minimum;
		s32 max = abs.maximum;
		printf("evdev: range of axis %d is from %d to %d\n", code, min, max);
		if(inverted)
		{
			this->range = (min - max);
			this->min = max;
		}
		else
		{
			this->range = (max - min);
			this->min = min;
		}
	}

	void EvdevController::init()
	{
		this->data_x.init(this->fd, this->mapping->Axis_Analog_X, this->mapping->Axis_Analog_X_Inverted);
		this->data_y.init(this->fd, this->mapping->Axis_Analog_Y, this->mapping->Axis_Analog_Y_Inverted);
		this->data_trigger_left.init(this->fd, this->mapping->Axis_Trigger_Left, this->mapping->Axis_Trigger_Left_Inverted);
		this->data_trigger_right.init(this->fd, this->mapping->Axis_Trigger_Right, this->mapping->Axis_Trigger_Right_Inverted);
		this->rumble_effect_id = -1;
	}

	std::map<std::string, EvdevControllerMapping> loaded_mappings;

	int load_keycode(ConfigFile* cfg, string section, string dc_key)   ///!!!!!!!!!!!!!!!!!!
	{
		int code = -1;

		string keycode = cfg->get(section, dc_key, "-1");
		if (strstr(keycode.c_str(), "KEY_") != NULL ||
			strstr(keycode.c_str(), "BTN_") != NULL ||
			strstr(keycode.c_str(), "ABS_") != NULL)
		{
			if(libevdev_available)
			{
				int type = ((strstr(keycode.c_str(), "ABS_") != NULL) ? EV_ABS : EV_KEY);
				code = libevdev_event_code_from_name(type, keycode.c_str());
			}
			if(code < 0)
			{
				printf("evdev: failed to find keycode for '%s'\n", keycode.c_str());
			}
			else
			{
				printf("%s = %s (%d)\n", dc_key.c_str(), keycode.c_str(), code);
			}
			return code;
		}

		code = cfg->get_int(section, dc_key, -1);
		if(code >= 0)
		{
			char* name = NULL;
			if(libevdev_available)
			{
				int type = ((strstr(dc_key.c_str(), "axis_") != NULL) ? EV_ABS : EV_KEY);
				name = (char*)libevdev_event_code_get_name(type, code);
			}
			if (name != NULL)
			{
				printf("%s = %s (%d)\n", dc_key.c_str(), name, code);
			}
			else
			{
				printf("%s = %d\n", dc_key.c_str(), code);
			}
		}
		return code;
	}

	EvdevControllerMapping load_mapping(FILE* fd)   //!!!!!!!!!!!!!!!
	{
		ConfigFile mf;
		mf.parse(fd);

		EvdevControllerMapping mapping = {
			mf.get("emulator", "mapping_name", "<Unknown>").c_str(),
			load_keycode(&mf, "dreamcast", "btn_a"),
			load_keycode(&mf, "dreamcast", "btn_b"),
			load_keycode(&mf, "dreamcast", "btn_c"),
			load_keycode(&mf, "dreamcast", "btn_d"),
			load_keycode(&mf, "dreamcast", "btn_x"),
			load_keycode(&mf, "dreamcast", "btn_y"),
			load_keycode(&mf, "dreamcast", "btn_z"),
			load_keycode(&mf, "dreamcast", "btn_start"),
			load_keycode(&mf, "emulator",  "btn_escape"),
			load_keycode(&mf, "dreamcast", "btn_dpad1_left"),
			load_keycode(&mf, "dreamcast", "btn_dpad1_right"),
			load_keycode(&mf, "dreamcast", "btn_dpad1_up"),
			load_keycode(&mf, "dreamcast", "btn_dpad1_down"),
			load_keycode(&mf, "dreamcast", "btn_dpad2_left"),
			load_keycode(&mf, "dreamcast", "btn_dpad2_right"),
			load_keycode(&mf, "dreamcast", "btn_dpad2_up"),
			load_keycode(&mf, "dreamcast", "btn_dpad2_down"),
			load_keycode(&mf, "compat",    "btn_trigger_left"),
			load_keycode(&mf, "compat",    "btn_trigger_right"),
			load_keycode(&mf, "compat",    "axis_dpad1_x"),
			load_keycode(&mf, "compat",    "axis_dpad1_y"),
			load_keycode(&mf, "compat",    "axis_dpad2_x"),
			load_keycode(&mf, "compat",    "axis_dpad2_y"),
			load_keycode(&mf, "dreamcast", "axis_x"),
			load_keycode(&mf, "dreamcast", "axis_y"),
			load_keycode(&mf, "dreamcast", "axis_trigger_left"),
			load_keycode(&mf, "dreamcast", "axis_trigger_right"),
			mf.get_bool("compat", "axis_x_inverted", false),
			mf.get_bool("compat", "axis_y_inverted", false),
			mf.get_bool("compat", "axis_trigger_left_inverted", false),
			mf.get_bool("compat", "axis_trigger_right_inverted", false)
		};
		return mapping;
	}

	int input_evdev_init(EvdevController* controller, const char* device, const char* custom_mapping_fname = NULL)
	{
		load_libevdev();

		char name[256] = "Unknown";

		printf("evdev: Trying to open device at '%s'\n", device);

		int fd = open(device, O_RDWR);

		if (fd >= 0)
		{
			fcntl(fd, F_SETFL, O_NONBLOCK);
			if(ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0)
			{
				perror("evdev: ioctl");
				return -2;
			}
			else
			{
				printf("evdev: Found '%s' at '%s'\n", name, device);

				controller->fd = fd;

				const char* mapping_fname;

				if(custom_mapping_fname != NULL)
				{
					mapping_fname = custom_mapping_fname;
				}
				else
				{
					#if defined(TARGET_PANDORA)
						mapping_fname = "controller_pandora.cfg";
					#elif defined(TARGET_GCW0)
						mapping_fname = "controller_gcwz.cfg";
					#else
						if (strcmp(name, "Microsoft X-Box 360 pad") == 0 ||
							strcmp(name, "Xbox 360 Wireless Receiver") == 0 ||
							strcmp(name, "Xbox 360 Wireless Receiver (XBOX)") == 0)
						{
							mapping_fname = "controller_xpad.cfg";
						}
						else if (strstr(name, "Xbox Gamepad (userspace driver)") != NULL)
						{
							mapping_fname = "controller_xboxdrv.cfg";
						}
						else if (strstr(name, "keyboard") != NULL ||
								 strstr(name, "Keyboard") != NULL)
						{
							mapping_fname = "keyboard.cfg";
						}
						else
						{
							mapping_fname = "controller_generic.cfg";
						}
					#endif
				}
				if(loaded_mappings.count(string(mapping_fname)) == 0)
				{
					FILE* mapping_fd = NULL;
					if(mapping_fname[0] == '/')
					{
						// Absolute mapping
						mapping_fd = fopen(mapping_fname, "r");
					}
					else
					{
						// Mapping from ~/.reicast/mappings/
						size_t size_needed = snprintf(NULL, 0, EVDEV_MAPPING_PATH, mapping_fname) + 1;
						char* mapping_path = (char*)malloc(size_needed);
						sprintf(mapping_path, EVDEV_MAPPING_PATH, mapping_fname);
						mapping_fd = fopen(get_readonly_data_path(mapping_path).c_str(), "r");
						free(mapping_path);
					}
					
					if(mapping_fd != NULL)
					{
						printf("evdev: reading mapping file: '%s'\n", mapping_fname);
						loaded_mappings.insert(std::make_pair(string(mapping_fname), load_mapping(mapping_fd)));
						fclose(mapping_fd);

					}
					else
					{
						printf("evdev: unable to open mapping file '%s'\n", mapping_fname);
						perror("evdev");
						return -3;
					}
				}
				controller->mapping = &loaded_mappings.find(string(mapping_fname))->second;
				printf("evdev: Using '%s' mapping\n", controller->mapping->name);
				controller->init();

				return 0;
			}
		}
		else
		{
			perror("evdev: open");
			return -1;
		}
	}

	bool input_evdev_handle(EvdevController* controller, u32 port)
	{
		#define SET_FLAG(field, mask, expr) field =((expr) ? (field & ~mask) : (field | mask))
		if (controller->fd < 0 || controller->mapping == NULL)
		{
			return false;
		}

		input_event ie;

		while(read(controller->fd, &ie, sizeof(ie)) == sizeof(ie))
		{
			switch(ie.type)
			{
				case EV_KEY:
					if (ie.code == controller->mapping->Btn_A) {
						SET_FLAG(kcode[port], DC_BTN_A, ie.value);
					} else if (ie.code == controller->mapping->Btn_B) {
						SET_FLAG(kcode[port], DC_BTN_B, ie.value);
					} else if (ie.code == controller->mapping->Btn_C) {
						SET_FLAG(kcode[port], DC_BTN_C, ie.value);
					} else if (ie.code == controller->mapping->Btn_D) {
						SET_FLAG(kcode[port], DC_BTN_D, ie.value);
					} else if (ie.code == controller->mapping->Btn_X) {
						SET_FLAG(kcode[port], DC_BTN_X, ie.value);
					} else if (ie.code == controller->mapping->Btn_Y) {
						SET_FLAG(kcode[port], DC_BTN_Y, ie.value);
					} else if (ie.code == controller->mapping->Btn_Z) {
						SET_FLAG(kcode[port], DC_BTN_Z, ie.value);
					} else if (ie.code == controller->mapping->Btn_Start) {
						SET_FLAG(kcode[port], DC_BTN_START, ie.value);
					} else if (ie.code == controller->mapping->Btn_Escape) {
						die("death by escape key");
					} else if (ie.code == controller->mapping->Btn_DPad_Left) {
						SET_FLAG(kcode[port], DC_DPAD_LEFT, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad_Right) {
						SET_FLAG(kcode[port], DC_DPAD_RIGHT, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad_Up) {
						SET_FLAG(kcode[port], DC_DPAD_UP, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad_Down) {
						SET_FLAG(kcode[port], DC_DPAD_DOWN, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad2_Left) {
						SET_FLAG(kcode[port], DC_DPAD2_LEFT, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad2_Right) {
						SET_FLAG(kcode[port], DC_DPAD2_RIGHT, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad2_Up) {
						SET_FLAG(kcode[port], DC_DPAD2_UP, ie.value);
					} else if (ie.code == controller->mapping->Btn_DPad2_Down) {
						SET_FLAG(kcode[port], DC_DPAD2_DOWN, ie.value);
					} else if (ie.code == controller->mapping->Btn_Trigger_Left) {
						lt[port] = (ie.value ? 255 : 0);
					} else if (ie.code == controller->mapping->Btn_Trigger_Right) {
						rt[port] = (ie.value ? 255 : 0);
					}
					break;
				case EV_ABS:
					if (ie.code == controller->mapping->Axis_DPad_X)
					{
						switch(ie.value)
						{
							case -1:
								SET_FLAG(kcode[port], DC_DPAD_LEFT,  1);
								SET_FLAG(kcode[port], DC_DPAD_RIGHT, 0);
								break;
							case 0:
								SET_FLAG(kcode[port], DC_DPAD_LEFT,  0);
								SET_FLAG(kcode[port], DC_DPAD_RIGHT, 0);
								break;
							case 1:
								SET_FLAG(kcode[port], DC_DPAD_LEFT,  0);
								SET_FLAG(kcode[port], DC_DPAD_RIGHT, 1);
								break;
						}
					}
					else if (ie.code == controller->mapping->Axis_DPad_Y)
					{
						switch(ie.value)
						{
							case -1:
								SET_FLAG(kcode[port], DC_DPAD_UP,   1);
								SET_FLAG(kcode[port], DC_DPAD_DOWN, 0);
								break;
							case 0:
								SET_FLAG(kcode[port], DC_DPAD_UP,  0);
								SET_FLAG(kcode[port], DC_DPAD_DOWN, 0);
								break;
							case 1:
								SET_FLAG(kcode[port], DC_DPAD_UP,  0);
								SET_FLAG(kcode[port], DC_DPAD_DOWN, 1);
								break;
						}
					}
					else if (ie.code == controller->mapping->Axis_DPad2_X)
					{
						switch(ie.value)
						{
							case -1:
								SET_FLAG(kcode[port], DC_DPAD2_LEFT,  1);
								SET_FLAG(kcode[port], DC_DPAD2_RIGHT, 0);
								break;
							case 0:
								SET_FLAG(kcode[port], DC_DPAD2_LEFT,  0);
								SET_FLAG(kcode[port], DC_DPAD2_RIGHT, 0);
								break;
							case 1:
								SET_FLAG(kcode[port], DC_DPAD2_LEFT,  0);
								SET_FLAG(kcode[port], DC_DPAD2_RIGHT, 1);
								break;
						}
					}
					else if (ie.code == controller->mapping->Axis_DPad2_X)
					{
						switch(ie.value)
						{
							case -1:
								SET_FLAG(kcode[port], DC_DPAD2_UP,   1);
								SET_FLAG(kcode[port], DC_DPAD2_DOWN, 0);
								break;
							case 0:
								SET_FLAG(kcode[port], DC_DPAD2_UP,  0);
								SET_FLAG(kcode[port], DC_DPAD2_DOWN, 0);
								break;
							case 1:
								SET_FLAG(kcode[port], DC_DPAD2_UP,  0);
								SET_FLAG(kcode[port], DC_DPAD2_DOWN, 1);
								break;
						}
					}
					else if (ie.code == controller->mapping->Axis_Analog_X)
					{
						joyx[port] = (controller->data_x.convert(ie.value) + 128);
					}
					else if (ie.code == controller->mapping->Axis_Analog_Y)
					{
						joyy[port] = (controller->data_y.convert(ie.value) + 128);
					}
					else if (ie.code == controller->mapping->Axis_Trigger_Left)
					{
						lt[port] = controller->data_trigger_left.convert(ie.value);
					}
					else if (ie.code == controller->mapping->Axis_Trigger_Right)
					{
						rt[port] = controller->data_trigger_right.convert(ie.value);
					}
					break;
			}
		}
	}

	void input_evdev_rumble(EvdevController* controller, u16 pow_strong, u16 pow_weak)
	{
		if (controller->fd < 0 || controller->rumble_effect_id == -2)
		{
			// Either the controller is not used or previous rumble effect failed
			printf("RUMBLE: %s\n", "Skipped!");
			return;
		}
		printf("RUMBLE: %u / %u (%d)\n", pow_strong, pow_weak, controller->rumble_effect_id);
		struct ff_effect effect;
		effect.type = FF_RUMBLE;
		effect.id = controller->rumble_effect_id;
		effect.u.rumble.strong_magnitude = pow_strong;
		effect.u.rumble.weak_magnitude = pow_weak;
		effect.replay.length = 0;
		effect.replay.delay = 0;
		if (ioctl(controller->fd, EVIOCSFF, &effect) == -1)
		{
			perror("evdev: Force feedback error");
			controller->rumble_effect_id = -2;
		}
		else
		{
			controller->rumble_effect_id = effect.id;

			// Let's play the effect
			input_event play;
			play.type = EV_FF;
			play.code = effect.id;
			play.value = 1;
			if (write(controller->fd, (const void*) &play, sizeof(play)) == -1)
			{
				perror("evdev: Force feedback error");
				controller->rumble_effect_id = -2;
			}
		}
	}
#endif

