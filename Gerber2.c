#include "Global.h"

// Function to read GERBER formatted files and stuff them into the model datastructure.
// Operation of these functions are based on the Ucamco rev 2018-11 specification with some allowance
// for depreciated commands.  Unfortunately, full backward compatibility is impractical.
//
// The basic concept is to load each GBR file into a slice where traces get converted to polygons and
// that slice then defines a board layer (i.e. bottom silk, bottom trace, top trace, top silk, etc.).

// This function allocates space for a single link list element of type gbr_aperture
gbr_aperature *gerber_ap_make(slice *sptr)
  {
  gbr_aperature 	*aptr;

  aptr=(gbr_aperature *)malloc(sizeof(gbr_aperature));	
  if(aptr==NULL)return(NULL);				
  aptr->ID=0;
  aptr->type=0;
  aptr->diam=0;
  aptr->X=0;
  aptr->Y=0;
  aptr->hole=0;
  memset(aptr->cmt,0,sizeof(aptr->cmt));
  aptr->next=NULL;
  return(aptr);
  }

// Function to insert new gerber aperature into the end of its linked list
int gerber_ap_insert(slice *sptr, gbr_aperature *ap_new)
  {
  gbr_aperature 	*aptr;
      
  if(sptr->gap_first==NULL)						// special case for first aperature
    {
    sptr->gap_first=ap_new;	
    }
  else 									// otherwise add to end of list
    {
    aptr=sptr->gap_first;
    while(aptr->next!=NULL)aptr=aptr->next;
    aptr->next=ap_new;
    ap_new->next=NULL;
    }
    
  printf("GBR: Aperature added = %X  ID=%d  diam=%f\n",ap_new,ap_new->ID,ap_new->diam);
  return(1);
  }

// Function to delete the entire gerber aperature linked list
int gerber_ap_purge(slice *sptr)
{
  gbr_aperature *aptr,*dptr;

  aptr=sptr->gap_first;
  while(aptr!=NULL)	
    {
    dptr=aptr;
    aptr=aptr->next;
    free(dptr);
    }
  sptr->gap_first=NULL;
  return(1);		
}

// Function to dump the aperature linked list to the consol for debug
int gerber_ap_dump(slice *sptr)
{
  gbr_aperature	*aptr;
  
  printf("\nGerber aperature dump:  Units=%d\n",gerber_units);
  aptr=sptr->gap_first;
  while(aptr!=NULL)	
    {
    printf("   ID........ = %d \n",aptr->ID);
    printf("     Type.... = %d \n",aptr->type);
    printf("     Diameter = %f \n",aptr->diam);
    printf("     X....... = %f \n",aptr->X);
    printf("     Y....... = %f \n",aptr->Y);
    printf("     Hole.... = %f \n",aptr->hole);
    printf("     Comment. = %s \n",aptr->cmt);
    printf("     Address. = %X \n",aptr);
    printf("     Next.... = %X \n",aptr->next);
    aptr=aptr->next;
    }
  
  return(1);
}

