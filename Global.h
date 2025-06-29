#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <pigpio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h> 		// open
#include <sys/stat.h>  		// open
#include <inttypes.h> 		// uint8_t, etc
#include <linux/i2c-dev.h> 	// I2C bus definitions
#include <sys/ioctl.h>
#include <vulkan/vulkan.h>

// Defines for version control
#define AA4X3D_MAJOR_VERSION 0
#define AA4X3D_MINOR_VERSION 625
#define AA4X3D_MICRO_VERSION 2

// Defines for math
#define PI 3.14159
#define ALMOST_ZERO 0.000001
#define TIGHTCHECK 0.00001
#define TOLERANCE 0.0001
#define CLOSE_ENOUGH 0.0010
#define LOOSE_CHECK 0.0100
#define INT_MAX_VAL 2000000000
#define INT_MIN_VAL -2000000000
#define UP 1
#define LEVEL 0
#define DOWN -1
#define FORWARD 1
#define NUETRAL 0
#define REVERSE -1
#define ON 1
#define OFF 0
#define NONE 0

// Defines for actions
#define	ACTION_ADD 1
#define	ACTION_SUBTRACT 2
#define	ACTION_AND 3
#define	ACTION_OR 4
#define	ACTION_XOR 5
#define ACTION_DELETE 6
#define ACTION_CLEAR 7
#define ACTION_UNIQUE_ADD 8
#define ACTION_INSERT 9

// Defines for 4X3D Printer - ALL sizes in millimeters
#define GAMMA_UNIT 1
#ifdef ALPHA_UNIT
  #define BUILD_TABLE_MIN_X 0.0
  #define BUILD_TABLE_MAX_X 415.0
  #define BUILD_TABLE_LEN_X 200.0
  #define BUILD_TABLE_MIN_Y 0.0
  #define BUILD_TABLE_MAX_Y 252.0
  #define BUILD_TABLE_LEN_Y 178.0
  #define BUILD_TABLE_MIN_Z 0.0
  #define BUILD_TABLE_MAX_Z 165.0
  #define BUILD_TABLE_LEN_Z 152.4
  #define ZHOMESET 18.25
#endif
#ifdef BETA_UNIT
  #define BUILD_TABLE_MIN_X 0.0
  #define BUILD_TABLE_MAX_X 415.0
  #define BUILD_TABLE_LEN_X 190.0
  #define BUILD_TABLE_MIN_Y 0.0
  #define BUILD_TABLE_MAX_Y 379.0
  #define BUILD_TABLE_LEN_Y 290.0
  #define BUILD_TABLE_MIN_Z 0.0
  #define BUILD_TABLE_MAX_Z 165.0
  #define BUILD_TABLE_LEN_Z 152.4
  #define ZHOMESET 18.25
#endif
#ifdef GAMMA_UNIT
  #define BUILD_TABLE_MIN_X 0.0
  #define BUILD_TABLE_MAX_X 415.0
  #define BUILD_TABLE_LEN_X 400.0
  #define BUILD_TABLE_MIN_Y 0.0
  #define BUILD_TABLE_MAX_Y 415.0
  #define BUILD_TABLE_LEN_Y 400.0
  #define BUILD_TABLE_MIN_Z 0.0
  #define BUILD_TABLE_MAX_Z 246.0
  #define BUILD_TABLE_LEN_Z 243.0
  #define ZHOMESET 245.0						// prod=245.0  proto=289.0
#endif
#ifdef DELTA_UNIT
  #define BUILD_TABLE_MIN_X 0.0
  #define BUILD_TABLE_MAX_X 475.0
  #define BUILD_TABLE_LEN_X 465.0
  #define BUILD_TABLE_MIN_Y 0.0
  #define BUILD_TABLE_MAX_Y 640.0
  #define BUILD_TABLE_LEN_Y 630.0
  #define BUILD_TABLE_MIN_Z 0.0
  #define BUILD_TABLE_MAX_Z 255.0
  #define BUILD_TABLE_LEN_Z 245.0
  #define ZHOMESET 257.0
#endif
#define HAS_AUTO_TIP_CALIB 1


// Defines for geometry control
#define VTX_INDEX 25							// number of hash table index pointers into vtx array linked list for each axis
#define MAX_VERTS 10

// Defines for model geomety elements
#define MODEL 1								// geometry of file that user loads, i.e. the model
#define SUPPORT 2							// geometry created to support overhands of the model in a given orientation
#define INTERNAL 3							// geometry created inside the model to support overhangs for minimal builds
#define TARGET 4							// geometry created outside the model to make the model a "negative"
#define MAX_MDL_TYPES 5

// Defines for model file/format types
#define STL 1
#define AMF 2
#define GERBER 3
#define GCODE 4
#define IMAGE 5
#define MAX_FORMAT_TYPES 6

// Defines for view types
#define VIEW_MODEL 0
#define VIEW_TOOL 1
#define VIEW_LAYER 2
#define VIEW_GRAPH 3
#define VIEW_CAMERA 4
#define VIEW_VULKAN 5

// Defines for system operation
#define MAX_IDLE_TIME 1800						// maximum time idle in secs before thermal setback, ~30 mins
#define MIN_BUFFER 26							// target minimum available tinyG buffer (out of 32) while running job
#define MAX_BUFFER 32							// maximum number of commands the tinyG can buffer
#define MIN_VEC_LEN 0.25						// mimimum printable vector length
#define SCAN_X_MAX 100							// maximum number of build table scan points in X
#define SCAN_Y_MAX 100							// maximum number of build table scan points in Y
#ifdef ALPHA_UNIT
  #define MAX_TOOLS 4							// maximum number of tool slots available
  #define MAX_MATERIALS 4						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 5						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 6						// expected number of heaters expected on system (4 tools, bld tbl takes 1, chamber)
  #define BLD_TBL1 MAX_TOOLS
  #define BLD_TBL2 MAX_TOOLS
  #define CHAMBER MAX_TOOLS+1
  #define UNIT_HAS_CAMERA 0
  #define CAMERA_POSITION 0
#endif
#ifdef BETA_UNIT
  #define MAX_TOOLS 4							// maximum number of tool slots available
  #define MAX_MATERIALS 4						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 5						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 7						// expected number of heaters expected on system (4 tools, bld tbl takes 2, chamber)
  #define BLD_TBL1 MAX_TOOLS
  #define BLD_TBL2 MAX_TOOLS+1
  #define CHAMBER MAX_TOOLS+2
  #define UNIT_HAS_CAMERA 0
  #define CAMERA_POSITION 0						// no camera present
#endif
#ifdef GAMMA_UNIT
  #define MAX_TOOLS 1							// maximum number of tool slots available
  #define MAX_MATERIALS 2						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 3						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 4						// expected number of heaters expected on system (tool, bld tbl takes 2, chamber)
  #define BLD_TBL1 1
  #define BLD_TBL2 2
  #define CHAMBER 3							// note: use 2 when reading from A2D
  #define UNIT_HAS_CAMERA 1
  #define CAMERA_POSITION 2						// camera give 3D view of table
#endif
#ifdef DELTA_UNIT
  #define MAX_TOOLS 1							// maximum number of tool slots available
  #define MAX_MATERIALS 2						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 3						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 4						// expected number of heaters expected on system (tool, bld tbl takes 2, chamber)
  #define BLD_TBL1 1
  #define BLD_TBL2 2
  #define CHAMBER 3							// note: use 2 when reading from A2D
  #define UNIT_HAS_CAMERA 1
  #define CAMERA_POSITION 1						// camera gives 2D view of table
#endif

// Defines for history tracking
#define HIST_TIME 0							// type neumonics...
#define HIST_MATL 1
#define HIST_TEMP 2
#define MAX_HIST_TYPES 3
#define MAX_HIST_COUNT 1000						// maximum data points to track for graphing/history per field

#define H_TOOL_TEMP 0							// ... tool temperature
#define H_TABL_TEMP 1							// ... build table temperature
#define H_CHAM_TEMP 2							// ... chamber temperature

#define H_MOD_MAT_EST 3							// ... model material estimate
#define H_SUP_MAT_EST 4							// ... support material estimate
#define H_MOD_MAT_ACT 5							// ... model material actual
#define H_SUP_MAT_ACT 6							// ... support material actual

#define H_MOD_TIM_EST 7							// ... model time estiamte
#define H_SUP_TIM_EST 8							// ... support time estimate
#define H_OHD_TIM_EST 9							// ... overhead time estimate

#define H_MOD_TIM_ACT 10						// ... model time actual
#define H_SUP_TIM_ACT 11						// ... support time actual
#define H_OHD_TIM_ACT 12						// ... overhead time actual

#define MAX_HIST_FIELDS 13						// maximum number of unique fields to track history

// defines for fixed camera use
#define CAM_LIVEVIEW 1							// live view at pace of idle loop (~0.5 secs)
#define CAM_SNAPSHOT 2							// saves snapshot of what is currently displayed
#define CAM_JOB_LAPSE 3							// shows time lapse of last job run
#define CAM_SCAN_LAPSE 4						// shows time lapse of last scan
#define CAM_IMG_DIFF 5							// shows difference between prev snapshot and current view
#define CAM_IMG_SCAN 6							// runs a scan of the build table

// defines for image processing
#define IMG_DIFFERENCE 1						// identifies difference bt two images
#define IMG_EDGE_DETECT_RED 2						// creates 2 color edge detect from only red channel
#define IMG_EDGE_DETECT_ALL 3						// creates 2 color edge detect from all channels
#define IMG_EDGE_DETECT_ADD 4						// creates edge detect from all channels and overlays existing image
#define IMG_GRAYSCALE 5							// converts color to gray scale - good for half tones
#define IMG_BLACK_AND_WHITE 6						// converts color to b&w
#define IMG_SHARPEN 7							// emphasizes transitions
#define IMG_SMOOTH 8							// mitigates transitions
#define IMG_FILTER 9							// applies various filters
#define IMG_COLOR_DETECT 10						// seeks specific color
#define IMG_DENOISE 11							// gets rid of spots

// Defines for table scanning
#define MAX_SCAN_IMAGES 500						// cap of max images from scan video

// Defines for steppers
#define CARG_DV_RATIO 1.25						// slot moves 1.25 linear mm for every 1.00 mm requested

// Defines for job state
#define	JOB_NOT_READY 1
#define JOB_HEATING 2
#define JOB_READY 3
#define JOB_CALIBRATING 4
#define JOB_RUNNING 5
#define JOB_PAUSED_BY_USER 6
#define JOB_PAUSED_BY_CMD 7
#define JOB_PAUSED_FOR_BIT_CHG 8
#define JOB_PAUSED_FOR_TOOL_CHG 9
#define JOB_PAUSED_DUE_TO_ERROR 10
#define JOB_PAUSED_DUE_TO_MATL_OUT 11
#define JOB_TABLE_SCAN 20
#define JOB_TABLE_SCAN_PAUSED 21
#define JOB_COMPLETE 55
#define JOB_SLEEPING 60
#define JOB_WAKING_UP 61
#define JOB_ABORTED_BY_USER 90
#define JOB_ABORTED_BY_SYSTEM 91
#define JOB_ERROR_UNKNOWN 99

// Defines for tool memory format
#define TL_MEM_REV "20200901"

// Defines for tool state
#define TL_EMPTY 0							// empty... nothing in slot at all
#define TL_UNKNOWN 1							// loop back closed... something in slot, but unknown what it is
#define TL_LOADED 2							// tool in slot is defined as to what it is, but no model yet
#define TL_READY 3							// tool has everything needed to operate
#define TL_SELECTED 4							// tool has been selected to be used in the current job
#define TL_ACTIVE 5							// tool actively in use, and proud
#define TL_RETRACTED 6							// tool actively in use, but retracted
#define TL_FAILED 7							// tool was working, but something went wrong

// Defines for line types
#define	MDL_PERIM 1							// perimeter as defined by the model file
#define MDL_BORDER 2							// outer most surface of model (i.e. perimeter offset by line width)
#define MDL_OFFSET 3							// outer surfaces inside the border of the model
#define MDL_FILL 4							// the interior of the model
#define MDL_LOWER_CO 5							// lower surface close offs
#define MDL_UPPER_CO 6							// upper surface close offs
#define MDL_LAYER_1 7							// first layer close off
#define	INT_PERIM 8							// perimeter as defined by the internal structure of the model file
#define INT_BORDER 9							// outer most surface of the internal structure model (i.e. perimeter offset by line width)
#define INT_OFFSET 10							// outer surfaces inside the border of the internal model
#define INT_FILL 11							// the interior of the internal model
#define INT_LOWER_CO 12							// lower surface close offs
#define INT_UPPER_CO 13							// upper surface close offs
#define INT_LAYER_1 14							// first layer close off
#define BASELYR 15							// z compensation layer between build plate and platform layer
#define PLATFORMLYR1 16							// smooth platform layer on top of z comp layer
#define PLATFORMLYR2 17							// first layer of model, different to promote separation from platform layers
#define SPT_PERIM 18							// perimeter of support 
#define SPT_BORDER 19							// outer most surface of support
#define SPT_OFFSET 20							// outer surfaces inside the support boarder
#define SPT_FILL 21							// interior of the support
#define SPT_LOWER_CO 22							// lower surface close offs
#define SPT_UPPER_CO 23							// upper surface close offs
#define SPT_LAYER_1 24							// first layer close off
#define TRACE 25							// GERBER conductive traces
#define DRILL 26							// drilled holes
#define VERTTRIM 27							// router vertical trimming
#define HORZTRIM 28							// router horizontal trimming
#define SURFTRIM 29							// rounter surface trimming
#define TEST_LC 30							// test line type
#define TEST_UC 31							// test line type
#define MAX_LINE_TYPES 32						// number of total line types defined

// Defines for job and tool types
// jobs are generally either additive or subtractive (defines z up or down), even if a combination of additive and subtractive tools.
// tools are more specific to define what they do under either job circumstance.  think of these as actions while tools are the objects.
#define UNDEFINED 0
#define ADDITIVE 1							// tools that add material to the job
#define SUBTRACTIVE 2							// tools that subtract material from the job
#define MEASUREMENT 3							// tools that measure material in the build volume
#define MARKING 4							// tools that mark material already in build
#define CURING 5							// tools that cure/modify material already in build
#define PLACEMENT 6							// tools that move/translate what is on the table
#define SCANNING 7							// tools that scan objects wihtin the build volume
#define MAX_TOOL_TYPES 8

// tool subclasses
#define TC_EXTRUDER 1							// syring type tools
#define TC_FDM 2							// all manner of fused desposition modeler tools
#define TC_ROUTER 3							// cutting/milling type tools
#define TC_LASER 4							// laser etching/cutting tools
#define TC_PROBE 5							// measurement tools
#define TC_LAMP 6							// curing type tools
#define TC_PLACE 7							// pick-up and place type tools
#define TC_PEN 8							// marking type tools
#define TC_CAMERA 9							// camera for object scanning

