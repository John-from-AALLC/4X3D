#include "Global.h"

// function to read RPi GPIO pin status
// some RPIs seem to "glich" now and then making a single direct read sketchy
// this function will do a multiple read and result with a rounded average of those reads
// returns TRUE if high, FALSE if low
int RPi_GPIO_Read(int pinID)
{
    int		i,max_reads,pin_state,pin_state_accum;
    float 	pin_state_avg;
  
    max_reads=3;
    pin_state_accum=0;
    for(i=0;i<max_reads;i++)
      {
      pin_state=gpioRead(pinID);	
      pin_state_accum+=pin_state;
      delay(25);
      }
    pin_state_avg=(float)pin_state_accum/(float)max_reads;
    pin_state=(int)floor(pin_state_avg+0.5);
    
    return(pin_state);
}

// function to load the 4X3D configuration file
int load_system_config(void)
{
  int	h,i,slot;
  int	tool_fan,tool_sel;
  int	copy_flag,send_flag;
  float dist;
  char	gstring[512];
  char 	*valstart;
  
  // read Config.json file for setup parameters and apply them
  gcode_in=fopen("Config.json","r");
  if(gcode_in==NULL)
    {
    tinyg_send=0;
    }
  else
    {
    //cmdCount=0;
    cmdBurst=1;
    cmdControl=TRUE;
    tinyg_send=1;
    job.state=UNDEFINED;
    crt_min_buffer=MAX_BUFFER-1;					// set to process only one cmd at a time
    printf("Running Config.json configuration instructions.\n");
    sprintf(job_status_msg," Configuring motion controller and homing axes ");
    gtk_label_set_text(GTK_LABEL(job_status_lbl),job_status_msg);
    gtk_widget_queue_draw(job_status_lbl);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
    gcode_out=fopen("setup.json","w");
    while(fgets(gcode_cmd,sizeof(gcode_cmd),gcode_in)!=NULL)
      {
      // skip if not sending output to controller
      if(tinyg_send==0)break;
	    
      // parse commands by type - note this is a custom format, some JSON, some custom - see file for details
      copy_flag=0;							// init to not copy anything
      send_flag=0;							// init to not send anything
      memset(gstring,0,sizeof(gstring));				// blank out command string
      for(i=0;i<strlen(gcode_cmd);i++)
	{
	if(gcode_cmd[i]==59)break;					// if ";" encountered, then ignore rest of line
	if(gcode_cmd[i]==123)						// if "{" encountered, then start copying input string
	  {	
	  h=0;								// ... set index to true command string to start
	  copy_flag=1;							// ... set flag to copy each char as loop goes by
	  }
	if(gcode_cmd[i]==125)						// if "}" encountered, then stop copying input string
	  {
	  gstring[h]=gcode_cmd[i];					// ... copy over the "}" to finish command string
	  gstring[h+1]=10;						// ... ensure command string has a line feed
	  gstring[h+2]=0;						// ... ensure command string is terminated
	  send_flag=1;							// ... set flag to indicate command pending
	  copy_flag=0;
	  break;							// ... ignore rest of line
	  }
	if(copy_flag)
	  {
	  gstring[h]=gcode_cmd[i];
	  h++;
	  }
	}
	
      // send a command and wait for tinyG to acknowledge that it is complete before sending the next one.
      if(send_flag)
	{
	if(strstr(gstring,"G28.2 X0"))printf("\n\nHoming X....\n");	// if X-axis homing request...
	if(strstr(gstring,"G28.2 Y0"))printf("\n\nHoming Y....\n");	// if Y-axis homing request...
	if(strstr(gstring,"G28.2 Z0"))printf("\n\nHoming Z....\n");	// if Z-axis limit homing request...
	if(strstr(gstring,"G28.3 Z0"))printf("\n\nHoming Z....\n");	// if Z-axis stationary homing request...
	strcat(gcode_burst,gstring);
	tinyGSnd(gcode_burst);						// send command
	memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
	
	valstart=strstr(scratch,"xvm");					// if x-axis pen up move velocity...
	if(valstart!=NULL)x_move_velocity=atof(valstart+5);		// ... extract for time estimate calcs
	valstart=strstr(scratch,"yvm");					// if y-axis pen up move velocity...
	if(valstart!=NULL)y_move_velocity=atof(valstart+5);		// ... extract for time estimate calcs
	valstart=strstr(scratch,"zvm");					// if z-axis pen up move velocity...
	if(valstart!=NULL)z_move_velocity=atof(valstart+5);		// ... extract for time estimate calcs
	
	if(strstr(gstring,"G1 Z10"))					// if Z up for clearance....
	  {
	  PostG.z=BUILD_TABLE_MAX_Z;
	  while(fabs(PostG.z-10.0)>0.001)tinyGRcv(1);
	  }
	if(strstr(gstring,"G28.2 X0"))					// if X-axis homing request...
	  {
	  PostG.x=BUILD_TABLE_MAX_X;
	  while(fabs(PostG.x)>0.001)tinyGRcv(1);
	  }
	if(strstr(gstring,"G28.2 Y0"))					// if Y-axis homing request...
	  {
	  PostG.y=BUILD_TABLE_MAX_Y;
	  while(fabs(PostG.y)>0.001)tinyGRcv(1);
	  }
	if(strstr(gstring,"G28.2 Z0"))					// if Z-axis limit homing request...
	  {
	  PostG.z=BUILD_TABLE_MAX_Z;
	  while(fabs(PostG.z)>0.001)tinyGRcv(1);
	  }
	if(strstr(gstring,"G28.3 Z0"))					// if Z-axis stationary homing request...
	  {
	  while(tinyGRcv(1)>=0);
	  }
	while(cmdCount>0)						// wait for command to process
	  {
	  while(tinyGRcv(1)==0);
	  }
	send_flag=0;							// re-init to no command ready to send
	}
      }
    if(ferror(gcode_in))
      {
      printf("/nError - unable to properly read config.json file data!\n");
      }
    fclose(gcode_out);
    gcode_out=NULL;							// ensure residual pointer is NULLed out for future checks
    }
  fclose(gcode_in);
  gcode_in=NULL;							// ensure residual pointer is NULLed out for future checks
  
  // code to adjust z such that build table is near "zero"
  printf("\nSetting build table height...\n");
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)				// alpha and beta units have home near z=minimum
    sprintf(gcode_cmd,"G1 Z%7.3f F300.0 \n",ZHOMESET);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    sprintf(gcode_cmd,"G28.3 Z0.0 \n");
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
  #endif

  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)			// gamma and delta units have home near z=maximum
    sprintf(gcode_cmd,"G28.3 Z%7.3f \n",ZHOMESET);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    sprintf(gcode_cmd,"G0 X0.0 Y0.0 Z%7.3f \n",(ZHOMESET-10));
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);						// get tinyg feedback
    PosIs.z=PostG.z;
  #endif
  
  //carriage_at_home=TRUE;						// set flag as "at home"
  goto_machine_home();
  crt_min_buffer=MIN_BUFFER;						// set to process at full throttle

  return(1);
}

// Callback to set response for unit calibration
void cb_unit_calib_response(GtkAlertDialog *dialog, GAsyncResult *response_id, gpointer user_data)
{
    gtk_alert_dialog_choose_finish (dialog, response_id, NULL);
    gtk_window_close(GTK_WINDOW(dialog));
    return;
}