// This function reads thru one (or several) gerber files and convertes them into
// slices where the aperature strokes become polygons within the slice.  One slice
// is used per gerber file (i.e. top trace, btm trace, drill, top silk, btm silk, etc.)
//
// The basic idea is that the file contains a bunch of set-up  type params as defined by a leading "%" in the command.
// These values get loaded into dynamically referenced data structures and then get called by commands further down in the file.
// All other commands in the file set the state.  They are sequence dependent.
int gerber_model_load(model * mptr)
{
  int		h,i,slot;
  
  float 	pre_X,pre_Y,pre_I,pre_J;				// previous X Y I and J positions
  float 	crt_X,crt_Y,crt_I,crt_J;				// current X Y I and J positions
  float 	nxt_X,nxt_Y,nxt_I,nxt_J;				// next X Y I and J positions
  float 	arc_st,arc_end,arc_inc,arc_rad;				// circular interp variables
  float 	arc_cen_x,arc_cen_y,circle_points=20;			// circular interp variables
  float 	del_x,del_y,theta;					// circular interp variables
  int		multi_cmd=0;						// flag to indicate if multiple commands in one line
  int		deprec_cmd=0;						// flag to indicate if depreciated command encountered
  char 		gbr_text[255];						// inbound line off gerber file
  char		comment[255];						// temporary string to hold comments
  char 		ach,astr[12];						// temporary reusable string space
  char		nv_str[12];
  char 		*nv_ptr;
  char		*cptr;							// temporary pointer to place in string
  gbr_aperature	*crt_ap;						// pointer to current aperature
  gbr_aperature	*aptr;							// scratch pointer
  vertex 	*vptr,*vold;
  vertex	*trace_vtx;
  polygon	*pptr;
  slice		*sptr;
  genericlist	*glptr;
    
  printf("\nEntering GERBER MODEL LOAD\n\n");  
    
  if(mptr==NULL)return(0);  
    
  // open file and check for version identifier... if not found, check if compressed  
  model_in=fopen(mptr->model_file,"r");					// attempt file open
  if(model_in==NULL)							// if unable to open then ...
    {
    printf("ERROR: Unable to open model file.\n");
    return(0);
    }

  // find tool requested to build this file
  slot=(-1);
  glptr=mptr->oper_list;
  while(glptr!=NULL)
    {
    if(strlen(glptr->name)>1){slot=glptr->ID;break;}
    glptr=glptr->next;
    }
  slot=0;
  printf("\nGBR: mptr=%X  slot=%d  name=%s \n",mptr,slot,mptr->model_file);  
  if(slot<0 || slot>=MAX_TOOLS){fclose(model_in);return(0);}		// if tool location not reasonable, leave

  sptr=slice_make();							// create a new slice to load this file into
  sptr->sz_level=0.25;							// artificially set z level of this slice
  slice_insert(mptr,slot,mptr->slice_first[slot],sptr);			// add it to the linked list of slices at end of list
  printf("GBR: mptr=%X  slot=%d  sptr=%X  slice_first=%X\n",mptr,slot,sptr,mptr->slice_first[slot]);
  
  // init params
  pre_X=0.0;pre_Y=0.0;pre_I=0.0;pre_J=0.0;
  crt_X=0.0;crt_Y=0.0;crt_I=0.0;crt_J=0.0;
  nxt_X=0.0;nxt_Y=0.0;nxt_I=0.0;nxt_J=0.0;
  vold=vertex_make();
  crt_ap=NULL;
  trace_vtx=NULL;
  gerber_interp_mode=1;							// default to linear
  multi_cmd=0;								// default to NOT multi to get first line off file
  
  // loop thru entire file building up geometry as we go
  while(1)								// grab line off file
    {
    // When searching for commands, note that some can be merged into one line.  They
    // will be delimited by an "*".  If only one command, the "*" will be followed by "%", LF, or NULL.

    if(!multi_cmd)							// if not multi command...
      {
      if(fgets(gbr_text,sizeof(gbr_text),model_in)==NULL)break;		// get line off file
      strcpy(scratch,gbr_text);						// make a working copy so original stays in tact

      // check if in bound line off gerber file is multi or single commanded
      cptr=strstr(gbr_text,"*");					// find delimeter
      if(*(cptr+1)>42)							// if char after * is anything that can start a command...
	{
	multi_cmd=1;							// set flag that there is more to parse
	printf("GBR: Multi-command detected\n");
	}
      else 
	{
	multi_cmd=0;							// set flag that no more to parse
	}
      }
    if(multi_cmd)							// if this is a multi command line...
      {
      // copy the first command into single scratch string
      i=0;h=0;
      memset(scratch,0,sizeof(scratch));
      while(gbr_text[i]!=42)						// copy until * including starting %
        {
	scratch[h]=gbr_text[i];
	i++;
	h++;
	}
	
      // reduce inbound gerber line by the scratch string we just took off it
      // but leave it with a starting "%" so it still looks like a viable command string.
      h=1;								// skip over starting "%"
      i++;								// skip over delimiting "*"
      while(i<strlen(gbr_text))
        {
	gbr_text[h]=gbr_text[i];					// shift left
	i++;
	h++;
	}
      gbr_text[h]=0;							// define new end of string
      
      //printf("GBR: Multi-command\n");
      //printf("GBR:    File=%s \n",gbr_text);
      //printf("GBR:    Cmd =%s \n",scratch);
      //while(!kbhit());
      
      // and check to see if enough was extracted to be a command
      // if not, turn off multi cmd parsing and get fresh line off file
      if(strlen(scratch)<3)
        {
	multi_cmd=0;							
	continue;
	}
      }
      
      
    // display command under inspection
    printf("\nGBR: %s",scratch);
    printf("      <%f><%f>\n",crt_X,crt_Y);
    //if(crt_ap!=NULL)printf("GBR:  Crt_X=%f   Crt_Y=%f  Crt_ap=%d \n",crt_X,crt_Y,crt_ap->ID);
    //while(!kbhit());
      
      
    // check for coord format specifier
    if(strstr(scratch,"%FSLA")!=NULL)
      {
      nv_ptr=strstr(scratch,"X");
      if(nv_ptr!=NULL)
        {
	nv_str[0]=*(nv_ptr+1);						// capture first char past X
	nv_str[1]=0;
	gerber_format_xi=atoi(nv_str);					// defines the number of chars before decimal
        nv_str[0]=*(nv_ptr+2);						// capture second char past X
	nv_str[1]=0;
	gerber_format_xd=atoi(nv_str);					// defines the number of chars after decimal
	}
      nv_ptr=strstr(scratch,"Y");
      if(nv_ptr!=NULL)
        {
	nv_str[0]=*(nv_ptr+1);						// capture first char past Y
	nv_str[1]=0;
	gerber_format_yi=atoi(nv_str);					// defines the number of chars before decimal
        nv_str[0]=*(nv_ptr+2);						// capture second char past Y
	nv_str[1]=0;
	gerber_format_yd=atoi(nv_str);					// defines the number of chars after decimal
	}
      printf("GBR:  Integers=%d Decimals=%d \n",gerber_format_xi,gerber_format_xd);
      }
      
    // check for unit type
    if(strstr(scratch,"%MO")!=NULL)
      {
      gerber_units=1;							// default to inches
      if(strstr(scratch,"MOMM"))gerber_units=2;				// set to millimeters
      if(gerber_units==1)printf("GBR: Units set to INCHES\n");
      if(gerber_units==2)printf("GBR: Units set to MILLIMETERS\n");
      }
      
    // check for file attribute
    // generally just human readable commentary
    cptr=strstr(scratch,"%TF");
    if(cptr!=NULL)
      {
      strcpy(comment,(cptr+4));						// copy from start of comment
      comment[strlen(comment)-2]=0;					// clip off "*%" chars from comment
      if((254-strlen(mptr->cmt))>strlen(comment))			// if enough room left in model comment...
        {
	strcat(mptr->cmt," ");						// ... add a delimeter
	strcat(mptr->cmt,comment);					// ... and tack on the next comment
	}
      }
      
    // check for aperature attribute
    // generally just human readable commentary
    cptr=strstr(scratch,"%TA");
    if(cptr!=NULL)
      {
      strcpy(comment,(cptr+4));						// copy from start of comment
      comment[strlen(comment)-2]=0;					// clip off "*%" chars from comment
      }
      
    // check for polarity setting
    cptr=strstr(scratch,"%LP");
    if(cptr!=NULL)
      {
      gerber_polarity=0;						// default to clear
      if(strstr(scratch,"LPD"))gerber_polarity=1;			// set to dark
      }
      
    // check for aperture definition
    // this will provide line width, thus offset values to create polygons from single lines
    // typical format:  %ADD10C,0.5X0.25*%
    if(strstr(scratch,"%AD")!=NULL)
      {
      aptr=gerber_ap_make(sptr);					// create a blank aperature element
      strcpy(aptr->cmt,comment);					// define the comment
      gerber_aperature_parse(sptr,scratch,aptr);			// parse and define remaining elements
      gerber_ap_insert(sptr,aptr);					// insert the new aperature in the linked list for this slice
      }

    // check for end of file gerber statement
    if(strstr(scratch,"M02")!=NULL)
      {
      printf("GBR: End-of-file\n");
      //if(trace_vtx!=NULL)printf("GBR:    Trace_vtx=%X  Next=%X \n",trace_vtx,trace_vtx->next);
      
      // check if any traces are pending to be converted to polygons, then exit this function
      if(trace_vtx!=NULL)						// if a previous string of verticies exists...
	{
	if(trace_vtx->next!=NULL)					// if that string is longer than one vertex...
	  {
	  if(crt_ap!=NULL){gerber_trace_polygon(sptr,crt_ap,trace_vtx);}  // convert existing string of verticies to polygon
	  else {printf("\n WARNING: Attempt to trace polygon without appature defined!\n");}
	  }
	}
      }

    // check for misc unsupported depreciated commands
    if(strstr(scratch,"%IR"))deprec_cmd=1;
    if(strstr(scratch,"%IP"))deprec_cmd=1;
    if(strstr(scratch,"%OF"))deprec_cmd=1;
    if(strstr(scratch,"%MI"))deprec_cmd=1;
    if(strstr(scratch,"%SF"))deprec_cmd=1;
    if(strstr(scratch,"%AS"))deprec_cmd=1;

    // check for mode commands, basically anything starting with "G".
    // this gets ugly due to depreciation support.
    if(scratch[0]==71)							// if first char is "G"
      {
      // figure out the command number
      // note it can be up to 10 chars and padded with leading zeros, but may have further commands after the number
      i=1;h=0;
      memset(astr,0,sizeof(astr));
      ach=scratch[i];
      while(ach>44 && ach<58 && i<strlen(scratch))			// skip "G" and collect up to next non-numeric digit
        {
	astr[h]=ach;
	h++;
	i++;
	ach=scratch[i];
	}
      
      // now act on the command
      if(atoi(astr)==01)						// if linear interpolation requested...
        {
	// basically strip off G01 portion of string and then just process as a regular command.
	// note that is dependent upon happening before looking for an X,Y, or D command later in this loop.
	memset(scratch,0,255);
	h=0;i=3;
	while(i<strlen(gbr_text))
	  {
	  if(gbr_text[i]>42 && gbr_text[i]<123)
	    {
	    scratch[h]=gbr_text[i];					// shift left by 3 chars
	    h++;
	    }
	  i++;
	  }
	scratch[h]=0;
	gerber_interp_mode=1;						// set flag to linear interpolation
	printf("GRB: Linear interpolation ON\n");
	printf("GRB:   Remaining cmd: <%s> \n",scratch);
	}
      if(atoi(astr)==02)						// if CW circular interpolation requested...
        {
	// basically strip off G02 portion of string and then just process as a regular command.
	// note that is dependent upon happening before looking for an X,Y, or D command later in this loop.
	memset(scratch,0,255);
	h=0;i=3;
	while(i<strlen(gbr_text))
	  {
	  scratch[h]=gbr_text[i];					// shift left by 3 chars
	  i++;
	  h++;
	  }
	gerber_interp_mode=2;						// set flag to CW interpolation
	printf("GRB: Circular CW interpolation ON\n");
	}
      if(atoi(astr)==03)						// if CCW circular interpolation requested...
        {
	// basically strip off G03 portion of string and then just process as a regular command.
	// note that is dependent upon happening before looking for an X,Y, or D command later in this loop.
	memset(scratch,0,255);
	h=0;i=3;
	while(i<strlen(gbr_text))
	  {
	  scratch[h]=gbr_text[i];					// shift left by 3 chars
	  i++;
	  h++;
	  }
	gerber_interp_mode=3;						// set flag to CCW interpolation
	printf("GRB: Circular CCW interpolation ON\n");
	}
      if(atoi(astr)==04)						// if comment...
        {
	sprintf(comment,"comment");
	}
      if(atoi(astr)==36)						// if start of region requested...
        {
	gerber_region_flag=1;						// turn on region flag
	sprintf(scratch,"D02*");					// force current trace to close and open a new one for this region
	printf("GRB: Region ON\n");
	}
      if(atoi(astr)==37)						// if end of region requested...
        {
	gerber_region_flag=0;						// turn off region flag
	sprintf(scratch,"D02*");					// force current trace to close for this region and open a new one for what's next
	printf("GRB: Region OFF\n");
	}
      if(atoi(astr)==54)						// depreceated start of aperature select command
        {
	// basically strip off G54 portion of string and then just process as "D" command.
	// note that is dependent upon happening before looking for a "D" command later in this loop.
	memset(scratch,0,255);
	h=0;i=3;
	while(i<strlen(gbr_text))
	  {
	  scratch[h]=gbr_text[i];					// shift left by 3 chars
	  i++;
	  h++;
	  }
	printf("GBR: G54 Parse - remaining command = %s \n",scratch);
	}
      if(atoi(astr)==55)						// depreceated prepare to flash command
        {
	// basically strip off G55 portion of string and then just process as "D" command.
	// note that is dependent upon happening before looking for a "D" command later in this loop.
	memset(scratch,0,255);
	h=0;i=3;
	while(i<strlen(gbr_text))
	  {
	  scratch[h]=gbr_text[i];					// shift left by 3 chars
	  i++;
	  h++;
	  }
	printf("GBR: G55 Parse - remaining command = %s \n",scratch);
	}
      if(atoi(astr)==70)						// depreciated method of setting units to inch
        {
	gerber_units=1;							// set to inches
	}
      if(atoi(astr)==71)						// depreciated method of setting units to mm
        {
	gerber_units=2;							// set to mm
	}
      if(atoi(astr)==74)						// if single quadrant requested...
        {
	gerber_quad_mode=1;						// set to single quad
	printf("GBR: Single Quad Interpolation ON\n");
	}
      if(atoi(astr)==75)						// if multi quadrant requested...
        {
	gerber_quad_mode=2;						// set to multi quad
	printf("GBR: Multi Quad Interpolation ON\n");
	}
      }

    // check for stand alone D command which may be a move/flash or maybe an aperature set request.
    // check for aperature set command of typical format "D11*", where "11" = the ID of the aperature requested.
    // this will set a pointer crt_ap to be applied to any subsequent stroke and/or flash commands
    if(scratch[0]==68)							// if first char is "D"
      {
      // scan for number
      h=0;i=1;
      memset(astr,0,10);
      while(i<(strlen(scratch)-1))					// extract ID
        {
        astr[h]=scratch[i];
        h++;
        i++;
        }
	
      // if D<10 then it is a move or flash command.
      // this is usually in the depreciated form of "G55D03*" where code above has stripped out the "G" portion
      if(atoi(astr)==3)	
        {
	gerber_flash_polygon(sptr,crt_ap,crt_X,crt_Y);
	}
	
      // if D>=10 then it is an aperature ID request
      if(atoi(astr)>=10)
        {
	printf("GBR: Aperature select = %d \n",atoi(astr));
	//gerber_ap_dump(sptr);
	crt_ap=sptr->gap_first;						// find ID in linked list of aperatures
	while(crt_ap!=NULL)
	  {
	  if(crt_ap->ID==atoi(astr))break;				// if a match is found... leave pointer at it
	  crt_ap=crt_ap->next;
	  }
	printf("GBR: Ap=%X ID=%d Ap->next=%X\n",crt_ap,atoi(astr),crt_ap->next);
	}
      }

    // check for stroke, move, and/or flash command
    // These will start with a coord call out followed by a D01 (PenUp), D02 (Pen Dn), or D03 (Flash) command.
    // A D02 will initiate a new empty string of verticies starting with trace_vtx.
    // A series of D01 strokes are collected into that string of verticies that was initiated by the previous D02.
    // Subsequent D02 cmds will terminate the existing trace vertex string, convert it to a polygon, and add
    // it to the slice, then create another new empty trace_vtx.
    // A D03 command simply creates a polygon as specified by the current aperature and adds it to the slice.
    if(scratch[0]==88 || scratch[0]==89)				// if first char is "X" or "Y"
      {
      vptr=vertex_make();						// create a new empty vertex
      vptr->x=vold->x;							// assign current values as default
      vptr->y=vold->y;
      vptr->i=vold->i;
      vptr->j=vold->j;
      if(crt_ap!=NULL)vptr->attr=crt_ap->ID;
      i=gerber_XYD_parse(scratch,vptr);					// load coord values into vptr values
      if((i >> 0) & 1)nxt_X=vptr->x;					// check which values actually got read
      if((i >> 1) & 1)nxt_Y=vptr->y;					// and update them accordingly
      if((i >> 2) & 1)nxt_I=vptr->i;					// note: if a value is NOT updated, then it
      if((i >> 3) & 1)nxt_J=vptr->j;					// will retain the previous "current" value
	
      // vptr->attr holds the "D" command for each coord change:
      // D02 = PenUP   D01 = PenDown  D03 = Flash
	
      // if a PenDown (D01) type movement...
      if(vptr->attr==1)
        {
	// cover case when sequence is off and this happens before anything else
	if(trace_vtx==NULL)
	  {
	  printf("GBR:  ERROR - encountered D01 stroke before setting position D02\n");
	  trace_vtx=vertex_make();
	  trace_vtx->x=crt_X;
	  trace_vtx->y=crt_Y;
	  vold=trace_vtx;
	  }
	// otherwise we are adding onto a trace/region that has already been defined
	else 
	  {
	  if(gerber_interp_mode==1)					// if linear interpolation... connect the previous vertex to the current vertex
	    {
	    printf("GBR: PenDn From: <%f><%f>   To: <%f><%f>\n",vold->x,vold->y,vptr->x,vptr->y);
	    printf("GBR: PenDn From: <%f><%f>   To: <%f><%f>\n",crt_X,crt_Y,nxt_X,nxt_Y);
	    vold->next=vptr;						// link the previous vertex to the current one		
	    vold=vptr;							// redefine the current vertex as the previous one
	    }
	  if(gerber_interp_mode==2 || gerber_interp_mode==3)		// if circular interpolation...
	    {
	    if(gerber_quad_mode==1)					// if single quadrant mode...
	      {
	      arc_cen_x=crt_X-nxt_I;					// center point of arc - in single quad mode it's a subtraction
	      if(crt_X<nxt_X)arc_cen_x=crt_X+nxt_I;
	      arc_cen_y=crt_Y-nxt_J;
	      if(crt_Y<nxt_Y)arc_cen_y=crt_Y+nxt_J;
	      arc_rad=sqrt(nxt_I*nxt_I + nxt_J*nxt_J);			// radius of arc

	      printf("GBR: Single Quadrant Interpolation\n");
	      printf("GBR:   X0=%f Y0=%f X1=%f Y1=%f I=%f J=%f\n",crt_X,crt_Y,nxt_X,nxt_Y,nxt_I,nxt_J);
	      printf("GBR:   Xc=%f Yc=%f R=%f \n",arc_cen_x,arc_cen_y,arc_rad);
	      
	      // we now know the start pt, end pt, center, and radius
	      // cacluculate angle to start point of arc from the X axis rounded to the precision of the incoming file
	      del_x=(float)round(powf(10,gerber_format_xd)*(crt_X-arc_cen_x))/powf(10,gerber_format_xd);
	      del_y=(float)round(powf(10,gerber_format_yd)*(crt_Y-arc_cen_y))/powf(10,gerber_format_yd);
	      theta=PI/2;						// default to no X, then vertical and angle=PI/2
	      if(fabs(del_x)>TOLERANCE)theta=atan(del_y/del_x);		// if delta X, then calc angle
	      arc_st=theta;
	      if(del_x<0 && del_y>=0)arc_st=PI+theta;			// 2nd quadrant, theta is negative, add to PI
	      if(del_x<=0 && del_y<0)arc_st=PI+theta;			// 3rd quadrant, theta is positive, add to PI
	      if(del_x>0 && del_y<0)arc_st=2*PI+theta;			// 4th quadrant, theta is negative, add to 2*PI
	      printf("GBR:   del_x=%f  del_y=%f  Theta=%f ArcSt=%f \n",del_x,del_y,theta*180/PI,arc_st*180/PI);
	      
	      // calculate angle to end point of arc
	      del_x=(float)round(powf(10,gerber_format_xd)*(nxt_X-arc_cen_x))/powf(10,gerber_format_xd);
	      del_y=(float)round(powf(10,gerber_format_yd)*(nxt_Y-arc_cen_y))/powf(10,gerber_format_yd);
	      theta=PI/2;						// default to no X, then vertical and angle=PI/2
	      if(fabs(del_x)>TOLERANCE)theta=atan(del_y/del_x);		// if delta X, then calc angle
	      arc_end=theta;
	      if(del_x<0 && del_y>=0)arc_end=PI+theta;			// 2nd quadrant, theta is negative, add to PI
	      if(del_x<=0 && del_y<0)arc_end=PI+theta;			// 3rd quadrant, theta is positive, add to PI
	      if(del_x>0 && del_y<0)arc_end=2*PI+theta;			// 4th quadrant, theta is negative, add to 2*PI
	      printf("GBR:   del_x=%f  del_y=%f  Theta=%f  ArcEnd=%f \n",del_x,del_y,theta*180/PI,arc_end*180/PI);

	      // if CW... draw actual arc segments clockwise
	      if(gerber_interp_mode==2)					
	        {
		arc_inc=fabs((arc_end-arc_st)/circle_points);
		if(arc_inc<TOLERANCE)arc_inc=1.0;
		while(arc_st>=arc_end)					
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;						// last vector made defined here
		  vptr=vertex_make();
		  arc_st-=arc_inc;					// note the "-" ... makes this CCW
		  }
		}
	      // if CCW... draw actual arc segments counter clockwise
	      if(gerber_interp_mode==3)					
	        {
		arc_inc=fabs((arc_end-arc_st)/circle_points);
		if(arc_inc<TOLERANCE)arc_inc=1.0;
		while(arc_st<=arc_end)					
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;						// last vector made defined here
		  vptr=vertex_make();
		  arc_st+=arc_inc;					// note the "+" ... makes this CW
		  }
		}
	      }
	    else  							// if not single quadrant mode...
	      {
	      // in MULTIQUANDRANT arcs, the arc is drawn from crt_X,crt_Y to new X,Y given in the command
	      // with the arc center located at (crt_X+crt_I),(crt_Y+crt_J).
		
	      arc_cen_x=crt_X+nxt_I;					// center point of arc - in multi quad mode it's an addition
	      arc_cen_y=crt_Y+nxt_J;
	      arc_rad=sqrt(nxt_I*nxt_I + nxt_J*nxt_J);			// radius of arc
	      
	      // cacluculate angle to start point of arc
	      del_x=crt_X-arc_cen_x;					
	      del_y=crt_Y-arc_cen_y;
	      theta=PI/2;						// default to no X, then vertical and angle=PI/2
	      if(fabs(del_x)>TOLERANCE)theta=atan(del_y/del_x);		// if delta X, then calc angle
	      if(del_x<0)theta=PI+theta;				// adjust if on other side of Y axis
	      arc_st=theta;
	      
	      // calculate angle to end point of arc
	      del_x=crt_X-arc_cen_x;
	      del_y=crt_Y-arc_cen_y;
	      theta=PI/2;						// default to no X, then vertical and angle=PI/2
	      if(fabs(del_x)>TOLERANCE)theta=atan(del_y/del_x);		// if delta X, then calc angle
	      if(del_x<0)theta=PI+theta;				// adjust if on other side of Y axis
	      arc_end=theta;
	      
	      printf("GBR: Multi Quadrant Interpolation\n");
	      printf("GBR:   X0=%f Y0=%f X1=%f Y1=%f I=%f J=%f\n",crt_X,crt_Y,nxt_X,nxt_Y,nxt_I,nxt_J);
	      printf("GBR:   Xc=%f Yc=%f R=%f \n",arc_cen_x,arc_cen_y,arc_rad);
	      printf("GBR:  ArcSt=%f  ArcEn=%f \n",arc_st*180/PI,arc_end*180/PI);
	      
	      // at this point, the start and end points have been calculated and should fall between 0 and 2*PI
	      // now determine how to increment based on CW or CCW and the start/end values.
	      
	      // if CW and not crossing over 0/2*PI boundary ... draw actual arc segments clockwise
	      if(arc_st>arc_end && gerber_interp_mode==2)		
	        {
		// now sweep arc from start point to end point
		arc_inc=(arc_st-arc_end)/circle_points;
		while(arc_st>arc_end)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st-=arc_inc;
		  }
		}
	      // if CW and crossing the 0/2*PI boundary... draw segments up to 2*PI
	      if(arc_st<arc_end && gerber_interp_mode==2)		
	        {
		arc_inc=((2*PI-arc_end)+arc_st)/circle_points;
		// first sweep from arc start to 2*PI
		while(arc_st>0)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st-=arc_inc;
		  }
		arc_st+=2*PI;
		// then sweep arc from 2*PI to end point
		while(arc_st>arc_end)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st-=arc_inc;
		  }
		}
		
	      // if CCW and not crossing the 0/2*PI boundary ...
	      if(arc_st<arc_end && gerber_interp_mode==3)		
	        {
		// now sweep arc from start point to end point
		arc_inc=(arc_end-arc_st)/circle_points;
		while(arc_st<arc_end)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st+=arc_inc;
		  }
		}
	      // if CCW and crossing the 0/2*PI boundary...
	      if(arc_st>arc_end && gerber_interp_mode==3)		
	        {
		arc_inc=((2*PI-arc_st)+arc_end)/circle_points;
		// sweep from arc start to 2*PI
		while(arc_st<2*PI)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st+=arc_inc;
		  }
		arc_st-=2*PI;
		// sweep arc from 2*PI to end point
		while(arc_st<arc_end)
		  {
		  vptr->x=arc_cen_x+(arc_rad*cos(arc_st));
		  vptr->y=arc_cen_y+(arc_rad*sin(arc_st));
		  if(vold!=NULL)vold->next=vptr;
		  vold=vptr;
		  vptr=vertex_make();
		  arc_st+=arc_inc;
		  }
		}
	      }		// end of multi-quadrant if
	      
	    pptr=polygon_make(trace_vtx,MDL_FILL,0);			// create a new polygon 
	    polygon_insert(sptr,MDL_FILL,pptr);				// insert it into the slice
	    }		// end of circular interpolation if
	    
	  }		// end of NULL trace_vtx check
	  
	nxt_X=vold->x;nxt_Y=vold->y;nxt_I=vold->i,nxt_J=vold->j;	// define the next vertex
	pre_X=crt_X;pre_Y=crt_Y;pre_I=crt_I;pre_J=crt_J;		// define the previous vertex
	crt_X=nxt_X;crt_Y=nxt_Y;crt_I=nxt_I;crt_J=nxt_J;		// define the current vertex
        }		// end of PenDown (D01) if
	
      // if a PenUp (D02) type movement...
      if(vptr->attr==2)
        {
	// convert previous string of verticies into a polygon
	if(trace_vtx!=NULL)						// if a previous string of verticies exists...
	  {
	  if(trace_vtx->next!=NULL)					// if that string is longer than one vertex...
	    {
	    if(crt_ap!=NULL){gerber_trace_polygon(sptr,crt_ap,trace_vtx);} // convert existing string of verticies to polygon
	    else {printf("\n WARNING: Attempt to trace polygon without appature defined!\n");}
	    }
	  }
	trace_vtx=vptr;							// define this NEW vertex as the start of the next trace
	vold=vptr;							// define the vertex to add onto, otherwise defined within DO1 command
	
	printf("GBR: PenUp From:<%f><%f>  To:<%f><%f>\n",crt_X,crt_Y,nxt_X,nxt_Y);
	printf("GBR: PenUp Vptr Start Pt: <%f><%f>\n",trace_vtx->x,trace_vtx->y);
	pre_X=crt_X;pre_Y=crt_Y;pre_I=crt_I;pre_J=crt_J;
	crt_X=nxt_X;crt_Y=nxt_Y;crt_I=nxt_I;crt_J=nxt_J;
        }
	
      // if a flash (D03) type movement...
      if(vptr->attr==3)
        {
	printf("      D03:  sptr=%X  crt_ap=%X  X=%f  Y=%f \n",sptr,crt_ap,nxt_X,nxt_Y);
	gerber_flash_polygon(sptr,crt_ap,nxt_X,nxt_Y);
	pre_X=crt_X;pre_Y=crt_Y;pre_I=crt_I;pre_J=crt_J;
	crt_X=nxt_X;crt_Y=nxt_Y;crt_I=nxt_I;crt_J=nxt_J;
        }
	
      }
    }		// end of file scan loop

  //gerber_ap_trace_merge(mptr,sptr);
  fclose(model_in);
  if(ferror(model_in))
    {
    printf("/nError - unable to properly read Gerber file data!\n");
    return(FALSE);
    }
  
  printf("\nExiting GERBER MODEL LOAD\n\n");  
      
  return(1);
}
    
