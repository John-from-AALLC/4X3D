#include "Global.h"

// Function to run rules check before slicing and/or building job
int job_rules_check(void)
{
  int		i,j,slot,layers_id,orient,mtyp=MODEL;
  float 	amt_done;
  float 	sptvol_min,layers_min;
  model		*mptr;
  
  if(smart_check==FALSE)return(0);
  
  // show the progress bar
  sprintf(scratch,"  Optimizing Orientation...  ");
  gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
  sprintf(scratch," ");
  gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
  gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
  gtk_widget_queue_draw(win_info);
  gtk_widget_set_visible(win_info,TRUE);

  // check for conflicting operations
  // violations:  1) overhangs of model without using any support
  //              2) milling material deposited by extruder
  
  // check sequence of operations
  // best sequence:  base lyrs, support, model
  
  // check orientation
  // best orientation:  least layers, least support
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    // update status to user
    amt_done = (double)(mptr->model_ID)/(double)(job.model_count);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
    while (g_main_context_iteration(NULL, FALSE));

    slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);
    if(slot<0 || slot>=MAX_TOOLS){mptr=mptr->next;continue;}
    mptr->slice_thick=Tool[slot].matl.layer_height;
    
    // get data on given orientation
    model_maxmin(mptr);				
    model_support_estimate(mptr);
    
    // calculate layer requirements for each orientation
    mptr->lyrs[0]=(mptr->zmax[MODEL]-mptr->zmin[MODEL])/mptr->slice_thick;
    mptr->lyrs[1]=(mptr->ymax[MODEL]-mptr->ymin[MODEL])/mptr->slice_thick;
    mptr->lyrs[2]=(mptr->xmax[MODEL]-mptr->xmin[MODEL])/mptr->slice_thick;

    // find orientation with minimal layers
    layers_id=0;
    layers_min=mptr->lyrs[layers_id];
    for(i=1;i<3;i++){if(mptr->lyrs[i]<layers_min)layers_id=i;}

    // find direction (up/down) with minimal support
    orient=layers_id;
    if(mptr->svol[layers_id]>mptr->svol[layers_id+3])orient=layers_id+3;
    
    printf("\nLeast Layers = %d   Least Support = %d\n",layers_id,orient);
    for(i=0;i<6;i++)
      {
      j=i;
      if(j>2)j-=3;
      if(i==0)printf(" %d  lyrs=%6.1f  svol=%6.1f  no rotation\n",i,mptr->lyrs[j],mptr->svol[i]);
      if(i==1)printf(" %d  lyrs=%6.1f  svol=%6.1f  90 y\n",i,mptr->lyrs[j],mptr->svol[i]);
      if(i==2)printf(" %d  lyrs=%6.1f  svol=%6.1f  90 x\n",i,mptr->lyrs[j],mptr->svol[i]);
      if(i==3)printf(" %d  lyrs=%6.1f  svol=%6.1f  180 x\n",i,mptr->lyrs[j],mptr->svol[i]);
      if(i==4)printf(" %d  lyrs=%6.1f  svol=%6.1f  -90 y\n",i,mptr->lyrs[j],mptr->svol[i]);
      if(i==5)printf(" %d  lyrs=%6.1f  svol=%6.1f  -90 x\n",i,mptr->lyrs[j],mptr->svol[i]);
      }

    // finally, rotate the model from original (orient=0) into optimize orientation
    if(orient==1){mptr->yrot[mtyp]=90*PI/180;}
    if(orient==2){mptr->xrot[mtyp]=90*PI/180;}
    if(orient==3){mptr->xrot[mtyp]=180*PI/180;}
    if(orient==4){mptr->yrot[mtyp]=-90*PI/180;}
    if(orient==5){mptr->xrot[mtyp]=-90*PI/180;}
    model_rotate(mptr);
    model_maxmin(mptr);
    mptr->xrot[mtyp]=0;mptr->yrot[mtyp]=0;
      
    mptr=mptr->next;
    }
  
  // check for overlapping models
  // violations:  1) if the bounding box overlaps any other bounding box
  
  // check for material availablility
  // violations:  1) if model calls for more than what is currently loaded
  
  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  return(1);
}

// function to calculate the time to build a job based on slice statistics.  it is much
// faster than the time simulator, but also less accurate.
float job_time_estimate_calculator(void)
{
  int		i,h,hfld,slot;
  float 	pre_tim_est=0.0,tim_est=0.0;
  float 	accel_rate,accel_time,accel_dist,velocity;
  float 	move_delay;
  float 	ltype_time=0.0,layer_time=0.0;
  slice 	*sptr;
  model		*mptr;
  genericlist	*aptr;
  linetype 	*lptr;
  
  accel_rate=9810.0;							// 1.0G = 9810 mm/s^2 avg accel (aproximating jerk)
  velocity=125.0;							// default move rate 7500 mm/min = 125 mm/s
  move_delay=0.050;							// processing dely bt every move (secs) 0.0457

  // clear history if any already existing
  if(hist_ctr[HIST_TIME]>0)
    {
    for(i=0;i<hist_ctr[HIST_TIME];i++)
      {
      for(h=H_MOD_TIM_EST;h<=H_OHD_TIM_ACT;h++){history[h][i]=0;}
      }
    hist_ctr[HIST_TIME]=0;
    }

  mptr=job.model_first;
  while(mptr!=NULL)
    {
    aptr=mptr->oper_list;						// get the pointer to start of operations list for this model
    while(aptr!=NULL)							// loop thru all operations in sequence
      {
      slot=aptr->ID;
      if(slot<0 || slot>=MAX_TOOLS)continue;				// skip if tool out of range
      sptr=mptr->slice_first[slot];
      while(sptr!=NULL)
	{
	layer_time=0.0;
	for(i=0;i<MAX_LINE_TYPES;i++)
	  {
	  ltype_time=0.0;
	  if(sptr->pqty[i]<=0)continue;
	  lptr=linetype_find(slot,i);					// get pointer to line type data
	  if(lptr==NULL)continue;
	  if(lptr->feedrate>0)velocity=lptr->feedrate/60.0;		// get pen-down feedrate in mm/s
	  accel_time = velocity/accel_rate;				// time for accel ramp
	  accel_dist = 0.5*accel_rate*(accel_time*accel_time);		// dist needed to reach velocity at this accel
	  ltype_time += sptr->pqty[i]*accel_time*2;				// time to accel and de-accel
	  ltype_time += (sptr->pdist[i]-(sptr->pqty[i]*2*accel_dist))/velocity;	// time at constant velocity
	  ltype_time += sptr->pqty[i]*move_delay;
	  sptr->ptime[i] = ltype_time*mptr->total_copies;
	  layer_time += ltype_time;

	  hfld=H_OHD_TIM_EST;
	  if(i>=MDL_PERIM && i<=MDL_LAYER_1)hfld=H_MOD_TIM_EST;
	  if(i>=SPT_PERIM && i<=SPT_LAYER_1)hfld=H_SUP_TIM_EST;
	  history[hfld][hist_ctr[HIST_TIME]] = ltype_time;		// save history values
	  hist_tool[hfld]=slot;
	  }
	sptr->time_estimate=layer_time;
	tim_est += layer_time;
	if(hist_ctr[HIST_TIME]<(MAX_HIST_COUNT-1))hist_ctr[HIST_TIME]++;
	sptr=sptr->next;
	}
      aptr=aptr->next;
      }
    mptr=mptr->next;
    }
    
  job.time_estimate=tim_est;						// time to build whole job
  return tim_est;
}

// Function to determin the build time of a job.  this function basically simulates the print function
// but with math calculations in place of mech movements.  In addition to time, it calculates the exact
// amount of material required for the job, and generates G-Code for the print.
// units are:  length=mm  time=sec
float job_time_estimate_simulator(void)
{
  int 		h,i,ptyp,slot,old_slot,opr,hfld,all_ok,mtyp=MODEL;
  int		x_copy_ctr,y_copy_ctr;
  int		vector_count,vtx_overlap;
  long int	move_counter=0;
  float 	amt_done,old_z;
  float 	tim_est,tvec,pre_tim_est,mat_est,pre_mat_est;
  float 	move_delay,tool_change_delay,layer_inc_time;
  float 	dx,dy,dist;
  float 	x0,y0,z0,x1,y1,z1,z_stop;
  float 	accel,moveveloc,feedveloc,da,ta;
  float 	original_xoff,original_yoff;
  float 	fil_to_ext_ratio=4.12;					// 1.75mm filament extruded at 0.425 mm bead
  vertex 	*vptr,*vnxt,*vpre,*vorigin;
  polygon 	*pptr;
  slice		*sptr,*sold;
  model		*mptr;
  genericlist	*aptr,*lt_ptr;
  operation	*optr;
  linetype	*lptr;

  printf("\n\nJTS:  entry \n");

  // initialize
  pre_tim_est=0.0;
  tim_est=0.0;								// init our estimate return value
  pre_mat_est=0.0;
  mat_est=0.0;
  job.time_estimate=0.0;						// init our job time estimate
  job.penup_dist=0.0;							// init our job non-deposit move distance
  job.pendown_dist=0.0;							// init out job deposit move distance
  job.total_dist=0.0;							// init our job total distance traveled
  accel=19620.0;							// 1.0G = 9810 mm/s^2 avg accel (aproximating jerk)
  move_delay=0.010;							// processing dely bt every move (secs)
  tool_change_delay=15.0;						// time to switch tools (secs)
  layer_inc_time=0.50;							// time to move in z (secs)
  feedveloc=83.0;							// default move rate 5000 mm/min = 83 mm/s
  moveveloc=83.0;							// default move rate 5000 mm/min = 83 mm/s
    
  // clear history if any already existing
  if(hist_ctr[HIST_TIME]>0)						// clear time estimates and actuals
    {
    for(i=0;i<hist_ctr[HIST_TIME];i++)
      {
      for(h=H_MOD_TIM_EST;h<=H_OHD_TIM_ACT;h++){history[h][i]=0;}
      }
    hist_ctr[HIST_TIME]=0;
    }
  if(hist_ctr[HIST_MATL]>0)						// clear material estimates and actuals
    {
    for(i=0;i<hist_ctr[HIST_MATL];i++)
      {
      for(h=H_MOD_MAT_EST;h<=H_SUP_MAT_ACT;h++){history[h][i]=0;}
      }
    hist_ctr[HIST_MATL]=0;
    }

  // verify that we have something to work with
  all_ok=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mptr==NULL){all_ok=FALSE; break;}
    if(mptr->input_type==GERBER){all_ok=FALSE; break;}
    if(mptr->reslice_flag==TRUE){all_ok=FALSE; break;}
    if(mptr->oper_list==NULL){all_ok=FALSE; break;}
    mptr=mptr->next;
    }
  if(all_ok==FALSE){printf("\nJob Time Est Sim: insufficient data - aborted.\n\n"); return(tim_est);}
  mptr=job.model_first;

  // show the progress bar
  sprintf(scratch,"        Generating time estimate for this job...  ");
  gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
  sprintf(scratch," ");
  gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
  gtk_window_set_transient_for (GTK_WINDOW(win_info), GTK_WINDOW(win_main));
  gtk_widget_set_visible(win_info,TRUE);
  gtk_widget_queue_draw(win_info);

  // determine the slower of x and y and use slowest for move velocity
  if(x_move_velocity<y_move_velocity && x_move_velocity>0)moveveloc=x_move_velocity;
  if(y_move_velocity<x_move_velocity && y_move_velocity>0)moveveloc=y_move_velocity;

  // set start/stop heights based on job type
  slot=(-1); old_slot=slot;
  z_stop=ZMax;
  if(job.type==ADDITIVE)job.current_z=0.0;
  if(job.type==SUBTRACTIVE)job.current_z=cnc_z_height;
  if(job.type==MARKING)job.current_z=z_cut;
  if(set_start_at_crt_z==TRUE)job.current_z=z_cut;
  mptr=job.model_first;
  while(mptr!=NULL)							// init each model's current z
    {
    // set this model's current z to bottom (i.e. not started) or currently selected z
    if(job.type==ADDITIVE)mptr->current_z=0.0;
    if(job.type==SUBTRACTIVE)
      {
      mptr->current_z=mptr->zmax[mtyp];
      if(mptr->target_z_stop<z_stop)z_stop=mptr->target_z_stop;
      if(set_start_at_crt_z==TRUE)mptr->current_z=z_cut;
      }
    if(job.type==MARKING)
      {
      //mptr->current_z=mptr->zmax[mtyp];
      //if(mptr->target_z_stop<z_stop)z_stop=mptr->target_z_stop;
      mptr->current_z=0.0;
      if(set_start_at_crt_z==TRUE)mptr->current_z=mptr->zmax[TARGET]-mptr->zoff[MODEL];		// set z of model to current z_cut/zoff      
      }
    mptr->time_estimate=0;						// reset to 0 since about to recalculate
    mptr->matl_estimate=0;
    mptr=mptr->next;
    }
  if(z_stop<job.min_slice_thk)z_stop=job.min_slice_thk;

  // find the model containing the thinnest slice in contact with the build surface (table if at z=0, model if at z=current)
  // search through the models looking at which tools they call out, and then check the specified tool layer thicknesses
  if(job.type==ADDITIVE)
    {
    if(set_start_at_crt_z==FALSE)z_cut=job.min_slice_thk/2;		// set to something above 0 and below min slice thk
    mptr=job_layer_seek(UP);						// sets z_cut to new value
    slot=(-1);
    aptr=mptr->oper_list;
    while(aptr!=NULL)
      {
      if(strstr(aptr->name,"ADD_MODEL_MATERIAL")!=NULL){slot=aptr->ID;break;}
      aptr=aptr->next;
      }
    }
  if(job.type==SUBTRACTIVE)
    {
    mptr=job.model_first;
    mptr->current_z=mptr->zmax[mtyp];
    if(set_start_at_crt_z==FALSE)z_cut=mptr->zmax[mtyp]-mptr->slice_thick;
    slot=(-1);
    aptr=mptr->oper_list;
    while(aptr!=NULL)
      {
      if(strstr(aptr->name,"MILL")!=NULL){slot=aptr->ID;break;}
      aptr=aptr->next;
      }
    }
  if(job.type==MARKING)
    {
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models
      {
      if(set_start_at_crt_z==FALSE)					// if starting on the build table...
	{
	z_cut=mptr->zmax[TARGET];					// ... set to top surface of target (i.e. bottom of model that sits on the target)
	mptr->zoff[MODEL] = mptr->zmax[TARGET];			  	// ... lower so first slice lands on target
	}
      else 
	{
	// if starting at current z, the model z offset is already set as is z_cut.
	//mptr->zoff[MODEL]=mptr->zmax[TARGET]-(z_cut-mptr->zmax[TARGET]);
	mptr->current_z=mptr->zmax[TARGET]-mptr->zoff[MODEL];		// set z of model to current z_cut/zoff
	}
      mptr=mptr->next;
      }
    mptr=job.model_first;						// to be here job.model_first must exist
    slot=0;
    //mptr=NULL;
    //mptr=job_layer_seek(UP);						// get next model and/or layer
    //if(mptr==NULL){printf("  NULL returned from job_layer_seek! - reset to first.\n");mptr=job.model_first;}
    //z_cut=mptr->zmax[TARGET];						// keep z on top surface of target
    }
    
  // if we couldn't find a tool or model to start with
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(mptr==NULL)return(0);

  // at this point we should have a starting model and tool defined to kick off the "first layer"
  vorigin=vertex_make();						// define "home" location of carriage
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    vorigin->x=0; vorigin->y=0; vorigin->z=0;
  #endif
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    vorigin->x=0; vorigin->y=0; vorigin->z=BUILD_TABLE_LEN_Z;
  #endif
  
  // validate some values that could cause divide by 0 if errant
  if(accel<1.0)accel=1.0;
  if(moveveloc<1.0)moveveloc=1.0;
  
  // main processing loop that uses job_layer_seek to increment - it mimics the printing loop
  vpre=vorigin;								// set previous vtx to origin
  while(TRUE)
    {
    active_model=mptr;							// expose which model is in use globally to rest of system
    
    // loop thru all operations required by this model at this layer
    aptr=mptr->oper_list;						// get the pointer to start of operations list for this model
    while(aptr!=NULL)							// loop thru all operations in sequence
      {
      slot=aptr->ID;							// set tool based on this model's operation sequence
      if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}		// skip if no operation defined or tool out of range
      if(slot!=old_slot)tim_est += tool_change_delay;			// if different tool, account for tool changing time
      old_slot=slot;							// save id of slot for next time thru loop

      // validate possible divide by 0 values
      if(Tool[slot].matl.diam<CLOSE_ENOUGH)Tool[slot].matl.diam=1.75;	// assume it was forgotten in matl file
      if(Tool[slot].tip_diam<CLOSE_ENOUGH)Tool[slot].tip_diam=0.40;	// assume it was forgotten in tool file
      if(Tool[slot].stepper_drive_ratio<CLOSE_ENOUGH)Tool[slot].stepper_drive_ratio=1.507;

      sold=NULL;							// save previous slice pointer
      sptr=slice_find(mptr,MODEL,slot,z_cut,mptr->slice_thick);		// find this model's slice for this tool at this z level
      
      //if(sptr!=NULL){printf("   z_cut=%f  sz_lvl=%f  zoff=%f  mcz=%f  jcz=%f\n",z_cut,sptr->sz_level,mptr->zoff[MODEL],mptr->current_z,job.current_z);}
      //else {printf("   z_cut=%f  sz_lvl=NULL  zoff=%f  mcz=%f  jcz=%f\n",z_cut,mptr->zoff[MODEL],mptr->current_z,job.current_z);}
      
      if(sptr!=NULL)							// if unable to locate a print-able slice then move onto next model
	{
	optr=operation_find_by_name(slot,aptr->name);			// get pointer to the tool's operation that matches the next operation in model's list
	if(optr==NULL){aptr=aptr->next;continue;}			// if this tool does not support this operation...
	opr=optr->ID;							// get the ID number of this tool operation
	slice_fill_model(mptr,z_cut);					// generate fill & close-offs for this model at this z level

	// loop thru all copies of this model in x then in y before next operation change
	sptr->matl_estimate=0.0;					// zero out material estimate for this slice
	sptr->time_estimate=0.0;					// zero out time estimate for this slice
	original_xoff=mptr->xorg[mtyp];
	original_yoff=mptr->yorg[mtyp];
	mptr->active_copy=0;
	for(x_copy_ctr=0;x_copy_ctr<mptr->xcopies;x_copy_ctr++)
	  {
	  mptr->xoff[mtyp]=original_xoff+x_copy_ctr*(mptr->xmax[mtyp]-mptr->xmin[mtyp]+mptr->xstp);
	  for(y_copy_ctr=0;y_copy_ctr<mptr->ycopies;y_copy_ctr++)
	    {
	    mptr->yoff[mtyp]=original_yoff+y_copy_ctr*(mptr->ymax[mtyp]-mptr->ymin[mtyp]+mptr->ystp);
	    if(mptr->active_copy>=mptr->total_copies)continue;
	    mptr->active_copy++;
	    lt_ptr=optr->lt_seq;					// get first line type off line type sequence for this operation
	    while(lt_ptr!=NULL)						// loop through all line types identified in the slice's deposition sequence
	      {
	      ptyp=lt_ptr->ID;						// get linetype id
	      pptr=sptr->pfirst[ptyp];					// point to start of polygon list for this type in this slice
	      if(pptr==NULL){lt_ptr=lt_ptr->next;continue;}		// if nothing exists for this line type, move onto next
	      lptr=linetype_find(slot,ptyp);				// get pointer to line type data
	      if(lptr==NULL){lt_ptr=lt_ptr->next;continue;}		// if not found, we must skip
	      
	      // validate possible divide by 0 values
	      if(Tool[slot].matl.diam<CLOSE_ENOUGH)Tool[slot].matl.diam=0.4;	// assume it was forgotten in tool/matl file
	      
	      pre_tim_est=tim_est;
	      pre_mat_est=mat_est;
	      while(pptr!=NULL)						// while we still have polygons to process...
		{
		vptr=pptr->vert_first;					// start at first vertex
		
		// calculate time for pen-up move to new polygon of this line type
		if(vptr!=NULL)
		  {
		  dist=vertex_distance(vpre,vptr);			// ... total 3D distance of vector
		  job.penup_dist += dist;				// ... accrue pen up distance
		  ta=moveveloc/accel;					// ... time for accel ramp
		  da=0.5*accel*(ta*ta);					// ... dist covered during accel
		  if(dist<(2*da))					// ... if veloc is never reached...
		    {tvec=sqrt(2*dist/accel);}
		  else 							// ... otherwise, veloc is reached...
		    {tvec=2*ta+(dist-2*da)/moveveloc;}			// ... time to accel/decell plus time at velocity
		  tim_est += (tvec+move_delay);				// ... accrue time
		  move_counter++;					// ... accrue number of moves
		  }

		// calculate time for pen-down moves along polygon of this line type
		if(lptr->feedrate>0)feedveloc=lptr->feedrate/60.0;	// get pen-down feedrate in mm/s
		if(feedveloc<0.5){printf("  feedrate error:  rate=%f  vel=%f \n",lptr->feedrate,feedveloc); feedveloc=0.5;}
		ta=feedveloc/accel;					// time for accel ramp
		da=0.5*accel*(ta*ta);					// dist needed to reach velocity at this accel
		vector_count=0;
	        vtx_overlap=set_vtx_overlap;				// set the number of vtxs to overlap on borders/offsets to fix start/stop seam
	        if(pptr->vert_qty<3)vtx_overlap=0;			// if just a fill line, reset to 0
	        if(Tool[slot].type==SUBTRACTIVE)vtx_overlap=0;
		do{							// if a valid vtx to move to...
		  vnxt=vptr->next;					// ... get the next vtx
		  if(vnxt==NULL)break;					// ... handles "open" polygons like FILL
		  dist=vertex_distance(vpre,vptr);			// ... total 3D distance of vector
		  job.pendown_dist += dist;				// ... accrue pen down distance
		  mat_est += ((dist*lptr->flowrate/500)*(Tool[slot].matl.diam / Tool[slot].tip_diam) / Tool[slot].stepper_drive_ratio); // ... amount of filament used on this slice in mm
		  if(dist<(2*da))					// ... if veloc is never reached...
		    {tvec=sqrt(2*dist/accel);}				// ... calc time to go up and down accel ramp
		  else 							// ... otherwise, veloc is reached...
		    {tvec=2*ta+(dist-2*da)/feedveloc;}			// ... calc time of ramps plus time at velocity
		  tim_est += (tvec+move_delay);				// ... accumulate time
		  move_counter++;					// ... accrue number of moves
		  vector_count++;
		  vpre=vptr;						// ... save previous position for next run thru loop
		  vptr=vptr->next;					// ... move onto next vertex
		  if(vptr==NULL)break;
		  }while(vector_count<(pptr->vert_qty+vtx_overlap));
		  
		pptr=pptr->next;					// move on to next polygon
		}
		
	      // record time history for this line type
	      hfld=H_OHD_TIM_EST;
	      if(ptyp>=MDL_PERIM && ptyp<=MDL_LAYER_1)hfld=H_MOD_TIM_EST;
	      if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)hfld=H_SUP_TIM_EST;
	      history[hfld][hist_ctr[HIST_TIME]] += (tim_est-pre_tim_est);	// accrue time total and save history values
	      hist_tool[hfld]=slot;
	      sptr->ptime[ptyp]=(tim_est-pre_tim_est);
	      
	      // record material history for this line type
	      // amount extruded = (vector length * flow rate) / 500		- accounted for in print vertex function
	      // filament used = amount extruded * (fila diam / tip diam)	- accounted for below
	      hfld=H_MOD_MAT_EST;						// default to model
	      if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)hfld=H_SUP_MAT_EST;	// adjust if support
	      history[hfld][hist_ctr[HIST_MATL]] += (mat_est-pre_mat_est);	// accrue mat'l total and save history values
	      sptr->matl_estimate=(mat_est-pre_mat_est);

	      lt_ptr=lt_ptr->next;					// move on to next line type
	      }
	    }								// end of y copies loop
	  }								// end of x copies loop
	  
	//accrue total slice time as sum of line types from this slice
        sptr->time_estimate=tim_est;					// in seconds
        sptr->matl_estimate=mat_est;					// in mm
	sold=sptr;
	}								// end of if slice != NULL
      aptr=aptr->next;							// move on to next operation for this model
      }
      
    // move onto next layer (which may be a different model)
    old_z=z_cut;
    mptr->current_z=z_cut;						// save current z position for this model - indicates it has already been deposited at this z level
    if(job.type==ADDITIVE)mptr=job_layer_seek(UP);			// find next model at next z height up
    if(job.type==SUBTRACTIVE)mptr=job_layer_seek(UP);			// find next model at next z height up
    if(job.type==MARKING)
      {
      // when incrementing layers for marking we want to move the model and not the carriage since
      // the carriage is marking on the top surface of a TARGET.  what changes is the slice of model
      // we are marking on that surface as the model "sinks" into it.
      mptr->current_z=mptr->zmax[TARGET]-mptr->zoff[MODEL]+mptr->slice_thick;	// increment z of model that was just printed
      mptr=NULL;
      mptr=job_layer_seek(UP);						// get next model and/or layer
      if(mptr==NULL){printf("  NULL returned from job_layer_seek! - reset to first.\n");mptr=job.model_first;}

      old_z=z_cut;
      z_cut=mptr->zmax[TARGET];						// keep z on top surface of target
      if(slc_print_view==TRUE){mptr->zoff[MODEL] -= slc_view_inc;}	// lower model into block by slice increment
      else {mptr->zoff[MODEL] -= mptr->slice_thick;}			// lower model into block by one slice
      
      if(mptr->zoff[MODEL]<0.0)break;
      }
    if(mptr==NULL)break;						// if no more models to process... we are done

    //printf("\nJTS:  z_cut=%f  zoff=%f  zmin=%f  zmax=%f \n",z_cut,mptr->zoff[MODEL],mptr->zmin[MODEL],mptr->zmax[MODEL]);
    //while(!kbhit());

    // determine if layer is done by comparing new z height to max z height
    if(old_z!=z_cut)
      {
      tim_est=0;							// reset for next layer
      mat_est=0;
      if(sold!=NULL)sold->time_estimate += layer_inc_time;		// add in time it takes to move to next layer

      // accrue the total time and material used in this model so far
      if(sold!=NULL)
        {
        mptr->time_estimate += sold->time_estimate;			// in seconds
        mptr->matl_estimate += sold->matl_estimate;			// in mm
	}
      
      // account for time to move z table in graph values
      history[H_OHD_TIM_EST][hist_ctr[HIST_TIME]] += layer_inc_time;
      if(hist_ctr[HIST_TIME]<(MAX_HIST_COUNT-1))hist_ctr[HIST_TIME]++;	// increment history layer counter
      if(hist_ctr[HIST_MATL]<(MAX_HIST_COUNT-1))hist_ctr[HIST_MATL]++;	// increment history layer counter
      
      // update status to user
      if(job.type==ADDITIVE)amt_done = (double)(z_cut)/(double)(ZMax-ZMin);
      if(job.type==SUBTRACTIVE)amt_done = 1.0 - (double)(z_cut)/(double)(ZMax-ZMin);
      if(job.type==MARKING)amt_done = (double)(old_z)/(double)(ZMax-ZMin);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));
      }

    // update z in job and determine if done
    job.current_z=z_cut;
    if(job.type==ADDITIVE && job.current_z>=ZMax)break;		
    if(job.type==SUBTRACTIVE && job.current_z<=ZMin)break;		
    if(job.type==MARKING && job.current_z>=ZMax)break;		
    }
  
  // reset all model z heights back to "not built yet"
  printf("\nJob Time Est Sim:\n");
  job.time_estimate=0;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    job.time_estimate += mptr->time_estimate;				// time to build whole job
    mptr->current_z=mptr->slice_thick;
    printf("  Model: %s \n",mptr->model_file);
    printf("  Time=%f hrs   Material=%f m \n",mptr->time_estimate/3600,mptr->matl_estimate/1000.0);
    mptr=mptr->next;
    }
    
  // since marking jobs are really run on a slice basis, adjust job time to be the time
  // of just a single slice
  if(job.type==MARKING)
    {
    slot=0;
    job.time_estimate=0;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      sptr=mptr->slice_last[slot];
      job.time_estimate += sptr->time_estimate;
      mptr=mptr->next;
      }
    } 
  
  job.current_z=0.0;							// reset to "not built yet"
  job.total_dist=job.penup_dist+job.pendown_dist;			// add up total job move distance
  z_cut=job.min_slice_thk;						// reset to first layer in job

  sprintf(scratch,"Job Time Est: %6.2f hrs\n",job.time_estimate/3600);
  entry_system_log(scratch);
  
  // clean up
  vertex_destroy(vorigin); vorigin=NULL;
  
  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  gtk_widget_queue_draw(win_info);
  while (g_main_context_iteration(NULL, FALSE));

  return(tim_est);
}

