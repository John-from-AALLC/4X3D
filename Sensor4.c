#include "Global.h"	
		
// Function to search for and open 1wire devices
// Input:  slot=tool slot to assign (assuming it is the ONLY one active)
//	   if slot<0, then check for the bus master
// Return: the number of 1wire devices found

int Open1Wire(int slot) 
{	
  DIR 		*dir;							// location of 1wire data files
  struct 	dirent *dirent;						// scratch directory data
  char 		path[] = "/sys/bus/w1/devices";
  int		result=0;
  
  // Attempt opening 1wire device directory and get/count what's there
  printf("\n1Wire Devices:\n");
  dir=opendir(path);
  if(dir!=NULL)
    {
    while((dirent=readdir(dir))!=NULL)
      {
      // list found directories as we scroll thru them skipping <.> and <..>
      if(strlen(dirent->d_name)<5)continue;
      printf("  <%s>\n",dirent->d_name);
	
      // check for bus master to ensure 1wire driver active
      if(dirent->d_type==DT_LNK && strstr(dirent->d_name,"w1_bus")!=NULL)
	{
	result++;
	printf("  Device:  <%s>\n",dirent->d_name);
	}
     
      // if not assigning to a device, skip rest
      if(slot<0 || slot>=MAX_TOOLS)continue;

      // 1-wire thermal devices are subdirs beginning with 28-
      if(dirent->d_type==DT_LNK && strstr(dirent->d_name, "28-")!=NULL) 
	{ 
	result++;
	thermal_devices_found++;
	strcpy(Tool[slot].mmry.dev,dirent->d_name);			// extract unique device ID from subdir name
	sprintf(Tool[slot].mmry.devPath, "%s/%s/w1_slave",path,Tool[slot].mmry.dev);
	Tool[slot].mmry.devType=28;
	printf("  Slot: %d  Device: <%s>\n",slot,dirent->d_name);
	}
	
      // 1-wire memory devices are subdirs beginning with 23-
      if(dirent->d_type==DT_LNK && strstr(dirent->d_name, "23-")!=NULL) 
	{ 
	result++;
	memory_devices_found++;
	strcpy(Tool[slot].mmry.dev,dirent->d_name);			// extract unique device ID from subdir name
	sprintf(Tool[slot].mmry.devPath, "%s/%s/eeprom",path,Tool[slot].mmry.dev);
	Tool[slot].mmry.devType=23;
	printf("  Slot: %d  Device: <%s>\n",slot,dirent->d_name);
	}
      }     
 
    (void)closedir(dir);
    }
  else
    {
    printf("1WIRE: Unable to open directory \"%s\"! \n",path);
    }
  
  printf(" \n");
  return(result);
}
  
// Function to validate values read from 1wire memory device
int MemDevDataValidate(int slot)
{
  int	status=TRUE;
  
  debug_flag=602;
  if(debug_flag==602)printf("\nTool Memory Verification Dump:\n");
   
  if(debug_flag==602)printf("<%s>\n",Tool[slot].mmry.md.rev);
  if(Tool[slot].mmry.md.rev[0]<32 || Tool[slot].mmry.md.rev[0]>122 || strlen(Tool[slot].mmry.md.rev)>sizeof(Tool[slot].mmry.md.rev))
    {
    memset(Tool[slot].mmry.md.rev,0,sizeof(Tool[slot].mmry.md.rev));
    status=FALSE;
    if(debug_flag==601)printf("  1wire - bad rev found: %d:%s\n",strlen(Tool[slot].mmry.md.rev),Tool[slot].mmry.md.rev);
    }
    
  if(debug_flag==602)printf("<%s>\n",Tool[slot].mmry.md.name);
  if(Tool[slot].mmry.md.name[0]<32 || Tool[slot].mmry.md.name[0]>122 || strlen(Tool[slot].mmry.md.name)>32)
    {
    memset(Tool[slot].mmry.md.name,0,sizeof(Tool[slot].mmry.md.name));
    status=FALSE;
    if(debug_flag==601)printf("  1wire - bad name found: %d:%s\n",strlen(Tool[slot].mmry.md.name),Tool[slot].mmry.md.name);
    }
  
  if(debug_flag==602)printf("<%s>\n",Tool[slot].mmry.md.manf);
  if(Tool[slot].mmry.md.manf[0]<32 || Tool[slot].mmry.md.manf[0]>122 || strlen(Tool[slot].mmry.md.manf)>32)
    {
    memset(Tool[slot].mmry.md.manf,0,sizeof(Tool[slot].mmry.md.manf));
    status=FALSE;
    if(debug_flag==601)printf("  1wire - bad manf found: %d:%s\n",strlen(Tool[slot].mmry.md.manf),Tool[slot].mmry.md.manf);
    }
  
  if(debug_flag==602)printf("<%s>\n",Tool[slot].mmry.md.date);
  if(Tool[slot].mmry.md.date[0]<32 || Tool[slot].mmry.md.date[0]>122 || strlen(Tool[slot].mmry.md.date)>10)
    {
    memset(Tool[slot].mmry.md.date,0,sizeof(Tool[slot].mmry.md.date));
    status=FALSE;
    if(debug_flag==601)printf("  1wire - bad date found: %d:%s\n",strlen(Tool[slot].mmry.md.date),Tool[slot].mmry.md.date);
    }
  
  if(debug_flag==602)printf("<%s>\n",Tool[slot].mmry.md.matp);
  if(Tool[slot].mmry.md.matp[0]<32 || Tool[slot].mmry.md.matp[0]>122 || strlen(Tool[slot].mmry.md.matp)>32)
    {
    memset(Tool[slot].mmry.md.matp,0,sizeof(Tool[slot].mmry.md.matp));
    status=FALSE;
    if(debug_flag==601)printf("  1wire - bad matp found: %d:%s\n",strlen(Tool[slot].mmry.md.matp),Tool[slot].mmry.md.matp);
    }

  if(debug_flag==602)printf("<%d>\n",Tool[slot].mmry.md.sernum);
  if(Tool[slot].mmry.md.sernum<0 || Tool[slot].mmry.md.sernum>100)
    {
    Tool[slot].mmry.md.sernum=(-1);
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad sernum found\n");
    }

  if(debug_flag==602)printf("<%d>\n",Tool[slot].mmry.md.toolID);
  if(Tool[slot].mmry.md.toolID<0 || Tool[slot].mmry.md.toolID>100)
    {
    Tool[slot].mmry.md.toolID=(-1);
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad toolID found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.powr_lo_calduty);
  if(isnanf(Tool[slot].mmry.md.powr_lo_calduty)==TRUE || Tool[slot].mmry.md.powr_lo_calduty<0 || Tool[slot].mmry.md.powr_lo_calduty>100)
    {
    Tool[slot].mmry.md.powr_lo_calduty=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad powr_lo_calduty found\n");
    }
  
  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.powr_lo_calvolt);
  if(isnanf(Tool[slot].mmry.md.powr_lo_calvolt)==TRUE || Tool[slot].mmry.md.powr_lo_calvolt<0 || Tool[slot].mmry.md.powr_lo_calvolt>5)
    {
    Tool[slot].mmry.md.powr_lo_calvolt=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad powr_lo_calvolt found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.powr_hi_calduty);
  if(isnanf(Tool[slot].mmry.md.powr_hi_calduty)==TRUE || Tool[slot].mmry.md.powr_hi_calduty<0 || Tool[slot].mmry.md.powr_hi_calduty>100)
    {
    Tool[slot].mmry.md.powr_hi_calduty=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad powr_hi_calduty found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.powr_hi_calvolt);
  if(isnanf(Tool[slot].mmry.md.powr_hi_calvolt)==TRUE || Tool[slot].mmry.md.powr_hi_calvolt<0 || Tool[slot].mmry.md.powr_hi_calvolt>50)
    {
    Tool[slot].mmry.md.powr_hi_calvolt=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad powr_hi_calvolt found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.temp_lo_caltemp);
  if(isnanf(Tool[slot].mmry.md.temp_lo_caltemp)==TRUE || Tool[slot].mmry.md.temp_lo_caltemp<0 || Tool[slot].mmry.md.temp_lo_caltemp>400)
    {
    Tool[slot].mmry.md.temp_lo_caltemp=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad temp_lo_caltemp found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.temp_lo_calvolt);
  if(isnanf(Tool[slot].mmry.md.temp_lo_calvolt)==TRUE || Tool[slot].mmry.md.temp_lo_calvolt<0 || Tool[slot].mmry.md.temp_lo_calvolt>5)
    {
    Tool[slot].mmry.md.temp_lo_calvolt=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad temp_lo_calvolt found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.temp_hi_caltemp);
  if(isnanf(Tool[slot].mmry.md.temp_hi_caltemp)==TRUE || Tool[slot].mmry.md.temp_hi_caltemp<0 || Tool[slot].mmry.md.temp_hi_caltemp>400)
    {
    Tool[slot].mmry.md.temp_hi_caltemp=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad temp_hi_caltemp found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.temp_hi_calvolt);
  if(isnanf(Tool[slot].mmry.md.temp_hi_calvolt)==TRUE || Tool[slot].mmry.md.temp_hi_calvolt<0 || Tool[slot].mmry.md.temp_hi_calvolt>5)
    {
    Tool[slot].mmry.md.temp_hi_calvolt=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad temp_hi_calvolt found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.tip_pos_X);
  if(isnanf(Tool[slot].mmry.md.tip_pos_X)==TRUE || Tool[slot].mmry.md.tip_pos_X<(-80) || Tool[slot].mmry.md.tip_pos_X>80)
    {
    Tool[slot].mmry.md.tip_pos_X=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad tip_pos_X found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.tip_pos_Y);
  if(isnanf(Tool[slot].mmry.md.tip_pos_Y)==TRUE || Tool[slot].mmry.md.tip_pos_Y<(-80) || Tool[slot].mmry.md.tip_pos_Y>80)
    {
    Tool[slot].mmry.md.tip_pos_Y=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad tip_pos_Y found\n");
    }

  if(debug_flag==602)printf("<%f>\n",Tool[slot].mmry.md.tip_pos_Z);
  if(isnanf(Tool[slot].mmry.md.tip_pos_Z)==TRUE || Tool[slot].mmry.md.tip_pos_Z<(-10) || Tool[slot].mmry.md.tip_pos_Z>10)
    {
    Tool[slot].mmry.md.tip_pos_Z=0;
    status=FALSE;
    if(debug_flag==601)printf("  1wire: bad tip_pos_Z found\n");
    }
  
  debug_flag=0;
  
  return(status);
}
  
