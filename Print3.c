#include "Global.h"	


// Function to print to a single vertex from current location
// Inputs:  	vptr=vertex destination, slot=which tool head,  pmode=pen-up/pen-down/other  lptr=line type pointer
// Return:	0=failure, 1=move done, or job.state
//
// note that this function does not actually send the print commands to the tinyg controller.  rather,
// it just builds the commands into the gcode_burst string that ultimately gets sent by the calling function.
int print_vertex(vertex *vptr, int slot, int pmode, linetype *lptr)
{
  float 	old_target,pu_lift_speed,feed_speed;
  float 	matl_amt,min_retract_veclen,alpha,fval;
  float 	adjlen,adjfeed,adjflow,adjlift,adjhtmod,adjtemp;
  float 	adjretract,adjadvance,adjeoldelay;
  float 	x0,y0,z0;
  float 	f01,f12,f23,ts,te;
  float 	dx,dy,dz,da;
  int 		ptyp,mtyp=MODEL;
  int		i,vec_breaks,invalid_target;
  int		h,xis,yis,xp,yp;
  float 	avg_z,z01,z23;
  float 	x_pos,y_pos,z_pos,x_step,y_step,z_step,x_vptr,y_vptr;
  int		prt_debug=FALSE;

  int		set_laser_focus_compensation=TRUE;
  float 	beam_angle,vec_angle;

  //prt_debug=TRUE;
  print_thread_alive=1;							// set to indicate this function still alive
  if(vptr!=NULL)VId++;							// increment vertex count
  
  // record last on part vtx for return from pause
  if(vptr!=NULL && on_part_flag==TRUE)
    {
    vtx_last_printed->x=vptr->x;
    vtx_last_printed->y=vptr->y;
    vtx_last_printed->z=vptr->z;
    vtx_last_printed->attr=vptr->attr;
    }

  // if in abort job mode...
  if(job.state==JOB_PAUSED_BY_CMD)return(job.state);

  if(prt_debug==TRUE)printf("PRINT:  vptr->x=%5.3f  vptr->y=%5.3f  vptr->z=%5.3f   slot=%d\n",vptr->x,vptr->y,vptr->z,slot);

  // save current position.  note - inbound values are in machine coords
  PosWas.x=PosIs.x;
  PosWas.y=PosIs.y;
  PosWas.z=PosIs.z;

  // now add in all the offsets required to get to the actual model location - gets complicated on multi-tool machines
  // it consists of:
  //	1) the offset from machine home to table home (values set when declared)
  //	2) the offset from the center of the carriage to the center of the tool mount (values in init_general function)
  //	3) the offset from the center of the tool mount to the tip of the tool (based on tool type - in TOOLS.XML)
  //	4) the offset from the ideal tool tip location to the actual tool tip location (fab tolerence of each tool - in tool memory and/or file)
  //	5) the offset the user set for this particular model (i.e. their placement on the table)

  // add in offsets that move machine home (0,0,0) to table home 0,0,0
  PosIs.x=vptr->x+xoffset;
  PosIs.y=vptr->y+yoffset;
  PosIs.z=vptr->z+zoffset;
  if(prt_debug==TRUE)printf("        Px=%5.3f Py=%5.3f   xoffset=%5.3f  yoffset=%5.3f \n",PosIs.x,PosIs.y,xoffset,yoffset);
  
  // if slot is NOT a tool, just move the center of the carriage to that location
  // in theory, the camera lens under the carriage is the 0,0 location
  if(slot<0 || slot>=MAX_TOOLS)
    {
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f Z%07.3f\n",PosIs.x,PosIs.y,PosIs.z);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    return(1);
    }

  // add in carriage offsets
  // these are the distances from the center of the carriage to the center of the tool mount.
  // since carriage slots are fixed, they have +/- offset values that get applied generically
  PosIs.x+=crgslot[slot].x_center;
  PosIs.y+=crgslot[slot].y_center;
  if(prt_debug==TRUE)printf("        Px=%5.3f Py=%5.3f   crgslot.x=%5.3f  crgslot.y=%5.3f \n",PosIs.x,PosIs.y,crgslot[slot].x_center,crgslot[slot].y_center);
    
  // add in default tool offsets from TOOLS.XML 
  // these are the "ideal" location of the tip relative to this tool type's mounting location.
  // Note the z offset is handled separately by tool_tip_deposit function.
  if(slot==0 || slot==2)PosIs.x+=Tool[slot].x_offset;			// tool mounted farther from x home
  if(slot==1 || slot==3)PosIs.x-=Tool[slot].x_offset;			// tool mounted nearer to x home
  if(slot==0 || slot==2)PosIs.y-=Tool[slot].y_offset;
  if(slot==1 || slot==3)PosIs.y+=Tool[slot].y_offset;			// account for tool being flipped 180 degrees on carriage
  if(prt_debug==TRUE)printf("        Px=%5.3f Py=%5.3f   Tool.xoff=%5.3f  Tool.yoff=%5.3f \n",PosIs.y,PosIs.y,Tool[slot].x_offset,Tool[slot].y_offset);

  // add in fabrication tool offsets from calibration
  // these are the variences that each individual tool (even of the same type) will have due to fabrication tolerences.
  // Note the z offset is handled separately by tool_tip_deposit function.
  if(slot==0 || slot==2)PosIs.x+=Tool[slot].tip_x_fab;			// tool mounted farther from x home
  if(slot==1 || slot==3)PosIs.x-=Tool[slot].tip_x_fab;			// tool mounted nearer to x home
  if(slot==0 || slot==2)PosIs.y-=Tool[slot].tip_y_fab;
  if(slot==1 || slot==3)PosIs.y+=Tool[slot].tip_y_fab;			// account for tool being flipped 180 degrees on carriage
  //PosIs.z+=Tool[slot].mmry.md.tip_pos_Z;				// z is handled by tip deposit function
  if(prt_debug==TRUE)printf("        Px=%5.3f Py=%5.3f   Tool.xtip=%5.3f  Tool.ytip=%5.3f \n",PosIs.x,PosIs.y,Tool[slot].tip_x_fab,Tool[slot].tip_y_fab);
  
  // may be better place to account for this... but for now...
  // in ALPHA and BETA type units this is accounted for in the tool_tip_deposit and tool_tip_retract functions
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    PosIs.z+=Tool[slot].tip_dn_pos;					// account for tool type offset
    PosIs.z+=Tool[slot].tip_z_fab;					// account for fabrication tolerance
  #endif

  // add in offsets that map model position on table (i.e. by autocenter or user translate)
  if(active_model!=NULL)
    {
    PosIs.x += active_model->xoff[mtyp];
    PosIs.y += active_model->yoff[mtyp];
    if(active_model->mdl_has_target==TRUE)				// if there is a base target ...
      {PosIs.z += (active_model->zmax[TARGET]-vptr->z);}		// ... add in height of target
    else 								// otherwise if no target...
      {
      PosIs.z += active_model->zoff[mtyp];				// ... add in height of model offset in z
      if(set_force_z_to_table==TRUE)PosIs.z -= z_start_level;    	// ... push down onto table if requested
      }
    PosIs.z += active_model->base_lyr_zoff;
    if(prt_debug==TRUE)printf("        Mxoff=%5.3f Myoff=%5.3f Mzoff=%5.3f \n",active_model->xoff[MODEL],active_model->yoff[MODEL],active_model->zoff[MODEL]);
    if(prt_debug==TRUE)printf("        Txoff=%5.3f Tyoff=%5.3f Tzoff=%5.3f \n",active_model->xoff[TARGET],active_model->yoff[TARGET],active_model->zoff[TARGET]);
    if(prt_debug==TRUE)printf("        Txoff=%5.3f Tyoff=%5.3f Tzoff=%5.3f \n",active_model->xmax[TARGET],active_model->ymax[TARGET],active_model->zmax[TARGET]);
    if(prt_debug==TRUE)printf("        Px=%5.3f Py=%5.3f Pz=%5.3f \n",PosIs.x,PosIs.y,PosIs.z);
    }

  // Perform one last z height check to avoid collisions of tool tips with the build table
  avg_z=z_table_offset(PosIs.x,PosIs.y);				// ... get z offset at this xy position
  if(PosIs.z < (Tool[slot].tip_dn_pos + Tool[slot].tip_z_fab + zoffset + avg_z))
    {
    printf("\nInvalid Z tip position requested! Adjusting to avaoid collision.\n");
    printf("    Target=%6.3f   Allowable = %6.3f + %6.3f + %6.3f + %6.3f = %6.3f \n",
    PosIs.z, Tool[slot].tip_dn_pos, Tool[slot].tip_z_fab, zoffset, avg_z, (Tool[slot].tip_dn_pos + Tool[slot].tip_z_fab + zoffset + avg_z));
    PosIs.z = Tool[slot].tip_dn_pos + Tool[slot].mmry.md.tip_pos_Z + zoffset + avg_z;
    }
   
  // PosIs values now reflect true table target position.  Save amount of change from last move for each axis.
  dx=PosIs.x-PosWas.x;
  dy=PosIs.y-PosWas.y;
  dz=PosIs.z-PosWas.z;
  
  // set defaults then switch over to line type params if available
  // note these params are needed for both penup and pendown modes
  ptyp=(-1);
  adjfeed=500.0;
  adjflow=1.0;
  adjlift=0.25;
  adjlen=0.25;
  adjhtmod=0.0;
  adjretract=0.1;
  adjadvance=0.1;
  adjeoldelay=0.0;
  adjlen=0.25;								// set default
  min_retract_veclen=2.0;						// distance must be longer than this to execute mat'l retract/advance
  if(lptr!=NULL)							// if type exists... reset params to line type params
    {
    adjfeed=lptr->feedrate;
    adjflow=lptr->flowrate;
    adjlift=lptr->move_lift;
    adjlen=lptr->minveclen;
    adjhtmod=lptr->height_mod;
    adjretract=lptr->retract;
    adjadvance=lptr->advance;
    adjeoldelay=lptr->eol_delay;
    min_retract_veclen=8*lptr->line_width;
    ptyp=lptr->ID;
    if(prt_debug==TRUE)printf("Using line type pointer data: %s \n",lptr->name);
    }
  
  // ensure XY coordinates requested land within the build volume
  if(PosIs.x<BUILD_TABLE_MIN_X)PosIs.x=BUILD_TABLE_MIN_X;
  if(PosIs.y<BUILD_TABLE_MIN_Y)PosIs.y=BUILD_TABLE_MIN_Y;
  if(PosIs.x>BUILD_TABLE_MAX_X)PosIs.x=BUILD_TABLE_MAX_X;
  if(PosIs.y>BUILD_TABLE_MAX_Y)PosIs.y=BUILD_TABLE_MAX_Y;
  
  // z coordinate is tricky since table could be low, or tool could by high, thus
  // requesting a z level less than BUILD_TABLE_MIN_Z
  //if(PosIs.z<BUILD_TABLE_MIN_Z)PosIs.z=BUILD_TABLE_MIN_Z;
  if(PosIs.z>BUILD_TABLE_MAX_Z)PosIs.z=BUILD_TABLE_MAX_Z;


  // determine the effective distance of the whole move.  Note the head moves in ALL 3 AXES.
  // if shorter than mimium then just skip, effectively combining with next vertex in list
  curDist=sqrt(dx*dx + dy*dy)+dz;					// note the "+dz" to not eliminate pure z moves
  if(adjlen<0.08)adjlen=0.08;						// min line length of tinyG is 0.08
  if(fabs(dz)<CLOSE_ENOUGH && curDist<adjlen && job.type==ADDITIVE)	// if move too small...
    {
    PosIs.x=PosWas.x;PosIs.y=PosWas.y;PosIs.z=PosWas.z;			// restore values to current location
    printf("\nSmall move requested!  %f vs %f Continuing.\n\n",curDist,adjlen);
    return(0);								// return w/o move which basically adds this to the next move
    }
    
  job.current_dist += curDist;						// add to total distance travelled so far in job

  // if near build table and model lower close off...
  //if(z_cut<(job.min_slice_thk*5) && ptyp==MDL_LOWER_CO)adjhtmod=0.0;	// remove lift since not on support
    
  // add line type hieght mod to z level
  PosIs.z+=adjhtmod;							// add in line type height modifier

  // At this point the vertex will be sent to the tinyG in one form or another depending on what's been requested.
  // Note this is not the same as cmdCount because many vtxs can be queued up in a burst before finally being sent
  // to the controller.  cmdCount should then catch up to vtxCount as the commands are parsed and sent.
  vtxCount++;								// increment number of vtxs sent to tinyG
  if(vtxCount>MAX_BUFFER)vtxCount=MAX_BUFFER-1;				// prevent overflow
  cmd_buffer[vtxCount].vptr=vptr;					// save address of vtx that generated this move

  // switch to printing with z adjustment for uneven build tables up to user set point
  // evenly distribute the delta across the set_z_contour distance
  avg_z=0.0;								// init to 0.  used if z_tbl_comp is enabled
  if(z_tbl_comp==TRUE && PosIs.z<=set_z_contour)			// if z contouring and low enough...
    {
    if(pmode==1 || pmode==5)						// if a pen down move ...
      {pmode=5;}							// ... set to use contouring
    else 								// otherwise adjust z for pen up move...
      {
      avg_z=z_table_offset(PosIs.x,PosIs.y);				// ... get z offset at this xy position
      avg_z *= ((set_z_contour-vptr->z)/set_z_contour);			// ... ratio based on how close to set_z_contour we are
      PosIs.z += avg_z;							// ... add in z table component
      }
    }

  // if laser cutting, compensate for beam focus oval by slowing down direction that is less focused
  set_laser_focus_compensation=FALSE;
  if(set_laser_focus_compensation==TRUE)
    {
    beam_angle=PI/2;							// beam aligns with y axis
    vec_angle=PI/2;							// assume pure y move
    if(fabs(dx)>CLOSE_ENOUGH)vec_angle=atan(dy/dx);			// if delta x, then calc actual angle - limit from 0 to PI/2
    fval=1.00;								// set feed rate adjustment factor to 100%
    if(beam_angle>CLOSE_ENOUGH)fval=fabs(vec_angle/beam_angle);		// calc factor if off angle
    if(fval>1.00)fval=1.00;
    if(fval<0.75)fval=0.75;						// limit to 75%
    adjfeed *= fval;							// adj feedrate based on how close to angle
    }

  // If request is for a PENUP move WITH tip lift and WITH material retract only if long move then ...
  if(pmode==0)
    {
    // if longer pen up move retract material to stop material flow and lift tip
    // (it does not make sense to retract when depositing neighboring close-offs and tight fills)
    if(ptyp==MDL_LAYER_1)
      {
      //min_retract_veclen=0.50;
      //if(lptr!=NULL)min_retract_veclen=lptr->line_pitch+0.01;			// lift on anything greater than neighboring vector
      }
    if(curDist>min_retract_veclen)
      {
      // ... retract material to stop flow
      if(Tool[slot].type==ADDITIVE && adjretract>0)
        {
	//Tool[slot].matl.mat_pos=PosIs.a;
	//Tool[slot].matl.mat_pos=PosWas.a;
	Tool[slot].matl.mat_pos-=adjretract;					// subtract retract amt from current position
	sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
	strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
	}
    
      // ... lift tool tip using z motors
      pu_lift_speed=500.0;
      sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",(PosIs.z+adjlift),pu_lift_speed);	// lift for additive jobs
      //if(Tool[slot].type==SUBTRACTIVE && job.type==SUBTRACTIVE)sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",(ZMax+adjlift),pu_lift_speed);
      strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
      }
	  
    // ... goto new XY positon
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f\n",PosIs.x,PosIs.y);			// applies to all job types
    strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
    
    // ... reset the tip height and material position
    if(curDist>min_retract_veclen)
      {
      // ... slow down plunge of subtractive tools as they may be going into material
      if(Tool[slot].type==SUBTRACTIVE)pu_lift_speed=300.0;
	  
      // if first layer, adjust z position
      if(Tool[slot].type==ADDITIVE && first_layer_flag==TRUE && Tool[slot].tool_ID!=TC_EXTRUDER)
	{
	//if(ptyp==MDL_BORDER)PosIs.z-=0.1;
	//if(ptyp==MDL_OFFSET)PosIs.z-=0.1;
	}
      
      // ... set tip of tool back to printing height
      sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",PosIs.z,pu_lift_speed);
      strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    
      // ... advance to start material flow
      if(Tool[slot].type==ADDITIVE && adjadvance>0)
        {
	Tool[slot].matl.mat_pos+=adjadvance;					// subtract retract amt from current position
	sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
	strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
	}
      }

    vptr->attr=(-1);							// indicate vtx has been place in buffer to be printed
    
    return(1);
    }

  // If request is for a PENDOWN then ...
  if(pmode==1)
    {
    if(Tool[slot].type==ADDITIVE)
      {
      // adjust depostion speed if running thru a group of short vectors, typically only when perimeter/offset drawing
      // debatable whether this actually helps or hurts.  when the deposition tip slows down, more material flows out
      // (even tho feed is also reduced per controller) due to pre-exisiting backpressure.  this leave bumps where
      // transitions occur.
      if(ptyp==MDL_PERIM || ptyp==MDL_OFFSET || ptyp==MDL_BORDER)
	{
	if(Tool[slot].tool_ID==TC_FDM)					// most tools must stay at constant speed
	  {
	  // slowing feed rate down around tight corners makes the corners look better, BUT it reduces pressure
	  // in the nozzle such that the next longer vectors after the corner are starved of material.  use
	  // with caution!
	  //if(adjfeed_flag==1 && curDist<adjfeed_trigger)adjfeed*=curDist/2.0;	// slow feedrate down around tight corners
	  //if(adjfeed_flag==1 && curDist<adjfeed_trigger)adjfeed*=0.75;	// slow feedrate down around tight corners
  
	  // if fidelity mode, slow perimeter line types down and reduce material slightly
	  if(fidelity_mode_flag==TRUE){adjfeed *= 0.50; adjflow *= 0.50;}
  
	  }
	}
  
      // reduce amount of material around short vectors
      //if(adjflow_flag==TRUE && curDist<adjflow_trigger)adjflow*=curDist/adjflow_trigger;
      if(adjflow_flag==TRUE && curDist<adjflow_trigger)adjflow*=0.75;

      // calculate amount of volume needed
      if(prt_debug==TRUE)printf("\n  curDist=%7.3f  adjflow=%7.3f  vptr->i=%7.3f \n",curDist,adjflow,vptr->i);
      matl_amt=curDist*adjflow;
      if(vptr->i!=0)matl_amt*=vptr->i;						// if an overlap vector, typically cut flowrate by half
      if(Tool[slot].tool_ID==TC_EXTRUDER)matl_amt/=500.0;			// if an EXTRUSION tool...
      if(Tool[slot].tool_ID==TC_FDM)matl_amt/=500.0;				// if an FDM tool...
      if(matl_amt<TOLERANCE)matl_amt=TOLERANCE;

      if(prt_debug==TRUE)printf("\n  Start: Tool[slot].matl.mat_pos=%7.3f \n",Tool[slot].matl.mat_pos);

      Tool[slot].matl.mat_used+=matl_amt;
      Tool[slot].matl.mat_pos+=matl_amt;
      
      if(prt_debug==TRUE)
        {
	printf("  End:   Tool[slot].matl.mat_pos=%7.3f \n",Tool[slot].matl.mat_pos);
	printf("Material for this vector = %7.3f \n",matl_amt);
	}
  
      if(adjfeed<10.0)adjfeed=10.0;
      sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f A%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,Tool[slot].matl.mat_pos,adjfeed);
      PosWas.a=PosIs.a;
      PosIs.a=Tool[slot].matl.mat_pos;
      }
	
    if(Tool[slot].type==SUBTRACTIVE)
      {
      sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,adjfeed);
      }
	
    if(Tool[slot].type==MARKING || Tool[slot].type==CURING)
      {
      sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,adjfeed);
      }

    if(Tool[slot].type==MEASUREMENT)
      {
      sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,adjfeed);
      }

    strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
    
    // if this line type has a forced delay associated with it...
    if(adjeoldelay>TOLERANCE)
      {
      tinyGSnd(gcode_burst);							// ... send string of commands to fill buffer
      memset(gcode_burst,0,sizeof(gcode_burst));				// ... null out burst string
      delay(adjeoldelay);
      printf(">>%f delay\n",adjeoldelay);
      }
    
    // check if move is shortest found yet
    if(curDist<minDist)minDist=curDist;

    // indicate vtx has been place in buffer to be printed
    vptr->attr=(-1);								

    return(1);
    }
	
  // If request is for a PENUP WITHOUT tip lift and WITHOUT material retract/advance then ...
  if(pmode==2)
    {
    // ... goto new XYZ positon without any extrusion change
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f Z%07.3f\n",PosIs.x,PosIs.y,PosIs.z);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    vptr->attr=(-1);							// indicate vtx has been place in buffer to be printed

    return(1);
    }

  // If request is for a PENUP move WITHOUT tip lift but WITH material retract/advance if long enough move then ...
  if(pmode==3)
    {
    // if longer pen up move retract material to stop material flow  
    // (it does not make sense to retract when depositing neighboring close-offs and tight fills)
    if(curDist>min_retract_veclen && Tool[slot].type==ADDITIVE)
      {
      Tool[slot].matl.mat_pos-=adjretract;					// subtract retract amt from current position
      sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);		// build command to move stepper to new position
      strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
      }

    // ... goto new XY positon
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f Z%07.3f\n",PosIs.x,PosIs.y,PosIs.z);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
	  
    // ... advance material to restart material flow
    if(curDist>min_retract_veclen && Tool[slot].type==ADDITIVE)
      {
      Tool[slot].matl.mat_pos+=adjadvance;					// subtract retract amt from current position
      sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
      strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
      }
    
    vptr->attr=(-1);							// indicate vtx has been placed in buffer to be printed

    return(1);
    }

  // If request is for a DRILL hole...
  if(pmode==4)
    {
    // ... lift tool tip
    sprintf(gcode_cmd,"G0 Z%07.3f\n",(ZMax+adjlift));
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
	  
    // ... goto new XY positon
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f\n",PosIs.x,PosIs.y);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
	  
    // ... drop tool tip to just above hole
    sprintf(gcode_cmd,"G0 Z%07.3f\n",ZMin);
    //sprintf(gcode_cmd,"G0 Z%07.3f \n",1.60);				// HARD CODED FOR 0.063" FR4
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor

    // ... drill hole to depth
    sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",PosIs.z,(adjfeed*0.25));
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    
    // ... lift tool tip out of hole and above model
    //sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",(ZMax+Tool[slot].matl.move_lift),(Tool[slot].matl.feedrate*0.25));
    sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",1.60,(adjfeed*0.25));
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    vptr->attr=(-1);							// indicate vtx has been place in buffer to be printed
    
    return(1);
    }

  // If request is for a z compensating PENDOWN then ...
  if(pmode==5)
    {
    // determine if the effective distance crosses over multiple table height calibration points
    // if so, break the vector such that height is adjusted up and down across its entire length.
    // note the vector positions are in PosIs variables at this point
    vec_breaks=(int)(curDist/scan_dist+0.5);				// calc number of scan point crossings
    if(vec_breaks<1)vec_breaks=1;					// always make at least one move
    if(z_tbl_comp==FALSE)vec_breaks=1;					// if turned off, then once thru loop and done
    //if(vec_breaks>1)printf("\n\n vec_breaks=%d \n\n",vec_breaks);

    // adjust depostion speed if running thru a group of short vectors, typically only when perimeter/offset drawing
    if(ptyp==MDL_PERIM || ptyp==MDL_OFFSET || ptyp==MDL_BORDER)
      {
      if(Tool[slot].tool_ID!=TC_EXTRUDER)				// extruders must stay at constant speed
        {
	if(adjfeed_flag==1 && curDist<adjfeed_trigger)adjfeed*=curDist/adjfeed_trigger;	// slow feedrate down around tight corners
	}
      }
	
    // break long moves into steps so that head height can be adjusted in accordance with scan_dist table variations
    x_pos=PosWas.x;
    x_step=dx/vec_breaks;
    x_vptr=(vptr->x-dx);
    y_pos=PosWas.y;
    y_step=dy/vec_breaks;
    y_vptr=(vptr->y-dy);
    for(i=0;i<vec_breaks;i++)
      {
      x_pos+=x_step;
      x_vptr+=x_step;
      y_pos+=y_step;
      y_vptr+=y_step;
      z_pos=PosIs.z;

      // adjust z value based on level/flatness of build table at target XY position only if build table was scanned
      // note:  the build table z adjustments are fixed to the table by position.  the vertex values are being mapped over-top of them.
      // the idea is to locate the 4 surrounding build table adjust values to the vertex, then bilinear interpolate to find the z adj
      // exactly at the vertex value.
      avg_z=z_table_offset(x_vptr,y_vptr);				// note vptr->x/y are currently mapped to table position
      avg_z *= ((set_z_contour-vptr->z)/set_z_contour);			// ratio based on how close to set_z_contour we are
      z_pos+=avg_z;
      
      if(Tool[slot].type==ADDITIVE)
	{
	// if first layer, slow down due to lower deposition temperature
	//if(first_layer_flag && ptyp==MDL_LOWER_CO && Tool[slot].tool_ID!=TC_EXTRUDER)adjfeed*=(0.5);
      
	// reduce amount of material around short vectors
	//if(adjflow_flag==1 && curDist<adjflow_trigger)adjflow*=curDist/adjflow_trigger;
	//if(adjflow_flag==1 && curDist<adjflow_trigger)adjflow*=0.75;
	if(adjflow_flag==1 && curDist<adjflow_trigger)adjflow*=0.95;
	
	// calculate amount of volume needed
	matl_amt=curDist/vec_breaks*adjflow;
	if(Tool[slot].tool_ID==TC_FDM)matl_amt/=500.0;			// if an FDM tool...
	if(vptr->i!=0)matl_amt*=vptr->i;				// if an overlap vector, cut flowrate in half
	if(matl_amt<TOLERANCE)matl_amt=TOLERANCE;
	Tool[slot].matl.mat_used+=matl_amt;
	Tool[slot].matl.mat_pos+=matl_amt;
    
	if(adjfeed<10.0)adjfeed=10.0;
	//sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f A%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,Tool[slot].matl.mat_pos,adjfeed);
	sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f A%07.3f F%07.3f\n",x_pos,y_pos,z_pos,Tool[slot].matl.mat_pos,adjfeed);
	PosWas.a=PosIs.a;
	PosIs.a=Tool[slot].matl.mat_pos;
	}
	
      if(Tool[slot].type==SUBTRACTIVE || Tool[slot].type==MEASUREMENT || Tool[slot].type==MARKING || Tool[slot].type==CURING)
	{
	sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f F%07.3f\n",x_pos,y_pos,z_pos,adjfeed);
	}
	
      strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
      
      // if about to exceed array space, it must get sent now...
      if(strlen(gcode_burst)>800)
        {
	tinyGSnd(gcode_burst);						// send to tinyg
	while(tinyGRcv(1)>=0);						// get tinyg feedback
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear string
	}
      }

    // save last used xyz coord
    PosIs.x=x_pos;
    PosIs.y=y_pos;
    PosIs.z=z_pos;
    
    // check if move is shortest found yet
    if(curDist<minDist)minDist=curDist;

    // now just draw line to new distination while extruding
    vptr->attr=(-1);								// indicate vtx has been place in buffer to be printed
    
    //printf("\n\nGBURST:\n");
    //printf("%s\n",gcode_burst);
    //while(!kbhit());
	  
    return(1);
    }
  
  // If request if for a PEN-UP with that is aligned with neighboring PEN-DOWNs then ...
  // This is a work around for a tinyg bug that chokes on penups that are perfectly aligned between two
  // pen downs.  The tinyg will throw a position error (shift) because it will skip the S curve acceleration.
  // So this penup will inject an extra move before proceeding to the target position thus breaking the alignment.
  if(pmode==6)
    {
    // ... force retract material to stop flow
    if(Tool[slot].type==ADDITIVE)
      {
      Tool[slot].matl.mat_pos-=adjretract;				// subtract retract amt from current position
      sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
      strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
      }
  
    // ... force lift tool tip using z motors - if a subtractive job, must lift above anything remaining
    pu_lift_speed=500.0;

    // ... slow down plunge of subtractive tools as they may be going into material
    if(Tool[slot].type==SUBTRACTIVE)pu_lift_speed=300.0;
	
    sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",(PosIs.z+adjlift),pu_lift_speed);	// applies to additive jobs
    //if(Tool[slot].type==SUBTRACTIVE && job.type==SUBTRACTIVE)sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",(ZMax+adjlift),pu_lift_speed);
    strcat(gcode_burst,gcode_cmd);						// add to the string to be sent to processor
    
    // ... calculate angle of colinear move to make sure our offsets do not align with it
    dx=PosWas.x-PosIs.x;						// get change in x
    dy=PosWas.y-PosIs.y;						// get change in y
    if(fabs(dx)<TOLERANCE)						// if move is purely in y...
      {
      dx=1.0; dy=0.0;							// ... add an x offset
      }
    else if(fabs(dy)<TOLERANCE)						// if move is purely in x...
      {
      dx=0.0; dy=1.0;							// ... add a y offset
      }
    else 								// but if not orthagonal...
      {
      alpha=atan(dy/dx);						// ... find angle
      alpha += PI/2;							// ... rotate by 90 deg
      dx=cos(alpha);							// ... derive x component
      dy=sin(alpha);							// ... derive y component
      }
    if(dx>(1+TOLERANCE))dx=1;						// safety check to not run amuck
    if(dy>(1+TOLERANCE))dy=1;
	  
    // ... goto extra XY positon
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f\n",PosIs.x+dx,PosIs.y+dy);	// add offset to avoid colinear alignment
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    
    // ... goto new XY positon
    sprintf(gcode_cmd,"G0 X%07.3f Y%07.3f\n",PosIs.x,PosIs.y);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    
    // ... set tip of tool back to printing height
    sprintf(gcode_cmd,"G1 Z%07.3f F%07.3f\n",PosIs.z,pu_lift_speed);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
  
    // ... advance to start material flow
    if(Tool[slot].type==ADDITIVE)
      {
      Tool[slot].matl.mat_pos+=adjadvance;				// subtract retract amt from current position
      sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
      strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
      }
    
    vptr->attr=(-1);							// indicate vtx has been place in buffer to be printed

    return(1);
    }

  // If request is for a PENUP WITHOUT tip lift and WITHOUT material retract/advance AT A SPECIFIC SPEED ...
  if(pmode==7)
    {
    // ... extract feed speed from vtx information
    feed_speed=vptr->k;
    if(feed_speed<10)feed_speed=10;
    if(feed_speed>6000)feed_speed=6000;
      
    // ... goto new XYZ positon without any extrusion change
    sprintf(gcode_cmd,"G1 X%07.3f Y%07.3f Z%07.3f F%07.3f\n",PosIs.x,PosIs.y,PosIs.z,feed_speed);
    strcat(gcode_burst,gcode_cmd);					// add to the string to be sent to processor
    vptr->attr=(-1);							// indicate vtx has been place in buffer to be printed

    return(1);
    }


    
  return(0);
}