// Defines for tool operations
#define	OP_NONE 0
#define OP_ADD_MODEL_MATERIAL 1						// add model material
#define OP_ADD_SUPPORT_MATERIAL 2					// add support structure material
#define OP_ADD_BASE_LAYER 3						// add base layer material
#define OP_MILL_OUTLINE 4						// mill just outlines
#define OP_MILL_AREA 5							// mill areas and outlines
#define OP_MILL_PROFILE 6						// mill with profile cutting
#define OP_MILL_HOLES 7							// mill holes (drill)
#define OP_MEASURE_X 8							// measure x dimensions
#define OP_MEASURE_Y 9							// measure y dimensions
#define OP_MEASURE_Z 10							// measure z dimensions
#define OP_MEASURE_HOLES 11						// measure hole diameters
#define OP_MARK_OUTLINE 12						// mark an outline
#define OP_MARK_AREA 13							// mark an area
#define OP_MARK_IMAGE 14						// mark a grayscale image
#define OP_MARK_CUT 15							// mark outline with high power
#define OP_CURE 16							// curing of added material
#define OP_PLACE 17							// placement of material
#define OP_SCAN 18							// scan build volume
#define MAX_OP_TYPES 19

// Defines for RPi GPIO addressing using BoardCom numbering
#define TINYG_RESET 13							// output - resets tinyG as needed
#define AUX1_INPUT 14							// input  - typically used for things like probe input
#define AUX2_OUTPUT 15							// output - typically used for things like a relay on tool
#define TOOL_TIP 17							// input  - tool tip switches mounted on build table
#define TOOL_LIMIT 19							// input  - on ALPHA/BETA it's tool limit, on GAMMA...
#define WATCH_DOG 27							// output - watchdog heartbeat to timer CCA keeping PWM alive

#if defined(ALPHA_UNIT) || defined(BETA_UNIT)
  #define TOOL_A_ACT 5							// output - only used on multi-slot units to activate carriage steppers
  #define TOOL_B_ACT 6
  #define TOOL_C_ACT 7
  #define TOOL_D_ACT 26
	
  #define TOOL_A_SEL 20							// output - turns on/off 5v to each slot/tool.  typ used to activate 1w memory
  #define TOOL_B_SEL 21
  #define TOOL_C_SEL 16
  #define TOOL_D_SEL 12
	
  #define TOOL_A_LOOP 22						// input  - detects if tool is inserted into a slot
  #define TOOL_B_LOOP 23
  #define TOOL_C_LOOP 24
  #define TOOL_D_LOOP 25
#endif

#ifdef GAMMA_UNIT
  #define CHAMBER_LED 12						// output - turns on/off internal lighting
  #define TOOL_AIR 16							// output - air pump/vacuum on GAMMA units
  #define TOOL_A_SEL 20							// output - turns on/off 5v to slot/tool.  typ used to activate 1w memory among other things.
  #define LASER_ENABLE 21						// output - enables 24v directly to laser
  #define TOOL_A_LOOP 22						// input - detects if tool is inserted into a slot
  #define AUX_STEP_DRIVE 23						// input - detects if alternate stepper drive on A channel of tinyG
  #define CARRIAGE_LED 18						// output - led lights under carriage
  #define FILAMENT_OUT 24						// input  - filament out detector

  #define TOOL_DIR_PWM 7						// tool direct pwm is on pin 7 of PWM module
  #define TOOL_24V_PWM 0						// tool 24v pwm is on pin 0 of PWM module (typically used for thermal)
  #define TOOL_48V_PWM 8						// tool 48v pwm is on pin 8 of PWM module (typically used for router)

  // set as outputs - all set to same pin on GAMMAs to free up other pins
  #define TOOL_A_ACT 5	
  #define TOOL_B_ACT 26
  #define TOOL_C_ACT 26
  #define TOOL_D_ACT 26
  #define TOOL_B_SEL 26
  #define TOOL_C_SEL 26
  #define TOOL_D_SEL 26

  // set as inputs - again set to same pin on GAMMAs to free up other pins
  #define TOOL_B_LOOP 25
  #define TOOL_C_LOOP 25
  #define TOOL_D_LOOP 25
#endif

#ifdef DELTA_UNIT
  #define CHAMBER_LED 12						// output - turns on/off internal lighting
  #define TOOL_AIR 16							// output - air pump/vacuum on GAMMA units
  #define TOOL_A_SEL 20							// output - turns on/off 5v to slot/tool.  typ used to activate 1w memory among other things.
  #define LASER_ENABLE 21						// output - enables 24v directly to laser
  #define TOOL_A_LOOP 22						// input - detects if tool is inserted into a slot
  #define AUX_STEP_DRIVE 23						// input - detects if alternate stepper drive on A channel of tinyG
  #define CARRIAGE_LED 18						// output - led lights under carriage

  // set as outputs - all set to same pin on GAMMAs to free up other pins
  #define TOOL_A_ACT 5	
  #define TOOL_B_ACT 26
  #define TOOL_C_ACT 26
  #define TOOL_D_ACT 26
  #define TOOL_B_SEL 26
  #define TOOL_C_SEL 26
  #define TOOL_D_SEL 26

  // set as inputs - again set to same pin on GAMMAs to free up other pins
  #define TOOL_B_LOOP 25
  #define TOOL_C_LOOP 25
  #define TOOL_D_LOOP 25
#endif

// Thermal sensor type defines
#define ONEWIRE 1							// 1wire temp sensor... cannot get very hot
#define RTD 2								// PT100 RTDs (default if "H" or "T" not specified)
#define RTDH 2								// PT100 RTDs ("H"undred)
#define RTDT 3								// PT1000 RTDs ("T"housand)
#define THERMISTER 4							// thermisters
#define THERMOCOUPLE 5							// thermocouples

// Setup PWM interface
#define PCA9685_MODE1 0x0						// sets to default values - see manual pg 14
#define PCA9685_PRESCALE 0xFE
#define PWM0_ON_L 0x6							// Define first LED and all LED. We calculate the rest
#define PWMALL_ON_L 0xFA
#define PIN_ALL 16
#define PIN_BASE 300
#define MAX_PWM 4095

// Simplify bit operations
#define bitset(byte,nbit)   ((byte) |=  (1<<(nbit)))
#define bitclear(byte,nbit) ((byte) &= ~(1<<(nbit)))
#define bitflip(byte,nbit)  ((byte) ^=  (1<<(nbit)))
#define bitcheck(byte,nbit) ((byte) &   (1<<(nbit)))

// Defines for GTK interface
#define LCD_WIDTH 1000
#define LCD_HEIGHT 750

// Defines for cairo colors
// these colors more or less match the default gtk color chooser grid
#define BLACK 0
#define DK_GRAY 3							// "DK" = DarK gray
#define DM_GRAY 4							// "DM" = Dark-Medium gray
#define MD_GRAY 5							// "MD" = MeDium gray
#define ML_GRAY 6							// "ML" = Medium-Light gray
#define LT_GRAY 7							// "LT" = LighT gray
#define SL_GRAY 8							// "SL" = Super-Light gray
#define DK_RED 13
#define DM_RED 14
#define MD_RED 15
#define ML_RED 16
#define LT_RED 17
#define SL_RED 18
#define DK_ORANGE 23
#define DM_ORANGE 24
#define MD_ORANGE 25
#define ML_ORANGE 26
#define LT_ORANGE 27
#define DK_YELLOW 33
#define DM_YELLOW 34
#define MD_YELLOW 35
#define ML_YELLOW 36
#define LT_YELLOW 37
#define DK_GREEN 43
#define DM_GREEN 44
#define MD_GREEN 45
#define ML_GREEN 46
#define LT_GREEN 47
#define DK_CYAN 53
#define DM_CYAN 54
#define MD_CYAN 55
#define ML_CYAN 56
#define LT_CYAN 57
#define DK_BLUE 63
#define DM_BLUE 64
#define MD_BLUE 65
#define ML_BLUE 66
#define LT_BLUE 67
#define DK_BROWN 73
#define DM_BROWN 74
#define MD_BROWN 75
#define ML_BROWN 76
#define LT_BROWN 77
#define DK_VIOLET 83
#define DM_VIOLET 84
#define MD_VIOLET 85
#define ML_VIOLET 86
#define LT_VIOLET 87
#define WHITE 90

// Generic list structure for general use
typedef struct st_genericlist
	{
	int			ID;					// unique identifier
	char			name[255];				// unique name
	struct	st_genericlist	*next;					// pointer to next name in linked list
	}genericlist;
extern	genericlist *tool_list;						// pointer to first tool name in index

// Arrayed vertex structure for OpenGL interface
typedef struct st_svtx
	{
	float	x,y,z,w;
	}svtx;
extern 	int		ndvtx;
extern	svtx		dvtx[MAX_VERTS];
extern	svtx		dnrm[MAX_VERTS];

// Physical position structure for TinyG interface
typedef struct st_position 						// Contains position information for carriage location
	{
	double x;							// x location			 
	double y;							// y location
	double z;							// z location
	double a;							// a location
	}position;
	
extern position PosIs,PosWas,PostG;					// current positon, previous position, last tinyG reported position

// Vertex structure for both 3D models AND 2D slices
typedef struct st_vertex{						// 3D point description.
	float			x;     					// Cartesian X
	float 			y;					// Cartesian Y
	float			z;					// Cartesian Z
	float		 	i;					// Moment about X, or scratch, or weight (attraction value) for support tree
	float 			j;					// Moment about Y, or scratch, or current cost for support tree
	float 			k;					// Moment about Z, or scratch  or estimated remaining cost for support tree
	int			supp;					// flag to indicate if this vtx location will need support/cooling
	int			attr;					// Attribute and/or reusable generic descriptor
	struct st_facet_list	*flist;					// Pointer to facet list this vertex is referenced by in faceted geometries
	struct st_vertex 	*next;					// Pointer to next vertex in list
	}vertex;

extern vertex	*vtest_array;

// Gerber aperature structure
typedef struct st_gbr_aperature{					// gerber aperature definitions
	int			ID;
	int			type;
	float			diam;
	float			X,Y;
	float			hole;
	char			cmt[255];
	struct	st_gbr_aperature *next;					// pointer to next aperature in library
	}gbr_aperature;
  
// Edge structure.  A simplified version of the vector structure.
typedef struct st_edge{							// 3D vector description
	struct st_vertex 	*tip;					// Start point.
	struct st_vertex 	*tail;					// End point.
	struct st_facet		*Af;					// Pointer to one of the two facets it represents
	struct st_facet		*Bf;					// Pointer to the other facet this edge represents
	int			status;					// Reusable flag to set status during computations
	int			display;				// Display flag
	int			type;					// Generic indicator for edge type
	struct st_edge		*next;					// Pointer to next vector in list.
	}edge;
	
// Facet structure for use with STL/AMF/3DM type formats
typedef struct st_facet{						// STL facet structure
	struct st_vertex	*vtx[3];				// Pointers to 3 vertex corners of facet
	struct st_vertex 	*unit_norm;				// Pointer to unit normal vertex
	struct st_facet 	*fct[3];				// Pointers to 3 neighboring facets: fct0 is along vtx0 -> vtx1, etc.
	int			member;					// Defines which solid body this facet is a member of
	float 			area;					// Area of facet in mm^2
	int			status;					// Reusable flag to set status during computations
	int			attr;					// Attribute and/or reusable generic descriptor
	int			supp;					// Flag used to indicate if this facet requires edge support
	struct st_facet 	*next;					// Pointer to next facet in list
	}facet;

// Polygon structure
typedef struct st_polygon{						// Polygons hold the organized list of vectors from the vector list
	struct st_vertex  	*vert_first;				// pointer to first vertex in list of vertices that define this polygon's perimeter
	long int	  	vert_qty;				// qauntity of verticies in this polygon
	int			ID;					// carries the polygons ID
	int		 	member;					// carries the parent polygon ID
	int			mdl_num;				// carries the parent model ID
	int		 	type;					// indicates which line type to use to draw this polygon
	int			hole;					// flag to indicate if it encloses material or a hole.. 0=Mat'l, 1=Hole, 2=Drill hole, etc.
	float 			diam;					// diameter of hole if such is the case.  used for subtractive tool bit changes.
	float 			prim;					// perimeter length of the polygon
	float 			area;					// area of the polygon
	float 			dist;					// distance this polygon has been offset from its original perimeter
	float 			centx,centy;				// coordinates of the polygons centroid
	int			status;					// re-usable status flag for processing
	int			perim_type;				// indicates if none, from file, or custom : (-1)=undefined, 0=none, 1=from file, 2=custom
	struct st_linetype	*perim_lt;				// pointer to node of custom perimeter line type definition for this polygon
	int			fill_type;				// indicates if none, from file, or custom : (-1)=undefined, 0=none, 1=from file, 2=custom
	struct st_linetype	*fill_lt;				// pointer to node of custom fill line type definition for this polygon
	struct st_polygon_list 	*p_child_list;				// pointer to list of child polygons enclosed by this polygon
	struct st_polygon 	*p_parent;				// pointer to parent polygon (NULL if outermost, !NULL if holes or nested)
	struct st_polygon 	*next;					// pointer to next polygon in this list
	}polygon;

// Vector structure
typedef struct st_vector{						// Vector list temporarily holds the random collection of vectors from slicing
	struct st_vertex 	*tip;					// start point
	struct st_vertex 	*tail;					// end point
	polygon			*psrc;					// the source polygon from which it came
	struct st_vector	*vsrcA;					// the source vector from which it came 
	struct st_vector	*vsrcB;					// the source vector from which it came 
	struct st_vector 	*vtgtA;					// the target vector that this vector made
	float 			curlen;					// current vector length
	float 			angle;					// vector angle relative to x axis
	int 			member;					// used to ID groups of consquetive vectors (i.e. precursers to polygons)
	int			type;					// scratch for holding whatever is needed at the time
	int			status;
	int			crmin,crmax;
	int 			wind_dir,wind_min,wind_max;		// wind_dir=0(CW) or >0(CCW), min=winding number to zero, max=winding number to table max
	int			pwind_min,pwind_max;			// save original winding numbers prior to offsetting for comparison
	struct st_vector 	*next;					// next vector in list
	struct st_vector 	*prev;					// prev vector in list
	}vector;
	
// Data structure for generic vertex lists
typedef struct st_vertex_list
	{
	vertex			*v_item;
	struct st_vertex_list	*next;
	}vertex_list;
extern	vertex_list		*vtx_pick_list;

// Data structure for generic edge lists
typedef struct st_edge_list
	{
	edge			*e_item;
	struct st_edge_list	*next;
	}edge_list;
extern	edge_list		*e_pick_list;

// Data structure for generic vector lists
typedef struct st_vector_list
	{
	vector			*v_item;
	struct st_vector_list	*next;
	}vector_list;
extern	vector_list		*vec_pick_list;

// Data structure for generic polygon lists
typedef struct st_polygon_list
	{
	polygon			*p_item;				// address of polygon in list
	struct st_polygon_list	*next;					// pointer to next element in list
	}polygon_list;
extern	polygon_list		*p_pick_list;				// user pick list of polygons

// Data structure for generic facet lists
typedef struct st_facet_list
	{
	facet			*f_item;
	struct st_facet_list	*next;
	}facet_list;
extern	facet_list		*f_pick_list;

// Data structure for vector cell
typedef struct st_vector_cell
	{
	int			id_x;					// cell id in x axis
	int			id_y;					// cell id in y axis
	struct st_vector_list	*vec_cell_list;				// vector list of vectors that influence this cell
	struct st_vector_cell	*next;					// pointer to next cell in list
	}vector_cell;

// Data structure for patch of facets/vtxs
typedef struct st_patch{						// a collection of facets that cover a patch
	int 			ID;					// unique ID number for each patch
	struct st_facet_list	*pfacet;				// pointer to first facet in list of facets that form the patch
	struct st_vertex_list	*pvertex;				// pointer to first vertex in list of vtxs that form the patch
	struct st_patch		*pchild;				// pointer to sub-patches (typically lower patches) of this patch
	struct st_polygon	*free_edge;				// pointer to head node of free edge 3D polygons (outer is head, holes are next)
	float 			area;					// area of patch (sum of 3D polygon only in XY enclosing patch)
	float 			minx,maxx;				// min and max x of patch
	float 			miny,maxy;				// min and max y of patch
	float 			minz,maxz;				// min and max z height of patch
	struct st_patch 	*next;					// pointer to next patch in list
	}patch;

