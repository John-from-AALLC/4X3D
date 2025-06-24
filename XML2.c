#include "Global.h"	

// Function to create a generic linked list element
genericlist *genericlist_make(void)
{
    genericlist	*gptr;
    
    gptr=(genericlist *)malloc(sizeof(genericlist));
    if(gptr==NULL)return(NULL);
    gptr->ID=(-1);
    memset(gptr->name,0,sizeof(gptr->name));
    gptr->next=NULL;
    
    return(gptr);
}

// Function to add a generic linked list element to an existing list
int genericlist_insert(genericlist *existing_list, genericlist *new_element)
{
    int			status=0;
    genericlist		*gptr;
    
    if(existing_list==NULL || new_element==NULL)return(0);
    gptr=existing_list;
    while(gptr!=NULL)
      {
      if(gptr->next==NULL)break;
      gptr=gptr->next;
      }
    if(gptr!=NULL)
      {
      gptr->next=new_element;
      new_element->next=NULL;
      status=1;
      }
    return(status);
}

// Fucntion to purge all nodes from an existing generic list
int genericlist_delete_all(genericlist *existing_list)
{
  genericlist *gptr,*gdel;
  
  if(existing_list==NULL)return(FALSE);
  
  gptr=existing_list;							// start at first element in list
  while(gptr!=NULL)							// loop thru entire list
    {
    gdel=gptr;
    gptr=gptr->next;							// get next element in list
    free(gdel);
    }
  existing_list=NULL;

  return(TRUE);
}

// Function to remove an element from an existing generic list
genericlist *genericlist_delete(genericlist *existing_list, genericlist *gdel)
{
    genericlist 	*gptr;
    
    if(existing_list==NULL)return(NULL);
    if(gdel==NULL)return(NULL);
    
    if(existing_list==gdel)						// if first node...
      {
      existing_list=gdel->next;						// ... redefine head node as next node
      free(gdel);							// ... and release delete node from memory
      }
    else 								// otherwise if NOT first node...
      {
      gptr=existing_list;						// ... start from head node
      while(gptr->next!=gdel)gptr=gptr->next;				// ... and search for node before delete node
      if(gptr!=NULL)							// ... if found...
        {
	gptr->next=gdel->next;						// ... skip over delete node
	free(gdel);							// ... release it from memory
	}
      }
    return(existing_list);						// return success
}

// Function to find an element in an existing list by name and return ID
int genericlist_find(genericlist *existing_list, char *find_name)
{
    int		found_ID=(-1);
    genericlist	*gptr;
    
    gptr=existing_list;
    while(gptr!=NULL)
      {
      if(strstr(gptr->name,find_name)!=NULL)break;
      gptr=gptr->next;
      }
    if(gptr!=NULL)found_ID=gptr->ID;

    return(found_ID);
}



// Function to manage line type linked lists
// This function allocates space for a single link list element of line type
linetype *linetype_make(void)
  {
  linetype 	*lptr;

    lptr=(linetype *)malloc(sizeof(linetype));
    if(lptr==NULL)return(NULL);
    lptr->ID=0;								// unique line type ID. See #defs for PERIM, OFFSET, FILL, SUPPORT, TRACE, DRILL
    memset(lptr->name,0,sizeof(lptr->name));				// line type name:  Perimeter, Offset, etc.
    lptr->tempadj=0.0;							// temperature adjustment off nominal
    lptr->flowrate=0.0;							// amount of material deposited per unit length drawn
    lptr->feedrate=0.0;							// speed at which a tool is moved while drawing/depositing/cutting
    lptr->line_width=0.0;						// width of single pass of deposition/cut in mm
    lptr->line_pitch=0.0;						// pitch between nieghboring single passes
    lptr->wall_width=0.0;						// width of multi deposition/cut in mm
    lptr->wall_pitch=0.0;						// pitch between multi nieghboring passes
    lptr->move_lift=0.0;						// height to lift tool tip during pen-up moves
    lptr->eol_delay=0.0;						// forced delay in seconds at the end of each vector
    lptr->retract=0.0;							// distance to retract plunger at start of pen up
    lptr->advance=0.0;							// distance to advance plunger at start of pen down
    lptr->touchdepth=0.0;						// distance to push into model/substrate at start of pen down
    lptr->minveclen=0.25;						// minimum acceptable vector length
    lptr->height_mod=0.0;						// z direction offset from standard layer height
    lptr->air_cool=FALSE;						// no cooling air requested
    lptr->poly_start_delay=0;						// mili second delay to allow pressure to build before a new polygon
    lptr->thickness=0.25;						// the total width of the resulting contour OR the size spacing for fills
    memset(lptr->pattern,0,sizeof(lptr->pattern));			// type of pattern to use
    lptr->p_angle=0.0;							// angle (deg) in XY plane to draw pattern
    lptr->next=NULL;
    
    return(lptr);
  }
	
// Function to insert new linetype definition onto the end of the linetype linked list
int linetype_insert(int slot, linetype *ltnew)
{
    linetype	*lptr;
    
    if(ltnew==NULL)return(0);
    if(slot<0 || slot>=MAX_TOOLS)return(0);
    if(Tool[slot].state<TL_UNKNOWN)return(0);				// if tool doesn't have matl defined yet... exit
    
    if(Tool[slot].matl.lt==NULL)					// if this is the first element in the list...
      {
      Tool[slot].matl.lt=ltnew;						// ... define as such.
      }
    else 								// if not first element in list...
      {
      lptr=Tool[slot].matl.lt;						// ... start at front of list
      while(lptr->next!=NULL)lptr=lptr->next;				// ... and loop till last element is found
      lptr->next=ltnew;							// ... make next of last element the new element
      }
    ltnew->next=NULL;							// make sure the list is terminated
    Tool[slot].matl.max_line_types++;					// increment the number of linetypes loaded
    
    return(1);
}

// Function to delete the linetype linked list from memory
int linetype_delete(int slot)
{
    linetype *lptr,*ldel;
    
    if(Tool[slot].state<TL_LOADED)return(0);				// if tool does not yet have matl defined... exit
    lptr=Tool[slot].matl.lt;						// start at first element in list
    while(lptr!=NULL)							// loop thru entire list
      {
      ldel=lptr;							// save address of element to delete
      lptr=lptr->next;							// move onto next element
      free(ldel);							// delete element
      }
    Tool[slot].matl.lt=NULL;						// make sure inbound pointer is nulled
    Tool[slot].matl.max_line_types=0;					// zero out the number of linetypes loaded
    
    return(1);
}

// Function to make a copy of a line type
linetype *linetype_copy(linetype *lt_inpt)
  {
  linetype 	*lt_copy;
  
  // validate input
  if(lt_inpt==NULL)return(NULL);

  lt_copy=(linetype *)malloc(sizeof(linetype));
  if(lt_copy==NULL)return(NULL);
  
  lt_copy->ID=lt_inpt->ID;						// unique line type ID. See #defs for PERIM, OFFSET, FILL, SUPPORT, TRACE, DRILL
  strncpy(lt_copy->name,lt_inpt->name,sizeof(lt_copy->name)-1);		// line type name:  Perimeter, Offset, etc.
  lt_copy->tempadj=lt_inpt->tempadj;					// temperature adjustment off nominal
  lt_copy->flowrate=lt_inpt->flowrate;					// amount of material deposited per unit length drawn
  lt_copy->feedrate=lt_inpt->feedrate;					// speed at which a tool is moved while drawing/depositing/cutting
  lt_copy->line_width=lt_inpt->line_width;				// width of deposition/cut in mm
  lt_copy->line_pitch=lt_inpt->line_pitch;				// pitch between nieghboring passes
  lt_copy->wall_width=lt_inpt->wall_width;				// width of deposition/cut in mm
  lt_copy->wall_pitch=lt_inpt->wall_pitch;				// pitch between nieghboring passes
  lt_copy->move_lift=lt_inpt->move_lift;				// height to lift tool tip during pen-up moves
  lt_copy->eol_delay=lt_inpt->eol_delay;				// forced delay in seconds at the end of each vector
  lt_copy->retract=lt_inpt->retract;					// distance to retract plunger at start of pen up
  lt_copy->advance=lt_inpt->advance;					// distance to advance plunger at start of pen down
  lt_copy->touchdepth=lt_inpt->touchdepth;				// distance to push into model/substrate at start of pen down
  lt_copy->minveclen=lt_inpt->minveclen;				// minimum acceptable vector length
  lt_copy->height_mod=lt_inpt->height_mod;				// z direction offset from standard layer height
  lt_copy->air_cool=lt_inpt->air_cool;
  lt_copy->poly_start_delay=lt_inpt->poly_start_delay;			// mili second delay to allow pressure to build before a new polygon
  lt_copy->thickness=lt_inpt->thickness;				// the total width of the resulting contour OR the size spacing for fills
  strncpy(lt_copy->pattern,lt_inpt->pattern,sizeof(lt_copy->pattern)-1);// type of pattern to use
  lt_copy->p_angle=lt_inpt->p_angle;					// angle (deg) in XY plane to draw pattern
  lt_copy->next=NULL;
  
  return(lt_copy);
}

