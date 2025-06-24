#include "Global.h"	
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

// Callback to destroy tool UI when user exits
void on_tool_exit (GtkWidget *btn, gpointer dead_window)
{
  int		slot;
  model		*mptr;
  operation	*optr;
  genericlist	*lptr;
  
/*
  // DEBUG
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(Tool[slot].state<TL_LOADED)continue;
    printf("\nTool Operation Dump: slot=%d \n",slot);
    optr=Tool[slot].oper_first;
    while(optr!=NULL)
      {
      printf("  operation=%s  ID=%d \n",optr->name,optr->ID);
      lptr=optr->lt_seq;
      while(lptr!=NULL)
        {
	printf("    linetype=%s  ID=%d \n",lptr->name,lptr->ID);
	lptr=lptr->next;
	}
      optr=optr->next;
      }
    }
*/

  // if a model is already loaded, apply values
  //if(job.model_first!=NULL)gtk_adjustment_configure (g_adj_z_level,z_cut,0,ZMax,job.min_slice_thk,0.50,1.00);
  
  // set build table temperature to the highest temp based on which materials are loaded
  if(Tool[BLD_TBL1].thrm.temp_override_flag==FALSE)
    {
    Tool[BLD_TBL1].thrm.setpC=10.0;
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(Tool[slot].state<TL_LOADED)continue;
      if(Tool[slot].thrm.bedtC>Tool[BLD_TBL1].thrm.setpC)Tool[BLD_TBL1].thrm.setpC=Tool[slot].thrm.bedtC;
      Tool[BLD_TBL1].thrm.operC=Tool[BLD_TBL1].thrm.setpC;
      #ifdef BETA_UNIT
        Tool[BLD_TBL2].thrm.setpC = Tool[BLD_TBL1].thrm.setpC;
	Tool[BLD_TBL2].thrm.operC = Tool[BLD_TBL1].thrm.operC;
      #endif
      #ifdef GAMMA_UNIT
        Tool[BLD_TBL2].thrm.setpC = Tool[BLD_TBL1].thrm.setpC;
	Tool[BLD_TBL2].thrm.operC = Tool[BLD_TBL1].thrm.operC;
      #endif
      }
    }
    
  for(slot=0;slot<MAX_TOOLS;slot++)win_tool_flag[slot]=FALSE;
  idle_start_time=time(NULL);						// reset idle time
  ztip_test_point=1;
  gtk_window_close(GTK_WINDOW(win_tool));
  gtk_widget_queue_draw(GTK_WIDGET(win_main));
  make_toolchange(-1,0);
  
  return;
  
}

// Callback to resposition the carriage for easier access to the slot for a tool change.
// Note that the idle loop will take care of identifying the tool and loading its params.
int on_tool_change (GtkWidget *btn, gpointer user_data)
{
  int		slot;
  model		*old_active;
  char		pre_tool_name[32];
  vertex 	*vptr;
  model		*mptr;

  // this function will not work with alpha or beta units due to their design
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    return(FALSE);
  #endif

  printf("Tool Change: entry \n");
  
  // init
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
  
  // determine if in middle of job or not
  if(job.state>JOB_READY)
    {
    sprintf(scratch,"\n Not allowed during a job. \n Abort job first. \n");
    aa_dialog_box(win_main,0,100,"Tool Change",scratch);
    return(FALSE);
    }
  
  // temporarly set to nothing so that model offsets are not applied in tool move
  old_active=active_model;
  active_model=NULL;
  
  // save name of tool coming out
  strcpy(pre_tool_name,Tool[slot].name);

  vptr=vertex_make();
  vptr->x=BUILD_TABLE_MAX_X-25;						// end of x less a little
  vptr->y=(BUILD_TABLE_MAX_Y/2.0);					// middle of y
  vptr->z=(BUILD_TABLE_MAX_Z-BUILD_TABLE_MIN_Z)/2.0;			// middle of z
  vptr->k=8000;								// a little slower than full pen up
  
  // reposition the carriage near the door
  comefrom_machine_home(slot,vptr);
  
  // must turn OFF holding torque on tool stepper to allow user to remove/add material manually
  tinyGSnd("{4pm:0}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
  delay(150);							
  while(cmdCount>0)tinyGRcv(1);
  
  // post msg to user
  aa_dialog_box(win_main,1,0,"Tool Change","\n You can change the tool now. \n Click Okay when change is complete. ");
  
  // wait for user to confirm change
  aa_dialog_result=FALSE;
  while(aa_dialog_result==FALSE)
    {
    delay(100);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    }
  
  // at this point the idle loop should have picked up information about the tool
  // that was just installed.  if not the same type of tool, reset stuff.
  if(strstr(Tool[slot].name,pre_tool_name)==NULL)
    {
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      mptr->mdl_has_target=FALSE;
      mptr->reslice_flag=TRUE;
      mptr=mptr->next;
      }
    job.regen_flag=TRUE;
    }
  
  // move carriage back to home
  goto_machine_home();
  
  // clean up
  active_model=old_active;
  free(vptr); vertex_mem--; vptr=NULL;
  printf("Tool Change: exit \n");

  return(TRUE);
}

// Function to test a given tool
int on_tool_test(GtkWidget *btn_call, gpointer user_data)
{
  int		h,i,on_ticks,slot;
  float 	fval;
  model 	*mptr;
  GtkWidget 	*dialog;
  vertex 	*vptr;
  linetype 	*lptr;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
  
  // attempt 1wire read
  Open1Wire(slot);

  // if 1wire has been identified for this tool...
  if(strlen(Tool[slot].mmry.dev)>8)				
    {
    MemDevRead(slot);							// ... upload tool information
    XMLRead_Tool(slot,Tool[slot].mmry.md.name);				// ... update tool params
    XMLRead_Matl(slot,Tool[slot].mmry.md.matp);				// ... update matl params
    }
  // otherwise reload based on previous references...
  else 
    {
    XMLRead_Tool(slot,Tool[slot].name);					// ... update tool params
    XMLRead_Matl(slot,Tool[slot].matl.name);				// ... update matl params
    }
    
  // update display
  sprintf(scratch,"Serial ID: Unknown");
  if(strlen(Tool[slot].mmry.dev)>7)sprintf(scratch,"Serial ID: %s",Tool[slot].mmry.dev);
  gtk_label_set_text(GTK_LABEL(lbl_tool_memID),scratch);
  gtk_widget_queue_draw(lbl_tool_memID);

  lptr=linetype_find(slot,MDL_BORDER);					// use border in this case
  fval=0.0;
  if(lptr!=NULL)fval=100*lptr->flowrate;
  sprintf(scratch," %4.0f%% ",fval);
  while(strlen(scratch)<15)strcat(scratch," ");
  gtk_label_set_text(GTK_LABEL(Tmatl_lbl[slot]),scratch);
  gtk_widget_queue_draw(Tmatl_lbl[slot]);
  gtk_level_bar_set_value (Tmatl_bar[slot],fval);
  
  /*
  printf("\nTool testing initiated: ------------------------------------\n");
  
  // position tool tip at 0,0,0 - note this is done thru the print_vertex function to ensure all is accounted for
  sprintf(gcode_cmd,"G0 Z5.0 \n");					// lift z
  tinyGSnd(gcode_cmd);
  vptr=vertex_make();
  
  vptr->x=0;vptr->y=0;vptr->z=0;					// set target to 0,0,0
  print_vertex(vptr,slot,2,NULL);					// build command
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for motion to stop
  while(!kbhit());

  // position tool tip at center of table
  vptr->x=BUILD_TABLE_LEN_X/2;vptr->y=BUILD_TABLE_LEN_Y/2;vptr->z=0;	// set target to mid X, mid Y, 0
  print_vertex(vptr,slot,2,NULL);					// build command
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for motion to stop
  while(!kbhit());

  // position tool tip at max of table
  vptr->x=BUILD_TABLE_LEN_X;vptr->y=BUILD_TABLE_LEN_Y;vptr->z=0;	// set target to max X, max Y, 0
  print_vertex(vptr,slot,2,NULL);					// build command
  tinyGSnd(gcode_burst);						// execute pen up move
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  motion_complete();							// wait for motion to stop
  while(!kbhit());

  sprintf(gcode_cmd,"G0 Z5.0 \n");					// lift z
  tinyGSnd(gcode_cmd);
  sprintf(gcode_cmd,"G0 X0.0 Y0.0 Z5.0 \n");				// move to park
  tinyGSnd(gcode_cmd);
  sprintf(gcode_cmd,"G0 Z0.0 \n");					// drop z
  tinyGSnd(gcode_cmd);
  

  // run a temperature check
  printf("Tool Temps: ");
  for(h=0;h<MAX_TOOLS;h++)
    {
    printf("  slot %d = %6.3f  \n", h, Tool[h].thrm.tempC);
    delay(100);
    }
  printf(" \n");
  
  //pthread_mutex_lock(&thermal_lock);      
  Tool[slot].thrm.setpC=50.0;
  //pthread_mutex_unlock(&thermal_lock);      
  

  // heater duty cycle loop
  printf("Running heater... \n");
  for(i=0;i<MAX_TOOLS;i++)
    {
    on_ticks=(int)(MAX_PWM*50/100);					// set based on duty cycle
    pca9685PWMWrite(I2C_PWM_fd,Tool[i].thrm.PWM_port,0,on_ticks);	// send to PWM chip via I2C
    printf("   %d ON ",i);
    while(!kbhit());
    }
  printf(" \n");
  for(i=0;i<MAX_TOOLS;i++)
    {
    pca9685PWMWrite(I2C_PWM_fd,Tool[i].thrm.PWM_port,0,0);		// send to PWM chip via I2C
    printf("   %d OFF",i);
    while(!kbhit());
    }
  printf("\n   done.\n");

  // tool power duty cycle loop
  printf("Running power... \n");
  for(i=0;i<MAX_TOOLS;i++)
    {
    on_ticks=(int)(MAX_PWM*50/100);					// set based on duty cycle
    pca9685PWMWrite(I2C_PWM_fd,Tool[i].thrm.PWM_port,0,on_ticks);	// send to PWM chip via I2C
    printf("   %d ON ",i);
    while(!kbhit());
    }
  printf(" \n");
  for(i=0;i<MAX_TOOLS;i++)
    {
    pca9685PWMWrite(I2C_PWM_fd,Tool[i].thrm.PWM_port,0,0);		// send to PWM chip via I2C
    printf("   %d OFF",i);
    while(!kbhit());
    }
  printf("\n   done.\n");
  
  // test stepper connection
  printf("Testing material drive stepper... ");
  sprintf(gcode_cmd,"G1 A2.5 F150 \n");
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);
  sprintf(gcode_cmd,"G1 A0.0 F150 \n");
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);
  delay(1000);
  printf("done.\n");

  printf("Tool testing complete. ---------------------------------------\n\n");
  */
  
  return(TRUE);
}

// Callback to destroy tool info window
void on_info_okay(GtkWidget *btn, GtkWidget *win_null, gpointer dead_window)
{

  gtk_window_close(GTK_WINDOW(dead_window));
  return;
  
}



void grab_xtip_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  float delta;
  
  slot=GPOINTER_TO_INT(user_data);
  delta=Tool[slot].tip_x_fab;						// save current offset value
  Tool[slot].tip_x_fab=gtk_spin_button_get_value(button);		// get new x offset per user input
  delta-=Tool[slot].tip_x_fab;						// calculate change in offset value
  tinyGSnd("{\"posx\":NULL}\n");					// get current position
  while(tinyGRcv(1)>=0);					// ensure it is processed
  sprintf(gcode_cmd,"G0 X%f \n",PostG.x+delta);				// move to the new adj value
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);

  if(win_tool_flag[slot]==TRUE)
    {
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].x_offset,Tool[slot].x_offset,Tool[slot].mmry.md.tip_pos_X);
    gtk_label_set_text(GTK_LABEL(lbl_tipx),scratch);
    }

  return;
}

void grab_ytip_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  float delta;
  
  slot=GPOINTER_TO_INT(user_data);
  delta=Tool[slot].tip_y_fab;
  Tool[slot].tip_y_fab=gtk_spin_button_get_value(button);		// get new y offset per user input
  delta-=Tool[slot].tip_y_fab;						// calculate change in offset value
  tinyGSnd("{\"posy\":NULL}\n");					// get current position
  while(tinyGRcv(1)>=0);						// ensure it is processed
  sprintf(gcode_cmd,"G0 Y%f \n",PostG.y+delta);				// move to the new adj value
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);

  if(win_tool_flag[slot]==TRUE)
    {
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].y_offset,Tool[slot].y_offset,Tool[slot].mmry.md.tip_pos_Y);
    gtk_label_set_text(GTK_LABEL(lbl_tipy),scratch);
    }
  
  return;
}

void grab_ztip_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot;

  // get z value
  slot=GPOINTER_TO_INT(user_data);
  Tool[slot].tip_z_fab=gtk_spin_button_get_value(button);

  // move tip to new z value
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    sprintf(gcode_cmd,"G1 A%07.3f F250.0 \n",Tool[slot].tip_z_fab);	// build move command
  #endif
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    sprintf(gcode_cmd,"G1 Z%07.3f F250.0 \n",(Tool[slot].tip_dn_pos+Tool[slot].tip_z_fab));	// build move command
  #endif
  tinyGSnd(gcode_cmd);							// send move command
  motion_complete();							// ensure it has reached its destination

  // update display(s) as needed
  if(win_tool_flag[slot]==TRUE)
    {
    //sprintf(scratch,"%+8.3f",Tool[slot].tip_z_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].z_offset,Tool[slot].tip_dn_pos,Tool[slot].tip_z_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipzdn),scratch);
    }
  
  return;
}

// Destroy callback for setting tool tip location
void xyztip_done_callback (GtkWidget *btn, gpointer dead_window)
{
  float 	avg_z;
  
  //avg_z=z_table_offset(PosIs.x,PosIs.y);				// get z offset at this xy position
  //Tool[current_tool].tip_z_fab += avg_z;				// compensate for table displacement at test point

  z_tbl_comp=old_z_tbl_comp;						// restore to prev state
  gtk_window_close(GTK_WINDOW(dead_window));
  goto_machine_home();
  make_toolchange(current_tool,1);

  return;
}

// Next position callback for setting tool tip location
void xyztip_next_callback (GtkWidget *btn, gpointer user_data)
{
  int 		slot;
  vertex	*vptr;
  
  slot=GPOINTER_TO_INT(user_data);
  vptr=vertex_make();
  
  // designed to use multiple test points, but currently only using one at center of table.
  // aligns test point to table scan point which becomes z offset reference for rest of the table.
  if(ztip_test_point>1)ztip_test_point=1;
  if(ztip_test_point==1)
    {
    vptr->x = BUILD_TABLE_LEN_X/2 + Tool[slot].x_offset + xoffset;;
    vptr->y = BUILD_TABLE_LEN_Y/2 + Tool[slot].y_offset + yoffset;
    vptr->z=5.0;
    vptr->attr=MDL_BORDER;
    }
  ztip_test_point++;
  
  printf("\ntpt=%d x=%f y=%f\n",ztip_test_point,vptr->x,vptr->y);
  
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    // raise z
    tinyGSnd("{\"posz\":NULL}\n");					// get current z position
    while(tinyGRcv(1)>=0);						// ensure it is processed
    sprintf(gcode_cmd,"G0 Z%f \n",PostG.z+5);				// move the carriage up 5 mm
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
  #endif
    
  // goto test point
  sprintf(gcode_cmd,"G0 X%f Y%f Z%f\n",vptr->x,vptr->y,vptr->z);
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);
  
  // lower z
  sprintf(gcode_cmd,"G0 Z0.0 \n");
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);

  free(vptr);vertex_mem--;
  
  return;
}

// Function to draw grid for XY tip alignment
void xyztip_draw_alignment(GtkWidget *btn, gpointer user_data)
{
  int 		slot;
  float 	mid_x,mid_y,off_x,off_y, siz_x,siz_y;
  vertex	*vpt;
  
  slot=GPOINTER_TO_INT(user_data);
  
  mid_x=(BUILD_TABLE_LEN_X/2.0);
  mid_y=(BUILD_TABLE_LEN_Y/2.0);
  off_x=100.0; off_y=100.0;
  siz_x=50.0; siz_y=50.0;
  
  vpt=vertex_make(); 
  
  vpt->x=off_x+mid_x-siz_x/2; vpt->y=off_y+mid_y-siz_y/2; vpt->z=0.2;
  print_vertex(vpt,slot,1,NULL);					// generate pen down without z comp
  tinyGSnd(gcode_burst);						// execute pen down move
  motion_complete();							// wait until it full completes
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  
  vpt->x=off_x+mid_x+siz_x/2; vpt->y=off_y+mid_y-siz_y/2; vpt->z=0.2;
  print_vertex(vpt,slot,1,NULL);					// generate pen down without z comp
  tinyGSnd(gcode_burst);						// execute pen down move
  motion_complete();							// wait until it full completes
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  
  vpt->x=off_x+mid_x+siz_x/2; vpt->y=off_y+mid_y+siz_y/2; vpt->z=0.2;
  print_vertex(vpt,slot,1,NULL);					// generate pen down without z comp
  tinyGSnd(gcode_burst);						// execute pen down move
  motion_complete();							// wait until it full completes
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  
  vpt->x=off_x+mid_x-siz_x/2; vpt->y=off_y+mid_y+siz_y/2; vpt->z=0.2;
  print_vertex(vpt,slot,1,NULL);					// generate pen down without z comp
  tinyGSnd(gcode_burst);						// execute pen down move
  motion_complete();							// wait until it full completes
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  
  free(vpt);

  return;
}


// Function to stop tip calibration on user request
void stop_tip_calib(GtkWidget *btn, gpointer dead_window)
{
  float 	retract_z=15.0;
  
  // abort what ever motion may be going on
  tinyGSnd("!\n");							// abort move when switch hit
  while(tinyGRcv(1)>=0);
  tinyGSnd("%\n");							// clear buffer - note: these cmds do not produce a response from tinyG
  while(tinyGRcv(1)>=0);

  // raise carriage
  sprintf(gcode_burst,"G0 Z%f \n",retract_z);				// lift carriage to clear anything before xy move
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);
  
  // move to xy home
  sprintf(gcode_burst,"G0 X0.000 Y0.000 \n");				// move to home
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);

  // move to z home
  sprintf(gcode_burst,"G0 Z0.000 \n");					// move to home
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);

  tools_need_tip_calib=TRUE;
  gtk_window_close(GTK_WINDOW(dead_window));
  gtk_widget_queue_draw(GTK_WIDGET(win_main));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