// function to get max min values of all loaded models in all tools and autoplace/center on table
int job_maxmin(void)
{
  int		h,i,cpy_cnt,row_cnt,col_cnt,mdl_cnt,mtyp;
  float 	xp,yp,dx,dy,xsize,ysize,xpos,ypos;
  float 	xzoom,yzoom,zzoom;
  float 	page_inc,page_max;
  model		*mptr,*mpre;

  // if nothing to process just return
  if(job.model_first==NULL)return(0);

  //printf("Job maxmin: entry \n");

  mtyp=MODEL;

  // reset max min values to opposite extremes
  XMin=INT_MAX_VAL;YMin=INT_MAX_VAL;ZMin=INT_MAX_VAL;
  XMax=INT_MIN_VAL;YMax=INT_MIN_VAL;ZMax=INT_MIN_VAL;
  job.XMin=XMin; job.YMin=YMin; job.ZMin=ZMin;
  job.XMax=XMax; job.YMax=YMax; job.ZMax=ZMax;
  job.XOff=0;    job.YOff=0;    job.ZOff=0;

  // autoplace model copies.  this loop figures out how many copies in X and Y are needed to meet
  // the total_copies requirement.  it will automatically limit the number of copies to those that will
  // fit onto the build table.
  // When done, each model (and its copies) occupies the following space on the build table:
  //    model size in x = (mptr->xcopies*(mptr->xmax[MODEL]-mptr->xmin[MODEL]+mptr->xstp)-mptr->xstp);
  //    model size in y = (mptr->ycopies*(mptr->ymax[MODEL]-mptr->ymin[MODEL]+mptr->ystp)-mptr->ystp);
  mdl_cnt=0;
  mpre=NULL;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    cpy_cnt=1;								// init total number of copies
    mptr->xcopies=1; mptr->ycopies=1;					// init copies in X and in Y
    row_cnt=1; col_cnt=1;						// init row and col counters
    while(cpy_cnt < mptr->total_copies)					// while adding more copies...
      {
      // determine current dimensions in x and y that this model and all its copies occupy
      dx=(mptr->xmax[mtyp] - mptr->xmin[mtyp] + mptr->xstp)*row_cnt - mptr->xstp;
      dy=(mptr->ymax[mtyp] - mptr->ymin[mtyp] + mptr->ystp)*col_cnt - mptr->ystp;
      
      // if space left in y... add another copy in y direction
      if((BUILD_TABLE_LEN_Y - dy) > ((dy + mptr->ystp)/col_cnt))
        {
	col_cnt++;							// increment copy counter in Y
	if(col_cnt>mptr->ycopies)mptr->ycopies=col_cnt;			// add to model count
	cpy_cnt++;							// increment our copy counter
	continue;
	}
      
      // if space is left in x ... add another copy in x direction
      if((BUILD_TABLE_LEN_X - dx) > ((dx + mptr->xstp)/row_cnt))
        {
	row_cnt++;							// increment copy count in X
	if(row_cnt>mptr->xcopies)mptr->xcopies=row_cnt;			// add to model count
	col_cnt=1;							// reset count in Y
	cpy_cnt++;							// increment our copy counter
	continue;
	}
      
      // and if there is no room left...
      mptr->total_copies=cpy_cnt;					// reset to max that would fit
      break;
      }
    
    //if(col_cnt>1)mptr->xcopies--;
    
    if((mptr->zmin[mtyp]+mptr->zoff[mtyp]) < ZMin)ZMin=mptr->zmin[mtyp]+mptr->zoff[mtyp];	// adjust z values
    if((mptr->zmax[mtyp]+mptr->zoff[mtyp]) > ZMax)ZMax=mptr->zmax[mtyp]+mptr->zoff[mtyp];

  printf("\n\nJob AutoPlacement:\n");
  printf("  Model Off = %06.3f %06.3f %06.3f \n",mptr->xoff[MODEL],mptr->yoff[MODEL],mptr->zoff[MODEL]);
  printf("  Model Min = %06.3f %06.3f %06.3f \n",mptr->xmin[MODEL],mptr->ymin[MODEL],mptr->zmin[MODEL]);
  printf("  Model Max = %06.3f %06.3f %06.3f \n",mptr->xmax[MODEL],mptr->ymax[MODEL],mptr->zmax[MODEL]);

    mdl_cnt++;								// total number of models in job
    mpre=mptr;								// save address of previous model
    mptr=mptr->next;
    }

  // if autoarranging models, figure out xy offsets for each model set.  this is not an elligant packing
  // algorithm.  it simply takes the next available model and checks to see if it fits best in x or y
  // relative to the other models that have already been placed on the table.
  xpos=0;  ypos=0;							// next available origin position
  xsize=0; ysize=0;							// overall size of all models currently on table
  if(autoplacement_flag==TRUE)
    {
    mptr=job.model_first;						// starting with first model
    while(mptr!=NULL)							// loop thru all loaded models
      {
      // calc overall size of this model and its copies
      dx = mptr->xcopies * (mptr->xmax[MODEL] - mptr->xmin[MODEL] + mptr->xstp);
      dy = mptr->ycopies * (mptr->ymax[MODEL] - mptr->ymin[MODEL] + mptr->ystp);
      
      if(mptr==job.model_first)
        {
	mptr->xorg[MODEL]=xpos;  
	mptr->yorg[MODEL]=ypos;
	xsize=dx;  ysize=dy;
	xpos=0; ypos=dy;
	mptr=mptr->next;
	continue;
	}
      
      // if space left in y is bigger than the model we want to add ... then add model in y direction
      if((BUILD_TABLE_LEN_Y - ypos) >= dy)
        {
	ysize=ypos+dy;
	if(dx>xsize)xsize=dx;
	}
      
      // otherwise if space left in x is big enough ... then add model in x direction
      else if((BUILD_TABLE_LEN_X - xsize) >= dx)
        {
	xpos=xsize+mptr->xstp;	ypos=0;
	}
      
      // and if there is no room left, leave it where it is and exit...
      else 
        {break;}
	
      // define position of the current model
      mptr->xorg[MODEL]=xpos;  
      mptr->yorg[MODEL]=ypos;
      mptr->xoff[MODEL]=mptr->xorg[MODEL];
      mptr->yoff[MODEL]=mptr->yorg[MODEL];

      // no matter where the model was placed, increment ypos by the model dy for the next model
      ypos += dy;
      
      mptr=mptr->next;
      }
    }
  else
    {
    if(job.model_first!=NULL)
      {
      mptr=job.model_first;
      xsize=mptr->xorg[MODEL] + (mptr->xmax[MODEL] - mptr->xmin[MODEL]);
      ysize=mptr->yorg[MODEL] + (mptr->ymax[MODEL] + mptr->ymin[MODEL]);
      }
    }
    
  XMin=job.model_first->xorg[MODEL]; YMin=job.model_first->yorg[MODEL];	// origin of first model
  XMax=xsize-job.model_first->xstp;  YMax=ysize-job.model_first->ystp;	// max size of last model
  //printf("\n\nJob MaxMin: entry\n");  
  //printf("  Global Min = %06.3f %06.3f %06.3f \n",XMin,YMin,ZMin);
  //printf("  Global Max = %06.3f %06.3f %06.3f \n",XMax,YMax,ZMax);

  // center entire collection of models, including copies, on build table if requested
  //set_center_build_job=FALSE;
  if(set_center_build_job==TRUE)
    {
    xp=BUILD_TABLE_LEN_X/2 - XMax/2;					// delta bt center of model group and center of build table
    yp=BUILD_TABLE_LEN_Y/2 - YMax/2;
    
    printf("\n\nJob MaxMin: centering\n");
    printf("  TblCenX=%f  TblCenY=%f \n",BUILD_TABLE_LEN_X/2,BUILD_TABLE_LEN_Y/2);
    printf("  MdlCenX=%f  MdlCenY=%f \n",XMax/2,YMax/2);
    printf("       xp=%f       yp=%f \n",xp,yp);

    // loop thru all models within each tool and adjust it's offsets by xp and yp
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      mptr->xorg[MODEL] += xp;
      mptr->yorg[MODEL] += yp;
      mptr->xoff[MODEL]=mptr->xorg[MODEL];
      mptr->yoff[MODEL]=mptr->yorg[MODEL];
      mptr=mptr->next;
      }
    XMin+=xp; XMax+=xp;
    YMin+=yp; YMax+=yp;
    }
  
  // update job size
  job.XMin=XMin; job.YMin=YMin; job.ZMin=ZMin;
  job.XMax=XMax; job.YMax=YMax; job.ZMax=ZMax;
  
  // update z slider with max value (ZMax) and minimum increment (job.min_slice_thk)
  // and force a filling operation
  if(job.model_first!=NULL)
    {
    page_inc=job.min_slice_thk;
    page_max=job.min_slice_thk*2;
    gtk_adjustment_configure(g_adj_z_level,z_cut,job.ZMin,job.ZMax,job.min_slice_thk,page_inc,page_max);
    slice_fill_all(z_cut);
    }
  
  // calculate zoom based on job bounding box
  // nominal reference: MVview_scale=0.90 fits the entire 16" box of GAMMA_UNIT
  if(set_auto_zoom==TRUE)
    {
    xzoom=BUILD_TABLE_LEN_X/(XMax-XMin);
    yzoom=BUILD_TABLE_LEN_Y/(YMax-YMin);
    //zzoom=BUILD_TABLE_LEN_Z/(ZMax-ZMin);
    MVview_scale=xzoom;
    if(yzoom<MVview_scale)MVview_scale=yzoom;
    //if(zzoom<MVview_scale)MVview_scale=zzoom;

    // set limits
    if(Perspective_flag==TRUE)
      {
      if(MVview_scale<0.2)MVview_scale=0.2;
      if(MVview_scale>12.0)MVview_scale=12.0;
      }

    //set_auto_zoom=FALSE;
    }
    
  // force all other model types to align with base model type
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    for(i=SUPPORT;i<MAX_MDL_TYPES;i++)
      {
      mptr->xmin[i]=mptr->xmin[MODEL]; mptr->ymin[i]=mptr->ymin[MODEL];
      mptr->xmax[i]=mptr->xmax[MODEL]; mptr->ymax[i]=mptr->ymax[MODEL];
      mptr->xorg[i]=mptr->xorg[MODEL]; mptr->yorg[i]=mptr->yorg[MODEL];   
      mptr->xoff[i]=mptr->xoff[MODEL]; mptr->yoff[i]=mptr->yoff[MODEL];   
      mptr->xrot[i]=mptr->xrot[MODEL]; mptr->yrot[i]=mptr->yrot[MODEL];
      mptr->xrpr[i]=mptr->xrpr[MODEL]; mptr->yrpr[i]=mptr->yrpr[MODEL];
      mptr->xscl[i]=mptr->xscl[MODEL]; mptr->yscl[i]=mptr->yscl[MODEL];
      mptr->xspr[i]=mptr->xspr[MODEL]; mptr->yspr[i]=mptr->yspr[MODEL];
      mptr->xmir[i]=mptr->xmir[MODEL]; mptr->ymir[i]=mptr->ymir[MODEL];
      }
    mptr=mptr->next;
    }
  
  printf("\nJob MaxMin: exit\n");  
  printf("  Global Min = %06.3f %06.3f %06.3f \n",job.XMin,job.YMin,job.ZMin);
  printf("  Global Max = %06.3f %06.3f %06.3f \n\n",job.XMax,job.YMax,job.ZMax);
  
  return(1);
}

// function to purge slice data and support structures out of a job, but not remove the model structures
int job_purge(void)
{
  int		slot;
  edge 		*eptr,*edel;
  polygon	*pptr,*pdel;
  patch 	*patptr,*patdel;
  model		*mptr;
  
  //printf("JobPurge entry: \n");

  // loop thru all models in job 
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mptr->reslice_flag==FALSE){mptr=mptr->next;continue;}		// if this model does not need slicing...

    model_purge(mptr);
    mptr=mptr->next;
    }
  
  job.state=JOB_NOT_READY;
    
  //printf("JobPurge exit: \n");
  
  return(TRUE);
}

// function to close out a completed job
int job_survey_done(GtkWidget *btn, gpointer dead_window)
{
  model		*mptr,*mdel;
  
  // destory this window
  gtk_window_close(GTK_WINDOW(dead_window));
    
  // clear slice data and support structure data
  job_purge();
  
  // clear 3D model data
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mdel=mptr;
    mptr=mptr->next;
    model_delete(mdel);
    }
    
  // clear job data
  job_clear();
  
  return(TRUE);
}

// fuction to collect model questionare data
void grab_surf_value(GtkSpinButton *btn, gpointer user_data)
{
  int	surf_type;
  
  surf_type=GPOINTER_TO_INT(user_data);

  // vertical side wall surfaces
  if(surf_type==1)active_model->vert_surf_val=gtk_spin_button_get_value(btn);

  // bottom surface
  if(surf_type==2)active_model->botm_surf_val=gtk_spin_button_get_value(btn);

  // lower close-off surfaces
  if(surf_type==3)active_model->lwco_surf_val=gtk_spin_button_get_value(btn);

  // upper close-off surfaces
  if(surf_type==4)active_model->upco_surf_val=gtk_spin_button_get_value(btn);

  return;
}