// function to run a unit level calibration
// this will establish the level/planarity of the build table and determine the unique aspects (z height and temp offset)
// for each carriage slot.
int unit_calibration(void)
{
  // flow:
  // loop thru each slot and:
  // 	prompt user to plug in probe tool
  // 	read temperature compared to 158 ohm precision resistor (equates to 300C)
  //	home the slot with the tool in it
  // 	move the slot to the z deposit position
  // 	probe 4 corners and middle of build plate with both camera and probe tool
  // based on difference bt camera and probe tool set camera calib values
  // based on left/right z locations determine if z leads need adjustment
  // based on front/back z locations determine if bld table needs adjustment
  // if adjustments were made, repeat test from beginning
  // record z delta for each slot
  // record thermal offset for each slot
   
  int		i,w1_trys,slot;
  char		info_msg[500];
  float 	prec_temp;
  float 	x_delta[MAX_TOOLS],y_delta[MAX_TOOLS];
  float 	z_touch[MAX_TOOLS][4],z_camera[MAX_TOOLS][4];
  vertex 	*vptr;
  GtkWidget 	*win_dialog,*dialog;
  GtkWidget	*win_tc_stat,*grd_tc_stat;
  GtkWidget	*lbl_da,*lbl_scratch,*goff_check;
  
  // initialize
  vptr=vertex_make();
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    x_delta[slot]=0.0; 
    y_delta[slot]=0.0; 
    crgslot[slot].x_offset=0.0;
    crgslot[slot].y_offset=0.0;
    crgslot[slot].z_offset=0.0;
    crgslot[slot].temp_offset=0.0; 
    for(i=0;i<4;i++){z_touch[slot][i]=0.0;}
    }
  
  // loop thru all carriage slots and collect data
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    // prompt user to insert probe tool
    sprintf(scratch,"\nInsert PROBE tool into slot %d\n",(slot+1));
    aa_dialog_box(win_settings,2,0,"Unit Calibration",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==0)return(0);
    
    // check tool, then home it, then set it into position
    w1_trys=0;
    printf("UC: Slot=%d Tool.name=%s Tool.mmy=%s\n",slot,Tool[slot].name,Tool[slot].mmry.md.name);
    while(Tool[slot].tool_ID!=TC_PROBE)
      {
      make_toolchange(slot,1);
      delay(1500);
      Open1Wire(slot);
      make_toolchange(slot,-1);
      w1_trys++;
      if(w1_trys>5)
        {
	sprintf(scratch,"\nPROBE tool not recognized - aborting.\n",(slot+1));
	aa_dialog_box(win_settings,2,0,"Unit Calibration",scratch);
	while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
	break;
	}
      }
    if(Tool[slot].tool_ID==TC_PROBE)
      {
      // execute the tip placement - note we can NOT use "Tool[slot].tip_dn_pos" because that alredy includes
      // a carriage offset.  we want this test to be done without any carriage offset applied.
      tool_tip_home(slot);						// find home
      make_toolchange(slot,2);						// turn on carriage stepper drive
      sprintf(gcode_cmd,"G1 A%07.3f F%07.3f \n",12.0,500.0);		// build move command to lift tool tip
      tinyGSnd(gcode_cmd);						// send move command
      motion_complete();						// ensure it has reached its destination
      Tool[slot].tip_cr_pos=12.0;					// set current tool tip position to height
      make_toolchange(-1,0);						// turn off all steppers to prevent overheating
      
      // move thru positions and record z at each
      sprintf(scratch,"\nProbing build table corners.\nPlease wait...\n");
      aa_dialog_box(win_settings,0,10,"Unit Calibration",scratch);
      
      //vptr->x=BUILD_TABLE_LEN_X/2; vptr->y=BUILD_TABLE_LEN_Y/2; vptr->z=5.0;
      //z_touch[slot][0]=touch_probe(vptr,slot);
      //vptr->x=BUILD_TABLE_LEN_X/2; vptr->y=BUILD_TABLE_LEN_Y/2; vptr->z=crgslot[slot].camera_focal_dist;
      //print_vertex(vptr,(-1),0);
      //tinyGSnd(gcode_burst);						// execute pen up move
      //memset(gcode_burst,0,1024);					// clear command string
      //motion_complete();
      //delay(500);
      //z_camera[slot][0]=distance_sensor(3,3);
      //printf("\nZ Table Center:  \n");
      //printf("  target:  	x=%6.3f y=%6.3f \n",vptr->x,vptr->y);
      //printf("  probe offset:	x=%6.3f y=%6.3f \n",Tool[slot].x_offset,Tool[slot].y_offset);
      //printf("  probe_z=%6.3f  camera_z=%6.3f\n",z_touch[slot][0],z_camera[slot][0]);
      //printf("Any key to continue...\n");
      //while(!kbhit());
      
      vptr->x=0.0; vptr->y=0.0; vptr->z=3.0;
      z_touch[slot][0]=touch_probe(vptr,slot)-Tool[slot].mmry.md.tip_pos_Z;
      //vptr->x=0.0; vptr->y=0.0; vptr->z=5.0;
      //print_vertex(vptr,(-1),0);
      //tinyGSnd(gcode_burst);						// execute pen up move
      //memset(gcode_burst,0,1024);					// clear command string
      //motion_complete();
      //delay(100);
      //z_camera[slot][0]=distance_sensor(3,3);
      printf("\nZ Distance corner 1:  probe=%f  camera=%f\n",z_touch[slot][0],z_camera[slot][0]);
      //printf("Any key to continue...\n");
      //while(!kbhit());
      
      vptr->x=0.0; vptr->y=BUILD_TABLE_LEN_Y-10; vptr->z=3.0;
      z_touch[slot][1]=touch_probe(vptr,slot)-Tool[slot].mmry.md.tip_pos_Z;
      //vptr->x=0.0; vptr->y=BUILD_TABLE_LEN_Y-15; vptr->z=crgslot[slot].camera_focal_dist;
      //print_vertex(vptr,(-1),0);
      //tinyGSnd(gcode_burst);						// execute pen up move
      //memset(gcode_burst,0,1024);					// clear command string
      //motion_complete();
      //delay(100);
      //z_camera[slot][1]=distance_sensor(3,3);
      printf("\nZ Distance corner 2:  probe=%f  camera=%f\n",z_touch[slot][1],z_camera[slot][1]);
      //printf("Any key to continue...\n");
      //while(!kbhit());
      
      vptr->x=BUILD_TABLE_LEN_X-15; vptr->y=BUILD_TABLE_LEN_Y-10; vptr->z=3.0;
      z_touch[slot][2]=touch_probe(vptr,slot)-Tool[slot].mmry.md.tip_pos_Z;
      //vptr->x=BUILD_TABLE_LEN_X-15; vptr->y=BUILD_TABLE_LEN_Y-15; vptr->z=crgslot[slot].camera_focal_dist;
      //print_vertex(vptr,(-1),0);
      //tinyGSnd(gcode_burst);						// execute pen up move
      //memset(gcode_burst,0,1024);					// clear command string
      //motion_complete();
      //delay(100);
      //z_camera[slot][2]=distance_sensor(3,3);
      printf("\nZ Distance corner 3:  probe=%f  camera=%f\n",z_touch[slot][2],z_camera[slot][2]);
      //printf("Any key to continue...\n");
      //while(!kbhit());
      
      vptr->x=BUILD_TABLE_LEN_X-10; vptr->y=0.0; vptr->z=3.0;
      z_touch[slot][3]=touch_probe(vptr,slot)-Tool[slot].mmry.md.tip_pos_Z;
      //vptr->x=BUILD_TABLE_LEN_X-15; vptr->y=0.0; vptr->z=crgslot[slot].camera_focal_dist;
      //print_vertex(vptr,(-1),0);
      //tinyGSnd(gcode_burst);						// execute pen up move
      //memset(gcode_burst,0,1024);					// clear command string
      //motion_complete();
      //delay(100);
      //z_camera[slot][3]=distance_sensor(3,3);
      printf("\nZ Distance corner 4:  probe=%f  camera=%f\n",z_touch[slot][3],z_camera[slot][3]);
      //printf("Any key to continue...\n");
      //while(!kbhit());
      
      // calculate averaged probe z offset
      x_delta[slot]=((z_touch[slot][3]-z_touch[slot][0])+(z_touch[slot][2]-z_touch[slot][1]))/2;
      y_delta[slot]=((z_touch[slot][1]-z_touch[slot][0])+(z_touch[slot][2]-z_touch[slot][3]))/2;
      crgslot[slot].z_offset=(z_touch[slot][0]+z_touch[slot][1]+z_touch[slot][2]+z_touch[slot][3])/4;
      
      // calculate averaged camera z offset
      crgslot[slot].camera_z_offset=(z_camera[slot][0]+z_camera[slot][1]+z_camera[slot][2]+z_camera[slot][3])/4;
      crgslot[slot].camera_z_offset-=crgslot[slot].camera_focal_dist;	// nominal dist between build table and camera when z=0.000

      // move carriage back to home
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X0.0 Y0.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      
      // record the temperature and voltage offsets
      prec_temp=Tool[slot].thrm.tempC;					// acquire temp INDEPENDENT of crgslot offset
      crgslot[slot].temp_offset=152.0-prec_temp;			// result of precision resistor
      if(fabs(crgslot[slot].temp_offset)>100.0)crgslot[slot].temp_offset=0.0;
      crgslot[slot].volt_offset=Tool[slot].thrm.tempV;			// acquire voltage offset
      gtk_window_close(GTK_WINDOW(win_dialog));
      printf(" slot=%d  temp=%6.3f  volts=%6.3f \n",slot,prec_temp,crgslot[slot].volt_offset);
      
      // post results
      sprintf(info_msg,"\nSlot	   		1		2		3		4\n");
      sprintf(scratch,"Temp	%+8.1f	%+8.1f	%+8.1f	%+8.1f\n",crgslot[0].temp_offset, crgslot[1].temp_offset, crgslot[2].temp_offset, crgslot[3].temp_offset);
      strcat(info_msg,scratch);
      sprintf(scratch,"Volts	%+8.3f	%+8.3f	%+8.3f	%+8.3f\n",crgslot[0].volt_offset, crgslot[1].volt_offset, crgslot[2].volt_offset, crgslot[3].volt_offset);
      strcat(info_msg,scratch);
      sprintf(scratch,"ZDepth 	%+8.3f	%+8.3f	%+8.3f	%+8.3f\n",crgslot[0].z_offset, crgslot[1].z_offset, crgslot[2].z_offset, crgslot[3].z_offset);
      //sprintf(scratch,"ZDepth 	%+8.3f	%+8.3f	%+8.3f	%+8.3f\n",z_touch[slot][0],z_touch[slot][1],z_touch[slot][2],z_touch[slot][3]);
      strcat(info_msg,scratch);
      //sprintf(scratch,"Z Cam	%+8.3f	%+8.3f	%+8.3f	%+8.3f\n",z_camera[slot][0],z_camera[slot][1],z_camera[slot][2],z_camera[slot][3]);
      //strcat(info_msg,scratch);
      aa_dialog_box(win_settings,1,0,"Unit Calibration",scratch);
      while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
      }
    }
    
  free(vptr);vertex_mem--;
  
  save_unit_calibration();
  return(1);
}