// Auto Calibration callback for setting tool tip location
// The idea is to use the limit switches mounted on the edges of the build table to determine z, then x and y.
// note that this does not actually calibrate the tools' location relative to build table 0,0,0, rather it
// calibrates them relative to each other.  the lowest number slot with a tool in it becomes the reference
// and all other installed tools calibrate relative to it.
void xyztip_autocal_callback (GtkWidget *btn, gpointer user_data)
{
  int 		h,i,slot,first_slot,last_slot, max_reads, cnt_reads, old_cmd_state;
  float 	sense_speed,oldz,newx,newy,newz_x,newz_y;
  float 	switch_z,retract_z;
  vertex 	*szx,*szy;
  time_t 	sense_start_time,sense_finish_time;
  FILE		*tip_data;
  GtkWidget	*win_dialog,*grid_tip_calib,*btn_done,*lbl_info,*lbl_slot,*lbl_space;
  
  // don't allow on units without switches
  if(HAS_AUTO_TIP_CALIB==FALSE)return;
  
  // check if doing one specific tool, or the whole set
  slot=GPOINTER_TO_INT(user_data);
  first_slot=slot;
  last_slot=slot+1;
  if(slot<0 || slot>=MAX_TOOLS){first_slot=0;last_slot=MAX_TOOLS;}
  
  // init
  first_layer_flag=FALSE;
  max_reads=500;
  sense_speed=2000.00;

  // open the file
  tip_data=fopen("TIP_DATA.csv","w");

  // build a specialized dialog window (default type won't work here)
  win_dialog = gtk_window_new ();
  gtk_window_set_transient_for (GTK_WINDOW(win_dialog), GTK_WINDOW(win_main));	
  gtk_window_set_modal(GTK_WINDOW(win_dialog),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_dialog),400,200);
  gtk_window_set_resizable(GTK_WINDOW(win_dialog),FALSE);			
  sprintf(scratch," Tool Tip Calibration ");
  gtk_window_set_title(GTK_WINDOW(win_dialog),scratch);			
  
  g_signal_connect (win_dialog, "destroy", G_CALLBACK (stop_tip_calib), win_dialog);

  grid_tip_calib=gtk_grid_new();
  gtk_window_set_child(GTK_WINDOW(win_dialog),grid_tip_calib);
  gtk_grid_set_row_spacing (GTK_GRID(grid_tip_calib),10);
  gtk_grid_set_column_spacing (GTK_GRID(grid_tip_calib),10);

  lbl_info= gtk_label_new ("Tool Tip Calibration");
  gtk_grid_attach (GTK_GRID(grid_tip_calib), lbl_info, 0, 0, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(lbl_info),0.5);
  if(slot<0 || slot>=MAX_TOOLS){sprintf(scratch,"Checking all tools");}
  else {sprintf(scratch,"Checking tool %d ",(slot+1));}
  lbl_slot= gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID(grid_tip_calib), lbl_slot, 1, 1, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(lbl_slot),0.5);
  lbl_space= gtk_label_new("      ");
  gtk_grid_attach (GTK_GRID(grid_tip_calib), lbl_space, 0, 2, 1, 1);

  btn_done = gtk_button_new_with_label("Cancel");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (stop_tip_calib), win_dialog);
  gtk_grid_attach (GTK_GRID(grid_tip_calib), btn_done, 1, 3, 1, 1);

  gtk_widget_set_visible(win_dialog,TRUE);
  
  // raise the up position of all tools to ensure they clear the table during calib
  for(h=0;h<MAX_TOOLS;h++)Tool[h].tip_up_pos+=5;
  
  // loop added for repeatability study... not necessary for typical operation
  for(i=0;i<1;i++)
    {
  
    for(slot=first_slot;slot<last_slot;slot++)
      {
      // don't check tools that are not fully defined
      if(Tool[slot].state<TL_READY)continue;

      // update window info
      sprintf(scratch,"Checking tool in slot %d",(slot+1));
      gtk_label_set_text(GTK_LABEL(lbl_slot),scratch);
      gtk_widget_queue_draw(lbl_slot);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

      // move slot we are testing into z deposit position and retract all the others
      for(h=0;h<MAX_TOOLS;h++)
        {
	if(h==slot)continue;
	if(Tool[h].state<=TL_EMPTY)continue;
	tool_tip_retract(h);
	}
      tool_tip_deposit(slot);
      
      // ensure all steppers are OFF so they do not overheat.  normally it should be done after each call to
      // change the tool position, but can be done here in one shot since so little time between them.  tool
      // changes are a time hit (see details in that function)... minimize calls to it if possible.
      make_toolchange((-1),0);						
	
      // init trigger position values
      newx=0; newy=0; newz_x=0; newz_y=0;
      retract_z=10.0;
      
      // the location of switches are defined as if they were model locations on the table.  that is, the x/y offsets from origin
      // and tool offsets must be accounted for since we want this to work for any tool in any slot.
      szx=vertex_copy(xsw_pos,NULL);					// copy over default position of x switch
      szx->k=sense_speed;						// assign approach feed rate
      szx->z=retract_z;							// start at 5mm above switch for z check
      szy=vertex_copy(ysw_pos,NULL);					// copy over default position of y switch
      szy->k=sense_speed;						// assign appraoch feed rate
      szy->z=retract_z;							// start at 5mm above switch for z check
    
      // check tip position in z then x
      {
	printf("\n\nRun:%d - CHECKING Z OVER X SWITCH...\n\n",i);
	
	// raise carriage
	sprintf(gcode_burst,"G0 Z%f \n",retract_z);			// lift carriage to clear anything before xy move
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	while(tinyGRcv(1)>=0);
	
	// move tool over zx switch
	print_vertex(szx,slot,2,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	
	// drop Z toward build table while monitoring the tool tip's limit switch (similar to homing)
	old_cmd_state=cmdControl;
	cmdControl=0;
	cnt_reads=0;
	newz_x=0;
	switch_z=xsw_pos->z;						// define max down location of switch
	sprintf(gcode_burst,"G1 Z%f F100.0\n",switch_z);		// slowly move head toward build table
	tinyGSnd(gcode_burst);
	while(RPi_GPIO_Read(TOOL_TIP)==TRUE && cnt_reads<max_reads){tinyGRcv(1);cnt_reads++;}	// constantly check tool limit switch during move - Unit 2
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	tinyGSnd("!\n");						// abort move when switch hit
	while(tinyGRcv(1)>=0);
	tinyGSnd("%\n");						// clear buffer - note: these cmds do not produce a response from tinyG
	while(tinyGRcv(1)>=0);
	cmdControl=old_cmd_state;
	cnt_reads=0;							// init escape counter
	while(fabs(newz_x-PostG.z)>TOLERANCE)				// iterate here until motion is settled
	  {
	  newz_x=PostG.z;						// update latest position
	  tinyGSnd("{\"posz\":NULL}\n");				// request z position
	  while(tinyGRcv(1)>=0);					// see what tinyg said
	  cnt_reads++;							// increment escape counter
	  if(cnt_reads>25)break;					// if here too long...
	  }
	newz_x=PostG.z;							// record z value
	PosIs.z=PostG.z;						// update current positon
	
	// retract z
	sprintf(gcode_burst,"G0 Z%f \n",retract_z);			// lift carriage to clear anything before xy move
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	while(tinyGRcv(1)>=0);
    
	// validate z check result
	if(fabs(newz_x-switch_z)<CLOSE_ENOUGH)
	  {
	  printf("\nAuto Tip Calib - Z-Check on X Sensor:  Sensor not triggered! \n");
	  printf("    cnt_reads=%d  max_reads=%d   newz_x=%f \n",cnt_reads,max_reads,newz_x);
	  return;
	  }
	
	// move to start of x sweep
	printf("\n\nRun: %d - CHECKING X OVER X SWITCH...\n",i);
	szx->x += 15; szx->z=newz_x+0.10;				// adjust position to start of x sweep
	print_vertex(szx,slot,2,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	
	// drag tool tip in x direction over zx switch and determine x trigger point
	old_cmd_state=cmdControl;
	cmdControl=0;
	cnt_reads=0;
	newz_x=0.0;
	szx->x -= 30;
	print_vertex(szx,slot,7,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	while(RPi_GPIO_Read(TOOL_TIP)==TRUE && cnt_reads<max_reads){tinyGRcv(1);cnt_reads++;}	// constantly check tool limit switch during move - Unit 2
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	tinyGSnd("!\n");						// abort move when switch hit
	while(tinyGRcv(1)>=0);
	tinyGSnd("%\n");						// clear buffer - note: these cmds do not produce a response from tinyG
	while(tinyGRcv(1)>=0);
	cmdControl=old_cmd_state;
	cnt_reads=0;
	while(fabs(newz_x-PostG.z)>TOLERANCE)
	  {
	  newz_x=PostG.z;
	  tinyGSnd("{\"posz\":NULL}\n");
	  while(tinyGRcv(1)>=0);
	  cnt_reads++;
	  if(cnt_reads>25)break;
	  }
	newx=PostG.x;							// record x value
	PosIs.z=PostG.z;
      
	printf("\n\nX=%f at Z=%f\n\n",newx,newz_x);
	//while(!kbhit());
      
	// retract
	sprintf(gcode_burst,"G0 Z%f \n",retract_z);			// lift carriage to clear anything before xy move
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	while(tinyGRcv(1)>=0);
	
	// validate x sweep result
	if(fabs(newx-szx->x)<CLOSE_ENOUGH)
	  {
	  printf("\nAuto Tip Calib - X-Sweep:  Sensor not triggered! \n");
	  printf("    cnt_reads=%d  max_reads=%d   newx=%f  x_target=%f\n",cnt_reads,max_reads,newx,szx->x);
	  return;
	  }
	
      }
      
      // check tip position in y
      {
	printf("\n\n%d CHECKING Z OVER Y SWITCH...\n",i);
	// move tool over zy switch and determine z trigger point of this switch
	print_vertex(szy,slot,2,NULL);					// build command
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	
	// drop Z toward build table while monitoring the tool tip's limit switch (similar to homing)
	old_cmd_state=cmdControl;
	cmdControl=0;
	cnt_reads=0;
	newz_y=0;			
	switch_z=ysw_pos->z;		
	sprintf(gcode_burst,"G1 Z%f F100.0\n",switch_z);		// slowly move head toward build table
	tinyGSnd(gcode_burst);
	while(RPi_GPIO_Read(TOOL_TIP)==TRUE && cnt_reads<max_reads){tinyGRcv(1);cnt_reads++;}	// constantly check tool limit switch during move - Unit 2
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	tinyGSnd("!\n");						// abort move when switch hit
	while(tinyGRcv(1)>=0);
	tinyGSnd("%\n");						// clear buffer - note: these cmds do not produce a response from tinyG
	while(tinyGRcv(1)>=0);
	cmdControl=old_cmd_state;
	cnt_reads=0;
	while(fabs(newz_y-PostG.z)>TOLERANCE)
	  {
	  newz_y=PostG.z;
	  tinyGSnd("{\"posz\":NULL}\n");
	  while(tinyGRcv(1)>=0);
	  cnt_reads++;
	  if(cnt_reads>25)break;
	  }
	newz_y=PostG.z;							// record z value
	PosIs.z=PostG.z;
	
	// retract
	sprintf(gcode_burst,"G0 Z%f \n",retract_z);			// lift carriage to clear anything before xy move
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	while(tinyGRcv(1)>=0);
    
	// validate z check result
	if(fabs(newz_y-switch_z)<CLOSE_ENOUGH)
	  {
	  printf("\nAuto Tip Calib - Z-Check on Y Sensor:  Sensor not triggered! \n");
	  printf("    cnt_reads=%d  max_reads=%d   newz_y=%f \n",cnt_reads,max_reads,newz_y);
	  return;
	  }
	
	printf("\n\n%d CHECKING Y OVER Y SWITCH...\n",i);
	// drag tool tip in y direction over zy switch and determine y trigger point
	szy->y += 15; szy->z=newz_y+0.10;
	print_vertex(szy,slot,2,NULL);
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();
	
	// record values as tool position
	// drop Z toward build table while monitoring the tool tip's limit switch (similar to homing)
	old_cmd_state=cmdControl;
	cmdControl=0;
	cnt_reads=0;
	newz_y=0;
	szy->y -= 30;
	print_vertex(szy,slot,7,NULL);
	tinyGSnd(gcode_burst);
	while(RPi_GPIO_Read(TOOL_TIP)==TRUE && cnt_reads<max_reads){tinyGRcv(1);cnt_reads++;}	// constantly check tool limit switch during move - Unit 2
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	tinyGSnd("!\n");						// abort move when switch hit
	while(tinyGRcv(1)>=0);
	tinyGSnd("%\n");						// clear buffer - note: these cmds do not produce a response from tinyG
	while(tinyGRcv(1)>=0);
	cmdControl=old_cmd_state;
	cnt_reads=0;
	while(fabs(newz_y-PostG.z)>TOLERANCE)
	  {
	  newz_y=PostG.z;
	  tinyGSnd("{\"posz\":NULL}\n");
	  while(tinyGRcv(1)>=0);
	  cnt_reads++;
	  if(cnt_reads>25)break;
	  }
	newy=PostG.y;							// record y value
	PosIs.z=PostG.z;
	
	printf("\n\nY=%f at Z=%f \n\n",newy,newz_y);
	//while(!kbhit());
      
	// retract
	sprintf(gcode_burst,"G0 Z%f \n",retract_z);			// lift carriage to clear anything before xy move
	tinyGSnd(gcode_burst);
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	while(tinyGRcv(1)>=0);
	
	// validate y sweep result
	if(fabs(newy-szy->y)<CLOSE_ENOUGH)
	  {
	  printf("\nAuto Tip Calib - Y-Sweep:  Sensor not triggered! \n");
	  printf("    cnt_reads=%d  max_reads=%d   newx=%f  x_target=%f\n",cnt_reads,max_reads,newy,szy->y);
	  return;
	  }
    
      }
      
      // we want to now derive where the switches triggered relative to the existing tip offset values.
      // since newx/y is provided in the unit coord frame, we need to add in the offsets to understand where it
      // lands in the tool coord frame and get that difference from where we think it ought to have triggered.
      newx -= (xoffset+crgslot[slot].x_center);
      newy -= (yoffset+crgslot[slot].y_center);
      if(slot==0 || slot==2)newx-=Tool[slot].x_offset;			// tool mounted farther from x home
      if(slot==1 || slot==3)newx+=Tool[slot].x_offset;			// tool mounted nearer to x home
      if(slot==0 || slot==2)newy+=Tool[slot].y_offset;
      if(slot==1 || slot==3)newy-=Tool[slot].y_offset;			// account for tool being flipped 180 degrees on carriage
      newx -= xsw_pos->x;
      newy -= ysw_pos->y;
      
      //newz_x=newz_x+(xsw_pos->z+1.451);					// compensate for switch actual position
      
      Tool[slot].swch_pos->x=newx;					// x value is from x switch
      Tool[slot].swch_pos->y=newy;					// y value is from y switch
      Tool[slot].swch_pos->z=newz_x;					// z value is from x switch
      
      Tool[slot].mmry.md.tip_pos_X=newx;
      Tool[slot].mmry.md.tip_pos_Y=newy;
      Tool[slot].mmry.md.tip_pos_Z=newz_x;
  
      if(tip_data!=NULL)
	{
	sprintf(scratch,"%7.3f %7.3f %7.3f %7.3f %d\n",newx,newz_x,newy,newz_y,cnt_reads);
	fwrite(scratch,1,strlen(scratch),tip_data);
	fflush(tip_data);
	}

      // clean up before moving onto next tool
      free(szx); vertex_mem--; szx=NULL;
      free(szy); vertex_mem--; szy=NULL;
      
      }	// end of slot incrment for loop
      
    for(slot=first_slot;slot<last_slot;slot++)  
      {
      if(Tool[slot].state<TL_READY)continue;
      printf("\nslot=%d  x=%7.3f  y=%7.3f  z=%7.3f \n",slot,Tool[slot].swch_pos->x,Tool[slot].swch_pos->y,Tool[slot].swch_pos->z);
      }
      
    } // end of for loop

  // reset the up position of all tools
  for(h=0;h<MAX_TOOLS;h++)Tool[h].tip_up_pos-=5;
  
  // put all tools back into their nominal up position
  for(h=0;h<MAX_TOOLS;h++)tool_tip_retract(h);

  fclose(tip_data);
  
  // raise carriage
  sprintf(gcode_burst,"G0 Z%f \n",retract_z);				// lift carriage to clear anything before xy move
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);
  
  // move to xy home
  sprintf(gcode_burst,"G0 X0.000 Y0.000 \n");				// move to home
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);

  // move to z home
  sprintf(gcode_burst,"G0 Z0.000 \n");					// move to home
  tinyGSnd(gcode_burst);
  memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
  while(tinyGRcv(1)>=0);

  tools_need_tip_calib=FALSE;

  gtk_window_close(GTK_WINDOW(win_dialog));
  gtk_widget_queue_draw(GTK_WIDGET(win_main));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

// Function to calibrate tool tip position both automatically and manually
void xyztip_manualcal_callback(GtkWidget *btn, gpointer user_data)
{
  int 		slot,h,i;
  int		manual_cal=TRUE;
  float 	fval;
  vertex	*vptr;
  model		*mptr,*old_active_model;
  GtkWidget	*win_tl_stat,*grd_tl_stat;
  GtkWidget	*win_dialog,*win_tlam_stat,*grd_tlam_stat;
  GtkWidget	*grd_stat,*lbl_da,*lbl_scratch;
  GtkWidget	*btn_z_up,*btn_z_dn,*btn_test,*btn_autocal,*separator;
  GtkAdjustment	*tip_diam_adj;
  GtkWidget	*tip_diam_btn;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(Tool[slot].state<TL_LOADED)					// can't adjust when not yet defined
    {
    sprintf(scratch," \nTool not yet defined.\nSelect tool type first. \n");
    aa_dialog_box(win_main,1,0,"Tool Tip Calibration",scratch);
    return;
    }
  old_active_model=active_model;
  active_model=NULL;
  make_toolchange(slot,2);						// turn on carriage stepper drive
  
  win_tl_stat = gtk_window_new();				
  gtk_window_set_transient_for (GTK_WINDOW(win_tl_stat), GTK_WINDOW(win_tool));
  gtk_window_set_modal (GTK_WINDOW(win_tl_stat),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_tl_stat),LCD_WIDTH/2,LCD_HEIGHT/2);
  gtk_window_set_resizable(GTK_WINDOW(win_tl_stat),FALSE);			
  sprintf(scratch,"Tool %d XYZ Tip Adjustment",slot);
  gtk_window_set_title(GTK_WINDOW(win_tl_stat),scratch);			

  grd_tl_stat = gtk_grid_new ();						
  gtk_window_set_child(GTK_WINDOW(win_tl_stat),grd_tl_stat);
  gtk_grid_set_row_spacing (GTK_GRID(grd_tl_stat),15);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tl_stat),15);

  //lbl_da=gtk_label_new("Adjust XYZ tip position:");
  //gtk_grid_attach (GTK_GRID(grd_tl_stat), lbl_da, 1, 1, 3, 1);
  
  // setup automatical calib section
  {
    btn_autocal = gtk_button_new_with_label ("Auto Cal");
    g_signal_connect (btn_autocal, "clicked", G_CALLBACK (xyztip_autocal_callback), GINT_TO_POINTER(slot));
    gtk_grid_attach (GTK_GRID (grd_tl_stat), btn_autocal, 1, 1, 1, 1);
    lbl_scratch=gtk_label_new(" Ensure build table is clear.\nPress Auto Cal when ready.");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 2, 1, 2, 1);
  }

  separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
  gtk_grid_attach (GTK_GRID(grd_tl_stat),separator, 0, 2, 4, 1);

  // setup manual calib section
  {
    lbl_scratch=gtk_label_new("  X:");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 0, 3, 1, 1);
    fval=Tool[slot].mmry.md.tip_pos_X;
    xtipadj=gtk_adjustment_new(fval, -25, 25, 0.1, 1, 5);
    xtipbtn=gtk_spin_button_new(xtipadj, 5, 3);
    gtk_grid_attach(GTK_GRID(grd_tl_stat),xtipbtn, 1, 3, 1, 1);
    g_signal_connect(xtipbtn,"value-changed",G_CALLBACK(grab_xtip_value),GINT_TO_POINTER(slot));
    lbl_scratch=gtk_label_new(" For X, + moves the tip toward home.");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 2, 3, 1, 1);
    
    lbl_scratch=gtk_label_new("  Y:");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 0, 4, 1, 1);
    fval=Tool[slot].mmry.md.tip_pos_Y;
    ytipadj=gtk_adjustment_new(fval, -25, 25, 0.1, 1, 5);
    ytipbtn=gtk_spin_button_new(ytipadj, 5, 3);
    gtk_grid_attach(GTK_GRID(grd_tl_stat),ytipbtn, 1, 4, 1, 1);
    g_signal_connect(ytipbtn,"value-changed",G_CALLBACK(grab_ytip_value),GINT_TO_POINTER(slot));
    lbl_scratch=gtk_label_new(" For Y, + moves the tip away from home.");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 2, 4, 1, 1);
  
    lbl_scratch=gtk_label_new("  Z:");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 0, 5, 1, 1);
    ztipadj=gtk_adjustment_new(Tool[slot].mmry.md.tip_pos_Z, -25, 25, 0.1, 1, 5);
    ztipbtn=gtk_spin_button_new(ztipadj, 5, 3);
    gtk_grid_attach(GTK_GRID(grd_tl_stat),ztipbtn, 1, 5, 1, 1);
    g_signal_connect(ztipbtn,"value-changed",G_CALLBACK(grab_ztip_value),GINT_TO_POINTER(slot));
    lbl_scratch=gtk_label_new(" Adjust to a paper thickness\n+ is away from table");
    gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 2, 5, 1, 1);
    
    //btn_test = gtk_button_new_with_label ("Next");					// add in "next" button
    //g_signal_connect (btn_test, "clicked", G_CALLBACK (xyztip_next_callback), GINT_TO_POINTER(slot));
    //gtk_grid_attach (GTK_GRID (grd_tl_stat), btn_test, 1, 7, 1, 1);
    //lbl_scratch=gtk_label_new(" Move to next location on table");
    //gtk_grid_attach (GTK_GRID(grd_tl_stat),lbl_scratch, 2, 7, 1, 1);
  }

  separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
  gtk_grid_attach (GTK_GRID(grd_tl_stat),separator, 0, 8, 4, 1);

  btn_done = gtk_button_new_with_label ("Done");					// add in "done" button
  g_signal_connect (btn_done, "clicked", G_CALLBACK (xyztip_done_callback), win_tl_stat);
  gtk_grid_attach (GTK_GRID(grd_tl_stat), btn_done, 2, 9, 1, 1);

  gtk_widget_set_visible(win_tl_stat,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  // if any tool is not homed on carraige, do that first since they ALL must be out of the way when
  // moving the carraige down close to the build table.
  for(i=0;i<MAX_TOOLS;i++)
    {
    if(Tool[i].state==TL_EMPTY)continue;				// home all tools, even if not ready for use
    tool_tip_home(i);
    //if(Tool[i].tip_cr_pos!=Tool[i].tip_up_pos)tool_tip_home(i);
    }

  // move tool over table and drop into position
  if(manual_cal==TRUE)
    {
    old_z_tbl_comp=z_tbl_comp;						// save current state
    z_tbl_comp=TRUE;							// turn ON to get best tip distance calib
    //z_tbl_comp=FALSE;
    z_cut=0.0;								// ensure we are at table height
    
    // caclulate target coords over build table for where to check offset
    vptr=vertex_make();
    vptr->x=(BUILD_TABLE_LEN_X/2);					// middle of x
    vptr->y=(BUILD_TABLE_LEN_Y/2);					// middle of y
    vptr->z=30.0;							// near surface of build table
    vptr->attr=MDL_BORDER;

    // goto test point.  note this uses print_vertex to include all other offsets including z table compensation
    if(carriage_at_home==TRUE)
      {
      comefrom_machine_home(slot,vptr);
      }
    else 
      {
      memset(gcode_burst,0,sizeof(gcode_burst));	
      print_vertex(vptr,slot,2,NULL);
      tinyGSnd(gcode_burst);			
      memset(gcode_burst,0,sizeof(gcode_burst));	
      motion_complete();
      }
    
    // lower tool into position
    tool_tip_deposit(slot);
    
    // lower from 5mm z down onto surface
    vptr->z=0.00;
    if(strstr(Tool[slot].name,"ROUTER")!=NULL)vptr->z=cnc_z_height+10;
    memset(gcode_burst,0,sizeof(gcode_burst));	
    print_vertex(vptr,slot,2,NULL);
    tinyGSnd(gcode_burst);			
    memset(gcode_burst,0,sizeof(gcode_burst));	
    motion_complete();
  
    free(vptr);vertex_mem--;						// release memory
    }

  //active_model=old_active_model;
  //ztip_next_callback (btn_test, GINT_TO_POINTER(slot));
  gtk_widget_set_visible(win_tl_stat,TRUE);

  return;
}

// Function to clear tool tip position manually
void xyztip_clear_callback(GtkWidget *btn, gpointer user_data)
{
  int 		slot,h,i;
  float 	fval;
  model		*old_active_model;

  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(Tool[slot].state<TL_LOADED)					// can't adjust when not yet defined
    {
    sprintf(scratch," \nTool not yet defined.\nSelect tool type first. \n");
    aa_dialog_box(win_main,1,0,"Tool Tip Calibration",scratch);
    return;
    }
    
  Tool[slot].tip_x_fab=0.000;
  Tool[slot].tip_y_fab=0.000;
  Tool[slot].tip_z_fab=0.000;
  
  Tool[slot].mmry.md.tip_pos_X=0.000;
  Tool[slot].mmry.md.tip_pos_Y=0.000;
  Tool[slot].mmry.md.tip_pos_Z=0.000;

  // update display(s) as needed
  if(win_tool_flag[slot]==TRUE)
    {
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].x_offset,Tool[slot].x_offset,Tool[slot].tip_x_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipx),scratch);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].y_offset,Tool[slot].y_offset,Tool[slot].tip_y_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipy),scratch);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].z_offset,Tool[slot].tip_dn_pos,Tool[slot].tip_z_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipzdn),scratch);
    }
  
  return;
}


int tool_temp_override(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*label;
  GtkWidget	*win_dialog;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].pwr24==FALSE || Tool[slot].thrm.sensor<1)
    {
    sprintf(scratch," \nThis tool is not configured for thermal \n control.  Check tool settings. \n");
    aa_dialog_box(win_main,1,0,"Tool Temperature Control",scratch);
    return(0);
    }
  
  return(1);
}

// Callback to get tool temperature override
void temp_override_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(Tool[slot].state==TL_EMPTY)return;

  Tool[slot].thrm.setpC=gtk_spin_button_get_value(button);
  if(Tool[slot].thrm.setpC<0)Tool[slot].thrm.setpC=0;
  if(Tool[slot].thrm.setpC>500)Tool[slot].thrm.setpC=500;
  //Tool[slot].thrm.operC=Tool[slot].thrm.setpC;
  printf("\nTemp override called.\n\n");
  
  return;
}


void grab_low_temp_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot;
  float 	temp_chk,volt_chk;
  
  // record low temp calib points
  idle_start_time=time(NULL);						// reset idle time
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  Tool[slot].thrm.sensor=RTDH;						// force type to RTD for calib loads

  temp_chk=gtk_spin_button_get_value(button)-crgslot[slot].temp_offset;	// get meter temperature less the offset for carriage to board resistance
  volt_chk=Tool[slot].thrm.tempV;					// get input voltage for that meter temperature

  Tool[slot].thrm.calib_t_low=temp_chk;					// save temperature
  Tool[slot].thrm.calib_v_low=volt_chk;					// save matching input voltage
  delay(500);
  
  // reset set point to now get ready for high temp reading
  //pthread_mutex_lock(&thermal_lock);      
  //Tool[slot].thrm.setpC=Tool[slot].thrm.operC+temp_calib_delta;
  //pthread_mutex_unlock(&thermal_lock);      

  // clean up
  //gtk_widget_set_sensitive(temp_cal_sys_lowbtn,FALSE);
  //gtk_widget_set_sensitive(temp_cal_sys_highbtn,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void grab_high_temp_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot,i;
  float 	temp_chk,volt_chk;
  
  // record high temp calib points
  idle_start_time=time(NULL);						// reset idle time
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  Tool[slot].thrm.sensor=RTDH;						// force type to RTD for calib loads
  
  volt_chk=Tool[slot].thrm.tempV;					// get input voltage for that meter temperature
  temp_chk=gtk_spin_button_get_value(button)-crgslot[slot].temp_offset;	// get meter temperature less the offset for carriage to board resistance

  Tool[slot].thrm.calib_t_high=temp_chk;				// save temperature
  Tool[slot].thrm.calib_v_high=volt_chk;				// save matching input voltage
  delay(500);
  
  // update voltage displayed value
  sprintf(scratch,"corrisponding voltage: %6.5fv",Tool[slot].thrm.calib_v_high);
  gtk_label_set_text(GTK_LABEL(tcal_vhi_lbl),scratch);

  // clean up
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void tool_temp_calib_done(GtkWidget *btn, gpointer dead_window)
{

  
  //pthread_mutex_lock(&thermal_lock);      
  //if(current_tool>=0 && current_tool<MAX_TOOLS)Tool[current_tool].thrm.setpC=Tool[current_tool].thrm.operC;
  //pthread_mutex_unlock(&thermal_lock);      

  // set low calib data based on start data
  //if(Tool[current_tool].thrm.start_t_low>0)					// only use if data was collected...
  //  {
  //  Tool[current_tool].thrm.calib_t_low=Tool[current_tool].thrm.start_t_low;
  //  Tool[current_tool].thrm.calib_v_low=Tool[current_tool].thrm.start_v_low;
  //  }

  win_tooloffset_flag=FALSE;
  gtk_window_close(GTK_WINDOW(dead_window));
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
}

void tool_temp_offset(GtkWidget *btn_call, gpointer user_data)
{
    int		slot;
    GtkWidget 	*win_tcal;
    GtkWidget	*grd_tcal;
    GtkWidget	*label;
    
    idle_start_time=time(NULL);						// reset idle time
    slot=GPOINTER_TO_INT(user_data);
    if(slot<0 || slot>=MAX_TOOLS)return;
    
    if(Tool[slot].pwr24==FALSE || Tool[slot].thrm.sensor<1)
      {
      sprintf(scratch," \nThis tool is not configured for thermal \n control.  Check tool settings. \n");
      aa_dialog_box(win_main,1,0,"Tool Temperature Offset",scratch);
      return;
      }
    
    win_tooloffset_flag=TRUE;
    Tool[slot].thrm.setpC=Tool[slot].thrm.operC;
    device_being_calibrated=slot;
    
    // build our window
    win_tcal = gtk_window_new();	
    gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
    gtk_window_set_modal(GTK_WINDOW(win_tcal),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_tcal),250,100);
    gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
    sprintf(scratch,"Temperature Offset");
    gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
      
    // create a grid to hold information
    grd_tcal = gtk_grid_new ();				
    gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
    gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
    gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);

    sprintf(scratch,"  Attach probe to tool.  Adjust offset until values match.");
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 1, 3, 1);
    gtk_label_set_xalign (GTK_LABEL(label),0.0);

    sprintf(scratch,"  Current reading: ");
    GtkWidget *lbl_current_reading=gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), lbl_current_reading, 0, 2, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_current_reading),0.0);
    
    sprintf(scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.tempC,Tool[device_being_calibrated].thrm.tempV);
    temp_tool_disp=gtk_label_new(scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), temp_tool_disp, 1, 2, 1, 1);
    
    sprintf(scratch,"  Temperature offset: ");
    GtkWidget *lbl_temp_offset = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), lbl_temp_offset, 0, 3, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_temp_offset),0.0);
    
    temp_cal_offset=gtk_adjustment_new(Tool[device_being_calibrated].thrm.temp_offset, -100, 100, 1, 10, 25);
    temp_cal_offset_btn=gtk_spin_button_new(temp_cal_offset, 3, 1);
    gtk_grid_attach(GTK_GRID(grd_tcal),temp_cal_offset_btn, 1, 3, 1, 1);
    g_signal_connect(temp_cal_offset_btn,"value-changed",G_CALLBACK(grab_temp_cal_offset),GINT_TO_POINTER(device_being_calibrated));
    
    btn_done = gtk_button_new_with_label ("Done");
    g_signal_connect (btn_done, "clicked", G_CALLBACK (tool_temp_calib_done), win_tcal);
    gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 0, 4, 1, 1);

    gtk_widget_set_visible(win_tcal,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

    return;
}

int tool_temp_calib(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*label;
  GtkWidget	*win_dialog;
  
  // like most calibrations, this one is specific to the tool only.  unit/carriage calibration should be
  // done before this.  to get the right answer for tool calibartion, the unit/carriage calibration has
  // to be negated out from it.
  
  // what is happening here:
  // the lower calibration point was already collected when the tool was first plugged into a slot.  it uses
  // the chamber temperature as the reference value (versus a meter) under the assumption that the chamber was 
  // calibrated properly when the machine was first brought to life.  this ultimately saves the user time by not
  // having to change temps between high and low to get the two calibration points. so this function basically 
  // collects the high calib point (using a meter) and then saves the info into the tool's calibration variables.
  // calibration corrections are made on the fly in the sensor function using the tool calib variables.

  idle_start_time=time(NULL);						// reset idle time
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].pwr24==FALSE || Tool[slot].thrm.sensor<1)
    {
    sprintf(scratch," \nThis tool is not configured for thermal \n control.  Check tool settings. \n");
    aa_dialog_box(win_main,1,0,"Tool Temperature Control",scratch);
    return(0);
    }

  // set tool set point to low temp calib temperature
  //pthread_mutex_lock(&thermal_lock);      
  Tool[slot].thrm.setpC=Tool[slot].thrm.operC;				// set to high temp calib point
  //pthread_mutex_unlock(&thermal_lock);      
  idle_start_time=time(NULL);						// reset idle time

  // build our window
  win_tcal = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
  gtk_window_set_modal(GTK_WINDOW(win_tcal),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_tcal),350,180);
  gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
  sprintf(scratch,"Temperature Calibration");
  gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
  
  // create a grid to hold information
  grd_tcal = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
  gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);
  
  sprintf(scratch,"  High Point:  Enter the meter reading once the");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 1, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  sprintf(scratch,"  system temperature stablizes near %6.1f C",Tool[slot].thrm.operC);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 2, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  
  sprintf(scratch,"  Meter reading: ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 3, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  temp_cal_sys_highadj=gtk_adjustment_new(Tool[slot].thrm.calib_t_high, 0, 300, 0.1, 1, 5);
  temp_cal_sys_highbtn=gtk_spin_button_new(temp_cal_sys_highadj, 5, 1);
  gtk_grid_attach(GTK_GRID(grd_tcal),temp_cal_sys_highbtn, 1, 3, 1, 1);
  g_signal_connect(temp_cal_sys_highbtn,"value-changed",G_CALLBACK(grab_high_temp_value),GINT_TO_POINTER(slot));
  
  sprintf(scratch,"corrisponding voltage: %6.5fv",Tool[slot].thrm.calib_v_high);
  tcal_vhi_lbl=gtk_label_new(scratch);
  gtk_grid_attach(GTK_GRID(grd_tcal),tcal_vhi_lbl, 0, 4, 1, 1);

  label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 5, 3, 1);

  sprintf(scratch,"  Low Point:  These values were read from the");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 6, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  sprintf(scratch,"  chamber RTD: T=%5.3fC  V=%5.3fv",Tool[slot].thrm.start_t_low,Tool[slot].thrm.start_v_low);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 7, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  
  label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 9, 3, 1);

  sprintf(scratch,"  Repeat until meter reading is within 2.0C");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 10, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  sprintf(scratch,"  degrees of the system display at high temp.");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 11, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (tool_temp_calib_done), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 0, 12, 1, 1);

  gtk_widget_set_visible(win_tcal,TRUE);
  gtk_widget_set_sensitive(temp_cal_sys_highbtn,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return(1);
}


void grab_high_duty_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  float delta;
  
  slot=GPOINTER_TO_INT(user_data);
  delta=Tool[slot].powr.calib_v_high-gtk_spin_button_get_value(button);
  if(fabs(delta)<24)Tool[slot].powr.calib_v_high=gtk_spin_button_get_value(button);

  printf("LTC:  Duty=%5.1f  Volt=%5.1f \n",100*Tool[slot].powr.calib_d_low,Tool[slot].powr.calib_v_low);
  printf("HTC:  Duty=%5.1f  Volt=%5.1f \n",100*Tool[slot].powr.calib_d_high,Tool[slot].powr.calib_v_high);

  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void grab_high_duty_done(GtkWidget *btn, gpointer dead_window)
{
  int	slot;
  
  for(slot=0;slot<MAX_TOOLS;slot++)tool_power_ramp(slot,TOOL_48V_PWM,OFF);		// turn off power to all tools

  gtk_window_close(GTK_WINDOW(dead_window));
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
}

int tool_high_duty_calib(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*label;
  GtkWidget	*win_dialog;
  
  idle_start_time=time(NULL);						// reset idle time
  slot=GPOINTER_TO_INT(user_data);
  if(Tool[slot].pwr48==FALSE)
    {
    sprintf(scratch," \nThis tool is not configured for power \n control.  Check tool settings. \n");
    aa_dialog_box(win_main,1,0,"Tool Power Control",scratch);
    return(0);
    }

  // build our window
  win_tcal = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
  gtk_window_set_modal(GTK_WINDOW(win_tcal),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_tcal),200,180);
  gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
  sprintf(scratch,"Tool %d",(slot+1));
  gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
  
  // create a grid to hold information
  grd_tcal = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
  gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);
  
  sprintf(scratch," High Duty Cycle Calibration ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 1, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.5);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  // spin up tool to high duty cycle refrence
  Tool[slot].powr.calib_d_high=0.75;
  Tool[slot].powr.calib_v_high=48.0*Tool[slot].powr.calib_d_high;
  tool_power_ramp(slot,TOOL_48V_PWM,(int)(100*Tool[slot].powr.calib_d_high));

  label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 2, 2, 1);
  
  sprintf(scratch," Enter the meter reading once the ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 3, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  sprintf(scratch," system stablizes near %6.1f v ",Tool[slot].powr.calib_v_high);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 4, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  
  sprintf(scratch," Meter reading...: ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 5, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  duty_cal_sys_highadj=gtk_adjustment_new(Tool[slot].powr.calib_v_high, 0, 300, 0.1, 1, 5);
  duty_cal_sys_highbtn=gtk_spin_button_new(duty_cal_sys_highadj, 5, 1);
  gtk_grid_attach(GTK_GRID(grd_tcal),duty_cal_sys_highbtn, 1, 5, 1, 1);
  g_signal_connect(duty_cal_sys_highbtn,"value-changed",G_CALLBACK(grab_high_duty_value),GINT_TO_POINTER(slot));

  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (grab_high_duty_done), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 1, 7, 1, 1);

  gtk_widget_set_visible(win_tcal,TRUE);
  
  return(1);
}