// Function to post job complete questionare
// The idea is to record feedback from the user regarding the quality of the job just run.
int job_complete(void)
{
  GtkWidget	*win_jc, *btn_exit, *img_exit, *lbl_scratch;
  GtkWidget	*hbox, *hbox_up, *grd_hbox_up, *hbox_dn, *grd_hbox_dn, *vbox;
  GtkWidget	*vert_surf_btn, *botm_surf_btn, *lwco_surf_btn, *upco_surf_btn;
  GtkAdjustment	*vert_surf_adj, *botm_surf_adj, *lwco_surf_adj, *upco_surf_adj;
	
  char		mdl_name[255];
  int		h,i;
  model		*mptr;
  
  sprintf(scratch,"Job completed.");
  entry_system_log(scratch);
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    active_model=mptr;
    
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

    // build window with questions for user about model quality
    // set up primary window
    win_jc = gtk_window_new();				
    gtk_window_set_transient_for (GTK_WINDOW(win_jc), GTK_WINDOW(win_main));
    gtk_window_set_modal (GTK_WINDOW(win_jc),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_jc),400,500);
    gtk_window_set_resizable(GTK_WINDOW(win_jc),FALSE);			
    sprintf(scratch,"Job Complete Survey");
    gtk_window_set_title(GTK_WINDOW(win_jc),scratch);			
    
    // set up an hbox to divide the screen into a narrower segments
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_window_set_child (GTK_WINDOW (win_jc), hbox);
    
    // now divide left side into segments for buttons
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vbox);
    
    // and further divide the right side into quads - two "up" and two "dn"
    hbox_up=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_box_append(GTK_BOX(hbox),hbox_up);
    grd_hbox_up=gtk_grid_new();
    gtk_box_append(GTK_BOX(hbox_up),grd_hbox_up);
    gtk_grid_set_row_spacing (GTK_GRID(grd_hbox_up),6);
    gtk_grid_set_column_spacing (GTK_GRID(grd_hbox_up),12);
    
    hbox_dn=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_box_append(GTK_BOX(hbox),hbox_dn);
    grd_hbox_dn=gtk_grid_new();
    gtk_box_append(GTK_BOX(hbox_dn),grd_hbox_dn);
    gtk_grid_set_row_spacing (GTK_GRID(grd_hbox_dn),6);
    gtk_grid_set_column_spacing (GTK_GRID(grd_hbox_dn),12);
    
    // set up EXIT button
    btn_exit = gtk_button_new ();
    g_signal_connect (btn_exit, "clicked", G_CALLBACK (job_survey_done), win_jc);
    img_exit = gtk_image_new_from_file("Back.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
    gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
    gtk_box_append(GTK_BOX(vbox),btn_exit);
    
    // display name of model this applies to
    lbl_scratch=gtk_label_new(mdl_name);
    gtk_grid_attach (GTK_GRID(grd_hbox_up),lbl_scratch, 0, 0, 1, 1);
    
    // surface finish questionare
    lbl_scratch=gtk_label_new("  Vertical surface finish:");
    gtk_grid_attach (GTK_GRID(grd_hbox_up),lbl_scratch, 0, 1, 1, 1);
    vert_surf_adj=gtk_adjustment_new(mptr->vert_surf_val, 0, 5, 1, 1, 1);
    vert_surf_btn=gtk_spin_button_new(vert_surf_adj, 2, 0);
    gtk_grid_attach(GTK_GRID(grd_hbox_up),vert_surf_btn, 1, 1, 1, 1);
    g_signal_connect(vert_surf_btn,"value-changed",G_CALLBACK(grab_surf_value),GINT_TO_POINTER(1));
	
    lbl_scratch=gtk_label_new("  Bottom surface finish:");
    gtk_grid_attach (GTK_GRID(grd_hbox_up),lbl_scratch, 0, 2, 1, 1);
    botm_surf_adj=gtk_adjustment_new(mptr->botm_surf_val, 0, 5, 1, 1, 1);
    botm_surf_btn=gtk_spin_button_new(botm_surf_adj, 2, 0);
    gtk_grid_attach(GTK_GRID(grd_hbox_up),botm_surf_btn, 1, 2, 1, 1);
    g_signal_connect(botm_surf_btn,"value-changed",G_CALLBACK(grab_surf_value),GINT_TO_POINTER(2));

    lbl_scratch=gtk_label_new("  Lower close-off surface finish:");
    gtk_grid_attach (GTK_GRID(grd_hbox_up),lbl_scratch, 0, 3, 1, 1);
    lwco_surf_adj=gtk_adjustment_new(mptr->lwco_surf_val, 0, 5, 1, 1, 1);
    lwco_surf_btn=gtk_spin_button_new(lwco_surf_adj, 2, 0);
    gtk_grid_attach(GTK_GRID(grd_hbox_up),lwco_surf_btn, 1, 3, 1, 1);
    g_signal_connect(lwco_surf_btn,"value-changed",G_CALLBACK(grab_surf_value),GINT_TO_POINTER(3));
	
    lbl_scratch=gtk_label_new("  Upper close-off surface finish:");
    gtk_grid_attach (GTK_GRID(grd_hbox_up),lbl_scratch, 0, 4, 1, 1);
    upco_surf_adj=gtk_adjustment_new(mptr->upco_surf_val, 0, 5, 1, 1, 1);
    upco_surf_btn=gtk_spin_button_new(upco_surf_adj, 2, 0);
    gtk_grid_attach(GTK_GRID(grd_hbox_up),upco_surf_btn, 1, 4, 1, 1);
    g_signal_connect(upco_surf_btn,"value-changed",G_CALLBACK(grab_surf_value),GINT_TO_POINTER(4));
	
    gtk_widget_set_visible(win_jc,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

    // wait here for user to fill out data on active model before moving onto next model
    while((mptr->vert_surf_val + mptr->botm_surf_val + mptr->lwco_surf_val + mptr->upco_surf_val)==0){delay(100);}

    mptr=mptr->next;
    }
    
  active_model=NULL;
  
  return(TRUE);
}

// function to clear job information
int job_clear(void)
{
  int	h,i;
  
  job.sync=FALSE;
  job.prev_state=job.state;
  job.state=JOB_NOT_READY;
  job.regen_flag=TRUE;
  job.baselayer_flag=FALSE;
  job.type=UNDEFINED;
  job.min_slice_thk=BUILD_TABLE_MAX_Z;
  job.baselayer_flag=FALSE;
  job.baselayer_type=1;
  job.model_count=0;
  job.XMax=0.0; job.YMax=0.0; job.ZMax=0.0;
  job.XMin=0.0; job.YMin=0.0; job.ZMin=0.0;
  job.current_z=0.0;
  job.time_estimate=0.0;
  job.current_dist=0.0;
  job.total_dist=0.0;
  job.start_time=time(NULL);
  job.finish_time=time(NULL);
  job.model_first=NULL;
  for(i=0;i<MAX_MDL_TYPES;i++)job.lfacet_upfacing[i]=NULL;
  
  zoffset=0.0;
  active_model=NULL;
  
  // clear existing history since this is possible a re-start/re-run
  for(i=0;i<=HIST_MATL;i++)hist_ctr[i]=0;				// clear time and matl counter values
  for(i=H_MOD_MAT_EST;i<=H_OHD_TIM_ACT;i++)				// clear time and matl, but leave thermal			
    {
    for(h=0;h<MAX_HIST_COUNT;h++){history[i][h]=0.0;}			// clear existing values (if any)
    }
  
  // reset views
  {MVview_scale=1.2; MVgxoffset=425; MVgyoffset=375; MVdisp_spin=(-37*PI/180); MVdisp_tilt=(210*PI/180);}
  {LVview_scale=1.00; LVgxoffset=155; LVgyoffset=(-490);}
  
  // clear job display
  //if(main_view_page==VIEW_TOOL)
  //  {
  //  for(i=0;i<MAX_THERMAL_DEVICES;i++)gtk_label_set_text(GTK_LABEL(mdl_desc[i]),"Empty     ");
  //  }

  sprintf(scratch,"Job cleared.");
  entry_system_log(scratch);
  
  return(1);
}

// Function to establish the thinnest layer thickness in use in the job
float job_find_min_thickness(void)
{
  int		slot;
  float 	min_thk; 
  model		*mptr;
  genericlist	*aptr;
  
  min_thk=1000.0;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    aptr=mptr->oper_list;
    while(aptr!=NULL)
      {
      slot=aptr->ID;
      if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
      if(Tool[slot].matl.layer_height<min_thk)min_thk=Tool[slot].matl.layer_height;
      aptr=aptr->next;
      }
    mptr=mptr->next;
    }
  
  return(min_thk);
}

// function to slice a job
int job_slice(void)
{
  int 		i,slot,oper_ID,keep_going,mtyp=MODEL;
  float 	bld_time;
  float 	old_xs,old_ys,old_zs;
  float 	old_xp,old_yp,old_zp;
  float 	page_inc,page_max;
  model		*mptr;
  genericlist	*aptr;
  
  printf("JobSlice entry:  job_regen=%d  mdl_first=%X \n",job.regen_flag,job.model_first);
  
  // ensure there is something to do
  if(job.model_first==NULL)return(0);					// make sure a model is defined
  keep_going=FALSE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    aptr=mptr->oper_list;
    if(aptr!=NULL){keep_going=TRUE; break;}				// make sure at least one model has an operation defined
    mptr=mptr->next;
    }
  if(keep_going==FALSE)return(0);					// if no operations defined, just leave now
  
  // reset temps to correct operational value
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(Tool[slot].state<TL_LOADED)continue;
    if(Tool[slot].pwr24==TRUE)Tool[slot].thrm.setpC=Tool[slot].thrm.operC;
    }
    
  // if job level change was made ...
  if(job.regen_flag==TRUE)						
    {
    // find thinnest slice thickness of all loaded tools and set z level there
    job.min_slice_thk=BUILD_TABLE_MAX_Z;
    job.min_slice_thk=job_find_min_thickness();
    if(job.min_slice_thk<=0.0)job.min_slice_thk=0.001;			// min allowable thickness
    if(job.type==ADDITIVE)z_cut=job.min_slice_thk;
    if(job.type==SUBTRACTIVE)z_cut=job.ZMax-job.min_slice_thk;
    if(job.type==MARKING)z_cut=job.ZMax-job.min_slice_thk;
    job.current_dist=0.0;
    job.total_dist=0.0;
  
    // ensure the model positions/orientations/parameters are compatible
    //job_rules_check();
  
    // build support structures if requested.  must be done before slicing.  note that this is not model
    // specific as supports can span across models.  it is a job level type operation.
    //job_build_grid_support();
    job_build_tree_support();
  
    // apply results of user inputs
    mptr=job.model_first;
    while(mptr!=NULL)							// loop thru all loaded models
      {
      printf("\nJobSlice:  Model=%X  ResliceStatus=%d \n",mptr,mptr->reslice_flag);
      if(mptr->reslice_flag==FALSE){mptr=mptr->next;continue;}		// if this model does not need slicing...
      aptr=mptr->oper_list;						// start with first operation in list
      while(aptr!=NULL)							// loop thru all of this model's operations
	{
	printf("   OperID=%d  OperName=%s \n",aptr->ID,aptr->name);
	
	// note:  a separate slice deck for the same model is created for each tool.
	// this allows each tool to work at different slice thicknesses depending on the parameters of depostion.
	slot=aptr->ID;							// get the tool that will perform this operation
	if(slot>=0 && slot<MAX_TOOLS)					// if a viable tool...
	  {
	  mptr->slice_thick=Tool[slot].matl.layer_height;		// make sure these are in sync
	  
	  if(mptr->input_type==IMAGE)					// if IMAGE...
	    {
	    if(strstr(aptr->name,"MARK_OUTLINE")!=NULL)			// if marking an outline...
	      {
	      printf("   oper=mark_outline  -- converting to STL\n");
	      mptr->grayscale_mode=0;					// ... intensity is not modulated
	      image_to_STL(mptr);					// ... convert to STL
	      mptr->input_type=STL;					// ... by changing here it will run as STL below
	      }
	    if(strstr(aptr->name,"MARK_AREA")!=NULL)			// if marking an area...
	      {
	      mptr->grayscale_mode=0;					// ... intensity is not modulated
	      printf("   oper=mark_area  -- slicing\n");
	      image_to_STL(mptr);					// ... convert to STL
	      mptr->input_type=STL;					// ... by changing here it will run as STL below
	      }
	    if(strstr(aptr->name,"MARK_IMAGE")!=NULL)			// if marking an image...
	      {
	      mptr->grayscale_mode=2;					// ... intensity is modulated by z height
	      printf("   oper=mark_image  -- slicing\n");
	      oper_ID=OP_MARK_IMAGE;
	      model_grayscale_cut(mptr);				// ... create fill lines based on grayscale values
	      set_view_image=FALSE;					// ... turn off to see slice instead
	      }
	    }
	  if(mptr->input_type==STL)					// if STL ... 
	    {
	    if(strstr(aptr->name,"ADD_MODEL_MATERIAL")!=NULL)		// if adding model material...
	      {
	      hist_use_flag[H_MOD_MAT_EST]=TRUE;			// turn on appropriate history tracking fields for model material
	      hist_use_flag[H_MOD_MAT_ACT]=TRUE;
	      hist_use_flag[H_MOD_TIM_EST]=TRUE;
	      hist_use_flag[H_OHD_TIM_EST]=TRUE;
	      hist_use_flag[H_MOD_TIM_ACT]=TRUE;
	      hist_use_flag[H_OHD_TIM_EST]=TRUE;
	      old_xs=mptr->xscl[mtyp]; old_xp=mptr->xspr[mtyp];		// ... save current and previous scale values
	      mptr->xspr[mtyp]=1.00;					// ... reset prev to current
	      mptr->xscl[mtyp]=1.00+(Tool[slot].matl.shrink/100);	// ... build shrink values from material data
	      old_ys=mptr->yscl[mtyp]; old_yp=mptr->yspr[mtyp];
	      mptr->yspr[mtyp]=1.00;
	      mptr->yscl[mtyp]=1.00+(Tool[slot].matl.shrink/100);
	      old_zs=mptr->zscl[mtyp]; old_zp=mptr->zspr[mtyp];
	      mptr->zspr[mtyp]=1.00;
	      mptr->zscl[mtyp]=1.00+(Tool[slot].matl.shrink/100);
	      model_scale(mptr,MODEL);					// ... apply mat'l shrink factor scaling
	      mptr->xscl[mtyp]=old_xs; mptr->xspr[mtyp]=old_xp;		// ... restore scale values
	      mptr->yscl[mtyp]=old_ys; mptr->yspr[mtyp]=old_yp;
	      mptr->zscl[mtyp]=old_zs; mptr->zspr[mtyp]=old_zp;
	      model_maxmin(mptr);					// ... scaling will have thrown off boundaries
	      oper_ID=OP_ADD_MODEL_MATERIAL;
	      
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      if(mptr->vase_mode==TRUE && mptr->internal_spt==TRUE)	// ... if vase mode with internal structure...
		{
		model_build_internal_support(mptr);			// ... create internal support facets
		slice_deck(mptr,INTERNAL,oper_ID);			// ... create a slice deck of internal structure for this model
		}
	      }
	    if(strstr(aptr->name,"ADD_SUPPORT_MATERIAL")!=NULL)		// if adding support material ...
	      {
	      hist_use_flag[H_SUP_MAT_EST]=TRUE;			// turn on appropriate history tracking fields for support material
	      hist_use_flag[H_SUP_MAT_ACT]=TRUE;
	      hist_use_flag[H_SUP_TIM_EST]=TRUE;
	      hist_use_flag[H_OHD_TIM_EST]=TRUE;
	      hist_use_flag[H_SUP_TIM_ACT]=TRUE;
	      hist_use_flag[H_OHD_TIM_EST]=TRUE;
	      oper_ID=OP_ADD_SUPPORT_MATERIAL;
	      slice_deck(mptr,SUPPORT,oper_ID);				// ... create a slice deck of support for this model
	      }
	    if(strstr(aptr->name,"MARK_OUTLINE")!=NULL)			// if marking an outline...
	      {
	      printf("   oper=mark_outline  -- slicing\n");
	      oper_ID=OP_MARK_OUTLINE;
	      if(mptr->mdl_has_target==FALSE)model_build_target(mptr);	// ... create target for marking
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      }
	    if(strstr(aptr->name,"MARK_AREA")!=NULL)			// if marking an area...
	      {
	      printf("   oper=mark_area  -- slicing\n");
	      oper_ID=OP_MARK_AREA;
	      if(mptr->mdl_has_target==FALSE)model_build_target(mptr);	// ... create target for marking
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      }
	    if(strstr(aptr->name,"MARK_IMAGE")!=NULL)			// if marking an image...
	      {
	      printf("   oper=mark_image  -- slicing\n");
	      oper_ID=OP_MARK_IMAGE;
	      if(mptr->mdl_has_target==FALSE)model_build_target(mptr);	// ... create target for marking
	      cnc_profile_cut_flag==TRUE;				// ... set flag to indicate profile cut slices (i.e. uses active Z to regulate intensity)
	      model_profile_cut(mptr);					// ... create profile cut slice deck
	      }
	    if(strstr(aptr->name,"MARK_CUT")!=NULL)			// if marking cut...
	      {
	      printf("   oper=mark_cut  -- slicing\n");
	      oper_ID=OP_MARK_CUT;
	      if(mptr->mdl_has_target==FALSE)model_build_target(mptr);	// ... create target for marking
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      }
	    if(strstr(aptr->name,"MILL_OUTLINE")!=NULL)			// if cnc milling...
	      {
	      printf("   oper=mill_outline  -- slicing\n");
	      oper_ID=OP_MILL_OUTLINE;
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      }
	    if(strstr(aptr->name,"MILL_AREA")!=NULL)			// if cnc milling...
	      {
	      printf("   oper=mill_area  -- slicing\n");
	      oper_ID=OP_MILL_AREA;
	      slice_deck(mptr,MODEL,oper_ID);				// ... create a slice deck of this model for this tool
	      }
	    if(strstr(aptr->name,"MILL_PROFILE")!=NULL)			// if marking an image...
	      {
	      printf("   oper=mill_profile  -- slicing\n");
	      oper_ID=OP_MILL_PROFILE;
	      cnc_profile_cut_flag==TRUE;				// ... set flag to indicate profile cut slices (i.e. Z will actively move tip up and down)
	      model_profile_cut(mptr);					// ... create profile cut slice deck
	      }
	    }
	  if(mptr->input_type==GERBER)					// if GERBER ...
	    {
	    set_view_lt[TRACE]=TRUE;					// ... turn on trace viewing
	    //set_view_lt[DRILL]=TRUE;					// ... turn on drill viewing
	    gerber_model_load(mptr);					// ... load gerber slice
	    model_maxmin(mptr);						// ... determine boundaries
	    mptr->current_z=0.25;					// ... set slice thickness for model
	    job.min_slice_thk=0.25;					// ... set min slice thickness for job
	    z_cut=0.25;							// ... set current layer
	    job.regen_flag=FALSE;					// ... set flag that job is ready
	    }
	  }
	idle_start_time=time(NULL);					// reset idle time
	aptr=aptr->next;						// move onto next operation for this model
	}
      if(mptr->oper_list!=NULL)mptr->reslice_flag=FALSE;		// if ops are defined... then it was sliced
      mptr=mptr->next;
      }
  
    smart_check=FALSE;
    job.regen_flag=FALSE;
    job_maxmin();							// determine boundaries of whole job
    job_base_layer2(job.min_slice_thk);					// generate base layers (must be done after slicing)
    job_map_to_table();							// set dynamic z compensation values
    job_time_estimate_simulator();					// run simulation to determine time to build job
    //job_time_estimate_calculator();					// alternate way to calculate time to build job
    }
	  
  if(job.type==ADDITIVE)z_cut=job.min_slice_thk;
  if(job.type==SUBTRACTIVE)z_cut=cnc_z_height;
  if(job.type==MARKING)
    {
    z_cut=cnc_z_height;
    if(BLD_TBL1 < MAX_THERMAL_DEVICES)Tool[BLD_TBL1].thrm.heat_duty=0.0;	// turn off
    if(BLD_TBL2 < MAX_THERMAL_DEVICES)Tool[BLD_TBL2].thrm.heat_duty=0.0;	// turn off
    if(CHAMBER < MAX_THERMAL_DEVICES) Tool[CHAMBER].thrm.heat_duty=100.0; 	// turn fnas on full
    }
    

  // update z slider with max value (ZMax) and minimum increment (job.min_slice_thk)
  // and force a filling operation
  if(job.model_first!=NULL)
    {
    page_inc=job.min_slice_thk;
    page_max=job.min_slice_thk*2;
    gtk_adjustment_configure(g_adj_z_level,z_cut,job.ZMin,job.ZMax,job.min_slice_thk,page_inc,page_max);
    slice_fill_all(z_cut);
    
    // calc the maximum number of layers currently in this job
    job.max_layer_count=0;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      for(mtyp=0; mtyp<MAX_MDL_TYPES; mtyp++)
	{
	if(mptr->slice_first[mtyp]!=NULL)job.max_layer_count += mptr->slice_qty[mtyp];
	}
      mptr=mptr->next;
      }
    }
  
  // reset job status if user deleted all pending models
  if(job.model_first==NULL && job.state>=JOB_READY){job.state=JOB_NOT_READY;}
  
  display_model_flag=TRUE;
  sprintf(scratch,"Job sliced.");
  entry_system_log(scratch);

  printf("\nJobSlice exit: \n");

  return(TRUE);
}