// Function to parse an aperature definition from a gerber file.
//
// Input is the model pointer, a pointer to the command string read from the file, and the aperature to load.
// Return is 1 if another element was added to the linked list of apertures already defined.  0 if not.
//
int gerber_aperature_parse(slice *sptr, char *in_str, gbr_aperature *aptr)
{
    
  int		h,i;
  int		is_number;
  char 		ach,astr[12];
  
  
  // Char by char parse of scratch string to pick off aperature values.
  //   ID starts with 5th char and continues until not a number.
  //   type is the first and only char after ID.
  //   diam are the chars immediately following the comma until not a number.
  //   hole are the chars immediately following "X" until not a number and may not be present.
  
//  printf("GBR: Entering APERATURE PARSE\n");
  h=0;									// init reusable counter into astr
  i=0;									// init counter into scratch string
  memset(astr,0,10);							// load temp string with zeros
  while(i<strlen(in_str))
    {
    ach=in_str[i];
    is_number=0;
    if(ach>44 && ach<58)is_number=1;					// set flag to indicate if a number or not

    // check if ID set yet
    if(i>3 && aptr->ID==0)						
      {
      if(is_number)
	{
	astr[h]=scratch[i];
	h++;
	}
      else 
	{
	astr[h]=0;
	h=0;
	aptr->ID=atoi(astr);
	memset(astr,0,10);
	}
      }

    // check if type set yet
    if(aptr->ID!=0 && aptr->type==0)				
      {
      aptr->type=ach;
      i++;								// increment here to skip over following comma
      }

    // type=C , get diam and hole
    if(aptr->type==67)						
      {
      if(aptr->diam==0)
	{
	if(is_number)
	  {
	  astr[h]=scratch[i];
	  h++;
	  }
	else 
	  {
	  astr[h]=0;
	  h=0;
	  aptr->diam=atof(astr);
	  if(gerber_units==1)aptr->diam*=25.4;				// if inbound dims in inches, convert to millimeters
	  memset(astr,0,10);
	  i++;
	  continue;
	  }
	}
      if(aptr->diam!=0 && aptr->hole==0)				// check if hole set yet
	{
	if(is_number)
	  {
	  astr[h]=scratch[i];
	  h++;
	  }
	else 
	  {
	  astr[h]=0;
	  h=0;
	  aptr->hole=atof(astr);
	  if(gerber_units==1)aptr->hole*=25.4;				// if inbound dims in inches, convert to millimeters
	  memset(astr,0,10);
	  }
	}
      }
    
    // type=R or O , get X, Y and hole
    if(aptr->type==82 || aptr->type==79)				  
      {
      if(aptr->X==0)							// check if X value set yet
	{
	if(is_number)
	  {
	  astr[h]=scratch[i];
	  h++;
	  }
	else 
	  {
	  astr[h]=0;
	  h=0;
	  aptr->X=atof(astr);
	  if(gerber_units==1)aptr->X*=25.4;				// if inbound dims in inches, convert to millimeters
	  memset(astr,0,10);
	  i++;
	  continue;
	  }
	}
      if(aptr->X!=0 && aptr->Y==0)					// check if Y value set yet
	{
	if(is_number)
	  {
	  astr[h]=scratch[i];
	  h++;
	  }
	else 
	  {
	  astr[h]=0;
	  h=0;
	  aptr->Y=atof(astr);
	  if(gerber_units==1)aptr->Y*=25.4;				// if inbound dims in inches, convert to millimeters
	  memset(astr,0,10);
	  i++;
	  continue;
	  }
	}
      if(aptr->Y!=0 && aptr->hole==0)					// check if hole value set yet
	{
	if(is_number)
	  {
	  astr[h]=scratch[i];
	  h++;
	  }
	else 
	  {
	  astr[h]=0;
	  h=0;
	  aptr->hole=atof(astr);
	  if(gerber_units==1)aptr->hole*=25.4;				// if inbound dims in inches, convert to millimeters
	  memset(astr,0,10);
	  i++;
	  continue;
	  }
	}
      }
      
    i++;								// increment to next char in scratch string
    }
/* 
 printf("GBR: Apperture Parse \n");
 printf("GBR:         gap_first = %X \n",sptr->gap_first);
 printf("GBR:           address = %X \n",aptr);
 printf("GBR:                ID = %d \n",aptr->ID);
 printf("GBR:              type = %d \n",aptr->type);
 printf("GBR:          diameter = %f \n",aptr->diam);
 printf("GBR:                 X = %f \n",aptr->X);
 printf("GBR:                 Y = %f \n",aptr->Y);
 printf("GBR:              hole = %f \n",aptr->hole);
 printf("GBR: Exiting APERATURE PARSE\n");
*/
 
 return(1);
}   
    