void grab_low_duty_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  float delta;
  
  slot=GPOINTER_TO_INT(user_data);
  delta=Tool[slot].powr.calib_v_low-gtk_spin_button_get_value(button);
  if(fabs(delta)<24)Tool[slot].powr.calib_v_low=gtk_spin_button_get_value(button);

  printf("LTC:  Duty=%5.1f  Volt=%5.1f \n",100*Tool[slot].powr.calib_d_low,Tool[slot].powr.calib_v_low);
  printf("HTC:  Duty=%5.1f  Volt=%5.1f \n",100*Tool[slot].powr.calib_d_high,Tool[slot].powr.calib_v_high);

  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void grab_low_duty_done(GtkWidget *btn, gpointer dead_window)
{
  int	slot;
  
  for(slot=0;slot<MAX_TOOLS;slot++)tool_power_ramp(slot,TOOL_48V_PWM,OFF);		// turn off power to all tools
  
  gtk_window_close(GTK_WINDOW(dead_window));
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
}

int tool_low_duty_calib(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*label;
  GtkWidget	*win_dialog;
  
  idle_start_time=time(NULL);						// reset idle time
  slot=GPOINTER_TO_INT(user_data);
  if(Tool[slot].pwr48==FALSE)
    {
    sprintf(scratch," \nThis tool is not configured for power \n control.  Check tool settings. \n");
    aa_dialog_box(win_main,1,0,"Tool Power Control",scratch);
    return(0);
    }

  // build our window
  win_tcal = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
  gtk_window_set_modal(GTK_WINDOW(win_tcal),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_tcal),200,180);
  gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
  sprintf(scratch,"Tool %d",(slot+1));
  gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
  
  // create a grid to hold information
  grd_tcal = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
  gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);
  
  sprintf(scratch," Low Duty Cycle Calibration ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 1, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.5);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  // spin up tool to low duty cycle refrence
  Tool[slot].powr.calib_d_low=0.25;
  Tool[slot].powr.calib_v_low=48.0*Tool[slot].powr.calib_d_low;
  tool_power_ramp(slot,TOOL_48V_PWM,(int)(100*Tool[slot].powr.calib_d_low));
  
  label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 2, 2, 1);
  
  sprintf(scratch," Enter the meter reading once the ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 3, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  sprintf(scratch," system stablizes near %6.1f v ",Tool[slot].powr.calib_v_low);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 4, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.0);
  
  sprintf(scratch," Meter reading...: ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 5, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  duty_cal_sys_lowadj=gtk_adjustment_new(Tool[slot].powr.calib_v_low, 0, 300, 0.1, 1, 5);
  duty_cal_sys_lowbtn=gtk_spin_button_new(duty_cal_sys_lowadj, 5, 1);
  gtk_grid_attach(GTK_GRID(grd_tcal),duty_cal_sys_lowbtn, 1, 5, 1, 1);
  g_signal_connect(duty_cal_sys_lowbtn,"value-changed",G_CALLBACK(grab_low_duty_value),GINT_TO_POINTER(slot));

  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (grab_low_duty_done), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 1, 7, 1, 1);

  gtk_widget_set_visible(win_tcal,TRUE);
  
  return(1);
}


void grab_tool_serial_num(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  float delta;
  
  slot=GPOINTER_TO_INT(user_data);
  Tool[slot].sernum=gtk_spin_button_get_value(button);
  
  return;
}

void tool_save_done(GtkWidget *btn, gpointer dead_window)
{
  // write to both tool's memory and unit's disc
  tool_information_write(current_tool);
  gtk_window_close(GTK_WINDOW(dead_window));

  return;
}

void tool_set_defaults(GtkWidget *btn, gpointer dead_window)
{
  // load variables that with data that will flow into ee-prom memory structure
  if(current_tool>=0 && current_tool<MAX_TOOLS)
    {
    Tool[current_tool].powr.power_sensor=UNDEFINED;			// 0=None, 1=voltage, 2=current, 3=force
    Tool[current_tool].powr.power_status=0;				// indicator if on or off
    Tool[current_tool].powr.power_duration=1.00;			// maximum duty cycle for this device (PWM control)
    Tool[current_tool].powr.power_duty=0.50;				// current power duty cycle (PWM control)
  
    Tool[current_tool].thrm.calib_t_low=20.0;
    Tool[current_tool].thrm.calib_v_low=0.88;
    Tool[current_tool].thrm.calib_t_high=230.0;
    Tool[current_tool].thrm.calib_v_high=1.65;

    tool_information_write(current_tool);
    }
  gtk_window_close(GTK_WINDOW(dead_window));

  return;
}

// Function to save tool parameters, including calibration data, to a file on disk
int tool_save(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*btn_defaults;
  GtkWidget	*label0,*label1,*label2,*label3;
  
  slot=GPOINTER_TO_INT(user_data);
  current_tool=slot;
  
  //printf("\n\nENTERED SAVE FUNCTION\n\n");

  // build our window
  win_tcal = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
  gtk_window_set_modal(GTK_WINDOW(win_tcal),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_tcal),250,180);
  gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
  sprintf(scratch,"Save Tool Information");
  gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
  
  // create a grid to hold information
  grd_tcal = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
  gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);
  
  sprintf(scratch,"    ");
  label0 = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label0, 0, 0, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label0),0.0);

  if(strlen(Tool[slot].mmry.dev)<8)
    {
    sprintf(scratch,"No tool memory found!");
    label1 = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), label1, 0, 4, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(label1),1.0);
    sprintf(scratch,"Assign Serial Number: ");
    label2 = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), label2, 0, 5, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(label2),1.0);
    tool_serial_numadj=gtk_adjustment_new(0, 0, 999, 1, 1, 5);
    tool_serial_numbtn=gtk_spin_button_new(tool_serial_numadj, 3, 0);
    gtk_grid_attach(GTK_GRID(grd_tcal),tool_serial_numbtn, 1, 5, 1, 1);
    g_signal_connect(tool_serial_numbtn,"value-changed",G_CALLBACK(grab_tool_serial_num),GINT_TO_POINTER(slot));
    }
  else 
    {
    sprintf(scratch,"         Tool Memory Found!           ");
    label1 = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), label1, 0, 4, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label1),1.0);
    sprintf(scratch,"  Using serial number for file name.  ");
    label2 = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_tcal), label2, 0, 5, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label2),1.0);
    }

  btn_defaults = gtk_button_new_with_label ("Use Defaults");
  g_signal_connect (btn_defaults, "clicked", G_CALLBACK (tool_set_defaults), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_defaults, 0, 6, 1, 1);

  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (tool_save_done), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 1, 6, 1, 1);

  gtk_widget_set_visible(win_tcal,TRUE);
  
  return(1);
}

// Callback to get response for file dialog
void cb_tool_file_response(GtkNativeDialog *native, int response)
{
  if(response==GTK_RESPONSE_ACCEPT)
    {
    GtkFileChooser *chooser = GTK_FILE_CHOOSER (native);
    strcpy(new_tool,g_file_get_parse_name(gtk_file_chooser_get_file(chooser)));
    }
  g_object_unref (native);
  return;
}

// Function to read tool parameters, including calibarion data, from a previously saved file
int tool_open(GtkWidget *btn_call, gpointer user_data)
{
  int		h,i,slot,status;
  GtkWidget 	*win_tool_open;
  GtkWidget	*grd_info;
  GtkWidget	*label;
  GtkWidget	*button;
  GtkFileFilter	*ALLfilter,*TOOLfilter;
  gint 		res;
  genericlist	*aptr,*tptr;
  
  if(job.state>=JOB_RUNNING)return(0);
  slot=GPOINTER_TO_INT(user_data);
  memset(new_tool,0,sizeof(new_tool));

  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  TOOLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(TOOLfilter,"*.tool");
  gtk_file_filter_set_name (TOOLfilter,"*.tool");
  ALLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(ALLfilter,"*.*");
  gtk_file_filter_set_name (ALLfilter,"*.*");

  GtkFileChooserNative *file_dialog;
  file_dialog = gtk_file_chooser_native_new("Open File", GTK_WINDOW(win_model), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), TOOLfilter);	
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), ALLfilter);	
  g_signal_connect (file_dialog, "response", G_CALLBACK (cb_tool_file_response), NULL);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (file_dialog));

  if(strlen(new_tool)<1)return(0);


  // strip path information off of tool file name to get tool name so it can be found in TOOLS.XML file
  // load those params first (also to know what materials it works with), then load custom params via tool_information_read
  h=0;
  memset(scratch,0,sizeof(scratch));
  for(i=0;i<strlen(new_tool);i++)
    {
    if(new_tool[i]==47)							// if "/" clear scratch string
      {
      memset(scratch,0,sizeof(scratch));
      h=0;
      continue;
      }
    if(new_tool[i]==45)							// if "-" save what's in front of it
      {
      strcpy(Tool[slot].mmry.md.name,scratch);
      memset(scratch,0,sizeof(scratch));
      h=0;
      continue;
      }
    if(new_tool[i]==46)							// if "." save what's in front of it
      {
      Tool[slot].mmry.md.sernum=atoi(scratch);
      break;
      }
    scratch[h]=new_tool[i];
    h++;
    }
  strcpy(Tool[slot].name,Tool[slot].mmry.md.name);
  
  // get tool ID of tool index list based on tool name
  tptr=tool_list;
  while(tptr!=NULL)
    {
    if(strstr(tptr->name,Tool[slot].name)!=NULL)break;
    tptr=tptr->next;
    }
  if(tptr==NULL)return(0);
  //Tool[slot].tool_ID=tptr->ID;
  gtk_combo_box_set_active(GTK_COMBO_BOX(combo_tool), Tool[slot].tool_ID);    
  
  status=tool_information_read(slot,new_tool);				// open specific tool file and read data
  if(status==TRUE)
    {
    // update tool parameter lables
    {
    sprintf(scratch,"Serial ID: Unknown");
    if(strlen(Tool[slot].mmry.dev)>7)sprintf(scratch,"Serial ID: %s",Tool[slot].mmry.dev);
    gtk_label_set_text(GTK_LABEL(lbl_tool_memID),scratch);

    //sprintf(scratch,"%s",Tool[slot].pwr48 ? "On":"Off");
    //gtk_label_set_text(GTK_LABEL(lbl_power),scratch);
  
    //sprintf(scratch,"%s",Tool[slot].pwr24 ? "On":"Off");
    //gtk_label_set_text(GTK_LABEL(lbl_heater),scratch);
  
    //if(Tool[slot].thrm.sensor==0)sprintf(scratch,"none");
    //if(Tool[slot].thrm.sensor==1)sprintf(scratch,"1wire");
    //if(Tool[slot].thrm.sensor==2)sprintf(scratch,"RTD");
    //if(Tool[slot].thrm.sensor==3)sprintf(scratch,"thermistor");
    //if(Tool[slot].thrm.sensor==4)sprintf(scratch,"thrmocouple");
    //gtk_label_set_text(GTK_LABEL(lbl_tsense),scratch);
  
    //sprintf(scratch,"%d",Tool[slot].step_max);
    //gtk_label_set_text(GTK_LABEL(lbl_stepper),scratch);
  
    //sprintf(scratch,"%s",Tool[slot].spibus ? "On":"Off");
    //gtk_label_set_text(GTK_LABEL(lbl_SPI),scratch);
  
    //sprintf(scratch,"%s",Tool[slot].wire1 ? "On":"Off");
    //gtk_label_set_text(GTK_LABEL(lbl_1wire),scratch);
  
    //sprintf(scratch,"none");
    //if(strlen(Tool[slot].mmry.dev)>1)sprintf(scratch,"%s",Tool[slot].mmry.dev);
    //gtk_label_set_text(GTK_LABEL(lbl_1wID),scratch);
    
    //sprintf(scratch,"%s",Tool[slot].limitsw ? "On":"Off");
    //gtk_label_set_text(GTK_LABEL(lbl_limitsw),scratch);
  
    //sprintf(scratch,"%4.1f g",Tool[slot].weight);
    //gtk_label_set_text(GTK_LABEL(lbl_weight),scratch);

    //sprintf(scratch,"%+8.3f",Tool[slot].mmry.md.tip_pos_X);
    //gtk_label_set_text(GTK_LABEL(lbl_tipx),scratch);
  
    //sprintf(scratch,"%+8.3f",Tool[slot].mmry.md.tip_pos_Y);
    //gtk_label_set_text(GTK_LABEL(lbl_tipy),scratch);
  
    //sprintf(scratch,"%+8.3f",(Tool[slot].mmry.md.tip_pos_Z+3.0));
    //gtk_label_set_text(GTK_LABEL(lbl_tipzup),scratch);
    
    //sprintf(scratch,"%+8.3f",Tool[slot].mmry.md.tip_pos_Z);
    //gtk_label_set_text(GTK_LABEL(lbl_tipzdn),scratch);
    }
  
    // update material parameters
    {
    aptr=Tool[slot].mats_list;					// load choices with index of found tools
    while(aptr!=NULL)
      {
      if(strstr(aptr->name,Tool[slot].mmry.md.matp)!=NULL)break;
      aptr=aptr->next;
      }
    if(aptr!=NULL){gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats),aptr->ID);} 
    else {gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats), 0);}    

    sprintf(scratch,"%6.3f",Tool[slot].matl.diam);
    gtk_label_set_text(GTK_LABEL(lbl_tipdiam),scratch);

    sprintf(scratch,"%6.3f",Tool[slot].matl.layer_height);
    gtk_label_set_text(GTK_LABEL(lbl_layerht),scratch);

    sprintf(scratch,"%6.1f",Tool[slot].thrm.setpC);
    gtk_label_set_text(GTK_LABEL(lbl_temper),scratch);
    }

    }

  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return(1);
}

// Function to save material parameters, including calibration data, to a file on disk
int material_save(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  GtkWidget 	*win_tcal;
  GtkWidget	*grd_tcal;
  GtkWidget	*label;
  
  if(job.state>=JOB_RUNNING)return(0);
  slot=GPOINTER_TO_INT(user_data);

  // build our window
  win_tcal = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_tcal), GTK_WINDOW(win_tool));
  gtk_window_set_default_size(GTK_WINDOW(win_tcal),200,180);
  gtk_window_set_resizable(GTK_WINDOW(win_tcal),FALSE);				
  sprintf(scratch,"Material %d",slot);
  gtk_window_set_title(GTK_WINDOW(win_tcal),scratch);			
  
  // create a grid to hold information
  grd_tcal = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_tcal),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_tcal),5);
  gtk_window_set_child(GTK_WINDOW(win_tcal),grd_tcal);
  
  sprintf(scratch,"Save Material Information");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 1, 2, 1);
  gtk_label_set_xalign (GTK_LABEL(label),0.5);
  
  sprintf(scratch,"Select Serial Number: ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_tcal), label, 0, 4, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  tool_serial_numadj=gtk_adjustment_new(0, 0, 100, 1, 1, 5);
  tool_serial_numbtn=gtk_spin_button_new(tool_serial_numadj, 3, 0);
  gtk_grid_attach(GTK_GRID(grd_tcal),tool_serial_numbtn, 1, 4, 1, 1);
  g_signal_connect(tool_serial_numbtn,"value-changed",G_CALLBACK(grab_tool_serial_num),GINT_TO_POINTER(slot));

  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (tool_save_done), win_tcal);
  gtk_grid_attach (GTK_GRID(grd_tcal), btn_done, 1, 6, 1, 1);

  gtk_widget_set_visible(win_tcal,TRUE);
  
  return(1);
}

// Function to read material parameters, including calibarion data, from a previously saved file
int material_open(GtkWidget *btn_call, gpointer user_data)
{
  int		h,i,slot;
  char		new_tool[255];
  GtkWidget 	*win_matl_open;
  GtkWidget	*grd_info;
  GtkWidget	*label;
  GtkWidget	*button;
  GtkWidget 	*dialog;
  GtkFileFilter	*ALLfilter,*MATLfilter;
  gint 		res;
  
  
  memset(new_tool,0,sizeof(new_tool));
  if(job.state>=JOB_RUNNING)return(0);
  slot=GPOINTER_TO_INT(user_data);
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  MATLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(MATLfilter,"*.matl");
  gtk_file_filter_set_name (MATLfilter,"*.matl");
  ALLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(ALLfilter,"*.*");
  gtk_file_filter_set_name (ALLfilter,"*.*");

  GtkFileChooserNative *file_dialog;
  file_dialog = gtk_file_chooser_native_new("Open File", GTK_WINDOW(win_model), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), MATLfilter);	
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), ALLfilter);	
  g_signal_connect (file_dialog, "response", G_CALLBACK (cb_tool_file_response), NULL);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (file_dialog));

  if(strlen(new_tool)<1)return(0);
  
  // strip path information off of tool file name to get tool name so it can be found in TOOLS.XML file
  // load those params first (also to know what materials it works with), then load custom params via tool_information_read
  h=0;
  memset(scratch,0,sizeof(scratch));
  for(i=0;i<strlen(new_tool);i++)
    {
    if(new_tool[i]==47)							// if "/" clear scratch string
      {
      memset(scratch,0,sizeof(scratch));
      h=0;
      continue;
      }
    if(new_tool[i]==45)							// if "-" save what's in front of it
      {
      strcpy(Tool[slot].mmry.md.name,scratch);
      memset(scratch,0,sizeof(scratch));
      h=0;
      continue;
      }
    if(new_tool[i]==46)							// if "." save what's in front of it
      {
      Tool[slot].mmry.md.sernum=atoi(scratch);
      break;
      }
    scratch[h]=new_tool[i];
    h++;
    }
  strcpy(Tool[slot].name,Tool[slot].mmry.md.name);
  //XMLRead_Tool(slot);							// extract parameters for that tool from the TOOLS.XML file
  //tool_information_read(slot,new_tool);

  // update tool parameter lables
  {
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo_tool), Tool[slot].tool_ID);    

    sprintf(scratch,"Serial ID: Unknown");
    if(strlen(Tool[slot].mmry.dev)>7)sprintf(scratch,"Serial ID: %s",Tool[slot].mmry.dev);
    gtk_label_set_text(GTK_LABEL(lbl_tool_memID),scratch);

    sprintf(scratch,"%s",Tool[slot].pwr48 ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_power),scratch);
  
    sprintf(scratch,"%s",Tool[slot].pwr24 ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_heater),scratch);
  
    if(Tool[slot].thrm.sensor==0)sprintf(scratch,"none");
    if(Tool[slot].thrm.sensor==1)sprintf(scratch,"1wire");
    if(Tool[slot].thrm.sensor==2)sprintf(scratch,"RTDH");
    if(Tool[slot].thrm.sensor==3)sprintf(scratch,"thermistor");
    if(Tool[slot].thrm.sensor==4)sprintf(scratch,"thrmocouple");
    gtk_label_set_text(GTK_LABEL(lbl_tsense),scratch);
  
    sprintf(scratch,"%d",Tool[slot].step_max);
    gtk_label_set_text(GTK_LABEL(lbl_stepper),scratch);
  
    sprintf(scratch,"%s",Tool[slot].spibus ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_SPI),scratch);
  
    sprintf(scratch,"%s",Tool[slot].wire1 ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_1wire),scratch);
  
    sprintf(scratch,"none");
    if(strlen(Tool[slot].mmry.dev)>1)sprintf(scratch,"%s",Tool[slot].mmry.dev);
    gtk_label_set_text(GTK_LABEL(lbl_1wID),scratch);
    
    sprintf(scratch,"%s",Tool[slot].limitsw ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_limitsw),scratch);
  
    sprintf(scratch,"%4.1f g",Tool[slot].weight);
    gtk_label_set_text(GTK_LABEL(lbl_weight),scratch);

    //sprintf(scratch,"%+8.3f",Tool[slot].tip_x_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].x_offset,Tool[slot].x_offset,Tool[slot].tip_x_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipx),scratch);
  
    //sprintf(scratch,"%+8.3f",Tool[slot].tip_y_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].y_offset,Tool[slot].y_offset,Tool[slot].tip_y_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipy),scratch);
    
    //sprintf(scratch,"%+8.3f",Tool[slot].tip_z_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].z_offset,Tool[slot].tip_dn_pos,Tool[slot].tip_z_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipzdn),scratch);
    
  }

  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
  
  return(1);
}

// Function to set line type from combo box dropdown
int lts_type(GtkWidget *cmb_box, gpointer user_data)
{
  int		i,slot,ltyp;
  
  slot=GPOINTER_TO_INT(user_data);
  ltyp=gtk_combo_box_get_active (GTK_COMBO_BOX(cmb_box));
  
  
  return(1);
}

// Function to get info and adjust settings of given tool
int on_tool_settings(GtkWidget *btn_call, gpointer user_data)
{
  int		h,j,slot;
  float 	fval;
  double 	amt_done=0.0;
  GtkWidget 	*win_local_info;
  GtkWidget	*grd_info;
  GtkWidget	*label,*xtip_lbl,*ytip_lbl,*ztip_lbl;
  GtkWidget	*button;
  GtkWidget	*ztip_test_btn,*xy_test_btn;
  
  edge		*eptr;
  facet 	*fptr,*fnxt;
  model 	*mptr;
  
  slot=GPOINTER_TO_INT(user_data);						// index into which tool slot we are dealing with
  if(Tool[slot].state==TL_EMPTY)return(0);
  
  win_local_info = gtk_window_new();							
  gtk_window_set_transient_for (GTK_WINDOW(win_local_info), GTK_WINDOW(win_tool));	// make subordinate to win_tool
  gtk_window_set_modal(GTK_WINDOW(win_local_info),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_local_info),LCD_WIDTH-50,LCD_HEIGHT-40);	// set to size to be contained by LCD 
  gtk_window_set_resizable(GTK_WINDOW(win_local_info),FALSE);				// since running on fixed LCD, do NOT allow resizing
  sprintf(scratch,"Tool %d - Settings",slot);
  gtk_window_set_title(GTK_WINDOW(win_local_info),scratch);			
  
  // create a grid to hold label information
  grd_info = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_info),12);
  gtk_grid_set_column_spacing (GTK_GRID(grd_info),5);
  gtk_window_set_child(GTK_WINDOW(win_local_info),grd_info);
  gtk_window_set_transient_for(GTK_WINDOW(win_local_info),GTK_WINDOW(win_tool));

  // display tool properties -------------------------------------------

  sprintf(scratch,"TOOL:");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 0, 0, 1, 1);	
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  sprintf(scratch,"%s with ID %d",Tool[slot].name,Tool[slot].tool_ID);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 1, 0, 2, 1);	
  gtk_label_set_xalign (GTK_LABEL(label),0.1);
  
  sprintf(scratch,"X tip offset:");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 0, 11, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  fval=0.0;
  if(slot==0 || slot==2)fval=Tool[slot].x_offset;
  if(slot==1 || slot==3)fval=0-Tool[slot].x_offset;
  sprintf(scratch,"%7.3f",fval);
  xtip_lbl = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), xtip_lbl, 1, 11, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(xtip_lbl),0.0);
  
  sprintf(scratch,"Y tip offset:");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 0, 12, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  fval=0.0;
  if(slot==0 || slot==2)fval=Tool[slot].y_offset;
  if(slot==1 || slot==3)fval=0-Tool[slot].y_offset;
  sprintf(scratch,"%7.3f",fval);
  ytip_lbl = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), ytip_lbl, 1, 12, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(ytip_lbl),0.0);
  
  sprintf(scratch,"Z tip offset:");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 0, 13, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(label),1.0);
  sprintf(scratch,"%7.3f",Tool[slot].tip_dn_pos);
  ztip_lbl = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), ztip_lbl, 1, 13, 1, 1);
  gtk_label_set_xalign (GTK_LABEL(ztip_lbl),0.0);

  ztip_test_btn=gtk_button_new_with_label("Adj\nTip\nXYZ");
  gtk_grid_attach (GTK_GRID (grd_info), ztip_test_btn, 2, 11, 1, 3);
  g_signal_connect (ztip_test_btn, "clicked", G_CALLBACK (xyztip_manualcal_callback), GINT_TO_POINTER(slot));
  

  // display OK button -------------------------------------------------
  button = gtk_button_new_with_label ("Ok");
  gtk_grid_attach (GTK_GRID (grd_info), button, 0, 15, 1, 1);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (on_info_okay), win_local_info);
  //gtk_label_set_xalign (GTK_LABEL(label),0.0);
  
 
  // put a blank space in column 2 to force better spacing
  sprintf(scratch,"            ");
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 2, 0, 1, 1);		
  gtk_label_set_xalign (GTK_LABEL(label),0.0);



/*
  // ERROR CHECKING CODE ---------------------------------------
  
  // show the progress bar
  //gtk_widget_show (info_progress);
  //sprintf(scratch,"Analyzing ...");
  //gtk_label_set_text(GTK_LABEL(info_percent),scratch);
  //gtk_widget_queue_draw(info_percent);

  // run a quick free edge check to determine model quality
  h=0;
  eptr=mptr->edge_first[MODEL];
  while(eptr!=NULL)
    {
    if(eptr->Af==NULL || eptr->Bf==NULL)h++;
    eptr=eptr->next;
    }
  sprintf(scratch,"Free edges....: %d",h);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 4, 4, 1, 1);		
  gtk_label_set_xalign (GTK_LABEL(label),0.0);

  // loop through facet list to ensure it is unique
  h=0;j=0;

  fptr=mptr->facet_first[MODEL];
  while(fptr!=NULL)
    {
    // update status to user
    j++;
    amt_done = (double)(j)/(double)(mptr->facet_qty[MODEL]);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
    while (g_main_context_iteration(NULL, FALSE));

    fnxt=fptr->next;
    while(fnxt!=NULL)
      {
      if(facet_compare(fptr,fnxt))h++;
      fnxt=fnxt->next;
      }
    fptr=fptr->next;
    }

  sprintf(scratch,"Double facets.: %d",h);
  label = gtk_label_new (scratch);
  gtk_grid_attach (GTK_GRID (grd_info), label, 4, 5, 1, 1);		
  gtk_label_set_xalign (GTK_LABEL(label),0.0);

  // hide progress bar
  //gtk_widget_set_visible(info_progress,FALSE);

*/

  gtk_window_set_transient_for(GTK_WINDOW(win_local_info),GTK_WINDOW(win_tool));
  gtk_widget_set_visible(win_local_info,TRUE);
    
  return(1);

}



// Destroy callback for centering tools
static void xoff_done_callback (GtkWidget *btn, gpointer dead_window)
{

  gtk_window_close(GTK_WINDOW(dead_window));					// get rid of the window

  tinyGSnd("{\"posz\":NULL}\n");					// get current z position
  while(tinyGRcv(1)>=0);					// ensure it is processed
  sprintf(gcode_cmd,"G0 Z%f \n",PostG.z+5);				// move the carriage back home
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);					// ensure it is processed
  sprintf(gcode_cmd,"G0 X0.0 Y0.0 \n");			
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);					// ensure it is processed
  sprintf(gcode_cmd,"G0 Z0.0 \n");			
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);					// ensure it is processed
  
}

// Get X values for centering tools
void xoff_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot;
  polygon	*pptr;
  slice 	*sptr;
  model		*mptr;
  
  slot=GPOINTER_TO_INT(user_data);	
  mptr=job.model_first;
  sptr=slice_find(mptr,MODEL,slot,z_cut,Tool[slot].matl.layer_height);
  pptr=sptr->p_zero_ref;
  if(global_offset){xoffset=gtk_spin_button_get_value(button);}
  else{Tool[slot].x_offset=gtk_spin_button_get_value(button);}
  sprintf(gcode_cmd,"G0 X%f Y%f \n",(pptr->centx+Tool[slot].x_offset+xoffset),(pptr->centy+Tool[slot].y_offset+yoffset));
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);
  return;
}

// Get Y values for centering tools
void yoff_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  polygon	*pptr;
  slice 	*sptr;
  model		*mptr;
  
  slot=GPOINTER_TO_INT(user_data);	
  mptr=job.model_first;
  sptr=slice_find(mptr,MODEL,slot,z_cut,Tool[slot].matl.layer_height);
  pptr=sptr->p_zero_ref;
  if(global_offset){yoffset=gtk_spin_button_get_value(button);}
  else{Tool[slot].y_offset=gtk_spin_button_get_value(button);}
  sprintf(gcode_cmd,"G0 X%f Y%f \n",(pptr->centx+Tool[slot].x_offset+xoffset),(pptr->centy+Tool[slot].y_offset+yoffset));
  tinyGSnd(gcode_cmd);
  while(tinyGRcv(1)>=0);
  return;
}