// Function to generate a job base layer that represents all models on the table.
// This function operates by aggregating model and support first layers to produce an outline of the first layer.
// It then dialates this outline and defines it as the baselayer.
int job_base_layer2(float zlvl)
{
  int		i,j,base_slot,model_slot,ptyp;
  float 	dialate_dist=(-2.0);
  double 	amt_done=0,amt_current=0,amt_total=0;

  vertex	*vptr,*vbas;
  polygon 	*pold,*pnew,*pptr;
  slice 	*sptr,*sptr_base,*sptr_frst;
  model 	*mptr;
  
  //printf("\n\nJOB_BASE_LAYER: Entry -----------------------------------\n");

  // loop thru models, find the ones that have a base layer op requested, and use their first layer
  // polygons (model and support combined) to generate their base layer.
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    base_slot=model_get_slot(mptr,OP_ADD_BASE_LAYER);			// determine which tool is adding the base layer
    if(base_slot<0 || base_slot>=MAX_TOOLS){mptr=mptr->next;continue;}
    model_slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);		// determine which tool has the slice data
    if(model_slot<0 && model_slot>=MAX_TOOLS){mptr=mptr->next;continue;}
    printf("\nGenerating baselayer for %s using tool %d ...\n",mptr->model_file,base_slot);

    // init the base layer slice
    if(mptr->base_layer==NULL)						// if not made yet... make it
      {
      mptr->base_layer=slice_make();
      }
    else  								// if already made... wipe out current contents
      {
      slice_wipe(mptr->base_layer,MDL_PERIM,0);
      slice_wipe(mptr->base_layer,MDL_OFFSET,0);
      slice_wipe(mptr->base_layer,BASELYR,0);
      slice_wipe(mptr->base_layer,PLATFORMLYR1,0);
      slice_wipe(mptr->base_layer,PLATFORMLYR2,0);
      }

    sptr_base=mptr->base_layer;
    sptr_base->sz_level=0.0;
    sptr_frst=slice_find(mptr,MODEL,model_slot,zlvl,mptr->slice_thick);
    if(sptr_frst==NULL || sptr_base==NULL)return(0);
    ptyp=MDL_PERIM;

    // DEBUG - dump slice data
    printf("\nsptr_frst=%X  z=%5.3f  MP_qty=%d  SP_qty=%d \n\n",sptr_frst,sptr_frst->sz_level,sptr_frst->pqty[MDL_PERIM],sptr_frst->pqty[SPT_PERIM]);

    
    // make super simple base layer out of fisrt layer max/min boundaries
    if(job.baselayer_type==0)
      {
      vptr=vertex_make();						// create a first vertex
      pnew=polygon_make(vptr,ptyp,(-1));				// create the "polygon" for this layer
      sptr_base->pfirst[ptyp]=pnew;					// define future access to it
      sptr_base->plast[ptyp]=pnew;					// define future access to it
      sptr_base->pqty[ptyp]=1;						// indicate that it has that one polygon in it
      vptr->x=sptr_frst->xmin; vptr->y=sptr_frst->ymin; vptr->z=0;
      vbas=vertex_make();
      vbas->x=sptr_frst->xmax; vbas->y=sptr_frst->ymin; vbas->z=0;
      vptr->next=vbas;
      vptr=vbas;
      vbas=vertex_make();
      vbas->x=sptr_frst->xmax; vbas->y=sptr_frst->ymax; vbas->z=0;
      vptr->next=vbas;
      vptr=vbas;
      vbas=vertex_make();
      vbas->x=sptr_frst->xmin; vbas->y=sptr_frst->ymax; vbas->z=0;
      vptr->next=vbas;
      vptr=vbas;
      vptr->next=pnew->vert_first;
      pnew->vert_qty=4;
      pnew->hole=FALSE;
      pnew->type=BASELYR;
      polygon_find_center(pnew);
      pnew->area=polygon_find_area(pnew);
      if(pnew->area>0)polygon_reverse(pnew);
      }

    // otherwise use the combined model and support polygons of the first layer to generate
    // the base layer.
    else 
      {
      // init
      sptr_base->pfirst[ptyp]=NULL;
      sptr_base->plast[ptyp]=NULL;
      sptr_base->pqty[ptyp]=0;	
      
      // copy model polygons over to base layer
      pold=NULL;
      pptr=sptr_frst->pfirst[MDL_PERIM];
      printf("   copying over %d model polygons\n",sptr_frst->pqty[MDL_PERIM]);
      while(pptr!=NULL)
        {
	pnew=polygon_copy(pptr);
	if(sptr_base->pfirst[ptyp]==NULL)sptr_base->pfirst[ptyp]=pnew;
	if(pold!=NULL)pold->next=pnew;
	pold=pnew;
	pptr=pptr->next;
	sptr_base->pqty[ptyp]++;	
	}
      
      // copy support polygons over to base layer
      pptr=sptr_frst->pfirst[SPT_PERIM];
      printf("   copying over %d support polygons\n",sptr_frst->pqty[SPT_PERIM]);
      while(pptr!=NULL)
        {
	pnew=polygon_copy(pptr);
	if(sptr_base->pfirst[ptyp]==NULL)sptr_base->pfirst[ptyp]=pnew;
	if(pold!=NULL)pold->next=pnew;
	pold=pnew;
	pptr=pptr->next;
	sptr_base->pqty[ptyp]++;	
	}
      }
  
    // now finally dialate it to give some margin around the actual model
    dialate_dist=(-2.0);
    slice_offset_winding(sptr_base,ptyp,MDL_OFFSET,dialate_dist);
    printf("   %d polygons generated for %s.\n",sptr_base->pqty[MDL_PERIM],mptr->model_file);
  
    job.baselayer_flag=TRUE;
    set_view_lt[MDL_LAYER_1]=TRUE;					// if baselayer is on, so is layer 1
    set_view_lt[INT_LAYER_1]=TRUE;
    set_view_lt[SPT_LAYER_1]=TRUE;
  
    mptr=mptr->next;
    }

  printf("   baselayer complete.\n");
  return(1);
}

// Function to find a z level in a job that corrisponds to a slice layer.  Since this code allows models of different
// slice thicknesses to simultaneous be built, this function serves the purpose of finding the next model & layer to be built.
// This is tricky since each tool may use a different slice thickness and each tool can have multiple models with varying z offsets.
// It's further complicated by all models being sliced relative to z=0 and then offsetting them in z by their mptr->zoff.
// Since this is a job function, it INCLUDES THE MPTR->ZOFF component (which is different than what slice_find function does).

// Inputs:   	UP = next z level in + direction that contains slice data from any model
//		LEVEL = closest z level to current z_cut that contains slice data from any model
//		DOWN = next z level in - direction that contains slice data from any model

// Returns:  	the function will return the a pointer to the first model encountered that is farthest behind the current z level,
//		and contains layer at that z level, and will set the global variable z_cut to the z value of that slice in build volume coords.

model *job_layer_seek(int direction)
{
  int 		mtyp=MODEL;
  float 	zdist,zmin,new_z_cut,pre_z_cut;
  float 	lyr_ht;
  model 	*mptr,*next_model;

  //printf("\nJLS Entry:  z_cut=%f direction=%d\n",z_cut,direction);

  //if(cnc_profile_cut_flag==TRUE)return(job.model_first);		// only one model when profile cutting (for now)

  zmin=0.0;								// init search metric for UP/DOWN (looking for a max value)
  if(direction==LEVEL)zmin=BUILD_TABLE_MAX_Z;				// the search works in the opposite way for LEVEL (looking for a min value)
  pre_z_cut=z_cut;							// save the entry value of z_cut
  next_model=NULL;							// init our return object
  while(next_model==NULL)						// loop unitl we've determined what it is
    {
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// need to check all models as they may be floating at various z levels
      {
      if(mptr->input_type==GERBER){mptr=mptr->next;continue;}		// skip GERBER files... they only have one slice
      if(mptr->input_type==IMAGE){mptr=mptr->next;continue;}		// skip image files... they only have one slice
      lyr_ht=mptr->slice_thick;						// simplify code at expense of variable
      
      // determine if this model needs a layer deposited to catch up to current z level (typical for additive)
      if(direction==UP)
	{
	zdist=mptr->current_z+lyr_ht;					// calculate model's current z height plus one layer
	if(zdist<=z_cut && zdist<=(mptr->zmax[mtyp]+mptr->zoff[mtyp]+lyr_ht))	// if it is less than the printing z level but still in span of model...
	  {
	  if(fabs(z_cut-zdist)>=zmin)					// if it is the farthest one behind the printing z level...
	    {
	    next_model=mptr;						// ... save the model address
	    zmin=fabs(z_cut-zdist);					// ... save the distance
	    }
	  }
	}
      // find the model with the closest layer to current z_cut level
      if(direction==LEVEL)
	{
	zdist=mptr->current_z;						// calculate model's current z height as is
	if(fabs(z_cut-zdist)<zmin)					// is it the closest one to current z level...
	  {
	  next_model=mptr;						// save the model address
	  zmin=fabs(z_cut-zdist);					// save the distance
	  }
	}
      // determine if this model needs a layer cut to catch up to current z level (typical for subtractive)
      if(direction==DOWN)
	{
	zdist=mptr->current_z-lyr_ht;					// calculate model's current z height less one layer
	if(zdist>=z_cut && zdist>=(mptr->zmin[mtyp]+mptr->zoff[mtyp]+lyr_ht))	// if it is higher than the printing z level...
	  {
	  if(fabs(zdist-z_cut)>=zmin)					// if it is the farthest one behind the printing z level...
	    {
	    next_model=mptr;						// ... save the model address
	    zmin=fabs(zdist-z_cut);					// ... save the distance
	    }
	  }
	}
      mptr=mptr->next;
      }
    if(next_model!=NULL)break;						// if we have found a model, then we're done
    
    // if no model was found, perterb z level slightly in requested direction and search again.
    // this is especially important when several tools/models all have the exact same layer thickness.
    if(direction==UP)   z_cut+=0.0001;					// nudge up
    if(direction==LEVEL)break;						// in this case, we just leave
    if(direction==DOWN) z_cut-=0.0001;					// nudge down
    if(z_cut>ZMax || z_cut<ZMin)break;					// if beyond job limits... exit
    if(z_cut>BUILD_TABLE_LEN_Z || z_cut<0)break;			// if beyond table limits... exit
    }

  // calculate the new z_cut so that it lands exactly on the next slice level of the found model
  new_z_cut=z_cut;							// default new z to current z
  if(next_model!=NULL)							// if a model was found...
    {
    // calculate new z_cut to land on this slice
    lyr_ht=next_model->slice_thick;					// ... simplify code at expense of variable
    if(direction==UP)   new_z_cut=next_model->current_z+lyr_ht;		// ... if up, add a slice thickness
    if(direction==LEVEL)new_z_cut=next_model->current_z;		// ... if level, stay at z
    if(direction==DOWN) new_z_cut=next_model->current_z-lyr_ht;		// ... if down, subtract a slice thickness
    }
  z_cut=new_z_cut;							// define global of z level as new level

  //printf("JLS Exit: oldzcut=%f  z_cut=%f  ZMax=%f  nxtmdl=%X \n\n",pre_z_cut,z_cut,ZMax,next_model);

  return(next_model);
}

// function to map entire job to build table z offsets
int job_map_to_table(void)
{
  int			h,i;
  float 		xp,yp,zp;
  float 		minPz;
  model			*mptr;
  
  //printf("  Adjusting build table scan to model.\n");
  
  // normalize all to 0,0 in raw array and load into processed array
  // note all other values of processed array outside of model bounds will be (-1)
  minPz=Pz_raw[0][0];
  for(i=0;i<SCAN_X_MAX;i++)
    {
    for(h=0;h<SCAN_Y_MAX;h++)
      {
      if(Pz_raw[i][h]>0)Pz[i][h]=Pz_raw[i][h]-minPz;
      }
    }
  build_table_scan_flag=FALSE;						// turn "need to map" flag off
  
  return(1);
}