// Data structure for support branches
typedef struct st_branch{
	struct st_vertex	*node;					// a pointer to the head node of a branch, typically highest z
	struct st_vertex	*goal;					// a pointer to the goal node of a branch, typically lowest z
	int 			wght;					// the weight of this branch (i.e. its attraction value)
	int			status;					// tracks state of branch during growing/faceting process
	struct st_polygon 	*pfirst;				// a pointer to the polygon list used to create facets
	struct st_patch		*patbr;					// a pointer to the patch under which this node exists
	struct st_branch	*next;					// pointer to next branch in the list
	}branch;

// Data structure for branch lists
typedef struct st_branch_list{
	branch			*b_item;
	struct st_branch_list	*next;
	}branch_list;
extern	branch_list		*b_pick_list;

// Data structure for slices
typedef struct st_slice{						// Slices hold a collection of polygons for each tool.  Typically several slices at a time.
	int			ID;					// typically layer number
	struct st_model		*msrc;					// pointer to source model
	struct st_polygon	*pfirst[MAX_LINE_TYPES];		// pointer to first polygon of line type (array value)
	struct st_polygon	*plast[MAX_LINE_TYPES];			// pointer to last polygon of line type (array value)
	int			pqty[MAX_LINE_TYPES];			// quantity of polygons of this line type in this slice
	float 			pdist[MAX_LINE_TYPES];			// total distance of deposition of this line type in this slice
	float 			ptime[MAX_LINE_TYPES];			// time required to deposit this line type
	int			stype[MAX_LINE_TYPES];			// re-usable flag to indicate status/type/process
	long int		perim_vec_qty[MAX_LINE_TYPES];
	vector 			*perim_vec_first[MAX_LINE_TYPES];	// pointer to vector representation of model perimeter
	vector 			*perim_vec_last[MAX_LINE_TYPES];
	long int		ss_vec_qty[MAX_LINE_TYPES];		// quantity of vectors held in straight skeleton linked list
	vector			*ss_vec_first[MAX_LINE_TYPES];		// pointer to first vector in linked list
	vector			*ss_vec_last[MAX_LINE_TYPES];		// pointer to last vector in linked list
	long int		raw_vec_qty[MAX_LINE_TYPES];		// quantity of vectors held in raw_vec_first linked list
	vector			*raw_vec_first[MAX_LINE_TYPES];		// pointer to first element in raw vectors resulting from slicing
	vector			*raw_vec_last[MAX_LINE_TYPES];		// pointer to last element in raw vector linked list
	struct st_vector_list	*vec_list[MAX_LINE_TYPES];		// vector list pointers (i.e. an index into the raw vector lists)
	vector 			*stl_vec_first;
	float 			cell_size_x,cell_size_y;		// vector grid cell sizes in x and y
	struct st_vector_cell	*vec_grid;				// xy grid of vector list pointers (i.e. an index into the raw vector lists)
	gbr_aperature		*gap_first;				// Pointer to first gerber aperature for gerber files
	struct st_polygon	*p_zero_ref;				// centroid of this polygon is reference to zero_ref of other slices
	struct st_polygon	*p_one_ref;				// centroid of this polygon used to calc rotation with p_zero for alignment reference
	float 			xmin,ymin;				// minimum values for this slice
	float 			xmax,ymax;				// maximum values for this slice
	float 			sz_level;				// the z level that this slice was created from
	float 			time_estimate;				// time estimate to build up to (not of) this slice
	float 			matl_estimate;				// material estimate to build up to (not of) this slice
	int			eol_pause;				// flag to pause at end of layer
	struct st_slice		*prev;					// pointer to previous slice in linked list
	struct st_slice		*next;					// pointer to next slice in linked list
	}slice;
	
extern	slice	*heatmap;						// pointer to autogenerated support material slice

// Data structure for models
typedef struct st_model{
	
	// model fundamentals
	int			model_ID;				// Unique ID of this model.  Assigned when loaded.
	char 			model_file[255];			// Holds physical name and location of the source model file
	char	 		cmt[255];				// Holds general comments
	int			geom_type;				// Holds geometry type of model:  MODEL, SUPPORT, INTERNAL, TARGET, etc.
	int			input_type;				// Holds input type of model:  0=undefined, 1=STL, 2=AMF, 3=GERBER, 4=IMAGE
	float 			slice_thick;				// Slice thickness for this model
	float		 	current_z;				// current z level while printing
	int			error_status;				// Holds type of error:  0=none(good), 1=free vtx, 2=free facet, 3=facet overlap...
	
	// this group of model variables need to be independent per their geometry type (MODEL, SUPPORT, TARGET, etc.) not 
	// just in their geometric definition but with things like position in Z or scale as well
	vertex 			*vertex_index[MAX_MDL_TYPES];		// A linked list of vtxs that can be used as a hash table for quick look-up
	long int		vertex_qty[MAX_MDL_TYPES];		// Keeps track of maxium number of vertices in linked list
	vertex			*vertex_first[MAX_MDL_TYPES];		// Pointer to first vertex in linked list
	vertex 			*vertex_last[MAX_MDL_TYPES];		// Pointer to last vertex in linked list
	long int		facet_qty[MAX_MDL_TYPES];		// Quantity of model facets used to define the model
	facet 			*facet_first[MAX_MDL_TYPES];		// Pointer to first facet in list of facets that define the model
	facet			*facet_last[MAX_MDL_TYPES];		// Pointer to last facet in linked list
	long int		edge_qty[MAX_MDL_TYPES];		// Keeps track of maxium number of edges in linked list
	edge			*edge_first[MAX_MDL_TYPES];		// Pointer to first edge in linked list
	edge			*edge_last[MAX_MDL_TYPES];		// Pointer to last edge in linked list
	float 			xmin[MAX_MDL_TYPES],ymin[MAX_MDL_TYPES],zmin[MAX_MDL_TYPES]; // Minimum vertex values for each coordinate in its given orientation
	float 			xmax[MAX_MDL_TYPES],ymax[MAX_MDL_TYPES],zmax[MAX_MDL_TYPES]; // Maximum vertex values for each coordinate in its given orientation
	float 			xoff[MAX_MDL_TYPES],yoff[MAX_MDL_TYPES],zoff[MAX_MDL_TYPES]; // coordinate offset of this model from build plate origin
	float 			xorg[MAX_MDL_TYPES],yorg[MAX_MDL_TYPES],zorg[MAX_MDL_TYPES]; // coordinate offsets as above, but a reserved copy
	float 			xrot[MAX_MDL_TYPES],yrot[MAX_MDL_TYPES],zrot[MAX_MDL_TYPES]; // total model rotation values from original orientation
	float 			xrpr[MAX_MDL_TYPES],yrpr[MAX_MDL_TYPES],zrpr[MAX_MDL_TYPES]; // previous model rotation values
	float 			xscl[MAX_MDL_TYPES],yscl[MAX_MDL_TYPES],zscl[MAX_MDL_TYPES]; // model scale values
	float 			xspr[MAX_MDL_TYPES],yspr[MAX_MDL_TYPES],zspr[MAX_MDL_TYPES]; // previous model scale values
	int			xmir[MAX_MDL_TYPES],ymir[MAX_MDL_TYPES],zmir[MAX_MDL_TYPES]; // model plane mirror flags
	
	// the following group of variables applie to the tool types used on this model
	long int		slice_qty[MAX_TOOLS];			// Keeps track of the maximum number of operations called out for this model
	slice 			*slice_first[MAX_TOOLS];		// Pointer to first operation to execute in linked list of operations for this model
	slice			*slice_last[MAX_TOOLS];			// Pointer to last operation to execute in linked list of operations for this model

	// flags
	int			MRedraw_flag;				// Flag to indicate if silhoette edge list needs to be regenerated
	int			reslice_flag;				// Indicates if model requires slicing
	
	// the following group of variables applies to all model types held for each input model
	edge 			*silho_edges;				// Pointer to list of silhoett edges to display
	patch 			*patch_model;
	patch 			*patch_first_upr;
	patch 			*patch_first_lwr;
	genericlist		*oper_list;				// pointer to a sequenced list of operations to perform on this model (slot & operation name)
	slice			*base_layer;				// Pointer to base layer that goes between build table and bottom layer of model
	slice			*plat1_layer;				// Pointer to platform 1 layer that goes between build table and bottom layer of model
	slice			*plat2_layer;				// Pointer to platform 2 layer that goes between build table and bottom layer of model
	slice			*ghost_slice;				// Pointer to last visible slice for display purposes
	
	GdkPixbuf		*g_img_mdl_buff;			// pointer to original image associated with this model
	int			mdl_n_ch,mdl_cspace,mdl_alpha,mdl_bits_per_pixel;	// image params
	int 			mdl_height,mdl_width,mdl_rowstride;			// image params
	guchar 			*mdl_pixels;				// pointer to pixel data
	GdkPixbuf		*g_img_act_buff;			// pointer to active image associated with this model
	int			act_n_ch,act_cspace,act_alpha,act_bits_per_pixel;	// image params
	int 			act_height,act_width,act_rowstride;			// image params
	guchar 			*act_pixels;				// pointer to pixel data
	float 			pix_size;				// conversion factor between pixels and mm
	int			pix_increment;				// controls resolution of STLs generated from images
	int			show_image;				// flag to turn on/off image viewing
	int			image_invert;				// flag to indicate image color inversion (T=inverted)

	int			grayscale_mode;				// flag to indicate type of grayscale: 1=PWM, 2=Z Ht, 3=Vel
	float 			grayscale_zdelta;			// max z height to move laser out of focus
	float 			grayscale_velmin;			// min vel to move when laser is on

	int			total_copies;				// Total number of copies to print of this model
	int			active_copy;				// Current copy being printed/in-used
	int			xcopies,ycopies;			// Number of copies of this model to print in each direction
	float 			xstp,ystp;				// Space between copies for autoplacement

	float 			svol[6];				// support volume needed for each direction (zmin,ymin,xmin,zmax,ymax,xmax)
	float 			lyrs[3];				// layers needed for each direction (zmin,ymin,xmin,zmax,ymax,xmax)
	float 			base_lyr_zoff;				// thickness of baselayer for this model

	float 			target_x_margin,target_y_margin;	// margins beyond model max/min to mill
	float 			target_z_height,target_z_stop;		// raw matl block height to be milled and where model stops in z
	int			mdl_has_target;				// flag to indicate if raw matl target has been added to model

	int			vase_mode;				// flag to indicate if being built with or without fill
	int			btm_co,top_co,internal_spt; 		// flags to specify how to apply vase mode
	float 			set_wall_thk;				// wall thickness over-ride
	float 			set_fill_density;			// fill density over-ride
	int			fidelity_mode;				// high fidelity mode
	float 			support_gap;				// the gap between model and support materails

	float 			time_estimate;				// time estimate to build this model
	float 			matl_estimate;				// material estimate to build this model
	
	int			vert_surf_val,botm_surf_val;		// side wall and bottom of model surface quality (post complete eval)
	int			lwco_surf_val,upco_surf_val;		// lower close-off and upper close-off surface quality 

	float 			facet_prox_tol;				// facet proximity tolerance
	struct st_model		*next;					// Pointer to next model in memory
	}model;

extern model			*active_model;				// pointer to user selected active model

// Data structure for eeprom memory devices that carry the specific calibration data associated with each tool
// Note that the generic data for a tool is kept in TOOLS.XML.  The eeprom data is linked via the toolID value.
typedef struct st_eeprom
	{
	char		rev[8];						// revision number of this memory format 
	char		name[32];					// unique condensed reference name of tool
	char 		manf[32];					// name of manufacturer
	char		date[10];					// date of manufacture
	char		matp[32];					// name of last material used by this tool
	int		sernum;						// unique serial number of this tool - typically 1wire ID
	int		toolID;						// unique numeric ID of tool as found in TOOLS.XML - links tool to file info
	float 		powr_lo_calduty;				// low power calib point in duty cycle
	float 		powr_lo_calvolt;				// matching low power calib point in volts (typ 0% duty = 0 volts)
	float 		powr_hi_calduty;				// high power calib point in duty cycle
	float 		powr_hi_calvolt;				// matching high power calib point in volts (typ 100% duty = 48 volts)
	float		temp_lo_caltemp;				// low temperature calib point in C
	float		temp_lo_calvolt;				// matching low temp calib point in volts (A2D conversion)
	float		temp_hi_caltemp;				// high temperature calib point in C
	float		temp_hi_calvolt;				// matching high temp calib point in volts
	float		tip_pos_X;					// x axis manuf tolerance differences for this specific tool
	float		tip_pos_Y;					// y axis manuf tolerance differences for this specific tool
	float		tip_pos_Z;					// z axis manuf tolerance differences for this specific tool
	float 		tip_diam;					// tip diameter
	float		extra_calib[4];					// extra storage space
	}eeprom;

// Tool memory management - a sub-structure to each tool
typedef struct st_memdev						// Contains memory device information
	{
	FILE		*fd;						// handle to device
	char		dev[20];					// device ID
	char		devPath[255];					// path to device
	char		desc[40];					// desciption of device
	int		devType;					// 23 for eeprom, 28 for thermal, etc.
	eeprom		md;						// memory data
	}memdev;
	
extern int memory_devices_found;					// number of memory devices actually found
extern int thermal_devices_found;					// number of thermal devices actually found

// Line type management - a sub-structure to each material
typedef struct st_linetype
	{
	int 	ID;							// unique line type ID. See #defs for PERIM, OFFSET, FILL, SUPPORT, TRACE, DRILL
	char 	name[255];						// line type name:  Perimeter, Offset, etc.
	float 	tempadj;						// temperature adjustment off nominal
	float 	flowrate;						// amount of material deposited per unit length drawn
	float 	feedrate;						// speed at which a tool is moved while drawing/depositing/cutting
	float 	line_width;						// width of a single additive deposition/cut/laser line width in mm
	float 	line_pitch;						// pitch between nieghboring passes of single passes that form "walls"
	float 	wall_width;						// width of a collection of additive deposition/cut/laser line width in mm
	float 	wall_pitch;						// pitch between nieghboring passes of a collection of passes that form "walls"
	float 	fidelity_line_width;					// narrow line width for capturing all features, but not dimensionally accurate
	float 	move_lift;						// height to lift tool tip during pen-up moves
	float 	eol_delay;						// forced delay in mili-seconds at the end of each vector
	float 	retract;						// distance to retract material at start of pen up
	float 	advance;						// distance to advance material at start of pen down
	float 	touchdepth;						// distance to push into model/substrate at start of pen down
	float 	minveclen;						// minimum acceptable vector length
	float 	height_mod;						// z direction offset from standard layer height
	int	air_cool;						// turn on/off air cooling during this line type
	int 	poly_start_delay;					// mili second delay to allow pressure to build before a new polygon
	float 	thickness;						// the total width of the resulting contour OR the size spacing for fills
	char	pattern[30];						// type of pattern to use
	float 	p_angle;						// angle (deg) in XY plane to draw pattern
	struct st_linetype	*next;					// pointer to next element in list
	}linetype;