// Function to return line type pointer given a tool and name
linetype *linetype_find(int slot, int ltdef)
{
    linetype	*lptr;
    
    if(slot<0 || slot>=MAX_TOOLS)return(NULL);
    lptr=Tool[slot].matl.lt;
    while(lptr!=NULL)
      {
      if(lptr->ID==ltdef)break;
      lptr=lptr->next;
      }
    
    return(lptr);
}

// Function to accrue the total distance a linetype travels
float linetype_accrue_distance(slice *sptr, int ptyp)
{
  float 	ldist=0.0;
  float 	x0,y0,x1,y1,dx,dy,dist;
  polygon 	*pptr;
  vertex 	*vptr,*vnxt;
  
  if(sptr==NULL)return(-1);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(-1);

  // since we don't know exactly where we are coming from we'll initiate
  // accrual from the start of the first polygon.
  pptr=sptr->pfirst[ptyp];
  if(pptr!=NULL)
    {
    vptr=pptr->vert_first;
    if(vptr!=NULL)
      {
      x1=sptr->pfirst[ptyp]->vert_first->x;
      y1=sptr->pfirst[ptyp]->vert_first->y;
      }
    }
    
  // loop thru all vtxs of all polygons for this linetype in this slice
  pptr=sptr->pfirst[ptyp];
  while(pptr!=NULL)							// while we still have polygons to process...
    {
    // calculate distance for all moves along and between polygons of this line type
    x0=x1; y0=y1; 							// save previous tip location to x0,y0,z0
    vptr=pptr->vert_first;						// start at first vertex
    while(vptr!=NULL)							// if a valid vtx to move to...
      {
      vnxt=vptr->next;							// ... get the next vtx
      if(vnxt==NULL)break;						// ... handles "open" polygons like FILL
      x1=vnxt->x;
      y1=vnxt->y;
      dx=x1-x0;
      dy=y1-y0;
      dist=sqrt(dx*dx+dy*dy);						// ... length of vector
      ldist += dist;							// ... accumulate distance
      x0=x1;  y0=y1; 							// ... save current position as last position
      vptr=vptr->next;							// ... move onto next vertex
      if(vptr==pptr->vert_first)break;					// ... if back at first vtx we're done
      }
    pptr=pptr->next;							// move on to next polygon
    }
  
  return(ldist);
}



// Functions to manage TOOL operation linked lists
// This function allocates space for a single link list element of operation
// Note that these are NOT MODEL operation linked lists.  They are referenced by them.
operation *operation_make(void)
  {
  operation 	*optr;

    optr=(operation *)malloc(sizeof(operation));
    if(optr==NULL)return(NULL);
    optr->ID=0;								// unique operation type ID. See #defs for OP_ADD_MODEL_MATERIAL, OP_ADD_SUPPORT_MATERIAL, etc.
    memset(optr->name,0,sizeof(optr->name));				// line type name:  Perimeter, Offset, etc.
    optr->active=FALSE;							// flag regarding if in current use 
    optr->geo_type=(-1);						// flag regarding type of model geometery (stl, sliced, etc.)
    optr->lt_seq=NULL;							// pointer to line type sequence read from tool.xml
    optr->next=NULL;							// pointer to next operation in list
    
    return(optr);
  }
	
// Function to insert new operation definition onto the end of the available operations linked list of a tool
int operation_insert(int slot, operation *onew)
{
    
    if(onew==NULL)return(0);
    if(slot<0 || slot>=MAX_TOOLS)return(0);
    
    if(Tool[slot].oper_first==NULL)					// if this is the first element in the list...
      {
      Tool[slot].oper_first=onew;					// ... define as such.
      Tool[slot].oper_last=onew;
      }
    else 								// if not first element in list...
      {
      Tool[slot].oper_last->next=onew;
      Tool[slot].oper_last=onew;
      }
    onew->next=NULL;							// make sure the list is terminated
    Tool[slot].oper_qty++;
    
    return(1);
}

// Function to delete the operation linked list from tool memory
int operation_delete(int slot, operation *odel)
{
    operation *optr,*opold;
    
    if(odel==NULL)return(FALSE);
    if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
    if(Tool[slot].oper_first==NULL)return(FALSE);
    
    if(odel==Tool[slot].oper_first)					// if the target is the first element in the list...
      {
      Tool[slot].oper_first=odel->next;					// ... redefine the head node
      free(odel);							// ... release the target element
      Tool[slot].oper_qty--;
      }
    else 
      {
      opold=NULL;
      optr=Tool[slot].oper_first;					// start at first element in list
      while(optr!=NULL)							// loop thru entire list
	{
	if(optr==odel)break;						// if this matches our target...
	opold=optr;							// save address of element prior to target
	optr=optr->next;						// get next element in list
	}
      if(optr!=NULL)							// if the target was found...
	{
	if(opold!=NULL)
	  {
	  if(optr==Tool[slot].oper_last)Tool[slot].oper_last=opold;
	  opold->next=odel->next;					// redefine link 
	  }
	free(odel);
	Tool[slot].oper_qty--;
	}
      }
    return(TRUE);
}

// Function to delete the entire operation linked list from tool memory
int operation_delete_all(int slot)
{
    operation *optr,*odel;
    
    if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
    if(Tool[slot].oper_first==NULL)return(FALSE);
    
    optr=Tool[slot].oper_first;						// start at first element in list
    while(optr!=NULL)							// loop thru entire list
      {
      odel=optr;
      optr=optr->next;							// get next element in list
      free(odel);
      }
    Tool[slot].oper_first=NULL;
    Tool[slot].oper_last=NULL;
    Tool[slot].oper_qty=0;
    
    return(TRUE);
}

// Function to return operation pointer given its slot and name
operation *operation_find_by_name(int slot, char *op_name)
{
    operation	*optr;
    
    optr=Tool[slot].oper_first;
    while(optr!=NULL)
	{
	if(strstr(optr->name,op_name)!=NULL)break;
	optr=optr->next;
	}
    
    return(optr);
}

// Function to return operation pointer given its slot and operation ID
operation *operation_find_by_ID(int slot, int op_ID)
{
    operation	*optr;
    
    optr=Tool[slot].oper_first;
    while(optr!=NULL)
	{
	if(optr->ID==op_ID)break;
	optr=optr->next;
	}
    
    return(optr);
}


// Function to scan tools subdirectory and build linked list of available tools by file name (which is also tool name).
// Names can be anything the user conjures up as long as they are unique (EXTRUDER, FDM1, FMD_BOB, ROUTER, etc.).
int build_tool_index(void)
{
  char 		*valstart,tool_name[255];
  int		i,tool_count=0;
  genericlist	*tptr;
  DIR 		*dir;							// location of tool definition files
  struct 	dirent *dir_entry;					// scratch directory data
  char 		path[] = "/home/aa/Documents/4X3D/Tools";
 
  // First purge old tool linked list if one exists
  if(tool_list!=NULL)
    {
    genericlist_delete_all(tool_list);
    tool_list=NULL;
    }
    
  // Attempt opening tool device directory and get/count what's there
  printf("\n1Tool Devices:\n");
  dir=opendir(path);
  if(dir!=NULL)								// if the subdirectory exists...
    {
    while((dir_entry=readdir(dir))!=NULL)				// get the next file off the directory contents
      {
      // if the file is a tool definition file...
      if(strstr(dir_entry->d_name,".xml")!=NULL || strstr(dir_entry->d_name,".XML")!=NULL)
        {
	// at this point a new tool has been found, so add it to the linked list of available tools
	tool_count++;
	tptr=genericlist_make();					// make a new element
	strncpy(tool_name,dir_entry->d_name,sizeof(tool_name)-1);
	
	// convert tool name to upper case only
	for(i=0;i<strlen(tool_name);i++){if(tool_name[i]>='a' && tool_name[i]<='z')tool_name[i] -= 32;}
	
	valstart=strstr(tool_name,".");					// find start of file extension
	valstart[0]=0;							// terminate extensions
	sprintf(tptr->name,"%s",tool_name);				// define its name data
	tptr->ID=tool_count;						// define its ID data
	if(tool_list==NULL){tool_list=tptr;}				// if first element, define as head node
	else {genericlist_insert(tool_list,tptr);}			// otherwise add it into the existing list
	}
      }
 
    (void)closedir(dir);
    }
  else
    {
    printf("XML - Tool Index: Unable to open directory \"%s\"! \n",path);
    return(0);
    }
      
  // DEBUG - dump tools index
  printf("\nbuild_tool_index: \n");
  tptr=tool_list;
  while(tptr!=NULL)
    {
    printf("  %d  %s \n",tptr->ID,tptr->name);
    tptr=tptr->next;
    }
  printf("Exiting build_tool_index...\n");

  return(1);
}