// Function to read data from 1wire memory device
int MemDevRead(int slot)
{
  int	i,result=0,status=FALSE;

  // check status
  if(slot<0 || slot>=MAX_TOOLS)return(status);
  if(strlen(Tool[slot].mmry.dev)<8)return(status);			// if invalid device path found...

  // open device
  Tool[slot].mmry.fd=fopen(Tool[slot].mmry.devPath,"rb");
  if(Tool[slot].mmry.fd==NULL)
    {
    printf("1Wire Device:  Couldn't open device %s for reading! \n",Tool[slot].mmry.devPath);
    status=FALSE;
    }
  else
    {
    if(Tool[slot].mmry.devType==23)					// eeprom
      {
      result=fread(&Tool[slot].mmry.md,sizeof(Tool[slot].mmry.md),1,Tool[slot].mmry.fd);
      if(result!=1)result=0;						// handle all cases

      // if read was successfull, verify values of what was read from 1wire 
      if(result==TRUE)
	{
	printf("Tool %d 1wire memory read successful... %d bytes read. \n",slot,sizeof(Tool[slot].mmry.md));
	
	// reads are not gauranteed to be successful even if fread claims so.  double check by validity of
	// all parameters read.
	status=MemDevDataValidate(slot);
	
	// if the data integrity check passed...
	if(status==TRUE)
	  {
	  printf("   ... memory data verified successfully.\n");
	  // now verify that the tool memory is of correct revision
	  //if(strstr(Tool[slot].mmry.md.rev,TL_MEM_REV)==NULL)status=0;
	
	  // load generical tool params based on tool.name and tool.toolID found in memory
	  strcpy(Tool[slot].name,Tool[slot].mmry.md.name);
	  Tool[slot].tool_ID=Tool[slot].mmry.md.toolID;
	  XMLRead_Tool(slot,Tool[slot].name);
	  
	  // apply thermal calib values
	  // these will over ride anything previously set (like for RTDs)
	  Tool[slot].thrm.calib_t_low=Tool[slot].mmry.md.temp_lo_caltemp;
	  Tool[slot].thrm.calib_v_low=Tool[slot].mmry.md.temp_lo_calvolt;
	  Tool[slot].thrm.calib_t_high=Tool[slot].mmry.md.temp_hi_caltemp;
	  Tool[slot].thrm.calib_v_high=Tool[slot].mmry.md.temp_hi_calvolt;
	  Tool[slot].thrm.temp_offset=Tool[slot].mmry.md.extra_calib[0];
	
	  // tip tolerances
	  Tool[slot].tip_x_fab = Tool[slot].mmry.md.tip_pos_X;
	  Tool[slot].tip_y_fab = Tool[slot].mmry.md.tip_pos_Y;
	  Tool[slot].tip_z_fab = Tool[slot].mmry.md.tip_pos_Z;
	  
	  //dump_tool_params(slot);
	  //while(!kbhit());
      
	  // load material params based on matp value found in memory
	  XMLRead_Matl(slot,Tool[slot].mmry.md.matp);
    
	  printf("Tool %d 1wire read successful.\n\n",slot);
	  }
	else 
	  {
	  result=FALSE;							// this will force read from disc (below)
	  printf("   ... memory data verification failed.\n");
	  }
	}
       
      // if read was NOT successfull, reset mmry.md values as they were likely corrupted by the read and then read from disc
      if(result==FALSE)
	{
	printf("Tool %d 1wire read failed... reading data from tool file.\n",slot);
	memset(scratch,0,sizeof(scratch));
	status=tool_information_read(slot,scratch);				// open specific tool file and read data
	
	// if reading values from tool file failed, reset to defaults
	if(status==FALSE)
	  {
	  printf("   ... tool %d file read failed. setting data to defaults.\n",slot);
	  sprintf(Tool[slot].mmry.md.rev,"%01d%04d%01d",AA4X3D_MAJOR_VERSION,AA4X3D_MINOR_VERSION,AA4X3D_MICRO_VERSION);
	  strcpy(Tool[slot].mmry.md.name,Tool[slot].name);
	  strcpy(Tool[slot].mmry.md.manf,Tool[slot].manuf);
	  memset(Tool[slot].mmry.md.matp,0,sizeof(Tool[slot].mmry.md.matp));
	  strncpy(Tool[slot].mmry.md.matp,Tool[slot].matl.name,31);
	  Tool[slot].mmry.md.sernum=Tool[slot].sernum;
	  Tool[slot].mmry.md.toolID=Tool[slot].tool_ID;
	  
	  Tool[slot].mmry.md.powr_lo_calduty=Tool[slot].powr.calib_d_low;
	  Tool[slot].mmry.md.powr_lo_calvolt=Tool[slot].powr.calib_v_low;
	  Tool[slot].mmry.md.powr_hi_calduty=Tool[slot].powr.calib_d_high;
	  Tool[slot].mmry.md.powr_hi_calvolt=Tool[slot].powr.calib_v_high;
	  
	  Tool[slot].mmry.md.temp_lo_caltemp=Tool[slot].thrm.calib_t_low;
	  Tool[slot].mmry.md.temp_lo_calvolt=Tool[slot].thrm.calib_v_low;
	  Tool[slot].mmry.md.temp_hi_caltemp=Tool[slot].thrm.calib_t_high;
	  Tool[slot].mmry.md.temp_hi_calvolt=Tool[slot].thrm.calib_v_high;
	  
	  Tool[slot].mmry.md.tip_pos_X = 0.0;
	  Tool[slot].mmry.md.tip_pos_Y = 0.0;
	  Tool[slot].mmry.md.tip_pos_Z = 0.0;
	  Tool[slot].mmry.md.extra_calib[0]=Tool[slot].thrm.temp_offset;
	  for(i=1;i<4;i++)Tool[slot].mmry.md.extra_calib[i]=0.0;
	  status=TRUE;
	  }
	}
      }
      
    if(Tool[slot].mmry.devType==28)					// temperature sensor
      {
      status=FALSE;
      }
    fclose(Tool[slot].mmry.fd);	
    }
    
  return(status);
}
	
// Function to write data to 1wire memory device
int MemDevWrite(int slot)
{
  int	result=0,status=FALSE;
	
  // validate inputs
  if(slot<0 || slot>=MAX_TOOLS)return(status);				// if invalid slot...
  if(strlen(Tool[slot].mmry.dev)<8)return(status);			// if invalid device path found...
	
  // open device
  Tool[slot].mmry.fd=fopen(Tool[slot].mmry.devPath,"wb");		// attempt to open for binary write
  if(Tool[slot].mmry.fd==NULL)						// if it did not open...
    {
    mk_writable(Tool[slot].mmry.devPath);				// ... run bash script to change privlage
    delay(250);								// ... wait for system to make change
    Tool[slot].mmry.fd=fopen(Tool[slot].mmry.devPath,"wb");		// ... attempt to open again
    if(Tool[slot].mmry.fd==NULL)					// ... if still failing give up
      {
      printf("1-WIRE:  Couldn't open device %s for writing! \n",Tool[slot].mmry.devPath);
      status=FALSE;
      }
    }
  if(Tool[slot].mmry.fd!=NULL)						// if able to open for write...
    {
    // write data to device
    if(Tool[slot].mmry.devType==23)					// if an eprom type device...
      {
      printf("\nWriting data to tool %d 1wire....\n",slot);
      result=fwrite(&Tool[slot].mmry.md,sizeof(Tool[slot].mmry.md),1,Tool[slot].mmry.fd); 	// ... write 1 element of size mmry.md
      if(result==1) {status=TRUE;} else {status=FALSE;}						// ... verify 1 element was written
      }
    fclose(Tool[slot].mmry.fd);						// ... close device handle
    }
  
  return(status);
}

