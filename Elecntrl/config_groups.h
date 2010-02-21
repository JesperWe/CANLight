#ifndef CONFIG_GROUPS_H_
#define CONFIG_GROUPS_H_

// The system config represents the entire system, and is stored in each device.
// The config "file" is actually a byte sequence sent from some master controller
// to all devices on update. This "file" is stored in User Flash memory and parsed at runtime.
// 
// A "device" in the system is a single piece of hardware with its own CPU.
// It has an address (device number) in the range 0-253.
//
// A "function" is one individually controllable output on this device.
//
// A "group" is a collection of functions on one or many devices that listen to the same events.
// A group can consist of only a single function on a single device.
// A function must be part of a group to be able to listen for events.
//
// The config "file" byte sequence follows this pattern:
//   <2 bytes sequence number>
//   group device func [ device func ] FE
//   device func event [ event ] FE
//   device func event [ event ] FE

//  ^^^ change these to: event action device func [ device func ] FE
// (Better fit with GUI data model?)

//   FE
//   group device func [ device func ] FE
//   device func event [ event ] FE
//   device func event [ event ] FE
//   FE
//   ...
//   FF
//

// These def's are only so that we can write group 00 as grp00 rather than just 00.
// ...makes it easier to visually separate group numbers from device numbers.
#define grp00		00
#define grp01		01
#define grp02		02
#define grp03		03
#define grp04		04
#define grp05		05
#define grp06		06
#define grp07		07
#define grp08		08
#define grp09		09
#define grp10		10
#define grp11		11
#define grp12		12
#define grp13		13
#define grp14		14
#define grp15		15
#define grp16		16
#define grp17		17
#define grp18		18
#define grp19		19
#define grp20		20
#define grp21		21
#define grp22		22
#define grp23		23
#define grp24		24
#define grp25		25
#define grp26		26
#define grp27		27
#define grp28		28
#define grp29		29
#define grp30		30
#define grp31		31
#define grp32		32
#define grp33		33
#define grp34		34
#define grp35		35
#define grp36		36
#define grp37		37
#define grp38		38
#define grp39		39
#define grp40		40


#define cfg_DEFAULT_CONFIG_FILE \
{ 00, 01, \
\
  grp00, 07, hw_LED_RED, config_GroupEnd, \
  07, hw_KEY1, e_KEY_CLICKED, e_KEY_HOLDING, e_KEY_RELEASED, config_GroupEnd, \
  config_GroupEnd, \
\
  grp01, 07, hw_LED3, config_GroupEnd, \
  23, hw_LED_LIGHT, e_SWITCH_ON, e_SWITCH_OFF, config_GroupEnd, \
  config_GroupEnd, \
\
  grp02, 07, hw_LED2, config_GroupEnd, \
  07, hw_KEY2, e_KEY_CLICKED, config_GroupEnd, \
  config_GroupEnd, \
\
  grp03, 23, hw_LED_LIGHT, config_GroupEnd, \
  07, hw_KEY3, e_KEY_CLICKED, e_KEY_DOUBLECLICKED, e_KEY_HOLDING, e_KEY_RELEASED, config_GroupEnd, \
  config_GroupEnd, \
\
endConfig }

#endif /* CONFIG_GROUPS_H_ */