// Function to read tool definition (*.tdef) file
// Note that the tool name is also the file name less the XML extension.
// the result of this function is to load parameters for a specific tool (by name) into memory
int XMLRead_Tool(int slot, char *tool_name)
{
  char *valstart,*valend,tool_class[64];
  char	full_tool_name[255],full_oper_name[255],new_oper[255];
  char	line_type_seq[255],*token;
  int	h,i,oper_ctr=0;
  int	tool_found=FALSE, oper_found=FALSE, rd_oper=FALSE, copy_flag=FALSE;
  operation	*optr;
  genericlist	*gptr;
	
  strncpy(full_tool_name,"/home/aa/Documents/4X3D/Tools/",sizeof(full_tool_name)-1);	// define path
  strcat(full_tool_name,tool_name);							// define file to open
  strcat(full_tool_name,".XML");							// define extension
  Tools_fd=fopen(full_tool_name,"r");							// attempt to open
  if(Tools_fd==NULL){printf("\n\nXMLRead_Tool: Tool file %s not found.\n\n",full_tool_name); return(0);}

  // build full tool/matl names with end marker so sub-set names aren't confused
  //printf("\nXML: full tool name = %s \n",full_tool_name);
  sprintf(full_tool_name,"%s>",Tool[slot].name);
  optr=NULL;
  
  // clear the existing tool's oper list if it exists
  if(Tool[slot].oper_first!=NULL)operation_delete_all(slot);
  
  while(fgets(scratch,sizeof(scratch),Tools_fd)!=NULL)
    {
    valstart=strstr(scratch,full_tool_name);
    if(valstart!=NULL)
      {
      if(tool_found==TRUE)break;					// if already found header, then this must be footer
      tool_found=TRUE;							// set flag to read data between header and footer
      continue;
      }

    if(tool_found==TRUE)
      {
      //printf(" *%s",scratch);
      valstart=strstr(scratch,"TOOL_ID>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	strncpy(tool_class,valstart+8,sizeof(tool_class)-1);
	if(strstr(tool_class,"EXTRUDER")!=NULL)Tool[slot].tool_ID=TC_EXTRUDER;
	if(strstr(tool_class,"FDM")!=NULL)Tool[slot].tool_ID=TC_FDM;
	if(strstr(tool_class,"ROUTER")!=NULL)Tool[slot].tool_ID=TC_ROUTER;
	if(strstr(tool_class,"LASER")!=NULL)Tool[slot].tool_ID=TC_LASER;
	if(strstr(tool_class,"LAMP")!=NULL)Tool[slot].tool_ID=TC_LAMP;
	if(strstr(tool_class,"PROBE")!=NULL)Tool[slot].tool_ID=TC_PROBE;
	if(strstr(tool_class,"PLACE")!=NULL)Tool[slot].tool_ID=TC_PLACE;
	if(strstr(tool_class,"PEN")!=NULL)Tool[slot].tool_ID=TC_PEN;
	if(strstr(tool_class,"CAMERA")!=NULL)Tool[slot].tool_ID=TC_CAMERA;
	continue;
	}
      valstart=strstr(scratch,"CLASS>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	strncpy(tool_class,valstart+6,sizeof(tool_class)-1);
	if(strstr(tool_class,"EXTRUDER")!=NULL)Tool[slot].tool_ID=TC_EXTRUDER;
	if(strstr(tool_class,"FDM")!=NULL)Tool[slot].tool_ID=TC_FDM;
	if(strstr(tool_class,"ROUTER")!=NULL)Tool[slot].tool_ID=TC_ROUTER;
	if(strstr(tool_class,"LASER")!=NULL)Tool[slot].tool_ID=TC_LASER;
	if(strstr(tool_class,"LAMP")!=NULL)Tool[slot].tool_ID=TC_LAMP;
	if(strstr(tool_class,"PROBE")!=NULL)Tool[slot].tool_ID=TC_PROBE;
	if(strstr(tool_class,"PLACE")!=NULL)Tool[slot].tool_ID=TC_PLACE;
	if(strstr(tool_class,"PEN")!=NULL)Tool[slot].tool_ID=TC_PEN;
	if(strstr(tool_class,"CAMERA")!=NULL)Tool[slot].tool_ID=TC_CAMERA;
	continue;
	}
      valstart=strstr(scratch,"DESC>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	sprintf(Tool[slot].desc,valstart+5);
	continue;
	}
      valstart=strstr(scratch,"MANF>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	sprintf(Tool[slot].manuf,valstart+5);
	continue;
	}
      valstart=strstr(scratch,"TYPE>");
      if(valstart!=NULL)
	{
	Tool[slot].type=UNDEFINED;
	if(strstr(scratch,"ADDITIVE"))Tool[slot].type=ADDITIVE;
	if(strstr(scratch,"SUBTRACTIVE"))Tool[slot].type=SUBTRACTIVE;
	if(strstr(scratch,"MEASUREMENT"))Tool[slot].type=MEASUREMENT;
	if(strstr(scratch,"MARKING"))Tool[slot].type=MARKING;
	if(strstr(scratch,"CURING"))Tool[slot].type=CURING;
	if(strstr(scratch,"PLACEMENT"))Tool[slot].type=PLACEMENT;
	continue;
	}
      valstart=strstr(scratch,"PWR48V>");
      if(valstart!=NULL)
	{
	Tool[slot].pwr48=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].pwr48=TRUE;
	continue;
	}
      valstart=strstr(scratch,"PWR24V>");
      if(valstart!=NULL)
	{
	Tool[slot].pwr24=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].pwr24=TRUE;
	continue;
	}
      valstart=strstr(scratch,"PWM_AT_TOOL>");
      if(valstart!=NULL)
	{
	Tool[slot].pwmtool=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].pwmtool=TRUE;
	continue;
	}
      valstart=strstr(scratch,"TEMPSENSE>");
      if(valstart!=NULL)
	{
	Tool[slot].thrm.sensor=NONE;
	if(strstr(scratch,"1WIRE")!=NULL)Tool[slot].thrm.sensor=ONEWIRE;
	if(strstr(scratch,"RTDH")!=NULL)Tool[slot].thrm.sensor=RTDH;
	if(strstr(scratch,"RTDT")!=NULL)Tool[slot].thrm.sensor=RTDT;
	if(strstr(scratch,"THERMISTOR")!=NULL)Tool[slot].thrm.sensor=THERMISTER;
	if(strstr(scratch,"THERMOCOUPLE")!=NULL)Tool[slot].thrm.sensor=THERMOCOUPLE;
	continue;
	}
      valstart=strstr(scratch,"MAX_TEMP>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.maxtC=atof(valstart+9);
	continue;
	}
      valstart=strstr(scratch,"SS_TEMP_ERR>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.errtC=atof(valstart+12);
	continue;
	}
      valstart=strstr(scratch,"TIME_PER_C>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.duration_per_C=atof(valstart+11);
	continue;
	}
      valstart=strstr(scratch,"KP>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.Kp=atof(valstart+3);
	continue;
	}
      valstart=strstr(scratch,"KD>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.Kd=atof(valstart+3);
	continue;
	}
      valstart=strstr(scratch,"KI>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].thrm.Ki=atof(valstart+3);
	continue;
	}
      valstart=strstr(scratch,"STEPPER>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].step_max=atoi(valstart+8);
	continue;
	}
      valstart=strstr(scratch,"STPR_DV_RATIO>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].stepper_drive_ratio=atof(valstart+14);
	continue;
	}
      valstart=strstr(scratch,"SPIBUS>");
      if(valstart!=NULL)
	{
	Tool[slot].spibus=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].spibus=TRUE;
	continue;
	}
      valstart=strstr(scratch,"1WIRE>");
      if(valstart!=NULL)
	{
	Tool[slot].wire1=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].wire1=TRUE;
	continue;
	}
      valstart=strstr(scratch,"LIMITSW>");
      if(valstart!=NULL)
	{
	Tool[slot].limitsw=FALSE;
	if(strstr(scratch,"TRUE")!=NULL)Tool[slot].limitsw=TRUE;
	continue;
	}
      valstart=strstr(scratch,"X_OFFSET>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].x_offset=atof(valstart+9);
	continue;
	}
      valstart=strstr(scratch,"Y_OFFSET>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].y_offset=atof(valstart+9);
	continue;
	}
      valstart=strstr(scratch,"TIP_DN_POS>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].tip_dn_pos=atof(valstart+11);			// nominal dist from slot home where deposition takes place
	continue;
	}
      valstart=strstr(scratch,"WEIGHT>");
      if(valstart!=NULL)
	{
	valend=strstr(valstart,"<");
	valend[0]=0;
	Tool[slot].weight=atof(valstart+7);
	continue;
	}
      valstart=strstr(scratch,"OPERATIONS>");
      if(valstart!=NULL)
	{
	if(oper_found==TRUE){oper_found=FALSE;rd_oper=FALSE;}		// if 2nd time found, must be end of operations block
	else {oper_found=TRUE;rd_oper=TRUE;}				// if 1st time found, must be start of operations block
	continue;
	}
      if(oper_found==TRUE)
        {
	//if(optr!=NULL)printf(" *  %s \n",optr->name);
        // everything within the following IF simply reads the name of the new operation then creates it
        if(rd_oper==TRUE)						// if flag set to read new operation type
	  {
	  h=0;
	  copy_flag=FALSE;
	  for(i=0;i<strlen(scratch);i++)
	    {
	    if(scratch[i]==60)						// "<"
	      {copy_flag=TRUE;continue;}
	    if(scratch[i]==62)						// ">"
	      {copy_flag=FALSE;continue;}
	    if(copy_flag==TRUE)
	      {
	      new_oper[h]=scratch[i];
	      h++;
	      if(h>254)h=254;
	      }
	    }
	  new_oper[h]=0;						// terminate new name string
	  if(h>0)							// if at least one char was read, call it a tool name
	    {
	    optr=operation_make();					// create our new operation
	    sprintf(optr->name,"%s",new_oper);				// copy over the new name
	    if(strstr(optr->name,"NONE"))optr->ID=OP_NONE;
	    if(strstr(optr->name,"ADD_MODEL_MATERIAL"))optr->ID=OP_ADD_MODEL_MATERIAL;
	    if(strstr(optr->name,"ADD_SUPPORT_MATERIAL"))optr->ID=OP_ADD_SUPPORT_MATERIAL;
	    if(strstr(optr->name,"ADD_BASE_LAYER"))optr->ID=OP_ADD_BASE_LAYER;
	    if(strstr(optr->name,"MILL_OUTLINE"))optr->ID=OP_MILL_OUTLINE;
	    if(strstr(optr->name,"MILL_AREA"))optr->ID=OP_MILL_AREA;
	    if(strstr(optr->name,"MILL_PROFILE"))optr->ID=OP_MILL_PROFILE;
	    if(strstr(optr->name,"MILL_HOLES"))optr->ID=OP_MILL_HOLES;
	    if(strstr(optr->name,"MEASURE_X"))optr->ID=OP_MEASURE_X;
	    if(strstr(optr->name,"MEASURE_Y"))optr->ID=OP_MEASURE_Y;
	    if(strstr(optr->name,"MEASURE_Z"))optr->ID=OP_MEASURE_Z;
	    if(strstr(optr->name,"MEASURE_HOLES"))optr->ID=OP_MEASURE_HOLES;
	    if(strstr(optr->name,"MARK_OUTLINE"))optr->ID=OP_MARK_OUTLINE;
	    if(strstr(optr->name,"MARK_AREA"))optr->ID=OP_MARK_AREA;
	    if(strstr(optr->name,"MARK_IMAGE"))optr->ID=OP_MARK_IMAGE;
	    if(strstr(optr->name,"MARK_CUT"))optr->ID=OP_MARK_CUT;
	    if(strstr(optr->name,"CURE"))optr->ID=OP_CURE;
	    if(strstr(optr->name,"PLACE"))optr->ID=OP_PLACE;
	    optr->type=Tool[slot].type;
	    operation_insert(slot,optr);
	    sprintf(scratch,"</%s>",new_oper);				// build block termination string
	    sprintf(new_oper,"%s",scratch);
	    }
	  rd_oper=FALSE;						// set flag to indicate tool name reading is done
	  continue;							// move onto next line in file
	  }
	    
	valstart=strstr(scratch,"GEOMETRY>");
	if(valstart!=NULL && optr!=NULL)
	  {
	  optr->geo_type=0;
	  if(strstr(scratch,"NONE")!=NULL)optr->geo_type=0;
	  if(strstr(scratch,"SLICE")!=NULL)optr->geo_type=1;
	  if(strstr(scratch,"SURFACE")!=NULL)optr->geo_type=2;
	  if(strstr(scratch,"VOXEL")!=NULL)optr->geo_type=3;
	  continue;
	  }
	  
	valstart=strstr(scratch,"SEQUENCE>");
	if(valstart!=NULL && optr!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[1]=0;
	  sprintf(line_type_seq,"%s",valstart+9);			// build search string
	  if(strstr(line_type_seq,",")!=NULL)				// if more than one in sequence...
	    {
	    h=strlen(line_type_seq);					// ... get length
	    line_type_seq[h-1]=0;					// ... and remove "<" char from end
	    token=strtok(line_type_seq,",");				// ... now get first token
	    }
	  else 
	    {token=strtok(line_type_seq,"<");}				// if only one in sequence ... get only token
	  while(token!=NULL)
	    {
	    //printf(" * next\n");
	    gptr=genericlist_make();
	    if(gptr==NULL)break;
	    strncpy(gptr->name,token,sizeof(gptr->name)-1);
	    //printf(" * %s\n",token);
	    
	    // this should change later, but quick way to patch code together for test
	    // the right way would be to include a few params in the lt fields of the XML file
	    // that describe what the lt does.  i.e. perimeter, fill, close-off, etc.
	    gptr->ID=UNDEFINED;
	    if(strstr(gptr->name,"MDL_PERIM")!=NULL)gptr->ID=MDL_PERIM;
	    if(strstr(gptr->name,"MDL_BORDER")!=NULL)gptr->ID=MDL_BORDER;
	    if(strstr(gptr->name,"MDL_OFFSET")!=NULL)gptr->ID=MDL_OFFSET;
	    if(strstr(gptr->name,"MDL_FILL")!=NULL)gptr->ID=MDL_FILL;
	    if(strstr(gptr->name,"MDL_LOWER_CO")!=NULL)gptr->ID=MDL_LOWER_CO;
	    if(strstr(gptr->name,"MDL_UPPER_CO")!=NULL)gptr->ID=MDL_UPPER_CO;
	    if(strstr(gptr->name,"MDL_LAYER_1")!=NULL)gptr->ID=MDL_LAYER_1;
	    if(strstr(gptr->name,"INT_PERIM")!=NULL)gptr->ID=INT_PERIM;
	    if(strstr(gptr->name,"INT_BORDER")!=NULL)gptr->ID=INT_BORDER;
	    if(strstr(gptr->name,"INT_OFFSET")!=NULL)gptr->ID=INT_OFFSET;
	    if(strstr(gptr->name,"INT_FILL")!=NULL)gptr->ID=INT_FILL;
	    if(strstr(gptr->name,"INT_LOWER_CO")!=NULL)gptr->ID=INT_LOWER_CO;
	    if(strstr(gptr->name,"INT_UPPER_CO")!=NULL)gptr->ID=INT_UPPER_CO;
	    if(strstr(gptr->name,"INT_LAYER_1")!=NULL)gptr->ID=INT_LAYER_1;
	    if(strstr(gptr->name,"BASELYR")!=NULL)gptr->ID=BASELYR;
	    if(strstr(gptr->name,"PLATFORMLYR1")!=NULL)gptr->ID=PLATFORMLYR1;
	    if(strstr(gptr->name,"PLATFORMLYR2")!=NULL)gptr->ID=PLATFORMLYR2;
	    if(strstr(gptr->name,"SPT_PERIM")!=NULL)gptr->ID=SPT_PERIM;
	    if(strstr(gptr->name,"SPT_BORDER")!=NULL)gptr->ID=SPT_BORDER;
	    if(strstr(gptr->name,"SPT_OFFSET")!=NULL)gptr->ID=SPT_OFFSET;
	    if(strstr(gptr->name,"SPT_FILL")!=NULL)gptr->ID=SPT_FILL;
	    if(strstr(gptr->name,"SPT_LOWER_CO")!=NULL)gptr->ID=SPT_LOWER_CO;
	    if(strstr(gptr->name,"SPT_UPPER_CO")!=NULL)gptr->ID=SPT_UPPER_CO;
	    if(strstr(gptr->name,"SPT_LAYER_1")!=NULL)gptr->ID=SPT_LAYER_1;
	    if(strstr(gptr->name,"TRACE")!=NULL)gptr->ID=TRACE;
	    if(strstr(gptr->name,"DRILL")!=NULL)gptr->ID=DRILL;
	    if(optr->lt_seq==NULL)
	      {
	      optr->lt_seq=gptr;
	      set_view_lt[gptr->ID]=TRUE;
	      //printf("  * adding first lt %s to operation seq %X \n",gptr->name,optr->lt_seq); 
	      }
	    else 
	      {
	      genericlist_insert(optr->lt_seq,gptr);
	      set_view_lt[gptr->ID]=TRUE;
	      //printf("  * adding next lt %s to operation seq %X \n",gptr->name,optr->lt_seq); 
	      }
	    token=strtok(NULL,",");					// get next type in list
	    //printf(" * done\n");
	    }
	  continue;
	  }
	  
	if(strstr(scratch,new_oper)!=NULL)				// if the terminator for this block
	  {
	  optr=NULL;							// stop all associations with the current operation
	  rd_oper=TRUE;							// set to read the next new operation name
	  continue;
	  }
	}	// end of oper_found if
      }		// end of tool_found if
    }		// end of fgets loop

  if(ferror(Tools_fd))
    {
    printf("/nError - unable to read tool operation file!\n");
    }
  fclose(Tools_fd);
  
  // finally build the tool's oper list based on the operations found
  set_view_lt_override=FALSE;						// view lt sequence matches tool's op sequence
  if(Tool[slot].oper_list==NULL)
    {
    optr=Tool[slot].oper_first;
    while(optr!=NULL)
      {
      gptr=genericlist_make();
      gptr->ID=optr->ID;						// add the operation ID number
      strncpy(gptr->name,optr->name,sizeof(gptr->name)-1);		// add the operation pnuemonic
      if(Tool[slot].oper_list==NULL)
        {Tool[slot].oper_list=gptr;}
      else 
        {genericlist_insert(Tool[slot].oper_list,gptr);}
      optr=optr->next;
      }
    }
  
  // DEBUG
  printf("\n\nXML Tool Params:  slot=%d  tool=%s  oper qty=%d \n",slot,Tool[slot].name,Tool[slot].oper_qty);
  optr=Tool[slot].oper_first;
  while(optr!=NULL)
    {
    printf("  %d  %s \n",optr->ID,optr->name);
    gptr=optr->lt_seq;
    while(gptr!=NULL)
      {
      printf("    LT: %d  %s \n",gptr->ID,gptr->name);
      gptr=gptr->next;
      }
    optr=optr->next;
    }
  printf("\n\n");
    
  return(1);	
}


