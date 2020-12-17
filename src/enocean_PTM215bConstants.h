/** Manufacturer data */
#define ENOCEAN_MANUFACTURER_ID 0x03DA
#define PMT215B_STATIC_SOURCE_ADDRESS_FIRST_BYTE 0xE2
#define PMT215B_STATIC_SOURCE_ADDRESS_SECOND_BYTE 0x15

/** Double rocker switch top left button (OA) */
#define BT_ENOCEAN_SWITCH_OA_PUSH 		0x03
#define BT_ENOCEAN_SWITCH_OA_RELEASE 	0x02
/** Double rocker switch bottom left button (IA) */
#define BT_ENOCEAN_SWITCH_IA_PUSH		0x05
#define BT_ENOCEAN_SWITCH_IA_RELEASE	0x04
/** Double rocker switch top right button (OB) */
#define BT_ENOCEAN_SWITCH_OB_PUSH		0x09
#define BT_ENOCEAN_SWITCH_OB_RELEASE	0x08
/** Double rocker switch bottom right button (IB) */
#define BT_ENOCEAN_SWITCH_IB_PUSH		0x11
#define BT_ENOCEAN_SWITCH_IB_RELEASE	0x10

/** Single rocker switch top button (O) */
#define BT_ENOCEAN_SWITCH_O_PUSH 		BT_ENOCEAN_SWITCH_OB_PUSH
#define BT_ENOCEAN_SWITCH_O_RELEASE 	BT_ENOCEAN_SWITCH_OB_RELEASE
/** Single rocker switch bottom button (I) */
#define BT_ENOCEAN_SWITCH_I_PUSH		BT_ENOCEAN_SWITCH_IB_PUSH
#define BT_ENOCEAN_SWITCH_I_RELEASE 	BT_ENOCEAN_SWITCH_IB_RELEASE

/** Power rocker switch without button press */
#define BT_ENOCEAN_SWITCH_P_PUSH		0x01
#define BT_ENOCEAN_SWITCH_P_RELEASE 	0x00

/** Contents of a payload telegram */
struct payload{
    char len[1] 			= {0};
    char type[1] 			= {0};
    char manufacturerId[2] 	= {0};
    char sequenceCounter[4] = {0};
    char switchStatus[1] 	= {0};
    char optionalData[4] 	= {0};
};

/** Contents of a commissioning telegram */
struct commissioning{
    char len[1] 				= {0};
    char type[1] 				= {0};
    char manufacturerId[2] 		= {0};
    char sequenceCounter[4] 	= {0};
    char securityKey[16] 		= {0};
    char staticSourceAddress[6] = {0};
};