// Function to parse XYD command from gerber file.
// Input is a pointer to the command string read from the file and
// a pointer to a vertex that will get loaded with the actual values found in the string.
//
// Note that per gerber spec, input strings can come in a variety of formats:
//   X250000Y11500000D01*    	where all three elements are given (X,Y, and D)
//   X0Y0D01*			where all three elements are given and precision implied
//   X200000D03*		where no Y value is given
//   Y150000D03*		where no X value is given
//   X-0100Y-5000I0J-0300D01*	where negative values and multiquad circular interp values are given
//
// Return value is a bit setting of which value got read:  bit0=x bit1=y bit2=i bit3=j bit4=d
//
int gerber_XYD_parse(char *in_str, vertex *vptr)
{
    int		i;							// scratch counters
    int		rval=0;							// tracks which values were read in bit format
    int 	is_number;						// flag to indicate if value is a number
    int		is_negative;						// flag to indicate if inbound value is negative
    int		x_read=0,y_read=0,i_read=0,j_read=0,d_read=0;		// flags to indicate if value we are accumulating
    int		x_pos=0,y_pos=0,i_pos=0,j_pos=0,d_pos=0;		// offsets into string accumulators
    char 	x_str[15],y_str[15],i_str[15],j_str[15],d_str[15];	// sting accumulators for each value (sign + 6i + 6d + "." + string-term = 15 chars max)
    char	t_str[15];

   printf("GBR: Entering XYIJD parse\n");

    i=0;								// init counter into scratch string
    memset(x_str,0,15);							// load accumulator string with zeros
    memset(y_str,0,15);	
    memset(i_str,0,15);	
    memset(j_str,0,15);	
    memset(d_str,0,15);	
    memset(t_str,0,15);
    while(i<strlen(in_str))
      {
      is_number=0;
      if(in_str[i]>44 && in_str[i]<58)is_number=1;			// set flag to indicate if a number or not
      
      if(in_str[i]==88){x_read=1;y_read=0;i_read=0;j_read=0;d_read=0;}	// if X encountered...
      if(in_str[i]==89){x_read=0;y_read=1;i_read=0;j_read=0;d_read=0;}	// if Y encountered...
      if(in_str[i]==73){x_read=0;y_read=0;i_read=1;j_read=0;d_read=0;}	// if I encountered...
      if(in_str[i]==74){x_read=0;y_read=0;i_read=0;j_read=1;d_read=0;}	// if J encountered...
      if(in_str[i]==68){x_read=0;y_read=0;i_read=0;j_read=0;d_read=1;}	// if D encountered...

      if(x_read==1 && is_number)					// if reading into x accumulator...	
	{
	x_str[x_pos]=in_str[i];						// ... define the new char
	x_pos++;							// ... incrment offset into string
	if(x_pos>13)x_pos=13;						// ... ensure no over-run
	}
      if(y_read==1 && is_number)					// if reading into y accumulator...	
	{
	y_str[y_pos]=in_str[i];						// ... define the new char
	y_pos++;							// ... incrment offset into string
	if(y_pos>13)y_pos=13;						// ... ensure no over-run
	}
      if(i_read==1 && is_number)					// if reading into d accumulator...	
	{
	i_str[i_pos]=in_str[i];						// ... define the new char
	i_pos++;							// ... incrment offset into string
	if(i_pos>13)i_pos=13;						// ... ensure no over-run
	}
      if(j_read==1 && is_number)					// if reading into d accumulator...	
	{
	j_str[j_pos]=in_str[i];						// ... define the new char
	j_pos++;							// ... incrment offset into string
	if(j_pos>13)j_pos=13;						// ... ensure no over-run
	}
      if(d_read==1 && is_number)					// if reading into d accumulator...	
	{
	d_str[d_pos]=in_str[i];						// ... define the new char
	d_pos++;							// ... incrment offset into string
	if(d_pos>13)d_pos=13;						// ... ensure no over-run
	}
	
      i++;								// increment to next char in in_str
      }
    
    printf("GBR:   XYIJD Parse Raw:  Xs=%s Ys=%s Is=%s Js=%s Ds=%s \n",x_str,y_str,i_str,j_str,d_str);
    
    // The three string accumulators are loaded with numbers, but have not yet been formatted.
    // A decimal must be inserted at the location as dictated by the format specifier (%FS) command.
    // Since leading zeros can be omitted, we must count backwards from the end of the string to insert it.
    
    // process X string
    i=strlen(x_str);
    if(i>0)
      {
      is_negative=0;
      if(x_str[0]==45)
        {
	is_negative=1;							// if "-" preceeds the value
	x_str[0]=48;							// replace with a 0 while padding
	}
      while(i<11)							// pad all strings with leading 0s
	{
	strcpy(t_str,"0");
	strcat(t_str,x_str);
	strcpy(x_str,t_str);
	i=strlen(x_str);
	}
      for(i=0;i<(10-gerber_format_xd);i++)				// shift left to make room for "."
	{
	x_str[i]=x_str[i+1];
	}
      x_str[10-gerber_format_xd]=46;
      if(is_negative)x_str[0]=45;
      }
    
    // process Y string
    i=strlen(y_str);
    if(i>0)
      {
      is_negative=0;
      if(y_str[0]==45)
        {
	is_negative=1;							// if "-" preceeds the value
	y_str[0]=48;							// replace with a 0 while padding
	}
      while(i<11)
	{
	strcpy(t_str,"0");						// ... pad with leading 0s
	strcat(t_str,y_str);
	strcpy(y_str,t_str);
	i=strlen(y_str);
	}
      for(i=0;i<(10-gerber_format_yd);i++)				// shift left to make room for "."
	{
	y_str[i]=y_str[i+1];
	}
      y_str[10-gerber_format_yd]=46;
      if(is_negative)y_str[0]=45;
      }

    // process I string
    i=strlen(i_str);
    if(i>0)
      {
      is_negative=0;
      if(i_str[0]==45)
        {
	is_negative=1;							// if "-" preceeds the value
	i_str[0]=48;							// replace with a 0 while padding
	}
      while(i<11)
	{
	strcpy(t_str,"0");						// ... pad with leading 0s
	strcat(t_str,i_str);
	strcpy(i_str,t_str);
	i=strlen(i_str);
	}
      for(i=0;i<(10-gerber_format_xd);i++)				// shift left to make room for "."
	{
	i_str[i]=i_str[i+1];
	}
      i_str[10-gerber_format_yd]=46;
      if(is_negative)i_str[0]=45;
      }
    
    // process J string
    i=strlen(j_str);
    if(i>0)
      {
      is_negative=0;
      if(j_str[0]==45)
        {
	is_negative=1;							// if "-" preceeds the value
	j_str[0]=48;							// replace with a 0 while padding
	}
      while(i<11)
	{
	strcpy(t_str,"0");						// ... pad with leading 0s
	strcat(t_str,j_str);
	strcpy(j_str,t_str);
	i=strlen(j_str);
	}
      for(i=0;i<(10-gerber_format_yd);i++)				// shift left to make room for "."
	{
	j_str[i]=j_str[i+1];
	}
      j_str[10-gerber_format_yd]=46;
      if(is_negative)j_str[0]=45;
      }
      
    printf("GBR:   XYIJD Parse Dec:  Xs=%s Ys=%s Is=%s Js=%s Ds=%s \n",x_str,y_str,i_str,j_str,d_str);

    // At this point the three string accumulators can be converted to values.
    // If any given string is still NULL (all zeros) then do not provide a value.
    if(strlen(x_str)>0)
      {
      rval |= 1 << 0;							// set bit 0 to true
      vptr->x=atof(x_str);
      if(gerber_units==1)vptr->x*=25.4;					// if inbound dims in inches, convert to millimeters
      }
    if(strlen(y_str)>0)
      {
      rval |= 1 << 1;							// set bit 1 to true
      vptr->y=atof(y_str);
      if(gerber_units==1)vptr->y*=25.4;					// if inbound dims in inches, convert to millimeters
      }
    if(strlen(i_str)>0)
      {
      rval |= 1 << 2;							// set bit 2 to true
      vptr->i=atof(i_str);
      if(gerber_units==1)vptr->i*=25.4;					// if inbound dims in inches, convert to millimeters
      }
    if(strlen(j_str)>0)
      {
      rval |= 1 << 3;							// set bit 3 to true
      vptr->j=atof(j_str);
      if(gerber_units==1)vptr->j*=25.4;					// if inbound dims in inches, convert to millimeters
      }
    if(strlen(d_str)>0)
      {
      rval |= 1 << 4;							// set bit 4 to true
      vptr->attr=atoi(d_str);
      }

    printf("GBR:   XYIJD Parse Val:  Xs=%f Ys=%f Is=%f Js=%f Ds=%d \n",vptr->x,vptr->y,vptr->i,vptr->j,vptr->attr);
    printf("GBR: Exiting XYIJD parse\n");
    return(rval);
}