// Function to scan the Materials subdirectory and build a linked list of available materials
// for the specific tool that exists in "slot".  Recall that within each material file, params
// for each tool are listed.  If that tool is not listed, it does not get put into this index.
int build_material_index(int slot)
{
  char		*valstart;
  char		full_tool_name[255],full_matl_name[255];
  int		matl_count=0;
  genericlist	*mlptr;
  DIR 		*dir;							// location of tool definition files
  struct 	dirent *dir_entry;					// scratch directory data
  char 		path[] = "/home/aa/Documents/4X3D/Materials";
 
  // First purge old list if one exists
  if(Tool[slot].mats_list!=NULL)
    {
    genericlist_delete_all(Tool[slot].mats_list);
    Tool[slot].mats_list=NULL;
    }

  // build tool name
  strncpy(full_tool_name,Tool[slot].name,sizeof(full_tool_name)-1);

  // Attempt opening materials directory and get/count what's there
  printf("\n\nMaterial Databases:\n");
  dir=opendir(path);
  if(dir!=NULL)								// if the subdirectory exists...
    {
    while((dir_entry=readdir(dir))!=NULL)				// get the next file off the directory contents
      {
      // if the file is a material databas file...
      if(strstr(dir_entry->d_name,".xml")!=NULL || strstr(dir_entry->d_name,".XML")!=NULL)
        {
	// at this point a new matl has been found, so first check if it can be used with
	// the tool located in "slot".  If so, add it to the linked list of available matls for this tool.
	
	// build clean mateial name from file name
	strncpy(full_matl_name,dir_entry->d_name,sizeof(full_matl_name)-1);
	valstart=strstr(full_matl_name,".");				// find start of file extension
	valstart[0]=0;							// terminate extensions

	// open the material file and scan for the tool
	if(XMLRead_Matl_find_Tool(full_tool_name,full_matl_name)==TRUE)	// if this matl supports this tool...
	  {
	  matl_count++;							// increment count of matls.  note: UNKNOWN=0
	  mlptr=genericlist_make();					// make a new element
	  sprintf(mlptr->name,"%s",full_matl_name);			// define its name data
	  mlptr->ID=matl_count;						// define its ID data.  available matls start at 1
	  if(Tool[slot].mats_list==NULL){Tool[slot].mats_list=mlptr;}	// if first element, define as head node
	  else {genericlist_insert(Tool[slot].mats_list,mlptr);}	// otherwise add it into the existing list
	  }
	}
      }
 
    (void)closedir(dir);
    }
  else
    {
    printf("XML - Material Index: Unable to open directory \"%s\"! \n",path);
    return(0);
    }
  
  // dump materials index
  printf("\n\nBuild_material_index:  slot=%d \n",slot);
  mlptr=Tool[slot].mats_list;
  while(mlptr!=NULL)
    {
    printf("  %d  %s \n",mlptr->ID,mlptr->name);
    mlptr=mlptr->next;
    }
  printf("Exiting build_material_index...\n\n");
  
  return(1);
}