// Function to generate a skript that creates sudo privlages for memory writing
void mk_writable(char* path)
{
  char* fn = NULL;
  int 	pln = strlen(path);

  if(pln<8)return;							// if invalid path just exit
  fn = malloc(pln + 30);						// build buffer for skript
  strcpy( fn, "#!/bin/bash\nsudo chmod a+w " );				// define first part of bash command
  strcat( fn, path );							// add in the device path
  strcat( fn, "\n" );							// finish command with line feed/return
  printf("1-WIRE:  Calling sudo skript: %s \n", fn);			// show what's going on
  system(fn);								// call the skript
  free(fn);								// release buffer
  
  return;
}



// Callback to set response for abort job
void cb_zcalib_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if(response_id==GTK_RESPONSE_OK)gtk_window_close(GTK_WINDOW(dialog));
    return;
}


// Function to scan the model area using the line laser and camera
int build_table_line_laser_scan(void)
{
  int		h,i,j,n,slot;
  int		pd;
  int		bi_directional=TRUE;
  int 		xstr,ystr,xend,yend;
  int		number_of_sensor_reads;
  float 	motion_shift=1.0;
  float 	total_fval;
  float 	accel, vid_time;
  float 	xstart,ystart,xfinish,yfinish,probe_z_position;
  float 	xpos,ypos,zpos,Pzold,fval,OldPosX;
  float 	dist,avg_z,std_z,v[4],maxval;
  FILE 		*sdata;
  vertex	*vstart,*vfinish;
  vertex 	*vptr,*oldvptr,*fstvptr_x;
  char		img_name[255];
  
  
  // initialize
  job.state=JOB_TABLE_SCAN;;
  build_table_scan_flag=1;					// reset to "needs to be mapped to model" if this is a re-scan
  display_model_flag=TRUE;
  vstart=vertex_make();
  vfinish=vertex_make();
  scan_x_start=0.0;  scan_x_end=BUILD_TABLE_LEN_X;
  scan_y_start=0.0;  scan_y_end=BUILD_TABLE_LEN_Y;
  
  // clear any existing array values
  Pz_index=0;
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      Pz_raw[h][i]=(-1);
      Pz_att[h][i]=(-1);
      }
    }
  
  // delete all the existing files (if any) in the tablescan subdirectory
  strcat(bashfn,"#!/bin/bash\nsudo rm -f /home/aa/Documents/4X3D/TableScan/*.*");	// delete all files in this folder
  system(bashfn);									// issue the command

  // set params and create new movie of table
  active_model=NULL;
  slot=0;
  zoffset=0;								// clear the previous value of bit adjustment
  sweep_speed=1000;							// in mm/min = 41.67 mm/sec
  accel=19620.0;							// 1.0G = 9810 mm/s^2 avg accel (aproximating jerk)
  probe_z_position=200;

  // define initial targets
  xstart=15.0;								// start at min edge of x	
  ystart=(BUILD_TABLE_LEN_Y/2.0);					// start at 1/4 the way of y biased on camera side
  
  xfinish=BUILD_TABLE_LEN_X+20.0;					// finish at max edge of x
  yfinish=(BUILD_TABLE_LEN_Y/2.0);					// finish at 1/4 the way of y biased on camera side
  
  // calc video recording time
  vid_time = fabs(xstart-xfinish)/(sweep_speed/60);			// ignore accel for now

  // move to starting position before we start scan
  vstart->x=xstart;
  vstart->y=ystart;		
  vstart->z=probe_z_position;						// probe type dependent
  vstart->k=20000;
  print_vertex(vstart,slot,7,NULL);					// create move command - note this accounts for tool offsets etc.
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for movement to finish
  
  gpioWrite(CHAMBER_LED,!OFF);						// turn off chamber light
  //gpioWrite(LINE_LASER,!ON);						// turn on line laser
  
  // define move finish
  vfinish->x=xfinish;							// calculate new x position
  vfinish->y=yfinish;							// calculate new y position
  vfinish->z=probe_z_position;
  vfinish->k=sweep_speed;						// set speed of xy movement
  
  // initiate scan sweep of carriage
  sprintf(gcode_burst,"G4 P1 \n");					// add delay to let camera init when recording
  print_vertex(vfinish,slot,7,NULL);					// build move command - sets PosIs.x and PosWas.x
  tinyGSnd(gcode_burst);						// execute scan move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string

  // initial recording of video at up to 41fps
  sprintf(bashfn,"#!/bin/bash\nlibcamera-vid -v 0 -n 1 -t %6d ",(int)(vid_time*1000)); // define base camera command
  strcat(bashfn,"--width 1640 --height 1232 --codec mjpeg ");			// ... add the image size
  sprintf(img_name,"-o /home/aa/Documents/4X3D/TableScan/scan.mjpeg ");		// ... build file name
  strcat(bashfn,img_name);							// ... add the target file name
  sprintf(scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
  strcat(bashfn,scratch);							// ... add region of interest (zoom)
  strcat(bashfn,"--autofocus-mode continuous ");				// ... add focus control
  system(bashfn);								// issue the command to take a picture

  // constantly record images until the move is complete
  // note: this relies on syncronizing the time it takes to make the move with the time to record the video
  motion_complete();							// ensure move is done
  
  //gpioWrite(LINE_LASER,!OFF);						// turn off line laser
  gpioWrite(CHAMBER_LED,!ON);						// turn off chamber light
  
  goto_machine_home();
  
  // run command to extract individual images from the video
  sprintf(bashfn,"#!/bin/bash\nffmpeg -i /home/aa/Documents/4X3D/TableScan/scan.mjpeg -vcodec copy /home/aa/Documents/4X3D/TableScan/x_%%d.jpg");
  system(bashfn);								// issue the command to take a picture

  build_table_scan_index(1);

  free(vstart); vertex_mem--; vstart=NULL;
  free(vfinish);vertex_mem--; vfinish=NULL;

  return(1);
}

// Function to index scan images in the TableScan subdirectory
// Inputs: typ==0 is for job time lapse, typ==1 is for build table laser scan lapse
int build_table_scan_index(int typ)
{
  int		h,i,n,pd;
  DIR 		*dir;							// location of image files
  struct 	dirent *dirent;						// scratch directory data
  char 		path[255],hdr[12],pos_val[16];

  // determine how many images there were captured in the TableScan subdirectory and
  // build the image index which will be used by the scan_lapse to grab the right image
  scan_image_count=0;
  if(typ==0){sprintf(path,"/home/aa/Documents/4X3D/ImageHistory"); sprintf(hdr,"z_");}
  if(typ==1){sprintf(path,"/home/aa/Documents/4X3D/TableScan");    sprintf(hdr,"x_");}
  dir=opendir(path);
  if(dir!=NULL)
    {
    // loop thru all files and build image index based on x position saved in file name
    while((dirent=readdir(dir))!=NULL)
      {
      if(dirent->d_type==DT_REG && strstr(dirent->d_name,hdr)!=NULL)
        {
        // parse file name to extract z/x position
        n=0;
        memset(pos_val,0,sizeof(pos_val));					// clear value string
        for(i=0;i<strlen(dirent->d_name);i++)				// build value string one char at a time - allows full control of stream
	  {
	  pd=dirent->d_name[i];						// get char off input string
	  if(pd<48 || pd>57)continue;					// check if it is a number
	  pos_val[n]=pd;							// add char to output string
	  n++;
	  if(n>sizeof(pos_val))n=sizeof(pos_val)-1;
	  }

        scan_img_index[scan_image_count]=atoi(pos_val);
        scan_image_count++;
	if(scan_image_count>MAX_SCAN_IMAGES)scan_image_count=MAX_SCAN_IMAGES-1;
	}     
      }
      
    // sort the image index from lowest to highest
    for(i=0;i<scan_image_count;i++)
      {
      for(h=i+1;h<scan_image_count;h++)
        {
	if(scan_img_index[h] < scan_img_index[i])
	  {
	  n=scan_img_index[i];
	  scan_img_index[i]=scan_img_index[h];
	  scan_img_index[h]=n;
	  }
	}
      }
      
    }
      
  return(TRUE);
}