// Global offset check box event
void goff_checkevent(GtkWidget *btn_call, gpointer user_data)
{
  int	slot;
  
  slot=GPOINTER_TO_INT(user_data);
  
  global_offset++;
  if(global_offset>1)global_offset=0;
  
  if(global_offset)
    {
    gtk_adjustment_set_value(xadj,xoffset);
    gtk_adjustment_set_value(yadj,yoffset);
    }
  else
    {
    gtk_adjustment_set_value(xadj,Tool[slot].x_offset);
    gtk_adjustment_set_value(yadj,Tool[slot].y_offset);
    }

  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    
  return;
}

// Function to center tool relative to reference polygon 
int on_tool_center(GtkWidget *btn_call, gpointer user_data)
{
  int		h,slot,on_ticks;
  vertex	*vptr;
  polygon	*pptr;
  slice		*sptr;
  model 	*mptr;
  GtkWidget 	*win_dialog,*dialog;
  GtkWidget	*win_tc_stat,*grd_tc_stat;
  GtkWidget	*lbl_da,*lbl_scratch,*goff_check;
  
  slot=GPOINTER_TO_INT(user_data);						// index into which tool slot we are dealing with

  PosWas.x=PosIs.x;PosWas.y=PosIs.y;PosWas.z=PosIs.z;

  // if a model loaded, move to the center of the zero reference polygon
  if(job.model_first!=NULL)
    {
    mptr=job.model_first;
    sptr=slice_find(mptr,MODEL,slot,z_cut,Tool[slot].matl.layer_height);
    pptr=sptr->p_zero_ref;
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    if(pptr==NULL)
      {
      sprintf(scratch,"\n No reference polygon selected! \n");
      aa_dialog_box(win_main,1,0,"Tool Center Warning",scratch);
      }
    if(pptr!=NULL)
      {
      // move tool to center of reference polygon
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X%f Y%f \n",(pptr->centx+Tool[slot].x_offset+xoffset),(pptr->centy+Tool[slot].y_offset+yoffset));
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      
      // display dialog to allow user to adjust tip in X/Y to center over reference polygon
      win_tc_stat = gtk_window_new();					// do NOT use type POPUP here... not for this purpose
      gtk_window_set_transient_for (GTK_WINDOW(win_tc_stat), GTK_WINDOW(win_tool));	// make subordinate to win_tool
      gtk_window_set_modal(GTK_WINDOW(win_tc_stat),TRUE);
      gtk_window_set_default_size(GTK_WINDOW(win_tc_stat),LCD_WIDTH/2.5,LCD_HEIGHT/3);	// set to size to be contained by LCD 
      gtk_window_set_resizable(GTK_WINDOW(win_tc_stat),FALSE);				// since running on fixed LCD, do NOT allow resizing
      sprintf(scratch,"Alignment by reference:");
      gtk_window_set_title(GTK_WINDOW(win_tc_stat),scratch);			
  
      grd_tc_stat = gtk_grid_new ();								// grid will hold cascading options information
      gtk_window_set_child(GTK_WINDOW(win_tc_stat),grd_tc_stat);
      gtk_grid_set_row_spacing (GTK_GRID(grd_tc_stat),15);
      gtk_grid_set_column_spacing (GTK_GRID(grd_tc_stat),15);
  
      lbl_da=gtk_label_new("Adjust X/Y until tool tip is centered over reference:");
      gtk_grid_attach (GTK_GRID(grd_tc_stat), lbl_da, 1, 1, 3, 1);
      
      lbl_scratch=gtk_label_new(" X Adjustment ");
      gtk_grid_attach (GTK_GRID(grd_tc_stat),lbl_scratch,1, 2, 1, 1);
      xadj=gtk_adjustment_new(2.500, -100.0, 100.0, 0.10, 0.1, 0.0);
      if(global_offset){gtk_adjustment_set_value(xadj,xoffset);}
      else{gtk_adjustment_set_value(xadj,Tool[slot].x_offset);}
      xbtn=gtk_spin_button_new(xadj, 0.10, 2);
      gtk_grid_attach(GTK_GRID(grd_tc_stat),xbtn, 2, 2, 1, 1);
      g_signal_connect(xbtn,"value-changed",G_CALLBACK(xoff_value),GINT_TO_POINTER(slot));
  
      lbl_scratch=gtk_label_new(" Y Adjustment ");
      gtk_grid_attach (GTK_GRID(grd_tc_stat),lbl_scratch,1, 3, 1, 1);
      yadj=gtk_adjustment_new(2.500, -100.0, 100.0, 0.10, 0.1, 0.0);
      if(global_offset){gtk_adjustment_set_value(yadj,yoffset);}
      else{gtk_adjustment_set_value(yadj,Tool[slot].y_offset);}
      ybtn=gtk_spin_button_new(yadj, 0.10, 2);
      gtk_grid_attach (GTK_GRID (grd_tc_stat), ybtn, 2, 3, 1, 1);
      g_signal_connect(ybtn,"value-changed",G_CALLBACK(yoff_value),GINT_TO_POINTER(slot));
  
      goff_check = gtk_check_button_new_with_label("Apply Globally");
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(goff_check),FALSE);
      if(global_offset)gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(goff_check),TRUE);
      g_signal_connect(goff_check,"toggled", G_CALLBACK(goff_checkevent), GINT_TO_POINTER(slot));
      gtk_grid_attach (GTK_GRID(grd_tc_stat), goff_check, 1, 4, 1, 1);
  
      btn_done = gtk_button_new_with_label ("Ok");						// add in "done" button
      g_signal_connect (btn_done, "clicked", G_CALLBACK (xoff_done_callback), win_tc_stat);
      gtk_grid_attach (GTK_GRID(grd_tc_stat), btn_done, 1, 5, 1, 1);
  
      gtk_widget_set_visible(win_tc_stat,TRUE);
      
      }
    }
    
  // if no model loaded, move to corners of build table
  else
    {
    if(PosIs.x==0 && PosIs.y==0 && PosIs.z==0)
      {
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X%f Y%f \n",xoffset+Tool[slot].x_offset+5,yoffset+Tool[slot].y_offset+BUILD_TABLE_LEN_Y/2);
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      while(!kbhit());
  
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X%f Y%f \n",xoffset+Tool[slot].x_offset+BUILD_TABLE_LEN_X-5,yoffset+Tool[slot].y_offset+BUILD_TABLE_LEN_Y/2);
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      while(!kbhit());

      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X%f Y%f \n",xoffset+Tool[slot].x_offset+BUILD_TABLE_LEN_X/2,yoffset+Tool[slot].y_offset+BUILD_TABLE_LEN_Y-5);
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      while(!kbhit());
  
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X%f Y%f \n",xoffset+Tool[slot].x_offset+BUILD_TABLE_LEN_X/2,yoffset+Tool[slot].y_offset+5);
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      while(!kbhit());
  
      sprintf(gcode_cmd,"G0 Z5.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 X0.0 Y0.0 \n");
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G0 Z0.0 \n");
      tinyGSnd(gcode_cmd);
      }
    else 
      {
      PosIs.x=0.0;
      PosIs.y=0.0;
      PosIs.z=0.0;
      }
    sprintf(gcode_cmd,"G1 Z%f F300 \n",PosIs.z);
    tinyGSnd(gcode_cmd);
    sprintf(gcode_cmd,"G0 X%f\n",PosIs.x);
    tinyGSnd(gcode_cmd);
    sprintf(gcode_cmd,"G0 Y%f\n",PosIs.y);
    tinyGSnd(gcode_cmd);
    }

  return(1);
}



// Function to allow user to define type for a given tool via pulldown menu
int tool_type(GtkWidget *cmb_box, gpointer user_data)
{
  int		i,pick,slot,col_width;
  genericlist	*tptr,*aptr;

  
  if(job.state>=JOB_RUNNING)return(0);					// no changes while job is running
  slot=GPOINTER_TO_INT(user_data);					// index into which tool slot we are dealing with
  
  // get count from user regarding which tool on linked list they want
  pick=gtk_combo_box_get_active (GTK_COMBO_BOX(cmb_box));
  
  // loop thru the combobox list to find the name of that tool that matches the pick
  i=0;
  tptr=tool_list;
  while(tptr!=NULL)
    {
    if(i==pick)break;
    tptr=tptr->next;
    i++;
    }
  if(tptr==NULL)return(0);
  
  // copy pick name over to tool name and reload/refresh the tool params from TOOLS.XML
  // note that this will wipe out any calib settings already in place for the tool as it is being redefined
  sprintf(Tool[slot].name,"%s",tptr->name);				// copy name found in index to tool
  XMLRead_Tool(slot,Tool[slot].name);					// extract parameters for that tool from the TOOLS.XML file
  Tool[slot].state=TL_LOADED;						// update state to loaded and defined
  dump_tool_params(slot);						// show what was read from tools.xls

  // clear the current contents of the mats combobox as a different tool will reference different materials
  GtkTreeModel	*mats_tree_model;
  mats_tree_model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo_mats));
  gtk_list_store_clear(GTK_LIST_STORE(mats_tree_model));
  
  // rebuild the mats combobox list
  build_material_index(slot);						// build index of materials available for this tool
  if(Tool[slot].mats_list==NULL)
    {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mats), NULL, "<empty>");
    }
  else 
    {
    col_width=15;
    sprintf(scratch,"UNKNOWN");
    while(strlen(scratch)<col_width)strcat(scratch," ");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mats), NULL, scratch);
    aptr=Tool[slot].mats_list;						// load choices with index of found materials
    while(aptr!=NULL)
      {
      sprintf(scratch,"%s",aptr->name);
      while(strlen(scratch)<col_width)strcat(scratch," ");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mats), NULL, scratch);
      aptr=aptr->next;
      }
    //if(Tool[slot].matl.ID>=0){gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats), (Tool[slot].matl.ID+1));}  // +1 accounts for "unknown" at top of list
    //else {gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats), 0);}    
    }
  
  // if user "un-defines" the tool
  if(strstr(Tool[slot].name,"UNKNOWN")!=NULL)
    {
    Tool[slot].state=TL_UNKNOWN;
    Tool[slot].matl.ID=0;
    Tool[slot].matl.mat_used=0.0;
    Tool[slot].matl.mat_volume=1.0;
    }

  // default material volume and level values
  // extruder tool
  if(Tool[slot].tool_ID==TC_EXTRUDER)
    {
    Tool[slot].matl.mat_used=1.0;
    Tool[slot].matl.mat_volume=20.0;
    }
  // fdm tools
  if(Tool[slot].tool_ID==TC_FDM)
    {
    Tool[slot].matl.mat_used=150000.0;					// full 1kg spool of PLA=330m
    Tool[slot].matl.mat_volume=330000.0;				
    }
  // heat lamp
  if(Tool[slot].tool_ID==TC_LAMP)
    {
    Tool[slot].powr.power_duty=0.10;
    }
 
  // update tool parameter lables
  {
    col_width=15;
    
    img_madd = gtk_image_new_from_file("Matl-Add-Generic-B.gif");
    if(Tool[slot].tool_ID==TC_EXTRUDER)img_madd = gtk_image_new_from_file("Matl-Extrude-Add-G.gif");
    if(Tool[slot].tool_ID==TC_FDM)img_madd = gtk_image_new_from_file("Matl-Filament-Add-G.gif");
    if(Tool[slot].tool_ID==TC_ROUTER)img_madd = gtk_image_new_from_file("Tip-Bit-Add-G.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_madd),50);
    gtk_button_set_child(GTK_BUTTON(btn_madd),img_madd);
    
    img_msub = gtk_image_new_from_file("Matl-Sub-Generic-B.gif");
    if(Tool[slot].tool_ID==TC_EXTRUDER)img_msub = gtk_image_new_from_file("Matl-Extrude-Remove-G.gif");
    if(Tool[slot].tool_ID==TC_FDM)img_msub = gtk_image_new_from_file("Matl-Filament-Remove-G.gif");
    if(Tool[slot].tool_ID==TC_ROUTER)img_msub = gtk_image_new_from_file("Tip-Bit-Remove-G.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_msub),50);
    gtk_button_set_child(GTK_BUTTON(btn_msub),img_msub);
    
    sprintf(scratch,"Status:  Material not defined");
    gtk_label_set_text(GTK_LABEL(lbl_tool_stat),scratch);
    
    sprintf(scratch,"Serial ID: Unknown");
    /*
    if(Tool[slot].wire1==TRUE)					// if this tool supports 1wire...
      {
      i=0;
      while(strlen(Tool[slot].mmry.dev)<8)
        {
	delay(500);
	Open1Wire(slot);
	i++;
	if(i>9)break;
	}
      if(strlen(Tool[slot].mmry.dev)>7)sprintf(scratch,"Serial ID: %s",Tool[slot].mmry.dev);
      }
    */
    gtk_label_set_text(GTK_LABEL(lbl_tool_memID),scratch);
    
    sprintf(scratch,"Description:  %s ",Tool[slot].desc);
    gtk_label_set_text(GTK_LABEL(lbl_tool_desc),scratch);
  
    /*
    sprintf(scratch,"%s",Tool[slot].pwr48 ? "On":"Off");
    while(strlen(scratch)<col_width)strcat(scratch," ");
    gtk_label_set_text(GTK_LABEL(lbl_power),scratch);
  
    sprintf(scratch,"%s",Tool[slot].pwr24 ? "On":"Off");
    while(strlen(scratch)<col_width)strcat(scratch," ");
    gtk_label_set_text(GTK_LABEL(lbl_heater),scratch);
  
    if(Tool[slot].thrm.sensor==0)sprintf(scratch,"none");
    if(Tool[slot].thrm.sensor==1)sprintf(scratch,"1wire");
    if(Tool[slot].thrm.sensor==2)sprintf(scratch,"RTDH");
    if(Tool[slot].thrm.sensor==3)sprintf(scratch,"thermistor");
    if(Tool[slot].thrm.sensor==4)sprintf(scratch,"thrmocouple");
    gtk_label_set_text(GTK_LABEL(lbl_tsense),scratch);
  
    sprintf(scratch,"%d",Tool[slot].step_max);
    while(strlen(scratch)<col_width)strcat(scratch," ");
    gtk_label_set_text(GTK_LABEL(lbl_stepper),scratch);
  
    sprintf(scratch,"%s",Tool[slot].spibus ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_SPI),scratch);
  
    sprintf(scratch,"%s",Tool[slot].wire1 ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_1wire),scratch);
  
    sprintf(scratch,"none");
    if(strlen(Tool[slot].mmry.dev)>1)sprintf(scratch,"%s",Tool[slot].mmry.dev);
    gtk_label_set_text(GTK_LABEL(lbl_1wID),scratch);
    
    sprintf(scratch,"%s",Tool[slot].limitsw ? "On":"Off");
    gtk_label_set_text(GTK_LABEL(lbl_limitsw),scratch);
  
    sprintf(scratch,"%4.1f g",Tool[slot].weight);
    gtk_label_set_text(GTK_LABEL(lbl_weight),scratch);
    */

    //sprintf(scratch,"%+8.3f",Tool[slot].tip_x_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].x_offset,Tool[slot].x_offset,Tool[slot].tip_x_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipx),scratch);
  
    //sprintf(scratch,"%+8.3f",Tool[slot].tip_y_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].y_offset,Tool[slot].y_offset,Tool[slot].tip_y_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipy),scratch);
  
    //sprintf(scratch,"%+8.3f",Tool[slot].tip_z_fab);
    sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].z_offset,Tool[slot].tip_dn_pos,Tool[slot].tip_z_fab);
    gtk_label_set_text(GTK_LABEL(lbl_tipzdn),scratch);
  }

  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
  
  return(1);
}

// Function to screen material selection
void on_combo_mats_button_press_event(GtkEventController *controller, double x, double y, guint button, guint state, gpointer user_data)
{
  int		slot;
  GtkWidget	*win_dialog;
  
  if(job.state>=JOB_RUNNING)return;
  slot=GPOINTER_TO_INT(user_data);	
  
  printf("\nComboMats:  Button press event!\n");
  
  // post msg if user trying to select material before tool
  if(button==GDK_BUTTON_PRIMARY && Tool[slot].tool_ID<=UNDEFINED)
    {
    sprintf(scratch," \nSelect a tool first, then a \n list of available materials for \n that tool will display. ");
    aa_dialog_box(win_main,1,0,"Tool Center Warning",scratch);
    }

  return;
}

// Function to allow user to define type for a given materail via pulldown menu
int mats_type(GtkWidget *cmb_box, gpointer user_data)
{
  int		i,slot;
  genericlist	*matptr;
  
  if(job.state>=JOB_RUNNING)return(0);
  slot=GPOINTER_TO_INT(user_data);	

  // get tool ID from user selection
  // Note that this changes depending on the number of materials available for the tool on this machine.
  // The matl ID is simply the sequence number it which it was found and exists on the mats list.
  Tool[slot].matl.ID = gtk_combo_box_get_active (GTK_COMBO_BOX(cmb_box));
  
  // if user selects "unknown" basically unloading the current material
  if(Tool[slot].matl.ID==0)
    {
    memset(Tool[slot].matl.name,0,sizeof(Tool[slot].matl.name));
    Tool[slot].matl.state=0;						// update state to loaded and defined
    printf("TOOL MAT TYP:  material set to unknown.\n");
    return(1);
    }

  // find the pointer to the material with that ID
  i=1;									// the list starts with 1
  matptr=Tool[slot].mats_list;
  while(matptr!=NULL)
    {
    if(i==Tool[slot].matl.ID)break;
    matptr=matptr->next;
    i++;
    }
  if(matptr==NULL)return(0);						// something went wrong...
  
  strcpy(Tool[slot].matl.name,matptr->name);
  printf("TOOL MAT TYP:  material set to: \n");
  printf("  matptr=%X  ID=%d  Name=%s \n",matptr,matptr->ID,matptr->name);    
  printf("  i=%d  Tool[slot].matl.ID=%d Tool[slot].matl.name=%s \n",i,Tool[slot].matl.ID,Tool[slot].matl.name);
  
  // extract data for that tool from the matl XML file
  if(XMLRead_Matl(slot,matptr->name)==TRUE)				// if data read was good...
    {
    // if the tool was defined AND now a material that it will operate on is defined, then it is ready
    if(strlen(Tool[slot].matl.name)>0)					// ... make sure valid name was found
      {
      Tool[slot].state=TL_READY;					// ... flag as good to go
      strcpy(Tool[slot].matl.name,matptr->name);			// ... update mat'l name
      Tool[slot].matl.state=1;						// ... flag as good to go
      }
    
      // update material parameters for this dialog and external dialogs
      {
      sprintf(scratch,"Status:  Material defined");
      gtk_label_set_text(GTK_LABEL(lbl_tool_stat),scratch);
  
      sprintf(scratch,"%s",Tool[slot].matl.name);
      if(strlen(scratch)>40)scratch[40]=0;
      gtk_label_set_text(GTK_LABEL(matl_desc[slot]),scratch);		// for tool view drawing area

      sprintf(scratch,"%s",Tool[slot].matl.description);
      if(strlen(scratch)>40)scratch[40]=0;
      gtk_label_set_text(GTK_LABEL(lbl_mat_description),scratch);	// for tool dialog 

      sprintf(scratch,"%s",Tool[slot].matl.brand);
      if(strlen(scratch)>40)scratch[40]=0;
      gtk_label_set_text(GTK_LABEL(lbl_mat_brand),scratch);

      sprintf(scratch,"%6.3f",Tool[slot].matl.diam);
      gtk_label_set_text(GTK_LABEL(lbl_tipdiam),scratch);
  
      sprintf(scratch,"%6.3f",Tool[slot].matl.layer_height);
      gtk_label_set_text(GTK_LABEL(lbl_layerht),scratch);
  
      sprintf(scratch,"%6.1f",Tool[slot].thrm.setpC);
      gtk_label_set_text(GTK_LABEL(lbl_temper),scratch);
      }
    }
    
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  return(1);
}



// Destroy callback for loading router bits
void zoff_done_callback (GtkWidget *btn, gpointer dead_window)
{

  tool_power_ramp(current_tool,TOOL_48V_PWM,OFF);				// turn off the tool
  gtk_window_close(GTK_WINDOW(dead_window));					// get rid of the window
  goto_machine_home();
  
}

// Test callback for loading router bits
// this draws a narrow box so the user can assess z tip position of the router.
void zoff_test_callback (GtkWidget *btn, gpointer user_data)
{
  float 	avg_z0,avg_z1;
  float 	x1,y1,ztippos;
  int 		slot;
  vertex 	*v0,*v1,*v2,*v3;
  
  slot=GPOINTER_TO_INT(user_data);

  // define coords of moves
  bit_load_y_inc+=0.762;						// in mm. = 0.015" trace width
  bit_load_y-=bit_load_y_inc;						// decrement from current position
  ztippos=Tool[slot].matl.layer_height+cnc_z_height;
  if(active_model!=NULL)ztippos=ZMax;

  // define moves
  v0=vertex_make(); v1=vertex_make();
  v0->x=bit_load_x;    v0->y=bit_load_y; v0->z=ztippos; v0->attr=MDL_BORDER;
  v1->x=bit_load_x+20; v1->y=bit_load_y; v1->z=ztippos; v1->attr=MDL_BORDER;

  // define moves
  v2=vertex_make(); v3=vertex_make();
  bit_load_y-=bit_load_y_inc/2;
  v2->x=bit_load_x+20; v2->y=bit_load_y; v2->z=ztippos; v2->attr=MDL_BORDER;
  v3->x=bit_load_x;    v3->y=bit_load_y; v3->z=ztippos; v3->attr=MDL_BORDER;
  
  // execute moves
  print_vertex(v0,slot,2,NULL);						// pen up to start
  print_vertex(v1,slot,1,NULL);						// cut 1st leg
  print_vertex(v2,slot,1,NULL);						// cut 2nd leg
  print_vertex(v3,slot,1,NULL);						// cut 3rd leg
  print_vertex(v0,slot,1,NULL);						// cut 4th leg
  tinyGSnd(gcode_burst);						// send commands to motion controller
  memset(gcode_burst,0,sizeof(gcode_burst));	
  motion_complete();
  free(v0); vertex_mem--; v0=NULL;
  free(v1); vertex_mem--; v1=NULL;
  free(v2); vertex_mem--; v2=NULL;
  free(v3); vertex_mem--; v3=NULL;
  
  return;
}

// Up btn callback for loading router bits
void zoff_up_callback(GtkWidget *btn_call, gpointer user_data)
{
  int		slot;
  float 	avg_z,ztippos;
  vertex 	*vptr;
  
  slot=GPOINTER_TO_INT(user_data);
  tinyGSnd("{\"pos\":NULL}\n");
  while(tinyGRcv(1)>=0);
  
  // define new z position
  avg_z=z_table_offset(bit_load_x,bit_load_y);
  ztippos=Tool[slot].matl.layer_height;
  if(active_model!=NULL)ztippos=ZMax;
  zoffset+=0.01;							// adjust tip up - applied in print_vertex function
  
  // execute move
  vptr=vertex_make();
  vptr->x=bit_load_x; 
  vptr->y=bit_load_y; 
  vptr->z=ztippos+cnc_z_height;
  vptr->attr=MDL_BORDER;
  print_vertex(vptr,slot,2,NULL);
  tinyGSnd(gcode_burst);			
  memset(gcode_burst,0,sizeof(gcode_burst));	
  motion_complete();
  free(vptr); vertex_mem--; vptr=NULL;

  // update dialog label
  sprintf(scratch,"  Current Height: %7.3f  ",zoffset);
  gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  return;
}

// Down btn callback for loading router bits
void zoff_dn_callback(GtkWidget *btn_call, gpointer user_data)
{
  int		slot;
  float 	avg_z,ztippos;
  vertex	*vptr;
  
  slot=GPOINTER_TO_INT(user_data);
  tinyGSnd("{\"pos\":NULL}\n");
  while(tinyGRcv(1)>=0);
  
  // define new z position
  avg_z=z_table_offset(bit_load_x,bit_load_y);
  ztippos=Tool[slot].matl.layer_height;
  if(active_model!=NULL)ztippos=ZMax;
  zoffset-=0.01;							// adjust tip down - applied in print_vertex function
  
  // execute move
  vptr=vertex_make();
  vptr->x=bit_load_x; 
  vptr->y=bit_load_y; 
  vptr->z=ztippos+cnc_z_height;
  vptr->attr=MDL_BORDER;
  print_vertex(vptr,slot,2,NULL);
  tinyGSnd(gcode_burst);			
  memset(gcode_burst,0,sizeof(gcode_burst));	
  motion_complete();
  free(vptr); vertex_mem--; vptr=NULL;

  // update dialog label
  sprintf(scratch,"  Current Height: %7.3f  ",zoffset);
  gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  return;
}

// Change tip diam callback for loading router bits
// this will override the tip_diam and line_width settings from the materials.xml file
void tip_diam_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot;
  linetype	*lptr;
  
  slot=GPOINTER_TO_INT(user_data);	
  Tool[slot].tip_diam=gtk_spin_button_get_value(button);
  
  // a change in tip diam on a cutting tool means line widths need to be overridden as well
  if(Tool[slot].matl.ID>0)						// if a material is defined...
    {
    lptr=linetype_find(slot,MDL_BORDER);
    if(lptr!=NULL)
      {
      lptr->line_width=Tool[slot].tip_diam;
      lptr->line_pitch=Tool[slot].tip_diam-0.1;
      }
    lptr=linetype_find(slot,MDL_OFFSET);
    if(lptr!=NULL)
      {
      lptr->line_width=Tool[slot].tip_diam;
      lptr->line_pitch=Tool[slot].tip_diam-0.1;
      }
    lptr=linetype_find(slot,MDL_FILL);
    if(lptr!=NULL)
      {
      lptr->line_width=Tool[slot].tip_diam;
      lptr->line_pitch=Tool[slot].tip_diam-0.1;
      }
    }
  
  gtk_widget_queue_draw(GTK_WIDGET(button));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
  
  return;
}