// Function to read matl XML file to see if it can be used with a specific tool.
// note this XML data file is ordered by:  MATERIAL->TOOL->LINETYPE
int XMLRead_Matl_find_Tool(char *find_tool, char *find_matl)
  {
  char	full_tool_name[255],full_matl_name[255];
  int 	i,success=FALSE;
	
  // build matl file name and open it
  strncpy(full_matl_name,"/home/aa/Documents/4X3D/Materials/",sizeof(full_matl_name)-1);// define path
  strcat(full_matl_name,find_matl);							// define file to open
  strcat(full_matl_name,".XML");							// define extension
  Mats_fd=fopen(full_matl_name,"r");							// attempt to open
  if(Mats_fd==NULL)return(FALSE);
  
  // build full tool name with end marker so sub-set names aren't confused
  sprintf(full_tool_name,"%s>",find_tool);
  
  // convert tool name to upper case only
  for(i=0;i<strlen(find_tool);i++){if(find_tool[i]>='a' && find_tool[i]<='z')find_tool[i] -= 32;}
  
  // search thru file looking for tool name
  while(fgets(scratch,sizeof(scratch),Mats_fd)!=NULL)
    {
    // convert scratch to upper case only
    for(i=0;i<strlen(scratch);i++){if(scratch[i]>='a' && scratch[i]<='z')scratch[i] -= 32;}
    if(strstr(scratch,full_tool_name)!=NULL)success=TRUE;
    }

  if(ferror(Mats_fd))
    {
    printf("/nError - unable to read material parameter file!\n");
    }
  fclose(Mats_fd);
  
  if(success==TRUE) printf("  searching %s for tool %s ... FOUND\n",find_matl,find_tool);
  if(success==FALSE)printf("  searching %s for tool %s ...\n",find_matl,find_tool);

  return(success);	
  }