// Material management - a sub-structure to each tool
typedef struct st_material						// Contains material specific information typically read from XML file
	{
	int 	ID;							// arbitrary ID of material type - see Materials.XML
	char 	name[32];						// space for name of material type
	char 	description[128];					// description of the material
	char 	brand[128];						// brand and/or manufacturer of the material
	int	color;							// color of material tool deposits - see defines
	float 	diam;							// diam of material supply (typically filament)
	int 	gage_ID;						// arbitrary ID of tip type
	int 	state;							// current state: 0=undefined 1=defined
	float 	layer_height;						// deposited layer height
	int 	min_layer_time;						// minium time per layer to let material cool/solidify
	float 	overhang_angle;						// max angle of facet before support is needed
	float 	shrink;							// shrinkage in percent
	float 	retract;						// material retraction amount when tool retracts from deposit position
	float 	advance;						// material advance amount when tool moves into deposit position
	float 	primepush;						// initial distance to compress plunger to start flow
	float 	primepull;						// distance to retract plunger after flow starts
	float 	primehold;						// duration to hold at primepush position before retracting to primepull position
	float 	mat_volume;						// material reservoir/spool/cartridge volume size when full (mL)
	float 	mat_used;						// volume of material currently used (mL)
	float 	mat_pos;						// stepper position for this cartridge (mm from full position)
	int	max_line_types;						// number of different line types defined
	linetype	*lt;						// pointer to first node of line type definition linked list (note this is NOT their execution sequence!)
	}material;

// Tool thermal management - a sub-structure to each tool
typedef struct st_thermal		 				// Contains thermal device information
	{
	int	sync;							// flag used to syncronize public data with thermal thread local data
	int	sensor;							// 0=None, 1=1wire, 2=RTD(PT100), 3=RTD(PT1000), 4=thermistor, 5=thermocouple
	int	fd;							// handle to device
	char	dev[16];  						// device ID
	char 	devPath[128]; 						// path to device
	char	desc[40];						// description of device
	float 	maxtC;							// maximum allowable operating temperature in C
	float 	tempC;							// current temperature in C
	float 	tempV;							// corrisponding voltage to provide tempC
	float 	temp_offset;						// calibration offset
	float 	setpC;							// setpoint temperature in C
	float 	tolrC;							// acceptable tolerance of temp in C
	float 	backC;							// setback temperature in C
	float 	operC;							// operating temperature in C
	float 	hystC;							// allowable drop before turn-on
	float 	waitC;							// temp adjustment to retracted tools
	float 	bedtC;							// build table operating temperature in C
	float 	errtC;							// forced steady state error to drive PID loop
	float 	duration_per_C;						// the secs it takes to change 1 deg C at full duty
	float 	Kp,Ki,Kd;						// thermal PID constants
	float 	calib_v_low,calib_v_high;				// voltage-to-temp calibration voltages
	float 	calib_t_low,calib_t_high;				// voltage-to-temp calibration temperatures
	float 	start_t_low,start_v_low;				// temp and volts at time when tool is attached to carriage
	float 	setpC_t_low,setpC_t_high;				// temperature low and high set points for calibration
	int	temp_override_flag;					// flag to indicate user has overridden default temps
	char	temp_port;						// channel of A2D for temperature sensor
	char	PWM_port;						// channel of PWM for heater modulation
	int	heat_status;						// 0=off, 1=on
	time_t 	heat_time;						// duration in seconds of heat on status
	int	heat_duration;						// current duty cycle for this device (PWM control)
	int	heat_duration_max;					// maximum duty cycle for this device (PWM control)
	float	heat_duty;						// maximum duty cycle for this device (PWM control)
	}thermal;
	
extern pthread_t	thermal_tid;					// thread to manage thermal stuff
extern pthread_mutex_t 	thermal_lock;					// mutex to manage thermal thread variables
extern int		thermal_thread_alive;				// detectable heartbeat to ensure thread is still alive
extern float		temperature_T;					// value output of thermistor (volts or temp)
extern float 		temp_calib_delta;				// the temp difference usde to set high/low calib points

extern	pthread_t 	build_job_tid;					// thread to manage printing
extern	int		print_thread_alive;				// detectable heartbeat to ensure thread is still alive

// Tool power management - a sub-structure to each tool
typedef struct st_power
	{
	int	power_sensor;						// 0=None, 1=voltage, 2=current, 3=force
	char	power_status;						// indicator if on or off
	float	power_duration;						// maximum duty cycle for this device (PWM control)
	float 	power_duty;						// current power duty cycle (PWM control) value bt. 0.00 and 1.00
	char	feed_back_port;						// channel of A2D for feedback, if any
	char	PWM_port;						// channel of PWM for power modulation
	float 	calib_v_low,calib_v_high;				// voltage-to-duty cycle calibration voltages
	float 	calib_d_low,calib_d_high;				// voltage-to-cuty cycle calibration duty values
	float 	setpC_d_low,setpC_d_high;				// duty cycle low and high set points for calibration
	}power;

// Tool operation management - a sub-structure to each tool
typedef struct st_operation
	{
	int			ID;					// ID of the operation (see operation defines)
	char			name[255];				// name of the operation
	int			type;					// type of operation (additive, subtractive, etc.)
	int			active;					// flag to indicate if active for this job
	int			status;					// flag to indicate if ready for print or needs re-generation (slicing, etc.)
	int			geo_type;				// defines what nature of geometry is needed (slice, surface, voxel, etc.)
	genericlist		*lt_seq;				// pointer to the linetype sequence as listed by ID and name
	struct st_operation	*next;					// pointer to next operation in list
	}operation;

// Top Level Tool Data Structure
typedef struct st_tool
	{	
	int	state;							// indicator if tool is loaded or not (<0=Not there, 1=there but not enough info, 2=defined, 3=define & homed & ready for use)
	int	select_status;						// indicator if tool has been selected for use in current job
	int	used_status;						// indicator if tool has been used on a specific layer or not
	int	sernum;							// unique tool identifier... typically 1wire serial number
	int 	tool_ID;						// unique tool type identifier (FDM, Router, Laser, etc.)
	int	type;							// indicator as to nature of tool (additive, subtractive, measurement, etc.)
	float 	weight;							// weight of tool in grams
	char 	name[32];						// unique condensed name:  FDM14, EXTRUDER, ROUTER, etc.
	char	desc[32];						// description of tool
	char	manuf[32];						// manufacturer of tool
	float	x_offset,y_offset;					// tool tip center offsets from carriage center
	float 	tip_x_fab,tip_y_fab,tip_z_fab;				// fabrication delta of this tool's tip vs theoretical
	vertex 	*swch_pos;						// trigger locations for tip calibration
	float 	tip_up_pos;						// tool tip vertical location when NOT depositing
	float 	tip_dn_pos;						// tool tip vertical location when depositing
	float 	tip_cr_pos;						// tool tip current vertical location in mm
	int	homed;							// flag if tool has been "homed" or not on the carriage (not to be confused with mech xyz homing)
	int	PWM_is_set;						// flag to confirm thermal thread set PWM
	bool	pwr48,pwr24,spibus,wire1,limitsw,pwmtool;		// tool characteristics:  true=tool has it, false=tool does NOT have it
	bool 	epwr48,epwr24,espibus,ewire1,elimitsw,epwmtool;		// error status:  true=component in error, false=component okay
	float 	tip_diam;						// tool tip/bit diameter
	float 	max_drill_diam;						// max size to drill, bigger holes will be milled
	int	step_ID,step_max;					// indicator of which stepper on the tool is active, and those available on tool
	float 	stepper_drive_ratio;					// scale factor to acount for various pulley sizes
	struct	timespec	active_time_stamp;			// time stamp of when tool is defined
	struct 	st_memdev	mmry;					// information about memory device on this tool
	struct 	st_thermal	thrm;					// information about thermal management of this tool
	struct 	st_material	matl;					// information about material this tool is using
	struct 	st_power	powr;					// information about power control for this tool
	int 			oper_qty;				// keeps the number of available operations found for this tool
	operation		*oper_first;				// pointer to the first available operation found
	operation		*oper_last;				// pointer to the last available operation found
	genericlist 		*mats_list;				// pointer to list of materials this tool supports
	genericlist		*oper_list;				// pointer to list of operations this tool can perform
	}toolx;
extern toolx Tool[MAX_THERMAL_DEVICES];					// array count of tools plus build table plus chamber

// Carriage data structure
typedef struct st_carriage
	{
	float 	x_center;						// distance from center of y-rail to face of tool plate
	float 	y_center;						// distance from center of carriage (i.e. camera) to center of tool plate between mounting posts
	float 	x_offset;						// delta between true home and x limit switch
	float 	y_offset;						// delta between true home and y limit switch
	float 	z_offset;						// delta between true home and z limit switch
	float 	camera_focal_dist;					// distance from object to position camera in z
	float 	camera_z_offset;					// delta distance between camera measurement and probe measurement
	float 	camera_spin;						// amount of spin off xy square alignment (i.e. turned clockwise = +val, turned ccw = -val)
	float 	camera_tilt;						// amount of tilt off horizontal (i.e. horz=0.0, tilt toward tool = +val, tilt away from tool = -val)
	float 	temp_offset;						// temperature offset specific to each slot location
	float 	volt_offset;						// volt offset specific to each slot location
	float 	calib_t_low,calib_t_high;				// voltage-to-temp calibration temperatures
	float 	calib_v_low,calib_v_high;				// voltage-to-temp calibration voltages
	}carriage;
	
extern carriage crgslot[MAX_THERMAL_DEVICES];

// Job data structure
typedef struct st_job
	{
	int			sync;					// flag used to syncronize public data with print thread local data
	int			state;					// current status of the job
	int			prev_state;				// previous status of the job
	int			regen_flag;				// indicates if job values need refresh
	int			type;					// additive, subtractive, marking, mixed, etc.
	int			model_count;				// number of models currently loaded in this job
	int			max_layer_count;			// total number of layers in this job
	float 			min_slice_thk;				// the thinnest slice thickness called out by all loaded tools
	int			baselayer_flag;				// indicates if base layer is requested
	int			baselayer_type;				// which type - simple bounding box or tight fit
	float 			XMax,YMax,ZMax;				// overall max dimensions of job
	float 			XMin,YMin,ZMin;				// overall min dimensions of job (typically 0,0,0)
	float 			XOff,YOff,ZOff;				// overall offset dimensions of min coords from 0,0,0
	float 			current_z;				// current z level of build
	float 			time_estimate;				// amount of time (secs) this job is estimated to take to build
	float 			current_dist;				// the current amount of tool travel while running a job
	float 			penup_dist;				// total distance of tool travel for non-deposit moves
	float 			pendown_dist;				// total distance of tool travel for deposit moves
	float 			total_dist;				// total distance of tool travel in the job
	time_t 			start_time,finish_time;			// duration in seconds that the job has been running
	model			*model_first;				// pointer to first model associated with this job
	branch 			*support_tree;				// pointer to first branch in support tree associated with this job
	facet_list 		*lfacet_upfacing[MAX_MDL_TYPES];	// Pointer to first facet of list that only faces upwards 
	}jobx;
extern	jobx job;			

// params for user interaction with geometry
extern vertex 			*vtx_pick_new;				// pointer to user "picked" vertex
extern vertex 			*vtx_pick_ref;				// pointer to previously picked vtx
extern vertex			*vtx_pick_ptr;				// pointer to another prev picked vtx
extern vertex			*vtx_zero_ref;				// pointer to picked zero reference
extern facet 			*fct_pick_ref;				// pointer to picked facet

// hash table indexing pointer used to speed up vtx searching
extern vertex			*vtx_index[VTX_INDEX][VTX_INDEX][VTX_INDEX];	// array of ptrs to speed vtx loading and searching [X][Y][Z]

// Command buffer structure - list of commands currently in process with motion controller
typedef struct st_cmdBuffer
	{
	char			cmd[255];				// copy of gcode command
	unsigned long 		tid;					// transaction ID
	unsigned long 		vid;					// vertex ID (count)
	vertex 			*vptr;					// vertex ID from which the move originated
	}cmdBuffer;
extern cmdBuffer		cmd_buffer[MAX_BUFFER];			// holds FIFO index of commands sent to tinyG pending execution

// tinyG status variables
  extern int bufferAvail;						// tracks buffering status into tinyG controller
  extern int bufferAdded;						// note: buffers do not equal commands...
  extern int bufferRemoved;						// one buffer can be stuffed with multiple commands
  extern int Rx_bytes;							// number of bytes currently in rcv buffer of tinyG
  extern int Rx_max;							// rcv buffer size of tinyG
  extern int cmd_que;
  extern int cmdCount;							// tracks number of commands pending in tinyG buffer
  extern int vtxCount;
  extern int cmdBurst;							// sets the limit for pending commands with the tinyG
  extern int cmdControl;						// flag to indicate if counting cmds or not
  extern int oldState;
  extern int burst;							// flag to indicate if to send moves or accumulate moves
  extern int crt_min_buffer;
  extern unsigned long TId;						// transaction ID for each command sent to tinyG
  extern unsigned long VId;						// vertex ID for each command sent to tinyG

// device handles
  extern int 	USB_fd;							// handle to tinyG stepper controller
  extern int	I2C_A2D_fd[2];						// handle to A2D on I2C bus
  extern int	I2C_PWM_fd;						// handle to PWM on I2C bus
  extern int	I2C_DS_fd;						// handle to Distance Sensor on I2C bus
  extern int 	SPI_fd;							// handle to SPI bus devices
  //extern struct 	wiringPiNodeStruct *node;

// file handles
  extern FILE 	*gcode_in;						// handle to gcode input file	(was gd)
  extern FILE 	*gcode_out;						// handle to gcode output file 	(was gc_ptr)
  extern FILE 	*gcode_gen;						// handle to gcode output file 	(was gc_ptr)
  extern FILE	*proc_log;						// handle to process log file (human text of what's happening)
  extern FILE	*model_log;						// handle to record semi-detailed account of model run
  extern FILE 	*system_log;						// handle to record system events
  extern FILE	*Tools_fd;						// handle to Tools.XML file
  extern FILE	*Mats_fd;						// handle to Materials.XML file
  extern FILE	*Opers_fd;						// handle to Operations.XML file
  extern FILE 	*model_in;						// handle to model input file
  extern FILE	*unit_state;						// handle to state of unit
  extern FILE	*unit_calib;						// handle to unit calibration data file

// generic strings
  extern char	calib_file_rev[10];					// revision of unit calibration data file
  extern char 	scratch[255];						// scratch string space
  extern char 	status_cmd[5][255];					// holds system msgs from program and tinyG in 5 lines
  extern char	tinyg_cmd[5][255];					// holds the command list of what has been sent to tinyg
  extern char	rcv_cmd[255];						// holds received data from tinyG
  extern char 	gcode_cmd[512];						// holds individual commands to be sent to tinyG
  extern char	gcode_burst[1024];					// holds string of multiple commands to be sent to tinyG
  extern char 	blank[255];						// string of spaces for clearing screen

// flags
  extern int	unit_calib_required;					// flag to indicate if unit level calibration is required
  extern int	gcode_send,tinyg_send;					// flags to indicate nature of command output
  extern int 	save_logfile_flag;					// flag to save or ignore job log file

// PWM direct control (non-thermal)
  extern int	PWM_direct_control;					// flag to control PWM module directly thru thermal thread
  extern int	PWM_direct_status;					// flag to indicate status of PMW direct setting
  extern char	PWM_direct_port;					// value from 0-15 indicating which PMW port
  extern int 	PWM_duty_cycle;						// duty cycle from 0-100 
  extern int	PWM_duty_report;					// value of duty cycle as read back from PWM