// Function to scan the model area under the tool in "slot" that is on the build table
// This version works by moving to a position, taking a z reading, then moving to next position.
// sensor_type:  1=touch probe  2=non-contact TOF  3=both (for comparison)
int build_table_step_scan(int slot, int sensor_type)
{
  
  int 		xis,yis,xstr,ystr,xmax,ymax,xold;
  int		number_of_sensor_reads;
  float 	probe_z_position;
  float 	xpos,ypos,zpos,Pzold,fval,total_val;
  int		h,i,j;
  float 	dist,avg_z,std_z,v[4],maxval;
  FILE 		*sdata;
  vertex	*vptr,*vnew;
  
  printf("/Table Step Z Scan:  entry \n");
  
  // force slot 0 and sensor type 2
  slot=0;
  sensor_type=1;
  active_model=NULL;
  
  // save state and set to false as the print vtx function reacts to this 
  old_z_tbl_comp=z_tbl_comp;
  z_tbl_comp=FALSE;
  
  // if using a touch probe, prompt user to insert probe tool
  if(sensor_type==1 && Tool[slot].tool_ID!=TC_PROBE)
    {
    sprintf(scratch,"\nInsert PROBE tool into slot %d\n",(slot+1));
    aa_dialog_box(win_settings,1,0,"Table Z Scan",scratch);

    // if probe still not installed, just leave
    if(Tool[slot].tool_ID!=TC_PROBE)return(0);
    }

  // if using a non-contact probe, prompt user to insert laser scan tool
  if(sensor_type==2 && Tool[slot].tool_ID!=TC_LASER)
    {
    sprintf(scratch,"\nInsert SCAN tool into slot %d\n",(slot+1));
    aa_dialog_box(win_settings,1,0,"Table Z Scan",scratch);

    // if laser still not installed, just leave
    if(Tool[slot].tool_ID!=TC_LASER)return(0);
    }
    
  printf("\nScanning using %s tool in slot %d. \n",Tool[slot].name,slot);
    
  // initialize
  win_testUI_flag=TRUE;
  job.state=JOB_TABLE_SCAN;
  build_table_scan_flag=1;					// reset to "needs to be mapped to model" if this is a re-scan
  win_settingsUI_flag=FALSE;
  display_model_flag=TRUE;
  set_view_build_lvl=TRUE;
  Pz_index=0;
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      Pz_raw[h][i]=(-1);
      Pz_att[h][i]=(-1);
      }
    }
  vptr=vertex_make();
  vnew=vertex_make();
  set_holding_torque(ON);
  
  // set up reads and position based on sensor type
  if(sensor_type==1)
    {
    number_of_sensor_reads=1;
    probe_z_position=3.0;						// starts at 3.000 mm above table

    // home all loaded tools so they are out of the way
    for(i=0;i<MAX_TOOLS;i++)
      {
      if(Tool[i].state>TL_UNKNOWN)tool_tip_home(i);
      }
    }
  if(sensor_type==2)
    {
    number_of_sensor_reads=3;
    probe_z_position=200.0;						// starts at 200.0 mm above table
    }
  zoffset=0;								// clear the previous value of bit adjustment

  // define the scan area relative to the build table (i.e. so it is always the same positions)
  xstr=0;								// the starting X position
  xmax=(int)(BUILD_TABLE_LEN_X/scan_dist)-1;				// number of readings in X
  if(xmax<0)xmax=0;if(xmax>=SCAN_X_MAX)xmax=SCAN_X_MAX-1;		// ensure it does not exceed array space
  scan_x_remain=(BUILD_TABLE_LEN_X-(xmax*scan_dist))/2;			// remainder in X
  
  ystr=0;								// the starting Y position
  ymax=(int)(BUILD_TABLE_LEN_Y/scan_dist)-1;				// number of readings in Y
  if(ymax<0)ymax=0;if(ymax>=SCAN_Y_MAX)ymax=SCAN_Y_MAX-1;		// ensure it does not exceed array space
  scan_y_remain=(BUILD_TABLE_LEN_Y-(ymax*scan_dist))/2;			// remainder in Y

  // move close to starting position before scan
  vptr->x=0.0;
  vptr->y=0.0;
  vptr->z=probe_z_position;						// probe type dependent
  print_vertex(vptr,slot,2,NULL);					// create move command
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for movement to finish
  tool_tip_deposit(slot);						// watch out for overheating drive stepper
  make_toolchange(slot,1);
  
  // loop thru table grid to collect z level moving from y low to y high
  for(h=ystr;h<=ymax;h++)
    {
	
    // move across the table in the positive X direction
    vptr->y=h*scan_dist+scan_y_remain;					// calculate new y position
    for(i=xstr;i<=xmax;i++)						// scan across x
      {
      //if(job.state!=JOB_RUNNING)break;
      vptr->x=i*scan_dist+scan_x_remain;				// calculate new x position
      if(sensor_type==1)
        {
        // move to XY location
        print_vertex(vptr,slot,2,NULL);					// build move cmd at full pen up speed
        tinyGSnd(gcode_burst);						// execute pen up move
        memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
        motion_complete();						// wait for motion to finish
	while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
	fval=touch_probe(vptr,slot);					// execute touch
	Pz_raw[i][h]=(probe_z_position - fval);				// save delta away from target of 0.000
	Pz_index++;
	
	// lift z before next xy move
	vptr->z=probe_z_position;					// move up to get over what is next (may not be flat)
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	print_vertex(vptr,slot,2,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	}
      if(sensor_type==2)
        {
	// move to XY location
	vptr->k=10000;							// set speed of xy movement
	print_vertex(vptr,slot,7,NULL);					// build move command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	//motion_complete();						// wait for motion to finish
	total_val=0.0;							// init averaging value
	j=0;
	while(j<number_of_sensor_reads)					// use external loop to avoid adding in error returns
	  {
	  fval=PostG.z-distance_sensor(slot,3,1,1);					// ... take a single read
	  if(fval>(-99) && fval<BUILD_TABLE_LEN_Z)			// ... if not an error, add it to sum
	    {
	    j++;
	    //fval -= (probe_z_position + Tool[slot].tip_dn_pos - 22.9);	// ... account for sensor z position on tool
	    fval -= 75.0;
	    total_val+=fval;
	    }
	  }
	Pz_raw[i][h]=total_val/number_of_sensor_reads;			// cacl average and save
	Pz_index++;
	}
      
      //printf("\n%d  Pz_raw = %f \n\n",Pz_index,Pz_raw[i][h]);
      
      Pz[i][h]=Pz_raw[i][h];
      Pz_att[i][h]=1;	
      }

    // move across table in the negative X direction
    h++;
    vptr->y=h*scan_dist+scan_y_remain;
    for(i=xmax;i>=xstr;i--)						// reverse scan direction in x
      {
      vptr->x=i*scan_dist+scan_x_remain;
      if(sensor_type==1)
        {
        // move to XY location
        print_vertex(vptr,slot,2,NULL);					// build move cmd at full pen up speed
        tinyGSnd(gcode_burst);						// execute pen up move
        memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
        motion_complete();						// wait for motion to finish
	while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
	fval=touch_probe(vptr,slot);					// execute touch
	Pz_raw[i][h]=(probe_z_position - fval);
	Pz_index++;

	// lift z before next xy move
	vptr->z=probe_z_position;					// move up to get over what is next (may not be flat)
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	print_vertex(vptr,slot,2,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	}
      if(sensor_type==2)
        {
	// move to XY location
	vptr->k=10000;							// set speed of xy movement
	print_vertex(vptr,slot,7,NULL);					// build move command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	//motion_complete();						// wait for motion to finish
	total_val=0.0;							// init averaging value
	j=0;
	while(j<number_of_sensor_reads)					// use external loop to avoid adding in error returns
	  {
	  fval=PostG.z-distance_sensor(slot,3,1,1);				// ... take a single read
	  if(fval>(-99) && fval<BUILD_TABLE_LEN_Z)			// ... if not an error, add it to sum
	    {
	    j++;
	    //fval -= (probe_z_position + Tool[slot].tip_dn_pos - 22.9);	// ... account for sensor z position on tool
	    fval -= 75.0;
	    total_val+=fval;
	    }
	  }
	Pz_raw[i][h]=total_val/number_of_sensor_reads;			// cacl average and save
	Pz_index++;
	}
      
      //printf("\n%d  Pz_raw = %f \n\n",Pz_index,Pz_raw[i][h]);
 
      Pz[i][h]=Pz_raw[i][h];
      Pz_att[i][h]=1;	
      }
    }

  // move carriage back home
  tool_tip_retract(slot);
  make_toolchange(slot,1);
  goto_machine_home();

  free(vptr);
  vertex_mem--;
  free(vnew);
  vertex_mem--;
  
  //if(job.state!=JOB_RUNNING)return(0);
  
  // save raw scan data to file
  //sprintf(scratch,"\nScan complete!  Save as table level?\n",(slot+1));
  //aa_dialog_box(win_settings,3,0,"Table Z Scan",scratch);
  //if(aa_dialog_result==TRUE)
    {
    sdata=fopen("/home/aa/Documents/4X3D/BldTblZData.txt","w");
    if(sdata!=NULL)
      {
      // first write the scan dist value used to create this scan
      sprintf(scratch,"%6.3f ",scan_dist);
      fwrite(scratch,1,strlen(scratch),sdata);
      sprintf(scratch,"\n");
      fwrite(scratch,1,strlen(scratch),sdata);
      
      // next write the table scan points themselves
      for(h=0;h<=xmax;h++)
	  {
	  for(i=0;i<=ymax;i++)
	      {
	      sprintf(scratch,"%6.3f ",Pz_raw[h][i]);
	      fwrite(scratch,1,strlen(scratch),sdata);
	      }
	  sprintf(scratch,"\n");
	  fwrite(scratch,1,strlen(scratch),sdata);
	  }
      sprintf(scratch,"\n");
      fwrite(scratch,1,strlen(scratch),sdata);
      fflush(sdata);
      fclose(sdata);
      printf("\nBldTblZData - done writing table displacement data. \n\n");
      }
    else 
      {
      sprintf(scratch,"\nError!  Unable to open scan data file.\n");
      aa_dialog_box(win_settings,1,0,"Table Z Scan",scratch);
      }
    }

  // scan thru data for lowest value
  /*
  maxval=BUILD_TABLE_LEN_Z;
  for(h=0;h<=xmax;h++)
    {
    for(i=0;i<=ymax;i++)
      {
      if(Pz_raw[h][i]==0)continue;
      if(Pz_raw[h][i]<maxval)maxval=Pz_raw[h][i];
      }
    }
  // normalize to the greatest reading
  for(h=0;h<=xmax;h++)
    {
    for(i=0;i<=ymax;i++)
      {
      Pz[h][i]=Pz_raw[h][i]+fabs(maxval);
      }
    }
  */

  win_testUI_flag=FALSE;
  build_table_scan_flag=1;
  z_tbl_comp=old_z_tbl_comp;
  
  return(1);
}

// Function to scan the model area under the tool in "slot" that is on the build table with single point z distance sensor.
// This version works by taking z readings while the tool remains moving to speed things up.
// To further speed things up it does an initial XY scan across the middle of the table to
// establish the min/max values in each direction and only scans within that area.
// sensor_type:  1=touch probe  2=non-contact TOF  3=both (for comparison)
int build_table_sweep_scan(int slot, int sensor_type)
{
  int		h,i,j;
  int		bi_directional=TRUE;
  int 		xstr,ystr,xend,yend;
  int		number_of_sensor_reads;
  float 	motion_shift=1.0;
  float 	total_fval;
  float 	probe_z_position;
  float 	xpos,ypos,zpos,Pzold,fval,OldPosX;
  float 	dist,avg_z,std_z,v[4],maxval;
  FILE 		*sdata;
  vertex	*vstart,*vfinish;
  vertex 	*vptr,*oldvptr,*fstvptr_x;
  
  
  printf("/Table Sweep Z Scan:  entry \n");
  
  // force slot 0 and sensor type 2
  slot=0;
  sensor_type=2;
  active_model=NULL;
  
  // if using a non-contact probe, prompt user to insert laser tool
  if(sensor_type==2 && Tool[slot].tool_ID!=TC_LASER)
    {
    sprintf(scratch,"\nInsert LASER tool into slot %d\n",(slot+1));
    aa_dialog_box(win_settings,1,0,"Table Z Scan",scratch);

    // if laser still not installed, just leave
    if(Tool[slot].tool_ID!=TC_LASER)return(0);
    }
    
  printf("\nScanning using %s tool in slot %d. \n",Tool[slot].name,slot);
    
  // initialize
  win_testUI_flag=TRUE;
  job.state=JOB_RUNNING;
  build_table_scan_flag=1;					// reset to "needs to be mapped to model" if this is a re-scan
  win_settingsUI_flag=FALSE;
  display_model_flag=TRUE;
  vstart=vertex_make();
  vfinish=vertex_make();
  scan_x_start=0.0;  scan_x_end=BUILD_TABLE_LEN_X;
  scan_y_start=0.0;  scan_y_end=BUILD_TABLE_LEN_Y;
  
  // clear any existing array values
  Pz_index=0;
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      Pz_raw[h][i]=(-1);
      Pz_att[h][i]=(-1);
      }
    }
  
  // set up reads and position based on sensor type
  if(sensor_type==2)
    {
    number_of_sensor_reads=3;
    probe_z_position=200.0;						// starts at 200.0 mm above table
    sweep_speed=1500;							// mm/min - slower is better quality
    
    // apply offset to align center of sensor to center of laser
    Tool[0].x_offset -= 12.0;
    Tool[0].y_offset += 27.0;
    Tool[0].tip_dn_pos -= 30.0;
    }
  zoffset=0;								// clear the previous value of bit adjustment

  // define the scan area relative to the build table (i.e. so it is always the same positions)
  build_table_centerline_scan(slot,sensor_type);			// establishes scan_x_start etc.
  
  xstr=(int)((scan_x_start-scan_x_remain)/scan_dist);			// the starting x array index (round down)
  if(xstr<0)xstr=0; if(xstr>=SCAN_X_MAX)xstr=SCAN_X_MAX-1;		// ensure it does not exceed array space
  
  ystr=(int)((scan_y_start-scan_y_remain)/scan_dist);			// the starting y array index
  if(ystr<0)ystr=0; if(ystr>=SCAN_Y_MAX)ystr=SCAN_Y_MAX-1;		// ensure it does not exceed array space
  
  xend=(int)((scan_x_end-scan_x_remain)/scan_dist);			// the ending x array index (round down)
  if(xend<0)xend=0; if(xend>=SCAN_X_MAX)xend=SCAN_X_MAX-1;		// ensure it does not exceed array space
  
  yend=(int)((scan_y_end-scan_y_remain)/scan_dist);			// the ending y array index
  if(yend<0)yend=0; if(yend>=SCAN_Y_MAX)yend=SCAN_Y_MAX-1;		// ensure it does not exceed array space
  
  printf("\nxmin=%6.0f  xmax=%6.0f   ymin=%6.0f  ymax=%6.0f\n",scan_x_start,scan_x_end,scan_y_start,scan_y_end);
  printf("xstr=%d   xend=%d     ystr=%d  yend=%d \n\n",xstr,xend,ystr,yend);
  //while(!kbhit());

  // make sure tool is all set
  tool_tip_deposit(slot);						// watch out for overheating drive stepper
  make_toolchange(slot,1);
  
  // move from y low to y high sweeping back and forth along x to collect z
  h=ystr;								// init y array index
  while(h<=yend)							// loop until at finish location
    {
	
    // move across the table in the positive X direction
    // move to start
    vstart->x=xstr*scan_dist+scan_x_remain;				// calculate new x position
    vstart->y=h*scan_dist+scan_y_remain;				// calculate new y position
    vstart->z=probe_z_position;
    vstart->k=20000;
    print_vertex(vstart,slot,7,NULL);					// build move command
    tinyGSnd(gcode_burst);						// execute pen up move
    memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
    
    // move to finish
    vfinish->x=xend*scan_dist+scan_x_remain;				// calculate new x position
    vfinish->y=h*scan_dist+scan_y_remain;				// calculate new y position
    vfinish->z=probe_z_position;
    vfinish->k=sweep_speed;						// set speed of xy movement
    print_vertex(vfinish,slot,7,NULL);					// build move command - sets PosIs.x and PosWas.x
    tinyGSnd(gcode_burst);						// execute scan move
    memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
    
    printf("\nmove: x=%6.0f y=%6.0f   to   x=%6.0f y=%6.0f \n\n",vstart->x,vstart->y,vfinish->x,vfinish->y);

    // get state at start of motion
    sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
    tinyGSnd(gcode_cmd);						// send request
    while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position if moving
    
    // make sweep across x
    j=0;
    fstvptr_x=NULL;
    oldvptr=NULL;
    while(tinyG_in_motion==TRUE)						// loop until motion is complete...
      {
      j++;
      vptr=vertex_make();
      vptr->x=PostG.x-motion_shift;
      vptr->y=PostG.y;
      vptr->z=probe_z_position+Tool[slot].tip_dn_pos-distance_sensor(slot,3,1,1);
      if(fstvptr_x==NULL)fstvptr_x=vptr;
      if(oldvptr!=NULL)oldvptr->next=vptr;
      oldvptr=vptr;
      sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
      tinyGSnd(gcode_cmd);						// send request
      while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position of PostG.x
      }
    motion_complete();							// ensure move is done
      
    // resolve data onto table grid
    i=xstr-1;
    vptr=fstvptr_x;
    while(vptr!=NULL)
      {
      printf("vptr: x=%6.3f  y=%6.3f  z=%6.3f \n",vptr->x,vptr->y,vptr->z);
      if(floor(vptr->x/scan_dist)!=i)
        {
        i=floor(vptr->x/scan_dist);
        if(i<0)i=0;
        if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;					// ensure within array space
        Pz_raw[i][h]=vptr->z;
        if(Pz_raw[i][h]<0)Pz_raw[i][h]=0;					// limit to zero
        Pz[i][h]=Pz_raw[i][h];
        Pz_att[i][h]=1;	
        Pz_index++;
        printf("  Pz[%d][%d]=%6.3f \n",i,h,Pz_raw[i][h]);
        }
      vptr=vptr->next;
      }
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}  // update display

    // clean up
    vptr=fstvptr_x;
    while(vptr!=NULL)
      {
      oldvptr=vptr;
      vptr=vptr->next;
      free(oldvptr); vertex_mem--;
      }

    // increment to next row
    h++;

    // move across the table in the negative X direction
    if(bi_directional==TRUE)
      {
	// move to start
	vstart->x=xend*scan_dist+scan_x_remain;				// calculate new x position
	vstart->y=h*scan_dist+scan_y_remain;				// calculate new y position
	vstart->z=probe_z_position;
	vstart->k=20000;
	print_vertex(vstart,slot,7,NULL);				// build move command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	
	// move to finish
	vfinish->x=xstr*scan_dist+scan_x_remain;			// calculate new x position
	vfinish->y=h*scan_dist+scan_y_remain;				// calculate new y position
	vfinish->z=probe_z_position;
	vfinish->k=sweep_speed;						// set speed of xy movement
	print_vertex(vfinish,slot,7,NULL);				// build move command
	tinyGSnd(gcode_burst);						// execute scan move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string

	printf("move: x=%6.0f y=%6.0f   to   x=%6.0f y=%6.0f \n",vstart->x,vstart->y,vfinish->x,vfinish->y);
	
	// get state at start of motion
	sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
	tinyGSnd(gcode_cmd);						// send request
	while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position if moving
	
	// make sweep across x
	j=0;
	fstvptr_x=NULL;
	oldvptr=NULL;
	while(tinyG_in_motion==TRUE)					// loop until motion is complete...
	  {
	  j++;
	  vptr=vertex_make();
	  vptr->x=PostG.x+motion_shift;
	  vptr->y=PostG.y;
	  vptr->z=probe_z_position+Tool[slot].tip_dn_pos-distance_sensor(slot,3,1,1);
	  if(fstvptr_x==NULL)fstvptr_x=vptr;
	  if(oldvptr!=NULL)oldvptr->next=vptr;
	  oldvptr=vptr;
	  sprintf(gcode_cmd,"{\"mots\":null}\n");			// req motion status
	  tinyGSnd(gcode_cmd);						// send request
	  while(tinyGRcv(1)==0);					// get tinyg feedback - this will update position of PostG.x
	  }
	motion_complete();						// ensure move is done
	  
	// resolve data onto table grid
	i=xend+1;
	vptr=fstvptr_x;
	while(vptr!=NULL)
	  {
	  printf("vptr: x=%6.3f  y=%6.3f  z=%6.3f \n",vptr->x,vptr->y,vptr->z);
	  if(floor(vptr->x/scan_dist)!=i)
	    {
	    i=floor(vptr->x/scan_dist);
	    if(i<0)i=0;
	    if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;					// ensure within array space
	    Pz_raw[i][h]=vptr->z;
	    if(Pz_raw[i][h]<0)Pz_raw[i][h]=0;					// limit to zero
	    Pz[i][h]=Pz_raw[i][h];
	    Pz_att[i][h]=1;	
	    Pz_index++;
	    printf("  Pz[%d][%d]=%6.3f \n",i,h,Pz_raw[i][h]);
	    }
	  vptr=vptr->next;
	  }
	while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}  // update display
    
	// clean up
	vptr=fstvptr_x;
	while(vptr!=NULL)
	  {
	  oldvptr=vptr;
	  vptr=vptr->next;
	  free(oldvptr); vertex_mem--;
	  }
    
	h++;
      }
    }

  // move carriage back home
  tool_tip_retract(slot);
  make_toolchange(slot,1);
  goto_machine_home();

  free(vstart); vertex_mem--; vstart=NULL;
  free(vfinish);vertex_mem--; vfinish=NULL;
  
  if(job.state!=JOB_RUNNING)return(0);
  
  // save raw scan data to file
  sprintf(scratch,"\nScan complete!  Save as table level?\n",(slot+1));
  aa_dialog_box(win_settings,3,0,"Table Z Scan",scratch);
  if(aa_dialog_result==TRUE)
    {
    sdata=fopen("BldTblZData.txt","w");
    for(i=0;i<SCAN_X_MAX;i++)
	{
	for(h=0;h<SCAN_Y_MAX;h++)
	    {
	    sprintf(scratch,"%6.3f ",Pz_raw[i][h]);
	    fwrite(scratch,1,strlen(scratch),sdata);
	    }
	sprintf(scratch,"\n");
	fwrite(scratch,1,strlen(scratch),sdata);
	}
    sprintf(scratch,"\n");
    fwrite(scratch,1,strlen(scratch),sdata);
    fflush(sdata);
    fclose(sdata);
    }

  // scan thru data for lowest value
  maxval=BUILD_TABLE_LEN_Z;
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      if(Pz_raw[h][i]==0)continue;
      if(Pz_raw[h][i]<maxval)maxval=Pz_raw[h][i];
      }
    }
  // normalize to the greatest reading
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      Pz[h][i]=Pz_raw[h][i]+fabs(maxval);
      }
    }

  Tool[0].x_offset += 12.0;
  Tool[0].y_offset -= 27.0;
  Tool[0].tip_dn_pos += 30.0;

  win_testUI_flag=FALSE;
  build_table_scan_flag=1;
  return(1);
}