// Function to generate a polygon from a gerber flash command
// Inputs:  model, desired aperature to use, and current x and y
// Return:  0 if failed, 1 if success resulting in closed polygon added to slice linked list
int gerber_flash_polygon(slice *sptr, gbr_aperature *aptr, float x, float y)
{
  float		i;
  int		circle_points=10;
  float 	stretch;
  vertex	*shape_start,*vptr,*vold;
  polygon	*pptr;

  printf("GBR: Entering flash polygon\n");
  printf("GBR:   Apr=%X ID=%d type=%c diam=%f\n",aptr,aptr->ID,aptr->type,aptr->diam);
  printf("GBR:   at x=%f y=%f dark=%d\n",x,y,gerber_polarity);

  // initialize
  shape_start=NULL;

  // if circular...
  if(aptr->type==67)
    {
    i=0;
    while(i<(2*PI-(2*PI/circle_points)))
      {
      vptr=vertex_make();
      vptr->x=x+(aptr->diam/2)*(cos(i));
      vptr->y=y+(aptr->diam/2)*(sin(i));
      vptr->i=1;							// use empty i slot to store value that indicates aperature flash
      if(shape_start==NULL)
        {
	shape_start=vptr;
	}
      else 
        {
	vold->next=vptr;
        }
      vold=vptr;
      i+=(2*PI/circle_points);
      }
    vold->next=shape_start;
    pptr=polygon_make(shape_start,TRACE,0);				// create a new polygon 
    pptr->vert_qty=circle_points;					// define qty of vtxs
    polygon_insert(sptr,TRACE,pptr);					// insert it into the slice
    if(gerber_polarity==0)pptr->hole=1;					// set to hole if polarity is clear
    }
    
    
  // if rectangular...
  if(aptr->type==82)
    {
    shape_start=vertex_make();
    shape_start->x=x-aptr->X/2;						// start in lower left corner
    shape_start->y=y-aptr->Y/2;
    shape_start->i=1;							// use empty i slot to store value that indicates aperature flash
    vptr=vertex_make();
    shape_start->next=vptr;
    vptr->x=x-aptr->X/2;						// upper left
    vptr->y=y+aptr->Y/2;
    vptr->i=1;								// use empty i slot to store value that indicates aperature flash
    vold=vptr;
    vptr=vertex_make();
    vold->next=vptr;
    vptr->x=x+aptr->X/2;						// upper right
    vptr->y=y+aptr->Y/2;
    vptr->i=1;								// use empty i slot to store value that indicates aperature flash
    vold=vptr;
    vptr=vertex_make();
    vold->next=vptr;
    vptr->x=x+aptr->X/2;						// upper left
    vptr->y=y-aptr->Y/2;
    vptr->i=1;								// use empty i slot to store value that indicates aperature flash
    vptr->next=shape_start;						// close polygon

    pptr=polygon_make(shape_start,TRACE,0);				// create a new polygon 
    pptr->vert_qty=4;							// define qty of vtxs
    polygon_insert(sptr,TRACE,pptr);					// insert it into the slice
    if(gerber_polarity==0)pptr->hole=1;					// set to hole if polarity is clear
    }
  
  // if rounded rectangular (obround)...
  if(aptr->type==79)		
    {
    // set diam based on smallest of X or Y values
    if(aptr->Y>0 && aptr->Y<aptr->X)
      {
      aptr->diam=aptr->Y;
      stretch=aptr->X-aptr->diam;
      }
      
    // draw vertical slots (i.e. stretched in Y)
    if(aptr->X>0 && aptr->X<=aptr->Y)
      {
      aptr->diam=aptr->X;
      stretch=aptr->Y-aptr->diam;
      // draw first half of circle
      i=0;
      while(i<PI)
	{
	vptr=vertex_make();
	vptr->x=x+(aptr->diam/2)*(cos(i));
	vptr->y=y+stretch/2+(aptr->diam/2)*(sin(i));
	vptr->i=1;							// use empty i slot to store value that indicates aperature flash
	if(shape_start==NULL)
	  {
	  shape_start=vptr;
	  }
	else 
	  {
	  vold->next=vptr;
	  }
	vold=vptr;
	i+=(2*PI/circle_points);
	}
      // draw second half of circle
      i=PI;
      while(i<(2*PI))
	{
	vptr=vertex_make();
	vptr->x=x+(aptr->diam/2)*(cos(i));
	vptr->y=y-stretch/2+(aptr->diam/2)*(sin(i));
	vptr->i=1;							// use empty i slot to store value that indicates aperature flash
	vold->next=vptr;
	vold=vptr;
	i+=(2*PI/circle_points);
	}
      }
    
    // draw horizontal slots (i.e. stretched in X)
    if(aptr->X>0 && aptr->X<=aptr->Y)
      {
      aptr->diam=aptr->X;
      stretch=aptr->Y-aptr->diam;
      // draw first half of circle
      i=(-PI/2);
      while(i<(PI/2))
	{
	vptr=vertex_make();
	vptr->x=x+stretch/2+(aptr->diam/2)*(cos(i));
	vptr->y=y+(aptr->diam/2)*(sin(i));
	vptr->i=1;							// use empty i slot to store value that indicates aperature flash
	if(shape_start==NULL)
	  {
	  shape_start=vptr;
	  }
	else 
	  {
	  vold->next=vptr;
	  }
	vold=vptr;
	i+=(2*PI/circle_points);
	}
      // draw second half of circle
      i=(PI/2);
      while(i<(3*PI/2))
	{
	vptr=vertex_make();
	vptr->x=x-stretch/2+(aptr->diam/2)*(cos(i));
	vptr->y=y+(aptr->diam/2)*(sin(i));
	vptr->i=1;							// use empty i slot to store value that indicates aperature flash
	vold->next=vptr;
	vold=vptr;
	i+=(2*PI/circle_points);
	}
      }
      
    vold->next=shape_start;
    pptr=polygon_make(shape_start,TRACE,0);				// create a new polygon 
    polygon_insert(sptr,TRACE,pptr);					// insert it into the slice
    if(gerber_polarity==0)pptr->hole=1;					// set to hole if polarity is clear
    }	
  
  // if polygon type requested...
  if(aptr->type==80)
    {
    printf("GBR:  ERROR - Unsupported polygon aperature request encountered!\n");
    }


  printf("GBR: Exiting flash polygon\n");
  
  return(1);
}