// Function to adjust tool temperature based on line type request
// ideally this function can be call on any tool at any time.  if the tool
// is active, it will retract it from deposition if requested to wait.
int temperature_adjust(int slot, float newTemp, int wait4it)
{
  char		proc_scratch[255];
  int		i,status=TRUE,tool_up_flag=FALSE;;
  float 	adjtemp;
  float 	cur_delta;
  linetype	*lptr;
  
  // check if this option is enabled
  if(set_ignore_lt_temps==TRUE)return(0);
  
  // ensure valid tool is available
  if(slot<0 || slot>=MAX_THERMAL_DEVICES)return(0);
  if(Tool[slot].state<TL_LOADED)return(0);
  if(Tool[slot].pwr24==FALSE)return(0);
  if(Tool[slot].thrm.sensor<=0)return(0);
  if(newTemp<1 || newTemp>Tool[slot].thrm.maxtC)return(0);
  if(abs(newTemp-Tool[slot].thrm.setpC)<1)return(0);

  printf("\nTemperature Adjustment Entry ---------------------------------\n");
  
  // enter data to process log file
  //sprintf(proc_scratch,"      Temperature adjust:  %5.1f to %5.1f \n",Tool[slot].thrm.setpC,newTemp);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
    
  // adjust temperature, but only within limits
  Tool[slot].thrm.setpC=newTemp;
    
  // wait for tool to reach temperature within tolerance
  if(wait4it==TRUE)
    {
    cur_delta=fabs(Tool[slot].thrm.tempC-Tool[slot].thrm.setpC);	// calc difference
    if(Tool[slot].thrm.setpC<=50)cur_delta=Tool[slot].thrm.tolrC-1;	// if low temp, ignore
    if(cur_delta>Tool[slot].thrm.tolrC)					// if beyond allowable tolerance...
      {
      printf("  Temperature adjustment being made.\n");
      tool_up_flag=TRUE;						// ... set tool position flag
      tool_tip_retract(slot);						// ... retract tool tip from model
      }
    i=0;								// init loop counter
    while(cur_delta>Tool[slot].thrm.tolrC)				// while temp is out of tolerance...
      {
      cur_delta=fabs(Tool[slot].thrm.tempC-Tool[slot].thrm.setpC);	// calc difference
      if(Tool[slot].thrm.setpC<=50)cur_delta=Tool[slot].thrm.tolrC-1;	// if low temp, ignore
      delay(250);							// wait
      i++;
      if(i>1200){status=FALSE;break;}					// if waiting for 5 mins...
      }
    if(tool_up_flag==TRUE)						// if retract flag was set...
      {
      tool_up_flag=FALSE;						// ... turn off flag
      tool_tip_deposit(slot);						// ... put tool back in deposit position
      }
    }

  if(status==TRUE){printf("\n-------------------Temperature Adjustment Exit with Success\n");}
  else {printf("\n-------------------Temperature Adjustment Exit with Failure\n");}

  return(status);
}