// Function to save unit calibration data
int save_unit_calibration(void)
{
  int	slot;
  
  // save data to file
  unit_calib=fopen("4X3D.calib","w");
  if(unit_calib!=NULL)
    {
    // write calib file version
    sprintf(calib_file_rev,"20230725");					// set current rev of calib file
    fwrite(&calib_file_rev,1,sizeof(calib_file_rev),unit_calib);
      
    // write status of each carriage slot
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      fwrite(&crgslot[slot].x_center,1,sizeof(crgslot[slot].x_center),unit_calib);
      fwrite(&crgslot[slot].y_center,1,sizeof(crgslot[slot].y_center),unit_calib);
      fwrite(&crgslot[slot].x_offset,1,sizeof(crgslot[slot].x_offset),unit_calib);
      fwrite(&crgslot[slot].y_offset,1,sizeof(crgslot[slot].y_offset),unit_calib);
      fwrite(&crgslot[slot].z_offset,1,sizeof(crgslot[slot].z_offset),unit_calib);
      fwrite(&crgslot[slot].camera_focal_dist,1,sizeof(crgslot[slot].camera_focal_dist),unit_calib);
      fwrite(&crgslot[slot].camera_z_offset,1,sizeof(crgslot[slot].camera_z_offset),unit_calib);
      fwrite(&crgslot[slot].temp_offset,1,sizeof(crgslot[slot].temp_offset),unit_calib);
      fwrite(&crgslot[slot].volt_offset,1,sizeof(crgslot[slot].volt_offset),unit_calib);
      fwrite(&crgslot[slot].calib_t_low,1,sizeof(crgslot[slot].calib_t_low),unit_calib);
      fwrite(&crgslot[slot].calib_v_low,1,sizeof(crgslot[slot].calib_v_low),unit_calib);
      fwrite(&crgslot[slot].calib_t_high,1,sizeof(crgslot[slot].calib_t_high),unit_calib);
      fwrite(&crgslot[slot].calib_v_high,1,sizeof(crgslot[slot].calib_v_high),unit_calib);
      }

    // write build table calibration
    fwrite(&Tool[BLD_TBL1].thrm.calib_t_low,1,sizeof(Tool[BLD_TBL1].thrm.calib_t_low),unit_calib);
    fwrite(&Tool[BLD_TBL1].thrm.calib_v_low,1,sizeof(Tool[BLD_TBL1].thrm.calib_v_low),unit_calib);
    fwrite(&Tool[BLD_TBL1].thrm.calib_t_high,1,sizeof(Tool[BLD_TBL1].thrm.calib_t_high),unit_calib);
    fwrite(&Tool[BLD_TBL1].thrm.calib_v_high,1,sizeof(Tool[BLD_TBL1].thrm.calib_v_high),unit_calib);
    fwrite(&Tool[BLD_TBL1].thrm.temp_offset,1,sizeof(Tool[BLD_TBL1].thrm.temp_offset),unit_calib);

    fwrite(&Tool[BLD_TBL2].thrm.calib_t_low,1,sizeof(Tool[BLD_TBL2].thrm.calib_t_low),unit_calib);
    fwrite(&Tool[BLD_TBL2].thrm.calib_v_low,1,sizeof(Tool[BLD_TBL2].thrm.calib_v_low),unit_calib);
    fwrite(&Tool[BLD_TBL2].thrm.calib_t_high,1,sizeof(Tool[BLD_TBL2].thrm.calib_t_high),unit_calib);
    fwrite(&Tool[BLD_TBL2].thrm.calib_v_high,1,sizeof(Tool[BLD_TBL2].thrm.calib_v_high),unit_calib);
    fwrite(&Tool[BLD_TBL2].thrm.temp_offset,1,sizeof(Tool[BLD_TBL2].thrm.temp_offset),unit_calib);

    // write chamber calibration
    fwrite(&Tool[CHAMBER].thrm.calib_t_low,1,sizeof(Tool[CHAMBER].thrm.calib_t_low),unit_calib);
    fwrite(&Tool[CHAMBER].thrm.calib_v_low,1,sizeof(Tool[CHAMBER].thrm.calib_v_low),unit_calib);
    fwrite(&Tool[CHAMBER].thrm.calib_t_high,1,sizeof(Tool[CHAMBER].thrm.calib_t_high),unit_calib);
    fwrite(&Tool[CHAMBER].thrm.calib_v_high,1,sizeof(Tool[CHAMBER].thrm.calib_v_high),unit_calib);
    fwrite(&Tool[CHAMBER].thrm.temp_offset,1,sizeof(Tool[CHAMBER].thrm.temp_offset),unit_calib);

    fflush(unit_calib);
    fclose(unit_calib);
    }
  
  return(1);
}

// Function to level z table
int ztable_calibration(void)
{
  int		slot,in_level=FALSE;
  char		info_msg[500],probe_msg[500];
  float 	z_touch[4],zleft,zright,zturns;
  vertex 	*vptr;
  GtkWidget 	*win_local_dialog;
  GtkWidget	*win_tc_stat,*grd_tc_stat;
  GtkWidget	*lbl_da,*lbl_scratch,*goff_check;
  GtkAlertDialog *local_alert_dialog;
  
  // use tool slot 1
  slot=0;
  vptr=vertex_make();
  GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    
  // check tool, then home it, then set it into position
  if(Tool[slot].tool_ID!=TC_PROBE)
    {
    sprintf(scratch,"\nPROBE tool not recognized - aborting.\n",(slot+1));
    aa_dialog_box(win_settings,1,0,"Table Z Calibration",scratch);
    }
  else 
    {
    set_holding_torque(ON);						// turn on motors
    tool_tip_home(slot);						// find home
    tool_tip_deposit(slot);						// move into z position
    make_toolchange(-1,0);						// turn off all steppers to prevent overheating
      
    sprintf(probe_msg,"\nProbing corner 1.\nPlease wait...\n");
    aa_dialog_box(win_settings,0,(-1),"Table Z Calibration",probe_msg);
    vptr->x=0.0; vptr->y=0.0; vptr->z=3.0;
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    print_vertex(vptr,slot,2,NULL);					// build cmd to move into position
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();
    z_touch[0]=touch_probe(vptr,slot);					// drop final 10mm to see where table is
    printf("\nZ Distance corner 1:  %f \n",z_touch[0]);
    
    sprintf(probe_msg,"\nProbing corner 2.\nPlease wait...\n");
    aa_dialog_box(win_settings,0,(-1),"Table Z Calibration",probe_msg);
    vptr->x=0.0; vptr->y=BUILD_TABLE_LEN_Y-10; vptr->z=3.0;
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    print_vertex(vptr,slot,2,NULL);					// build cmd to move into position
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();
    z_touch[1]=touch_probe(vptr,slot);
    printf("\nZ Distance corner 2:  %f \n",z_touch[1]);
    
    sprintf(probe_msg,"\nProbing corner 3.\nPlease wait...\n");
    aa_dialog_box(win_settings,0,(-1),"Table Z Calibration",probe_msg);
    vptr->x=BUILD_TABLE_LEN_X-10; vptr->y=BUILD_TABLE_LEN_Y-10; vptr->z=3.0;
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    print_vertex(vptr,slot,2,NULL);					// build cmd to move into position
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();
    z_touch[2]=touch_probe(vptr,slot);
    printf("\nZ Distance corner 3:  %f \n",z_touch[2]);
    
    sprintf(probe_msg,"\nProbing corner 4.\nPlease wait...\n");
    aa_dialog_box(win_settings,0,(-1),"Table Z Calibration",probe_msg);
    vptr->x=BUILD_TABLE_LEN_X-10; vptr->y=0.0; vptr->z=3.0;
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    print_vertex(vptr,slot,2,NULL);					// build cmd to move into position
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();
    z_touch[3]=touch_probe(vptr,slot);
    printf("\nZ Distance corner 4:  %f \n",z_touch[3]);
      
    // calculate adjustments
    zleft=(z_touch[0]+z_touch[3])/2;
    zright=(z_touch[1]+z_touch[2])/2;
    if(fabs(zleft-zright)<0.10)
      {
      sprintf(info_msg,"\n Looks good!\n The z table is level within %6.3f mm\n",(zright-zleft));
      in_level=TRUE;
      }
    else
      {
      // 4.0mm per turn in z
      zturns=fabs(zright-zleft)/4.0;
      if(zleft<zright)
	{
	//sprintf(info_msg,"\n The z table is high on the right by %6.3f\n",(zright-zleft));
	sprintf(info_msg,"\n Left: %6.3f    Right: %6.3f\n",zleft,zright);
	sprintf(probe_msg,"\n Turn the right screw %4.2f turns clockwise.\n",(zturns));
	strcat(info_msg,probe_msg);
	in_level=FALSE;
	}
      else
	{
	//sprintf(info_msg,"\n The z table is low on the right by %6.3f\n",(zleft-zright));
	sprintf(info_msg,"\n Left: %6.3f    Right: %6.3f\n",zleft,zright);
	sprintf(probe_msg,"\n Turn the right screw %4.2f turns counterclockwise.\n",(zturns));
	strcat(info_msg,probe_msg);
	in_level=FALSE;
	}
      }

    // post results
    aa_dialog_box(win_settings,1,0,"Table Z Calibration",info_msg);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    }
    
  // move carriage back to home
  goto_machine_home();
  
  free(vptr);vertex_mem--;
  
  return(1);
}