// Function to make a polygon out of a string of trace verticies.
//
// The initial trace is a center line down the path the trace wants to occupy.
// This routine offsets that centerline in both directions normal to it by a width dictated
// by the aperature setting.  It then closes off the ends to form a closed polygon.
// That polygon should then intersect other closed polygons that were formed by aperature flashes
// (i.e. component pads).  So the bottom end of this function then looks for those intersections
// and combines overlapping polygons into a single trace polygon that includes the pads.

int gerber_trace_polygon(slice *sptr, gbr_aperature *cap, vertex *trace)
{

  int		i,pinp;
  vertex	*vptr;
  vertex	*vlast,*vcrnt,*vnext,*vbase,*vnew,*vnold,*vtxA,*vtxB,*vtxC;
  vector	*vecptr,*vecfirst,*vecnext,*veclast,*vecA,*vecB,*vec_ss,*uA,*uB,*vrot;
  polygon	*pptr,*pinpt,*plst,*pnxt;
	  
  float 	offset=1.0,odist;					// offset distance in millimeters
  float 	base_angle_A,base_angle_B;				// absolute angle of primary vector in radians
  float 	rel_angle_AB,vec_angle;					// relative angle between vectors in radians
  double  	magvecA,magvecB,magvecC;
  double 	dxA,dyA,dxB,dyB,vtxAx,vtxAy,vtxBx,vtxBy;
  double 	dxC,dyC;
  double 	newdist,alpha,sa;

  printf("GBR: Entering trace polygon\n");

  if(sptr==NULL || trace==NULL)return(0);				// don't try to process what does not exist
  pinpt=polygon_make(trace,2,0);					// create a single line polygon from the vertex list
  polygon_insert(sptr,MDL_PERIM,pinpt);					// add the new polygon to the slice as a trace
  pinpt->ID=99;
  printf("GBR:   polygon created\n");

  // add a perpendicular cross bar of aperature width at the tail (base) of the FIRST vertex (vector)
  vlast=pinpt->vert_first;						// save address of previous vertex
  vcrnt=pinpt->vert_first->next;					// start at one vertex past the polygon's first vertex
  dxA=vcrnt->x-vlast->x;
  dyA=vcrnt->y-vlast->y;
  magvecA=sqrt(dxA*dxA+dyA*dyA);
  if(magvecA<TOLERANCE)magvecA=1.0;
  odist=cap->diam/2.0;
  if(odist<=0)odist=0.25;
  vtxAx=odist*dxA/magvecA;						//  adj magnitude to aperature size
  vtxAy=odist*dyA/magvecA;
  
  vnew=vertex_make();							// create the tip vertex of the cross vector
  vnew->x=vlast->x-vtxAy;						// X & Y ARE CORRECT HERE... it's how one does
  vnew->y=vlast->y+vtxAx;						// a 90 degree rotation about a point
  vnew->z=vlast->z;
  vnew->i=2;								// indicate it was made for a trace
  vnew->attr=0;
  //vnew->attr=vlast->attr;
  
  vbase=vertex_make();							// create the tail vertex of the cross vector
  vbase->x=vlast->x+vtxAy;
  vbase->y=vlast->y-vtxAx;
  vbase->z=vlast->z;
  vbase->i=2;								// indicate it was made for a trace
  vbase->attr=0;
  //vbase->attr=vlast->attr;

  vec_ss=vector_make(vnew,vbase,pinpt->ID);				// temporary vector to define straight skeleton
  vector_insert(sptr,TRACE,vec_ss);					// insert it into our linked list

  // calculate bisector between adjacent vectors of each vertex and strore in straight skeleton.
  // initially use only the normalized (unity) value of offset to help ensure we are offsetting to the right direction.
  // the next loop will adjust bisector length depending on what needs to get done.
  vlast=pinpt->vert_first;						// save address of previous vertex
  vcrnt=pinpt->vert_first->next;					// start at one vertex past the polygon's first vertex
  do
    {
    if(vcrnt->next==NULL)break;						// not enough verticies to offset by this method
    vnext=vcrnt->next;

    // create base vertex of the bisector vector from vcrnt
    vbase=vertex_make();						// temporary copy of vcrnt
    vbase->x=vcrnt->x;
    vbase->y=vcrnt->y;
    vbase->z=vcrnt->z;
    vbase->i=2;								// indicate it was made for a trace
    vbase->attr=0;
    //vbase->attr=vcrnt->attr;

    // normalize the previous vector
    dxA=vcrnt->x-vlast->x;
    dyA=vcrnt->y-vlast->y;
    magvecA=sqrt(dxA*dxA+dyA*dyA);
    if(magvecA<TOLERANCE)magvecA=1.0;
    vtxAx=dxA/magvecA;	
    vtxAy=dyA/magvecA;
    
    // normalize the next vector
    dxB=vcrnt->x-vnext->x;
    dyB=vcrnt->y-vnext->y;
    magvecB=sqrt(dxB*dxB+dyB*dyB);
    if(magvecB<TOLERANCE)magvecB=1.0;
    vtxBx=dxB/magvecB;	
    vtxBy=dyB/magvecB;
    
    // create the tip of the bisector vector by adding the two unit vectors and averaging the result
    vnew=vertex_make();	
    vnew->x=vbase->x+(vtxAx+vtxBx)/2;
    vnew->y=vbase->y+(vtxAy+vtxBy)/2;
    vnew->z=vbase->z;
    vnew->i=2;								// indicate it was made for a trace
    vnew->next=NULL;

    // we need to get that angle bt last and next vectors to determine the correct magnitude of the bisector vector to
    // keep the offsets moving consistently away from walls vs consistently away from the intersection point
    vecA=vector_make(vnew,vbase,0);
    vecA->curlen=vertex_distance(vnew,vbase);
    vecB=vector_make(vnext,vbase,0);
    vecB->curlen=vertex_distance(vnext,vbase);
    alpha=vector_relangle(vecA,vecB);
    sa=sin(alpha);
    if(sa<TOLERANCE)sa=1.0;
    odist=cap->diam/2.0;
    if(odist<=0)odist=0.25;
    newdist=odist/sa;
    vector_destroy(vecA,FALSE);
    vector_destroy(vecB,FALSE);
    
    // now adjust the magnitude of the bisector vector 
    dxC=vnew->x - vbase->x;
    dyC=vnew->y - vbase->y;
    magvecC=sqrt(dxC*dxC+dyC*dyC);
    if(magvecC<TOLERANCE)						// if vectors are exactly opposite...
      {
      vnew->x=vbase->x+vtxBy;						// ... new vector is negative recipocal
      vnew->y=vbase->y-vtxBx;
      dxC=vnew->x - vbase->x;		
      dyC=vnew->y - vbase->y;
      magvecC=sqrt(dxC*dxC+dyC*dyC);
      if(magvecC<TOLERANCE)
	{
	magvecC=CLOSE_ENOUGH;
	}
      }
    vnew->x=vbase->x+newdist*dxC/magvecC;
    vnew->y=vbase->y+newdist*dyC/magvecC;
    
    // now readjust the vbase location to offset the opposite direction that vnew went.
    vbase->x=vbase->x-(newdist*dxC/magvecC);
    vbase->y=vbase->y-(newdist*dyC/magvecC);

    // create the actual bisector vector from the base and tip (new) verticies
    veclast=vec_ss;							// save address of any previously created ss vector
    vec_ss=vector_make(vnew,vbase,pinpt->ID);				// temporary vector to define straight skeleton
    vec_ss->psrc=pinpt;
      
    // insert it into our linked list
    vector_insert(sptr,TRACE,vec_ss);	
    
    // To ensure we are offsetting to the correct side of the PERIM vector each time, we need to test the newly created ss vector
    // along with the previous ss vector to make sure they make a line that is parallel ot the PERIM vector.  If not we need
    // to swap the tip and tail of the newly created ss vector.
    vecA=vector_make(veclast->tail,vec_ss->tail,1);
    vecB=vector_make(veclast->tip,vec_ss->tip,1);
    if(vector_parallel(vecA,vecB)==FALSE)
      {
      vtxA=vec_ss->tail;
      vec_ss->tail=vec_ss->tip;
      vec_ss->tip=vtxA;
      }
    vector_destroy(vecA,FALSE);
    vector_destroy(vecB,FALSE);
    
    // move onto next vertex
    vnold=vnew;								// save address for linked listing
    vlast=vcrnt;
    vcrnt=vcrnt->next;
    if(vcrnt==NULL)break;						// allows offsetting of open polygons
    }while(vcrnt!=pinpt->vert_first->next);

  // add a perpendicular cross bar of aperature width at the tail (base) of the LAST vertex (vector)
  vlast=pinpt->vert_first;
  while(vlast->next!=NULL)						// loop thru vertex list to find last vertex
    {
    vcrnt=vlast;							// saves the vertex just before last one
    vlast=vlast->next;
    }
  dxA=vcrnt->x-vlast->x;
  dyA=vcrnt->y-vlast->y;
  magvecA=sqrt(dxA*dxA+dyA*dyA);
  if(magvecA<TOLERANCE)magvecA=1.0;
  odist=cap->diam/2.0;
  if(odist<=0)odist=0.25;
  vtxAx=odist*dxA/magvecA;						//  adj magnitude to aperature size
  vtxAy=odist*dyA/magvecA;
  
  vnew=vertex_make();							// create the tip vertex of the cross vector
  vnew->x=vlast->x-vtxAy;						// X & Y ARE CORRECT HERE... it's how one does
  vnew->y=vlast->y+vtxAx;						// a 90 degree rotation about a point
  vnew->z=vlast->z;
  vnew->i=2;								// indicate it was made for a trace
  vnew->attr=0;
  //vnew->attr=vlast->attr;
  
  vbase=vertex_make();							// create the tail vertex of the cross vector
  vbase->x=vlast->x+vtxAy;
  vbase->y=vlast->y-vtxAx;
  vbase->z=vlast->z;
  vbase->i=2;								// indicate it was made for a trace
  vbase->attr=0;
  //vbase->attr=vlast->attr;
  
  vec_ss=vector_make(vbase,vnew,pinpt->ID);				// temporary vector to define straight skeleton
  vector_insert(sptr,TRACE,vec_ss);					// insert it into our linked list

  
  // At this point we have a string of vectors stored in the straight skeleton that define our trace.
  // Note that the ss vectors extend in both directions away from their original vertex locations by half the
  // aperature diameter.
  
  // Loop thru the ss vector list and create vertex copies of the tips, then link all those tip verticies
  // to each other for this polygon - this becomes the actual trace.
  i=0;
  vecptr=sptr->ss_vec_first[TRACE];						// start with first vector in list
  while(vecptr!=NULL)							// loop thru all of them
    {
    if(vecptr->type==pinpt->ID)						// if this vector belongs to the correct polygon...
      {
      vnew=vertex_make();
      vnew->x=vecptr->tip->x;
      vnew->y=vecptr->tip->y;
      vnew->z=vecptr->tip->z;
      vnew->i=3;							// created as offset to trace
      vnew->attr=0;
      //vnew->attr=vecptr->tip->attr;
      if(i==0)								// if the first vertex being created...
	{
	vbase=vnew;							// save address of this first vtx to connect with to form loop
	}
      else 								// if not the first vertex being created...
	{
	vnold->next=vnew;						// connect the prev vertex to it
	}
      vnold=vnew;
      i++;
      vecptr->type*=(-1);						// flip sign to indicate it's been used
      }
    vecptr=vecptr->next;
    if(vecptr==sptr->ss_vec_first[TRACE])break;
    }
    
  // reverse the ss linked list so we can create the other side of the polygon
  veclast=NULL;vecnext=NULL;
  vecptr=sptr->ss_vec_first[TRACE];
  while(vecptr!=NULL)
    {
    if(vecptr->type<0)vecptr->type*=(-1);
    vecnext=vecptr->next;
    vecptr->next=veclast;
    veclast=vecptr;
    vecptr=vecnext;
    }
  sptr->ss_vec_first[TRACE]=veclast;					// reset pointer into ss list
  
  // loop thru ss list to create other side of polygon from ss tails
  vecptr=sptr->ss_vec_first[TRACE];					// start with first vector in list
  while(vecptr!=NULL)							// loop thru all of them
    {
    if(vecptr->type==pinpt->ID)						// if this vector belongs to the correct polygon...
      {
      vnew=vertex_make();
      vnew->x=vecptr->tail->x;
      vnew->y=vecptr->tail->y;
      vnew->z=vecptr->tail->z;
      vnew->i=3;							// created as offset to trace
      vnew->attr=vecptr->tail->attr;
      vnold->next=vnew;							// connect the prev vertex to it
      vnold=vnew;
      i++;
      vecptr->type=0;							// flip sign to indicate it's been used
      }
    vecptr=vecptr->next;
    if(vecptr==sptr->ss_vec_first[TRACE])break;
    }
    
  if(i>0)								// if new verticies from ss vectors were created...
    {
    vnold->next=vbase;							// close the loop
    
    // now create an offset polygon and point it to this vector list
    pptr=polygon_make(vbase,TRACE,99);
    pptr->ID=pinpt->ID;
    pptr->member=0;							// important!  this variable will be used to merge overlapping polygons
    pptr->hole=pinpt->hole;
    pptr->dist=odist;
    pptr->vert_qty=i;
    polygon_insert(sptr,TRACE,pptr);
    
    printf("GBR:   polygon=%X  start_vtx=%X  qty=%d\n",pptr,pptr->vert_first,pptr->vert_qty);
    vptr=pptr->vert_first;
    while(vptr!=NULL)
      {
      printf("GBR:   vptr=%X  x=%f  y=%f  next=%X\n",vptr,vptr->x,vptr->y,vptr->next);
      vptr=vptr->next;
      if(vptr==pptr->vert_first)break;
      }
      
    //printf("Offset Insert:  sptr=%X  pptr=%X  ID=%d  Vqty=%d  Next=%X \n",sptr,pptr,pptr->ID,i,pptr->next);
    }

  printf("GBR: Exiting trace polygon\n");
  
  return(1);
}