// set up mark-up for error messages
  extern char *markup;
  extern char error_fmt[255];
  extern char okay_fmt[255];
  extern char norm_fmt[255];
  extern char unkw_fmt[255];
  extern char stat_fmt[255];

// 3D Model viewing
  extern int	MVview_click;
  extern float 	MVview_scale;						// scale factor for viewing models
  extern float 	MVdisp_spin,MVdisp_tilt;				// spin and tilt factors for Model Viewing
  extern float 	MVdisp_cen_x,MVdisp_cen_y,MVdisp_cen_z;
  extern int 	MVgxoffset,MVgyoffset;		 		        // location on the display area
  extern int	MVgx_drag_start,MVgy_drag_start;
  extern facet 	*MDisplay_facets;					// pointer to list of facets to display
  extern edge 	*MDisplay_edges;					// pointer to list of edges to display
  extern float	pers_scale;						// perspective scalar
  extern int	set_view_image;						// turn on/off drawing of source image
  extern int	Superimpose_flag;					// superimpose 3D geometry over camera image flag
  extern int	Perspective_flag;					// view 3D geometry with depth perspective flag

// 2D Layer viewing
  extern float 	LVview_scale;						// scale factor for viewing slices
  extern float 	LVdisp_spin,LVdisp_tilt;				// spin and tilt factors for Layer Viewing
  extern float 	LVdisp_cen_x,LVdisp_cen_y,LVdisp_cen_z;
  extern int 	LVgxoffset,LVgyoffset;		 		        // location on the display area
  extern int	LVgx_drag_start,LVgy_drag_start;
  extern float	slc_view_start,slc_view_end,slc_view_inc;		// starting/ending z values of slices to view in 2D
  extern int 	slc_just_one,slc_show_all,slc_print_view;		// flags for showing just one layer, or all layers, or printing what one sees

// Graph viewing
  extern float 	GVview_scale;						// scale factor for viewing slices
  extern float 	GVdisp_spin,GVdisp_tilt;				// spin and tilt factors for Layer Viewing
  extern float 	GVdisp_cen_x,GVdisp_cen_y,GVdisp_cen_z;
  extern int 	GVgxoffset,GVgyoffset;		 		        // location on the display area
  extern int	GVgx_drag_start,GVgy_drag_start;
  extern int	GRedraw_flag;

// Camera viewing
  extern float 	CVview_scale;						// scale factor for viewing slices
  extern float 	CVdisp_spin,CVdisp_tilt;				// spin and tilt factors for Layer Viewing
  extern float 	CVdisp_cen_x,CVdisp_cen_y,CVdisp_cen_z;
  extern int 	CVgxoffset,CVgyoffset;		 		        // location on the display area
  extern int	CVgx_drag_start,CVgy_drag_start;
  extern int	CRedraw_flag;
  extern float 	cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1;
  extern int 	save_image_history;					// flag to save or ignore images during build
  extern int 	cam_image_mode;						// flag to set what user wants to do with camera
  
// Image processing
  extern guchar	*pixfilter;
  extern int	edge_detect_threshold;					// value used to identify edges
  extern int	xy_on_table[415][415];					// xy map of object on build table
  extern int	need_new_image;

// Gerber file format translator
  extern int	gerber_format_xi,gerber_format_xd;
  extern int	gerber_format_yi,gerber_format_yd;
  extern int	gerber_polarity;
  extern int	gerber_units;	
  extern int	gerber_region_flag;
  extern int	gerber_quad_mode;					// gerber quandrant mode
  extern int	gerber_interp_mode;					// gerber interpolation mode
  extern int	gerber_vtx_inter;
  extern int	gerber_track_ID;

extern int	tool_count;						// the number of active tools
extern float 	acceptable_temp_tol;
extern int	pu_flag;

extern char	op_description[MAX_OP_TYPES][32];			// Description of each possible operation
extern char	lt_description[MAX_LINE_TYPES][32];			// Description of each possible line type

// Memory management variables
// These are used to ensure no memory leaks exist across all program functions
  extern long int	vertex_mem;					// Keeps track of maximum number of vertices in memory
  extern long int	edge_mem;					// Keeps track of maximum number of edges in memory
  extern long int	facet_mem;					// Keeps track of maximum number of facets in memory
  extern long int	vector_mem;					// quantity of vectors held in memory (typically more than in linked list)
  extern long int	polygon_mem;					// Keeps track of maximum number of polygons in memory
  extern long int	slice_mem;					// Keeps track of maximum number of slices in memory
  extern long int	model_mem;					// Keeps track of maximum number of models in memory
	
// Unit specific parameters
extern int	barx;

// Thermal control params
  extern int	wdog;							// watchdog kick value
  extern int	PWMsettings;						// settings of PWM

// Tracking info
extern int	unit_in_error;						// generic flag set when something is wrong - a check engine light of sorts
extern float 	XMin,YMin,ZMin;						// min and max of ALL models in memory at current positions
extern float 	XMax,YMax,ZMax;						// see model structure for more details
extern float 	cam_offset_x,cam_offset_y;				// distance sensor offsets
extern int	current_copy;
extern int	new_copy_cnt_flag;					// flag to indicate copy count has changed
extern int	global_offset;
extern time_t 	idle_start_time,idle_current_time;			// timer used to detect inactivity
extern int	job_count_total;					// total count of jobs this unit has run
extern int 	job_count_fail;						// total count of jobs that did not provide a good result
extern int	tinyG_state;						// general status of tinyG card
extern int	tinyG_in_motion;					// flag to indicate if motion complete or not
extern char	tinyG_id[15];						// id of this specific tinyG card
extern int	current_tool,previous_tool;				// tool slot in use
extern int	current_step,previous_step;				// tool or carriage power in use
extern float	Bld_Tbl_Lvl[4];						// z location of each corner at z=40.0mm
extern int	bld_lvl_x_ref,bld_lvl_y_ref;				// index of build table z level reference location in array
extern float 	bit_load_x,bit_load_y,bit_load_x_inc,bit_load_y_inc;
extern float 	grid_x_delta,grid_y_delta;
extern int	i2c_devices_found;					// i2c devices
extern int	dr_offset,dr_fill,dr_spt;
extern int	mdlmm_count;
extern long 	move_buffer_count;					// the current total number of moves submitted to the tinyG during a layer
extern long 	move_buffer_rewind;					// move count to go back to as requested
extern int	show_path;
extern int	autoplacement_flag;					// automatically positions models as they are loaded
extern int	ignore_tool_bit_change;
extern int	drill_holes_done_flag;
extern int	only_drill_small_holes_flag;
extern int	ztip_test_point;
extern int	model_align_flag;
extern int	linet_state[MAX_TOOLS];
extern int	oper_state[MAX_TOOLS];
extern int	ibreak;
extern int	init_done_flag;
extern int	gpio_ready_flag;					// flag to indicate gpio can be used
extern int	carriage_at_home;					// flag to indicate if carriage at home position
extern int	on_part_flag;						// flag to indicate if tool is working on part
extern vertex	*vtx_last_printed;
extern vertex 	*vtx_debug;
extern vertex 	*vmid,*vlow,*vhgh;
extern vector	*vec_debug;
extern int	material_feed;
extern int 	max_feed_time;
extern int	tools_need_tip_calib;
extern time_t 	read_cur_time,read_prev_time;
extern float 	oldposta;

// block target params
  extern int	cnc_add_block_flag;
  extern int	cnc_profile_cut_flag;
  extern float	cnc_x_margin,cnc_y_margin;				// margins beyond model max/min to mill
  extern float	cnc_z_height,cnc_z_stop;				// raw matl block height to be milled and where model stops in z

// build table scanning variables
  extern int 	build_table_scan_flag;					// flag to indicate if build table has or has not been scanned
  extern int	scan_in_progress;					// flag to indicate if a scan is in progress
  extern int	z_tbl_comp,old_z_tbl_comp;
  extern float	set_z_contour;						// the z height to contour up to
  extern float  sensor_z_height;					// ideal height for non-contact z sensor
  extern float 	scan_dist;						// distance in mm between scan points on build table
  extern float 	sweep_speed;						// speed of sweep scan
  extern int 	scan_xmax,scan_ymax;					// max array size at current scan_dist
  extern float 	scan_x_remain,scan_y_remain;				// offset of first scan point from table 0,0 during scans
  extern float	scan_x_start,scan_y_start,scan_x_end,scan_y_end;	// start and end pts of scan area
  extern int	Pz_index;
  extern float 	Pz[SCAN_X_MAX][SCAN_Y_MAX];				// grid of normalized Z values across build table
  extern float 	Pz_raw[SCAN_X_MAX][SCAN_Y_MAX];				// raw scan data as received from the sensor
  extern int	Pz_att[SCAN_X_MAX][SCAN_Y_MAX];
  extern int 	scan_img_index[MAX_SCAN_IMAGES];			// index of file sub-names (position values)
  extern int	scan_image_count;					// total count of scanned images in TableScan subdirectory
  extern int	scan_image_current;					// current value into index


// variables used to track history
  extern int 	hist_type;						// 0=thermal, 1=build time, etc.
  extern float	history[MAX_HIST_FIELDS][MAX_HIST_COUNT];		// user selected data fields
  extern int	hist_ctr[MAX_HIST_TYPES];				// number of data elements collected for estimate (e) or print (p)
  extern int	hist_use_flag[MAX_HIST_FIELDS];				// flag to indicate if field is in use/displayed
  extern int 	hist_tool[MAX_HIST_FIELDS];				// defines which tool data relates to
  extern int	hist_color[MAX_HIST_FIELDS];				// line color in graph
  extern char 	hist_name[MAX_HIST_FIELDS][255];			// name of each field

// debug flag used as "in code" method of out putting data to console during debug.
// flag values follow the general rules of:
//   0-99  =  vector related functions
// 100-199 =  polygon related functions
// 200-299 =  slice related functions
// 300-399 =  model related functions
// 400-499 =  job related functions
// 500-599 =  deposition related functions
// 600-699 =  tool related functions
  extern int		debug_flag;
  extern long int	total_vector_count;

// geometery processing
  extern float 	max_spt_angle,max_base_angle;
  extern float 	min_perim_length,min_polygon_area;			// minimum size filter values for polygons
  extern float 	ss_wavefront_increment;					// distance to move wavefront in straight skeleton
  extern float 	ss_wavefront_maxdist;					// max distance to move wavefront in straight skeleton
  extern float 	ss_wavefront_crtdist;					// current distance of ss wavefront
  extern float 	max_colinear_angle;					// angular threshold value to combine vectors
  extern float 	min_vector_length;					// default minimum vector length value.  superceded by line type values.
  extern int	nonboolean_input;					
  extern float 	max_support_facet_len;
  extern int	max_vtxs_per_polygon;
  extern int	max_polygons_per_slice;
  extern float 	is_round_radii_tol;
  extern float 	is_round_length_tol;
  extern int	poly_fill_none,poly_fill_all,poly_fill_unique;		// flags regarding how to apply fills to polygons
  extern float  poly_fill_flow,poly_fill_feed,poly_fill_thick,poly_fill_pitch,poly_fill_angle;	// global values for polygon filling
  extern int	poly_perim_none,poly_perim_all,poly_perim_unique;	// flags regarding how to apply fills to polygons
  extern float 	poly_perim_flow,poly_perim_feed,poly_perim_thick,poly_perim_pitch;
  extern int	slice_has_many_polygons[MAX_MDL_TYPES],slice_offset_type_qty[MAX_MDL_TYPES];	// controls type of offsetting function to use

// print control settings
  extern float 	xoffset,yoffset,zoffset;				// generic model offset from origin
  extern float 	curDist,minDist;					// current and minium total move distance
  extern float  z_cut, z_cut_at_pause;					// global slice level in mm. a model's min/max must be in range of z_cut to show.
  extern float 	step_inc;						// generic value to increase/reduce param adjustments
  extern int 	abort_flag;						// indicates if abort request is pending
  extern int	adjfeed_flag;						// flag to turn on/off feed rate adjustment for short vectors
  extern float 	adjfeed_trigger;					// value at which feed ajustment kicks in (vector length in mm)
  extern int 	adjflow_flag;						// flag to turn on/off flow rate adjustment for short vectors
  extern float 	adjflow_trigger;					// value at which flow adjustment kicks in (vector length in mm)
  extern int	first_layer_flag;					// flag to indicate first layer deposition
  extern int	smart_check;						// turn on/off job rules checking
  extern int	set_assign_to_all;					// flag to assign tools to all models
  extern int 	set_ignore_lt_temps;					// turn on/off ignoring line type temperature changes
  extern int	set_center_build_job;					// turn on/off centering of build job on table
  extern int	set_vtx_overlap;					// qty of vectors to overlap when depositing contour polygons
  extern int	set_build_leveling;					// turn on/off build table leveling
  extern int	set_start_at_crt_z;					// turn on/off starting at z=0 or current z height
  extern float	z_start_level;						// value of z where printing will start from
  extern int	set_force_z_to_table;					// turn on/off forcing non-zero z start to build table
  extern float 	set_z_manual_adj;					// manual z increment while job running
  extern int 	set_include_btm_detail;					// turn on/off 1st layer details if below slice height
  extern int	fidelity_mode_flag;					// turn on/off high fidelity mode
  extern float 	fidelity_feed_adjust,fidelity_flow_adjust;		// high fidelity adjustment values
  extern float 	x_move_velocity,y_move_velocity,z_move_velocity;
  extern int	motion_is_complete;					// flag to indicate code is in motion complete function
  extern int 	tool_air_status_flag;					// flag to indicate if air pump is on/off
  extern int 	tool_air_override_flag;					// flag to indicate if air pump is not under line type control
  extern float 	tool_air_slice_time;					// tool air will turn on if slice time under this limit
  extern vertex 	*xsw_pos,*ysw_pos;				// x/y tool calib switch positions

// test control settings
extern int	test_slot;
extern int	test_element;
extern int	move_slot;
extern float 	move_cdist,move_tdist;
extern float 	move_x,move_y,move_z;
extern float 	t24v_duty,t48v_duty,PWMv_duty;
extern int	aux2_port,aux3_port;