// Function to save unit state to file
// The purpose of this file is to provide history information in the event of a restart.
int save_unit_state(void)
{
  char		out_data[255];
  model		*mptr;
  
  // open the file
  unit_state=fopen("4X3D.stat","w");
  if(unit_state!=NULL)
    {
    // save system information
    sprintf(out_data,"Job Count Total = %d \n",job_count_total);
    fwrite(out_data,1,strlen(out_data),unit_state);
    sprintf(out_data,"Job Count Fail = %d \n",job_count_fail);
    fwrite(out_data,1,strlen(out_data),unit_state);
    
    // save job information
    // job copies, etc.
    // model information
    // tool information
    // slice information
    
    // save status information
    // current z level
    
    // save job information
    // note that certain items, such as pointers, will become invalid
    fwrite(&job,sizeof(job),1,unit_state);
    
    // save model information
    
  
    }
  fclose(unit_state);
  
  return(TRUE);
}

// Function to load unit state to file
int load_unit_state(void)
{
  char		in_data[255],val_string[255];
  char	 	*valstart,*valend;
  int		i,h,copy_flag, send_flag;
  
  // open the file
  unit_state=fopen("4X3D.stat","r");
  if(unit_state!=NULL)
    {
    while(fgets(in_data,sizeof(in_data),unit_state)!=NULL)
      {
      copy_flag=0;							// init to not copy anything
      send_flag=0;							// init to not send anything
      memset(val_string,0,sizeof(val_string));				// blank out command string
      for(i=0;i<strlen(in_data);i++)
	{
	if(in_data[i]==59)break;					// if ";" encountered, then ignore rest of line
	if(in_data[i]==123)						// if "{" encountered, then start copying input string
	  {	
	  h=0;								// ... set index to true command string to start
	  copy_flag=1;							// ... set flag to copy each char as loop goes by
	  }
	if(in_data[i]==125)						// if "}" encountered, then stop copying input string
	  {
	  val_string[h]=in_data[i];					// ... copy over the "}" to finish command string
	  val_string[h+1]=10;						// ... ensure command string has a line feed
	  val_string[h+2]=0;						// ... ensure command string is terminated
	  send_flag=1;							// ... set flag to indicate command pending
	  copy_flag=0;
	  break;							// ... ignore rest of line
	  }
	if(copy_flag)
	  {
	  val_string[h]=in_data[i];
	  h++;
	  }
	}
      if(send_flag)
	{
	valstart=strstr(val_string,"Job Count Total");
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"=");
	  job_count_total=atoi(valend+1);
	  continue;
	  }
	valstart=strstr(val_string,"Job Count Fail");
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"=");
	  job_count_fail=atoi(valend+1);
	  continue;
	  }
	
	// load system information
	
	// load job information
	
	// load status information
	}
      }
    }
  if(ferror(unit_state))
    {
    printf("/nError - unable to properly read system state file data!\n");
    }
  fclose(unit_state);
  

  //result=fread(&job,sizeof(job),1,job);  
  
  
  return(TRUE);
}

// Function to open system log file
// The purpose of the system log file is to provide a history of the machine's use.  It is a summary
// of all the models/jobs run along with user interactions (tool changes, calibrations, etc.)
int open_system_log(void)
{
  system_log=fopen("system.log","a");
  if(system_log==NULL)
    {
    printf("*** ERROR *** - Unable to open system log file. \n");
    return(FALSE);
    }
  return(TRUE);
}

// Function to close system log file
int close_system_log(void)
{
  fflush(system_log);
  fclose(system_log);
  return(TRUE);
}

// Function to add entry to system log file.
// Note that this should only be called from within the same thread.  Do NOT call from either the
// print or thermal threads.
int entry_system_log(char *in_data)
{
  char		out_data[335];
  time_t	curtime;
  struct	tm *loc_time;
  
  // open system log file
  if(open_system_log()==FALSE)return(FALSE);
  
  // build time stamp
  curtime=time(NULL);
  loc_time=localtime(&curtime);
  memset(out_data,0,sizeof(out_data));
  strftime(out_data,80,"\n%x %X ",loc_time);
  
  // verify inbound data
  if(strlen(in_data)<1)return(FALSE);
  if(strlen(in_data)>255)return(FALSE);
  
  // build outbound data and make entry
  strcat(out_data,in_data);
  fwrite(out_data,1,strlen(out_data),system_log);
  fflush(system_log);
  
  // close log file
  close_system_log();
  
  return(TRUE);
}

// Function to open model log files
// The purpose of model log files are to provide a semi-detailed history of the model build.  This
// data is primarily directed toward being used as a tool for improvement either manually or thru 
// deep learning.
//
// The idea is to include enough detail from the system to build an understand of what effects the
// speed and quality of build without overwhelming the system using it.  That being said, here are
// the variables that should go into this log file:
//
//	Input parameters:
//	- Size of model: deltaX, deltaY, deltaZ
//	- Material configuration file and all its parameters
//	
//	Automatically calculated parameters:
//	- Number of layers in this model
//	- Layer thickness
//	- Perimeter thickness
//	- Fill density
//	- Time estimate for model and support
//	- Volume estimate for model and support
//
//	Automatically measured parameters:
//	- Start time and date
//	- Finish time and date
// 	- Average tool hot end temperature over duration of build
//	- Deviation of tool hot end temperature over duration of build
//	- Average build table temperature over duration of build
//	- Deviation of build table temperature over duration of build
//	- Average chamber temperature over duration of build
//	- Deviation of chamber temperature over duration of build
//	- Area of model material in contact with build table
//	- Aspect ratio (x vs y) of model material in contact with build table
//	- Area of support structure in contact with build table
//	- Aspect ratio (x vs y) of support structure in contact with build table
//	- Average volume of model material per layer
//	- Deviation of volume of model material per layer
//	- Average volume of support structure per layer
//	- Deviation of volume of support structure per layer
//	- Actual build time
//	- Did the model complete
//
//	Human assessment parameters: 0=poor / 5=excellent
//	- Surface finish of vertical walls
//	- Surface finish of bottom layer
//	- Surface finish of lower close-offs
//	- Surface finish of upper close-offs
//	- Did the model curl at all
//	- Dimensions of model: deltaX, deltaY, deltaZ

int open_model_log(void)
{
  char		mdl_name[255];
  int		h,i;
  model 	*mptr;
  
  if(job.model_first==NULL)return(FALSE);
  mptr=job.model_first;
  
  // build simplified model name for file name
  h=0;
  memset(mdl_name,0,sizeof(mdl_name));
  for(i=0;i<strlen(mptr->model_file);i++)if(mptr->model_file[i]==41)h=i;	// find the last "/" in file name structure
  if(h>0)
    {
    i=0; h++;
    while(h<strlen(mptr->model_file))
      {mdl_name[i]=mptr->model_file[h]; i++; h++; if(i>40)break;}
    }
  else 
    {strncpy(mdl_name,mptr->model_file,40);}
  
  // open file for write
  model_log=fopen(mdl_name,"w");
  if(model_log==NULL)
    {
    printf("*** ERROR *** - Unable to open model log file. \n");
    return(FALSE);
    }
  return(TRUE);
}

// Function to close model log file
int close_model_log(void)
{
  fflush(model_log);
  fclose(model_log);
  return(TRUE);
}


// Function to open job log files
// The purpose of job log files are to provide the most detailed history of the last build done on
// the unit.  They consist of two files:  what was issued and what was the response to that command.
int open_job_log(void)
{
  time_t	curtime;
  struct	tm *loc_time;
  
  proc_log=fopen("process.log","w");
  if(proc_log==NULL)return(0);
  
  gcode_out=fopen("job.log","w");					// log of tinyG replies received from last model run
  if(gcode_out==NULL)return(0);

  gcode_gen=fopen("cmd.log","w");					// log of commands issued to build the last model run
  if(gcode_gen==NULL)return(0);
  
  curtime=time(NULL);
  loc_time=localtime(&curtime);
  
  strftime(scratch,80,"Date: %x \n",loc_time);
  fwrite(scratch,1,strlen(scratch),proc_log);  fflush(proc_log);
  fwrite(scratch,1,strlen(scratch),gcode_out); fflush(gcode_out);
  fwrite(scratch,1,strlen(scratch),gcode_gen); fflush(gcode_gen);
  
  strftime(scratch,80,"Time: %X \n", loc_time);
  fwrite(scratch,1,strlen(scratch),proc_log);  fflush(proc_log);
  fwrite(scratch,1,strlen(scratch),gcode_out); fflush(gcode_out);
  fwrite(scratch,1,strlen(scratch),gcode_gen); fflush(gcode_gen);
  
  return(1);
}