// Function to clear add material window
void on_add_matl_exit(GtkWidget *btn, gpointer dead_window)
{
  tinyGSnd("{4pm:0}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
  delay(150);							
  while(cmdCount>0)tinyGRcv(1);

  gtk_window_close(GTK_WINDOW(dead_window));
  gtk_widget_queue_draw(GTK_WIDGET(win_main));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
  return;
}

// Function to stop material feed on user request
void stop_material_feed(GtkWidget *btn, gpointer user_data)
{
  material_feed=FALSE;
  return;
}

// Function to load material (or bit) into a tool
int tool_add_matl(GtkWidget *btn, gpointer user_data)
{
  char 		text_lbl[255];
  int		ptyp;
  int		h,i,slot,lowx,lowy;
  float 	old_x,old_y,old_z;
  float 	old_temp,fval;
  float 	minPz,old_zoffset;
  float 	x1,y1,ztippos;
  float 	flow_thru;
  time_t 	feed_start_time,feed_current_time;
  vertex 	*vptr;
  linetype	*lptr;
  model 	*old_active;
  GtkWidget	*grd_stat,*lbl_da,*lbl_scratch;
  GtkWidget	*btn_z_up,*btn_z_dn,*btn_test;
  GtkAdjustment	*tip_diam_adj;
  GtkWidget	*tip_diam_btn;
  GtkWidget	*win_dialog,*win_tlam_stat,*grd_tlam_stat;
  GtkWidget	*grid_add_matl,*btn_done,*lbl_info,*lbl_time,*lbl_space;
  
  slot=GPOINTER_TO_INT(user_data);
  
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].tool_ID==UNDEFINED)return(1);
  oldState=cmdControl;
  cmdControl=0;

  // first ensure the tool is at operational temperature, it's okay if above
  if(Tool[slot].pwr24==TRUE && (Tool[slot].thrm.tempC+Tool[slot].thrm.tolrC)<Tool[slot].thrm.setpC)
    {
    sprintf(scratch," \nTool not yet at operational temperature.\nTry again once at temperature.");
    if(win_tool_flag[slot]==TRUE)
      {
      aa_dialog_box(win_tool,0,100,"Not Ready!",scratch);
      }
    else 
      {
      aa_dialog_box(win_main,0,100,"Not Ready!",scratch);
      }
    return(FALSE);
    }
  
  // get pointer to linetype that is requested
  ptyp=MDL_FILL;
  lptr=linetype_find(slot,ptyp);
  if(lptr==NULL)lptr=Tool[slot].matl.lt;				// if none found, use perimeter
  
  // verify tool material flowrate is reasonible
  //printf("Tool %d flowrate = %f \n",slot,Tool[slot].matl.lt->flowrate);
  if(Tool[slot].type==ADDITIVE && lptr->flowrate<0.1)lptr->flowrate=0.1;

  // build a specialized dialog window (default type won't work here) for tools that need it
  if(Tool[slot].tool_ID==TC_FDM || Tool[slot].tool_ID==TC_ROUTER)
    {
    win_dialog = gtk_window_new();
    if(win_tool_flag[slot]==TRUE)
      {
      gtk_window_set_transient_for (GTK_WINDOW(win_dialog), GTK_WINDOW(win_tool));	
      }
    else 
      {
      gtk_window_set_transient_for (GTK_WINDOW(win_dialog), GTK_WINDOW(win_main));	
      }
    gtk_window_set_modal(GTK_WINDOW(win_dialog),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_dialog),350,210);
    gtk_window_set_resizable(GTK_WINDOW(win_dialog),FALSE);			
    sprintf(scratch," Add Material ");
    gtk_window_set_title(GTK_WINDOW(win_dialog),scratch);			
    
    g_signal_connect (win_dialog, "destroy", G_CALLBACK (on_add_matl_exit), win_dialog);
  
    grid_add_matl=gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(win_dialog),grid_add_matl);
    gtk_grid_set_row_spacing (GTK_GRID(grid_add_matl),10);
    gtk_grid_set_column_spacing (GTK_GRID(grid_add_matl),10);
  
    lbl_info= gtk_label_new ("Add Material");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_info, 0, 0, 3, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_info),0.5);
    sprintf(scratch," ");
    lbl_time= gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_time, 1, 1, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_time),0.0);
    lbl_space= gtk_label_new("      ");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_space, 0, 2, 1, 1);
  
    btn_done = gtk_button_new_with_label("OK");
    g_signal_connect (btn_done, "clicked", G_CALLBACK (stop_material_feed), NULL);
    gtk_grid_attach (GTK_GRID(grid_add_matl), btn_done, 1, 3, 1, 1);
    lbl_space= gtk_label_new("      ");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_space, 0, 4, 1, 1);
    
    gtk_widget_set_visible(win_dialog,TRUE);
    }

  // Extruder tools
  if(Tool[slot].tool_ID==TC_EXTRUDER)
    {
    tinyGSnd("{4pm:3}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
    delay(150);							
    while(cmdCount>0)tinyGRcv(1);
    sprintf(scratch," \nPushing material.\n");
    aa_dialog_box(win_tool,0,25,"Extruder Push",scratch);
    PostG.a=30.0;
    make_toolchange(slot,1);
    Tool[slot].matl.mat_pos+=0.25;
    sprintf(gcode_cmd,"G1 A%f F50.0 \n",Tool[slot].matl.mat_pos);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    motion_complete();
    }

  // FDM tools
  if(Tool[slot].tool_ID==TC_FDM)
    {
    tinyGSnd("{4pm:3}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
    delay(150);							
    while(cmdCount>0)tinyGRcv(1);

    make_toolchange(slot,1);						// set to tool stepper drive
    
    memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
    sprintf(gcode_cmd,"G28.3 A0.0 \n");					// reset home of A stepper drive
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);

    memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
    flow_thru=lptr->flowrate*10;
    sprintf(gcode_cmd,"G1 A10.0 F%f \n\n",flow_thru);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);

    //sprintf(text_lbl," \n  Install filament tip into tool, until you feel it pull in. \n  Let run it ejects from tip.  Click OK to finish. ");
    sprintf(text_lbl," \n  Linetype: %s \n  Flowrate: %6.2f ",lptr->name,lptr->flowrate);
    gtk_label_set_text(GTK_LABEL(lbl_info),text_lbl);
    gtk_widget_queue_draw(lbl_info);
    
    material_feed=TRUE;
    feed_start_time=time(NULL);
    while(material_feed==TRUE)
      {
      delay(250);
      feed_current_time=time(NULL);					// ... get current time
      sprintf(text_lbl,"    Extruding 00:%02d sec ",(int)(max_feed_time-(feed_current_time-feed_start_time)));
      gtk_label_set_text(GTK_LABEL(lbl_time),text_lbl);
      gtk_widget_queue_draw(lbl_time);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      if((int)(feed_current_time-feed_start_time)>(max_feed_time-1))break;
      }
  
    tinyGSnd("!\n");							// abort move after ok clicked
    tinyGSnd("%\n");							// clear the tinyg buffer of any residual moves
    sprintf(gcode_cmd,"G28.3 A0.0 \n");					// reset home to just loaded position
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    motion_complete();
    
    cmdControl=oldState;
    on_add_matl_exit(btn_done,win_dialog);
    return(TRUE);
    }

  // if a router, this serves as the bit add/change function
  if(Tool[slot].tool_ID==TC_ROUTER)
    {
    printf("Router Bit Change: entry \n");

    // init
    tinyGSnd("{\"pos\":NULL}\n");					// refresh current position
    while(tinyGRcv(1)>=0);
    old_x=PostG.x;old_y=PostG.y;old_z=PostG.z;				// save position to return to when done
    old_active=active_model;
    active_model=job.model_first;
    material_feed=TRUE;
    bit_load_x_inc=0;
    bit_load_y_inc=0;
    make_toolchange(slot,0);
    
    // define location to move tool to for bit change
    vptr=vertex_make();
    #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
      // move carriage to front center of these units.
      vptr->x=(BUILD_TABLE_MAX_X/2.0);					// middle of X
      vptr->y=(BUILD_TABLE_MAX_Y/2.0);					// middle of y
      vptr->z=(BUILD_TABLE_MIN_Z+25.0);					// a little above min z
      vptr->k=8000;							// a little slower than full pen up
    #endif
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
      // move carriage to front center of these units.
      vptr->x=BUILD_TABLE_MAX_X-25;					// end of x less a little
      vptr->y=(BUILD_TABLE_MAX_Y/2.0);					// middle of y
      vptr->z=(BUILD_TABLE_MAX_Z-BUILD_TABLE_MIN_Z)/2.0;		// middle of z
      vptr->k=8000;							// a little slower than full pen up
    #endif
    comefrom_machine_home(slot,vptr);					// reposition the carriage to bit change position

    // wait for user to loosely insert new bit
    sprintf(scratch,"\n  Install router %6.3f diam bit deep into ",Tool[slot].tip_diam);
    strcat(scratch, "\n  the collet and only finger tighten it.  ");
    strcat(scratch, "\n\n      Click OK when ready. \n");
    gtk_label_set_text(GTK_LABEL(lbl_info),scratch);
    gtk_widget_queue_draw(lbl_info);
    while(material_feed==TRUE)
      {
      delay(100);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      }
    
    // home tool slot if not done already
    if(Tool[slot].tip_cr_pos!=Tool[slot].tip_up_pos)tool_tip_home(slot);
      
    // define bit tightening position (on model target material)
    bit_load_x=(BUILD_TABLE_MAX_X/2.0);
    bit_load_y=(BUILD_TABLE_MAX_Y/2.0);
    ztippos=Tool[slot].matl.layer_height;
    zoffset=0;		

    // move to bit tightening position
    vptr->x=bit_load_x; 
    vptr->y=bit_load_y; 
    vptr->z=ztippos+cnc_z_height;					// top of target even if no model loaded
    vptr->attr=MDL_BORDER;
    printf("moving to target top:  x=%f y=%f z=%f \n",vptr->x,vptr->y,vptr->z);
    print_vertex(vptr,slot,2,NULL);					// move to xy above target
    tinyGSnd(gcode_burst);						// send move command
    memset(gcode_burst,0,sizeof(gcode_burst));				// clear command string
    tool_tip_deposit(slot);						// drop z into position (alpha & beta units)
    tool_power_ramp(slot,TOOL_48V_PWM,OFF);				// ensure tool power is OFF

    // ask user to tighten bit in collet
    material_feed=TRUE;
    sprintf(scratch,"\n  Loosen collet and drop tip of bit ");
    strcat(scratch, "\n  onto target then wrench tighten it.");
    strcat(scratch, "\n\n     Click OK when done. \n");
    gtk_label_set_text(GTK_LABEL(lbl_info),scratch);
    gtk_widget_queue_draw(lbl_info);
    while(material_feed==TRUE)
      {
      delay(100);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      }

    // post warning that it will now spin up
    material_feed=TRUE;
    sprintf(scratch,"\n  The router will now spin up to speed. ");
    strcat(scratch, "\n  Clear hands and tools from the area.  ");
    strcat(scratch, "\n\n       Click OK to start. \n");
    gtk_label_set_text(GTK_LABEL(lbl_info),scratch);
    gtk_widget_queue_draw(lbl_info);
    while(material_feed==TRUE)
      {
      delay(100);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      }
    gtk_window_close(GTK_WINDOW(win_dialog));
    
    // post dialog that allows user to tweak the position of the tip
    win_tlam_stat = gtk_window_new();					
    gtk_window_set_transient_for (GTK_WINDOW(win_tlam_stat), GTK_WINDOW(win_tool));		// make subordinate to win_tool
    gtk_window_set_default_size(GTK_WINDOW(win_tlam_stat),LCD_WIDTH/2,LCD_HEIGHT/2);		// set to size to be contained by LCD 
    gtk_window_set_resizable(GTK_WINDOW(win_tlam_stat),FALSE);					// since running on fixed LCD, do NOT allow resizing
    sprintf(scratch,"Bit Details");
    gtk_window_set_title(GTK_WINDOW(win_tlam_stat),scratch);			
    g_signal_connect (win_tlam_stat, "destroy", G_CALLBACK (zoff_done_callback), win_tlam_stat);

    grd_tlam_stat = gtk_grid_new ();								// grid will hold cascading options information
    gtk_window_set_child(GTK_WINDOW(win_tlam_stat),grd_tlam_stat);
    gtk_grid_set_row_spacing (GTK_GRID(grd_tlam_stat),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_tlam_stat),15);

    lbl_da=gtk_label_new(" Adjust height until bit is in contact: ");
    gtk_grid_attach (GTK_GRID(grd_tlam_stat), lbl_da, 1, 1, 3, 1);
    
    sprintf(scratch," Current Height: %7.3f ",zoffset);
    lbl_info1=gtk_label_new(scratch);
    gtk_grid_attach (GTK_GRID(grd_tlam_stat), lbl_info1, 2, 2, 3, 1);
    
    btn_z_up = gtk_button_new_with_label ("+");				    		// add in "up" adjustment button
    g_signal_connect (btn_z_up, "clicked", G_CALLBACK (zoff_up_callback), GINT_TO_POINTER(slot));
    gtk_grid_attach (GTK_GRID(grd_tlam_stat), btn_z_up, 1, 3, 1, 1);
    lbl_scratch=gtk_label_new(" - to raise the bit away from the table");
    gtk_grid_attach (GTK_GRID(grd_tlam_stat),lbl_scratch,2,3,3,1);
    
    btn_z_dn = gtk_button_new_with_label ("-");						// add in "down" adjustment button
    g_signal_connect (btn_z_dn, "clicked", G_CALLBACK (zoff_dn_callback), GINT_TO_POINTER(slot));
    gtk_grid_attach (GTK_GRID (grd_tlam_stat), btn_z_dn, 1, 4, 1, 1);
    lbl_scratch=gtk_label_new(" - to lower the bit in toward the table");
    gtk_grid_attach (GTK_GRID(grd_tlam_stat),lbl_scratch,2,4,3,1);
    
    btn_test = gtk_button_new_with_label (" Test ");					// add in "test" button
    g_signal_connect (btn_test, "clicked", G_CALLBACK (zoff_test_callback), GINT_TO_POINTER(slot));
    gtk_grid_attach (GTK_GRID (grd_tlam_stat), btn_test, 1, 5, 1, 1);
    lbl_scratch=gtk_label_new(" - to cut a test line at current depth");
    gtk_grid_attach (GTK_GRID(grd_tlam_stat),lbl_scratch,2,5,3,1);
    
    // create hole drilling bit size option
    lbl_scratch=gtk_label_new (" Bit diam: ");
    gtk_grid_attach (GTK_GRID (grd_tlam_stat), lbl_scratch, 1, 7, 1, 1);
    fval=Tool[slot].tip_diam;
    tip_diam_adj=gtk_adjustment_new(fval, 0.0, 100.0, 0.10, 0.1, 0.0);
    tip_diam_btn=gtk_spin_button_new(tip_diam_adj, 1, 3);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(tip_diam_btn), 3);
    gtk_grid_attach(GTK_GRID(grd_tlam_stat),tip_diam_btn, 2, 7, 1, 1);
    g_signal_connect(tip_diam_btn,"value-changed",G_CALLBACK(tip_diam_value),GINT_TO_POINTER(slot));

    btn_done = gtk_button_new_with_label ("Done");					// add in "done" button
    g_signal_connect (btn_done, "clicked", G_CALLBACK (zoff_done_callback), win_tlam_stat);
    gtk_grid_attach (GTK_GRID(grd_tlam_stat), btn_done, 5, 7, 1, 1);

    gtk_widget_set_visible(win_tlam_stat,TRUE);

    // spin up router
    tool_power_ramp(slot,TOOL_48V_PWM,(int)(100*Tool[slot].powr.power_duty));
    free(vptr); vertex_mem--; vptr=NULL;
    }
    
  // heat lamp
  if(Tool[slot].tool_ID==TC_LAMP)
    {
    sprintf(scratch," \nPower is ON.\n");
    aa_dialog_box(win_tool,0,25,"Heat Lamp",scratch);
    tool_power_ramp(slot, TOOL_48V_PWM, (int)(100*Tool[slot].powr.power_duty));
    delay(500);
    tool_power_ramp(slot,TOOL_48V_PWM,OFF);
    }
    
  cmdControl=oldState;

  return(TRUE);
}

// Function to remove material to a tool
int tool_sub_matl(GtkWidget *btn, gpointer user_data)
{
  int		slot;
  float 	old_temp;
  time_t 	feed_start_time,feed_current_time;
  GtkWidget	*win_dialog,*win_tlam_stat,*grd_tlam_stat;
  GtkWidget	*grid_add_matl,*btn_done,*lbl_info,*lbl_time,*lbl_space;

  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].tool_ID==UNDEFINED)return(1);
  
  // first ensure the tool is at operational temperature
  if(Tool[slot].pwr24==TRUE && (Tool[slot].thrm.tempC+Tool[slot].thrm.tolrC)<Tool[slot].thrm.setpC)
    {
    sprintf(scratch," \nTool not yet at operational temperature.\nTry again once at temperature.");
    aa_dialog_box(win_main,0,100,"Not Ready!",scratch);
    return(FALSE);
    }
    
  // build a specialized dialog window (default type won't work here) for tools that need it
  if(Tool[slot].tool_ID==TC_FDM || Tool[slot].tool_ID==TC_ROUTER)
    {
    win_dialog = gtk_window_new();
    gtk_window_set_transient_for (GTK_WINDOW(win_dialog), GTK_WINDOW(win_main));	
    gtk_window_set_modal(GTK_WINDOW(win_dialog),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_dialog),350,200);
    gtk_window_set_resizable(GTK_WINDOW(win_dialog),FALSE);			
    sprintf(scratch," Subtract Material ");
    gtk_window_set_title(GTK_WINDOW(win_dialog),scratch);			
  
    g_signal_connect (win_dialog, "destroy", G_CALLBACK (on_add_matl_exit), win_dialog);

    grid_add_matl=gtk_grid_new();
    gtk_window_set_child(GTK_WINDOW(win_dialog),grid_add_matl);
    gtk_grid_set_row_spacing (GTK_GRID(grid_add_matl),10);
    gtk_grid_set_column_spacing (GTK_GRID(grid_add_matl),10);

    lbl_info= gtk_label_new ("Subtract Material");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_info, 0, 0, 3, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_info),0.5);
    sprintf(scratch,"00:%02d",max_feed_time);
    lbl_time= gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_time, 1, 1, 1, 1);
    gtk_label_set_xalign (GTK_LABEL(lbl_time),0.0);
    lbl_space= gtk_label_new("      ");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_space, 0, 2, 1, 1);

    btn_done = gtk_button_new_with_label("OK");
    g_signal_connect (btn_done, "clicked", G_CALLBACK (stop_material_feed), NULL);
    gtk_grid_attach (GTK_GRID(grid_add_matl), btn_done, 1, 3, 1, 1);
    lbl_space= gtk_label_new("      ");
    gtk_grid_attach (GTK_GRID(grid_add_matl), lbl_space, 0, 4, 1, 1);

    gtk_widget_set_visible(win_dialog,TRUE);
    }
    
  if(Tool[slot].tool_ID==TC_EXTRUDER)
    {
    tinyGSnd("{4pm:3}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
    delay(150);							
    while(cmdCount>0)tinyGRcv(1);
    sprintf(scratch," \nPulling material.\n");
    aa_dialog_box(win_tool,0,25,"Extruder Pull",scratch);
    PostG.a=30.0;
    make_toolchange(slot,1);
    Tool[slot].matl.mat_pos-=0.25;
    sprintf(gcode_cmd,"G1 A%f F50.0 \n",Tool[slot].matl.mat_pos);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    motion_complete();
    }
    
  if(Tool[slot].tool_ID==TC_FDM)
    {
    tinyGSnd("{4pm:3}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
    delay(150);							
    while(cmdCount>0)tinyGRcv(1);
    make_toolchange(slot,1);
    sprintf(gcode_cmd,"G28.3 A10.0 \n");	
    tinyGSnd(gcode_cmd);
    sprintf(gcode_cmd,"G1 A0.0 F%f \n",Tool[slot].matl.lt->flowrate*10);					// move head down into position
    tinyGSnd(gcode_cmd);

    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
    sprintf(scratch," \n  Gently pull filament from tool, until you feel it release.\n  Click OK to finish.");
    gtk_label_set_text(GTK_LABEL(lbl_info),scratch);
    gtk_widget_queue_draw(lbl_info);
    
    material_feed=TRUE;
    feed_current_time=time(NULL);					// ... get current time
    feed_start_time=time(NULL);
    while((int)(feed_current_time-feed_start_time)<max_feed_time && material_feed==TRUE)
      {
      delay(100);
      feed_current_time=time(NULL);					// ... get current time
      sprintf(scratch,"   Retracting 00:%02d sec ",(int)(max_feed_time-(feed_current_time-feed_start_time)));
      gtk_label_set_text(GTK_LABEL(lbl_time),scratch);
      gtk_widget_queue_draw(lbl_time);
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      }

    oldState=cmdControl;
    cmdControl=0;
    tinyGSnd("!\n");							// abort move after ok clicked
    tinyGSnd("%\n");							// clear the tinyg buffer of any residual moves
    cmdControl=oldState;
    sprintf(gcode_cmd,"G28.3 A0.0 \n");					// reset home position
    tinyGSnd(gcode_cmd);
    on_add_matl_exit(btn_done,win_dialog);
    return(TRUE);

    Tool[slot].matl.mat_used=33000.0;
    }

  // heat lamp
  if(Tool[slot].tool_ID==TC_LAMP)
    {
    tool_power_ramp(slot,TOOL_48V_PWM,OFF);
    }

  return(1);
}


// Function to reset a tool in the event of an error
int tool_reset(GtkWidget *btn, gpointer user_data)
{
  int 	slot,all_ok=TRUE;
  
  slot=GPOINTER_TO_INT(user_data);
  
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].tool_ID==UNDEFINED)return(1);
  oldState=cmdControl;
  cmdControl=0;
  
  // clear tool error settings
  Tool[slot].epwr48=FALSE;
  Tool[slot].epwr24=FALSE;
  Tool[slot].espibus=FALSE;
  Tool[slot].ewire1=FALSE;
  Tool[slot].elimitsw=FALSE;
  Tool[slot].epwmtool=FALSE;
  
  // clear build table error settings
  Tool[BLD_TBL1].epwr48=FALSE;
  Tool[BLD_TBL1].epwr24=FALSE;
  Tool[BLD_TBL1].espibus=FALSE;
  Tool[BLD_TBL1].ewire1=FALSE;
  Tool[BLD_TBL1].elimitsw=FALSE;

  Tool[BLD_TBL2].epwr48=FALSE;
  Tool[BLD_TBL2].epwr24=FALSE;
  Tool[BLD_TBL2].espibus=FALSE;
  Tool[BLD_TBL2].ewire1=FALSE;
  Tool[BLD_TBL2].elimitsw=FALSE;

  // clear chamber error settings
  Tool[CHAMBER].epwr48=FALSE;
  Tool[CHAMBER].epwr24=FALSE;
  Tool[CHAMBER].espibus=FALSE;
  Tool[CHAMBER].ewire1=FALSE;
  Tool[CHAMBER].elimitsw=FALSE;
  
  // check if type defined
  if(Tool[slot].type<=UNDEFINED || Tool[slot].type>=MAX_TOOL_TYPES)all_ok=FALSE;
  
  // check if memory is accessible
  
  
  // reset to operational
  if(all_ok==TRUE)
    {
    Tool[slot].state=TL_READY;
    job.state=JOB_PAUSED_BY_CMD;
    }
  else 
    {
    Tool[slot].state=TL_LOADED;
    }
  
  
  return(all_ok);
}

// Function to set temp for hot purge material out of a tool -  clears jams
int tool_hotpurge_matl(GtkWidget *btn, gpointer user_data)
{
  int		h,i,slot,lowx,lowy;
  float 	old_x,old_y,old_z;
  float 	old_temp,fval;
  float 	minPz,old_zoffset;
  float 	x1,y1;
  GtkWidget	*win_dialog;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(Tool[slot].tool_ID==UNDEFINED)return(1);
  if(Tool[slot].pwr24==FALSE || Tool[slot].thrm.sensor<1)
    {
    sprintf(scratch," \nThis tool is not configured for thermal \n control.  Check tool settings. \n");
    aa_dialog_box(win_main,1,0,"Tool Temperature Control",scratch);
    return(0);
    }
  
  if(Tool[slot].thrm.maxtC>=250.0)
    {
    //pthread_mutex_lock(&thermal_lock);      
    Tool[slot].thrm.setpC=250.0;
    //pthread_mutex_unlock(&thermal_lock);      
    }
    
  return(1);
}

void color_selected(GtkColorChooser *chooser, GdkRGBA *new_color, gpointer user_data)
{
  int		slot;
  
  slot=GPOINTER_TO_INT(user_data);
  gtk_color_chooser_get_rgba (chooser, new_color);
  mcolor[slot]=*new_color;
  
  printf(" MColor[%d]: red=%4.3f blue=%4.3f green=%4.3f alpha=%4.3f \n",slot,mcolor[slot].red,mcolor[slot].blue,mcolor[slot].green,mcolor[slot].alpha);
  
  gtk_color_chooser_set_rgba (chooser, &mcolor[slot]);
  
  return;
}

// Function to set material color
int matl_set_color(GtkWidget *btn, gpointer user_data)
{
  int		slot;
  
  slot=GPOINTER_TO_INT(user_data);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER(btn), &mcolor[slot]);
  Tool[slot].matl.color=get_color(&mcolor[slot]);
  
  return(1);
}

// Function to set material color - GTK 4.10
/*
int matl_set_color(GtkColorDialog *dialog, gpointer user_data)
{
  int		slot;
  
  slot=GPOINTER_TO_INT(user_data);
  GAsyncResult* res;
  gtk_color_dialog_choose_rgba_finish(dialog, res, NULL);
  mcolor[slot] = GTK_COLOR(res);
  
  return(1);
}
*/

// Function to close material amount input window
void grab_matl_lvl_done(GtkWidget *btn, gpointer dead_window)
{
  gtk_window_close(GTK_WINDOW(dead_window));					// get rid of the window

  return;
}

// Function to grab material max level
void grab_matl_max_value(GtkSpinButton *btn, gpointer user_data)
{
  int	slot;
  float max_matl;
  
  slot=GPOINTER_TO_INT(user_data);
  max_matl=gtk_spin_button_get_value(btn);
  
  // run some validity checks on what was selected and convert to tool's units of volume/length measurement
  if(Tool[slot].matl.mat_volume<1.0)Tool[slot].matl.mat_volume=1.0;
  if(Tool[slot].tool_ID==TC_EXTRUDER)					// extruder tools - volume measured in mL
    {
    if(max_matl>25.0)max_matl=25.0;					// 25mL syringes max (so far)
    Tool[slot].matl.mat_volume=max_matl;
    }
  if(Tool[slot].tool_ID==TC_FDM)					// FDM tools - volume measured in mm, but input in kg
    {
    if(max_matl>1.0)max_matl=1.0;					//  1.0kg max spool size (so far)
    Tool[slot].matl.mat_volume=330000*max_matl;				// 330m of filament per 1 kg spool
    }
  
  return;
}

// Function to grab material current level remaining
// note: value input by user via combobox is % remaining, value in tool struct is in that tool's units (FDM=mm, Extruder=mL, etc.)
//       matl.mat_volume is stored in that tool's unit of measure - see above
void grab_matl_lvl_value(GtkSpinButton *btn, gpointer user_data)
{
  int	slot;
  float used_matl;
  
  slot=GPOINTER_TO_INT(user_data);
  
  if(Tool[slot].type==ADDITIVE)
    {
    used_matl=1.0-(gtk_spin_button_get_value(btn)/100.0);		// convert from % remaining to % used
    if(used_matl<0)used_matl=0;
    if(used_matl>1)used_matl=1;
    Tool[slot].matl.mat_used=used_matl*Tool[slot].matl.mat_volume;	// convert % to tool's unit of measurement
    }
    
  if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==TRUE)
    {
    used_matl=gtk_spin_button_get_value(btn)/100.0;			
    if(used_matl<0)used_matl=0;
    if(used_matl>1)used_matl=1;
    Tool[slot].powr.power_duty=used_matl;
    }
    

  return;
}

// Function to set material amount if an additive type tool
// if not additive, but pwr control is enabled, this will instead set duty cycle
int matl_set_amount(GtkWidget *btn, gpointer user_data)
{
  int		slot;
  float 	used_matl,max_matl;
  GtkWidget	*label;
  GtkAdjustment	*adj_matl_lvl,*adj_matl_max;
  GtkWidget	*spn_matl_lvl,*spn_matl_max;
  
  slot=GPOINTER_TO_INT(user_data);
  
  // build our window
  win_data = gtk_window_new();	
  gtk_window_set_transient_for (GTK_WINDOW(win_data), GTK_WINDOW(win_tool));
  gtk_window_set_modal(GTK_WINDOW(win_data),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_data),400,280);
  gtk_window_set_resizable(GTK_WINDOW(win_data),FALSE);	
  sprintf(scratch,"Tool %d",(slot+1));			
  if(Tool[slot].type==ADDITIVE)sprintf(scratch,"Material Level for Tool %d",(slot+1));
  if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==TRUE)sprintf(scratch,"Power Duty Cycle for Tool %d",(slot+1));
  gtk_window_set_title(GTK_WINDOW(win_data),scratch);			
  
  // create a grid to hold information
  grd_data = gtk_grid_new ();				
  gtk_grid_set_row_spacing (GTK_GRID(grd_data),10);
  gtk_grid_set_column_spacing (GTK_GRID(grd_data),5);
  gtk_window_set_child(GTK_WINDOW(win_data),grd_data);
  
  // check if not an additive type tool and now power contorl...
  if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==FALSE)
    {
    sprintf(scratch,"This type of tool does not");
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 1, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label),0.5);
    sprintf(scratch,"require any settings here.");
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 2, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label),0.5);
    }
  
  // but if additive, get reservoir size and amount remaining
  if(Tool[slot].type==ADDITIVE)
    {
    if(Tool[slot].tool_ID==TC_EXTRUDER)
      {
      sprintf(scratch,"Enter the syringe size (mL)...:");
      max_matl=Tool[slot].matl.mat_volume;				// stays in mL of material
      }
    if(Tool[slot].tool_ID==TC_FDM)
      {
      sprintf(scratch,"Enter the spool size (kg).....:");
      max_matl=Tool[slot].matl.mat_volume/330000;			// convert mm back to kg of material
      }
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 1, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label),0.0);
    adj_matl_max=gtk_adjustment_new(max_matl, 0, 25, 0.1, 1, 5);
    spn_matl_max=gtk_spin_button_new(adj_matl_max, 5, 1);
    gtk_grid_attach(GTK_GRID(grd_data),spn_matl_max, 2, 1, 1, 1);
    g_signal_connect(spn_matl_max,"value-changed",G_CALLBACK(grab_matl_max_value),GINT_TO_POINTER(slot));
    
    used_matl=(100*(Tool[slot].matl.mat_volume-Tool[slot].matl.mat_used)/Tool[slot].matl.mat_volume);
    sprintf(scratch,"Enter the amount remaining (%):");
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 2, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label),1.0);
    adj_matl_lvl=gtk_adjustment_new(used_matl, 0, 100, 10, 1, 5);
    spn_matl_lvl=gtk_spin_button_new(adj_matl_lvl, 5, 1);
    gtk_grid_attach(GTK_GRID(grd_data),spn_matl_lvl, 2, 2, 1, 1);
    g_signal_connect(spn_matl_lvl,"value-changed",G_CALLBACK(grab_matl_lvl_value),GINT_TO_POINTER(slot));
    }

  // if not additive but does require a power setting...
  if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==TRUE)
    {
    label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 1, 3, 1);

    used_matl=100*Tool[slot].powr.power_duty;
    sprintf(scratch,"Enter the power duty cycle (%):");
    label = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_data), label, 0, 2, 2, 1);
    gtk_label_set_xalign (GTK_LABEL(label),1.0);
    adj_matl_lvl=gtk_adjustment_new(used_matl, 0, 100, 10, 25, 25);
    spn_matl_lvl=gtk_spin_button_new(adj_matl_lvl, 5, 1);
    gtk_grid_attach(GTK_GRID(grd_data),spn_matl_lvl, 2, 2, 1, 1);
    g_signal_connect(spn_matl_lvl,"value-changed",G_CALLBACK(grab_matl_lvl_value),GINT_TO_POINTER(slot));
    }

  label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_attach (GTK_GRID (grd_data), label, 0, 3, 3, 1);

  btn_done = gtk_button_new_with_label ("Done");
  g_signal_connect (btn_done, "clicked", G_CALLBACK (grab_matl_lvl_done), win_data);
  gtk_grid_attach (GTK_GRID(grd_data), btn_done, 0, 4, 1, 1);

  gtk_widget_set_visible(win_data,TRUE);
  
  return(1);
}