// Settings flags
extern int	set_view_endpts;					// turn on/off vector endpoints when displaying slices
extern int	set_view_vecnum;					// turn on/off vector sequence numbers when displaying slices
extern int	set_view_veclen;					// turn on/off vector length when displaying slices
extern int	set_view_vecloc;					// turn on/off vector location of where it WILL BE deposited
extern int	set_view_poly_ids;					// turn on/off polygon IDs
extern int	set_view_poly_fam;					// turn on/off polygon family relationships (i.e. parent/child)
extern int	set_view_poly_dist;					// turn on/off polygon offset distance
extern int	set_view_poly_perim;					// turn on/off polygon perimeter length
extern int	set_view_poly_area;					// turn on/off polygon area
extern int 	set_view_postloc;					// turn on/off vector location of where it WAS deposited
extern int	set_view_raw_stl;					// turn on/off stl vector viewing
extern int	set_view_raw_mdl;					// turn on/off raw slice vector viewing
extern int	set_view_raw_off;	
extern int	set_view_strskl;					// turn on/off offset straight skeleton when displaying slices
extern int	set_view_lt[MAX_LINE_TYPES];				// turn on/off display of each/any line type
extern int	set_view_model_lt_width;				// turn on/off display of line width in 2D view
extern int	set_view_support_lt_width;				// turn on/off display of line width in 2D view
extern int	set_view_lt_override;					// flag to indicate viewed line types no longer match tools operation sequence
extern int	set_view_build_lvl;					// turn on/off build table z level
extern int	set_view_mdl_slice;					// turn on/off slice viewing in model view
extern int	set_view_mdl_bounds;					// turn on/off bounding box around each model
extern int	set_view_tool_pos;					// turn on/off display of tool position while printing
extern float 	set_view_edge_angle;					// angle to display edges in 3D
extern float 	set_view_edge_min_length;				// minimum length of edge to display in 3D
extern int	set_show_above_layer;					// turn on/off displaying layer above current zcut
extern int	set_show_below_layer;					// turn on/off displaying layer below current zcut
extern int	set_auto_zoom;
extern int	set_view_models;					// flag to show/hide model geometry
extern int	set_view_supports;					// flag to show/hide support geometry
extern int	set_view_support_tree;					// flag to show/hide support tree
extern int	set_view_internal;					// flag to show/hide internal geometry
extern int	set_view_target;					// flag to show/hide target geometry
extern int 	set_view_patches;
extern int	set_view_normals;
extern int	set_view_edge_support;
extern int	set_view_edge_shadow;
extern int 	set_view_free_edges;
extern int	set_edge_display;
extern int 	set_facet_display;
extern int	set_view_bound_box;
extern int	show_view_bound_box;
extern int	set_view_max_facets;
extern int 	set_view_penup_moves;
extern int	set_suppress_qr_cmd;
extern int	set_suppress_mots_cmd;
extern int	display_model_flag;
extern int	good_to_go;
extern int	set_temp_cal_load;

extern int	set_mouse_select;
extern int	set_mouse_clear;
extern int	set_mouse_drag;
extern int 	set_mouse_align;
extern int	set_mouse_center;
extern int	set_mouse_merge;
extern int	set_mouse_pick_entity;
extern int	set_show_pick_neighbors;
extern int 	set_show_pick_clear;

extern char	new_mdl[255];
extern char	new_tool[255];

extern GError*	my_errno;


// vulkan display variables
extern VkInstance 		instance;
extern VkPhysicalDevice 	physical_device;
extern VkDevice 		device;
extern uint32_t 		queue_family_index;
extern VkQueue 			queue;
extern VkCommandPool 		command_pool;
extern VkCommandBuffer 		command_buffer;
extern VkSurfaceKHR 		surface;
extern VkSurfaceFormatKHR 	surface_format;
extern VkRenderPass 		render_pass;
extern VkPipelineLayout 	pipeline_layout;
extern VkPipeline 		graphics_pipeline;
extern VkSemaphore 		image_available_semaphore;
extern VkSemaphore 		render_finished_semaphore;
extern VkImageView 		image_view;
extern VkFramebuffer 		framebuffer;

// GTK4 variables
extern  int		win_tool_flag[MAX_TOOLS];
extern	int		win_modelUI_flag;
extern	int		win_settingsUI_flag;
extern  int		win_tooloffset_flag;				// flag to indicate if tool temp offset window is displayed
extern	int		win_testUI_flag;
extern 	int		main_view_page;					// indicates which main page user is currently viewing
extern GtkWidget	*win_main,*win_model,*win_tool,*win_settings;	// primary windows
extern GtkWidget 	*frame_time_box;				// primary window time/status box
extern GtkWidget	*g_notebook;					// primary notebook for various views
extern GtkWidget	*img_job_btn, *img_job_abort, *img_settings;	// persistant buttons 
extern GtkWidget	*btn_settings, *btn_run, *btn_abort, *btn_test, *btn_quit;
extern int		img_job_btn_index,img_model_index,img_tool_index[4];
extern GtkWidget	*combo_mdls;
extern GtkWidget	*combo_lts;

extern GtkAdjustment  	*g_adj_z_level;
extern GtkWidget	*g_adj_z_scroll;

extern GtkWidget	*aa_dialog;					// generic user dialog window
extern int		aa_dialog_result;				// holds result of user dialog interactions

// model view dependent buttons
extern GtkWidget	*btn_model_add,*btn_model_sub,*btn_model_select,*btn_model_clear,*btn_model_options,*btn_model_align;
extern GtkWidget	*img_model_add,*img_model_sub,*img_model_select,*img_model_clear,*img_model_options,*img_model_align;
extern GtkWidget	*btn_model_XF,*btn_model_YF,*btn_model_ZF,*btn_model_view;
extern GtkWidget	*img_model_XF,*img_model_YF,*img_model_ZF,*img_model_view;
extern GtkWidget	*g_model_all,*g_model_area,*g_model_btn,*grid_model_btn;
extern GtkWidget	*btn_model_control, *btn_model_load, *btn_model_remove, *btn_tool[4];
extern GtkWidget	*img_model_control, *img_model_load, *img_model_remove, *img_tool[4];
extern GtkWidget	*btn_model_settings,*img_model_settings;

// tool view dependent buttons
extern GtkWidget	*g_tool_all,*g_tool_btn,*grid_tool_btn;
extern GtkWidget 	*btn_tool_ctrl, *img_tool_ctrl;
extern GtkWidget	*material_lbl[4],*type_lbl[4],*mdl_lbl[4],*mlvl_lbl[4];
extern GtkWidget 	*combo_tool,*combo_mats;
extern GtkWidget	*lbl_tool_stat,*lbl_slot_stat,*lbl_tool_memID,*lbl_tool_desc;
extern GtkWidget	*lbl_val_pos_x,*lbl_val_pos_y,*lbl_val_pos_z,*lbl_val_pos_a;
extern GtkWidget	*lbl_val_spd_crt,*lbl_val_spd_avg,*lbl_val_spd_max;
extern GtkWidget 	*lbl_val_flw_crt,*lbl_val_flw_avg,*lbl_val_flw_max;
extern GtkWidget 	*lbl_val_cmdct,*lbl_val_buff_used,*lbl_val_buff_avail;
extern GtkWidget	*btn_tool_feed_fwd, *btn_tool_feed_rvs;
extern GtkWidget	*img_tool_feed_fwd, *img_tool_feed_rvs;
extern float 		PostGVel,PostGVAvg,PostGVMax,PostGVCnt,PostGFlow,PostGFAvg,PostGFMax,PostGFCnt;

// layer view dependent buttons
extern GtkWidget	*g_layer_all,*g_layer_area,*g_layer_btn,*grid_layer_btn;
extern GtkWidget	*btn_layer_count,*btn_layer_eol,*btn_merge_poly,*btn_poly_attr;
extern GtkWidget 	*img_layer_count,*img_layer_eol,*img_merge_poly,*img_poly_attr;
extern GtkWidget	*btn_layer_settings,*img_layer_settings;
extern GtkAdjustment	*slc_start_adj,*slc_end_adj,*slc_inc_adj;
extern GtkWidget	*slc_start_btn,*slc_end_btn,*slc_inc_btn;
extern GtkWidget	*just_current_btn,*show_all_btn;
extern GtkAdjustment	*perim_fed_adj,*perim_flw_adj,*perim_thk_adj,*perim_pitch_adj;
extern GtkWidget	*perim_fed_btn,*perim_flw_btn,*perim_thk_btn,*perim_pitch_btn;
extern GtkWidget	*perim_no_pattern_btn,*perim_same_pattern_btn,*perim_unique_pattern_btn;
extern GtkAdjustment	*fill_fed_adj,*fill_flw_adj,*fill_thk_adj,*fill_pitch_adj,*fill_angle_adj;
extern GtkWidget	*fill_fed_btn,*fill_flw_btn,*fill_thk_btn,*fill_pitch_btn,*fill_angle_btn;
extern GtkWidget	*fill_no_pattern_btn,*fill_same_pattern_btn,*fill_unique_pattern_btn;

// graph view dependent buttons
extern GtkWidget 	*g_graph_all,*g_graph_area,*g_graph_btn,*grid_graph_btn;
extern GtkWidget 	*btn_graph_temp,*img_graph_temp;
extern GtkWidget 	*btn_graph_time,*img_graph_time;
extern GtkWidget 	*btn_graph_matl,*img_graph_matl;

// camera view dependent buttons
extern char 		bashfn[255];
extern GtkWidget	*g_camera_all,*g_camera_area,*g_camera_btn,*grid_camera_btn,*g_camera_image;
extern GdkPixbuf	*g_camera_buff;
extern GtkWidget 	*btn_camera_liveview,*img_camera_liveview;
extern GtkWidget 	*btn_camera_snapshot,*img_camera_snapshot;
extern GtkWidget 	*btn_camera_job_timelapse,*img_camera_job_timelapse;
extern GtkWidget 	*btn_camera_scan_timelapse,*img_camera_scan_timelapse;
extern GtkWidget 	*btn_camera_review,*img_camera_review;
extern GtkWidget	*btn_camera_settings,*img_camera_settings;

// vulkan view dependent buttons
extern GtkWidget	*g_vulkan_area;

// generic re-usable windows for posting progress bars, msgs, and getting tid bits of data
extern	GtkWidget	*win_info,*grd_info,*btn_done;				// window for posting status - includes progress bar & lables
extern 	GtkWidget	*info_progress,*info_percent,*info_sleep,*grid_info,*lbl_info1,*lbl_info2;
extern  GtkWidget	*win_stat,*grd_stat,*lbl_stat1,*lbl_stat2,*btn_stat;	// window, grid, lables, and done button - for posting messages
extern	GtkWidget	*win_data,*grd_data;					// window and grid - created/destroyed after each use as needed

// model UI widgets
extern	GtkWidget	*lbl_job_type, *lbl_job_mdl_qty, *lbl_job_slice_thk;
extern	GtkWidget	*lbl_job_xmax, *lbl_job_ymax, *lbl_job_zmax;
extern 	GtkWidget 	*lbl_mdl_error, *lbl_model_name;
extern  GtkAdjustment	*xrotadj,*yrotadj,*zrotadj;			// model rotate adjustments
extern  GtkWidget	*xrotbtn,*yrotbtn,*zrotbtn;			// model rotate adj buttons
extern  GtkAdjustment	*xposadj,*yposadj,*zposadj;			// model translate adjustments
extern  GtkWidget	*xposbtn,*yposbtn,*zposbtn;			// model translate adj buttons
extern  GtkAdjustment	*xscladj,*yscladj,*zscladj;			// model scale adjustments
extern  GtkWidget	*xsclbtn,*ysclbtn,*zsclbtn;			// model scale adj buttons
extern  GtkWidget	*xmirbtn,*ymirbtn,*zmirbtn;			// model mirror checkboxes
extern  GtkWidget 	*lbl_x_size,*lbl_y_size,*lbl_z_size;		// model sizes in current orientation
extern 	GtkAdjustment	*mdl_copies_adj;				// number of copies of this model
extern 	GtkWidget	*mdl_copies_btn;				// spin btn for number of copies

extern  GtkAdjustment	*xtipadj,*ytipadj,*ztipadj;			// tool tip xyz adjustments
extern  GtkWidget	*xtipbtn,*ytipbtn,*ztipbtn;			// tool tip xyz adj buttons

extern  GtkAdjustment	*edge_angle_adj;				// model view edge angle for display
extern  GtkWidget	*edge_angle_btn,*lbl_edge_angle;		

extern  GtkAdjustment	*facet_qty_adj;					// model view max facets to display
extern  GtkWidget	*facet_qty_btn,*lbl_facet_qty;		

extern  GtkAdjustment	*temp_tol_adj,*temp_chm_or_adj,*temp_tbl_or_adj;	
extern  GtkWidget	*temp_tol_btn,*lbl_temp_tol;		
extern  GtkWidget	*lbl_chmbr_temp_or,*lbl_table_temp_or;
extern  GtkWidget	*temp_chm_or_btn,*temp_tbl_or_btn;

extern  GtkAdjustment	*facet_tol_adj;					// facet proximity tolerance
extern  GtkWidget	*facet_tol_btn,*lbl_facet_tol;			
extern	GtkAdjustment	*colinear_angle_adj,*wavefront_inc_adj;

extern GtkAdjustment	*pix_size_adj;				
extern GtkWidget	*pix_size_btn,*lbl_pix_size;			
extern GtkAdjustment	*pix_increment_adj;				
extern GtkWidget	*pix_increment_btn,*lbl_pix_incr;			

extern 	GtkAdjustment	*matl_feed_time_adj;
extern 	GtkWidget	*matl_feed_time_btn,*lbl_matl_feed_time;

extern	GtkAdjustment	*img_edge_thld_adj;
extern	GtkWidget	*img_edge_thld_btn,*lbl_img_edge_thld;

extern	char		job_state_msg[80],job_status_msg[255];
extern 	GtkWidget	*job_status_lbl,*job_state_lbl;
extern 	GtkWidget	*elaps_time_lbl,*est_time_lbl;
extern	GtkWidget	*img_madd,*btn_madd;				// material add/subtract btns on tool menu
extern	GtkWidget	*img_msub,*btn_msub;

extern 	GtkWidget	*stat_box,*time_box,*grid_time;
extern 	GtkWidget	*grid_job,*stat_lbl_job,*stat_lbl_start,*stat_lbl_end;

// temperature and material bar graphs for MAIN window status frame.
// they are updated by the on_idle function.
extern GtkWidget	*temp_lbl[MAX_THERMAL_DEVICES],*setp_lbl[MAX_THERMAL_DEVICES],*matl_lbl[MAX_THERMAL_DEVICES];
extern GtkWidget	*temp_lvl[MAX_THERMAL_DEVICES],*matl_lvl[MAX_THERMAL_DEVICES];
extern GtkLevelBar	*temp_bar[MAX_THERMAL_DEVICES],*matl_bar[MAX_THERMAL_DEVICES];
extern GtkWidget	*type_desc[MAX_THERMAL_DEVICES],*matl_desc[MAX_THERMAL_DEVICES],*mdl_desc[MAX_THERMAL_DEVICES];
extern GtkWidget	*tool_stat[MAX_THERMAL_DEVICES],*linet_stat[MAX_THERMAL_DEVICES],*oper_st_lbl[MAX_THERMAL_DEVICES],*tip_pos_lbl[MAX_THERMAL_DEVICES];
extern GtkWidget	*temp_lvl_bldtbl,*temp_lvl_chamber;
extern GtkLevelBar	*temp_bar_bldtbl,*temp_bar_chamber;
extern GtkWidget	*lbl_bldtbl_tempC,*lbl_bldtbl_setpC,*lbl_chmbr_tempC,*lbl_chmbr_setpC;
extern GtkWidget	*stat_bldtbl,*stat_chm_lbl;

// temperature and material bar graphs for TOOL window status frame.
// would have liked to use same ones as in main, but child widgets can't have two parents... so we duplicate.
// note:  these will be created in main because we want the on_idle function to update them whilst the tool window is up.
extern GtkWidget	*Ttemp_lbl[MAX_THERMAL_DEVICES],*Tsetp_lbl[MAX_THERMAL_DEVICES],*Tmatl_lbl[MAX_THERMAL_DEVICES],*Tmlvl_lbl[MAX_THERMAL_DEVICES];
extern GtkWidget	*Ttemp_lvl[MAX_THERMAL_DEVICES],*Tmatl_lvl[MAX_THERMAL_DEVICES];
extern GtkLevelBar	*Ttemp_bar[MAX_THERMAL_DEVICES],*Tmatl_bar[MAX_THERMAL_DEVICES];
extern GtkAdjustment	*temp_override_adj, *matl_override_adj;
extern GtkWidget	*temp_override_btn, *matl_override_btn;