// Function to scan the centerline of the build table with a single point z distance sensor.
// sensor_type:  1=touch probe  2=non-contact TOF  3=both (for comparison)
int build_table_centerline_scan(int slot, int sensor_type)
{
  int 		xis,yis,xold;
  int		number_of_sensor_reads,total_fval;
  float 	xstart,ystart,xfinish,yfinish,xinc,yinc;
  float 	probe_z_position;
  float 	edge_threshold=2.0;
  float 	StartPosX,FinishPosX;
  float 	xpos,ypos,zpos,Pzold,fval,OldPosX,OldPosY;
  int		h,i,j;
  float 	dist,avg_z,std_z,v[4],maxval;
  vertex	*vstart,*vfinish;
  vertex 	*vptr,*fstvptr_x,*fstvptr_y,*oldvptr;
  
  
  // force slot 0 and sensor type 2
  slot=0;
  sensor_type=2;
  active_model=NULL;
  printf("\nCenterline scan using %s tool in slot %d. \n",Tool[slot].name,slot);
    
  // initialize
  job.state=JOB_RUNNING;
  build_table_scan_flag=1;						// reset to "needs to be mapped to model" if this is a re-scan
  win_settingsUI_flag=FALSE;						// turn off to not conflict with thermal thread
  display_model_flag=TRUE;						// turn on to see result in model view
  gtk_widget_queue_draw(g_model_area);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}  // update display

  vstart=vertex_make();
  vfinish=vertex_make();
  
  // set up reads and position based on sensor type
  if(sensor_type==2)
    {
    number_of_sensor_reads=3;
    probe_z_position=200.0;						// starts at 150.0 mm above table
    edge_threshold=10.0;
    }
  zoffset=0;								// clear the previous value of bit adjustment

  // define initial targets
  xstart=0.0;								// start at edge of x	
  ystart=(BUILD_TABLE_LEN_Y/2.0);					// start at middle of y
  
  xfinish=BUILD_TABLE_LEN_X;						// finish at end of x
  yfinish=(BUILD_TABLE_LEN_Y/2.0);					// finish at middle of y

  // move to starting position before we start scan
  vstart->x=xstart;
  vstart->y=ystart;		
  vstart->z=probe_z_position;						// probe type dependent
  vstart->k=20000;
  print_vertex(vstart,slot,7,NULL);					// create move command - note this accounts for tool offsets etc.
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for movement to finish
  tool_tip_deposit(slot);						// watch out for overheating drive stepper
  make_toolchange(slot,1);
  
  // define move finish
  vfinish->x=xfinish;							// calculate new x position
  vfinish->y=yfinish;							// calculate new y position
  vfinish->z=probe_z_position;
  vfinish->k=sweep_speed;						// set speed of xy movement
  print_vertex(vfinish,slot,7,NULL);					// build move command - sets PosIs.x and PosWas.x
  tinyGSnd(gcode_burst);						// execute scan move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string

  // get state at start of motion
  sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
  tinyGSnd(gcode_cmd);							// send request
  while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position if moving

  // constantly record data as we move across the table
  // since we are first sweeping from low x to high x at the same (middle) y, we
  // need to define the array location in y to fill... which is the max/2
  h=(int)(scan_ymax/2);
  if(h<0)h=0; 
  if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;					// ensure we stay in bounds of array
  j=0;
  fstvptr_x=NULL;
  oldvptr=NULL;
  while(tinyG_in_motion==TRUE)						// loop until motion is complete...
    {
    j++;
    vptr=vertex_make();
    vptr->x=PostG.x;
    vptr->y=PostG.y;
    vptr->z=probe_z_position+Tool[slot].tip_dn_pos-distance_sensor(slot,3,1,1);
    if(fstvptr_x==NULL)fstvptr_x=vptr;
    if(oldvptr!=NULL)oldvptr->next=vptr;
    oldvptr=vptr;
    sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
    tinyGSnd(gcode_cmd);						// send request
    while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position of PostG.x
    }
  motion_complete();							// ensure move is done
      
  // resolve data onto table grid
  i=(-1);
  vptr=fstvptr_x;
  while(vptr!=NULL)
    {
    printf("vptr: x=%6.3f  y=%6.3f  z=%6.3f \n",vptr->x,vptr->y,vptr->z);
    if(floor(vptr->x/scan_dist)!=i)
      {
      i=floor(vptr->x/scan_dist);
      if(i<0)i=0;
      if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;					// ensure within array space
      Pz_raw[i][h]=vptr->z;
      if(Pz_raw[i][h]<0)Pz_raw[i][h]=0;					// limit to zero
      
      if(i>0 && Pz_raw[i-1][h]<edge_threshold && Pz_raw[i][h]>edge_threshold)scan_x_start=vptr->x;  // if the start of an edge... save position
      
      if(i>0 && Pz_raw[i-1][h]>edge_threshold && Pz_raw[i][h]<edge_threshold)scan_x_end=vptr->x;    // if the end of an edge ... save position
      
      Pz[i][h]=Pz_raw[i][h];
      Pz_att[i][h]=1;	
      Pz_index++;
      printf("  Pz[%d]=%6.3f \n",i,Pz_raw[i][h]);
      }
    vptr=vptr->next;
    }
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}  // update display

  // clean up
  vptr=fstvptr_x;
  while(vptr!=NULL)
    {
    oldvptr=vptr;
    vptr=vptr->next;
    free(oldvptr); vertex_mem--;
    }

  // define initial targets
  xstart=(BUILD_TABLE_LEN_X/2.0);					// start at middle of x
  ystart=0.0;								// start at edge of y	
  
  xfinish=(BUILD_TABLE_LEN_X/2.0);					// finish at middle of x
  yfinish=BUILD_TABLE_LEN_Y;						// finish at end of y

  // move close to starting position before scan
  vstart->x=xstart;
  vstart->y=ystart;		
  vstart->z=probe_z_position;						// probe type dependent
  vstart->k=20000;
  print_vertex(vstart,slot,7,NULL);					// create move command
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for movement to finish
  tool_tip_deposit(slot);						// watch out for overheating drive stepper
  make_toolchange(slot,1);
    
  // move to finish
  vfinish->x=xfinish;							// calculate new x position
  vfinish->y=yfinish;							// calculate new y position
  vfinish->z=probe_z_position;
  vfinish->k=sweep_speed;						// set speed of xy movement
  print_vertex(vfinish,slot,7,NULL);					// build move command - sets PosIs.x and PosWas.x
  tinyGSnd(gcode_burst);						// execute scan move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string

  // get state at start of motion
  sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
  tinyGSnd(gcode_cmd);							// send request
  while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position if moving
    
  // constantly record data as we move across the table
  j=0;
  fstvptr_y=NULL;
  oldvptr=NULL;
  while(tinyG_in_motion==TRUE)						// loop until motion is complete...
    {
    j++;
    vptr=vertex_make();
    vptr->x=PostG.x;
    vptr->y=PostG.y;
    vptr->z=probe_z_position+Tool[slot].tip_dn_pos-distance_sensor(slot,3,1,1);
    if(fstvptr_y==NULL)fstvptr_y=vptr;
    if(oldvptr!=NULL)oldvptr->next=vptr;
    oldvptr=vptr;
    sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
    tinyGSnd(gcode_cmd);						// send request
    while(tinyGRcv(1)==0);						// get tinyg feedback - this will update position of PostG.x
    }
  motion_complete();							// ensure move is done
      
  // resolve data onto table grid
  i=(int)(scan_xmax/2);
  if(i<0)i=0; 
  if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;
  h=(-1);
  vptr=fstvptr_y;
  while(vptr!=NULL)
    {
    printf("vptr: x=%6.3f  y=%6.3f  z=%6.3f \n",vptr->x,vptr->y,vptr->z);
    if(floor(vptr->y/scan_dist)!=h)
      {
      h=floor(vptr->y/scan_dist);
      if(h<0)h=0;
      if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;					// ensure within array space
      Pz_raw[i][h]=vptr->z;
      if(Pz_raw[i][h]<0)Pz_raw[i][h]=0;					// limit to zero
      
      // if the prev value is close to the table, and the current value is above the edge threshold...
      if(h>0 && Pz_raw[i][h-1]<edge_threshold && Pz_raw[i][h]>edge_threshold)scan_y_start=vptr->y;  // if the start of an edge... save position
      
      // if the prev value is above the edge threshold, and the current value is close to the table...
      if(h>0 && Pz_raw[i][h-1]>edge_threshold && Pz_raw[i][h]<edge_threshold)scan_y_end=vptr->y;    // if the end of an edge ... save position

      Pz[i][h]=Pz_raw[i][h];
      Pz_att[i][h]=1;	
      Pz_index++;
      printf("  Pz[%d]=%6.3f \n",h,Pz_raw[i][h]);
      }
    vptr=vptr->next;
    }
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}  // update display

  // clean up
  vptr=fstvptr_y;
  while(vptr!=NULL)
    {
    oldvptr=vptr;
    vptr=vptr->next;
    free(oldvptr); vertex_mem--;
    }

  // add on scan_dist buffer to found values
  scan_x_start -= scan_dist;
  scan_y_start -= scan_dist;
  scan_x_end   += scan_dist;
  scan_y_end   += scan_dist;

  // more clean up
  free(vstart); vertex_mem--; vstart=NULL;
  free(vfinish);vertex_mem--; vfinish=NULL;
  
  return(1);
}