// Callback to get tool material volume override
void matl_override_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  
  slot=GPOINTER_TO_INT(user_data);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(Tool[slot].state==TL_EMPTY)return;

  Tool[slot].matl.mat_volume=gtk_spin_button_get_value(button);
  if(Tool[slot].matl.mat_volume<1)Tool[slot].matl.mat_volume=1;
  if(Tool[slot].matl.mat_volume>100)Tool[slot].matl.mat_volume=100;
  
  return;
}


gint grab_int_value(GtkSpinButton *button, gpointer user_data)
{
  return gtk_spin_button_get_value_as_int(button);
}


// Function to write tool specific parameters and calibration values to both
// tool memories and to tool files.
int tool_information_write(int slot)
{
  int		i,status;
  char		tool_file_name[255];
  FILE 		*tool_file;
  time_t 	memtime;
  struct tm 	memtm;
  

  printf("\n\nTool Information Write ----------------------------------\n");
  dump_tool_params(slot);

  if(strlen(Tool[slot].name)<2)strcpy(Tool[slot].name,"XXX");
  
  // if tool has no memory... build file name from "name" and "sernum"
  if(strlen(Tool[slot].mmry.dev)<8)
    {sprintf(tool_file_name,"/home/aa/Documents/4X3D/%s-%04d.tool",Tool[slot].name,Tool[slot].sernum);}
    
  // if tool has memory ID... use it for the file name
  else 
    {sprintf(tool_file_name,"/home/aa/Documents/4X3D/%s.tool",Tool[slot].mmry.dev);}
    
  printf("\nTool Information Write:\n");
  printf("  name=%s\n",Tool[slot].name);
  printf("  sernum=%s\n",Tool[slot].sernum);
  printf("  mmry.dev=%s\n",Tool[slot].mmry.dev);
  printf("  materail=%s\n",Tool[slot].matl.name);

  // first ensure the mmry.md data and the raw tool data are in agreement
  sprintf(Tool[slot].mmry.md.rev,"%01d%04d%01d",AA4X3D_MAJOR_VERSION,AA4X3D_MINOR_VERSION,AA4X3D_MICRO_VERSION);
  strcpy(Tool[slot].mmry.md.name,Tool[slot].name);
  strcpy(Tool[slot].mmry.md.manf,Tool[slot].manuf);
  memtime = time(NULL);
  memtm = *localtime(&memtime);
  sprintf(Tool[slot].mmry.md.date,"%04d%02d%02d",memtm.tm_year+1900,memtm.tm_mon+1,memtm.tm_mday);
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
  
  Tool[slot].mmry.md.tip_pos_X=Tool[slot].tip_x_fab;
  Tool[slot].mmry.md.tip_pos_Y=Tool[slot].tip_y_fab;
  Tool[slot].mmry.md.tip_pos_Z=Tool[slot].tip_z_fab;
  Tool[slot].mmry.md.tip_diam=Tool[slot].tip_diam;
  
  Tool[slot].mmry.md.extra_calib[0]=Tool[slot].thrm.temp_offset;
  
  for(i=1;i<4;i++)Tool[slot].mmry.md.extra_calib[i]=0.0;
  
  // write to 1wire memory if it is available
  printf("\nTool Info Write: \n");
  if(strlen(Tool[slot].mmry.dev)>8)
    {
    status=MemDevWrite(slot);
    if(status==TRUE){printf("   ... memory data write succesfful.\n");}
    else {printf("   ... memory data write failed.\n");}
    }

  // debug
  status=MemDevDataValidate(slot);
  if(status==TRUE){printf("   ... memory data validation succesfful.\n");}
  else {printf("   ... memory data validation failed.\n");}

  // write to tool file on disc as backup so they match
  tool_file=fopen(tool_file_name,"w");
  if(tool_file!=NULL)
    {
    fwrite(&Tool[slot].mmry.md,sizeof(Tool[slot].mmry.md),1,tool_file);
    fflush(tool_file);
    fclose(tool_file);
    printf("   ... tool info write to disc successful.\n");
    }

  dump_tool_params(slot);
  printf("---------------------------------------Tool Information Write\n");
  
  return(1);
}

// Function to read tool specific parameters and calibration values
int tool_information_read(int slot, char *tool_file_name)
{
  int		h,i,status=FALSE;
  FILE 		*tool_file;

  printf("\n\nTool Information Read ------------------------------------\n");
  dump_tool_params(slot);

  // if tool has no memory...strip path information off of tool file name to get tool name
  if(strlen(Tool[slot].mmry.dev)<8)
    {
    h=0;
    memset(scratch,0,sizeof(scratch));
    for(i=0;i<strlen(tool_file_name);i++)
      {
      if(tool_file_name[i]==47)						// if "/" clear scratch string
	{
	memset(scratch,0,sizeof(scratch));
	h=0;
	continue;
	}
      if(tool_file_name[i]==45)						// if "-" save what's in front of it
	{
	strcpy(Tool[slot].mmry.md.name,scratch);
	memset(scratch,0,sizeof(scratch));
	h=0;
	continue;
	}
      if(tool_file_name[i]==46)						// if "." save what's in front of it
	{
	Tool[slot].mmry.md.sernum=atoi(scratch);
	break;
	}
      scratch[h]=tool_file_name[i];
      h++;
      }
    }
  // if tool has a memory ID... use it as file name
  else 
    {
    sprintf(tool_file_name,"/home/aa/Documents/4X3D/%s.tool",Tool[slot].mmry.dev);
    }

  // read parameters from tool file
  printf("\nOpening tool file %s for read.\n",tool_file_name);
  tool_file=fopen(tool_file_name,"r");
  if(tool_file!=NULL)
    {
    // load what's found in file into memory data structure as if it were read from memory
    if(fread(&Tool[slot].mmry.md,sizeof(Tool[slot].mmry.md),1,tool_file)<1)
      {
      printf("/nError - unable to correctly read tool memory file!\n");
      }	
    fclose(tool_file);

    // reflect what was just loaded into memory data structure into tool data structures
    strcpy(Tool[slot].name,Tool[slot].mmry.md.name);
    Tool[slot].sernum=Tool[slot].mmry.md.sernum;
    Tool[slot].tool_ID=Tool[slot].mmry.md.toolID;

    // extract remaining parameters for that tool from the TOOLS.XML file now that tool type is known
    // tool type is referenced in .name variable
    status=XMLRead_Tool(slot,Tool[slot].name);	
    
    // if we now have a complete tool profile...
    printf("\ntool_information_read:  slot=%d  tool=%s  matp=%s \n",slot,Tool[slot].name,Tool[slot].mmry.md.matp);
    if(status==TRUE)
      {
      Tool[slot].powr.calib_d_low=Tool[slot].mmry.md.powr_lo_calduty;
      Tool[slot].powr.calib_v_low=Tool[slot].mmry.md.powr_lo_calvolt;
      Tool[slot].powr.calib_d_high=Tool[slot].mmry.md.powr_hi_calduty;
      Tool[slot].powr.calib_v_high=Tool[slot].mmry.md.powr_hi_calvolt;
    
      Tool[slot].thrm.calib_t_low=Tool[slot].mmry.md.temp_lo_caltemp;
      Tool[slot].thrm.calib_v_low=Tool[slot].mmry.md.temp_lo_calvolt;
      Tool[slot].thrm.calib_t_high=Tool[slot].mmry.md.temp_hi_caltemp;
      Tool[slot].thrm.calib_v_high=Tool[slot].mmry.md.temp_hi_calvolt;
    
      Tool[slot].tip_x_fab = Tool[slot].mmry.md.tip_pos_X;
      Tool[slot].tip_y_fab = Tool[slot].mmry.md.tip_pos_Y;
      Tool[slot].tip_z_fab = Tool[slot].mmry.md.tip_pos_Z;
      
      Tool[slot].tip_diam=Tool[slot].mmry.md.tip_diam;
      
      Tool[slot].thrm.temp_offset=Tool[slot].mmry.md.extra_calib[0];
  
      printf("\nTool values read from file.\n");
  
      // load material params based on matp value found in memory/file
      build_material_index(slot);
      status=XMLRead_Matl(slot,Tool[slot].mmry.md.matp);
      }
      
    }
  
  dump_tool_params(slot);
  printf("-----------------------------------------Tool Information Read\n");

  return(status);
}

// Function to dump tool parameters
int dump_tool_params(int slot)
{
  printf("\n\n  Tool %d Parameters:\n",slot);
  printf("  memory rev.....: %01d%04d%01d  %s\n",AA4X3D_MAJOR_VERSION,AA4X3D_MINOR_VERSION,AA4X3D_MICRO_VERSION,Tool[slot].mmry.md.rev);
  printf("  name...........: %s  %s\n",Tool[slot].name,Tool[slot].mmry.md.name);
  printf("  tool_ID........: %d  %d\n",Tool[slot].tool_ID,Tool[slot].mmry.md.toolID);
  printf("  manufacturer...: %s  %s\n",Tool[slot].manuf,Tool[slot].mmry.md.manf);
  printf("  material.......: %s  %s\n",Tool[slot].matl.name,Tool[slot].mmry.md.matp);
  printf("  sernum.........: %04d  %04d\n",Tool[slot].sernum,Tool[slot].mmry.md.sernum);
  printf("  date...........: %s  \n",Tool[slot].mmry.md.date);
  printf("  type...........: %d  \n",Tool[slot].type);
  printf("  weight.........: %f  \n",Tool[slot].weight);
  printf("  power 48v......: %d  \n",Tool[slot].pwr48);
  printf("  power 24v......: %d  \n",Tool[slot].pwr24);
  printf("  temp sensor....: %d  \n",Tool[slot].thrm.sensor);
  printf("  maxtC..........: %f  \n",Tool[slot].thrm.maxtC);

  printf("  calib_d_low....: %f  %f\n",Tool[slot].powr.calib_d_low,Tool[slot].mmry.md.powr_lo_calduty);
  printf("  calib_v_low....: %f  %f\n",Tool[slot].powr.calib_v_low,Tool[slot].mmry.md.powr_lo_calvolt);
  printf("  calib_d_high...: %f  %f\n",Tool[slot].powr.calib_d_high,Tool[slot].mmry.md.powr_hi_calduty);
  printf("  calib_v_high...: %f  %f\n",Tool[slot].powr.calib_v_high,Tool[slot].mmry.md.powr_hi_calvolt);
  
  printf("  calib_t_low....: %f  %f\n",Tool[slot].thrm.calib_t_low,Tool[slot].mmry.md.temp_lo_caltemp);
  printf("  calib_v_low....: %f  %f\n",Tool[slot].thrm.calib_v_low,Tool[slot].mmry.md.temp_lo_calvolt);
  printf("  calib_t_high...: %f  %f\n",Tool[slot].thrm.calib_t_high,Tool[slot].mmry.md.temp_hi_caltemp);
  printf("  calib_v_high...: %f  %f\n",Tool[slot].thrm.calib_v_high,Tool[slot].mmry.md.temp_hi_calvolt);
  
  printf("  tip x fab......: %f  %f\n",Tool[slot].tip_x_fab,Tool[slot].mmry.md.tip_pos_X);
  printf("  tip y fab......: %f  %f\n",Tool[slot].tip_y_fab,Tool[slot].mmry.md.tip_pos_Y);
  printf("  tip z fab......: %f  %f\n",Tool[slot].tip_z_fab,Tool[slot].mmry.md.tip_pos_Z);
  printf("  tip_up_pos.....: %f  \n",Tool[slot].tip_up_pos);
  
  printf("  extra calibs...: %f %f %f %f \n\n",Tool[slot].mmry.md.extra_calib[0],Tool[slot].mmry.md.extra_calib[1],
         Tool[slot].mmry.md.extra_calib[2],Tool[slot].mmry.md.extra_calib[3]);
 
  return(1);
}

// Load tool with default params
// The idea is to provide a minimalist set of values to make the tool operational.
int load_tool_defaults(int slot)
{
	
    // init general tool data
    // this is typically overwritten by initialization or when a new tool is installed
    // the data ultimately comes from the TOOLS.XML file, but gets directed there either by the user, by a file, or by 1wire memory
    Tool[slot].state=TL_UNKNOWN;
    Tool[slot].select_status=FALSE;
    Tool[slot].used_status=FALSE;
    Tool[slot].sernum=UNDEFINED;
    Tool[slot].tool_ID=UNDEFINED;
    Tool[slot].type=UNDEFINED;
    Tool[slot].homed=FALSE;
    sprintf(Tool[slot].name,"UNKNOWN");
    sprintf(Tool[slot].desc,"UNKNOWN");
    sprintf(Tool[slot].manuf,"UNKNOWN");
    Tool[slot].x_offset= 0.0; Tool[slot].y_offset= 0.0;
    if(slot==0){Tool[0].x_offset=  20.0; Tool[0].y_offset=  32.0;}
    if(slot==1){Tool[1].x_offset= -20.5; Tool[1].y_offset=  40.8;}
    if(slot==2){Tool[2].x_offset=  20.0; Tool[2].y_offset= -41.0;}
    if(slot==3){Tool[3].x_offset= -21.5; Tool[3].y_offset= -40.0;}
    Tool[slot].tip_x_fab=0.0;						// these hold fabrication deltas if no memory
    Tool[slot].tip_y_fab=0.0;
    Tool[slot].tip_z_fab=0.0;
    Tool[slot].step_ID=0;
    Tool[slot].step_max=0;
    Tool[slot].tip_up_pos=12.0;
    Tool[slot].tip_dn_pos=9.0;
    Tool[slot].tip_cr_pos= 0.0;
    Tool[slot].pwr48=FALSE;						// element present in tool
    Tool[slot].pwr24=FALSE;
    Tool[slot].spibus=FALSE;
    Tool[slot].wire1=FALSE;
    Tool[slot].limitsw=FALSE;
    Tool[slot].pwmtool=FALSE;
    Tool[slot].epwr48=FALSE;						// element error status
    Tool[slot].epwr24=FALSE;
    Tool[slot].espibus=FALSE;
    Tool[slot].ewire1=FALSE;
    Tool[slot].elimitsw=FALSE;
    Tool[slot].epwmtool=FALSE;
    Tool[slot].tip_diam=1.0;
    Tool[slot].max_drill_diam=1.624;					// 1/16'th of an inch
    Tool[slot].weight=100.0;
    Tool[slot].stepper_drive_ratio=1.000;
    
    // init tool memory data
    // this is typically overwritten by initialization or when a new tool is installed
    if(strlen(Tool[slot].mmry.dev)<7)
        {
	Tool[slot].mmry.fd=NULL;
	memset(Tool[slot].mmry.dev,0,sizeof(Tool[slot].mmry.dev));
	memset(Tool[slot].mmry.devPath,0,sizeof(Tool[slot].mmry.devPath));
	memset(Tool[slot].mmry.desc,0,sizeof(Tool[slot].mmry.desc));
	Tool[slot].mmry.devType=UNDEFINED;
	
	// init tool eeprom substructure data
	// this is typically overwritten by initialization or when a new tool is installed
	memset(Tool[slot].mmry.md.rev,0,sizeof(Tool[slot].mmry.md.rev));
	sprintf(Tool[slot].mmry.md.rev,"%01d%04d%01d",AA4X3D_MAJOR_VERSION,AA4X3D_MINOR_VERSION,AA4X3D_MICRO_VERSION);
	memset(Tool[slot].mmry.md.name,0,sizeof(Tool[slot].mmry.md.name));
	memset(Tool[slot].mmry.md.manf,0,sizeof(Tool[slot].mmry.md.manf));
	memset(Tool[slot].mmry.md.date,0,sizeof(Tool[slot].mmry.md.date));
	memset(Tool[slot].mmry.md.matp,0,sizeof(Tool[slot].mmry.md.matp));
	Tool[slot].mmry.md.sernum=UNDEFINED;
	Tool[slot].mmry.md.toolID=UNDEFINED;
	Tool[slot].mmry.md.powr_lo_calduty=0.0;
	Tool[slot].mmry.md.powr_lo_calvolt=0.0;
	Tool[slot].mmry.md.powr_hi_calduty=1.0;
	Tool[slot].mmry.md.powr_hi_calvolt=48.0;
	Tool[slot].mmry.md.temp_lo_caltemp=50.0;
	Tool[slot].mmry.md.temp_lo_calvolt=0.893;
	Tool[slot].mmry.md.temp_hi_caltemp=200.0;
	Tool[slot].mmry.md.temp_hi_calvolt=1.259;
	Tool[slot].mmry.md.tip_pos_X=0.0;
	Tool[slot].mmry.md.tip_pos_Y=0.0;
	Tool[slot].mmry.md.tip_pos_Z=0.0;
	Tool[slot].mmry.md.extra_calib[0]=0.0;
	Tool[slot].mmry.md.extra_calib[1]=0.0;
	Tool[slot].mmry.md.extra_calib[2]=0.0;
	Tool[slot].mmry.md.extra_calib[3]=0.0;
        }

    // init tool thermal data
    // this data will be overwritten by several events:  when a tool is defined, when a material is defined, or by the user
    Tool[slot].thrm.sensor=UNDEFINED;
    Tool[slot].thrm.fd=0;
    memset(Tool[slot].thrm.dev,0,sizeof(Tool[slot].thrm.dev));
    memset(Tool[slot].thrm.devPath,0,sizeof(Tool[slot].thrm.devPath));
    sprintf(Tool[slot].thrm.desc,"Tool %d",slot+1);
    Tool[slot].thrm.maxtC=25.0;
    Tool[slot].thrm.temp_offset=0.0;
    Tool[slot].thrm.setpC=20.0;
    Tool[slot].thrm.tolrC=acceptable_temp_tol;
    Tool[slot].thrm.backC=20.0;
    Tool[slot].thrm.operC=20.0;
    Tool[slot].thrm.hystC=2.0;
    Tool[slot].thrm.waitC=0.0;
    Tool[slot].thrm.bedtC=10.0;
    Tool[slot].thrm.errtC=3.0;
    Tool[slot].thrm.duration_per_C=10.0;
    Tool[slot].thrm.Kp=5.00;						// was 8.00
    Tool[slot].thrm.Ki=0.05;						// was 0.15
    Tool[slot].thrm.Kd=10.00;						// was 10.00
    
    Tool[slot].thrm.calib_t_low=crgslot[slot].calib_t_low;		// as read from 4x3d calib file
    Tool[slot].thrm.calib_v_low=crgslot[slot].calib_v_low;
    Tool[slot].thrm.calib_t_high=crgslot[slot].calib_t_high;
    Tool[slot].thrm.calib_v_high=crgslot[slot].calib_v_high;

    //Tool[slot].thrm.calib_t_low=50.0;
    //Tool[slot].thrm.calib_v_low=0.893;
    //Tool[slot].thrm.calib_t_high=200.0;
    //Tool[slot].thrm.calib_v_high=1.259;
    
    Tool[slot].thrm.start_t_low=Tool[slot].thrm.calib_t_low;
    Tool[slot].thrm.start_v_low=Tool[slot].thrm.calib_v_low;
    Tool[slot].thrm.setpC_t_low=25.0;
    Tool[slot].thrm.setpC_t_high=25.0;
    Tool[slot].thrm.temp_port=slot;
    Tool[slot].thrm.PWM_port=slot;
    Tool[slot].thrm.heat_status=0;
    Tool[slot].thrm.heat_duration=0;
    Tool[slot].thrm.heat_duration_max=4095;
    Tool[slot].thrm.heat_duty=0.0;
    Tool[slot].thrm.heat_time=time(NULL);

    // init material data
    // this data ultimately comes from the MATERIALS.XML file, but gets directed there by file, by 1wire, or by user
    Tool[slot].matl.ID=(-1);						// init to undefined
    sprintf(Tool[slot].matl.name,"UNKNOWN");				// space for name of material type
    Tool[slot].matl.gage_ID=0;						// arbitrary ID of tip type
    Tool[slot].matl.state=UNDEFINED;					// init as nothing loaded
    Tool[slot].matl.diam=1.750;						// diam of material supply in mm
    Tool[slot].matl.layer_height=0.500;					// deposited layer height in mm
    Tool[slot].matl.min_layer_time=15;					// minimum time per layer to allow cooling/solidification
    Tool[slot].matl.overhang_angle=(-45.0);				// max allowable overhang angle in degrees
    Tool[slot].matl.shrink=0.00;					// thermal shrinkage
    Tool[slot].matl.retract=0.30;					// material retraction amt when tool retracts from deposit postion
    Tool[slot].matl.advance=0.30;					// material advance amt when tool moves into deposit position
    Tool[slot].matl.primepush=0.100;					// initial distance to compress plunger to start flow
    Tool[slot].matl.primepull=0.100;					// distance to retract plunger after flow starts
    Tool[slot].matl.primehold=1.000;					// duration to hold at primepush position before retracting to primepull position
    Tool[slot].matl.mat_volume=330000;					// cartridge volume size when full (mL)
    Tool[slot].matl.mat_used=150000;					// amount of volume currently used (mL)
    Tool[slot].matl.mat_pos=10.0;					// stepper position for this cartridge (mm from full position)
    Tool[slot].matl.color=MD_GREEN;
    Tool[slot].matl.max_line_types=MAX_LINE_TYPES;
    Tool[slot].matl.lt=NULL;

    // init power data
    Tool[slot].powr.power_sensor=UNDEFINED;				// 0=None, 1=voltage, 2=current, 3=force
    Tool[slot].powr.power_status=0;					// indicator if on or off
    Tool[slot].powr.power_duration=1.00;				// maximum duty cycle for this device (PWM control)
    Tool[slot].powr.power_duty=0.50;					// current power duty cycle (PWM control)
    Tool[slot].powr.feed_back_port=(-1);
    Tool[slot].powr.PWM_port=slot+8;
    Tool[slot].powr.calib_d_low=Tool[slot].mmry.md.powr_lo_calduty;
    Tool[slot].powr.calib_v_low=Tool[slot].mmry.md.powr_lo_calvolt;
    Tool[slot].powr.calib_d_high=Tool[slot].mmry.md.powr_hi_calduty;
    Tool[slot].powr.calib_v_high=Tool[slot].mmry.md.powr_hi_calvolt;
	
    // init list data
    // this data is overwritten when tools get defined and there associated lists are built (see XML.c functions)
    // operations are defined in the TOOLS.XML file
    // matertials are defined in the MATERIALS.XML file
    Tool[slot].oper_qty=0;
    Tool[slot].oper_first=NULL;
    Tool[slot].oper_last=NULL;
    Tool[slot].mats_list=NULL;
    Tool[slot].oper_list=NULL;
	
    job.state=UNDEFINED;
    tools_need_tip_calib=TRUE;
	
    return(1);
}

// Unload tool with "slot empty" params
int unload_tool(int slot)
{
    int		i;
    slice 	*sdel;
    model	*mptr,*mold;
  
    // clear tool variables
    sprintf(Tool[slot].thrm.desc,"Tool %d",slot+1);
    Tool[slot].select_status=FALSE;
    Tool[slot].used_status=FALSE;
    Tool[slot].pwr48=0;
    Tool[slot].pwr24=0;
    Tool[slot].thrm.sensor=0;
    Tool[slot].step_max=0;
    Tool[slot].spibus=0;
    Tool[slot].wire1=0;
    Tool[slot].limitsw=0;

    Tool[slot].mmry.fd=NULL;
    memset(Tool[slot].mmry.dev,0,sizeof(Tool[slot].mmry.dev));
    memset(Tool[slot].mmry.devPath,0,sizeof(Tool[slot].mmry.devPath));
    memset(Tool[slot].mmry.desc,0,sizeof(Tool[slot].mmry.desc));
    Tool[slot].mmry.devType=UNDEFINED;
    
    Tool[slot].thrm.maxtC=25.0;
    Tool[slot].thrm.setpC=25.0;
    Tool[slot].thrm.operC=25.0;
    Tool[slot].thrm.backC=25.0;
    Tool[slot].thrm.waitC=0.0;
    Tool[slot].thrm.hystC=1.0;
    Tool[slot].thrm.errtC=4.0;
    Tool[slot].thrm.Kp=0.50;	
    Tool[slot].thrm.Ki=0.06;	
    Tool[slot].thrm.Kd=0.10;	
    Tool[slot].thrm.temp_port=(-1);
    Tool[slot].thrm.PWM_port=(-1);
    Tool[slot].thrm.heat_status=0;
    Tool[slot].thrm.heat_duration=0;
    Tool[slot].thrm.heat_duration_max=0;
    Tool[slot].thrm.heat_duty=0.0;
    Tool[slot].thrm.heat_time=time(NULL);
    
    Tool[slot].matl.ID=(-1);
    Tool[slot].matl.state=0;						// update state to empty
    Tool[slot].matl.ID=0;						// update state to empty

    Tool[slot].state=TL_EMPTY;						// update state to unloaded
    Tool[slot].sernum=(-1);
    Tool[slot].tool_ID=UNDEFINED;					// init as undefined
    Tool[slot].type=UNDEFINED;
    Tool[slot].step_ID=0;						// init to null
    Tool[slot].tip_cr_pos=0.0;
    sprintf(Tool[slot].desc,"Empty");
    sprintf(Tool[slot].name,"Empty");
    sprintf(Tool[slot].matl.name,"Empty");

    job.state=UNDEFINED;
	
    return(1);
}