// Function to merge aperature flash polygons with trace polygons
//
// Loop thru all trace polygons looking for overlapping polygon vectors.
// This works by looking for intersections of each polygon side against all sides of every other
// polygon.  When it finds one, it creates a vertex there breaks the two vectors that cross it.
// After all intersections are found, another loop then looks for verticies that lie inside other
// polygons.  When found, the entire vector connected to that vertex is removed.  The trick is
// combining the resulting polygons into one and keeping all the pointer references straight.

int gerber_ap_trace_merge(model *mptr,slice *sptr)
{
  int		h,i,slot,result,restart,keep_going,start_over;
  polygon	*pptr,*pnxt,*ppre,*pnpr,*pchk,*pnew,*pdel; 
  vertex 	*vptrA,*vold;
  slice 	*snew;
  genericlist	*glptr;
  
  //printf("GBR: Entering aperature-trace merge.\n");
  
  //slice_dump(sptr);
  
  // init polygons and vertexes
  gerber_track_ID=0;
  gerber_vtx_inter=1000;						// init unique vtx ID intersection counter
  sptr->pqty[TRACE]=0;
  pptr=sptr->pfirst[TRACE];
  while(pptr!=NULL)
    {
    vptrA=pptr->vert_first;
    while(vptrA!=NULL)
      {
      vptrA->attr=0;
      vptrA->supp=0;
      vptrA=vptrA->next;
      if(vptrA==pptr->vert_first)break;
      }
    //pptr->type=TRACE;
    pptr->member=(1);
    polygon_contains_material(sptr,pptr,TRACE);
    pptr->area=polygon_find_area(pptr);
    polygon_find_center(pptr);
    sptr->pqty[TRACE]++;
    pptr=pptr->next;
    }
    
  //return(1);  // debug

  // find tool requested to build this file
  slot=(-1);
  glptr=mptr->oper_list;
  while(glptr!=NULL)
    {
    if(strlen(glptr->name)>1){slot=glptr->ID;break;}
    glptr=glptr->next;
    }
  printf("\nGBR - trace merge: mptr=%X  slot=%d  name=%s \n",mptr,slot,mptr->model_file);  
  if(slot<0 || slot>=MAX_TOOLS)return(0);				// if tool location not reasonable, leave

  // create a new slice after the current one to load the merged polygons into for debug
  sptr->sz_level=0.25;
  snew=slice_make();
  slice_insert(mptr,slot,sptr,snew);
  snew->sz_level=0.50;
  
  // Loop thru all polygons to create intersection vertexes and associate overlapping polygons into singular tracks
  // of the same polygon->member ID
  printf("   Finding intersections...\n");
  ppre=NULL;						
  keep_going=TRUE;
  while(keep_going==TRUE)
    {
    start_over=FALSE;
    pptr=sptr->pfirst[TRACE];						// start with first poly
    while(pptr!=NULL)							// loop thru the whole set
      {
      pnxt=sptr->pfirst[TRACE];						// start with first poly
      while(pnxt!=NULL)							// loop thru the whole set because prev polys may now intersect after combined
	{
	if(pnxt==pptr){pnxt=pnxt->next; continue;}			// don't test against itself
	pnew=polygon_boolean2(sptr,pptr,pnxt,1);			// combine them into a new poly
	if(pnew==pnxt)
	  {
	  polygon_delete(sptr,pptr);					// if A enclosed B, keep A and delete B
	  pptr=sptr->pfirst[TRACE];
	  pnxt=sptr->pfirst[TRACE];
	  start_over=TRUE;
	  }
	else if(pnew==pptr)
	  {
	  polygon_delete(sptr,pnxt);					// if B enclosed A, keep B and delete A
	  pptr=sptr->pfirst[TRACE];
	  pnxt=sptr->pfirst[TRACE];
	  start_over=TRUE;
	  }
	else if(pnew!=NULL && pnew!=pptr && pnew!=ppre)			// if neither was entirely inside the other...
	  {
	  polygon_insert(sptr,TRACE,pnew);				// add in the new poly
	  polygon_delete(sptr,pptr);					// delete the first one
	  polygon_delete(sptr,pnxt);					// delete the second one
	  pptr=sptr->pfirst[TRACE];
	  pnxt=sptr->pfirst[TRACE];
	  start_over=TRUE;
	  }
	if(start_over==TRUE)break;
	pnxt=pnxt->next;
	}
      if(start_over==TRUE)break;
      ppre=pptr;
      pptr=pptr->next;
      }
    if(pnxt==NULL && pptr==NULL)keep_going=FALSE;
    }

  return(1);
}