// Function to read camera distance sensor via A2D on I2C
// note that this is very similar to the RTD read function
// this function is used for BOTH the physical touch probe and the IR sensor probe
float distance_sensor(int slot, int channel, int readings, int typ) 
{
  int 		i,cnt,dvc;
  uint8_t 	buf[4];
  int16_t 	val;
  float 	distZ,voltZ,avg_volt=0.0,max_volt=0.0;
    
  int		max_reads,old_cmd_state;
  float 	zht,oldz;
  
  // Gamma units only have one A2D
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    channel=3;
  #endif
  
  // imperical readings between Z dist and center of build table used to derive
  // curve fitting constants
  // 250mm = 0.468v
  // 200mm = 0.588v
  // 150mm = 0.862v
  // 100mm = 1.402v
  //  61mm = 2.425v
  
  // dist = 119.5294 * volts^(-.9246783)
  

  for(i=0;i<readings;i++)
      {

      // set config register for ADS1115 and start conversion
      // first 8 bits
      //		Bit		Value 	Comment
      //		15		1	Start Conversion
      //		14:12		100	Singled-ended input AIN0 (CHANNEL 0)
      //				101	Singled-ended input AIN1 (CHANNEL 1)
      //				110	Singled-ended input AIN2 (CHANNEL 2)
      //				111	Singled-ended input AIN3 (CHANNEL 3)
      //		11:9		000	FS 6.144v to cover the active range of interest of distance sensor
      //		8		1	Single shot mode
      // next 8 bits
      //		7:5		000	8 Samples per second (or 1 sample takes 1/8 of a second)
      //		4:2		000	Not relevant
      //		1:0		11	Disable comparator	
      //
      // 		Resulting in 11000001  00000011  or 0xc1 0x03  (for channel 0) as set below
    
      buf[0] = 1;   							// config register is 1
      buf[1] = 0xc1;							// first set default channel to 0 (see above)
      if(channel==0){dvc=0;buf[1]=0xc1;}				// tool slot 0
      if(channel==1){dvc=0;buf[1]=0xd1;}				// tool slot 1
      if(channel==2){dvc=0;buf[1]=0xe1;}				// tool slot 2
      if(channel==3){dvc=0;buf[1]=0xf1;}				// tool slot 3
      if(channel==4){dvc=1;buf[1]=0xc1;}				// channel 1 of build table
      if(channel==5){dvc=1;buf[1]=0xd1;}				// channel 2 of build table
      if(channel==6){dvc=1;buf[1]=0xe1;}				// chamber
      if(channel==7){dvc=1;buf[1]=0xf1;}				// (empty) spare
      buf[2] = 0x03;							// next set of params (see above)
      if(write(I2C_A2D_fd[dvc],buf,3) != 3)return(-100.0);		// failed to write to register
      delay(15);							// greater delay seems help quality of read

      // wait for conversion complete
      cnt=0;
      do
	{
	if(read(I2C_A2D_fd[dvc],buf,2) != 2)return(-101.0);		// failed to read conversion
	delay(15);							// greater delay seems help quality of read
	cnt++;
	if(cnt>30)break;						// bail out if stuck otherwise RPi will lock process status
	}while ((buf[0] & 0x80) == 0);					// keep reading until none left
      if(cnt>30)return(-102.0);						// fail if too many attempt to read


      // read conversion register
      buf[0]=0;								// conversion register is at 0
      if(write(I2C_A2D_fd[dvc],buf,1) != 1)return(-103.0);		// failed write register select
      if( read(I2C_A2D_fd[dvc],buf,2) != 2)return(-103.0);		// failed to read conversion

      // convert output and collect results
      val = (int16_t)buf[0]*256 + (uint16_t)buf[1];
      voltZ=(float)val*6.144/32768.0;					// values based on A2D's FS param setting
      if(voltZ>6.000)return(-104.0);					// out of range on high side
      if(voltZ<0.001)return(-104.0);					// out of range on low side
      if(voltZ>max_volt)max_volt=voltZ;					// filter to highest value found in readings
      avg_volt+=voltZ;
      delay(10);
      }

  // slightly non-linear curve fit established imperically
  avg_volt /= readings;
  distZ = (-78.2085)*pow(avg_volt,3) + 395.2662*pow(avg_volt,2) - 672.1709*avg_volt + 490.0;  // 3rd order
  //distZ = 119.5294 * pow(avg_volt,(-.9264783));			// power
  //distZ = 40.13 + 367.35 * exp(-1.49107*avg_volt);			// basic exponential
  //distZ= avg_volt*(-74.24) + 209.91;					// linear
    
  //printf("VOLTS=%f  DIST=%f \n",avg_volt,distZ);
    
  if(typ==0)return(avg_volt);
  return(distZ);
}


