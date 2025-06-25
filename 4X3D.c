#include <gtk/gtk.h>
#include <gdk/x11/gdkx.h>
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
#include <sched.h>
#include <termios.h>
#include <time.h>
#include <stdbool.h>
#include <sys/types.h> 		
#include <sys/stat.h>  		
#include <inttypes.h> 		
#include <linux/i2c-dev.h> 	
#include <sys/ioctl.h>
#include <sys/resource.h>
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
#define VTX_INDEX 25							// number of index pointers into vtx array linked list
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
  #define MAX_THERMAL_DEVICES 6						// expected number of heaters expected on system (bld tbl takes 1)
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
  #define MAX_THERMAL_DEVICES 7						// expected number of heaters expected on system (bld tbl takes 2)
  #define BLD_TBL1 MAX_TOOLS
  #define BLD_TBL2 MAX_TOOLS+1
  #define CHAMBER MAX_TOOLS+2
  #define UNIT_HAS_CAMERA 0
  #define CAMERA_POSITION 0
#endif
#ifdef GAMMA_UNIT
  #define MAX_TOOLS 1							// maximum number of tool slots available
  #define MAX_MATERIALS 2						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 3						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 4						// expected number of heaters expected on system (bld tbl takes 2)
  #define BLD_TBL1 1
  #define BLD_TBL2 2
  #define CHAMBER 3							// note: use 2 when reading from A2D
  #define UNIT_HAS_CAMERA 1
  #define CAMERA_POSITION 2
#endif
#ifdef DELTA_UNIT
  #define MAX_TOOLS 1							// maximum number of tool slots available
  #define MAX_MATERIALS 2						// maximum number of different materials available in one job
  #define MAX_MEMORY_DEVICES 3						// expected number of memory devices on system
  #define MAX_THERMAL_DEVICES 4						// expected number of heaters expected on system (bld tbl takes 2)
  #define BLD_TBL1 1
  #define BLD_TBL2 2
  #define CHAMBER 3							// note: use 2 when reading from A2D
  #define UNIT_HAS_CAMERA 1
  #define CAMERA_POSITION 1
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

// defines for camera use
#define CAM_LIVEVIEW 1
#define CAM_SNAPSHOT 2
#define CAM_JOB_LAPSE 3
#define CAM_SCAN_LAPSE 4
#define CAM_IMG_DIFF 5
#define CAM_IMG_SCAN 6

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
#define MAX_SCAN_IMAGES 500

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
#define MEASUREMENT 3							// tools that measure material already in build
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

// Thermal sensor defines
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
genericlist 	*tool_list;						// pointer to first tool name in index

// Arrayed vertex structure for OpenGL interface
typedef struct st_svtx
	{
	float	x,y,z,w;
	}svtx;
int		ndvtx;
svtx		dvtx[MAX_VERTS];
svtx		dnrm[MAX_VERTS];

// Physical position structure for TinyG interface
typedef struct st_position 						// Contains position information for carriage location
	{
	double x;							// x location			 
	double y;							// y location
	double z;							// z location
	double a;							// a location
	}position;
	
position 	PosIs,PosWas,PostG;					// current positon, previous position, last tinyG reported position

// Vertex structure for both 3D models AND 2D slices
typedef struct st_vertex{						// 3D point description.
	float			x;     					// Cartesian X
	float 			y;					// Cartesian Y
	float			z;					// Cartesian Z
	float		 	i;					// Moment about X, or scratch, or total cost for support tree
	float 			j;					// Moment about Y, or scratch, or current cost for support tree
	float 			k;					// Moment about Z, or scratch  or estimated remaining cost for support tree
	int			supp;					// flag to indicate if this vtx location will need support/cooling
	int			attr;					// Attribute and/or reusable generic descriptor
	struct st_facet_list	*flist;					// Pointer to facet list this vertex is referenced by in faceted geometries
	struct st_vertex 	*next;					// Pointer to next vertex in list
	}vertex;

vertex		*vtest_array;

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
vertex_list			*vtx_pick_list;

// Data structure for generic edge lists
typedef struct st_edge_list
	{
	edge			*e_item;
	struct st_edge_list	*next;
	}edge_list;
edge_list			*e_pick_list;

// Data structure for generic vector lists
typedef struct st_vector_list
	{
	vector			*v_item;
	struct st_vector_list	*next;
	}vector_list;
vector_list			*vec_pick_list;

// Data structure for generic polygon lists
typedef struct st_polygon_list
	{
	polygon			*p_item;				// address of polygon in list
	struct st_polygon_list	*next;					// pointer to next element in list
	}polygon_list;
polygon_list			*p_pick_list;				// user pick list of polygons

// Data structure for generic facet lists
typedef struct st_facet_list
	{
	facet			*f_item;
	struct st_facet_list	*next;
	}facet_list;
facet_list			*f_pick_list;

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
	struct st_polygon	*free_edge;				// pointer to free edge 3D polygon
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
branch_list			*b_pick_list;

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
	
slice	*heatmap;							// pointer to autogenerated support material slice
	
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
	float 			target_z_height,target_z_stop;		// raw matl target height to be milled and where model stops in z
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
model				*active_model;				// pointer to user selected active model, otherwise model held in job data struct

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

// Tool eeprom memory management
typedef struct st_memdev						// Contains memory device information
	{
	FILE		*fd;						// handle to device
	char		dev[20];					// device ID
	char		devPath[255];					// path to device
	char		desc[40];					// desciption of device
	int		devType;					// 23 for eeprom, 28 for thermal, etc.
	eeprom		md;						// memory data
	}memdev;

int memory_devices_found=0;						// number of memory devices actually found
int thermal_devices_found=0;						// number of thermal devices actually found

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
pthread_t 	thermal_tid;						// thread to manage thermal stuff
pthread_mutex_t thermal_lock;						// mutex to manage thermal thread variables
int		thermal_thread_alive;					// detectable heartbeat to ensure thread is still alive
float 		temperature_T;						// value output of thermistor (volts or temp)
float 		temp_calib_delta=50.0;					// the temp difference usde to set high/low calib points

pthread_t 	build_job_tid;						// thread to manage printing
int		print_thread_alive;					// detectable heartbeat to ensure thread is still alive

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
// Note this struct defines the operations that this tool is able to perform.
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
toolx Tool[MAX_THERMAL_DEVICES];					// array corrisponds to slot on carriage plus build table

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
	
carriage crgslot[MAX_THERMAL_DEVICES];

// Job data structure
typedef struct st_job
	{
	int			sync;					// flag used to syncronize public data with print thread local data
	int			state;					// current status of the job
	int			prev_state;				// previous status of the job
	int			regen_flag;				// indicates if job values need refresh
	int			type;					// additive, subtractive, mixed, etc.
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
jobx job;			

long int	vector_qty;
vector		*vector_first;
vector		*vector_last;

// mouse pick and reference entities
vertex 		*vtx_pick_new;
vertex 		*vtx_pick_ref;
vertex		*vtx_pick_ptr;
vertex		*vtx_zero_ref;
facet 		*fct_pick_ref;

// hash table indexing entities
vertex		*vtx_index[VTX_INDEX][VTX_INDEX][VTX_INDEX];		// array of ptrs to speed vtx loading and searching [X][Y][Z]

// Command buffer to motion processor
// It retains a copy of what was sent to processor, but not yet executed
typedef struct st_cmdBuffer
	{
	char			cmd[255];				// copy of gcode command
	unsigned long 		tid;					// transaction ID
	unsigned long 		vid;					// vertex ID (count)
	vertex 			*vptr;					// vertex ID from which the move originated
	}cmdBuffer;
cmdBuffer 			cmd_buffer[MAX_BUFFER];			// holds FIFO index of verticies sent to tinyG pending execution


//vl6180 handle;


// tinyG status variables
int	bufferAvail=MAX_BUFFER;						// tracks buffering status into tinyG controller, max=32
int 	bufferAdded=0;
int 	bufferRemoved=0;
int	Rx_bytes=0;							// number of bytes currently in rcv buffer of tinyG
int	Rx_max=300;							// rcv buffer size of tinyG
int	cmd_que=0;
int	cmdCount=0;							// keeps track of the number of commands queued with tinyG
int 	vtxCount=0;
int 	cmdBurst=1;							// sets the limit for pending commands with the tinyG
int	cmdControl=0;							// flag to indicate if counting cmds or not
int	oldState;
int 	crt_min_buffer;
unsigned long TId;							// transaction ID for each command sent to tinyG
unsigned long VId;							// vertex ID for each command sent to tinyG

// Global variables
//struct 	wiringPiNodeStruct *node;
int 	USB_fd;						  		// handle to tinyG stepper controller
int	I2C_A2D_fd[2];							// handles to A2D modules on I2C bus
int	I2C_PWM_fd;							// handle to PWM on I2C bus
int	PWMsettings;							// settings of PWM
int	I2C_DS_fd;							// handle to Distance Sensor on I2C bus
int 	SPI_fd;								// handle to SPI bus devices
FILE 	*gcode_in;							// handle to gcode input file	(was gd)
FILE 	*gcode_out;							// handle to gcode output file 	(was gc_ptr)
FILE 	*gcode_gen;							// handle to gcode output file 	(was gc_ptr)
FILE	*proc_log;							// handle to process log file (human text of what's happening)
FILE	*model_log;							// handle to record semi-detailed account of model run
FILE 	*system_log;							// handle to record system events
FILE	*Tools_fd;							// handle to Tools.XML file
FILE	*Mats_fd;							// handle to Materials.XML file
FILE	*Opers_fd;							// handle to Operations.XML file
FILE	*unit_state;							// handle to state of unit
FILE	*unit_calib;							// handle to unit calibration data file
char	calib_file_rev[10];						// revision of unit calibration data file
int	unit_calib_required=TRUE;					// flag to indicate if unit level calibration is required
FILE 	*model_in;							// handle to model input file
int	gcode_send=1;
int 	save_logfile_flag=1;						// flag to save or ignore job log file
char 	scratch[255];							// scratch string space - used everywhere - use with caution
char 	blank[255];							// string of spaces for clearing screen
int   	tinyg_send=1;		    					// flags to indicate nature of command output
char	tinyg_cmd[5][255];						// holds the command list of what has been sent to tinyg
char	rcv_cmd[255];							// holds received data from tinyG
char 	gcode_cmd[512];							// string that holds descrete moves
char	gcode_burst[1024];						// string that holds accumulated moves
int	burst=0;							// flag to indicate to send descrete moves or accumulate moves

int	PWM_direct_control=FALSE;					// flag to control PWM module directly thru thermal thread
int	PWM_direct_status=FALSE;					// flag to indicate status of PMW direct setting
char	PWM_direct_port;						// value from 0-15 indicating which PMW port
int 	PWM_duty_cycle;							// duty cycle from 0-100% 
int	PWM_duty_report;						// value of duty cycle as read back from PWM

// set up formatting for GTK messages - see Initialize.c for details
  char *markup;
  char error_fmt[255];
  char okay_fmt[255];
  char norm_fmt[255];
  char unkw_fmt[255];
  char stat_fmt[255];

// 3D Model viewing
  int	MVview_click=0;
  float MVdisp_spin=-0.7892,MVdisp_tilt=3.753;				// spin and tilt factors for Model Viewing
  float MVdisp_cen_x=0,MVdisp_cen_y=0,MVdisp_cen_z=0;
  int 	MVgxoffset=425,MVgyoffset=375; 			                // location on the display area
  int	MVgx_drag_start=0,MVgy_drag_start=0;
  facet *MDisplay_facets;						// pointer to list of facets to display
  edge 	*MDisplay_edges;						// pointer to list of edges to display
  float pers_scale=0.10;						// perspective scalar.  0.55 matches camera
  int	set_view_image=1;						// turn on/off drawing of source image
  int	Superimpose_flag=0;						// superimpose 3D geometry over camera image flag
  int	Perspective_flag=0;						// view 3D geometry with depth perspective flag

// 2D Layer viewing
#ifdef ALPHA_UNIT
  float	MVview_scale=2.00;						// scale factor for viewing
  float	LVview_scale=2.10;						// scale factor for viewing
  int 	LVgxoffset=225,LVgyoffset=(-475); 		                // location on the display area
#endif
#ifdef BETA_UNIT
  float	MVview_scale=1.50;						// scale factor for viewing
  float	LVview_scale=1.70;						// scale factor for viewing
  int 	LVgxoffset=255,LVgyoffset=(-530); 		                // location on the display area
#endif
#ifdef GAMMA_UNIT
  float	MVview_scale=1.20;						// scale factor for viewing
  float	LVview_scale=1.00;						// scale factor for viewing
  int 	LVgxoffset=225,LVgyoffset=(-480); 		                // location on the display area
#endif
#ifdef DELTA_UNIT
  float	MVview_scale=1.20;						// scale factor for viewing
  float	LVview_scale=1.00;						// scale factor for viewing
  int 	LVgxoffset=225,LVgyoffset=(-480); 		                // location on the display area
#endif
  float	LVdisp_spin=-0.6428,LVdisp_tilt=3.665;				// spin and tilt factors for Layer Viewing
  float	LVdisp_cen_x=0,LVdisp_cen_y=0,LVdisp_cen_z=0;
  int	LVgx_drag_start=0,LVgy_drag_start=0;
  float	slc_view_start,slc_view_end,slc_view_inc;			// starting/ending z values of slices to view in 2D
  int 	slc_just_one=1,slc_show_all=0,slc_print_view=1;			// flags for showing just one layer, or all layers, and printing what one sees

// Graph viewing
  float	GVview_scale=2.10;						// scale factor for viewing slices
  float	GVdisp_spin=0.7854,GVdisp_tilt=3.665;				// spin and tilt factors for Layer Viewing
  float	GVdisp_cen_x=0,GVdisp_cen_y=0,GVdisp_cen_z=0;
  int 	GVgxoffset=225,GVgyoffset=(-475); 		 	      	// location on the display area
  int	GVgx_drag_start=0,GVgy_drag_start=0;
  int	GRedraw_flag=TRUE;

// Camera viewing
  float CVview_scale=1.0;						// scale factor for viewing slices
  float CVdisp_spin=-0.6428,CVdisp_tilt=3.665;				// spin and tilt factors for Layer Viewing
  float CVdisp_cen_x=0,CVdisp_cen_y=0,CVdisp_cen_z=0;
  int 	CVgxoffset=0,CVgyoffset=0;		 		 	// location on the display area
  int	CVgx_drag_start=0,CVgy_drag_start=0;
  int	CRedraw_flag=TRUE;
  float cam_roi_x0=0.0,cam_roi_y0=0.0,cam_roi_x1=1.0,cam_roi_y1=1.0;	// region of interest (digital zoom)
  int 	save_image_history=0;						// flag to save or ignore images during build
  int 	cam_image_mode=0;						// flag to set what user wants to do with camera

// Image processing
  guchar *pixfilter;
  int	edge_detect_threshold=50;					// value used to identify edges.  lower =  more edges
  int	xy_on_table[415][415];						// xy map of object on build table
  int	need_new_image=TRUE;

// Gerber file format translator
  int	gerber_format_xi,gerber_format_xd;				// number of digits before and after the decimal for X
  int	gerber_format_yi,gerber_format_yd;				// number of digits before and after the decimal for Y
  int	gerber_polarity;						// gerber polarity: 0=clear 1=dark
  int	gerber_units;							// gerber file units: 0=undefined, 1=inches, 2=millimeters
  int	gerber_region_flag;						// flag to indicate if reading values for a specific region
  int	gerber_quad_mode;						// gerber quandrant mode
  int	gerber_interp_mode;						// gerber interpolation mode
  int	gerber_vtx_inter=1000;
  int	gerber_track_ID;

int	old_mouse_x=0,old_mouse_y=0;
int	tinyG_state=0;							// general status of tinyG card
int	tool_count=0;							// the number of active tools
float 	acceptable_temp_tol=5.0;					// temps within 5C are okay to run
int	pu_flag=0;

char	op_description[MAX_OP_TYPES][32];				// Description of each possible operation
char	lt_description[MAX_LINE_TYPES][32];				// Description of each possible line type

// Memory management variables
// These are used to ensure no memory leaks exist across all program functions
long int	vertex_mem;						// Keeps track of maximum number of vertices in memory
long int	edge_mem;						// Keeps track of maximum number of edges in memory
long int	facet_mem;						// Keeps track of maximum number of facets in memory
long int	vector_mem;						// quantity of vectors held in memory (typically more than in linked list)
long int	polygon_mem;						// Keeps track of maximum number of polygons in memory
long int	slice_mem;						// Keeps track of maximum number of slices in memory
long int	model_mem;						// Keeps track of maximum number of models in memory
	
// Unit specific parameters
int		barx;

// Tracking info
int		unit_in_error=FALSE;					// generic flag set when something is wrong - a check engine light of sorts
float 		XMin,YMin,ZMin;						// min and max of ALL models in memory
float 		XMax,YMax,ZMax;						// see model structure for more details
float 		cam_offset_x,cam_offset_y;				// distance sensor offsets
int		current_copy=1;
int		new_copy_cnt_flag=FALSE;				// flag to indicate copy count has changed
int		global_offset=1;					// flag to indicate that offsets are applied globally (to all tools)
float 		curDist,minDist;					// current and minium total move distance
float		z_cut=1.0, z_cut_at_pause=1.0;				// global slice level in mm. a model's min/max must be in range of z_cut to show.
time_t 		idle_start_time,idle_current_time;			// timer used to detect inactivity
int		job_count_total=0;					// total count of jobs this unit has run
int 		job_count_fail=0;					// total count of jobs that did not provide a good result
int		tinyG_state;			  			// general status of tinyG card
int		tinyG_in_motion;					// flag to indicate if motion complete or not
char		tinyG_id[15];						// id of this specific tinyG card
int		current_tool,previous_tool;				// tool ID (i.e. print head) in use
int		current_step,previous_step;				// tool or carriage power in use
float 		step_inc;				    		// generic value to increase/reduce param adjustments
int 		abort_flag;			  			// indicates if abort request is pending
int		adjfeed_flag=1;						// flag to turn on/off feed rate adjustment for tiny vectors
float 		adjfeed_trigger=2.00;					// value at which feed adjustment kicks in (vector length in mm)
int		adjflow_flag=1;						// flag to turn on/off flow rate adjustment for tiny vectors
float 		adjflow_trigger=0.75;					// value at which flow adjustment kicks in (vector length in mm)
int		first_layer_flag=0;					// flag to indicate first layer deposition
float	  	Bld_Tbl_Lvl[4];						// z location of each corner at z=40.0mm
int		bld_lvl_x_ref,bld_lvl_y_ref;				// index of build table z level reference location in array
float	 	bit_load_x,bit_load_y,bit_load_x_inc,bit_load_y_inc;
float 		grid_x_delta=0.75,grid_y_delta=0.75;			// grid spacing used to build supports and base layers
int		i2c_devices_found;					// i2c devices
int		wdog=0;					      		// watchdog kick value
int		dr_offset=0,dr_fill=0,dr_spt=0;
int		mdlmm_count=0;
long 		move_buffer_count;					// the current total number of moves submitted to the tinyG during a layer
long	 	move_buffer_rewind;					// move count to go back to as requested
int		show_path=0;
int		autoplacement_flag=1;
int		ignore_tool_bit_change=0;
int		drill_holes_done_flag=0;
int		only_drill_small_holes_flag=1;				// flag to only drill holes smaller than milling bit diam
int		ztip_test_point=1;
int		model_align_flag=1;
int		linet_state[MAX_TOOLS];					// see definitions above
int		oper_state[MAX_TOOLS];
int		ibreak=1;
int		init_done_flag=FALSE;					// flag to indicate full system init done
int		gpio_ready_flag=FALSE;					// flag to indicate gpio can be used
int		carriage_at_home=FALSE;					// flag to indicate if carriage at home position
int		on_part_flag=FALSE;					// flag to indicate if tool is working on part
vertex 		*pause_vtx=NULL;					// model vtx at which pause occured
vertex		*vtx_debug;
vertex 		*vmid,*vlow,*vhgh;
vector		*vec_debug;
int		material_feed=FALSE;
int		max_feed_time=10;
int		tools_need_tip_calib=TRUE;
time_t 		read_cur_time,read_prev_time;
float 		oldposta=10.0;
float 		old_jobz=0.0;

// block target params
int		cnc_add_block_flag=1;  
int		cnc_profile_cut_flag=0;
float		cnc_x_margin=5.0,cnc_y_margin=5.0;			// margins beyond model max/min to mill
float		cnc_z_height=17.0,cnc_z_stop=2.0;			// raw matl block height to be milled and where model stops in z

// build table scanning variables
int 		build_table_scan_flag=TRUE;				// flag to indicate if build table needs to be mapped to the model
int		scan_in_progress=FALSE;					// flag to indicate if a scan is in progress
int		z_tbl_comp=TRUE;					// flag to turn on/off build table z compensation
int		old_z_tbl_comp=TRUE;
float		set_z_contour=10.0;					// the z height to contour up to
float  		sensor_z_height=200.0;					// ideal height for non-contact z sensor
float	 	scan_dist=20.0;						// distance in mm between scan points on build table
float 		sweep_speed=1000;					// speed of sweep scan
int 		scan_xmax,scan_ymax;					// max array size at current scan_dist
float	 	scan_x_remain,scan_y_remain;				// offset of first scan point from table 0,0 during scans
float 		scan_x_start,scan_y_start,scan_x_end,scan_y_end;	// start and end pts of scan area
int		Pz_index;						// used as counter to track amount of completion
float	 	Pz[SCAN_X_MAX][SCAN_Y_MAX];				// grid of Z values across build table [x][y]
float	 	Pz_raw[SCAN_X_MAX][SCAN_Y_MAX];
int		Pz_att[SCAN_X_MAX][SCAN_Y_MAX];
int 		scan_img_index[MAX_SCAN_IMAGES];			// index of file sub-names (position values)
int 		scan_image_count=0;					// total count of scanned images in TableScan subdirectory
int		scan_image_current=0;					// current value into index

// variables used to track history
int	 	hist_type;						// 0=thermal, 1=build time, etc.
float		history[MAX_HIST_FIELDS][MAX_HIST_COUNT];		// user selected data fields
int		hist_ctr[MAX_HIST_TYPES];				// number of data elements collected for estimate (e) or print (p)
int		hist_use_flag[MAX_HIST_FIELDS];				// flag to indicate if field is in use/displayed
int	 	hist_tool[MAX_HIST_FIELDS];				// defines which tool data relates to
int		hist_color[MAX_HIST_FIELDS];				// line color in graph
char 		hist_name[MAX_HIST_FIELDS][255];			// name of each field

// debug flag used as "in code" method of out putting data to console during debug.
// flag values follow the general rules of:
//   0-99  =  vector related functions
// 100-199 =  polygon related functions
// 200-299 =  slice related functions
// 300-399 =  model related functions
// 400-499 =  job related functions
// 500-599 =  deposition related functions
// 600-699 =  camera/image functions
int		debug_flag=0;
long int	total_vector_count=0;


// geometery processing
float 		max_spt_angle=(-45.0),max_base_angle=0.0;
float 		min_perim_length=0.1,min_polygon_area=0.5;		// min perimeter length of viable model perimeter polygon
float 		ss_wavefront_increment=0.025;
float 		ss_wavefront_maxdist=2.0;
float 		ss_wavefront_crtdist=0.025;
float 		max_colinear_angle=3.00;
float 		min_vector_length=0.10;
int		nonboolean_input=FALSE;
float	 	max_support_facet_len=25.0;				// mm
int		max_vtxs_per_polygon=5000;
int		max_polygons_per_slice=1000;
float	 	is_round_radii_tol=0.05;				// used as % difference in radius
float 		is_round_length_tol=0.02;				// used as % difference in segment length
int		poly_fill_none=0,poly_fill_all=0,poly_fill_unique=0;	// flags regarding how to apply fills to polygons
float  		poly_fill_flow,poly_fill_feed,poly_fill_thick,poly_fill_pitch,poly_fill_angle;	// global values for polygon filling
int		poly_perim_none=0,poly_perim_all=0,poly_perim_unique=0;	// flags regarding how to apply fills to polygons
float 		poly_perim_flow,poly_perim_feed,poly_perim_thick,poly_perim_pitch;
int		slice_has_many_polygons[MAX_MDL_TYPES],slice_offset_type_qty[MAX_MDL_TYPES];	// controls type of offsetting function to use

// print control settings
#ifdef ALPHA_UNIT
  float 	xoffset=100.0;						// generic offset from origin for all moves
  float 	yoffset=32.0;						// positive values move AWAY from stepper motors/limit switches
#endif
#ifdef BETA_UNIT
  float 	xoffset=110.0;					  	// generic offset from origin for all moves
  float 	yoffset=45.0;						// positive values move AWAY from stepper motors/limit switches
#endif
#ifdef GAMMA_UNIT
  float 	xoffset=(0.0);					  	// generic offset from origin for all moves
  float 	yoffset=0.0;						// positive values move AWAY from stepper motors/limit switches
#endif
#ifdef DELTA_UNIT
  float 	xoffset=(-20.0);				  	// generic offset from origin for all moves
  float 	yoffset=8.0;						// positive values move AWAY from stepper motors/limit switches
#endif
float 		zoffset=0.0;
int		smart_check=1;						// turn on/off job rules checking
int		set_assign_to_all=0;					// flag to assign tools to all models
int	 	set_ignore_lt_temps=0;					// turn on/off ignoring line type temperature changes
int		set_center_build_job=1;					// turn on/off centering of build job on table
int		set_vtx_overlap=0;					// qty of vectors to overlap when depositing contour polygons
int		set_build_leveling=1;					// turn on/off build table leveling
int		set_start_at_crt_z=FALSE;				// turn on/off starting at z=0 or current z height
float 		z_start_level=0.000;					// value of z where printing will start from
int		set_force_z_to_table=TRUE;				// turn on/off forcing non-zero z start to build table
float 		set_z_manual_adj=0.10;					// z manual adjust while job running
int	 	set_include_btm_detail=FALSE;				// turn on/off 1st layer details if below slice height
int		fidelity_mode_flag=FALSE;				// turn on/off high fidelity mode
float 		fidelity_feed_adjust=0.50,fidelity_flow_adjust=0.90;	// high fidelity adjustment values
float	 	x_move_velocity=(-1),y_move_velocity=(-1),z_move_velocity=(-1);
int		motion_is_complete=TRUE;				// flag to indicate code is in motion complete function
int 		tool_air_status_flag=OFF;				// flag to indicate if air pump is on/off
int 		tool_air_override_flag=FALSE;				// flag to indicate if air pump is not under line type control
float 		tool_air_slice_time=15.0;				// tool air will turn on if slice time under this limit
vertex 		*xsw_pos,*ysw_pos;					// x/y tool calib switch positions

// test control settings
int		test_slot=0;
int		test_element=0;
int 		move_slot=0;
float 		move_cdist=10,move_tdist=10;
float 		move_x=10,move_y=10,move_z=10;
float 		t24v_duty=50,t48v_duty=50,PWMv_duty=50;
int		aux2_port=6;						// PWM channel sent to tool
int		aux3_port=7;						// PWM channel sent to tool

// Settings flags
int		set_view_endpts=0;					// turn on/off vector endpoints when displaying slices
int		set_view_vecnum=0;					// turn on/off vector sequence numbers when displaying slices
int		set_view_veclen=0;					// turn on/off vector length when displaying slices
int		set_view_vecloc=0;					// turn on/off vector location of where it WILL BE deposited
int		set_view_poly_ids=0;					// turn on/off polygon IDs
int		set_view_poly_fam=0;					// turn on/off polygon family relationships (i.e. parent/child)
int		set_view_poly_dist=0;					// turn on/off polygon offset distance
int		set_view_poly_perim=0;					// turn on/off polygon perimeter length
int		set_view_poly_area=0;					// turn on/off polygon area
int 		set_view_postloc=0;					// turn on/off vector location of where it WAS deposited
int		set_view_raw_stl=0;					// turn on/off slt vector viewing
int		set_view_raw_mdl=0;					// turn on/off raw slice vector viewing
int		set_view_raw_off=0;	
int		set_view_strskl=0;					// turn on/off offset straight skeleton when displaying slices
int		set_view_lt[MAX_LINE_TYPES];				// turn on/off display of each/any line type
int		set_view_model_lt_width=1;				// turn on/off display of line width in 2D view
int		set_view_support_lt_width=1;				// turn on/off display of line width in 2D view
int		set_view_lt_override;					// flag to indicate viewed line types no longer match tools operation sequence
int		set_view_build_lvl=0;					// turn on/off build table z level
int		set_view_mdl_slice=1;					// turn on/off slice viewing in model view
int		set_view_mdl_bounds=0;					// turn on/off bounding box around each model
int		set_view_tool_pos=0;					// turn on/off display of tool position while printing
float	 	set_view_edge_angle=75;					// angle to display edges in 3D
float	 	set_view_edge_min_length=0.25;				// minimum length of edge to display in 3D
int		set_show_above_layer;					// turn on/off displaying layer above current zcut
int		set_show_below_layer;					// turn on/off displaying layer below current zcut
int		set_auto_zoom=1;
int		set_view_models=1;					// flag to show/hide model geometry
int		set_view_supports=0;					// flag to show/hide support geometry
int		set_view_support_tree=0;				// flag to show/hide support tree
int		set_view_internal=0;					// flag to show/hide internal geometry
int		set_view_target=0;					// flag to show/hide target geometry
int 		set_view_patches=0;
int		set_view_normals=0;
int		set_view_edge_support=0;
int		set_view_edge_shadow=0;
int 		set_view_free_edges=1;
int		set_edge_display=1;
int 		set_facet_display=0;
int		set_view_bound_box=1;
int		show_view_bound_box=1;
int		set_mouse_select=1;
int		set_mouse_clear=0;
int		set_mouse_drag=0;
int 		set_mouse_align=0;
int		set_mouse_center=0;
int		set_mouse_merge=0;
int		set_mouse_pick_entity=2;				// 0=none, 1=vtx, 2=facets, 3=patches
int		set_show_pick_neighbors=1;
int		set_show_pick_clear=0;
int		set_view_max_facets=10;
int 		set_view_penup_moves=0;
int		set_suppress_qr_cmd=FALSE;
int		set_suppress_mots_cmd=FALSE;
int		display_model_flag=TRUE;
int		good_to_go=FALSE;
int		set_temp_cal_load=FALSE;

char 		new_mdl[255];
char		new_tool[255];

GError*		my_errno;


// vulkan display variables
VkInstance 		instance;
VkPhysicalDevice 	physical_device;
VkDevice 		device;
uint32_t 		queue_family_index = UINT32_MAX;
VkQueue 		queue;
VkCommandPool 		command_pool;
VkCommandBuffer 	command_buffer;
VkSurfaceKHR 		surface;
VkSurfaceFormatKHR 	surface_format;
VkRenderPass 		render_pass;
VkPipelineLayout 	pipeline_layout;
VkPipeline 		graphics_pipeline;
VkSemaphore 		image_available_semaphore;
VkSemaphore 		render_finished_semaphore;
VkImageView 		image_view;
VkFramebuffer 		framebuffer;


// GTK4 specific globals
int		widget_init=0;
int		win_tool_flag[MAX_TOOLS];				// flag to indicate if tool window is displayed
int		win_modelUI_flag=FALSE;					// flag to indicate if modelUI window is displayed
int		win_settingsUI_flag=FALSE;				// flag to indicate if settingsUI window is displayed
int		win_tooloffset_flag=FALSE;				// flag to indicate if tool temp offset window is displayed
int		win_testUI_flag=FALSE;					// flag to indicate if testUI window in settings is displayed
int		main_view_page=1;					// indicates which main page user is currently viewing
double 		da_center_x=400,da_center_y=250;                	// location of what is being displayed
GtkWidget	*win_main,*win_model,*win_tool,*win_settings;		// primary windows
GtkWidget	*g_notebook;						// primary notebook for various views
GtkWidget 	*frame_time_box;					// primary window time/status box
GtkWidget	*img_job_btn, *img_job_abort, *img_settings;		// persistant buttons 
GtkWidget	*btn_settings, *btn_run, *btn_abort, *btn_test, *btn_quit;
int		img_job_btn_index,img_model_index,img_tool_index[4];
GtkWidget	*combo_mdls;
GtkWidget	*combo_lts;

GtkAdjustment  	*g_adj_z_level;
GtkWidget	*g_adj_z_scroll;

GtkWidget	*aa_dialog;						// generic user dialog window
int		aa_dialog_result=0;					// holds result of user dialog interactions

// model view dependent buttons
GtkWidget	*btn_model_add,*btn_model_sub,*btn_model_select,*btn_model_clear,*btn_model_options,*btn_model_align;
GtkWidget	*img_model_add,*img_model_sub,*img_model_select,*img_model_clear,*img_model_options,*img_model_align;
GtkWidget	*btn_model_XF,*btn_model_YF,*btn_model_ZF,*btn_model_view;
GtkWidget	*img_model_XF,*img_model_YF,*img_model_ZF,*img_model_view;
GtkWidget	*g_model_all,*g_model_area,*g_model_btn,*grid_model_btn;
GtkWidget	*btn_model_control, *btn_model_load, *btn_model_remove, *btn_tool[4];
GtkWidget	*img_model_control, *img_model_load, *img_model_remove, *img_tool[4];
GtkWidget	*btn_model_settings,*img_model_settings;

// tool view dependent buttons
GtkWidget	*g_tool_all,*g_tool_btn,*grid_tool_btn;
GtkWidget 	*btn_tool_ctrl, *img_tool_ctrl;
GtkWidget	*material_lbl[4],*type_lbl[4],*mdl_lbl[4],*mlvl_lbl[4];
GtkWidget 	*combo_tool,*combo_mats;
GtkWidget	*lbl_tool_stat,*lbl_slot_stat,*lbl_tool_memID,*lbl_tool_desc;
GtkWidget	*lbl_val_pos_x,*lbl_val_pos_y,*lbl_val_pos_z,*lbl_val_pos_a;
GtkWidget	*lbl_val_spd_crt,*lbl_val_spd_avg,*lbl_val_spd_max;
GtkWidget 	*lbl_val_flw_crt,*lbl_val_flw_avg,*lbl_val_flw_max;
GtkWidget 	*lbl_val_cmdct,*lbl_val_buff_used,*lbl_val_buff_avail;
GtkWidget	*btn_tool_feed_fwd, *btn_tool_feed_rvs;
GtkWidget	*img_tool_feed_fwd, *img_tool_feed_rvs;
float 		PostGVel,PostGVAvg,PostGVMax,PostGVCnt,PostGFlow,PostGFAvg,PostGFMax,PostGFCnt;

// layer view dependent buttons
GtkWidget	*g_layer_all,*g_layer_area,*g_layer_btn,*grid_layer_btn;
GtkWidget	*btn_layer_count,*btn_layer_eol,*btn_merge_poly,*btn_poly_attr;
GtkWidget 	*img_layer_count,*img_layer_eol,*img_merge_poly,*img_poly_attr;
GtkWidget	*btn_layer_settings,*img_layer_settings;
GtkAdjustment	*slc_start_adj,*slc_end_adj,*slc_inc_adj;
GtkWidget	*slc_start_btn,*slc_end_btn,*slc_inc_btn;
GtkWidget	*just_current_btn,*show_all_btn;
GtkAdjustment	*perim_fed_adj,*perim_flw_adj,*perim_thk_adj,*perim_pitch_adj;
GtkWidget	*perim_fed_btn,*perim_flw_btn,*perim_thk_btn,*perim_pitch_btn;
GtkWidget	*perim_no_pattern_btn,*perim_same_pattern_btn,*perim_unique_pattern_btn;
GtkAdjustment	*fill_fed_adj,*fill_flw_adj,*fill_thk_adj,*fill_pitch_adj,*fill_angle_adj;
GtkWidget	*fill_fed_btn,*fill_flw_btn,*fill_thk_btn,*fill_pitch_btn,*fill_angle_btn;
GtkWidget	*fill_no_pattern_btn,*fill_same_pattern_btn,*fill_unique_pattern_btn;

// graph view dependent buttons
GtkWidget 	*g_graph_all,*g_graph_area,*g_graph_btn,*grid_graph_btn;
GtkWidget 	*btn_graph_temp,*img_graph_temp;
GtkWidget 	*btn_graph_time,*img_graph_time;
GtkWidget 	*btn_graph_matl,*img_graph_matl;

// camera view variables
char 		bashfn[255];
GtkWidget	*g_camera_all,*g_camera_area,*g_camera_btn,*grid_camera_btn,*g_camera_image;
GdkPixbuf	*g_camera_buff;

// camera view dependent buttons
GtkWidget 	*btn_camera_liveview,*img_camera_liveview;
GtkWidget 	*btn_camera_snapshot,*img_camera_snapshot;
GtkWidget 	*btn_camera_job_timelapse,*img_camera_job_timelapse;
GtkWidget 	*btn_camera_scan_timelapse,*img_camera_scan_timelapse;
GtkWidget 	*btn_camera_review,*img_camera_review;
GtkWidget	*btn_camera_settings,*img_camera_settings;

// vulkan view dependent buttons
GtkWidget	*g_vulkan_area;

// generic re-usable windows for posting progress bars, msgs, and getting tid bits of data
GtkWidget	*win_info,*grd_info,*btn_done;					
GtkWidget	*info_progress,*info_percent,*info_sleep,*grid_info,*lbl_info1,*lbl_info2;
GtkWidget	*win_stat,*grd_stat,*lbl_stat1,*lbl_stat2,*btn_stat;		
GtkWidget	*win_data,*grd_data;						

// model UI widgets
GtkWidget	*lbl_job_type, *lbl_job_mdl_qty, *lbl_job_slice_thk;
GtkWidget	*lbl_job_xmax, *lbl_job_ymax, *lbl_job_zmax;
GtkWidget 	*lbl_mdl_error, *lbl_model_name;
GtkAdjustment	*xrotadj,*yrotadj,*zrotadj;
GtkWidget	*xrotbtn,*yrotbtn,*zrotbtn;
GtkAdjustment	*xposadj,*yposadj,*zposadj;
GtkWidget	*xposbtn,*yposbtn,*zposbtn;
GtkAdjustment	*xscladj,*yscladj,*zscladj;
GtkWidget	*xsclbtn,*ysclbtn,*zsclbtn;
GtkWidget	*xmirbtn,*ymirbtn,*zmirbtn;
GtkWidget 	*lbl_x_size,*lbl_y_size,*lbl_z_size;			// model sizes in current orientation
GtkAdjustment	*mdl_copies_adj;					// number of copies of this model
GtkWidget	*mdl_copies_btn;					// spin btn for number of copies

GtkAdjustment	*xtipadj,*ytipadj,*ztipadj;
GtkWidget	*xtipbtn,*ytipbtn,*ztipbtn;

GtkAdjustment	*edge_angle_adj;
GtkWidget	*edge_angle_btn,*lbl_edge_angle;

GtkAdjustment	*facet_qty_adj;						// model view max facets to display
GtkWidget	*facet_qty_btn,*lbl_facet_qty;		

GtkAdjustment	*temp_tol_adj,*temp_chm_or_adj,*temp_tbl_or_adj;	
GtkWidget	*temp_tol_btn,*lbl_temp_tol;		
GtkWidget	*lbl_chmbr_temp_or,*lbl_table_temp_or;
GtkWidget	*temp_chm_or_btn,*temp_tbl_or_btn;

GtkAdjustment	*facet_tol_adj;				
GtkWidget	*facet_tol_btn,*lbl_facet_tol;			
GtkAdjustment	*colinear_angle_adj,*wavefront_inc_adj;

GtkAdjustment	*pix_size_adj;				
GtkWidget	*pix_size_btn,*lbl_pix_size;			
GtkAdjustment	*pix_increment_adj;				
GtkWidget	*pix_increment_btn,*lbl_pix_incr;			

GtkAdjustment	*matl_feed_time_adj;
GtkWidget	*matl_feed_time_btn,*lbl_matl_feed_time;

GtkAdjustment	*img_edge_thld_adj;
GtkWidget	*img_edge_thld_btn,*lbl_img_edge_thld;


char		job_state_msg[80],job_status_msg[255];
GtkWidget	*job_status_lbl,*job_state_lbl;
GtkWidget	*elaps_time_lbl,*est_time_lbl;
GtkWidget	*img_madd,*btn_madd;					// material add/subtract btns on tool menu
GtkWidget	*img_msub,*btn_msub;

GtkWidget	*stat_box,*time_box,*grid_time;
GtkWidget	*grid_job,*stat_lbl_job,*stat_lbl_start,*stat_lbl_end;

// temperature and material bar graphs for MAIN window status frame.
// they are updated by the on_idle function.
GtkWidget	*temp_lbl[MAX_THERMAL_DEVICES],*setp_lbl[MAX_THERMAL_DEVICES],*matl_lbl[MAX_THERMAL_DEVICES];
GtkWidget	*temp_lvl[MAX_THERMAL_DEVICES],*matl_lvl[MAX_THERMAL_DEVICES];
GtkLevelBar	*temp_bar[MAX_THERMAL_DEVICES],*matl_bar[MAX_THERMAL_DEVICES];
GtkWidget	*type_desc[MAX_THERMAL_DEVICES],*matl_desc[MAX_THERMAL_DEVICES],*mdl_desc[MAX_THERMAL_DEVICES];
GtkWidget	*tool_stat[MAX_THERMAL_DEVICES],*linet_stat[MAX_THERMAL_DEVICES],*oper_st_lbl[MAX_THERMAL_DEVICES],*tip_pos_lbl[MAX_THERMAL_DEVICES];
GtkWidget	*temp_lvl_bldtbl,*temp_lvl_chamber;
GtkLevelBar	*temp_bar_bldtbl,*temp_bar_chamber;
GtkWidget	*lbl_bldtbl_tempC,*lbl_bldtbl_setpC,*lbl_chmbr_tempC,*lbl_chmbr_setpC;
GtkWidget	*stat_bldtbl,*stat_chm_lbl;

// temperature and material bar graphs for TOOL window status frame.
// would have liked to use same ones as in main, but child widgets can't have two parents... so we duplicate.
// note:  these will be created in main because we want the on_idle function to update them whilst the tool window is up.
GtkWidget	*Ttemp_lbl[MAX_THERMAL_DEVICES],*Tsetp_lbl[MAX_THERMAL_DEVICES],*Tmatl_lbl[MAX_THERMAL_DEVICES],*Tmlvl_lbl[MAX_THERMAL_DEVICES];
GtkWidget	*Ttemp_lvl[MAX_THERMAL_DEVICES],*Tmatl_lvl[MAX_THERMAL_DEVICES];
GtkLevelBar	*Ttemp_bar[MAX_THERMAL_DEVICES],*Tmatl_bar[MAX_THERMAL_DEVICES];
GtkAdjustment	*temp_override_adj, *matl_override_adj;
GtkWidget	*temp_override_btn, *matl_override_btn;

// public widgets for the TOOL window.
// they need to be updated on tool changes by routines outside of the window generation
GtkWidget	*lbl_power,*lbl_heater,*lbl_tsense,*lbl_stepper,*lbl_SPI,*lbl_1wire,*lbl_1wID,*lbl_limitsw,*lbl_weight;
GtkWidget	*lbl_tipdiam,*lbl_layerht,*lbl_temper;
GtkWidget	*lbl_tipx,*lbl_tipy,*lbl_tipzdn;
GtkWidget	*lbl_tipzup,*btn_tipset;

// tool information and calibration widgets
GtkAdjustment	*tool_serial_numadj;
GtkWidget	*tool_serial_numbtn;
GtkAdjustment	*temp_cal_sys_highadj;					// tool temperature high calibration adjustment
GtkWidget	*temp_cal_sys_highbtn;					// tool temperature high calibration button
GtkAdjustment	*duty_cal_sys_lowadj,*duty_cal_sys_highadj;		// tool power duty cycle low/high calibration adjustment
GtkWidget	*duty_cal_sys_lowbtn,*duty_cal_sys_highbtn;		// tool power duty cycle low/high calibration button
GtkWidget 	*tcal_vhi_lbl;						// voltage value for temp high point calibartion
int		device_being_calibrated=0;				// device to be calibrated
GtkWidget	*lbl_device_being_calibrated;				// label for device
GtkAdjustment	*temp_cal_device;					// spin box to select which device to calibrate
GtkWidget	*temp_cal_btn;						// temperature calibration button
GtkWidget	*temp_chbr_disp,*temp_btbl_disp,*temp_tool_disp;	// real time temp display for settings
GtkWidget 	*lbl_lo_values,*lbl_hi_values;				// labels to show hi/lo temperature calibration values
GtkAdjustment	*temp_cal_offset;					// spin box to set the temperature offset for a tool
GtkWidget	*temp_cal_offset_btn;					// temperature offset calibration button
GtkWidget 	*lbl_mat_description,*lbl_mat_brand;			// labels for materials in tool info window

// settings layer viewing
GtkAdjustment	*slc_view_start_adj, *slc_view_end_adj, *slc_view_inc_adj;
GtkWidget	*slc_view_start_btn, *slc_view_end_btn, *slc_view_inc_btn;

// settings model viewing
GtkWidget	*btn_pick_vertex, *btn_pick_facet, *btn_pick_patch, *btn_pick_neighbors;

// settings XYZ move adjustments
GtkAdjustment 	*xdistadj,*ydistadj,*zdistadj;

// settings for tool voltage tests
GtkAdjustment	*t48vadj,*t24vadj,*PWMvadj;
GtkWidget	*t48vbtn,*t24vbtn,*PWMvbtn;

// material color
GdkRGBA 	mcolor[4];						// note: NOT a pointer!

GtkAdjustment	*xadj,*yadj;
GtkWidget	*xbtn,*ybtn;

// operation selection and control widgets
GtkWidget	*btn_op[MAX_OP_TYPES][MAX_TOOLS];
GtkWidget	*btn_view_linetype[MAX_LINE_TYPES];
GtkWidget	*btn_view_model_lt_width,*btn_view_support_lt_width;

// build table scanning widgets
GtkAdjustment	*bld_tbl_res_adj;
GtkWidget	*bld_tbl_res_btn,*bld_tbl_res_lbl;

// fabrication widgets
GtkAdjustment	*zcontour_adj, *vtx_overlap_adj;
GtkWidget	*btn_z_tbl_comp,*zcontour_btn,*zcontour_lbl;
GtkWidget	*vtx_overlap_btn,*vtx_overlap_lbl;

// cnc widgets
GtkAdjustment	*cnc_block_x_adj,*cnc_block_y_adj,*cnc_block_z_adj;
GtkWidget	*cnc_block_x_btn,*lbl_cnc_block_x;			
GtkWidget	*cnc_block_y_btn,*lbl_cnc_block_y;			
GtkWidget	*cnc_block_z_btn,*lbl_cnc_block_z;			



// Functions located in 4X3D.c
void set_color(cairo_t *cr, GdkRGBA *st_color, int color);
int get_color(GdkRGBA *st_color);
void on_active_model_is_null(void);
int on_job_will_need_regen(void);
int Print_to_Model_Coords(vertex *vptr, int slot);
int Model_to_Print_Coords(vertex *vptr, int slot);

// Functions located in Model_UI.c
model *model_file_load(GtkWidget *btn_call, gpointer src_window);
int model_file_remove(GtkWidget *btn_call, gpointer user_data);
void set_operation_checkbutton(int slot, int op_typ);
void build_options_anytime_cb(GtkWidget *btn, gpointer src_window);
void build_options_preslice_cb(GtkWidget *btn, gpointer src_window);
int model_UI(GtkWidget *btn_call, gpointer user_data);
void update_size_values(model *mnew);

// Functions located in Model_Data.c
int memory_status(void);
vertex *vertex_make(void);
int vertex_insert(model *mptr, vertex *local, vertex *vertexnew, int typ);	// inserts a vertex into a model
vertex *vertex_unique_insert(model *mptr, vertex *local, vertex *vertexnew, int typ, float tol);
int vertex_delete(model *mptr, vertex *vdel);
int vertex_redundancy_check(vertex *vtxlist);
vertex *vertex_copy(vertex *vptr, vertex *vnxt);
int vertex_swap(vertex *A, vertex *B);
int vertex_purge(vertex *vlist);
int vertex_colinear_test(vertex *v1, vertex *v2, vertex *v3);
float vertex_2D_distance(vertex *vtxA, vertex *vtxB);
float vertex_distance(vertex *vtxA, vertex *vtxB);
int vertex_3D_interpolate(float f_dist,vertex *vtxA,vertex *vtxB, vertex *vresult);
int vertex_compare(vertex *A, vertex *B, float tol);
int vertex_xy_compare(vertex *A, vertex *B, float tol);
int vertex_cross_product(vertex *v1, vertex *v2, vertex *v3, vertex *vresult);
vertex *vertex_find_in_grid(polygon *pgrid, vertex *vcenter);
vertex *vertex_random_insert(vertex *vpre, vertex *vnew, double tolerance);
vertex *vertex_match_XYZ(vertex *vtest,polygon *ptest);
vertex *vertex_match_ID(vertex *vtest,polygon *pA);
vertex *vertex_previous(vertex *vtest,polygon *pA);
vertex_list *vertex_neighbor_list(vertex *vinpt);
vertex_list *vertex_list_make(vertex *vinpt);
vertex_list *vertex_list_manager(vertex_list *vlist, vertex *vinpt, int action);
vertex *vertex_list_add_unique(vertex_list *vl_inpt, vertex *vtx_inpt);

edge *edge_make(void);
edge *edge_copy(edge *einp);
int edge_insert(model *mptr, edge *local, edge *edgenew, int typ);
int edge_delete(model *mptr, edge *edel, int typ);
int edge_purge(edge *elist);
edge_list *edge_list_make(edge *einpt);
edge_list *edge_list_manager(edge_list *elist, edge *einpt, int action);
edge *edge_angle_id(edge *inp_edge_list, float inp_angle);		// creates edge list base on neighboring facet angles
int edge_compare(edge *A, edge *B);
int edge_display(model *mptr, float angle, int typ);
int edge_silhouette(model *mptr, float spin, float tilt);
float facet_angle(facet *Af, facet *Bf);
int edge_dump(model *mptr);

facet *facet_make(void);
int facet_insert(model *mptr, facet *local, facet *facetnew, int typ);
int facet_delete(model *mptr, facet *fdel, int typ);
int facet_purge(facet *flist);
int facet_normal(facet *finp);
int facet_flip_normal(facet *fptr);
int facet_centroid(facet *fptr,vertex *vctr);
float facet_area(facet *fptr);
int facet_find_all_neighbors(facet *flist);
facet *facet_find_vtx_neighbor(facet *fstart, facet *finpt, vertex *vtx0, vertex *vtx1);
int facet_contains_point(facet *finpt, vertex *vtest);
int facet_compare(facet *A, facet *B);
int vector_facet_intersect(facet *fptr, vector *vecptr, vertex *vint);
facet *facet_copy(facet *fptr);
facet *facet_subdivide(model *mptr, facet *fptr, float area);
int facet_unit_normal(facet *fptr);
int facet_swap_value(facet *fA, facet *fB);
int facet_share_vertex(facet *fA, facet *fB, vertex *vtx_test);
vector *facet_share_edge(facet *fA, facet *fB);
facet_list *facet_list_make(facet *finpt);
facet_list *facet_list_manager(facet_list *flist, facet *finpt, int action);

model *model_make(void);
int model_insert(model *modelnew);
int model_delete(model *mdl_del);
int model_dump(model *mptr);
int model_purge(model *mdl_del);
model *model_get_pointer(int mnum);
int model_get_slot(model *mptr, int op_id);				// gets tool to be used for operation called out by modelint model_dump(model *mptr);
int model_linetype_active(model *mptr, int lineType);
slice *model_raw_slice(model *mptr, int slot, int mtyp, int ptyp, float z_level);
int model_integrity_check(model *mptr);
int model_maxmin(model *mptr);						// establishes the max/min bounds and centers
int model_map_to_table(model *mptr);					// maps build table z offsets to model bounds
int model_mirror(model *mptr);						// mirrors models
int model_scale(model *mptr, int mytp);					// scales models 
int model_rotate(model *mptr);						// rotates models
int model_support_estimate(model *mptr);
int model_auto_orient(model *mptr);
int model_overlap(model *minp);
int model_out_of_bounds(model *mptr);
int model_convert_type(model *mptr,int target_type);
int model_build_target(model *mptr);
int model_profile_cut(model *mptr);
int model_grayscale_cut(model *mptr);
int model_build_internal_support(model *mptr);

patch *patch_make(void);
int patch_delete(patch *patdel);
patch *patch_copy(model *mptr, patch *pat_src, int target_mdl_typ);
int patch_flip_normals(patch *patptr);					// flips normals of all facets pointed to by patch
int patch_find_z(patch *patptr, vertex *vinpt);
int patch_contains_facet(patch *patptr, facet *fptr);
int patch_find_edge_facets(patch *patptr);
int patch_find_free_edge(patch *patptr);				// creates patch vtx list from existing patch facet list
int patch_vertex_on_edge(patch *patptr, vertex *vptr);

branch *branch_make(void);
int branch_delete(branch *brdel);
int branch_sort(branch *br_list);
branch_list *branch_list_make(branch *br_inpt);
branch_list *branch_list_manager(branch_list *br_list, branch *br_inpt, branch *br_loc, int action);

// Functions located in Job.c
int job_rules_check(void);						// verifies job set up
int job_slice(void);							// slices models of the job
int job_purge(void);							// clears slice data out of a job, but keeps model geometry
int job_complete(void);							// posts complete msg and deletes job contensts
int job_clear(void);							// clears job data
int job_maxmin(void);							// establishes the max/min bounds of all tools & models
float job_find_min_thickness(void);					// finds the thinnest slice thickness in use in the job
float job_time_estimate_calculator(void);
float job_time_estimate_simulator(void);
int job_map_to_table(void);						// maps build table z offsets to all tools & models
int job_base_layer(void);
int job_base_layer2(float zlvl);
model *job_layer_seek(int direction);
int job_build_grid_support(void);
int job_build_tree_support(void);

// Functions located in Vector.c
vector *vector_make(vertex *A, vertex *B, int typ);			// allocates memory for a single vector
int vector_insert(slice *sptr, int ptyp, vector *vecnew);		// inserts a single vector into linked list
int vector_raw_insert(slice *sptr, int ptyp, vector *vecnew);
vector *vector_delete(vector *veclist, vector *vdel);			// deletes a single vector from linked list
vector *vector_wipe(vector *vec_list, int del_typ);
vector *vector_copy(vector *vec_src);
int vector_list_dump(vector *vec_list);
int vector_compare(vector *A, vector *B, float tol);			// checks if two vectors are identical
int vector_purge(vector *vpurge);					// wipes the vector list out of memory
vector *vector_colinear_merge(vector *vec_first, int ptyp, float delta);// merges colinear vectors
vector_list *vector_sort(vector *vec_group, float tolerance, int mem_flag);	// sorts vector linked list tip to tail into polygons
int vector_crossings(vector *vec_list, int save_in_new);
int vector_crosses_polygon(slice *sinpt, vector *vecA, int ptyp);
int vector_subdivide(vector *vecinpt, vertex *vtxinpt);
int vector_bisector_get_direction(vector *vA, vector *vB, vertex *vnew);
int vector_bisector_set_length(vector *vBi, float newdist);
int vector_bisector_inc_length(vector *vBi, float incdist);
polygon *ss_veclist_2_single_polygon(vector *vec_list, int ptyp);
polygon *veclist_2_single_polygon(vector *vec_first, int ptyp);
int vector_winding_number(vector *vec_list, vector *vec_inpt);
vector *vector_find_neighbor(vector *veclist, vector *vecinpt, int endpt);
int vector_duplicate_delete(vector *veclist, float tolerance);
unsigned int vector_intersect(vector *vA, vector *vB, vertex *intvtx);	// finds the intersection point of two vectors
int vector_parallel(vector *vA, vector *vB);				// quick test to check if parallel
double vector_magnitude(vector *A);					// calculate the magnitude of a vector
double vector_dotproduct(vector *A, vector *B);				// calculates dot product of two vectors
float vector_absangle(vector *A);					// calculates the absolute angle of a 2D vector
float vector_relangle(vector *A, vector *B);				// finds the angle between two vectors in 3D space
int vector_unit_normal(vector *vecin, vertex *vtx_norm);		// calculates unit normal of a vector
int vector_dump(vector *vptr);						// dumps vector list to screen
int vector_crossproduct(vector *A, vector *B, vertex *vptr);		// calculates cross product of two vectors
int vector_rotate(vector *vinpt, float theta);				// rotates a vector about its tail vertex
int vertex_crossproduct(vertex *A, vertex *B, vertex *vptr);
double vertex_dotproduct(vertex *A, vertex *B);
int vector_tip_tail_swap(vector *vinpt);
vector_list *vector_list_make(vector *vinpt);
vector_list *vector_list_manager(vector_list *vlist, vector *vinpt, int action);
int vector_list_check(vector_list *vlist);

// Functions located in Polygon.c
// Functions located in Polygon.c
polygon *polygon_make(vertex *vtx, int typ, unsigned int memb);		// allocated memory for a polygon element
int polygon_insert(slice *sptr, int type, polygon *plynew);		// inserts polygon element into list of polygon vertices
int polygon_delete(slice *sptr, polygon *pdel);				// deletes a polygon from the slice's linked list of polygons
int polygon_free(polygon *pdel);					// deletes a stand alone polygon from memory
polygon *polygon_purge(polygon *pptr, float odist);			// wipes the entire polygon list from memory
polygon *polygon_copy(polygon *psrc);
int polygon_build_sublist(slice *ssrc, int mtyp);			// builds list of polygons enclosed by this polygon
polygon_list *polygon_list_make(polygon *pinpt);
polygon_list *polygon_list_manager(polygon_list *plist, polygon *pinpt, int action);
int polygon_dump(polygon *pptr);					// dumps the polygon list to the screen
float polygon_perim_length(polygon *pinpt);
int polygon_contains_point(polygon *pptr, vertex *vtest);		// test if a point (vertex) lies within a polygon
float polygon_to_vertex_distance(polygon *ptest, vertex *vtest, vertex *vrtn);	// finds min dist bt polygon and test vtx
int polygon_find_start(polygon *pinpt, slice *sprev);			// finds the best place to initiate perimeter deposition
int polygon_find_center(polygon *pinpt);				// finds the center of a polygon
float polygon_find_area(polygon *pinpt);				// finds the area of a polygon
slice *polygon_get_slice(polygon *pinpt);				// finds the slice to which the polygon belongs
int polygon_swap_contents(polygon *pA, polygon *pB);			// swaps the contents of two polygons
int polygon_colinear_merge(slice *sptr, polygon *pptr, double delta);	// merges colinear vectors
int polygon_subdivide(polygon *pinpt, float interval);			// subdivides line segments around polygon at interval
int polygon_contains_material(slice *sptr, polygon *pinpt, int ptyp);	// determines if poly is a hole or encloses material
int polygon_make_offset(slice *sinpt, polygon *pinpt);			// generates custom offsets for this polygon
extern int polygon_wipe_offset(slice *sinpt, polygon *pinpt, int ptyp, float odist); // wipes out custom offsets for this polygon
int polygon_make_fill(slice *sinpt, polygon *pinpt);			// generates fill pattern of specific type for this polygon
int polygon_wipe_fill(slice *sinpt, polygon *pinpt);			// wipes out fill pattern of a specific type for this polygon
int polygon_reverse(polygon *pptr);					// reverses the vertex list
int polygon_sort(slice *sinpt, int typ);				// sorts polygons to minimize pen up movements
int polygon_sort_perims(slice *sinpt, int line_typ, int sort_typ);
vector *polylist_2_veclist(polygon *pinpt, int ptyp);
int polygon_verify(polygon *pinpt);					// verifies contents of polygon
int polygon_selfintersect_check(polygon *pA);
int polygon_overlap(polygon *pA, polygon *pB);
int polygon_intersect(polygon *pA, polygon *pB, double tolerance);
polygon *polygon_boolean2(slice *sinpt, polygon *pA, polygon *pB, int action);
int polygon_edge_detection(slice *sptr, int ptyp);

// Functions located in Slice.c
slice *slice_make(void);
int slice_insert(model *mptr, int slot, slice *local, slice *slicenew);
int slice_purge(model *mptr, int slot);
slice *slice_copy(slice *ssrc);
int slice_delete(model *mptr, int slot, slice *sdel);			// deletes a single slice from the linked list for a model
void slice_dump(slice *sptr);						// dumps slice information to consol
void slice_scale(slice *ssrc, float scale);				// scales a slice in the xy
slice *slice_find(model *mptr, int mytp, int slot, float zlvl, float delta);
model *slice_get_model(slice *sinp);					// finds ptr to model that contains this slice
int slice_polygon_count(slice *sptr);					// recounts the number of polygons in the slice
polygon *slice_find_outermost(polygon *pinpt);				// finds outer most polygon in group of polygons
int slice_maxmin(slice *sptr);
int slice_rotate(slice *sptr, float rot_angle, int typ);		// rotates a slice
int slice_set_attr(int slot, float zlvl, int newattr);
int slice_point_in_material(slice *sptr, vertex *vtest, int ptyp, float odist);
float slice_point_near_material(slice *sinpt, vertex *vinpt, int ptyp);
slice *slice_model_bounds(model *mptr,int mtyp, int ptyp);
int slice_honeycomb(slice *str_sptr, slice *end_sptr, float cell_size, int Ltyp, int Ftyp);
int slice_linefill(slice *blw_sptr,slice *crt_sptr,slice *abv_sptr,float scanstep,float scanangle,int SLtyp,int ConTyp,int FillTyp,int BoolAction);
slice *slice_boolean(model *mptr, slice *sA, slice *sB, int ltypA, int ltypB, int ltyp_save, int action);
int vertex_in_facet(vertex *vtest, facet *finpt);
int slice_fill_model(model *mptr, float zlvl);
int slice_fill_all(float zlvl);
int slice_deck(model *mptr, int mtyp, int oper_ID);
int slice_wipe(slice *sptr, int ptyp, float odist);
int slice_skeleton(slice *sinpt, int source_typ, int ptyp, float odist);
int slice_offset_skeleton(slice *sinpt, int source_typ, int ptyp, float odist);
int slice_offset_winding(slice *sinpt, int source_typ, int ptyp, float odist);
int slice_offset_winding_by_polygon(slice *sinpt, int source_typ, int ptyp, float odist);
vector_cell *vector_cell_make(void);
vector_cell *vector_cell_lookup(slice *sinpt, int x, int y);
int vector_grid_destroy(slice *sinpt);
vector *vector_add_to_list(vector *veclist, vector *vecadd);
vector *vector_del_from_list(vector *veclist, vector *vecdel);
vector *vector_search(vector *veclist, vector *vecinp);
vector *slice_to_veclist(slice *sinpt, int ptyp);

// Functions located in SENSOR.c
int Open1Wire(int slot);
int MemDevDataValidate(int slot);
int MemDevRead(int slot);
int MemDevWrite(int slot);
void mk_writable(char* path);
float distance_sensor(int slot, int channel, int readings, int typ);
int build_table_line_laser_scan(void);
int build_table_scan_index(int typ);
int build_table_step_scan(int slot, int sensor_type);
int build_table_sweep_scan(int slot, int sensor_type);
int build_table_centerline_scan(int slot, int sensor_type);
float touch_probe(vertex *vptr, int slot);
float z_table_offset(float x, float y);
//int get_distance(vl6180 handle);
//void set_scaling(vl6180 handle, int scaling);

// Functions located in SETTINGS.c
static void btn_endpts_toggled_cb(GtkWidget *btn, gpointer user_data);
static void btn_vecnum_toggled_cb(GtkWidget *btn, gpointer user_data);
static void btn_strskl_toggled_cb(GtkWidget *btn, gpointer user_data);
void grab_temp_cal_offset(GtkSpinButton *button, gpointer user_data);
static void on_settings_exit (GtkWidget *btn, gpointer dead_window);
int settings_UI(GtkWidget *btn_call, gpointer user_data);
//int on_test(GtkWidget *btn_call, gpointer user_data);

// Functions located in STL.c
int stl_model_load(model *mptr);					// loads STL from disk into structured memory
int stl_raw_load(model *mptr);

// Functions located in Image.c
int scan_build_volume(void);
int image_loader(model *mptr, int xpix, int ypix, int perserv_aspect);
int image_processor(GdkPixbuf *g_img_src1_buff, int img_proc_type);
int laser_scan_images_to_STL(void);
int laser_scan_matrix(void);
int image_to_STL(model *mptr);

// Functions located in Gerber.c
gbr_aperature *gbr_ap_make(slice *sptr);
int gerber_ap_insert(slice *sptr, gbr_aperature *ap_new);
int gerber_ap_purge(slice *sptr);
int gerber_ap_dump(slice *sptr);
int gerber_model_load(model * mptr);
int gerber_aperature_parse(slice *sptr, char *in_str, gbr_aperature *aptr);
int gerber_XYD_parse(char *in_str, vertex *vptr);
int gerber_flash_polygon(slice *sptr, gbr_aperature *aptr, float x, float y);
int gerber_trace_polygon(slice *sptr, gbr_aperature *cap, vertex *trace);
int gerber_ap_trace_merge(model *mptr,slice *sptr);

// Functions located in Print.c
int tool_matl_forward(int slot, float forward_amt);
int tool_matl_retract(int slot, float retract_amt);
int tool_tip_home(int slot);
int tool_tip_retract(int slot);
int tool_tip_deposit(int slot);
int tool_tip_touch(float delta_z);
int tool_air(int slot, int status);
int motion_complete(void);
void motion_done(void);
int print_vertex(vertex *vptr, int slot, int pmode, linetype *lptr);	// moves/prints from current location to vertex location
int temperature_adjust(int slot, float newTemp, int wait4it);
int step_aside(int slot, int wait_duration);
int print_status(jobx *local_job);
int tool_PWM_set(int slot, int port, int new_duty);
int tool_PWM_check(int slot, int port);
int tool_power_ramp(int slot, int port, int new_duty);
void set_holding_torque(int status);
int goto_machine_home(void);
int comefrom_machine_home(int slot, vertex *vtarget);
extern void* build_job(void *arg);					// builds a job in a separate thread

// Functions located in System_utils.c
int RPi_GPIO_Read(int pinID);						// performs multi GPIO reads
int load_system_config(void);						// load and process config.json file
int unit_calibration(void);						// run a unit level calibration
int save_unit_calibration(void);					// save unit calibration data to file
int ztable_calibration(void);						// table leveling function
int save_unit_state(void);						// save current operational state of unit
int load_unit_state(void);						// load current operational state of unit
int open_system_log(void);						// open system log file
int close_system_log(void);						// close system log file
int entry_system_log(char *in_data);					// add entry to system log file
int open_model_log(void);						// open model log file
int close_model_log(void);						// close model log file
int open_job_log(void);							// open job log files
int close_job_log(void);						// close job log files
int entry_process_log(char *in_data);					// add entry to process log file
int tinyGSnd(char *cmd);						// sends commands to tinyg controller
int tinyGRcv(int disp_val);						// receives status from tinyg controller
void delay(unsigned int msecs);
int kbhit(void);
void dialog_okay_cb(GtkWidget *widget, gpointer dialog);
void dialog_cncl_cb(GtkWidget *widget, gpointer dialog);
void aa_dialog_box(GtkWidget *parent, int type, int display_dur, char *title, char *msg);
int pca9685Setup(const int pinBase, const int i2cAddress, float freq);
void pca9685PWMFreq(int fd, float freq);
void pca9685PWMReset(int fd);
void pca9685PWMWrite(int fd, int pin, int on, int off);
void pca9685PWMRead(int fd, int pin, int *on, int *off);
int baseReg(int pin);

// Functions located in Initialize.c
int init_system(void);
int init_device_RPi(void);
int init_device_tinyG(void);
int init_device_PWM(void);
int init_device_I2C(void);
int init_device_SPI(void);
int init_device_1Wire(void);
int init_general(void);

// Functions located in Thermal.c
extern void* ThermalControl(void *arg);					// manages thermal devices in a separate thread
extern float read_RTD_sensor(int channel, int readings, int typ);	// reads temperature sensors (RTDs)
extern float read_THERMISTOR_sensor(int channel, int readings, int typ);// reads temperature sensors (thermistors)

// Functions located in Tool.c
static void on_tool_exit (GtkWidget *btn, gpointer dead_window);
int on_tool_change (GtkWidget *btn, gpointer user_data);
int tool_UI(GtkWidget *btn_call, gpointer user_data);
void xyztip_autocal_callback (GtkWidget *btn, gpointer user_data);
int tool_information_read(int slot, char *tool_file_name);
int tool_information_write(int slot);
int dump_tool_params(int slot);
int on_tool_settings(GtkWidget *btn_call, gpointer user_data);
void on_info_ok (GtkWidget *btn, gpointer dead_window);
int tool_type(GtkWidget *cmb_box, gpointer user_data);
int mats_type(GtkWidget *cmb_box, gpointer user_data);
int tool_add_matl(GtkWidget *btn, gpointer user_data);
int tool_sub_matl(GtkWidget *btn, gpointer user_data);
int tool_hotpurge_matl(GtkWidget *btn, gpointer user_data);
int load_tool_defaults(int slot);					// loads tool with default values
int unload_tool(int slot);						// clears variables for tool data when tool removed from unit
int make_toolchange(int ToolID, int StepID);				// selects which print cartridge to use
int print_toolchange(int newtool);					// sends a tool change command
int build_mat_file(GtkWidget *btn_call, gpointer user_data);

// Functions located in Vulkan.c
void create_instance();
void create_device();
void create_command_pool();
void create_render_pass();
void record_commands();

// Functions located in XML.c
genericlist *genericlist_make(void);
int genericlist_insert(genericlist *existing_list, genericlist *new_element);
int genericlist_delete_all(genericlist *existing_list);
genericlist *genericlist_delete(genericlist *existing_list, genericlist *gdel);
int genericlist_find(genericlist *existing_list, char *find_name);
linetype *linetype_make(void);
int linetype_insert(int slot, linetype *ltnew);
int linetype_delete(int slot);
linetype *linetype_copy(linetype *lt_inpt);
linetype *linetype_find(int slot, int ltdef);
float linetype_accrue_distance(slice *sptr, int ptyp);
operation *operation_make(void);
int operation_insert(int slot, operation *onew);
int operation_delete(int slot, operation *odel);
int operation_delete_all(int slot);
operation *operation_find_by_name(int slot, char *op_name);
operation *operation_find_by_ID(int slot, int op_ID);
int build_tool_index(void);						// reads tool database file and builds list of whats found
int XMLRead_Tool(int slot, char *tool_name);				// loads tool data from XML file
int build_material_index(int slot);					// reads material database file and builds list of whats found
int XMLRead_Matl_find_Tool(char *find_tool, char *find_matl);		// checks if matl is use-able by tool
int XMLRead_Matl(int slot,char *find_matl);				// loads material data from XML file


//-------------------------------------------------------------------------------------------------------------------------

 
// Callback to close down the application
static void destroy( GtkWidget *widget, gpointer user_data )
{
    int		i,slot;
    polygon	*pptr;
    slice	*sptr;
    model	*mptr,*mold;

    printf("\nShutting down...\n");
    sprintf(scratch,"System shut down.\n");
    entry_system_log(scratch);
    memory_status();

    // only execute this part of shut down if it initialized
    if(gpio_ready_flag==TRUE)
      {
      // send motion abort to tinyG
      printf("\n  ... clearing motion controller.\n");
      tinyGSnd("!\n");							// abort all moves
      tinyGSnd("%\n");							// clear entire buffer
      while(tinyGRcv(1)==0)
  
      // turn off holding torque to reduce stress on tinyG card when unit not in operation
      //if(unit_in_error==FALSE)						// if unit is in error, tinyg may not respond which hangs system so skip talking to it
      //  {
	printf("\n  ... turning off holding torque.\n");
	tinyGSnd("{1pm:0}\n");						// X: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
	delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
	while(cmdCount>0)tinyGRcv(1);
	tinyGSnd("{2pm:0}\n");						// Y: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
	delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
	while(cmdCount>0)tinyGRcv(1);
	tinyGSnd("{3pm:0}\n");						// Z: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
	delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
	while(cmdCount>0)tinyGRcv(1);
	tinyGSnd("{4pm:0}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
	delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
	while(cmdCount>0)tinyGRcv(1);
      //  }
  
      // shut off tools
      printf("\n  ... shutting off tools.\n");
      cmdControl=0;
      make_toolchange(-1,0);
      
      // turn off chamber lighting
      gpioWrite(CHAMBER_LED,!OFF);
  
      // turn off carriage lighting
      gpioWrite(CARRIAGE_LED,!OFF);
  
      // set all PWM outputs to 0
      pca9685PWMWrite(I2C_PWM_fd,PIN_BASE+0,0,0);
      delay(200);								// wait for all to shut of eligantly
  
      // DEBUG - turn WD off to ensure it shuts off PWM in this state
      gpioWrite(WATCH_DOG,0);
      
      // record any state information to state file
      printf("\n  ... saving unit state.\n");
      save_unit_state();
   
      // remove any model data from memory
      printf("\n  ... clearing models from memory.\n");
      mptr=job.model_first;
      while(mptr!=NULL)
	{
	mold=mptr;
	mptr=mptr->next;
	model_delete(mold);
	}
	
      // display memory status to consol to check for memory leaks
      //memory_status();
      }

    memory_status();

    // kill the print control thread
    pthread_cancel(build_job_tid);					// destroy thread
    printf("\n  ... print thread shut down. \n");

    // kill the thermal control thread
    pthread_mutex_destroy(&thermal_lock);
    pthread_cancel(thermal_tid);					// destroy thread
    printf("\n  ... thermal thread shut down. \n");
    
    printf("\n  ... good bye!\n");
    gtk_window_close(GTK_WINDOW(win_main));
    exit(0);

}

// Callback to change the job state from play/pause/abort button
// not used as a generic state changer.  this is typically done in the idle loop.
static void change_job_state( GtkWidget *btn, gpointer user_data )
{
  int		i,slot,next_slot;
  int		tool_use_count,clear_list_flag;
  model		*mptr;
  genericlist	*aptr;

  printf("\nJob State: inbound=%d ",job.state);

  job.prev_state=job.state;						// save in-bound state for future reference
  job.sync=TRUE;							// set flag to indicate user made a job state change
  if(job.state==JOB_READY)						// job is ready... this is first switch to running state
    {
    // autocalibrate tool tips if more than one tool used in this job
    if(HAS_AUTO_TIP_CALIB==TRUE)
      {
      //slot=MAX_TOOLS;
      //if(tool_use_count>1)xyztip_autocal_callback(NULL,GINT_TO_POINTER(slot));
      }
    
    // if the polygons exist on the pick list, check with user what to do about them
    clear_list_flag=FALSE;
    if(p_pick_list!=NULL)
      {
      sprintf(scratch," The polygon pick list is not empty. \n  Empty the list? ");
      aa_dialog_box(win_main,3,0,"Run Job",scratch);
      while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
      clear_list_flag=aa_dialog_result;
      if(clear_list_flag==TRUE)p_pick_list=polygon_list_manager(p_pick_list,NULL,ACTION_CLEAR); // clear poly pick list before start
      }

    job.start_time=time(NULL);
    sprintf(scratch,"Job print initiated.");
    entry_system_log(scratch);
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      sprintf(scratch,"%d copies of %s ",mptr->total_copies,mptr->model_file);
      entry_system_log(scratch);
      mptr=mptr->next;
      }
    job.state=JOB_RUNNING;
    printf("outbound=%d \n",job.state);
    return;
    }
  if(job.state==JOB_RUNNING)						// job is running... 
    {
    job.state=JOB_PAUSED_BY_USER;					// ... set the state to paused
    printf("outbound=%d \n",job.state);
    return;
    }
  if(job.state==JOB_PAUSED_BY_USER)					// job is paused by the user...
    {
    job.state=JOB_RUNNING;						// ... set the state to run
    printf("outbound=%d \n",job.state);
    return;
    }
  if(job.state==JOB_PAUSED_BY_CMD)					// job is paused at predefined point...
    {
    job.state=JOB_RUNNING;						// ... set the state to run
    printf("outbound=%d \n",job.state);
    return;
    }
  if(job.state==JOB_PAUSED_FOR_BIT_CHG)					// job is paused for bit change...
    {
    job.state=JOB_RUNNING;						// ... set the state to run
    return;
    }
  if(job.state==JOB_PAUSED_FOR_TOOL_CHG)
    {
    job.state=JOB_RUNNING;
    return;
    }
  if(job.state==JOB_PAUSED_DUE_TO_ERROR)
    {
    job.state=JOB_RUNNING;
    return;
    }
  if(job.state==JOB_TABLE_SCAN_PAUSED)
    {
    job.state=JOB_TABLE_SCAN;
    return;
    }
  if(job.state==JOB_CALIBRATING)
    {
    job.state=JOB_COMPLETE;						// idle loop will switch to READY if all else is good
    return;
    }
  if(job.state==JOB_COMPLETE)
    {
    job.state=JOB_NOT_READY;						// idle loop will switch to READY if all else is good
    return;
    }
  if(job.state==JOB_HEATING)	
    {
    // acknowledge play button press by toggling good_to_go to state, then
    // wait for idle loop to detect temps at operational at which point
    // it will switch state to job_ready so just return.
    good_to_go =! good_to_go;
    return;
    }
  if(job.state==JOB_SLEEPING)	
    {
    job.state=job.prev_state;
    return;
    }
  if(job.state==JOB_ABORTED_BY_USER)
    {
    job.state=JOB_READY;	
    return;
    }
  if(job.state==JOB_ABORTED_BY_SYSTEM)					// unknown error encountered... run diagnostics
    {

    printf("\nSystem error detected!\n");

    printf("Checking status of thermal thread...");
    thermal_thread_alive=0;
    delay(500);
    if(thermal_thread_alive==0 && init_done_flag==TRUE)
      {
      printf("not responding!\n");
      
      // RE-init PWM contorller
      pca9685PWMReset(I2C_PWM_fd);					// reset to default values (know state)
      delay(250);							// give time for reset to process
      pca9685PWMFreq(I2C_PWM_fd,500);					// fix freq for all channels at 500 hz
      delay(250);							// give time for reset to process
      if(current_tool>=0 && current_tool<MAX_THERMAL_DEVICES)
	{
	pca9685PWMWrite(I2C_PWM_fd,1,0,Tool[current_tool].thrm.heat_duration);	// send to PWM for TEST PIN
	}
      }
    if(thermal_thread_alive==1)printf("seems okay!\n");

    printf("Checking status of print engine thread...");
    print_thread_alive=0;
    delay(500);
    if(print_thread_alive==0)printf("not responding!\n");
    if(print_thread_alive==1)printf("seems okay!\n");

    job.state=JOB_PAUSED_DUE_TO_ERROR;
    return;
    }

  return;
}

// Callback to abort job button
// note this reacts within the time scope of the idle loop which is ~1 second
static void abort_job( GtkWidget *btn, gpointer user_data )
{
  int			clear_models_flag=FALSE;
  GtkAlertDialog	*win_dialog;
  model			*mptr,*mold;

  // reset since clearly no longer idle
  idle_start_time=time(NULL);						// reset idle time

  // now provide response depending on the job state upon entry
  job.prev_state=job.state;						// save incoming state
  job.state=JOB_ABORTED_BY_USER;					// set to define this action 
  clear_models_flag=FALSE;						// set default to bypass clearing code
  if(job.prev_state<JOB_RUNNING) 
    {
    sprintf(scratch,"All loaded models will be cleared.\n");
    strcat(scratch,"Press NO to cancel, YES to clear.\n");
    aa_dialog_box(win_main,3,0,"Abort Job",scratch);			// display dialog to user, await response
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    clear_models_flag=aa_dialog_result;
    job.state=JOB_NOT_READY;
    }
  if(job.prev_state==JOB_RUNNING)
    {
    tinyGSnd("!\n");							// abort all moves
    tinyGSnd("%\n");							// clear entire buffer
    while(tinyGRcv(1)>=0);
    job.state=JOB_PAUSED_BY_USER;					// force pause
    }
  if(job.prev_state>=JOB_PAUSED_BY_USER && job.prev_state<JOB_TABLE_SCAN)
    {
    sprintf(scratch,"The current job will be aborted,\n");
    strcat(scratch,"but not cleared allowing a restart.\n");
    strcat(scratch,"Press NO to cancel, YES to abort.\n");
    aa_dialog_box(win_main,3,0,"Abort Job",scratch);			// display dialog to user, await response
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==TRUE){job.state=JOB_ABORTED_BY_USER;}		// either force print to loop close out...
    else {job.state==JOB_PAUSED_BY_USER;}				// ... or stay in paused state
    }
  if(job.prev_state==JOB_TABLE_SCAN || job.prev_state==JOB_TABLE_SCAN_PAUSED)
    {
    sprintf(scratch,"The table scan will be aborted.\n");
    strcat(scratch,"Press NO to cancel, YES to abort.\n");
    aa_dialog_box(win_main,3,0,"Abort Job",scratch);			// display dialog to user, await response
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==TRUE){job.state=JOB_ABORTED_BY_USER;}
    else {job.state==JOB_PAUSED_BY_USER;}			
    }
  if(job.prev_state==JOB_COMPLETE)
    {
    sprintf(scratch,"The current job and any loaded\n");
    strcat(scratch,"models will be cleared.\n");
    strcat(scratch,"Press NO to cancel, YES to clear.\n");
    aa_dialog_box(win_main,3,0,"Abort Job",scratch);			// display dialog to user, await response
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==TRUE){job.state=JOB_NOT_READY; clear_models_flag=TRUE;}
    else {job.state==JOB_READY;}
    }
  if(job.prev_state>=JOB_SLEEPING)
    {
    sprintf(scratch,"The current job and any loaded\n");
    strcat(scratch,"models will be cleared.\n");
    strcat(scratch,"Press NO to cancel, YES to clear.\n");
    aa_dialog_box(win_main,3,0,"Abort Job",scratch);			// display dialog to user, await response
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    clear_models_flag=aa_dialog_result;
    if(aa_dialog_result==TRUE){job.state=JOB_NOT_READY;}
    else {job.state==JOB_READY;}
    }

  printf("\nclear_models_flag=%d \n",clear_models_flag);
  
  // for all other job states and if clearing model files...
  if(clear_models_flag==TRUE)
    {
    sprintf(scratch,"Job aborted by user.");
    entry_system_log(scratch);
    // remove any model data from memory
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      mold=mptr;
      mptr=mptr->next;
      model_delete(mold);
      }
    job_clear();
    }
  
  return;

}

// Function to convert print coords back to model coords
// This function will take in the print coords of vptr and modify them to model coords.
int Print_to_Model_Coords(vertex *vptr, int slot)
{
  // validate inputs
  if(vptr==NULL)return(FALSE);
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
  
  // subtract off offsets that move table home (0,0,0 on build table) to unit home 0,0,0 (unit home/park position)
  vptr->x=PostG.x-xoffset;
  vptr->y=PostG.y-yoffset;
  vptr->z=PostG.z-zoffset;

  // subtract off carriage offsets
  // since carriage slots are fixed, they have +/- offset values that get applied generically
  vptr->x -= crgslot[slot].x_center;
  vptr->y -= crgslot[slot].y_center;
    
  // subtract off default tool offsets from TOOLS.XML
  // since a tool can be mounted in any location on the carriage, we need to adjust the +/- offset
  // values depending on where it is mounted.
  if(slot==0 || slot==2)vptr->x -= Tool[slot].x_offset;			// tool mounted farther from x home
  if(slot==1 || slot==3)vptr->x += Tool[slot].x_offset;			// tool mounted nearer to x home
  if(slot==0 || slot==2)vptr->y += Tool[slot].y_offset;
  if(slot==1 || slot==3)vptr->y -= Tool[slot].y_offset;			// account for tool being flipped 180 degrees on carriage

  // subtract off fabrication tool offsets from calibration - these use the switch position as reference
  // since a tool can be mounted in any location on the carriage, we need to adjust the +/- offset
  // values depending on where it is mounted.
  if(slot==0 || slot==2)vptr->x -= Tool[slot].mmry.md.tip_pos_X;	// tool mounted farther from x home
  if(slot==1 || slot==3)vptr->x += Tool[slot].mmry.md.tip_pos_X;	// tool mounted nearer to x home
  if(slot==0 || slot==2)vptr->y += Tool[slot].mmry.md.tip_pos_Y;
  if(slot==1 || slot==3)vptr->y -= Tool[slot].mmry.md.tip_pos_Y;	// account for tool being flipped 180 degrees on carriage
  //PosIs.z+=Tool[slot].mmry.md.tip_pos_Z;				// z is handled by tip deposit function

  return(TRUE);
}

// Function to convert model coords to print coords
// This function will take in the model coords of vptr and modify them to print coords.
int Model_to_Print_Coords(vertex *vptr, int slot)
{
  // validate inputs
  if(vptr==NULL)return(FALSE);
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
  
  // add in offsets that move unit home (unit home/park position) to table home (0,0,0 on build table) 
  vptr->x += xoffset;
  vptr->y += yoffset;
  vptr->z += zoffset;

  // add in carriage offsets
  // since carriage slots are fixed, they have +/- offset values that get applied generically
  vptr->x += crgslot[slot].x_center;
  vptr->y += crgslot[slot].y_center;
    
  // add in default tool offsets from TOOLS.XML
  // since a tool can be mounted in any location on the carriage, we need to adjust the +/- offset
  // values depending on where it is mounted.
  if(slot==0 || slot==2)vptr->x += Tool[slot].x_offset;			// tool mounted farther from x home
  if(slot==1 || slot==3)vptr->x -= Tool[slot].x_offset;			// tool mounted nearer to x home
  if(slot==0 || slot==2)vptr->y -= Tool[slot].y_offset;
  if(slot==1 || slot==3)vptr->y += Tool[slot].y_offset;			// account for tool being flipped 180 degrees on carriage

  // add in fabrication tool offsets from calibration - these use the switch position as reference
  // since a tool can be mounted in any location on the carriage, we need to adjust the +/- offset
  // values depending on where it is mounted.
  if(slot==0 || slot==2)vptr->x += Tool[slot].mmry.md.tip_pos_X;	// tool mounted farther from x home
  if(slot==1 || slot==3)vptr->x -= Tool[slot].mmry.md.tip_pos_X;	// tool mounted nearer to x home
  if(slot==0 || slot==2)vptr->y -= Tool[slot].mmry.md.tip_pos_Y;
  if(slot==1 || slot==3)vptr->y += Tool[slot].mmry.md.tip_pos_Y;	// account for tool being flipped 180 degrees on carriage
  //PosIs.z -= Tool[slot].mmry.md.tip_pos_Z;				// z is handled by tip deposit function

  return(TRUE);
}



// functions to draw and manage SLICE VIEW to the display
gboolean layer_draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  int		mtyp;
  int		h,i,j,pcnt,slot,op_typ,ptyp,total_copy_ctr;
  int		vec_ctr,x_copy_ctr,y_copy_ctr;
  int		x_cell_id,y_cell_id;
  int		view_copy=TRUE,skip_vtx;
  int		model_slot,support_slot,ms_same_slot;
  int		view_poly_ids,view_poly_fam,view_poly_perim,view_poly_area,view_poly_dist;
  char 		img_name[255];
  long int   	isum=0, fct_total, edg_total, vtx_total, tcount=0;
  float 	newx,newy,oldx,oldy,copy_xoff,copy_yoff;
  float 	prex,prey;
  float 	dx,dy,dz;
  float 	xb[8],yb[8];
  float 	hl,hh,vl,vh;
  int 		xstr,ystr,xmax,ymax,bld_x,bld_y;
  float 	xstart,ystart,tbl_inc;
  float 	bld_z_min,distxy;
  float 	vec_angle,arrow_angle,arrow_length,dot,det;
  float 	x0,y0,x1,y1,x2,y2,x3,y3,x4,y4;
  float 	slc_z_cut,delta;
  float 	z_view_start,z_view_end;
  vertex	*vptr,*vnxt,*vdel,*vpos;
  vector	*vecptr;
  vertex	*vtest,*v1,*v2;
  polygon	*pptr;
  slice		*sptr;
  model		*mptr;
  genericlist	*amptr,*ltptr;
  linetype 	*linetype_ptr;
  polygon_list	*pl_ptr;
  vector_list	*vl_ptr;
  operation	*optr;
  GdkRGBA 	color;
  guchar 	*pix1;
  int		xpix,ypix;
  int		pix_inc;

  // init
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr,1.0);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
  vpos=vertex_make();
      
  // if combo view of image and 2D slice is requested and camera above build table...
  if(Superimpose_flag==TRUE && CAMERA_POSITION==1)
    {
    // set camera zoom to nominal
    cam_roi_x0=0.0; cam_roi_y0=0.0;
    cam_roi_x1=1.5; cam_roi_y1=0.980;
      
    // load the new image as our pixel background
    if(g_camera_buff!=NULL){g_object_unref(g_camera_buff); g_camera_buff=NULL;}		// clear old buffer if it exists
    sprintf(img_name,"/home/aa/Documents/4X3D/still-test.jpg");				// load last snap shot
    g_camera_buff = gdk_pixbuf_new_from_file_at_scale(img_name, 830,574, FALSE, NULL);	// load new buffer with image
    if(g_camera_buff!=NULL)
      {
      gdk_cairo_set_source_pixbuf(cr, g_camera_buff, 0, 0);
      cairo_paint(cr);
      }
    //if(g_camera_buff!=NULL)g_object_unref(g_camera_buff);
    
    // force the viewing angle to match the camera's view of the table
    LVview_scale=1.435;	
    LVdisp_cen_x=(BUILD_TABLE_LEN_X/2);
    LVdisp_cen_y=0-(BUILD_TABLE_LEN_Y/2);
    LVdisp_cen_z=0.0;
    LVgxoffset=45;
    LVgyoffset=(-695);
    
    // init for drawing
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    }
  else
    {
    // draw build table outline
    set_color(cr,&color,MD_GRAY);
    color.alpha=0.30;gdk_cairo_set_source_rgba (cr,&color);
    cairo_move_to(cr,LVgxoffset,0-LVgyoffset);								// move to 0,0
    cairo_line_to(cr,LVgxoffset+LVview_scale*BUILD_TABLE_LEN_X,0-LVgyoffset);					// draw to x max, 0
    cairo_line_to(cr,LVgxoffset+LVview_scale*BUILD_TABLE_LEN_X,0-(LVgyoffset+LVview_scale*BUILD_TABLE_LEN_Y));	// draw to x max, y max
    cairo_line_to(cr,LVgxoffset,0-(LVgyoffset+LVview_scale*BUILD_TABLE_LEN_Y));					// draw to 0, y max
    cairo_line_to(cr,LVgxoffset,0-LVgyoffset);								// draw to 0,0
    cairo_stroke(cr);
    
    // draw build table grid
    set_color(cr,&color,SL_GRAY);
    tbl_inc=10;
    x1=(BUILD_TABLE_LEN_X-(int)(BUILD_TABLE_LEN_X/tbl_inc)*tbl_inc)/2;
    y1=0;y2=BUILD_TABLE_LEN_Y;
    while(x1<=BUILD_TABLE_LEN_X)
      {
      x2=x1;
      cairo_move_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y1));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x2,0-(LVgyoffset+LVview_scale*y2));
      x1+=tbl_inc;
      }
    x1=0;x2=BUILD_TABLE_LEN_X;
    y1=(BUILD_TABLE_LEN_Y-(int)(BUILD_TABLE_LEN_Y/tbl_inc)*tbl_inc)/2;
    while(y1<=BUILD_TABLE_LEN_Y)
      {
      y2=y1;
      cairo_move_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y1));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x2,0-(LVgyoffset+LVview_scale*y2));
      y1+=tbl_inc;
      }
    cairo_stroke(cr);
    }
  
  // draw center lines on table grid
  cairo_set_line_width (cr,0.5);
  set_color(cr,&color,MD_RED);
  x1=(BUILD_TABLE_LEN_X-(int)(BUILD_TABLE_LEN_X/tbl_inc)*tbl_inc)/2 + BUILD_TABLE_LEN_X/2;	// table center in X
  y1=(BUILD_TABLE_LEN_Y-(int)(BUILD_TABLE_LEN_Y/tbl_inc)*tbl_inc)/2 + BUILD_TABLE_LEN_Y/2;	// table center in Y
  x2=x1-25;  y2=y1-25;
  x3=x1+25;  y3=y1+25;
  cairo_move_to(cr,LVgxoffset+LVview_scale*x2,0-(LVgyoffset+LVview_scale*y1));			// X hash mark
  cairo_line_to(cr,LVgxoffset+LVview_scale*x3,0-(LVgyoffset+LVview_scale*y1));
  cairo_move_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y2));			// Y hash mark
  cairo_line_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y3));
  cairo_stroke(cr);
    
  // draw coordinate axes
  set_color(cr,&color,DK_RED);
  color.alpha=0.60;gdk_cairo_set_source_rgba (cr,&color);
  cairo_move_to(cr,LVgxoffset,0-LVgyoffset);				// move to 0,0
  cairo_line_to(cr,LVgxoffset+LVview_scale*20,0-LVgyoffset);		// draw to x=20, 0
  sprintf(scratch,"X");
  cairo_show_text(cr,scratch);
  cairo_move_to(cr,LVgxoffset,0-LVgyoffset);				// move to 0,0
  cairo_line_to(cr,LVgxoffset,0-(LVgyoffset+LVview_scale*20));		// draw to y=20, 0
  sprintf(scratch,"Y");
  if(cnc_profile_cut_flag==TRUE)sprintf(scratch,"Z");
  cairo_show_text(cr,scratch);
  cairo_stroke(cr);

  // DEBUG - draw xy map of objects on build table
  if(Superimpose_flag==TRUE && UNIT_HAS_CAMERA==TRUE)
    {
    int 	xx,yy;
    float 	zval;
    xx=0;
    while(xx<BUILD_TABLE_LEN_X)
      {
      yy=0;
      while(yy<BUILD_TABLE_LEN_Y)
	{
	set_color(cr,&color,BLACK);
	if(xy_on_table[xx][yy] > 0)
	  {
	  zval=xy_on_table[xx][yy];
	  if(fabs(zval)> 0.0)set_color(cr,&color,MD_BLUE);
	  if(fabs(zval)> 5.0)set_color(cr,&color,DK_GREEN);
	  if(fabs(zval)>10.0)set_color(cr,&color,MD_GREEN);
	  if(fabs(zval)>15.0)set_color(cr,&color,MD_ORANGE);
	  if(fabs(zval)>20.0)set_color(cr,&color,DK_ORANGE);
	  if(fabs(zval)>25.0)set_color(cr,&color,DK_RED);
	  if(fabs(zval)>30.0)set_color(cr,&color,MD_RED);
	  if(fabs(zval)>30.0)set_color(cr,&color,MD_VIOLET);
	  }
	cairo_move_to(cr,LVgxoffset+LVview_scale*xx,0-(LVgyoffset+LVview_scale*yy));
	cairo_line_to(cr,LVgxoffset+LVview_scale*xx,0-(LVgyoffset+LVview_scale*(yy+1)));
	cairo_stroke(cr);
	yy++;
	}
      xx++;
      }
    }
  

  // draw build table z levels
  if(set_view_build_lvl==TRUE)
    {
    // draw job global maxmin bounds
    if(TRUE)
      {
      set_color(cr,&color,LT_RED);
      cairo_move_to(cr,(LVgxoffset+XMin*LVview_scale),0-(LVgyoffset+YMin*LVview_scale));
      cairo_line_to(cr,(LVgxoffset+XMax*LVview_scale),0-(LVgyoffset+YMin*LVview_scale));
      cairo_line_to(cr,(LVgxoffset+XMax*LVview_scale),0-(LVgyoffset+YMax*LVview_scale));
      cairo_line_to(cr,(LVgxoffset+XMin*LVview_scale),0-(LVgyoffset+YMax*LVview_scale));
      cairo_line_to(cr,(LVgxoffset+XMin*LVview_scale),0-(LVgyoffset+YMin*LVview_scale));
      cairo_stroke(cr);
      }
    // draw probe points and their corrisponding z values
    for(h=0;h<50;h++)
      {
      for(i=0;i<50;i++)
	{
	x1=h*scan_dist+scan_x_remain;
	y1=i*scan_dist+scan_y_remain;
	if(x1>BUILD_TABLE_LEN_X || y1>BUILD_TABLE_LEN_Y)continue;
	//if(job.model_count==0)Pz[h][i]=Pz_raw[h][i];
	newx=LVgxoffset+LVview_scale*(x1);
	newy=0-(LVgyoffset+LVview_scale*(y1));
	if(Pz[h][i]==0.00)set_color(cr,&color,MD_BLUE);
	if(fabs(Pz[h][i])>0.00)set_color(cr,&color,DK_GREEN);
	if(fabs(Pz[h][i])>0.25)set_color(cr,&color,MD_GREEN);
	if(fabs(Pz[h][i])>0.50)set_color(cr,&color,MD_ORANGE);
	if(fabs(Pz[h][i])>0.75)set_color(cr,&color,DK_ORANGE);
	if(fabs(Pz[h][i])>1.00)set_color(cr,&color,DK_RED);
	if(fabs(Pz[h][i])>1.25)set_color(cr,&color,MD_RED);
	if(fabs(Pz[h][i])>1.50)set_color(cr,&color,MD_VIOLET);
	cairo_arc(cr,newx,newy,2,0.0,2*PI);
	//sprintf(scratch,"%4.2f,%4.2f,%4.2f",(h*scan_dist+scan_x_remain),(i*scan_dist+scan_y_remain),Pz[h][i]);
	sprintf(scratch,"%5.2f",Pz[h][i]);
	cairo_move_to(cr,newx+3,newy-3);
	cairo_show_text (cr, scratch);
	cairo_stroke(cr);
	}
      }
    }

  // draw slice geometry by looping thru models, then each model's operations and their line-types/polygons/vertexes.
  // note that a single model may use a variety of different tools for different operations
  //set_view_penup_moves=0;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mptr->reslice_flag==TRUE){mptr=mptr->next;continue;}		// if not sliced, just skip
    //if(mptr->type==GERBER)z_cut=mptr->zmax;				// if a GERBER file, limit to this layer only

    // draw MODEL max-min bounding box which includes all copies
    if(show_view_bound_box==TRUE && set_view_bound_box==TRUE)
      {
      printf("\nshow_box=%d  set_box=%d \n\n",show_view_bound_box,set_view_bound_box);
      set_color(cr,&color,MD_GREEN);
      cairo_set_line_width (cr,1.0);
      if(model_overlap(mptr)==TRUE || model_out_of_bounds(mptr)==TRUE)
	{
	set_color(cr,&color,DK_RED);
	cairo_set_line_width (cr,1.5);
	}

      // draw min/max boundary of model
      x1=mptr->xorg[MODEL]+mptr->xmin[MODEL]-2;
      y1=mptr->yorg[MODEL]+mptr->ymin[MODEL]-2;
      x2=mptr->xorg[MODEL]+mptr->xmax[MODEL]+2;
      y2=mptr->yorg[MODEL]+mptr->ymax[MODEL]+2;
      cairo_move_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y1));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x2,0-(LVgyoffset+LVview_scale*y1));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x2,0-(LVgyoffset+LVview_scale*y2));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y2));
      cairo_line_to(cr,LVgxoffset+LVview_scale*x1,0-(LVgyoffset+LVview_scale*y1));
      cairo_stroke(cr);
  
      // display hash mark at middle of box
      cairo_set_line_width (cr,0.5);
      x1=mptr->xorg[MODEL]+(mptr->xmin[MODEL]+mptr->xmax[MODEL])/2;
      y1=mptr->yorg[MODEL]+(mptr->ymin[MODEL]+mptr->ymax[MODEL])/2;
      x2=x1-5;  y2=y1-5;
      x3=x1+5;  y3=y1+5;
      hl=LVgxoffset+LVview_scale*x2;
      vl=0-(LVgyoffset+LVview_scale*y1);
      hh=LVgxoffset+LVview_scale*x3;
      vh=0-(LVgyoffset+LVview_scale*y1);
      cairo_move_to(cr,hl,vl);				
      cairo_line_to(cr,hh,vh);	
      hl=LVgxoffset+LVview_scale*x1;
      vl=0-(LVgyoffset+LVview_scale*y2);
      hh=LVgxoffset+LVview_scale*x1;
      vh=0-(LVgyoffset+LVview_scale*y3);
      cairo_move_to(cr,hl,vl);				
      cairo_line_to(cr,hh,vh);	
      cairo_stroke(cr);
      }

    // loop thru all the geometry types for this model
    for(mtyp=MODEL;mtyp<MAX_MDL_TYPES;mtyp++)
      {
      if(mtyp==MODEL && set_view_models==FALSE)continue;
      if(mtyp==SUPPORT && set_view_supports==FALSE)continue;
      if(mtyp==INTERNAL && set_view_internal==FALSE)continue;
      if(mtyp==TARGET && set_view_target==FALSE)continue;
      if(mptr->input_type==STL && mptr->facet_qty[STL]==0)continue;
      
      // if model out of range of this z level, just skip
      if(z_cut>(mptr->zmax[mtyp]+mptr->zoff[mtyp]))continue;	
      
      // if slices are not ready, do not display
      //if(mptr->reslice_flag==TRUE)continue;		
  
      // define starting position of tool for showing penup moves
      prex=LVgxoffset;							// home position
      prey=0-LVgyoffset;
  
      // check if same tool is being used to deposit MODEL and SUPPORT
      model_slot=(-1); support_slot=(-1); ms_same_slot=FALSE;
      amptr=mptr->oper_list;
      while(amptr!=NULL)
	{
	if(strstr(amptr->name,"ADD_MODEL_MATERIAL")!=NULL)model_slot=amptr->ID;
	if(strstr(amptr->name,"ADD_SUPPORT_MATERIAL")!=NULL)support_slot=amptr->ID;
	amptr=amptr->next;
	}
      if(model_slot>=0 && model_slot==support_slot)ms_same_slot=TRUE;
	    
      // loop thru all possible operations for this model via the model's list of opers
      // the operation list provides slot and oper_name from which the actual operation can be found
      amptr=mptr->oper_list;
      while(amptr!=NULL)
	{
	//printf("Checking operation..: %d %s \n",amptr->ID,amptr->name);
	slot=amptr->ID;							// find which tool is used for this operation
	if(slot<0 || slot>=MAX_TOOLS){amptr=amptr->next;continue;}	// if no tool specified for this operation, skip
	if(Tool[slot].state<TL_LOADED){amptr=amptr->next;continue;}	// if tool not fully specified, skip
	
	// set slice range to display
	z_view_start=z_cut-slc_view_start;				// current z minus distance below current z user has set
	if(z_view_start<job.ZMin)z_view_start=job.ZMin;			// can't be lower than bottom of job
	z_view_end=z_cut+slc_view_end+CLOSE_ENOUGH;			// current z plus distance above current z user has set
	if(z_view_end>(job.ZMax+CLOSE_ENOUGH))z_view_end=job.ZMax+CLOSE_ENOUGH;  // can't be above top of job
	
	//printf("\nz_cut=%5.3f  zv_srt=%5.3f  zv_end=%5.3f  zv_inc=%5.3f \n",z_cut,z_view_start,z_view_end,slc_view_inc);
	
	// loop thru N slices worth of model, displaying what was set by user (scnt="slice count")
	for(slc_z_cut=z_view_start; slc_z_cut<z_view_end; slc_z_cut+=slc_view_inc)
	  {
	  sptr=slice_find(mptr,mtyp,slot,(slc_z_cut-mptr->zmax[TARGET]),job.min_slice_thk);
	  if(sptr==NULL)continue;					// if not slice at this z level, move onto next operation
	  
	  //printf("sptr=%X ID=%d  slc_sz_lvl=%f  slc_z_cut=%f  zoff=%f \n",sptr,sptr->ID,sptr->sz_level,slc_z_cut,mptr->zoff[MODEL]);
	  
	  // display end of layer pause status
	  if(sptr->eol_pause==TRUE)
	    {
	    sprintf(scratch,"PAUSE AT END OF LAYER");
	    set_color(cr,&color,MD_RED);
	    newx=LVgxoffset+LVview_scale*(BUILD_TABLE_LEN_X/2);
	    newy=0-(LVgyoffset+LVview_scale*(BUILD_TABLE_LEN_Y/2));
	    cairo_move_to(cr,newx,newy);
	    cairo_show_text (cr, scratch);
	    cairo_move_to(cr,newx,newy);
	    cairo_stroke(cr);  
	    }
	    
	  // display slice information
	  sprintf(scratch,"frst=%X  last=%X  sptr=%X ID=%d slc_z=%f z_cut=%f zoff=%f ",mptr->slice_first[0],mptr->slice_last[0],sptr,sptr->ID,sptr->sz_level,slc_z_cut,mptr->zoff[MODEL]);
	  set_color(cr,&color,MD_RED);
	  newx=LVgxoffset+LVview_scale*(5);
	  newy=0-(LVgyoffset+LVview_scale*(5));
	  cairo_move_to(cr,newx,newy);
	  cairo_show_text (cr, scratch);
	  cairo_move_to(cr,newx,newy);
	  cairo_stroke(cr);  
	  
	  // loop thru all copies of this model
	  total_copy_ctr=0;
	  for(x_copy_ctr=0;x_copy_ctr<mptr->xcopies;x_copy_ctr++)
	    {
	    copy_xoff=mptr->xorg[MODEL]+(x_copy_ctr*(mptr->xmax[MODEL]-mptr->xmin[MODEL]+mptr->xstp));
	    for(y_copy_ctr=0;y_copy_ctr<mptr->ycopies;y_copy_ctr++)
	      {
	      copy_yoff=mptr->yorg[MODEL]+(y_copy_ctr*(mptr->ymax[MODEL]-mptr->ymin[MODEL]+mptr->ystp));
	      if(total_copy_ctr>=mptr->total_copies)continue;
	      total_copy_ctr++;
	      view_copy=FALSE;
	      if(total_copy_ctr>1)view_copy=TRUE;

	      // only display this block if NOT debugging
	      if(set_view_raw_mdl==FALSE)
		{
	    
		// display associated image on table if it exists AND it is not grayscale sliced
		// note that this is drawn first to subsequent layers are drawn over it and visible to user
		if(mptr->g_img_act_buff!=NULL && mptr->input_type==STL && set_view_image==TRUE)
		  {
		  // display the image
		  for(ypix=0; ypix<mptr->act_height; ypix+=mptr->pix_increment)
		    {
		    for(xpix=0; xpix<mptr->act_width; xpix+=mptr->pix_increment)
		      {
		      // load pixel from mdl
		      pix1 = mptr->act_pixels + ypix * mptr->act_rowstride + xpix * mptr->act_n_ch;
		      //printf("%d %d %d %d \n",pix1[0],pix1[1],pix1[2],pix1[3]);
		      
		      // set color based on input pixel
		      color.red  =(float)(pix1[0])/255.0;
		      color.green=(float)(pix1[1])/255.0;
		      color.blue =(float)(pix1[2])/255.0;
		      color.alpha=0.25;						// set to mostly transparent
		      gdk_cairo_set_source_rgba (cr,&color);
	      
		      // calc position of pixel on table
		      newx=LVgxoffset+LVview_scale*(xpix*mptr->pix_size+copy_xoff);
		      newy=0-(LVgyoffset+LVview_scale*(ypix*mptr->pix_size+copy_yoff));
	      
		      //draw pixel
		      cairo_move_to(cr,newx,newy);
		      cairo_arc(cr,newx,newy,2,0.0,2*PI);
		      cairo_close_path (cr);
		      cairo_fill_preserve (cr);
		      cairo_stroke(cr);	
		      
		      }	// end of xpix loop
		    }	// end of ypix loop
		  }	// end of if image buffer != null
	      
		// find this operation in the tool's list of opers to get the line types to display
		optr=Tool[slot].oper_first;
		while(optr!=NULL)
		  {
		  if(strstr(amptr->name,optr->name)==NULL){optr=optr->next;continue;}	// if this operation was not called out by the model... skip it
		  ltptr=optr->lt_seq;
		  while(ltptr!=NULL)  
		    {
		    ptyp=ltptr->ID;							// get pointer to line type called out at this sequence position
		    if(set_view_lt[ptyp]==FALSE){ltptr=ltptr->next;continue;}		// only display the requested line types
		    if(sptr->pqty[ptyp]<=0){ltptr=ltptr->next;continue;}			// if nothing to process... move on
		    if(set_view_raw_mdl==TRUE){ltptr=ltptr->next;continue;}		// if drawing raw vecs... move on
		    
		    //printf("LayerDisplay: sptr=%X  slot=%d  ptyp=%d  pfirst=%X  plast=%X  pqty=%d \n",
		    //	  sptr,slot,ptyp,sptr->pfirst[ptyp],sptr->plast[ptyp],sptr->pqty[ptyp]);
  
		    // set display line width based on line type line width
		    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		    cairo_set_line_width(cr, 0.5);
		    linetype_ptr=linetype_find(slot,ptyp);
		    if(linetype_ptr==NULL){ltptr=ltptr->next;continue;}		// if no matching linetype was found...
		    if(set_view_model_lt_width==TRUE)
		      {
		      if((ptyp>=MDL_PERIM && ptyp<=MDL_LAYER_1) || (ptyp>=BASELYR && ptyp<=PLATFORMLYR2))
			{
			cairo_set_line_width(cr,linetype_ptr->line_width*LVview_scale*0.95);
			if(ptyp==MDL_BORDER && z_cut<(mptr->slice_thick*1.1))
			  {
			  linetype_ptr=linetype_find(slot,MDL_LAYER_1);
			  if(linetype_ptr!=NULL)cairo_set_line_width (cr,linetype_ptr->line_width*LVview_scale*0.95);
			  linetype_ptr=linetype_find(slot,ptyp);
			  }
			}
		      }
		    if(set_view_support_lt_width==TRUE)
		      {
		      if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)
			{
			cairo_set_line_width (cr,linetype_ptr->line_width*LVview_scale*0.95);
			if(ptyp==SPT_OFFSET && z_cut<(mptr->slice_thick*1.1))
			  {
			  linetype_ptr=linetype_find(slot,SPT_LAYER_1);
			  if(linetype_ptr!=NULL)cairo_set_line_width (cr,linetype_ptr->line_width*LVview_scale*0.95);
			  linetype_ptr=linetype_find(slot,ptyp);
			  }
			}
		      }
	    
		    // loop thru all polygons called out in this line type
		    vtx_total=0;						// init vtx total counter
		    pcnt=0;
		    pptr=sptr->pfirst[ptyp];
		    while(pptr!=NULL)
		      {
		      if(pptr->vert_qty<1 || pptr->vert_qty>10000 || pptr->vert_first==NULL){pptr=pptr->next;continue;}
		      
		      if(ptyp==MDL_LAYER_1 && pptr->vert_qty>3) 		// if first layer and a perimeter/border/offset poly...
			{pptr=pptr->next; continue;}				// ... skip it as it causes elephant foot type defects
		      
		      if(pptr->vert_qty>2)
		        {
			view_poly_ids=TRUE; view_poly_fam=TRUE;			// init for first time thru poly loop of this line type...
			view_poly_perim=TRUE; view_poly_area=TRUE;		// ... because we only want to display once for each polygon
			view_poly_dist=TRUE;
			}
		      else 
		        {
			view_poly_ids=FALSE; view_poly_fam=FALSE;		// don't want to show for fills and close-offs
			view_poly_perim=FALSE; view_poly_area=FALSE;	
			view_poly_dist=FALSE;
			}
		      

		      pcnt++;					  		// increment polygon ID counter
		      isum+=pptr->vert_qty;					// accumulate vector quantity
		      
		      //printf("pptr=%X  vqty=%d  vfirst=%X  x=%f y=%f z=%f i=%f \n",
		      //pptr,pptr->vert_qty,pptr->vert_first,pptr->vert_first->x,pptr->vert_first->y,pptr->vert_first->z,pptr->vert_first->i);
		      
		      // if drawing penups, start with a pen-up to the start of the polygon
		      if(set_view_penup_moves==TRUE)				// if displaying penups...
			{
			cairo_set_line_width(cr, 0.5);			// ...get rid of line width
			cairo_move_to(cr,prex,prey);			// ...move to last position
			vptr=pptr->vert_first;				// ...start at first vertex
			newx=LVgxoffset+LVview_scale*(vptr->x+copy_xoff);	// ...calculate its position
			newy=0-(LVgyoffset+LVview_scale*(vptr->y+copy_yoff));
			if(cnc_profile_cut_flag==TRUE)newy=0-(LVgyoffset+LVview_scale*(vptr->z+copy_yoff));
			set_color(cr,&color,DK_YELLOW);			// ... penup color
			cairo_line_to(cr,newx,newy);			// ... draw line to start
			if(set_view_endpts)
			  {
			  cairo_move_to(cr,prex,prey);
			  cairo_arc(cr,prex,prey,2,0.0,2*PI);		// draw circle at tail (base) vertex
			  arrow_angle=20*PI/180;				// angle of arrow head
			  arrow_length=12;					// size of arrow head
			  x1=newx-prex;
			  y1=newy-prey;
			  if(fabs(x1)>0){vec_angle=atan2f(y1,x1);}
			  else {vec_angle=PI/2;if(y1<0)vec_angle=(-PI/2);}
			  x1 = newx - arrow_length * cos(vec_angle - arrow_angle);
			  y1 = newy - arrow_length * sin(vec_angle - arrow_angle);
			  x2 = newx - arrow_length * cos(vec_angle + arrow_angle);
			  y2 = newy - arrow_length * sin(vec_angle + arrow_angle);	      
			  cairo_move_to(cr,newx,newy);
			  cairo_line_to(cr,x1,y1);				// draw arrow head at tip vertex
			  cairo_line_to(cr,x2,y2);
			  cairo_line_to(cr,newx,newy);
			  }
			if(set_view_vecnum)
			  {
			  //distxy=sqrt((newx-prex)*(newx-prex)+(newy-prey)*(newy-prey))/LVview_scale;
			  //sprintf(scratch,"%5.3f",distxy);
			  sprintf(scratch,"%5d",vtx_total);
			  cairo_move_to(cr,(prex+newx)/2,(prey+newy)/2);
			  cairo_show_text (cr, scratch);
			  cairo_move_to(cr,newx,newy);
			  }
			cairo_stroke(cr);  
			
			// reset line width if implemented
			if(set_view_model_lt_width==TRUE)
			  {
			  if((ptyp>=MDL_PERIM && ptyp<=MDL_LAYER_1) || (ptyp>=BASELYR && ptyp<=PLATFORMLYR2))
			    {cairo_set_line_width (cr,linetype_ptr->line_width*LVview_scale*0.95);}
			  }
			if(set_view_support_lt_width==TRUE)
			  {
			  if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)
			    {cairo_set_line_width (cr,linetype_ptr->line_width*LVview_scale*0.95);}
			  }
			}
	      
		      // get the first vertex off this polygon
		      vptr=pptr->vert_first;					// start at first vertex
		      
		      // set color for this polygon
		      //set_color(cr,&mcolor[slot],(-1));			// note "slot" may change depending on op type even if same mptr
		      set_color(cr,&color,Tool[slot].matl.color);		// note "slot" may change depending on op type even if same mptr
		      if(mtyp==MODEL && mptr==active_model)			// if this is the active model...
			{
			if(job.state<JOB_RUNNING)set_color(cr,&color,MD_ORANGE);
			if(job.state>=JOB_RUNNING)set_color(cr,&color,MD_ORANGE);
			}
		      if(vptr->attr<0)						// if the frist vertex has been printed... assume this whole polygon is printed.					
			{
			if(vptr->attr==(-1))set_color(cr,&color,MD_GRAY);	// sent to tinyG
			if(vptr->attr==(-2))set_color(cr,&color,MD_GRAY);	// processed by tinyG
			}
	    
		      // color adjustments for special cases
		      if(set_mouse_clear==TRUE){sptr->p_one_ref=NULL;sptr->p_zero_ref=NULL;}
		      if(ptyp==MDL_BORDER && mptr==active_model && job.state<JOB_RUNNING)set_color(cr,&color,DK_ORANGE);
		      if(ptyp==TEST_LC)set_color(cr,&color,MD_ORANGE);
		      if(ptyp==TEST_UC)set_color(cr,&color,MD_RED);
		      if(ptyp==TRACE && vptr->supp<0)set_color(cr,&color,MD_BLUE);
		      if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1 && ms_same_slot==TRUE)set_color(cr,&color,DK_RED);
		      if(view_copy==TRUE)set_color(cr,&color,SL_GRAY);
		      
		      // if current polygon matches the address of one on the current pick list, change color to red
		      pl_ptr=p_pick_list;
		      while(pl_ptr!=NULL)
			{
			if(pl_ptr->p_item==pptr)break;
			pl_ptr=pl_ptr->next;
			}
		      if(pl_ptr!=NULL)set_color(cr,&color,LT_RED);
		       
		      // special case for debugging/viewing perimeters
		      /*
		      if(ptyp==MDL_PERIM || ptyp==SPT_PERIM)
			{
			//if(Tool[slot].type==SUBTRACTIVE && pptr->hole==2)
			if(pptr->diam>CLOSE_ENOUGH)
			  {
			  // draw cross-hairs across hole
			  set_color(cr,&color,DK_RED);
			  newx=LVgxoffset+LVview_scale*(pptr->centx+copy_xoff);
			  newy=0-(LVgyoffset+LVview_scale*(pptr->centy+copy_yoff));
			  cairo_move_to(cr,newx-5,newy);
			  cairo_line_to(cr,newx+5,newy);
			  cairo_move_to(cr,newx,newy+5);
			  cairo_line_to(cr,newx,newy-5);
			  // label with sequence number
			  //sprintf(scratch,"%d",pcnt);
			  //cairo_move_to(cr,newx+5,newy+10);
			  //cairo_show_text(cr,scratch);
			  // label with diameter
			  sprintf(scratch,"%5.3f",pptr->diam);
			  cairo_move_to(cr,newx+5,newy+5);
			  cairo_show_text(cr,scratch);
			  // display
			  cairo_stroke(cr);					
			  }
		
			}
		      */
		      
		      // special case for GERBER files
		      //if(mptr->type==GERBER)set_color(cr,&color,DK_BROWN);
		    
		      // draw vertices of the current polygon
		      oldx=LVgxoffset+LVview_scale*(vptr->x+copy_xoff);
		      oldy=0-(LVgyoffset+LVview_scale*(vptr->y+copy_yoff));
		      if(cnc_profile_cut_flag==TRUE)oldy=0-(LVgyoffset+LVview_scale*(vptr->z+copy_yoff));
		      
		      //vtx_total=0;						// init vtx total counter
		      vptr=vptr->next;						// since we already moved to vptr
		      while(vptr!=NULL)
			{
			
			//printf("  pptr=%X vptr=%X  cnc=%d  x=%f  y=%f  i=%f\n",pptr,vptr,cnc_profile_cut_flag,oldx,oldy,vptr->i);
  
			// calculate new position and draw entity
			newx=LVgxoffset+LVview_scale*(vptr->x+copy_xoff);
			newy=0-(LVgyoffset+LVview_scale*(vptr->y+copy_yoff));
			
			// speed things up a bit by skipping vtxs if zoomed out
			skip_vtx=floor(10.0/LVview_scale);
			if(skip_vtx>floor(pptr->vert_qty/20))skip_vtx=floor(pptr->vert_qty/20);
			if(pptr->vert_qty<100)skip_vtx=0;
			if(skip_vtx<1)skip_vtx=0;
			skip_vtx=0;
			while(skip_vtx>0)
			  {
			  skip_vtx--;
			  vtx_total++;					// increment vtx total counter
			  vptr=vptr->next;				// move onto next vertex
			  if(vptr==NULL)break;				// end if open polygon
			  if(vptr==pptr->vert_first)break;		// end if contour is closed polygon
			  }
			if(vptr==NULL)break;					// if we already hit the end of the vtx list...
  
			// adjust color for grayscale slice display
			//if(mptr->g_img_mdl_buff!=NULL && mptr->input_type==IMAGE)
			if(mptr->grayscale_mode>0)
			  {
			  if(mptr->grayscale_mode==1)				// if pwm is modulated...
			    {
			    color.red  =1.0-vptr->i; 			// ... higher duty cycle = darker but opposite on display
			    color.blue =1.0-vptr->i;
			    color.green=1.0-vptr->i;
			    color.alpha=0.5;
			    }
			  if(mptr->grayscale_mode==2)				// if z is modulated...
			    {
			    color.red  =vptr->z/mptr->grayscale_zdelta;   	// ... reduce back to duty cycle
			    color.blue =vptr->z/mptr->grayscale_zdelta;
			    color.green=vptr->z/mptr->grayscale_zdelta; 
			    color.alpha=0.9;
			    }
			  if(mptr->grayscale_mode==3)
			    {
			    color.red  =vptr->i*mptr->grayscale_velmin;   	// ... reduce back to duty cycle
			    color.blue =vptr->i*mptr->grayscale_velmin;
			    color.green=vptr->i*mptr->grayscale_velmin; 
			    color.alpha=0.5;
			    }
			  gdk_cairo_set_source_rgba (cr, &color);
			  cairo_move_to(cr,oldx,oldy);
			  cairo_line_to(cr,newx,newy);
			  cairo_stroke(cr);  
			  }
			else 
			  {
			  if(cnc_profile_cut_flag==TRUE)newy=0-(LVgyoffset+LVview_scale*(vptr->z+copy_yoff));
			  cairo_move_to(cr,oldx,oldy);
			  cairo_line_to(cr,newx,newy);
			  }
  
			if(linetype_ptr!=NULL)
			  {
			  if(linetype_ptr->eol_delay>TOLERANCE)		// if this line type has a forced delay...
			    {
			    cairo_move_to(cr,newx,newy);
			    cairo_arc(cr,newx,newy,3,0.0,2*PI);		// draw circle at tip (finishing) vertex
			    }
			  }
			if(set_view_endpts==TRUE)
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  arrow_angle=20*PI/180;				// angle of arrow head
			  arrow_length=12;					// size of arrow head
			  x1=newx-oldx;
			  y1=newy-oldy;
			  if(fabs(x1)>0){vec_angle=atan2f(y1,x1);}
			  else {vec_angle=PI/2;if(y1<0)vec_angle=(-PI/2);}
			  x1 = newx - arrow_length * cos(vec_angle - arrow_angle);
			  y1 = newy - arrow_length * sin(vec_angle - arrow_angle);
			  x2 = newx - arrow_length * cos(vec_angle + arrow_angle);
			  y2 = newy - arrow_length * sin(vec_angle + arrow_angle);	      
			  cairo_move_to(cr,newx,newy);
			  cairo_line_to(cr,x1,y1);				// draw arrow head at tip vertex
			  cairo_line_to(cr,x2,y2);
			  cairo_line_to(cr,newx,newy);
			  }
			if(set_view_vecnum==TRUE)
			  {
			  x1=(oldx+newx)/2-5;
			  y1=(oldy+newy)/2;
			  sprintf(scratch,"%5d",vtx_total);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  }
			if(set_view_vecloc==TRUE)
			  {
			  x1=newx;
			  y1=newy;
			  sprintf(scratch,"%5.3f, %5.3f, %5.3f",vptr->x,vptr->y,vptr->z);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  }
			if(set_view_postloc==TRUE && vptr->attr==(-2))		// if requeste AND has been processed
			  {
			  x1=newx;
			  y1=newy;
			  sprintf(scratch,"%5.3f, %5.3f, %5.3f",vptr->i,vptr->j,vptr->k);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  }
			if(set_view_veclen==TRUE)
			  {
			  distxy=sqrt((newx-oldx)*(newx-oldx)+(newy-oldy)*(newy-oldy))/LVview_scale;
			  x1=(oldx+newx)/2+5;
			  y1=(oldy+newy)/2;
			  sprintf(scratch,"%5.3f",distxy);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  }
			if(set_view_poly_ids==TRUE && view_poly_ids==TRUE)	// only display once for each polygon at location of first vtx
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  x1=oldx+5;
			  y1=oldy-5;
			  sprintf(scratch,"%5d:%5.3f",pptr->ID,pptr->dist);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  view_poly_ids=FALSE;
			  }
			if(set_view_poly_fam==TRUE && view_poly_fam==TRUE)	// only display once for each polygon at location of first vtx
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  x1=oldx+5;
			  y1=oldy;
			  sprintf(scratch,"%3d:-",pptr->ID);
			  if(pptr->p_parent!=NULL)sprintf(scratch,"%3d:%3d",pptr->ID,pptr->p_parent->ID);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  view_poly_fam=FALSE;
			  }
			if(set_view_poly_dist==TRUE && view_poly_dist==TRUE)	// only display once for each polygon at location of first vtx
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  x1=oldx+5;
			  y1=oldy+5;
			  sprintf(scratch,"%4.3f",pptr->dist);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  view_poly_fam=FALSE;
			  }
			if(set_view_poly_perim==TRUE && view_poly_perim==TRUE)	// only display once for each polygon at location of first vtx
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  x1=oldx+5;
			  y1=oldy+10;
			  sprintf(scratch,"%6.3f",pptr->prim);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  view_poly_perim=FALSE;
			  }
			if(set_view_poly_area==TRUE && view_poly_area==TRUE)	// only display once for each polygon at location of first vtx
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tail (base) vertex
			  x1=oldx+5;
			  y1=oldy+15;
			  sprintf(scratch,"%6.3f",pptr->area);
			  cairo_move_to(cr,x1,y1);
			  cairo_show_text (cr, scratch);
			  view_poly_area=FALSE;
			  }
			cairo_move_to(cr,newx,newy);
			oldx=newx;
			oldy=newy;
			vtx_total++;						// increment vtx total counter
			vptr=vptr->next;					// move onto next vertex
			if(vptr==NULL)break;
			if(vptr==pptr->vert_first->next)break;			// end if contour is closed polygon
			}
		      prex=oldx;
		      prey=oldy;
		      cairo_stroke(cr);  
		      
		      // draw the polygon ID in the center of the polygon
		      /*
		      if(ptyp==MDL_PERIM || ptyp==SPT_PERIM)
			{
			//sprintf(scratch,"%d ",pptr->member);
			sprintf(scratch,"%d %6.2f",pcnt,pptr->prim);
			//sprintf(scratch,"%6.2f",pptr->type);
			newx=LVgxoffset+LVview_scale*(pptr->centx+copy_xoff);
			newy=0-(LVgyoffset+LVview_scale*(pptr->centy+copy_yoff));
			cairo_move_to(cr,newx,newy);
			cairo_show_text (cr, scratch);				
			cairo_stroke(cr);	
			}
		      */
		      pptr=pptr->next;						// move on to next polygon
		      }
		
		    // draw STRAIGHT SKELETON of this slice from slice vectors
		    if(set_view_strskl==TRUE)
		      {
		      set_color(cr,&color,DK_VIOLET);
		      vec_ctr=0;
		      vecptr=sptr->ss_vec_first[ptyp];
		      while(vecptr!=NULL)
			{
			vec_ctr++;
			oldx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
			oldy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
			cairo_move_to(cr,oldx,oldy);
			newx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
			newy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
			cairo_line_to(cr,newx,newy);			// draw line out to tip
			
			if(set_view_endpts==TRUE)
			  {
			  cairo_move_to(cr,oldx,oldy);
			  cairo_arc(cr,oldx,oldy,2,0.0,2*PI);		// draw circle at tip (base) vertex
			  arrow_angle=20*PI/180;				// angle of arrow head
			  arrow_length=12;					// size of arrow head
			  x1=newx-oldx;
			  y1=newy-oldy;
			  if(fabs(x1)>0){vec_angle=atan2f(y1,x1);}
			  else {vec_angle=PI/2;if(y1<0)vec_angle=(-PI/2);}
			  x1 = newx - arrow_length * cos(vec_angle - arrow_angle);
			  y1 = newy - arrow_length * sin(vec_angle - arrow_angle);
			  x2 = newx - arrow_length * cos(vec_angle + arrow_angle);
			  y2 = newy - arrow_length * sin(vec_angle + arrow_angle);	      
			  cairo_move_to(cr,newx,newy);
			  cairo_line_to(cr,x1,y1);				// draw arrow head at tip vertex
			  cairo_line_to(cr,x2,y2);
			  cairo_line_to(cr,newx,newy);
			  }
	    
			if(set_view_vecnum==TRUE)
			  {
			  //sprintf(scratch,"%d/%d",vecptr->old_cross,vecptr->new_cross);
			  //if(vecptr->psrc!=NULL){sprintf(scratch,"%d",vecptr->psrc->hole);}else{sprintf(scratch,"N");}
			  //sprintf(scratch,"%d",vecptr->status);
			  sprintf(scratch,"%d",vecptr->member);
			  //sprintf(scratch,"%03d",vec_ctr);
			  //sprintf(scratch,"%4.1f",vecptr->tip->k);
			  //sprintf(scratch,"%5.3f",vecptr->curlen);
			  //sprintf(scratch,"%X",vecptr);
			  //cairo_move_to(cr,(newx+2),(newy+2));
			  cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
			  cairo_show_text (cr, scratch);			// display tip attribute
			  }
	      
			vecptr=vecptr->next;
			if(vecptr==sptr->ss_vec_first[ptyp])break;
			}
		      cairo_stroke(cr);						// must draw here or miss color change
		      
		      // draw skeleton lines to source vectors for bisector decomposition
		      set_color(cr,&color,LT_VIOLET);
		      vecptr=sptr->ss_vec_first[ptyp];
		      while(vecptr!=NULL)
			{
			if(set_view_endpts && vecptr->vsrcA!=NULL && vecptr->vsrcB!=NULL)
			  {
			  oldx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
			  oldy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
			  newx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
			  newy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
			  x1=LVgxoffset+LVview_scale*((vecptr->vsrcA)->tip->x+copy_xoff);
			  y1=0-(LVgyoffset+LVview_scale*((vecptr->vsrcA)->tip->y+copy_yoff));
			  x2=LVgxoffset+LVview_scale*((vecptr->vsrcA)->tail->x+copy_xoff);
			  y2=0-(LVgyoffset+LVview_scale*((vecptr->vsrcA)->tail->y+copy_yoff));
			  x3=LVgxoffset+LVview_scale*((vecptr->vsrcB)->tip->x+copy_xoff);
			  y3=0-(LVgyoffset+LVview_scale*((vecptr->vsrcB)->tip->y+copy_yoff));
			  x4=LVgxoffset+LVview_scale*((vecptr->vsrcB)->tail->x+copy_xoff);
			  y4=0-(LVgyoffset+LVview_scale*((vecptr->vsrcB)->tail->y+copy_yoff));
			  cairo_move_to(cr,(x1+x2)/2,(y1+y2)/2);
			  cairo_line_to(cr,(newx+oldx)/2,(newy+oldy)/2);
			  cairo_line_to(cr,(x3+x4)/2,(y3+y4)/2);
			  }
			vecptr=vecptr->next;
			//if(vecptr==sptr->ss_vec_first[ptyp])break;
			}
		      cairo_stroke(cr);						// must draw here or miss color change
		      }
	
		    ltptr=ltptr->next;
		    }	// end of line type loop
		  optr=optr->next;
		  }	// end of tool operation loop
    
		// draw selected highligh around image if set
		if(mptr==active_model && mptr->g_img_act_buff!=NULL && mtyp==TARGET && set_view_image==TRUE)
		  {
		  // draw border
		  set_color(cr,&color,DK_ORANGE);
		  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		  cairo_set_line_width(cr, 3.0);
		  dx=mptr->xmax[mtyp];
		  dy=mptr->ymax[mtyp];
		  dz=cnc_z_height;
		  xb[0]=0.0;	yb[0]=0.0;
		  xb[1]=dx;	yb[1]=0.0;
		  xb[2]=dx;	yb[2]=dy;
		  xb[3]=0.0;	yb[3]=dy;
		  for(j=0;j<4;j++)
		    {
		    if(j==0){i=0;h=1;}
		    if(j==1){i=1;h=2;}
		    if(j==2){i=2;h=3;}
		    if(j==3){i=3;h=0;}
		    hl=LVgxoffset+LVview_scale*(xb[i]+copy_xoff);
		    vl=0-(LVgyoffset+LVview_scale*(yb[i]+copy_yoff));
		    hh=LVgxoffset+LVview_scale*(xb[h]+copy_xoff);
		    vh=0-(LVgyoffset+LVview_scale*(yb[h]+copy_yoff));
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);	
		    cairo_stroke(cr);
		    }
		  }
	
		}	// end of if set_view_raw_mdl==FALSE
	
	      // draw RAW VECTORS of this slice - typically not compatible with drawing the slice as it will print
	      if(set_view_raw_mdl==TRUE)
		{
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 0.4);
		
		// draw PERIM source line types - reference for all else
		vecptr=sptr->perim_vec_first[MDL_PERIM];
		while(vecptr!=NULL)
		  {
		  set_color(cr,&color,DK_BLUE);
		  if(vecptr->status<0)set_color(cr,&color,DK_RED);
		  if(vecptr->status==98)set_color(cr,&color,LT_CYAN);
		  if(vecptr->status==99)set_color(cr,&color,MD_GREEN);
		  vec_ctr++;
		  oldx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
		  oldy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
		  cairo_move_to(cr,oldx,oldy);					// move to tip of vector (start)
		  newx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
		  newy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
		  cairo_line_to(cr,newx,newy);					// draw line out to tail (end)
		  if(set_view_vecnum)
		    {
		    sprintf(scratch,"p%d/%d ",vecptr->pwind_min,vecptr->pwind_max);
		    cairo_move_to(cr,(int)((newx+oldx)/2+5),(int)((newy+oldy)/2));
		    cairo_show_text (cr, scratch);			
		    }
		  if(set_view_endpts)
		    {
		    cairo_move_to(cr,oldx,oldy);				// move to tip of vector
		    cairo_arc(cr,oldx,oldy,2,0.0,2*PI);				// draw circle at tip (base) vertex
		    arrow_angle=20*PI/180;					// angle of arrow head
		    arrow_length=12;						// size of arrow head
		    x1=newx-oldx;
		    y1=newy-oldy;
		    if(fabs(x1)>0){vec_angle=atan2f(y1,x1);}
		    else {vec_angle=PI/2;if(y1<0)vec_angle=(-PI/2);}
		    x1 = newx - arrow_length * cos(vec_angle - arrow_angle);
		    y1 = newy - arrow_length * sin(vec_angle - arrow_angle);
		    x2 = newx - arrow_length * cos(vec_angle + arrow_angle);
		    y2 = newy - arrow_length * sin(vec_angle + arrow_angle);	      
		    cairo_move_to(cr,newx,newy);
		    cairo_line_to(cr,x1,y1);				// draw arrow head at tip vertex
		    cairo_line_to(cr,x2,y2);
		    cairo_line_to(cr,newx,newy);
		    }
		  vecptr=vecptr->next;
		  if(vecptr==sptr->perim_vec_first[MDL_PERIM])break;
		  cairo_stroke(cr);						// must draw here or miss color change
		  }
		
		// draw all other line types
		for(ptyp=MDL_BORDER;ptyp<MAX_LINE_TYPES;ptyp++)
		  {
		  if(set_view_lt[ptyp]==FALSE)continue;			// only display the requested line types
		  //if(ptyp>=MDL_PERIM && ptyp<=MDL_UPPER_CO)set_color(cr,&color,DK_BLUE);
		  //if(ptyp>=SPT_PERIM && ptyp<=SPT_UPPER_CO)set_color(cr,&color,DK_RED);
		  vec_ctr=0;
		  vl_ptr=sptr->vec_list[ptyp];
		  while(vl_ptr!=NULL)
		    {
		    vecptr=vl_ptr->v_item;
		    while(vecptr!=NULL)
		      {
		      set_color(cr,&color,DK_BLUE);
		      if(vecptr->status<0)set_color(cr,&color,DK_RED);
		      if(vecptr->status==97)set_color(cr,&color,MD_ORANGE);
		      if(vecptr->status==98)set_color(cr,&color,LT_CYAN);
		      if(vecptr->status==99)set_color(cr,&color,MD_GREEN);
		      vec_ctr++;
		      oldx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
		      oldy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
		      cairo_move_to(cr,oldx,oldy);				// move to tip of vector (start)
		      newx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
		      newy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
		      cairo_line_to(cr,newx,newy);				// draw line out to tail (end)
		      //cairo_arc(cr,oldx,oldy,1,0.0,2*PI);			// draw dot to indicate raw vector
		      if(set_view_vecnum)
			{
			//sprintf(scratch,"%d",(int)vecptr->tip->attr);
			//sprintf(scratch,"%d",vecptr->type);
			//sprintf(scratch,"%d",vecptr->status);
			//sprintf(scratch,"%d",vecptr->tail->supp);
			//sprintf(scratch,"%X",vecptr->psrc);
			//sprintf(scratch,"%d",vec_ctr);
			//cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
			//cairo_show_text (cr, scratch);			// display tip attribute
			//sprintf(scratch,"%4.1f",(180*vecptr->angle/PI));
			//cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
			//cairo_show_text (cr, scratch);			
			//sprintf(scratch,"%5.3f",vecptr->curlen);
			//cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2)-10);
			//cairo_show_text (cr, scratch);			
			sprintf(scratch,"p%d/%d c:%d/%d",vecptr->pwind_min,vecptr->pwind_max,vecptr->wind_min,vecptr->wind_max);
			//sprintf(scratch,"%6.4f/%6.4f",vecptr->tip->x,vecptr->tip->y);
			//cairo_move_to(cr,(newx+5),(newy+5));
			cairo_move_to(cr,(int)((newx+oldx)/2+5),(int)((newy+oldy)/2));
			cairo_show_text (cr, scratch);			
			}
		      if(set_view_endpts)
			{
			cairo_move_to(cr,oldx,oldy);				// move to tip of vector
			cairo_arc(cr,oldx,oldy,2,0.0,2*PI);			// draw circle at tip (base) vertex
			arrow_angle=20*PI/180;					// angle of arrow head
			arrow_length=12;					// size of arrow head
			x1=newx-oldx;
			y1=newy-oldy;
			if(fabs(x1)>0){vec_angle=atan2f(y1,x1);}
			else {vec_angle=PI/2;if(y1<0)vec_angle=(-PI/2);}
			x1 = newx - arrow_length * cos(vec_angle - arrow_angle);
			y1 = newy - arrow_length * sin(vec_angle - arrow_angle);
			x2 = newx - arrow_length * cos(vec_angle + arrow_angle);
			y2 = newy - arrow_length * sin(vec_angle + arrow_angle);	      
			cairo_move_to(cr,newx,newy);
			cairo_line_to(cr,x1,y1);				// draw arrow head at tip vertex
			cairo_line_to(cr,x2,y2);
			cairo_line_to(cr,newx,newy);
			}
		      vecptr=vecptr->next;
		      if(vecptr==vl_ptr->v_item)break;
		      cairo_stroke(cr);						// must draw here or miss color change
		      }
		    vl_ptr=vl_ptr->next;
		    }
		  }
		  
		  if(vec_pick_list!=NULL)
		    {
		    //printf("drawing debug vector grid\n");
		    // draw 3x3 grid around vector of interest midpoint
		    /*
		    vecptr=vec_pick_list->v_item;
		    if(vecptr!=NULL)
		      {
		      set_color(cr,&color,LT_BLUE);
		      x_cell_id=floor(((vecptr->tip->x+vecptr->tail->x)/2)/sptr->cell_size_x);	// calc x array position
		      y_cell_id=floor(((vecptr->tip->y+vecptr->tail->y)/2)/sptr->cell_size_y);	// calc y array position

		      y0=sptr->ymin+((y_cell_id-1)*sptr->cell_size_y);
		      y1=sptr->ymin+((y_cell_id+2)*sptr->cell_size_y);
		      for(h=(-1);h<3;h++)				// increment x - 3 cells needs 4 lines
			{
			x0=sptr->xmin+((x_cell_id+h)*sptr->cell_size_x);
			x1=x0;
			cairo_move_to(cr,LVgxoffset+LVview_scale*(x0+copy_xoff),0-(LVgyoffset+LVview_scale*(y0+copy_yoff)));
			cairo_line_to(cr,LVgxoffset+LVview_scale*(x1+copy_xoff),0-(LVgyoffset+LVview_scale*(y1+copy_yoff)));
			cairo_stroke(cr);
			}

		      x0=sptr->xmin+((x_cell_id-1)*sptr->cell_size_x);
		      x1=sptr->xmin+((x_cell_id+2)*sptr->cell_size_x);
		      for(i=(-1);i<3;i++)				// increment y
			{
			y0=sptr->ymin+((y_cell_id+i)*sptr->cell_size_y);
			y1=y0;
			cairo_move_to(cr,LVgxoffset+LVview_scale*(x0+copy_xoff),0-(LVgyoffset+LVview_scale*(y0+copy_yoff)));
			cairo_line_to(cr,LVgxoffset+LVview_scale*(x1+copy_xoff),0-(LVgyoffset+LVview_scale*(y1+copy_yoff)));
			cairo_stroke(cr);
			}
		      }
		    */
		    }
		}	// end of if set_view_raw_mdl==TRUE
	      
	      // draw STL VECTORS of this slice
	      if(set_view_raw_stl==TRUE)
		{
		cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		cairo_set_line_width(cr, 0.4);
		
		vec_ctr=0;
		vecptr=sptr->stl_vec_first;
		while(vecptr!=NULL)
		  {
		  vec_ctr++;
		  if(vecptr->type==MDL_PERIM)set_color(cr,&color,DK_BLUE);
		  if(vecptr->type==SPT_PERIM)set_color(cr,&color,DK_RED);
  
		  oldx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
		  oldy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
		  cairo_move_to(cr,oldx,oldy);
		  newx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
		  newy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
		  cairo_line_to(cr,newx,newy);				// draw line out to tip
		  if(set_view_vecnum)
		    {
		    //sprintf(scratch,"%6.3f",vecptr->tail->k);
		    //sprintf(scratch,"%d",vecptr->status);
		    //sprintf(scratch,"%4.1f",(180*vecptr->angle/PI));
		    //cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
		    //cairo_show_text (cr, scratch);			
		    //sprintf(scratch,"%5.3f",vecptr->curlen);
		    //cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2)-10);
		    //cairo_show_text (cr, scratch);			
		    sprintf(scratch,"%d",vecptr->member);
		    cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
		    cairo_show_text (cr, scratch);			
		    }
		  if(set_view_endpts)
		    {
		    cairo_move_to(cr,newx,newy);
		    cairo_arc(cr,newx,newy,2,0.0,2*PI);			// draw circle at tip (base) vertex
		    }
		  vecptr=vecptr->next;
		  cairo_stroke(cr);					// must draw here or miss color change
		  }
		}
	      
	      }	// end of for y_copy_ctr loop
	    }	// end of for x_copy_ctr loop
      
	  // draw tool position if requested
	  if(set_view_tool_pos==TRUE)
	    {
	    // note the PostG coords are machine coords.  that is, the offset to the build table and to 
	    // the position of the model on the table need to be subtracted from them.
	    set_color(cr,&color,LT_RED);
	    vpos->x=PostG.x; vpos->y=PostG.y; vpos->z=PostG.z;		// define coords of tool from tinyGRcv
	    Print_to_Model_Coords(vpos,slot);				// convert print coord to model coords
	    newx=LVgxoffset+LVview_scale*(vpos->x);			// apply this views scale and offset
	    newy=0-(LVgyoffset+LVview_scale*(vpos->y));
	    cairo_arc(cr,newx,newy,3,0.0,2*PI);
	    cairo_stroke(cr);						
	    }
	    
	  // draw vtx_debug
	  i=0;
	  vptr=vtx_debug;
	  while(vptr!=NULL && set_view_raw_mdl==TRUE)
	    {
	    set_color(cr,&color,BLACK);
	    newx=LVgxoffset+LVview_scale*(vptr->x+copy_xoff);
	    newy=0-(LVgyoffset+LVview_scale*(vptr->y+copy_yoff));
	    cairo_move_to(cr,newx,newy);
	    cairo_arc(cr,newx,newy,2,0.0,2*PI);
	    cairo_stroke(cr);
	    if(set_view_vecnum==TRUE)
	      {
	      sprintf(scratch,"%d %4.3f %4.3f",vptr->attr,vptr->i,vptr->j);
	      cairo_move_to(cr,(newx+2),(int)(newy+2));
	      cairo_show_text (cr,scratch);		
	      cairo_stroke(cr);	
	      }
	    vptr=vptr->next;
	    i++;
	    }
	  
	  // draw vec_debug
	  vecptr=vec_debug;
	  while(vecptr!=NULL && set_view_raw_mdl==TRUE)
	    {
	    set_color(cr,&color,BLACK);
	    newx=LVgxoffset+LVview_scale*(vecptr->tip->x+copy_xoff);
	    newy=0-(LVgyoffset+LVview_scale*(vecptr->tip->y+copy_yoff));
	    oldx=LVgxoffset+LVview_scale*(vecptr->tail->x+copy_xoff);
	    oldy=0-(LVgyoffset+LVview_scale*(vecptr->tail->y+copy_yoff));
    
	    cairo_move_to(cr,oldx,oldy);
	    cairo_line_to(cr,newx,newy);
	    //cairo_move_to(cr,newx,newy);
	    cairo_arc(cr,newx,newy,3,0.0,2*PI);
	    cairo_stroke(cr);
  
	    sprintf(scratch,"%d/%d",vecptr->type,vecptr->member);
	    cairo_move_to(cr,(int)((newx+oldx)/2+2),(int)((newy+oldy)/2));
	    cairo_show_text (cr, scratch);			
	    cairo_stroke(cr);	
    
	    vecptr=vecptr->next;
	    if(vecptr==vec_debug)break;
	    }
    
	  // draw vtest array if requested - meant to be a DEBUG tool
	  if(vtest_array!=NULL)
	    {
	    vptr=vtest_array;
	    while(vptr!=NULL)
	      {
	      // calculate new position and draw entity
	      newx=LVgxoffset+LVview_scale*(vptr->x+copy_xoff);
	      newy=0-(LVgyoffset+LVview_scale*(vptr->y+copy_yoff));
	      set_color(cr,&color,DK_GREEN);
	      cairo_move_to(cr,newx,newy);	
	      cairo_arc(cr,newx,newy,3,0.0,2*PI);
	      //sprintf(scratch,"x=%6.3f y=%6.3f z=%6.3f",vtx_pick_ref->x,vtx_pick_ref->y,vtx_pick_ref->z);
	      sprintf(scratch,"%d,%d",vptr->attr,vptr->supp);
	      cairo_move_to(cr,newx,newy);	
	      cairo_show_text (cr, scratch);
	      cairo_stroke(cr);	
	      vptr=vptr->next;
	      }
	    }

	  }	// end of for slc_z_cut loop
	amptr=amptr->next;
	}		// end of model operations loop
      }		// end of mtyp loop
      
    mptr=mptr->next;
    }		// end of model loop
	    
    // draw mouse pick on screen for one refresh cycle
    if(vtx_pick_ref->x!=0 && vtx_pick_ref->y!=0)
      {
      x1=vtx_pick_ref->x;
      y1=vtx_pick_ref->y;
      vtx_pick_ref->z=z_table_offset(x1,y1);
      set_color(cr,&color,BLACK);
      newx=LVgxoffset+LVview_scale*vtx_pick_ref->x;			// convert from print coords back to display coords
      newy=0-(LVgyoffset+LVview_scale*vtx_pick_ref->y);
      cairo_arc(cr,newx,newy,3,0.0,2*PI);
      sprintf(scratch,"x=%6.3f y=%6.3f z=%6.3f",vtx_pick_ref->x,vtx_pick_ref->y,vtx_pick_ref->z);
      cairo_move_to(cr,newx,newy);	
      cairo_show_text (cr, scratch);
      cairo_stroke(cr);	
      vtx_pick_ref->x=0;			
      vtx_pick_ref->y=0;			
      }
    
  // release temporary vtx
  free(vpos); vertex_mem--;

  return(TRUE);
}

static gboolean on_layer_area_left_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    int		h,i,j,k,slot,ptyp,result,polygon_found=0,vpick_type=1;
    int		xp,yp,int_res,intx;
    float 	dx,dy,avg_z,dist,min_dist,dyn,dyp;
    float 	cell_size_x,cell_size_y;
    int		x_cell_id,y_cell_id,windnum;
    int		cell_count_x,cell_count_y;
    float 	dix,diy,diz,x1,y1,z1;
    vertex	*vscr[4],*vnxt;
    float 	z01,z23;
    vertex	*vptr,*vtxint,*vtest;
    vertex	*newvtx,*oldvtx;
    vector 	*vecptr,*vecpre,*vecnxt,*vecdist,*vecDptr;
    vector_list	*vl_ptr;
    vector_list *vec_search_list;
    vector_cell	*vcell_ptr;
    polygon	*pptr,*ppre,*pnew,*psct;
    slice	*sptr;
    model	*mptr,*mply,*mpick,*old_active_model;
    genericlist	*aptr,*lptr;
    operation	*optr;
    polygon_list	*pl_ptr,*pptr_list;
    GdkRGBA 		color;
    GtkStyleContext 	*context;
  
    // don't allow changes while building, but user can make "picks" while job_paused
    if(job.state==JOB_RUNNING)return(TRUE);				
      
    // check if mouse pick is withing bounds of each model
    mpick=NULL;
    old_active_model=active_model;
    active_model=NULL;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      // establish size of this model and its copies
      dix=(mptr->xmax[MODEL] - mptr->xmin[MODEL] + mptr->xstp)*mptr->xcopies - mptr->xstp;
      diy=(mptr->ymax[MODEL] - mptr->ymin[MODEL] + mptr->ystp)*mptr->ycopies - mptr->ystp;
      diz=mptr->zmax[MODEL]-mptr->zmin[MODEL];
      
      // define the 4 possible vertex corners of this model's bounding box.  these 4 vtx represent
      // the 2d polygon of the model's xy bounds as projected onto the display.
      for(i=0;i<4;i++)
	{
	if(i==0){x1=mptr->xorg[MODEL]-LVdisp_cen_x;  y1=0-mptr->yorg[MODEL]-LVdisp_cen_y; z1=mptr->zorg[MODEL]-LVdisp_cen_z;}
	if(i==1){x1=mptr->xorg[MODEL]+dix-LVdisp_cen_x;  y1=0-mptr->yorg[MODEL]-LVdisp_cen_y; z1=mptr->zorg[MODEL]-LVdisp_cen_z;}
	if(i==2){x1=mptr->xorg[MODEL]+dix-LVdisp_cen_x;  y1=0-mptr->yorg[MODEL]-diy-LVdisp_cen_y; z1=mptr->zorg[MODEL]-LVdisp_cen_z;}
	if(i==3){x1=mptr->xorg[MODEL]-LVdisp_cen_x;  y1=0-mptr->yorg[MODEL]-diy-LVdisp_cen_y; z1=mptr->zorg[MODEL]-LVdisp_cen_z;}
	vscr[i]=vertex_make();
	vscr[i]->x=((LVview_scale*x1)+LVgxoffset);
	vscr[i]->y=((LVview_scale*y1)-LVgyoffset);
	//vscr[i]->x=((LVview_scale*x1)*osc+(LVview_scale*y1)*oss+LVgxoffset);
	//vscr[i]->y=((LVview_scale*x1)*oss*ots-(LVview_scale*y1)*osc*ots+(LVview_scale*z1)*otc+LVgyoffset);
	}
      vscr[0]->next=vscr[1];
      vscr[1]->next=vscr[2];
      vscr[2]->next=vscr[3];
      vscr[3]->next=vscr[0];
      
      // DEBUG
      printf("p0: x=%3.0f y=%3.0f  p1: x=%3.0f y=%3.0f  p2: x=%3.0f y=%3.0f  p3: x=%3.0f y=%3.0f \n",
	vscr[0]->x,vscr[0]->y,vscr[1]->x,vscr[1]->y,vscr[2]->x,vscr[2]->y,vscr[3]->x,vscr[3]->y);
      printf("event: x=%d  y=%d \n",old_mouse_x,old_mouse_y);
      
      // test if mouse pick is inside model bounds
      windnum=0;
      vptr=vscr[0];
      while(vptr!=NULL)
	{
	vnxt=vptr->next;
	if( ((vptr->y > old_mouse_y) != (vnxt->y > old_mouse_y)) &&
	    (old_mouse_x < ((vnxt->x-vptr->x)*(old_mouse_y-vptr->y)/(vnxt->y-vptr->y)+vptr->x)) )windnum=!windnum;
	vptr=vptr->next;
	if(vptr==vscr[0])break;
	}
      
      // clear memory 
      for(i=0;i<4;i++){free(vscr[i]); vertex_mem--;}
      if(windnum==TRUE){mpick=mptr; break;}

      mptr=mptr->next;
      }
      
    if(mpick!=NULL)active_model=mpick;

    // now cycle thru entity list looking for a proximity match to a contour type polygon to the mouse pick
    // if found, the address of that polygon will be added to the p_pick_list for processing later.
    min_dist=1000;
    vecdist=NULL;
    vptr=vertex_make();
    mply=NULL;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      if(z_cut<(mptr->zmin[MODEL]+mptr->zoff[MODEL]) || z_cut>(mptr->zmax[MODEL]+mptr->zoff[MODEL]))	// if entire model is below/above current z level, just skip it
	{
	mptr=mptr->next;
	continue;
	}
      if(mptr->oper_list==NULL){mptr=mptr->next; continue;}
      vtx_pick_ref->x=(float)((x-LVgxoffset)/LVview_scale);
      vtx_pick_ref->y=(float)((0-y-LVgyoffset)/LVview_scale);
      vptr->x=vtx_pick_ref->x-mptr->xorg[MODEL];
      vptr->y=vtx_pick_ref->y-mptr->yorg[MODEL];
      aptr=mptr->oper_list;						// look thru all operations called out by this model
      while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation
	{
	slot=aptr->ID;
	if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
	if(Tool[slot].state!=TL_READY){aptr=aptr->next;continue;}
	sptr=slice_find(mptr,MODEL,slot,z_cut,job.min_slice_thk);
	if(sptr==NULL){aptr=aptr->next;continue;}
	
	optr=operation_find_by_name(slot,aptr->name);			// find the matching operation in the tool's abilities (operations)
	if(optr==NULL){lptr=NULL;}
	else {lptr=optr->lt_seq;}  					// now loop thru that operation's line type sequence
	while(lptr!=NULL)
	  {
	  ptyp=lptr->ID;						// get index to line type called out at this sequence position
	  if(set_view_lt[ptyp]==FALSE){lptr=lptr->next;continue;}

	  if(set_view_raw_mdl==TRUE)					// if picking a raw vector...
	    {
	    if(vpick_type==0)
	      {  
	      vecptr=sptr->perim_vec_first[MDL_PERIM];			// search perim list first....
	      while(vecptr!=NULL)
		{
		vmid->x=(vecptr->tip->x + vecptr->tail->x)/2;
		vmid->y=(vecptr->tip->y + vecptr->tail->y)/2;
		dist=vertex_distance(vptr,vmid);
		if(dist<min_dist){min_dist=dist;vecdist=vecptr;}
		if(vecptr->status>1)vecptr->status=0;
		vecptr=vecptr->next;
		}
	      }
	    else 
	      {
	      vl_ptr=sptr->vec_list[ptyp];				// then search all other line types...
	      while(vl_ptr!=NULL)
		{
		vecptr=vl_ptr->v_item;
		while(vecptr!=NULL)
		  {
		  vmid->x=(vecptr->tip->x + vecptr->tail->x)/2;
		  vmid->y=(vecptr->tip->y + vecptr->tail->y)/2;
		  dist=vertex_distance(vptr,vmid);
		  if(dist<min_dist){min_dist=dist;vecdist=vecptr;}
		  if(vecptr->status>1)vecptr->status=0;
		  vecptr=vecptr->next;
		  if(vecptr==vl_ptr->v_item)break;
		  }
		vl_ptr=vl_ptr->next;
		
		}
	      }
	    }
	  else 								// otherwise picking a polygon edge...
	    {
	    if(ptyp==MDL_PERIM || ptyp==MDL_BORDER || ptyp==MDL_OFFSET || ptyp==TRACE || ptyp==DRILL || ptyp==SPT_PERIM || ptyp==SPT_BORDER || ptyp==SPT_OFFSET) // skip scanning everything except contours
	      {
	      // deermine which polygon the user may want.
	      // do this by first finding out which polygons the pick fell within.  if inside a
	      // poly then determine how close to a vertex they are.  the poly with the closest vtx wins.
	      min_dist=10000;
	      psct=NULL;
	      pptr=sptr->pfirst[ptyp];
	      while(pptr!=NULL)
		{
		if(polygon_contains_point(pptr,vptr)==TRUE)
		  {
		  // scan poly vtx list for distance
		  vtest=pptr->vert_first;
		  while(vtest!=NULL)
		    {
		    dist=vertex_distance(vptr,vtest);
		    if(dist<min_dist){min_dist=dist; psct=pptr;}
		    vtest=vtest->next;
		    if(vtest==pptr->vert_first)break;
		    }
		  }
		pptr=pptr->next;
		}
	      
	      if(psct!=NULL)
	        {
		// at this point we have found the poly closest to the pick point
		if(p_pick_list==NULL)
		  {
		  p_pick_list=polygon_list_make(psct);
		  }
		else
		  {
		  // check if this polygon is already on the pick list.  if so, check if there 
		  pl_ptr=p_pick_list;
		  while(pl_ptr!=NULL)
		    {
		    if(pl_ptr->p_item==psct)break;
		    pl_ptr=pl_ptr->next;
		    }
		  if(pl_ptr!=NULL){p_pick_list=polygon_list_manager(p_pick_list,psct,ACTION_DELETE);}
		  else {p_pick_list=polygon_list_manager(p_pick_list,psct,ACTION_ADD);}
		  }
		mply=mptr;
		pptr=psct;
		polygon_found=TRUE;
		}
	      }
	    }
	  lptr=lptr->next;
	  if(polygon_found==TRUE)break;
	  }
	aptr=aptr->next;
	if(polygon_found==TRUE)break;
	}
      mptr=mptr->next;
      if(polygon_found==TRUE)break;
      }
      
    // process raw vector pick if one was found...
    if(vecdist!=NULL)
      {
      dx=vecdist->tip->x-vecdist->tail->x;
      dy=vecdist->tip->y-vecdist->tail->y;
      printf("\nRaw vector pick found!  %X  type=%d \n",vecdist,vecdist->type);
      printf("  tip: x=%f y=%f z=%f   tail: x=%f y=%f z=%f\n",vecdist->tip->x,vecdist->tip->y,vecdist->tip->z,vecdist->tail->x,vecdist->tail->y,vecdist->tail->z);
      printf("  dx=%f   dy=%f   TOL=%f \n\n",dx,dy,TOLERANCE);

      // DEBUG - identify the vector cell(s) for this raw vector pic
      /*
      if(debug_flag==275)
	{
	if(vec_pick_list!=NULL)vec_pick_list=vector_list_manager(vec_pick_list,NULL,ACTION_CLEAR);
	vec_pick_list=vector_list_manager(vec_pick_list,vecdist,ACTION_ADD);
	  
	// determine which cell this vector falls within
	x_cell_id=floor(((vecdist->tip->x+vecdist->tail->x)/2)/sptr->cell_size_x);	// calc x array position
	y_cell_id=floor(((vecdist->tip->y+vecdist->tail->y)/2)/sptr->cell_size_y);	// calc y array position
	cell_count_x=floor((sptr->xmax - sptr->xmin)/sptr->cell_size_x + 0.5);
	cell_count_y=floor((sptr->ymax - sptr->ymin)/sptr->cell_size_y + 0.5);
	
	// ensure cell values are within array bounds
	if(x_cell_id<0)x_cell_id=0;
	if(x_cell_id>=cell_count_x)x_cell_id=cell_count_x-1;
	if(y_cell_id<0)y_cell_id=0;
	if(y_cell_id>=cell_count_y)y_cell_id=cell_count_y-1;
	
	// build vector search list based on this cell location
	vec_search_list=NULL;
	for(h=(x_cell_id-1);h<=(x_cell_id+1);h++)				// increment across x cells
	  {
	  if(h<0)continue;							// skip if out of array bounds
	  if(h>=cell_count_x)continue;
	  for(i=(y_cell_id-1);i<=(y_cell_id+1);i++)				// increment across y cells
	    {
	    if(i<0)continue;							// skip if out of array bounds
	    if(i>=cell_count_y)continue;
	    
	    vcell_ptr=vector_cell_lookup(sptr,h,i);
	    if(vcell_ptr!=NULL)
	      {
	      vl_ptr=vcell_ptr->vec_cell_list;					// get ptr to vec list for this cell
	      while(vl_ptr!=NULL)
		{
		vec_search_list=vector_list_manager(vec_search_list,vl_ptr->v_item,ACTION_UNIQUE_ADD);	// add vectors if not already on list
		vl_ptr=vl_ptr->next;									// move onto next vector in this cell
		if(vl_ptr==vcell_ptr->vec_cell_list)break;						// if circular loop and back at start, break
		}
	      }
	    }
	  }
	}
      */
  
      ptyp=vecdist->type;
      vmid->x=(vecdist->tip->x + vecdist->tail->x)/2;
      vmid->y=(vecdist->tip->y + vecdist->tail->y)/2;
      if(vecdist->vtgtA!=NULL)								// get child vector made from this vector
	{
	vmid->x=( (vecdist->vtgtA)->tip->x + (vecdist->vtgtA)->tail->x )/2.0;		// set test pt to x midpoint of child vector
	}
      vlow->x=BUILD_TABLE_MIN_X-10;
      vlow->y=vmid->y;
      vhgh->x=BUILD_TABLE_LEN_X+10;
      vhgh->y=vmid->y;
    
      
      if(vpick_type==0)
        {
	oldvtx=vtx_debug;
	vtxint=vertex_make();  
	vecptr=sptr->perim_vec_first[MDL_PERIM];
	while(vecptr!=NULL)
	  {
	  if(vecptr->type!=MDL_PERIM){vecptr=vecptr->next; continue;}
	  if(vecptr!=vecdist)
	    {
	    vecDptr=vec_debug;
	    while(vecDptr!=NULL)
	      {
	      int_res=vector_intersect(vecDptr,vecptr,vtxint);
	      if(int_res>203 && int_res<255)
		{
		if(vecDptr->next==NULL){printf("  High: ");} else {printf("  Low: ");}
		printf("  Intersection found:  %d  t=%f s=%f \n",int_res,vtxint->i,vtxint->j);			
		newvtx=vertex_copy(vtxint,NULL);
		newvtx->attr=int_res;
		newvtx->next=NULL;
		oldvtx->next=newvtx;
		oldvtx=newvtx;
		}
	      if(int_res==204)
		{
		vecptr->status=98;
		vecptr->crmin=int_res;
		//printf("  mid point %d  vec=%X/%d \n",int_res,vecptr,vecptr->status);
		}
	      if(int_res==206 || int_res==236 || int_res==238)	
		{
		dyp=0;
		if(fabs(dy)>fabs(dx))
		  {
		  dy =vecptr->tip->y - vecptr->tail->y;
		  //if(fabs(dy)<TOLERANCE)dy=0;
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;
		  //if(fabs(dyp)<TOLERANCE)dyp=0;
		  }
		else 
		  {
		  dy =vecptr->tip->x - vecptr->tail->x;
		  //if(fabs(dy)<TOLERANCE)dy=0;
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL)dyp=vecpre->tip->x - vecpre->tail->x;
		  //if(fabs(dyp)<TOLERANCE)dyp=0;
		  }
		if(dy>0 && dyp>0){vecptr->status=98;vecptr->crmin=int_res;}
		if(dy<0 && dyp<0){vecptr->status=98;vecptr->crmin=int_res;}
		//printf("  end point %d  vec=%X/%d  dy=%f  dyp=%d \n",int_res,vecptr,vecptr->status,dy,dyp);
		}
	      if(int_res>257)
		{
		vecptr->status=98;
		vecptr->crmin=int_res;
		dyp=0; dyn=0;
		if(fabs(dy)>fabs(dx))
		  {
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL){dyp=vecptr->tip->y - vecptr->tail->y;vecpre->status=98;}
		  if(fabs(dyp)<TOLERANCE)dyp=0;
		  vecnxt=vecptr->next;
		  if(vecnxt!=NULL){dyn=vecnxt->tip->y - vecptr->tail->y;vecnxt->status=98;}
		  if(fabs(dyn)<TOLERANCE)dyn=0;
		  }
		else 
		  {
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL){dyp=vecptr->tip->x - vecptr->tail->x;vecpre->status=98;}
		  if(fabs(dyp)<TOLERANCE)dyp=0;
		  vecnxt=vecptr->next;
		  if(vecnxt!=NULL){dyn=vecnxt->tip->x - vecptr->tail->x;vecnxt->status=98;}
		  if(fabs(dyn)<TOLERANCE)dyn=0;
		  }
		if(dyp<0 && dyn<0)vecptr->crmin=int_res;
		if(dyp>0 && dyn>0)vecptr->crmin=int_res;
		if(vecpre!=NULL && vecnxt!=NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=%X/%d  next=%X/%d \n",int_res,vecptr,vecptr->status,vecpre,vecpre->status,vecnxt,vecnxt->status);
		  }
		if(vecpre==NULL && vecnxt!=NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=NULL  next=%X/%d \n",int_res,vecptr,vecptr->status,vecnxt,vecnxt->status);
		  }
		if(vecpre!=NULL && vecnxt==NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=%X/%d  next=NULL \n",int_res,vecptr,vecptr->status,vecpre,vecpre->status);
		  }
		if(vecpre==NULL && vecnxt==NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=NULL  next=NULL \n",int_res,vecptr,vecptr->status);
		  }
		}
		
	      vecDptr=vecDptr->next;
	      }
	    }
	  vecptr=vecptr->next;
	  if(vecptr==sptr->perim_vec_first[MDL_PERIM])break;
	  }
	free(vtxint); vertex_mem--; vtxint=NULL;
	}
      else 
        {
	oldvtx=vtx_debug;
	vtxint=vertex_make();  
	vl_ptr=sptr->vec_list[ptyp];
	while(vl_ptr!=NULL)
	  {
	  vecptr=vl_ptr->v_item;
	  if(vecptr->type!=ptyp){vl_ptr=vl_ptr->next; continue;}

	  vecptr->status=0;						// clear status
	  if(vecptr!=vecdist)
	    {
	    vecptr->status=97;						// set to part of check list
	    printf("  vec=%X  tip:x=%5.1f y=%5.1f  tail:x=%5.1f y=%5.1f \n",vecptr,vecptr->tip->x,vecptr->tip->y,vecptr->tail->x,vecptr->tail->y);
	    vecDptr=vec_debug;
	    while(vecDptr!=NULL)
	      {
	      int_res=vector_intersect(vecDptr,vecptr,vtxint);
	      if(int_res>203 && int_res<255)
		{
		if(vecDptr->next==NULL){printf("  High: ");} else {printf("  Low: ");}
		printf("  Intersection found:  %d  t=%f s=%f \n",int_res,vtxint->i,vtxint->j);			
		newvtx=vertex_copy(vtxint,NULL);
		newvtx->attr=int_res;
		newvtx->next=NULL;
		oldvtx->next=newvtx;
		oldvtx=newvtx;
		}
	      if(int_res==204)
		{
		vecptr->status=98;					// set to intersecting
		vecptr->crmin=int_res;
		//printf("  mid point %d  vec=%X/%d \n",int_res,vecptr,vecptr->status);
		}
	      if(int_res==206 || int_res==236 || int_res==238)	
		{
		dyp=0;
		if(fabs(dy)>fabs(dx))
		  {
		  dy =vecptr->tip->y - vecptr->tail->y;
		  //if(fabs(dy)<TOLERANCE)dy=0;
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;
		  //if(fabs(dyp)<TOLERANCE)dyp=0;
		  }
		else 
		  {
		  dy =vecptr->tip->x - vecptr->tail->x;
		  //if(fabs(dy)<TOLERANCE)dy=0;
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL)dyp=vecpre->tip->x - vecpre->tail->x;
		  //if(fabs(dyp)<TOLERANCE)dyp=0;
		  }
		if(dy>0 && dyp>0){vecptr->status=98;vecptr->crmin=int_res;}	// set to intersecting
		if(dy<0 && dyp<0){vecptr->status=98;vecptr->crmin=int_res;}
		//printf("  end point %d  vec=%X/%d  dy=%f  dyp=%d \n",int_res,vecptr,vecptr->status,dy,dyp);
		}
	      if(int_res>257)
		{
		vecptr->status=98;					// set to intersecting
		vecptr->crmin=int_res;
		dyp=0; dyn=0;
		if(fabs(dy)>fabs(dx))
		  {
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL){dyp=vecptr->tip->y - vecptr->tail->y;vecpre->status=98;}
		  if(fabs(dyp)<TOLERANCE)dyp=0;
		  vecnxt=vecptr->next;
		  if(vecnxt!=NULL){dyn=vecnxt->tip->y - vecptr->tail->y;vecnxt->status=98;}
		  if(fabs(dyn)<TOLERANCE)dyn=0;
		  }
		else 
		  {
		  vecpre=vecptr->prev;
		  if(vecpre!=NULL){dyp=vecptr->tip->x - vecptr->tail->x;vecpre->status=98;}
		  if(fabs(dyp)<TOLERANCE)dyp=0;
		  vecnxt=vecptr->next;
		  if(vecnxt!=NULL){dyn=vecnxt->tip->x - vecptr->tail->x;vecnxt->status=98;}
		  if(fabs(dyn)<TOLERANCE)dyn=0;
		  }
		if(dyp<0 && dyn<0)vecptr->crmin=int_res;
		if(dyp>0 && dyn>0)vecptr->crmin=int_res;
		if(vecpre!=NULL && vecnxt!=NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=%X/%d  next=%X/%d \n",int_res,vecptr,vecptr->status,vecpre,vecpre->status,vecnxt,vecnxt->status);
		  }
		if(vecpre==NULL && vecnxt!=NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=NULL  next=%X/%d \n",int_res,vecptr,vecptr->status,vecnxt,vecnxt->status);
		  }
		if(vecpre!=NULL && vecnxt==NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=%X/%d  next=NULL \n",int_res,vecptr,vecptr->status,vecpre,vecpre->status);
		  }
		if(vecpre==NULL && vecnxt==NULL)
		  {
		  printf("  colinear %d  vec=%X/%d  prev=NULL  next=NULL \n",int_res,vecptr,vecptr->status);
		  }
		}
		
	      vecDptr=vecDptr->next;
	      }
	    }
	  vl_ptr=vl_ptr->next;
	  }
	free(vtxint); vertex_mem--; vtxint=NULL;
	}

      // ensure vector search list is empty
      if(vec_search_list!=NULL)
	{
	vec_search_list=vector_list_manager(vec_search_list,NULL,ACTION_CLEAR);
	vec_search_list=NULL;
	}
      }

    // clear polygon pick list if none were found...
    if(polygon_found==FALSE)
      {
      p_pick_list=polygon_list_manager(p_pick_list,pptr,ACTION_CLEAR);
      }
    // process polygon pick if one was found...
    else 
      {
      active_model=NULL;
      if(mply!=NULL)
	{
	avg_z=z_table_offset(vptr->x,vptr->y);
	active_model=mply;
	
	/*
	printf("\n  Polygon Pick List: \n");
	pl_ptr=p_pick_list;
	while(pl_ptr!=NULL)
	  {
	  pptr=pl_ptr->p_item;
	  printf("   main: pptr=%X  qty=%d  area=%6.2f  type=%d  hole=%d \n",pptr,pptr->vert_qty,pptr->area,pptr->type,pptr->hole);
	  pptr_list=pptr->p_child_list;
	  while(pptr_list!=NULL)
	    {
	    pptr=pptr_list->p_item;
	    printf("     sub: pptr=%X  qty=%d  area=%6.2f  type=%d hole=%d \n",pptr,pptr->vert_qty,pptr->area,pptr->type,pptr->hole);
	    pptr_list=pptr_list->next;
	    }
	  //polygon_dump(pl_ptr->p_item);printf("\n");
	  pl_ptr=pl_ptr->next;
	  }
	*/
	}
      }
      
    // set vtx_pick_ref values so they are displayed on next screen refresh
    vtx_pick_ref->x=(float)((x-LVgxoffset)/LVview_scale);
    vtx_pick_ref->y=(float)((0-y-LVgyoffset)/LVview_scale);

    if(vptr!=NULL){free(vptr); vertex_mem--; vptr=NULL;}
      
    return(TRUE);
}

static gboolean on_layer_area_middle_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    LVview_scale=1.00;
    LVgxoffset=155;
    LVgyoffset=(-490);
    return(TRUE);
}

static gboolean on_layer_area_right_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    old_mouse_x=x;
    old_mouse_y=y;
    return(TRUE);
}

static gboolean on_layer_area_start_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    LVgx_drag_start=x-old_mouse_x;					// normalize start location with pointer location
    LVgy_drag_start=y-old_mouse_y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_layer_area_left_update_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    LVgxoffset-=(LVgx_drag_start-x);					// only add in delta from previous move or becomes exponential
    LVgyoffset+=(LVgy_drag_start-y);
    LVgx_drag_start=x;
    LVgy_drag_start=y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_layer_area_right_update_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    int		mtyp;
    float 	pickx,picky;
    float 	hl,vl;
    int		state=TRUE;
      
    if(job.state==JOB_RUNNING)return(TRUE);				// can't move parts once printing starts
    if(active_model!=NULL)
      {
      pickx=(LVgx_drag_start-x);
      picky=(LVgy_drag_start-y);
      
      for(mtyp=MODEL;mtyp<MAX_MDL_TYPES;mtyp++)
        {
        //active_model->xoff-=(0.1*pickx);
        //active_model->yoff+=(0.1*picky);
        active_model->xorg[mtyp]-=(0.1*pickx);
        active_model->yorg[mtyp]+=(0.1*picky);

        //hl=active_model->xmax[mtyp]-active_model->xmin[mtyp];
        //vl=active_model->ymax[mtyp]-active_model->ymin[mtyp];
      
        //if(active_model->xoff[mtyp]<0)active_model->xoff[mtyp]=0;
        //if(active_model->xoff[mtyp]>(BUILD_TABLE_LEN_X-hl))active_model->xoff[mtyp]=BUILD_TABLE_LEN_X-hl;
        //if(active_model->yoff[mtyp]<0)active_model->yoff[mtyp]=0;
        //if(active_model->yoff[mtyp]>(BUILD_TABLE_LEN_Y-vl))active_model->yoff[mtyp]=BUILD_TABLE_LEN_Y-vl;
      
        //active_model->xorg[mtyp]=active_model->xoff[mtyp];
        //active_model->yorg[mtyp]=active_model->yoff[mtyp];
	}
      
      set_view_bound_box=TRUE;						// turns on temporary min/max bounds in layer view
      set_center_build_job=FALSE;					// if user moving model, then they don't want it centered
      autoplacement_flag=FALSE;						// if user moving model, do not autoplace
      build_table_scan_flag=1;						// set to indication it needs to be re-done
      }

    LVgx_drag_start=x;
    LVgy_drag_start=y;
    gtk_widget_queue_draw(da);

    return(TRUE);
}

static gboolean on_layer_area_motion_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    old_mouse_x=dx;
    old_mouse_y=dy;
    return(TRUE);
}

static gboolean on_layer_area_scroll_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    double mx0,my0;

    da_center_x=old_mouse_x;
    da_center_y=0-old_mouse_y;
    mx0=(da_center_x-LVgxoffset)/LVview_scale;
    my0=(da_center_y-LVgyoffset)/LVview_scale;
  
    if(dy<0)LVview_scale*=1.1;
    if(dy>0)LVview_scale/=1.1;

    mx0=LVgxoffset+(mx0*LVview_scale);
    my0=(LVgyoffset+(my0*LVview_scale));
    LVgxoffset+=(da_center_x-mx0);
    LVgyoffset+=(da_center_y-my0);

    gtk_widget_queue_draw(da);
    
    return(TRUE);
}



// function to draw and manage MODEL VIEW to the display
gboolean model_draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
    vertex	*vptr,*vold,*vnxt,*vtool_pos;
    vertex 	*vptr1,*vptr2,*vtxA,*vtxB,*vtxC;
    edge 	*eptr;
    facet 	*fptr;
    facet 	*lfptr;
    model	*mptr,*old_mptr;
    slice	*sptr;
    polygon	*pptr;
    patch	*patptr;
    vertex_list	*pvptr;
    facet_list	*pfptr;
    
    char	cam_scratch[255],img_name[255];
    int		h,i,j,k,slot,ptyp,facet_ok,vtx_total,vtx_count,pat_ctr;
    int		mtyp,pix_step,skip_vtx;
    int		total_copy_ctr,x_copy_ctr,y_copy_ctr;;
    float 	xstart,ystart,tbl_inc,copy_xoff,copy_yoff;
    int		view_copy=TRUE;
    int		model_slot,support_slot,ms_same_slot;
    float 	hl,vl,zl,hh,vh,zh,hstart,vstart;
    float 	x0,y0,z0,x1,y1,z1,x2,y2,z2;
    float 	oldx,oldy,newx,newy;
    float 	oss,osc,ots,otc;
    float 	dx,dy,dz;
    float 	xb[8],yb[8],zb[8];
    float 	vangle,t,shade,alpha;
    float 	ax1,ay1,ax2,ay2;
    float 	arrow_angle,arrow_length,vec_angle;
    float 	slc_z_cut;
    float 	z_view_start,z_view_end;
    
    vertex	*vwpt,*Basevtx,*Avtx,*Vvtx,*vtx_ctr;
    vector	*ViewVec,*Avec,*Bvec,*vecptr;
    float	Adot,Bdot;
    float 	dmax,dmin;
    float 	max_angle;
    int		fcolor,skip_facet_cnt;
    int		disp_edge=0,draw_edge=0;
    
    float 	z_depth,deform_scale;
    
    genericlist	*amptr,*lptr;
    linetype 	*linetype_ptr;
    facet_list	*flptr;
    operation	*optr;
    edge_list	*el_ptr;
    
    branch 	*brptr;

    GdkRGBA 		color;
    guchar 		*pix1;
    int			xpix,ypix;
    int			pix_inc;
		


    // if still loading a model, do not attempt to display it
    if(win_modelUI_flag==TRUE)
      {
      //printf("modelUI_flag=TRUE\n"); 
      return(TRUE);
      }
    if(win_settingsUI_flag==TRUE)
      {
      //printf("settingsUI_flag=TRUE\n"); 
      return(TRUE);
      }
    
    // if scanning do no display.  it disrupts the view orientation.
    if(scan_in_progress==TRUE)
      {
      //printf("scan_in_progress=TRUE\n"); 
      //return(TRUE);
      }
    
    // init
    mtyp=MODEL;

    // if combo view of image and 3D model is requested and camera located to give 3D view...
    if(Superimpose_flag==TRUE && CAMERA_POSITION==2)
      {
      // set camera zoom to nominal
      cam_roi_x0=0.0; cam_roi_y0=0.0;
      cam_roi_x1=1.0; cam_roi_y1=1.0;
	
      // load the new image as our pixel background
      if(g_camera_buff!=NULL){g_object_unref(g_camera_buff); g_camera_buff=NULL;}		// clear old buffer if it exists
      sprintf(img_name,"/home/aa/Documents/4X3D/still-test.jpg");				// load last snap shot
      g_camera_buff = gdk_pixbuf_new_from_file_at_scale(img_name, 830,574, FALSE, NULL);	// load new buffer with image
      if(g_camera_buff!=NULL)
	{
	gdk_cairo_set_source_pixbuf(cr, g_camera_buff, 0, 0);
	cairo_paint(cr);
	}
      //if(g_camera_buff!=NULL)g_object_unref(g_camera_buff);
      
      // force the viewing angle to match the camera's view of the table
      MVdisp_spin=-0.800;						// more spins clockwise on image
      MVdisp_tilt= 3.640;						// less makes grid flatter
      MVview_scale=2.300;						// more makes grid bigger on image
      MVdisp_cen_x=(BUILD_TABLE_LEN_X/2);
      MVdisp_cen_y=0-(BUILD_TABLE_LEN_Y/2);
      MVdisp_cen_z=0.0;
      MVgxoffset=415;							// more pushes grid to right of image
      MVgyoffset=195;							// more pushes grid down on image
      pers_scale=0.65;							// more bends axis inward

      // init for 3D drawing
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
      vangle=set_view_edge_angle*PI/180;
      oss=sin(MVdisp_spin);						// pre-calculate to save time downstream
      osc=cos(MVdisp_spin);
      ots=sin(MVdisp_tilt);
      otc=cos(MVdisp_tilt);

      }		// end of if superimpose==true...
      
    // otherwise set view center, draw grid, volume boundaries, table level, and axes
    else 
      {
      // set view center
      MVdisp_cen_x=(BUILD_TABLE_LEN_X/2);				// default center to center of volume
      MVdisp_cen_y=0-(BUILD_TABLE_LEN_Y/2);
      MVdisp_cen_z=0;
      if(job.model_count>0)						// if models loaded but none active, set to center of job
        {
	MVdisp_cen_x=(XMax+XMin)/2;
	MVdisp_cen_y=0-((YMax+YMin)/2);
	MVdisp_cen_z=0;
	}
      if(active_model!=NULL)						// if a model is active, set to center of that model
	{
	MVdisp_cen_x=active_model->xorg[MODEL]+(active_model->xmax[MODEL]+active_model->xmin[MODEL])/2;
	MVdisp_cen_y=0-(active_model->yorg[MODEL]+(active_model->ymax[MODEL]+active_model->ymin[MODEL])/2);
	MVdisp_cen_z=0;
	}

      // init for 3D drawing
      if(Perspective_flag==FALSE)pers_scale=0.0;
      gdk_cairo_set_source_rgba (cr, &color);
      cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
      vangle=set_view_edge_angle*PI/180;
      oss=sin(MVdisp_spin);						// pre-calculate to save time downstream
      osc=cos(MVdisp_spin);
      ots=sin(MVdisp_tilt);
      otc=cos(MVdisp_tilt);
      }		// end of else for superimpose or not
  
    // draw build table grid
    {
      set_color(cr,&color,LT_GRAY);
      cairo_set_line_width (cr,0.5);

      if(set_view_build_lvl==TRUE)
	{
	deform_scale=10.0;
	tbl_inc=scan_dist;
	if(tbl_inc<5)tbl_inc=5;
	if(tbl_inc>50)tbl_inc=50;
	xstart=(BUILD_TABLE_LEN_X-floor(BUILD_TABLE_LEN_X/tbl_inc-1)*tbl_inc)/2;
	ystart=(BUILD_TABLE_LEN_Y-floor(BUILD_TABLE_LEN_Y/tbl_inc-1)*tbl_inc)/2;
	x1=xstart; x2=x1;
	while(x1<BUILD_TABLE_LEN_X)
	  {
	  y1=ystart; y2=y1+tbl_inc;
	  while(y1<BUILD_TABLE_LEN_Y)
	    {
	    if(scan_image_count==0)
	      {
	      z1=deform_scale*z_table_offset(x1,y1);
	      z2=deform_scale*z_table_offset(x2,y2);
	      }
	    else 
	      {
	      z1=xy_on_table[(int)(x1)][(int)(y1)];
	      z2=xy_on_table[(int)(x2)][(int)(y2)];
	      }
	    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(0-y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(0-y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
	    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(0-y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	    cairo_move_to(cr,hl,vl);
	    cairo_line_to(cr,hh,vh);
	    y2=y1;
	    y1+=tbl_inc;
	    }
	  x1+=tbl_inc;
	  x2=x1;
	  }
	y1=ystart; y2=y1;
	while(y1<BUILD_TABLE_LEN_Y)
	  {
	  x1=xstart; x2=x1+tbl_inc;
	  while(x1<BUILD_TABLE_LEN_X)
	    {
	    if(scan_image_count==0)
	      {
	      z1=deform_scale*z_table_offset(x1,y1);
	      z2=deform_scale*z_table_offset(x2,y2);
	      }
	    else 
	      {
	      z1=xy_on_table[(int)(x1)][(int)(y1)];
	      z2=xy_on_table[(int)(x2)][(int)(y2)];
	      }
	    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(0-y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(0-y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
	    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(0-y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	    cairo_move_to(cr,hl,vl);
	    cairo_line_to(cr,hh,vh);
	    x2=x1;
	    x1+=tbl_inc;
	    }
	  y1+=tbl_inc;
	  y2=y1;
	  }
	}
      else
	{
	tbl_inc=10;
	xstart=(BUILD_TABLE_LEN_X-(int)(BUILD_TABLE_LEN_X/tbl_inc)*tbl_inc)/2;
	ystart=(BUILD_TABLE_LEN_Y-(int)(BUILD_TABLE_LEN_Y/tbl_inc)*tbl_inc)/2;
	y1=0;y2=0-BUILD_TABLE_LEN_Y;z1=0;z2=0;	
	x1=xstart;
	while(x1<=BUILD_TABLE_LEN_X)
	  {
	  x2=x1;
	  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
	  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	  cairo_move_to(cr,hl,vl);
	  cairo_line_to(cr,hh,vh);
	  x1+=tbl_inc;
	  }
	x1=0;x2=BUILD_TABLE_LEN_X;z1=0;z2=0;
	y1=ystart;
	while(y1>=(0-BUILD_TABLE_LEN_Y))
	  {
	  y2=y1;
	  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
	  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	  cairo_move_to(cr,hl,vl);
	  cairo_line_to(cr,hh,vh);
	  y1-=tbl_inc;
	  }
	}
      cairo_stroke(cr);
    }
    
    // draw tool spot on XY plane
    {
      vtool_pos=vertex_make();
      vtool_pos->x=PostG.x;
      vtool_pos->y=PostG.y;
      vtool_pos->z=0.0;
      Print_to_Model_Coords(vtool_pos,0);
      x2=vtool_pos->x;
      y2=vtool_pos->y;
      z2=0.0;
      set_color(cr,&color,LT_RED);
      cairo_set_line_width (cr,2.0);
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(0-y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(0-y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hh,vh);
      cairo_arc(cr,hh,vh,3,0.0,2*PI);
    }
    

    // DEBUG image to STL scanning
    /*
    if(Superimpose_flag==TRUE && UNIT_HAS_CAMERA==TRUE)
    {
      GdkPixbuf		*g_img_diff_buff;					// difference image
      int		diff_n_ch;
      int		diff_cspace;
      int		diff_alpha;
      int		diff_bits_per_pixel;
      int 		diff_height;
      int 		diff_width;
      int 		diff_rowstride;
      guchar 		*diff_pixels;
      guchar 		*pix_new;
      int		pix_sum,on_wall,in_object;
  
      // scan across entire x axis across the whole table
      set_color(cr,&color,LT_BLUE);
      cairo_set_line_width (cr,0.5);
      tbl_inc=5;
      x1=0;
      x2=tbl_inc;
      while(x1<=(BUILD_TABLE_LEN_X-tbl_inc))
	{
	y1=0;
	y2=tbl_inc;
	while(y1<=(BUILD_TABLE_LEN_Y-tbl_inc))
	  {
  
	  z1=xy_on_table[(int)(x1)][(int)(y1)];
	  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(0-y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;

	  z2=xy_on_table[(int)(x2)][(int)(y2)];
	  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(0-y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
	  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(0-y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	  
	  cairo_move_to(cr,hl,vl);
	  cairo_line_to(cr,hh,vh);

	  y2=y1;
	  y1+=tbl_inc;							// increment y position along grid line (recall y axis is reversed on screen and in image)
	  }
	  
	x2=x1;
	x1+=tbl_inc;							// increment x position across table
	}

      cairo_stroke(cr);
      }
      */
      
    // draw build volume boundaries
    {
      set_color(cr,&color,ML_GRAY);
      cairo_set_line_width (cr,0.5);

      // define the size of the build volume
      dx=BUILD_TABLE_LEN_X-BUILD_TABLE_MIN_X;
      dy=BUILD_TABLE_LEN_Y-BUILD_TABLE_MIN_Y;
      dz=BUILD_TABLE_LEN_Z-BUILD_TABLE_MIN_Z;

      // define the coords of the volume's bounding box
      xb[0]=BUILD_TABLE_MIN_X;	yb[0]=0-BUILD_TABLE_MIN_Y;	zb[0]=BUILD_TABLE_MIN_Z;
      xb[1]=BUILD_TABLE_MIN_X+dx;	yb[1]=0-BUILD_TABLE_MIN_Y;	zb[1]=BUILD_TABLE_MIN_Z;
      xb[2]=BUILD_TABLE_MIN_X+dx;	yb[2]=0-(BUILD_TABLE_MIN_Y+dy);	zb[2]=BUILD_TABLE_MIN_Z;
      xb[3]=BUILD_TABLE_MIN_X;	yb[3]=0-(BUILD_TABLE_MIN_Y+dy);	zb[3]=BUILD_TABLE_MIN_Z;
      xb[4]=BUILD_TABLE_MIN_X;	yb[4]=0-BUILD_TABLE_MIN_Y;	zb[4]=BUILD_TABLE_MIN_Z+dz;
      xb[5]=BUILD_TABLE_MIN_X+dx;	yb[5]=0-BUILD_TABLE_MIN_Y;	zb[5]=BUILD_TABLE_MIN_Z+dz;
      xb[6]=BUILD_TABLE_MIN_X+dx;	yb[6]=0-(BUILD_TABLE_MIN_Y+dy);	zb[6]=BUILD_TABLE_MIN_Z+dz;
      xb[7]=BUILD_TABLE_MIN_X;	yb[7]=0-(BUILD_TABLE_MIN_Y+dy);	zb[7]=BUILD_TABLE_MIN_Z+dz;

      // draw the 12 lines that define the box
      for(j=0;j<12;j++)
	{
	if(j==0){i=0;h=1;}
	if(j==1){i=1;h=2;}
	if(j==2){i=2;h=3;}
	if(j==3){i=3;h=0;}
	if(j==4){i=4;h=5;}
	if(j==5){i=5;h=6;}
	if(j==6){i=6;h=7;}
	if(j==7){i=7;h=4;}
	if(j==8){i=0;h=4;}
	if(j==9){i=1;h=5;}
	if(j==10){i=2;h=6;}
	if(j==11){i=3;h=7;}
	
	zl=1+(600 + (MVview_scale*(xb[i]-MVdisp_cen_x))*oss - (MVview_scale*(yb[i]-MVdisp_cen_y))*osc + (MVview_scale*(zb[i]-MVdisp_cen_z))*ots)/600*pers_scale;
	hl=((MVview_scale*(xb[i]-MVdisp_cen_x))*osc+(MVview_scale*(yb[i]-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	vl=((MVview_scale*(xb[i]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[i]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[i]-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	
	zh=1+(600 + (MVview_scale*(xb[h]-MVdisp_cen_x))*oss - (MVview_scale*(yb[h]-MVdisp_cen_y))*osc + (MVview_scale*(zb[h]-MVdisp_cen_z))*ots)/600*pers_scale;
	hh=((MVview_scale*(xb[h]-MVdisp_cen_x))*osc+(MVview_scale*(yb[h]-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	vh=((MVview_scale*(xb[h]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[h]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[h]-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	
	cairo_move_to(cr,hl,vl);				
	cairo_line_to(cr,hh,vh);	
	
	// debug coord display
	//z_depth= 500 + (MVview_scale*(xb[i]-MVdisp_cen_x))*oss - (MVview_scale*(yb[i]-MVdisp_cen_y))*osc + (MVview_scale*(zb[i]-MVdisp_cen_z))*ots;
	//sprintf(scratch,"%4.1f",zl);
	//cairo_move_to(cr,hl,vl);				
	//cairo_show_text (cr, scratch);				
	  
	cairo_stroke(cr);
	}
      cairo_set_line_width (cr,0.5);
    }
    
    // draw coordinate axes
    {
      set_color(cr,&color,DK_RED);
      x1=0;y1=0;z1=0;		
      x2=20;y2=0;z2=0;
      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hl,vl);
      cairo_line_to(cr,hh,vh);
      sprintf(scratch,"X");
      cairo_show_text(cr,scratch);
      x1=0;y1=0;z1=0;		
      x2=0;y2=-20;z2=0;
      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hl,vl);
      cairo_line_to(cr,hh,vh);
      sprintf(scratch,"Y");
      cairo_show_text(cr,scratch);
      x1=0;y1=0;z1=0;		
      x2=0;y2=0;z2=20;
      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hl,vl);
      cairo_line_to(cr,hh,vh);
      sprintf(scratch,"Z");
      cairo_show_text(cr,scratch);
      cairo_stroke(cr);
    }
    

    // abort here if not ready to display model yet
    if(display_model_flag==FALSE)
      {
      //printf("display_model_flag=FALSE\n"); 
      return(TRUE);
      }

    // create view point vector that is dependent upon spin and tilt of view
    // this is basically a unit normal vector of the plane defined by the viewing screen
    // the default distance away is set to match the camera, then zoom is applied (~530mm)
    vwpt=vertex_make();
    vwpt->x=oss*ots*(-1);
    vwpt->y=osc*ots*(-1);
    vwpt->z=otc;	
    
/*
    // draw the view vector from the origin
    // if correct it should always just appear as a point with a lable
    x1=0;          y1=0;          z1=0;
    x2=20*vwpt->x; y2=20*vwpt->y; z2=20*vwpt->z;
    hl=(MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss+MVgxoffset;
    vl=(MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc+MVgyoffset;
    hh=(MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss+MVgxoffset;
    vh=(MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc+MVgyoffset;
    cairo_move_to(cr,hl,vl);
    cairo_line_to(cr,hh,vh);
    sprintf(scratch,"V");
    cairo_show_text(cr,scratch);
    cairo_stroke(cr);
*/

    // main loop to display models
    Basevtx=vertex_make();	
    Avtx=vertex_make();	
    Vvtx=vertex_make();	
    vtx_ctr=vertex_make();
    old_mptr=NULL;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      // if an event has occured that requires a new edge set to be genereted
      if(mptr->MRedraw_flag==TRUE) {edge_silhouette(mptr,MVdisp_spin,MVdisp_tilt);}

      // check if same tool is being used to deposit MODEL and SUPPORT
      slot=(-1); model_slot=(-1); support_slot=(-1); ms_same_slot=FALSE;
      amptr=mptr->oper_list;
      while(amptr!=NULL)
	{
	slot=amptr->ID;
	if(strstr(amptr->name,"ADD_MODEL_MATERIAL")!=NULL)model_slot=amptr->ID;
	if(strstr(amptr->name,"ADD_SUPPORT_MATERIAL")!=NULL)support_slot=amptr->ID;
	amptr=amptr->next;
	}
      #ifdef GAMMA_UNIT
        slot=0;
      #endif
      #ifdef DELTA_UNIT
        slot=0;
      #endif
      if(model_slot>=0 && model_slot==support_slot)ms_same_slot=TRUE;

      // draw MODEL max-min bounding box which includes all copies
      //MVdisp_cen_z=z_start_level;
      if(show_view_bound_box==TRUE && set_view_bound_box==TRUE)
	{
	set_color(cr,&color,MD_GREEN);
	cairo_set_line_width (cr,1.0);
	if(model_overlap(mptr)==TRUE || model_out_of_bounds(mptr)==TRUE)
	  {
	  set_color(cr,&color,DK_RED);
	  cairo_set_line_width (cr,1.5);
	  }

	// define the size of this model including its copies
	mtyp=MODEL;
	dx=(mptr->xmax[mtyp] - mptr->xmin[mtyp] + mptr->xstp)*mptr->xcopies - mptr->xstp;
	dy=(mptr->ymax[mtyp] - mptr->ymin[mtyp] + mptr->ystp)*mptr->ycopies - mptr->ystp;
	dz=mptr->zoff[mtyp]+(mptr->zmax[mtyp]-mptr->zmin[mtyp]);

	// define the model's bounding box
	xb[0]=mptr->xorg[mtyp];		yb[0]=0-mptr->yorg[mtyp];	zb[0]=mptr->zorg[mtyp];
	xb[1]=mptr->xorg[mtyp]+dx;	yb[1]=0-mptr->yorg[mtyp];	zb[1]=mptr->zorg[mtyp];
	xb[2]=mptr->xorg[mtyp]+dx;	yb[2]=0-(mptr->yorg[mtyp]+dy);	zb[2]=mptr->zorg[mtyp];
	xb[3]=mptr->xorg[mtyp];		yb[3]=0-(mptr->yorg[mtyp]+dy);	zb[3]=mptr->zorg[mtyp];
	xb[4]=mptr->xorg[mtyp];		yb[4]=0-mptr->yorg[mtyp];	zb[4]=mptr->zorg[mtyp]+dz;
	xb[5]=mptr->xorg[mtyp]+dx;	yb[5]=0-mptr->yorg[mtyp];	zb[5]=mptr->zorg[mtyp]+dz;
	xb[6]=mptr->xorg[mtyp]+dx;	yb[6]=0-(mptr->yorg[mtyp]+dy);	zb[6]=mptr->zorg[mtyp]+dz;
	xb[7]=mptr->xorg[mtyp];		yb[7]=0-(mptr->yorg[mtyp]+dy);	zb[7]=mptr->zorg[mtyp]+dz;

	for(j=0;j<12;j++)
	  {
	  if(j==0){i=0;h=1;}
	  if(j==1){i=1;h=2;}
	  if(j==2){i=2;h=3;}
	  if(j==3){i=3;h=0;}
	  if(j==4){i=4;h=5;}
	  if(j==5){i=5;h=6;}
	  if(j==6){i=6;h=7;}
	  if(j==7){i=7;h=4;}
	  if(j==8){i=0;h=4;}
	  if(j==9){i=1;h=5;}
	  if(j==10){i=2;h=6;}
	  if(j==11){i=3;h=7;}
	  zl=1+(600 + (MVview_scale*(xb[i]-MVdisp_cen_x))*oss - (MVview_scale*(yb[i]-MVdisp_cen_y))*osc + (MVview_scale*(zb[i]-MVdisp_cen_z))*ots)/600*pers_scale;
	  hl=((MVview_scale*(xb[i]-MVdisp_cen_x))*osc+(MVview_scale*(yb[i]-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	  vl=((MVview_scale*(xb[i]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[i]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[i]-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	  zh=1+(600 + (MVview_scale*(xb[h]-MVdisp_cen_x))*oss - (MVview_scale*(yb[h]-MVdisp_cen_y))*osc + (MVview_scale*(zb[h]-MVdisp_cen_z))*ots)/600*pers_scale;
	  hh=((MVview_scale*(xb[h]-MVdisp_cen_x))*osc+(MVview_scale*(yb[h]-MVdisp_cen_y))*oss)/zh+MVgxoffset;
	  vh=((MVview_scale*(xb[h]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[h]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[h]-MVdisp_cen_z))*otc)/zh+MVgyoffset;
	  cairo_move_to(cr,hl,vl);				
	  cairo_line_to(cr,hh,vh);	
	  cairo_stroke(cr);
	  }
	cairo_set_line_width (cr,0.5);
	
	// display model name at center of bounds
	x0=mptr->xorg[mtyp]+(dx/2);
	y0=(0-mptr->yorg[mtyp]+(dy/2));
	z0=mptr->zorg[mtyp]+(dz/2);
	zl=1+(600 + (MVview_scale*(x0-MVdisp_cen_x))*oss - (MVview_scale*(y0-MVdisp_cen_y))*osc + (MVview_scale*(z0-MVdisp_cen_z))*ots)/600*pers_scale;
	hl=((MVview_scale*(x0-MVdisp_cen_x))*osc + (MVview_scale*(y0-MVdisp_cen_y))*oss)/zl + MVgxoffset;
	vl=((MVview_scale*(x0-MVdisp_cen_x))*oss*ots - (MVview_scale*(y0-MVdisp_cen_y))*osc*ots + (MVview_scale*(z0-MVdisp_cen_z))*otc)/zl + MVgyoffset;
	cairo_move_to(cr,hl,vl);				
	sprintf(scratch,"%s",mptr->model_file);
	cairo_show_text (cr, scratch);				
	cairo_stroke(cr);
	}
	  
      // reset the color context back to that of the tool being used
      //if(model_slot>=0 && model_slot<MAX_TOOLS)gdk_cairo_set_source_rgba(cr, &mcolor[model_slot]);
      if(model_slot>=0 && model_slot<MAX_TOOLS)
	{
	//gtk_style_context_get_color(context, &mcolor[model_slot]);
	gdk_cairo_set_source_rgba (cr, &mcolor[model_slot]);
	}
  
      // loop thru all the geometry types for this model
      for(mtyp=MODEL;mtyp<MAX_MDL_TYPES;mtyp++)
        {
	if(mtyp==MODEL && set_view_models==FALSE)continue;
	if(mtyp==SUPPORT && set_view_supports==FALSE)continue;
	if(mtyp==INTERNAL && set_view_internal==FALSE)continue;
	if(mtyp==TARGET && set_view_target==FALSE)continue;
	if(mptr->input_type==STL && mptr->facet_qty[STL]==0)continue;
  
	// loop thru all copies of this model
	total_copy_ctr=0;
	for(x_copy_ctr=0;x_copy_ctr<mptr->xcopies;x_copy_ctr++)
	  {
	  copy_xoff=mptr->xorg[mtyp]+(x_copy_ctr*(mptr->xmax[mtyp]-mptr->xmin[mtyp]+mptr->xstp));
	  for(y_copy_ctr=0;y_copy_ctr<mptr->ycopies;y_copy_ctr++)
	    {
	    copy_yoff=mptr->yorg[mtyp]+(y_copy_ctr*(mptr->ymax[mtyp]-mptr->ymin[mtyp]+mptr->ystp));
	    if(total_copy_ctr>=mptr->total_copies)continue;
	    total_copy_ctr++;
	    view_copy=FALSE;
	    if(total_copy_ctr>1)view_copy=TRUE;
  
	    // reset display line width for 3D display
	    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	    cairo_set_line_width (cr,0.5);
  
	    // adjust behavior based on the amount of facets/edges to display
	    skip_facet_cnt=1;
	    if(mptr->facet_qty[MODEL] > set_view_max_facets*1000)
	      {
	      skip_facet_cnt=floor(mptr->facet_qty[MODEL]/(1000*set_view_max_facets));
	      if(skip_facet_cnt<1)skip_facet_cnt=1;
	      }
  
	    // display raw facets (STL facet wireframe) of all geometries
	    if(set_facet_display==FALSE && set_edge_display==FALSE)
	      {
	      vtx_total=0;
	      fptr=mptr->facet_first[mtyp];
	      while(fptr!=NULL)
		{
		// set color based on tool and errors
		if(mtyp==MODEL)
		  {
		  if(slot<0)						// if no tool is defined for this model...
		    {set_color(cr,&color,LT_GRAY);}			// ... display in gray
		  else 
		    {set_color(cr,&color,Tool[slot].matl.color);}	// ... otherwise display in tool mat'l color
		  }
		if(mtyp==SUPPORT)
		  {
		  {set_color(cr,&color,LT_RED);}			// ... display in red
		  }
		if(mtyp==INTERNAL)
		  {
		  {set_color(cr,&color,LT_CYAN);}			// ... display in cyan
		  }
		if(mtyp==TARGET)
		  {
		  {set_color(cr,&color,LT_BROWN);}			// ... display in brown
		  }
		
		facet_ok=TRUE;						// check if a free edge facet...
		for(i=0;i<3;i++){if(fptr->fct[i]==NULL)facet_ok=FALSE;}	// ... if any of the three neighbors are missing
		if(facet_ok==FALSE)set_color(cr,&color,MD_RED);		// ... highlight in red.
  
		// calculate start point of facet's 1st vtx
		vptr1=fptr->vtx[0];
		x1=vptr1->x+copy_xoff;
		y1=0-(vptr1->y+copy_yoff);
		z1=vptr1->z+mptr->zoff[mtyp];
		zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		hstart=hl; vstart=vl;
		cairo_move_to(cr,hl,vl);
  
		// calc and draw lines bt v0-v1 and v1-v2
		for(i=1;i<3;i++)
		  {
		  vptr2=fptr->vtx[i];
		  x2=vptr2->x+copy_xoff;
		  y2=0-(vptr2->y+copy_yoff);
		  z2=vptr2->z+mptr->zoff[mtyp];
		  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		  cairo_rel_line_to(cr,(hh-hl),(vh-vl));
		  hl=hh; vl=vh;
		  }
		cairo_rel_line_to(cr,(hstart-hl),(vstart-vl));		// draw line bt v2-v0  
		cairo_stroke(cr);
  
		// set options
		if(set_view_vecnum==TRUE && set_mouse_pick_entity==2)
		  {
		  for(i=0;i<3;i++)
		    {
		    vtx_total++;
		    vptr2=fptr->vtx[i];
		    x2=vptr2->x+copy_xoff;
		    y2=0-(vptr2->y+copy_yoff);
		    z2=vptr2->z+mptr->zoff[mtyp];
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hh,vh);
		    sprintf(scratch,"%5d",vtx_total);
		    cairo_show_text (cr, scratch);
		    }
		  cairo_stroke(cr);
		  }
		if(set_view_vecnum==TRUE && set_mouse_pick_entity==1)
		  {
		  vtx_total++;
		  facet_centroid(fptr,vtx_ctr);
		  vptr2=vtx_ctr;
		  x2=vptr2->x+copy_xoff;
		  y2=0-(vptr2->y+copy_yoff);
		  z2=vptr2->z+mptr->zoff[mtyp];
		  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		  cairo_move_to(cr,hh,vh);
		  sprintf(scratch,"%3d",fptr->attr);
		  cairo_show_text (cr, scratch);
		  cairo_stroke(cr);
		  }
	       
		// skip some facets if too many to display
		while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
		for(i=0;i<skip_facet_cnt;i++)
		  {
		  if(fptr!=NULL)fptr=fptr->next;
		  }
		}
	      }
  
	    // display shaded facets for all model types
	    if(set_facet_display==TRUE)
	      {
	      vtx_total=0;
	      fptr=mptr->facet_first[mtyp];
	      while(fptr!=NULL)
		{
		facet_centroid(fptr,Basevtx);
		Basevtx->x+=copy_xoff;
		Basevtx->y+=copy_yoff;
		Basevtx->z+=mptr->zoff[mtyp];
      
		Vvtx->x=(Basevtx->x+vwpt->x);
		Vvtx->y=(Basevtx->y+vwpt->y);
		Vvtx->z=(Basevtx->z+vwpt->z);
		ViewVec=vector_make(Basevtx,Vvtx,99);			// define view vector
		
		Avtx->x=(Basevtx->x+fptr->unit_norm->x);
		Avtx->y=(Basevtx->y+fptr->unit_norm->y);
		Avtx->z=(Basevtx->z+fptr->unit_norm->z);
		Avec=vector_make(Basevtx,Avtx,99);			// define facet unit normal vector
		
		disp_edge=FALSE;
		Adot=vector_dotproduct(ViewVec,Avec);
		if(Adot<0)disp_edge=TRUE; 				// if the facet faces the viewer
		disp_edge=TRUE;
		
		// and finally display the facets that passed the filters
		if(disp_edge==TRUE)
		  {
		  shade=(-1)*Adot;
		  if(shade<0.02)shade=0.02;
		  if(shade>1.00)shade=1.00;
		  if(set_view_normals==TRUE)
		    {
		    // draw Avec
		    if(mtyp==MODEL)set_color(cr,&color,MD_BLUE);
		    if(mtyp==SUPPORT)set_color(cr,&color,MD_RED);
		    if(mtyp==INTERNAL)set_color(cr,&color,MD_BLUE);
		    if(mtyp==TARGET)set_color(cr,&color,MD_GRAY);
		    x1=Avec->tip->x;  y1=0-Avec->tip->y;  z1=Avec->tip->z;
		    x2=Avec->tail->x; y2=0-Avec->tail->y; z2=Avec->tail->z;
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);	
		    //sprintf(scratch,"%4.3f,%4.3f,%4.3f",fptr->unit_norm->x,fptr->unit_norm->y,fptr->unit_norm->z);
		    //sprintf(scratch,"%4.3f",fptr->unit_norm->z);
		    //sprintf(scratch,"%4.3f",shade);
		    sprintf(scratch,"%4.3f",fptr->area);
		    //sprintf(scratch,"%d",fptr->member);
		    cairo_move_to(cr,hl,vl);				
		    cairo_show_text (cr, scratch);				
		    cairo_stroke(cr);
		    }
		  
		  // set color
		  if(slot<0)						// if no tool is defined for this model...
		    {set_color(cr,&color,LT_GRAY);}			// ... display in gray
		  else 
		    {
		    if(mtyp==MODEL)set_color(cr,&mcolor[slot],(shade*(-100)-100));
		    if(mtyp==SUPPORT)
		      {
		      if(ms_same_slot==TRUE)				// if same tool as model mat'l ...
			{
			set_color(cr,&color,SL_RED);			// ... display supports in red
			if((fptr->vtx[0])->j==99)set_color(cr,&color,MD_GREEN);
			}
		      else 						// otherwise if not same tool ...
			{set_color(cr,&mcolor[slot],(shade*(-100)-100));} // ... display in tool mat'l color
		      color.alpha=0.20;					// set support to be more transparent than model
		      gdk_cairo_set_source_rgba (cr,&color);
		      }
		    if(mtyp==INTERNAL)set_color(cr,&mcolor[slot],(shade*(-100)-100));
		    if(mtyp==TARGET)
		      {
		      set_color(cr,&color,LT_GRAY);
		      color.alpha=0.05;					// set block to be more transparent than model
		      gdk_cairo_set_source_rgba (cr,&color);
		      }
		    }
  
		  // set color of free edges on models only
		  if(mtyp==MODEL)
		    {
		    facet_ok=TRUE;					
		    for(i=0;i<3;i++){if(fptr->fct[i]==NULL)facet_ok=FALSE;}
		    if(facet_ok==FALSE)set_color(cr,&color,LT_RED);	
		    
		    // DEBUG
		    //if(fptr->supp==10)set_color(cr,&color,MD_BLUE);	
		    //if(fptr->supp==11)set_color(cr,&color,MD_CYAN);	
		    }
  
		  if(slot<0){max_angle=cos(-50*PI/180);}
		  else {max_angle=cos(Tool[slot].matl.overhang_angle*PI/180);}
  
		  // calculate start point of facet's 1st vtx
		  vptr1=fptr->vtx[0];
		  x1=vptr1->x+copy_xoff;
		  y1=0-(vptr1->y+copy_yoff);
		  z1=vptr1->z+mptr->zoff[mtyp];
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  hstart=hl; vstart=vl;
		  cairo_move_to(cr,hl,vl);
  
		  // calc and draw lines bt v0-v1 and v1-v2
		  for(i=1;i<3;i++)
		    {
		    vptr2=fptr->vtx[i];
		    x2=vptr2->x+copy_xoff;
		    y2=0-(vptr2->y+copy_yoff);
		    z2=vptr2->z+mptr->zoff[mtyp];
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_rel_line_to(cr,(hh-hl),(vh-vl));
		    hl=hh; vl=vh;
		    }
		    
		  // draw line bt v2-v0  
		  cairo_rel_line_to(cr,(hstart-hl),(vstart-vl));
		    
		  cairo_close_path (cr);
		  cairo_fill_preserve (cr);
		  cairo_stroke(cr);
		  
		  if(set_view_vecnum==TRUE && set_mouse_pick_entity==2)
		    {
		    for(i=0;i<3;i++)
		      {
		      vtx_total++;
		      vptr2=fptr->vtx[i];
		      x2=vptr2->x+copy_xoff;
		      y2=0-(vptr2->y+copy_yoff);
		      z2=vptr2->z+mptr->zoff[mtyp];
		      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		      cairo_move_to(cr,hh,vh);
		      sprintf(scratch,"%5d",vtx_total);
		      cairo_show_text (cr, scratch);
		      }
		    cairo_stroke(cr);
		    }
		  if(set_view_vecnum==TRUE && set_mouse_pick_entity==1)
		    {
		    vtx_total++;
		    facet_centroid(fptr,vtx_ctr);
		    vptr2=vtx_ctr;
		    x2=vptr2->x+copy_xoff;
		    y2=0-(vptr2->y+copy_yoff);
		    z2=vptr2->z+mptr->zoff[mtyp];
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hh,vh);
		    sprintf(scratch,"%3d",fptr->attr);
		    cairo_show_text (cr, scratch);
		    cairo_stroke(cr);
		    }
		  }
		
		while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
		for(i=0;i<skip_facet_cnt;i++)
		  {
		  if(fptr!=NULL)fptr=fptr->next;
		  }
		  
		free(ViewVec); vector_mem--;
		free(Avec); vector_mem--;
		}
		
	      // display SUPPORT upper and lower patch facets
	      if(set_view_patches==TRUE)
		{
		// if too many facets to rotate smoothly, just display bounding boxes
		if(set_view_bound_box==TRUE)
		  {
		  //define the model's bounding box
		  xb[0]=mptr->xmin[mtyp]+copy_xoff; yb[0]=0-(mptr->ymin[mtyp]+copy_yoff); zb[0]=mptr->zmin[mtyp]+mptr->zoff[mtyp];
		  xb[1]=mptr->xmax[mtyp]+copy_xoff; yb[1]=0-(mptr->ymin[mtyp]+copy_yoff); zb[1]=mptr->zmin[mtyp]+mptr->zoff[mtyp];
		  xb[2]=mptr->xmax[mtyp]+copy_xoff; yb[2]=0-(mptr->ymax[mtyp]+copy_yoff); zb[2]=mptr->zmin[mtyp]+mptr->zoff[mtyp];
		  xb[3]=mptr->xmin[mtyp]+copy_xoff; yb[3]=0-(mptr->ymax[mtyp]+copy_yoff); zb[3]=mptr->zmin[mtyp]+mptr->zoff[mtyp];
	
		  xb[4]=mptr->xmin[mtyp]+copy_xoff; yb[4]=0-(mptr->ymin[mtyp]+copy_yoff); zb[4]=mptr->zmax[mtyp]+mptr->zoff[mtyp];
		  xb[5]=mptr->xmax[mtyp]+copy_xoff; yb[5]=0-(mptr->ymin[mtyp]+copy_yoff); zb[5]=mptr->zmax[mtyp]+mptr->zoff[mtyp];
		  xb[6]=mptr->xmax[mtyp]+copy_xoff; yb[6]=0-(mptr->ymax[mtyp]+copy_yoff); zb[6]=mptr->zmax[mtyp]+mptr->zoff[mtyp];
		  xb[7]=mptr->xmin[mtyp]+copy_xoff; yb[7]=0-(mptr->ymax[mtyp]+copy_yoff); zb[7]=mptr->zmax[mtyp]+mptr->zoff[mtyp];
		  
		  for(j=0;j<12;j++)
		    {
		    if(j==0){i=0;h=1;}
		    if(j==1){i=1;h=2;}
		    if(j==2){i=2;h=3;}
		    if(j==3){i=3;h=0;}
		    if(j==4){i=4;h=5;}
		    if(j==5){i=5;h=6;}
		    if(j==6){i=6;h=7;}
		    if(j==7){i=7;h=4;}
		    if(j==8){i=0;h=4;}
		    if(j==9){i=1;h=5;}
		    if(j==10){i=2;h=6;}
		    if(j==11){i=3;h=7;}
		    set_color(cr,&color,DK_RED);
		    hl=(MVview_scale*(xb[i]-MVdisp_cen_x))*osc+(MVview_scale*(yb[i]-MVdisp_cen_y))*oss+MVgxoffset;
		    vl=(MVview_scale*(xb[i]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[i]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[i]-MVdisp_cen_z))*otc+MVgyoffset;
		    hh=(MVview_scale*(xb[h]-MVdisp_cen_x))*osc+(MVview_scale*(yb[h]-MVdisp_cen_y))*oss+MVgxoffset;
		    vh=(MVview_scale*(xb[h]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[h]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[h]-MVdisp_cen_z))*otc+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);	
		    cairo_stroke(cr);
		    }
		  
		  }
		// otherwise display all patch facets
		else 
		  {
		  // display upper and lower support surface patches
		  pat_ctr=0;
		  //patptr=mptr->patch_first_upr;
		  patptr=mptr->patch_model;
		  while(patptr!=NULL)
		    {
		    pat_ctr++;
		    for(j=0;j<2;j++)
		      {
		      pfptr=NULL;
		      if(j==0)						// upper patch
			{
			set_color(cr,&color,MD_GREEN);
			color.alpha=0.30;
			gdk_cairo_set_source_rgba (cr,&color);
			pfptr=patptr->pfacet;	
			}
		      if(j==1)						// lower child patch(es)
			{
			set_color(cr,&color,LT_VIOLET);
			color.alpha=0.30;
			gdk_cairo_set_source_rgba (cr,&color);
			if(patptr->pchild==NULL)continue;
			pfptr=(patptr->pchild)->pfacet;
			}
		      while(pfptr!=NULL)
			{
			lfptr=pfptr->f_item;
			for(i=0;i<3;i++)
			  {
			  if(i==0){vptr1=lfptr->vtx[0]; vptr2=lfptr->vtx[1];}
			  if(i==1){vptr1=lfptr->vtx[1]; vptr2=lfptr->vtx[2];}
			  if(i==2){vptr1=lfptr->vtx[2]; vptr2=lfptr->vtx[0];}
			  
			  x1=vptr1->x+copy_xoff;
			  y1=0-(vptr1->y+copy_yoff);
			  z1=vptr1->z+mptr->zoff[mtyp];
			  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	      
			  x2=vptr2->x+copy_xoff;
			  y2=0-(vptr2->y+copy_yoff);
			  z2=vptr2->z+mptr->zoff[mtyp];
			  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
			  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
			  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
			  
			  if(i==0){cairo_move_to(cr,hl,vl);}
			  cairo_rel_line_to(cr,(hh-hl),(vh-vl));
			  if(set_view_vecnum)
			    {
			    //facet_centroid(lfptr,Basevtx);
			    //x1=Basevtx->x+copy_xoff;
			    //y1=0-(Basevtx->y+copy_yoff);
			    //z1=Basevtx->z+mptr->zoff;
			    //hl=(MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss+MVgxoffset;
			    //vl=(MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc+MVgyoffset;
			    //cairo_move_to(cr,hl,vl);
			    //cairo_arc(cr,hl,vl,2,0.0,2*PI);	
			    //sprintf(scratch,"%4.2f",patptr->area);
			    sprintf(scratch,"%d",patptr->ID);
			    //if(j==1)sprintf(scratch,"%d",patptr->pchild->ID);
			    //sprintf(scratch,"%X",lfptr->vtx[i]);
			    //sprintf(scratch,"%6.3f",patptr->area)
			    //cairo_move_to(cr,hl+3,vl-3);
			    //sprintf(scratch,"%6.3f,%6.3f,%6.3f",vptr2->x,vptr2->y,vptr2->z);
			    cairo_move_to(cr,hh,vh);
			    cairo_show_text (cr, scratch);
			    cairo_move_to(cr,hh,vh);
			    }
			  }
			cairo_close_path (cr);
			cairo_fill_preserve (cr);
			cairo_stroke(cr);
			pfptr=pfptr->next;
			}
		      }
		    patptr=patptr->next;
		    }
		  }
		}		// end of if support patch display set
	      }		// end of if facet display set
  
	    // draw wireframe edges for all model types
	    if(set_edge_display==TRUE)
	      {
	      // only model facets have silhoette edges
	      if(mtyp==MODEL)
		{
		eptr=mptr->silho_edges;
		while(eptr!=NULL)
		  {
		  if(eptr->display==TRUE)
		    {
		    // set color
		    if(slot<0)
		      {set_color(cr,&color,DK_GRAY);}			// if not identified with a tool yet... leave as gray`
		    else 
		      //{set_color(cr,&mcolor[slot],(-1));}			// if identified with a specific tool... make tool color
		      {set_color(cr,&color,Tool[slot].matl.color);}
		    if(mptr==active_model)					// if the active model... highlight
		      {
		      if(job.state<JOB_RUNNING)set_color(cr,&color,DK_ORANGE);
		      if(job.state>=JOB_RUNNING)set_color(cr,&color,DK_ORANGE);
		      }
    
		    // draw 3D shape
		    if(eptr->Af==NULL || eptr->Bf==NULL)set_color(cr,&color,LT_RED);	// if a free edge, highlight in red
		    if(eptr->tip==NULL || eptr->tail==NULL){eptr=eptr->next; continue;}
		    x1=eptr->tip->x+copy_xoff;
		    y1=0-(eptr->tip->y+copy_yoff);
		    z1=eptr->tip->z+mptr->zoff[mtyp];
		    x2=eptr->tail->x+copy_xoff;
		    y2=0-(eptr->tail->y+copy_yoff);
		    z2=eptr->tail->z+mptr->zoff[mtyp];
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);
		    cairo_stroke(cr);
		    }
		  eptr=eptr->next;
		  }
		}

	      // all model types have "hard" edges that can be displayed
	      eptr=mptr->edge_first[mtyp];
	      while(eptr!=NULL)
		{
		if(eptr->display==TRUE)
		  {
		  // set color
		  if(slot<0)
		    {set_color(cr,&color,DK_GRAY);}
		  else 
		    {
		    if(mtyp==MODEL)set_color(cr,&color,Tool[slot].matl.color);
		    if(mtyp==SUPPORT)set_color(cr,&color,DK_RED);
		    if(mtyp==INTERNAL)set_color(cr,&color,(Tool[slot].matl.color-1));
		    if(mtyp==TARGET)set_color(cr,&color,LT_GRAY);
		    }
		  if(mtyp==MODEL && mptr==active_model)
		    {
		    if(job.state<JOB_RUNNING)set_color(cr,&color,MD_ORANGE);
		    if(job.state>=JOB_RUNNING)set_color(cr,&color,MD_ORANGE);
		    }
  
		  if(eptr->Af==NULL || eptr->Bf==NULL)set_color(cr,&color,LT_RED);	// if a free edge, highlight in red
		  if(eptr->tip==NULL || eptr->tail==NULL){eptr=eptr->next; continue;}
		  x1=eptr->tip->x+copy_xoff;
		  y1=0-(eptr->tip->y+copy_yoff);
		  z1=eptr->tip->z+mptr->zoff[mtyp];
		  x2=eptr->tail->x+copy_xoff;
		  y2=0-(eptr->tail->y+copy_yoff);
		  z2=eptr->tail->z+mptr->zoff[mtyp];
		  if(z1<0 && z2<0)set_color(cr,&color,SL_GRAY);
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		  cairo_move_to(cr,hl,vl);				
		  cairo_line_to(cr,hh,vh);
		  cairo_stroke(cr);
		  
		  // draw facet normals if requested
		  if(set_view_normals==TRUE)
		    {
		    if(eptr->Af==NULL || eptr->Bf==NULL){eptr=eptr->next; continue;}
		    if(eptr->Af->unit_norm==NULL || eptr->Bf->unit_norm==NULL){eptr=eptr->next; continue;}
		    if(eptr->Af->unit_norm->z>(-0.1)){set_color(cr,&color,DK_GREEN);}
		    else {set_color(cr,&color,MD_BLUE);}
		    if(eptr->tip==NULL || eptr->tail==NULL){eptr=eptr->next; continue;}
		    x1=eptr->tip->x+copy_xoff;
		    y1=0-(eptr->tip->y+copy_yoff);
		    z1=eptr->tip->z+mptr->zoff[mtyp];
		    x2=eptr->tip->x + 2*eptr->Af->unit_norm->x + copy_xoff;
		    y2=0-(eptr->tip->y + 2*eptr->Af->unit_norm->y + copy_yoff);
		    z2=eptr->tip->z + 2*eptr->Af->unit_norm->z + mptr->zoff[mtyp];
		    
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);
		    cairo_stroke(cr);
      
		    if(eptr->Bf->unit_norm->z>(-0.1)){set_color(cr,&color,DK_GREEN);}
		    else {set_color(cr,&color,DK_RED);}
		    x1=eptr->tail->x+copy_xoff;
		    y1=0-(eptr->tail->y+copy_yoff);
		    z1=eptr->tail->z+mptr->zoff[mtyp];
		    x2=eptr->tail->x + eptr->Bf->unit_norm->x + copy_xoff;
		    y2=0-(eptr->tail->y + eptr->Bf->unit_norm->y + copy_yoff);
		    z2=eptr->tail->z + eptr->Bf->unit_norm->z + mptr->zoff[mtyp];
		    
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);
		    cairo_stroke(cr);
		    }
		  
		  }
		eptr=eptr->next;
		//if(gtk_events_pending()==TRUE)break;
		}
	
	      // draw edge pick list
	      el_ptr=e_pick_list;
	      while(el_ptr!=NULL)
		{
		set_color(cr,&color,DK_BROWN);
		eptr=el_ptr->e_item;
		if(eptr==NULL){el_ptr=el_ptr->next; continue;}
		if(eptr->tip==NULL || eptr->tail==NULL){el_ptr=el_ptr->next; continue;}
		x1=eptr->tip->x+copy_xoff;
		y1=0-(eptr->tip->y+copy_yoff);
		z1=eptr->tip->z+mptr->zoff[mtyp];
		x2=eptr->tail->x+copy_xoff;
		y2=0-(eptr->tail->y+copy_yoff);
		z2=eptr->tail->z+mptr->zoff[mtyp];
		zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		cairo_move_to(cr,hl,vl);				
		cairo_line_to(cr,hh,vh);
		cairo_stroke(cr);
		el_ptr=el_ptr->next;
		}
		
	      // draw SUPPORT upper and lower patch wireframe edges
	      if(set_view_patches==TRUE)
		{
		patptr=mptr->patch_model;
		while(patptr!=NULL)
		  {
		  for(j=0;j<2;j++)
		    {
		    pptr=NULL;
		    if(j==0)						// upper parent patch
		      {
		      set_color(cr,&color,DK_GREEN);
		      color.alpha=1.0;
		      gdk_cairo_set_source_rgba (cr,&color);
		      pptr=patptr->free_edge;
		      }
		    if(j==1)						// lower child patch(es)
		      {
		      set_color(cr,&color,DK_BLUE);
		      color.alpha=1.0;
		      gdk_cairo_set_source_rgba (cr,&color);
		      if(patptr->pchild==NULL)continue;
		      pptr=(patptr->pchild)->free_edge;
		      }
		    //printf("ID=%d patptr=%X pptr=%X \n",patptr->ID,patptr,pptr);
		    while(pptr!=NULL)
		      {
		      vptr=pptr->vert_first;
		      while(vptr!=NULL)
		        {
			vnxt=vptr->next;
			if(vnxt==NULL)break;
			x1=vptr->x+copy_xoff;
			y1=0-(vptr->y+copy_yoff);
			z1=vptr->z+mptr->zoff[mtyp];
			x2=vnxt->x+copy_xoff;
			y2=0-(vnxt->y+copy_yoff);
			z2=vnxt->z+mptr->zoff[mtyp];
			zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
			zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
			hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
			vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
			cairo_move_to(cr,hl,vl);
			if(vptr->supp==1){cairo_arc(cr,hl,vl,4,0.0,2*PI);}
			else {cairo_arc(cr,hl,vl,2,0.0,2*PI);}		
			cairo_move_to(cr,hl,vl);
			cairo_line_to(cr,hh,vh);
			cairo_stroke(cr);
			
		      /*
		      vold=pptr->vert_first;
		      if(vold==NULL){pptr=pptr->next;continue;}
		      x1=vold->x+copy_xoff;
		      y1=0-(vold->y+copy_yoff);
		      z1=vold->z+mptr->zoff[mtyp];
		      x0=x1; y0=y1; z0=z1;
		      vptr=vold->next;
		      while(vptr!=NULL)
			{
			x2=vptr->x+copy_xoff;
			y2=0-(vptr->y+copy_yoff);
			z2=vptr->z+mptr->zoff[mtyp];
			zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
			zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
			hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
			vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
			cairo_move_to(cr,hl,vl);				
			cairo_line_to(cr,hh,vh);
			cairo_stroke(cr);
			*/
			
			if(set_view_endpts==TRUE)
			  {
			  //cairo_move_to(cr,hl,vl);
			  //cairo_arc(cr,hl,vl,2,0.0,2*PI);			// draw circle at tip vertex
			  //cairo_move_to(cr,hh,vh);
			  //cairo_arc(cr,hh,vh,2,0.0,2*PI);			// draw circle at tail vertex
			  //cairo_stroke(cr);
    
			  cairo_move_to(cr,hl,vl);
			  cairo_arc(cr,hl,vl,2,0.0,2*PI);		// draw circle at tail (base) vertex
			  arrow_angle=20*PI/180;				// angle of arrow head
			  arrow_length=12;					// size of arrow head
			  ax1=hh-hl;
			  ay1=vh-vl;
			  if(fabs(ax1)>0){vec_angle=atan2f(ay1,ax1);}
			  else {vec_angle=PI/2;if(ay1<0)vec_angle=(-PI/2);}
			  ax1 = hh - arrow_length * cos(vec_angle - arrow_angle);
			  ay1 = vh - arrow_length * sin(vec_angle - arrow_angle);
			  ax2 = hh - arrow_length * cos(vec_angle + arrow_angle);
			  ay2 = vh - arrow_length * sin(vec_angle + arrow_angle);	      
			  cairo_move_to(cr,hh,vh);
			  cairo_line_to(cr,ax1,ay1);			// draw arrow head at tip vertex
			  cairo_line_to(cr,ax2,ay2);
			  cairo_line_to(cr,hh,vh);
			  cairo_move_to(cr,hh,vh);
			  cairo_stroke(cr);
			  }
			if(set_view_vecnum==TRUE)
			  {
			  //sprintf(scratch,"%6.3f,%6.3f,%6.3f",vptr->x,vptr->y,vptr->z);
			  //cairo_move_to(cr,hh,vh);
			  //cairo_show_text (cr, scratch);
			  //cairo_stroke(cr);
			  }
			
			vptr=vptr->next;
			if(vptr==pptr->vert_first)break;
			}		// end of vtx list loop
		      // draw connector line for last vtx back to first vtx
		      /*
		      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		      cairo_move_to(cr,hl,vl);				
		      cairo_line_to(cr,hh,vh);
		      cairo_stroke(cr);
		      */
		      
		      // display patch and/or polygon level data
		      if(set_view_vecnum==TRUE)
			{
			x1=pptr->centx+copy_xoff;
			y1=0-(pptr->centy+copy_yoff);
			z1=pptr->vert_first->z+mptr->zoff[mtyp];
			zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
			//sprintf(scratch,"%d",pptr->hole);
			//sprintf(scratch,"%6.3f",pptr->area);
			if(j==0)sprintf(scratch,"%d",patptr->ID);
			if(j==1)sprintf(scratch,"%d",patptr->pchild->ID);
			cairo_move_to(cr,hl,vl);
			cairo_show_text (cr, scratch);
			cairo_move_to(cr,hh,vh);
			}
    
		      set_color(cr,&color,DK_YELLOW);
		      pptr=pptr->next;
		      }
		    }		// end of for loop
		    
		  patptr=patptr->next;
		  } 		// end of patptr loop
		}			// end of if
    
	      }
  
	    // draw support tree branches
	    if(set_view_patches==TRUE && job.support_tree!=NULL)
	      {
		
	      // highlight primary branch of interest (DEBUG)
	      /*
	      set_color(cr,&color,LT_RED);
	      brptr=job.support_tree;
	      while(brptr!=NULL)
	        {
		if(brptr->wght==99)
		  {
		  vptr=brptr->goal;
		  x1=vptr->x+copy_xoff;
		  y1=0-(vptr->y+copy_yoff);
		  z1=vptr->z+mptr->zoff[mtyp];
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  cairo_move_to(cr,hl,vl);
		  cairo_arc(cr,hl,vl,4,0.0,2*PI);
		  sprintf(scratch,"P");
		  cairo_move_to(cr,hl,vl+5);
		  cairo_show_text (cr, scratch);				
		  cairo_stroke(cr);
		  vptr=vptr->next;
		  }
		if(brptr->wght==88)
		  {
		  vptr=brptr->goal;
		  x1=vptr->x+copy_xoff;
		  y1=0-(vptr->y+copy_yoff);
		  z1=vptr->z+mptr->zoff[mtyp];
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  cairo_move_to(cr,hl,vl);
		  cairo_arc(cr,hl,vl,3,0.0,2*PI);
		  sprintf(scratch,"%6.3f",brptr->node->i);
		  cairo_move_to(cr,hl,vl+5);
		  cairo_show_text (cr, scratch);				
		  cairo_stroke(cr);
		  vptr=vptr->next;
		  }
		brptr=brptr->next;
		}
	      cairo_stroke(cr);
	      */
	      
		
	      // draw branch goals
	      /*
	      set_color(cr,&color,MD_BLUE);
	      brptr=job.support_tree;
	      while(brptr!=NULL)
	        {
		vptr=brptr->goal;
		while(vptr!=NULL)
		  {
		  x1=vptr->x+copy_xoff;
		  y1=0-(vptr->y+copy_yoff);
		  z1=vptr->z+mptr->zoff[mtyp];
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  cairo_move_to(cr,hl,vl);
		  cairo_arc(cr,hl,vl,2,0.0,2*PI);
		  sprintf(scratch,"%6.3f",brptr->goal->i);
		  cairo_move_to(cr,hl,vl-10);
		  cairo_show_text (cr, scratch);				
		  cairo_stroke(cr);
		  vptr=vptr->next;
		  }
		cairo_stroke(cr);
		brptr=brptr->next;
		}
	      */
		  
	      // draw tree branch centerlines
	      set_color(cr,&color,MD_VIOLET);
	      brptr=job.support_tree;
	      while(brptr!=NULL)
	        {
		// draw centerline of branch
		vptr=brptr->node;
		while(vptr!=NULL)
		  {
		  vnxt=vptr->next;
		  if(vnxt==NULL)break;
		  x1=vptr->x+copy_xoff;
		  y1=0-(vptr->y+copy_yoff);
		  z1=vptr->z+mptr->zoff[mtyp];
		  x2=vnxt->x+copy_xoff;
		  y2=0-(vnxt->y+copy_yoff);
		  z2=vnxt->z+mptr->zoff[mtyp];
		  if(z1<0 && z2<0)set_color(cr,&color,SL_GRAY);
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		  cairo_move_to(cr,hl,vl);				
		  cairo_line_to(cr,hh,vh);
		  cairo_arc(cr,hh,vh,3,0.0,2*PI);
		  //sprintf(scratch,"%6.3f",brptr->node->z);
		  //cairo_move_to(cr,hl,vl-5);
		  //cairo_show_text (cr, scratch);				
		  cairo_stroke(cr);
		  vptr=vptr->next;
		  }
		cairo_stroke(cr);
		brptr=brptr->next;
		}
		  
	      // draw polygons of branch
	      set_color(cr,&color,MD_RED);
	      brptr=job.support_tree;
	      while(brptr!=NULL)
	        {
		pptr=brptr->pfirst;
		while(pptr!=NULL)
		  {
		  vptr=pptr->vert_first;
		  while(vptr!=NULL)
		    {
		    vnxt=vptr->next;
		    if(vnxt==NULL)break;
		    x1=vptr->x+copy_xoff;
		    y1=0-(vptr->y+copy_yoff);
		    z1=vptr->z+mptr->zoff[mtyp];
		    x2=vnxt->x+copy_xoff;
		    y2=0-(vnxt->y+copy_yoff);
		    z2=vnxt->z+mptr->zoff[mtyp];
		    if(z1<0 && z2<0)set_color(cr,&color,SL_GRAY);
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);
		    cairo_stroke(cr);
		    vptr=vptr->next;
		    if(vptr==pptr->vert_first)break;
		    }
		  pptr=pptr->next;
		  }
		cairo_stroke(cr);
		brptr=brptr->next;
		}

	      }
  
	    // draw vertex pick reference if it is non-zero
	    if(vtx_pick_ref!=NULL)
	      {
	      /*
	      if(fabs(vtx_pick_ref->x+vtx_pick_ref->y+vtx_pick_ref->z)>TOLERANCE)
		{
		set_color(cr,&color,MD_VIOLET);
		x1=vtx_pick_ref->x+copy_xoff;
		y1=0-(vtx_pick_ref->y+copy_yoff);
		z1=vtx_pick_ref->z+mptr->zoff;
		hl=(MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss+MVgxoffset;
		vl=(MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc+MVgyoffset;
		cairo_move_to(cr,hl,vl);
		cairo_arc(cr,hl,vl,3,0.0,2*PI);
		sprintf(scratch,"%6.3f,%6.3f,%6.3f",vtx_pick_ref->x,vtx_pick_ref->y,vtx_pick_ref->z);
		cairo_move_to(cr,hl,vl-10);
		cairo_show_text (cr, scratch);				
		sprintf(scratch,"%d",vtx_pick_ref->attr);
		cairo_move_to(cr,hl,vl-20);
		cairo_show_text (cr, scratch);				
		cairo_stroke(cr);
		}
	      */
	      }
      
	    // draw vertex pick new if it is non-zero
	    if(vtx_pick_new!=NULL && vtx_pick_new!=vtx_pick_ref)
	      {
	      if(fabs(vtx_pick_new->x+vtx_pick_new->y+vtx_pick_new->z)>TOLERANCE)
		{
		// highlight the selected vertex
		set_color(cr,&color,MD_RED);
		x1=vtx_pick_new->x+copy_xoff;
		y1=0-(vtx_pick_new->y+copy_yoff);
		z1=vtx_pick_new->z+mptr->zoff[mtyp];
		zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		cairo_move_to(cr,hl,vl);
		cairo_arc(cr,hl,vl,3,0.0,2*PI);
		cairo_stroke(cr);
		vtx_pick_new->x=0;vtx_pick_new->y=0;vtx_pick_new->z=0;
		
		// highlight the selected vtx's facets
		if(set_show_pick_neighbors==TRUE)
		  {
		  set_color(cr,&color,LT_YELLOW);
		  flptr=vtx_pick_new->flist;
		  while(flptr!=NULL)
		    {
		    fptr=flptr->f_item;
		    if(fptr==NULL){flptr=flptr->next;continue;}
		    if(fptr->vtx[0]==NULL || fptr->vtx[1]==NULL || fptr->vtx[2]==NULL){flptr=flptr->next;continue;}
		    for(i=0;i<3;i++)
		      {
		      if(i==0){vptr1=fptr->vtx[0]; vptr2=fptr->vtx[1];}
		      if(i==1){vptr1=fptr->vtx[1]; vptr2=fptr->vtx[2];}
		      if(i==2){vptr1=fptr->vtx[2]; vptr2=fptr->vtx[0];}
		      
		      x1=vptr1->x+copy_xoff;
		      y1=0-(vptr1->y+copy_yoff);
		      z1=vptr1->z+mptr->zoff[mtyp];
		      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	  
		      x2=vptr2->x+copy_xoff;
		      y2=0-(vptr2->y+copy_yoff);
		      z2=vptr2->z+mptr->zoff[mtyp];
		      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		      
		      if(i==0){cairo_move_to(cr,hl,vl);}
		      cairo_rel_line_to(cr,(hh-hl),(vh-vl));
		      }
		    cairo_close_path (cr);
		    cairo_fill_preserve (cr);
		    flptr=flptr->next;
		    }
		  cairo_stroke(cr);
		  }
		// now null out the pick as it has become the vtx_pick_ref
		if(set_show_pick_clear==TRUE)vtx_pick_new->flist=NULL;
		}
	      }
   
	    // draw facet pick reference if it is not null
	    if(fct_pick_ref!=NULL)
	      {
	      set_color(cr,&color,DK_YELLOW);
	      for(h=0;h<4;h++)
		{
		if(h>0 && set_show_pick_neighbors==FALSE)break;
		if(h==0)fptr=fct_pick_ref;
		if(h==1)fptr=fct_pick_ref->fct[0];
		if(h==2)fptr=fct_pick_ref->fct[1];
		if(h==3)fptr=fct_pick_ref->fct[2];
		if(fptr==NULL)continue;
		if(h==0)set_color(cr,&color,DK_YELLOW);
		if(h>=1)set_color(cr,&color,DK_GREEN);
		if(fptr->vtx[0]==NULL || fptr->vtx[1]==NULL || fptr->vtx[2]==NULL)continue;
		for(i=0;i<3;i++)
		  {
		  if(i==0){vptr1=fptr->vtx[0]; vptr2=fptr->vtx[1];}
		  if(i==1){vptr1=fptr->vtx[1]; vptr2=fptr->vtx[2];}
		  if(i==2){vptr1=fptr->vtx[2]; vptr2=fptr->vtx[0];}
		  
		  x1=vptr1->x+copy_xoff;
		  y1=0-(vptr1->y+copy_yoff);
		  z1=vptr1->z+mptr->zoff[mtyp];
		  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		  hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		  vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      
		  x2=vptr2->x+copy_xoff;
		  y2=0-(vptr2->y+copy_yoff);
		  z2=vptr2->z+mptr->zoff[mtyp];
		  zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
		  hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		  vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		  
		  if(i==0){cairo_move_to(cr,hl,vl);}
		  cairo_rel_line_to(cr,(hh-hl),(vh-vl));
		  }
		cairo_close_path (cr);
		cairo_fill_preserve (cr);
		cairo_stroke(cr);
		}
	      }
      
	    // draw slice if requested
	    //if(set_view_mdl_slice==TRUE && mtyp==MODEL)
	    if(set_view_mdl_slice==TRUE)
	      {
	      if(slot<0){amptr=NULL;}
	      else {amptr=mptr->oper_list;}
	      while(amptr!=NULL)
		{
		//printf("Checking operation..: %d %s \n",amptr->ID,amptr->name);
		slot=amptr->ID;							// find which tool is used for this operation
		if(slot<0 || slot>=MAX_TOOLS){amptr=amptr->next;continue;}	// if no tool specified for this operation, skip
		if(Tool[slot].state<TL_LOADED){amptr=amptr->next;continue;}	// if tool not fully specified, skip
      
		// set slice range
		z_view_start=z_cut-slc_view_start;
		if(z_view_start<job.ZMin)z_view_start=job.ZMin;
		z_view_end=z_cut+slc_view_end+CLOSE_ENOUGH;
		if(z_view_end>(job.ZMax+CLOSE_ENOUGH))z_view_end=job.ZMax+CLOSE_ENOUGH;
		
		//printf("MV: z_cut=%f  slc_st=%f  zv_st=%f  slc_ed=%f  zv_ed=%f \n",z_cut,slc_view_start,z_view_start,slc_view_end,z_view_end);
		
		// loop thru N slices worth of model, displaying what was set by user (scnt="slice count")
		// recall that slice z levels are relative to the model, NOT location in the build volume.
		// that is, the slices run from zmin[MODEL] thru zmax[MODEL] regardless of zoff[MODEL]
		for(slc_z_cut=z_view_start; slc_z_cut<z_view_end; slc_z_cut+=slc_view_inc)
		  {

		  sptr=slice_find(mptr,mtyp,slot,slc_z_cut,job.min_slice_thk);		// find existing slice at this z level

		  //printf("MV: slc_z=%f  zoff=%f  mdl_z=%f \n",slc_z_cut,mptr->zoff[mtyp],(slc_z_cut-mptr->zoff[mtyp]));
		  //printf("MV: mptr=%X mtyp=%d slot=%d sptr=%X \n",mptr,mtyp,slot,sptr);
		  //if(sptr!=NULL)printf(" sz_level=%f \n\n",sptr->sz_level);
		  if(sptr==NULL)							// if not slice at this z level, move onto next operation
		    {
		    //printf("MV: sptr=NULL  slc_z=%f  zoff=%f  mdl_z=%f \n",slc_z_cut,mptr->zoff[mtyp],(slc_z_cut-mptr->zoff[mtyp]));
		    continue;					
		    }

		  // loop thru all operations of this tool
		  optr=Tool[slot].oper_first;
		  while(optr!=NULL)
		    {
		    if(strstr(amptr->name,optr->name)==NULL){optr=optr->next;continue;}	// if this operation was not called out by the model... skip it
		    //printf("Processing operation: %d %s \n",optr->ID,optr->name);
		    lptr=optr->lt_seq;							// get first line type called out in generic list sequence
		    while(lptr!=NULL)  							// loop thru the sequence in order
		      {
		      //printf("Processing line type: %d %s \n",lptr->ID,ltptr->name);
		      //printf("                      %d %X \n",sptr->pqty[ptyp],sptr->pfirst[ptyp]);
		      ptyp=lptr->ID;	
		      if(ptyp<0 || ptyp>=MAX_LINE_TYPES){lptr=lptr->next;continue;}
		      if(set_view_lt[ptyp]==FALSE){lptr=lptr->next;continue;}		// only display the requested line types
		      if(sptr->pqty[ptyp]<=0){lptr=lptr->next;continue;}		// if nothing to process... move on
		
		      // set display color
		      //set_color(cr,&color,BLACK);
		      //set_color(cr,&mcolor[slot],(-1));				// note "slot" may change depending on op type even if same mptr
		      set_color(cr,&color,Tool[slot].matl.color);
		      
		      // set display line width based on line type line width
		      cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		      cairo_set_line_width(cr, 0.5);
		      linetype_ptr=linetype_find(slot,ptyp);				// get actual linetype ptr... note lptr is a generic list ptr
		      if(linetype_ptr==NULL){lptr=lptr->next;continue;}
		      if(set_view_model_lt_width==TRUE)
			{
			if((ptyp>=MDL_PERIM && ptyp<=MDL_LAYER_1) || (ptyp>=BASELYR && ptyp<=PLATFORMLYR2))
			  {
			  cairo_set_line_width (cr,linetype_ptr->line_width*MVview_scale*0.95);
			  if(ptyp==MDL_BORDER && z_cut<(mptr->slice_thick*1.1))
			    {
			    linetype_ptr=linetype_find(slot,MDL_LAYER_1);
			    if(linetype_ptr!=NULL)cairo_set_line_width (cr,linetype_ptr->line_width*MVview_scale*0.95);
			    linetype_ptr=linetype_find(slot,ptyp);
			    }
			  }
			}
		      if(set_view_support_lt_width==TRUE)
			{
			if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)
			  {
			  cairo_set_line_width (cr,linetype_ptr->line_width*MVview_scale*0.95);
			  if(ptyp==SPT_OFFSET && z_cut<(mptr->slice_thick*1.1))
			    {
			    linetype_ptr=linetype_find(slot,SPT_LAYER_1);
			    if(linetype_ptr!=NULL)cairo_set_line_width (cr,linetype_ptr->line_width*MVview_scale*0.95);
			    linetype_ptr=linetype_find(slot,ptyp);
			    }
			  }
			}
  
		      // colorize supports as red
		      if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)set_color(cr,&color,DK_RED);
		      
		      pptr=sptr->pfirst[ptyp];
		      while(pptr!=NULL)
			{
			// start with a pen-up to the start of the polygon
			vtx_count=0;
			vptr=pptr->vert_first;						// start at first vertex
			if(vptr==NULL){pptr=pptr->next; continue;}
			x1=vptr->x+copy_xoff;
			y1=0-(vptr->y+copy_yoff);
			z1=sptr->sz_level+mptr->zoff[mtyp];
			zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			oldx=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			oldy=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
			cairo_move_to(cr,oldx,oldy);
		
			// draw vertices of the current polygon
			vptr=vptr->next;						// since we already moved to vptr
			while(vptr!=NULL)
			  {
			  // speed things up a bit by skipping vtxs if zoomed out
			  /*
			  skip_vtx=floor(10.0/MVview_scale);			// skips N vtxs when zoom=1.0
			  if(skip_vtx>floor(pptr->vert_qty/20))skip_vtx=floor(pptr->vert_qty/20);
			  if(pptr->vert_qty<100)skip_vtx=0;
			  if(skip_vtx<1)skip_vtx=0;
			  while(skip_vtx>0)					// loop thru undisplayed vtxs
			    {
			    skip_vtx--;						// decrement counter
			    vtx_count++;					// increment vtx total counter
			    vptr=vptr->next;					// move onto next vertex
			    if(vptr==NULL)break;				// end if open polygon
			    if(vptr==pptr->vert_first)break;			// end if contour is closed polygon
			    }
			  if(vptr==NULL)break;					// if we already hit the end of the vtx list...
			  */

			  // if this vertex has been flagged as sent to TinyG already...
			  if(vptr->attr<0)				
			    {
			    if(vptr->attr==(-1))set_color(cr,&color,MD_GRAY);
			    if(vptr->attr==(-2))set_color(cr,&color,MD_GRAY);
			    }
			  
			  // calculate new position and draw entity
			  x1=vptr->x+copy_xoff;
			  y1=0-(vptr->y+copy_yoff);
			  z1=sptr->sz_level+mptr->zoff[mtyp];
			  zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
			  newx=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
			  newy=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;

			  // adjust color for grayscale slice display on a vtx by vtx basis
			  if(mptr->grayscale_mode>0)
			    {
			    if(mptr->grayscale_mode==1)			// if pwm is modulated...
			      {
			      color.red  =1.0-vptr->i; 			// ... higher duty cycle = darker but opposite on display
			      color.blue =1.0-vptr->i;
			      color.green=1.0-vptr->i;
			      color.alpha=0.5;
			      }
			    if(mptr->grayscale_mode==2)			// if z is modulated...
			      {
			      color.red  =vptr->z/mptr->grayscale_zdelta;   	// ... reduce back to duty cycle
			      color.blue =vptr->z/mptr->grayscale_zdelta;
			      color.green=vptr->z/mptr->grayscale_zdelta; 
			      color.alpha=0.9;
			      }
			    if(mptr->grayscale_mode==3)			// if dwell is modulated...
			      {
			      color.red  =vptr->i*mptr->grayscale_velmin;   	// ... reduce back to duty cycle
			      color.blue =vptr->i*mptr->grayscale_velmin;
			      color.green=vptr->i*mptr->grayscale_velmin; 
			      color.alpha=0.5;
			      }
			    gdk_cairo_set_source_rgba (cr, &color);
			    cairo_move_to(cr,oldx,oldy);
			    cairo_line_to(cr,newx,newy);
			    cairo_stroke(cr);  
			    }
			  else 
			    {
			    cairo_move_to(cr,oldx,oldy);
			    cairo_line_to(cr,newx,newy);
			    }
			  if(set_view_endpts==TRUE)
			    {
			    cairo_move_to(cr,oldx,oldy);
			    cairo_arc(cr,oldx,oldy,2,0.0,2*PI);		// draw circle at tail (base) vertex
			    cairo_move_to(cr,newx,newy);
			    }
			  if(set_view_vecnum==TRUE)
			    {
			    vtx_count++;
			    x1=(oldx+newx)/2-5;
			    y1=(oldy+newy)/2;
			    sprintf(scratch,"%5d",vtx_count);
			    cairo_move_to(cr,x1,y1);
			    cairo_show_text (cr, scratch);
			    cairo_move_to(cr,newx,newy);
			    }
			  oldx=newx;
			  oldy=newy;
			  vptr=vptr->next;					// move onto next vertex
			  if(vptr==NULL)break;
			  if(vptr==pptr->vert_first->next)break;		// end if contour is closed polygon
			  }
			cairo_stroke(cr);  
			
			pptr=pptr->next;					// move on to next polygon
			}
		      lptr=lptr->next;						// move on to next line type
		      }
		    optr=optr->next;						// move on to next tool operation
		    }
		  }								// end of for slc_z_cut loop
		amptr=amptr->next;						// move on to next model operation
		}	// end of model operation loop
	      }	// end of if set_view_mdl_slice
	      
	    // process images if any are present in this model
	    if(mptr->g_img_act_buff!=NULL && mtyp==MODEL)
	      {
	      // draw source image onto surface if requested
	      if(set_view_image==TRUE)
		{
		// do this by looping thru the entire image pixel by pixel and mapping them from their 2D position
		// onto the top surface of their TARGET in 3D.
		pix_step=floor(mptr->act_width/250*MVview_scale);		// ratio of pixels in image to pixels available on display
		if(pix_step<1)pix_step=1;
		if(pix_step>10)pix_step=10;
		for(ypix=0; ypix<mptr->act_height; ypix+=pix_step)
		  {
		  for(xpix=0; xpix<mptr->act_width; xpix+=pix_step)
		    {
		    // load pixel from mdl
		    pix1 = mptr->act_pixels + ypix * mptr->act_rowstride + xpix * mptr->act_n_ch;
		    //printf("%d %d %d %d \n",pix1[0],pix1[1],pix1[2],pix1[3]);
		    
		    // set color based on input pixel
		    color.red  =(float)(pix1[0])/255.0;
		    color.green=(float)(pix1[1])/255.0;
		    color.blue =(float)(pix1[2])/255.0;
		    color.alpha=1.0;
		    if(mptr->reslice_flag==FALSE)color.alpha=0.25;		// if sliced, make it mostly transparent
		    gdk_cairo_set_source_rgba (cr,&color);
  
		    // calc position of pixel on model
		    x1=xpix*mptr->pix_size+copy_xoff;
		    y1=0-(ypix*mptr->pix_size+copy_yoff);
		    z1=mptr->zmax[TARGET];
		    //z1=cnc_z_height;
		    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    
		    //draw pixel
		    cairo_move_to(cr,hl,vl);
		    cairo_arc(cr,hl,vl,2.0,0.0,2*PI);
		    cairo_close_path (cr);
		    cairo_fill_preserve (cr);
		    cairo_stroke(cr);	
		    
		    }	// end of xpix loop
		  }	// end of ypix loop
	     
		// draw selected highligh around image if set
		if(mptr==active_model)
		  {
		  set_color(cr,&color,DK_ORANGE);
		  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
		  cairo_set_line_width(cr, 3.0);
		  dx=mptr->act_width*mptr->pix_size;
		  dy=mptr->act_height*mptr->pix_size;
		  dz=0;
		  xb[0]=mptr->xorg[mtyp];	yb[0]=0-mptr->yorg[mtyp];	zb[0]=mptr->zorg[mtyp]+dz;
		  xb[1]=mptr->xorg[mtyp]+dx;	yb[1]=0-mptr->yorg[mtyp];	zb[1]=mptr->zorg[mtyp]+dz;
		  xb[2]=mptr->xorg[mtyp]+dx;	yb[2]=0-(mptr->yorg[mtyp]+dy);	zb[2]=mptr->zorg[mtyp]+dz;
		  xb[3]=mptr->xorg[mtyp];	yb[3]=0-(mptr->yorg[mtyp]+dy);	zb[3]=mptr->zorg[mtyp]+dz;
		  for(j=0;j<4;j++)
		    {
		    if(j==0){i=0;h=1;}
		    if(j==1){i=1;h=2;}
		    if(j==2){i=2;h=3;}
		    if(j==3){i=3;h=0;}
		    zl=1+(600 + (MVview_scale*(xb[i]-MVdisp_cen_x))*oss - (MVview_scale*(yb[i]-MVdisp_cen_y))*osc + (MVview_scale*(zb[i]-MVdisp_cen_z))*ots)/600*pers_scale;
		    hl=((MVview_scale*(xb[i]-MVdisp_cen_x))*osc+(MVview_scale*(yb[i]-MVdisp_cen_y))*oss)/zl+MVgxoffset;
		    vl=((MVview_scale*(xb[i]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[i]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[i]-MVdisp_cen_z))*otc)/zl+MVgyoffset;
		    zh=1+(600 + (MVview_scale*(xb[h]-MVdisp_cen_x))*oss - (MVview_scale*(yb[h]-MVdisp_cen_y))*osc + (MVview_scale*(zb[h]-MVdisp_cen_z))*ots)/600*pers_scale;
		    hh=((MVview_scale*(xb[h]-MVdisp_cen_x))*osc+(MVview_scale*(yb[h]-MVdisp_cen_y))*oss)/zh+MVgxoffset;
		    vh=((MVview_scale*(xb[h]-MVdisp_cen_x))*oss*ots-(MVview_scale*(yb[h]-MVdisp_cen_y))*osc*ots+(MVview_scale*(zb[h]-MVdisp_cen_z))*otc)/zh+MVgyoffset;
		    cairo_move_to(cr,hl,vl);				
		    cairo_line_to(cr,hh,vh);	
		    cairo_stroke(cr);
		    }
		  }
		}	// end of if set_view_image=TRUE
	      }		// end of if g_img_act_buf!=NULL

	    }	// end of y copy loop
	  }	// end of x copy loop
        }	// end of for mtyp=MODEL loop
      mptr=mptr->next;							// and go on to next model
      }
    free(vtx_ctr); 	vertex_mem--;
    free(Vvtx);		vertex_mem--;
    free(Avtx);		vertex_mem--;
    free(Basevtx);	vertex_mem--;
    free(vwpt);		vertex_mem--;
    free(vtool_pos); 	vertex_mem--;

    return(TRUE);							// stop propagation of the draw signal here
}

static gboolean on_model_area_left_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    int		i,windnum,slot,mtyp=MODEL;
    int		mouse_x,mouse_y;
    float 	x1,y1,z1;
    float 	dix,diy,diz;
    float 	dx,dy,dz,dzmin;
    float 	oss,osc,ots,otc;
    vertex	*posptr,*vpick;
    facet	*ltfptr,*fptr,*fpick;
    facet_list	*flptr;
    slice	*scrt;
    model 	*mptr,*mpick;
    vertex	*vptr,*vnxt,*vscr[4];
    vertex_list	*vl_list,*vl_ptr;
    
    if(job.state==JOB_RUNNING)return(TRUE);				// don't interfere with active model while running

    // capture coords first thing.  it seems they can become corrupted otherwise.
    mouse_x=x;
    mouse_y=y;
	
    // pre-calculate tilt and spin values
    oss=sin(MVdisp_spin);
    osc=cos(MVdisp_spin);
    ots=sin(MVdisp_tilt);
    otc=cos(MVdisp_tilt);
    if(job.state>=JOB_RUNNING)return(TRUE);				// don't reset active model during build!				
    mpick=NULL;
    active_model=NULL;
    if(set_mouse_select || set_mouse_align)
      {
      dzmin=(5.0*MVview_scale);
      mpick=NULL;

      /*
      // check if mouse pick is withing bounds of each model
      mptr=job.model_first;
      while(mptr!=NULL)
	{
	// establish size of this model and its copies
	dix=(mptr->xmax[mtyp] - mptr->xmin[mtyp] + mptr->xstp)*mptr->xcopies - mptr->xstp;
	diy=(mptr->ymax[mtyp] - mptr->ymin[mtyp] + mptr->ystp)*mptr->ycopies - mptr->ystp;
	diz=mptr->zmax[mtyp]-mptr->zmin[mtyp];
	
	// define the 4 possible vertex corners of this model's bounding box.  these 4 vtx represent
	// the 2d polygon of the model's min max xy bounds as projected onto the display.
	for(i=0;i<4;i++)
	  {
	  if(i==0){x1=mptr->xorg[mtyp]-MVdisp_cen_x;	  y1=0-mptr->yorg[mtyp]-MVdisp_cen_y;	  z1=mptr->zorg[mtyp]-MVdisp_cen_z;}
	  if(i==1){x1=mptr->xorg[mtyp]+dix-MVdisp_cen_x;  y1=0-mptr->yorg[mtyp]-MVdisp_cen_y;	  z1=mptr->zorg[mtyp]-MVdisp_cen_z;}
	  if(i==2){x1=mptr->xorg[mtyp]+dix-MVdisp_cen_x;  y1=0-mptr->yorg[mtyp]-diy-MVdisp_cen_y; z1=mptr->zorg[mtyp]-MVdisp_cen_z;}
	  if(i==3){x1=mptr->xorg[mtyp]-MVdisp_cen_x;	  y1=0-mptr->yorg[mtyp]-diy-MVdisp_cen_y; z1=mptr->zorg[mtyp]-MVdisp_cen_z;}
	  vscr[i]=vertex_make();
	  vscr[i]->x=((MVview_scale*x1)*osc+(MVview_scale*y1)*oss+MVgxoffset);
	  vscr[i]->y=((MVview_scale*x1)*oss*ots-(MVview_scale*y1)*osc*ots+(MVview_scale*z1)*otc+MVgyoffset);
	  }
	vscr[0]->next=vscr[1];
	vscr[1]->next=vscr[2];
	vscr[2]->next=vscr[3];
	vscr[3]->next=vscr[0];
	
	// DEBUG
	//printf("p0: x=%3.0f y=%3.0f  p1: x=%3.0f y=%3.0f  p2: x=%3.0f y=%3.0f  p3: x=%3.0f y=%3.0f \n",
	//  vscr[0]->x,vscr[0]->y,vscr[1]->x,vscr[1]->y,vscr[2]->x,vscr[2]->y,vscr[3]->x,vscr[3]->y);
	//printf("event: x=%d  y=%d \n",mouse_x,mouse_y);
	
	// test if mouse pick is inside model bounds
	windnum=0;
	vptr=vscr[0];
	while(vptr!=NULL)
	  {
	  vnxt=vptr->next;
	  if( ((vptr->y > mouse_y) != (vnxt->y > mouse_y)) &&
	      (mouse_x < ((vnxt->x-vptr->x)*(mouse_y-vptr->y)/(vnxt->y-vptr->y)+vptr->x)) )windnum=!windnum;
	  vptr=vptr->next;
	  if(vptr==vscr[0])break;
	  }
	
	// clear memory 
	for(i=0;i<4;i++){free(vscr[i]); vertex_mem--;}
	if(windnum==TRUE){mpick=mptr; break;}
	
	mptr=mptr->next;
	}
	
      if(mpick!=NULL)active_model=mpick;
      */

      // find closest vtx of any model and define its parent as the mpick
      mptr=job.model_first;
      while(mptr!=NULL)
	{
	// compare mouse pick against facet vtxs for this model's model material 
	if(set_mouse_pick_entity==1)
	  {
	  posptr=mptr->vertex_first[MODEL];
	  if(set_view_models==FALSE)posptr=NULL;
	  while(posptr!=NULL)
	    {
	    x1=posptr->x+mptr->xoff[mtyp]-MVdisp_cen_x;
	    y1=0-(posptr->y+mptr->yoff[mtyp])-MVdisp_cen_y;
	    z1=posptr->z+mptr->zoff[mtyp]-MVdisp_cen_z;
	    dx=fabs(x-((MVview_scale*x1)*osc+(MVview_scale*y1)*oss+MVgxoffset));
	    dy=fabs(y-((MVview_scale*x1)*oss*ots-(MVview_scale*y1)*osc*ots+(MVview_scale*z1)*otc+MVgyoffset));
	    dz=sqrt(dx*dx+dy*dy);
	    printf("pick dist=%f   max dist=%f \n",dz,dzmin);
	    if(dz<dzmin)
	      {
	      dzmin=dz;
	      mpick=mptr;
	      vtx_pick_new->x=posptr->x;
	      vtx_pick_new->y=posptr->y;
	      vtx_pick_new->z=posptr->z;
	      vtx_pick_new->attr=posptr->attr;
	      vtx_pick_new->flist=posptr->flist;
	      vpick=posptr;
	      }
	    posptr=posptr->next;
	    }
	  // compare mouse pick against facet vtxs for this model's support material 
	  posptr=mptr->vertex_first[SUPPORT];
	  if(set_view_supports==FALSE)posptr=NULL;
	  while(posptr!=NULL)
	    {
	    x1=posptr->x+mptr->xoff[mtyp]-MVdisp_cen_x;
	    y1=0-(posptr->y+mptr->yoff[mtyp])-MVdisp_cen_y;
	    z1=posptr->z+mptr->zoff[mtyp]-MVdisp_cen_z;
	    dx=fabs(x-((MVview_scale*x1)*osc+(MVview_scale*y1)*oss+MVgxoffset));
	    dy=fabs(y-((MVview_scale*x1)*oss*ots-(MVview_scale*y1)*osc*ots+(MVview_scale*z1)*otc+MVgyoffset));
	    dz=sqrt(dx*dx+dy*dy);
	    if(dz<=dzmin)
	      {
	      dzmin=dz;
	      mpick=mptr;
	      vtx_pick_new->x=posptr->x;
	      vtx_pick_new->y=posptr->y;
	      vtx_pick_new->z=posptr->z;
	      vtx_pick_new->attr=posptr->attr;
	      vtx_pick_new->flist=posptr->flist;
	      vpick=posptr;
	      //printf("VTX PICK:  vtx=%X  dz=%6.3f  supp=%d\n",vpick,dzmin,vpick->supp);
	      }
	    posptr=posptr->next;
	    }
	  }
	  
	// compare mouse pick against facet centroid for this model's model material 
	if(set_mouse_pick_entity==2)
	  {
	  fpick=NULL;
	  fptr=mptr->facet_first[MODEL];
	  if(set_view_models==FALSE)fptr=NULL;
	  posptr=vertex_make();
	  while(fptr!=NULL)
	    {
	    posptr->x = (fptr->vtx[0]->x + fptr->vtx[1]->x + fptr->vtx[2]->x) /3;
	    posptr->y = (fptr->vtx[0]->y + fptr->vtx[1]->y + fptr->vtx[2]->y) /3;
	    posptr->z = (fptr->vtx[0]->z + fptr->vtx[1]->z + fptr->vtx[2]->z) /3;
	    x1=posptr->x+mptr->xoff[mtyp]-MVdisp_cen_x;
	    y1=0-(posptr->y+mptr->yoff[mtyp])-MVdisp_cen_y;
	    z1=posptr->z+mptr->zoff[mtyp]-MVdisp_cen_z;
	    dx=fabs(x-((MVview_scale*x1)*osc+(MVview_scale*y1)*oss+MVgxoffset));
	    dy=fabs(y-((MVview_scale*x1)*oss*ots-(MVview_scale*y1)*osc*ots+(MVview_scale*z1)*otc+MVgyoffset));
	    dz=sqrt(dx*dx+dy*dy);
	    if(dz<dzmin)
	      {
	      dzmin=dz;
	      mpick=mptr;
	      fpick=fptr;
	      }
	    fptr=fptr->next;
	    }
	  free(posptr); vertex_mem--;

	  fptr=mptr->facet_first[SUPPORT];
	  if(set_view_supports==FALSE)fptr=NULL;
	  posptr=vertex_make();
	  while(fptr!=NULL)
	    {
	    posptr->x = (fptr->vtx[0]->x + fptr->vtx[1]->x + fptr->vtx[2]->x) /3;
	    posptr->y = (fptr->vtx[0]->y + fptr->vtx[1]->y + fptr->vtx[2]->y) /3;
	    posptr->z = (fptr->vtx[0]->z + fptr->vtx[1]->z + fptr->vtx[2]->z) /3;
	    x1=posptr->x+mptr->xoff[mtyp]-MVdisp_cen_x;
	    y1=0-(posptr->y+mptr->yoff[mtyp])-MVdisp_cen_y;
	    z1=posptr->z+mptr->zoff[mtyp]-MVdisp_cen_z;
	    dx=fabs(x-((MVview_scale*x1)*osc+(MVview_scale*y1)*oss+MVgxoffset));
	    dy=fabs(y-((MVview_scale*x1)*oss*ots-(MVview_scale*y1)*osc*ots+(MVview_scale*z1)*otc+MVgyoffset));
	    dz=sqrt(dx*dx+dy*dy);
	    if(dz<dzmin)
	      {
	      dzmin=dz;
	      mpick=mptr;
	      fpick=fptr;
	      }
	    fptr=fptr->next;
	    }
	  free(posptr); vertex_mem--;
	  }
	 
	mptr=mptr->next;
	}
      
      active_model=mpick;
      if(set_mouse_select==TRUE && set_mouse_align==FALSE)mpick=NULL;	// if just picking a model...
      
      if(mpick!=NULL)
	{
	if(dzmin<(1.0*MVview_scale))
	  {
	  if(set_mouse_align && active_model!=NULL)			// align active model to new pick location
	    {
	    dx=(vtx_pick_new->x-vtx_pick_ref->x);
	    dy=(vtx_pick_new->y-vtx_pick_ref->y);
	    //dz=(vtx_pick_new->z-vtx_pick_ref->z);
	    active_model->xoff[mtyp]=mpick->xoff[mtyp]+dx;
	    active_model->yoff[mtyp]=mpick->yoff[mtyp]+dy;
	    //active_model->zoff[mtyp]=mpick->zoff[mtyp]+dz;
	    active_model->xorg[mtyp]=active_model->xoff[mtyp];
	    active_model->yorg[mtyp]=active_model->yoff[mtyp];
	    //active_model->zorg[mtyp]=active_model->zoff[mtyp];
	    active_model=NULL;
	    //job_maxmin();						// stuff may have moved... gotta re-eval
	    vtx_pick_ref->x=0;vtx_pick_ref->y=0;vtx_pick_ref->z=0;
	    printf("\nMODEL ALIGNMENT COMPLETE\n");
	    }
	  else 
	    {
	    active_model=mpick;
	    if(set_mouse_pick_entity==1)
	      {
	      vtx_pick_ref->x=vtx_pick_new->x;
	      vtx_pick_ref->y=vtx_pick_new->y;
	      vtx_pick_ref->z=vtx_pick_new->z;
	      vtx_pick_ref->attr=vtx_pick_new->attr;
	      vtx_pick_ref->flist=vtx_pick_new->flist;
	      
	      // debug - dump vtx_pick_ref neighbors
	      //printf("Vertex Pick:  vtx=%X  x=%6.3f y=%6.3f z=%6.3f\n",vpick,vpick->x,vpick->y,vpick->z);
	      vl_list=vertex_neighbor_list(vpick);
	      vl_ptr=vl_list;
	      while(vl_ptr!=NULL)
		{
		vptr=vl_ptr->v_item;
		//printf("   vtx=%X  x=%6.3f y=%6.3f z=%6.3f\n",vptr,vptr->x,vptr->y,vptr->z);
		vl_ptr=vl_ptr->next;
		}
	      }
	    if(set_mouse_pick_entity==2)
	      {
	      fct_pick_ref=fpick;
	      //printf("  fct=%X  n0=%X n1=%X n2=%X \n",fct_pick_ref,fct_pick_ref->fct[0],fct_pick_ref->fct[1],fct_pick_ref->fct[2]);
	      if(fct_pick_ref->fct[0]!=NULL)
		{
		if(fct_pick_ref->fct[0]->attr==11)printf("  fn0=A");
		if(fct_pick_ref->fct[0]->attr==12)printf("  fn0=B");
		if(fct_pick_ref->fct[0]->attr==13)printf("  fn0=C");
		}
	      if(fct_pick_ref->fct[1]!=NULL)
		{
		if(fct_pick_ref->fct[1]->attr==11)printf("  fn1=A");
		if(fct_pick_ref->fct[1]->attr==12)printf("  fn1=B");
		if(fct_pick_ref->fct[1]->attr==13)printf("  fn1=C");
		}
	      if(fct_pick_ref->fct[2]!=NULL)
		{
		if(fct_pick_ref->fct[2]->attr==11)printf("  fn2=A");
		if(fct_pick_ref->fct[2]->attr==12)printf("  fn2=B");
		if(fct_pick_ref->fct[2]->attr==13)printf("  fn2=C");
		}
	      printf(" \n");
	      }
	    }
	  }
	else 
	  {
	  active_model=NULL;
	  if(set_mouse_pick_entity==1){vtx_pick_new->x=0;vtx_pick_new->y=0;vtx_pick_new->z=0;}
	  //vtx_pick_new->flist=NULL;
	  //printf("vtx pick:  vertex cleared. \n");
	  }
	}	// end of if mpick!=NULL
      }	// end of if mouse_select==...
      
    old_mouse_x=mouse_x;
    old_mouse_y=mouse_y;

    gtk_widget_queue_draw(da);

    return(TRUE);
}

static gboolean on_model_area_middle_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    // reset auto max/min of view
    //set_auto_zoom=TRUE;
    //job_maxmin();

    return(TRUE);
}

static gboolean on_model_area_right_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    old_mouse_x=x;
    old_mouse_y=y;

    gtk_widget_queue_draw(da);

    return(TRUE);
}

static gboolean on_model_area_start_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    if(job.state==JOB_RUNNING)return(TRUE);				// can't move parts once printing starts
    MVgx_drag_start=x-old_mouse_x;					// normalize start location with pointer location
    MVgy_drag_start=y-old_mouse_y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_model_area_middle_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    model	*mptr;
    
    //if(Superimpose_flag==TRUE)return(TRUE);				// can't rotate camera
    set_view_bound_box=TRUE;
    MVdisp_spin-=(0.3*(PI/180)*(MVgx_drag_start-x));
    MVdisp_tilt-=(0.3*(PI/180)*(MVgy_drag_start-y));
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      mptr->MRedraw_flag=TRUE;
      mptr=mptr->next;
      }
    MVgx_drag_start=x;
    MVgy_drag_start=y;
    gtk_widget_queue_draw(da);

    return(TRUE);
}

static gboolean on_model_area_right_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    int		mtyp=MODEL;
    float 	pickx,picky;
    float 	oss,osc,ots,otc;
    float 	oldbldx,oldbly,newbldx,newbldy;
    model	*mptr;

    if(job.state>=JOB_RUNNING)return(TRUE);
    if(active_model!=NULL)
      {
      active_model->MRedraw_flag=TRUE;
      set_view_bound_box=TRUE;
      
      // since the build table view can be rotated relative to the screen, the mouse movement direction
      // needs to be determined relative to the build table.  assume z=0 for these calculations
      oss=sin(MVdisp_spin);
      osc=cos(MVdisp_spin);
      ots=sin(MVdisp_tilt);
      otc=cos(MVdisp_tilt);
      
      // translate mouse pick in xy of screen to xy of build table.  this is the reverse of drawing a point on
      // the build table.
      //oldbldx= ((old_mouse_x-MVgxoffset)-((MVview_scale*(0-y1-MVdisp_cen_y))*oss))/(osc*MVview_scale)+MVdisp_cen_x;
      //oldbldy= 0 - (((old_mouse_y-MVgyoffset)-((MVview_scale*(x1-MVdisp_cen_x))*oss*ots))/((-1)*osc*ots*MVview_scale)+MVdisp_cen_y);
      //newbldx= ((event->x-MVgxoffset)-((MVview_scale*(0-y1-MVdisp_cen_y))*oss))/(osc*MVview_scale)+MVdisp_cen_x;
      //newbldy= 0 - (((event->y-MVgyoffset)-((MVview_scale*(x1-MVdisp_cen_x))*oss*ots))/((-1)*osc*ots*MVview_scale)+MVdisp_cen_y);
      //pickx=(oldbldx-newbldx);
      //picky=(oldbldy-newbldy);
      
      //hl=(MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss+MVgxoffset;
      //vl=(MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc+MVgyoffset;

      pickx=MVgx_drag_start-x;
      picky=MVgy_drag_start-y;

      // add displacement to active model origin
      active_model->xorg[mtyp]-=(0.2*pickx);
      active_model->yorg[mtyp]+=(0.2*picky);

      MVgx_drag_start=x;
      MVgy_drag_start=y;

      set_center_build_job=FALSE;					// if user moving model, then they don't want it centered
      autoplacement_flag=FALSE;						// if user moving model, do not autoplace
      build_table_scan_flag=1;						// set to indication it needs to be re-done
      }

    gtk_widget_queue_draw(da);

    return(TRUE);
}

static gboolean on_model_area_motion_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    old_mouse_x=dx;
    old_mouse_y=dy;
    return(TRUE);
}

static gboolean on_model_area_scroll_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    double 	mx0,my0;

    //if(Superimpose_flag==TRUE)return(TRUE);				// disable for now, but possible with some work
    set_auto_zoom=FALSE;
    set_view_bound_box=TRUE;
    da_center_x=old_mouse_x;
    da_center_y=old_mouse_y;
    mx0=(da_center_x-MVgxoffset)/MVview_scale;
    my0=(da_center_y-MVgyoffset)/MVview_scale;

    // make adjustment
    if(dy<0)MVview_scale*=1.1;
    if(dy>0)MVview_scale/=1.1;

    // set limits if perspective is enabled otherwise view gets wonky
    if(Perspective_flag==TRUE)
      {
      if(MVview_scale<0.2)MVview_scale=0.2;
      if(MVview_scale>12.0)MVview_scale=12.0;
      }

    // reset offsets
    mx0=MVgxoffset+(mx0*MVview_scale);
    my0=MVgyoffset+(my0*MVview_scale);
    MVgxoffset+=da_center_x-mx0;
    MVgyoffset+=da_center_y-my0;
    
    gtk_widget_queue_draw(da);

    return(TRUE);
}



// function to draw information in the GRAPH VIEW
gboolean graph_draw_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  int		slot,h,i,j,layer_ttl,layer_num,lnum_y;
  int		hstart,hend;
  int		a_hrs,a_mins;
  float 	total_to_current;
  float 	newx,newy,oldx,oldy;
  float 	min_val_x,max_val_x,min_val_y,max_val_y;
  float 	x_scale,y_scale;
  float 	x1,y1,x2,y2;
  int 		xstr,ystr,xmax,ymax,bld_x,bld_y;
  float 	xstart,ystart,tbl_inc;
  GdkRGBA 	color;

  // init
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr,1.0);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
      
  // draw graph outline
  set_color(cr,&color,MD_GRAY);
  cairo_move_to(cr,GVgxoffset,0-GVgyoffset);				// move to 0,0
  cairo_line_to(cr,GVgxoffset+GVview_scale*200,0-GVgyoffset);			// draw to x max, 0
  cairo_line_to(cr,GVgxoffset+GVview_scale*200,0-(GVgyoffset+GVview_scale*200));// draw to x max, y max
  cairo_line_to(cr,GVgxoffset,0-(GVgyoffset+GVview_scale*200));		// draw to 0, y max
  cairo_line_to(cr,GVgxoffset,0-GVgyoffset);				// draw to 0,0
  cairo_stroke(cr);

  // scan data array for min/max values
  min_val_x=5000; max_val_x=-5000;
  min_val_y=0.0;  max_val_y=20.0;
  
  // if a time graph, find max values in data arrays
  if(hist_type==HIST_TIME)
    {
    for(h=H_MOD_TIM_EST;h<=H_OHD_TIM_ACT;h++)				// loop thru time fields
      {
      if(hist_use_flag[h]==TRUE)					// if field in use...
	{
	for(i=0;i<hist_ctr[HIST_TIME];i++)				// ... loop thru data array
	  {
	  if(history[h][i]<min_val_y)min_val_y=history[h][i];
	  if(history[h][i]>max_val_y)max_val_y=history[h][i];
	  }
	}
      }
    }
    
  // if a material graph, find max values in data arrays
  if(hist_type==HIST_MATL)
    {
    for(h=H_MOD_MAT_EST;h<=H_SUP_MAT_ACT;h++)				// loop thru material fields
      {
      if(hist_use_flag[h]==TRUE)					// if field in use...
	{
	for(i=0;i<hist_ctr[HIST_MATL];i++)				// ... loop thru data array
	  {
	  if(history[h][i]<min_val_y)min_val_y=history[h][i];
	  if(history[h][i]>max_val_y)max_val_y=history[h][i];
	  }
	}
      }
    }
    
  // if a thermal graph, set y max values based on max tool value
  if(hist_type==HIST_TEMP)
    {
    for(h=H_TOOL_TEMP;h<H_CHAM_TEMP;h++)
      {
      if(Tool[h].state<TL_LOADED)continue;
      if(Tool[h].thrm.maxtC > max_val_y)max_val_y=Tool[h].thrm.maxtC+10;
      }
    }
  
  // calculate scale based on min/max values
  // graph grid equals 10 pixels per square, about 200 by 200
  min_val_x=0;max_val_x=hist_ctr[hist_type];				// if no x axis defined... use array count
  if(min_val_x==max_val_x){min_val_x=0;max_val_x=10;}
  x_scale=200/(max_val_x-min_val_x);					// calc scale in X
  if(min_val_y==max_val_y){min_val_y=0;max_val_y=10;}
  y_scale=200/(max_val_y-min_val_y);					// calc scale in Y
    
  // draw X grid and axis values
  set_color(cr,&color,LT_GRAY);
  tbl_inc=10;								// number of pixels per increment
  x1=(200-(int)(200/tbl_inc)*tbl_inc)/2;
  h=0;
  y1=0;y2=200;
  while(x1<=200)							// draw vertical grid lines and x values
    {
    x2=x1;
    cairo_move_to(cr,GVgxoffset+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
    cairo_line_to(cr,GVgxoffset+GVview_scale*x2,0-(GVgyoffset+GVview_scale*y2));
    if(h%2==0){cairo_move_to(cr,GVgxoffset+GVview_scale*x1,30-(GVgyoffset+GVview_scale*y1));}
    else {cairo_move_to(cr,GVgxoffset+GVview_scale*x1,20-(GVgyoffset+GVview_scale*y1));}
    sprintf(scratch,"%3.0f",x1/x_scale);
    cairo_show_text(cr,scratch);
    x1+=tbl_inc;
    h++;
    }
  cairo_stroke(cr);
  x1=80;
  y1=(-20);
  set_color(cr,&color,BLACK);
  cairo_move_to(cr,GVgxoffset+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
  if(hist_type==HIST_TIME)sprintf(scratch,"LAYERS");
  if(hist_type==HIST_TEMP)sprintf(scratch,"SAMPLE");
  if(hist_type==HIST_MATL)sprintf(scratch,"LAYERS");
  cairo_show_text(cr,scratch);
  cairo_stroke(cr);

  // draw Y grid and axis values
  // we want to draw about 10 grid lines from the ymin to the ymax at some for of even increment (2, or 5, or 10, etc)
  //tbl_inc=10*y_scale;							// desired number of grids x number of pixels per increment
  tbl_inc=(max_val_y-min_val_y)/20;					// desired number of grids x number of pixels per increment
  x1=0;x2=200;
  y1=min_val_y;
  //y1=(200-(int)(200/tbl_inc)*tbl_inc)/2;
  while(y1<=200)							// draw horizontal grid lines and y values
    {
    set_color(cr,&color,LT_GRAY);
    y2=y1;
    cairo_move_to(cr,GVgxoffset+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
    cairo_line_to(cr,GVgxoffset+GVview_scale*x2,0-(GVgyoffset+GVview_scale*y2));
    cairo_move_to(cr,GVgxoffset-35+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
    sprintf(scratch,"%5.1f",(y1/y_scale));
    cairo_show_text(cr,scratch);
    cairo_stroke(cr);
    y1+=(tbl_inc*y_scale);
    }
  set_color(cr,&color,BLACK);
  if(hist_type==HIST_TEMP)
    {
    sprintf(scratch,"THERMAL PROFILE");
    x1=75; y1=210;
    }
  if(hist_type==HIST_TIME)
    {
    sprintf(scratch,"BUILD TIME PER LAYER PROFILE");
    x1=60; y1=210;
    }
  if(hist_type==HIST_MATL)
    {
    sprintf(scratch,"MATERIAL USE PER LAYER PROFILE");
    x1=55; y1=210;
    }
  cairo_move_to(cr,GVgxoffset+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
  cairo_show_text(cr,scratch);
  cairo_stroke(cr);
  x1=(-50);
  y1=200;
  set_color(cr,&color,BLACK);
  cairo_move_to(cr,GVgxoffset+GVview_scale*x1,0-(GVgyoffset+GVview_scale*y1));
  if(hist_type==HIST_TEMP)sprintf(scratch,"TEMP(C)");
  if(hist_type==HIST_TIME)sprintf(scratch,"TIME(sec)");
  if(hist_type==HIST_MATL)sprintf(scratch,"LENGTH(mm)");
  cairo_show_text(cr,scratch);
  cairo_stroke(cr);
  
  // define current layer for time/mat'l graphs
  lnum_y=0;
  layer_num=0;
  if(job.min_slice_thk>0)layer_num=floor(z_cut/job.min_slice_thk)-1;
  
  // draw data arrays
  if(hist_ctr[hist_type]<1)return(FALSE);
  if(hist_type==HIST_TIME){hstart=H_MOD_TIM_EST; hend=H_OHD_TIM_ACT;}
  if(hist_type==HIST_TEMP){hstart=H_TOOL_TEMP;   hend=H_CHAM_TEMP;}
  if(hist_type==HIST_MATL){hstart=H_MOD_MAT_EST; hend=H_SUP_MAT_ACT;}
  for(h=hstart;h<=hend;h++)
    {
    if(hist_use_flag[h]==TRUE && hist_ctr[hist_type]>1)
      {
      total_to_current=0;
      set_color(cr,&color,hist_color[h]);
      x1=GVgxoffset;
      y1=0-(GVgyoffset+GVview_scale*(history[h][0]*y_scale));
      for(i=1;i<hist_ctr[hist_type];i++)
	{
	x2=GVgxoffset+GVview_scale*(i*x_scale);
	if(history[h][i]<max_val_y)
	  {
	  if(history[h][i]>min_val_y)
	    {y2=0-(GVgyoffset+GVview_scale*(history[h][i]*y_scale));}	// draw in graph
	    else
	    {y2=0-(GVgyoffset+GVview_scale*(min_val_y*y_scale));}
	  }
	else 
	  {y2=0-(GVgyoffset+GVview_scale*(max_val_y*y_scale));}		// clip to max y value
	cairo_move_to(cr,x1,y1);
	cairo_line_to(cr,x2,y2);
	cairo_stroke(cr);
	x1=x2;
	y1=y2;
	
	// display current layer bar if time/mat'l graph
	if(hist_type==HIST_TIME || hist_type==HIST_MATL)
	  {
	  if(i<=layer_num)total_to_current += history[h][i];
	  if(i==layer_num)
	    {
	    lnum_y=y2;
	    set_color(cr,&color,MD_YELLOW);
	    cairo_move_to(cr,x2,(0-GVgyoffset));
	    cairo_line_to(cr,x2,(0-(GVgyoffset+GVview_scale*(max_val_y*y_scale))));
	    cairo_stroke(cr);
	    set_color(cr,&color,hist_color[h]);
	    }
	  }
	}

      // display field label to right of graph at y location if non-zero
      //if(lnum_y<(0-(GVgyoffset)))
	{
	set_color(cr,&color,hist_color[h]);
	x2=GVgxoffset+GVview_scale*210;
	cairo_move_to(cr,x2,y2);
	if(hist_type==HIST_TEMP)
	  {
	  if(h==H_TOOL_TEMP)sprintf(scratch,"%s - %5.1fC : %3.0f%%",hist_name[h],Tool[0].thrm.tempC,(Tool[0].thrm.heat_duty));
	  if(h==H_CHAM_TEMP)sprintf(scratch,"%s - %5.1fC : %3.0f%%",hist_name[h],Tool[CHAMBER].thrm.tempC,(Tool[CHAMBER].thrm.heat_duty));
	  if(h==H_TABL_TEMP)sprintf(scratch,"%s - %5.1fC : %3.0f%%",hist_name[h],Tool[BLD_TBL1].thrm.tempC,(Tool[BLD_TBL1].thrm.heat_duty));
	  cairo_show_text(cr,scratch);
	  }
	if(hist_type==HIST_TIME)
	  {
	  if(total_to_current>CLOSE_ENOUGH || i==0)
	    {
	    a_hrs=(int)(total_to_current/3600.0);				// hours elapsed
	    a_mins=(int)((total_to_current)/60.0)-(a_hrs*60);		// minutes elapsed
	    sprintf(scratch,"%s - %3d hr %2d min",hist_name[h],a_hrs,a_mins);
	    cairo_move_to(cr,x2,lnum_y);
	    cairo_show_text(cr,scratch);
	    }
	  }
	if(hist_type==HIST_MATL)
	  {
	  if(total_to_current>CLOSE_ENOUGH || i==0)
	    {
	    sprintf(scratch,"%s - %5.2f m",hist_name[h],total_to_current/1000);
	    cairo_move_to(cr,x2,lnum_y);
	    cairo_show_text(cr,scratch);
	    }
	  }
	cairo_stroke(cr);
	}
      }
    }
    
  // draw hash lines for selected layer only if x axis is in layers
  /*
  h=8;
  if(job.state==JOB_READY)h=4;
  layer_ttl=floor((ZMax-ZMin)/job.min_slice_thk);
  layer_num=floor(z_cut/job.min_slice_thk)-1;
  if(layer_num>=0 && layer_num<layer_ttl)
    {
    set_color(cr,&color,BLACK);
    
    x1=GVgxoffset+GVview_scale*(layer_num*x_scale);
    y1=0-(GVgyoffset+GVview_scale*(history[h][layer_num]*y_scale));
    cairo_move_to(cr,x1,y1);
    sprintf(scratch,"%6.2f",history[h][layer_num]);
    cairo_show_text(cr,scratch);
    cairo_stroke(cr);

    x2=x1;
    y2=0-(GVgyoffset+GVview_scale*(min_val_y*y_scale));
    cairo_move_to(cr,x1,y1);
    cairo_line_to(cr,x2,y2);
    cairo_stroke(cr);

    x2=GVgxoffset+GVview_scale*(min_val_x*x_scale);
    y2=y1;
    cairo_move_to(cr,x1,y1);
    cairo_line_to(cr,x2,y2);
    cairo_stroke(cr);
    }
  */

  return(TRUE);
}

static gboolean on_graph_area_left_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    old_mouse_x=x;
    old_mouse_y=y;
    if(job.state>=JOB_RUNNING)return(TRUE);				// don't allow changes during builds					
    return(TRUE);
}

static gboolean on_graph_area_middle_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    GVview_scale=2.10;
    GVgxoffset=225;
    GVgyoffset=(-475);
    return(TRUE);
}

static gboolean on_graph_area_right_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    old_mouse_x=x;
    old_mouse_y=y;
    return(TRUE);
}

static gboolean on_graph_area_start_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    GVgx_drag_start=x-old_mouse_x;					// normalize start location with pointer location
    GVgy_drag_start=y-old_mouse_y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_graph_area_left_update_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    GVgxoffset-=(GVgx_drag_start-x);
    GVgyoffset+=(GVgy_drag_start-y);
    GVgx_drag_start=x;
    GVgy_drag_start=y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_graph_area_right_update_motion_press_event(GtkGestureDrag *gesture, double x, double y, double dx, double dy, gpointer da)
{
    GVgxoffset-=(GVgx_drag_start-x);
    GVgyoffset+=(GVgy_drag_start-y);
    GVgx_drag_start=x;
    GVgy_drag_start=y;
    gtk_widget_queue_draw(da);
    return(TRUE);
}

static gboolean on_graph_area_motion_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    old_mouse_x=dx;
    old_mouse_y=dy;
    return(TRUE);
}

static gboolean on_graph_area_scroll_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    double mx0,my0;

    da_center_x=old_mouse_x;
    da_center_y=0-old_mouse_y;
    mx0=(da_center_x-GVgxoffset)/GVview_scale;
    my0=(da_center_y-GVgyoffset)/GVview_scale;
  
    if(dy<0)GVview_scale*=1.1;
    if(dy>0)GVview_scale/=1.1;

    mx0=GVgxoffset+(mx0*GVview_scale);
    my0=(GVgyoffset+(my0*GVview_scale));
    GVgxoffset+=(da_center_x-mx0);
    GVgyoffset+=(da_center_y-my0);

    gtk_widget_queue_draw(da);
    
    return(TRUE);
}


// fuction to display CAMERA VIEW
gboolean camera_callback (GtkWidget *widget, cairo_t *cr, gpointer data)
{
  char		cam_scratch[128];
  char		img_name[255];
  int		slot;
  slice		*sptr;
  model		*mptr;
  float 	hl,vl,zl,hh,vh,zh;
  float 	x0,y0,z0,x1,y1,z1,x2,y2,z2;
  float 	oldx,oldy,newx,newy;
  float 	oss,osc,ots,otc;
  float 	xstart,ystart,tbl_inc;
  float 	vangle,t,shade,alpha;
  GdkRGBA 	color;

  // verify function is applicable
  if(UNIT_HAS_CAMERA==FALSE)return(FALSE);
  //if(need_new_image==FALSE)return(FALSE);
  
  // set default image to display as no image
  sprintf(img_name,"/home/aa/Documents/4X3D/No_Image.jpg");

  // display live view of camera (as fast as idle loop updates)
  if(cam_image_mode==CAM_LIVEVIEW || cam_image_mode==CAM_SNAPSHOT)
    {
    crgslot[0].camera_tilt=1.0;
    strcpy(bashfn,"#!/bin/bash\nsudo libcamera-jpeg -v 0 -n 1 -t 10 ");		// define base camera command
    strcat(bashfn,"-o /home/aa/Documents/4X3D/still-test.jpg ");		// ... add the target file name
    strcat(bashfn,"--width 830 --height 574 ");					// ... add the image size
    sprintf(cam_scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
    strcat(bashfn,cam_scratch);							// ... add region of interest (zoom)
    strcat(bashfn,"--autofocus-range macro ");					// ... focus on tool tip
    strcat(bashfn,"--autofocus-mode auto ");					// ... add focus control
    if(crgslot[0].camera_tilt<0.872){strcat(bashfn,"--rotation 180 ");}		// ... if tilted toward tool, rotate view 180 deg
    else {strcat(bashfn,"--rotation 0 ");}					// ... otherwise keep at 0 deg
    system(bashfn);								// issue the command to take a picture
    sprintf(img_name,"/home/aa/Documents/4X3D/still-test.jpg");
    }

  // display job time lapse images
  if(cam_image_mode==CAM_JOB_LAPSE)
    {
    sprintf(img_name,"/home/aa/Documents/4X3D/ImageHistory/z_%5.3f.jpg",z_cut);	 // ... build image name from z level
    }
    
  // display table scan time lapse images
  if(cam_image_mode==CAM_SCAN_LAPSE)
    {
    // use z_cut (which now represents x position on table) to determine which image to display
    // and we know that the table is 415mm across with images every 5mm equates to 83 images
    //sprintf(img_name,"/home/aa/Documents/4X3D/TableScan/x_100.jpg");			// DEBUG, forced name
    sprintf(img_name,"/home/aa/Documents/4X3D/TableScan/x_%000d.jpg",(int)(z_cut));	// ... build image name from z level
    //g_camera_buff = gdk_pixbuf_new_from_file(img_name, NULL);	
    //gdk_pixbuf_save (g_camera_buff, "/home/aa/Documents/4X3D/tbscan0.jpg", "jpeg", &my_errno, "quality", "100", NULL);
    //image_processor(IMG_EDGE_DETECT_ALL,TRUE);
    //sprintf(img_name,"/home/aa/Documents/4X3D/tb_diff.jpg");	
    }

  // display current laser scan image
  if(cam_image_mode==CAM_IMG_SCAN)
    {
    sprintf(img_name,"/home/aa/Documents/4X3D/TableScan/x_%000d.jpg",(int)(z_cut));	// ... build image name from z level
    if(debug_flag==603)sprintf(img_name,"/home/aa/Documents/4X3D/tb_diff.jpg");
    }
  
  // display image difference
  if(cam_image_mode==CAM_IMG_DIFF)
    {
    sprintf(img_name,"/home/aa/Documents/4X3D/tb_diff.jpg");			// ... build image name
    }


  // display image
  if(g_camera_buff!=NULL){g_object_unref(g_camera_buff); g_camera_buff=NULL;}		// clear old buffer if it exists
  g_camera_buff = gdk_pixbuf_new_from_file_at_scale(img_name, 830,574, FALSE, NULL);	// load new buffer with image
  if(g_camera_buff!=NULL)
    {
    gdk_cairo_set_source_pixbuf(cr, g_camera_buff, 0, 0);				// set drawing target
    cairo_paint(cr);									// draw to view
    need_new_image=FALSE;
    }
  else 
    {
    g_camera_buff = gdk_pixbuf_new_from_file("/home/aa/Documents/4X3D/No_Image.jpg", NULL);	
    }
  //if(g_camera_buff!=NULL)g_object_unref(g_camera_buff);					// clear buffer if it exists
  
  // if superimposing the table grid...
  if(Superimpose_flag==TRUE)
    {
    // force the viewing angle to match the camera's view of the table
    MVdisp_spin=-0.800;							// more negative spins grid clockwise on image
    MVdisp_tilt= 3.640;							// more negative makes grid flatter
    MVview_scale=2.300;							// more positive makes grid bigger on image
    MVdisp_cen_x=(BUILD_TABLE_LEN_X/2);0;
    MVdisp_cen_y=0-(BUILD_TABLE_LEN_Y/2);
    MVdisp_cen_z=0.0;
    MVgxoffset=415;							// more pushes grid to right of image
    MVgyoffset=195;							// more pushes grid down on image
    pers_scale=0.65;							// more bends axis inward

    // init for 3D drawing
    gdk_cairo_set_source_rgba (cr, &color);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
    vangle=set_view_edge_angle*PI/180;
    oss=sin(MVdisp_spin);						// pre-calculate to save time downstream
    osc=cos(MVdisp_spin);
    ots=sin(MVdisp_tilt);
    otc=cos(MVdisp_tilt);
    
    set_color(cr,&color,LT_GRAY);
    cairo_set_line_width (cr,0.5);
    tbl_inc=10;
    xstart=(BUILD_TABLE_LEN_X-(int)(BUILD_TABLE_LEN_X/tbl_inc)*tbl_inc)/2;
    ystart=(BUILD_TABLE_LEN_Y-(int)(BUILD_TABLE_LEN_Y/tbl_inc)*tbl_inc)/2;
    y1=0;y2=0-BUILD_TABLE_LEN_Y;z1=0;z2=0;	
    x1=xstart;
    while(x1<=BUILD_TABLE_LEN_X)
      {
      x2=x1;
      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hl,vl);
      cairo_line_to(cr,hh,vh);
      x1+=tbl_inc;
      }
    x1=0;x2=BUILD_TABLE_LEN_X;z1=0;z2=0;
    y1=ystart;
    while(y1>=(0-BUILD_TABLE_LEN_Y))
      {
      y2=y1;
      zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
      hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
      vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
      zh=1+(600 + (MVview_scale*(x2-MVdisp_cen_x))*oss - (MVview_scale*(y2-MVdisp_cen_y))*osc + (MVview_scale*(z2-MVdisp_cen_z))*ots)/600*pers_scale;
      hh=((MVview_scale*(x2-MVdisp_cen_x))*osc+(MVview_scale*(y2-MVdisp_cen_y))*oss)/zh+MVgxoffset;
      vh=((MVview_scale*(x2-MVdisp_cen_x))*oss*ots-(MVview_scale*(y2-MVdisp_cen_y))*osc*ots+(MVview_scale*(z2-MVdisp_cen_z))*otc)/zh+MVgyoffset;
      cairo_move_to(cr,hl,vl);
      cairo_line_to(cr,hh,vh);
      y1-=tbl_inc;
      }
    cairo_stroke(cr);
    }
  
  
  return(TRUE);
}

static gboolean on_camera_area_left_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    GdkPixbuf	*g_img_src1_buff;					// 1st source image
    int		src1_n_ch;
    int		src1_cspace;
    int		src1_alpha;
    int		src1_bits_per_pixel;
    int 	src1_height;
    int 	src1_width;
    int 	src1_rowstride;
    guchar 	*src1_pixels;
    
    old_mouse_x=x;
    old_mouse_y=y;
    
    // verify both images (buffers) contain data
    g_img_src1_buff = gdk_pixbuf_new_from_file_at_scale("/home/aa/Documents/4X3D/still-test.jpg", 830,574, FALSE, NULL);	// load new buffer with image
    
    // get color and pixels per row information of 1st source image
    src1_n_ch = gdk_pixbuf_get_n_channels (g_img_src1_buff);		// n channels - should always be 4
    src1_cspace = gdk_pixbuf_get_colorspace (g_img_src1_buff);		// color space - should be RGB
    src1_alpha = gdk_pixbuf_get_has_alpha (g_img_src1_buff);		// alpha - should be FALSE
    src1_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_src1_buff); // bits per pixel - should be 8
    src1_width = gdk_pixbuf_get_width (g_img_src1_buff);		// number of columns (image size)
    src1_height = gdk_pixbuf_get_height (g_img_src1_buff);		// number of rows (image size)
    src1_rowstride = gdk_pixbuf_get_rowstride (g_img_src1_buff);	// get pixels per row... may change tho
    src1_pixels = gdk_pixbuf_get_pixels (g_img_src1_buff);		// get pointer to pixel data

    // find RGB values at mouse click xy
    pixfilter = src1_pixels + (int)y * src1_rowstride + (int)x * src1_n_ch;	// load pixel from mouse xy from src1
    printf("\n R=%d G=%d B=%d A=%d \n",pixfilter[0],pixfilter[1],pixfilter[2],pixfilter[3]);
    
    g_object_unref(g_img_src1_buff);
    
    return(TRUE);
}

static gboolean on_camera_area_middle_button_press_event(GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    CVview_scale=1.0;
    CVgxoffset=(830.0/2.0);
    CVgyoffset=(574.0/2.0);
    cam_roi_x0=0.0; cam_roi_y0=0.0;
    cam_roi_x1=1.0; cam_roi_y1=1.0;
    return(TRUE);
}

static gboolean on_camera_area_motion_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    old_mouse_x=dx;
    old_mouse_y=dy;
    return(TRUE);
}

static gboolean on_camera_area_scroll_event(GtkEventController *controller, double dx, double dy, gpointer da)
{
    float 	XHalf,YHalf;
    int		Xwin=830,Ywin=574;					// default window size
    
    // in this context, the CVgx/CVgy offsets are the current number of pixels into the image
    // that the lower left corner is cropped to.  For example: with a 1920x1080 image, CVgx=300, CVgy=100
    // equates to the roix=300/1920 and roiy=100/1080

    // locate mouse cursor as center of zoom in 830x574 window
    da_center_x=old_mouse_x;
    da_center_y=old_mouse_y;
  
    // adjust scale - at zoom=1.00 the box size is the window size (830x574)
    if(dy<0)CVview_scale*=1.1;
    if(dy>0)CVview_scale/=1.1;
    if(CVview_scale>1.0)CVview_scale=1.0;
    if(CVview_scale<0.01)CVview_scale=0.1;

    // calc new box size based on zoom
    // scaling both with same value perserves x-to-y ratio
    CVgxoffset = Xwin*CVview_scale;
    CVgyoffset = Ywin*CVview_scale;

    // define lower left corner of what we want based on mouse position and new box size
    XHalf=CVgxoffset/2;
    cam_roi_x0=0.0;
    if(da_center_x>=XHalf)cam_roi_x0=(float)(da_center_x-XHalf)/Xwin;
    if(cam_roi_x0<0.0)cam_roi_x0=0.0;
    if(cam_roi_x0>0.9)cam_roi_x0=0.9;
    
    YHalf=CVgyoffset/2;
    cam_roi_y0=0.0;
    if(da_center_y>=YHalf)cam_roi_y0=(float)(da_center_y-YHalf)/Ywin;
    if(cam_roi_y0<0.0)cam_roi_y0=0.0;
    if(cam_roi_y0>0.9)cam_roi_y0=0.9;
    
    // define upper right corner of what we want, but we want to keep the same window x-to-y ratio
    cam_roi_x1=1.0;
    if((da_center_x+XHalf)<=Xwin)cam_roi_x1=(float)(da_center_x+XHalf)/Xwin;
    if(cam_roi_x1<0.1)cam_roi_x1=0.1;
    if(cam_roi_x1>1.0)cam_roi_x1=1.0;
    if(cam_roi_x1<cam_roi_x0)cam_roi_x1=cam_roi_x0+0.1;
    
    cam_roi_y1=1.0;
    if((da_center_y+YHalf)<=Ywin)cam_roi_y1=(float)(da_center_y+YHalf)/Ywin;
    if(cam_roi_y1<0.1)cam_roi_y1=0.1;
    if(cam_roi_y1>1.0)cam_roi_y1=1.0;
    if(cam_roi_y1<cam_roi_y0)cam_roi_y1=cam_roi_y0+0.1;
    
    printf("\n\nXoff=%d Yoff=%d Scal=%f\n",CVgxoffset,CVgyoffset,CVview_scale);  
    printf("x0=%f y0=%f  x1=%f y1=%f \n",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
      
    gtk_widget_queue_draw(da);
    
    return(TRUE);
}



// OPENGL Code
// Sorta works, but has main window blanking bug.  Needs much more work.
// Small section of code with gl_area in main.c needs to be uncommented to use this.
/*
GLuint gl_vao, gl_buffer, gl_program;
static GLuint ModelViewProjectionMatrix_location,
              NormalMatrix_location,
              LightSourcePosition_location,
              MaterialColor_location;
static GLfloat ProjectionMatrix[16];					// projection matrix
static const GLfloat LightSourcePosition[4] = { 5.0, 5.0, 10.0, 1.0};	// direction of light


// Function to multiply two 4x4 matricies for opengl
static void multiply(GLfloat *m, const GLfloat *n)
{
   GLfloat 		tmp[16];
   const GLfloat 	*row, *column;
   div_t 		d;
   int 			i, j;

   for(i=0;i<16;i++)
     {
     tmp[i] = 0;
     d = div(i, 4);
     row = n + d.quot * 4;
     column = m + d.rem;
     for(j=0;j<4;j++)tmp[i] += row[j]*column[j*4];
     }
   memcpy(m, &tmp, sizeof (tmp));
}

// Function to rotate a 4x4 matrix for opengl
static void rotate(GLfloat *m, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   double s, c;

   s=sin(angle);
   c=cos(angle);
   GLfloat r[16] = {
      x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
      x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0, 
      x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
      0, 0, 0, 1
      };

   multiply(m, r);
}

// Function to translate a 4x4 matrix for opengl
static void translate(GLfloat *m, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat t[16] = { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 };

   multiply(m, t);
}

// Function to create an identity 4x4 matrix for opengl
static void identity(GLfloat *m)
{
   GLfloat t[16] = {
      1.0, 0.0, 0.0, 0.0,
      0.0, 1.0, 0.0, 0.0,
      0.0, 0.0, 1.0, 0.0,
      0.0, 0.0, 0.0, 1.0,
   };

   memcpy(m, t, sizeof(t));
}

// Function to transpose a 4x4 matrix for opengl
static void transpose(GLfloat *m)
{
   GLfloat t[16] = {
      m[0], m[4], m[8],  m[12],
      m[1], m[5], m[9],  m[13],
      m[2], m[6], m[10], m[14],
      m[3], m[7], m[11], m[15]};

   memcpy(m, t, sizeof(t));
}

// Function to invert a 4x4 matrix for opengl
static void invert(GLfloat *m)
{
   GLfloat t[16];
   identity(t);

   // Extract and invert the translation part 't'. The inverse of a
   // translation matrix can be calculated by negating the translation
   // coordinates.
   t[12] = -m[12];  t[13] = -m[13];  t[14] = -m[14];

   // Invert the rotation part 'r'. The inverse of a rotation matrix is
   // equal to its transpose.
   m[12] = m[13] = m[14] = 0;
   transpose(m);

   // inv(m) = inv(r) * inv(t)
   multiply(m, t);
}

// vertex shader
const GLchar *vert_src ="\n"
"#version 120 \n"
"attribute vec4 position; \n"
"\n"
"void main(void)\n"
"{\n"
"    gl_Position = position; \n"
"}";


// fragment shader
const GLchar *frag_src ="\n" 
"#version 120 \n"
"\n"
"void main(void)\n"
"{\n"
"    gl_FragColor = vec4(0.1, 0.8, 0.2, 1.0); \n"
"}";


// Function to init opengl view
static gboolean on_realize(GtkGLArea *area, GdkGLContext *context)
{
  char 		msg[512];
  GLuint 	frag_shader, vert_shader;

  gtk_gl_area_make_current(area);
  if(gtk_gl_area_get_error(GTK_GL_AREA(area)) != NULL)
    {
    printf("Failed to initialiize buffers for OpenGL area!\n");
    return FALSE;
    }
  GError *internal_error = NULL;
  printf("OpenGL area realized:\n");

  char *GL_version=(char *)glGetString(GL_VERSION);
  char *GL_vendor=(char *)glGetString(GL_VENDOR);
  char *GL_renderer=(char *)glGetString(GL_RENDERER);
  printf("  version = %s\n",GL_version);
  printf("  vendor  = %s\n",GL_vendor);
  printf("  render  = %s\n",GL_renderer);
  
  frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(frag_shader, 1, &frag_src, NULL);
  glCompileShader(frag_shader);
  glGetShaderInfoLog(frag_shader, sizeof msg, NULL, msg);
  if(strlen(msg)==0)sprintf(msg,"ok");
  printf("fragment shader info: %s\n", msg);

  vert_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vert_shader, 1, &vert_src, NULL);
  glCompileShader(vert_shader);
  glGetShaderInfoLog(vert_shader, sizeof msg, NULL, msg);
  if(strlen(msg)==0)sprintf(msg,"ok");
  printf("vertex shader info: %s\n", msg);

  gl_program = glCreateProgram();
  glAttachShader(gl_program, frag_shader);
  glAttachShader(gl_program, vert_shader);
  glLinkProgram(gl_program);
  glGetProgramInfoLog(gl_program, sizeof msg, NULL, msg);
  if(strlen(msg)==0)sprintf(msg,"ok");
  printf("info: %s\n", msg);

  glUseProgram(gl_program);

  glGenVertexArrays(1, &gl_vao);
  glBindVertexArray(gl_vao);

  glGenBuffers(1, &gl_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(dvtx), dvtx, GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0);
  
  glBindBuffer(GL_ARRAY_BUFFER,0);
  glBindVertexArray(0);
  
  //glDeleteBuffers(1, &gl_buffer);

  return TRUE;
}


static gboolean on_render(GtkGLArea *area, GdkGLContext *context)
{

  printf("GLArea rendered\n");
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(0.0, 0.0, 0.0, 1.0);

  glUseProgram(gl_program);

  glBindBuffer(GL_ARRAY_BUFFER, gl_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(dvtx), &dvtx[0], GL_DYNAMIC_DRAW);
  glBindVertexArray(gl_vao);

  glVertexPointer(3, GL_FLOAT, 4, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);
  glDrawArrays(GL_LINES, 0, ndvtx);
  //glDrawArrays(GL_TRIANGLES, 0, ndvtx);

  glBindVertexArray (0);
  glUseProgram (0);

  glFlush();

  return TRUE;
}
*/



// Function to set GdkRGBA color based on defined nuemonic (single integer value)
void set_color(cairo_t *cr, GdkRGBA *st_color, int color)
{
  int		base_color;
  float 	shade;
  GdkRGBA	new_color;
  
  // set color for input that is already using GdkRGBA
  if(color==(-1) && cr!=NULL){gdk_cairo_set_source_rgba(cr, st_color);return;}
  
  // set color for shading primary models
  if(color<(-100) && color>(-200) && cr!=NULL)
    {
    shade=(color+100)/(-100.0);
    if(shade<=0)shade=0.02;
    if(shade>1)shade=0.99;
    new_color.red=st_color->red*shade; 
    new_color.blue=st_color->blue*shade; 
    new_color.green=st_color->green*shade; 
    new_color.alpha=0.25;
    gdk_cairo_set_source_rgba(cr, &new_color);
    return;
    }

  // set color for shading copies of models
  if(color<(-200) && color>(-300) && cr!=NULL)
    {
    shade=(color+200)/(-100.0);
    if(shade<=0)shade=0.02;
    if(shade>1)shade=0.99;
    new_color.red=shade; 
    new_color.blue=shade; 
    new_color.green=shade; 
    new_color.alpha=0.25;
    gdk_cairo_set_source_rgba(cr, &new_color);
    return;
    }

  // Defines for color mnuemonics with color numeric range from 0 to 99
  st_color->alpha=0.75;
  
  if(color==BLACK)	{st_color->red=0.0;st_color->blue=0.0;st_color->green=0.0;st_color->alpha=1.0;}
  if(color==WHITE)	{st_color->red=1.0;st_color->blue=1.0;st_color->green=1.0;st_color->alpha=1.0;}

  if(color==SL_GRAY) 	{st_color->red=0.733;st_color->blue=0.721;st_color->green=0.741;}
  if(color==LT_GRAY)	{st_color->red=0.533;st_color->blue=0.521;st_color->green=0.541;}
  if(color==ML_GRAY)	{st_color->red=0.433;st_color->blue=0.425;st_color->green=0.441;}
  if(color==MD_GRAY)	{st_color->red=0.333;st_color->blue=0.325;st_color->green=0.341;}
  if(color==DM_GRAY)	{st_color->red=0.280;st_color->blue=0.262;st_color->green=0.274;}
  if(color==DK_GRAY)	{st_color->red=0.180;st_color->blue=0.212;st_color->green=0.204;}
  
  if(color==SL_RED) 	{st_color->red=0.988;st_color->blue=0.310;st_color->green=0.310;}
  if(color==LT_RED) 	{st_color->red=0.937;st_color->blue=0.161;st_color->green=0.161;}
  if(color==MD_RED) 	{st_color->red=0.800;st_color->blue=0.000;st_color->green=0.000;}
  if(color==DK_RED) 	{st_color->red=0.643;st_color->blue=0.000;st_color->green=0.000;}
  
  if(color==LT_ORANGE)	{st_color->red=0.988;st_color->blue=0.243;st_color->green=0.686;}
  if(color==MD_ORANGE)	{st_color->red=0.961;st_color->blue=0.000;st_color->green=0.475;}
  if(color==DK_ORANGE)	{st_color->red=0.808;st_color->blue=0.000;st_color->green=0.361;}
  
  if(color==LT_YELLOW)	{st_color->red=0.988;st_color->blue=0.310;st_color->green=0.914;}
  if(color==MD_YELLOW)	{st_color->red=0.929;st_color->blue=0.000;st_color->green=0.831;}
  if(color==DK_YELLOW)	{st_color->red=0.769;st_color->blue=0.000;st_color->green=0.627;}
  
  if(color==LT_GREEN)	{st_color->red=0.541;st_color->blue=0.204;st_color->green=0.886;}
  if(color==MD_GREEN)	{st_color->red=0.450;st_color->blue=0.086;st_color->green=0.824;}
  if(color==DK_GREEN)	{st_color->red=0.306;st_color->blue=0.024;st_color->green=0.604;}
  
  if(color==DK_CYAN)	{st_color->red=0.0;st_color->blue=0.2;st_color->green=0.2;}
  if(color==MD_CYAN)	{st_color->red=0.0;st_color->blue=0.5;st_color->green=0.5;}
  if(color==LT_CYAN)	{st_color->red=0.0;st_color->blue=0.8;st_color->green=0.8;}
  
  if(color==LT_BLUE)	{st_color->red=0.447;st_color->blue=0.812;st_color->green=0.624;}
  if(color==MD_BLUE)	{st_color->red=0.204;st_color->blue=0.643;st_color->green=0.396;}
  if(color==DK_BLUE)	{st_color->red=0.125;st_color->blue=0.529;st_color->green=0.290;}
  
  if(color==LT_BROWN)	{st_color->red=0.914;st_color->blue=0.431;st_color->green=0.725;}
  if(color==MD_BROWN)	{st_color->red=0.757;st_color->blue=0.067;st_color->green=0.490;}
  if(color==DK_BROWN)	{st_color->red=0.561;st_color->blue=0.008;st_color->green=0.349;}
  
  if(color==LT_VIOLET)	{st_color->red=0.678;st_color->blue=0.659;st_color->green=0.498;}
  if(color==MD_VIOLET)	{st_color->red=0.459;st_color->blue=0.482;st_color->green=0.314;}
  if(color==DK_VIOLET)	{st_color->red=0.361;st_color->blue=0.400;st_color->green=0.208;}
  
  if(cr!=NULL)gdk_cairo_set_source_rgba (cr, st_color);
  return;

}

// Function to set defined mnuemonic based on GdkRGBA input
int get_color(GdkRGBA *st_color)
{
  int	new_color=BLACK;
  float ctol=0.15;
  
  //printf("\nGetColor: R=%f G=%f B=%f A=%f \n",st_color->red,st_color->green,st_color->blue,st_color->alpha);
  
  if(fabs(st_color->red-0.000)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.000)<ctol)new_color=BLACK;
  if(fabs(st_color->red-1.000)<ctol && fabs(st_color->blue-1.000)<ctol && fabs(st_color->green-1.000)<ctol)new_color=WHITE;

  if(fabs(st_color->red-0.937)<ctol && fabs(st_color->blue-0.161)<ctol && fabs(st_color->green-0.161)<ctol)new_color=LT_RED;
  if(fabs(st_color->red-0.800)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.000)<ctol)new_color=MD_RED;
  if(fabs(st_color->red-0.643)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.000)<ctol)new_color=DK_RED;
  
  if(fabs(st_color->red-0.988)<ctol && fabs(st_color->blue-0.243)<ctol && fabs(st_color->green-0.686)<ctol)new_color=LT_ORANGE;
  if(fabs(st_color->red-0.961)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.475)<ctol)new_color=MD_ORANGE;
  if(fabs(st_color->red-0.808)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.361)<ctol)new_color=DK_ORANGE;

  if(fabs(st_color->red-0.988)<ctol && fabs(st_color->blue-0.310)<ctol && fabs(st_color->green-0.914)<ctol)new_color=LT_YELLOW;
  if(fabs(st_color->red-0.929)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.831)<ctol)new_color=MD_YELLOW;
  if(fabs(st_color->red-0.769)<ctol && fabs(st_color->blue-0.000)<ctol && fabs(st_color->green-0.627)<ctol)new_color=DK_YELLOW;

  if(fabs(st_color->red-0.541)<ctol && fabs(st_color->blue-0.204)<ctol && fabs(st_color->green-0.886)<ctol)new_color=LT_GREEN;
  if(fabs(st_color->red-0.450)<ctol && fabs(st_color->blue-0.089)<ctol && fabs(st_color->green-0.824)<ctol)new_color=MD_GREEN;
  if(fabs(st_color->red-0.306)<ctol && fabs(st_color->blue-0.024)<ctol && fabs(st_color->green-0.604)<ctol)new_color=DK_GREEN;

  if(fabs(st_color->red-0.447)<ctol && fabs(st_color->blue-0.812)<ctol && fabs(st_color->green-0.624)<ctol)new_color=LT_BLUE;
  if(fabs(st_color->red-0.204)<ctol && fabs(st_color->blue-0.643)<ctol && fabs(st_color->green-0.396)<ctol)new_color=MD_BLUE;
  if(fabs(st_color->red-0.125)<ctol && fabs(st_color->blue-0.529)<ctol && fabs(st_color->green-0.290)<ctol)new_color=DK_BLUE;

  if(fabs(st_color->red-0.914)<ctol && fabs(st_color->blue-0.431)<ctol && fabs(st_color->green-0.725)<ctol)new_color=LT_BROWN;
  if(fabs(st_color->red-0.757)<ctol && fabs(st_color->blue-0.067)<ctol && fabs(st_color->green-0.490)<ctol)new_color=MD_BROWN;
  if(fabs(st_color->red-0.561)<ctol && fabs(st_color->blue-0.008)<ctol && fabs(st_color->green-0.349)<ctol)new_color=DK_BROWN;
  
  if(fabs(st_color->red-0.678)<ctol && fabs(st_color->blue-0.659)<ctol && fabs(st_color->green-0.498)<ctol)new_color=LT_VIOLET;
  if(fabs(st_color->red-0.459)<ctol && fabs(st_color->blue-0.482)<ctol && fabs(st_color->green-0.314)<ctol)new_color=MD_VIOLET;
  if(fabs(st_color->red-0.361)<ctol && fabs(st_color->blue-0.400)<ctol && fabs(st_color->green-0.208)<ctol)new_color=DK_VIOLET;
  
  if(fabs(st_color->red-0.533)<ctol && fabs(st_color->blue-0.521)<ctol && fabs(st_color->green-0.541)<ctol)new_color=LT_GRAY;
  if(fabs(st_color->red-0.333)<ctol && fabs(st_color->blue-0.325)<ctol && fabs(st_color->green-0.341)<ctol)new_color=MD_GRAY;
  if(fabs(st_color->red-0.180)<ctol && fabs(st_color->blue-0.212)<ctol && fabs(st_color->green-0.204)<ctol)new_color=DK_GRAY;
  
  //printf("GetColor: new_color=%d \n",new_color);
  
  return(new_color);
}


// User information - model not sliced
void on_active_model_not_sliced(void)
{
    GtkWidget	*win_local_dialog;
  
    sprintf(scratch," \nThis operation requires a sliced model.\n");
    strcat(scratch,"Select a model, slice it, then try again.");
    aa_dialog_box(win_main,0,100,"Unavailable",scratch);

    return;
}

// User information - model not loaded
void on_active_model_is_null(void)
{
    GtkWidget	*win_local_dialog;
  
    sprintf(scratch," \nThis operation is model specific.\n");
    strcat(scratch,"Select a model first, then try again.");
    aa_dialog_box(win_main,0,100,"Unavailable",scratch);

    return;
}

// User information - job in progress
void on_job_in_progress(void)
{
    GtkWidget	*win_local_dialog;
  
    sprintf(scratch," \nThis operation cannot be done \n");
    strcat(scratch,"while a job is in progress.");
    aa_dialog_box(win_main,0,100,"Unavailable",scratch);

    return;
}


// Function to increment z up by the thinnest job slice thickness
// recall that z_cut is the absolute z position in the build volume
int z_up_callback (GtkWidget *btn, gpointer user_data)
{
    int			i,slot;
    float 		old_z_cut;
    model 		*mptr;
    GtkAdjustment	*zadj;

    zadj=user_data;
    
    // if job is running, this button will add to z offset moving the tool tip away from the table
    if(job.state==JOB_RUNNING){zoffset+=set_z_manual_adj; return(TRUE);}
	
    // since this adjustment is used to scroll thru models and camera images,
    // we need to determine how to apply it
    
    // camera view - this slider acts as an image scroll
    if(main_view_page==VIEW_CAMERA)
      {
      // get new z level from slider adjustment (or from UP/DOWN button click)
      // but the slider may not provide a z level in which a slice actually exists
      if(cam_image_mode==CAM_JOB_LAPSE || cam_image_mode==CAM_SCAN_LAPSE)
        {
	scan_image_current++;
	if(scan_image_current<0)scan_image_current=0;
	if(scan_image_current>scan_image_count)scan_image_current=scan_image_count-1;
	z_cut=scan_img_index[scan_image_current];
	if(z_cut < scan_img_index[0])z_cut=scan_img_index[0];
	if(z_cut > scan_img_index[scan_image_count-1])z_cut=scan_img_index[scan_image_count-1];
	need_new_image=TRUE;
	}
      }

    // model/slice view - this slider acts as a layer scroll
    if(main_view_page==VIEW_MODEL || main_view_page==VIEW_LAYER || main_view_page==VIEW_GRAPH)
      {
      // if no models are loaded, or not sliced, inform user
      if(job.model_first==NULL){on_active_model_is_null(); return(0);}
      if(job.model_first!=NULL)
	{
	if(job.model_first->slice_first==NULL){on_active_model_not_sliced(); return(0);}
	}
  
      // increment z_cut by the thinnest amount, if sliced, to display the next slice
      // or just move the z by a default thickness if not sliced
      old_z_cut=z_cut;
      if(job.min_slice_thk>0){z_cut+=job.min_slice_thk;}		// increment by job slice thickness
      else {z_cut+=0.200;}						// or use default if not sliced yet
      
      // if above max level of all geometry, set to top of all geometry
      if(z_cut>ZMax)z_cut=ZMax;
      z_start_level=0.000;
  
      // if we are moving the model instead of the slice as is the case when milling or marking
      // on a block surface
      //if(set_start_at_crt_z==TRUE && set_force_z_to_table==TRUE)
      if(set_force_z_to_table==TRUE)
	{
	z_start_level=z_cut;
	mptr=job.model_first;
	while(mptr!=NULL)
	  {
	  // if creating a negative and there is still space left in the block...
	  //if(mptr->input_type==STL && mptr->mdl_has_target==TRUE && mptr->zoff[MODEL]>job.min_slice_thk)
	  if(mptr->mdl_has_target==TRUE && mptr->zoff[MODEL]>job.min_slice_thk)
	    {
	    // lower the model into the block as slice stays on top surface of block
	    mptr->zoff[MODEL] -= job.min_slice_thk;	
	    
	    // ensure top of model is not below top of block
	    if(mptr->zoff[MODEL]<(mptr->zmax[TARGET]-mptr->zmax[MODEL])) mptr->zoff[MODEL]=mptr->zmax[TARGET]-mptr->zmax[MODEL];
	    
	    // ensure bottom of model is not above top of block
	    if(mptr->zoff[MODEL]>mptr->zmax[TARGET]) mptr->zoff[MODEL]=mptr->zmax[TARGET];

	    // set z_cut to top surface of block
	    z_cut=mptr->zmax[TARGET];

	    printf("ZUP:  JMin=%f  JMax=%f  Zoff=%f  z_old=%f  z_cut=%f \n",job.ZMin,job.ZMax,job.model_first->zoff[MODEL],old_z_cut,z_cut);
	    }
	  mptr=mptr->next;
	  }
	}
      }
      
    gtk_adjustment_set_value(zadj,z_cut);
    
    //printf("\nZUP:     z_cut=%f \n",z_cut);

    return(TRUE);
}

// Function to decrement z up by the thinnest job slice thickness
// recall that z_cut is the absolute z position in the build volume
int z_dn_callback (GtkWidget *btn, gpointer user_data)
{
    int			i,slot;
    float 		old_z_cut;
    model 		*mptr;
    GtkAdjustment	*zadj;

    zadj=user_data;

    // if job is running, this button will adjust all tool heights by changing the global zoffset
    if(job.state==JOB_RUNNING){zoffset-=set_z_manual_adj; return(TRUE);}
    
    // since this adjustment is used to scroll thru models and camera images,
    // we need to determine how to apply it
    
    // camera view - this slider acts as an image scroll
    if(main_view_page==VIEW_CAMERA)
      {
      // get new z level from slider adjustment (or from UP/DOWN button click)
      // but the slider may not provide a z level in which a slice actually exists
      if(cam_image_mode==CAM_JOB_LAPSE || cam_image_mode==CAM_SCAN_LAPSE)
        {
	scan_image_current--;
	if(scan_image_current<0)scan_image_current=0;
	if(scan_image_current>scan_image_count)scan_image_current=scan_image_count-1;
	z_cut=scan_img_index[scan_image_current];
	if(z_cut < scan_img_index[0])z_cut=scan_img_index[0];
	if(z_cut > scan_img_index[scan_image_count-1])z_cut=scan_img_index[scan_image_count-1];
	need_new_image=TRUE;
	}
      }

    // model/slice view - this slider acts as a layer scroll
    if(main_view_page==VIEW_MODEL || main_view_page==VIEW_LAYER || main_view_page==VIEW_GRAPH)
      {
      // if no models are loaded, or not sliced, inform user
      if(job.model_first==NULL){on_active_model_is_null(); return(0);}
      if(job.model_first!=NULL)
	{
	if(job.model_first->slice_first==NULL){on_active_model_not_sliced(); return(0);}
	}
  
      // if job not running, this button will decrement the z level
      old_z_cut=z_cut;
      if(job.min_slice_thk>0){z_cut-=job.min_slice_thk;}
      else {z_cut-=0.200;}
      
      // cannot move below table level
      if(z_cut<ZMin)z_cut=ZMin;
      z_start_level=0.000;
      
      // if we are moving the model instead of the slice as is the case when milling or marking
      // on a target surface that needs to stay at a fixed z height.
      if(set_force_z_to_table==TRUE)
	{
	z_start_level=z_cut;
	mptr=job.model_first;
	while(mptr!=NULL)
	  {
	  // if creating a negative and there is still space left in the z of the target...
	  if(mptr->mdl_has_target==TRUE && mptr->zoff[MODEL]<mptr->zmax[TARGET])
	    {
	    // raise up the model out of the target as slice stays on top surface of target
	    mptr->zoff[MODEL] += job.min_slice_thk;	
	    
	    // ensure top of model is not below top of target
	    if(mptr->zoff[MODEL]<(mptr->zmax[TARGET]-mptr->zmax[MODEL])) mptr->zoff[MODEL]=mptr->zmax[TARGET]-mptr->zmax[MODEL];
	    
	    // ensure bottom of model is not above top of target
	    if(mptr->zoff[MODEL]>mptr->zmax[TARGET]) mptr->zoff[MODEL]=mptr->zmax[TARGET];
	    
	    // set z_cut to top surface of target
	    z_cut=mptr->zmax[TARGET];

	    printf("ZDN:  JMin=%f  JMax=%f  Zoff=%f  z_old=%f  z_cut=%f \n",job.ZMin,job.ZMax,job.model_first->zoff[MODEL],old_z_cut,z_cut);
	    }
	  mptr=mptr->next;
	  }
	}
      }
      
    gtk_adjustment_set_value(zadj,z_cut);

    //printf("\nZDOWN:   z_cut=%f \n",z_cut);

    return(TRUE);
}

// Function to identify closest z level when z is changed, and then load all the fill vectors
// recall that z_cut is the absolute z position in the build volume
void on_adj_z_value_changed (GtkAdjustment *adj, GtkWidget *da)
{
    int		i,slot;
    float 	old_z_cut;
    polygon	*pptr;
    slice	*sptr;
    model 	*mptr;
    genericlist	*optr;

    // since this adjustment is used to scroll thru models and camera images,
    // we need to determine how to apply it
    old_z_cut=z_cut;
    z_cut = gtk_adjustment_get_value(adj);
    
    // camera view - this slider acts as an image scroll
    if(main_view_page==VIEW_CAMERA)
      {
      if(cam_image_mode==CAM_JOB_LAPSE || cam_image_mode==CAM_SCAN_LAPSE)
        {
	for(i=0;i<scan_image_count;i++)
	  {
	  if(scan_img_index[i]>z_cut)break;
	  }
	scan_image_current=i;
	if(scan_image_current<0)scan_image_current=0;
	if(scan_image_current>scan_image_count)scan_image_current=scan_image_count-1;
	z_cut=scan_img_index[scan_image_current];
	if(z_cut < scan_img_index[0])z_cut=scan_img_index[0];
	if(z_cut > scan_img_index[scan_image_count-1])z_cut=scan_img_index[scan_image_count-1];
	need_new_image=TRUE;
	}
      }

    // model/slice view - this slider acts as a layer scroll
    if(main_view_page==VIEW_MODEL || main_view_page==VIEW_LAYER || main_view_page==VIEW_GRAPH)
      {
      // if no models are loaded, or not sliced, inform user
      if(job.model_first==NULL){on_active_model_is_null(); return;}
      if(job.model_first!=NULL)
	{
	if(job.model_first->slice_first==NULL){on_active_model_not_sliced(); return;}
	}
  
      // if running, z_cut is being set by print function
      if(job.state==JOB_RUNNING)return;
  
      // get new z level from slider adjustment (or from UP/DOWN button click)
      // but the slider may not provide a z level in which a slice actually exists
      old_z_cut=z_cut;
      z_cut = gtk_adjustment_get_value(adj);
      
      job.start_time=time(NULL);
      job.finish_time=time(NULL);
      job.current_z=z_cut;
      z_start_level=0.000;
      //if(set_start_at_crt_z==TRUE && set_force_z_to_table==TRUE)	// if moving the model and not the slice...
      if(set_force_z_to_table==TRUE)					// if moving the model and not the slice...
	{
	z_start_level=z_cut;
	mptr=job.model_first;
	while(mptr!=NULL)
	  {
	  // if creating a negative and there is still space left in the target...
	  if(mptr->mdl_has_target==TRUE)
	    {
	    //mptr->zoff[MODEL] += (old_z_cut - z_cut);			// calc how far we moved
	    mptr->zoff[MODEL]=mptr->zmax[TARGET]-(z_cut-mptr->zmax[TARGET]);
	    
	    // ensure top of model is not below top of target
	    if(mptr->zoff[MODEL]<(mptr->zmax[TARGET]-mptr->zmax[MODEL])) mptr->zoff[MODEL]=mptr->zmax[TARGET]-mptr->zmax[MODEL];
	    
	    // ensure bottom of model is not above top of target
	    if(mptr->zoff[MODEL]>mptr->zmax[TARGET]) mptr->zoff[MODEL]=mptr->zmax[TARGET];

	    // set z_cut to top surface of target
	    z_cut=mptr->zmax[TARGET];
	    
	    //printf("ZADJ:  JMin=%f  JMax=%f  Zoff=%f  z_old=%f  z_cut=%f \n",job.ZMin,job.ZMax,job.model_first->zoff[MODEL],old_z_cut,z_cut);
	    }
	    
	  mptr=mptr->next;
	  }
	}
      
      
      // loop thru all models and find their slices if they exist
      mptr=job.model_first;
      while(mptr!=NULL)
	{
	  
	// get the slot(tool) the model is sliced for/with
	slot=(-1);
	optr=mptr->oper_list;
	while(optr!=NULL)
	  {
	  slot=optr->ID;
	  if(slot>=0 && slot<MAX_TOOLS)break;
	  optr=optr->next;
	  }
	  
	// if a valid slot, get the slice and reset the job time estimates
	if(slot>=0 && slot<MAX_TOOLS)
	  {
	  sptr=slice_find(mptr,MODEL,slot,z_cut,job.min_slice_thk);
	  
	  //if(sptr!=NULL)printf("ZADJ:  sptr->sz_level=%f  z_cut=%f  zoff=%f \n",sptr->sz_level,z_cut,mptr->zoff[MODEL]);
	  
	  while(sptr!=NULL)						// recall sliced from top down, so "next" sptr
	    {								// is the one below the current one.
	    job.finish_time += (sptr->time_estimate*mptr->total_copies);
	    sptr=sptr->next;
	    }
	  }
	  
	mptr->current_z=z_cut;						// set z height of this model
	
	mptr=mptr->next;						// move onto the next model
	}
      
      // and now that an appropriate z is set, go create all the fills and close-offs etc.
      slice_fill_all(z_cut);
      }
      
    // finally, update the window with new information
    gtk_widget_queue_draw(win_main);
    
    return;
}


// Callback to set response for unavailable operation
void cb_unavailable_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    gtk_window_close(GTK_WINDOW(dialog));
    return;
}


void cb_job_regen_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    gint *reslice = (gint*)user_data;
    if(response_id==GTK_RESPONSE_YES)*reslice=TRUE;
    gtk_window_close(GTK_WINDOW(dialog));
    return;
}

int on_job_will_need_regen(void)
{
    int		reslice=FALSE;
    GtkWidget	*win_local_dialog;
  
    sprintf(scratch," \nThis operation will force reslicing.\n");
    strcat(scratch,"Select YES to confirm.");
    aa_dialog_box(win_main,3,0,"Reslice",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    reslice=aa_dialog_result;

    return(reslice);
}


// Functions to support the model view buttons
void model_add_callback (GtkWidget *btn, gpointer user_data)
{
    
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}

    model_file_load(NULL,win_main);

    return;
}

void model_sub_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL)
      {
      on_active_model_is_null();
      return;
      }
    model_file_remove(NULL,NULL);
    job_maxmin();

    gtk_widget_queue_draw(win_main);
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    
    return;
}

void model_select_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL)
      {
      active_model=job.model_first;
      }
    else 
      {
      active_model=active_model->next;
      if(active_model==NULL)active_model=job.model_first;
      }
    
    return;
}

void model_clear_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    active_model=NULL;
    return;
}

void model_options_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL)
      {
      on_active_model_is_null();
      return;
      }
    
    build_options_anytime_cb(NULL,win_main);

    gtk_widget_queue_draw(win_main);
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    return;
}

void model_align_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL)
      {
      on_active_model_is_null();
      return;
      }

    return;
}

void model_XF_bulk_callback (GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    int		mtyp=MODEL;
    int		rot_increment=90;
    float 	fval;

    printf("\nX bulk rotate entry...\n");

    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL){on_active_model_is_null(); return;}
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // non-vtx based model need to be treated differently.  instead of moving their vtx to a new
    // location, non-vtx models will just get their rotation values applied dynamically at
    // display/processing/print time.
    active_model->xrot[mtyp] += rot_increment*PI/180;
    if(active_model->xrot[mtyp] > (359*PI/180))active_model->xrot[mtyp]=0;
    model_purge(active_model);						// purge support and slice data
    if(active_model->input_type==IMAGE || active_model->input_type==GERBER)
      {
      }
    else 
      {
      model_rotate(active_model);					// apply rotations
      model_maxmin(active_model);					// re-establish its boundaries
      job_maxmin();							// re-establish its boundaries
  
      // swap y and z scale values
      //fval=active_model->yscl[mtyp];
      //active_model->yscl[mtyp]=active_model->zscl[mtyp];
      //active_model->zscl[mtyp]=fval;
      //fval=active_model->yspr[mtyp];
      //active_model->yspr[mtyp]=active_model->zspr[mtyp];
      //active_model->zspr[mtyp]=fval;
      }

    // reset redraw flag for this model
    active_model->MRedraw_flag=TRUE;
  
    gtk_widget_queue_draw(g_model_area);				// update the model display area
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    
    printf("\nX Bulk Rotate: %f \n\n",active_model->xrot[mtyp]*180/PI);
    
    return;
}

void model_XF_fine_callback (GtkGestureClick *gesture, int button, double x, double y, gpointer da)
{
    int		mtyp=MODEL;
    int		rot_increment=1;
    float 	fval;

    printf("\nX fine rotate entry...\n");

    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL){on_active_model_is_null(); return;}
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // non-vtx based model need to be treated differently.  instead of moving their vtx to a new
    // location, non-vtx models will just get their rotation values applied dynamically at
    // display/processing/print time.
    active_model->xrot[mtyp] += rot_increment*PI/180;
    if(active_model->xrot[mtyp] > (359*PI/180))active_model->xrot[mtyp]=0;
    model_purge(active_model);						// purge support and slice data
    if(active_model->input_type==IMAGE || active_model->input_type==GERBER)
      {
      }
    else 
      {
      model_rotate(active_model);						// apply rotations
      model_maxmin(active_model);						// re-establish its boundaries
      job_maxmin();							// re-establish its boundaries
  
      }

    // reset redraw flag for this model
    active_model->MRedraw_flag=TRUE;
  
    gtk_widget_queue_draw(g_model_area);				// update the model display area
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    
    printf("\nX Fine Rotate: %f \n\n",active_model->xrot[mtyp]*180/PI);
    
    return;
}

void model_XF_callback (GtkWidget *btn, gpointer user_data)
{
    int		mtyp=MODEL;
    float 	fval;
    
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL){on_active_model_is_null(); return;}
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    //if(job.state>=JOB_HEATING && job.state<JOB_RUNNING){if(on_job_will_need_regen()==FALSE)return;}
    
    active_model->xrot[mtyp] += 90*PI/180;
    if(active_model->xrot[mtyp] > (359*PI/180))active_model->xrot[mtyp]=0;
    model_purge(active_model);						// purge support and slice data
    model_rotate(active_model);						// apply rotations
    model_maxmin(active_model);						// re-establish its boundaries
    job_maxmin();							// re-establish its boundaries
    
    // swap y and z scale values
    fval=active_model->yscl[mtyp];
    active_model->yscl[mtyp]=active_model->zscl[mtyp];
    active_model->zscl[mtyp]=fval;
    fval=active_model->yspr[mtyp];
    active_model->yspr[mtyp]=active_model->zspr[mtyp];
    active_model->zspr[mtyp]=fval;

    // reset redraw flag for this model
    active_model->MRedraw_flag=TRUE;
  
    gtk_widget_queue_draw(win_main);
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    return;
}

void model_YF_callback (GtkWidget *btn, gpointer user_data)
{
    int		mtyp=MODEL;
    float 	fval;
    
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL){on_active_model_is_null(); return;}
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    //if(job.state>=JOB_HEATING && job.state<JOB_RUNNING){if(on_job_will_need_regen()==FALSE)return;}
    
    active_model->yrot[mtyp] += 90*PI/180;
    if(active_model->yrot[mtyp] > (359*PI/180))active_model->yrot[mtyp]=0;
    model_purge(active_model);						// purge support and slice data
    model_rotate(active_model);						// apply rotations
    model_maxmin(active_model);						// re-establish its boundaries
    job_maxmin();							// re-establish its boundaries
    
    // swap x and z scale values
    fval=active_model->xscl[mtyp];
    active_model->xscl[mtyp]=active_model->zscl[mtyp];
    active_model->zscl[mtyp]=fval;
    fval=active_model->xspr[mtyp];
    active_model->xspr[mtyp]=active_model->zspr[mtyp];
    active_model->zspr[mtyp]=fval;

    // reset redraw flag for this model
    active_model->MRedraw_flag=TRUE;
  
    gtk_widget_queue_draw(win_main);
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    return;
}

void model_ZF_callback (GtkWidget *btn, gpointer user_data)
{
    int		mtyp=MODEL;
    float 	fval;
    
    if(job.state>=JOB_RUNNING && job.state<=JOB_PAUSED_DUE_TO_ERROR){on_job_in_progress(); return;}
    if(active_model==NULL){on_active_model_is_null(); return;}
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    //if(job.state>=JOB_HEATING && job.state<JOB_RUNNING){if(on_job_will_need_regen()==FALSE)return;}

    active_model->zrot[mtyp] += 90*PI/180;
    if(active_model->zrot[mtyp] > (359*PI/180))active_model->zrot[mtyp]=0;
    model_purge(active_model);					// purge support and slice data
    model_rotate(active_model);					// apply rotations
    model_maxmin(active_model);					// re-establish its boundaries
    job_maxmin();							// re-establish its boundaries

    // swap y and x scale values
    fval=active_model->yscl[mtyp];
    active_model->yscl[mtyp]=active_model->xscl[mtyp];
    active_model->xscl[mtyp]=fval;
    fval=active_model->yspr[mtyp];
    active_model->yspr[mtyp]=active_model->xspr[mtyp];
    active_model->xspr[mtyp]=fval;

    // reset redraw flag for this model
    active_model->MRedraw_flag=TRUE;
  
    gtk_widget_queue_draw(win_main);
    while (g_main_context_iteration(NULL, FALSE));			// update display promptly
    return;
}

void model_view_callback (GtkWidget *btn, gpointer user_data)
{
  model 	*mptr;
  
  if(MVview_click==0)
    {MVview_scale=1.2; MVgxoffset=425; MVgyoffset=375; MVdisp_spin=(-37*PI/180); MVdisp_tilt=(210*PI/180);}
  if(MVview_click==1)
    {MVview_scale=1.5; MVgxoffset=425; MVgyoffset=475; MVdisp_spin=(0*PI/180);  MVdisp_tilt=(180*PI/180);}
  if(MVview_click==2)
    {MVview_scale=1.5; MVgxoffset=425; MVgyoffset=475; MVdisp_spin=(270*PI/180); MVdisp_tilt=(180*PI/180);}
  if(MVview_click==3)
    {MVview_scale=1.5; MVgxoffset=425; MVgyoffset=250; MVdisp_spin=(0*PI/180);  MVdisp_tilt=(270*PI/180);}
  MVview_click++;
  if(MVview_click>3)MVview_click=0;

  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->MRedraw_flag=TRUE;
    mptr=mptr->next;
    }
  
  set_auto_zoom=TRUE;
  job_maxmin();
  gtk_widget_queue_draw(win_main);
  while (g_main_context_iteration(NULL, FALSE));			// update display promptly
  return;
}

void select_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING)return;
    set_mouse_clear=0;
    set_mouse_select=0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))set_mouse_select=1;
    set_mouse_drag=0;
    set_mouse_align=0;
    set_mouse_center=0;
    set_mouse_merge=0;
    return;
}

void clear_callback (GtkWidget *btn, gpointer user_data)
{
    vector	*vecptr,*vecdel;
    
    if(job.state>=JOB_RUNNING)return;
    
    p_pick_list=polygon_list_manager(p_pick_list,NULL,ACTION_CLEAR);
    active_model=NULL;
    fct_pick_ref=NULL;
    vtx_pick_ptr=NULL;
    vtx_pick_ref->x=0;
    vtx_pick_ref->y=0;
    vtx_pick_ref->z=0;
    set_mouse_clear=0;
    set_mouse_select=0;
    set_mouse_drag=0;
    set_mouse_align=0;
    set_mouse_center=0;
    set_mouse_merge=0;
    
    printf("\nPick list cleared.\n\n");
    printf("\nTotal Vector Count:  %ld \n",total_vector_count);

    // clear vector list
    vecptr=vec_debug;
    vecptr=NULL;
    while(vecptr!=NULL)
      {
      vecdel=vecptr;
      vecptr=vecptr->next;
      free(vecdel);vector_mem--;
      if(vecptr==vec_debug)break;
      }
    vec_debug=NULL;
    
    set_mouse_select=1;
    
    return;
}

void drag_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING)return;
    set_mouse_clear=0;
    set_mouse_select=0;
    set_mouse_drag=0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))set_mouse_drag=1;
    set_mouse_align=0;
    set_mouse_center=0;
    set_mouse_merge=0;
    return;
}

void align_callback (GtkWidget *btn, gpointer user_data)
{
    if(job.state>=JOB_RUNNING)return;
    set_mouse_clear=0;
    set_mouse_select=0;
    set_mouse_drag=0;
    set_mouse_align=0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))set_mouse_align=1;
    set_mouse_center=0;
    set_mouse_merge=0;
    return;
}


// Functions to set the layer range (by height) to be viewed and/or printed
void get_layer_height_value(GtkSpinButton *button, gpointer user_data)
{
  int		val_typ;
  
  val_typ=GPOINTER_TO_INT(user_data);					// get details of what this call is for
  
  // get starting value
  if(val_typ==1)
    {
    slc_view_start=gtk_spin_button_get_value(button);
    slc_just_one=FALSE;
    slc_show_all=FALSE;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(show_all_btn),slc_show_all);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(just_current_btn),slc_just_one);
    gtk_widget_queue_draw(show_all_btn);
    gtk_widget_queue_draw(just_current_btn);
    }
  // get ending value
  if(val_typ==2)
    {
    slc_view_end=gtk_spin_button_get_value(button);
    slc_just_one=FALSE;
    slc_show_all=FALSE;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(show_all_btn),slc_show_all);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(just_current_btn),slc_just_one);
    gtk_widget_queue_draw(show_all_btn);
    gtk_widget_queue_draw(just_current_btn);
    }
  // get increment value
  if(val_typ==3)
    {
    slc_view_inc=gtk_spin_button_get_value(button);
    //if(slc_view_inc<job.min_slice_thk)slc_view_inc=job.min_slice_thk;
    //if(slc_view_inc>ZMax)slc_view_inc=ZMax;
    }
  // set to just one (current) layer
  if(val_typ==4)
    {
    slc_just_one = !slc_just_one;
    if(slc_just_one==TRUE)
      {
      slc_show_all=FALSE;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(show_all_btn),slc_show_all);
      gtk_widget_queue_draw(show_all_btn);
      }
    slc_view_start=0.0;
    gtk_adjustment_set_value(fill_thk_adj,slc_view_start);
    slc_view_end=0.0;
    gtk_adjustment_set_value(fill_pitch_adj,slc_view_end);
    }
  // set to show all layers
  if(val_typ==5)
    {
    slc_show_all = !slc_show_all;
    if(slc_show_all==TRUE)
      {
      slc_just_one=FALSE;
      gtk_check_button_set_active(GTK_CHECK_BUTTON(just_current_btn),slc_just_one);
      gtk_widget_queue_draw(just_current_btn);
      }
    slc_view_start=ZMax;
    gtk_adjustment_set_value(fill_thk_adj,slc_view_start);
    slc_view_end=ZMax;
    gtk_adjustment_set_value(fill_pitch_adj,slc_view_end);
    }
  // set as print
  if(val_typ==6)
    {
    slc_print_view = !slc_print_view;
    }

  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void set_layer_height_done_callback (GtkWidget *btn, gpointer dead_window)
{

  gtk_window_close(GTK_WINDOW(dead_window));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}

void set_layer_heights_callback (GtkWidget *btn, gpointer user_data)
  {
  GtkWidget	*win_bo, *btn_exit, *img_exit, *lbl_new, *win_local_dialog;
  GtkWidget	*hbox, *hbox_up, *hbox_dn, *vbox;
  GtkWidget	*count_frame, *grd_count;
  GtkWidget	*lbl_start_count,*lbl_end_count,*lbl_inc_count;
  GtkWidget	*set_print_view;
  
  float 	fval;
  model 	*mptr;

  // make sure we have slices to work with
  //if(job.max_layer_count<1)
  //  {
  //  sprintf(scratch," \nThere are no layers to display.\n");
  //  strcat(scratch,"Open a model and slice it first.");
  //  aa_dialog_box(win_model,1,0,"Not Applicable",scratch);
  //  return;
  //  }

  // set up primary window
  win_bo = gtk_window_new ();				
  gtk_window_set_transient_for (GTK_WINDOW(win_bo), GTK_WINDOW(win_main));
  gtk_window_set_modal (GTK_WINDOW(win_bo),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_bo),300,200);
  gtk_window_set_resizable(GTK_WINDOW(win_bo),FALSE);			
  sprintf(scratch,"Layer Display Count");
  gtk_window_set_title(GTK_WINDOW(win_bo),scratch);			

  // set up an hbox to divide the screen into a narrower segments
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
  //gtk_box_set_spacing (GTK_BOX(hbox),20);
  gtk_window_set_child (GTK_WINDOW (win_bo), hbox);

  // now divide left side into segments for buttons
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
  gtk_box_append(GTK_BOX(hbox),vbox);
    
  // and further divide the right side into quads - two "up" and two "dn"
  hbox_up=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_up);
  hbox_dn=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_dn);

  // set up EXIT button
  btn_exit = gtk_button_new ();
  g_signal_connect (btn_exit, "clicked", G_CALLBACK (set_layer_height_done_callback), win_bo);
  img_exit = gtk_image_new_from_file("Back.gif");
  gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
  gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
  gtk_box_append(GTK_BOX(vbox),btn_exit);

  
  // set up count spin buttons
  {
    count_frame = gtk_frame_new("Slice View Heights");
    GtkWidget *count_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(count_frame), count_box);
    gtk_widget_set_size_request(count_frame,350,100);
    gtk_box_append(GTK_BOX(hbox_up),count_frame);

    grd_count = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(count_box),grd_count);
    gtk_grid_set_row_spacing (GTK_GRID(grd_count),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_count),15);

    slc_start_adj=gtk_adjustment_new(slc_view_start, 0.0, job.ZMax, job.min_slice_thk, 1, 10);
    slc_start_btn=gtk_spin_button_new(slc_start_adj, 6, 3);
    lbl_start_count=gtk_label_new("Distance below current height:");
    gtk_grid_attach (GTK_GRID (grd_count), lbl_start_count, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_count), slc_start_btn, 1, 0, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(slc_start_btn),"value-changed",G_CALLBACK(get_layer_height_value),GINT_TO_POINTER(1));
    
    slc_end_adj=gtk_adjustment_new(slc_view_end, 0.0, job.ZMax, job.min_slice_thk, 1, 10);
    slc_end_btn=gtk_spin_button_new(slc_end_adj, 6, 3);
    lbl_end_count=gtk_label_new("Distance above current height:");
    gtk_grid_attach (GTK_GRID (grd_count), lbl_end_count, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_count), slc_end_btn, 1, 1, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(slc_end_btn),"value-changed",G_CALLBACK(get_layer_height_value),GINT_TO_POINTER(2));
    
    slc_inc_adj=gtk_adjustment_new(slc_view_inc, job.min_slice_thk, job.ZMax, job.min_slice_thk, 1, 10);
    slc_inc_btn=gtk_spin_button_new(slc_inc_adj, 6, 3);
    lbl_inc_count=gtk_label_new("Height increment:");
    gtk_grid_attach (GTK_GRID (grd_count), lbl_inc_count, 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_count), slc_inc_btn, 1, 2, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(slc_inc_btn),"value-changed",G_CALLBACK(get_layer_height_value),GINT_TO_POINTER(3));
    
  }

  just_current_btn=gtk_check_button_new_with_label ("Current layer ");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(just_current_btn), slc_just_one);
  g_signal_connect (GTK_CHECK_BUTTON (just_current_btn), "toggled", G_CALLBACK (get_layer_height_value),GINT_TO_POINTER(4));
  gtk_grid_attach(GTK_GRID (grd_count),just_current_btn, 0, 3, 1, 1);

  show_all_btn=gtk_check_button_new_with_label ("All layers ");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(show_all_btn), slc_show_all);
  g_signal_connect (GTK_CHECK_BUTTON (show_all_btn), "toggled", G_CALLBACK (get_layer_height_value),GINT_TO_POINTER(5));
  gtk_grid_attach(GTK_GRID (grd_count),show_all_btn, 1, 3, 1, 1);
  
  set_print_view=gtk_check_button_new_with_label ("Use same for print ");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(set_print_view), slc_print_view);
  g_signal_connect (GTK_CHECK_BUTTON (set_print_view), "toggled", G_CALLBACK (get_layer_height_value),GINT_TO_POINTER(6));
  gtk_grid_attach(GTK_GRID (grd_count),set_print_view, 0, 4, 1, 1);
  

  gtk_widget_set_visible(win_bo,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}


// Remaining functions to support slice view buttons
void set_layer_pause_callback (GtkWidget *btn, gpointer user_data)
{
    int		i,slot;
    polygon	*pptr;
    slice	*sptr;
    model 	*mptr;
    genericlist	*optr;

    /*
    // Old center callback
    if(job.state>=JOB_RUNNING)return;
    set_mouse_clear=0;
    set_mouse_select=0;
    set_mouse_drag=0;
    set_mouse_align=0;
    set_mouse_center=0;
    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)))set_mouse_center=1;
    set_mouse_merge=0;
    */

    // get slice based on current z level
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      slot=(-1);
      optr=mptr->oper_list;
      while(optr!=NULL)
        {
	slot=optr->ID;
	if(slot>=0 && slot<MAX_TOOLS)break;
	optr=optr->next;
	}
      if(slot<0){mptr=mptr->next;continue;}
      sptr=slice_find(mptr,MODEL,slot,z_cut,job.min_slice_thk);
      if(sptr!=NULL)
        {
	sptr->eol_pause++;
	if(sptr->eol_pause>1)sptr->eol_pause=0;
	break;
	}
      mptr=mptr->next;
      }

    return;
}

void intersect_callback (GtkWidget *btn, gpointer user_data)
{
  int		result,slot;
  polygon_list	*pl_ptr,*pl_nxt,*pl_pre,*pl_pnt;
  polygon	*pptr,*ppre,*pnew;
  slice		*sptr;
  model		*mptr;
  genericlist	*aptr;
  
  
  if(job.state>=JOB_RUNNING)return;

  // determine which slice/tool we are dealing with
  //mptr=active_model;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    aptr=mptr->oper_list;							// look thru all operations called out by this model
    while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation
      {
      slot=aptr->ID;
      if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
      if(Tool[slot].state<TL_READY){aptr=aptr->next;continue;}
      sptr=slice_find(mptr,MODEL,slot,z_cut,job.min_slice_thk);
      if(sptr!=NULL)break;
      aptr=aptr->next;
      }
      
    printf("Call: mptr=%X slot=%d sptr=%X \n",mptr,slot,sptr);
      
    mptr=mptr->next;
    }
   
  /* polygon intersecton
  //if(active_model==NULL)return;
  if(sptr!=NULL)
    {
    pl_ptr=p_pick_list;
    while(pl_ptr!=NULL)
      {
      ppre=pl_ptr->p_item;
      
      //pl_nxt=p_pick_list;
      pl_nxt=pl_ptr->next;
      
      while(pl_nxt!=NULL)
	{
	pptr=pl_nxt->p_item;
	if(pptr==ppre){pl_nxt=pl_nxt->next;continue;}
	
	result=polygon_intersect(ppre,pptr,LOOSE_CHECK);
	
	pl_nxt=pl_nxt->next;
	}
      pl_ptr=pl_ptr->next;
      }
    }
  */
    
    return;
}

void merge_polygon_callback (GtkWidget *btn, gpointer user_data)
{
  int		result,slot,action;
  polygon_list	*pl_ptr,*pl_nxt,*pl_pre,*pl_pnt;
  polygon	*pptr,*ppre,*pnew;
  model		*mptr;
  genericlist	*aptr;
  slice		*sptr,*snxt,*snew;
  vector 	*vecptr;
  vector_list 	*vl_ptr;
    
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  action=GPOINTER_TO_INT(user_data);

  // model op list = slot  and op_name
  // tool op list  = op_ID and op_name

  // determine which slice/tool we are dealing with
  sptr=NULL;
  mptr=active_model;
  aptr=mptr->oper_list;							// look thru all operations called out by this model
  while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation
    {
    slot=aptr->ID;
    if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
    if(Tool[slot].state<TL_READY){aptr=aptr->next;continue;}
    sptr=slice_find(mptr,MODEL,slot,z_cut,(job.min_slice_thk));
    if(sptr!=NULL)
      {
      snxt=sptr->next;
      break;
      }
    aptr=aptr->next;
    }
  
    
/*   
  if(sptr==NULL || snxt==NULL)
    {
    printf("Slice not found!  sptr=%X  snxt=%X \n",sptr,snxt);
    return;
    }
  
  // add this slice to next...
  if(action==ACTION_ADD)
    {
    snew=slice_boolean(mptr,sptr,snxt,MDL_PERIM,MDL_PERIM,TEST_LC,ACTION_ADD);
    pptr=snew->pfirst[TEST_LC];
    while(pptr!=NULL)
      {
      polygon_insert(sptr,TEST_LC,pptr);
      pptr=pptr->next;
      }
    }
   
  // subtract this slice from the next...
  if(action==ACTION_SUBTRACT)
    {
    snew=slice_boolean(mptr,snxt,sptr,MDL_PERIM,MDL_PERIM,TEST_UC,ACTION_SUBTRACT);
    pptr=snew->pfirst[TEST_LC];
    while(pptr!=NULL)
      {
      polygon_insert(sptr,TEST_LC,pptr);
      pptr=pptr->next;
      }
    }
*/
   
 
  // add by polygon.... 
  if(sptr!=NULL)
    {
    pl_ptr=p_pick_list;

    pptr=NULL; ppre=NULL;
    if(pl_ptr!=NULL)
      {
      ppre=pl_ptr->p_item;
      pl_nxt=pl_ptr->next;
      if(pl_nxt!=NULL)pptr=pl_nxt->p_item;
      }
    if(pptr==ppre)return;
    if(pptr==NULL || ppre==NULL)return;
    p_pick_list=polygon_list_manager(p_pick_list,NULL,ACTION_CLEAR);
    
    //debug_flag=199;
    //polygon_intersect(ppre,pptr,TOLERANCE);
    //debug_flag=0;
    //return;
    
    pnew=polygon_boolean2(sptr,ppre,pptr,action);
    
    if(pnew!=NULL)
      {
      printf("\npnew %X created with %d verticies \n",pnew,pnew->vert_qty);
      printf("pnew=%X ppre=%X pptr=%X \n\n",pnew,ppre,pptr);
      pnew->type=ppre->type;
      if(pnew==ppre)polygon_delete(sptr,pptr);				// if A enclosed B, keep A and delete B
      if(pnew==pptr)polygon_delete(sptr,ppre);				// if B enclosed A, keep B and delete A
      if(pnew!=NULL && pnew!=pptr && pnew!=ppre)			// if neither was entirely inside the other...
        {
	polygon_delete(sptr,ppre);					// ... delete A
	polygon_delete(sptr,pptr);					// ... delete B
	polygon_insert(sptr,pnew->type,pnew);				// ... add the new one
	}
      }
    }
    
    return;
}


// Functions to adjust perimeter values of a group of selected polygons
void apply_perim_value_to_plist(void)
{
  int		slot;
  polygon 	*pptr;
  polygon_list	*ppick;
  slice 	*sptr;
  linetype	*ltptr;
  
  // determine what slice we are on
  slot=model_get_slot(active_model,OP_MARK_OUTLINE);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_AREA);
  if(slot<0 || slot>=MAX_TOOLS)slot=0;
  sptr=slice_find(active_model, MODEL, slot, z_cut, active_model->slice_thick);
  ltptr=linetype_find(slot,MDL_PERIM);

  // now apply the new settings to the polygon list
  ppick=p_pick_list;
  while(ppick!=NULL)
    {
    pptr=ppick->p_item;
    if(poly_perim_none==TRUE)						// if reset to default requested...
      {
      pptr->perim_type=(-1);						// ... turn off custom offset of this poly
      }
    else 								// but if custom values still wanted...
      {
      pptr->perim_type=2;						// ... set to custom offset type
      if(pptr->perim_lt==NULL)pptr->perim_lt=linetype_copy(ltptr);	// ... if not there yet, make a copy of the default line type
      if(pptr->perim_lt!=NULL)
        {
	(pptr->perim_lt)->flowrate=poly_perim_flow;			// ... and set poly values
	(pptr->perim_lt)->feedrate=poly_perim_feed;		
	(pptr->perim_lt)->line_width=poly_perim_thick;	
	(pptr->perim_lt)->line_pitch=poly_perim_pitch;
	}
      }
    
    // wipe out existing perim offsets for this polygon and re-offset with new params
    if(sptr!=NULL)
      {
      //printf("  Re-offsetting polygon %X \n",pptr);
      polygon_wipe_offset(sptr,pptr,MDL_PERIM,5.0);
      polygon_make_offset(sptr,pptr);
      }
    ppick=ppick->next;
    }

  return;
}

void apply_fill_value_to_plist(void)
{
  int		slot;
  polygon 	*pptr;
  polygon_list	*ppick;
  slice 	*sptr;
  linetype 	*ltptr;
  
  // determine what slice we are on
  slot=model_get_slot(active_model,OP_MARK_OUTLINE);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_AREA);
  if(slot<0 || slot>=MAX_TOOLS)slot=0;
  sptr=slice_find(active_model, MODEL, slot, z_cut, active_model->slice_thick);
  ltptr=linetype_find(slot,MDL_FILL);

  // now apply the new settings to the polygon list
  ppick=p_pick_list;
  while(ppick!=NULL)
    {
    pptr=ppick->p_item;
    if(poly_fill_none==TRUE)						// if default values requested...
      {	
      // set to bypass custom fill settings. note that we don't want to mess with the pptrs linetype
      // values here as other polys may be referencing that same linetype pointer.  so just turn it off.
      pptr->fill_type=1;						// ... reset to "from file"					
      //printf("  setting pptr=%X to fill type %d \n",pptr,pptr->fill_type);
      }
    else 								// but if user still wants custom...
      {
      pptr->fill_type=2;						// ... set to custom fill type
      if(pptr->fill_lt==NULL)pptr->fill_lt=linetype_copy(ltptr);	// ... if not there yet, make a copy of the default line type
      if(pptr->fill_lt!=NULL)
        {
	(pptr->fill_lt)->flowrate=poly_fill_flow;			// ... and set poly values
	(pptr->fill_lt)->feedrate=poly_fill_feed;		
	(pptr->fill_lt)->line_width=poly_fill_thick;	
	(pptr->fill_lt)->line_pitch=poly_fill_pitch;
	(pptr->fill_lt)->p_angle=poly_fill_angle;
	}
      }
    
    // wipe out existing fill for this polygon and refill with new params
    if(sptr!=NULL)
      {
      //printf("  re-filling polygon %X at sptr->z=%f \n",pptr,sptr->sz_level);
      polygon_wipe_fill(sptr,pptr);
      if(pptr->fill_type>1)polygon_make_fill(sptr,pptr);
      }
    ppick=ppick->next;
    }

  return;
}

void get_perim_flow_value(GtkSpinButton *button, gpointer user_data)
{
  poly_perim_flow=gtk_spin_button_get_value(button);
  return;
}

void get_perim_feed_value(GtkSpinButton *button, gpointer user_data)
{
  poly_perim_feed=gtk_spin_button_get_value(button);
  return;
}

void get_perim_thick_value(GtkSpinButton *button, gpointer user_data)
{
  poly_perim_thick=gtk_spin_button_get_value(button);
  apply_perim_value_to_plist();
  return;
}

void get_perim_pitch_value(GtkSpinButton *button, gpointer user_data)
{
  poly_perim_pitch=gtk_spin_button_get_value(button);
  apply_perim_value_to_plist();
  return;
}

void get_perim_type_value(GtkButton *button, gpointer user_data)
{
  int		val_typ,slot;
  
  val_typ=GPOINTER_TO_INT(user_data);					// get details of what this call is for
  
  printf("GPTV: entry  val_typ=%d \n",val_typ);
  
  // reset polys in pick list to default perim type
  if(val_typ==4)
    {
    poly_perim_none = !poly_perim_none;
    }
  // set all polys in this slice with this definitiion
  if(val_typ==5)
    {
    poly_perim_all = !poly_perim_all;
    }
  // set to fill all in this slice but each with a unique type
  if(val_typ==6)
    {
    poly_perim_unique = !poly_perim_unique;
    }

  apply_perim_value_to_plist();
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  printf("GPTV: exit\n");
  
  return;
}


// Functions to adjust fill values of a group of selected polygons
void get_fill_flow_value(GtkSpinButton *button, gpointer user_data)
{
  poly_fill_flow=gtk_spin_button_get_value(button);
  return;
}

void get_fill_feed_value(GtkSpinButton *button, gpointer user_data)
{
  poly_fill_feed=gtk_spin_button_get_value(button);
  return;
}

void get_fill_thick_value(GtkSpinButton *button, gpointer user_data)
{
  poly_fill_thick=gtk_spin_button_get_value(button);
  apply_fill_value_to_plist();
  return;
}

void get_fill_pitch_value(GtkSpinButton *button, gpointer user_data)
{
  poly_fill_pitch=gtk_spin_button_get_value(button);
  apply_fill_value_to_plist();
  return;
}

void get_fill_angle_value(GtkSpinButton *button, gpointer user_data)
{
  poly_fill_angle=gtk_spin_button_get_value(button);
  apply_fill_value_to_plist();
  return;
}

void get_fill_type_value(GtkButton *button, gpointer user_data)
{
  int		val_typ,slot;
  
  val_typ=GPOINTER_TO_INT(user_data);					// get details of what this call is for
  
  printf("GFTV: entry  val_typ=%d \n",val_typ);
  
  // reset polys in pick list to default fill type
  if(val_typ==4)
    {
    // process global and UI
    poly_fill_none = !poly_fill_none;
    printf("   poly_fill_none=%d \n",poly_fill_none);
    }
  // set all polys in this slice with this definitiion
  if(val_typ==5)
    {
    poly_fill_all = !poly_fill_all;
    printf("   poly_fill_all=%d \n",poly_fill_all);
    }
  // set to fill all in this slice but each with a unique type
  if(val_typ==6)
    {
    poly_fill_unique = !poly_fill_unique;
    printf("   poly_fill_unique=%d \n",poly_fill_unique);
    }

  apply_fill_value_to_plist();
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  printf("GFTV: exit\n");
  
  return;
}


// Functions to display polygon attributes (diaglog for perims and fill adjustments)
static void on_poly_nb_page_switch (GtkWidget *g_notebook, GtkWidget *page, gint page_num)
{
  
  return;
}

void set_polygon_attr_callback (GtkWidget *btn, gpointer user_data)
{
  GtkWidget	*win_bo, *btn_exit, *img_exit, *lbl_new, *win_local_dialog;
  GtkWidget	*hbox, *hbox_up, *hbox_dn, *vbox;
  GtkWidget	*poly_notebook;
  GtkWidget	*perim_box,*lbl_perim_frame, *grd_perim;
  GtkWidget	*fill_box,*lbl_fill_frame, *grd_fill;
  GtkWidget	*lbl_start_count,*lbl_end_count,*lbl_inc_count;
  GtkWidget	*set_print_view;
  
  int		action,slot;
  float 	fval;
  float 	temp_perim_flw,temp_perim_fed,temp_perim_thk,temp_perim_pitch;
  float 	temp_fill_flw,temp_fill_fed,temp_fill_thk,temp_fill_pitch,temp_fill_angle;
  polygon	*pptr;
  model 	*mptr;
  linetype	*lptr;

  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  action=GPOINTER_TO_INT(user_data);

  // if no polygons are selected, just leave.  otherwise get first in list.
  if(p_pick_list==NULL)return;
  pptr=p_pick_list->p_item;						// get first poly off list
  
  // set to global values
  temp_perim_flw=poly_perim_flow;
  temp_perim_fed=poly_perim_feed;
  temp_perim_thk=poly_perim_thick;
  temp_perim_pitch=poly_perim_pitch;
  temp_fill_flw=poly_fill_flow;
  temp_fill_fed=poly_fill_feed;
  temp_fill_thk=poly_fill_thick;
  temp_fill_pitch=poly_fill_pitch;
  temp_fill_angle=poly_fill_angle;
  
  // if the polygon already has previous values defined, use them instead
  if(pptr->perim_type==2 && pptr->perim_lt!=NULL)
    {
    temp_perim_flw=(pptr->perim_lt)->flowrate;
    temp_perim_fed=(pptr->perim_lt)->feedrate;
    temp_perim_thk=(pptr->perim_lt)->line_width;
    temp_perim_pitch=(pptr->perim_lt)->line_pitch;
    }
  if(pptr->fill_type==2 && pptr->fill_lt!=NULL)
    {
    temp_fill_flw=(pptr->fill_lt)->flowrate;
    temp_fill_fed=(pptr->fill_lt)->feedrate;
    temp_fill_thk=(pptr->fill_lt)->line_width;
    temp_fill_pitch=(pptr->fill_lt)->line_pitch;
    temp_fill_angle=(pptr->fill_lt)->p_angle;
    }

  // set up primary window
  win_bo = gtk_window_new ();				
  gtk_window_set_transient_for (GTK_WINDOW(win_bo), GTK_WINDOW(win_main));
  gtk_window_set_modal (GTK_WINDOW(win_bo),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_bo),300,200);
  gtk_window_set_resizable(GTK_WINDOW(win_bo),FALSE);			
  sprintf(scratch,"Polygon Attributes");
  gtk_window_set_title(GTK_WINDOW(win_bo),scratch);			

  // set up an hbox to divide the screen into a narrower segments
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
  //gtk_box_set_spacing (GTK_BOX(hbox),20);
  gtk_window_set_child (GTK_WINDOW (win_bo), hbox);

  // now divide left side into segments for buttons
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
  gtk_box_append(GTK_BOX(hbox),vbox);
    
  // and further divide the right side into quads - two "up" and two "dn"
  hbox_up=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_up);
  hbox_dn=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_dn);
  
  // set up EXIT button
  btn_exit = gtk_button_new ();
  g_signal_connect (btn_exit, "clicked", G_CALLBACK (set_layer_height_done_callback), win_bo);
  img_exit = gtk_image_new_from_file("Back.gif");
  gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
  gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
  gtk_box_append(GTK_BOX(vbox),btn_exit);

  // build a notebook for different effects
  poly_notebook = gtk_notebook_new ();
  gtk_box_append(GTK_BOX(hbox_up),poly_notebook);
  g_signal_connect (g_notebook, "switch_page",G_CALLBACK (on_poly_nb_page_switch), NULL);
  gtk_widget_set_size_request(poly_notebook,300,200);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(poly_notebook),TRUE);

  // set up perimeter page of notebook
  {
    perim_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    lbl_perim_frame=gtk_label_new("Perimeter");
    gtk_notebook_append_page (GTK_NOTEBOOK(poly_notebook),perim_box,lbl_perim_frame);

    grd_perim = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(perim_box),grd_perim);
    gtk_grid_set_row_spacing (GTK_GRID(grd_perim),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_perim),15);
  
    perim_flw_adj=gtk_adjustment_new(temp_perim_flw, 0, 10, 0.1, 1, 2);
    perim_flw_btn=gtk_spin_button_new(perim_flw_adj, 6, 3);
    GtkWidget *lbl_perim_flw=gtk_label_new(" Perimeter flow:");
    gtk_grid_attach (GTK_GRID (grd_perim), lbl_perim_flw, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_perim), perim_flw_btn, 1, 0, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(perim_flw_btn),"value-changed",G_CALLBACK(get_perim_feed_value),NULL);
    
    perim_fed_adj=gtk_adjustment_new(temp_perim_fed, 0, 20000, 10, 50, 100);
    perim_fed_btn=gtk_spin_button_new(perim_fed_adj, 6, 0);
    GtkWidget *lbl_perim_fed=gtk_label_new(" Perimeter feed:");
    gtk_grid_attach (GTK_GRID (grd_perim), lbl_perim_fed, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_perim), perim_fed_btn, 1, 1, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(perim_fed_btn),"value-changed",G_CALLBACK(get_perim_flow_value),NULL);
    
    perim_thk_adj=gtk_adjustment_new(temp_perim_thk, 0.0, 10.0, 0.1, 1, 2);
    perim_thk_btn=gtk_spin_button_new(perim_thk_adj, 6, 3);
    GtkWidget *lbl_perim_thk=gtk_label_new(" Perimeter width:");
    gtk_grid_attach (GTK_GRID (grd_perim), lbl_perim_thk, 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_perim), perim_thk_btn, 1, 2, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(perim_thk_btn),"value-changed",G_CALLBACK(get_perim_thick_value),NULL);
    
    perim_pitch_adj=gtk_adjustment_new(temp_perim_pitch, 0.0, 100.0, 0.1, 1, 2);
    perim_pitch_btn=gtk_spin_button_new(perim_pitch_adj, 6, 3);
    GtkWidget *lbl_perim_pitch=gtk_label_new(" Perimeter pitch:");
    gtk_grid_attach (GTK_GRID (grd_perim), lbl_perim_pitch, 0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_perim), perim_pitch_btn, 1, 3, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(perim_pitch_btn),"value-changed",G_CALLBACK(get_perim_pitch_value),NULL);

    perim_no_pattern_btn=gtk_button_new_with_label ("Reset to default ");
    g_signal_connect (GTK_BUTTON (perim_no_pattern_btn), "clicked", G_CALLBACK (get_perim_type_value),GINT_TO_POINTER(4));
    gtk_grid_attach(GTK_GRID (grd_perim),perim_no_pattern_btn, 0, 4, 1, 1);
  
    perim_same_pattern_btn=gtk_button_new_with_label ("Apply to all ");
    g_signal_connect (GTK_BUTTON (perim_same_pattern_btn), "clicked", G_CALLBACK (get_perim_type_value),GINT_TO_POINTER(5));
    gtk_grid_attach(GTK_GRID (grd_perim),perim_same_pattern_btn, 0, 5, 1, 1);
    
    perim_unique_pattern_btn=gtk_button_new_with_label ("All unique pattern ");
    g_signal_connect (GTK_BUTTON (perim_unique_pattern_btn), "clicked", G_CALLBACK (get_perim_type_value),GINT_TO_POINTER(6));
    gtk_grid_attach(GTK_GRID (grd_perim),perim_unique_pattern_btn, 0, 6, 1, 1);
    
  }

  // set up fill notebook page
  {
    fill_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    lbl_fill_frame=gtk_label_new("Fill");
    gtk_notebook_append_page (GTK_NOTEBOOK(poly_notebook),fill_box,lbl_fill_frame);

    grd_fill = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(fill_box),grd_fill);
    gtk_grid_set_row_spacing (GTK_GRID(grd_fill),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_fill),15);
  
    fill_flw_adj=gtk_adjustment_new(temp_fill_flw, 0, 10, 0.1, 1, 2);
    fill_flw_btn=gtk_spin_button_new(fill_flw_adj, 6, 3);
    GtkWidget *lbl_fill_flw=gtk_label_new(" Fill line flow:");
    gtk_grid_attach (GTK_GRID (grd_fill), lbl_fill_flw, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_fill), fill_flw_btn, 1, 0, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_flw_btn),"value-changed",G_CALLBACK(get_fill_feed_value),NULL);
    
    fill_fed_adj=gtk_adjustment_new(temp_fill_fed, 0, 20000, 10, 50, 100);
    fill_fed_btn=gtk_spin_button_new(fill_fed_adj, 6, 0);
    GtkWidget *lbl_fill_fed=gtk_label_new(" Fill line feed:");
    gtk_grid_attach (GTK_GRID (grd_fill), lbl_fill_fed, 0, 1, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_fill), fill_fed_btn, 1, 1, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_fed_btn),"value-changed",G_CALLBACK(get_fill_flow_value),NULL);
    
    fill_thk_adj=gtk_adjustment_new(temp_fill_thk, 0.0, 10.0, 0.1, 1, 2);
    fill_thk_btn=gtk_spin_button_new(fill_thk_adj, 6, 3);
    GtkWidget *lbl_fill_thk=gtk_label_new("Fill line width:");
    gtk_grid_attach (GTK_GRID (grd_fill), lbl_fill_thk, 0, 2, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_fill), fill_thk_btn, 1, 2, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_thk_btn),"value-changed",G_CALLBACK(get_fill_thick_value),NULL);
    
    fill_pitch_adj=gtk_adjustment_new(temp_fill_pitch, 0.0, 100.0, 0.1, 1, 2);
    fill_pitch_btn=gtk_spin_button_new(fill_pitch_adj, 6, 3);
    GtkWidget *lbl_fill_pitch=gtk_label_new("Fill line pitch:");
    gtk_grid_attach (GTK_GRID (grd_fill), lbl_fill_pitch, 0, 3, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_fill), fill_pitch_btn, 1, 3, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_pitch_btn),"value-changed",G_CALLBACK(get_fill_pitch_value),NULL);
    
    fill_angle_adj=gtk_adjustment_new(temp_fill_angle, 0.0, 180.0, 5.0, 1, 10);
    fill_angle_btn=gtk_spin_button_new(fill_angle_adj, 6, 3);
    GtkWidget *lbl_fill_angle=gtk_label_new("Fill line angle:");
    gtk_grid_attach (GTK_GRID (grd_fill), lbl_fill_angle, 0, 4, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_fill), fill_angle_btn, 1, 4, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_angle_btn),"value-changed",G_CALLBACK(get_fill_angle_value),NULL);
      
    fill_no_pattern_btn=gtk_button_new_with_label ("Reset to default ");
    g_signal_connect (GTK_BUTTON (fill_no_pattern_btn), "clicked", G_CALLBACK (get_fill_type_value),GINT_TO_POINTER(4));
    gtk_grid_attach(GTK_GRID (grd_fill),fill_no_pattern_btn, 0, 5, 1, 1);
  
    fill_same_pattern_btn=gtk_button_new_with_label ("Apply to all ");
    g_signal_connect (GTK_BUTTON (fill_same_pattern_btn), "clicked", G_CALLBACK (get_fill_type_value),GINT_TO_POINTER(5));
    gtk_grid_attach(GTK_GRID (grd_fill),fill_same_pattern_btn, 0, 6, 1, 1);
    
    fill_unique_pattern_btn=gtk_button_new_with_label ("All unique pattern ");
    g_signal_connect (GTK_BUTTON (fill_unique_pattern_btn), "clicked", G_CALLBACK (get_fill_type_value),GINT_TO_POINTER(6));
    gtk_grid_attach(GTK_GRID (grd_fill),fill_unique_pattern_btn, 0, 7, 1, 1);
  
  }

  gtk_widget_set_visible(win_bo,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}


void norms_callback (GtkWidget *btn, gpointer user_data)
{
  int		wind=0;
  model		*mptr;
  facet		*fbase,*ftest;
  vertex	*vtxA,*vtxB;
  vector	*vecA;

  int		slot;
  polygon_list	*pl_ptr;
  polygon	*pptr;
  slice 	*scrt,*sblw,*snew;
  genericlist	*aptr;

  if(job.state>=JOB_RUNNING)return;
  if(job.model_first==NULL)return;
  if(active_model==NULL)active_model=job.model_first;
  mptr=active_model;
  if(mptr->input_type!=STL)return;

  // determine which slice/tool we are dealing with
  mptr=active_model;
  aptr=mptr->oper_list;							// look thru all operations called out by this model
  while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation
    {
    slot=aptr->ID;
    if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
    if(Tool[slot].state<TL_READY){aptr=aptr->next;continue;}
    scrt=slice_find(mptr,MODEL,slot,z_cut,job.min_slice_thk);
    if(scrt!=NULL)break;
    aptr=aptr->next;
    }
  
  // debug code to check polygon self intersection function
  pl_ptr=p_pick_list;
  while(pl_ptr!=NULL)
    {
    pptr=pl_ptr->p_item;
    polygon_selfintersect_check(pptr);
    pl_ptr=pl_ptr->next;
    }
  
  
/*
  // fix facet normals using winding numbers
  // draw vector from facet center to outside the model's bounding box and count crossings.
  // adjust/flip normal based on number of crossings (odd=points to inside and needs flip).
  vtxB=vertex_make();
  fbase=mptr->facet_first[MODEL];
  while(fbase!=NULL)
    {
    // establish fbase's unit normal
    facet_normal(fbase);
    
    // verify that it is pointing correct direction
    wind=0;
    vtxA=fbase->edg[0]->tip;						// any point on fbase
    vtxB->x=vtxA->x + 1000*fbase->unit_norm->x;
    vtxB->y=vtxA->y + 1000*fbase->unit_norm->y;
    vtxB->z=vtxA->z + 1000*fbase->unit_norm->z;
    vecA=vector_make(vtxA,vtxB,0);
    ftest=mptr->facet_first[MODEL];
    while(ftest!=NULL)							// check if vecA intersects any other facets
      {
      if(ftest!=fbase && vector_facet_intersect(ftest,vecA,NULL)==TRUE)wind++;
      ftest=ftest->next;
      }
    free(vecA);vector_mem--;
    
    // if the wind number is odd, then must reverse normal
    if(wind%2!=0)
      {
      fbase->unit_norm->x*=(-1);
      fbase->unit_norm->y*=(-1);
      fbase->unit_norm->z*=(-1);
      }
    
    fbase=fbase->next;
    }

  free(vtxB);vertex_mem--;
  //mptr->edge_spt_upper=edge_angle_id(mptr->edge_first,max_spt_angle);
*/

  return;
}


void set_history_type_callback (GtkWidget *btn, gpointer user_data)
{

  hist_type=GPOINTER_TO_INT(user_data);
  printf("History type set to %d \n",hist_type);
  return;
}


void set_image_mode_callback (GtkWidget *btn, gpointer user_data)
{
  float		fval,z_slider_max,z_slider_inc;
  char		cam_scratch[128];
  
  // turn off an residuals from previous mode
  if(cam_image_mode==CAM_LIVEVIEW)
    {
    // update button to show live view is off
    gtk_image_clear(GTK_IMAGE(img_camera_liveview));			// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_liveview = gtk_image_new_from_file("Live_View_Off.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_liveview),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_liveview),img_camera_liveview);
    }
  if(cam_image_mode==CAM_JOB_LAPSE)
    {
    // update button to show job time lapse is off
    gtk_image_clear(GTK_IMAGE(img_camera_job_timelapse));		// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_job_timelapse = gtk_image_new_from_file("Time_Lapse_Off.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_job_timelapse),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_job_timelapse),img_camera_job_timelapse);
    }
  if(cam_image_mode==CAM_SCAN_LAPSE)
    {
    // update button to show scan time lapse is off
    gtk_image_clear(GTK_IMAGE(img_camera_scan_timelapse));		// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_scan_timelapse = gtk_image_new_from_file("Scan_Lapse_Off.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_scan_timelapse),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_scan_timelapse),img_camera_scan_timelapse);
    }

  // set the new mode
  cam_image_mode=GPOINTER_TO_INT(user_data);
  printf("Camera image mode set to %d \n",cam_image_mode);
  z_cut=0.00;

  // set up live view params
  if(cam_image_mode==CAM_LIVEVIEW)
    {
    // update button to show live view is on
    gtk_image_clear(GTK_IMAGE(img_camera_liveview));			// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_liveview = gtk_image_new_from_file("Live_View_On.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_liveview),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_liveview),img_camera_liveview);
    }

  // set up job time lapse params
  if(cam_image_mode==CAM_JOB_LAPSE)
    {
    // update button to show job time lapse is on
    gtk_image_clear(GTK_IMAGE(img_camera_job_timelapse));		// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_job_timelapse = gtk_image_new_from_file("Time_Lapse_On.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_job_timelapse),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_job_timelapse),img_camera_job_timelapse);

    // determine how many images there are in the TableScan subdirectory
    build_table_scan_index(0);
    if(scan_image_count>0)
      {
      z_cut=scan_img_index[1];
      z_slider_inc=fabs(scan_img_index[2]-scan_img_index[1]);
      z_slider_max=scan_img_index[scan_image_count];
    
      printf("\n\nJob Scan Lapse Params:\n");
      printf("  image count = %d \n",scan_image_count);
      printf("  z_cut=%f  z_slider_inc=%f   z_slider_max=%f \n",z_cut,z_slider_inc,z_slider_max);

      // set right side slider bar to max range found by scan_index
      gtk_adjustment_configure(g_adj_z_level,z_cut,0,z_slider_max,z_slider_inc,z_slider_inc,z_slider_inc*2);
      }
    
    // set right side slider bar to range from 0 to 415mm in 5mm increments so z_cut will select image
    gtk_adjustment_configure(g_adj_z_level,z_cut,0,z_slider_max,z_slider_inc,10.00,25.00);
    }

  // set up table scan time lapse params
  if(cam_image_mode==CAM_SCAN_LAPSE)
    {
    // update button to show job time lapse is on
    gtk_image_clear(GTK_IMAGE(img_camera_scan_timelapse));		// MUST be cleared then re-loaded or it becomes severe memory leak
    img_camera_scan_timelapse = gtk_image_new_from_file("Scan_Lapse_On.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_scan_timelapse),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_scan_timelapse),img_camera_scan_timelapse);

    // if there is something to display
    build_table_scan_index(1);
    if(scan_image_count>0)
      {
      z_cut=scan_img_index[1];
      if(z_cut<0)z_cut=1;
      z_slider_inc=fabs(scan_img_index[2]-scan_img_index[1]);
      if(z_slider_inc<1)z_slider_inc=1;
      if(z_slider_inc>10)z_slider_inc=10;
      z_slider_max=scan_img_index[scan_image_count];
      if(z_slider_max>600)z_slider_max=600;
    
      printf("\n\nTable Scan Lapse Params:\n");
      printf("  image count = %d \n",scan_image_count);
      printf("  z_cut=%f  z_slider_inc=%f   z_slider_max=%f \n",z_cut,z_slider_inc,z_slider_max);

      // set right side slider bar to max range found by scan_index
      gtk_adjustment_configure(g_adj_z_level,z_cut,0,z_slider_max,1,1,5);
      }
    }

  // take a snap shot and save as requested
  if(cam_image_mode==CAM_SNAPSHOT)
    {
    // just copy the current still over to a new user specified file name
    sprintf(bashfn,"sudo cp /home/aa/Documents/4X3D/still-test.jpg /home/aa/Documents/4X3D/tbscan0.jpg");
    system(bashfn);

    sprintf(scratch," \n  Snapshot taken!  \n");
    aa_dialog_box(win_main,0,3,"Camera",scratch);
    
    // update button to show live view is on
    cam_image_mode=CAM_LIVEVIEW;					// switch back to live view
    gtk_image_clear(GTK_IMAGE(img_camera_liveview));
    img_camera_liveview = gtk_image_new_from_file("Live_View_On.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_liveview),50);
    gtk_button_set_child(GTK_BUTTON(btn_camera_liveview),img_camera_liveview);
    
    }

  // run build table scan
  if(cam_image_mode==CAM_IMG_SCAN)
    {
    printf("\nRunning image processor...");

    cam_roi_x0=0.0; cam_roi_y0=0.0;
    cam_roi_x1=1.0; cam_roi_y1=1.0;
    strcpy(bashfn,"#!/bin/bash\nsudo libcamera-jpeg -v 0 -n 1 -t 10 ");		// define base camera command
    strcat(bashfn,"-o /home/aa/Documents/4X3D/still-test.jpg ");		// ... add the target file name
    strcat(bashfn,"--width 830 --height 574 ");					// ... add the image size
    sprintf(cam_scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
    strcat(bashfn,cam_scratch);							// ... add region of interest (zoom)
    strcat(bashfn,"--autofocus-mode continuous ");				// ... add focus control
    system(bashfn);								// issue the command to take a picture

    
    scan_in_progress=TRUE;
    //  build_table_line_laser_scan();
    //  build_table_scan_index(1);
    //  laser_scan_images_to_STL();
    scan_build_volume();
    
    scan_in_progress=FALSE;
    cam_image_mode=CAM_LIVEVIEW;
    //Superimpose_flag=TRUE;
    //main_view_page=VIEW_MODEL;
    
    printf(" done!\n");
    }

  //need_new_image=TRUE;

  return;
}


void on_redraw (GtkWidget *widget, GtkWidget *da)
{

    gtk_widget_queue_draw(da);

    return;
}


void on_test_btn (GtkWidget *widget, GtkWidget *da)
{

    model_maxmin(job.model_first);
    gtk_widget_queue_draw(da);

    return;
}


static void on_nb_page_switch (GtkWidget *g_notebook, GtkWidget *page, gint page_num)
{
  // copy page number to global for settings page call.  see also #defines for this.
  // 0=VIEW_MODEL  1=VIEW_TOOL  2=VIEW_LAYER  3=VIEW_GRAPH  4=VIEW_CAMERA  5=VIEW_VULKAN
  main_view_page=page_num;
  
  return;
}


// Idle loop funtion - polling function to evaluate system status at regular interval
static gboolean on_idle_status_poll(GtkWidget *hbox)
{
    int		h,i,j,k,slot,mdl_count[MAX_TOOLS],remove_count=0;
    int		col_width=15,modl_chk,oper_chk,tool_chk,new_state,old_state;
    int		a_hrs,a_mins,e_hrs,e_mins;
    int		show_ready_status=FALSE;
    static int 	show_tool_temp, show_tool_matl, show_tool_feed;
    char	idl_scratch[255];
    char	mdl_name[255],mdl_build[255];
    char 	oper_st_txt[40],lint_st_txt[40];
    float 	q,amt_complete,temp_chk;
    model	*mptr;
    genericlist	*amptr,*gptr,*gdel;
  
    // init status msg each loop thru to force update
    memset(job_status_msg,0,sizeof(job_status_msg));
    memset(idl_scratch,0,sizeof(idl_scratch));

    // update idle time - gets reset in the tool_UI, model_UI, and tinyGSnd functions.
    // may need other sprinkles elsewhere.  it determines if the system is active or idle.
    {
      idle_current_time=time(NULL);					// ... get current time
      //if((idle_current_time - idle_start_time)>(MAX_IDLE_TIME-3000) && job.state<JOB_RUNNING)
        //{
	//if((idle_current_time - idle_start_time)>0)
	 // {
	 // sprintf(idl_scratch," %5.0f mins to sleep. ",((float)(MAX_IDLE_TIME-(idle_current_time-idle_start_time))/60));
	 // sprintf(job_status_msg,"%s",idl_scratch);
	 // }
	//}
    }

    // update load status of each tool which includes status widgets (like temperature and material status bars, etc.).
    // some units may have flaky loop back connections so system status is checked before reacting.  
    // it takes a sustained dis-engagement of the tool to register as a tool being unloaded.
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
    
      // check if tool is plugged in via loop back connector state change. if so new state is TL_UNKNOWN (i.e. 1)
      new_state=TL_EMPTY;						// default to empty
      if(slot==0)new_state=RPi_GPIO_Read(TOOL_A_LOOP);	
      if(slot==1)new_state=RPi_GPIO_Read(TOOL_B_LOOP);
      if(slot==2)new_state=RPi_GPIO_Read(TOOL_C_LOOP);
      if(slot==3)new_state=RPi_GPIO_Read(TOOL_D_LOOP);
      
      // if a tool just got UNLOADED...
      if(Tool[slot].state>TL_EMPTY && new_state==TL_EMPTY)
	{
	// if job is running, a removal must be sustained for 3 loops thru idle routine.
	// there have been occassions where a loose connection sends the signal a tool had
	// been removed when it really was not.
	if(job.state>=JOB_RUNNING && remove_count<3)			
	  {
	  remove_count++;						// increment counts thru idle loop
	  }
	else 
	  {
	  if(remove_count>0)printf("\n\n WARNING: Loose 5v or LOOP BACK connection detected on tool %d! \n\n",(slot+1));
	  remove_count=0;
	  job.state=JOB_NOT_READY;					// this will effectively abort the job
	  // deal with case when toolUI window is up
	  if(win_tool_flag[slot]==TRUE)
	    {
	    gtk_window_close(GTK_WINDOW(win_tool));
	    win_tool_flag[slot]=FALSE;
	    }
	  Open1Wire(slot);						// list 1wire directories to consol
	  make_toolchange(-1,0);					// turn off all tools
	  unload_tool(slot);						// force system params to harmless values
	  Tool[slot].state=TL_EMPTY;					// force state to empty for next pass thru idle loop

	  // turn on/off tool temp display in graph depending on sensor
	  show_tool_temp=FALSE;							// set default
	  gtk_widget_set_visible(temp_lbl[slot],show_tool_temp);		// show temperature values
	  gtk_widget_set_visible(GTK_WIDGET(temp_bar[slot]),show_tool_temp);	// show the temperature level graph
	  gtk_widget_set_visible(setp_lbl[slot],show_tool_temp);		// show temperature values
	  hist_use_flag[H_TOOL_TEMP]=show_tool_temp;				// turn off graph display
	    
	  // turn on/off tool material level depending on tool type
	  show_tool_matl=FALSE;							// set default
	  gtk_widget_set_visible(mlvl_lbl[slot],show_tool_matl);		// show material values
	  gtk_widget_set_visible(GTK_WIDGET(matl_bar[slot]),show_tool_matl);	// show the material level graph
	  gtk_widget_set_visible(matl_lbl[slot],show_tool_matl);		// show material values
	  
	  // turn on/off tool feed buttons depending on tool type
	  show_tool_feed=FALSE;
	  gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	  gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	    
	  // loop thru model list and remove any operations already associated to the removed tool
	  mptr=job.model_first;						
	  while(mptr!=NULL)
	    {
	    gptr=mptr->oper_list;
	    while(gptr!=NULL)
	      {
	      gdel=gptr;
	      gptr=gptr->next;
	      if(gdel->ID==slot)
		{
		mptr->oper_list=genericlist_delete(mptr->oper_list,gdel);	// remove this tool's operations for model's list of ops
		if(mptr->slice_first!=NULL)slice_purge(mptr,slot);		// if it has also already been sliced...
		}
	      }
	    mptr=mptr->next;
	    }

	  // deal with case when tool unloaded while modelUI window is up
	  if(win_modelUI_flag==TRUE)
	    {
	    gtk_window_close(GTK_WINDOW(win_model));
	    win_modelUI_flag=FALSE;
	    }

	  }
	}
 
      // if a tool just got LOADED...
      // this section of code will determine what the tool is and configure what/how widgets are show to match the tool.
      if(Tool[slot].state==TL_EMPTY && new_state>TL_EMPTY && init_done_flag==TRUE)
	{
	sprintf(idl_scratch," Checking tool %d configuration... ",(slot+1));
	printf("\n  %s\n",idl_scratch);
	if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
	else {sprintf(job_status_msg,"%s",idl_scratch);}
	if(job.state>=JOB_RUNNING)job.state=JOB_NOT_READY;		// this will effectively abort the job
	load_tool_defaults(slot);					// load unknown tool params
	current_tool=(-1);						// force tool select to activate
	  
	// attempt to read tool 1-wire memory...
	if(strlen(Tool[slot].mmry.dev)<7)				// does the tool does NOT have a mem device number...
	  {
	  make_toolchange(slot,1);					// turn on tool to power to ground memory chip (if available)
	  delay(500);							// delay needed to allow 1-wire chips to power up/down
	  j=0;
	  while(strlen(Tool[slot].mmry.dev)<7 && j<5)			// attempt to read chip 5X
	    {
	    delay(500);							// delay to allow power up
	    Open1Wire(slot);						// check for memory chip which defines mmry.dev
	    j++;
	    }
	  }

	// block comment this IF statement when programming new memories
	
	// if a 1-wire device was found...
	if(strlen(Tool[slot].mmry.dev)>8)				// check if 1wire has been identified for this tool
	  {
	  for(i=0;i<3;i++)						// 1wires are notorously sketchy... attempt to read 3 times
	    {
	    printf("\n\n1WIRE READ: i=%d \n\n",i);
	    if(MemDevRead(slot)==TRUE)					// attempt to read tool data from 1wire memory
	      {
	      // check for name (from 1wire) and type (from TOOLS.XML)... if both there, then define as loaded
	      if(strlen(Tool[slot].name)>2 && Tool[slot].type>0)Tool[slot].state=TL_LOADED;
	      
	      // check if this type of tool requires a material call out... if so, check if present
	      if(Tool[slot].type==MEASUREMENT)
	        {
		Tool[slot].state=TL_READY;
		clock_gettime(CLOCK_REALTIME, &Tool[slot].active_time_stamp);	// get time at which tool becomes active
		}
	      else 
	        {
		if(Tool[slot].type==MARKING)
		  {
		  //pers_scale=0.0;					// turn off perspective when marking
		  // special case for 40w laser using 24v power to drive it
		  // a bit sketchy because calling the pca9685 functions from the idle loop could conflict
		  // with calls from the print thread.  BUT, the two calls below ought to only happen on tool
		  // changes when the print thread is idle... and should never be called during a print job.
		  if(strstr(Tool[slot].name,"LASER40W")!=NULL)
		    {
		    //pca9685PWMWrite(I2C_PWM_fd,0,0,MAX_PWM); 		// port 0 set to full 24v for laser
		    }
		  // special case for 80w laser using 24v power to drive it
		  else if(strstr(Tool[slot].name,"LASER80W")!=NULL)
		    {
		    //pca9685PWMWrite(I2C_PWM_fd,0,0,MAX_PWM); 		// port 0 set to full 24v for laser
		    }
		  }

		// check of name (from 1wire) and material ID (from MATERIALS.XML)... if both there, then define as ready
		//if(strlen(Tool[slot].matl.name)>2 && Tool[slot].matl.ID>0)
		if(strlen(Tool[slot].matl.name)>2)
		  {
		  // update tool and material states
		  Tool[slot].matl.state=1;
		  Tool[slot].state=TL_READY;
		  clock_gettime(CLOCK_REALTIME, &Tool[slot].active_time_stamp);	// get time at which tool becomes active
		  Tool[slot].thrm.sync = TRUE;					// force thermal thread to update to new values
  
		  // set build table temperature to the highest temp based on which materials are loaded
		  if(Tool[slot].thrm.bedtC > Tool[BLD_TBL1].thrm.setpC)
		    {
		    Tool[BLD_TBL1].thrm.setpC = Tool[slot].thrm.bedtC;
		    Tool[BLD_TBL1].thrm.bedtC = Tool[slot].thrm.bedtC;
		    Tool[BLD_TBL2].thrm.setpC = Tool[slot].thrm.bedtC;
		    Tool[BLD_TBL2].thrm.bedtC = Tool[slot].thrm.bedtC;
		    Tool[BLD_TBL1].thrm.sync = TRUE;			// force thermal thread to update to new values
		    Tool[BLD_TBL2].thrm.sync = TRUE;
		    }
		  }
		}
	      break;
	      }
	    }
	  }
	
	// end of block comment ----------------------------------------
	make_toolchange(-1,0);							// turn off tool, which disconnects tool's 1wire chip

	// turn on/off tool temp display in graph depending on sensor
	show_tool_temp=FALSE;							// set default
	if(Tool[slot].thrm.sensor>NONE)show_tool_temp=TRUE;			// if this tool has a temp sensor...
	gtk_widget_set_visible(temp_lbl[slot],show_tool_temp);			// show temperature values
	gtk_widget_set_visible(GTK_WIDGET(temp_bar[slot]),show_tool_temp);	// show the temperature level graph
	gtk_widget_set_visible(setp_lbl[slot],show_tool_temp);			// show temperature values
	hist_use_flag[H_TOOL_TEMP]=show_tool_temp;				// turn off graph display
	  
	// turn on/off tool material level depending on tool type
	show_tool_matl=FALSE;							// set default
	if(Tool[slot].type==ADDITIVE)show_tool_matl=TRUE;			// only additive tools have material
	gtk_widget_set_visible(mlvl_lbl[slot],show_tool_matl);			// show material values
	gtk_widget_set_visible(GTK_WIDGET(matl_bar[slot]),show_tool_matl);	// show the material level graph
	gtk_widget_set_visible(matl_lbl[slot],show_tool_matl);			// show material values
	hist_use_flag[H_MOD_MAT_EST]=show_tool_matl;				// turn off graph display
	hist_use_flag[H_MOD_MAT_ACT]=show_tool_matl;				// turn off graph display
	hist_use_flag[H_SUP_MAT_EST]=show_tool_matl;				// turn off graph display
	hist_use_flag[H_SUP_MAT_ACT]=show_tool_matl;				// turn off graph display
	
	// turn on/off tool feed buttons and set their image depending on tool type
	show_tool_feed=FALSE;
	if(Tool[slot].tool_ID==TC_EXTRUDER || Tool[slot].tool_ID==TC_FDM || Tool[slot].tool_ID==TC_ROUTER)
	  {
	  gtk_image_clear(GTK_IMAGE(img_tool_feed_fwd));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  if(Tool[slot].tool_ID==TC_EXTRUDER)img_tool_feed_fwd = gtk_image_new_from_file("Matl-Extrude-Add-G.gif");
	  if(Tool[slot].tool_ID==TC_FDM)img_tool_feed_fwd = gtk_image_new_from_file("Matl-Filament-Add-G.gif");
	  if(Tool[slot].tool_ID==TC_ROUTER)img_tool_feed_fwd = gtk_image_new_from_file("Tip-Bit-Add-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_feed_fwd),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_feed_fwd),img_tool_feed_fwd);
	  gtk_image_clear(GTK_IMAGE(img_tool_feed_rvs));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  if(Tool[slot].tool_ID==TC_EXTRUDER)img_tool_feed_rvs = gtk_image_new_from_file("Matl-Extrude-Remove-G.gif");
	  if(Tool[slot].tool_ID==TC_FDM)img_tool_feed_rvs = gtk_image_new_from_file("Matl-Filament-Remove-G.gif");
	  if(Tool[slot].tool_ID==TC_ROUTER)img_tool_feed_rvs = gtk_image_new_from_file("Tip-Bit-Remove-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_feed_rvs),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_feed_rvs),img_tool_feed_rvs);
	  show_tool_feed=TRUE;							// set to display
	  }
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	
	}
	
      // if a tool slot is still EMPTY...
      if(Tool[slot].state==TL_EMPTY)
	{
	show_tool_temp=FALSE;
	show_tool_matl=FALSE;
	gtk_label_set_text(GTK_LABEL(type_desc[slot]),"Empty     ");
	gtk_label_set_text(GTK_LABEL(matl_desc[slot]),"Empty     ");
	}

      // if a tool remains LOADED slot (whether defined or not)...
      // this section of code will update widgets that are specific to this type of tool.
      memset(idl_scratch,0,sizeof(idl_scratch));
      if(Tool[slot].state>TL_UNKNOWN)
	{
	remove_count=0;
	if(main_view_page==VIEW_TOOL)
	  {
	  sprintf(idl_scratch,"%s",Tool[slot].name);
	  while(strlen(idl_scratch)<10){strcat(idl_scratch," ");}
	  if(strlen(idl_scratch)>29)idl_scratch[30]=0;
	  gtk_label_set_text(GTK_LABEL(type_desc[slot]),idl_scratch);
	  gtk_widget_queue_draw(type_desc[slot]);
	  if(show_tool_matl==TRUE && strlen(Tool[slot].matl.name)>0)
	    {
	    sprintf(idl_scratch,"%s",Tool[slot].matl.name);
	    while(strlen(idl_scratch)<10){strcat(idl_scratch," ");}
	    if(strlen(idl_scratch)>29)idl_scratch[30]=0;
	    gtk_label_set_text(GTK_LABEL(matl_desc[slot]),idl_scratch);
	    gtk_widget_queue_draw(matl_desc[slot]);
	    }
  
	  // debug for distance sensor
	  /*
	  if(Tool[slot].tool_ID==TC_LASER && win_testUI_flag==TRUE)
	    {
	    int 		number_of_sensor_reads=1;
	    float 	fval,total_fval,distZ,
	    
	    j=0;
	    total_fval=0;
	    while(j<number_of_sensor_reads)
	      {
	      fval=distance_sensor(slot,3,1,0);
	      if(fval>0.0 && fval<5.0){total_fval+=fval; j++;}
	      }
	    fval=total_fval/number_of_sensor_reads;	
	    distZ = (-78.2085)*pow(fval,3) + 395.2662*pow(fval,2) - 672.1709*fval + 481.8335;
	    //distZ = 119.5294 * pow(fval,(-.9264783));			// power
	    //distZ = 40.13 + 367.35 * exp(-1.49107*fval);		// basic exponential
	    //distZ= fval*(-74.24) + 209.91;				// linear
	    printf("\n\nDist=%f   Volts=%f\n",distZ,fval);
	    }
	  */
	  
	  // loop thru all models to find the first one referenced by this tool and post its name
	  sprintf(mdl_name,"Empty");					// default model name display if none loaded yet
	  mdl_count[slot]=0;
	  mptr=job.model_first;
	  while(mptr!=NULL)
	    {
	    memset(mdl_name,0,sizeof(mdl_name));
	    amptr=mptr->oper_list;
	    while(amptr!=NULL)
	      {
	      if(amptr->ID==slot)
		{
		mdl_count[slot]++;					// increment number of models using this tool
		if(strlen(mdl_name)<1)					// if model name not yet defined for viewing...
		  {
		  if(strlen(mptr->model_file)>27)				// ... and it is a long name...
		    {
		    sprintf(mdl_name,"...");				// ... add the dots
		    strcat(mdl_name,&mptr->model_file[strlen(mptr->model_file)-27]);	// ... add the last part of name
		    }
		  else 							// ... and if it is a short name...
		    {sprintf(mdl_name,"%s",mptr->model_file);}		// ... just copy it over
		    
		  while(strlen(mdl_name)<10){strcat(mdl_name," ");}	// ... pad it with spaces if too short
		  if(strlen(mdl_name)>29)mdl_name[30]=0;			// ... truncate if something went wrong
		  gtk_label_set_text(GTK_LABEL(mdl_desc[slot]),mdl_name);	// ... update the gtk label with text
		  gtk_widget_queue_draw(mdl_desc[slot]);			// ... post to gtk for view update
		  }
		}
	      amptr=amptr->next;
	      }
	    mptr=mptr->next;
	    }
	  }

	// update tool temperature setback
	// if any one tool uses thermal, and we're idle too long, ALL thermal tools will be set back
	if((Tool[slot].pwr24==TRUE || Tool[slot].pwr48==TRUE) && Tool[slot].thrm.sensor>NONE)	// if this tool has a thermal control...
	  {
	  // if sitting idle too long and not yet in setback...
	  if((idle_current_time - idle_start_time)>MAX_IDLE_TIME && Tool[slot].thrm.setpC!=Tool[slot].thrm.backC) 
	    {
	    job.prev_state=job.state;					// ... save where we were
	    job.state=JOB_SLEEPING;					// ... set jobstate to indicate we are in setback
	    good_to_go=FALSE;						// ... reset so no auto start can happen
	    for(i=0;i<MAX_THERMAL_DEVICES;i++)				// ... cycle thru all devices
	      {
	      if(Tool[i].state<TL_LOADED)continue;			// ... if not loaded, skip it
	      if(Tool[i].pwr24==FALSE)continue;				// ... if not thermal, skip it
	      Tool[i].thrm.setpC=Tool[i].thrm.backC;			// ... reset temp to setback temp for ALL tools
	      }
	    }
	  // if still in setback and no longer idle...
	  if((idle_current_time - idle_start_time)<MAX_IDLE_TIME && Tool[slot].thrm.setpC==Tool[slot].thrm.backC)	
	    {
	    if(job.state==JOB_SLEEPING)job.state=JOB_HEATING;		// ... if we were in setback, reset to "heating tools"
	    for(i=0;i<MAX_THERMAL_DEVICES;i++)				// ... cycle thru all devices
	      {
	      if(Tool[i].state<TL_LOADED)continue;			// ... if not loaded, skip it
	      if(Tool[i].pwr24==FALSE || Tool[i].thrm.sensor<=0)continue;	// ... if not thermal, skip it
	      Tool[i].thrm.setpC=Tool[i].thrm.operC;			// ... reset temp to operating temp for ALL tools
	      }
	    }
	  }
	
	// update material usage bar graph for main window
	if(main_view_page==VIEW_TOOL && show_tool_matl==TRUE)
	  {
	  if(Tool[slot].tool_ID==TC_FDM)
	    {
	    #ifdef GAMMA_UNIT
	    if(RPi_GPIO_Read(FILAMENT_OUT)==ON)
	      {
	      job.state==JOB_PAUSED_BY_CMD;				// if out of filament, force system to pause
	      Tool[slot].matl.mat_used=Tool[slot].matl.mat_volume;	// set volume used to volume of spool (i.e. 0% remaining)
	      }
	    #endif
	    }
	  sprintf(idl_scratch," -- %% ");
	  q=100-(100*Tool[slot].matl.mat_used/Tool[slot].matl.mat_volume);
	  if(q<0)q=0.1;
	  if(q>100)q=99.9;
	  gtk_level_bar_set_value (matl_bar[slot],q);
	  sprintf(idl_scratch,"%4.0f %%",q);
	  gtk_label_set_text(GTK_LABEL(matl_lbl[slot]),idl_scratch);
	  gtk_widget_queue_draw(matl_lbl[slot]);
	  
	  // update material values for tool window
	  if(win_tool_flag[slot]==TRUE)
	    {
	    col_width=15;
	    sprintf(idl_scratch," -- %% ");
	    if(Tool[slot].state>=TL_READY)
	      {
	      if(Tool[slot].type==ADDITIVE)q=(100-(100*Tool[slot].matl.mat_used/Tool[slot].matl.mat_volume));
	      if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==TRUE)q=(100*Tool[slot].powr.power_duty);
	      if(q<0)q=0.1;
	      if(q>100)q=99.9;
	      gtk_level_bar_set_value(Tmatl_bar[slot],q);
	      sprintf(idl_scratch,"%4.0f %%",q);
	      }
	    while(strlen(idl_scratch)<col_width)strcat(idl_scratch," ");
	    if(strlen(idl_scratch)>29)idl_scratch[30]=0;
	    gtk_label_set_text(GTK_LABEL(Tmatl_lbl[slot]),idl_scratch);
	    gtk_widget_queue_draw(Tmatl_lbl[slot]);

	    if(Tool[slot].tool_ID==TC_EXTRUDER)sprintf(idl_scratch,"%4.1f mL",Tool[slot].matl.mat_volume);
	    if(Tool[slot].tool_ID==TC_FDM)sprintf(idl_scratch,"%4.1f kg",Tool[slot].matl.mat_volume/330000);
	    if(Tool[slot].tool_ID>TC_FDM)sprintf(idl_scratch,"100%%");
	    while(strlen(idl_scratch)<col_width)strcat(idl_scratch," ");
	    if(strlen(idl_scratch)>29)idl_scratch[30]=0;
	    //gtk_label_set_text(GTK_LABEL(Tmlvl_lbl[slot]),idl_scratch);
	    //gtk_widget_queue_draw(Tmlvl_lbl[slot]);
	    }
	  }

	// display tool temperature if it has been set as such
	col_width=8;
	if(main_view_page==VIEW_TOOL && show_tool_temp==TRUE)
	  {
	  // update temperature graph values for main window status
	  q=Tool[slot].thrm.tempC;
	  if(q<0)q=1.0;
	  if(q>Tool[slot].thrm.maxtC)q=Tool[slot].thrm.maxtC;
	  sprintf(idl_scratch,"%6.0f C",q);
	  while(strlen(idl_scratch)<(col_width/2))strcat(idl_scratch," ");
	  if(q>(Tool[slot].thrm.setpC+Tool[slot].thrm.tolrC))
	    {
	    markup = g_markup_printf_escaped (error_fmt,idl_scratch);
	    }
	  else 
	    {
	    markup = g_markup_printf_escaped (norm_fmt,idl_scratch);
	    }
	  gtk_label_set_text(GTK_LABEL(temp_lbl[slot]),idl_scratch);
	  gtk_label_set_markup(GTK_LABEL(temp_lbl[slot]),markup);
	  gtk_widget_queue_draw(temp_lbl[slot]);
	  gtk_level_bar_set_min_value(temp_bar[slot],0.0);
	  gtk_level_bar_set_max_value(temp_bar[slot],(Tool[slot].thrm.maxtC+10.0));
	  gtk_level_bar_add_offset_value(temp_bar[slot],GTK_LEVEL_BAR_OFFSET_LOW,(Tool[slot].thrm.setpC*0.1));
	  gtk_level_bar_add_offset_value(temp_bar[slot],GTK_LEVEL_BAR_OFFSET_HIGH,(Tool[slot].thrm.setpC-Tool[slot].thrm.tolrC));
	  gtk_level_bar_add_offset_value(temp_bar[slot],GTK_LEVEL_BAR_OFFSET_FULL,(Tool[slot].thrm.setpC+Tool[slot].thrm.tolrC));
	  gtk_level_bar_set_value (temp_bar[slot],q);
	  sprintf(idl_scratch,"---");
	  if(Tool[slot].pwr24==TRUE)sprintf(idl_scratch," %6.0f C ",Tool[slot].thrm.setpC);
	  gtk_label_set_text(GTK_LABEL(setp_lbl[slot]),idl_scratch);
	  gtk_widget_queue_draw(setp_lbl[slot]);

	  // update temperature values for tool window status
	  if(win_tool_flag[slot]==TRUE)
	    {
	    col_width=32;
	    q=Tool[slot].thrm.tempC;
	    if(q<0)q=1.0;
	    if(q>Tool[slot].thrm.maxtC)q=Tool[slot].thrm.maxtC;
	    sprintf(idl_scratch,"%6.0f C",q);
	    while(strlen(idl_scratch)<(col_width/2))strcat(idl_scratch," ");
	    if(q>(Tool[slot].thrm.setpC+Tool[slot].thrm.tolrC))
	      {
	      markup = g_markup_printf_escaped (error_fmt,idl_scratch);
	      }
	    else 
	      {
	      markup = g_markup_printf_escaped (norm_fmt,idl_scratch);
	      }
	    gtk_label_set_text(GTK_LABEL(Ttemp_lbl[slot]),idl_scratch);
	    gtk_label_set_markup(GTK_LABEL(Ttemp_lbl[slot]),markup);
	    gtk_widget_queue_draw(Ttemp_lbl[slot]);
	    gtk_level_bar_set_max_value(Ttemp_bar[slot],(Tool[slot].thrm.maxtC+10.0));
	    gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_LOW,(Tool[slot].thrm.setpC*0.1));
	    gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_HIGH,(Tool[slot].thrm.setpC-Tool[slot].thrm.tolrC));
	    gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_FULL,(Tool[slot].thrm.setpC+Tool[slot].thrm.tolrC));
	    gtk_level_bar_set_value (Ttemp_bar[slot],Tool[slot].thrm.tempC);
	    sprintf(idl_scratch,"---");
	    if(Tool[slot].pwr24==TRUE)sprintf(idl_scratch,"%6.0f C",Tool[slot].thrm.setpC);
	    while(strlen(idl_scratch)<(col_width/2))strcat(idl_scratch," ");
	    gtk_adjustment_set_value(temp_override_adj,Tool[slot].thrm.setpC);
	    }
	  }
	else 
	  {
	  // not elegant... constantly resets table temp if table temp out of normal operating range
	  // also relies on thermal thread NOT actually trying to keep table at this temp.  i.e. table should be cooling.
	  //Tool[BLD_TBL1].thrm.setpC=Tool[BLD_TBL1].thrm.tempC-Tool[BLD_TBL1].thrm.tolrC+1; // ignores table temp if still hot from other work
	  //Tool[BLD_TBL2].thrm.setpC=Tool[BLD_TBL2].thrm.tempC-Tool[BLD_TBL2].thrm.tolrC+1; // ignores table temp if still hot from other work
	  }
	}	// end of tool status > empty
      }		// end of slot loop

    // update tool button display to reflect state
    tool_chk=0;
    if(main_view_page==VIEW_TOOL)
      {
      for(slot=0;slot<MAX_TOOLS;slot++)
        {
	if(Tool[slot].state==TL_EMPTY)					// If tool slot is empty...
	  {
	  gtk_image_clear(GTK_IMAGE(img_tool_ctrl));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  img_tool_ctrl = gtk_image_new_from_file("Tool_Blank.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_ctrl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_ctrl),img_tool_ctrl);
	  }
	if(Tool[slot].state==TL_UNKNOWN)				// If tool is plugged in but unknown...
	  {
	  gtk_image_clear(GTK_IMAGE(img_tool_ctrl));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  img_tool_ctrl = gtk_image_new_from_file("Tool_Unknown.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_ctrl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_ctrl),img_tool_ctrl);
	  }
	if(Tool[slot].state>=TL_LOADED && Tool[slot].state<TL_FAILED)	// If tool is loaded, ready, or active...
	  {
	  gtk_image_clear(GTK_IMAGE(img_tool_ctrl));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  sprintf(idl_scratch,"Tool_Info.gif");
	  if(Tool[slot].tool_ID==TC_FDM)sprintf(idl_scratch,"Tool_FDM.gif");
	  if(Tool[slot].tool_ID==TC_ROUTER)sprintf(idl_scratch,"Tool_Mill.gif");
	  if(Tool[slot].tool_ID==TC_LASER)sprintf(idl_scratch,"Tool_Laser.gif");
	  if(Tool[slot].tool_ID==TC_PROBE)sprintf(idl_scratch,"Tool_Probe.gif");
	  if(Tool[slot].tool_ID==TC_EXTRUDER)sprintf(idl_scratch,"Tool_Syringe.gif");
	  img_tool_ctrl = gtk_image_new_from_file(idl_scratch);
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_ctrl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_ctrl),img_tool_ctrl);
	  if(Tool[slot].state>=TL_READY)tool_chk++;			// If tool is ready to go... increment tool ready count
	  }
	if(Tool[slot].state==TL_FAILED)					// If tool is failed...
	  {
	  if(job.state!=JOB_PAUSED_DUE_TO_ERROR)job.state=JOB_PAUSED_DUE_TO_ERROR;
	  gtk_image_clear(GTK_IMAGE(img_tool_ctrl));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  img_tool_ctrl = gtk_image_new_from_file("Tool_Empty.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_ctrl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_ctrl),img_tool_ctrl);
	  }
	
	// update status labels
	sprintf(idl_scratch,"Empty");
	if(Tool[slot].state==TL_UNKNOWN)sprintf(idl_scratch," Unknown");
	if(Tool[slot].state==TL_LOADED)sprintf(idl_scratch," Loaded");
	if(Tool[slot].state==TL_READY)sprintf(idl_scratch," Ready");
	if(Tool[slot].state==TL_ACTIVE)sprintf(idl_scratch," Active");
	if(Tool[slot].state==TL_RETRACTED)sprintf(idl_scratch," Retracted");
	markup = g_markup_printf_escaped (norm_fmt,idl_scratch);
	if(Tool[slot].state==TL_FAILED)
	  {
	  sprintf(idl_scratch,"Failed");
	  if(Tool[slot].epwr48==TRUE)strcat(idl_scratch," - 48v power control");
	  if(Tool[slot].epwr24==TRUE)strcat(idl_scratch," - 24v power control");
	  if(Tool[slot].espibus==TRUE)strcat(idl_scratch," - SPI bus communication");
	  if(Tool[slot].ewire1==TRUE)strcat(idl_scratch," - 1 wire communication");
	  if(Tool[slot].elimitsw==TRUE)strcat(idl_scratch," - limit switch sensing");
	  if(Tool[slot].epwmtool==TRUE)strcat(idl_scratch," - pulse width modulation");
	  markup = g_markup_printf_escaped (error_fmt,idl_scratch);
	  }
	//if(Tool[slot].select_status==TRUE)strcat(idl_scratch," * ");
	if(first_layer_flag==TRUE)strcat(idl_scratch," * ");
	gtk_label_set_text(GTK_LABEL(tool_stat[slot]),idl_scratch);
	gtk_label_set_markup(GTK_LABEL(tool_stat[slot]),markup);
	gtk_widget_queue_draw(tool_stat[slot]);
	
	// update operation state to user
	sprintf(oper_st_txt,"Undefined");
	if(oper_state[slot]>=0 && oper_state[slot]<MAX_OP_TYPES)
	  {sprintf(oper_st_txt,"%s",op_description[oper_state[slot]]);}
	
	// update line type state to user
	sprintf(lint_st_txt,"Undefined");
	if(linet_state[slot]>=0 && linet_state[slot]<MAX_LINE_TYPES)
	  {sprintf(lint_st_txt,"%s",lt_description[linet_state[slot]]);}
	
	//sprintf(idl_scratch,"%6.3f",Tool[slot].matl.mat_pos);
	//sprintf(idl_scratch,"%6.3f",Tool[slot].tip_cr_pos);
	//sprintf(idl_scratch,"FWD");
	//gtk_label_set_text(GTK_LABEL(tip_pos_lbl[slot]),idl_scratch);
	//gtk_widget_queue_draw(tip_pos_lbl[slot]);
	//if(Tool[slot].tip_cr_pos>(Tool[slot].tip_up_pos+1))
	//  {
	//  job.state=JOB_PAUSED_DUE_TO_ERROR;
	//  Tool[slot].state=TL_FAILED;
	//  printf("\n\n** ERROR - SLOT %d OUT OF TIP POSITION **\n\n",slot);
	//  }
	}
      }

    // update max/min and table offsets
    if(build_table_scan_flag==TRUE && job.model_first!=NULL)
      {
      job_maxmin();
      job_map_to_table();
      build_table_scan_flag=FALSE;
      }

    // update count of tools that are loaded for use but not necessarily ready (i.e. at temp) or assigned
    tool_count=0;							// init number of tools ready for use
    for(slot=0;slot<MAX_TOOLS;slot++)					// loop thru all tools on carriage
      {
      if(Tool[slot].state<TL_LOADED)continue;				// if tool not defined, skip it
      //if(Tool[slot].select_status==TRUE)tool_count++;			// if tool selected for this job
      tool_count++;
      }

    // update model load and model control buttons
    // do so by checking if models are loaded and if so, if the active model has any operations defined
    memset(idl_scratch,0,sizeof(idl_scratch));
    oper_chk=0;
    modl_chk=0;
    if(job.state<JOB_RUNNING)						// if not yet running...
      {
      mptr=job.model_first;						// loop thru all models and check if each
      while(mptr!=NULL)							// referenced tool is loaded and defined
	{
	modl_chk++;							// increment model count
	if(mptr==active_model)						// only check active model for ops defined
	  {
	  amptr=mptr->oper_list;					// get first op off model list
	  while(amptr!=NULL)
	    {
	    j=amptr->ID;						// get tool ID for that op
	    if(j<0 || j>=MAX_TOOLS){amptr=amptr->next;continue;}	// if out of range of tools for this machine
	    oper_chk++;							// increment operation count
	    amptr=amptr->next;						// get next op 
	    }
	  }
	mptr=mptr->next;						// get next model
	}
      if(modl_chk>0)							// if we have a model available... change btn to green
        {
	gtk_image_clear(GTK_IMAGE(img_model_add));			// MUST be cleared then re-loaded or it becomes severe memory leak
	img_model_add=gtk_image_new_from_file("Model-Add-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_model_add),50);
	gtk_button_set_child(GTK_BUTTON(btn_model_add),img_model_add);
	}
      else 								// otherwise leave btn yellow
        {
	gtk_image_clear(GTK_IMAGE(img_model_add));			// MUST be cleared then re-loaded or it becomes severe memory leak
	img_model_add=gtk_image_new_from_file("Model-Add-Y.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_model_add),50);
	gtk_button_set_child(GTK_BUTTON(btn_model_add),img_model_add);
	}

//printf("\ntool=%d  model=%d  oper=%d  tool_state=%d \n",tool_chk,modl_chk,oper_chk,Tool[0].state);

      if(tool_count<1)							// if no tools available yet...
	{
	if(img_model_index!=0)						// ... and gray btn not displayed, display gray btn
	  {
	  if(img_model_control!=NULL)gtk_image_clear(GTK_IMAGE(img_model_control));
	  img_model_control = gtk_image_new_from_file("Model-Ctrl-B.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_control),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_control),img_model_control);
	  img_model_index=0;
	  }
	}
      else 								// otherwise we have at least one tool ready...
	{
	if(modl_chk>0 && oper_chk>0)					// if model loaded and ops assigned...
	  {
	  if(img_model_index!=2)					// ... and green btn not displayed yet, display green btn
	    {
	    if(img_model_control!=NULL)gtk_image_clear(GTK_IMAGE(img_model_control));
	    img_model_control = gtk_image_new_from_file("Model-Ctrl-G.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_model_control),50);
	    gtk_button_set_child(GTK_BUTTON(btn_model_control),img_model_control);
	    img_model_index=2;
	    }
	  }
	else if(modl_chk>0 && oper_chk==0)				// if model loaded but no ops assigned to it...
	  {
	  if(img_model_index!=1)					// ... and yellow btn not displayed yet, display yellow btn
	    {
	    if(img_model_control!=NULL)gtk_image_clear(GTK_IMAGE(img_model_control));
	    img_model_control = gtk_image_new_from_file("Model-Ctrl-Y.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_model_control),50);
	    gtk_button_set_child(GTK_BUTTON(btn_model_control),img_model_control);
	    img_model_index=1;
	    }
	  }
	else 								// if no models yet...
	  {
	  if(img_model_index!=0)					// ... and gray btn not displayed, display gray btn
	    {
	    if(img_model_control!=NULL)gtk_image_clear(GTK_IMAGE(img_model_control));
	    img_model_control = gtk_image_new_from_file("Model-Ctrl-B.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_model_control),50);
	    gtk_button_set_child(GTK_BUTTON(btn_model_control),img_model_control);
	    img_model_index=0;
	    }
	  }
	}
      }

    //show_ready_status=TRUE;
    if(show_ready_status==TRUE)
      {
      printf("\n\n");
      printf(" Tool Count=%d  Tool Check= %d  Model Count=%d  Oper Count=%d \n\n",tool_count,tool_chk,modl_chk,oper_chk);
      }

    // determine if job is ready by making sure all model operations reference tools that are defined
    // and at least one of those tools has a model loaded
    memset(idl_scratch,0,sizeof(idl_scratch));
    if(job.state<JOB_READY && tool_count>0)
      {
      oper_chk=0;
      tool_chk=0;
      mptr=job.model_first;						// loop thru all models and check if each
      while(mptr!=NULL)							// referenced tool is loaded and defined
	{
	amptr=mptr->oper_list;
	while(amptr!=NULL)						// loop thru all operations for this model
	  {
	  j=amptr->ID;							// get slot ID for this operation
	  if(j<0 || j>=MAX_TOOLS){amptr=amptr->next;continue;}		// make sure it is valid
	  oper_chk++;							// increment the number of operations used in this job
	  if(Tool[j].state>=TL_READY)tool_chk++;			// increment the number of tools ready to do those operations (same one counted multi times)
	  amptr=amptr->next;
	  }
	mptr=mptr->next;
	}
      if(oper_chk>0 && tool_chk>0 && job.model_first!=NULL)		// if all operations have tools at the ready...
        {
	// set state to allow job to start
	if(job.regen_flag==FALSE)					// good to go once slicing is done
	  {
	  job.state=JOB_READY;						// if temps are at operational
    
	  // set state depending on if build table are at temp. carriage tools get taken care of by print loop
	  // check build table
	  q=fabs(Tool[BLD_TBL1].thrm.tempC-Tool[BLD_TBL1].thrm.setpC);	// get delta between actual and desired of build table
	  if(q>Tool[BLD_TBL1].thrm.tolrC)
	    {
	    job.state=JOB_HEATING;					// if temp not close enough, then stay at heating
	    }
	  else 
	    {
	    if(good_to_go==TRUE)
	      {
	      job.state=JOB_READY;					// otherwise if user play was pressed and now at temp, go
	      change_job_state(NULL,NULL);
	      }
	    }

	  // check chamber
	  //q=fabs(Tool[CHAMBER].thrm.tempC-Tool[CHAMBER].thrm.setpC);	// get delta between actual and desired of chamber
	  //if(q>Tool[CHAMBER].thrm.tolrC)job.state=JOB_HEATING;	// if temp not close enough, then stay at heating
	  }
	}
      }

    // update play/pause/abort buttons and information status message
    // if not ready, the play and abort buttons are gray
    // if job ready, the play button is green, the abort is gray
    // if job running, the puase is displayed in place of play, the abort is red - abort in this case is an instant stop
    // if job paused, the play button is green, and the abort button is red - abort in this case kills the current job
    
    // calculate % complete based on z height... used in most of the job state status reports
    //amt_complete=z_cut/ZMax*100;						// percent complete for additive
    //if(Tool[current_tool].type==SUBTRACTIVE)amt_complete=100-amt_complete; 	// percent complete for subtractive
    
    // calculate job status % complete based on distance travelled...
    amt_complete=0.0;
    if(job.total_dist>0)amt_complete=job.current_dist/job.total_dist*100;
    
    // handle condition when user "unloads" all models after job was ready to go
    if(job.model_first==NULL)job.state=JOB_NOT_READY;
    
    // turn on carriage leds to indicate ready to go
    if(job.state>=JOB_READY && job.state<JOB_COMPLETE){gpioWrite(CARRIAGE_LED,!ON);}
    else {gpioWrite(CARRIAGE_LED,!OFF);}
    
    if(job.state==UNDEFINED)
      {
      sprintf(job_state_msg," Job undefined ");
      if(img_job_btn_index!=0)
        {
        gtk_image_clear(GTK_IMAGE(img_job_btn));
        img_job_btn = gtk_image_new_from_file("Play-B.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
        img_job_btn_index=0;

	sprintf(idl_scratch,"Job Time");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);
        }
      //printf(" tool count=%d  ready count=%d  operation count=%d  job.model_first=%X \n",tool_count,tool_chk,oper_chk,job.model_first);
      }
    if(job.state==JOB_NOT_READY)
      {
      sprintf(job_state_msg," Job not ready ");
      if(img_job_btn_index!=0)
        {
        gtk_image_clear(GTK_IMAGE(img_job_btn));
        img_job_btn = gtk_image_new_from_file("Play-B.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
        img_job_btn_index=0;

	sprintf(idl_scratch,"Job Time");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);
        }
      //printf(" tool count=%d  ready count=%d  operation count=%d  job.model_first=%X \n",tool_count,tool_chk,oper_chk,job.model_first);
      }
    if(job.state==JOB_HEATING)
      {
      idle_start_time=time(NULL);						// reset idle time
      sprintf(job_state_msg," Heating ");
      sprintf(idl_scratch," Stabilizing system temperatures... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(good_to_go==TRUE)
        {
	sprintf(idl_scratch," Job will start when at temperature... ");
	if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
	else {sprintf(job_status_msg,"%s",idl_scratch);}
	if(img_job_btn_index!=2)
	  {
	  gtk_image_clear(GTK_IMAGE(img_job_btn));
	  img_job_btn = gtk_image_new_from_file("Pause.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	  gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	  img_job_btn_index=2;
	  }
	}
      else 
        {
	if(img_job_btn_index!=1)
	  {
	  gtk_image_clear(GTK_IMAGE(img_job_btn));
	  img_job_btn = gtk_image_new_from_file("Play-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	  gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	  img_job_btn_index=1;

	  sprintf(idl_scratch,"Job Time");
	  gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	  // turn on tool feed buttons while paused
	  show_tool_feed=TRUE;
	  gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	  gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	  }
	}
      }
    if(job.state==JOB_READY)
      {
      sprintf(job_state_msg," Job ready ");
      sprintf(idl_scratch," Press play button to start job... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Time");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_CALIBRATING)
      {
      job.finish_time=time(NULL);
      sprintf(job_state_msg," Calibrating ");
      }
    if(job.state==JOB_RUNNING)
      {
      job.finish_time=time(NULL);
      sprintf(job_state_msg," Running ");
      if(current_tool>=0 && current_tool<MAX_TOOLS)
        {
	if((Tool[current_tool].pwr24==TRUE || Tool[current_tool].pwr48==TRUE) && Tool[current_tool].tool_ID!=TC_LASER)
	  {
	  q=fabs(Tool[current_tool].thrm.tempC-Tool[current_tool].thrm.setpC);					// get temp delta
	  if(q<=Tool[current_tool].thrm.tolrC)q=fabs(Tool[BLD_TBL1].thrm.tempC-Tool[BLD_TBL1].thrm.setpC);	// if tool in tolerance... check bld table
	  if(q>Tool[current_tool].thrm.tolrC)sprintf(idl_scratch," Adjusting temperature... within %2.0fC of target. ",q);
	  }
	else 
	  {
	  if(Tool[current_tool].type==ADDITIVE)sprintf(idl_scratch," Depositing using tool %d... ",(current_tool+1));
	  if(Tool[current_tool].type==SUBTRACTIVE)sprintf(idl_scratch," Cutting using tool %d... ",(current_tool+1));
	  if(Tool[current_tool].type==MARKING)sprintf(idl_scratch," Marking using tool %d... ",(current_tool+1));
	  }
	if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
	else {sprintf(job_status_msg,"%s",idl_scratch);}
	}
      gtk_adjustment_set_value(g_adj_z_level,z_cut);
      if(img_job_btn_index!=2)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Pause.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=2;

	sprintf(idl_scratch,"Job Running");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn off tool feed buttons while running
	show_tool_feed=FALSE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_PAUSED_BY_USER)
      {
      sprintf(job_state_msg," Paused by user request ");
      if(bufferAvail==MAX_BUFFER){sprintf(idl_scratch," Press play to resume... ");}
      if((MAX_BUFFER-bufferAvail)>2){sprintf(idl_scratch," Processing %d queued moves before pausing... ",(MAX_BUFFER-bufferAvail));}
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Paused");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_PAUSED_BY_CMD)
      {
      sprintf(job_state_msg," Paused by system command ");
      if(fabs(Tool[slot].matl.mat_used-Tool[slot].matl.mat_volume)<1)strcat(job_state_msg," - out of material.");
      sprintf(idl_scratch," Press play to resume... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Paused");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_PAUSED_FOR_BIT_CHG)
      {
      sprintf(job_state_msg," Paused for bit change ");
      sprintf(idl_scratch," Use tool menu to change bit.  Press play to resume... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      tool_add_matl(NULL, GINT_TO_POINTER(current_tool));		// show dialog to load new bit
      job.state=JOB_PAUSED_BY_USER;					// change state to user paused to not redisplay dialog
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Paused");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_PAUSED_DUE_TO_ERROR)
      {
      sprintf(job_state_msg," Paused due to error ");
      sprintf(idl_scratch," Check tool operation and system status... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Paused");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_TABLE_SCAN)
      {
      sprintf(job_state_msg," Scanning table ");
      amt_complete=(Pz_index)/(BUILD_TABLE_LEN_X/scan_dist)*(BUILD_TABLE_LEN_Y/scan_dist)*100;
      sprintf(idl_scratch," Tool=%d ... ",(current_tool+1));
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=2)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Pause.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);	
	img_job_btn_index=2;

	sprintf(idl_scratch,"Scan Running");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn off tool feed buttons while scanning
	show_tool_feed=FALSE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_TABLE_SCAN_PAUSED)
      {
      sprintf(job_state_msg," Table scan paused ");
      amt_complete=(Pz_index)/(BUILD_TABLE_LEN_X/scan_dist)*(BUILD_TABLE_LEN_Y/scan_dist)*100;
      sprintf(idl_scratch," Press play to resume... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);	
	img_job_btn_index=1;

	sprintf(idl_scratch,"Scan Paused");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);

	// turn off tool feed buttons while running
	show_tool_feed=FALSE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
	}
      }
    if(job.state==JOB_COMPLETE)	
      {
      job.current_dist=job.total_dist;
      sprintf(job_state_msg," Job Complete ");
      sprintf(idl_scratch,"Click <Abort> to clear the job, or <Pause/Run> to run again... ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=1)
	{
	gtk_image_clear(GTK_IMAGE(img_job_btn));
	img_job_btn = gtk_image_new_from_file("Play-G.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
	img_job_btn_index=1;

	sprintf(idl_scratch,"Job Complete");
	gtk_frame_set_label(GTK_FRAME(frame_time_box),idl_scratch);
	
	// turn on tool feed buttons while paused
	show_tool_feed=TRUE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);

	sprintf(scratch,"Job print completed.");
	entry_system_log(scratch);
	//job_complete();
        }
      }
    if(job.state==JOB_SLEEPING)	
      {
      sprintf(job_state_msg," Sleeping ... Zzzz ");
      sprintf(idl_scratch," Select tool or model to wake ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      if(img_job_btn_index!=0)
        {
        gtk_image_clear(GTK_IMAGE(img_job_btn));
        img_job_btn = gtk_image_new_from_file("Play-B.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
        img_job_btn_index=0;

	// turn off tool feed buttons while sleeping
	show_tool_feed=FALSE;
	gtk_widget_set_visible(btn_tool_feed_fwd,show_tool_feed);
	gtk_widget_set_visible(btn_tool_feed_rvs,show_tool_feed);
        }
      }
    if(job.state==JOB_WAKING_UP)
      {
      if(img_job_btn_index!=0)
        {
        gtk_image_clear(GTK_IMAGE(img_job_btn));
        img_job_btn = gtk_image_new_from_file("Play-B.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
	gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
        img_job_btn_index=0;
        }
      }
    if(job.state==JOB_ABORTED_BY_USER)
      {
      sprintf(idl_scratch," Aborted by user request ");
      if(strlen(job_status_msg)<(sizeof(job_status_msg)-strlen(idl_scratch))){strcat(job_status_msg,idl_scratch);}
      else {sprintf(job_status_msg,"%s",idl_scratch);}
      }
    
    // update temperatures in tool temperature offset if up
    if(win_tooloffset_flag==TRUE)
      {
      sprintf(idl_scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.tempC,Tool[device_being_calibrated].thrm.tempV);
      gtk_label_set_text(GTK_LABEL(temp_tool_disp),idl_scratch);
      }

    // update temperatures in settings window if calibrating
    if(win_settingsUI_flag==TRUE)
      {
      if(device_being_calibrated<0 || device_being_calibrated>=MAX_THERMAL_DEVICES)device_being_calibrated=0;
      if(Tool[device_being_calibrated].thrm.sensor<=0)Tool[device_being_calibrated].thrm.sensor=RTDH;
      if(Tool[device_being_calibrated].thrm.tempV>4.90)			// if nothing connected...
        {
	sprintf(idl_scratch,"--C %6.3fv",Tool[device_being_calibrated].thrm.tempV);
	set_temp_cal_load=FALSE;
	}
      else 								// if able to read a clean temperature...
        {
	sprintf(idl_scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.tempC,Tool[device_being_calibrated].thrm.tempV);
	set_temp_cal_load=TRUE;
	}
      gtk_label_set_text(GTK_LABEL(temp_tool_disp),idl_scratch);
      
      //printf("Setting Calib: %s \n",idl_scratch);
      }
    
    // update build table status and temperature
    // this info only appears in the tool view, so only update when active
    if(main_view_page==VIEW_TOOL)
      {
      sprintf(idl_scratch," Okay ");
      if(Tool[BLD_TBL1].epwr48==TRUE || Tool[BLD_TBL2].epwr48==TRUE)
	{
	sprintf(idl_scratch," thermal control failure");
	markup = g_markup_printf_escaped (error_fmt,idl_scratch);
	}
      else 
	{
	sprintf(idl_scratch," Okay ");
	markup = g_markup_printf_escaped (norm_fmt,idl_scratch);
	}
      gtk_label_set_text(GTK_LABEL(stat_bldtbl),idl_scratch);
      gtk_label_set_markup(GTK_LABEL(stat_bldtbl),markup);
      gtk_widget_queue_draw(stat_bldtbl);
      q=(Tool[BLD_TBL1].thrm.tempC+Tool[BLD_TBL2].thrm.tempC)/2.0;
      if(q<0)q=1.0;
      if(q>500)q=500.0;
      sprintf(idl_scratch,"%6.0fC ",q);
      gtk_label_set_text(GTK_LABEL(lbl_bldtbl_tempC),idl_scratch);
      gtk_widget_queue_draw(lbl_bldtbl_tempC);
      gtk_level_bar_set_min_value(temp_bar_bldtbl,0.0);
      gtk_level_bar_set_max_value(temp_bar_bldtbl,(Tool[BLD_TBL1].thrm.maxtC));
      gtk_level_bar_add_offset_value(temp_bar_bldtbl,GTK_LEVEL_BAR_OFFSET_LOW,(Tool[BLD_TBL1].thrm.setpC*0.1));
      gtk_level_bar_add_offset_value(temp_bar_bldtbl,GTK_LEVEL_BAR_OFFSET_HIGH,(Tool[BLD_TBL1].thrm.setpC-Tool[BLD_TBL1].thrm.tolrC));
      gtk_level_bar_add_offset_value(temp_bar_bldtbl,GTK_LEVEL_BAR_OFFSET_FULL,(Tool[BLD_TBL1].thrm.setpC+Tool[BLD_TBL1].thrm.tolrC));
      gtk_level_bar_set_value (temp_bar_bldtbl,q);
      sprintf(idl_scratch,"---");
      if(Tool[BLD_TBL1].pwr48==TRUE)sprintf(idl_scratch,"%6.0f C",Tool[BLD_TBL1].thrm.setpC);
      gtk_label_set_text(GTK_LABEL(lbl_bldtbl_setpC),idl_scratch);
      gtk_widget_queue_draw(lbl_bldtbl_setpC);
      }

    // update chamber status and temperature
    // this info only appears in the tool view, so only update when active
    if(main_view_page==VIEW_TOOL)
      {
      sprintf(idl_scratch," Okay ");
      if(Tool[CHAMBER].epwr24==TRUE)
	{
	sprintf(idl_scratch," thermal control failure");
	markup = g_markup_printf_escaped (error_fmt,idl_scratch);
	}
      else 
	{
	sprintf(idl_scratch," Okay ");
	markup = g_markup_printf_escaped (norm_fmt,idl_scratch);
	}
      gtk_label_set_text(GTK_LABEL(stat_chm_lbl),idl_scratch);
      gtk_label_set_markup(GTK_LABEL(stat_chm_lbl),markup);
      gtk_widget_queue_draw(stat_chm_lbl);
      q=Tool[CHAMBER].thrm.tempC;
      if(q<0)q=1.0;
      if(q>300)q=300.0;
      sprintf(idl_scratch,"%6.0fC ",q);
      gtk_label_set_text(GTK_LABEL(lbl_chmbr_tempC),idl_scratch);
      gtk_widget_queue_draw(lbl_chmbr_tempC);
      gtk_level_bar_set_min_value(temp_bar_chamber,0.0);
      gtk_level_bar_set_max_value(temp_bar_chamber,(Tool[CHAMBER].thrm.maxtC+10.0));
      gtk_level_bar_add_offset_value(temp_bar_chamber,GTK_LEVEL_BAR_OFFSET_LOW,(Tool[CHAMBER].thrm.setpC*0.1));
      gtk_level_bar_add_offset_value(temp_bar_chamber,GTK_LEVEL_BAR_OFFSET_HIGH,(Tool[CHAMBER].thrm.setpC-Tool[CHAMBER].thrm.tolrC));
      gtk_level_bar_add_offset_value(temp_bar_chamber,GTK_LEVEL_BAR_OFFSET_FULL,(Tool[CHAMBER].thrm.setpC+Tool[CHAMBER].thrm.tolrC));
      gtk_level_bar_set_value (temp_bar_chamber,q);
      sprintf(idl_scratch,"%6.0f C",Tool[CHAMBER].thrm.setpC);
      gtk_label_set_text(GTK_LABEL(lbl_chmbr_setpC),idl_scratch);
      gtk_widget_queue_draw(lbl_chmbr_setpC);
      }

    // update job state message
    //if(strlen(job_state_msg)<(sizeof(job_state_msg)-5)){sprintf(idl_scratch,"%d",cmdCount);strcat(job_state_msg,idl_scratch);}
    gtk_label_set_text(GTK_LABEL(job_state_lbl),job_state_msg);
    gtk_widget_queue_draw(job_state_lbl);

    // update job status msg
    gtk_label_set_text(GTK_LABEL(job_status_lbl),job_status_msg);
    gtk_widget_queue_draw(job_status_lbl);

    // update actual hours elapsed message
    a_hrs=(int)((job.finish_time-job.start_time)/3600.0);					// hours elapsed
    a_mins=(int)((job.finish_time-job.start_time)/60.0)-(a_hrs*60);				// minutes elapsed
    sprintf(idl_scratch," Elapsed..:  %3d hr %2d min",a_hrs,a_mins);
    gtk_label_set_text( GTK_LABEL(elaps_time_lbl), idl_scratch);
    gtk_widget_queue_draw(elaps_time_lbl);
    
    // update estimated hours left message
    e_hrs=(int)((job.time_estimate-(job.finish_time-job.start_time))/3600.0);			// hours remaining until done
    e_mins=(int)((job.time_estimate-(job.finish_time-job.start_time))/60.0)-(e_hrs*60);		// minutes remaining until done
    sprintf(idl_scratch," Remaining:  %3d hr %2d min",e_hrs,e_mins);
    gtk_label_set_text( GTK_LABEL(est_time_lbl), idl_scratch);
    gtk_widget_queue_draw(est_time_lbl);

    // turn off bound box viewing as it is meant to only be visible for one idle cycle
    set_view_bound_box=FALSE;
    
    // update tool view params for single head units
    // other units don't have the screen space the way the gammas do
    if(MAX_TOOLS==1)
      {
      // if a new layer, reset stats
      if(fabs(job.current_z-old_jobz)>=CLOSE_ENOUGH)
	{
	PostGVAvg=0.0; PostGVCnt=0; PostGVMax=0.0;
	PostGFAvg=0.0; PostGFCnt=0; PostGFMax=0.0;
	read_prev_time=read_cur_time;
	oldposta=PostG.a;
	old_jobz=job.current_z;
	}

      // position
      sprintf(idl_scratch,"%06.3f",PostG.x);
      gtk_label_set_text(GTK_LABEL(lbl_val_pos_x),idl_scratch);
      gtk_widget_queue_draw(lbl_val_pos_x);
      sprintf(idl_scratch,"%06.3f",PostG.y);
      gtk_label_set_text(GTK_LABEL(lbl_val_pos_y),idl_scratch);
      gtk_widget_queue_draw(lbl_val_pos_y);
      sprintf(idl_scratch,"%06.3f",PostG.z);
      if(Tool[0].state>TL_UNKNOWN)sprintf(idl_scratch,"%06.3f",(PostG.z-Tool[0].tip_dn_pos+Tool[0].tip_z_fab));
      gtk_label_set_text(GTK_LABEL(lbl_val_pos_z),idl_scratch);
      gtk_widget_queue_draw(lbl_val_pos_z);
      sprintf(idl_scratch,"%06.3f",PostG.a);
      gtk_label_set_text(GTK_LABEL(lbl_val_pos_a),idl_scratch);
      gtk_widget_queue_draw(lbl_val_pos_a);
      
      // speed
      PostGVel /= 60.0;							// convert from mm/min to mm/sec
      PostGVCnt++;							// increment speed counter
      
      sprintf(idl_scratch,"%6.1f",PostGVel);				// update current speed
      gtk_label_set_text(GTK_LABEL(lbl_val_spd_crt),idl_scratch);
      gtk_widget_queue_draw(lbl_val_spd_crt);

      PostGVAvg=((PostGVAvg*(PostGVCnt-1))+PostGVel)/PostGVCnt;		// update average speed
      sprintf(idl_scratch,"%6.1f",PostGVAvg);
      gtk_label_set_text(GTK_LABEL(lbl_val_spd_avg),idl_scratch);
      gtk_widget_queue_draw(lbl_val_spd_avg);
      
      if(PostGVel>PostGVMax)PostGVMax=PostGVel;				// update maxium speed
      sprintf(idl_scratch,"%6.1f",PostGVMax);
      gtk_label_set_text(GTK_LABEL(lbl_val_spd_max),idl_scratch);
      gtk_widget_queue_draw(lbl_val_spd_max);
      
      // flow rate
      if(PostG.a>10.0)
        {
	read_cur_time=time(NULL);
	if(abs(read_cur_time-read_prev_time)>1)
	  {
	  PostGFlow=60*(2.405*(PostG.a-oldposta))/(read_cur_time-read_prev_time); // 60 * filament area * filament length / interval = mm^3/min
	  PostGFCnt++;							// increment flow counter
	  
	  sprintf(idl_scratch,"%6.2f",PostGFlow);			// update current flowrate
	  gtk_label_set_text(GTK_LABEL(lbl_val_flw_crt),idl_scratch);
	  gtk_widget_queue_draw(lbl_val_flw_crt);

	  PostGFAvg=((PostGFAvg*(PostGFCnt-1))+PostGFlow)/PostGFCnt;	// update average flowrate
	  sprintf(idl_scratch,"%6.2f",PostGFAvg);
	  gtk_label_set_text(GTK_LABEL(lbl_val_flw_avg),idl_scratch);
	  gtk_widget_queue_draw(lbl_val_flw_avg);
	  
	  if(PostGFlow>PostGFMax)PostGFMax=PostGFlow;			// update maxium flowrate
	  sprintf(idl_scratch,"%6.2f",PostGFMax);
	  gtk_label_set_text(GTK_LABEL(lbl_val_flw_max),idl_scratch);
	  gtk_widget_queue_draw(lbl_val_flw_max);
	  }
	}
	
      // controller buffers
      sprintf(idl_scratch,"%d",cmdCount);
      gtk_label_set_text(GTK_LABEL(lbl_val_cmdct),idl_scratch);
      gtk_widget_queue_draw(lbl_val_cmdct);
	
      sprintf(idl_scratch,"%d",cmd_que);
      gtk_label_set_text(GTK_LABEL(lbl_val_buff_used),idl_scratch);
      gtk_widget_queue_draw(lbl_val_buff_used);
	
      //sprintf(idl_scratch,"%d",bufferAvail);
      sprintf(idl_scratch,"%d",Rx_bytes);
      gtk_label_set_text(GTK_LABEL(lbl_val_buff_avail),idl_scratch);
      gtk_widget_queue_draw(lbl_val_buff_avail);
      }

    // check the status of separate threads
    if(thermal_thread_alive!=1)
      {
      job.state=JOB_PAUSED_DUE_TO_ERROR;
      printf("\n\n** ERROR - thermal thread is not responding! ** \n\n");
      }
    if(job.state>JOB_READY && print_thread_alive!=1)			// only worry about it when we need to use it
      {
      job.state=JOB_PAUSED_DUE_TO_ERROR;
      printf("\n\n** ERROR - print engine thread is not responding! ** \n\n");
      }

    gtk_widget_queue_draw(hbox);					// update container for status info
    gtk_widget_queue_draw(g_model_area);
    //gtk_widget_queue_draw(g_tool_area);
    gtk_widget_queue_draw(g_layer_area);
    gtk_widget_queue_draw(g_graph_area);
    gtk_widget_queue_draw(g_camera_area);
    //gtk_widget_queue_draw(g_vulkan_area);

    return(TRUE);							// return TRUE to make it get called again
}


// Function to build the primary interface
void activate_app (GtkApplication* app, gpointer user_data)
{
    char		padding[32],cam_scratch[128];
    int			h,i=0,col_width;;
    int 		pmin, pmax;
    int			tool_stat_x,tool_stat_y;
    
    GtkWidget		*btn_draw_select;
      
    GtkWidget		*lbl_draw, *lbl_model, *lbl_status, *lbl_graph, *lbl_camera, *lbl_vulkan, *vlabel;
    GtkWidget		*tstat_lbl,*ltype_lbl,*oper_lbl,*tip_stat_lbl;
    GtkWidget		*hbox; 
    GtkWidget		*vboxleft,*vboxmid,*vboxright;
    GtkWidget		*stat_top,*stat_btm,*stat_btm_left,*stat_btm_right;
    GtkWidget		*stat_tool[MAX_TOOLS];
    GtkWidget		*grid_tool[MAX_TOOLS];
    GtkWidget		*hsep;
    GtkWidget		*stat_tool_type[MAX_TOOLS],*stat_tool_matl[MAX_TOOLS];
    GtkWidget 		*stat_tool_lvl[MAX_TOOLS],*stat_tool_temp[MAX_TOOLS];
    GtkWidget		*btn_z_up,*btn_z_dn;
    GtkWidget		*grid_manip,*btn_select,*btn_clear,*btn_drag;
    GtkWidget		*btn_align,*btn_center,*btn_intrs,*btn_merge,*btn_norms;
    GtkWidget 		*btn_separator,*spacer1,*spacer2;
    GtkWidget		*frame;
    GtkWidget		*stat_bld_tbl,*stat_chamber,*grid_bldtbl,*grid_chamber;
    GtkWidget		*type_bldtbl,*type_chamber;
    GError     		*error = NULL;

    // define formats for GTK messages
    strcpy(error_fmt,"<span foreground='red'>%s</span>");
    strcpy(okay_fmt,"<span foreground='green'>%s</span>");
    strcpy(norm_fmt,"<span foreground='black'>%s</span>");
    strcpy(unkw_fmt,"<span foreground='orange'>%s</span>");
    strcpy(stat_fmt,"<span foreground='blue'>%s</span>");

    // create our main window
    win_main = gtk_application_window_new(app);
    //g_signal_connect (win_main, "destroy", G_CALLBACK (destroy), NULL);

    // set up window the way we want it
    sprintf(scratch," Nx3D ver %01d.%04d.%01d ", AA4X3D_MAJOR_VERSION, AA4X3D_MINOR_VERSION, AA4X3D_MICRO_VERSION);
    gtk_window_set_title(GTK_WINDOW(win_main),scratch);			
    gtk_window_set_resizable(GTK_WINDOW(win_main),TRUE);				// since running on fixed LCD, do NOT allow resizing

    // set up an hbox to divide the screen into a narrow left segment, a wide mid segment, and a narrow right segment
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,3);
    gtk_window_set_child (GTK_WINDOW (win_main), hbox);
    
    // set up a vbox on left side of screen to encapsulate persistant buttons
    {
      vboxleft = gtk_box_new(GTK_ORIENTATION_VERTICAL,7);
      gtk_widget_set_size_request(vboxleft,80,LCD_HEIGHT-10);
      gtk_box_append(GTK_BOX(hbox),vboxleft);
      
      // set up RUN button
      btn_run = gtk_button_new ();
      g_signal_connect (btn_run, "clicked", G_CALLBACK (change_job_state), NULL);
      gtk_box_append(GTK_BOX(vboxleft),btn_run);
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_run), "Run/Pause job");
      gtk_widget_set_size_request(btn_run,80,50);

      img_job_btn = gtk_image_new_from_file("Play-B.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_job_btn),50);
      img_job_btn_index=0;						// 0=not ready, 1=play, 2=pause
      gtk_button_set_child(GTK_BUTTON(btn_run),img_job_btn);
      
      // set up ABORT button
      btn_abort = gtk_button_new ();
      g_signal_connect (btn_abort, "clicked", G_CALLBACK (abort_job), NULL);
      gtk_box_append(GTK_BOX(vboxleft),btn_abort);
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_abort), "Stop immediately");
      gtk_widget_set_size_request(btn_abort,80,50);

      img_job_abort = gtk_image_new_from_file("Abort-R.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_job_abort),50);
      gtk_button_set_child(GTK_BUTTON(btn_abort),img_job_abort);
      
      // set up button separator
      btn_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
      gtk_box_append(GTK_BOX(vboxleft),btn_separator);

      // set up SETTINGS button
      btn_settings = gtk_button_new();
      g_signal_connect (btn_settings, "clicked", G_CALLBACK (settings_UI), GINT_TO_POINTER(main_view_page));
      gtk_box_append(GTK_BOX(vboxleft),btn_settings);
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_settings), "Settings");
      gtk_widget_set_size_request(btn_settings,80,50);
      img_settings = gtk_image_new_from_file("Settings.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_settings),50);
      gtk_button_set_child(GTK_BUTTON(btn_settings),img_settings);
      
      // set up TEST button
      //btn_test = gtk_button_new_with_label ("Test");
      //g_signal_connect (btn_test, "clicked", G_CALLBACK (on_test_btn), win_main);
      //gtk_box_pack_start (GTK_BOX(vboxleft), btn_test, FALSE, FALSE, 0);
      
      // set up button separator
      btn_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
      gtk_box_append(GTK_BOX(vboxleft),btn_separator);

      // set up QUIT button
      btn_quit = gtk_button_new_with_label ("Quit");
      gtk_widget_set_size_request(btn_quit,80,50);
      g_signal_connect (btn_quit, "clicked", G_CALLBACK (destroy), win_main);
      gtk_box_append(GTK_BOX(vboxleft),btn_quit);
      
    }

    // set up a vbox in middle of screen to encapsulate the notebook of different views
    // divide it into two vertical segments:  top of job stats, btm for tools stats and drawing views
    vboxmid = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_widget_set_size_request(vboxmid,LCD_WIDTH-170,LCD_HEIGHT-10);
    gtk_box_append(GTK_BOX(hbox),vboxmid);
    
    // set up job status bar across top
    {
    stat_top=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);	
    gtk_widget_set_size_request(stat_top,LCD_WIDTH-170,70);
    gtk_box_append(GTK_BOX(vboxmid),stat_top);		
    
    sprintf(scratch,"Job Status");
    GtkWidget *frame_stat_box=gtk_frame_new(scratch);
    stat_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(frame_stat_box), stat_box);
    gtk_widget_set_size_request(stat_box,630,65);
    gtk_widget_set_hexpand(stat_box,TRUE);
    gtk_widget_set_vexpand(stat_box,FALSE);
    gtk_box_append(GTK_BOX(stat_top),frame_stat_box);

    grid_job=gtk_grid_new();
    gtk_box_append(GTK_BOX(stat_box),grid_job);
    gtk_grid_set_row_spacing (GTK_GRID(grid_job),0);
    gtk_grid_set_column_spacing (GTK_GRID(grid_job),0);

    job_state_lbl=gtk_label_new(" ");
    gtk_grid_attach(GTK_GRID(grid_job),job_state_lbl,0,0,1,1);
    gtk_label_set_xalign (GTK_LABEL(job_state_lbl),0.0);

    job_status_lbl=gtk_label_new(" ");
    gtk_grid_attach(GTK_GRID(grid_job),job_status_lbl,0,1,3,1);
    gtk_label_set_xalign (GTK_LABEL(job_status_lbl),0.0);

    // encapsulate time elapsed/remaining in its own box
    sprintf(scratch,"Job Time");
    frame_time_box=gtk_frame_new(scratch);
    time_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(frame_time_box), time_box);
    gtk_widget_set_size_request(time_box,200,65);
    gtk_widget_set_hexpand(time_box,FALSE);
    gtk_widget_set_vexpand(time_box,FALSE);
    gtk_box_append(GTK_BOX(stat_top),frame_time_box);

    grid_time=gtk_grid_new();
    gtk_box_append(GTK_BOX(time_box),grid_time);
    gtk_grid_set_row_spacing (GTK_GRID(grid_time),0);
    gtk_grid_set_column_spacing (GTK_GRID(grid_time),0);

    elaps_time_lbl=gtk_label_new(" ");
    gtk_grid_attach(GTK_GRID(grid_time),elaps_time_lbl,0,0,1,1);
    gtk_label_set_xalign (GTK_LABEL(elaps_time_lbl),0.0);

    est_time_lbl=gtk_label_new(" ");
    gtk_grid_attach(GTK_GRID(grid_time),est_time_lbl,0,1,1,1);
    gtk_label_set_xalign (GTK_LABEL(est_time_lbl),0.0);
    
    }
    

    // set up drawing areas for view displays
    {
      // set up the notebook. note the children must be created first, then added
      // they exist below the job status box to allow status display under all views
      g_notebook = gtk_notebook_new ();
      gtk_box_append(GTK_BOX(vboxmid),g_notebook);
      g_signal_connect (g_notebook, "switch_page",G_CALLBACK (on_nb_page_switch), NULL);
      gtk_widget_set_size_request(g_notebook,LCD_WIDTH-170,LCD_HEIGHT-180);
      gtk_notebook_set_show_border(GTK_NOTEBOOK(g_notebook),TRUE);

      // set up model view area
	{
	g_model_all=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	lbl_model=gtk_label_new("Model View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_model_all,lbl_model);
	
	g_model_area = gtk_drawing_area_new ();
	gtk_box_append(GTK_BOX(g_model_all),g_model_area);
	gtk_widget_set_hexpand(g_model_area,TRUE);
	gtk_widget_set_vexpand(g_model_area,TRUE);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(g_model_area), (GtkDrawingAreaDrawFunc)model_draw_callback, NULL, NULL);

	GtkGesture *model_left_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (model_left_click_gesture), 1);
	g_signal_connect(model_left_click_gesture, "pressed", G_CALLBACK(on_model_area_left_button_press_event),  g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_left_click_gesture));

	GtkGesture *model_middle_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (model_middle_click_gesture), 2);
	g_signal_connect(model_middle_click_gesture, "pressed", G_CALLBACK(on_model_area_middle_button_press_event),  g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_middle_click_gesture));

	GtkGesture *model_right_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (model_right_click_gesture), 3);
	g_signal_connect(model_right_click_gesture, "pressed", G_CALLBACK(on_model_area_right_button_press_event),  g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_right_click_gesture));

	GtkGesture *model_middle_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (model_middle_drag_gesture), 2);
	g_signal_connect(model_middle_drag_gesture, "drag-begin", G_CALLBACK(on_model_area_start_motion_press_event), g_model_area);
	g_signal_connect(model_middle_drag_gesture, "drag-update", G_CALLBACK(on_model_area_middle_motion_press_event), g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_middle_drag_gesture));

	GtkGesture *model_right_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (model_right_drag_gesture), 3);
	g_signal_connect(model_right_drag_gesture, "drag-begin", G_CALLBACK(on_model_area_start_motion_press_event), g_model_area);
	g_signal_connect(model_right_drag_gesture, "drag-update", G_CALLBACK(on_model_area_right_motion_press_event), g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_right_drag_gesture));

	// the purpose of this controller is to track the location of the cursor within the model drawing area
	GtkEventController *model_pointer_ctlr = gtk_event_controller_motion_new();
	g_signal_connect(model_pointer_ctlr, "motion", G_CALLBACK(on_model_area_motion_event), g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_pointer_ctlr));	

	// the purpose of this controller is to zoom in/out of the model drawing area
	GtkEventController *model_scroll_ctlr = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
	g_signal_connect(model_scroll_ctlr, "scroll", G_CALLBACK(on_model_area_scroll_event), g_model_area);
	gtk_widget_add_controller(g_model_area, GTK_EVENT_CONTROLLER(model_scroll_ctlr));	

	// set up model view buttons
	  {
	  g_model_btn=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_box_append(GTK_BOX(g_model_all),g_model_btn);
	  gtk_widget_set_size_request(g_model_all,LCD_WIDTH-170,50);
	  gtk_widget_set_hexpand(g_model_btn,TRUE);
	  gtk_widget_set_vexpand(g_model_btn,FALSE);
  
	  grid_model_btn=gtk_grid_new();
	  gtk_box_append(GTK_BOX(g_model_btn),grid_model_btn);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_model_btn),1);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_model_btn),1);
	
	  btn_model_add = gtk_button_new();
	  g_signal_connect (btn_model_add, "clicked", G_CALLBACK (model_add_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_add), "Add a model");
	  if(job.model_count==0){img_model_add = gtk_image_new_from_file("Model-Add-Y.gif");} 
	  if(job.model_count >0){img_model_add = gtk_image_new_from_file("Model-Add-G.gif");}
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_add),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_add),img_model_add);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_add,0,0,1,1);
      
	  btn_model_sub = gtk_button_new();
	  g_signal_connect (btn_model_sub, "clicked", G_CALLBACK (model_sub_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_sub), "Remove a model");
	  img_model_sub = gtk_image_new_from_file("Model-Sub-B.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_sub),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_sub),img_model_sub);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_sub,1,0,1,1);
      
	  btn_model_select = gtk_button_new();
	  g_signal_connect (btn_model_select, "clicked", G_CALLBACK (model_select_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_select), "Selects next model");
	  img_model_select = gtk_image_new_from_file("Select.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_select),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_select),img_model_select);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_select,4,0,1,1);
  
	  btn_model_options = gtk_button_new();
	  g_signal_connect (btn_model_options, "clicked", G_CALLBACK (model_options_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_options), "Build options");
	  img_model_options = gtk_image_new_from_file("Options.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_options),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_options),img_model_options);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_options,5,0,1,1);
  
	  btn_model_align = gtk_button_new();
	  g_signal_connect (btn_model_align, "clicked", G_CALLBACK (model_align_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_align), "Align models");
	  img_model_align = gtk_image_new_from_file("Align.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_align),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_align),img_model_align);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_align,6,0,1,1);

	  btn_model_XF = gtk_button_new();
	  g_signal_connect (btn_model_XF, "clicked", G_CALLBACK (model_XF_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_XF), "Rotate about X");
	  img_model_XF = gtk_image_new_from_file("XRot.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_XF),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_XF),img_model_XF);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_XF,7,0,1,1);
	  
	  // a meager attempt to provide both bulk and fine rotation control
	  //GtkGesture *XF_left_click_gesture = gtk_gesture_click_new();
	  //gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(XF_left_click_gesture), 1);	// left mouse btn
	  //g_signal_connect(XF_left_click_gesture, "pressed", G_CALLBACK(model_XF_bulk_callback), NULL);
	  //gtk_widget_add_controller(btn_model_XF, GTK_EVENT_CONTROLLER(XF_left_click_gesture));
	  
	  //GtkGesture *XF_right_click_gesture = gtk_gesture_click_new();
	  //gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(XF_right_click_gesture), 3);	// right mouse btn
	  //g_signal_connect(XF_right_click_gesture, "pressed", G_CALLBACK(model_XF_fine_callback), NULL);
	  //gtk_widget_add_controller(btn_model_XF, GTK_EVENT_CONTROLLER(XF_right_click_gesture));

	  btn_model_YF = gtk_button_new();
	  g_signal_connect (btn_model_YF, "clicked", G_CALLBACK (model_YF_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_YF), "Rotate 90 about Y");
	  img_model_YF = gtk_image_new_from_file("YRot.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_YF),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_YF),img_model_YF);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_YF,8,0,1,1);
  
	  btn_model_ZF = gtk_button_new();
	  g_signal_connect (btn_model_ZF, "clicked", G_CALLBACK (model_ZF_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_ZF), "Rotate 90 about Z");
	  img_model_ZF = gtk_image_new_from_file("ZRot.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_ZF),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_ZF),img_model_ZF);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_ZF,9,0,1,1);

	  btn_model_view = gtk_button_new();
	  g_signal_connect (btn_model_view, "clicked", G_CALLBACK (model_view_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_view), "Ortho views");
	  img_model_view = gtk_image_new_from_file("View.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_view),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_view),img_model_view);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_view,10,0,1,1);

	  // set up model control button
	  btn_model_control = gtk_button_new();
	  g_signal_connect (btn_model_control, "clicked", G_CALLBACK (model_UI), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_model_control), "Assign and slice");
	  img_model_index=0;						// 0=gray, 1=yellow, 2=green, 3=red
	  img_model_control = gtk_image_new_from_file("Model-Ctrl-B.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_model_control),50);
	  gtk_button_set_child(GTK_BUTTON(btn_model_control),img_model_control);
	  gtk_grid_attach(GTK_GRID(grid_model_btn),btn_model_control,11,0,1,1);

	  }

	}

      // set up tool view area
	{
	g_tool_all=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	lbl_status=gtk_label_new("Tool View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_tool_all,lbl_status);

	stat_btm=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	gtk_box_append(GTK_BOX(g_tool_all),stat_btm);
  
	stat_btm_left=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_box_append(GTK_BOX(stat_btm),stat_btm_left);
	stat_btm_right=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_box_append(GTK_BOX(stat_btm),stat_btm_right);
	tool_stat_x=(LCD_WIDTH-170)/2;
	tool_stat_y=(LCD_HEIGHT-235)/2;
	
	GtkWidget *frame_stat_tool[4];
	for(i=0;i<MAX_TOOLS;i++)
	  {
	  sprintf(scratch,"Tool %d",(i+1));
	  frame_stat_tool[i]=gtk_frame_new(scratch);
	  gtk_widget_set_size_request(frame_stat_tool[i],tool_stat_x,tool_stat_y);
	  stat_tool[i]=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_frame_set_child(GTK_FRAME(frame_stat_tool[i]), stat_tool[i]);
	  }
	 
	col_width=24;
	for(i=0;i<MAX_TOOLS;i++)
	  {
	  if(i==0)gtk_box_append(GTK_BOX(stat_btm_left),frame_stat_tool[0]);
	  if(i==1)gtk_box_append(GTK_BOX(stat_btm_left),frame_stat_tool[1]);
	  if(i==2)gtk_box_append(GTK_BOX(stat_btm_right),frame_stat_tool[2]);
	  if(i==3)gtk_box_append(GTK_BOX(stat_btm_right),frame_stat_tool[3]);
	  grid_tool[i]=gtk_grid_new();
	  gtk_box_append(GTK_BOX(stat_tool[i]),grid_tool[i]);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_tool[i]),6);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_tool[i]),12);
	  sprintf(scratch,"               Type:");				// padding with spaces is cheap way of setting column width
	  type_lbl[i]=gtk_label_new(scratch);
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),type_lbl[i],0,0,1,1);
	  gtk_label_set_xalign (GTK_LABEL(type_lbl[i]),1.0);
	  type_desc[i]=gtk_label_new("Empty     ");				// must always be exactly 10 chars long or "Negative content width" warnings
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),type_desc[i],1,0,3,1);
	  gtk_label_set_xalign (GTK_LABEL(type_desc[i]),0.1);
	  material_lbl[i]=gtk_label_new("Material:");
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),material_lbl[i],0,1,1,1);
	  gtk_label_set_xalign (GTK_LABEL(material_lbl[i]),1.0);
	  matl_desc[i]=gtk_label_new("Empty     ");
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),matl_desc[i],1,1,3,1);
	  gtk_label_set_xalign (GTK_LABEL(matl_desc[i]),0.1);
	  mdl_lbl[i]=gtk_label_new("Model:");
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),mdl_lbl[i],0,2,1,1);
	  gtk_label_set_xalign (GTK_LABEL(mdl_lbl[i]),1.0);
	  mdl_desc[i]=gtk_label_new("Empty     ");
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),mdl_desc[i],1,2,3,1);
	  gtk_label_set_xalign (GTK_LABEL(mdl_desc[i]),0.1);
    
	  tstat_lbl=gtk_label_new("State:");
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),tstat_lbl,0,3,1,1);
	  gtk_label_set_xalign (GTK_LABEL(tstat_lbl),1.0);
	  sprintf(scratch,"Empty     ");
	  if(Tool[i].state==TL_UNKNOWN)sprintf(scratch," Unknown ");
	  if(Tool[i].state==TL_LOADED)sprintf(scratch," Loaded ");
	  if(Tool[i].state==TL_READY)sprintf(scratch," Ready ");
	  if(Tool[i].state==TL_ACTIVE)sprintf(scratch," Active ");
	  if(Tool[i].state==TL_RETRACTED)sprintf(scratch," Retracted ");
	  if(Tool[i].state==TL_FAILED)sprintf(scratch," Failed ");
	  tool_stat[i]=gtk_label_new(scratch);
	  gtk_grid_attach(GTK_GRID(grid_tool[i]),tool_stat[i],1,3,1,1);
	  gtk_label_set_xalign (GTK_LABEL(tool_stat[i]),0.1);
    
	  // create material graph for each tool for the main window status frame
	  mlvl_lbl[i]=gtk_label_new("Material:");
	  gtk_grid_attach (GTK_GRID (grid_tool[i]), mlvl_lbl[i], 0, 4, 1, 1);
	  gtk_label_set_xalign (GTK_LABEL(mlvl_lbl[i]),1.0);
	  
	  matl_lvl[i]=gtk_level_bar_new();
	  matl_bar[i]=GTK_LEVEL_BAR(matl_lvl[i]);    
	  gtk_level_bar_set_mode (matl_bar[i],GTK_LEVEL_BAR_MODE_CONTINUOUS);
	  gtk_level_bar_set_min_value(matl_bar[i],0.0);
	  gtk_level_bar_set_max_value(matl_bar[i],100.0);
	  gtk_level_bar_set_value (matl_bar[i],1.0);
	  gtk_level_bar_add_offset_value(matl_bar[i],GTK_LEVEL_BAR_OFFSET_LOW,10.0);
	  gtk_level_bar_add_offset_value(matl_bar[i],GTK_LEVEL_BAR_OFFSET_HIGH,25.0);
	  gtk_level_bar_add_offset_value(matl_bar[i],GTK_LEVEL_BAR_OFFSET_FULL,100.0);
	  gtk_widget_set_size_request(matl_lvl[i],150,15);
	  gtk_grid_attach_next_to (GTK_GRID (grid_tool[i]),matl_lvl[i],mlvl_lbl[i],GTK_POS_RIGHT, 3, 1);
  
	  sprintf(scratch," -- \%");
	  if(strlen(Tool[i].matl.name)>0)sprintf(scratch,"%4.0f \%",(100-(100*Tool[i].matl.mat_used/Tool[i].matl.mat_volume)));
	  matl_lbl[i]=gtk_label_new(scratch);
	  gtk_grid_attach_next_to (GTK_GRID (grid_tool[i]),matl_lbl[i],matl_lvl[i],GTK_POS_RIGHT, 1, 1);
  
	  // create temperature thermometer for each tool for the main window status frame
	  sprintf(scratch,"%6.0f C",Tool[i].thrm.tempC);
	  if(Tool[i].thrm.sensor==0)sprintf(scratch,"-- C");
	  temp_lbl[i]=gtk_label_new(scratch);
	  gtk_grid_attach (GTK_GRID (grid_tool[i]), temp_lbl[i], 0, 5, 1, 1);
	  gtk_label_set_xalign (GTK_LABEL(temp_lbl[i]),0.8);
	  
	  temp_lvl[i]=gtk_level_bar_new();
	  temp_bar[i]=GTK_LEVEL_BAR(temp_lvl[i]);    
	  gtk_level_bar_set_mode (temp_bar[i],GTK_LEVEL_BAR_MODE_CONTINUOUS);
	  gtk_level_bar_set_min_value(temp_bar[i],1.0);
	  gtk_level_bar_set_max_value(temp_bar[i],300.0);
	  gtk_level_bar_set_value (temp_bar[i],1.0);
	  gtk_widget_set_size_request(temp_lvl[i],150,15);
	  gtk_grid_attach_next_to (GTK_GRID (grid_tool[i]),temp_lvl[i],temp_lbl[i],GTK_POS_RIGHT, 3, 1);
	  
	  sprintf(scratch," %6.0f C ",Tool[i].thrm.setpC);
	  if(Tool[i].thrm.sensor==0)sprintf(scratch," -- C ");
	  setp_lbl[i]=gtk_label_new(scratch);
	  gtk_grid_attach_next_to (GTK_GRID (grid_tool[i]),setp_lbl[i],temp_lvl[i],GTK_POS_RIGHT, 1, 1);
	  }
	
	// since some units only use 1 tool there is lots of space for other stats
	// for machines with multiple tools this space is normally used up by the tool info
	if(MAX_TOOLS==1)
	  {
	  sprintf(scratch,"Performance");
	  GtkWidget *frame_perf_tool=gtk_frame_new(scratch);
	  gtk_widget_set_size_request(frame_perf_tool,tool_stat_x,tool_stat_y);
	  gtk_box_append(GTK_BOX(stat_btm_right),frame_perf_tool);
	  GtkWidget *perf_tool=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_frame_set_child(GTK_FRAME(frame_perf_tool), perf_tool);
	  GtkWidget *grid_perf=gtk_grid_new();
	  gtk_box_append(GTK_BOX(perf_tool),grid_perf);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_perf),6);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_perf),12);
	  
	  col_width=20;
	  sprintf(scratch,"   Tip Position ");
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  GtkWidget *lbl_tool_pos=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_tool_pos),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_tool_pos,0,0,1,1);
	  GtkWidget *lbl_pos_x=gtk_label_new("  X: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_pos_x),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_pos_x,0,1,1,1);
	  GtkWidget *lbl_pos_y=gtk_label_new("  Y: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_pos_y),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_pos_y,0,2,1,1);
	  GtkWidget *lbl_pos_z=gtk_label_new("  Z: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_pos_z),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_pos_z,0,3,1,1);
	  GtkWidget *lbl_pos_a=gtk_label_new("  A: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_pos_a),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_pos_a,0,4,1,1);
	  
	  sprintf(scratch,"%6.3f",PostG.x);
	  lbl_val_pos_x=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_pos_x),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_pos_x,1,1,1,1);
	  
	  sprintf(scratch,"%6.3f",PostG.y);
	  lbl_val_pos_y=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_pos_y),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_pos_y,1,2,1,1);
	  
	  sprintf(scratch,"%6.3f",PostG.z);
	  lbl_val_pos_z=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_pos_z),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_pos_z,1,3,1,1);
	  
	  sprintf(scratch,"%6.3f",PostG.a);
	  lbl_val_pos_a=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_pos_a),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_pos_a,1,4,1,1);
	  
	  GtkWidget *perf_separator0=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
	  gtk_grid_attach(GTK_GRID(grid_perf),perf_separator0,0,5,4,1);
  
	  GtkWidget *lbl_speed=gtk_label_new("   Speed (mm/s)");
	  gtk_label_set_xalign (GTK_LABEL(lbl_speed),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_speed,0,6,1,1);
	  GtkWidget *lbl_speed_crt=gtk_label_new("Current: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_speed_crt),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_speed_crt,0,7,1,1);
	  GtkWidget *lbl_speed_avg=gtk_label_new("Avgerage: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_speed_avg),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_speed_avg,0,8,1,1);
	  GtkWidget *lbl_speed_max=gtk_label_new("Maximum: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_speed_max),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_speed_max,0,9,1,1);
  
	  sprintf(scratch,"0.0");
	  lbl_val_spd_crt=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_spd_crt),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_spd_crt,1,7,1,1);
	  sprintf(scratch,"0.0");
	  lbl_val_spd_avg=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_spd_avg),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_spd_avg,1,8,1,1);
	  sprintf(scratch,"0.0");
	  lbl_val_spd_max=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_spd_max),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_spd_max,1,9,1,1);
  
	  GtkWidget *perf_separator1=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
	  gtk_grid_attach(GTK_GRID(grid_perf),perf_separator1,0,10,4,1);
	  
	  GtkWidget *lbl_flowrate=gtk_label_new("   Flow Rate (mm^3/min)");
	  gtk_label_set_xalign (GTK_LABEL(lbl_flowrate),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_flowrate,0,11,1,1);
	  GtkWidget *lbl_flow_crt=gtk_label_new("Current: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_flow_crt),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_flow_crt,0,12,1,1);
	  GtkWidget *lbl_flow_avg=gtk_label_new("Avgerage: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_flow_avg),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_flow_avg,0,13,1,1);
	  GtkWidget *lbl_flow_max=gtk_label_new("Maximum: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_flow_max),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_flow_max,0,14,1,1);
	
	  sprintf(scratch,"0.0");
	  lbl_val_flw_crt=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_flw_crt),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_flw_crt,1,12,1,1);
	  sprintf(scratch,"0.0");
	  lbl_val_flw_avg=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_flw_avg),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_flw_avg,1,13,1,1);
	  sprintf(scratch,"0.0");
	  lbl_val_flw_max=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_flw_max),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_flw_max,1,14,1,1);
	  
	  GtkWidget *perf_separator2=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
	  gtk_grid_attach(GTK_GRID(grid_perf),perf_separator2,0,15,4,1);
	  
	  GtkWidget *lbl_motionctrl=gtk_label_new("   Motion Control");
	  gtk_label_set_xalign (GTK_LABEL(lbl_motionctrl),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_motionctrl,0,16,1,1);
	  GtkWidget *lbl_cmdct=gtk_label_new("Commands pending: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_cmdct),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_cmdct,0,17,1,1);
	  GtkWidget *lbl_buff_used=gtk_label_new("Cmd buffers in use: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_buff_used),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_buff_used,0,18,1,1);
	  GtkWidget *lbl_buff_avail=gtk_label_new("TxR buffers in use: ");
	  gtk_label_set_xalign (GTK_LABEL(lbl_buff_avail),1.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_buff_avail,0,19,1,1);
	
	  sprintf(scratch,"0");
	  lbl_val_cmdct=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_cmdct),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_cmdct,1,17,1,1);
	  lbl_val_buff_used=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_buff_used),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_buff_used,1,18,1,1);
	  //sprintf(scratch,"%d",MAX_BUFFER);
	  sprintf(scratch,"%d",Rx_bytes);
	  lbl_val_buff_avail=gtk_label_new(scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_val_buff_avail),0.0);
	  gtk_grid_attach(GTK_GRID(grid_perf),lbl_val_buff_avail,1,19,1,1);
	  }
      
	// set up build table and environment info boxes below tools for tool view
	{
	sprintf(scratch,"Build Table");
	GtkWidget *frame_stat_bld_tbl=gtk_frame_new(scratch);
	stat_bld_tbl=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	gtk_frame_set_child(GTK_FRAME(frame_stat_bld_tbl), stat_bld_tbl);
	gtk_widget_set_size_request(stat_bld_tbl,tool_stat_x,95);
	gtk_box_append(GTK_BOX(stat_btm_left),frame_stat_bld_tbl);
	grid_bldtbl=gtk_grid_new();
	gtk_box_append(GTK_BOX(stat_bld_tbl),grid_bldtbl);
	gtk_grid_set_row_spacing (GTK_GRID(grid_bldtbl),6);
	gtk_grid_set_column_spacing (GTK_GRID(grid_bldtbl),12);
	type_bldtbl=gtk_label_new(" Status:");
	gtk_label_set_xalign (GTK_LABEL(type_bldtbl),1.0);
	gtk_grid_attach(GTK_GRID(grid_bldtbl),type_bldtbl,0,1,1,1);
	stat_bldtbl=gtk_label_new(" Initializing ");
	gtk_label_set_xalign (GTK_LABEL(stat_bldtbl),0.0);
	gtk_grid_attach(GTK_GRID(grid_bldtbl),stat_bldtbl,1,1,3,1);
	sprintf(scratch,"--- C");
	lbl_bldtbl_tempC=gtk_label_new(scratch);
	gtk_grid_attach(GTK_GRID(grid_bldtbl),lbl_bldtbl_tempC,0,2,1,1);
	temp_lvl_bldtbl=gtk_level_bar_new();
	temp_bar_bldtbl=GTK_LEVEL_BAR(temp_lvl_bldtbl);    
	gtk_level_bar_set_mode (temp_bar_bldtbl,GTK_LEVEL_BAR_MODE_CONTINUOUS);
	gtk_level_bar_set_min_value(temp_bar_bldtbl,0.0);
	gtk_level_bar_set_max_value(temp_bar_bldtbl,150.0);
	gtk_level_bar_set_value (temp_bar_bldtbl,1.0);
	gtk_widget_set_size_request(temp_lvl_bldtbl,150,15);
	gtk_grid_attach_next_to (GTK_GRID (grid_bldtbl),temp_lvl_bldtbl,lbl_bldtbl_tempC,GTK_POS_RIGHT, 3, 1);
	sprintf(scratch,"%6.0f C",Tool[BLD_TBL1].thrm.setpC);
	lbl_bldtbl_setpC=gtk_label_new(scratch);
	gtk_grid_attach_next_to (GTK_GRID (grid_bldtbl),lbl_bldtbl_setpC,temp_lvl_bldtbl,GTK_POS_RIGHT, 1, 1);
	
	sprintf(scratch,"Chamber");
	GtkWidget *frame_stat_chamber=gtk_frame_new(scratch);
	stat_chamber=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	gtk_frame_set_child(GTK_FRAME(frame_stat_chamber), stat_chamber);
	gtk_widget_set_size_request(stat_chamber,tool_stat_x,95);
	#ifdef ALPHA_UNIT
	  gtk_box_append(GTK_BOX(stat_btm_right),frame_stat_chamber);
	#endif
	#ifdef BETA_UNIT
	  gtk_box_append(GTK_BOX(stat_btm_right),frame_stat_chamber);
	#endif
	#ifdef GAMMA_UNIT
	  gtk_box_append(GTK_BOX(stat_btm_left),frame_stat_chamber);
	#endif
	#ifdef DELTA_UNIT
	  gtk_box_append(GTK_BOX(stat_btm_left),frame_stat_chamber);
	#endif
	grid_chamber=gtk_grid_new();
	gtk_box_append(GTK_BOX(stat_chamber),grid_chamber);
	gtk_grid_set_row_spacing (GTK_GRID(grid_chamber),6);
	gtk_grid_set_column_spacing (GTK_GRID(grid_chamber),12);
	type_chamber=gtk_label_new(" Status:");
	gtk_label_set_xalign (GTK_LABEL(type_chamber),1.0);
	gtk_grid_attach(GTK_GRID(grid_chamber),type_chamber,0,1,1,1);
	stat_chm_lbl=gtk_label_new(" Initializing ");
	gtk_label_set_xalign (GTK_LABEL(stat_chm_lbl),0.0);
	gtk_grid_attach(GTK_GRID(grid_chamber),stat_chm_lbl,1,1,3,1);
	sprintf(scratch,"--- C");
	lbl_chmbr_tempC=gtk_label_new(scratch);
	gtk_grid_attach(GTK_GRID(grid_chamber),lbl_chmbr_tempC,0,2,1,1);
	temp_lvl_chamber=gtk_level_bar_new();
	temp_bar_chamber=GTK_LEVEL_BAR(temp_lvl_chamber);    
	gtk_level_bar_set_mode (temp_bar_chamber,GTK_LEVEL_BAR_MODE_CONTINUOUS);
	gtk_level_bar_set_min_value(temp_bar_chamber,0.0);
	gtk_level_bar_set_max_value(temp_bar_chamber,100.0);
	gtk_level_bar_set_value (temp_bar_chamber,1.0);
	gtk_widget_set_size_request(temp_lvl_chamber,100,15);
	gtk_grid_attach_next_to (GTK_GRID (grid_chamber),temp_lvl_chamber,lbl_chmbr_tempC,GTK_POS_RIGHT, 3, 1);
	sprintf(scratch,"%6.0f C",Tool[CHAMBER].thrm.setpC);
	lbl_chmbr_setpC=gtk_label_new(scratch);
	gtk_grid_attach_next_to (GTK_GRID (grid_chamber),lbl_chmbr_setpC,temp_lvl_chamber,GTK_POS_RIGHT, 1, 1);
	}

	// set up tool button area
	  {
	  g_tool_btn=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_widget_set_size_request(g_tool_btn,LCD_WIDTH-170,50);
	  gtk_widget_set_hexpand(g_tool_btn,TRUE);
	  gtk_widget_set_vexpand(g_tool_btn,FALSE);
	  gtk_box_append(GTK_BOX(g_tool_all),g_tool_btn);
	  
	  grid_tool_btn=gtk_grid_new();
	  gtk_box_append(GTK_BOX(g_tool_btn),grid_tool_btn);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_tool_btn),1);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_tool_btn),1);
    
	  // set up TOOL button operations
	  img_tool[0] = gtk_image_new_from_file("ToolChg.GIF");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool[0]),50);
	  img_tool_index[0]=0;
	  
	  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	    img_tool[1] = gtk_image_new_from_file("Tool-2-B.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_tool[1]),50);
	    img_tool_index[1]=0;
	    
	    img_tool[2] = gtk_image_new_from_file("Tool-3-B.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_tool[2]),50);
	    img_tool_index[2]=0;
	    
	    img_tool[3] = gtk_image_new_from_file("Tool-4-B.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_tool[3]),50);
	    img_tool_index[3]=0;
	  #endif
	  
	  for(i=0;i<MAX_TOOLS;i++)
	    {
	    btn_tool[i] = gtk_button_new();
	    g_signal_connect (btn_tool[i], "clicked", G_CALLBACK (on_tool_change), GINT_TO_POINTER(i));
	    sprintf(scratch,"Tool change");
	    gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tool[i]), scratch);
	    gtk_widget_set_size_request(btn_tool[i],80,50);
	    gtk_button_set_child(GTK_BUTTON(btn_tool[i]),img_tool[i]);
	    gtk_grid_attach(GTK_GRID(grid_tool_btn),btn_tool[i],i,0,1,1);
	    }
      
	  // set up button separator
	  btn_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
	  gtk_box_append(GTK_BOX(g_tool_btn),btn_separator);
	  
	  btn_tool_ctrl = gtk_button_new();
	  g_signal_connect (btn_tool_ctrl, "clicked", G_CALLBACK (tool_UI),GINT_TO_POINTER(0));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tool_ctrl), "Tool & material details");
	  img_tool_ctrl = gtk_image_new_from_file("Tool_Info.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_ctrl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_ctrl),img_tool_ctrl);
	  gtk_grid_attach(GTK_GRID(grid_tool_btn),btn_tool_ctrl,5,0,1,1);

	  // short cut to advance material if tool is loaded
	  // the image for this button is changed upon tool detection/activation in idle loop
	  btn_tool_feed_fwd = gtk_button_new();
	  g_signal_connect (btn_tool_feed_fwd, "clicked", G_CALLBACK (tool_add_matl),GINT_TO_POINTER(0));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tool_feed_fwd), "Feed material forward");
	  img_tool_feed_fwd = gtk_image_new_from_file("Matl-Add-Generic-B.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_feed_fwd),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_feed_fwd),img_tool_feed_fwd);
	  gtk_grid_attach(GTK_GRID(grid_tool_btn),btn_tool_feed_fwd,6,0,1,1);

	  // short cut to retract material if tool is loaded
	  // the image for this button is changed upon tool detection/activation in idle loop
	  btn_tool_feed_rvs = gtk_button_new();
	  g_signal_connect (btn_tool_feed_rvs, "clicked", G_CALLBACK (tool_sub_matl),GINT_TO_POINTER(0));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tool_feed_rvs), "Feed material backward");
	  img_tool_feed_rvs = gtk_image_new_from_file("Matl-Sub-Generic-B.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_tool_feed_rvs),50);
	  gtk_button_set_child(GTK_BUTTON(btn_tool_feed_rvs),img_tool_feed_rvs);
	  gtk_grid_attach(GTK_GRID(grid_tool_btn),btn_tool_feed_rvs,7,0,1,1);

	  // init these to buttons as NOT visible until a tool gets loaded (hadled by idle loop)
	  gtk_widget_set_visible(btn_tool_feed_fwd,FALSE);
	  gtk_widget_set_visible(btn_tool_feed_rvs,FALSE);
	  }
	}

      // set up layer view area
	{
	g_layer_all=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	//gtk_widget_set_size_request(g_layer_all,LCD_WIDTH-170,LCD_HEIGHT-180);
	lbl_draw=gtk_label_new("Slice View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_layer_all,lbl_draw);

	g_layer_area = gtk_drawing_area_new ();
	gtk_box_append(GTK_BOX(g_layer_all),g_layer_area);
	gtk_widget_set_size_request(g_layer_area,LCD_WIDTH-170,LCD_HEIGHT-200);
	gtk_widget_set_hexpand(g_layer_area,TRUE);
	gtk_widget_set_vexpand(g_layer_area,TRUE);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(g_layer_area), (GtkDrawingAreaDrawFunc) layer_draw_callback, g_layer_area, NULL);

	GtkGesture *layer_left_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (layer_left_click_gesture), 1);
	g_signal_connect(layer_left_click_gesture, "pressed", G_CALLBACK(on_layer_area_left_button_press_event),  g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_left_click_gesture));

	GtkGesture *layer_middle_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (layer_middle_click_gesture), 2);
	g_signal_connect(layer_middle_click_gesture, "pressed", G_CALLBACK(on_layer_area_middle_button_press_event),  g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_middle_click_gesture));

	GtkGesture *layer_right_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (layer_right_click_gesture), 3);
	g_signal_connect(layer_right_click_gesture, "pressed", G_CALLBACK(on_layer_area_right_button_press_event),  g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_right_click_gesture));

	GtkGesture *layer_left_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (layer_left_drag_gesture), 1);
	g_signal_connect(layer_left_drag_gesture, "drag-begin", G_CALLBACK(on_layer_area_start_motion_press_event), g_layer_area);
	g_signal_connect(layer_left_drag_gesture, "drag-update", G_CALLBACK(on_layer_area_left_update_motion_press_event), g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_left_drag_gesture));
	
	GtkGesture *layer_right_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (layer_right_drag_gesture), 3);
	g_signal_connect(layer_right_drag_gesture, "drag-begin", G_CALLBACK(on_layer_area_start_motion_press_event), g_layer_area);
	g_signal_connect(layer_right_drag_gesture, "drag-update", G_CALLBACK(on_layer_area_right_update_motion_press_event), g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_right_drag_gesture));
	
	// the purpose of this controller is to track the location of the cursor within the layer drawing area
	GtkEventController *layer_pointer_ctlr = gtk_event_controller_motion_new();
	g_signal_connect(layer_pointer_ctlr, "motion", G_CALLBACK(on_layer_area_motion_event), g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_pointer_ctlr));	

	// the purpose of this controller is to zoom in/out of the layer drawing area
	GtkEventController *layer_scroll_ctlr = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
	g_signal_connect(layer_scroll_ctlr, "scroll", G_CALLBACK(on_layer_area_scroll_event), g_layer_area);
	gtk_widget_add_controller(g_layer_area, GTK_EVENT_CONTROLLER(layer_scroll_ctlr));	

	// set up layer button area
	  {
	  g_layer_btn=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_widget_set_size_request(g_layer_btn,LCD_WIDTH-170,50);
	  gtk_widget_set_hexpand(g_layer_btn,TRUE);
	  gtk_widget_set_vexpand(g_layer_btn,FALSE);
	  gtk_box_append(GTK_BOX(g_layer_all),g_layer_btn);
	  grid_layer_btn=gtk_grid_new();
	  gtk_box_append(GTK_BOX(g_layer_btn),grid_layer_btn);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_layer_btn),1);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_layer_btn),1);
    
	  btn_layer_count = gtk_button_new();
	  g_signal_connect (btn_layer_count, "clicked", G_CALLBACK (set_layer_heights_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_layer_count), "Set layers to view");
	  img_layer_count = gtk_image_new_from_file("Slice-Ctrl-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_layer_count),50);
	  gtk_button_set_child(GTK_BUTTON(btn_layer_count),img_layer_count);
	  gtk_grid_attach(GTK_GRID(grid_layer_btn),btn_layer_count,0,0,1,1);
	  
	  btn_layer_eol = gtk_button_new();
	  g_signal_connect (btn_layer_eol, "clicked", G_CALLBACK (set_layer_pause_callback), NULL);
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_layer_eol), "Pause build at this layer");
	  img_layer_eol = gtk_image_new_from_file("Slice-Pausel-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_layer_eol),50);
	  gtk_button_set_child(GTK_BUTTON(btn_layer_eol),img_layer_eol);
	  gtk_grid_attach(GTK_GRID(grid_layer_btn),btn_layer_eol,1,0,1,1);
	  
	  btn_merge_poly = gtk_button_new();
	  g_signal_connect (btn_merge_poly, "clicked", G_CALLBACK (merge_polygon_callback),GINT_TO_POINTER(ACTION_ADD));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_merge_poly), "Merge polygons together");
	  img_merge_poly = gtk_image_new_from_file("Tip-Bit-Add-G.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_merge_poly),50);
	  gtk_button_set_child(GTK_BUTTON(btn_merge_poly),img_merge_poly);
	  gtk_grid_attach(GTK_GRID(grid_layer_btn),btn_merge_poly,2,0,1,1);
	  
	  btn_poly_attr = gtk_button_new();
	  g_signal_connect (btn_poly_attr, "clicked", G_CALLBACK (set_polygon_attr_callback),GINT_TO_POINTER(ACTION_ADD));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_poly_attr), "Set polygon attributes");
	  img_poly_attr = gtk_image_new_from_file("PolyEdit.GIF");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_poly_attr),50);
	  gtk_button_set_child(GTK_BUTTON(btn_poly_attr),img_poly_attr);
	  gtk_grid_attach(GTK_GRID(grid_layer_btn),btn_poly_attr,3,0,1,1);
	  }
	  
	}

      // set up graph view area
	{
	g_graph_all=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	//gtk_widget_set_size_request(g_graph_all,LCD_WIDTH-170,LCD_HEIGHT-180);
	lbl_graph=gtk_label_new("Graph View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_graph_all,lbl_graph);

	g_graph_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request(g_graph_area,LCD_WIDTH-170,LCD_HEIGHT-200);
	gtk_widget_set_hexpand(g_graph_area,TRUE);
	gtk_widget_set_vexpand(g_graph_area,TRUE);
	gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA(g_graph_area), (GtkDrawingAreaDrawFunc) graph_draw_callback, g_graph_area, NULL);
	gtk_box_append(GTK_BOX(g_graph_all),g_graph_area);

	GtkGesture *graph_left_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (graph_left_click_gesture), 1);
	g_signal_connect(graph_left_click_gesture, "pressed", G_CALLBACK(on_graph_area_left_button_press_event),  g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_left_click_gesture));

	GtkGesture *graph_middle_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (graph_middle_click_gesture), 2);
	g_signal_connect(graph_middle_click_gesture, "pressed", G_CALLBACK(on_graph_area_middle_button_press_event),  g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_middle_click_gesture));

	GtkGesture *graph_right_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (graph_right_click_gesture), 3);
	g_signal_connect(graph_right_click_gesture, "pressed", G_CALLBACK(on_graph_area_right_button_press_event),  g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_right_click_gesture));

	GtkGesture *graph_left_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (graph_left_drag_gesture), 1);
	g_signal_connect(graph_left_drag_gesture, "drag-begin", G_CALLBACK(on_graph_area_start_motion_press_event), g_graph_area);
	g_signal_connect(graph_left_drag_gesture, "drag-update", G_CALLBACK(on_graph_area_left_update_motion_press_event), g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_left_drag_gesture));
	
	GtkGesture *graph_right_drag_gesture = gtk_gesture_drag_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (graph_right_drag_gesture), 3);
	g_signal_connect(graph_right_drag_gesture, "drag-begin", G_CALLBACK(on_graph_area_start_motion_press_event), g_graph_area);
	g_signal_connect(graph_right_drag_gesture, "drag-update", G_CALLBACK(on_graph_area_right_update_motion_press_event), g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_right_drag_gesture));
	
	// the purpose of this controller is to track the location of the cursor within the graph drawing area
	GtkEventController *graph_pointer_ctlr = gtk_event_controller_motion_new();
	g_signal_connect(graph_pointer_ctlr, "motion", G_CALLBACK(on_graph_area_motion_event), g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_pointer_ctlr));	

	// the purpose of this controller is to zoom in/out of the graph drawing area
	GtkEventController *graph_scroll_ctlr = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
	g_signal_connect(graph_scroll_ctlr, "scroll", G_CALLBACK(on_graph_area_scroll_event), g_graph_area);
	gtk_widget_add_controller(g_graph_area, GTK_EVENT_CONTROLLER(graph_scroll_ctlr));	
	
	// set up graph button area
	  {
	  g_graph_btn=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	  gtk_widget_set_size_request(g_graph_btn,LCD_WIDTH-170,50);
	  gtk_widget_set_hexpand(g_graph_btn,TRUE);
	  gtk_widget_set_vexpand(g_graph_btn,FALSE);
	  gtk_box_append(GTK_BOX(g_graph_all),g_graph_btn);
	  grid_graph_btn=gtk_grid_new();
	  gtk_box_append(GTK_BOX(g_graph_btn),grid_graph_btn);
	  gtk_grid_set_row_spacing (GTK_GRID(grid_graph_btn),1);
	  gtk_grid_set_column_spacing (GTK_GRID(grid_graph_btn),1);
  
	  btn_graph_temp = gtk_button_new();
	  g_signal_connect (btn_graph_temp, "clicked", G_CALLBACK (set_history_type_callback), GINT_TO_POINTER(HIST_TEMP));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_graph_temp), "Thermal History");
	  img_graph_temp = gtk_image_new_from_file("Temperature.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_graph_temp),50);
	  gtk_button_set_child(GTK_BUTTON(btn_graph_temp),img_graph_temp);
	  gtk_grid_attach(GTK_GRID(grid_graph_btn),btn_graph_temp,0,0,1,1);
	  
	  btn_graph_time = gtk_button_new();
	  g_signal_connect (btn_graph_time, "clicked", G_CALLBACK (set_history_type_callback), GINT_TO_POINTER(HIST_TIME));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_graph_time), "Time History");
	  img_graph_time = gtk_image_new_from_file("Time.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_graph_time),50);
	  gtk_button_set_child(GTK_BUTTON(btn_graph_time),img_graph_time);
	  gtk_grid_attach(GTK_GRID(grid_graph_btn),btn_graph_time,1,0,1,1);
	  
	  btn_graph_matl = gtk_button_new();
	  g_signal_connect (btn_graph_matl, "clicked", G_CALLBACK (set_history_type_callback), GINT_TO_POINTER(HIST_MATL));
	  gtk_widget_set_tooltip_text (GTK_WIDGET (btn_graph_matl), "Material History");
	  img_graph_matl = gtk_image_new_from_file("Material.gif");
	  gtk_image_set_pixel_size(GTK_IMAGE(img_graph_matl),50);
	  gtk_button_set_child(GTK_BUTTON(btn_graph_matl),img_graph_matl);
	  gtk_grid_attach(GTK_GRID(grid_graph_btn),btn_graph_matl,2,0,1,1);
	  }
	  
	}
 
      // set up camera view area
      if(UNIT_HAS_CAMERA==TRUE)
	{
	g_camera_all=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	lbl_camera=gtk_label_new("Camera View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_camera_all,lbl_camera);

	g_camera_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request(g_camera_area,LCD_WIDTH-170,LCD_HEIGHT-200);
	gtk_widget_set_hexpand(g_camera_area,TRUE);
	gtk_widget_set_vexpand(g_camera_area,TRUE);
	gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(g_camera_area), (GtkDrawingAreaDrawFunc) camera_callback, g_camera_area, NULL);
	gtk_box_append(GTK_BOX(g_camera_all),g_camera_area);
	
	GtkGesture *camera_left_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (camera_left_click_gesture), 1);
	g_signal_connect(camera_left_click_gesture, "pressed", G_CALLBACK(on_camera_area_left_button_press_event),  g_camera_area);
	gtk_widget_add_controller(g_camera_area, GTK_EVENT_CONTROLLER(camera_left_click_gesture));

	GtkGesture *camera_middle_click_gesture = gtk_gesture_click_new();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (camera_middle_click_gesture), 2);
	g_signal_connect(camera_middle_click_gesture, "pressed", G_CALLBACK(on_camera_area_middle_button_press_event),  g_camera_area);
	gtk_widget_add_controller(g_camera_area, GTK_EVENT_CONTROLLER(camera_middle_click_gesture));

	// the purpose of this controller is to track the location of the cursor within the camera drawing area
	GtkEventController *camera_pointer_ctlr = gtk_event_controller_motion_new();
	g_signal_connect(camera_pointer_ctlr, "motion", G_CALLBACK(on_camera_area_motion_event), g_camera_area);
	gtk_widget_add_controller(g_camera_area, GTK_EVENT_CONTROLLER(camera_pointer_ctlr));	

	// the purpose of this controller is to zoom in/out of the camera drawing area
	GtkEventController *camera_scroll_ctlr = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
	g_signal_connect(camera_scroll_ctlr, "scroll", G_CALLBACK(on_camera_area_scroll_event), g_camera_area);
	gtk_widget_add_controller(g_camera_area, GTK_EVENT_CONTROLLER(camera_scroll_ctlr));	

	cam_roi_x0=0.0; cam_roi_y0=0.0;
	cam_roi_x1=1.0; cam_roi_y1=1.0;
	strcpy(bashfn,"#!/bin/bash\nsudo libcamera-jpeg -v 0 -n 1 -t 10 ");	// define base camera command
	strcat(bashfn,"-o /home/aa/Documents/4X3D/still-test.jpg ");		// ... add the target file name
	strcat(bashfn,"--width 830 --height 574 ");				// ... add the image size
	sprintf(cam_scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
	strcat(bashfn,cam_scratch);						// ... add region of interest (zoom)
	strcat(bashfn,"--autofocus-mode continuous ");				// ... add focus control
	system(bashfn);
	
	g_camera_btn=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
	gtk_widget_set_size_request(g_camera_btn,LCD_WIDTH-170,50);
	gtk_widget_set_hexpand(g_camera_btn,TRUE);
	gtk_widget_set_vexpand(g_camera_btn,FALSE);
	gtk_box_append(GTK_BOX(g_camera_all),g_camera_btn);
	grid_camera_btn=gtk_grid_new();
	gtk_box_append(GTK_BOX(g_camera_btn),grid_camera_btn);
	gtk_grid_set_row_spacing (GTK_GRID(grid_camera_btn),1);
	gtk_grid_set_column_spacing (GTK_GRID(grid_camera_btn),1);

	// set up camera live view button
	btn_camera_liveview = gtk_button_new();
	g_signal_connect (btn_camera_liveview, "clicked", G_CALLBACK (set_image_mode_callback), GINT_TO_POINTER(CAM_LIVEVIEW));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_liveview), "Show live view");
	img_camera_liveview = gtk_image_new_from_file("Live_View_On.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_liveview),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_liveview),img_camera_liveview);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_liveview,0,0,1,1);

	// set up camera time lapse of build on/off button
	btn_camera_job_timelapse = gtk_button_new();
	g_signal_connect (btn_camera_job_timelapse, "clicked", G_CALLBACK (set_image_mode_callback), GINT_TO_POINTER(CAM_JOB_LAPSE));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_job_timelapse), "Show job time lapse");
	img_camera_job_timelapse = gtk_image_new_from_file("Time_Lapse_Off.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_job_timelapse),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_job_timelapse),img_camera_job_timelapse);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_job_timelapse,1,0,1,1);
	
	// set up camera time lapse table scan on/off button
	btn_camera_scan_timelapse = gtk_button_new();
	g_signal_connect (btn_camera_scan_timelapse, "clicked", G_CALLBACK (set_image_mode_callback), GINT_TO_POINTER(CAM_SCAN_LAPSE));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_scan_timelapse), "Show scan time lapse");
	img_camera_scan_timelapse = gtk_image_new_from_file("Scan_Lapse_Off.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_scan_timelapse),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_scan_timelapse),img_camera_scan_timelapse);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_scan_timelapse,2,0,1,1);
	
	// set up camera snapshot button
	btn_camera_snapshot = gtk_button_new();
	g_signal_connect (btn_camera_snapshot, "clicked", G_CALLBACK (set_image_mode_callback), GINT_TO_POINTER(CAM_SNAPSHOT));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_snapshot), "Take & save picture");
	img_camera_snapshot = gtk_image_new_from_file("Snapshot.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_snapshot),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_snapshot),img_camera_snapshot);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_snapshot,3,0,1,1);

	// set up image difference button for debug
	GtkWidget *btn_image_diff;
	btn_image_diff = gtk_button_new_with_label("Table Scan");
	g_signal_connect (btn_image_diff, "clicked", G_CALLBACK (set_image_mode_callback), GINT_TO_POINTER(CAM_IMG_SCAN));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_image_diff), "Scan the build table for objects");
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_image_diff,4,0,1,1);

/*
	btn_camera_time = gtk_button_new();
	g_signal_connect (btn_camera_time, "clicked", G_CALLBACK (set_history_type_callback), GINT_TO_POINTER(HIST_TIME));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_time), "Time History");
	img_camera_time = gtk_image_new_from_file("Time.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_time),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_time),img_camera_time);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_time,1,0,1,1);
	
	btn_camera_matl = gtk_button_new();
	g_signal_connect (btn_camera_matl, "clicked", G_CALLBACK (set_history_type_callback), GINT_TO_POINTER(HIST_MATL));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_camera_matl), "Thermal History");
	img_camera_matl = gtk_image_new_from_file("Material.gif");
	gtk_image_set_pixel_size(GTK_IMAGE(img_camera_matl),50);
	gtk_button_set_child(GTK_BUTTON(btn_camera_matl),img_camera_matl);
	gtk_grid_attach(GTK_GRID(grid_camera_btn),btn_camera_matl,2,0,1,1);
*/
        }
      
      
      // set up vulkan view area
/*	{
	  
	create_instance();
	create_device();
	create_command_pool();
	//create_render_pass();
	  
	g_vulkan_area = gtk_drawing_area_new ();
	gtk_widget_set_size_request(g_graph_area,LCD_WIDTH-170,LCD_HEIGHT-200);
	
	lbl_vulkan=gtk_label_new("Vulkan View");
	gtk_notebook_append_page (GTK_NOTEBOOK(g_notebook),g_vulkan_area,lbl_vulkan);
*/
	/*
	// Create a Vulkan instance
	  {
	  VkResult res;
	  VkInstanceCreateInfo info = {};
	  info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
      
	  VkInstance instance;
	  res = vkCreateInstance(&info, NULL, &instance);
	  if(res==VK_SUCCESS) 
	    {
	    // Get the Vulkan instance information
	    uint32_t extensionCount = 0;
	    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	    VkExtensionProperties *extensions = malloc(sizeof(VkExtensionProperties) * extensionCount);
	    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
	
	    // Display the Vulkan instance information in the label widget
	    printf("\n\nVulkan instance created successfully.\n");
	    printf("  Instance Extensions:  qty=%d \n",extensionCount);
	    for(i=0;i<extensionCount;i++) 
	      {
	      printf("  %d %s \n",i,extensions[i].extensionName);
	      }

	    // Select physical device
	    uint32_t physical_device_count = 0;
	    vkEnumeratePhysicalDevices(instance, &physical_device_count, NULL);		// get the count
	    VkPhysicalDevice physical_devices[physical_device_count];			// create the array
	    VkPhysicalDevice chosen_physical_device = VK_NULL_HANDLE;			// default device to NULL
	    VkPhysicalDeviceProperties properties;					// create variable to hold properties
	    printf("\n\n Supported Vulkan Devices:  qty=%d \n",physical_device_count);
	    //for (i=0;i<physical_device_count;i++) 
	    //  {
	    //  vkGetPhysicalDeviceProperties(physical_devices[i], &properties);
	    //  printf("  Device Name: %s \n",&properties.deviceName);
	    //  printf("  API Version: %s.%s.%s \n",VK_VERSION_MAJOR(properties.apiVersion),VK_VERSION_MINOR(properties.apiVersion),VK_VERSION_PATCH(properties.apiVersion));
	    //  }

	    // create a logical device
	    VkDevice device;
	    VkDeviceCreateInfo createInfo = {};
	    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	    
	    // Specify the physical device to use
	    createInfo.pNext = NULL;
	    createInfo.queueCreateInfoCount = 1;
	    createInfo.pQueueCreateInfos = &queueCreateInfo;
	    
	    // Specify device features to enable
	    createInfo.pEnabledFeatures = &deviceFeatures;
	    
	    // Specify the device extensions to enable
	    createInfo.enabledExtensionCount = deviceExtensionCount;
	    createInfo.ppEnabledExtensionNames = deviceExtensionNames;
	    
	    // Specify validation layers to enable (optional)
	    createInfo.enabledLayerCount = validationLayerCount;
	    createInfo.ppEnabledLayerNames = validationLayerNames;
	    
	    // Create the logical device
	    if (vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) 
	      {
	      printf("Error: Failed to create logical device!\n");
	      exit(EXIT_FAILURE);
	      }
	  

	    // Clean up
	    vkDestroyInstance(instance, NULL);
	    free(extensions);
	    
	    }
	  else 
	    {
	    printf("Failed to create Vulkan instance.");
	    }
	  }
	*/
/*
	}
*/	
      }


    // set up a vbox on right side of screen to encapsulate slice level control
    {
      vboxright = gtk_box_new(GTK_ORIENTATION_VERTICAL,3);
      gtk_box_append(GTK_BOX(hbox),vboxright);
      gtk_widget_set_size_request(vboxright,30,LCD_HEIGHT-65);
      gtk_widget_set_hexpand(vboxright,FALSE);
      gtk_widget_set_vexpand(vboxright,TRUE);
  
      // add in slice level slider bar
      g_adj_z_level = gtk_adjustment_new (0.20, 0.00, 10.00, 0.20, 0.20, 1.00);
      g_signal_connect(g_adj_z_level, "value-changed", G_CALLBACK(on_adj_z_value_changed), g_adj_z_level);
      g_adj_z_scroll = gtk_scale_new (GTK_ORIENTATION_VERTICAL, g_adj_z_level);
      gtk_scale_set_draw_value(GTK_SCALE(g_adj_z_scroll), TRUE);
      gtk_scale_set_digits(GTK_SCALE(g_adj_z_scroll), 2);
      gtk_widget_set_vexpand (g_adj_z_scroll, TRUE);
      gtk_range_set_inverted(GTK_RANGE(g_adj_z_scroll), TRUE);
      gtk_box_append(GTK_BOX(vboxright),g_adj_z_scroll);
  
      // add in "up" slice level button
      btn_z_up = gtk_button_new_with_label ("+ ");
      g_signal_connect (btn_z_up, "clicked", G_CALLBACK (z_up_callback), g_adj_z_level);
      gtk_box_append(GTK_BOX(vboxright),btn_z_up);
  
      // add in "down" slice level button
      btn_z_dn = gtk_button_new_with_label (" -");
      g_signal_connect (btn_z_dn, "clicked", G_CALLBACK (z_dn_callback), g_adj_z_level);
      gtk_box_append(GTK_BOX(vboxright),btn_z_dn);
    }

    // win_stat set up - generic window for messaging
    // this window will be called by various functions so it needs to be set-up right up front
    // it can be used by making it visible, posting information, then hiding it when done
    {
    /*
      win_stat = gtk_window_new ();
      gtk_window_set_transient_for (GTK_WINDOW(win_stat), GTK_WINDOW(win_main));
      gtk_window_set_default_size(GTK_WINDOW(win_stat),420,250);		
      gtk_window_set_resizable(GTK_WINDOW(win_stat),FALSE);
      gtk_window_set_title(GTK_WINDOW(win_stat),"Nx3D Message");			
      grd_stat = gtk_grid_new();
      gtk_window_set_child(GTK_WINDOW(win_stat),grd_stat);
      gtk_grid_set_column_homogeneous(GTK_GRID(grd_stat), TRUE);		// set columns to same
      sprintf(scratch,"     ");
      GtkWidget *lbl_vspacer=gtk_label_new(scratch);
      gtk_grid_attach(GTK_GRID(grd_stat),lbl_vspacer,0,0,1,1);
      lbl_stat1 = gtk_label_new (scratch);
      gtk_grid_attach(GTK_GRID(grd_stat),lbl_stat1,0,1,1,1);
      lbl_stat2 = gtk_label_new (scratch);
      gtk_grid_attach(GTK_GRID(grd_stat),lbl_stat2,0,2,1,1);
    */
    }

    // win_info set up - generic global window with message AND progress bar widgets.
    // this window can be called by various functions.  use it by setting its parent, then
    // making it visible, posting information, then hiding it when done.
    {
      win_info = gtk_window_new ();
      gtk_window_set_transient_for (GTK_WINDOW(win_info), GTK_WINDOW(win_main));
      gtk_window_set_default_size(GTK_WINDOW(win_info),450,150);	
      gtk_window_set_resizable(GTK_WINDOW(win_info),FALSE);	
      sprintf(scratch,"Nx3D Progress");
      gtk_window_set_title(GTK_WINDOW(win_info),scratch);			
      grid_info = gtk_grid_new ();				
      gtk_grid_set_row_spacing (GTK_GRID(grid_info),10);
      gtk_window_set_child(GTK_WINDOW(win_info),grid_info);
      
      GtkWidget *lbl_spacer0=gtk_label_new("        ");			// adds space above text
      gtk_grid_attach (GTK_GRID (grid_info), lbl_spacer0, 0, 0, 1, 1);

      sprintf(scratch," ");
      lbl_info1 = gtk_label_new (scratch);				// modify this label for title
      gtk_grid_attach (GTK_GRID (grid_info), lbl_info1, 0, 1, 3, 1);	
      
      lbl_info2 = gtk_label_new (scratch);				// modify this label for description
      gtk_grid_attach (GTK_GRID (grid_info), lbl_info2, 0, 2, 3, 1);

      GtkWidget *lbl_spacer1=gtk_label_new("         ");		// adds space to left of progress bar
      gtk_grid_attach (GTK_GRID (grid_info), lbl_spacer1, 0, 4, 1, 1);

      info_progress = gtk_progress_bar_new ();
      gtk_widget_set_size_request(info_progress, 300, 25);
      gtk_grid_attach (GTK_GRID (grid_info), info_progress, 1, 4, 1, 1);
      gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR(info_progress), TRUE);

      GtkWidget *lbl_spacer2=gtk_label_new("  ");			// adds space below progress bar
      gtk_grid_attach (GTK_GRID (grid_info), lbl_spacer2, 2, 4, 1, 1);

      gtk_widget_set_visible(win_info,FALSE);
    }

    
    // display window with all widgets now generated
    gtk_window_present (GTK_WINDOW(win_main));
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

    // check proper init of system to ensure operation
    if(init_system()!=1)
      {
      printf("Init Failed - Shutting down!\n\n");
      unit_in_error=TRUE;
      destroy(win_main,NULL);
      }

    // this basically adds polling of machine input sources once a second
    g_timeout_add(500,(GSourceFunc)on_idle_status_poll,hbox);
  
    //gtk_main ();

    return;
}


int main (int argc, char *argv[])
{
    int app_status,result,i;

    pthread_attr_t 	tattr;
    int 		newprio;
    struct sched_param 	param;
    pthread_attr_t 	STLtattr;
    int 		STLnewprio;
    struct sched_param 	STLparam;
  
    const rlim_t kStackSize = 16L * 1024L;   				// min stack size = 16 Mb
    struct rlimit rl;
    GtkApplication 	*Nx3D;
    
    printf("4X3D version: %d.%d.%d\n", AA4X3D_MAJOR_VERSION, AA4X3D_MINOR_VERSION, AA4X3D_MICRO_VERSION);
    printf(" GTK version: %d.%d.%d\n", GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);    
    
    // first ensure stack size is large enough, reset to 16Mb if necessary
    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
      {
      if (rl.rlim_cur < kStackSize)
        {
	rl.rlim_cur = kStackSize;
	result = setrlimit(RLIMIT_STACK, &rl);
	if (result != 0)
	  {
	  fprintf(stderr, "setrlimit returned result = %d\n", result);
	  }
	}
      }
    
    // launch seperate thread to control printing
    // note that this thread does not use any GTK calls and therefore uses pthread vs GTKs thread management
    // there is also an effort to keep variables isolated in these functions to loosely keep them thread safe
    // in lieu of mutexing.
    print_thread_alive=0;						// detectable heartbeat to ensure thread is still alive
    newprio=98;								// set a bit higher than default 20
    i=pthread_attr_init (&tattr);					// init attributes
    i=pthread_attr_getschedparam (&tattr, &param);			// get existing attributes
    param.sched_priority = newprio;					// set new priority
    i=pthread_attr_setschedparam (&tattr, &param);			// set new sched param
    i=pthread_create(&(build_job_tid),&tattr,&build_job,NULL);
    if(i!=0)
      {
      printf("Error creating print control thread... shutting down.\n");
      return(0);
      }

    // launch seperate thread to control thermal devices
    // note that this thread does not use any GTK calls and therefore uses pthread vs GTKs thread management
    // there is also an effort to keep variables isolated in these functions to loosely keep them thread safe
    // in lieu of mutexing.
    thermal_thread_alive=0;						// detectable heartbeat to ensure thread is still alive
    newprio=99;								// set to highest priority
    i=pthread_attr_init (&tattr);					// init attributes
    i=pthread_attr_getschedparam (&tattr, &param);			// get existing attributes
    param.sched_priority = newprio;					// set new priority
    i=pthread_attr_setschedparam (&tattr, &param);			// set new sched param
    i=pthread_create(&(thermal_tid),&tattr,&ThermalControl,NULL);
    if(i!=0)
      {
      printf("Error creating thermal control thread... shutting down.\n");
      return(0);
      }

    // Create the application
    gtk_init();
    Nx3D = gtk_application_new ("aallc.Nx3D", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (Nx3D, "activate", G_CALLBACK (activate_app), NULL);
    app_status = g_application_run (G_APPLICATION(Nx3D), argc, argv);
    g_object_unref(Nx3D); 
    
    return(0);   
}
    