// Function to extrude the gerber slice into a 3D faceted model.
// this function copies the gerber trace outlines to a new hieght then connects them with vertical facets.
// this allows gerber traces to fit more neatly into the rest of the code structure.
int gerber_slice_extrude(model *mptr)
{
  vertex 	*vptr,*vnxt,*vup1,*vup2;
  vertex 	*vold,*vfirst;
  polygon 	*pptr,*pupr,*pold;
  slice		*sptr,*slcbtm,*slctop;
  
  if(mptr==NULL)return(FALSE);
  
  slcbtm=mptr->slice_first[TRACE];
  slctop=slice_make();
  slctop->sz_level=1.000;
  
  pold=NULL;
  pptr=slcbtm->pfirst[TRACE];
  while(pptr!=NULL)
    {
    vfirst=NULL; vold=NULL;
    vptr=pptr->vert_first;
    while(vptr!=NULL)
      {
      vup1=vertex_copy(vptr,NULL);
      vup1->z=1.0;
      if(vfirst==NULL)vfirst=vptr;
      if(vold!=NULL)vold->next=vptr;
      vold=vptr;
      if(vptr==pptr->vert_first)break;
      }
      
    // close polygon and create a copy of the btm polygon for the top
    if(vold!=NULL)vold->next=vfirst;
    pupr=polygon_make(vfirst,2,pptr->member);
      
    // add polygon to upper slice
    if(slctop->pfirst[TRACE]==NULL)slctop->pfirst[TRACE]=pupr;
    if(pold!=NULL)pold->next=pupr;
    pold=pupr;

    pptr=pptr->next;
    }
  
  
  return(TRUE);
}