// Function to generate GRID type support structure for a job.
//
// Basic order of events:
// 1. Identify model facets that will require support and turn them into upper patches.
// 2. Identify upward facing model facets that fall directly under the upper patches.  Store these
//    facets in child patches of the upper patch.
// 3. Step thru each upper support patch in a pre-defined XY grid.  Use the Z at each grid point to
//    create a vector that projects down to the build table.  Find the heighest facet it intersects with
//    and use that for the bottom Z that exists within each child patch.
// 4. Build the vertical facet walls by connecting the upper and lower meshes.
//
int job_build_grid_support(void)
{
  int		h,i,j,k,slot,ptyp;
  int		vtst,vint_resA,vint_resB,vint_resC;
  int		patch_cnt,mfacet_dn_cnt,mfacet_up_cnt;
  int		add_spt,vint_count;
  int	 	ixcnt,iycnt,grid_xcnt,grid_ycnt;
  int		vtx_in[4];
  float 	ix,iy,deltaz,grid_xinc,grid_yinc;
  float 	max_angle,vz0,vz1,vz2,avg_z,avg_n[3];
  float 	dx,dy,dz,u,Czmax,Dzmax,odist=0.01;
  float 	support_post_size=2.0,max_facet_area,ctr_offset=0.01;
  float 	minx,maxx,miny,maxy,minz,maxz;
  double 	amt_done=0,amt_current=0,amt_total=0;
  
  facet		*fptr,*fnxt,*mfptr,*sfptr,*test_fptr,*ftest;
  facet		*fnew,*fwdg,*fct0,*fct1,*fct2,*fngh;
  
  vector	*vecnew,*A,*D,*vectest,*vecA,*vecB,*vecC;
  vector 	*vec[4];
  
  vertex 	*vintA,*vintB,*vintC,*vtxnew[4],*vtxlow[4],*vtxint;
  vertex 	*vtxyA,*vtxyB,*vtxyC,*vtxyD,*vtxD;
  vertex	*vtxtest[3],*vtxtest0, *vtx_ptr, *vtx_norm;
  vertex 	*vctr,*vptr,*vnxt,*vdel,*vnew,*vold,*cvptr,*cvnxt,*vtxdel;
  vertex	*vtx_f0,*vtx_f1,*vtx_f2,*vtx_minz;
  
  polygon 	*polyedge,*pptr,*fctedge;
  patch		*patptr,*pchptr,*patdel,*patpre,*patnxt,*pat_last,*patold,*pchold,*pchnew,*pupptr,*patspt,*pchspt;
  vertex_list	*vl_ptr,*vl_list;
  facet_list	*pfptr,*pfnxt,*pfset,*ptest;
  slice 	*sptr;
  model 	*mptr;
  
  printf("\nJob Build Support:  entry\n");
  debug_flag=0;

  // validate inputs
  if(job.model_first==NULL)return(FALSE);
  
  // determine if user is requesting support
  {
    add_spt=FALSE;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot>=0 && slot<MAX_TOOLS)add_spt=TRUE;				// if a viable tool is specified...
      mptr=mptr->next;
      }
    printf("Support Check:  slot=%d \n",slot);
    if(add_spt==FALSE)return(FALSE);
  }
  
  // initialize
  {
    // loop thru all models in this job.  this is a job level function since the user has the ability to "stack"
    // models above each other which makes them codependent regarding supports.  so even if the "under" model does
    // not have support requested, the "above" model may have its supports landing on "under's" upper facing facets.
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      // clear existing model and support patches if they exist
      if(mptr->patch_model!=NULL)
	{
	patptr=mptr->patch_model;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);						// this will delete child patches as well
	  }
	mptr->patch_model=NULL;
	}
      if(mptr->patch_first_lwr!=NULL)
	{
	patptr=mptr->patch_first_lwr;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);
	  }
	mptr->patch_first_lwr=NULL;
	}
      if(mptr->patch_first_upr!=NULL)
	{
	patptr=mptr->patch_first_upr;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);
	  }
	mptr->patch_first_upr=NULL;
	}
    
      // clear existing support facets if they exist
      if(mptr->facet_first[SUPPORT]!=NULL)
	{
	facet_purge(mptr->facet_first[SUPPORT]);
	mptr->facet_first[SUPPORT]=NULL;  mptr->facet_last[SUPPORT]=NULL; mptr->facet_qty[SUPPORT]=0;
	vertex_purge(mptr->vertex_first[SUPPORT]);
	mptr->vertex_first[SUPPORT]=NULL; mptr->vertex_last[SUPPORT]=NULL; mptr->vertex_qty[SUPPORT]=0;
	}
	
      // set all model facet and vertex attributes to 0.  this is needed because we'll be using the attr value as a
      // status check in the subsequent operation.
      mfptr=mptr->facet_first[MODEL];
      while(mfptr!=NULL)
	{
	mfptr->status=0;
	mfptr->attr=0;
	for(j=0;j<3;j++)
	  {
	  if(mfptr->vtx[j]==NULL){printf("  facet=%X has null vtx at %d \n",mfptr,j);}
	  else {(mfptr->vtx[j])->attr=0; (mfptr->vtx[j])->supp=0;}
	  }
	mfptr=mfptr->next;
	}
  
      amt_total += mptr->facet_qty[MODEL];				// accrue total number of facets to examine
      mptr=mptr->next;
      }
    
    // purge the upward facing facet list
    job.lfacet_upfacing[MODEL]=facet_list_manager(job.lfacet_upfacing[MODEL],NULL,ACTION_CLEAR);
    job.lfacet_upfacing[MODEL]=NULL;
      
    // show the progress bar
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Identifying areas that need support  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    //gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_model));
    //gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);
    
    printf("  initialization complete.\n");
  }
  
  // step 1 - identify patches (contiguous groups of model facets) that will need support.  these will
  // become the upper support patches.  do this by scanning thru the model facet list, finding
  // a facet that is pointing down, creating a new patch for it, then traversing thru its neighoring
  // facet ptrs to add to the patch.  once all neighbors that meet the downward criterion are found,
  // resume scanning thru the model facet list to find a new "seed" facet for the next patch.
  //
  // at the same time create a list of only upward facing model facets.  these would be the facets
  // that would stop support structure.  this shortened list speeds things up later.
  //
  // also search the model facet list for local minimum vtxs.  this would be a vtx that has ALL the facets around it
  // face downward but are not so shallow as to be identified as being "downward enough" AND has all the vtxs
  // around it higher, or equal to it, in z.  in other words, it's a local minium point.  build upper
  // support patches from those cases as well with the same logic.
  {
    amt_done=0; amt_current=0;
    patch_cnt=0; mfacet_dn_cnt=0; mfacet_up_cnt=0;
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      // check if supports are requested for this model by checking its operations list
      slot=(-1);							// default no tool selected
      max_angle=cos(-50*PI/180);					// default if no tool selected yet
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot>=0 && slot<MAX_TOOLS)					// if a viable tool is specified...
	{
	max_angle=cos(Tool[slot].matl.overhang_angle*PI/180);		// ... puts in terms of normal vector value (note, this is a negative value)
	mptr->slice_thick=Tool[slot].matl.layer_height;			// ... reset to keep in sync
	}
	
      if(debug_flag==322)printf("  mptr=%X  max_angle=%6.3f \n",max_angle);
	
      // loop thru all facets of this model and determine which need support, or which could have
      // support land on them.
      h=0;
      pat_last=NULL;
      mfptr=mptr->facet_first[MODEL];
      while(mfptr!=NULL)
	{
	// update status to user
	amt_current++;
	amt_done = (double)(amt_current)/(double)(amt_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
      
	h++;								// increment facet counter for this patch
	
	// skip any model facets that have already been added to a patch
	// status meaning:  0=not yet checked, 1=checked and not added to patch, 2=checked and added to patch
	if(mfptr->status>0)
	  {
	  //printf(" %d mfptr=%X  status=%d  next=%X\n",h,mfptr,mfptr->status,mfptr->next); 
	  mfptr=mfptr->next; 
	  continue;
	  }
	//printf(" %d mfptr=%X  status=%d  next=%X\n",h,mfptr,mfptr->status,mfptr->next); 
	
	//if(debug_flag==322)printf("  mfptr->unit_norm=%X \n",mfptr->unit_norm);
	
	// skip all but downward facing facets.  tolerance eliminates near vertical facets.
	if((mfptr->unit_norm)->z >= (0-TOLERANCE))
	  {
	  // only add facet to upward facing list if not vertical AND has other facets above it
	  if((mfptr->unit_norm)->z > TOLERANCE)
	    {
	    //if(debug_flag==322)printf("  %d mfptr=%X is upward.\n",h,mfptr);
	    mfacet_up_cnt++;
	    job.lfacet_upfacing[MODEL]=facet_list_manager(job.lfacet_upfacing[MODEL],mfptr,ACTION_ADD);
	    }
	  mfptr->status=1;
	  //if(debug_flag==322)printf("  %d mfptr=%X  ADD  next=%X\n",h,mfptr,mfptr->status,mfptr->next); 
	  mfptr=mfptr->next; 
	  continue;
	  }
    
	// skip facets that are too close to the build table.  we cannot add support to anything
	// thinner than a slice thickness.
	vz0=mfptr->vtx[0]->z;
	vz1=mfptr->vtx[1]->z;
	vz2=mfptr->vtx[2]->z;
	avg_z=(vz0+vz1+vz2)/3;
	if(avg_z<=mptr->slice_thick)
	  {
	  mfptr->status=1; 
	  mfptr=mfptr->next; 
	  continue;
	  }	
      
	// include downward facing facets that are more shallow than max angle which is defined by the material
	// properties of the support material.  this needs to happen before checking for a minimum vtx otherwise
	// we'll get redundant supports.
	//if(debug_flag==322)printf("  %d mfptr=%X unit_norm:  x=%6.3f  y=%6.3g  z=%6.3f \n",h,mfptr,mfptr->unit_norm->x,mfptr->unit_norm->y,mfptr->unit_norm->z);
	if((mfptr->unit_norm->z*mfptr->unit_norm->z)>fabs(max_angle))
	  {
	  mfacet_dn_cnt++;
	  
	  // now that a seed facet is identified, create a new patch.
	  patch_cnt++;							// increment our patch count
	  patptr=patch_make();						// make a new patch
	  patptr->ID=patch_cnt;						// assign ID based on count.  this is basically its body ID
	  if(mptr->patch_model==NULL)mptr->patch_model=patptr;		// if first one, define as head of list
	  if(pat_last!=NULL)pat_last->next=patptr;			// if not first, add linkage
	  pat_last=patptr;						// save address for link next time thru loop
	  if(debug_flag==322)printf("  new patch created %d \n",patch_cnt);
	  
	  // add the seed facet to the new patch.  note that patch_add_facet only creates a
	  // pointer to the model facet.
	  mfptr->status=2;						// set status of facet to "added to patch"
	  mfptr->member=patch_cnt;
	  patptr->pfacet=facet_list_manager(patptr->pfacet,mfptr,ACTION_ADD);
	  patptr->area = mfptr->area;
	  if(debug_flag==322)printf("   seed facet added\n");
    
	  // loop thru list of facets already added to the patch and check the three neighbors 
	  // for any that have not yet been checked.  if not checked, see if they should be added.
	  test_fptr=mfptr;						// set to seed facet at entry
	  while(test_fptr!=NULL)					// loop until no other facets qualify
	    {
	    // test first neighbor
	    fptr=test_fptr->fct[0];					// set facet to first neighbor of seed
	    if(fptr!=NULL)						// if a facet exists ...
	      {
	      if(debug_flag==322)printf("   testing n0 facet...\n");
	      if(fptr->status==0)					// ... if not yet checked
		{
		fptr->status=1;						// ... set to checked
		if((fptr->unit_norm)->z < (0-TOLERANCE))		// ... if facing downward
		  {
		  vz0=fptr->vtx[0]->z;
		  vz1=fptr->vtx[1]->z;
		  vz2=fptr->vtx[2]->z;
		  avg_z=(vz0+vz1+vz2)/3;
		  if(avg_z>mptr->slice_thick)				// ... if z is off of the build table
		    {
		    if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))  // ... if steeper than material allows
		      {
		      fptr->member=patptr->ID;				// ... identify which patch it went to
		      fptr->status=2;					// ... set status to "added to patch"
		      patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD); // ... add it to the patch's facet list
		      patptr->area += fptr->area;			// ... accrue the patch area
		      if(debug_flag==322)printf("   n0 facet added\n");
		      }
		    }
		  }
		}
	      }
	    // test second neighbor
	    fptr=test_fptr->fct[1];
	    if(fptr!=NULL)
	      {
	      if(debug_flag==322)printf("   testing n1 facet...\n");
	      if(fptr->status==0)					// ... if not yet checked
		{
		fptr->status=1;						// ... set to checked
		if((fptr->unit_norm)->z < (0-TOLERANCE))		// ... if facing downward
		  {
		  vz0=fptr->vtx[0]->z;
		  vz1=fptr->vtx[1]->z;
		  vz2=fptr->vtx[2]->z;
		  avg_z=(vz0+vz1+vz2)/3;
		  if(avg_z>mptr->slice_thick)				// ... if z is off of the build table
		    {
		    if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
		      {
		      fptr->member=patptr->ID;
		      fptr->status=2;
		      patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD);
		      patptr->area += fptr->area;
		      if(debug_flag==322)printf("   n1 facet added\n");
		      }
		    }
		  }
		}
	      }
	    // test third neighbor
	    fptr=test_fptr->fct[2];
	    if(fptr!=NULL)
	      {
	      if(debug_flag==322)printf("   testing n2 facet...\n");
	      if(fptr->status==0)					// ... if not yet checked
		{
		fptr->status=1;						// ... set to checked
		if((fptr->unit_norm)->z < (0-TOLERANCE))		// ... if facing downward
		  {
		  vz0=fptr->vtx[0]->z;
		  vz1=fptr->vtx[1]->z;
		  vz2=fptr->vtx[2]->z;
		  avg_z=(vz0+vz1+vz2)/3;
		  if(avg_z>mptr->slice_thick)				// ... if z is off of the build table
		    {
		    if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
		      {
		      fptr->member=patptr->ID;
		      fptr->status=2;
		      patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD);
		      patptr->area += fptr->area;
		      if(debug_flag==322)printf("   n2 facet added\n");
		      }
		    }
		  }
		}
	      }
	    // find a facet in this patch that has a neighbor that has not been checked.  note this will
	    // eventually check all facets within the bounds of our conditions (i.e. downward, above zero,
	    // and material angle) along with the ring of facets (neighbors) around the patch that are
	    // the ones that fail the conditions.
	    if(debug_flag==322)printf("  searching for next neighbor seed in this patch... \n");
	    test_fptr=NULL;						// undefine the seed facet
	    pfptr=patptr->pfacet;					// starting with first facet in this patch
	    while(pfptr!=NULL)						// loop thru all facets in this patch
	      {
	      fptr=pfptr->f_item;					// get address of actual facet
	      if(fptr!=NULL)						// if valid ...
		{
		if(fptr->fct[0]->status==0){test_fptr=fptr;break;}  	// ... if 1st neighbor not checked set as seed
		if(fptr->fct[1]->status==0){test_fptr=fptr;break;}    	// ... if 2nd neighbor not checked set as seed
		if(fptr->fct[2]->status==0){test_fptr=fptr;break;}    	// ... if 3rd neighbor not checked set as seed
		}
	      pfptr=pfptr->next;
	      }
	    if(debug_flag==322)printf("  next neighbor seed = %X \n",test_fptr);
	    }
	  }	// end of if overhang angle requires support for this material
	  
	mfptr=mfptr->next;
	}		// end of main facet loop
  
      // verify that we have viable patches
      patdel=NULL;
      patpre=NULL;
      patptr=mptr->patch_model;
      while(patptr!=NULL)
	{
	if(patptr->area < (2*min_polygon_area))				// if patch is too small to process...
	  {
	  patdel=patptr;
	  if(patdel==mptr->patch_model)					// if the head node...
	    {
	    mptr->patch_model=patdel->next;				// ... reset head node to next element
	    patptr=patptr->next;					// ... update current pointer to next element
	    free(patdel);						// ... remove the patch from memory
	    }
	  else 								// otherwise if not head node....
	    {
	    if(patpre!=NULL)patpre->next=patdel->next;			// ... if prev patch address is known, link past del patch
	    patptr=patdel->next;					// ... update current pointer
	    free(patdel);						// ... remove the patch from memory
	    }
	  }
	else 
	  {
	  patpre=patptr;
	  patptr=patptr->next;
	  }
	}

      // this section now finds the perimeter vertex list for each patch.  that is, it determines the list of vtxs
      // that form the free edge(s) around and inside (holes) for each patch and create a 3D polygon(s) out of them.
      // this same function also establishes the minz and maxz of the patch.  that is, the highest and lowest vtxs in it.
      if(debug_flag==322)printf("  building perimeter vtx list for patches...\n");
      patptr=mptr->patch_model;
      while(patptr!=NULL)
	{
	// find out which facets do not have neighbors
	patch_find_free_edge(patptr);
	patptr=patptr->next;
	}

      mptr=mptr->next;
      }
      
    printf(" upper patches and upward facing facets complete.\n");
  }
 
  //gtk_widget_set_visible(win_info,FALSE);
  //return(1);
  
  // step 2 - identify facets in lower patches
  // project the upper patch outlines onto the upper facing model facets to create lower (child) patches.
  // the lower patch facets will not align with the upper patch facets but should cover the entire upper
  // patch area.  so if any vtx of an upper facing model facet falls within the bound of the upper patch,
  // that facet is included into the lower patch.  in a fashion similar to creating the upper patches, a
  // model facet falling inside the upper patch outline is found (the seed), then facets are added by
  // looking thru neighboring facets.  note that in theory this will make multiple child patches if the
  // lower surface under the upper patch is discontinuos (think mushroom feature under large overhang).
  {
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Analyzing model intersection areas  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    amt_done=0;
    amt_current=0;
    amt_total=mfacet_up_cnt;
    
    vtxint=vertex_make();
    vtx_minz=vertex_make();
    vtxD=vertex_make();							// scratch vtx used to hold facet centroid
    vtx_f0=vertex_make(); vtx_f1=vertex_make(); vtx_f2=vertex_make();	// scratch vtxs used to define facet edge polygon
    vtx_f0->next=vtx_f1; vtx_f1->next=vtx_f2; vtx_f2->next=vtx_f0;	// loop them into a polygon
    fctedge=polygon_make(vtx_f0,0,0);					// define the polygon name and address
    pfptr=job.lfacet_upfacing[MODEL];					// start with first upward facing model facet
    while(pfptr!=NULL)							// loop thru all upward facing model facets
      {
      // update status to user
      amt_current++;
      amt_done = (double)(amt_current)/(double)(amt_total);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      mfptr=pfptr->f_item;						// get address of actual facet
      
      // find lowest point of this facet
      vtx_minz->x=mfptr->vtx[0]->x; vtx_minz->y=mfptr->vtx[0]->y; vtx_minz->z=mfptr->vtx[0]->z;
      if(mfptr->vtx[1]->z<vtx_minz->z){vtx_minz->x=mfptr->vtx[1]->x; vtx_minz->y=mfptr->vtx[1]->y; vtx_minz->z=mfptr->vtx[1]->z;}
      if(mfptr->vtx[2]->z<vtx_minz->z){vtx_minz->x=mfptr->vtx[2]->x; vtx_minz->y=mfptr->vtx[2]->y; vtx_minz->z=mfptr->vtx[2]->z;}
      minz=vtx_minz->z;

      // load test vtx with facet centroid location
      facet_centroid(mfptr,vtxD);

      // define coords of edge polygon around the facet to aid in testing
      vtx_f0->x=mfptr->vtx[0]->x; vtx_f0->y=mfptr->vtx[0]->y; vtx_f0->z=0.0;
      vtx_f1->x=mfptr->vtx[1]->x; vtx_f1->y=mfptr->vtx[1]->y; vtx_f1->z=0.0;
      vtx_f2->x=mfptr->vtx[2]->x; vtx_f2->y=mfptr->vtx[2]->y; vtx_f2->z=0.0;

      // loop thru all models in the job that request support
      mptr=job.model_first;						// start with first model in job
      while(mptr!=NULL)							// loop thru all models in job
	{
	// loop thru upper patches and determine if this facet is within the xy bound of its free edge polygon.  if so,
	// add this facet to its child patch.  note that model facets may belong to multiple child patches.
	patold=NULL;
	patptr=mptr->patch_model;					// start with first model upper patch
	while(patptr!=NULL)						// loop thru all model upper patches
	  {
	  if(minz > patptr->maxz){patptr=patptr->next; continue;}	// skip if this facet is above this patch
	  polyedge=patptr->free_edge;
	  
	  // determine if this facet overlaps the free edge polygon in any way.
	  vtst=polygon_overlap(fctedge,polyedge);
	  
	  // if any of this model facet falls within the patch bounds... 
	  if(vtst==TRUE)
	    {
	    //vtxint->x=vtx_minz->x; vtxint->y=vtx_minz->y;		// define xy at facet minz
	    //patch_find_z(patptr,vtxint);				// get z height at this xy of the upper patch
	    // test if this facet is below the upper patch ... if so, include it in the child patch
	    //if(vtx_minz->z < vtxint->z)
	      {
	      // if child patch does not yet exist, define it
	      if(patptr->pchild==NULL)
		{
		pchptr=patch_make();					// make a new child patch
		patptr->pchild=pchptr;		
		pchptr->ID=patptr->ID;					// define as from same body ID
		}
	      else 
		{
		pchptr=patptr->pchild;
		}
		
	      // add this facet to the child patch
	      pchptr->pfacet=facet_list_manager(pchptr->pfacet,mfptr,ACTION_ADD);	
	      pchptr->area += mfptr->area;
	      }
	    }
	      
	  patptr=patptr->next;						// move onto next upper patch
	  }
	mptr=mptr->next;
	}

      pfptr=pfptr->next;						// move onto next upward facing model facet
      }
      
    // clean up
    free(fctedge); polygon_mem--; fctedge=NULL;
    vertex_destroy(vtx_f0); vtx_f0=NULL;
    vertex_destroy(vtx_f1); vtx_f1=NULL;
    vertex_destroy(vtx_f2); vtx_f2=NULL;
    vertex_destroy(vtxD);   vtxD=NULL;
    vertex_destroy(vtxint); vtxint=NULL;

    printf("  lower model patches identified.\n");
  }
  
  // step 3 - superimpose xy grid over each upper patch to generate upper support facet mesh.  at each grid
  // point, build a vertical vector down to the build table and look for the highest upward facing facet
  // intersection in that patch's child patches.  use that z value to build the lower facet mesh.  Also 
  // create upper and lower support patches while doing this step.
  {
    // initialize
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Building support facets  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    amt_done=0;
    amt_current=0;
    amt_total=patch_cnt;
    vtxint=vertex_make();
    
    // loop thru all models in the job that request support
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot<0 || slot>=MAX_TOOLS){mptr=mptr->next; continue;}		// if this model does not require support... skip it.

      // loop thru each patch and apply grid
      patold=NULL;
      patptr=mptr->patch_model;						// start with first upper patch
      while(patptr!=NULL)						// loop thru all found in model
	{
	// update status to user
	amt_current++;
	amt_done = (double)(amt_current)/(double)(amt_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
  
	// ensure there is a free edge polygon associated with this patch.  recall that the first polyedge
	// referenced by the patch contains material, but all the other polyedges pointed to by that first
	// polyedge are holes.  so as we identify if a grid point is inside the patch (i.e. first polyedge)
	// we also need to verify that it is not inside a hole.
	polyedge=patptr->free_edge;
	if(polyedge==NULL){patptr=patptr->next;continue;}
	
	// create the upper support patch and its corresponding lower support child patch
	patspt=patch_make();
	if(mptr->patch_first_upr==NULL)mptr->patch_first_upr=patspt;
	if(patold!=NULL)patold->next=patspt;
	patold=patspt;
	pchspt=patch_make();
	patspt->pchild=pchspt;
	patspt->ID=patptr->ID;
	pchspt->ID=patptr->ID;
    
	// get the max/min bounds of this patch.
	minx=BUILD_TABLE_MAX_X;miny=BUILD_TABLE_MAX_Y;minz=BUILD_TABLE_MAX_Z;
	maxx=BUILD_TABLE_MIN_X;maxy=BUILD_TABLE_MIN_Y;maxz=BUILD_TABLE_MIN_Z;
	vnew=polyedge->vert_first;					// start with first vtx in free edge list
	while(vnew!=NULL)						// loop thru all vtxs in free edge list
	  {
	  if(vnew->x>maxx)maxx=vnew->x;					// get max x
	  if(vnew->x<minx)minx=vnew->x;					// get min x
	  if(vnew->y>maxy)maxy=vnew->y;					// get max y
	  if(vnew->y<miny)miny=vnew->y;					// get min y
	  if(vnew->z>maxz)maxz=vnew->z;					// get max z
	  if(vnew->z<minz)minz=vnew->z;					// get min z
	  vnew=vnew->next;						// move onto next vtx in free edge list
	  if(vnew==polyedge->vert_first)break;
	  }

	/*
	// calculate xy offsets and count qty based on patch size and grid increment sizes
	grid_xinc=(maxx-minx)/10;					// set x grid based on size
	if(grid_xinc<0.1)grid_xinc=0.1;					// lower threshold
	if(grid_xinc>1.0)grid_xinc=1.0;					// upper threshold
	 
	grid_yinc=(maxy-miny)/10;					// set y grid based on size
	if(grid_yinc<0.1)grid_yinc=0.1;
	if(grid_yinc>1.0)grid_yinc=1.0;
	*/
	
	// calculate xy count qty based on z delta of patch - steeper slope needs higher resolution.
	deltaz=(patptr->maxz-patptr->minz);				// get z delta
	if(deltaz>CLOSE_ENOUGH)						// if not flat...
	  {
	  grid_xinc=1/deltaz;						// ... grid size is inverse to z delta
	  }
	else 								// otherwise it is flat...
	  {
	  grid_xinc=1.0;						// ... so use a wide grid
	  }
	if(grid_xinc<0.1)grid_xinc=0.1;					// lower threshold
	if(grid_xinc>1.0)grid_xinc=1.0;					// upper threshold
	
	grid_xinc=2.000;						// DEBUG - force to lower resolution
	grid_yinc=grid_xinc;
	
	
	// this currently just uses min/max of the patch to get support up to edge of the patch, but this only
	// works well for square patches.  this out to be drawing a ray across the patch at each ix value to
	// get the min/max of y so that odd shaped polygons can be supported properly.
	
	grid_xcnt=floor((maxx-minx)/grid_xinc);				// calc count of steps in x less one
	grid_ycnt=floor((maxy-miny)/grid_yinc);				// calc count of steps in y less one
	minx += 0.1;
	miny += 0.1;
	
	// sweep across x from start to stop
	for(ixcnt=0;ixcnt<=grid_xcnt;ixcnt++)
	  {
	  ix=minx+ixcnt*grid_xinc;					// calc new x position
	  dx=grid_xinc;							// reset increment
	  if(ixcnt==grid_xcnt)dx=maxx-ix-0.1;				// if at final edge, change increment to remainder
	    
	  // sweep across y from start to stop
	  for(iycnt=0;iycnt<=grid_ycnt;iycnt++)
	    {
	    iy=miny+iycnt*grid_yinc;					// calc new y position
	    dy=grid_yinc;						// reset increment
	    if(iycnt==grid_ycnt)dy=maxy-iy-0.1;				// if at final edge, change increment to remainder
	    
	    // create 4 test vtxs and check if all are within patch and not inside holes of that patch
	    vtxnew[0]=vertex_make(); vtxnew[0]->x=ix;           vtxnew[0]->y=iy;			// vtx at current increment
	    vtxnew[1]=vertex_make(); vtxnew[1]->x=ix+dx; 	vtxnew[1]->y=iy;			// vtx at next increment in x
	    vtxnew[2]=vertex_make(); vtxnew[2]->x=ix;           vtxnew[2]->y=iy+dy;			// vtx at next increment in y
	    vtxnew[3]=vertex_make(); vtxnew[3]->x=ix+dx;	vtxnew[3]->y=iy+dy;			// vtx at next increment in xy
	    
	    
	    //if(make_facet==TRUE)
	      {
	    
	      // since our xy grid is sweeping in a min/max square over this patch, we need to check that
	      // each new vtx coord is acutally within the odd shaped polygon of that patch.  this also
	      // accounts for the edge condition to get that last vtx near the edge of the polygon right
	      // onto its edge.
	      for(i=0;i<4;i++)
		{
		vtx_in[i]=polygon_contains_point(polyedge,vtxnew[i]);	// check if this vtx is within the patch
		if(vtx_in[i]==TRUE)					// if it does fall inside the patch...
		  {
		  // check if it falls within a hole
		  pptr=polyedge->next;					// starting with the first hole (if there)
		  while(pptr!=NULL)					// loop thru all holes in this polyedge
		    {
		    if(pptr->hole>0)					// if this polygon is a hole of any type
		      {
		      if(polygon_contains_point(pptr,vtxnew[i])==TRUE)vtx_in[i]=FALSE; // if the grid pt falls inside the hole, reverse "in" status
		      }
		    pptr=pptr->next;					// move onto next hole in this polygedge
		    }
		  }
		}
		
	      // if at least 3 vtxs fall within the patch...
	      if((vtx_in[0]+vtx_in[1]+vtx_in[2]+vtx_in[3]) >=3)
		{
		// get z value of vtxs and add the vtxs to support structure as appropriate
		for(i=0;i<4;i++)
		  {
		  if(vtx_in[i]==TRUE)
		    {
		    patch_find_z(patptr,vtxnew[i]);
		    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[SUPPORT],vtxnew[i],SUPPORT,CLOSE_ENOUGH);
		    if(vtx_ptr!=vtxnew[i]){vertex_destroy(vtxnew[i]); vtxnew[i]=vtx_ptr;}
		    }
		  }
		  
		// determine which vtxs to use for the first facet
		fnew=facet_make();
		if(vtx_in[0]==FALSE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[3]; fnew->vtx[2]=vtxnew[1];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==FALSE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[3]; fnew->vtx[2]=vtxnew[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==FALSE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxnew[3]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==FALSE)
		  {
		  fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
		  }
		  
		// add the first facet to support structure
		facet_unit_normal(fnew);
		for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
		facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);
		patspt->pfacet=facet_list_manager(patspt->pfacet,fnew,ACTION_ADD);
		    
		// add second new upper facet if all 4 vtx are in
		if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew=facet_make();
		  fnew->vtx[0]=vtxnew[2];
		  fnew->vtx[1]=vtxnew[1];
		  fnew->vtx[2]=vtxnew[3];
		  facet_unit_normal(fnew);
		  for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
		  facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);
		  patspt->pfacet=facet_list_manager(patspt->pfacet,fnew,ACTION_ADD);
		  }
		  
		
		// build the LOWER (child) support surface for this upper (parent) patch.
		// make 4 more vtxs that will be the lower compliment to the upper four and add to support structure.
		// add them with z=0 initially so that they default to being on the build table.
		// note that the lower mesh gets a little tricky when it overlaps vertical discontinuities.  if the
		// zdelta between neighboring grid points is greater than the support wall spacing, then the z of the
		// lower z is brought up to prevent the support facets from overlapping into model facets.
		for(i=0;i<4;i++)
		  {
		  vtxlow[i]=vertex_make();				// use make, not copy, to avoid carrying over flist
		  vtxlow[i]->x=vtxnew[i]->x;	      			// lower vtxs have same xy as upper vtxs
		  vtxlow[i]->y=vtxnew[i]->y;
		  vtxlow[i]->z=0.0;					// lower vtxs defualt to build table
		  }
		  
		// scan thru upward facing facets to see if any of the new upper facet vertical vectors intersect with them to
		// establish their correct z.
		ptest=job.lfacet_upfacing[MODEL];				// first item in upward facet list for entir job (all models)
		while(ptest!=NULL)
		  {
		  ftest=ptest->f_item;					// address of upward facing facet
		  minz=BUILD_TABLE_LEN_Z;
		  for(i=0;i<3;i++)					// loop thru all 3 vtxs
		    {
		    if(ftest->vtx[i]->z < minz)minz=ftest->vtx[i]->z;	// get lowest z of facet
		    }
		  
		  for(i=0;i<4;i++)					// loop thru the four vtx of the parent patch facet
		    {
		    if(vtx_in[i]==TRUE && minz<vtxnew[i]->z)		// if the xy was in AND the z of the facet is below the upper z
		      {
		      vtxint->x=vtxlow[i]->x;				// set the temp vtx to their xy values
		      vtxint->y=vtxlow[i]->y;
		      vtxint->z=vtxlow[i]->z;
		      if(facet_contains_point(ftest,vtxint)==TRUE)	// check if they fall within patch, gets z if it does
			{
			if(vtxint->z > vtxlow[i]->z)
			  {
			  vtxlow[i]->z=vtxint->z; 			// if it is the highest intersection... set perm vtx to it
			  vtxlow[i]->attr=ftest->attr;
			  }
			}
		      vtxlow[i]->j=0;					// debug
		      }
		    }
		  
		  ptest=ptest->next;
		  }
		  
		  
		// deal with lower discontinuities by ensuring the lower surface of the support extends beyond the
		// model patch that it lands on (rather than falling short).  this is needed to prevent support from
		// overlapping model geometry at discontinuities.  do this by comparing z values to determine if we
		// hit a discontinuity, then bumping the lower z of the new vtx up to the level of its previous 
		// neighbor.
		
		  
		// add them to support structure only after their new z is established or they will not
		// have connectivity
		for(i=0;i<4;i++)
		  {
		  if(vtx_in[i]==TRUE)
		    {
		    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[SUPPORT],vtxlow[i],SUPPORT,CLOSE_ENOUGH);
		    if(vtx_ptr!=vtxlow[i]){vertex_destroy(vtxlow[i]); vtxlow[i]=vtx_ptr;}
		    }
		  }
      
		// determine which vtxs to use for the first facet
		fnew=facet_make();
		if(vtx_in[0]==FALSE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxlow[2]; fnew->vtx[1]=vtxlow[3]; fnew->vtx[2]=vtxlow[1];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==FALSE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxlow[2]; fnew->vtx[1]=vtxlow[3]; fnew->vtx[2]=vtxlow[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==FALSE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxlow[3]; fnew->vtx[1]=vtxlow[1]; fnew->vtx[2]=vtxlow[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==FALSE)
		  {
		  fnew->vtx[0]=vtxlow[2]; fnew->vtx[1]=vtxlow[1]; fnew->vtx[2]=vtxlow[0];
		  }
		else if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew->vtx[0]=vtxlow[2]; fnew->vtx[1]=vtxlow[1]; fnew->vtx[2]=vtxlow[0];
		  }
		  
		// add the first lower facet to support structure
		facet_unit_normal(fnew);
		for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
		facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);
		pchspt->pfacet=facet_list_manager(pchspt->pfacet,fnew,ACTION_ADD);
      
		    
		// add second new lower facet to support structure
		if(vtx_in[0]==TRUE && vtx_in[1]==TRUE && vtx_in[2]==TRUE && vtx_in[3]==TRUE)
		  {
		  fnew=facet_make();
		  fnew->vtx[0]=vtxlow[2];
		  fnew->vtx[1]=vtxlow[1];
		  fnew->vtx[2]=vtxlow[3];
		  facet_unit_normal(fnew);
		  for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
		  facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);
		  pchspt->pfacet=facet_list_manager(pchspt->pfacet,fnew,ACTION_ADD);
		  }
		
		// free any unused vtxs that did not become part of the support structure
		for(i=0;i<4;i++)
		  {
		  if(vtx_in[i]==FALSE)
		    {
		    vertex_destroy(vtxnew[i]); vtxnew[i]=NULL;
		    vertex_destroy(vtxlow[i]); vtxlow[i]=NULL;
		    }
		  }
		  
		}
	      else 
		{
		// delete all test vtxs since all not in patch
		for(i=0;i<4;i++){vertex_destroy(vtxnew[i]); vtxnew[i]=NULL;}
		}
	      }	// end of make_facet==TRUE decision
	      
	    }	// end of iy increment loop
	  }	// end of ix increment loop
  
	patptr=patptr->next;
	}
      
      // re-associate the entire support facet set which includes upper and lower patches
      facet_find_all_neighbors(mptr->facet_first[SUPPORT]);
      
      mptr=mptr->next;
      }
      
    // clean up
    vertex_destroy(vtxint); vtxint=NULL;
    
    printf("  upper and lower support facets generated.\n");

  }
 
  // step 4 - build vertical facets between upper and lower patches
  // use the lower support facet set as the guide to building the vertical walls.  this helps to
  // build the vertical walls needed between vertical discontinuities of other lower patches.  so
  // this means the algorithm needs to not only connect from the lower patch to the upper patch,
  // but to other lower patches as well.
  {
    sprintf(scratch,"Creating wall support facets");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);
    amt_done=0;
    amt_current=0;
    vtxD=vertex_make();
  
    // loop thru all models in the job that request support
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot<0 || slot>=MAX_TOOLS){mptr=mptr->next; continue;}		// if this model does not require support... skip it.
      
      amt_total=mptr->facet_qty[SUPPORT];

      patptr=mptr->patch_first_upr;					// start with first upper support patch
      while(patptr!=NULL)						// loop thru all upper support patches
	{
	pchptr=patptr->pchild;						// get lower support patch (child patch)
	if(pchptr==NULL){patptr=patptr->next;continue;}			// ensure there is at least one bottom patch to connect with
	pfptr=patptr->pfacet;						// start with first facet in facet list of this patch
	while(pfptr!=NULL)						// loop thru all facets in this facet list of this patch
	  {
	  sfptr=pfptr->f_item;						// get actual facet address
	  
	  // update status to user
	  amt_current++;
	  amt_done = (double)(amt_current)/(double)(amt_total);
	  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	  while (g_main_context_iteration(NULL, FALSE));
      
	  // check if upper facet has a null neighbor edge.
	  // if so, build a pair of wall facets between it and the same edge of the lower facet. 
	  // do this by checking all neighbors of current facet.  if one is null, then that's a free edge
	  // that needs a vertical wall built from it.
	  for(i=0;i<3;i++)
	    {
	    //fngh=facet_find_vtx_neighbor(mptr->facet_first[SUPPORT],sfptr,sfptr->vtx[i],sfptr->vtx[h]);
	    fngh=sfptr->fct[i];
	    if(fngh==NULL)
	      {
	      // at this point we know that sfptr is a free edge facet.  and we know the free edge
	      // exists between vtx[i] and vtx[h] on this upper support facet.  now we need to find
	      // the matching free edge vtxs in the matching lower support facet.
	      h=i+1; if(h>2)h=0;
	      vtxyA=sfptr->vtx[i]; 
	      vtxyB=sfptr->vtx[h];
	      vtxyC=NULL;
	      vtxyD=NULL;
	      //Czmax=0; Dzmax=0;
	      Czmax=vtxyA->z; Dzmax=vtxyB->z;
	      
	      // search facets in child patch for matching vtxs. if two vtx in the child patch share the
	      // same xy, use the vtx with the lowest z.
	      pfnxt=pchptr->pfacet;
	      while(pfnxt!=NULL)
		{
		fptr=pfnxt->f_item;
		for(j=0;j<3;j++)
		  {
		  if(vertex_xy_compare(vtxyA,fptr->vtx[j],CLOSE_ENOUGH)==TRUE)
		    {
		    if(vtxyC!=NULL)vtxyC->attr=9;						// if already found one before, flag it as being at same xy
		    if(fptr->vtx[j]->z < vtxyA->z && fptr->vtx[j]->z < Czmax)			// find lowest vtx at this xy location
		      {
		      Czmax=fptr->vtx[j]->z; 
		      vtxyC=fptr->vtx[j];
		      }	
		    }
		  if(vertex_xy_compare(vtxyB,fptr->vtx[j],CLOSE_ENOUGH)==TRUE)
		    {
		    if(vtxyD!=NULL)vtxyD->attr=9;						// if already found one before, flag it as being at same xy
		    if(fptr->vtx[j]->z < vtxyB->z && fptr->vtx[j]->z < Dzmax)			// find lowest vtx at this xy location
		      {
		      Dzmax=fptr->vtx[j]->z; 
		      vtxyD=fptr->vtx[j];
		      }
		    }
		  }
		pfnxt=pfnxt->next;
		}
	      
	      //printf("  free edge: Ax=%6.3f Bx=%6.3f Cx=%6.3f Dx=%6.3f \n",vtxyA->x,vtxyB->x,vtxyC->x,vtxyD->x);
	      //printf("             Ay=%6.3f By=%6.3f Cy=%6.3f Dy=%6.3f \n",vtxyA->y,vtxyB->y,vtxyC->y,vtxyD->y);
	      //printf("             Az=%6.3f Bz=%6.3f Cz=%6.3f Dz=%6.3f \n",vtxyA->z,vtxyB->z,vtxyC->z,vtxyD->z);
	      
	      if(vtxyA!=NULL && vtxyB!=NULL && vtxyC!=NULL && vtxyD!=NULL)
		{
		// build two new vertical facets between sfptr and fnew and add to support list from
		// the two free edge vtxs.
		fnew=facet_make();						// allocate memory and set default values for facet
		vtxyA->flist=facet_list_manager(vtxyA->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		fnew->vtx[0]=vtxyA;						// assign which vtxs belong to this facet
		fnew->vtx[1]=vtxyB;
		fnew->vtx[2]=vtxyC;
		facet_unit_normal(fnew);
		facet_area(fnew);
		fnew->status=4;							// flag this as a lower facet
		fnew->member=sfptr->member;					// keep track of which body it belongs to
		facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);	// adds new facet to support facet list
		
		// make new facet between vtxB, vtxC, and vtxD
		fnew=facet_make();						// allocate memory and set default values for facet
		vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		vtxyD->flist=facet_list_manager(vtxyD->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
		fnew->vtx[0]=vtxyB;						// assign which vtxs belong to this facet
		fnew->vtx[1]=vtxyD;
		fnew->vtx[2]=vtxyC;
		facet_unit_normal(fnew);
		facet_area(fnew);
		fnew->status=4;							// flag this as a lower facet
		fnew->member=sfptr->member;					// keep track of which body it belongs to
		facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);	// adds new facet to support facet list
		}
	      }		// end of if neighbor NULL
	    }		// end of for i=...
	   
	  pfptr=pfptr->next;
	  }
	patptr=patptr->next;
	}
	
      // set support bounds to same as model bounds
      mptr->xmin[SUPPORT]=mptr->xmin[MODEL];  mptr->xmax[SUPPORT]=mptr->xmax[MODEL];
      mptr->ymin[SUPPORT]=mptr->ymin[MODEL];  mptr->ymax[SUPPORT]=mptr->ymax[MODEL];
      mptr->zmin[SUPPORT]=mptr->zmin[MODEL];  mptr->zmax[SUPPORT]=mptr->zmax[MODEL];
      mptr->xorg[SUPPORT]=mptr->xorg[MODEL];
      mptr->yorg[SUPPORT]=mptr->yorg[MODEL];
      mptr->zorg[SUPPORT]=mptr->zorg[MODEL];
      mptr->xoff[SUPPORT]=mptr->xoff[MODEL];
      mptr->yoff[SUPPORT]=mptr->yoff[MODEL];
      mptr->zoff[SUPPORT]=mptr->zoff[MODEL];
  
      // re-associate the entire support facet set 
      facet_find_all_neighbors(mptr->facet_first[SUPPORT]);
      
      mptr=mptr->next;
      }

    // clean up
    vertex_destroy(vtxD); vtxD=NULL;
    
    printf("  wall support facets made.\n");
  }

  /*
  // step 5 - adjust for discontinuities
  // scan thru the newly created support facets and find neighboring vtxs that have large z differences.
  // raise the lower vtx z's up to the level of their neighbor's z.  this reduces interference with model
  // facets and makes for cleaner removal of the support material after printing.
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    // init status
    sfptr=mptr->facet_first[SUPPORT];
    while(sfptr!=NULL)
      {
      sfptr->status=0;							// use as flag to indicate if processed already
      sfptr=sfptr->next;
      }
	
    // check z levels and adjust
    sfptr=mptr->facet_first[SUPPORT];
    while(sfptr!=NULL)
      {
      avg_z=((sfptr->vtx[0])->z + (sfptr->vtx[1])->z + (sfptr->vtx[2])->z)/3;	// avg z of current facet
      for(i=0;i<3;i++)
        {
	avg_n[i]=((sfptr->fct[i]->vtx[0])->z + (sfptr->fct[i]->vtx[1])->z + (sfptr->fct[i]->vtx[2])->z)/3;	// avg z of each neighboring facet
	if((avg_z - avg_n[i])>1.0 && (sfptr->fct[i])->status==0)		// if current is higher than neghbor and not yet adjusted...
	  {
	  (sfptr->fct[i]->vtx[0])->z = avg_z;					// ... raise neighbor
	  (sfptr->fct[i])->status = 1;						// ... flag as adjusted
	  }
	}
      sfptr=sfptr->next;
      }
    
    mptr=mptr->next;
    }
  */
  
  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    // build edge list for support
    edge_display(mptr,set_view_edge_angle,SUPPORT);			// determine which sharp edges to display in 3D view

    printf("  %d  model facet qty = %d    support facet qty =%d \n",mptr->model_ID,mptr->facet_qty[MODEL],mptr->facet_qty[SUPPORT]);
    mptr=mptr->next;
    }
  printf("\nJob Build Support:  exit \n");
  return(1);
}