// Function to read  matl XML file based on tool loaded in unit and material name requested
// the result of this function is to load a tool with specific material (by name) parameters.
// note this XML data file is ordered by:  MATERIAL->TOOL->LINETYPE
int XMLRead_Matl(int slot, char *find_matl)
  {
  char 	*mat_flag,*gage_flag,*matID_flag,*lt_flag;
  char 	*valstart,*valend,*token,input_string[255];
  char	full_tool_name[255],full_matl_name[255];
  char	new_linetype_name[255],line_type_seq[255];
  int 	success=FALSE;
  int 	temp_gage=0;
  int 	linetype_found=FALSE,material_found=FALSE,tool_found=FALSE;
  int 	lt_ID=0,copy_flag=0;
  int 	i,h,matl_count=0;
  linetype	*lptr;
	
	
  // convert matl name to upper case only
  for(i=0;i<strlen(find_matl);i++){if(find_matl[i]>='a' && find_matl[i]<='z')find_matl[i] -= 32;}

  strncpy(full_matl_name,"/home/aa/Documents/4X3D/Materials/",sizeof(full_matl_name)-1);// define path
  strcat(full_matl_name,find_matl);							// define file to open
  strcat(full_matl_name,".XML");							// define extension
  Mats_fd=fopen(full_matl_name,"r");							// attempt to open
  if(Mats_fd==NULL){printf("\n\nXMLRead_Matl: Matl file %s not found.\n\n",full_matl_name); return(0);}
  
  // build full tool/matl names with end marker so sub-set names aren't confused
  sprintf(full_tool_name,"%s>",Tool[slot].name);			// tool names already stored in uppercase
  sprintf(full_matl_name,"%s>",find_matl);

  printf("\nXLM Read Matl:  Tool=%s  Matl=%s \n",full_tool_name,full_matl_name);
  
  lptr=NULL;
  while(fgets(scratch,sizeof(scratch),Mats_fd)!=NULL)
    {
    // convert scratch to upper case only
    for(i=0;i<strlen(scratch);i++){if(scratch[i]>='a' && scratch[i]<='z')scratch[i] -= 32;}

    // if a material name has been found, search for correct tool support
    if(material_found==TRUE)
      {
      if(strstr(scratch,full_matl_name)==NULL)				// if end definition has NOT been hit...
        {
	// load generic material properties that are tool indpendent
	valstart=strstr(scratch,"DESCR");				// description
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[0]=0;
	  sprintf(input_string,"%s",(valstart+6));
	  strncpy(Tool[slot].matl.description,input_string,sizeof(Tool[slot].matl.description)-1);
	  continue;
	  }
	valstart=strstr(scratch,"BRAND");				// manufacturer
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[0]=0;
	  sprintf(input_string,"%s",(valstart+6));
	  strncpy(Tool[slot].matl.brand,input_string,sizeof(Tool[slot].matl.brand)-1);
	  continue;
	  }
	valstart=strstr(scratch,"COLOR");				// integer neumonic to rgba structure - see defines
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[0]=0;
	  Tool[slot].matl.color=atoi(valstart+6);
	  set_color(NULL,&mcolor[slot],Tool[slot].matl.color);
	  continue;
	  }
	valstart=strstr(scratch,"MAT_DIAM");				// diam of material supply, typ for filament
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[0]=0;
	  Tool[slot].matl.diam=atof(valstart+9);
	  continue;
	  }

	// check if the correct tool is used by this material
	if(tool_found==FALSE && strstr(scratch,full_tool_name)!=NULL)
	  {
	  tool_found=TRUE;
	  continue;
	  }
	if(tool_found==TRUE && strstr(scratch,full_tool_name)!=NULL && strstr(scratch,"/")!=NULL)
	  {
	  tool_found=FALSE;
	  //continue;
	  break;
	  }

	// at this point the right tool for this material has been found... load linetype params
	if(tool_found==TRUE)
	  {
	    // load predefined named params
	    {
	      valstart=strstr(scratch,"TIP_DIAM");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].tip_diam=atof(valstart+9);
		continue;
		}
	      valstart=strstr(scratch,"LAYER_HEIGHT");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.layer_height=atof(valstart+13);
		continue;
		}
	      valstart=strstr(scratch,"MIN_LAYER_TIME");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.min_layer_time=atoi(valstart+15);
		continue;
		}
	      valstart=strstr(scratch,"TEMPERATURE");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].thrm.setpC=atof(valstart+12);			// setpoint can vary based on line type
		Tool[slot].thrm.operC=atof(valstart+12);			// always stays at what is requested by material
		continue;
		}
	      valstart=strstr(scratch,"SETBACKTEMP");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].thrm.backC=atof(valstart+12);
		continue;
		}
	      valstart=strstr(scratch,"WAITTEMPADJ");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].thrm.waitC=atof(valstart+12);
		continue;
		}
	      valstart=strstr(scratch,"BEDTEMP");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].thrm.bedtC=atof(valstart+8);
		continue;
		}
	      valstart=strstr(scratch,"TOOL_RETRACT");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.retract=atof(valstart+13);
		continue;
		}
	      valstart=strstr(scratch,"TOOL_ADVANCE");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.advance=atof(valstart+13);
		continue;
		}
	      valstart=strstr(scratch,"OVERHANG_ANGLE");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.overhang_angle=(-1)*atof(valstart+15);
		continue;
		}
	      valstart=strstr(scratch,"SHRINK");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].matl.shrink=atof(valstart+7);
		continue;
		}
	      valstart=strstr(scratch,"DUTYCYCLE");
	      if(valstart!=NULL)
		{
		valend=strstr(valstart,"<");
		valend[0]=0;
		Tool[slot].powr.power_duty=atof(valstart+10)/100.0;
		continue;
		}
	    }

	  // at this point an "unrecognized" parameter has shown up.  this will be interpreted as
	  // a new line type name with the expectation of what's between it's start and end in the XML
	  // file are line type parameters.
	  
	  // if a line type has been found and we are within it's start/stop block
	  if(linetype_found==TRUE && lptr!=NULL)
	    {
	    valstart=strstr(scratch,"LINE_WIDTH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->line_width=atof(valstart+11);
	      lptr->fidelity_line_width=0.0010;
	      continue;
	      }
	    valstart=strstr(scratch,"LINE_PITCH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->line_pitch=atof(valstart+11);
	      continue;
	      }
	    valstart=strstr(scratch,"WALL_WIDTH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->wall_width=atof(valstart+11);
	      if(lptr->wall_width < lptr->line_width)lptr->wall_width=lptr->line_width;
	      continue;
	      }
	    valstart=strstr(scratch,"WALL_PITCH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->wall_pitch=atof(valstart+11);
	      continue;
	      }
	    valstart=strstr(scratch,"MOVE_LIFT");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->move_lift=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"EOL_DELAY");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->eol_delay=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"LT_TEMPADJ");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->tempadj=atof(valstart+11);
	      continue;
	      }
	    valstart=strstr(scratch,"FLOWRATE");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->flowrate=atof(valstart+9);
	      if(Tool[slot].tool_ID==TC_FDM || Tool[slot].tool_ID==TC_EXTRUDER)
	        {
	        lptr->flowrate*=Tool[slot].stepper_drive_ratio;		// both use steppers to push material
		}
	      if(Tool[slot].tool_ID==TC_ROUTER || Tool[slot].tool_ID==TC_LASER)
	        {
		if(lptr->flowrate>1.0)lptr->flowrate=1.0;		// both use power/PWM ratios for power
		}
	      continue;
	      }
	    valstart=strstr(scratch,"FEEDRATE");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->feedrate=atof(valstart+9);
	      continue;
	      }
	    valstart=strstr(scratch,"RETRACTION");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->retract=atof(valstart+11);
	      lptr->retract*=Tool[slot].stepper_drive_ratio;
	      continue;
	      }
	    valstart=strstr(scratch,"ADVANCE");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->advance=atof(valstart+8);
	      lptr->advance*=Tool[slot].stepper_drive_ratio;
	      continue;
	      }
	    valstart=strstr(scratch,"PRIMEPUSH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      Tool[slot].matl.primepush=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"PRIMEPULL");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      Tool[slot].matl.primepull=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"PRIMEHOLD");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      Tool[slot].matl.primehold=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"MINVECLEN");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->minveclen=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"TOUCHDEPTH");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->touchdepth=atof(valstart+11);
	      continue;
	      }
	    valstart=strstr(scratch,"HEIGHT_MOD");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->height_mod=atof(valstart+11);
	      continue;
	      }
	    valstart=strstr(scratch,"AIR_COOL");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      sprintf(input_string,"%s",(valstart+9));
	      lptr->air_cool=FALSE;
	      if(strstr(input_string,"ON")!=NULL)lptr->air_cool=TRUE;
	      if(strstr(input_string,"TRUE")!=NULL)lptr->air_cool=TRUE;
	      continue;
	      }
	    valstart=strstr(scratch,"POLY_START_DELAY");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->poly_start_delay=atoi(valstart+17);
	      continue;
	      }
	    valstart=strstr(scratch,"THICKNESS");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->thickness=atof(valstart+10);
	      continue;
	      }
	    valstart=strstr(scratch,"PATTERN");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      sprintf(lptr->pattern,"%s",valstart+8);
	      continue;
	      }
	    valstart=strstr(scratch,"P_ANGLE");
	    if(valstart!=NULL)
	      {
	      valend=strstr(valstart,"<");
	      valend[0]=0;
	      lptr->p_angle=atof(valstart+8);
	      continue;
	      }
		
	    // end of line type identified.  turn off collecting pararms.
	    valstart=strstr(scratch,lptr->name);
	    if(valstart!=NULL)
	      {
	      linetype_found=FALSE;
	      lptr=NULL;
	      continue;
	      }
	    }

	  // everything within this IF simply reads the name of the new line type
	  if(linetype_found==FALSE)					// if flag set to read new name
	    {
	    h=0;
	    copy_flag=FALSE;
	    for(i=0;i<strlen(scratch);i++)
	      {
	      if(scratch[i]==60)					// "<"
	        {copy_flag=TRUE;continue;}
	      if(scratch[i]==62)					// ">"
	        {copy_flag=FALSE;continue;}
	      if(copy_flag==TRUE)
	        {
	        new_linetype_name[h]=scratch[i];
	        h++;
	        if(h>254)h=254;
	        }
	      }
	    copy_flag=FALSE;
	    new_linetype_name[h]=0;
	    if(h>0)
	      {
	      linetype_found=TRUE;					// set flag to indicate we're now processing a linetype
	      lptr=linetype_make();					// create a memory location for the new linetype
	      linetype_insert(slot,lptr);				// add the linetype to this tools linked list of linetypes
	      strncpy(lptr->name,new_linetype_name,sizeof(lptr->name)-1);	// give this new linetype a name
	      
	      // this should change later, but quick way to patch code together for test
	      // the right way would be to include a few params in the lt fields of the XML file
	      // that describe what the lt does.  i.e. perimeter, fill, close-off, etc.
	      if(strstr(lptr->name,"MDL_PERIM")!=NULL)lptr->ID=MDL_PERIM;
	      if(strstr(lptr->name,"MDL_BORDER")!=NULL)lptr->ID=MDL_BORDER;
	      if(strstr(lptr->name,"MDL_OFFSET")!=NULL)lptr->ID=MDL_OFFSET;
	      if(strstr(lptr->name,"MDL_FILL")!=NULL)lptr->ID=MDL_FILL;
	      if(strstr(lptr->name,"MDL_LOWER_CO")!=NULL)lptr->ID=MDL_LOWER_CO;
	      if(strstr(lptr->name,"MDL_UPPER_CO")!=NULL)lptr->ID=MDL_UPPER_CO;
	      if(strstr(lptr->name,"MDL_LAYER_1")!=NULL)lptr->ID=MDL_LAYER_1;
	      if(strstr(lptr->name,"INT_PERIM")!=NULL)lptr->ID=INT_PERIM;
	      if(strstr(lptr->name,"INT_BORDER")!=NULL)lptr->ID=INT_BORDER;
	      if(strstr(lptr->name,"INT_OFFSET")!=NULL)lptr->ID=INT_OFFSET;
	      if(strstr(lptr->name,"INT_FILL")!=NULL)lptr->ID=INT_FILL;
	      if(strstr(lptr->name,"INT_LOWER_CO")!=NULL)lptr->ID=INT_LOWER_CO;
	      if(strstr(lptr->name,"INT_UPPER_CO")!=NULL)lptr->ID=INT_UPPER_CO;
	      if(strstr(lptr->name,"INT_LAYER_1")!=NULL)lptr->ID=INT_LAYER_1;
	      if(strstr(lptr->name,"BASELYR")!=NULL)lptr->ID=BASELYR;
	      if(strstr(lptr->name,"PLATFORMLYR1")!=NULL)lptr->ID=PLATFORMLYR1;
	      if(strstr(lptr->name,"PLATFORMLYR2")!=NULL)lptr->ID=PLATFORMLYR2;
	      if(strstr(lptr->name,"SPT_PERIM")!=NULL)lptr->ID=SPT_PERIM;
	      if(strstr(lptr->name,"SPT_BORDER")!=NULL)lptr->ID=SPT_BORDER;
	      if(strstr(lptr->name,"SPT_OFFSET")!=NULL)lptr->ID=SPT_OFFSET;
	      if(strstr(lptr->name,"SPT_FILL")!=NULL)lptr->ID=SPT_FILL;
	      if(strstr(lptr->name,"SPT_LOWER_CO")!=NULL)lptr->ID=SPT_LOWER_CO;
	      if(strstr(lptr->name,"SPT_UPPER_CO")!=NULL)lptr->ID=SPT_UPPER_CO;
	      if(strstr(lptr->name,"SPT_LAYER_1")!=NULL)lptr->ID=SPT_LAYER_1;
	      if(strstr(lptr->name,"TRACE")!=NULL)lptr->ID=TRACE;
	      if(strstr(lptr->name,"DRILL")!=NULL)lptr->ID=DRILL;
	      success=TRUE;
	      }
	    }

	  continue;							// move onto next line off file
	  }
	}
      else  								// end of this material has been hit and correct tool was not found
        {
	lptr=NULL;
	linetype_found=FALSE;
	material_found=FALSE;						// set flag to show we are no longer "in" a material
	tool_found=FALSE;						// set flag to show the correct tool has not been found
	continue;							// move onto next line off file
	}
      }
    else 
      {
      mat_flag=strstr(scratch,full_matl_name);
      if(mat_flag!=NULL)
	{
        if(material_found==TRUE)break;					// if 2nd time found, then must be end of this material
        material_found=TRUE;						// if 1st time found, then must be start of this material
	matl_count++;							// count the number of matls we've encountered
	}
      }
    }

  if(ferror(Mats_fd))
    {
    printf("/nError - unable to read material parameter file!\n");
    }
  fclose(Mats_fd);
  
  // define material by name and ID for reference in rest of code
  strncpy(Tool[slot].matl.name,find_matl,sizeof(Tool[slot].matl.name)-1);
      
  // set custom perim and fill global values to mat'l input file values as a starting point
  lptr=linetype_find(slot,MDL_PERIM);
  if(lptr!=NULL)
    {
    poly_perim_flow=lptr->flowrate;
    poly_perim_feed=lptr->feedrate;
    poly_perim_thick=lptr->line_width; 
    poly_perim_pitch=lptr->line_pitch;
    }
  lptr=linetype_find(slot,MDL_FILL);
  if(lptr!=NULL)
    {
    poly_fill_flow=lptr->flowrate;
    poly_fill_feed=lptr->feedrate;
    poly_fill_thick=lptr->line_width; 
    poly_fill_pitch=lptr->line_pitch;
    poly_fill_pitch=lptr->p_angle;
    }
      
  // debug dump to console
  printf("\n\nXMLRead_Matl:\n");
  printf("Tool = %d %s\n",slot,Tool[slot].name);
  printf(" Material.... = %s \n",Tool[slot].matl.name);
  printf("  Color....... = %d \n",Tool[slot].matl.color);
  printf("  ID.......... = %d \n",Tool[slot].matl.ID);
  printf("  Tip diam.... = %f \n",Tool[slot].tip_diam);
  printf("  Layer height = %f \n",Tool[slot].matl.layer_height);
  printf("  Min Lyr Time = %d \n",Tool[slot].matl.min_layer_time);
  printf("  48v power... = %d \n",Tool[slot].pwr48);
  printf("  24v power... = %d \n",Tool[slot].pwr24);
  printf("  Temp sensor. = %d \n",Tool[slot].thrm.sensor);
  printf("  Temperature. = %f \n",Tool[slot].thrm.setpC);
  printf("  SetBackTemp. = %f \n",Tool[slot].thrm.backC);
  printf("  WaitTempAdj. = %f \n",Tool[slot].thrm.waitC);
  printf("  BedTemp..... = %f \n",Tool[slot].thrm.bedtC);
  //printf("Primepush... = %f \n",Tool[slot].matl.primepush);
  //printf("Primepull... = %f \n",Tool[slot].matl.primepull);
  //printf("Primehold... = %f \n",Tool[slot].matl.primehold);
  printf("\n\n");
  
  
  lptr=Tool[slot].matl.lt;  
  while(lptr!=NULL)
    {
    printf("\nLine type... = %s \n",lptr->name);
    printf("  ID ....... = %d \n",lptr->ID);
    printf("  Temp adj.. = %7.3f \n",lptr->tempadj);
    printf("  Line width = %7.3f \n",lptr->line_width);
    printf("  Line pitch = %7.3f \n",lptr->line_pitch);
    printf("  Wall width = %7.3f \n",lptr->wall_width);
    printf("  Wall pitch = %7.3f \n",lptr->wall_pitch);
    printf("  Flowrate.. = %7.3f \n",lptr->flowrate);
    printf("  Feedrate.. = %7.3f \n",lptr->feedrate);
    printf("  Retraction = %7.3f \n",lptr->retract);
    printf("  Advance... = %7.3f \n",lptr->advance);
    printf("  Min veclen = %7.3f \n",lptr->minveclen);
    printf("  Touchdepth = %7.3f \n",lptr->touchdepth);
    printf("  Hieght mod = %7.3f \n",lptr->height_mod);
    printf("  Thickness. = %7.3f \n",lptr->thickness);
    printf("  Pattern... = %s \n",lptr->pattern);
    printf("  Pat angle. = %7.3f \n",lptr->p_angle);
    lptr=lptr->next;
    }
  

  return(success);	
  }