// Function to use touch probe switch for z height verification
// Input:  vertex that provides XY location to check
// Return:  z height measured at that location 
// note that the return values is ALWAYS POSITIVE since this function is measuring the down distance
// between where the tip is upon entry to the function to where it touches something.  by doing it this
// way the calibration of the touch probe itself becomes irrelevent.
float touch_probe(vertex *vptr, int slot)
{
  int		i,max_reads,old_cmd_state;
  float 	zht,oldz,newz;
  
  //printf("touch enter\n");
  
  // get current z location
  oldz=PostG.z;

  // drop Z toward build table while monitoring the tool's limit switch (similar to homing)
  old_cmd_state=cmdControl;
  cmdControl=0;
  max_reads=0;
  sprintf(gcode_cmd,"G1 Z-10.0 F100.0\n");				// slowly move tool toward build table
  tinyGSnd(gcode_cmd);
  while(RPi_GPIO_Read(AUX1_INPUT)==FALSE && max_reads<250){tinyGRcv(1);max_reads++;}	// constantly check tool limit switch during move - Unit 2
  if(max_reads>=250)printf("\n\nUnable to sense contact!  Aborting. \n\n");
  tinyGSnd("!\n");							// abort move when switch hit
  tinyGSnd("%\n");							// clear buffer - note: these cmds do not produce a response from tinyG
  cmdControl=old_cmd_state;
  
  // read new z where probe has stopped
  max_reads=0;
  for(i=0;i<5;i++)
    {
    newz=PostG.z;
    tinyGSnd("{\"posz\":NULL}\n");
    tinyGRcv(1);
    max_reads++;
    printf("  max_reads=%d\n",max_reads);
    }
    
  //zht=PostG.z-Tool[slot].tip_dn_pos;					// subtract off height of probe tool
  zht = oldz - newz;
  PosIs.z=PostG.z;
  
  //printf("touch exit\n");
  
  return(zht);
}