// public widgets for the TOOL window.
// they need to be updated on tool changes by routines outside of the window generation
extern	GtkWidget	*lbl_power,*lbl_heater,*lbl_tsense,*lbl_stepper,*lbl_SPI;
extern	GtkWidget	*lbl_1wire,*lbl_1wID,*lbl_limitsw,*lbl_weight;
extern	GtkWidget	*lbl_tipdiam,*lbl_layerht,*lbl_temper;
extern	GtkWidget	*lbl_tipx,*lbl_tipy,*lbl_tipzdn;
extern	GtkWidget	*lbl_tipzup,*btn_tipset;

// tool information and calibration widgets
extern  GtkAdjustment	*tool_serial_numadj;
extern  GtkWidget	*tool_serial_numbtn;
extern  GtkAdjustment	*temp_cal_sys_highadj;				// tool temperature high calibration adjustment
extern  GtkWidget	*temp_cal_sys_highbtn;				// tool temperature high calibration button
extern  GtkAdjustment	*duty_cal_sys_lowadj,*duty_cal_sys_highadj;	// tool power duty cycle low/high calibration adjustment
extern  GtkWidget	*duty_cal_sys_lowbtn,*duty_cal_sys_highbtn;	// tool power duty cycle low/high calibration button
extern 	GtkWidget 	*tcal_vhi_lbl;					// voltage value for temp high point calibartion
extern 	int		device_being_calibrated;			// device to be calibrated
extern	GtkWidget	*lbl_device_being_calibrated;			// label for device
extern 	GtkAdjustment	*temp_cal_device;				// spin box to select which device to calibrate
extern 	GtkWidget	*temp_cal_btn;					// temperature calibration button
extern 	GtkWidget	*temp_chbr_disp,*temp_btbl_disp,*temp_tool_disp;// real time temp display for settings
extern 	GtkWidget 	*lbl_lo_values,*lbl_hi_values;			// labels to show hi/lo temperature calibration values
extern 	GtkAdjustment	*temp_cal_offset;				// spin box to set the temperature offset for a tool
extern 	GtkWidget	*temp_cal_offset_btn;				// temperature offset calibration button
extern  GtkWidget 	*lbl_mat_description,*lbl_mat_brand;		// labels for materials in tool info window


// settings layer viewing
extern GtkAdjustment	*slc_view_start_adj, *slc_view_end_adj, *slc_view_inc_adj;
extern GtkWidget	*slc_view_start_btn, *slc_view_end_btn, *slc_view_inc_btn;

// settings model viewing
extern GtkWidget	*btn_pick_vertex, *btn_pick_facet, *btn_pick_patch, *btn_pick_neighbors;

// settings XYZ move adjustments
extern 	GtkAdjustment 	*xdistadj,*ydistadj,*zdistadj;

// settings for tool voltage tests
extern 	GtkAdjustment	*t48vadj,*t24vadj,*PWMvadj;
extern  GtkWidget	*t48vbtn,*t24vbtn,*PWMvbtn;

// material color for each slot
extern 	GdkRGBA 	mcolor[4];					// note: NOT a pointer!

extern	GtkAdjustment	*xadj,*yadj;
extern	GtkWidget	*xbtn,*ybtn;

// operation selection and control widgets
extern  GtkWidget	*btn_op[MAX_OP_TYPES][MAX_TOOLS];
extern  GtkWidget	*btn_view_linetype[MAX_LINE_TYPES];
extern 	GtkWidget	*btn_view_model_lt_width,*btn_view_support_lt_width;

// build table scanning widgets
extern GtkAdjustment	*bld_tbl_res_adj;
extern GtkWidget	*bld_tbl_res_btn,*bld_tbl_res_lbl;

// fabrication widgets
extern GtkAdjustment	*zcontour_adj, *vtx_overlap_adj;
extern GtkWidget	*btn_z_tbl_comp,*zcontour_btn,*zcontour_lbl;
extern GtkWidget	*vtx_overlap_btn,*vtx_overlap_lbl;

// cnc widgets
extern  GtkAdjustment	*cnc_block_x_adj,*cnc_block_y_adj,*cnc_block_z_adj;
extern  GtkWidget	*cnc_block_x_btn,*lbl_cnc_block_x;			
extern  GtkWidget	*cnc_block_y_btn,*lbl_cnc_block_y;			
extern  GtkWidget	*cnc_block_z_btn,*lbl_cnc_block_z;			

// Functions located in 4X3D.c
extern void set_color(cairo_t *cr, GdkRGBA *st_color, int color);
extern int get_color(GdkRGBA *st_color);
extern void on_active_model_is_null(void);
extern int on_job_will_need_regen(void);
extern int Print_to_Model_Coords(vertex *vptr, int slot);
extern int Model_to_Print_Coords(vertex *vptr, int slot);

// Functions located in Model_UI.c
extern model *model_file_load(GtkWidget *btn_call, gpointer src_window);
extern int model_file_remove(GtkWidget *btn_call, gpointer user_data);
extern void set_operation_checkbutton(int slot, int op_typ);
extern void build_options_anytime_cb(GtkWidget *btn, gpointer src_window);
extern void build_options_preslice_cb(GtkWidget *btn, gpointer src_window);
extern int model_UI(GtkWidget *btn_call, gpointer user_data);
extern void update_size_values(model *mnew);

// Functions located in Model_Data.c
extern int memory_status(void);
extern vertex *vertex_make(void);
extern int vertex_insert(model *mptr, vertex *local, vertex *vertexnew, int typ);// inserts a vertex into a model
extern vertex *vertex_unique_insert(model *mptr, vertex *local, vertex *vertexnew, int typ, float tol);
extern int vertex_delete(model *mptr, vertex *vdel, int typ);
extern int vertex_redundancy_check(vertex *vtxlist);
extern vertex *vertex_copy(vertex *vptr, vertex *vnxt);
extern int vertex_swap(vertex *A, vertex *B);
extern int vertex_purge(vertex *vlist);
extern int vertex_compare(vertex *A, vertex *B, float tol);
extern int vertex_xy_compare(vertex *A, vertex *B, float tol);
extern float vertex_2D_distance(vertex *vtxA, vertex *vtxB);
extern float vertex_distance(vertex *vtxA, vertex *vtxB);
extern int vertex_3D_interpolate(float f_dist,vertex *vtxA,vertex *vtxB, vertex *vresult);
extern int vertex_colinear_test(vertex *v1, vertex *v2, vertex *v3);
extern int vertex_cross_product(vertex *v1, vertex *v2, vertex *v3, vertex *vresult);
extern vertex *vertex_find_in_grid(polygon *pgrid, vertex *vcenter);
extern vertex *vertex_random_insert(vertex *vpre, vertex *vnew, double tolerance);
extern vertex *vertex_match_XYZ(vertex *vtest,polygon *ptest);
extern vertex *vertex_match_ID(vertex *vtest,polygon *pA);
extern vertex *vertex_previous(vertex *vtest,polygon *pA);
extern vertex_list *vertex_neighbor_list(vertex *vinpt);
extern vertex_list *vertex_list_make(vertex *vinpt);
extern vertex_list *vertex_list_manager(vertex_list *vlist, vertex *vinpt, int action);
extern vertex *vertex_list_add_unique(vertex_list *vl_inpt, vertex *vtx_inpt);

extern edge *edge_make(void);
extern edge *edge_copy(edge *einp);
extern int edge_insert(model *mptr, edge *local, edge *edgenew, int typ);
extern int edge_delete(model *mptr, edge *edel, int typ);
extern int edge_purge(edge *elist);
extern edge_list *edge_list_make(edge *einpt);
extern edge_list *edge_list_manager(edge_list *elist, edge *einpt, int action);
extern edge *edge_angle_id(edge *inp_edge_list,float inp_angle);	// creates edge list based on neighboring facet angles
extern int edge_compare(edge *A, edge *B);
extern int edge_display(model *mptr, float angle, int typ);
extern int edge_silhouette(model *mptr, float spin, float tilt);
extern float facet_angle(facet *Af, facet *Bf);
extern int edge_dump(model *mptr);

extern facet *facet_make(void);
extern int facet_insert(model *mptr, facet *local, facet *facetnew, int typ);
extern int facet_delete(model *mptr, facet *fdel, int typ);
extern int facet_purge(facet *flist);
extern int facet_normal(facet *finp);
extern int facet_flip_normal(facet *fptr);
extern int facet_centroid(facet *fptr,vertex *vctr);
extern float facet_area(facet *fptr);
extern int facet_find_all_neighbors(facet *flist);
extern facet *facet_find_vtx_neighbor(facet *fstart, facet *finpt, vertex *vtx0, vertex *vtx1);
extern int facet_contains_point(facet *finpt, vertex *vtest);
extern int facet_compare(facet *A, facet *B);
extern int vector_facet_intersect(facet *fptr, vector *vecptr, vertex *vint);
extern facet *facet_copy(facet *fptr);
extern facet *facet_subdivide(model *mptr, facet *fptr, float area);
extern int facet_unit_normal(facet *fptr);
extern int facet_swap_value(facet *fA, facet *fB);
extern int facet_share_vertex(facet *fA, facet *fB, vertex *vtx_test);
extern vector *facet_share_edge(facet *fA, facet *fB);
extern facet_list *facet_list_make(facet *finpt);
extern facet_list *facet_list_manager(facet_list *flist, facet *finpt, int action);

extern model *model_make(void);
extern int model_insert(model *modelnew);
extern int model_delete(model *mdl_del);
extern int model_dump(model *mptr);
extern int model_purge(model *mdl_del);
extern model *model_get_pointer(int mnum);
extern int model_dump(model *mptr);
extern int model_get_slot(model *mptr, int op_id);			// gets tool to be used for operation called out by model
extern int model_linetype_active(model *mptr, int lineType);
extern slice *model_raw_slice(model *mptr, int slot, int mtyp, int ptyp, float z_level);
extern int model_maxmin(model *mptr);					// establishes the max/min bounds of a model
extern int model_integrity_check(model *mptr);
extern int model_map_to_table(model *mptr);				// maps build table z offsets to model bounds
extern int model_mirror(model *mptr);					// mirrors models
extern int model_scale(model *mptr, int mtyp);				// scales models
extern int model_rotate(model *mptr);					// rotates models
extern int model_support_estimate(model *mptr);
extern int model_auto_orient(model *mptr);
extern int model_overlap(model *minp);
extern int model_out_of_bounds(model *mptr);
extern int model_convert_type(model *mptr,int target_type);
extern int model_build_target(model *mptr);
extern int model_profile_cut(model *mptr);
extern int model_grayscale_cut(model *mptr);
extern int model_build_internal_support(model *mptr);

extern patch *patch_make(void);						// creates an empty patch in memory
extern int patch_delete(patch *patdel);					// deletes a patch from memory
extern patch *patch_copy(model *mptr, patch *pat_src, int target_mdl_typ); // makes a copy of a patch
extern int patch_flip_normals(patch *patptr);				// flips normals of all facets pointed to by patch
extern int patch_find_z(patch *patptr, vertex *vinpt);
extern int patch_contains_facet(patch *patptr, facet *fptr);
extern int patch_find_edge_facets(patch *patptr);
extern int patch_find_free_edge(patch *patptr);				// creates patch vtx list from existing patch facet list
extern int patch_vertex_on_edge(patch *patptr, vertex *vptr);

extern branch *branch_make(void);
extern int branch_delete(branch *brdel);
extern int branch_sort(branch *br_list);
extern branch_list *branch_list_make(branch *br_inpt);
extern branch_list *branch_list_manager(branch_list *br_list, branch *br_inpt, branch *br_loc, int action);

// Functions located in Job.c
extern int job_rules_check(void);					// verifies job set up
extern int job_slice(void);						// slices models of the job
extern int job_purge(void);						// clears slice data out of a job, but keeps model geometry
extern int job_complete(void);						// posts complete msg and deletes job contensts
extern int job_clear(void);						// clears job parameters only
extern int job_maxmin(void);						// establishes the max/min bounds of all tools & models
extern float job_find_min_thickness(void);				// finds the thinnest slice thickness in use in the job
extern float job_time_estimate_calculator(void);
extern float job_time_estimate_simulator(void);
extern int job_map_to_table(void);					// maps build table z offsets to all tools & models
extern int job_base_layer2(float zlvl);
extern int job_base_layer(void);
extern model *job_layer_seek(int direction);
extern int job_build_grid_support(void);
extern int job_build_tree_support(void);

// Functions located in Vector.c
extern vector *vector_make(vertex *A, vertex *B, int typ);		// allocates memory for a single vector
extern int vector_insert(slice *sptr, int ptyp, vector *vecnew);	// inserts a single vector into linked list
extern int vector_raw_insert(slice *sptr, int ptyp, vector *vecnew);
extern vector *vector_delete(vector *veclist, vector *vdel);		// deletes a single vector from linked list
extern vector *vector_wipe(vector *vec_list, int del_typ);
extern vector *vector_copy(vector *vec_src);
extern int vector_list_dump(vector *vec_list);
extern int vector_compare(vector *A, vector *B, float tol);		// checks if two vectors are identical
extern int vector_purge(vector *vpurge);				// wipes the vector list out of memory
extern vector *vector_colinear_merge(vector *vec_first, int ptyp, float delta);	// merges colinear vectors
extern vector_list *vector_sort(vector *vec_group, float tolerance, int mem_flag);	// sorts vector linked list tip to tail
extern int vector_crossings(vector *vec_list, int save_in_new);
extern int vector_crosses_polygon(slice *sinpt, vector *vecA, int ptyp);
extern int vector_subdivide(vector *vecinpt, vertex *vtxinpt);
extern int vector_bisector_get_direction(vector *vA, vector *vB, vertex *vnew);
extern int vector_bisector_set_length(vector *vBi, float newdist);
extern int vector_bisector_inc_length(vector *vBi, float incdist);
extern polygon *ss_veclist_2_single_polygon(vector *vec_list, int ptyp);
extern polygon *veclist_2_single_polygon(vector *vec_first, int ptyp);
extern int vector_winding_number(vector *vec_list, vector *vec_inpt);
extern vector *vector_find_neighbor(vector *veclist, vector *vecinpt, int endpt);
extern int vector_duplicate_delete(vector *veclist, float tolerance);
extern unsigned int vector_intersect(vector *vA, vector *vB, vertex *intvtx);	// finds the intersection point of two vectors
extern int vector_parallel(vector *vA, vector *vB);			// quick test to check if parallel
extern double vector_dotproduct(vector *A, vector *B);			// calculates dot product of two vectors
extern double vector_magnitude(vector *A);				// calculate the magnitude of a vector
extern float vector_absangle(vector *A);				// calculates the absolute angle of a 2D vector
extern float vector_relangle(vector *A, vector *B);			// finds the angle between two vectors in 3D space
extern int vector_unit_normal(vector *vecin, vertex *vtx_norm);		// calculates unit normal of a vector
extern int vector_dump(vector *vptr);					// dumps vector list to screen
extern int vector_crossproduct(vector *A, vector *B, vertex *vptr);	// calculates cross product of two vectors
extern int vector_rotate(vector *vinpt, float theta);			// rotates a vector about its tail vertex
extern int vertex_crossproduct(vertex *A, vertex *B, vertex *vptr);
extern double vertex_dotproduct(vertex *A, vertex *B);
extern int vector_tip_tail_swap(vector *vinpt);
extern vector_list *vector_list_make(vector *vinpt);
extern vector_list *vector_list_manager(vector_list *vlist, vector *vinpt, int action);
extern int vector_list_check(vector_list *vlist);