// Function to select printing tool and/or carriage lift stepper
// Inputs: 	ToolID=slot location of carriage/tool, and Step_ID=stepper:  0=no stepper, 1=material drive, 2=carriage drive, etc.
// Outputs:  	0=failure 1=success
int make_toolchange(int ToolID, int StepID)
{
    char	tlchg_scratch[255];
    int		i=0;
    float 	dist=0.0;

    /*
    printf("\nInitiating tool change ------------------------------\n");
    printf("  cur_tool=%d  cur_step=%d  ",current_tool,current_step);
    if(current_step==1 && (current_tool>=0 && current_tool<MAX_TOOLS))
      {printf("  cur_pos=%5.3f   PostGa=%5.3f \n",Tool[current_tool].matl.mat_pos,PostG.a);}
    else if(current_step==2 && (current_tool>=0 && current_tool<MAX_TOOLS))
      {printf("  cur_pos=%5.3f   PostGa=%5.3f \n",Tool[current_tool].tip_cr_pos,PostG.a);}
    else 
      {printf(" \n");}
    printf("  pre_tool=%d  pre_step=%d  ",previous_tool,previous_step);
    if(previous_step==1 && (previous_tool>=0 && previous_tool<MAX_TOOLS))
      {printf("  pre_pos=%5.3f   PostGa=%5.3f \n",Tool[previous_tool].matl.mat_pos);}
    else if(current_step==2 && (previous_tool>=0 && previous_tool<MAX_TOOLS))
      {printf("  pre_pos=%5.3f   PostGa=%5.3f\n",Tool[previous_tool].tip_cr_pos,PostG.a);}
    else 
      {printf(" \n");}
    printf("  target_tool=%d  target_step=%d  ",ToolID,StepID);
    if(StepID==1 && (ToolID>=0 && ToolID<MAX_TOOLS))
      {printf("  target_pos=%5.3f   PostGa=%5.3f \n",Tool[ToolID].matl.mat_pos);}
    else if(StepID==2 && (ToolID>=0 && ToolID<MAX_TOOLS))
      {printf("  target_pos=%5.3f   PostGa=%5.3f \n",Tool[ToolID].tip_cr_pos,PostG.a);}
    else 
      {printf(" \n");}
    */

    // if request is for exactly what is already selected, just return with success
    // make sure that ToolID=(-1) is allowed to fall thru this IF statement... need to turn tools off regardless
    // of which tool was last used.
    if(ToolID>=0 && ToolID==current_tool && StepID==current_step)
      {
      printf("\nAlready using selected tool and stepper... no change made.\n");
      printf("-------------------------------------tool change done. \n\n");
      return(1);
      }
    else 
      {
      printf("\nTool change: tool=%d  step=%d  TO  tool=%d  step%d \n\n",current_tool,current_step,ToolID,StepID);
      }
      

    // enter data to process log file
    //sprintf(tlchg_scratch,"    Tool change.  %d to %d with step=%d\n",current_tool,ToolID,StepID);
    //if(proc_log!=NULL)fwrite(tlchg_scratch,1,strlen(tlchg_scratch),proc_log);
    
    // only call motion complete if a tool was in use, if not no need
    if(current_tool>=0 && current_tool<MAX_TOOLS)motion_complete();

    // turn off all relays to all tools.  since stepper motors are inductive, they hold their charge for a bit
    // preventing the relay from being able to reliably switch.  a delay of at least 750 ms is required to let
    // the stepper that was in use loose its inductive hold so the relay can be cleanly shut off.  this is the cost
    // of sharing one stepper driver among many motors.
    #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
      delay(1250);							// delay needed to let prev stepper discharge bt tool changes
      gpioWrite(TOOL_A_SEL,1);
      gpioWrite(TOOL_B_SEL,1);  
      gpioWrite(TOOL_C_SEL,1);  
      gpioWrite(TOOL_D_SEL,1);  
      gpioWrite(TOOL_A_ACT,1);  
      gpioWrite(TOOL_B_ACT,1);  
      gpioWrite(TOOL_C_ACT,1);  
      gpioWrite(TOOL_D_ACT,1);  
    #endif
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
      gpioWrite(TOOL_A_SEL,1);
    #endif
    
    // if a valid tool stepper is currently active...
    if(current_tool>=0 && current_tool<MAX_TOOLS)
      {
      // and that drive is the material drive...
      if(Tool[current_tool].step_ID==1)
	{
	Tool[current_tool].matl.mat_pos=PostG.a;			// save location of current tool material feed
	printf("WAS:  Tool=%d MAT_pos=%f \n",current_tool+1,Tool[current_tool].matl.mat_pos);
	}
	
      // and that drive is the carriage drive...
      if(Tool[current_tool].step_ID==2)
	{
	Tool[current_tool].tip_cr_pos=PostG.a;				// save vertical location of current tool tip
	printf("WAS:  Tool=%d TIP_pos=%f \n",current_tool+1,Tool[current_tool].tip_cr_pos);
	}
      }

    // shut off all tools request being made
    // note:  requesting ToolID=(-1) is a common way to shut off all tools
    if(ToolID<0 || ToolID>=MAX_TOOLS)
      {
      // turn off holding torque of tool when at home as this is both a strain on the motors/controller but also creates electric noise
      // that interferes with 1wire
      while(tinyGRcv(1)>=0){delay(150);}				// tinyg has a bug where all replies must be cleared before issuing this command;						// get latest tinyg feedback
      tinyGSnd("{4pm:0}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
      delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
      while(tinyGRcv(1)>=0);						// get latest tinyg feedback
      previous_tool=current_tool;					// save off what previous tool was
      current_tool=ToolID;						// switch current tool in use to newly defined tool
      previous_step=current_step;
      current_step=StepID;
      printf("All tools shut off.\n");
      printf("-------------------------------------tool change done. \n\n");
      return(0);
      }

    // if a stepper is no longer requested...
    if(StepID<=0)
      {
      tinyGSnd("{4pm:2}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
      delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
      while(tinyGRcv(1)>=0);						// get latest tinyg feedback
      }

    // if a stepper is requested to be active...
    if(StepID>0)
      {
      oldState=cmdControl;
      cmdControl=0;
      // if a material drive...
      if(StepID==1)
	{
	printf("Activating tool %d power.\n",ToolID+1);
	tinyGSnd("{4pm:2}\n");						// A: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
	delay(150);							// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
	while(tinyGRcv(1)>=0);						// get latest tinyg feedback
	Tool[ToolID].step_ID=1;						// ... set stepper ID
	if(ToolID==0)gpioWrite(TOOL_A_ACT,0);				// ... turn on GPIO port
	if(ToolID==1)gpioWrite(TOOL_B_ACT,0);
	if(ToolID==2)gpioWrite(TOOL_C_ACT,0);
	if(ToolID==3)gpioWrite(TOOL_D_ACT,0);
	sprintf(gcode_cmd,"G28.3 A%f \n",Tool[ToolID].matl.mat_pos);	// ... reset stepper to position of NEW tool
	tinyGSnd(gcode_cmd);						// ... issue command
	PostG.a=Tool[ToolID].matl.mat_pos;				// ... ensure post postion matches
	}
      // if a carriage tip drive...
      if(StepID==2) 
	{
	printf("Activating carriage slot %d power.\n",ToolID+1);
	Tool[ToolID].step_ID=2;						// ... set stepper ID
	if(ToolID==0)gpioWrite(TOOL_A_SEL,0);				// ... turn on GPIO port
	if(ToolID==1)gpioWrite(TOOL_B_SEL,0);
	if(ToolID==2)gpioWrite(TOOL_C_SEL,0);
	if(ToolID==3)gpioWrite(TOOL_D_SEL,0);
	sprintf(gcode_cmd,"G28.3 A%f \n",Tool[ToolID].tip_cr_pos);	// ... reset stepper to position of NEW tool tip
	tinyGSnd(gcode_cmd);						// ... issue command
	PostG.a=Tool[ToolID].tip_cr_pos;				// ... ensure post position matches
	}
      cmdControl=oldState;
      }

    previous_tool=current_tool;						// save off what previous tool was
    current_tool=ToolID;						// switch current tool in use to newly defined tool
    previous_step=current_step;
    current_step=StepID;
    if(current_step==0)printf("IS:   Tool=%d no stepper \n",current_tool+1);
    if(current_step==1)printf("IS:   Tool=%d MAT_pos=%f \n",current_tool+1,Tool[current_tool].matl.mat_pos);
    if(current_step==2)printf("IS:   Tool=%d TIP_pos=%f \n",current_tool+1,Tool[current_tool].tip_cr_pos);
    delay(250);
    printf("-------------------------------------tool change done. \n");
    return(1);
}
	
// Function to print tool change command to PolyMat
// Inputs:  new tool
// Return:	0=failure, 1=success
int print_toolchange(int ToolID)
{
	
    sprintf(gcode_cmd,"M6 T%d \n",ToolID);			// ... write a tool change command
    tinyGSnd(gcode_cmd);
    previous_tool=current_tool;
    current_tool=ToolID;
	
    return(1);
}

// Function to stop material test on user request
void stop_material_test(GtkWidget *btn, gpointer user_data)
{
  job.state=JOB_ABORTED_BY_USER;
  return;
}

// Function to build a single wall of all line types called out in a material file
// This is used as a calibration tool to understand the actual result of what is predicted.
int build_mat_file(GtkWidget *btn_call, gpointer user_data)
{
  int		slot;
  float 	x_pos,y_pos,z_pos;
  float 	v_len,y_inc,z_max;
  linetype	*lptr;
  vertex	*vtx_st,*vtx_end;
  model 	*mdlptr;
  GtkWidget	*win_dialog,*grid_test_matl,*btn_abort,*lbl_info;
  
  // build dialog window so process can be aborted
  win_dialog = gtk_window_new();
  gtk_window_set_transient_for (GTK_WINDOW(win_dialog), GTK_WINDOW(win_tool));	
  gtk_window_set_modal(GTK_WINDOW(win_dialog),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_dialog),300,150);
  gtk_window_set_resizable(GTK_WINDOW(win_dialog),FALSE);			
  sprintf(scratch," Print Material Linetypes ");
  gtk_window_set_title(GTK_WINDOW(win_dialog),scratch);			

  grid_test_matl=gtk_grid_new();
  gtk_window_set_child(GTK_WINDOW(win_dialog),grid_test_matl);
  gtk_grid_set_row_spacing (GTK_GRID(grid_test_matl),10);
  gtk_grid_set_column_spacing (GTK_GRID(grid_test_matl),10);
  
  sprintf(scratch,"  ");
  lbl_info=gtk_label_new(scratch);
  gtk_grid_attach (GTK_GRID(grid_test_matl), lbl_info, 0, 0, 3, 1);

  sprintf(scratch,"  Printing test pattern... ");
  lbl_info=gtk_label_new(scratch);
  gtk_grid_attach (GTK_GRID(grid_test_matl), lbl_info, 0, 1, 3, 1);
  gtk_label_set_xalign (GTK_LABEL(lbl_info),0.5);

  sprintf(scratch,"  ");
  lbl_info=gtk_label_new(scratch);
  gtk_grid_attach (GTK_GRID(grid_test_matl), lbl_info, 0, 2, 3, 1);

  btn_abort = gtk_button_new_with_label(" Abort ");
  g_signal_connect (btn_abort, "clicked", G_CALLBACK (stop_material_test), NULL);
  gtk_grid_attach (GTK_GRID(grid_test_matl), btn_abort, 1, 3, 1, 1);
  
  gtk_widget_set_visible(win_dialog,TRUE);
  gtk_widget_queue_draw(GTK_WIDGET(win_dialog));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  // init
  mdlptr=NULL;
  slot=GPOINTER_TO_INT(user_data);
  if(active_model!=NULL){mdlptr=active_model; active_model=NULL;}

  slot=0;
  x_pos=250.000;
  y_pos=100.000;
  z_pos=0.050;

  v_len=100.000;
  y_inc=10.000;
  z_max=20*Tool[slot].matl.layer_height+z_pos;

  fidelity_mode_flag=FALSE;
  adjflow_flag=FALSE;
  z_tbl_comp=TRUE;
  
  // define line length and location on table
  vtx_st=vertex_make();
  vtx_st->x=x_pos; vtx_st->y=y_pos;
  vtx_end=vertex_make();
  vtx_end->x=x_pos+v_len; vtx_end->y=y_pos;
  
  // move tool to table
  make_toolchange(slot,1);						// set to tool stepper drive
  comefrom_machine_home(slot,vtx_st);
  memset(gcode_burst,0,sizeof(gcode_burst));
  
  // starting at z=z_pos and repeating until up to z_max
  job.state=JOB_CALIBRATING;
  while(z_pos<z_max)
    {
    // set starting coords for first line type of this layer
    vtx_st->x  =x_pos; 		vtx_st->y  =y_pos; 	vtx_st->z  =z_pos;
    vtx_end->x =x_pos+v_len; 	vtx_end->y =y_pos; 	vtx_end->z =z_pos;
      
    // cycle through list of loaded line types
    lptr=Tool[slot].matl.lt;						// head node of material line type list
    while(lptr!=NULL)
      {
      if(strstr(lptr->name,"MDL")!=NULL)
	{
	printf("\n\nCurrent z of zmax = %7.3f of %7.3f \n",z_pos,z_max);
	printf("Line type... = %s \n",lptr->name);
	printf("  ID ....... = %d \n",lptr->ID);
	printf("  Temp adj.. = %7.3f \n",lptr->tempadj);
	printf("  Line width = %7.3f \n",lptr->line_width);
	printf("  Line pitch = %7.3f \n",lptr->line_pitch);
	printf("  Wall width = %7.3f \n",lptr->wall_width);
	printf("  Wall pitch = %7.3f \n",lptr->wall_pitch);
	printf("  Flowrate.. = %7.3f \n",lptr->flowrate/Tool[slot].stepper_drive_ratio);
	printf("  Feedrate.. = %7.3f \n",lptr->feedrate);
	printf("  Retraction = %7.3f \n",lptr->retract);
	printf("  Advance... = %7.3f \n",lptr->advance);
	printf("  Min veclen = %7.3f \n",lptr->minveclen);
	printf("  Touchdepth = %7.3f \n",lptr->touchdepth);
	printf("  Hieght mod = %7.3f \n",lptr->height_mod);
	printf("  Thickness. = %7.3f \n",lptr->thickness);
	printf("  Pattern... = %s \n",lptr->pattern);
	printf("  Pat angle. = %7.3f \n",lptr->p_angle);
    
	// pen up to line start
	vtx_st->i=0;
	print_vertex(vtx_st,slot,0,lptr);				// pen up to start of line (includes mat'l retract/advance & tip lift/set)
	tinyGSnd(gcode_burst);						// send string of commands to fill buffer
	memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
	motion_complete();

	// deposit the line
	vtx_end->i=0;
	delay(lptr->poly_start_delay);					// delay to allow pressure to build up in nozzle before moving
	print_vertex(vtx_end,slot,1,lptr);				// pen down to end of line with z table compensation
	tinyGSnd(gcode_burst);						// send string of commands to fill buffer
	memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
	motion_complete();

	// increment y position
	vtx_st->y  += y_inc;
	vtx_end->y += y_inc;
	vtx_st->attr=0;
	vtx_end->attr=0;
	}

      lptr=lptr->next;
    
      while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
      if(job.state==JOB_ABORTED_BY_USER)break;
      }

    // increment z
    z_pos += Tool[slot].matl.layer_height;
    if(job.state==JOB_ABORTED_BY_USER)break;
    }
    
  gtk_window_close(GTK_WINDOW(win_dialog));
  gtk_widget_queue_draw(GTK_WIDGET(win_tool));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  job.state=JOB_COMPLETE;
  goto_machine_home();
  free(vtx_st); vertex_mem--;
  free(vtx_end); vertex_mem--;
  if(mdlptr!=NULL)active_model=mdlptr;
  
  return(1);
}

// Callback to manage UI with each tool -------------------------------------------------------------------------------------
// Create the window with buttons, but use different function to display information
int tool_UI(GtkWidget *btn_call, gpointer user_data)
{
  
    gint		slot;

    if(job.state==JOB_RUNNING)						// don't allow entry if job in progress
      {
      sprintf(scratch,"\n This operation is not allowed when the job ");
      strcat(scratch, "\n is running.  Pause first, then try again.  ");
      aa_dialog_box(win_main,0,100,"Unavailable",scratch);
      return(0);
      }

    // before creating a bunch of GTK variables that would need to be destroyed, test if tool is there
    slot=GPOINTER_TO_INT(user_data);
    if(slot<0 || slot>=MAX_TOOLS)return(0);
    if(Tool[slot].state==TL_EMPTY)return(0);
      
    gint		h;
    GtkWidget		*hbox, *mbox, *lbox, *vboxleft,*vboxright;
    GtkWidget		*tool_box,*statboxhi,*statboxlo,*toolbox,*matlbox;
    GtkWidget		*tl_btn_box,*tl_temp_box,*tl_set_box,*tl_tip_box;
    GtkWidget		*tl_btn_grd,*tl_temp_grd,*tl_set_grd,*tl_tip_grd;
    GtkWidget		*mt_btn_box,*mt_amnt_box,*mt_set_box,*mt_lnt_box;
    GtkWidget		*mt_btn_grd,*mt_amnt_grd,*mt_set_grd,*mt_lnt_grd;

    GtkWidget		*tool_status_area,*tool_definition_area,*matl_definition_area;
    GtkWidget		*grid_status_area,*grid_matl_area;
    GtkWidget		*lbl_tool_definition,*lbl_tool_model,*lbl_tool_settings;

    GtkWidget		*hsep;
    GtkWidget 		*label,*lbl_spacer;
    GtkWidget 		*lbl_info,*lbl_file;
    GtkWidget		*img_file,*btn_file;
    GtkWidget		*img_clear,*btn_clear;
    GtkWidget		*img_chng, *btn_chng;
    GtkWidget		*img_hprg, *btn_hprg;
    GtkWidget		*img_test,*btn_test;
    GtkWidget		*img_info,*btn_info;
    GtkWidget		*img_home,*btn_home;
    GtkWidget		*img_tset,*btn_tset;
    GtkWidget		*img_exit,*btn_exit;
    GtkWidget		*ztip_test_btn,*xy_test_btn;
    GtkWidget		*btn_lotemp,*btn_hitemp;
    GtkWidget		*btn_override,*btn_reset;
    GtkWidget		*btn_tipcenter,*btn_tip2pts;
    GtkWidget		*btn_color, *btn_setamt;
    GtkWidget		*btn_loduty,*btn_hiduty;
    GtkWidget		*btn_tool_save,*btn_tool_open, *btn_matl_save, *btn_matl_open;

    GtkAdjustment	*drill_hole_adj;
    GtkWidget		*drill_hole_btn,*btn_base_layer;
    
    int			i,win_width,win_height;
    int			col_width=32,combo_active;
    char 		mdl_name[255];
    float 		q,fval=0.0;
    model		*mptr;
    linetype		*lptr;
    genericlist		*tptr,*aptr;


    
    make_toolchange(slot,1);						// turn tool power on and set current_tool
    idle_start_time=time(NULL);						// reset idle time
    active_model=job.model_first;					// default to first model
    win_width=LCD_WIDTH-150;
    win_height=LCD_HEIGHT-100;

    // if system sleeping... wake this tool up
    //if(Tool[slot].pwr24==TRUE && Tool[slot].thrm.sensor>0)		// if this tool has a thermal control...
    //  {
    //  if(Tool[slot].thrm.setpC==Tool[slot].thrm.backC)			// ... reset temp to setback temp for ALL tools
	//{Tool[slot].thrm.setpC=Tool[slot].thrm.operC;}			// ... reset temp to operating temp
      //}
    
    // set up tool interface window
    win_tool = gtk_window_new();
    gtk_window_set_transient_for (GTK_WINDOW(win_tool), GTK_WINDOW(win_main));	
    gtk_window_set_modal(GTK_WINDOW(win_tool),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_tool),win_width,win_height);
    gtk_window_set_resizable(GTK_WINDOW(win_tool),FALSE);			
    sprintf(scratch," Tool %d ",(slot+1));
    gtk_window_set_title(GTK_WINDOW(win_tool),scratch);			
    g_signal_connect (win_tool, "destroy", G_CALLBACK (on_tool_exit), win_tool);
    
    // set up an mbox to divide the screen into a narrower segments
    mbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_window_set_child(GTK_WINDOW(win_tool),mbox);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    //gtk_box_set_spacing (GTK_BOX(hbox),20);
    gtk_box_append(GTK_BOX(mbox),hbox);
    lbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_box_append(GTK_BOX(mbox),lbox);

    // now divide left side into segments for buttons
    vboxleft = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vboxleft);
    
    // now divide the right side into segments for status, tool def, and matl def
    vboxright = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vboxright);
    statboxhi = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),statboxhi);
    statboxlo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),statboxlo);
    toolbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(statboxlo),toolbox);
    matlbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(statboxlo),matlbox);
    
    // set up control buttons on left side 
    {
      if(Tool[slot].tool_ID!=TC_LASER)
        {
        btn_madd = gtk_button_new ();
        g_signal_connect (btn_madd, "clicked", G_CALLBACK (tool_add_matl), GINT_TO_POINTER(slot));
        gtk_box_append(GTK_BOX(vboxleft),btn_madd);
        img_madd = gtk_image_new_from_file("Matl-Add-Generic-B.gif");
        if(Tool[slot].tool_ID==TC_EXTRUDER)img_madd = gtk_image_new_from_file("Matl-Extrude-Add-G.gif");
        if(Tool[slot].tool_ID==TC_FDM)img_madd = gtk_image_new_from_file("Matl-Filament-Add-G.gif");
        if(Tool[slot].tool_ID==TC_ROUTER)img_madd = gtk_image_new_from_file("Tip-Bit-Add-G.gif");
        gtk_image_set_pixel_size(GTK_IMAGE(img_madd),50);
        gtk_button_set_child(GTK_BUTTON(btn_madd),img_madd);
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_madd), "Advance Material");
  
        btn_msub = gtk_button_new ();
        g_signal_connect (btn_msub, "clicked", G_CALLBACK (tool_sub_matl), GINT_TO_POINTER(slot));
        gtk_box_append(GTK_BOX(vboxleft),btn_msub);
        img_msub = gtk_image_new_from_file("Matl-Sub-Generic-B.gif");
        if(Tool[slot].tool_ID==TC_EXTRUDER)img_msub = gtk_image_new_from_file("Matl-Extrude-Remove-G.gif");
        if(Tool[slot].tool_ID==TC_FDM)img_msub = gtk_image_new_from_file("Matl-Filament-Remove-G.gif");
        if(Tool[slot].tool_ID==TC_ROUTER)img_msub = gtk_image_new_from_file("Tip-Bit-Remove-G.gif");
        gtk_image_set_pixel_size(GTK_IMAGE(img_msub),50);
        gtk_button_set_child(GTK_BUTTON(btn_msub),img_msub);
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_msub), "Retract Material");
	}
  
      if(Tool[slot].pwr24==TRUE)
        {
        btn_hprg = gtk_button_new ();
        g_signal_connect (btn_hprg, "clicked", G_CALLBACK (tool_hotpurge_matl), GINT_TO_POINTER(slot));
        gtk_box_append(GTK_BOX(vboxleft),btn_hprg);
        img_hprg = gtk_image_new_from_file("Matl-HotPurge.gif");
        gtk_image_set_pixel_size(GTK_IMAGE(img_hprg),50);
        gtk_button_set_child(GTK_BUTTON(btn_hprg),img_hprg);
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_hprg), "Hot Purge");
	}
  
      btn_test = gtk_button_new ();
      g_signal_connect (btn_test, "clicked", G_CALLBACK (on_tool_test), GINT_TO_POINTER(slot));
      gtk_box_append(GTK_BOX(vboxleft),btn_test);
      img_test = gtk_image_new_from_file("Test.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_test),50);
      gtk_button_set_child(GTK_BUTTON(btn_test),img_test);
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_test), "Reload tool data");
  
      btn_exit = gtk_button_new ();
      g_signal_connect (btn_exit, "clicked", G_CALLBACK (on_tool_exit), win_tool);
      gtk_box_append(GTK_BOX(vboxleft),btn_exit);
      img_exit = gtk_image_new_from_file("Back.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
      gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_exit), "Exit");
    }

    // set up STATUS area - this is the area in the top portion of the window
    {
      col_width=35;
      
      sprintf(scratch,"Tool Information");
      tool_status_area=gtk_frame_new(scratch);
      gtk_box_append(GTK_BOX(statboxhi),tool_status_area);

      GtkWidget *tool_status_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(tool_status_area), tool_status_box);
      gtk_widget_set_size_request(tool_status_area,win_width,120);
      
      grid_status_area=gtk_grid_new();
      gtk_box_append(GTK_BOX(tool_status_box),grid_status_area);
      gtk_grid_set_row_spacing (GTK_GRID(grid_status_area),10);
      gtk_grid_set_column_spacing (GTK_GRID(grid_status_area),10);
      
      // display some options
      sprintf(scratch,"Status:  Unknown");
      if(Tool[slot].state==TL_UNKNOWN)sprintf(scratch,"  Status:  Tool not identified");
      if(Tool[slot].state==TL_LOADED)sprintf(scratch,"  Status:  Tool not defined");
      if(Tool[slot].state==TL_READY)sprintf(scratch,"  Status:  Tool ready");
      if(Tool[slot].state==TL_ACTIVE)sprintf(scratch,"  Status:  Tool active");
      if(Tool[slot].state==TL_RETRACTED)sprintf(scratch,"  Status:  Tool retracted");
      if(Tool[slot].state==TL_FAILED)
        {
	sprintf(scratch,"Status:  Tool has a problem");
	if(Tool[slot].epwr48==TRUE)strcat(scratch," with 48v power control.");
	if(Tool[slot].epwr24==TRUE)strcat(scratch," with 24v power control.");
	if(Tool[slot].espibus==TRUE)strcat(scratch," with SPI bus communication.");
	if(Tool[slot].ewire1==TRUE)strcat(scratch," with 1 wire communication.");
	if(Tool[slot].elimitsw==TRUE)strcat(scratch," with limit switch sensing.");
	if(Tool[slot].epwmtool==TRUE)strcat(scratch," with pulse width modulation");
	}
      lbl_tool_stat = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID(grid_status_area), lbl_tool_stat, 0, 0, 2, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_tool_stat),0.0);
  
      sprintf(scratch,"  Serial ID: Unknown");
      if(strlen(Tool[slot].mmry.dev)>7)sprintf(scratch,"  Serial ID: %s",Tool[slot].mmry.dev);
      lbl_tool_memID = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID(grid_status_area), lbl_tool_memID, 0, 1, 2, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_tool_memID),0.0);
  
      sprintf(scratch,"  Description:  ");
      if(strlen(Tool[slot].desc)>2)sprintf(scratch,"  Description:  %s ",Tool[slot].desc);
      lbl_tool_desc = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID(grid_status_area), lbl_tool_desc, 0, 2, 3, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_tool_desc),0.0);
    }  
      
    // set up TOOL TYPE area
    {
      // build the button box
      col_width=15;
      sprintf(scratch,"Tool Type");
      GtkWidget *tl_btn_frame=gtk_frame_new(scratch);
      gtk_box_append(GTK_BOX(toolbox),tl_btn_frame);

      tl_btn_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(tl_btn_frame), tl_btn_box);
      gtk_widget_set_size_request(tl_btn_box,(win_width/2),70);

      tl_btn_grd = gtk_grid_new();
      gtk_box_append(GTK_BOX(tl_btn_box),tl_btn_grd);
      gtk_grid_set_row_spacing (GTK_GRID(tl_btn_grd),10);
      gtk_grid_set_column_spacing (GTK_GRID(tl_btn_grd),10);
      
      lbl_spacer = gtk_label_new ("   ");
      gtk_grid_attach (GTK_GRID (tl_btn_grd), lbl_spacer, 0, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
      
      btn_tool_open = gtk_button_new_with_label ("Open");
      g_signal_connect (btn_tool_open, "clicked", G_CALLBACK (tool_open), GINT_TO_POINTER(slot));
      gtk_grid_attach (GTK_GRID (tl_btn_grd), btn_tool_open, 1, 1, 1, 1);		

      // build tool type combo box menu
      {
      combo_tool = gtk_combo_box_text_new();
      i=0;
      tptr=tool_list;							// load choices with index of found tools
      while(tptr!=NULL)
	{
	sprintf(scratch,"%s",tptr->name);
	while(strlen(scratch)<col_width)strcat(scratch," ");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_tool), NULL, scratch);
	if(strstr(Tool[slot].name,tptr->name)!=NULL && strlen(Tool[slot].name)==strlen(tptr->name))combo_active=i;
	tptr=tptr->next;
	i++;
	}
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_tool), combo_active);    
      gtk_grid_attach (GTK_GRID (tl_btn_grd), combo_tool, 2, 1, 1, 1);
      g_signal_connect (combo_tool, "changed", G_CALLBACK (tool_type), GINT_TO_POINTER(slot));

      btn_tool_save = gtk_button_new_with_label ("Save");
      g_signal_connect (btn_tool_save, "clicked", G_CALLBACK (tool_save), GINT_TO_POINTER(slot));
      gtk_grid_attach (GTK_GRID (tl_btn_grd), btn_tool_save, 3, 1, 1, 1);		
      }

      // build the thermometer
      // this frame is tool dependent - it must have some sort of temp sensor on it
      if(Tool[slot].state==TL_UNKNOWN || Tool[slot].thrm.sensor>NONE)
        {
        sprintf(scratch,"Tool Temperature");
        GtkWidget *tl_temp_frame=gtk_frame_new(scratch);
        gtk_box_append(GTK_BOX(toolbox),tl_temp_frame);
      
        tl_temp_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
        gtk_frame_set_child(GTK_FRAME(tl_temp_frame), tl_temp_box);
        gtk_widget_set_size_request(tl_temp_box,(win_width/2),95);
      
        tl_temp_grd = gtk_grid_new();
        gtk_box_append(GTK_BOX(tl_temp_box),tl_temp_grd);
        gtk_grid_set_row_spacing (GTK_GRID(tl_temp_grd),10);
        gtk_grid_set_column_spacing (GTK_GRID(tl_temp_grd),5);

        lbl_spacer = gtk_label_new ("   ");
        gtk_grid_attach (GTK_GRID (tl_temp_grd), lbl_spacer, 0, 1, 1, 1);		
        gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
      
        // create temperature graph for tool window
	q=Tool[slot].thrm.tempC;
	if(q<0)q=1.0;
	if(q>Tool[slot].thrm.maxtC)q=Tool[slot].thrm.maxtC;
        sprintf(scratch,"%6.0f C",q);
        if(Tool[slot].thrm.sensor==0)sprintf(scratch," -- C");
        while(strlen(scratch)<col_width)strcat(scratch," ");
        Ttemp_lbl[slot]=gtk_label_new(scratch);
        gtk_grid_attach (GTK_GRID(tl_temp_grd), Ttemp_lbl[slot], 1, 1, 1, 1);
      
        Ttemp_lvl[slot]=gtk_level_bar_new();
        gtk_widget_set_size_request(Ttemp_lvl[slot],150,15);
        Ttemp_bar[slot]=GTK_LEVEL_BAR(Ttemp_lvl[slot]);    
        gtk_level_bar_set_mode (Ttemp_bar[slot],GTK_LEVEL_BAR_MODE_CONTINUOUS);
        gtk_level_bar_set_min_value(Ttemp_bar[slot],0.0);
        gtk_level_bar_set_max_value(Ttemp_bar[slot],(Tool[slot].thrm.maxtC+10.0));
        gtk_level_bar_set_value (Ttemp_bar[slot],q);
        gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_LOW,(Tool[slot].thrm.setpC*0.1));
        gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_HIGH,(Tool[slot].thrm.setpC-Tool[slot].thrm.tolrC));
        gtk_level_bar_add_offset_value(Ttemp_bar[slot],GTK_LEVEL_BAR_OFFSET_FULL,(Tool[slot].thrm.setpC+Tool[slot].thrm.tolrC));
        gtk_grid_attach_next_to (GTK_GRID(tl_temp_grd),Ttemp_lvl[slot],Ttemp_lbl[slot],GTK_POS_RIGHT, 1, 1);

        fval=Tool[slot].thrm.setpC;
        temp_override_adj=gtk_adjustment_new(fval, 0, 500, 1, 1, 20);
        temp_override_btn=gtk_spin_button_new(temp_override_adj, 4, 0);
        gtk_grid_attach_next_to (GTK_GRID(tl_temp_grd),temp_override_btn,Ttemp_lvl[slot],GTK_POS_RIGHT, 1, 1);
        g_signal_connect(GTK_SPIN_BUTTON(temp_override_btn),"value-changed",G_CALLBACK(temp_override_value),GINT_TO_POINTER(slot));

        btn_lotemp = gtk_button_new_with_label ("Calibrate");
        gtk_grid_attach (GTK_GRID(tl_temp_grd), btn_lotemp, 1, 3, 1, 1);
        g_signal_connect (btn_lotemp, "clicked", G_CALLBACK (tool_temp_offset), GINT_TO_POINTER(slot));
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_lotemp), "Adjust temperature offset");

        btn_reset = gtk_button_new_with_label ("Reset");
        gtk_grid_attach (GTK_GRID(tl_temp_grd), btn_reset, 3, 3, 1, 1);
        g_signal_connect (btn_reset, "clicked", G_CALLBACK (tool_reset), GINT_TO_POINTER(slot));
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_reset), "Reset PWM if failed");
        }

      //sprintf(scratch,"_______________");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      //lbl_spacer = gtk_label_new (scratch);
      //gtk_grid_attach (GTK_GRID (grid_tool_area), lbl_spacer, 1, 5, 1, 1);		
      //gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);

      // build the tool info area
      /*
      {
      sprintf(scratch,"Tool Information");
      tl_set_box=gtk_frame_new(scratch);
      gtk_container_add (GTK_CONTAINER (toolbox), tl_set_box);
      gtk_widget_set_size_request(tl_set_box,(win_width/2),210);
      tl_set_grd = gtk_grid_new();
      gtk_container_add (GTK_CONTAINER (tl_set_box), tl_set_grd);
      gtk_grid_set_row_spacing (GTK_GRID(tl_set_grd),8);
      gtk_grid_set_column_spacing (GTK_GRID(tl_set_grd),5);

      sprintf(scratch,"Power control:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 0, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].pwr48 ? "On":"Off");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_power = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_power, 1, 0, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_power),0.1);
      
      sprintf(scratch,"Heater control:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].pwr24 ? "On":"Off");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_heater = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_heater, 1, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_heater),0.1);
      
      sprintf(scratch,"Stepper control:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      while(strlen(scratch)<col_width)strcat(scratch," ");
      sprintf(scratch,"%d",Tool[slot].step_max);
      lbl_stepper = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_stepper, 1, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_stepper),0.1);
      
      sprintf(scratch,"Temp sensor:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 3, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      if(Tool[slot].thrm.sensor==0)sprintf(scratch,"none");
      if(Tool[slot].thrm.sensor==1)sprintf(scratch,"1wire");
      if(Tool[slot].thrm.sensor==2)sprintf(scratch,"RTDH");
      if(Tool[slot].thrm.sensor==3)sprintf(scratch,"thermistor");
      if(Tool[slot].thrm.sensor==4)sprintf(scratch,"thrmocouple");
      lbl_tsense = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_tsense, 1, 3, 2, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_tsense),0.1);
      
      sprintf(scratch,"SPI enable:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 2, 0, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].spibus ? "On":"Off");
      lbl_SPI = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_SPI, 3, 0, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_SPI),0.1);
      
      sprintf(scratch,"1Wire enable:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 2, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].wire1 ? "On":"Off");
      lbl_1wire = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_1wire, 3, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_1wire),0.1);
      
      sprintf(scratch,"1Wire device:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 4, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"none");
      if(strlen(Tool[slot].mmry.dev)>1)sprintf(scratch,"%s",Tool[slot].mmry.dev);
      lbl_1wID = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_1wID, 1, 4, 2, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_1wID),0.1);
    
      sprintf(scratch,"Limit switch:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 2, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].limitsw ? "On":"Off");
      lbl_limitsw = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_limitsw, 3, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_limitsw),0.1);
      
      sprintf(scratch,"Weight:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_info, 0, 5, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%4.1f g",Tool[slot].weight);
      lbl_weight = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_set_grd), lbl_weight, 1, 5, 2, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_weight),0.0);
      }
      */
      
      // build the tool tip area
      {
      sprintf(scratch,"Tool Tip Location");
      GtkWidget *tl_tip_frame=gtk_frame_new(scratch);
      gtk_box_append(GTK_BOX(toolbox),tl_tip_frame);
      
      tl_tip_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(tl_tip_frame), tl_tip_box);
      gtk_widget_set_size_request(tl_tip_box,(win_width/2),150);
      
      tl_tip_grd = gtk_grid_new();
      gtk_box_append(GTK_BOX(tl_tip_box),tl_tip_grd);
      gtk_grid_set_row_spacing (GTK_GRID(tl_tip_grd),2);
      gtk_grid_set_column_spacing (GTK_GRID(tl_tip_grd),5);

      lbl_spacer = gtk_label_new ("   ");
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_spacer, 0, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
      
      sprintf(scratch,"     Carriage    Tool    Fab");
      lbl_info=gtk_label_new(scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_info, 1, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),0.5);
      
      sprintf(scratch,"  X:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_info, 0, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].x_offset,Tool[slot].x_offset,Tool[slot].tip_x_fab);
      lbl_tipx = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_tipx, 1, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_tipx),0.0);
      
      sprintf(scratch,"  Y:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_info, 0, 3, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].y_offset,Tool[slot].y_offset,Tool[slot].tip_y_fab);
      lbl_tipy = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_tipy, 1, 3, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_tipy),0.0);
      
      sprintf(scratch,"  Z:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_info, 0, 4, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"      %6.3f     %6.3f      %6.3f",crgslot[slot].z_offset,Tool[slot].tip_dn_pos,Tool[slot].tip_z_fab);
      lbl_tipzdn = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_tipzdn, 1, 4, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_tipzdn),0.0);
      
      lbl_spacer = gtk_label_new ("   ");
      gtk_grid_attach (GTK_GRID (tl_tip_grd), lbl_spacer, 5, 1, 2, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
      
      btn_tipset = gtk_button_new_with_label ("Adjust");
      gtk_grid_attach (GTK_GRID (tl_tip_grd), btn_tipset, 7, 1, 1, 1);		
      g_signal_connect (btn_tipset, "clicked", G_CALLBACK (xyztip_manualcal_callback), GINT_TO_POINTER(slot));
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tipset), "Adjust tip to table");
      
      GtkWidget *btn_tipclear;
      btn_tipclear = gtk_button_new_with_label ("Clear");
      gtk_grid_attach (GTK_GRID (tl_tip_grd), btn_tipclear, 7, 3, 1, 1);		
      g_signal_connect (btn_tipclear, "clicked", G_CALLBACK (xyztip_clear_callback), GINT_TO_POINTER(slot));
      gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tipclear), "Clear tip values");
      
      // alignment options for router tip relative to part
      if(Tool[slot].tool_ID==TC_ROUTER)
	{
	btn_tipcenter = gtk_button_new_with_label ("Center");
	gtk_grid_attach (GTK_GRID (tl_tip_grd), btn_tipcenter, 7, 2, 1, 1);		
	g_signal_connect (btn_tipcenter, "clicked", G_CALLBACK (xyztip_manualcal_callback), GINT_TO_POINTER(slot));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tipcenter), "Center tip on part");
    
	btn_tip2pts = gtk_button_new_with_label ("Points");
	gtk_grid_attach (GTK_GRID (tl_tip_grd), btn_tip2pts, 7, 3, 1, 1);		
	g_signal_connect (btn_tip2pts, "clicked", G_CALLBACK (xyztip_manualcal_callback), GINT_TO_POINTER(slot));
	gtk_widget_set_tooltip_text (GTK_WIDGET (btn_tip2pts), "Align tip on part");
	}

      }
      
    } 
  
    // set up MATERIAL TYPE area
    {
      // build the button box
      col_width=15;
      sprintf(scratch,"Material Type");
      GtkWidget *mt_btn_frame=gtk_frame_new(scratch);
      gtk_box_append(GTK_BOX(matlbox),mt_btn_frame);
      
      mt_btn_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(mt_btn_frame), mt_btn_box);
      gtk_widget_set_size_request(mt_btn_box,(win_width/2),70);
      
      mt_btn_grd = gtk_grid_new();
      gtk_box_append(GTK_BOX(mt_btn_box),mt_btn_grd);
      gtk_grid_set_row_spacing (GTK_GRID(mt_btn_grd),10);
      gtk_grid_set_column_spacing (GTK_GRID(mt_btn_grd),10);
      
      lbl_spacer = gtk_label_new ("   ");
      gtk_grid_attach (GTK_GRID (mt_btn_grd), lbl_spacer, 0, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
      
      btn_matl_open = gtk_button_new_with_label ("Open");
      g_signal_connect (btn_matl_open, "clicked", G_CALLBACK (tool_open), GINT_TO_POINTER(slot));
      gtk_grid_attach (GTK_GRID (mt_btn_grd), btn_matl_open, 1, 1, 1, 1);		

      // material drop down menu
      combo_mats = gtk_combo_box_text_new();
      sprintf(scratch,"UNKNOWN");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mats), NULL, scratch); // add UNKNOWN at 0 to allow user to "undefine" matl
      build_material_index(slot);					// build index of materials available for this tool
      i=0;								// init matl index counter to unknown
      aptr=Tool[slot].mats_list;					// load choices with index of found tools
      while(aptr!=NULL)
	{
	i++;
	sprintf(scratch,"%s",aptr->name);
	while(strlen(scratch)<col_width)strcat(scratch," ");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mats), NULL, scratch);  // add found matls starting with id 1
	if(strstr(aptr->name,Tool[slot].matl.name)!=NULL)Tool[slot].matl.ID=i;
	aptr=aptr->next;
	}
      //aptr=Tool[slot].mats_list;					// load choices with index of found tools
      //while(aptr!=NULL)
	//{
	//if(strstr(aptr->name,Tool[slot].mmry.md.matp)!=NULL)break;
	//aptr=aptr->next;
	//}
      //if(aptr!=NULL){gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats),aptr->ID);} 
      //else {gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats), 0);}    
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mats),Tool[slot].matl.ID); 
      gtk_grid_attach (GTK_GRID (mt_btn_grd), combo_mats, 2, 1, 1, 1);
      g_signal_connect (combo_mats, "changed", G_CALLBACK (mats_type), GINT_TO_POINTER(slot));

      //GtkEventController *mats_motion_ctlr;
      //mats_motion_ctlr = gtk_event_controller_motion_new();
      //gtk_widget_add_controller(combo_mats, GTK_EVENT_CONTROLLER(mats_motion_ctlr));
      //g_signal_connect(mats_motion_ctlr, "button-pressed", G_CALLBACK(on_combo_mats_button_press_event), GINT_TO_POINTER(slot));

      btn_matl_save = gtk_button_new_with_label ("Print");
      g_signal_connect (btn_matl_save, "clicked", G_CALLBACK (build_mat_file), GINT_TO_POINTER(slot));
      //g_signal_connect (btn_matl_save, "clicked", G_CALLBACK (tool_save), GINT_TO_POINTER(slot));
      gtk_grid_attach (GTK_GRID (mt_btn_grd), btn_matl_save, 3, 1, 1, 1);		
      

      // create materiallevel / duty cycle graph for tool window.  each tool type has a custom use for this section
      // depending on what the tool does.
      {
      if(Tool[slot].type==UNDEFINED || Tool[slot].type==ADDITIVE)
	{
	// set up frame
	sprintf(scratch," Material Level ");
	GtkWidget *mt_amnt_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(matlbox),mt_amnt_frame);
	
	mt_amnt_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(mt_amnt_frame), mt_amnt_box);
	gtk_widget_set_size_request(mt_amnt_box,(win_width/2),95);
	
	mt_amnt_grd = gtk_grid_new();
	gtk_box_append(GTK_BOX(mt_amnt_box),mt_amnt_grd);
	gtk_grid_set_row_spacing (GTK_GRID(mt_amnt_grd),10);
	gtk_grid_set_column_spacing (GTK_GRID(mt_amnt_grd),5);
	
	lbl_spacer = gtk_label_new ("   ");
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), lbl_spacer, 0, 1, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
  
	// set up left graph label
	sprintf(scratch," -- \%");
	if(strlen(Tool[slot].matl.name)>0)sprintf(scratch," %4.0f%% ",(100-(100*Tool[slot].matl.mat_used/Tool[slot].matl.mat_volume)));
	while(strlen(scratch)<col_width)strcat(scratch," ");
	Tmatl_lbl[slot]=gtk_label_new(scratch);
	gtk_grid_attach (GTK_GRID(mt_amnt_grd),Tmatl_lbl[slot], 1, 1, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(Tmatl_lbl[slot]),1.0);
	
	// set up bar graph in middle
	Tmatl_lvl[slot]=gtk_level_bar_new();
	gtk_widget_set_size_request(Tmatl_lvl[slot],150,15);
	Tmatl_bar[slot]=GTK_LEVEL_BAR(Tmatl_lvl[slot]);    
	gtk_level_bar_set_mode (Tmatl_bar[slot],GTK_LEVEL_BAR_MODE_CONTINUOUS);
	gtk_level_bar_set_min_value(Tmatl_bar[slot],0.0);
	gtk_level_bar_set_max_value(Tmatl_bar[slot],100.0);
	gtk_level_bar_set_value (Tmatl_bar[slot],1.0);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_LOW,10.0);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_HIGH,25.0);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_FULL,100.0);
	gtk_grid_attach_next_to (GTK_GRID(mt_amnt_grd),Tmatl_lvl[slot],Tmatl_lbl[slot],GTK_POS_RIGHT, 1, 1);
	
	// set up right graph label
	//sprintf(scratch," ");
	//if(Tool[slot].tool_ID==TC_EXTRUDER)sprintf(scratch,"%4.1f mL",Tool[slot].matl.mat_volume);
	//if(Tool[slot].tool_ID==TC_FDM)sprintf(scratch,"%4.1f kg",Tool[slot].matl.mat_volume/330000);
	//while(strlen(scratch)<col_width)strcat(scratch," ");
	//Tmlvl_lbl[slot]=gtk_label_new(scratch);
	//gtk_grid_attach_next_to (GTK_GRID(mt_amnt_grd),Tmlvl_lbl[slot],Tmatl_lvl[slot],GTK_POS_RIGHT, 1, 1);
	
	sprintf(scratch," ");
	if(Tool[slot].tool_ID==TC_EXTRUDER)sprintf(scratch,"%3.1f",Tool[slot].matl.mat_volume);
	if(Tool[slot].tool_ID==TC_FDM)sprintf(scratch,"%3.1f",Tool[slot].matl.mat_volume/330000);
	matl_override_adj=gtk_adjustment_new(Tool[slot].matl.mat_volume, 1, 100, 1, 1, 20);
	matl_override_btn=gtk_spin_button_new(matl_override_adj, 4, 0);
	gtk_grid_attach_next_to (GTK_GRID(mt_amnt_grd),matl_override_btn,Tmatl_lvl[slot],GTK_POS_RIGHT, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(matl_override_btn),"value-changed",G_CALLBACK(matl_override_value),GINT_TO_POINTER(slot));

	// material color button
	btn_color=gtk_color_button_new_with_rgba(&mcolor[slot]);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(btn_color), &mcolor[slot]);
	gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(btn_color),TRUE);
	g_signal_connect (btn_color, "color-set", G_CALLBACK (matl_set_color), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), btn_color, 1, 3, 1, 1);		
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_color), "Set material color");

	// material color button - GTK 4.10
	//GtkColorDialog *matl_color_dialog;
	//gtk_color_dialog_choose_rgba(matl_color_dialog, GTK_WINDOW(win_tool), &mcolor[slot], NULL, G_CALLBACK(matl_set_color), GINT_TO_POINTER(slot));
	//btn_color=gtk_color_dialog_button_new(matl_color_dialog);
	//gtk_grid_attach (GTK_GRID (mt_amnt_grd), btn_color, 1, 3, 1, 1);		
        //gtk_widget_set_tooltip_text (GTK_WIDGET (btn_color), "Set material color");


	// material level button
	btn_setamt = gtk_button_new_with_label ("Set Level");
	g_signal_connect (btn_setamt, "clicked", G_CALLBACK (matl_set_amount), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), btn_setamt, 3, 3, 1, 1);	
        gtk_widget_set_tooltip_text (GTK_WIDGET (btn_setamt), "Set material level");
	}
	
      if(Tool[slot].type!=ADDITIVE && Tool[slot].pwr48==TRUE)
	{
	// set up frame
	sprintf(scratch,"Duty Cycle");
	GtkWidget *mt_amnt_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(matlbox),mt_amnt_frame);
	
	mt_amnt_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(mt_amnt_frame), mt_amnt_box);
	gtk_widget_set_size_request(mt_amnt_box,(win_width/2),95);
	
	mt_amnt_grd = gtk_grid_new();
	gtk_box_append(GTK_BOX(mt_amnt_box),mt_amnt_grd);
	gtk_grid_set_row_spacing (GTK_GRID(mt_amnt_grd),10);
	gtk_grid_set_column_spacing (GTK_GRID(mt_amnt_grd),5);
	
	lbl_spacer = gtk_label_new ("   ");
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), lbl_spacer, 0, 1, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(lbl_spacer),0.5);
	
	// set up left graph label
	sprintf(scratch," -- \%");
	//if(strlen(Tool[slot].matl.name)>0)sprintf(scratch," %4.0f%% ",(100*Tool[slot].powr.power_duty));
	if(strlen(Tool[slot].matl.name)>0)
	  {
	  lptr=linetype_find(slot,MDL_BORDER);				// use border in this case
	  fval=0.0;
	  if(lptr!=NULL)fval=100*lptr->flowrate;
	  sprintf(scratch," %4.0f%% ",fval);
	  }
	while(strlen(scratch)<col_width)strcat(scratch," ");
	Tmatl_lbl[slot]=gtk_label_new(scratch);
	gtk_grid_attach (GTK_GRID(mt_amnt_grd),Tmatl_lbl[slot], 1, 1, 1, 1);
	gtk_label_set_xalign(GTK_LABEL(Tmatl_lbl[slot]),1.0);

	// set up bar graph in middle
	Tmatl_lvl[slot]=gtk_level_bar_new();
	gtk_widget_set_size_request(Tmatl_lvl[slot],150,15);
	Tmatl_bar[slot]=GTK_LEVEL_BAR(Tmatl_lvl[slot]);    
	gtk_level_bar_set_mode (Tmatl_bar[slot],GTK_LEVEL_BAR_MODE_CONTINUOUS);
	gtk_level_bar_set_min_value(Tmatl_bar[slot],0.0);
	gtk_level_bar_set_max_value(Tmatl_bar[slot],100.0);
	gtk_level_bar_set_value (Tmatl_bar[slot],fval);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_LOW,10.0);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_HIGH,25.0);
	gtk_level_bar_add_offset_value(Tmatl_bar[slot],GTK_LEVEL_BAR_OFFSET_FULL,100.0);
	gtk_grid_attach_next_to (GTK_GRID(mt_amnt_grd),Tmatl_lvl[slot],Tmatl_lbl[slot],GTK_POS_RIGHT, 1, 1);

	// set up right graph label
	sprintf(scratch,"100%%");
	while(strlen(scratch)<col_width)strcat(scratch," ");
	Tmlvl_lbl[slot]=gtk_label_new(scratch);
	gtk_grid_attach_next_to (GTK_GRID(mt_amnt_grd),Tmlvl_lbl[slot],Tmatl_lvl[slot],GTK_POS_RIGHT, 1, 1);
	
	// low duty cycle calibration button
	btn_loduty = gtk_button_new_with_label ("Low Cal");
	g_signal_connect (btn_loduty, "clicked", G_CALLBACK (tool_low_duty_calib), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), btn_loduty, 1, 3, 1, 1);	

	// high duty cycle calibration button
	btn_hiduty = gtk_button_new_with_label ("High Cal");
	g_signal_connect (btn_hiduty, "clicked", G_CALLBACK (tool_high_duty_calib), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (mt_amnt_grd), btn_hiduty, 3, 3, 1, 1);	
	}

      }

      // build the material info area
      sprintf(scratch,"Material Information");
      GtkWidget *mt_set_frame=gtk_frame_new(scratch);
      gtk_box_append(GTK_BOX(matlbox),mt_set_frame);
      
      mt_set_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(mt_set_frame),mt_set_box);
      gtk_widget_set_size_request(mt_set_box,(win_width/2),150);

      mt_set_grd = gtk_grid_new();
      gtk_box_append(GTK_BOX(mt_set_box),mt_set_grd);
      gtk_grid_set_row_spacing (GTK_GRID(mt_set_grd),10);
      gtk_grid_set_column_spacing (GTK_GRID(mt_set_grd),5);

      col_width=15;
      sprintf(scratch," Description:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_info, 0, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].matl.description);
      if(strlen(scratch)>40)scratch[40]=0;
      lbl_mat_description = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_mat_description, 1, 1, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_mat_description),0.1);
      
      sprintf(scratch," Brand......:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_info, 0, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%s",Tool[slot].matl.brand);
      if(strlen(scratch)>40)scratch[40]=0;
      lbl_mat_brand = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_mat_brand, 1, 2, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_mat_brand),0.1);
      
      sprintf(scratch," Input diam.:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_info, 0, 3, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%6.3f",Tool[slot].matl.diam);
      lbl_tipdiam = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_tipdiam, 1, 3, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_tipdiam),0.1);
      
      sprintf(scratch,"  Layer height..:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_info, 0, 4, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%6.3f",Tool[slot].matl.layer_height);
      lbl_layerht = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_layerht, 1, 4, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_layerht),0.1);
    
      sprintf(scratch,"  Temperature...:");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_info, 0, 5, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%6.1f",Tool[slot].thrm.setpC);
      lbl_temper = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (mt_set_grd), lbl_temper, 1, 5, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(lbl_temper),0.1);
    