// Function to check print status if a job.sync is requested in the printing loop of build_job.
// The print loop will jump to this function if job.sync flag is set.  That likely happend because
// the user hit pause or abort which set the public job.state and raised the sync flag.  However,
// it is possible the system detected a problem and forced the change as well.  Handle both here.
// Input:  local_job is a pointer to the localized job data used in build_job
// Return: 0=if anything other than JOB_RUNNING (i.e abort printing), 1=JOB_RUNNING (i.e. resume printing)
int print_status(jobx *local_job)
{
  char		proc_scratch[255];
  char 		gcode_rtn[255];
  int		old_job_state;
  int		i,j,tool_at_pause;
  int		pwm_lo,pwm_hi,pwm_at_pause,prev_air_status;
  float		XPause,YPause,ZPause;
  model 	*model_at_pause;

  printf("\nPrint status - entry.\n\n");

  // copy over the local_job status to public job
  job.current_z=local_job->current_z;
  job.current_dist=local_job->current_dist;
  job.total_dist=local_job->total_dist;
  job.penup_dist=local_job->penup_dist;
  job.pendown_dist=local_job->pendown_dist;

  // now determine how we wound up here.  if it was by a UI request, then we want to
  // push the current job state into the local job state.  if it was by a problem
  // encountered in the build_job function then we want to push the local job state
  // into the public job state.
  if(job.sync==TRUE)							// this is the flag to indicate the USER did something...
    {
    local_job->state=job.state;						// ... copy the state from public to local job
    job.sync=FALSE;							// ... reset the flag
    }
  else 									// otherwise the system generated the state change
    {
    job.state=local_job->state;
    }

  // save current conditions at start of pause
  tool_at_pause=current_tool;
  model_at_pause=active_model;
  z_cut_at_pause=z_cut;	
  prev_air_status=tool_air_status_flag;
  
  // enter data to process log file
  //sprintf(proc_scratch,"      Print paused. Job state = %d \n",job.state);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  tinyGSnd(gcode_burst);						// send commands if any were pending
  memset(gcode_burst,0,sizeof(gcode_burst));				// ensure any pending moves are cleared
  motion_complete();							// let tinyG finish what it's doing
  tool_matl_retract(tool_at_pause,Tool[tool_at_pause].matl.retract);	// retract material before tip lift
  tool_tip_retract(tool_at_pause);					// lift tool tip
  tool_air(tool_at_pause,OFF);						// turn air off

  // address tools that require attention while still at build table
  if(Tool[tool_at_pause].tool_ID==TC_LASER)				 // ... turn off laser power
    {
    pwm_at_pause=Tool[tool_at_pause].thrm.heat_duty;
    Tool[tool_at_pause].thrm.heat_duty=0.0;
    while(Tool[tool_at_pause].thrm.heat_status==ON);
    }
  
  // since other moves (like tool bit changes) can happen, all relative moves must be zero-ed out immediately
  on_part_flag=FALSE;							// set to indicate leaving part
  goto_machine_home();
  
  // address tools that require attention after they have left the build table
  if(Tool[tool_at_pause].tool_ID==TC_ROUTER)				// ... turn off router power
    {
    tool_power_ramp(tool_at_pause,TOOL_48V_PWM,OFF);
    }

  // wait in this loop until the job state is changed by user
  old_job_state=job.state;
  while(job.state==old_job_state)					// while nothing has changed...
    {
    print_thread_alive=1;						// still alive
    g_main_context_iteration(NULL, FALSE);				// poll for UI changes
    delay(100);								// don't hammer system with this loop
    }
   
  // resume once user changes state back to running
  if(job.state==JOB_RUNNING)
    {  
    sprintf(job_status_msg," Initiating resume... ");
    
    // restore params that were active when pause intitiated
    z_cut=z_cut_at_pause;
    active_model=model_at_pause;
    
    // address tools that need to be "on" before getting to the build table
    if(Tool[tool_at_pause].tool_ID==TC_ROUTER)
      {
      tool_power_ramp(tool_at_pause,TOOL_48V_PWM,100);
      }
    if(prev_air_status==ON)tool_air(tool_at_pause,ON);			// turn air back on
    
    comefrom_machine_home(tool_at_pause,vtx_last_printed);		// move back to part
    on_part_flag=TRUE;							// set to indicate back on part
    make_toolchange(tool_at_pause,1);					// make tool active again
    tool_tip_deposit(tool_at_pause);					// wait for tool to reach position
    tool_matl_forward(tool_at_pause,Tool[tool_at_pause].matl.advance);	// move material up to tip

    if(Tool[tool_at_pause].tool_ID==TC_LASER)				 // turn laser power back on
      {
      Tool[tool_at_pause].thrm.heat_duty=pwm_at_pause;
      //while(Tool[tool_at_pause].thrm.heat_status==OFF);
      }
    }
  // handle everything else but a return to job running.  the state will be handled
  // by the function this is returning to.
  else 
    {
    local_job->state=job.state; 					// push user job state into local job state
    job.sync=FALSE;							// no need to sync
    return(0);								// return with "failure"
    }

  // enter data to process log file
  //sprintf(proc_scratch,"      Print resumed. Job state = %d \n",job.state);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  local_job->state=job.state;
  job.sync=FALSE;
  
  printf("\nPrint paused - exit.\n\n");
  return(1);
}

// Function to move deposition tip away from model
int step_aside(int slot, int wait_duration)
{
  int		mtyp=MODEL;
  char		proc_scratch[255];
  float 	xhold,yhold;
  float		XPause,YPause,ZPause;
  char 		gcode_rtn[255];
  model 	*mptr;
  
  // enter data to process log file
  //sprintf(proc_scratch,"      Step aside to allow material to cool. \n");
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  // get current location
  motion_complete();
  XPause=PostG.x;YPause=PostG.y;ZPause=PostG.z;				// save location when pause becomes active
  
  // find min corner of all models in job
  xhold=BUILD_TABLE_LEN_X;
  yhold=BUILD_TABLE_LEN_Y;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mptr->xmin[mtyp]<xhold)xhold=mptr->xmin[mtyp]+mptr->xoff[mtyp];
    if(mptr->ymin[mtyp]<yhold)yhold=mptr->ymin[mtyp]+mptr->yoff[mtyp];
    mptr=mptr->next;
    }

  // move tool tip over to min corner
  tool_matl_retract(slot,Tool[slot].matl.retract);			// retract material
  sprintf(gcode_rtn,"G90 G0 Z%f F300\n",ZPause+1.0);			// raise carriage out of the way
  tinyGSnd(gcode_rtn);
  sprintf(gcode_rtn,"G90 G0 X%f Y%f \n",xhold,yhold);			// move carriage to hold spot
  tinyGSnd(gcode_rtn);
  while(tinyGRcv(1)>=0);
  
  // wait for defined duration
  delay(wait_duration*1000);
  
  // move tool tip back over to model
  sprintf(gcode_rtn,"G0 X%f Y%f Z%f\n",XPause,YPause,ZPause);		// move to location when pause started
  tinyGSnd(gcode_rtn);
  while(tinyGRcv(1)>=0);
  tool_matl_forward(slot,Tool[slot].matl.advance);			// move material up to tip
  
  return(1);
}

// Function to retract material from tip
int tool_matl_retract(int slot, float retract_amt)
{
  char		proc_scratch[255];
  
  printf("\nMaterial feed retract in slot %d \n",slot);

  if(slot<0 || slot>=MAX_TOOLS)return(0);				// ensure we address an existing tool
  if(Tool[slot].type!=ADDITIVE)return(0);
  if(Tool[slot].step_max<1)return(0);					// can't move a stepper that isn't there
  
  // enter data to process log file
  //sprintf(proc_scratch,"      Material feed retract.  Slot=%d  Amt=%f \n",slot,retract_amt);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  make_toolchange(slot,1);						// set to tool stepper
  Tool[slot].matl.mat_pos-=retract_amt;					// subtract retract amt from current position
  sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
  tinyGSnd(gcode_cmd);
  memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
  while(tinyGRcv(1)>=0);						// get latest tinyg feedback
  motion_complete();

  return(1);
}

// Function to forward material from tip
int tool_matl_forward(int slot, float forward_amt)
{
  char		proc_scratch[255];

  printf("\nMaterial feed forward in slot %d \n",slot);

  if(slot<0 || slot>=MAX_TOOLS)return(0);				// ensure we address an existing tool
  if(Tool[slot].type!=ADDITIVE)return(0);
  if(Tool[slot].step_max<1)return(0);					// can't move a stepper that isn't there

  // enter data to process log file
  //sprintf(proc_scratch,"      Material feed forward.  Slot=%d  Amt=%f \n",slot,forward_amt);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
  
  make_toolchange(slot,1);						// set to tool stepper
  Tool[slot].matl.mat_pos+=forward_amt;					// add forward amt to current position
  sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.mat_pos);	// build command to move stepper to new position
  tinyGSnd(gcode_cmd);
  memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
  while(tinyGRcv(1)>=0);						// get latest tinyg feedback
  motion_complete();

  return(1);
}