// Functions located in Polygon.c
extern polygon *polygon_make(vertex *vtx, int typ, unsigned int memb);	// allocated memory for a polygon element
extern int polygon_insert(slice *sptr, int type, polygon *plynew);	// inserts polygon element into list of polygon vertices
extern int polygon_delete(slice *sptr, polygon *pdel);			// deletes a polygon from the slice's linked list of polygons
extern int polygon_free(polygon *pdel);					// deletes a stand alone polygon from memory
extern polygon *polygon_purge(polygon *pptr, float odist);		// wipes the entire polygon list from memory
extern polygon *polygon_copy(polygon *psrc);
extern int polygon_build_sublist(slice *ssrc, int mtyp);		// builds list of polygons enclosed by this polygon
extern polygon_list *polygon_list_make(polygon *pinpt);
extern polygon_list *polygon_list_manager(polygon_list *plist, polygon *pinpt, int action);
extern int polygon_dump(polygon *pptr);					// dumps the polygon list to the screen
extern float polygon_perim_length(polygon *pinpt);
extern int polygon_contains_point(polygon *pptr, vertex *vtest);	// test if a point (vertex) lies within a polygon
extern float polygon_to_vertex_distance(polygon *ptest, vertex *vtest, vertex *vrtn);	// finds min dist bt polygon and test vtx
extern int polygon_find_start(polygon *pinpt, slice *sprev);		// finds the best place to initiate perimeter deposition
extern int polygon_find_center(polygon *pinpt);				// finds the center of a polygon
extern float polygon_find_area(polygon *pinpt);				// finds the area of a polygon
extern slice *polygon_get_slice(polygon *pinpt);			// finds the slice to which the polygon belongs
extern int polygon_swap_contents(polygon *pA, polygon *pB);		// swaps the contents of two polygons
extern int polygon_colinear_merge(slice *sptr, polygon *pptr, double delta);	// merges colinear vectors
extern int polygon_subdivide(polygon *pinpt, float interval);			// subdivides line segments around polygon at interval
extern int polygon_contains_material(slice *sptr, polygon *pinpt, int ptyp);	// determines if poly is a hole or encloses material
extern int polygon_make_offset(slice *sinpt, polygon *pinpt);			// generates custom offsets for this polygon
extern int polygon_wipe_offset(slice *sinpt, polygon *pinpt, int ptyp, float odist); // wipes out custom offsets for this polygon
extern int polygon_make_fill(slice *sinpt, polygon *pinpt);			// generates fill pattern of specific type for this polygon
extern int polygon_wipe_fill(slice *sinpt, polygon *pinpt);			// wipes out fill pattern of a specific type for this polygon
extern int polygon_reverse(polygon *pptr);				// reverses the vertex list
extern int polygon_sort(slice *sinpt, int typ);				// sorts polygons to minimize pen up movements
extern int polygon_sort_perims(slice *sinpt, int line_typ, int sort_typ);
extern vector *polylist_2_veclist(polygon *pinpt, int ptyp);
extern int polygon_verify(polygon *pinpt);				// verifies contents of polygon
extern int polygon_selfintersect_check(polygon *pA);
extern int polygon_overlap(polygon *pA, polygon *pB);
extern int polygon_intersect(polygon *pA, polygon *pB, double tolerance);
extern polygon *polygon_boolean2(slice *sinpt, polygon *pA, polygon *pB, int action);
extern int polygon_edge_detection(slice *sptr, int ptyp);

// Functions located in Slice.c
extern slice *slice_make(void);						// creates a blank slice
extern int slice_insert(model *mptr, int slot, slice *local, slice *slicenew);	// inserts a slice into the linked list of slices for a model
extern int slice_delete(model *mptr, int slot, slice *sdel);			// deletes a single slice from the linked list for a model
extern int slice_purge(model *mptr, int slot);				// deletes all slice data from memory
extern slice *slice_copy(slice *ssrc);					// makes a copy of a slice
extern void slice_dump(slice *sptr);					// dumps slice information to consol
extern void slice_scale(slice *ssrc, float scale);			// scales a slice in the xy
extern slice *slice_find(model *mptr, int mytp, int slot, float zlvl, float delta); // finds the slice at a specific z level
extern model *slice_get_model(slice *sinp);				// finds ptr to model that contains this slice
extern int slice_polygon_count(slice *sptr);				// recounts the number of polygons in the slice
extern polygon *slice_find_outermost(polygon *pinpt);			// finds outer most polygon in group of polygons
extern int slice_maxmin(slice *sptr);					// establishes the max/min boundaries of this slice
extern int slice_rotate(slice *sptr, float rot_angle, int typ);		// rotates a slice
extern int slice_set_attr(int slot, float zlvl, int newattr);
extern int slice_point_in_material(slice *sptr, vertex *vtest, int ptyp, float odist);
extern float slice_point_near_material(slice *sinpt, vertex *vinpt, int ptyp);
extern slice *slice_model_bounds(model *mptr,int mtyp, int ptyp);
extern int slice_honeycomb(slice *str_sptr, slice * end_sptr, float cell_size, int Ltyp, int Ftyp);	// fills the slice interior with honeycomb
extern int slice_linefill(slice *blw_sptr,slice *crt_sptr,slice *abv_sptr,float scanstep,float scanangle,int ConTyp,int FillTyp,int BoolAction);
extern slice *slice_boolean(model *mptr, slice *sA, slice *sB, int ltypA, int ltypB, int ltyp_save, int action);
extern int vertex_in_facet(vertex *vtest, facet *finpt);
extern int slice_deck(model *mptr, int mtyp, int oper_ID);
extern int slice_fill_model(model *mptr, float zlvl);
extern int slice_fill_all(float zlvl);
extern int slice_wipe(slice *sptr, int ptyp, float odist);
extern int slice_skeleton(slice *sinpt, int source_typ, int ptyp, float odist);
extern int slice_offset_skeleton(slice *sinpt, int source_type, int ptyp, float odist);
extern int slice_offset_winding(slice *sinpt, int source_typ, int ptyp, float odist);
extern int slice_offset_winding_by_polygon(slice *sinpt, int source_typ, int ptyp, float odist);
extern vector_cell *vector_cell_make(void);
extern vector_cell *vector_cell_lookup(slice *sinpt, int x, int y);
extern int vector_grid_destroy(slice *sinpt);
extern vector *vector_add_to_list(vector *veclist, vector *vecadd);
extern vector *vector_del_from_list(vector *veclist, vector *vecdel);
extern vector *vector_search(vector *veclist, vector *vecinp);
extern vector *slice_to_veclist(slice *sinpt, int ptyp);

// Functions located in SENSOR.c
extern int Open1Wire(int slot);
extern int MemDevDataValidate(int slot);
extern int MemDevRead(int slot);
extern int MemDevWrite(int slot);
extern void mk_writable(char* path);
extern float distance_sensor(int slot, int channel, int readings, int typ);
extern int build_table_line_laser_scan(void);
extern int build_table_scan_index(int typ);
extern int build_table_step_scan(int slot, int sensor_type);
extern int build_table_sweep_scan(int slot, int sensor_type);
extern int build_table_centerline_scan(int slot, int sensor_type);
extern float touch_probe(vertex *vptr, int slot);
extern float z_table_offset(float x, float y);
//extern int get_distance(vl6180 handle);
//extern void set_scaling(vl6180 handle, int scaling);

// Functions located in SETTINGS.c
static void btn_endpts_toggled_cb(GtkWidget *btn, gpointer user_data);
static void btn_vecnum_toggled_cb(GtkWidget *btn, gpointer user_data);
static void btn_strskl_toggled_cb(GtkWidget *btn, gpointer User_data);
extern void grab_temp_cal_offset(GtkSpinButton *button, gpointer user_data);
static void on_settings_exit (GtkWidget *btn, gpointer dead_window);
extern int settings_UI(GtkWidget *btn_call, gpointer user_data);
//extern int on_test(GtkWidget *btn_call, gpointer user_data);

// Functions located in STL.c
extern int stl_model_load(model *mptr);					// loads STL from disk into structured memory
extern int stl_raw_load(model *mptr);

// Functions located in Image.c
extern int scan_build_volume(void);
extern int image_loader(model *mptr, int xpix, int ypix, int perserv_aspect);
extern int image_processor(GdkPixbuf *g_img_src1_buff, int img_proc_type);
extern int laser_scan_images_to_STL(void);
extern int laser_scan_matrix(void);
extern int image_to_STL(model *mptr);

// Functions located in Gerber.c
extern gbr_aperature *gbr_ap_make(slice *sptr);
extern int gerber_ap_insert(slice *sptr, gbr_aperature *ap_new);
extern int gerber_ap_purge(slice *sptr);
extern int gerber_ap_dump(slice *sptr);
extern int gerber_model_load(model * mptr);
extern int gerber_aperature_parse(slice *sptr, char *in_str, gbr_aperature *aptr);
extern int gerber_XYD_parse(char *in_str, vertex *vptr);
extern int gerber_flash_polygon(slice *sptr, gbr_aperature *aptr, float x, float y);
extern int gerber_trace_polygon(slice *sptr, gbr_aperature *cap, vertex *trace);
extern int gerber_ap_trace_merge(model *mptr,slice *sptr);

// Functions located in Print.c
extern int tool_matl_forward(int slot, float forward_amt);
extern int tool_matl_retract(int slot, float retract_amt);
extern int tool_tip_home(int slot);
extern int tool_tip_retract(int slot);
extern int tool_tip_deposit(int slot);
extern int tool_tip_touch(float delta_z);
extern int tool_air(int slot, int status);
extern int motion_complete(void);
extern void motion_done(void);
extern int print_vertex(vertex *vptr, int slot, int pmode, linetype *lptr); // moves/prints from current location to vertex location
extern int temperature_adjust(int slot, float newTemp, int wait4it);
extern int print_status(jobx *local_job);
extern int step_aside(int slot, int wait_duration);
extern int tool_PWM_set(int slot, int port, int new_duty);
extern int tool_PWM_check(int slot, int port);
extern int tool_power_ramp(int slot, int port, int new_duty);
extern void set_holding_torque(int status);
extern int goto_machine_home(void);
extern int comefrom_machine_home(int slot, vertex *vtarget);
extern void* build_job(void *arg);					// builds a job in separate thread

// Functions located in System_utils.c
extern int RPi_GPIO_Read(int pinID);
extern int load_system_config(void);		                        // load and process config.json file
extern int unit_calibration(void);					// run a unit level calibration
extern int save_unit_calibration(void);					// save unit calibration data to file
extern int ztable_calibration(void);					// table leveling function
extern int save_unit_state(void);					// save current operational state of unit
extern int load_unit_state(void);					// load current operational state of unit
extern int open_system_log(void);					// open system log file
extern int close_system_log(void);					// close system log file
extern int entry_system_log(char *in_data);				// add entry to system log file
extern int open_model_log(void);					// open model log file
extern int close_model_log(void);					// close model log file
extern int open_job_log(void);						// open job log file
extern int close_job_log(void);						// close job log file
extern int entry_process_log(char *in_data);				// add entry to process log file
extern int tinyGSnd(char *cmd);			                        // sends commands to tinyg controller
extern int tinyGRcv(int disp_val);			                // receives status from tinyg controller
extern void delay(unsigned int msecs);
extern int kbhit(void);
extern void dialog_okay_cb(GtkWidget *widget, gpointer dialog);
extern void dialog_cncl_cb(GtkWidget *widget, gpointer dialog);
extern void aa_dialog_box(GtkWidget *parent, int type, int display_dur, char *title, char *msg);
extern int pca9685Setup(const int pinBase, const int i2cAddress, float freq);
extern void pca9685PWMFreq(int fd, float freq);
extern void pca9685PWMReset(int fd);
extern void pca9685PWMWrite(int fd, int pin, int on, int off);
extern void pca9685PWMRead(int fd, int pin, int *on, int *off);
extern int baseReg(int pin);

// Functions located in Initialize.c
extern int init_system(void);
extern int init_device_RPi(void);
extern int init_device_tinyG(void);
extern int init_device_PWM(void);
extern int init_device_I2C(void);
extern int init_device_SPI(void);
extern int init_device_1Wire(void);
extern int init_general(void);

// Functions located in Thermal.c
extern void* ThermalControl(void *arg);					// manages thermal devices in separate thread
extern float read_RTD_sensor(int channel, int readings, int typ);	// reads temperature sensors (RTDs)
extern float read_THERMISTOR_sensor(int channel, int readings, int typ);// reads temperature sensors (thermistors)

// Functions located in Tool.c
static void on_tool_exit (GtkWidget *btn, gpointer dead_window);
extern int on_tool_change (GtkWidget *btn, gpointer user_data);
extern int tool_UI(GtkWidget *btn_call, gpointer user_data);
extern void xyztip_autocal_callback (GtkWidget *btn, gpointer user_data);
extern int tool_information_read(int slot, char *tool_file_name);
extern int tool_information_write(int slot);
extern int dump_tool_params(int slot);
extern int on_tool_settings(GtkWidget *btn_call, gpointer user_data);
extern void on_info_ok (GtkWidget *btn, gpointer dead_window);
extern int tool_type(GtkWidget *cmb_box, gpointer user_data);
extern int mats_type(GtkWidget *cmb_box, gpointer user_data);
extern int tool_add_matl(GtkWidget *btn, gpointer user_data);
extern int tool_sub_matl(GtkWidget *btn, gpointer user_data);
extern int tool_hotpurge_matl(GtkWidget *btn, gpointer user_data);
extern int load_tool_defaults(int slot);				// loads tool default values
extern int unload_tool(int slot);					// clears variables for tool data when tool removed from unit
extern int make_toolchange(int ToolID, int StepID);			// selects which tool to use
extern int print_toolchange(int newtool);				// sends a tool change command
extern int build_mat_file(GtkWidget *btn_call, gpointer user_data);	// prints a material file's line types

// Functions located in Vulkan.c
extern void create_instance();
extern void create_device();
extern void create_command_pool();
extern void create_render_pass();

extern void record_commands();

// Functions located in XML.c
extern genericlist *genericlist_make(void);
extern int genericlist_insert(genericlist *existing_list, genericlist *new_element);
extern int genericlist_delete_all(genericlist *existing_list);
extern genericlist *genericlist_delete(genericlist *existing_list, genericlist *gdel);
extern int genericlist_find(genericlist *existing_list, char *find_name);
extern linetype *linetype_make(void);
extern int linetype_insert(int slot, linetype *ltnew);
extern int linetype_delete(int slot);
extern linetype *linetype_copy(linetype *lt_inpt);
extern linetype *linetype_find(int slot, int ltdef);
extern float linetype_accrue_distance(slice *sptr, int ptyp);
extern operation *operation_make(void);
extern int operation_insert(int slot, operation *onew);
extern int operation_delete(int slot, operation *odel);
extern int operation_delete_all(int slot);
extern operation *operation_find_by_name(int slot, char *op_name);
extern operation *operation_find_by_ID(int slot, int op_ID);
extern int build_tool_index(void);					// reads tool database file and builds list of whats found
extern int XMLRead_Tool(int slot, char *tool_name);			// loads tool data from XML file
extern int build_material_index(int slot);				// reads material database file and builds list of whats found
extern int XMLRead_Matl_find_Tool(char *find_tool, char *find_matl);	// checks if matl is use-able by tool
extern int XMLRead_Matl(int slot,char *find_matl);			// loads material data from XML file