// Function to close job log files
int close_job_log(void)
{
  time_t	curtime;
  struct	tm *loc_time;
  int		add_time_stamp=TRUE;
  char 		stamp[80];
  
  curtime=time(NULL);
  loc_time=localtime(&curtime);
  memset(stamp,0,sizeof(stamp));
  
  if(add_time_stamp==TRUE)
    {
    curtime=time(NULL);
    loc_time=localtime(&curtime);
    strftime(stamp,80,"\n%x %X ",loc_time);
    strcat(stamp,"End of file.\n");
    }
  
  if(proc_log!=NULL)
    {
    fwrite(stamp,1,strlen(stamp),proc_log);
    fflush(proc_log);
    fclose(proc_log);
    proc_log=NULL;
    }
  if(gcode_out!=NULL)
    {
    fwrite(stamp,1,strlen(stamp),gcode_out);
    fflush(gcode_out);
    fclose(gcode_out);
    gcode_out=NULL;
    }
  if(gcode_gen==NULL)
    {
    fwrite(stamp,1,strlen(stamp),gcode_gen);
    fflush(gcode_gen);
    fclose(gcode_gen);
    gcode_gen=NULL;
    }
  
  return(1);
}

// Function to add entry to system log file. 
// Note that this should only be called from within the same thread.  Do NOT call from either the
// print or thermal threads.
int entry_process_log(char *in_data)
{
  int		add_time_stamp=TRUE;
  char		out_data[335];
  time_t	curtime;
  struct	tm *loc_time;
  
  if(proc_log==NULL)return(FALSE);
  
  memset(out_data,0,sizeof(out_data));

  // build time stamp
  if(add_time_stamp==TRUE)
    {
    curtime=time(NULL);
    loc_time=localtime(&curtime);
    strftime(out_data,80,"\n%x %X ",loc_time);
    }
      
  // verify inbound data
  if(strlen(in_data)<1)return(FALSE);
  if(strlen(in_data)>255)return(FALSE);
  
  // build outbound data and make entry
  strcat(out_data,in_data);
  fwrite(out_data,1,strlen(out_data),proc_log);
  fflush(system_log);
  
  //printf("ProcLog: %s \n",out_data);
  
  return(TRUE);
}


// Function to issue commands to tinyG in controlled manner
// The goal is to keep commands flowing in such a manner that motion is continuous and smooth.  This
// is limited by the size of the receive buffer on the tinyG (versus the command queue size) and the
// speed at which this code runs and each command is processed in the machine.
//
// The basic idea it to fill the rcv buffer as fast as possible when starting out and keep track of 
// how full it is by watching tinyG responses (the 3rd param of an "r" response shows how many bytes
// were pulled for each command processed, or in other words how many bytes just opened up).  Then
// when there is room, stuff the next command into the buffer to keep it as full as possible.
//
// Inputs:	cmd=pointer to command string
// Return:	0=failure, 1=success
int tinyGSnd(char *cmd)
{
	char	*stp,pd,gc_out[255];					// sizeof gc_out must match cmd_buffer string size
	int 	i,j,n,cmd_skip,max_wait,rx;
	int	old_cmdCount;
	int	err_chk=(-1);
	
	//printf("tinyGSnd: ");
	
	if(strlen(cmd)<=0)return(1);					// if nothing to send, just return

	// if an abort or clear command, process immediately regardless of buffer status so skip checking and waiting
	if(strstr(gc_out,"!")==NULL && strstr(gc_out,"%")==NULL)	// If neither exists in request...
	  {
	  // If available command buffer space is low, just poll until some commands are processed
	  while(bufferAvail < MIN_BUFFER){tinyGRcv(1);}
  
	  // If available receive buffer space is low, just poll until some commands are processed
	  while(Rx_bytes > (Rx_max-strlen(cmd)-10)){tinyGRcv(1);}
	  }
	
	idle_start_time=time(NULL);					// reset idle time
	
	// write command to gcode file if requested
	//if(gcode_gen!=NULL && gcode_send==1)fwrite(cmd,1,strlen(cmd),gcode_gen);
	
	// send command to tinyG motion controller one line at a time.
	// copy each line (command) sent to tinyG into the cmd_buffer structure.
	// the variabel cmdCount keeps track of how many are currently pending in tinyG.
	if(tinyg_send==TRUE)
	  {
	  n=0;								// init position in command string
	  memset(gc_out,0,sizeof(gc_out));				// clear output string
	  for(i=0;i<strlen(cmd);i++)					// build output string one char at a time - allows full control of stream
	    {
	    pd=cmd[i];							// get char off command string
	    if(pd<0 || pd>127)						// check for out-of-bounds char in command string
	      {
	      printf("\n\n *** ERROR - Atempt to send non-ascii char to tinyG! *** \n\n");
	      pd=32;							// overwrite with harmless character, will likely cause process failure
	      }
	    gc_out[n]=pd;						// add char to output string
	    if(pd==10 || pd==13)					// if line feed or return, send output string...
	      {
	      if(strlen(gc_out)>1)					// must be at least one char AND a lf
	        {
		TId++;							// increment transaction ID
		if(strstr(gc_out,"!")!=NULL || strstr(gc_out,"%")!=NULL)// these cmds do not produce a "complete" response from tinyG
		  {
		  if(strstr(gc_out,"!")!=NULL)printf(" Aborting motion!\n");
		  if(strstr(gc_out,"%")!=NULL)printf(" Clearing buffer!\n");
		  cmdCount=0;						// reset to zero since buffer is now cleared
		  
		  // note that since these two commands do not elicite a response from the tinyG
		  // the Rx_bytes cannot be controlled so it is not used with these two commands.
		  }
		else
		  {
		  sprintf(cmd_buffer[cmdCount].cmd,"%s",gc_out);	// retain the syntax of the command
		  cmd_buffer[cmdCount].tid=TId;				// retain the transaction ID
		  cmd_buffer[cmdCount].vid=VId;				// retain the vertex ID (which is incremented in the print_vertex function)
		  
		  // cmdCount is incremented each time a command is sent to tinyG.  it is decremented
		  // in tinyGRcv each time a command is completed.  so if moves are slow enough, cmdCount
		  // should equal the number of used buffers in the tinyG.
		  cmdCount++;						// increment the number of commands out at tinyG (note tinyGRcv decrements this value)

		  // tinyG usb rcv buffer claims to be ~1kbyte, but seems more like 500 bytes, or ~8 commands.
		  // increment the number of bytes sent to tinyG rcv buffer
		  rx=strlen(gc_out);
		  Rx_bytes += rx;
		  }
		if(strstr(gc_out,"mots")==NULL && strstr(gc_out,"qr")==NULL)	// don't echo request for status cmds
		  {
		  printf(">>(%d)%s",cmdCount,gc_out);			// display what is being sent
		  }
		
		if(serWrite(USB_fd,gc_out,strlen(gc_out))!=0)		// send command to tinyG
		  {
		  printf("\n\n***ERROR - tinyG data write failed! ***\n\n");
		  job.state=JOB_PAUSED_DUE_TO_ERROR;
		  break;
		  }
		}
	      memset(gc_out,0,sizeof(gc_out));				// clear the output string contents for next load
	      n=0;							// reset postion in command string
	      }
	    else 							// if not a cr or lf, just keep adding to same string
	      {
	      n++;							// increment to next position in output string
	      if(n>sizeof(gc_out))
	        {
		printf("\n\n*** ERROR - Excessively long GCode command formed! ***\n\n");
		job.state=JOB_PAUSED_DUE_TO_ERROR;
		break;
		}
	      }
	    }
	  }
	  
	return(1);
}

