/** Manufacturer data */
#define ENOCEAN_MANUFACTURER_ID 0x03DA
#define PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE 0xE2
#define PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE 0x15

#define BT_ENOCEAN_SWITCH_A0            100
#define BT_ENOCEAN_SWITCH_A1            110
#define BT_ENOCEAN_SWITCH_B0            120
#define BT_ENOCEAN_SWITCH_B1            130

#define ROCKER_A                        1
#define ROCKER_B                        2

#define PUSHED_SHORT                    0
#define PUSHED_LONG                     1
#define PUSHED_UNDEFINED                3

#define LONG_PRESS_INTERVAL_MS          1000

/** Double rocker switch top left button (OA) */
#define BT_ENOCEAN_SWITCH_A0_PUSH 		3
#define BT_ENOCEAN_SWITCH_A0_RELEASE 	2
/** Double rocker switch bottom left button (IA) */
#define BT_ENOCEAN_SWITCH_A1_PUSH		5
#define BT_ENOCEAN_SWITCH_A1_RELEASE	4
/** Double rocker switch top right button (OB) */
#define BT_ENOCEAN_SWITCH_B0_PUSH		9
#define BT_ENOCEAN_SWITCH_B0_RELEASE	8
/** Double rocker switch bottom right button (IB) */
#define BT_ENOCEAN_SWITCH_B1_PUSH		17
#define BT_ENOCEAN_SWITCH_B1_RELEASE	16

/** Single rocker switch top button (O) */
#define BT_ENOCEAN_SWITCH_0_PUSH 		BT_ENOCEAN_SWITCH_B0_PUSH
#define BT_ENOCEAN_SWITCH_0_RELEASE 	BT_ENOCEAN_SWITCH_B0_RELEASE
/** Single rocker switch bottom button (I) */
#define BT_ENOCEAN_SWITCH_1_PUSH		BT_ENOCEAN_SWITCH_B1_PUSH
#define BT_ENOCEAN_SWITCH_1_RELEASE 	BT_ENOCEAN_SWITCH_B1_RELEASE

/** Power rocker switch without button press */
#define BT_ENOCEAN_SWITCH_PUSH		1
#define BT_ENOCEAN_SWITCH_RELEASE 	0