/*
      // set up line type drop down combo menu
      lbl_info = gtk_label_new ("Line Type:");
      gtk_grid_attach (GTK_GRID (grid_matl_area), lbl_info, 0, 9, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      combo_lts = gtk_combo_box_text_new();
      lptr=Tool[slot].matl.lt;
      while(lptr!=NULL)
	{
	sprintf(scratch,"%s",lptr->name);				// get name of line type at this position
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_lts), NULL, scratch);
	lptr=lptr->next;
	}
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_lts), 1);    
      gtk_grid_attach (GTK_GRID (grid_matl_area), combo_lts, 1, 9, 1, 1);
      g_signal_connect (combo_mats, "changed", G_CALLBACK (lts_type), &Tool[slot]);
*/    

     }
/*
    // set up SETTINGS notebook page ----------------------------------------------------------
    {
      sprintf(scratch,"X tip offset:");
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 0, 11, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),1.0);
      if(slot==0 || slot==2)fval=Tool[slot].x_offset-crgslot[slot].x_center;
      if(slot==1 || slot==3)fval=crgslot[slot].x_center-Tool[slot].x_offset;
      sprintf(scratch,"%7.3f",fval);
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 1, 11, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),0.0);
      
      sprintf(scratch,"Y tip offset:");
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 0, 12, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),1.0);
      if(slot==0 || slot==1)fval=Tool[slot].y_offset-crgslot[slot].y_center;
      if(slot==2 || slot==3)fval=Tool[slot].y_offset-crgslot[slot].y_center;
      sprintf(scratch,"%7.3f",fval);
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 1, 12, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),0.0);
      
      sprintf(scratch,"Z tip offset:");
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 0, 13, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),1.0);
      sprintf(scratch,"%7.3f",Tool[slot].tip_dn_pos);
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 1, 13, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(label),0.0);
    
      ztip_test_btn=gtk_button_new_with_label("Adj\nTip\nXYZ");
      gtk_grid_attach (GTK_GRID (grid_settings_area), ztip_test_btn, 2, 11, 1, 3);
      g_signal_connect (ztip_test_btn, "clicked", G_CALLBACK (xyztip_manualcal_callback), GINT_TO_POINTER(slot));
     
      // put a blank space in column 2 to force better spacing
      sprintf(scratch,"            ");
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 2, 0, 1, 1);		
      gtk_label_set_xalign (GTK_LABEL(label),0.0);
    
      // display material properties if available --------------------------
      
      sprintf(scratch,"MATERIAL...:");
      label = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_settings_area), label, 3, 0, 1, 1);		
    
      
      
      // THIS SECTION OF CODE OUGHT TO BE MOVED UNDER INFO ON THE MODEL UI SCREEN ---------------------------------------
      
      mptr=job.model_first;
      if(mptr!=NULL)
        {
	sprintf(scratch,"MODEL......:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 0, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%s",mptr->model_file);
	if(strlen(scratch)>12)
	  {
	  strcpy(mdl_name,&scratch[strlen(scratch)-12]);
	  sprintf(scratch,"...");
	  strcat(scratch,mdl_name);
	  }
	while(strlen(scratch)<15){strcat(scratch," ");}
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 0, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
      
	sprintf(scratch,"Vertex count.:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 1, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%ld",mptr->vertex_qty[MODEL]);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 1, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
	
	sprintf(scratch,"Edge count....:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 2, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%ld",mptr->edge_qty[MODEL]);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 2, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
      
	sprintf(scratch,"Facet count...:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 3, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%ld",mptr->facet_qty[MODEL]);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 3, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
      
	sprintf(scratch,"X Min/Max.....:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 4, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%6.3f/%6.3f",mptr->xmin,mptr->xmax);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 4, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
      
	sprintf(scratch,"Y Min/Max.....:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 5, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%6.3f/%6.3f",mptr->ymin,mptr->ymax);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 5, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
      
	sprintf(scratch,"Z Min/Max.....:");
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 5, 6, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),1.0);
	sprintf(scratch,"%6.3f/%6.3f",mptr->zmin,mptr->zmax);
	label = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grid_settings_area), label, 6, 6, 1, 1);		
	gtk_label_set_xalign (GTK_LABEL(label),0.1);
	}
    }
*/

    win_tool_flag[slot]=TRUE;
    gtk_widget_set_visible(win_tool,TRUE);
    
    //if(Tool[slot].state>TL_EMPTY)make_toolchange(slot,1);
    
    return(TRUE);
}



G_GNUC_END_IGNORE_DEPRECATIONS