// Function to calculate the exact z offset based on xy position and table leveling data
// The X and Y inputs are expected to be in build table coords.  That is, add all necessary offsets 
// (model, tool, etc.) before hand.
float z_table_offset(float x, float y)
{
  int		xp,yp;
  float 	z01,z23,avg_z;
  
  // adjust z value based on level/flatness of build table at target XY position only if build table was scanned
  // note:  the build table z adjustments are fixed to the table by position.  the vertex values are being mapped over-top of them.
  // the idea is to locate the 4 surrounding build table adjust values to the incoming x/y, then bilinear interpolate to find the 
  // z adj exactly at the x/y position.
  avg_z=0.0;
  x-=scan_x_remain;							// account for offset - see scanning function formula
  y-=scan_y_remain;
  
  xp=floor(x/scan_dist);						// define the index into z offset array, always rounded down
  if(xp<0)xp=0;								// don't let exceed array bounds
  if(xp>(SCAN_X_MAX-2))xp=(SCAN_X_MAX-2);				// note the "+1" index into array below, hence "-2" not "-1"

  yp=floor(y/scan_dist);						// note these equations are the recipicol of what's in the scan function
  if(yp<0)yp=0;
  if(yp>(SCAN_Y_MAX-2))yp=(SCAN_Y_MAX-2);

  // handle the edge conditions
  // if at the lowest edge of x-axis... use 1 x value and 2 y values
  if(x<BUILD_TABLE_MIN_X)
    {
    xp=floor(BUILD_TABLE_MIN_X/scan_dist);
    avg_z= Pz[xp][yp] + (y-(yp+1)*scan_dist)*(Pz[xp][yp+1]-Pz[xp][yp]) / (scan_dist);
    }
  
  // if at the highest edge of x-axis... use 1 x value and 2 y values
  if(x>BUILD_TABLE_MAX_X)
    {
    xp=floor(BUILD_TABLE_MAX_X/scan_dist)-1;
    avg_z= Pz[xp][yp] + (y-(yp+1)*scan_dist)*(Pz[xp][yp+1]-Pz[xp][yp]) / (scan_dist);
    }
  
  // if at the lowest edge of y-axis... use 2 x values and 1 y value
  if(y<BUILD_TABLE_MIN_Y)
    {
    yp=floor(BUILD_TABLE_MIN_Y/scan_dist);
    avg_z= Pz[xp][yp] + (x-(xp+1)*scan_dist)*(Pz[xp+1][yp]-Pz[xp][yp]) / (scan_dist);
    }
  
  // if at the highest edge of y-axis...  use 2 x values and 1 y value
  if(y>BUILD_TABLE_MAX_Y)
    {
    yp=floor(BUILD_TABLE_MAX_Y/scan_dist)-1;
    avg_z= Pz[xp][yp] + (x-(xp+1)*scan_dist)*(Pz[xp+1][yp]-Pz[xp][yp]) / (scan_dist);
    }
  
  // if in the middle of the build table somewhere...  use 2 x values and 2 y values
  {
    // define who the four neighbors are, always need 4, to get bilinear interpolated value.
    // recall that the build_table_scan covers one point in each direction beyond min/max of the model
    // thus putting every point in the model in the middle of points that can be bilinearly interpolated
    z01=(Pz[xp][yp]*((xp*scan_dist+scan_dist)-x) + Pz[xp+1][yp]*(x-xp*scan_dist) ) / (scan_dist);		// 1st x interpolation
    z23=(Pz[xp][yp+1]*((xp*scan_dist+scan_dist)-x) + Pz[xp+1][yp+1]*(x-xp*scan_dist) ) / (scan_dist);	// 2nd x interpolation
    avg_z=(z01*((yp*scan_dist+scan_dist)-y) + z23*(y-yp*scan_dist) ) / (scan_dist);			// y interpolation bt x results
  }
  
  //printf("\nBld Tbl:  X=%f  Y=%f  xp=%d yp=%d \n",x+scan_dist/2,y+scan_dist/2,xp,yp);
  //printf("          z0=%6.3f z1=%6.3f z2=%6.3f z3=%6.3f  avg_z=%6.3f \n",Pz[xp][yp],Pz[xp+1][yp],Pz[xp+1][yp+1],Pz[xp][yp+1],avg_z);
  
  return(avg_z);
}