// Function to home a tool tip
// note that the carriage stepper lead screw ratio moves linear 1.25mm for every 1.00 requested
int tool_tip_home(int slot)
{
  char		proc_scratch[255];
  int		status=0,h;
  int		tool_sel,tool_fan;
  float 	retract_speed=500, home_speed=250;
  time_t 	home_start_time,home_finish_time;
  
  
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    Tool[slot].tip_cr_pos=Tool[slot].tip_up_pos;
    Tool[slot].homed=TRUE;
    return(TRUE);
  #endif

  if(Tool[slot].homed==TRUE){status=1;return(status);}			// if already homed...

  // enter data to process log file
  //sprintf(proc_scratch,"      Tool tip homing.  Slot=%d \n",slot);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
  sprintf(job_status_msg," Homing tool %d tip.",(slot+1));

  #ifdef ALPHA_UNIT
    // lift z a bit so that tool slot 4 clears the build table when homing
    if(slot==3)
      {
      // first freshen up the current position values
      sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
      tinyGSnd(gcode_cmd);						// send request
      while(tinyGRcv(1)>=0);						// get tinyg feedback - this may change tinyG_in_motion value
      
      // now move entire carriage up to clear the build table before homing
      sprintf(gcode_cmd,"G0 Z%5.3f \n",(PostG.z+5.0));	
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
      }
  #endif
  
  oldState=cmdControl;
  cmdControl=0;
  make_toolchange(slot,2);
  if(RPi_GPIO_Read(TOOL_LIMIT))						// if limit switch is already tripped...
    {
    // note it is possible for more than one tool to be grounded against their limit switch and since
    // the limit switch circuit is daisy chained across all tools, all tools must be bumped up to free
    // the switch
    for(h=0;h<MAX_TOOLS;h++)
      {
      if(h==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
      if(h==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
      if(h==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
      if(h==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
      gpioWrite(tool_sel,0); 
      sprintf(gcode_cmd,"G28.3 A0.0 \n");				// reset current location as home
      tinyGSnd(gcode_cmd);
      sprintf(gcode_cmd,"G1 A3.0 F%07.3f \n",retract_speed);		// move head up off switch by a bit
      tinyGSnd(gcode_cmd);
      motion_complete();
      gpioWrite(tool_sel,1); 
      }
    // if limit still trigged after moving all slots, then we hit a hardware failure
    if(RPi_GPIO_Read(TOOL_LIMIT))
      {
      Tool[slot].homed=FALSE;
      job.state=JOB_ABORTED_BY_SYSTEM;
      printf("\n\n*** ERROR - UNABLE TO CLEAR TOOL LIMIT SWITCH **\n\n");
      return(status);
      }
    }
  else 									// else if limit is not tripped yet ...
    {
    home_start_time=time(NULL);						// get start time of homing effort
    sprintf(gcode_cmd,"G28.3 A200.0 \n");				// set home at max travel
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    sprintf(gcode_cmd,"G1 A0.0 F%07.3f \n",home_speed);			// move head toward limit switch
    tinyGSnd(gcode_cmd);
    while(!RPi_GPIO_Read(TOOL_LIMIT))					// constantly check tool limit switch during move
      {
      home_finish_time=time(NULL);
      if(home_finish_time-home_start_time>10)break;			// limit effort to 10 sec or motor will burn out
      }
    tinyGSnd("!\n");							// abort move when switch hit
    while(tinyGRcv(1)>=0);
    tinyGSnd("%\n");							// note: these cmds do not produce a response from tinyG
    while(tinyGRcv(1)>=0);
    delay(250);
    if(home_finish_time-home_start_time>15)
      {
      job.state=JOB_ABORTED_BY_SYSTEM;
      printf("\n\n*** ERROR - UNABLE TO TRIGGER TOOL LIMIT SWITCH **\n\n");
      status=0;
      }
    else 
      {
      printf("Tool %d home:  Was %6.3f away from limit.\n",slot,(200.0-PostG.a));
      sprintf(gcode_cmd,"G28.3 A0.0 \n");				// set home at limit trip
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      sprintf(gcode_cmd,"G1 A%07.3f F%07.3f \n",Tool[slot].tip_up_pos,retract_speed);	// move head up off switch into retracted position
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      //motion_complete();
      Tool[slot].tip_cr_pos=Tool[slot].tip_up_pos;
      Tool[slot].homed=TRUE;
      status=1;
      }
    }

  #ifdef ALPHA_UNIT
    // put z back if it was raised
    if(slot==3)
      {
      sprintf(gcode_cmd,"G0 Z%5.3f \n",PostG.z);	
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
      }
  #endif

    make_toolchange(slot,0);						// turn off the stepper so it does not burn up

    cmdControl=oldState;
    printf("Exiting tool homing.\n");
      
  return(status);
}

// Function to retract a tool tip
// At one time this function also included setting back the temperature and material retraction, but it turns out to
// be too awkward to include those functions here as there are occurances that require different combinations
// of those events.  Keep this function to just moving the tip into retraction... nothing more.
//
// On multi-tool machines this is the function that retracts a tool tip from being proud of the other tools.
// In other words, it puts the tool away.  On single tool machines this is not necessary, but to try and keep code
// functionality consistent this function will move the tool tip up by the amount specified in the tool file.
//
// Note that tip_deposit and tip_retract are not meant to be done from home.  They are meant to be done when in
// position over the part.
int tool_tip_retract(int slot)
{
  char		proc_scratch[255];
  float 	retract_speed=500;
  float 	newz;
  
  printf("Tool tip retract: entry\n");
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    return(TRUE);
  #endif

  // enter data to process log file
  //sprintf(proc_scratch,"      Tool tip retract.  Slot=%d \n",slot);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  // avoid a "double retraction" if errantly called
  if(Tool[slot].tip_cr_pos==Tool[slot].tip_up_pos)return(0);
  if(Tool[slot].state==TL_RETRACTED)return(0);
  
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    // note that the carriage stepper lead screw ratio moves linear 1.25mm for every 1.00 requested
    make_toolchange(slot,2);
    sprintf(gcode_cmd,"G1 A%07.3f F%07.3f \n",Tool[slot].tip_up_pos,retract_speed);
  #endif
  
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    // on gammas use the z as surrogate to carriage stepper. bascially just raise z by tip delta
    while(tinyGRcv(1)>=0);						// get tinyg feedback
    newz=0-(Tool[slot].tip_dn_pos+Tool[slot].tip_z_fab);		// this effectively LOWERS the tool tip by the amount specified in the tool xml file
    sprintf(gcode_burst,"G91\nG0 Z%7.3f\nG90\n",newz);			// move carriage up with relative move
  #endif
  
  // execute the move
  tinyGSnd(gcode_cmd);
  memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
  if(motion_complete()==TRUE)Tool[slot].tip_cr_pos=Tool[slot].tip_up_pos;
  Tool[slot].state=TL_RETRACTED;

  return(1);
}

// Function to move tool tip into deposition position
// At one time this function also included readying the temperature and material forward, but it turns out to
// be too awkward to include those functions here as there are occurances that require different combinations
// of those events.  Keep this function to just moving the tip into position... nothing more.
//
// On multi-tool machines this is the function that makes a tool tip proud of the other tools.  In other words,
// it puts it in a position for use.  On single tool machines this is not necessary, but to try and keep code
// functionality consistent this function will move the tool tip down by the amount specified in the tool file.
//
// Note that tip_deposit and tip_retract are not meant to be done from home.  They are meant to be done when in
// position over the part.
int tool_tip_deposit(int slot)
{
  char		proc_scratch[255];
  int		i;
  float 	cur_delta,newz;
  float 	plunge_speed=500;
  
  printf("Tool tip deposit: entry\n");
  if(slot<0 || slot>=MAX_TOOLS)return(0);

  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    return(TRUE);
  #endif
  
  // enter data to process log file
  //sprintf(proc_scratch,"      Tool tip deposit.  Slot=%d \n",slot);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}

  // avoid a "double deposition" if errantly called
  if(Tool[slot].tip_cr_pos==Tool[slot].tip_dn_pos)return(0);
  if(Tool[slot].state==TL_ACTIVE)return(0);

  // reduce plunge speed for subtractive tool as they may be cutting as they go down
  if(Tool[slot].type==SUBTRACTIVE)plunge_speed=150.0;

  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    // note that the carriage stepper lead screw ratio moves linear 1.25mm for every 1.00 requested
    make_toolchange(slot,2);						// turn on carriage stepper drive
    sprintf(gcode_cmd,"G1 A%07.3f F%07.3f \n",Tool[slot].tip_dn_pos,plunge_speed);	// build move command
  #endif

  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    // on gammas use the z as surrogate to carriage stepper. basically just lower z by tip delta.
    while(tinyGRcv(1)>=0);							// get tinyg feedback to ensure PostG.z is up to date
    newz=Tool[slot].tip_z_fab+Tool[slot].tip_dn_pos;				// this effectively RAISES the tool tip the amount specified in the tool xml file
    sprintf(gcode_cmd,"G91\nG1 Z%07.3f F%07.3f\nG90\n",newz,plunge_speed);	// use relative move to drop z
  #endif

  // execute the tip placement
  tinyGSnd(gcode_cmd);							// send move command
  memset(gcode_cmd,0,sizeof(gcode_cmd));				// clear command string
  if(motion_complete()==TRUE)Tool[slot].tip_cr_pos=Tool[slot].tip_dn_pos;
  Tool[slot].state=TL_ACTIVE;

  return(1);
}

// Function to drop tool tip to touchdepth to initiate contact using Z axis
int tool_tip_touch(float delta_z)
{
  char		proc_scratch[255];
  float 	zlvl,apos;
  
  if(delta_z<=0)return(1);

  // enter data to process log file
  //sprintf(proc_scratch,"      Tool tip touch.  Amt=%f \n",delta_z);
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
  
  motion_complete();
  zlvl=PostG.z;

  // note that the carriage stepper lead screw ratio moves linear 1.25mm for every 1.00 requested
  sprintf(gcode_cmd,"G0 Z%07.3f \n",zlvl-delta_z);
  tinyGSnd(gcode_cmd);					
  
  // add small material push
  //apos=PostG.a;
  //sprintf(gcode_cmd,"G1 A%07.3f F250.0 \n",apos+0.08);
  //tinyGSnd(gcode_cmd);
  //motion_complete();
  
  // add small delay to allow contact to take place
  delay(100);
  
  // move z back to where it was
  sprintf(gcode_cmd,"G0 Z%07.3f \n",zlvl);	
  tinyGSnd(gcode_cmd);					
  motion_complete();
  memset(gcode_cmd,0,sizeof(gcode_cmd));		
  
  return(1);
}

// Function to turn on/off FDM filament cooling fan
int tool_air(int slot, int status)
{
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
    
  if(Tool[slot].tool_ID==TC_LASER)gpioWrite(LASER_ENABLE,!status);
  
  if(Tool[slot].tool_ID==TC_FDM)gpioWrite(TOOL_AIR,!status);					
  
  tool_air_status_flag=status;
  
  printf("\nTool air slot = %d  status = %d \n",slot,status);
  return(TRUE);
}

// Function to wait for tinyG to reach motion complete.  the idea is to first empty what ever is left in
// the motion controller buffer. once that is empty, then just wait until the last move is complete.  it gets ugly
// because sometimes tinyg does not accurately report either condition.  often it will think it has one move left
// in the buffer, but there is no motion.  other times it empties the buffer okay but then reports there is no 
// motion when in fact it is still finishing the last move.  this is most noticable on long moves.
//
// hence the old school way of checking if the position has changed while still keep an escape counter.
//
// Returns FALSE if NOT complete, TRUE if complete.
int motion_complete(void)
{
  char 		proc_scratch[255];
  int 		max_time_allowed=30;					// 30 second time-out
  int		status=TRUE;
  int 		oldCmd;
  float 	oldx,oldy,oldz,olda;
  float 	dx,dy,dz,da;
  time_t 	time_start,time_now;
   
  printf("\nInitiating motion complete... \n");

  // enter data to process log file
  //sprintf(proc_scratch,"      Waiting for motion to complete.  Cmds in buffer = %d \n",(MAX_BUFFER-bufferAvail));
  //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
  
  motion_is_complete=FALSE;						// set to indicate we may still be moving

  printf("  checking tinyg_rcv...\n");
  while(tinyGRcv(0)>=0);						// get latest tinyg feedback

  printf("  checking buffer...\n");
  oldCmd=cmdControl;							// save current state of cmdControl
  cmdControl=0;								// turn off for this.  no need to buffer these requests
  time_start=time(NULL);
  while(TRUE)								// loop until buffer empty. need to force entry into this loop (hence while(TRUE))
    {
    oldx=PostG.x; oldy=PostG.y; oldz=PostG.z; olda=PostG.a;		// save current positions
    print_thread_alive=TRUE;						// broadcast that still operational
    sprintf(gcode_cmd,"{\"qr\":null}\n");				// req buffer report
    tinyGSnd(gcode_cmd);						// send request
    while(tinyGRcv(0)==0);						// get tinyg feedback - this will update position if moving
    if((MAX_BUFFER-bufferAvail)<2)break;				// leave as soon as possible
    
    // calc change in position. if any change reset start time
    dx=fabs(oldx-PostG.x); dy=fabs(oldy-PostG.y);			// calc change in position
    dz=fabs(oldz-PostG.z); da=fabs(olda-PostG.a);
    if((dx+dy+dz+da)>CLOSE_ENOUGH)time_start=time(NULL);		// get time of last motion
    
    // get current time
    time_now=time(NULL);
    
    // if no motion for more than 2 secs... clear buffer and exit.
    // not known why tinyG does not completely clear buffer some times, but when a value of "1" persists
    // it seems to stay there regardless of how many move are queued up subsequently.  clearing and
    // moving on seems to be harmless.
    if((time_now - time_start)>2 && (dx+dy+dz+da)<CLOSE_ENOUGH)
      {
      tinyGSnd("%\n");							// clear the buffer
      while(tinyGRcv(0)==0);						// get all tinyG has to say
      bufferAvail=MAX_BUFFER;						// resync buffer space after clearing buffer
      break;
      }
    
    // if dormant for more than max allowed... exit
    if((time_now - time_start)>max_time_allowed)status=FALSE;
    
    // if failed for some reason...
    if(status==FALSE)
      {
      printf("\n\nERROR - Motion complete timed out waiting for buffer to empty!\n");
      printf("        Unable to empty motion control buffer.  time delta=%d \n\n",(time_now-time_start));
      tinyGSnd("!\n");							// something went wrong... stop
      tinyGSnd("%\n");							// clear the buffer
      while(tinyGRcv(0)==0);						// get all tinyG has to say
      bufferAvail=MAX_BUFFER;						// resync buffer space after clearing buffer
      job.state=JOB_PAUSED_DUE_TO_ERROR;
      job.sync=TRUE;
      break;
      }
    }
    
  printf("  checking for motion...\n");
  if(status==TRUE)							// if buffer empty was successful...
    {
    time_start=time(NULL);
    while(TRUE)								// loop until motion is complete. need to force entry into this loop (hence while(TRUE))
      {
      oldx=PostG.x; oldy=PostG.y; oldz=PostG.z; olda=PostG.a;		// save current positions
      print_thread_alive=TRUE;						// broadcast that still operational
      sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
      tinyGSnd(gcode_cmd);						// send request
      while(tinyGRcv(0)==0);						// get tinyg feedback - this will update position if moving
      if(tinyG_in_motion==FALSE)break;					// leave as soon as possible

      // if any motion at all reset start time
      dx=fabs(oldx-PostG.x); dy=fabs(oldy-PostG.y);			// calc change in position
      dz=fabs(oldz-PostG.z); da=fabs(olda-PostG.a);
      if((dx+dy+dz+da)>CLOSE_ENOUGH)time_start=time(NULL);
    
      // get current time
      time_now=time(NULL);
    
      // if no motion for more than 2 secs...
      //if((time_now - time_start)>2 && (dx+dy+dz+da)<CLOSE_ENOUGH)status=FALSE;
      if((time_now - time_start)>2 && (dx+dy+dz+da)<CLOSE_ENOUGH)break;
      
      // if dormant for more than max allowed...
      if((time_now - time_start)>max_time_allowed)status=FALSE;
    
      // if failed for some reason...
      if(status==FALSE)
	{
	printf("\n\nERROR - Motion complete timed out waiting for last move to complete!\n");
	printf("        Unable to complete last move.  time delta=%d \n\n",(time_now-time_start));
	tinyGSnd("!\n");						// something went wrong... stop
	tinyGSnd("%\n");						// clear the buffer
	while(tinyGRcv(0)==0);						// get all tinyG has to say
	bufferAvail=MAX_BUFFER;						// resync buffer space after clearing buffer
	job.state=JOB_PAUSED_DUE_TO_ERROR;
	job.sync=TRUE;
	break;
	}
      }
    }
    
  cmdControl=oldCmd;							// restore cmdControl
  motion_is_complete=TRUE;
  printf("... motion complete done with status %s. \n\n",status ? "OKAY" : "ERROR");
  
  return(status);
}

// short version of motion complete
inline void motion_done(void)
{
  float 	dx,dy,dz,da;
  float 	oldx,oldy,oldz,olda;
  time_t 	time_start,time_now;
  
  time_start=time(NULL);
  sprintf(gcode_cmd,"{\"qr\":null}\n");					// req buffer status
  tinyGSnd(gcode_cmd);							// send request
  sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
  tinyGSnd(gcode_cmd);							// send request
  while(tinyGRcv(0)==0);						// get tinyg feedback - this will update position if moving
  while(TRUE)
    {
    oldx=PostG.x; oldy=PostG.y; oldz=PostG.z; olda=PostG.a;		// save current positions
    print_thread_alive=TRUE;						// broadcast that still operational
    sprintf(gcode_cmd,"{\"qr\":null}\n");				// req buffer status
    tinyGSnd(gcode_cmd);						// send request
    sprintf(gcode_cmd,"{\"mots\":null}\n");				// req motion status
    tinyGSnd(gcode_cmd);						// send request
    while(tinyGRcv(0)==0);						// get tinyg feedback - this will update position if moving
    
    // calc change in position. if any change reset start time
    dx=fabs(oldx-PostG.x); dy=fabs(oldy-PostG.y);			// calc change in position
    dz=fabs(oldz-PostG.z); da=fabs(olda-PostG.a);
      
    // leave if nothing in buffer and no motion happening
    if((bufferAvail==MAX_BUFFER) && ((dx+dy+dz+da)<CLOSE_ENOUGH))break;	
    
    // sometimes it seems buffers get out of sync.  if no motion happening for more than 2 secs, leave.
    if(fabs(bufferAvail-MAX_BUFFER)>0 && (dx+dy+dz+da)<CLOSE_ENOUGH)
      {
      time_now=time(NULL);
      if((time_now - time_start)>2)break;
      }
    }
  return;
  
}

// Function to set tool PWM duty cycle (i.e. old AUX3 pin)
// Input duty cycle is in %.  That is, a value bt 0 and 100.
// Recall, to keep things thread safe, the thermal thread is where adjustments
// to the PWM module are made.  This functions simply sets the global params
// that the thermal thread will use to make those changes.
int tool_PWM_set(int slot, int port, int new_duty)
{
  
  if(slot<0 || slot>=MAX_THERMAL_DEVICES)return(-1);

  // validate port, if good then process
  if(port==TOOL_24V_PWM || port==TOOL_48V_PWM || port==TOOL_DIR_PWM)
    {
  
    // evaluate inputs
    if(new_duty<=0)new_duty=0;
    if(new_duty>100)new_duty=100;
    PWM_duty_cycle=new_duty;
    PWM_direct_port=port;
  
    // set global to allow PWM controller to change
    PWM_direct_control=TRUE;
    }
    
  return(TRUE);
}

// Function to read current PWM setting and return duty cycle
int tool_PWM_check(int slot, int port)
{
  time_t 	time_start,time_now;
  
  if(slot<0 || slot>=MAX_THERMAL_DEVICES)return(-1);
  
  if(port==TOOL_24V_PWM || port==TOOL_48V_PWM || port==TOOL_DIR_PWM)
    {
    // init for direct control
    PWM_direct_port=port;						// set the port to user request
    PWM_direct_status=FALSE;						// set flag to verify if read happens
    PWM_direct_control=TRUE;						// enable direct control

    // wait for PWM read confirmation
    time_start=time(NULL);
    while(PWM_direct_status!=TRUE)
      {
      delay(10);
      time_now=time(NULL);
      if((time_now - time_start)>3)break;
      }
      
    PWM_direct_control=FALSE;
    }
    
  return(PWM_duty_report);
}

// Function to ramp up PWM drive
// note:  max_duty is between 0.00 and 1.00 for direct control
// if the recommended duty cycle for the material is desired, set max_duty above 1.00
int tool_power_ramp(int slot, int port, int new_duty)
{
  char		proc_scratch[255];
  float 	duty_cycle;

  if(slot<0 || slot>=MAX_THERMAL_DEVICES)return(-1);

  // validate port, if good then process
  if(port==TOOL_24V_PWM || port==TOOL_48V_PWM || port==TOOL_DIR_PWM)
    {
    // enter data to process log file
    //sprintf(proc_scratch,"      Tool PWM ramp.  Slot=%d  Port=%d  Duty_cycle=%f \n",slot,port,new_duty);
    //if(proc_log!=NULL){fwrite(proc_scratch,1,strlen(proc_scratch),proc_log); fflush(proc_log);}
  
    // init for direct control
    PWM_direct_port=port;						// set the port to user request
    
    // if request is to shut power off...
    if(new_duty<=0)
      {
      printf("Tool PWM: shutting off.\n");
      new_duty=0;
      PWM_duty_cycle=new_duty;
      PWM_direct_control=TRUE;
      }
    else 
      {
      if(new_duty>100)new_duty=100;
    
      // ramp up power.  if done too quickly the current limiter on the driver chip will be exceeded and
      // shut tool power off.
      printf("Tool PWM: ramping up to %d duty cycle. \n",new_duty);
      duty_cycle=0.001;
      while(duty_cycle<new_duty)
	{
	PWM_direct_control=TRUE;					// must keep re-setting for each pass thru loop
	PWM_duty_cycle=(int)(100*duty_cycle);				// set based on duty cycle
	duty_cycle+=0.05;						// increment up power
	delay(500);							// wait long enough for thermal thread to apply changes
	}
      }
    }

  return(TRUE);
}

// Function to set the holding torque of stepper motors
// status: 0=disabled, 1=always on, 2=on when in cycle, 3=on when moving
void set_holding_torque(int status)
{
  sprintf(gcode_cmd,"{1pm:%d}\n",status);				// set X axis
  tinyGSnd(gcode_cmd);
  delay(150);								// must give tinyG time to clear buffer and settle else subsequent cmds also get cleared
  while(cmdCount>0)tinyGRcv(1);
  sprintf(gcode_cmd,"{2pm:%d}\n",status);				// set Y axis
  tinyGSnd(gcode_cmd);
  delay(150);							
  while(cmdCount>0)tinyGRcv(1);
  sprintf(gcode_cmd,"{3pm:%d}\n",status);				// set Z axis
  tinyGSnd(gcode_cmd);
  delay(150);							
  while(cmdCount>0)tinyGRcv(1);
  sprintf(gcode_cmd,"{4pm:%d}\n",status);				// set A axis
  tinyGSnd(gcode_cmd);
  delay(150);							
  while(cmdCount>0)tinyGRcv(1);
  
  return;
}

// Function to move carriage to machine home
int goto_machine_home(void)
{
  
    PosIs.x=PostG.x;							// record current position
    PosIs.y=PostG.y;
    PosIs.z=PostG.z;

    #ifdef ALPHA_UNIT
      sprintf(gcode_burst,"G90 G0 Z%f F300\n",PostG.z+10.0);		// raise carriage out of the way
      tinyGSnd(gcode_burst);
      sprintf(gcode_burst,"G90 G0 X0.0 Y0.0 \n");			// move carriage to home
      tinyGSnd(gcode_burst);
      while(tinyGRcv(1)>=0);						// get tinyg feedback
      sprintf(gcode_burst,"G0 X0.0 Y0.0 Z0.0 \n");			// move carriage to home
      tinyGSnd(gcode_burst);
      while(tinyGRcv(1)>=0);						// get tinyg feedback
    #endif
    #ifdef BETA_UNIT
      sprintf(gcode_burst,"G90 G0 Z%f F300\n",PostG.z+10.0);		// raise carriage out of the way
      tinyGSnd(gcode_burst);
      sprintf(gcode_burst,"G90 G0 X0.0 Y0.0 \n");			// move carriage to home
      tinyGSnd(gcode_burst);
      while(tinyGRcv(1)>=0);						// get tinyg feedback
      sprintf(gcode_burst,"G0 X0.0 Y0.0 Z0.0 \n");			// move carriage to home
      tinyGSnd(gcode_burst);
      while(tinyGRcv(1)>=0);						// get tinyg feedback
    #endif
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
      if(PostG.z<(ZHOMESET-10.0))					// if lower than home...
        {
        sprintf(gcode_burst,"G0 Z%f \n",PostG.z+10.0);			// raise carriage out of the way
        tinyGSnd(gcode_burst);
	}
      sprintf(gcode_burst,"G1 X0.0 Y0.0 Z%7.3f F10000 \n",ZHOMESET);	// move carriage to home
      tinyGSnd(gcode_burst);
      while(tinyGRcv(1)>=0);						// get tinyg feedback
    #endif
    
    motion_complete();
    make_toolchange(-1,0);						// turn off all tools
    PosIs.x=PostG.x;							// record current position
    PosIs.y=PostG.y;
    PosIs.z=PostG.z;

    printf("\nTurning off holding torque: \n");
    set_holding_torque(OFF);
    printf("\n");
    
    carriage_at_home=TRUE;
  
  return(1);
}

// Function to move to part from machine home
// Input:  slot=tool slot, vtarget = pointer to the vertex the machine is going to
int comefrom_machine_home(int slot, vertex *vtarget)
{
    vertex 	*vmid;
    
    if(slot<0 || slot>=MAX_TOOLS)return(0);
    if(vtarget==NULL)return(0);

    printf("\nTurning on holding torque: \n");
    set_holding_torque(ON);
    printf("\n");

    #if defined(ALPHA_UNIT) || defined(BETA_UNIT)			// because home position is at z=0 we must raise z to not scrape across build table
      tinyGSnd("{\"posz\":NULL}\n");					// get current z position
      while(tinyGRcv(1)>=0);						// ensure it is processed
      sprintf(gcode_cmd,"G0 Z%f \n",PostG.z+10);			// move the carriage up 10 mm
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
    #endif
      
    // move to target position
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    vmid=vertex_make();							// make a temporary vtx
    vmid->x=vtarget->x; vmid->y=vtarget->y; vmid->z=vtarget->z+10;	// set mid pt to 10mm above xy target
    vmid->attr=vtarget->attr; 
    vmid->k=10000;							// set default drop to fast (G0 speed)
    print_vertex(vmid,slot,7,NULL);					// pen-up to mid pt at full speed
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();

    vtarget->k=1000;							// set default drop to fast (G0 speed)
    if(Tool[slot].tool_ID==TC_ROUTER)vtarget->k=300;			// set cutting drop speed to slow
    if(vtarget->k<100)vtarget->k=100;
    if(vtarget->k>40000)vtarget->k=40000;
    print_vertex(vtarget,slot,7,NULL);					// pen-up to the start of the polygon
    tinyGSnd(gcode_burst);						// send string of commands to fill buffer
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();
    
    free(vmid); vertex_mem--; vmid=NULL;				// release temporary vtx from memeory
    carriage_at_home=FALSE;
    
    return(1);
    
}

// Function to build the job that is currently defined by tools, materials, and models.
// Note that this function runs as its own thread, which means caution must be applied when
// doing anything with variables that can be modified outside this thread.  Case in point,
// the proc_log file entries have to be direct here (vs thru a separate function) otherwise
// threads crash (learned that one the hard way!)
// To help with thread safe, local copies of job struct and tool struct are used for the
// logic within this thread, then synced with the public versions at safe intervals.
void* build_job(void *arg)
{
  float 	dist=0.0,min_stay_on_dist;
  int		zlyr=0;
  float 	z_ht=0.0, old_z;
  float 	z_overshoot=1.00;
  float 	dx,dy,cmdBLen,delta_z=0.0,delta_z_min=0.0;
  float 	crt_drill_diam=(-1.0),pre_drill_diam=(-1.0);
  float 	cal_touch_depth=0.0;
  float 	depTemp,oldTemp;
  float 	original_xoff,original_yoff;
  float 	old_td=0.0,pre_matl[MAX_TOOLS];
  float 	XPark,YPark,ZPark;
  float 	pwm_duty;
  int 		h=0,i=0,hfld=0,hist_ctr_run;
  int		mtyp=MODEL; 
  int		x_copy_ctr,y_copy_ctr;
  int		opr,current_layer=0;
  int		slot=(-1),next_slot=(-1),prev_slot=(-1);
  int		tool_use_count,tool_used[4];
  int		tool_up_flag=FALSE;
  int		first_model_flag;
  int		pause_at_end_of_layer=FALSE;
  int		burst_ctr=0;
  int		tinyG_err=0;
  int		print_err=0;
  int		pick_start;
  int		job_hours,job_seconds;
  int		vtx_overlap,vector_count;
  int		styp,ptyp;
  int		penup_mode,all_ok;
  int		zh=0;
  int		draw_calib_lines,old_auto_zoom;
  int		on_ticks;
  int		rerun_flag=FALSE;
  int		repeat_first_slice=FALSE;
  slice		*sptr,*last_sptr,*sptr_check,*slc_test;
  polygon_list	*pl_ptr;
  polygon	*pptr,*pptr_start;
  vertex	*vptr,*vnxt,*vold,*vcal_start,*vcal_end;
  vertex 	*vptr_p0,*vptr_p1,*vtx_aside;
  vector	*vecptr;
  model		*mptr,*mchk;
  linetype	*lptr;
  operation	*optr;
  genericlist	*aptr,*lt_ptr,*tl_optr;
  int		layer_time;
  time_t 	layer_start_time,layer_finish_time;
  time_t 	lt_start_time,lt_finish_time,lt_time_delta;
  time_t 	oh_start_time,oh_finish_time,oh_time_delta;
  time_t	curtime;
  struct	tm *loc_time;
  FILE		*print_log;
  char		prt_scratch[255],img_name[255];				// scratch string for this thread only
  
  jobx		local_job;						// local version of job to protect thread safe operation
	
  // init variables
  PosIs.x=0;PosIs.y=0;PosIs.z=0;
  PosWas.x=0;PosWas.y=0;PosWas.z=0;
  printf("Print control thread launched...\n");
  
  while(TRUE)
    {
    print_thread_alive=TRUE;
    delay(250);
    
    // just spin waiting for job to be ready
    if(job.state!=JOB_RUNNING)continue;					
    
    // at this point, a job has be requested to run so copy the public job over to the local job
    // to minimize thread conflicts
    job.sync=FALSE;
    memcpy(&local_job,&job,sizeof(job));

    // check that user has set everything needed to run the job
    if(local_job.model_first==NULL)
      {
      local_job.state=JOB_ABORTED_BY_SYSTEM; 
      job.state=local_job.state;
      continue;
      }
  
    job_count_total++;							// total count of jobs run on this unit
    save_unit_state();							// save unit info to system log file
    printf("Print: starting print.\n");
    
    // if saving a logfile for this local_job...
    if(save_logfile_flag==TRUE)
      {
      open_job_log();
      mptr=local_job.model_first;
      while(mptr!=NULL)
        {
	sprintf(prt_scratch,"\n; Model = %s\n",mptr->model_file);
	if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	if(gcode_gen!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_gen);
	aptr=mptr->oper_list;
	while(aptr!=NULL)
	  {
	  sprintf(prt_scratch,";  Operation = %s   Tool = %d \n",aptr->name,aptr->ID);
	  if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	  if(gcode_gen!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_gen);
	  aptr=aptr->next;
	  }
	mptr=mptr->next;
	}
      sprintf(prt_scratch,"; ------------------------------------------------\n");
      if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
      if(gcode_gen!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_gen);
      }
    
    // open log file for error recording/debug
    // note this file is thread specific.  writing "outside of" or "across" threads is dangerous because it takes too much time.
    print_log=fopen("print.log","w");

    // scan build table to register flatness or if something is on it
    //build_table_scan();
    
    // update job maxmin boundaries
    job_maxmin();							// this will work off public copy
    local_job.XMin=job.XMin; local_job.XMax=job.XMax;
    local_job.YMin=job.YMin; local_job.YMax=job.YMax;
    local_job.ZMin=job.ZMin; local_job.ZMax=job.ZMax;
    
    // flush any existing cmds from buffer
    cmdControl=1;
    //cmdCount=0;
    for(i=0;i<MAX_BUFFER;i++)memset(cmd_buffer[i].cmd,0,sizeof(cmd_buffer[i]));		
    cmdBurst=1;
    old_auto_zoom=set_auto_zoom;
    set_auto_zoom=FALSE;
  
    // if extruder is using a shear thinning material, execute prime purge to initiate flow
    /*
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(Tool[slot].state!=TL_READY)continue;
      if(Tool[slot].tool_ID!=TC_EXTRUDER)continue;
      if(Tool[slot].matl.primehold>0)
	{
	make_toolchange(slot,1);						// change to material delivery control

	sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.primepush);	// press pluger to build pressure
	tinyGSnd(gcode_cmd);
	
	delay(Tool[slot].matl.primehold);					// hold at pressure to get flow going
	//while(!kbhit());
	
	sprintf(gcode_cmd,"G1 A%07.3f F50.0 \n",Tool[slot].matl.primepull);	// reduce pressure to slow flow
	tinyGSnd(gcode_cmd);
	
	Tool[slot].matl.primehold=0.0;					// set to 0 to prevent doing it again
	}
      }
    */
    
    // home ALL loaded tools if not done yet.  even if not used in this job, they gotta be outta the way.
    // this must be done while at 0,0,0 otherwise tools may hit the build table
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      tool_used[slot]=FALSE;						// init all as un-used
      if(Tool[slot].state==TL_EMPTY)continue;				// skip if not viable
      sprintf(job_status_msg," Homing tool tip positions. ");
      tool_tip_home(slot);
      if(job.sync==TRUE){if(print_status(&local_job)==0)break;}		// check for pause/abort/etc.
      }

    // determine which tools this job is using so we don't waste time calibrating tools that are not used
    tool_use_count=0;
    mptr=local_job.model_first;
    while(mptr!=NULL)
      {
      // loop thru all operations of this model and see which slots (tools) it uses
      // note that one model may use multiple tools, or each model could use a seperate tool, or all same tool
      aptr=mptr->oper_list;
      while(aptr!=NULL)
	{
	slot=aptr->ID;							// get tool for this operation
	if(slot>=0 && slot<MAX_TOOLS)					// if a valid slot...
	  {
	  if(tool_used[slot]==FALSE)					// if not already set for use in prev model/operation...
	    {
	    tool_used[slot]=TRUE;					// ... indicate it will be used
	    tool_use_count++;						// ... increment count of tools used
	    }
	  }
	aptr=aptr->next;
	}
      mptr=mptr->next;
      }
      
    // draw calibration lines
    // this operation only needs loaded/used tools, it is completely model independent
    // this operation cannot run if starting anywhere other than z_cut=0 (on the build table)
    draw_calib_lines=FALSE;
    if(set_start_at_crt_z==FALSE && draw_calib_lines==TRUE)
      {
      first_layer_flag=TRUE;
      active_model=NULL;
      adjfeed_flag=0;
      adjflow_flag=0;
      PosIs.x=0.0;PosIs.y=0.0;PosIs.z=0.0;				// init starting carriage postion
      for(slot=0;slot<MAX_TOOLS;slot++)
	{
	if(Tool[slot].state<TL_READY || Tool[slot].state==TL_FAILED)continue;	// skip if not viable
	if(Tool[slot].type!=ADDITIVE)continue;
	if(tool_used[slot]!=TRUE)continue;
	//Tool[slot].state=TL_ACTIVE;
	linet_state[slot]=MDL_BORDER;
	make_toolchange(slot,1);					// set tool and matl drive
	sprintf(job_status_msg," Printing calibration lines. ");
	
	// define calibration line coords along low x stretching from low y to high y
	vcal_start=vertex_make();
	vcal_start->x=(slot+1);
	vcal_start->y=(slot+1);
	vcal_start->z=Tool[slot].matl.layer_height;
	
	vcal_end=vertex_make();
	vcal_end->x=BUILD_TABLE_LEN_X-15-(slot+1);
	vcal_end->y=(slot+1);
	vcal_end->z=Tool[slot].matl.layer_height;
	
	// draw calibration line
	print_vertex(vcal_start,slot,3,NULL);				// generate pen up over to start
	tinyGSnd(gcode_burst);						// execute pen up move
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	motion_complete();						// wait until it completes
	tool_tip_deposit(slot);						// move tool tip into position
	oldTemp=Tool[slot].thrm.operC;					// save current operating temperature
	depTemp=oldTemp;						// default depostion temp
	lptr=linetype_find(slot,MDL_BORDER);				// get pointer to line type
	if(lptr!=NULL)depTemp=oldTemp+lptr->tempadj;			// if pointer valid, adjust deposition temperature
	temperature_adjust(slot,depTemp,TRUE);				// ensure tool at temperature
	tool_matl_forward(slot,Tool[slot].matl.advance);		// advance material to tip
	tool_tip_touch(cal_touch_depth);				// initiate material contact
	print_vertex(vcal_end,slot,1,lptr);				// generate pen down without z comp
	tinyGSnd(gcode_burst);						// execute pen down move
	motion_complete();						// wait until it full completes
	memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	
	// extra code to draw box around entire build volume in XY
	if(Tool[slot].tool_ID==TC_FDM)
	  {
	  vcal_end->x=BUILD_TABLE_LEN_X-15-(slot+1);
	  vcal_end->y=BUILD_TABLE_LEN_Y-10-(slot+1);
	  vcal_end->z=Tool[slot].matl.layer_height;
	  tool_tip_touch(cal_touch_depth);				// initiate material contact
	  print_vertex(vcal_end,slot,1,lptr);				// generate pen down without z comp
	  tinyGSnd(gcode_burst);					// execute pen down move
	  motion_complete();						// wait until it full completes
	  memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	  
	  vcal_end->x=(slot+1);
	  vcal_end->y=BUILD_TABLE_LEN_Y-10-(slot+1);
	  vcal_end->z=Tool[slot].matl.layer_height;
	  tool_tip_touch(cal_touch_depth);				// initiate material contact
	  print_vertex(vcal_end,slot,1,lptr);				// generate pen down without z comp
	  tinyGSnd(gcode_burst);					// execute pen down move
	  motion_complete();						// wait until it full completes
	  memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string
	  
	  vcal_end->x=(slot+1);
	  vcal_end->y=(slot+1);
	  vcal_end->z=Tool[slot].matl.layer_height;
	  tool_tip_touch(cal_touch_depth);				// initiate material contact
	  print_vertex(vcal_end,slot,1,lptr);				// generate pen down without z comp
	  tinyGSnd(gcode_burst);					// execute pen down move
	  motion_complete();						// wait until it full completes
	  memset(gcode_burst,0,sizeof(gcode_burst));			// clear command string

	  tool_tip_touch(cal_touch_depth);				// initiate material contact
	  motion_complete();						// wait until it full completes
	  }
	
	if(lptr!=NULL)lptr->touchdepth=old_td;				// reset back to prev value
	tool_matl_retract(slot,Tool[slot].matl.retract);		// retract material from tip
	tool_tip_retract(slot);						// retract tool tip away from build table
	      
	// release calibration line coordinates from memory
	free(vcal_start); vertex_mem--;
	free(vcal_end);	vertex_mem--;
	Tool[slot].state=TL_READY;					// shift state from active back to ready
	linet_state[slot]=UNDEFINED;

	if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	}
      }

    // clear history if starting from first layer
    if(set_start_at_crt_z==FALSE)
      {
      for(h=0;h<MAX_HIST_COUNT;h++){history[H_MOD_TIM_ACT][h]=0.0;}	// clear existing values of actuals (if any)
      for(h=0;h<MAX_HIST_COUNT;h++){history[H_SUP_TIM_ACT][h]=0.0;}
      for(h=0;h<MAX_HIST_COUNT;h++){history[H_MOD_MAT_ACT][h]=0.0;}
      for(h=0;h<MAX_HIST_COUNT;h++){history[H_SUP_MAT_ACT][h]=0.0;}
      hist_ctr_run=0;							// reset local counter value
      }

    // draw base layers if requested
    // loop thru models of each tool and look at which tool they request for their base layers.
    // the catch is that base layers are multiple layers so this must be done between z increments.
    // typically done from same tool, or tool with same material, but can be any active tool
    if(local_job.baselayer_flag==TRUE && set_start_at_crt_z==FALSE)
      {
      printf("Entering base layer printing. \n");
  
      mptr=local_job.model_first;
      while(mptr!=NULL)
	{
	if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	if(local_job.state!=JOB_RUNNING)break;				// accounts for nested looping

	// determine if this model will get a base layer by looking thru its operations list
	aptr=mptr->oper_list;						// start with first oper in list					
	while(aptr!=NULL)						// loop thru all operations in sequence
	  {
	  if(strstr(aptr->name,"ADD_BASE_LAYER")!=NULL)break;		// if adding a base layer... exit with pointer to oper
	  aptr=aptr->next;						// move on to next oper
	  }
	if(aptr==NULL){mptr=mptr->next; continue;}			// if no base layer oper was found... move onto next model
	
	printf("print baselayer found!\n");
	
	// determine if there is layer data for the base layer
	sptr=mptr->base_layer;						// get pointer to base layer slice data
	if(sptr==NULL){mptr=mptr->next;continue;}			// if not there for some reason
	if(sptr->pqty[MDL_OFFSET]<=0){mptr=mptr->next;continue;}	// if nothing to process... move on

	// determine which tool is depositing the base layer
	slot=aptr->ID;							// get slot based on operation
	if(mptr->base_layer==NULL){mptr=mptr->next;continue;}		// if model does not have baselayer defined... skip
	if(slot<0 || slot>=MAX_TOOLS){mptr=mptr->next;continue;}	// if tool is out of range... skip
	if(Tool[slot].state<TL_READY || Tool[slot].state==TL_FAILED){mptr=mptr->next;continue;}	// if tool is not ready... skip
	if(Tool[slot].type!=ADDITIVE){mptr=mptr->next;continue;}	// if tool is not putting down material... skip
	
	// make sure that depositing a base layer is something this tool can do
	optr=operation_find_by_name(slot,aptr->name);			// get pointer to base layer tool operation
	if(optr==NULL){mptr=mptr->next;continue;}			// if this tool does not support this operation...
	
	printf("baselayer being made with tool %d \n",slot);
	
	// get tool ready to deposit
	tool_matl_forward(slot,Tool[slot].matl.advance);		// advance material to tip
	
	// loop through all line types identified in the slice's deposition sequence for this operation
	lt_ptr=optr->lt_seq;
	while(lt_ptr!=NULL)						
	  {
	  if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	  if(local_job.state!=JOB_RUNNING)break;			// accounts for nested looping

	  // now get ptr to linetype for rest of data and proceed
	  ptyp=lt_ptr->ID;
	  lptr=linetype_find(slot,ptyp);
	  printf("baselayer:  using ptyp=%d and lptr=%x \n",ptyp,lptr);
	  if(lptr==NULL){lt_ptr=lt_ptr->next; continue;}
	  printf("baselayer:  sptr=%X  sz=%5.3f  ptyp=%d \n",sptr,sptr->sz_level,ptyp);
	  slice_fill_model(mptr,sptr->sz_level);			// generate base layer fills 
	  oper_state[slot]=OP_ADD_BASE_LAYER;				// set global to display operation state
	  linet_state[slot]=ptyp;
	  active_model=mptr;
	  mptr->base_lyr_zoff += lptr->thickness;			// adjust z of model to compensate for base layer thickness
	  
	  // loop thru all copies of this model in x then in y before next loop change to platform layers
	  original_xoff=mptr->xorg[mtyp];
	  original_yoff=mptr->yorg[mtyp];
	  mptr->active_copy=0;						// init total copy counter
  
	  for(x_copy_ctr=0;x_copy_ctr<mptr->xcopies;x_copy_ctr++)	// loop thru all in X
	    {
	    if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	    if(local_job.state!=JOB_RUNNING)break;			// accounts for nested looping
	    mptr->xoff[mtyp]=original_xoff+x_copy_ctr*(mptr->xmax[mtyp]-mptr->xmin[mtyp]+mptr->xstp);	// calc X offset
	    for(y_copy_ctr=0;y_copy_ctr<mptr->ycopies;y_copy_ctr++)	// loop thru all in Y
	      {
	      if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	      if(local_job.state!=JOB_RUNNING)break;			// accounts for nested looping
	      mptr->yoff[mtyp]=original_yoff+y_copy_ctr*(mptr->ymax[mtyp]-mptr->ymin[mtyp]+mptr->ystp);	// calc Y offset
	      if(mptr->active_copy>=mptr->total_copies)continue;	// if beyond total copies...
	      mptr->active_copy++;					// increment copy counter
	      pptr=sptr->pfirst[ptyp];					// point to start of polygon list for this type in this slice
	      if(pptr==NULL)continue;					// if polygon list enpty... move on
	      vptr=pptr->vert_first;					// start vertex
	      vptr->z=0.0;						// set z level (undefined at this point)
	      //vptr->attr=ptyp;						// set the vtx attribute to the polygon type (aka line type)
	      print_vertex(vptr,slot,2,NULL);				// start with a pen-up to the start of the polygon
	      tinyGSnd(gcode_burst);					// send string of commands to fill buffer
	      memset(gcode_burst,0,sizeof(gcode_burst));		// null out burst string
	      if(mptr->active_copy==1)					// if the first copy, then set up tool ...
		{
		oldTemp=depTemp;
		depTemp=Tool[slot].thrm.operC+lptr->tempadj;		// calculate new temperature for this line type
		temperature_adjust(slot,depTemp,TRUE);			// ensure tool at temperature
		tool_tip_deposit(slot);					// drop tool tip into position to deposit
		make_toolchange(slot,1);				// set material drive active
		//Tool[slot].matl.mat_pos+=lptr->advance;			// add advance to current material position
		//tool_matl_forward(slot,Tool[slot].matl.advance);	// advance material to tip
		}
	      pptr=sptr->pfirst[ptyp];
	      while(pptr!=NULL)						// while we still have polygons to process
		{
		if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		if(local_job.state!=JOB_RUNNING)break;				// accounts for nested looping
		cmdBurst=1;
		burst_ctr=0;						// init burst counter to count down bursted cmds
		vector_count=0;						// init vector counter used to count up number of vtx sent to tinyG in this polygon
		vtx_overlap=0;						// set the number of vtxs to overlap on borders/offsets to fix start/stop seam
		vptr=pptr->vert_first;					// start at first vertex
		if(vptr==NULL){pptr=pptr->next;continue;}		// if vertex list empty...
		vptr->z=0.000;						// set z level
		//vptr->attr=ptyp;					// set the vtx attribute to the polygon type (aka line type)
		print_vertex(vptr,slot,3,NULL);				// start with a PU to the start of the polygon
		tool_tip_touch(lptr->touchdepth);			// touch tip to surface if requested
		do{
		  if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		  vptr=vptr->next;					// move onto next vertex
		  if(vptr==NULL)break;					// handles "open" polygons like FILL
		  vptr->z=0.000;					// just in case z value has not been updated
		  vptr->i=0;						// default to not an overlap vector
		  if(vector_count>=pptr->vert_qty)vptr->i=1;		// indicate that this is an overlap vector
		  //vptr->attr=ptyp;
		  print_vertex(vptr,slot,1,lptr);			// send PD deposit command to tinyG
		  burst_ctr++;
		  if(burst_ctr>=cmdBurst || vptr==pptr->vert_first)
		    {
		    tinyGSnd(gcode_burst);				// send string of commands to fill buffer
		    memset(gcode_burst,0,sizeof(gcode_burst));		// null out burst string
		    burst_ctr=0;					// reset burst count down counter
		    cmdBurst=1;						// reset to only 1 cmd per burst to not overload tinyG
		    }
		  vector_count++;
		  if(vector_count>=(pptr->vert_qty+vtx_overlap))break;	// stop after drawing all vectors and then some to cover start/stop seam
		  }while(vptr!=NULL);
		pptr=pptr->next;					// move on to next polygon
		}
	      motion_complete();
	      }		// end of y copy loop
	    }		// end of x copy loop
	  
	  lt_ptr=lt_ptr->next;
	  }
	  
	tool_matl_retract(slot,Tool[slot].matl.retract);		// retract material from tip
	tool_tip_retract(slot);						// retract tool tip away from build table
	if(lptr!=NULL)
	  {
	  if(lptr->tempadj>0)						// if tool is hotter than normal, adjust back down
	    {
	    depTemp=Tool[slot].thrm.operC;
	    temperature_adjust(slot,depTemp,FALSE);
	    make_toolchange(slot,1);
	    }
	  }
	Tool[slot].state=TL_READY;
	linet_state[slot]=UNDEFINED;
	mptr->base_lyr_zoff += 0.200;					// add extra offset to provide peel-ability
	mptr=mptr->next;
	}
      //local_job.baselayer_flag=FALSE;						// done with base layers... turn off
      }
    else 
      {
      // otherwise we need to at least get material up to the nozzle
      mptr=local_job.model_first;						// grab the first model
      aptr=mptr->oper_list;						// start with first oper in list					
      while(aptr!=NULL)							// loop thru all operations in sequence
	{
	if(strstr(aptr->name,"OP_ADD_MODEL_MATERIAL")!=NULL)break;	// if adding model matl... exit with pointer to oper
	aptr=aptr->next;						// move on to next oper
	}
      if(aptr!=NULL)							// if ptr was found...
        {
	// determine which tool we'll be using and get material up to it
	slot=aptr->ID;							// get slot based on operation
	if(slot>=0 && slot<MAX_TOOLS)tool_matl_forward(slot,Tool[slot].matl.advance);
	}
      }
 
    // init ALL loaded models' current z heights
    // mptr->current_z will be used to track what has been, or needs to be, deposited.
    // this can be tricky because mptr->current_z is in build volume coords (same as z_cut), so
    // it can appear that mptr->current_z is beyond mptr->zmax if mptr->zoff is not applied.
    mptr=local_job.model_first;
    if(set_start_at_crt_z==FALSE)z_cut=local_job.min_slice_thk;				// otherwise it is where the user set the slider bar
    while(mptr!=NULL)
      {
      mptr->current_z=0.0;								// ... start seek from bottom up
      if(set_start_at_crt_z==TRUE)mptr->current_z=z_cut-local_job.min_slice_thk;	// ... start seek from one slice below
      mptr=mptr->next;
      }
      
    printf("Print: A  zcut=%f \n",z_cut);
    
    // determine the starting model and starting z
    // find the model containing the thinnest slice in contact with the build surface (table if at z=0, model if at z=current)
    // search through the models looking at which tools they call out, and then check the specified tool layer thicknesses
    if(local_job.type==ADDITIVE)						// we want the model closest to the build table with the thinnest slice thickness
      {	
      if(set_start_at_crt_z==FALSE)					// if starting on the build table...
	{
	z_cut=CLOSE_ENOUGH;						// ... set to target first slice level
	mptr=job_layer_seek(UP);					// ... sets z_cut to new value matching first slice's z
	}
      else 							      	// otherwise we are starting at a user defined level...
	{
	mptr=job_layer_seek(LEVEL);					// ... find model that has slice at this level.  leave z_cut as is.
	}
      }
    if(local_job.type==SUBTRACTIVE || local_job.type==MARKING)		// we want to start at top surface of target
      {
      mptr=local_job.model_first;					// start with first model in job
      while(mptr!=NULL)							// loop thru all models
	{
	if(set_start_at_crt_z==FALSE)					// if starting on the build table...
	  {
	  z_cut=mptr->zmax[TARGET];					// ... set to top surface of target (i.e. bottom of model that sits on the target)
	  mptr->zoff[MODEL] = mptr->zmax[TARGET];		  	// ... lower so first slice lands on target
	  }
	else 
	  {
	  // if starting at current z, the model z offset is already set as is z_cut.
	  //mptr->zoff[MODEL]=mptr->zmax[TARGET]-(z_cut-mptr->zmax[TARGET]);
	  mptr->current_z=mptr->zmax[TARGET]-mptr->zoff[MODEL];		// set z of model to current z_cut/zoff
	  }
	mptr=mptr->next;
	}
      repeat_first_slice=TRUE;						// set to reprint same job over and over until aborted
      mptr=local_job.model_first;						// to be here local_job.model_first must exist
      }

    // determine tool (slot) that will be used with the stating model
    slot=(-1); next_slot=(-1); prev_slot=(-1);				// init to invalid tool
    aptr=mptr->oper_list;						// start with first operation for the starting model
    while(aptr!=NULL)							// loop thru all operations to find starting tool
      {
      printf("  mptr=%X  aptr->ID=%d  aptr->name=%s \n",mptr,aptr->ID,aptr->name);
      slot=aptr->ID;							// get slot intended to use for this operation
      if(slot>=0 && slot<MAX_TOOLS)break;				// if a valid slot, exit with slot set
      aptr=aptr->next;							// otherwise check next operation
      }
    if(aptr==NULL)							// if we couldn't find a tool for the starting model
      {
      printf("\n\nPrint: *** ERROR - no available tool! *** \n\n");
      local_job.state=JOB_ABORTED_BY_SYSTEM;						
      print_status(&local_job);
      }
    if(aptr!=NULL)
      {if(aptr->next!=NULL)next_slot=(aptr->next)->ID;}			// init next tool slot
    prev_slot=slot;							// init previous tool slot in use

    // init job params to match our (re)starting point
    sptr=slice_find(mptr,MODEL,slot,z_cut,mptr->slice_thick);		// find this model's slice at this z level
    if(sptr==NULL)sptr=mptr->slice_last[slot];				// null is not an option here
    //local_job.start_time=time(NULL)-sptr->time_estimate;		// go back in time to when it would have started
    local_job.current_z=z_cut;						// set to current view z slider position
    hist_ctr_run=(int)(local_job.current_z/local_job.min_slice_thk);	// set history counter to starting layer count
    
    PostGVCnt=1; PostGFCnt=1;						// init velocity and flow rate counters for performance view
    PostGVAvg=0; PostGFAvg=0;
    first_model_flag=TRUE;						// init that this is the first model to get attention
    first_layer_flag=TRUE;						// init that it's also the first layer
    last_sptr=NULL;
    rerun_flag=FALSE;							// init that this will be a fresh slice
    tool_air_override_flag=FALSE;					// init so that line type has control of air
    if(p_pick_list!=NULL)						// if user is running just the pick list polygons...
      {
      rerun_flag=TRUE; 							// ... set as if we are rerunning just polygons
      last_sptr=sptr;							// ... set "previous" slice to what current slice is
      }
    old_z=z_cut;							// save current z for future reference
    oh_start_time=time(NULL);						// get start time for over head
    layer_start_time=time(NULL);					// get start time for this layer
    
    tool_air(slot,OFF);							// turn tool air off
    if(Tool[slot].tool_ID==TC_LASER)Tool[slot].thrm.heat_duty=0.0;	// turn tool PWM off
    

    // PRIMARY OPERATING LOOP:  build/remove layers until we've reached the top/bottom ---------------------------------------
    while(TRUE)
      {
      if(mptr==NULL){printf("Print:  NULL model pointer unexpectedly encountered.  Aborting. \n"); break;}
      printf("\n\n****************************************************************************\n");
      printf("* Print:  z_cut=%6.3f  slot=%d   mz=%f  jz=%f  \n",z_cut,slot,mptr->current_z,local_job.current_z);
      printf("****************************************************************************\n\n");
      
      // check if user has hit pause or abort.  this is a fixed set of instructions to ideally
      // process inside each loop of the build cycle, preferably at the top of each loop.
      if(job.sync==TRUE){if(print_status(&local_job)==0)break;}		// check for pause/abort/etc.
      if(local_job.state!=JOB_RUNNING)break;				// account for nested loop abort requests

      active_model=mptr;						// expose which model is in use globally to rest of system
      fidelity_mode_flag=FALSE;						// default to not in fidelity mode
      if(active_model->fidelity_mode==TRUE)fidelity_mode_flag=TRUE;	// turn on if this model calls for it

      // enter data to print log file
      sprintf(prt_scratch,"\nModel=%s \n",active_model->model_file);
      if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}
      
      // loop thru all operations in sequence as required by this model at this layer.  recall that an
      // operation is completed with a specific tool.  so if this unit has multiple tools then this may
      // mean the tool (slot) needs to change as well.
      oh_finish_time=time(NULL);					// over head is time spent outside of operations		
      aptr=mptr->oper_list;						// get the pointer to start of operations list for this model
      while(aptr!=NULL)							// loop thru all operations in sequence
	{
	if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	if(local_job.state!=JOB_RUNNING)break;				// account for nested loop abort requests
	
	// get the slot and corresponding slice pointer for this z level
	printf("\n\nStart of operation:  %s  with tool  %d\n\n",aptr->name,aptr->ID);
	if(strstr(aptr->name,"ADD_BASE_LAYER")!=NULL){aptr=aptr->next;continue;}// base layer already done above
	slot=aptr->ID;								// set tool based on model input
	if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}		// skip if no operation defined or tool out of range
	if(Tool[slot].state<TL_READY){local_job.state=JOB_PAUSED_DUE_TO_ERROR;}	// if user unplugs tool...
	if(Tool[slot].state==TL_FAILED){local_job.state=JOB_PAUSED_DUE_TO_ERROR;}// if tool fails for some reason...

	// get pointer to slice data. note that models are all sliced with their min at 0.000 and the z offset
	// is compensated for later.  so the slice fetch must account for the z offset.  this is particularly
	// important when the model has a target (which means z_cut is constant at top of target) and the model's
	// zoff value is sinking into the target to get the next slice in the model.
	if(rerun_flag==TRUE){sptr=last_sptr;}					// if re-running last slice...
	else {sptr=slice_find(mptr,MODEL,slot,z_cut,mptr->slice_thick);} 	// ... otherwise find new one at z_cut
	if(repeat_first_slice==TRUE)sptr=mptr->slice_last[slot];		// if repeating same print over and over
	if(sptr!=NULL)								// if unable to locate a print-able slice then move onto next model
	  {
	  // enter data to print log file
	  sprintf(prt_scratch,"  Z Level=%f  Slice_Z=%f  ID=%d\n",z_cut,sptr->sz_level,sptr->ID);
	  if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}
      
	  // if slice pointer indicates a pause, set flag as such.  use a seperate flag since we want all
	  // slices from all loaded models/copies done before a z increment (which is outside the sptr loop).
	  pause_at_end_of_layer=FALSE;					// init to no pause
	  if(sptr->eol_pause==TRUE)pause_at_end_of_layer=TRUE;		// if user set pause on this layer...
	  if(local_job.type==MARKING)pause_at_end_of_layer=TRUE;		// if job is marking, puase at each marking type change...
	  
	  // get pointer to tool's corresponding operation.  this will provide access to mat'l and line type params.
	  printf("\nZ SYNC:  zcut=%f  slice_z=%f  ID=%d  Tzmax=%f \n\n",z_cut,sptr->sz_level,sptr->ID,mptr->zmax[TARGET]);
	  if(mptr->mdl_has_target==FALSE)z_cut=mptr->zoff[MODEL]+sptr->sz_level;			// readjust z_cut to exactly match slice value
	  optr=operation_find_by_name(slot,aptr->name);			// get pointer to tool operation that matches model operation
	  if(optr==NULL){aptr=aptr->next;continue;}			// if this tool does not support this operation...
	  opr=optr->ID;							// get the ID number of this operation
	  oper_state[slot]=opr;						// set global to display operation state
	  slice_fill_model(mptr,z_cut);					// generate fill & close-offs for this model at this z level
	  move_buffer_count=0;						// reset move count every layer
	  
	  // regardless of line type setting, turn on air if print time of slice is short
	  if(Tool[slot].type==ADDITIVE)
	    {
	    if(Tool[slot].tool_ID==TC_FDM)
	      {
	      if(sptr->time_estimate>0 && sptr->time_estimate<tool_air_slice_time){tool_air(slot,ON); tool_air_override_flag=TRUE;}
	      else {tool_air(slot,OFF); tool_air_override_flag=FALSE;}
	      }
	    }

	  // enter data to print log file
	  sprintf(prt_scratch,"    Slot=%d  Operation=%s \n",slot,aptr->name);
	  if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}

	  // debug - print line type sequence
	  /*
	  slc_test=mptr->slice_first[slot];
	  while(slc_test!=NULL)
	    {
	    printf(" slc=%X  ID=%d  z=%f  slot=%d \n",slc_test,slc_test->ID,slc_test->sz_level,slot);
	    lt_ptr=optr->lt_seq;			
	    while(lt_ptr!=NULL)
	      {
	      ptyp=lt_ptr->ID;
	      printf("   ptyp=%d  lt_ptr=%X  lt_ID=%d  lt_name=%s\n",ptyp,lt_ptr,lt_ptr->ID,lt_ptr->name);
	      printf("   slc->pqty[ptyp]=%d \n",slc_test->pqty[ptyp]);
	      printf("   slc->pfirst[ptyp]=%X \n",slc_test->pfirst[ptyp]);
	      if(slc_test->pfirst[ptyp]!=NULL)printf("   vert_qty=%d  vert_first=%X \n",(slc_test->pfirst[ptyp])->vert_qty,(slc_test->pfirst[ptyp])->vert_first);
	      lt_ptr=lt_ptr->next;
	      }
	    if(slc_test==mptr->slice_last[slot])break;
	    slc_test=slc_test->next;
	    }
	  while(!kbhit());
	  */
	  
	  // loop thru all line types in sequence as called out by this tool's operation (see ToolName.XML) to find our start point
	  pptr_start=NULL;			
	  lt_ptr=optr->lt_seq;						// get first line type called out for deposit by this operation's sequence
	  while(lt_ptr!=NULL)						// loop through all line types identified in the slice's deposition sequence
	    {
	    ptyp=lt_ptr->ID;
	    if(ptyp<0 || ptyp>MAX_LINE_TYPES){lt_ptr=lt_ptr->next;continue;}
	    if(sptr->pqty[ptyp]<1){lt_ptr=lt_ptr->next;continue;}	// must have at least one polygon
	    pptr_start=sptr->pfirst[ptyp];				// point to start of polygon list for this type in this slice
	    if(rerun_flag==TRUE && p_pick_list!=NULL)pptr_start=p_pick_list->p_item;	// if rerunning... set to first node of pick list
	    if(pptr_start!=NULL){if(pptr_start->vert_qty>0)break;}	// and if it has a vtx, then we found what we're looking for
	    lt_ptr=lt_ptr->next;
	    }
	  if(pptr_start==NULL){aptr=aptr->next;continue;}		// if nothing viable in slice, move onto next operation
	  
	  // ready tool for this slice using the first polygon and first line type
	  lptr=linetype_find(slot,ptyp);				// get pointer to line type
	  if(lptr!=NULL)						// if valid line type found...
	    {
	    if(Tool[slot].pwr48==TRUE)
	      {
	      if(strstr(Tool[slot].name,"ROUTER")!=NULL)tool_power_ramp(slot,TOOL_48V_PWM,(int)(100*lptr->flowrate)); // ramp up power
	      }
	    }
	  
	  // if more than one tool in use, we need to make the active one proud
	  if(slot!=prev_slot)tool_matl_forward(slot,Tool[slot].matl.advance);	// advance material for first use of this tool (slot)
	  if(first_layer_flag==TRUE || tool_use_count>1)			// prepare all tools when at first layer
	    {
	    tool_tip_deposit(slot);					// drop tool tip into position to deposit
	    make_toolchange(slot,1);					// switch tool to deposit mode
	    }
	  
	  // loop thru all copies of this model in x then in y before next operation change
	  printf("\nL_TYPE:  ptyp=%d  pptr_start=%X  vert_qty=%d \n\n",ptyp,pptr_start,pptr_start->vert_qty);
	  original_xoff=mptr->xorg[mtyp];
	  original_yoff=mptr->yorg[mtyp];
	  mptr->active_copy=0;						// init total copy counter
	  for(x_copy_ctr=0;x_copy_ctr<mptr->xcopies;x_copy_ctr++)	// loop thru all in X
	    {
	    if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	    if(local_job.state!=JOB_RUNNING)break;			// account for nested loop abort requests

	    mptr->xoff[mtyp]=original_xoff+x_copy_ctr*(mptr->xmax[mtyp]-mptr->xmin[mtyp]+mptr->xstp);	// calc X offset
	    for(y_copy_ctr=0;y_copy_ctr<mptr->ycopies;y_copy_ctr++)	// loop thru all in Y
	      {
	      if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
	      if(local_job.state!=JOB_RUNNING)break;			// account for nested loop abort requests

	      mptr->yoff[mtyp]=original_yoff+y_copy_ctr*(mptr->ymax[mtyp]-mptr->ymin[mtyp]+mptr->ystp);	// calc Y offset
	      if(mptr->active_copy>=mptr->total_copies)continue;	// if beyond total copies...
	      mptr->active_copy++;					// increment copy counter

	      // enter data to print log file
	      sprintf(prt_scratch,"    Copy=%d  \n",mptr->active_copy);
	      if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}

	      // if drilling holes with subtractive tool...
	      if(opr==OP_MILL_HOLES)
		{
		pptr=sptr->pfirst[MDL_PERIM];					// scan slice for any drilled hole
		while(pptr!=NULL)
		  {
		  if(pptr->hole>1)break;					// if one is found, break here... we gotta drill
		  pptr=pptr->next;
		  }
		if(pptr==NULL)drill_holes_done_flag=1;				// if none were found, skip hole drill loop
		if(Tool[slot].type==SUBTRACTIVE && !drill_holes_done_flag)
		  {
		  pre_drill_diam=pptr->diam;					// init to first hole size encountered
		  Tool[slot].tip_diam=pptr->diam;				// set require drill bit size
		  if(!ignore_tool_bit_change)
		    {
		    local_job.state=JOB_PAUSED_FOR_BIT_CHG;			// set job state to paused by system awaiting bit change
		    if(print_status(&local_job)==0)break;
		    }
		  else 
		    {
		    tool_power_ramp(slot,TOOL_48V_PWM,100);
		    }
		  tool_tip_deposit(slot);					// drop tool tip into position to deposit
		  make_toolchange(slot,1);
		  vptr=vertex_make();
		  pptr=sptr->pfirst[MDL_PERIM];					// polygons already sorted by hole size...
		  while(pptr!=NULL)						// ... should only have to run thru loop once
		    {
		    if(pptr->hole==2)						// if this is a round drill-able hole
		      {
		      if(fabs(pptr->diam-pre_drill_diam)>CLOSE_ENOUGH)		// if a new hole size is found...
			{
			Tool[slot].tip_diam=pptr->diam;
			if(!ignore_tool_bit_change)
			  {
			  local_job.state=JOB_PAUSED_FOR_BIT_CHG;		// set job state to paused by system awaiting bit change
			  if(print_status(&local_job)==0)break;
			  }
			}
		      vptr->x=pptr->centx;
		      vptr->y=pptr->centy;
		      vptr->z=(ZMin-ZMax-0.5);					// drill full depth hole plus enough to break thru
		      print_vertex(vptr,slot,4,NULL);				// move to hole and drill
		      tinyGSnd(gcode_burst);					// send string of commands to tinyG
		      memset(gcode_burst,0,sizeof(gcode_burst));		// null out burst string
		      motion_complete();					// wait for motion to complete
		      pptr->type=(-2);						// set to indicate it has been printede
		      vold=pptr->vert_first;	   				// mark all vtxs in this poly as having been printed
		      while(vold!=NULL)
			{
			vold=vold->next;
			if(vold==pptr->vert_first)break;
			}
		      pre_drill_diam=pptr->diam;
		      if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		      }
		    pptr=pptr->next;
		    }
		  free(vptr);
		  vertex_mem--;
		  tool_tip_retract(slot);
		  make_toolchange(slot,1);					// make tool change to matl drive at start of every layer
		  drill_holes_done_flag=1;					// all polys have already been drilled
		  local_job.state=JOB_PAUSED_FOR_BIT_CHG;			// pause to allow bit change
		  if(print_status(&local_job)==0)break;
		  }
		}
	
	      // position head over first vtx of first polygon in layer before dropping tool into position
	      pptr=pptr_start;							// reset back to first polygon
	      if(rerun_flag==TRUE && p_pick_list!=NULL)				// if reprinting specific polygons...
	        {
		pl_ptr=p_pick_list;						// ... get the first poly off pick list
		pptr=pl_ptr->p_item;						// ... define as the poly we'll work with
		if(pptr==NULL)break;						// ... just in case it's not valid
		sptr_check=polygon_get_slice(pptr);
		if(sptr_check!=NULL)sptr=sptr_check;
		}
	      vptr=pptr->vert_first;						// starting vertex
	      vtx_last_printed->x=vptr->x;					// forced new target for comefrom_machine_home
	      vtx_last_printed->y=vptr->y;
	      vtx_last_printed->z=vptr->z;
	      vtx_last_printed->attr=vptr->attr;
	      if(vptr!=NULL)							// if a viable vertex...
		{
		//vptr->attr=ptyp;
		if(mptr->mdl_has_target==TRUE)vptr->z=mptr->zmax[TARGET];	// set to top surface of target
		if(carriage_at_home==TRUE)					// if at home...
		  {
		  comefrom_machine_home(slot,vtx_last_printed);			// ... move onto part at vptr
		  on_part_flag=TRUE;						// ... set flag as such
		  }
		else 								// if not at home...
		  {
		  on_part_flag=TRUE;						// ... set flag as such
		  print_vertex(vptr,slot,2,NULL);				// ... pen-up to the start of the polygon
		  tinyGSnd(gcode_burst);					// ... send string of commands to fill buffer
		  memset(gcode_burst,0,sizeof(gcode_burst));			// ... null out burst string
		  }
		}
		
	      // draw polygons in line type sequence as defined by this operation in ToolName.XML using params in the MatlName.XLS.
	      // a typcial sequence for an operation might be:  OFFSET, BORDER, FILL, LOWER_CO, UPPER_CO where those line types
	      // are then defined in the MatlName.XML file.
	      vptr_p0=NULL; vptr_p1=NULL;					// init vtx ptrs for colinear check
	      lt_ptr=optr->lt_seq;						// define line type ID and name...note this points to a generic list of linetypes (the sequence)
	      while(lt_ptr!=NULL)						// loop through all line types identified in the model's deposition sequence
		{
		if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		if(local_job.state!=JOB_RUNNING)break;				// account for nested loop break request

		pre_matl[slot]=Tool[slot].matl.mat_used;			// save amt of mat used at start of this line type
		ptyp=lt_ptr->ID;						// get linetype ID
		//if(ptyp==SPT_BORDER || ptyp==SPT_OFFSET){lt_ptr=lt_ptr->next;continue;}	// skip support contours
		pptr=sptr->pfirst[ptyp];					// point to start of polygon list for this type in this slice
		if(rerun_flag==TRUE && p_pick_list!=NULL)			// if rerunning... start with first poly on pick list
		  {
		  pptr=p_pick_list->p_item;
		  if(pptr!=NULL)
		    {
		    sptr_check=polygon_get_slice(pptr);
		    if(sptr_check!=NULL)sptr=sptr_check;
		    }
		  }
		if(pptr==NULL){lt_ptr=lt_ptr->next;continue;}			// if not found, we must skip
		lptr=linetype_find(slot,ptyp);					// define actual pointer to the linetype intended for use
		if(lptr==NULL){lt_ptr=lt_ptr->next;continue;}			// if not found, we must skip

		// enter data to print log file
		//sprintf(prt_scratch,"    Line_type=%s  Polygon_qty=%d \n",lt_ptr->name,sptr->pqty[ptyp]);
		//if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}

		lt_start_time=time(NULL);					// get time at start of this line type
		linet_state[slot]=ptyp;						// provides global exposure for display/tracking
		cmdControl=TRUE;						// once control is on, do not reset count

		// prepare tool for deposition with this line type
		if(Tool[slot].tool_ID==TC_FDM)
		  {
		  depTemp=Tool[slot].thrm.operC+lptr->tempadj;			// run at requested line type temp
		  temperature_adjust(slot,depTemp,TRUE);			// bring htr up/down to temp per line type adjustment request
		  make_toolchange(slot,1);					// reset stepper drive to move material
		  if(tool_air_override_flag==FALSE)tool_air(slot,OFF);		// default to off if under line type control
		  if(lptr->air_cool==TRUE && z_cut>2.0)tool_air(slot,ON);	// turn on air if above 2mm and line type in material file says so
		  }
		
		// loop thru all polygons of this line type and draw lines between verticies
		// note that if this a rerun of specific polygons pptr is incremented by the polygon pick list
		adjfeed_flag=TRUE;						// turn on flag to adjust feed rate around tight corners
		adjflow_flag=TRUE;						// turn on flag to adjust flow rate around tight corners
		while(pptr!=NULL)						// while we still have polygons to process
		  {
		  if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		  if(local_job.state!=JOB_RUNNING)break;			// account for nested loop break request
		  
		  if(pptr->hole==2){pptr=pptr->next;continue;}			// if a drilled hole, then skip it - it was done above
		  if(ptyp==MDL_PERIM && pptr->perim_type==2)			// if custom settings for this poly need to be applied...
		    {if(pptr->perim_lt!=NULL)lptr=pptr->perim_lt;}		// ... reset line type pointer to one specific to this poly
		  if(ptyp==MDL_FILL && pptr->fill_type==2)			// if custom settings for this poly need to be applied...
		    {if(pptr->fill_lt!=NULL)lptr=pptr->fill_lt;}		// ... reset line type pointer to one specific to this poly
		  //if(first_layer_flag==TRUE && ptyp==MDL_LAYER_1 && pptr->vert_qty>3) 	// if first layer and a perimeter/border/offset poly...
		  //  {pptr=pptr->next; continue;}					// ... skip it as it causes elephant foot type defects
		  
		  cmdBurst=1;							// set the number of cmds to be sent to tinyG in one burst
		  if(cmdBurst>pptr->vert_qty)cmdBurst=pptr->vert_qty;		// if greater than what's in this poly, set to max in poly
		  if(Tool[slot].type==SUBTRACTIVE)cmdBurst=1;			// only send one move at a time to allow instant pause if routing
		  burst_ctr=0;							// init burst counter to count down bursted cmds
		  vector_count=0;						// init vector counter used to count up number of vtx sent to tinyG in this polygon
		  vtx_overlap=set_vtx_overlap;					// set the number of vtxs to overlap on borders/offsets to fix start/stop seam
		  if(pptr->vert_qty<3)vtx_overlap=0;				// if just a fill line, reset to 0
		  if(local_job.type==SUBTRACTIVE)vtx_overlap=0;
		  if(local_job.type==MARKING)vtx_overlap=0;
	    
		  // enter data to print log file
		  sprintf(prt_scratch,"      Printing polygon of %d vectors in line type %d. \n",pptr->vert_qty,ptyp);

		  //printf("\nPRINT: Printing polygon of %d vectors in line type %d. \n",pptr->vert_qty,ptyp);
		  //printf("  tool=%d  matl=%s  \n",slot,Tool[slot].matl.description);
		  //printf("  lptr=%X  flow=%f   feed=%f \n",lptr,lptr->flowrate,lptr->feedrate);
		  //while(!kbhit());
		  
		  //if(print_log!=NULL){fwrite(prt_scratch,1,strlen(prt_scratch),print_log); fflush(print_log);}
    
		  // pen up to first vtx of (next) poly and set tool up for action.  note this is broken
		  // down into job type then tool type as each tool generally has specific set up requirements.
		  vptr=pptr->vert_first;					// start at first vertex
		  //vptr->attr=ptyp;						// set the vtx attribute to the polygon type (aka line type)
		  penup_mode=0;							// penup with mat retract and tip lift at penup speed
		  if(vertex_colinear_test(vptr_p0,vptr_p1,vptr)==TRUE)penup_mode=6; // if 3 consequetive vtxs are in perfect alignment... force z move
		  
		  // execute pre polygon adjustments based on tool being used
		  if(Tool[slot].type==ADDITIVE)
		    {
		    if(Tool[slot].tool_ID==TC_FDM)
		      {
		      // note that material retract/advance from polygon to polygon is handled
		      // directly by the print_vertex function.  it is set by the type of pen-up request.
		      }
		    if(Tool[slot].tool_ID==TC_EXTRUDER)
		      {
		      }
		    print_vertex(vptr,slot,penup_mode,NULL);			// start with a pen-up to the start of the polygon
		    }
		  if(Tool[slot].type==SUBTRACTIVE)
		    {
		    print_vertex(vptr,slot,penup_mode,NULL);			// start with a pen-up to the start of the polygon
		    tinyGSnd(gcode_burst);					// send string of commands to fill buffer
		    memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
		    motion_complete();						// ensure next move is just in z
		    }
		  if(Tool[slot].type==MARKING)
		    {
		    print_vertex(vptr,slot,2,lptr);				// ... pen-up to start of polygon
		    tinyGSnd(gcode_burst);					// ... send string of commands to fill buffer
		    memset(gcode_burst,0,sizeof(gcode_burst));			// ... null out burst string
		    motion_complete();						// ... wait to get there before turning laser on
		    if(Tool[slot].tool_ID==TC_LASER)
		      {
		      if(strstr(Tool[slot].name,"LASER40W")!=NULL)tool_air(slot,ON); // ... blows smoke away
		      if(strstr(Tool[slot].name,"LASER80W")!=NULL)tool_air(slot,ON); // ... blows smoke away
		      if(strstr(Tool[slot].name,"LT4LDSV2")!=NULL)tool_air(slot,ON);
		      if(opr==OP_MARK_OUTLINE)Tool[slot].thrm.heat_duty=100*lptr->flowrate;
		      if(opr==OP_MARK_AREA)Tool[slot].thrm.heat_duty=100*lptr->flowrate;
		      if(opr==OP_MARK_IMAGE && mptr->grayscale_mode==1)Tool[slot].thrm.heat_duty=100*vptr->i;
		      if(opr==OP_MARK_IMAGE && mptr->grayscale_mode==2)Tool[slot].thrm.heat_duty=100*lptr->flowrate;
		      if(opr==OP_MARK_CUT)Tool[slot].thrm.heat_duty=100*lptr->flowrate;
		      while(Tool[slot].thrm.heat_status==OFF);			// wait for it to turn on
		      }
		    }
		  //tool_tip_touch(lptr->touchdepth);				// touch tip to surface if requested

		  // execute penup move to start of polygon
		  tinyGSnd(gcode_burst);					// send string of commands to tinyG
		  memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
		  delay(lptr->poly_start_delay);				// delay to allow pressure to build up in nozzle before moving

		  // loop thru all vtxs of this polygon
		  do{
		    if(job.sync==TRUE){if(print_status(&local_job)==0)break;}	// check for pause/abort/etc.
		    
		    vector_count++;						// increment vector count
		    vptr_p0=vptr_p1;						// save pre-prev vertex position
		    vptr_p1=vptr;						// save previous vertex position
		    vptr=vptr->next;						// move onto next vertex
		    if(vptr==NULL)break;					// handles "open" polygons like FILL
		    if(Tool[slot].tool_ID==TC_FDM)vptr->i=0;			// default to not an overlap vector
		    if(vtx_overlap>0 && Tool[slot].tool_ID==TC_FDM)
		      {
		      if(vector_count<vtx_overlap)vptr->i=(0.5);		// set flow volume for begining overlap vectors
		      if(vector_count>pptr->vert_qty)vptr->i=(0.5);		// set flow volume for ending overlap vectors
		      }
		    vptr->attr=ptyp;						// set attr to line type
		    print_vertex(vptr,slot,1,lptr);				// build PD move command for tinyG
		    burst_ctr++;						// increment the number of cmds added to command string
		    if(burst_ctr>=cmdBurst || vector_count>=(pptr->vert_qty+vtx_overlap)) // if enough cmds have been added ...
		      {
		      tinyGSnd(gcode_burst);					// ... send string of commands to fill buffer
		      memset(gcode_burst,0,sizeof(gcode_burst));		// ... null out burst string
		      burst_ctr=0;						// ... reset burst count down counter
		      cmdBurst=1;
		      }
		    }while(vector_count<(pptr->vert_qty+vtx_overlap));		// stop after drawing all vectors and then some to cover start/stop seam
		   
		  // execute post polygon adjustments based on tool type being used
		  if(Tool[slot].type==ADDITIVE)
		    {
		    if(Tool[slot].tool_ID==TC_FDM)
		      {
		      // extra retraction done at end of each polygon to help with start/stop seem.  pressure builds up in the nozzle as
		      // the perimeter is being deposited.  when the nozzle comes to a stop at the end of the polygon extra material oozes
		      // out due to the increased pressure.  this retraction sucks up that excess.
		      //if(ptyp==MDL_BORDER || ptyp==MDL_OFFSET)tool_matl_retract(slot,0.03);
		      }
		    if(Tool[slot].tool_ID==TC_EXTRUDER)
		      {
		      }
		    }
		  if(Tool[slot].type==SUBTRACTIVE)
		    {
		    }
		  if(Tool[slot].type==MARKING)
		    {
		    // turn off laser at the end of every poly as soon as possible
		    if(Tool[slot].tool_ID==TC_LASER)
		      {
		      motion_complete();					// finish any pending moves
		      Tool[slot].thrm.heat_duty=0.0;				// turn power off for pen up line to start of next raster
		      while(Tool[slot].thrm.heat_status==ON);			// wait for it to shut off
		      }
  
		    }
		  //tool_tip_touch(lptr->touchdepth);				// touch tip to surface if requested

		  // if not back to running, exit the polygon loop after making sure laser is off
		  // note there will be a series of cascading "breaks" to get out of all the printing control loops
		  if(local_job.state!=JOB_RUNNING)break;

		  // get next polygon to process
		  pptr=pptr->next;						// move on to next polygon
		  if(rerun_flag==TRUE && p_pick_list!=NULL)			// if rerunning specific polygons...
		    {
		    pl_ptr=pl_ptr->next; 					// ... get next pick list element
		    if(pl_ptr!=NULL)						// if there is a polygon on the list...
		      {
		      pptr=pl_ptr->p_item;					// ... set pptr to item found in list
		      sptr_check=polygon_get_slice(pptr);			// ... get pointer to slice containing this polygon
		      if(sptr_check!=NULL)last_sptr=sptr_check;			// ... if a valid slice...
		      }
		    else 							// ... if done with list...
		      {
		      pptr=NULL; 						// ... set next poly to NULL
		      p_pick_list=polygon_list_manager(p_pick_list,NULL,ACTION_CLEAR);	// ... clear the list
		      local_job.state=JOB_PAUSED_BY_CMD;
		      }
		    }
		  }		// end of polygon loop
		
		// if not back to running, exit the line type loop immediately
		if(local_job.state!=JOB_RUNNING)break;

		lt_finish_time=time(NULL);					// get time at end of this line type
		//motion_complete();

		// record material history
		// amount extruded = (vector length * flow rate) / 500		- accounted for in print vertex function
		// filament used = amount extruded * (fila diam / tip diam)	- accounted for below
		hfld=H_MOD_MAT_ACT;						// set default field to model mat'l
		if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)hfld=H_SUP_MAT_ACT;	// set field to support mat'l
		history[hfld][hist_ctr_run] += ((Tool[slot].matl.mat_used - pre_matl[slot]) * (Tool[slot].matl.diam / Tool[slot].tip_diam));

		// record time history
		hfld=H_OHD_TIM_ACT;						// set default field to system delay (i.e. unaccounted for)
		if(ptyp>=MDL_PERIM && ptyp<=PLATFORMLYR2)hfld=H_MOD_TIM_ACT;	// set field to model material sum
		if(ptyp>=SPT_PERIM && ptyp<=SPT_LAYER_1)hfld=H_SUP_TIM_ACT;	// set field to support material sum
		lt_time_delta=lt_finish_time-lt_start_time;
		if(lt_time_delta<0)lt_time_delta=0;
		history[hfld][hist_ctr_run] += lt_time_delta;			// accrue line type total and save history value
		hist_tool[hfld]=slot;

		lt_ptr=lt_ptr->next;						// move on to next line type in sequence
		}		// end of line type loop
	
	      // releive tool from deposition state
	      if(Tool[slot].tool_ID==TC_LASER)				 	// if using a laser tool...
		{
		motion_complete();						// ... finish pending moves
		Tool[slot].thrm.heat_duty=0.0;					// ... turn off power
		while(Tool[slot].thrm.heat_status==ON);
		}

	      // if not back to running, exit y axis copies loop
	      if(local_job.state!=JOB_RUNNING)break;
	      }		// end of y axis copies loop

	    // if not back to running, exit x axis copies loop
	    if(local_job.state!=JOB_RUNNING)break;
	    }		// end of x axis copies loop
	    
	  // if not back to running, exit if SPTR!=NULL statement
	  if(local_job.state!=JOB_RUNNING)break;

	  mptr->xoff[mtyp]=original_xoff;					// restore original offset values
	  mptr->yoff[mtyp]=original_yoff;
	  last_sptr=sptr;							// save pointer to last non-null slice
	  
	  // if flag to pause is set...
	  // this also allows user to re-run this slice, and/or pick specific polygons to re-reun
	  if(pause_at_end_of_layer==TRUE)
	    {
	    // clear polygon pick list at the end of each layer
	    if(p_pick_list!=NULL)p_pick_list=polygon_list_manager(p_pick_list,pptr,ACTION_CLEAR);	
	    
	    // go into pause where user can select any polygons to re-run
	    local_job.state=JOB_PAUSED_BY_CMD;
	    if(print_status(&local_job)==0)break;			// check for anything other than JOB_RUNNING
	    pause_at_end_of_layer=FALSE;
	    rerun_flag=FALSE;						// default to not re-run
	    if(p_pick_list!=NULL)rerun_flag=TRUE;			// if user selected polygons for reprint... then re-run
	    }
    
	  }  		// end of sptr!=NULL
	else 
	  {
	  printf("Print:  NULL slice pointer unexectedly encountered.  Attempting recovery. \n");
	  printf("        z_cut=%f  mptr->slice_thick=%f  mptr->current_z=%f \n",z_cut,mptr->slice_thick,mptr->current_z);
	  }
	  
	// if not back to running, exit model operation loop
	if(local_job.state!=JOB_RUNNING)break;

	// test if next tool is same as current tool on multi-tool machines
	prev_slot=slot;							// save id of tool that was just used
	if(aptr->next!=NULL)
	  {
	  next_slot=(aptr->next)->ID;					// get id of next tool
	  if(next_slot!=slot)						// if next tool is not the same as current tool...
	    {
	    tool_matl_retract(slot,Tool[slot].matl.retract);		// this balances the advance at the start of the operation
	    tool_tip_retract(slot);					// ... retract tool tip
	    make_toolchange(-1,0);					// ... turn off all tools
	    if(Tool[slot].pwr48==TRUE)					// ... turn off tool power if enabled
	      {
	      if(strstr(Tool[slot].name,"ROUTER")!=NULL)tool_power_ramp(slot,TOOL_48V_PWM,OFF);
	      }
	    depTemp=Tool[slot].thrm.operC;				// ... set base deposition temperature
	    temperature_adjust(slot,depTemp,FALSE);			// ... reset tool to oper temperature, no need to wait
	    }
	  }

	// finish with current tool at this height...
	Tool[slot].state=TL_READY;
	linet_state[slot]=UNDEFINED;
	oper_state[slot]=OP_NONE;
	
	printf("\n\nEnd of operation:  %s  with tool  %d\n\n",aptr->name,aptr->ID);
	//while(!kbhit());
	
	if(rerun_flag==FALSE)aptr=aptr->next;				// move on to next operation in sequence
	}	// end of operation loop - all operations required by this model
	
      // if not back to running, exit "while TRUE" loop as if job complete
      if(local_job.state!=JOB_RUNNING)break;

      // record overhead time history
      // this is the time OUTSIDE of the line type loop
      hfld=H_OHD_TIM_ACT;			
      oh_time_delta=oh_finish_time-oh_start_time;
      if(oh_time_delta<0)oh_time_delta=0;	
      history[hfld][hist_ctr_run] += oh_time_delta;			// add to what may have accrued during line type deposition
      oh_start_time=time(NULL);			

      // complete all pending moves before going any further
      motion_complete();

      // seek next layer to print
      // could be at same z with differnt model, or could be same model and new z with same tool
      old_z=z_cut;							// save current z position
      if(local_job.type==ADDITIVE)
        {
	// setting the model to the current z height indicates it has already been deposited
	// in other words, if the model z height equals the job z height, that model is caught up for ALL operations/tools
	mptr->current_z=z_cut;
	mptr=NULL;							// ensure no model active
	mptr=job_layer_seek(UP);					// find next model at next z height up
	}
      if(local_job.type==SUBTRACTIVE)
        {
	mptr->current_z=z_cut;
	mptr=NULL;							// ensure no model active
	mptr=job_layer_seek(UP);					// find next model at next z height up
	if(mptr!=NULL)mptr->zoff[MODEL] -= (2*mptr->slice_thick);	// sink this slice into the base material
	}
      if(local_job.type==MARKING)
        {
	// when incrementing layers for marking we want to move the model and not the carriage since
	// the carriage is marking on the top surface of a TARGET.  what changes is the slice of model
	// we are marking on that surface as the model "sinks" into it.
	mptr->current_z=mptr->zmax[TARGET]-mptr->zoff[MODEL]+mptr->slice_thick;	// increment z of model that was just printed

	printf("\n\nModel Status at EOL:\n");
	mchk=local_job.model_first;
	while(mchk!=NULL)
	  {
	  printf("  mptr=%X  mptr->z=%f  zoff=%f \n",mchk,mchk->current_z,mchk->zoff[MODEL]);
	  mchk=mchk->next;
	  }
	printf("\nLast model/slice:  zcut=%f  mptr=%X  mptr->z=%f  zoff=%f \n",z_cut,mptr,mptr->current_z,mptr->zoff[MODEL]);

	mptr=NULL;
	mptr=job_layer_seek(UP);					// get next model and/or layer

	if(mptr==NULL){printf("  NULL returned from job_layer_seek! - reset to first.\n");mptr=local_job.model_first;}

	old_z=z_cut;							// we want to force old_z and z_cut to be different
	z_cut=mptr->zmax[TARGET];					// keep z on top surface of target
	if(slc_print_view==TRUE){mptr->zoff[MODEL] -= slc_view_inc;}	// lower model into block by slice increment
	else {mptr->zoff[MODEL] -= mptr->slice_thick;}			// lower model into block by one slice

	printf("Next model/slice:  zcut=%f  mptr=%X  mptr->z=%f  zoff=%f \n\n",	z_cut,mptr,mptr->current_z,mptr->zoff[MODEL]);
	}
	
      local_job.current_z=z_cut;					// new z_cut set by job_layer_seek function
      if(mptr==NULL)
        {
	if(local_job.current_z<(ZMax-local_job.min_slice_thk))
	  {
	  printf("\nJob_layer_seek did not return a model pointer. \n\n"); 
	  local_job.state=JOB_PAUSED_DUE_TO_ERROR;
	  if(print_status(&local_job)==0)break;				// check for pause/abort/etc.
	  if(local_job.state!=JOB_RUNNING)break;
	  }
	}
      first_model_flag=FALSE;

      // if we moved onto a new layer...
      if(old_z!=z_cut)
        {
	// if just finishing the first layer, adjust height difference
	if(first_layer_flag==TRUE)
	  {
	  lptr=linetype_find(slot,MDL_LAYER_1);
	  if(lptr!=NULL)zoffset+=(lptr->thickness-mptr->slice_thick);
	  } 
	  
	// now that all operations on first layer for this model are complete, turn flag off.
	// this is tricky because this could still be the first layer of the next model being deposited.
	first_layer_flag=FALSE;							// default to off
	if(mptr!=NULL)								// if there is another model to go to...
	  {
	  if(fabs(z_cut-mptr->zoff[MODEL])<(1.5*mptr->slice_thick))first_layer_flag=TRUE;	// ... and if layer 1 of next model, set back to true
	  }
	
	// calc layer time
	layer_finish_time=time(NULL);					// get finish time of this layer
	layer_time=(int)(layer_finish_time-layer_start_time);		// calculate this layer's time
	layer_start_time=time(NULL);					// get time at start of the next layer
	PostGVCnt=1; PostGFCnt=1;					// reset velocity and flow rate counters for performance view
	PostGVAvg=0; PostGFAvg=0;
	
	// track layer time history
	if(hist_ctr_run<(MAX_HIST_COUNT-1))hist_ctr_run++;		// don't overrun array space
	
	// add entry to job log as such...
	if(save_logfile_flag==TRUE)
	  {
	  sprintf(prt_scratch,"\n; End of Layer Status ------------------------------------------------\n");
	  if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	  if(gcode_gen!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_gen);
	  curtime=time(NULL);
	  loc_time=localtime(&curtime);
	  strftime(prt_scratch,80,"; %x %X\n",loc_time);
	  if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	  sprintf(prt_scratch,";   Layer Time = %6d   Layer Height = %6.3f \n",layer_time,z_cut);
	  if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	  sprintf(prt_scratch,";   Ambient Temp = %6.2f   Build Table Temp = %6.2f \n",Tool[CHAMBER].thrm.tempC,Tool[BLD_TBL1].thrm.tempC);
	  if(gcode_out!=NULL)fwrite(prt_scratch,1,strlen(prt_scratch),gcode_out);
	  }

	// record image history if enabled
	if(save_image_history==TRUE)
	  {
	  // move print head out of the way for clear picture of model
	  motion_complete();						// let tinyG finish what it's doing
	  vtx_aside=vertex_make();
	  vtx_aside->x=(-25);
	  vtx_aside->y=(-25);
	  vtx_aside->z=z_cut;
	  vtx_aside->attr=MDL_BORDER;
	  memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
	  print_vertex(vtx_aside,slot,2,NULL);				// build command
	  tinyGSnd(gcode_burst);					// send string of commands to fill buffer
	  memset(gcode_burst,0,sizeof(gcode_burst));			// null out burst string
	  free(vtx_aside); vertex_mem--; vtx_aside=NULL;
	  
	  // record image and save
	  strcpy(bashfn,"#!/bin/bash\nsudo libcamera-jpeg -v 0 -n 1 -t 10 ");	// define base camera command
	  sprintf(img_name,"-o /home/aa/Documents/4X3D/ImageHistory/z_%06d.jpg ",(int)(z_cut*100));// ... build image name from z level
	  strcat(bashfn,img_name);						// ... add the target file name
	  strcat(bashfn,"--width 830 --height 574 ");				// ... add the image size
	  sprintf(prt_scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
	  strcat(bashfn,prt_scratch);						// ... add region of interest (zoom)
	  strcat(bashfn,"--autofocus-mode continuous ");			// ... add focus control
	  system(bashfn);							// issue the command to take a picture
	  
	  printf("\nImage %s saved.\n\n",img_name);
	  // print head move back automatically happens on pen-up to first polygon of next slice
	  }

	// wait for minimum layer time clock to exprire regardless if tool air is on or off
	// this action is driven by min_layer_time value in material.xls files
	if((Tool[slot].matl.min_layer_time-layer_time)>2.0) 			// ... if layer time less than min allowable time...
	  {step_aside(slot,(Tool[slot].matl.min_layer_time-layer_time));}	// ... move tool off model for difference
	  
	}
	
      // update public job information
      job.state=local_job.state;
      job.current_z=local_job.current_z;
      job.current_dist=local_job.current_dist;
      job.total_dist=local_job.total_dist;
      job.penup_dist=local_job.penup_dist;
      job.pendown_dist=local_job.pendown_dist;

      // determine if the job is done by comparing new z height to the job max/min z height
      //if(local_job.current_z<=ZMin || local_job.current_z>=ZMax)break;		// <- need to acct for target in ZMin/ZMax for this to work
      if(local_job.type==ADDITIVE && local_job.current_z>=ZMax){local_job.state=JOB_COMPLETE; break;}
      if(local_job.type==SUBTRACTIVE && local_job.current_z<=ZMin){local_job.state=JOB_COMPLETE; break;}
      if(local_job.type==MARKING && local_job.current_z>=ZMax){local_job.state=JOB_COMPLETE; break;}
      }	  // end of while TRUE loop that controls printer actions
      
    // at this point the job is either completed or aborted.
    // clean up end-of-job params and set status to not run in this thread.
    motion_complete();							// make sure last move is finished
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(Tool[slot].state<TL_LOADED)continue;
      tool_power_ramp(slot,TOOL_48V_PWM,OFF);				// turn tool pwr off
      if(Tool[slot].tool_ID==TC_LASER)Tool[slot].thrm.heat_duty=0.0;	// turn tool pwm off
      tool_matl_retract(slot,Tool[slot].matl.retract);			// retract material
      tool_air(slot,OFF);						// turn tool air pump off
      }
    on_part_flag=FALSE;							// set to indicate leaving part
    tool_air_override_flag=FALSE;					// reset to default state
    goto_machine_home();						// move carriage home
    cmdControl=0;  
    set_auto_zoom=old_auto_zoom;
    //save_unit_state();
    if(save_logfile_flag==TRUE)close_job_log();				// if saving logfile, close it here
    fclose(print_log);
    job.state=local_job.state;
    job.current_z=local_job.current_z;
    job.current_dist=local_job.current_dist;
    job.total_dist=local_job.total_dist;
    job.penup_dist=local_job.penup_dist;
    job.pendown_dist=local_job.pendown_dist;
    printf("Printing complete.  Now idle.\n\n");
    }	// end of WHILE loop
    
}