// Fucntion to generate TREE type support structure for a job.
//
// Basic order of events:
// 1.  Identify model facets that will require support and turn them into upper patches.
// 2.  Generate upper nodes across the surface of each upper patch.
// 3.  Generate lower goal nodes on landing surfaces.
// 5.  Grow centerlines of tree branches from nodes downward toward goal nodes using A* alogrithm.
// 5.  Sweep facet circles down each centerline to build braches with facets.

//
int job_build_tree_support(void)
{
  int		debug_ctr=0;
  int		status=TRUE,match,make_node;
  int		h,i,j,slot,add_spt;
  int	 	branch_vtx_cnt=8;
  int		temp_branch_cnt=0,tree_branch_cnt=0;
  int		patch_cnt,mfacet_dn_cnt,mfacet_up_cnt;
  int		is_vtx_in;
  int		grid_xcnt,grid_ycnt,ixcnt,ix,iycnt,iy;
  
  float 	dx,dy,dz,xy_dist,z_dist,z_trans,xy_pctr,scale;
  float 	minx,miny,minz,maxx,maxy,maxz;
  float 	grid_xinc,grid_yinc,grid_zinc;
  float 	branch_diam=2.0,spt_perim_pitch=2.0,spt_grid_pitch=5.0;
  float 	effective_diam;
  float 	node_dist,merge_dist,max_center_dist;
  float 	old_goal_z,old_brptr_z;
  float 	theta,sin_max_angle,cos_max_angle,tan_max_angle;
  float 	max_angle,vz0,vz1,vz2,avg_z;
  float 	weight_merge,weight_center;
  float 	score_merge,score_center,score_total,score_best;
  float 	vtx_dist,crt_dist,rem_dist;
  double 	amt_done=0,amt_current=0,amt_total=0;
  
  vertex	*vptr,*vnxt,*vtxint,*vtx_new,*vtx_old,*vtx_first,*vtx_best;
  vertex 	*vtx_temp,*vtx_dir,*vtx_up,*vtx_origin,*vtx_x_axis,*vtx_y_axis;
  vertex 	*vptr2,*vnxt2,*vcirc,*vtx_int,*vrtn,*vtx_pctr;
  vector 	*vec_x_axis,*vec_y_axis,*vec_branch;
	    
  polygon 	*pptr,*pnxt,*pnew,*pold,*pfirst;
  polygon 	*polyedge;
  polygon 	*branch_ptr,*branch_first;

  facet 	*mfptr,*test_fptr,*fptr,*fnew;
  patch		*patptr,*patmain,*pat_last,*patdel,*patold;
  facet_list	*pfptr,*pfnxt,*pfset,*ptest;
  
  model 	*mptr;
  
  branch 	*temp_tree,*brptr,*brold,*brdel,*new_branch,*old_branch;
  branch 	*nbrptr,*nbrtest,*nbrpre,*nbrbest;
  branch_list	*bl_ptr,*bl_neighbor_list;
  
  // determine if user is requesting support
  {
    add_spt=FALSE;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot>=0 && slot<MAX_TOOLS)add_spt=TRUE;			// if a viable tool is specified...
      mptr=mptr->next;
      }
    printf("Support Check:  slot=%d \n",slot);
    if(add_spt==FALSE)return(FALSE);
  }

  // initialize
  {
    // loop thru all models in this job.  this is a job level function since the user has the ability to "stack"
    // models above each other which makes them codependent regarding supports.  so even if the "under" model does
    // not have support requested, the "above" model may have its supports landing on "under's" upper facing facets.
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      // clear existing model and support patches if they exist
      if(mptr->patch_model!=NULL)
	{
	patptr=mptr->patch_model;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);						// this will delete child patches as well
	  }
	mptr->patch_model=NULL;
	}
      if(mptr->patch_first_lwr!=NULL)
	{
	patptr=mptr->patch_first_lwr;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);
	  }
	mptr->patch_first_lwr=NULL;
	}
      if(mptr->patch_first_upr!=NULL)
	{
	patptr=mptr->patch_first_upr;
	while(patptr!=NULL)
	  {
	  patdel=patptr;
	  patptr=patptr->next;
	  patch_delete(patdel);
	  }
	mptr->patch_first_upr=NULL;
	}
    
      // clear existing support facets if they exist
      if(mptr->facet_first[SUPPORT]!=NULL)
	{
	facet_purge(mptr->facet_first[SUPPORT]);
	mptr->facet_first[SUPPORT]=NULL;  mptr->facet_last[SUPPORT]=NULL; mptr->facet_qty[SUPPORT]=0;
	vertex_purge(mptr->vertex_first[SUPPORT]);
	mptr->vertex_first[SUPPORT]=NULL; mptr->vertex_last[SUPPORT]=NULL; mptr->vertex_qty[SUPPORT]=0;
	}
	
      // set all model facet and vertex attributes to 0.  this is needed because we'll be using the attr value as a
      // status check in the subsequent operation.
      mfptr=mptr->facet_first[MODEL];
      while(mfptr!=NULL)
	{
	mfptr->status=0;
	mfptr->attr=0;
	for(j=0;j<3;j++)
	  {
	  if(mfptr->vtx[j]==NULL){printf("  facet=%X has null vtx at %d \n",mfptr,j);}
	  else {(mfptr->vtx[j])->attr=0; (mfptr->vtx[j])->supp=0;}
	  }
	mfptr=mfptr->next;
	}
  
      amt_total += mptr->facet_qty[MODEL];				// accrue total number of facets in job to examine
      mptr=mptr->next;
      }
    
    // purge support tree branch list if it exists
    if(job.support_tree!=NULL)
      {
      brptr=job.support_tree;
      while(brptr!=NULL)
        {
	brdel=brptr;
	brptr=brptr->next;
	if(brdel->node!=NULL){vertex_destroy(brdel->node);}
	if(brdel->goal!=NULL){vertex_destroy(brdel->goal);}
	free(brdel);
	}
      job.support_tree=NULL;
      }
    
    // purge the upward facing facet list
    job.lfacet_upfacing[MODEL]=facet_list_manager(job.lfacet_upfacing[MODEL],NULL,ACTION_CLEAR);
    job.lfacet_upfacing[MODEL]=NULL;
      
    //printf("TreeSupport:  Initialization complete.\n");
  }


  // Step 1 - Identify model facets that will need support, and those that can have support land
  //	      on them.
  {
    //printf("TreeSupport:  Identifying patches... \n");
    // show the progress bar
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Identifying areas that need support  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_widget_set_visible(win_info,TRUE);
    
    amt_done=0; amt_current=0;
    patch_cnt=0; mfacet_dn_cnt=0; mfacet_up_cnt=0;
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      // check if supports are requested for this model by checking its operations list
      slot=(-1);							// default no tool selected
      cos_max_angle=cos(-45*PI/180);					// default if no tool selected yet
      sin_max_angle=sin(-45*PI/180);					// default if no tool selected yet
      tan_max_angle=tan(-45*PI/180);					// default if no tool selected yet
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot>=0 && slot<MAX_TOOLS)					// if a viable tool is specified...
	{
	cos_max_angle=cos(Tool[slot].matl.overhang_angle*PI/180);	// ... puts in terms of normal vector value (note, this is a negative value)
	sin_max_angle=sin(Tool[slot].matl.overhang_angle*PI/180);	// ... puts in terms of normal vector value (note, this is a negative value)
	tan_max_angle=tan(Tool[slot].matl.overhang_angle*PI/180);	// ... puts in terms of normal vector value (note, this is a negative value)
	mptr->slice_thick=Tool[slot].matl.layer_height;			// ... reset to keep in sync
	}

      // loop thru all facets of this model and determine which need support, or which could have
      // support land on them.
      h=0;
      pat_last=NULL;
      mfptr=mptr->facet_first[MODEL];
      while(mfptr!=NULL)
	{
	// update status to user
	amt_current++;
	amt_done = (double)(amt_current)/(double)(amt_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
      
	h++;								// increment facet counter for this patch
	//printf("  mptr=%X  facet=%X  h=%d \n",mptr,mfptr,h);
	
	// skip any model facets that have already been added to a patch
	// status meaning:  0=not yet checked, 1=checked and not added to patch, 2=checked and added to patch
	if(mfptr->status>0)
	  {
	  mfptr=mfptr->next; 
	  continue;
	  }
	
	// only add facet to upward facing list if not vertical 
	if((mfptr->unit_norm)->z > TOLERANCE)
	  {
	  mfacet_up_cnt++;
	  //printf("adding facet %X to upward list %X \n",mfptr,job.lfacet_upfacing[MODEL]);
	  job.lfacet_upfacing[MODEL]=facet_list_manager(job.lfacet_upfacing[MODEL],mfptr,ACTION_ADD);
	  mfptr->status=1;
	  mfptr=mfptr->next; 
	  continue;
	  }
    
	// skip facets that are too close to the build table.  we cannot add support to anything
	// thinner than a slice thickness.
	vz0=mfptr->vtx[0]->z;
	vz1=mfptr->vtx[1]->z;
	vz2=mfptr->vtx[2]->z;
	avg_z=(vz0+vz1+vz2)/3;
	if(avg_z<=mptr->slice_thick)
	  {
	  mfptr->status=1; 
	  mfptr=mfptr->next; 
	  continue;
	  }	
      
	// include downward facing facets that are more shallow than max angle which is defined by the material
	// properties of the support material.  this needs to happen before checking for a minimum vtx otherwise
	// we'll get redundant supports.
	if((mfptr->unit_norm->z*mfptr->unit_norm->z)>fabs(cos_max_angle))
	  {
	  mfacet_dn_cnt++;						// increment count of downward facets found
	  
	  // now that a seed facet is identified, create a new patch.
	  patch_cnt++;							// increment our patch count
	  patptr=patch_make();						// make a new patch
	  patptr->ID=patch_cnt;						// assign ID based on count.  this is basically its body ID
	  if(mptr->patch_model==NULL)mptr->patch_model=patptr;		// if first one, define as head of list
	  if(pat_last!=NULL)pat_last->next=patptr;			// if not first, add linkage
	  pat_last=patptr;						// save address for link next time thru loop
	  
	  // add the seed facet to the new patch.  note that patch_add_facet only creates a
	  // pointer to the model facet... it does NOT create another facet.
	  mfptr->status=2;						// set status of facet to "added to patch"
	  mfptr->member=patch_cnt;
	  patptr->pfacet=facet_list_manager(patptr->pfacet,mfptr,ACTION_ADD);
	  patptr->area = mfptr->area;
    
	  // loop thru list of facets already added to the patch and check it's three neighbors 
	  // for any that have not yet been checked.  if not checked, see if they should be added.
	  test_fptr=mfptr;						// set to seed facet at entry
	  while(test_fptr!=NULL)					// loop until no other facets qualify
	    {
	    //printf("   test_fptr=%X \n",test_fptr);
	    for(i=0;i<3;i++)						// loop thru all 3 neighbors of test_fptr facet
	      {
	      fptr=test_fptr->fct[i];					// set facet to neighbor of seed facet
	      if(fptr!=NULL)						// if a facet exists ...
		{
		if(fptr->status==0)					// ... if not yet checked
		  {
		  fptr->status=1;					// ... set to checked
		  if((fptr->unit_norm)->z < (0-TOLERANCE))		// ... if facing downward
		    {
		    vz0=fptr->vtx[0]->z;
		    vz1=fptr->vtx[1]->z;
		    vz2=fptr->vtx[2]->z;
		    avg_z=(vz0+vz1+vz2)/3;
		    if(avg_z>(3*mptr->slice_thick))					// ... if z is sufficiently off of the build table
		      {
		      // if steeper than material allows... add it to the patch
		      if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(cos_max_angle))
			{
			fptr->member=patptr->ID;					// ... identify which patch it went to
			fptr->status=2;							// ... set status to "added to patch"
			patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD); // ... add it to the patch's facet list
			patptr->area += fptr->area;					// ... accrue the patch area
			}
		      }
		    }
		  }	// end of if(fptr->status==0)
		}	// end of if(fptr!=NULL)
	      }		// end of for loop

	    // find a facet in this patch that has a neighbor that has not been checked.  note this will
	    // eventually check all facets within the bounds of our conditions (i.e. downward, above zero,
	    // and material angle) along with the ring of facets (neighbors) around the patch that are
	    // the ones that fail the conditions.
	    test_fptr=NULL;						// undefine the seed facet
	    pfptr=patptr->pfacet;					// starting with first facet in this patch
	    while(pfptr!=NULL)						// loop thru all facets in this patch
	      {
	      fptr=pfptr->f_item;					// get address of actual facet
	      if(fptr!=NULL)						// if valid ...
		{
		if(fptr->fct[0]->status==0){test_fptr=fptr;break;}  	// ... if 1st neighbor not checked set as seed
		if(fptr->fct[1]->status==0){test_fptr=fptr;break;}    	// ... if 2nd neighbor not checked set as seed
		if(fptr->fct[2]->status==0){test_fptr=fptr;break;}    	// ... if 3rd neighbor not checked set as seed
		}
	      pfptr=pfptr->next;
	      }
	    }
	  }	// end of if overhang angle requires support for this material
	  
	mfptr=mfptr->next;
	}		// end of main facet loop
  
      mptr=mptr->next;
      }
      
    // this section now finds the perimeter vertex list for each patch.  that is, it determines the list of vtxs
    // that form the free edge(s) around and inside (holes) for each patch and create a 3D polygon(s) out of them.
    // this same function also establishes the minz and maxz of the patch.  that is, the highest and lowest vtxs in it.
    //printf("TreeSupport:  finding patch free edge...\n");
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      patptr=mptr->patch_model;
      while(patptr!=NULL)
	{
	patch_find_edge_facets(patptr);					// flag facets and their vtxs that will need perimeter support
	patch_find_free_edge(patptr);					// create 3D polygons of this patch's free edge
	patptr=patptr->next;
	}
      mptr=mptr->next;
      }
      

    //printf("TreeSupport:  Step 1 - upper patches and upward facing facets complete.\n");
  }

  
  // Step 2 - Generate nodes from upper patches.  Nodes serve as the contact points between model
  //          and support material.  As a general rule, they should be spaced close enough for
  //          the model material to bridge with minimal sagging but allow for clean break-away.
  //
  //	      These nodes will define the start points (vtxs) of branches.  each branch will grow
  //	      downward toward a goal (which may be the build table for some, or other branch nodes
  //	      for others).
  {
    // initialize
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Indentifying contact nodes  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    
    amt_done=0;
    amt_current=0;
    amt_total=patch_cnt;
    temp_branch_cnt=0;
    temp_tree=NULL;
    old_branch=NULL;
    vtxint=vertex_make();
    
    // loop thru all models in the job that request support
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot<0 || slot>=MAX_TOOLS){mptr=mptr->next; continue;}		// if this model does not require support... skip it.

      // loop thru each patch and apply where perimeter and grid where nodes will land
      patold=NULL;
      patptr=mptr->patch_model;						// start with first upper patch
      while(patptr!=NULL)						// loop thru all found in model
	{
	// update status to user
	amt_current++;
	amt_done = (double)(amt_current)/(double)(amt_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
	
	// ensure this patch has its free edge defined
	if(patptr->free_edge==NULL){printf("\nERROR - patch free edge not defined!\n\n"); patptr=patptr->next; continue;}
  
	// this currently just uses min/max of the patch to get support up to edge of the patch, but this only
	// works well for square patches.  this out to be drawing a ray across the patch at each ix value to
	// get the min/max of y so that odd shaped polygons can be supported properly.
	
	minx=patptr->minx; maxx=patptr->maxx;
	miny=patptr->miny; maxy=patptr->maxy;
	
	/*
	// DEBUG
	// build node outline around free edge
	pptr=patptr->free_edge;
	vptr=pptr->vert_first;
	while(vptr!=NULL)
	  {
	  vtx_new=vertex_copy(vptr,NULL);
	  temp_branch_cnt++;						// increment branch counter
	  new_branch=branch_make();					// create a new support branch
	  new_branch->node=vtx_new;					// define head node
	  new_branch->patbr=patptr;					// define patch it belongs to
	  if(temp_tree==NULL)temp_tree=new_branch;			// if temporary support tree not yet defined assign this as first branch
	  if(old_branch!=NULL)old_branch->next=new_branch;		// if other branches do exist, add link
	  old_branch=new_branch;					// save address for next time thru loop
	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  }
	*/

	// build node outline of nodes around perimeter of patch at set interval
	pptr=patptr->free_edge;							// start with outer most patch free edge (i.e this outlines material, all rest outline holes)
	while(pptr!=NULL)							// then loop thru all the holes for this patch as well
	  {
	  rem_dist=0.0;
	  vptr=pptr->vert_first;
	  while(vptr!=NULL)
	    {
	    vnxt=vptr->next;
	    if(vnxt==NULL)break;
	    
	    // determine if a perimeter node is needed based on boundary facet orientation
	    // this info must be derived at the time when the free edge is made otherwise subsequent
	    // polygons vtxs won't match the facet vtxs due to min length and smoothing of polys.
	    make_node=FALSE;
	    if(vptr->supp==1 && vnxt->supp==1)make_node=TRUE;
	    
	    dx=vnxt->x - vptr->x;
	    dy=vnxt->y - vptr->y;
	    dz=vnxt->z - vptr->z;
	    vtx_dist=sqrt(dx*dx + dy*dy + dz*dz);
	    while((vtx_dist + rem_dist) >= spt_perim_pitch) 
	      {
	      crt_dist = spt_perim_pitch - rem_dist;
	      scale = crt_dist / vtx_dist;
	      vtx_new=vertex_make();
	      vtx_new->x = vptr->x + dx * scale;				// interpolate point
	      vtx_new->y = vptr->y + dy * scale;
	      vtx_new->z = vptr->z + dz * scale;
	  
	      if(make_node==TRUE)						// if support perimeter is needed here...
		{
		temp_branch_cnt++;						// increment branch counter
		new_branch=branch_make();					// create a new support branch
		new_branch->node=vtx_new;					// define head node
		new_branch->patbr=patptr;					// define patch it belongs to
		if(temp_tree==NULL)temp_tree=new_branch;			// if temporary support tree not yet defined assign this as first branch
		if(old_branch!=NULL)old_branch->next=new_branch;		// if other branches do exist, add link
		old_branch=new_branch;						// save address for next time thru loop
		}
	    
	      vtx_dist -= crt_dist;						// update remaining segment length
	      if(vtx_dist>0.0) 
		{
		dx = vnxt->x - vtx_new->x;
		dy = vnxt->y - vtx_new->y;
		dz = vnxt->z - vtx_new->z;
		}
		
	      if(make_node==FALSE)						// if support was not neeeded, delete working vtx as it is not used as a node
		{
		if(vtx_new!=NULL){vertex_destroy(vtx_new); vtx_new=NULL;}
		}
		
	      rem_dist = 0.0;							// reset since new vtx just added
	      }
	    rem_dist += vtx_dist;						// not enough segment left to place another vertex ... save remainder
  
	    vptr = vptr->next;
	    if(vptr==pptr->vert_first)break;
	    }
	  
	  pptr=pptr->next;							// increment to next free edge polygon
	  }
	pptr=patptr->free_edge;							// reset back to outer looop for interior grid generation
	
      
/*
	// build node outline of nodes around perimeter of patch at set interval
	vtx_dist=0.0;
	crt_dist=0.0;
	pptr=patptr->free_edge;
	vptr=pptr->vert_first;
	while(vptr!=NULL)
	  {
	  vnxt=vptr->next;
	  if(vnxt==NULL)break;
	  vtx_dist=vertex_distance(vptr,vnxt);				// get 3D dist bt verts
	  while((vtx_dist+rem_dist)>=spt_perim_pitch)			// 
	    {
	    crt_dist = spt_perim_pitch - rem_dist;
	    vtx_new=vertex_make();
	    vertex_3D_interpolate(crt_dist,vptr,vnxt,vtx_new);
	    temp_branch_cnt++;						// increment branch counter
	    new_branch=branch_make();					// create a new support branch
	    new_branch->node=vtx_new;					// define head node
	    new_branch->patbr=patptr;					// define patch it belongs to
	    if(temp_tree==NULL)temp_tree=new_branch;			// if temporary support tree not yet defined assign this as first branch
	    if(old_branch!=NULL)old_branch->next=new_branch;		// if other branches do exist, add link
	    old_branch=new_branch;					// save address for next time thru loop
	    rem_dist=0.0;
	    vtx_dist -= crt_dist;					// increment down length of segment
	    }

	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  }
*/
	
	// build node grid across bulk area of patch
	grid_xinc=spt_grid_pitch;					// set node spacing
	grid_yinc=grid_xinc;
	grid_xcnt=floor((maxx-minx)/grid_xinc);				// calc count of steps in x less one
	grid_ycnt=floor((maxy-miny)/grid_yinc);				// calc count of steps in y less one
	minx += 0.1;
	miny += 0.1;

/*	
	// sweep across x from start to stop
	for(ixcnt=0;ixcnt<=grid_xcnt;ixcnt++)
	  {
	  ix=minx+ixcnt*grid_xinc;					// calc new x position
	  dx=grid_xinc;							// reset increment
	  if(ixcnt==grid_xcnt)dx=maxx-ix-0.1;				// if at final edge, change increment to remainder
	    
	  // sweep across y from start to stop
	  for(iycnt=0;iycnt<=grid_ycnt;iycnt++)
	    {
	    iy=miny+iycnt*grid_yinc;					// calc new y position
	    dy=grid_yinc;						// reset increment
	    if(iycnt==grid_ycnt)dy=maxy-iy-0.1;				// if at final edge, change increment to remainder
	    
	    // create a test vtx that we'll use to check if it's within patch and not inside holes of that patch
	    vtx_new=vertex_make(); 					// vtx at current increment
	    vtx_new->x=ix;           
	    vtx_new->y=iy;
	    
	    // since our xy grid is sweeping in a min/max square over this patch, we need to check that
	    // each new vtx coord is acutally within the odd shaped polygon of that patch.  this also
	    // accounts for the edge condition to get that last vtx near the edge of the polygon right
	    // onto its edge.
	    polyedge=patptr->free_edge;					// get address of primary free edge of this patch
	    is_vtx_in=polygon_contains_point(polyedge,vtx_new);		// check if this vtx is within the patch
	    if(is_vtx_in==TRUE)						// if it does fall inside the outer boundary of the patch...
	      {
	      // check if it falls within a hole
	      pptr=polyedge->next;					// starting with the first hole (if there)
	      while(pptr!=NULL)						// loop thru all holes in this polyedge
		{
		if(pptr->hole>0)					// if this polygon is a hole of any type
		  {
		  if(polygon_contains_point(pptr,vtx_new)==TRUE)is_vtx_in=FALSE; // if the grid pt falls inside the hole, reverse "in" status
		  }
		pptr=pptr->next;					// move onto next hole in this polygedge
		}
	      }
	      
	    // if this vtx DOES fall within the patch and NOT on a hole...
	    if(is_vtx_in==TRUE)
	      {
	      temp_branch_cnt++;						// increment branch counter
	      
	      // get z value of vtx and add the vtx to node list for this patch
	      patch_find_z(patptr,vtx_new);				// get z based on x y location
	      new_branch=branch_make();					// create a new support branch
	      new_branch->node=vtx_new;					// define head node
	      new_branch->patbr=patptr;					// define patch it belongs to
	      if(temp_tree==NULL)temp_tree=new_branch;			// if temporary support tree not yet defined assign this as first branch
	      if(old_branch!=NULL)old_branch->next=new_branch;		// if other branches do exist, add link
	      old_branch=new_branch;					// save address for next time thru loop
	      }
	      
	    }	// end of iy increment loop
	  }	// end of ix increment loop
*/
	patptr=patptr->next;
	}
	  
      mptr=mptr->next;
      }
      
    // clean up
    vertex_destroy(vtxint); vtxint=NULL;
    
  }	// end of step  2
  
  
  // Step 3 - Generate goal nodes.  These are the lower nodes that the upper nodes are trying to
  //	      reach at the lowest "cost" possible.
  {

    // initialize
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Indentifying goal nodes  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    
    amt_done=0;
    amt_current=0;
    amt_total=temp_branch_cnt;
    
    vtx_temp=vertex_make();

    // loop thru all upper nodes and generate a matching lower node
    brold=NULL;
    brdel=NULL;
    brptr=temp_tree;
    while(brptr!=NULL)
      {
      if(brptr->goal==NULL)						// if branch does not have a goal...
	{
	// update status to user
	amt_current++;
	amt_done = (double)(amt_current)/(double)(amt_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
	
	// grow branch to next node by looking at closest neighboring nodes at the z increment
	// that allows material growth within the angular limits of the material.  this will
	// basically assign the "goal node" from which cost estimate can be calculated.  "cost"
	// in our euqation is basically length.  we want the shortest length.
	
	// first just make the goal node in the same xy location but at z=0
	brptr->goal=vertex_copy(brptr->node,NULL);
	brptr->goal->z=0.0;
	
	// load test vtx with xyz coords to test against upward facets
	vtx_temp->x=brptr->goal->x;
	vtx_temp->y=brptr->goal->y;
	vtx_temp->z=brptr->goal->z;
	
	// find the highest upward facing facet that the ray bt node and goal pierces
	pfptr=job.lfacet_upfacing[MODEL];
	while(pfptr!=NULL)							// loop thru all facets that are upward facing in this job
	  {
	  fptr=pfptr->f_item;							// get address of actual facet
	  if(fptr!=NULL)							// if valid ...
	    {
	    // if goal is within xy bounds of fptr, that function will asign z of intersection to goal
	    if(facet_contains_point(fptr,vtx_temp)==TRUE)			// loads goal with z at intersection
	      {
	      if(vtx_temp->z<brptr->node->z)					// if below the branch node...
	        {
		if(vtx_temp->z>brptr->goal->z)brptr->goal->z=vtx_temp->z;	// ... and above the current branch goal, set goal to it
		}
	      }
	    }
	  pfptr=pfptr->next;
	  }
	  
	brptr->status=0;
	
	brptr->node->i = 0.0;						
	brptr->node->j = 0.0;						
	brptr->node->k = 0.0;
	
	brptr->goal->i = 0.0;						// distance this branch is away from model surface based on patch free edge polygon
	brptr->goal->j = 0.0;						// z value of highest node in branch (i.e. node that contacts underside of model)
	brptr->goal->k = 0.0;						// trunk weight - number of branches that have merged to this goal
	}
    
      brold=brptr;							// save address for next pass thru loop
      brptr=brptr->next;						// move onto next element in list
      }
      
    // ideal goal nodes are those located centrally under an upper patch.  this allows many smaller
    // branches to merge together into a central trunk, not unlike a real tree.  to do this we'll 
    // weight the central goal nodes with their xy distance to a model surface.
    // loop thru all models in the job that request support
    vrtn=vertex_make();
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      // loop thru each patch and find goal nodes that land within
      patptr=mptr->patch_model;						// start with first upper patch
      while(patptr!=NULL)						// loop thru all found in model
	{
	brptr=temp_tree;						
	while(brptr!=NULL)
	  {
	  if(brptr->patbr == patptr)
	    {
	    // since we are only using the first free edge in the linked list of free edges for this patch
	    // we'll be ignoring all "sub free edges" (aka "holes).  this will allow tree trunks to actually
	    // locate with holes as needed.
	    pptr=polygon_copy(patptr->free_edge);			// make a copy of the free edge poly
	    vptr=pptr->vert_first;
	    while(vptr!=NULL)
	      {
	      vptr->z=brptr->goal->z;					// set all z values to same as branch goal
	      vptr=vptr->next;
	      if(vptr==pptr->vert_first)break;
	      }
	    brptr->goal->i=fabs(polygon_to_vertex_distance(pptr,brptr->goal,vrtn));
	    //if(brptr->goal->i < 0)brptr->goal->i=0;
	    polygon_free(pptr);
	    }
	  brptr=brptr->next;
	  }
	patptr=patptr->next;
	}
      mptr=mptr->next;
      }
    if(vrtn!=NULL){vertex_destroy(vrtn); vrtn=NULL;}
     
    // clean up
    if(vtx_temp!=NULL){vertex_destroy(vtx_temp); vtx_temp=NULL;}
    
    // sort the branch linked list by highest z value. highest z value is first in list.
    branch_sort(temp_tree);
    
  }	// end of step 3
  
  // Debug
  /*
  printf("\n  Sorted root node list: \n");
  i=0;
  brptr=temp_tree;
  while(brptr!=NULL)
    {
    printf(" %d  brptr:  %X status=%d next=%X\n",i,brptr,brptr->status,brptr->next); 
    printf("      node:  %X x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->node, brptr->node->x,brptr->node->y,brptr->node->z,brptr->node->i,brptr->node->j,brptr->node->k);
    printf("      goal:  %X x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->goal, brptr->goal->x,brptr->goal->y,brptr->goal->z,brptr->goal->i,brptr->goal->j,brptr->goal->k);
    i++;
    brptr=brptr->next;
    }
  while(!kbhit());
  */
  
  
  // At this point we have a collection of branches that contian start nodes that are underneath
  // model material and corrisponding goal nodes which are either on the build table or upward facing
  // model material below them.  They are sorted by lowest node->k to highest node->k.
  
  
  // Step 4 - Grow branches of support tree
  {
    
    // initialize
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Growing branches  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    
    amt_done=0;
    amt_current=0;
    amt_total=temp_branch_cnt;
    
    grid_zinc=1.0;							// nominal z growth amount
    z_trans = spt_grid_pitch * fabs(tan_max_angle) * 4.5;		// distance away from model surface to focus on centering trunk under patch
    vtx_pctr=vertex_make();						// reusable vtx for holding patch centroid location
    
    // first grow all contact nodes by one grid increment downward to facilitate break-away.
    // note that the head node of each branch is the current (typically lowest in z) node.
    // also use this loop to identify the most central branch.  when merging with neighbors
    // we'll intentionally skip merging this one with anything to encourage other to merge
    // with it.
    max_center_dist=0.0;
    brptr=temp_tree;
    while(brptr!=NULL)
      {
      brptr->goal->j=brptr->node->z;					// save contact node z value for future use
      vtx_new=vertex_copy(brptr->node,NULL);
      vtx_new->z -= grid_zinc;
      if(vtx_new->z<brptr->goal->z)vtx_new->z=brptr->goal->z;		// should go below goal of this node - could be build table, could be model surface
      vtx_new->next=brptr->node;
      brptr->node=vtx_new;
      
      if(brptr->goal->i > max_center_dist)max_center_dist=brptr->goal->i;
      
      brptr=brptr->next;
      }
    
    // grow branch down or to another node by evaluating how far away the node is and by
    // how close that node's goal is to the center of open space (i.e. away from model surfaces).
    vtx_best=vertex_make();							// temp storage
    while(TRUE)									// loop until all branches are done (status==1)
      {
      // update status to user
      amt_current=amt_total-temp_branch_cnt;
      amt_done = (double)(amt_current)/(double)(amt_total);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));
      

      // grab the first branch off the list as it should be the one with the highest z level
      // and the most potential to merge with another branch.
      brptr=temp_tree;								// grab the first element off the branch list
      if(brptr==NULL)break;							// if the temp list is empty then we are done
      //brptr->wght=99;
      //debug_ctr++;
      //printf("\n  Processing branch %X  (next=%X) .... \n",brptr,brptr->next);
      //printf("  node: x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->node->x,brptr->node->y,brptr->node->z,brptr->node->i,brptr->node->j,brptr->node->k);
      //printf("  goal: x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->goal->x,brptr->goal->y,brptr->goal->z,brptr->goal->i,brptr->goal->j,brptr->goal->k);
      //printf("  temp_branch_cnt = %d   tree_branch_cnt=%d \n\n",temp_branch_cnt,tree_branch_cnt);
      
      // init vtx_best to the primary branch we are growing
      dz=brptr->goal->j - brptr->node->z;					// calc current distance away from underside of model surface this branch supports
      nbrbest=brptr;								// init address of best branch to merge with (start with self)
      vtx_best->x=brptr->node->x;
      vtx_best->y=brptr->node->y;
      vtx_best->z=brptr->node->z;
      score_best=0;
      
      // loop thru remainder of list searching for best node to merge with.
      // to qualify, their xy dist has to be reachable in z before hitting the build table or landing
      // surface of model.  then, the branches farthest away from model surfaces get more attraction
      // in an attempt to build more singular tree trucks down the center of open spaces.
      if(brptr->goal->i != max_center_dist)				  	// skip the most central node because we want others to merge with it
        {
	nbrptr=brptr->next;
	while(nbrptr!=NULL)
	  {
	  if(brptr->patbr != nbrptr->patbr){nbrptr=nbrptr->next;continue;}	// only merge with branches from the same patch
	    
	  xy_dist = vertex_2D_distance(brptr->node,nbrptr->node);		// get xy distance bt nodes
	  z_dist = xy_dist * fabs(tan_max_angle);				// min z distance needed to make the merge at max mat'l slope
	  
	  /*
	  // DEBUG
	  printf("\nNeighbor eval: %d \n",i);
	  printf("   bptr=%X  n: x=%6.3f y=%6.3f z=%6.3f   g: x=%6.3f y=%6.3f z=%6.3f \n",brptr,brptr->node->x,brptr->node->y,brptr->node->z,brptr->goal->x,brptr->goal->y,brptr->goal->z);
	  printf("   nptr=%X  n: x=%6.3f y=%6.3f z=%6.3f   g: x=%6.3f y=%6.3f z=%6.3f \n",nbrptr,nbrptr->node->x,nbrptr->node->y,nbrptr->node->z,nbrptr->goal->x,nbrptr->goal->y,nbrptr->goal->z);
	  printf("   dz=%6.3f  xy_dist=%6.3f  xy_pctr=%6.3f \n",dz,xy_dist,xy_pctr);
	  printf("   w_merge=%6.3f  w_center=%6.3f \n",w_merge,w_center);
	  */
  
	  //score_merge = spt_grid_pitch / xy_dist + nbrptr->goal->k;		// distance of this node to branch plus trunk weight - the closer it is, the higher the score
	  score_merge = spt_grid_pitch / xy_dist;
	  score_center= nbrptr->goal->i / max_center_dist;			// distance of this node to patch perimeter - the further from model, the higher the score
	  
	  weight_merge = z_trans / dz;						// high weight if near model surface, lowers as it moves toward goal
	  weight_center = (dz-z_trans) / dz;					// fixed low weight above trans, increases as it moves toward goal
	  if(weight_center<0.1)weight_center=0.1;				// provides subtle bias toward center even when near model surface
	  
	  score_total = score_merge*weight_merge + score_center*weight_center;	// calc total score for this branch
	  
	  if(score_total > score_best)
	    {
	    score_best=score_total;
	    vtx_best->x=nbrptr->node->x;
	    vtx_best->y=nbrptr->node->y;
	    vtx_best->z=nbrptr->node->z-z_dist;
	    if(vtx_best->z < nbrptr->goal->z)vtx_best->z=nbrptr->goal->z;		// if there is enough z left to possibly merge with this node...
	    nbrbest=nbrptr;
	    }
	  
	  nbrptr=nbrptr->next;
	  }
	}
      
      // if better branch was found use it for next node in brptr branch
      if(nbrbest!=brptr)							// if a merge neighbor was found...
        {
	//printf("\n     merging branches... \n");
	//printf("      brptr: x=%6.3f y=%6.3f z=%6.3f \n",brptr->node->x,brptr->node->y,brptr->node->z);
	//printf("      vbest: x=%6.3f y=%6.3f z=%6.3f \n",vtx_best->x,vtx_best->y,vtx_best->z);

	// add the node to brptr and terminate it here
	vtx_new=vertex_copy(vtx_best,NULL);					// ... create a new head node on this branch
	vtx_new->next=brptr->node;						// ... create link to existing branch node
	brptr->node=vtx_new;							// ... define it as head node for this branch
	brptr->status=1;
	
	// add the node to the branch that it merged with
	nbrbest->goal->k += 0.25;						// ... add trunk weight
	vtx_new=vertex_copy(vtx_best,NULL);					// ... create another new vtx
	vtx_new->next=nbrbest->node;						// ... create link to existing neighbor node
	nbrbest->node=vtx_new;							// ... define it as new neighbor node
	}
      else 									// otherwise just add downward node to brptr at same xy
        {
	//printf("\n     extending branch... \n");
	//printf("      brptr: x=%6.3f y=%6.3f z=%6.3f  newz=%6.3f \n",brptr->node->x,brptr->node->y,brptr->node->z,vtx_new->z);
	
	// if just extending, we do not add a new node... just lower the z a bit toward goal
	brptr->node->z -= grid_zinc;						// ... grow down in z
	if(brptr->node->z <= brptr->goal->z)					// ... if at or below our goal...
	  {
	  brptr->node->z = brptr->goal->z;					// ... ensure alignment
	  brptr->status=1;							// ... flag as finished
	  }
	}

      //while(!kbhit());
	
      // check if this branch is finished.  if so, remove from this list and
      // add to the finished list.
      if(brptr->status==1)
        {
	// remove it from temp tree
	temp_tree=brptr->next;							// since it was head element, just redefine head as next element
	temp_branch_cnt--;							// decrement branch counter of what's left in temp tree
	//printf("   Removing branch %X from temp_tree (%X) ... temp_branch_cnt=%d \n",brptr,temp_tree,temp_branch_cnt);
	//printf("      brptr=%X  brptr->next=%X \n",brptr,brptr->next);
	  
	// add it to support tree
	if(job.support_tree==NULL)
	  {
	  job.support_tree=brptr;
	  brptr->next=NULL;
	  }
	else 
	  {
	  nbrptr=job.support_tree;
	  while(nbrptr->next!=NULL){nbrptr=nbrptr->next;}
	  nbrptr->next=brptr;
	  brptr->next=NULL;
	  }
	tree_branch_cnt++;
	//printf("   Adding branch %X to support_tree (%X) ... tree_branch_cnt=%d \n",brptr,job.support_tree,tree_branch_cnt);
	//printf("      brptr=%X  brptr->next=%X \n",brptr,brptr->next);
	}
	
      // resort list to put highest z first and finished branches last
      //printf("    re-sorting...\n");
      branch_sort(temp_tree);
      
      /*
      // Debug
      printf("\n  Sorted root node list: \n");
      i=0;
      brptr=temp_tree;
      while(brptr!=NULL)
	{
	printf(" %d  brptr:  %X status=%d next=%X\n",i,brptr,brptr->status,brptr->next); 
	printf("      node:  %X x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->node, brptr->node->x,brptr->node->y,brptr->node->z,brptr->node->i,brptr->node->j,brptr->node->k);
	printf("      goal:  %X x=%6.3f y=%6.3f z=%6.3f  i=%6.3f j=%6.3f k=%6.3f \n",brptr->goal, brptr->goal->x,brptr->goal->y,brptr->goal->z,brptr->goal->i,brptr->goal->j,brptr->goal->k);
	i++;
	brptr=brptr->next;
	}

      while(!kbhit());
      */
      
      }		// end of while(TRUE)
	
    vertex_destroy(vtx_pctr); vtx_pctr=NULL;
    vertex_destroy(vtx_best); vtx_best=NULL;
  }
  
  //return(1);
  
  // Step 5 - Create facets along branches
  {
    
    // initialize
    sprintf(scratch,"  Generating Supports...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Constructing facets  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);

    // loop thru all models in the job that request support
    mptr=job.model_first;						// start with first model in job
    while(mptr!=NULL)							// loop thru all models in job
      {
      slot=(-1);							// default no tool selected
      slot=model_get_slot(mptr,OP_ADD_SUPPORT_MATERIAL);		// determine which tool is adding support
      if(slot>=0 && slot<MAX_TOOLS)break;				// we found the tool providing support
      mptr=mptr->next;
      }
    if(mptr==NULL)mptr=job.model_first;
      
    amt_total=tree_branch_cnt;
    amt_current=0;
    brptr=job.support_tree;						// start with first branch
    while(brptr!=NULL)							// loop thru all branches
      {
      
      // update status to user
      amt_current++;
      amt_done = (double)(amt_current)/(double)(amt_total);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      // create a series of circlular polygons up the length of the branch that we are going to orient
      // and then sweep to make facets.
      pold=NULL;
      brptr->pfirst=NULL;
      vptr=brptr->node;							// start with top node of branch
      while(vptr!=NULL)							// loop thru all nodes of this branch
	{
	// build a polygon circle in xy plane of unit diameter around pt 0,0,0
	vtx_first=NULL;
	vtx_old=NULL;
	for(i=0; i<branch_vtx_cnt; i++) 
	  {
	  vtx_new=vertex_make();
	  theta = (2*PI*i+PI/4) / branch_vtx_cnt;			// start at 45deg in even 4 sided so aligns with xy axes
	  vtx_new->x=cosf(theta);					// build normalized circle around centerlien of branch
	  vtx_new->y=sinf(theta);
	  vtx_new->z=0.0;
	  if(vtx_first==NULL)vtx_first=vtx_new;
	  if(vtx_old!=NULL)vtx_old->next=vtx_new;
	  vtx_old=vtx_new;
	  }
	vtx_old->next=vtx_first;					// make list circular

	/*
	// orient the new circle such that it is perpendicular to the centerline
	vtx_temp=vertex_make();						// re-usable temporary vtx
	vtx_dir=vertex_make();						// create a vector to store direction from this node to next node
	if(vptr->next!=NULL)
	  {
	  vtx_dir->x = (vptr->next)->x - vptr->x;
	  vtx_dir->y = (vptr->next)->y - vptr->y;
	  vtx_dir->z = (vptr->next)->z - vptr->z;
	  }
	else 
	  {
	  vtx_dir->x=0;
	  vtx_dir->y=0;
	  vtx_dir->z=0;
	  }
	vtx_up=vertex_make();						// create a vector to tentatively define "up"
	vtx_up->x=0; vtx_up->y=0; vtx_up->z=1.0;
	if(vertex_dotproduct(vtx_dir,vtx_up)>0.99)			// if too close to parallel with vtx_dir...
	  {vtx_up->x=1.0; vtx_up->y=0; vtx_up->z=0;}			// ... switch to different direction
  
	vtx_origin=vertex_make();					// create an origin
	vtx_origin->x=0; vtx_origin->y=0; vtx_origin->z=0;
  
	vtx_x_axis=vertex_make();					// create vertex to hold direction of x-axis
	vertex_crossproduct(vtx_up,vtx_dir,vtx_x_axis);			// generate cross product
	vec_x_axis=vector_make(vtx_origin,vtx_x_axis,19);
	vector_unit_normal(vec_x_axis,vtx_temp);
	vtx_x_axis->x=vtx_temp->x;					// vtx_x_axis is now normalized in plane perpendicular to branch
	vtx_x_axis->y=vtx_temp->y;
	vtx_x_axis->z=vtx_temp->z;
	vector_destroy(vec_x_axis); vec_x_axis=NULL;
  
	vtx_y_axis=vertex_make();					// create vertex to hold direction of y-axis
	vertex_crossproduct(vtx_dir,vtx_x_axis,vtx_y_axis);		// generate cross product
	vec_y_axis=vector_make(vtx_origin,vtx_y_axis,19);
	vector_unit_normal(vec_y_axis,vtx_temp);
	vtx_y_axis->x=vtx_temp->x;					// vtx_y_axis is now normalized in plane perpendicular to branch
	vtx_y_axis->y=vtx_temp->y;
	vtx_y_axis->z=vtx_temp->z;
	vector_destroy(vec_y_axis); vec_y_axis=NULL;
	
	// finally apply transform to move, rotate, and scale the circle
	vptr=vtx_first;
	while(vptr!=NULL)
	  {
	  vptr->x = vptr->x + (brptr->wght*branch_diam/2);		// just move and scale
	  vptr->y = vptr->y + (brptr->wght*branch_diam/2);
	  vptr->z = vptr->z + (brptr->wght*branch_diam/2);
	  //vptr->x = vptr->x + (brptr->wght*branch_diam/2) * (vptr->x*vtx_x_axis->x + vptr->y*vtx_y_axis->x);	// move, scale, and rotate
	  //vptr->y = vptr->y + (brptr->wght*branch_diam/2) * (vptr->x*vtx_x_axis->y + vptr->y*vtx_y_axis->y);
	  //vptr->z = vptr->z + (brptr->wght*branch_diam/2) * (vptr->x*vtx_x_axis->z + vptr->y*vtx_y_axis->z);
	  vptr=vptr->next;
	  if(vptr==vtx_first)break;
	  }
	  
	// clean-up
	vertex_destroy(vtx_x_axis); vtx_x_axis=NULL;  
	vertex_destroy(vtx_y_axis); vtx_y_axis=NULL;  
	vertex_destroy(vtx_temp);   vtx_temp=NULL;
	vertex_destroy(vtx_dir);    vtx_dir=NULL;
	vertex_destroy(vtx_up);     vtx_up=NULL;
	*/
	
	// apply transform to move to branch center and scale in proportion to weight
	vcirc=vtx_first;
	while(vcirc!=NULL)
	  {
	  effective_diam=brptr->wght*branch_diam;
	  if(effective_diam<2.0)effective_diam=2.0;
	  if(vptr->next==NULL)effective_diam=1.0;					// if at contact node under model, reduce diam
	  if(effective_diam>8.0)effective_diam=8.0;
	  if(vptr->z<(2*mptr->slice_thick) && effective_diam<5.0)effective_diam=5.0;	// widen build table attachment layer(s)
	  vcirc->x = vptr->x + (effective_diam/2)*vcirc->x;
	  vcirc->y = vptr->y + (effective_diam/2)*vcirc->y;
	  vcirc->z = vptr->z;
	  vcirc=vcirc->next;
	  if(vcirc==vtx_first)break;
	  }

	// add the new poly circle to the list of poly circles for this branch
	pnew=polygon_make(vtx_first,SPT_PERIM,19);
	pnew->vert_qty=8;
	if(brptr->pfirst==NULL)brptr->pfirst=pnew;
	if(pold!=NULL)pold->next=pnew;
	pold=pnew;
	
	/*
	// DEBUG
	printf("\nbrptr=%X  pptr=%X \n",brptr,pold);
	printf("    node=%X  x=%6.3f y=%6.3f z=%6.3f \n",brptr->node,brptr->node->x,brptr->node->y,brptr->node->z);
	i=0;
	vnxt=pold->vert_first;
	while(vnxt!=NULL)
	  {
	  printf("   v%d: x=%6.3f y=%6.3f z=%6.3f \n",i,vnxt->x,vnxt->y,vnxt->z);
	  i++;
	  vnxt=vnxt->next;
	  if(vnxt==pold->vert_first)break;
	  }
	*/ 
	
	vptr=vptr->next;
	}
	
      // at this point we now have polygons at each node of the branch oriented along
      // to the centerline of the branch.  now move down the list first creating facets from
      // the neighboring polygons.  note this relies on the polygons having the SAME number of vtxs.
      pptr=brptr->pfirst;							// start with poly at lowermost node
      while(pptr!=NULL)								// loop until we get to top poly
	{
	pnxt=pptr->next;							// get address of poly below current (lower poly)
	if(pnxt==NULL)break;							// if nothing below current we are done
	
	vptr=pptr->vert_first;							// start with 1st vtx in current poly (lower poly)
	vnxt=pnxt->vert_first;							// start with 1st vtx in poly below (upper poly)
	while(vptr!=NULL)							// loop around entire poly
	  {
	  vptr2=vptr->next;							// get next vtx in lower poly
	  vnxt2=vnxt->next;							// get next vtx in upper poly
	  
	  // build a new vertical facet between vptr/vptr2 and vnxt and add to support facet list
	  fnew=facet_make();							// allocate memory and set default values for facet
	  vptr->flist =facet_list_manager(vptr->flist, fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  vptr2->flist=facet_list_manager(vptr2->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  vnxt->flist =facet_list_manager(vnxt->flist, fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  fnew->vtx[0]=vptr;							// assign which vtxs belong to this facet
	  fnew->vtx[1]=vptr2;
	  fnew->vtx[2]=vnxt;
	  facet_unit_normal(fnew);
	  facet_area(fnew);
	  fnew->status=4;							// flag this as a lower facet
	  facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);		// adds new facet to support facet list
	  
	  // build a new vertical facet between vptr2 and vnxt/vnxt2 and add to support facet list
	  fnew=facet_make();							// allocate memory and set default values for facet
	  vptr2->flist=facet_list_manager(vptr2->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  vnxt->flist =facet_list_manager(vnxt->flist, fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  vnxt2->flist=facet_list_manager(vnxt2->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	  fnew->vtx[0]=vptr2;							// assign which vtxs belong to this facet
	  fnew->vtx[1]=vnxt;
	  fnew->vtx[2]=vnxt2;
	  facet_unit_normal(fnew);
	  facet_area(fnew);
	  fnew->status=4;							// flag this as a lower facet
	  facet_insert(mptr,mptr->facet_last[SUPPORT],fnew,SUPPORT);		// adds new facet to support facet list
	  
	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  vnxt=vnxt->next;
	  if(vnxt==pnxt->vert_first)break;
	  }
	  
	pptr=pptr->next;
	}
	
      brptr=brptr->next;
      } // end of branch_ptr loop
      
      // set support bounds to same as model bounds
      mptr->xmin[SUPPORT]=mptr->xmin[MODEL];  mptr->xmax[SUPPORT]=mptr->xmax[MODEL];
      mptr->ymin[SUPPORT]=mptr->ymin[MODEL];  mptr->ymax[SUPPORT]=mptr->ymax[MODEL];
      mptr->zmin[SUPPORT]=mptr->zmin[MODEL];  mptr->zmax[SUPPORT]=mptr->zmax[MODEL];
      mptr->xorg[SUPPORT]=mptr->xorg[MODEL];
      mptr->yorg[SUPPORT]=mptr->yorg[MODEL];
      mptr->zorg[SUPPORT]=mptr->zorg[MODEL];
      mptr->xoff[SUPPORT]=mptr->xoff[MODEL];
      mptr->yoff[SUPPORT]=mptr->yoff[MODEL];
      mptr->zoff[SUPPORT]=mptr->zoff[MODEL];
  
      // re-associate the entire support facet set 
      facet_find_all_neighbors(mptr->facet_first[SUPPORT]);

    } // end of step 5
     
  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    // build edge list for support
    edge_display(mptr,set_view_edge_angle,SUPPORT);			// determine which sharp edges to display in 3D view

    printf("  %d  model facet qty = %d    support facet qty =%d \n",mptr->model_ID,mptr->facet_qty[MODEL],mptr->facet_qty[SUPPORT]);
    mptr=mptr->next;
    }
     
  return(status);
}