/*
// Function to scan OPERATIONS.XML and build linked list of available operations based on a specific tool type
// names can be anything the user conjures up as long as they are unique.  ideally they are descriptive.
// operation IDs are assigned as the sequence they are found and added to the linked list index.
int build_operation_index(int slot)
{
  int		h,i,ID_ctr;
  int		copy_flag=0;
  int		rd_type=0,oper_found=0,tool_found=0;
  char		new_oper[255],full_tool_name[255];
  genericlist	*optr,*olast;
 
  // First purge old list if one exists
  if(Tool[slot]->oper_list!=NULL)
    {
    optr=Tool[slot]->oper_list;
    while(optr!=NULL)
      {
      olast=optr;
      optr=optr->next;
      free(olast);
      }
    Tool[slot]->oper_list=NULL;
    }

  // build full tool name with end marker so sub-set tool names aren't confused
  sprintf(full_tool_name,"%s>",Tool[slot].name);

  // Now build new list
  Opers_fd=fopen("Operations.XML","r");
  if(Opers_fd==NULL)return(0);						// file not found
  ID_ctr=0;
  while(fgets(scratch,255,Opers_fd)!=NULL)				// grab line off file
    {
    if(strstr(scratch,"<OPERATION>")!=NULL)
      {
      rd_type=TRUE;							// set flag to read next line as a type of operation
      continue;								// move on to next line
      }
    if(strstr(scratch,"</OPERATION>")!=NULL)break;			// definitions are done... time to leave
    
    // check for the tool we are looking for
    if(tool_found==FALSE && strstr(scratch,full_tool_name)!=NULL)tool_found=TRUE;  
    if(tool_found==TRUE && strstr(scratch,full_tool_name)!=NULL && strstr(scratch,"/")!=NULL)tool_found=FALSE;	  

    // if the correct tool has been found
    if(tool_found==TRUE)
      {
      // if an operation name has been found, skip everything in between its start and end definitions
      if(oper_found==TRUE)
	{
	if(strstr(scratch,new_oper)!=NULL)				// if end definition has been found...
	  {
	  // at this point a new tool has been found, now it must be added to the linked list of available tools
	  optr=(genericlist *)malloc(sizeof(genericlist));		// create space for new element and define its address
	  optr->next=NULL;						// define the next element as nothing
	  strncpy(optr->name,new_oper,sizeof(optr->name)-1);		// copy over the new name into the list element
	  ID_ctr++;
	  optr->ID=ID_ctr;
	  if(Tool[slot]->oper_list==NULL)				// if the index has no elements in it yet ...
	    {Tool[slot]->oper_list=optr;}				// ... create space for first element and define its address
	  else 								// if NOT the first element in the list...
	    {olast->next=optr;}						// ... create connection to new element
	  olast=optr;							// define new element as last one in list
	  oper_found=FALSE;						// set flag to show we are no longer "in" a tool
	  rd_type=TRUE;							// set flag to read next line as a tool type
	  memset(new_oper,0,sizeof(new_oper));				// blank out stale new tool data
	  continue;							// move onto next line off file
	  }
	}
	
      // everything within this IF simply reads the name of the new operation
      if(rd_type==TRUE)							// if flag set to read tool type
	{
	h=0;
	copy_flag=FALSE;
	for(i=0;i<strlen(scratch);i++)
	  {
	  if(scratch[i]==60)						// "<"
	    {copy_flag=TRUE;continue;}
	  if(scratch[i]==62)						// ">"
	    {copy_flag=FALSE;continue;}
	  if(copy_flag==TRUE)
	    {
	    new_oper[h]=scratch[i];
	    h++;
	    if(h>254)h=254;
	    }
	  }
	rd_type=FALSE;							// set flag to indicate tool name reading is done
	new_oper[h]=0;							// terminate new name string
	if(h>0)oper_found=TRUE;						// if at least one char was read, call it a tool name
	}
      }	// end of tool_found loop
    } 	// end of while file reading loop
    
  fclose(Opers_fd);
 
  return(1);
}

// Function to read OPERATIONS.XML file parameters for a specific operation.
// Note this XML file is ordered by:  TOOL->OPERATION
int XMLRead_Oper(model *mptr, int slot, char *find_oper)
  {
  char 	*mat_flag,*gage_flag,*matID_flag,*lt_flag;
  char 	*valstart,*valend,*token;
  char	full_tool_name[255],full_oper_name[255];
  char	new_operation_name[255];
  char	line_type_seq[255];
  int 	success=0;
  int 	temp_gage=0;
  int 	oper_found=0,material_found=0,tool_found=0;
  int 	oper_ID=0,copy_flag=0;
  int 	i,h;
  
  linetype	*lptr;
  operation	*optr;
	
	
  Opers_fd=fopen("Operations.XML","r");
  if(Opers_fd==NULL)return(0);
  
  // build full tool/oper names with end marker so sub-set names aren't confused
  sprintf(full_tool_name,"%s>",Tool[slot].name);
  sprintf(full_oper_name,"%s>",find_oper);
  
  // scan thru entire file searching for operations assocaiated with this tool and
  // load those operations into a linked list called out for this model
  optr=NULL;
  while(fgets(scratch,254,Opers_fd)!=NULL)
    {
    // watch for the requested tool
    if(tool_found==FALSE && strstr(scratch,full_tool_name)!=NULL)
      {tool_found=TRUE;continue;}
    if(tool_found==TRUE && strstr(scratch,full_tool_name)!=NULL && strstr(scratch,"/")!=NULL)
      {tool_found=FALSE;continue;}

    // at this point the right tool has been found, now find the requested operation
    if(tool_found==TRUE)
      {
      // read non-operation specific parameters
      valstart=strstr(scratch,"GEOMETRY");
      if(valstart!=NULL)
	{
	optr->geo_type=(-1);
	if(strstr(scratch,"SLICE"))optr->geo_type=1;
	if(strstr(scratch,"VOXEL"))optr->geo_type=2;
	if(strstr(scratch,"SURFACE"))optr->geo_type=3;
	}
	  
      // watch for the requested operation
      if(oper_found==FALSE && strstr(scratch,full_oper_name)!=NULL)
	{oper_found=TRUE;continue;}
      if(oper_found==TRUE && strstr(scratch,full_oper_name)!=NULL && strstr(scratch,"/")!=NULL)
	{oper_found=FALSE;continue;}

      // read operation specific parameters
      if(oper_found==TRUE)
	{
	valstart=strstr(scratch,"SEQUENCE");
	if(valstart!=NULL)
	  {
	  valend=strstr(valstart,"<");
	  valend[1]=0;
	  sprintf(line_type_seq,"%s",valstart+9);			// build search string

	  if(strstr(line_type_seq,",")!=NULL){token=strtok(line_type_seq,",");}	// if more than one in sequence
	  else {token=strtok(line_type_seq,"<");}			// if only one in sequence
	  while(token!=NULL)
	    {
	    token=strtok(NULL,",");					// get next type in list
	    }
	  continue;
	  }
	    
	// end of operation identified.  turn off collecting pararms.
	valstart=strstr(scratch,optr->name);
	if(valstart!=NULL)
	  {
	  oper_found=FALSE;
	  continue;
	  }
	}

      // everything within this IF simply reads the name of the new operation
      if(oper_found==FALSE)						// if flag set to read new name
	{
	h=0;
	copy_flag=FALSE;
	for(i=0;i<strlen(scratch);i++)
	  {
	  if(scratch[i]==60)					// "<"
	    {copy_flag=TRUE;continue;}
	  if(scratch[i]==62)					// ">"
	    {copy_flag=FALSE;continue;}
	  if(copy_flag==TRUE)
	    {
	    new_operation_name[h]=scratch[i];
	    h++;
	    if(h>254)h=254;
	    }
	  }
	copy_flag=FALSE;
	new_operation_name[h]=0;
	if(h>0)
	  {
	  oper_found=TRUE;
	  optr=operation_make();
	  oper_ID++;
	  optr->ID=oper_ID;
	  strncpy(optr->name,new_operation_name,sizeof(optr->name)-1);
	  operation_insert(mptr,optr);
	  mptr->oper_qty=oper_ID;
	  success=TRUE;
	  }
	}

      continue;								// move onto next line off file
      }
    else  								// end of this tool has been hit and correct tool was not found
      {
      optr=NULL;
      oper_found=0;
      tool_found=0;							// set flag to show the correct tool has not been found
      continue;								// move onto next line off file
      }
    }		// end of Opers_fd file read

  return(success);	
  }
*/