// Function to retrieve and parse tinyG status replies
// Inputs:	disp_val=1 to display status to stdio, 0 to not
// Return:  	-1 = buffer empty,  0 = buffer processed/processing & may be more, all else = error number	
int tinyGRcv(int disp_val)
{
	int 	h,i,j,n,p,rx;
	int 	err_no=(-1), pos_error, rev_no;
	char 	*qr_flag,*ft_flag,*v_start;
	char 	pd,gc_out[255],dir_cmd[255];
	char 	*token;
	char 	st_cmd[255],gcd_fmt[255],err_out[255];			// should match sizeof rcv_cmd


	//disp_val=FALSE;		// DEBUG
	
	// this delay allows what ever command that was just sent to the tinyG to get into
	// the queue to be processed.  checking without the delay will not give the controller
	// time to respond.
	delay(50);							

	p=serDataAvailable(USB_fd);					// see if anything is available
	n=strlen(rcv_cmd);						// init input string index to what was left over from previous call
	print_thread_alive=1;

	// if nothing in buffer and no pending left overs from previous call then leave
	if(p<=0 && n==0)
	  {
	  //if(disp_val==TRUE)printf(" <<(%d)buffer empty - nothing to process\n",cmdCount);
	  return(-1);
	  }

	while(TRUE)
	  {
	  if(err_no>=0)break;						// if an error has been hit...
	  if(p<=0)break;						// if no more data left in serial buffer or in left over cmds...
	  
	  pd=serReadByte(USB_fd);					// Load data off USB port a char at a time
	  if(pd<32 || pd>254)						// check for out-of-bounds char read in error
	    {
	    if(pd!=10 && pd!=13)
	      {
	      printf("\n\n*** ERROR - Non-ascii char read from tinyG! ***  pd=%d\n\n",(int)pd);
	      n=0;							// reset string index
	      memset(rcv_cmd,0,sizeof(rcv_cmd));			// clear string data
	      return(1);						// exit with generic error code
	      //p=serDataAvailable(USB_fd);				// recheck buffer for anything more
	      //continue;							// restart loop
	      }
	    }
	  rcv_cmd[n]=pd;						// Save to collective input string
	  n++;								// Increment input string index
	  if(n>(sizeof(rcv_cmd)-1))pd=10;				// Force line feed to next string if too long to display
	  rcv_cmd[n]=0;							// Ensure the string always has a proper ending
	  
	  // Check for line feed or return, process any flags, and reset input string index
	  if(pd==10 || pd==13)
	    {
	    // Buffer usage processing
	    // Check if "qr" occurs, then tokenize input string.  typically like {"qr":8} or {qr:8}
	    i=5;							// Set offset into qr_flag string with quotes
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    qr_flag=strstr(st_cmd,"\"qr\":");				// Check for quoted version
	    if(qr_flag==NULL)						// If not quoted, check for unquoted
	      {
	      qr_flag=strstr(st_cmd,"qr:");
	      i=3;							// Reset offset to length without quotes
	      }
	    if(qr_flag!=NULL)
	      {
	      token=strtok(qr_flag+i,",");				// Extract 1st value after match to buffer available
	      bufferAvail=atoi(token);
	      cmd_que=MAX_BUFFER-bufferAvail;				// count of commands in tinyG buffer
	      if(cmd_que<0)cmd_que=0;					// ensure we stay within array bounds
	      if(cmd_que>=MAX_BUFFER)cmd_que=MAX_BUFFER-1;
	      }

	    // Status report processing
	    // Check for status and tokenize input string {"sr"... which indicates status return similar to...
	    //{"sr":{"line":1245,"posx":23.4352,"posx":-9.4386,"posx":0.125,"vel":600,"unit":"1","stat":"5"}}
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"sr\"");
	    if(ft_flag!=NULL)
	      {
	      token=strtok(ft_flag,",");
	      while(token!=NULL)
		{
		if(strstr(token,"posx"))
		  {
		  v_start=strstr(token,"\"posx\":");
		  PostG.x=atof(v_start+7);
		  }
		if(strstr(token,"posy"))
		  {
		  v_start=strstr(token,"\"posy\":");
		  PostG.y=atof(v_start+7);
		  }
		if(strstr(token,"posz"))
		  {
		  v_start=strstr(token,"\"posz\":");
		  PostG.z=atof(v_start+7);
		  } 
		if(strstr(token,"posa"))
		  {
		  v_start=strstr(token,"\"posa\":");
		  PostG.a=atof(v_start+7);
		  }
		if(strstr(token,"vel"))
		  {
		  v_start=strstr(token,"\"vel\":");
		  PostGVel=atof(v_start+6);
		  }
		if(strstr(token,"stat"))
		  {
		  v_start=strstr(token,"\"stat\":");
		  tinyG_state=atoi(v_start+7);
		  }
		token=strtok(NULL,",");					// get next token in reply
		}
		
	      // ensure we are still in bounds to avoid physical damage
	      // note that these values may be exceeded any time when not "on part"
	      pos_error=FALSE;
	      //if(PostG.x<(job.XMin-25) || PostG.x>(job.XMax+25))pos_error=TRUE;
	      //if(PostG.y<(job.YMin-25) || PostG.y>(job.YMax+25))pos_error=TRUE;
	      //if(PostG.z<(job.ZMin-25) || PostG.z>(job.ZMax+25))pos_error=TRUE;
	      if(PostG.x<(BUILD_TABLE_MIN_X+10) || PostG.x>(BUILD_TABLE_MAX_X-10))pos_error=TRUE;
	      if(PostG.y<(BUILD_TABLE_MIN_Y+10) || PostG.y>(BUILD_TABLE_MAX_Y-10))pos_error=TRUE;
	      if(PostG.z<(BUILD_TABLE_MIN_Z+10) || PostG.z>(BUILD_TABLE_MAX_Z-10))pos_error=TRUE;

	      pos_error=FALSE;	// DEBUG

	      if(on_part_flag==TRUE && pos_error==TRUE)
	        {
		printf("\n\nERROR - Motion exceeding job bounds!  x=%f y=%f z=%f \n",PostG.x,PostG.y,PostG.z);
		printf("    job:Xmin=%f  YMin=%f  ZMin=%f \n",job.XMin,job.YMin,job.ZMin);
		printf("    job:Xmax=%f  YMax=%f  ZMax=%f \n",job.XMax,job.YMax,job.ZMax);
		printf("        Xmin=%f  YMin=%f  ZMin=%f \n",XMin,YMin,ZMin);
		printf("        Xmax=%f  YMax=%f  ZMax=%f \n\n",XMax,YMax,ZMax);
		sprintf(gc_out,"!\n");					// abort all moves
		serWrite(USB_fd,gc_out,strlen(gc_out));
		sprintf(gc_out,"%\n");					// clear entire buffer
		serWrite(USB_fd,gc_out,strlen(gc_out));
		//job.state=JOB_PAUSED_DUE_TO_ERROR;
		return(-1);
		}
	      }

	    // Command complete processing
	    // Check for status and tokenize input string {"r":{ ... which indicates status return similar to...
	    //{"r":{},"f":[1,0,41,163]}  It basically means a command has been processed by tinyG.
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"r\":{");
	    if(ft_flag!=NULL)						// if a command has been processed...
	      {
	      //printf("\nCnt:%d / Buf:%d \n",cmdCount,cmd_que);
	      //for(j=(cmdCount-1);j>=0;j--)				// dump pending commands in tinyG to display
	      //  {
	      //  if(strlen(cmd_buffer[j].cmd)>1)
	      //    {
	      //    sprintf(gcd_fmt,"<%d> %X %s",j,cmd_buffer[j].vptr,cmd_buffer[j].cmd);
	      //    printf("%s",gcd_fmt);		
	      //    }
	      //  }

	      // display/log the command that was just processed (i.e. the oldest one in the buffer)
	      sprintf(gcd_fmt,">>%s",cmd_buffer[0].cmd);		// copy oldest command (one that just got processed)
	      //if(disp_val==TRUE)printf("%s",gcd_fmt);			// pop buffered cmd out of FIFO tracking what has been sent/processed
	      if(save_logfile_flag==TRUE && gcode_gen!=NULL)
	        {
		if(strstr(gcd_fmt,"\"qr\"")==NULL && strstr(gcd_fmt,"\"mots\"")==NULL)	// filter out qr and mots requests
		  {fwrite(gcd_fmt,1,strlen(gcd_fmt),gcode_gen);}
		}
	      
	      // record move finish information back into source vertex that instigated the move
	      // this is generally just used as debug feedback regarding what really happened in the move
	      if(cmd_buffer[0].vptr!=NULL)
	        {
		cmd_buffer[0].vptr->attr=(-2);				// set attribute as "printed"
		cmd_buffer[0].vptr->i=PostG.x;				// save coords at which the move finished
		cmd_buffer[0].vptr->j=PostG.y;
		cmd_buffer[0].vptr->k=(PostG.z-Tool[0].tip_dn_pos+Tool[0].tip_z_fab);
		}

	      // cmd_buffer[0] is the oldest cmd in the list.  this response from tinyG indicates this cmd is done.
	      // shift cmd buffer contents to "left" to match "pop" of oldest cmd, and null last cmd in list
	      for(j=0;j<cmdCount;j++)
	        {
		strcpy(cmd_buffer[j].cmd,cmd_buffer[j+1].cmd);
		cmd_buffer[j].tid=cmd_buffer[j+1].tid;
		cmd_buffer[j].vid=cmd_buffer[j+1].vid;
		cmd_buffer[j].vptr=cmd_buffer[j+1].vptr;
		}
	      memset(cmd_buffer[cmdCount].cmd,0,sizeof(cmd_buffer[cmdCount]));	// clear contents of obsolete cmd
	      cmd_buffer[cmdCount].tid=0;
	      cmd_buffer[cmdCount].vid=0;
	      cmd_buffer[cmdCount].vptr=NULL;					// clear all references to vtxs
	      if(cmdCount==0)sprintf(cmd_buffer[0].cmd,"null\n");		// default to keep consol disply nice

	      // adjust counters
	      // cmdCount is incremented in tinyGSnd.  it tells us how many cmds are pending in tinyG.
	      // (i.e. if at 5, then cmd_buffer[0] thru cmd_buffer[4] are live)
	      // vtxCount is incremented in print_vertex.  it tells us how many vtxs are pending in tinyG.
	      // (i.e. if at 5, then cmd_buffer[0]->vptr thru cmd_buffer[4]->vptr should not be NULL)
	      vtxCount--; if(vtxCount<0)vtxCount=0;			// decrement number of vtxs residing in tinyG buffer
	      cmdCount--; if(cmdCount<0)cmdCount=0;			// decrement count of commands queued in tinyG

	      }

	    // Footer processing
	    // Check if footer "f" flag is up and tokenize input string {"r":{"qr":32},"f":[1,0,8,7767]}
	    // The first value of the footer = revision number, 2nd = status code, 3rd = number of bytes
	    // pulled from rcv buffer for this command, 4th = checksum.
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"f\":[");
	    if(ft_flag!=NULL)
	      {
	      // Revision number processing
	      token=strtok(ft_flag,",");
	      rev_no=atoi(token);
		  
	      // Error processing
	      token=strtok(NULL,",");					// get next token in reply
	      err_no=atoi(token);					// assign to err_no (0=okay)
	      if(err_no>0)
		{
		if(err_no>=201 && err_no<=202)				// move too small errors, just continue
		  {
		  printf("  <<%s",rcv_cmd);
		  printf("  >>Minimum move error encountered... resuming with next move.\n");
		  }
		else 							// some other error, typcially catastrophic
		  {
		  sprintf(err_out,"\n>>controller error = %d \n",err_no);
		  strcat(err_out,rcv_cmd);
		  printf("%s\n",err_out);
		  if(err_no==101 || err_no==102)
		    {
		    printf("    Invalid command - ignoring and continuing.\n\n");
		    //printf("    Invalid command - clearing buffer and continuing.\n\n");
		    //serWrite(USB_fd,"!",1);
		    //serWrite(USB_fd,"%",1);
		    }
		  else 
		    {
		    if(save_logfile_flag==TRUE){fwrite(err_out,1,strlen(err_out),gcode_out);}
		    sprintf(dir_cmd,"{\"clear\":true} \n");
		    serWrite(USB_fd,dir_cmd,strlen(dir_cmd));		// attempt to clear soft alarm
		    //return(err_no);
		    }
		  }
		}
		
	      // Rx buffer processing
	      token=strtok(NULL,",");					// get next token in reply
	      rx=atoi(token);
	      Rx_bytes -= rx;						// reduce bytes in buffer by command that just processed
	      if(Rx_bytes<0)Rx_bytes=0;
	      
	      }
	      
	    // Check if "er" flag is up and color display string red {"er"... which indicates error encountered
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"er\"");
	    if(ft_flag!=NULL)
	      {
	      printf("\n>>controller errror = %d \n\n",err_no);
	      //return(err_no);
	      }

	    // CCA ID request processing
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"id\"");				// replies with:  {"r":{"id":"7W4698-3LH"},"f":[1,0,10,3076]}  where 7W4698-3LH is the ID
	    if(ft_flag!=NULL)				
	      {
	      token=strtok(ft_flag+5,"\"");
	      strcpy(tinyG_id,token);
	      }	

	    // Motion status processing
	    // Check for motion status and tokenize input string {"mots":0}
	    strcpy(st_cmd,rcv_cmd);					// make copy of rcv_cmd to allow tokenizing
	    ft_flag=strstr(st_cmd,"\"mots\":");
	    if(ft_flag!=NULL)
	      {
	      token=strtok(ft_flag+7,"\"");
	      tinyG_in_motion=atoi(token);
	      }

	    // output command to console
	    if(strlen(rcv_cmd)>1 && disp_val==TRUE)printf("  <<(%d)%s",cmdCount,rcv_cmd); // assume the command has a lf on it...

	    // If request to save log file is on...
	    if(save_logfile_flag==TRUE)
	      {
	      if(gcode_out!=NULL && strlen(rcv_cmd)>1)
	        {
		sprintf(gcd_fmt,"%s",rcv_cmd);
		fwrite(gcd_fmt,1,strlen(gcd_fmt),gcode_out);
		}
	      }
		    
	    // Now reset input string index ONLY after a lf/return to leave remainder
	    n=0;							// reset string index
	    memset(rcv_cmd,0,sizeof(rcv_cmd));				// clear string data
	    } 	// end of if pd==lf or pd==cr
	  
	  p=serDataAvailable(USB_fd);					// recheck buffer for anything more
	  }
		
	// if nothing in buffer and no pending left overs from previous call then leave
	if(p<=0 && n==0)
	  {
	  //if(disp_val==TRUE)printf(" <<(%d)buffer empty - processing complete\n",cmdCount);
	  return(-1);
	  }
	return(0);	// return with indication that current buffer is empty but more may be comming and ought to be rechecked
}


