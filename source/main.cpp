#define TESLA_INIT_IMPL // If you have more than one file using the tesla header, only define this in the main one
#include <tesla.hpp>    // The Tesla Header
#include <inttypes.h>

///*Notes BatteryTempMode
//	x == temperature
//	  temp range: state  voltage max           	current unknown
//	     x >= 60: 4      unk0 == 2 ? 4208 : 3952,   2048,   0
//	51 >= x < 60: 3      unk0 == 2 ? 4208 : 3952,   2048,   unk0 != 2 ? 1 : 0
//	17 >= x < 51: 2      4208,                      2048,   1
//	4  >= x < 17: 1      4208,                      768,    1
//	      x <  4: 0      4208,                      768,    0
//
//	if unk0 == 0 then current is set to 512.
///End of Notes

typedef enum {
	Undocked	=	BIT(0),
	Charging	=	BIT(8),
} BatteryChargeInfoFieldsFlags;

typedef enum {
	None		=	0,
	PD		=	1,
	TypeC_1500mA	=	2,
	TypeC_3000mA	=	3,
	DCP		=	4,
	CDP		=	5,
	SDP		=	6,
	Apple_500mA	=	7,
	Apple_1000mA	=	8,
	Apple_2000mA	=	9
	
} BatteryChargeInfoFieldsChargerType;

typedef enum {
	Unknown			=	0,
	Sink			=	1,
	Source			=	2
} BatteryChargeInfoFieldsPowerRole;

typedef struct {
	int32_t InputCurrentLimit; //Input current limit in mA
	int32_t unk_x04; //Possibly a limit in mA
	int32_t FastChargeCurrentLimit; //Charging current limit in mA
	int32_t ChargeVoltageLimit; //Charge voltage limit in mV
	int32_t unk_x10; //Possibly an emum
	int32_t unk_x14; //Possibly a set a flags
	int32_t BatteryTempMode; // Battery Temperature State (more in Notes)
	int32_t BatteryTemperature; //Battery temperature in milli C
	int32_t RawBatteryCharge; //Raw battery charge per cent-mille (i.e. 100% = 100000 pcm)
	int32_t VoltageNow; //Voltage now in mV
	int32_t BatteryAge; //Battery age per cent-mille (i.e. 100% = 100000 pcm)
	BatteryChargeInfoFieldsPowerRole PowerRole;
	BatteryChargeInfoFieldsChargerType ChargerType;
	int32_t ChargerVoltageLimit; //Charger voltage limit in mV
	int32_t ChargerCurrentLimit; //Charger current limit in mA
	BatteryChargeInfoFieldsFlags Flags; //	Possibly a set a flags (potentially 0x100 for charging, 0x1 for undocked)
} BatteryChargeInfoFields;

Result psmGetBatteryChargeInfoFields(Service* psmService, BatteryChargeInfoFields *out) {
	return serviceDispatchOut(psmService, 17, *out);
}

BatteryChargeInfoFields* _batteryChargeInfoFields;
bool threadexit = false;
char Print_x[512];
Thread t0;

void GetBatteryLoop(void*) {
	Service* psmService = psmGetServiceSession();
	_batteryChargeInfoFields = new BatteryChargeInfoFields;
	while (threadexit == false) {
		psmGetBatteryChargeInfoFields(psmService, _batteryChargeInfoFields);
		snprintf(Print_x, sizeof(Print_x), "Input current limit: %u mA" "\nunk_x04: %u mA" "\nCharging current limit: %u mA" "\nCharge voltage limit: %u mV" "\nunk_x10: 0x%08" PRIx32 "\nunk_x14: 0x%08" PRIx32 "\nBattery Temperature State: %u" "\nBattery Temperature: %.1f%s" "\nBattery Charge: %.1f%s" "\nVoltage now: %u mV" "\nBattery Age: %.1f%s" "\nPower Role: %u" "\nCharger Type: %u" "\nCharger Voltage Limit: %u mV" "\nCharger Current Limit: %u mA" "\nunk_x3c: 0x%08" PRIx32, _batteryChargeInfoFields->InputCurrentLimit, _batteryChargeInfoFields->unk_x04, _batteryChargeInfoFields->FastChargeCurrentLimit, _batteryChargeInfoFields->ChargeVoltageLimit, _batteryChargeInfoFields->unk_x10, _batteryChargeInfoFields->unk_x14, _batteryChargeInfoFields->BatteryTempMode, (float)_batteryChargeInfoFields->BatteryTemperature / 1000, "\u00B0C", (float)_batteryChargeInfoFields->RawBatteryCharge / 1000, "%", _batteryChargeInfoFields->VoltageNow, (float)_batteryChargeInfoFields->BatteryAge / 1000, "%", _batteryChargeInfoFields->PowerRole, (int32_t)_batteryChargeInfoFields->ChargerType, _batteryChargeInfoFields->ChargerVoltageLimit, _batteryChargeInfoFields->ChargerCurrentLimit, (int32_t)_batteryChargeInfoFields->Flags);
		svcSleepThread(33'333'333);
	}
	delete _batteryChargeInfoFields;
}

class GuiTest : public tsl::Gui {
public:
	GuiTest(u8 arg1, u8 arg2, bool arg3) { }

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("BatteryChargeInfoNX", APP_VERSION);

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();
		
		list->addItem(new tsl::elm::CustomDrawer([](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {
			renderer->drawString(Print_x, false, x, y+50, 20, renderer->a(0xFFFF));
	}), 500);

		// Add the list to the frame for it to be drawn
		frame->setContent(list);
		
        
		// Return the frame to have it become the top level element of this Gui
		return frame;
	}

	// Called once every frame to update values
	virtual void update() override {
			
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, touchPosition touchInput, JoystickPosition leftJoyStick, JoystickPosition rightJoyStick) override {
		if (keysHeld & KEY_A) {
			tsl::hlp::requestForeground(false);
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class OverlayTest : public tsl::Overlay {
public:
	// libtesla already initialized fs, hid, pl, pmdmnt, hid:sys and set:sys
	virtual void initServices() override {
		smInitialize();
		psmInitialize();
		threadCreate(&t0, GetBatteryLoop, NULL, NULL, 0x4000, 0x3F, -2);
		threadStart(&t0);
	}  // Called at the start to initialize all services necessary for this Overlay
	
	virtual void exitServices() override {
		threadexit = true;
		threadWaitForExit(&t0);
		threadClose(&t0);
		psmExit();
		smExit();
	}  // Callet at the end to clean up all services previously initialized

	virtual void onShow() override {}    // Called before overlay wants to change from invisible to visible state
	
	virtual void onHide() override {}    // Called before overlay wants to change from visible to invisible state

	virtual std::unique_ptr<tsl::Gui> loadInitialGui() override {
		return initially<GuiTest>(1, 2, true);  // Initial Gui to load. It's possible to pass arguments to it's constructor like this
	}
};

int main(int argc, char **argv) {
    return tsl::loop<OverlayTest>(argc, argv);
}