// Function to create an execution delay
void delay(unsigned int msecs)
{
  clock_t goal;
  
  goal = msecs*CLOCKS_PER_SEC/1000 + clock();  				// convert msecs to clock count  
  while(goal>clock());               					// loop until it arrives.
  return;
}

// Function to emulate ansi C kbhit() not available in linux with the exception
// that this will return the ascii code of what was hit (not just 1).
int kbhit(void) 
  {
  struct termios oldt, newt;
  int ch;
  int oldf;
  
  tcgetattr(STDIN_FILENO, &oldt);					// save current terminal settings
  newt = oldt;								// make a new copy
  newt.c_lflag &= ~(ICANON | ECHO);					// turn off buffering and echo
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);				// switch to new terminal settings
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);			
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  
  ch = getchar();
  
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);				// restore old terminal settings
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  
  if(ch!=EOF) 
    {
//  ungetc(ch, stdin);
//  printf("** %d **\n",ch);
    return(ch);
    }
  
  return(0);
  }



void dialog_okay_cb(GtkWidget *widget, gpointer dialog)
{
    aa_dialog_result=TRUE;
    gtk_window_close(GTK_WINDOW(dialog));
  return;
}

void dialog_cncl_cb(GtkWidget *widget, gpointer dialog)
{
    aa_dialog_result=FALSE;
    gtk_window_close(GTK_WINDOW(dialog));
  return;
}

// Function to display a message dialog box and return user response
//   parent=calling window.  important to keep on top so user sees result of their actions
//   type=0: show message without any buttons for either a duration or until back into this function (persistant)
//   type=1: show message with just okay button.  destroy and leave after user acknowledges.
//   type=2: show message with Okay/Cancel choice.  destroy and leave after user makes choice.
//   type=3: show message with Yes/No choice.  destroy and leave after user makes choice.
//   display_dur=time in tenths of a second to display window
//   title=title of this new message window
//   msg=pointer to the message to be displayed in the window
void aa_dialog_box(GtkWidget *parent, int type, int display_dur, char *title, char *msg)
{
  int		i;
  GtkWidget	*dgrid,*lbl_msg,*lbl_spacer;
  GtkWidget	*btn_okay, *btn_cncl;
  
  // if dialog was left as persistant, this will close it
  if(aa_dialog_result<0)
    {
    gtk_window_close(GTK_WINDOW(aa_dialog));
    g_main_context_iteration(NULL, FALSE);
    }
  
  // build the dialog window
  aa_dialog_result=(-1);
  aa_dialog=gtk_window_new();
  if(parent!=NULL)gtk_window_set_transient_for(GTK_WINDOW(aa_dialog), GTK_WINDOW(parent));
  gtk_window_set_default_size(GTK_WINDOW(aa_dialog),350,125);	
  gtk_window_set_resizable(GTK_WINDOW(aa_dialog),FALSE);	
  if(strlen(title)>1)gtk_window_set_title(GTK_WINDOW(aa_dialog),title);			
  dgrid = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(dgrid),10);
  gtk_window_set_child(GTK_WINDOW(aa_dialog),dgrid);
  gtk_grid_set_column_homogeneous(GTK_GRID(dgrid), TRUE);
  lbl_msg = gtk_label_new (msg);
  gtk_grid_attach (GTK_GRID(dgrid), lbl_msg, 0, 0, 1, 1);	
  
  // show message without any buttons for either a duration or until back into this function (persistant)
  if(type==0)
    {
    if(display_dur<=0)
      {
      aa_dialog_result=(-1);
      gtk_widget_set_visible(aa_dialog,TRUE);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      delay(100);
      return;
      }
    if(display_dur>250)display_dur=250;
    gtk_widget_set_visible(aa_dialog,TRUE);
    i=0;
    while(i<display_dur)
      {
      g_main_context_iteration(NULL, FALSE);
      delay(100);
      i++;
      }
    aa_dialog_result=0;
    gtk_window_close(GTK_WINDOW(aa_dialog));
    }
    
  // show message with just okay button.  destroy and leave after user acknowledges.
  if(type==1)
    {
    gtk_window_set_modal(GTK_WINDOW(aa_dialog),TRUE);
    btn_okay=gtk_button_new_with_label("Okay");
    gtk_grid_attach(GTK_GRID(dgrid),btn_okay,0,1,1,1);
    g_signal_connect (btn_okay, "clicked", G_CALLBACK(dialog_okay_cb),aa_dialog);
    lbl_spacer = gtk_label_new ("    ");
    gtk_grid_attach (GTK_GRID(dgrid), lbl_spacer, 0, 2, 1, 1);	
    gtk_widget_set_visible(aa_dialog,TRUE);
    g_main_context_iteration(NULL, FALSE);
    }
    
  // show message with choice.  destroy and leave after user makes choice.
  if(type==2)
    {
    gtk_window_set_modal(GTK_WINDOW(aa_dialog),TRUE);
    btn_okay=gtk_button_new_with_label("Okay");
    gtk_grid_attach(GTK_GRID(dgrid),btn_okay,0,1,1,1);
    g_signal_connect (btn_okay, "clicked", G_CALLBACK (dialog_okay_cb),aa_dialog);
    btn_cncl=gtk_button_new_with_label("Cancel");
    gtk_grid_attach(GTK_GRID(dgrid),btn_cncl,0,2,1,1);
    g_signal_connect (btn_cncl, "clicked", G_CALLBACK (dialog_cncl_cb),aa_dialog);
    lbl_spacer = gtk_label_new ("    ");
    gtk_grid_attach (GTK_GRID(dgrid), lbl_spacer, 0, 3, 1, 1);	
    gtk_widget_set_visible(aa_dialog,TRUE);
    g_main_context_iteration(NULL, FALSE);
    }

  // same as above but with different button names.
  if(type==3)
    {
    gtk_window_set_modal(GTK_WINDOW(aa_dialog),TRUE);
    btn_okay=gtk_button_new_with_label("Yes");
    gtk_grid_attach(GTK_GRID(dgrid),btn_okay,0,1,1,1);
    g_signal_connect (btn_okay, "clicked", G_CALLBACK (dialog_okay_cb),aa_dialog);
    btn_cncl=gtk_button_new_with_label("No");
    gtk_grid_attach(GTK_GRID(dgrid),btn_cncl,0,2,1,1);
    g_signal_connect (btn_cncl, "clicked", G_CALLBACK (dialog_cncl_cb),aa_dialog);
    lbl_spacer = gtk_label_new ("    ");
    gtk_grid_attach (GTK_GRID(dgrid), lbl_spacer, 0, 3, 1, 1);	
    gtk_widget_set_visible(aa_dialog,TRUE);
    g_main_context_iteration(NULL, FALSE);
    }
  
  return;
}


// Function to setup PWM controller using PIGPIO library
int pca9685Setup(const int pinBase, const int i2cAddress, float freq)
{
	uint8_t buf[4];
	
	// ensure gpio is initialized
	if(I2C_PWM_fd<0)
	  {
	  printf("\n\nPWM Initialisation Failure! \n");
	  return(0);
	  }

	// Setup the chip:
	// 	- Enable auto-increment of registers.
	//	- Disable sleep mode so oscillator stays on all the time
	//	- Disable I2C subaddresses
	//	- Disable all call
	PWMsettings=0x30;						// 0011 0000  see pg 14 of manual
	buf[0]=PCA9685_MODE1;
	buf[1]=PWMsettings;
	if(write(I2C_PWM_fd,buf,2)!=2)printf("  PWMSetup write failed. \n");

	// Set frequency of PWM signals. Also ends sleep mode and starts PWM output.
	if(freq>0)pca9685PWMFreq(I2C_PWM_fd,freq);
	
	buf[0]=PCA9685_MODE1;
	if(write(I2C_PWM_fd,buf,1)!=1)printf("  PWMSetup write failure. \n");
	if(read(I2C_PWM_fd,buf,1)!=1)printf("  PWMSetup read failure. \n");
	int pwmconfig=buf[0];
	printf("  PWMconfig = %X \n",pwmconfig);
	
	printf("  PWM setup complete.\n");

	return(TRUE);
}

// Function to set frequency of PWM
void pca9685PWMFreq(int fd, float freq)
{
	uint8_t buf[4];

	// Cap at min and max
	freq = (freq > 1000 ? 1000 : (freq < 40 ? 40 : freq));

	// To set pwm frequency we have to set the prescale register. The formula is:
	// prescale = round(osc_clock / (4096 * frequency))) - 1 where osc_clock = 25 MHz
	// Further info here: http://www.nxp.com/documents/data_sheet/PCA9685.pdf Page 24
	int prescale = (int)(25000000.0f / (4096 * freq) - 0.5f);

	// Get settings and calc bytes for the different states.
	int sleep=bitset(PWMsettings,4);
	int wake=bitclear(PWMsettings,4);
	int restart=bitset(PWMsettings,7);

	// Go to sleep, set prescale and wake up again.
	buf[0]=PCA9685_MODE1;
	buf[1]=sleep;
	if(write(I2C_PWM_fd,buf,2)!=2)printf("  PWMFreq write failed. \n");
	delay(10);
	
	buf[0]=PCA9685_PRESCALE;
	buf[1]=prescale;
	if(write(I2C_PWM_fd,buf,2)!=2)printf("  PWMFreq write failed. \n");
	delay(10);
	
	buf[0]=PCA9685_MODE1;
	buf[1]=wake;
	if(write(I2C_PWM_fd,buf,2)!=2)printf("  PWMFreq write failed. \n");

	// Now wait a few milliseconds until oscillator finished stabilizing and restart PWM.
	delay(50);

	buf[0]=PCA9685_MODE1;
	buf[1]=restart;
	if(write(I2C_PWM_fd,buf,2)!=2)printf("  PWMFreq write failed. \n");

	printf("  PWM frequency set to %f6.2 complete.\n",freq);
	
	return;
}

// Function to reset PWM device
void pca9685PWMReset(int fd)
{
	uint8_t buf[4];

	buf[0]=PCA9685_MODE1;
	if(write(I2C_PWM_fd,buf,1)!=1)printf("  PWMReset write failure. 1\n");
	if(read(I2C_PWM_fd,buf,1)!=1)printf("  PWMReset read failure. 1\n");
	int restart=buf[0];
	printf("  PWMReset:  restart=%X \n",restart);
	if(bitcheck(restart,7)==TRUE)					// if bit 7 is high...
	  {
	  buf[0]=PCA9685_MODE1;
	  buf[1]=bitset(restart,4);					// set bit 4 high
	  if(write(I2C_PWM_fd,buf,2)!=1)printf("  PWMReset write failure. 2\n");
	  delay(10);
	  buf[0]=PCA9685_MODE1;
	  buf[1]=bitclear(restart,4);
	  if(write(I2C_PWM_fd,buf,2)!=1)printf("  PWMReset write failure. 2\n");
	  delay(10);
	  printf("  PWM full reset complete.\n");
	  }
	else if(bitcheck(restart,4)==TRUE)				// switch out of low power mode
	  {
	  buf[0]=PCA9685_MODE1;
	  buf[1]=bitclear(restart,4);
	  //if(write(I2C_PWM_fd,buf,2)!=1)printf("  PWMRset write failure. \n");
	  delay(10);
	  printf("  PWM no longer in low power mode.\n");
	  }
	delay(250);
	
	return;
}

// Function to write data to PWM
// This function will write LSB first, then MSB next in accordance to spec.
void pca9685PWMWrite(int fd, int pin, int on, int off)
{
	uint8_t buf[4];
	
	// calc address
	if(pin<0)return;
	pin=baseReg(pin);
	
	// Write to on and off registers and mask the 12 lowest bits of data to overwrite full-on and off
	buf[0]=pin;							// set reg address
	buf[1]=on & 0xFF;						// set LSB = bottom 8 bits
	buf[2]=(on >> 8) & 0x0F;					// set MSB = shift left 8 bits then take bottom 4 bits
	if(write(I2C_PWM_fd,buf,3)!=3)printf("  PWM Write: failure \n");
	//printf("\nPWM ON : pin=%X  On=%X%X  ",buf[0],buf[2],buf[1]);
	
	buf[0]=pin+2;
	buf[1]=off & 0xFF;
	buf[2]=(off >> 8) & 0x0F;
	if(write(I2C_PWM_fd,buf,3)!=3)printf("  PWM Write: failure \n");
	//printf("OFF=%X%X \n",buf[2],buf[1]);
	
	return;
}

// Function to read data from PWM.
// Per spec this will read the LSB first then MSB next
void pca9685PWMRead(int fd, int pin, int *on, int *off)
{
	uint8_t buf[4];

	// calc address
	if(pin<0)return;
	pin=baseReg(pin);
	
	// Set register, get data, combine, and put back into in-bound pointer
	if(on)
	  {
	  buf[0]=pin;
	  if(write(I2C_PWM_fd,buf,1)!=1)printf("  PWM Read : reg failure \n");
	  if(read(I2C_PWM_fd,buf,2)!=2)printf("  PWM Read : data failure \n");
	  *on=(((int16_t)buf[1] & 0x0F)<<8)+(uint16_t)buf[0];
	  }
	
	if(off)
	  {
	  buf[0]=pin+2;
	  if(write(I2C_PWM_fd,buf,1)!=1)printf("  PWM Read : reg failure \n");
	  if(read(I2C_PWM_fd,buf,2)!=2)printf("  PWM Read : data failure \n");
	  *off=(((int16_t)buf[1] & 0x0F)<<8)+(uint16_t)buf[0];
	  }
	
	return;
}

// Function to calculate base register
int baseReg(int pin)
{
	return (pin >= PIN_ALL ? PWMALL_ON_L : PWM0_ON_L + 4 * pin);
}

