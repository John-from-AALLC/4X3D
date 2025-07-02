#include "Global.h"


// STL loader

// Set up a model data structure based on linked lists using the fact that:
// Every facet should have 3 neighboring facets and be defined by 3 vertexes.  One
// vertex however, can have many associated facets.
//
// The idea is to NOT have repetative data held in memory.  Therefore, as each vertex
// is read, it is checked against all other vertices in memeory.  If a match is found then
// that vertex is referenced by the appropriate facet.  If a match is not found, then it is
// created and then referenced.
//
// Ultimately, the vertex list is scrubbed to remove redundant vertexes and the facet list
// held by each vertex is used to define the neighboring facets.
//
// The result is a model data structure that demonstrates any errors instantly, allows
// for faster support generation, and facet manipulation and/or display.


// Callback to set response for warnings/errors
void cb_stl_load_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if(response_id==GTK_RESPONSE_OK)gtk_window_close(GTK_WINDOW(dialog));
    return;
}

// Load STL file into structured model format
int stl_model_load(model *mptr)
{
    int			mtyp=MODEL;
    char	 	*valstart,*valend,*token;
    int			h,i,j,k,vix,viy,viz;
    int			bar_size,stl_binary,read_error;
    double		amt_done=0.0,old_done=0.0;
    float		vert[3][3],norm[3];
    float 		index_convert_x,index_convert_y,index_convert_z;
    unsigned int	fattr;
    unsigned long	facet_ctr,facet_total;
    unsigned long 	vertex_used,vertex_total;
    char		title[255];
    char		mdl_name[255];
    int			vAfound=0,vBfound=0,vCfound=0;			// flags to control vertex identification

    vertex 		*vptr,*vnew,*vold,*vdel,*vnewA,*vnewB,*vnewC;	// scratch vertex holders
    vertex 		*vtx_idx_new,*vtx_idx_old;
    facet		*fptr,*fnxt,*fnew;				// scratch facet holder
    model		*mnew,*mpic;
    GtkWidget		*win_local_dialog;


    //debug_flag=367;
    if(debug_flag==366)printf("STL Load:  Entering function... \n");

    // scan the file first to get the max/min boundaries
    // we need to do this to ensure (move) the model into the positive quandrant, and to evenly
    // distribute the vertex index markers to provide consistent and reasonable loading speed.
    if(mptr==NULL)return(FALSE);
    if(model_in==NULL)return(FALSE);
    mptr->xmin[MODEL]=INT_MAX_VAL; mptr->ymin[MODEL]=INT_MAX_VAL; mptr->zmin[MODEL]=INT_MAX_VAL;
    mptr->xmax[MODEL]=INT_MIN_VAL; mptr->ymax[MODEL]=INT_MIN_VAL; mptr->zmax[MODEL]=INT_MIN_VAL;
    mptr->facet_prox_tol=TIGHTCHECK;
    
    // build simplified model name for info display
    {
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
    }

    // show the progress bar
    sprintf(scratch,"  %s  ",mdl_name);
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Scanning file...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_set_visible(win_info,TRUE);
    gtk_widget_queue_draw(win_info);
    while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
    
    // test if text or binary format
    stl_binary=TRUE;
    rewind(model_in);
    i=0;
    while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
      {
      if(strstr(scratch,"facet normal")!=NULL){stl_binary=FALSE;break;} // must have "facet normal"... always
      i++;
      if(i>25)break;
      }
    if(ferror(model_in))
      {
      printf("/nError - unable to properly read ASCII STL file data!\n");
      return(FALSE);
      }
    rewind(model_in);
	
    // scan binary file for size to establish index values
    if(stl_binary==TRUE)
      {
      for(j=0;j<80;j++)							// read header
        {
	if(fread(&title[j],1,1,model_in)<1)
	  {
	  printf("Error - STL file read error reading header.\n");
	  return(FALSE);
	  }
	}
      if(fread(&facet_total,4,1,model_in)<1)				// read number of facets
        {
	printf("Error - STL file read error acquiring facet count.\n");
	return(FALSE);
	}
      printf("Opening binary STL file... %ld facets.\n",facet_total);

      // verify that a reasonable number of facets is found.
      if(facet_total<3 || facet_total>5.0e6)return(0);			// limit binary files to 5M facets

      // scan facet verticies to establish max/min boundaries of this model
      for(facet_ctr=1;facet_ctr<=facet_total;facet_ctr++)
	{
	for(h=0;h<3;h++)
	  {
	  if(fread(&norm[h],4,1,model_in)<1)				// get unit normal vector vertex data
	    {
	    printf("Error - STL file read error acquiring facet normal at facet %d.\n",facet_ctr);
	    return(FALSE);
	    }
	  }
	for(h=0;h<3;h++)						// get 3 vertex corners of facet
	  {
	  for(j=0;j<3;j++)
	    {
	    if(fread(&vert[h][j],4,1,model_in)<1)
	      {
	      printf("Error - STL file read error acquiring facet vertex at facet %d.\n",facet_ctr);
	      return(FALSE);
	      }
	    }
	  if(vert[h][0]<mptr->xmin[MODEL])mptr->xmin[MODEL]=vert[h][0];
	  if(vert[h][0]>mptr->xmax[MODEL])mptr->xmax[MODEL]=vert[h][0];
	  if(vert[h][1]<mptr->ymin[MODEL])mptr->ymin[MODEL]=vert[h][1];
	  if(vert[h][1]>mptr->ymax[MODEL])mptr->ymax[MODEL]=vert[h][1];
	  if(vert[h][2]<mptr->zmin[MODEL])mptr->zmin[MODEL]=vert[h][2];
	  if(vert[h][2]>mptr->zmax[MODEL])mptr->zmax[MODEL]=vert[h][2];
	  }
	if(fread(&fattr,2,1,model_in)<1)				// get facet attribute
	  {
	  printf("Error - STL file read error acquiring facet attribute at facet %d.\n",facet_ctr);
	  return(FALSE);
	  }
	}
      }
	  
    // scan ASCII file for size to establish index values and number of facets
    else 
      {
      printf("Opening ASCII STL file...\n");
      facet_total=0;
      while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
	{
	if(strstr(scratch,"facet normal")!=NULL)			// count number of facets
	  {
	  facet_total++;

	  // verify that a reasonable number of facets is found.
	  if(facet_total>5.0e6)return(0);

	  continue;
	  }
	if(strstr(scratch,"vertex")!=NULL)				// get vertex max/min values
	  {
	  token=strtok(scratch," ");					// extract the word "vertex"
	  token=strtok(NULL," ");					// get X
	  vert[0][0]=atof(token);
	  token=strtok(NULL," ");					// get Y
	  vert[0][1]=atof(token);
	  token=strtok(NULL," ");					// get Z
	  vert[0][2]=atof(token);

	  if(vert[0][0]<mptr->xmin[MODEL])mptr->xmin[MODEL]=vert[0][0];
	  if(vert[0][0]>mptr->xmax[MODEL])mptr->xmax[MODEL]=vert[0][0];
	  if(vert[0][1]<mptr->ymin[MODEL])mptr->ymin[MODEL]=vert[0][1];
	  if(vert[0][1]>mptr->ymax[MODEL])mptr->ymax[MODEL]=vert[0][1];
	  if(vert[0][2]<mptr->zmin[MODEL])mptr->zmin[MODEL]=vert[0][2];
	  if(vert[0][2]>mptr->zmax[MODEL])mptr->zmax[MODEL]=vert[0][2];
	  
	  fattr=0;							// default to zero
	  }
	}
      if(ferror(model_in))
	{
	printf("/nError - unable to properly read ASCII STL file data!\n");
	return(FALSE);
	}
      }
	
    vertex_total=facet_total*3;
    
    // calculate the index array space conversion factors (index increments)
    // in other words, size the hash table to fit the size of this model
    index_convert_x=(mptr->xmax[MODEL]-mptr->xmin[MODEL])/VTX_INDEX;
    index_convert_y=(mptr->ymax[MODEL]-mptr->ymin[MODEL])/VTX_INDEX;
    index_convert_z=(mptr->zmax[MODEL]-mptr->zmin[MODEL])/VTX_INDEX;

    if(debug_flag==366)
      {
      printf("\nSTL FILE INPUT:\n");
      printf("  facet total=%ld\n",facet_total);
      printf("  xmin=%6.3f  ymin=%6.3f  zmin=%6.3f \n",mptr->xmin[MODEL],mptr->ymin[MODEL],mptr->zmin[MODEL]);
      printf("  xmax=%6.3f  ymax=%6.3f  zmax=%6.3f \n",mptr->xmax[MODEL],mptr->ymax[MODEL],mptr->zmax[MODEL]);
      printf("  xidx=%6.3f  yIdx=%6.3f  zIdx=%6.3f \n",index_convert_x,index_convert_y,index_convert_z);
      printf("  binary=%d\n",stl_binary);
      }

    // build index pointers into hash table.  these pointers "jump" into the vertex list
    // based on the xyz values of the vtx so that the entire vtx list does not have to be
    // searched... just what is between this index pointer and the next index pointer.
    for(i=0;i<VTX_INDEX;i++)						//  x index
      {
      for(h=0;h<VTX_INDEX;h++)						//  y index
	{
	for(j=0;j<VTX_INDEX;j++)					//  z index
	  {
	  vnew=vertex_make();						// create a new vertex element
	  vnew->x=i*index_convert_x;					// set all coords to match array index position
	  vnew->y=h*index_convert_y;
	  vnew->z=j*index_convert_z;
	  vnew->attr=411;						// set to indicate it is an index element
	  vnew->flist=NULL;
	  vnew->next=NULL;
	  vtx_index[i][h][j]=vnew;					// save address of this index element for future reference
	  vertex_insert(mptr,mptr->vertex_last[MODEL],vtx_index[i][h][j],MODEL);
	  }
	}
      }
    if(debug_flag==366)printf("  index pointers built...\n");

    // restart at beginning to read and process each vertex and facet
    read_error=FALSE;
    rewind(model_in);
    if(stl_binary==TRUE)
      {
      for(j=0;j<80;j++)
        {
	if(fread(&title[j],1,1,model_in)<1)read_error=TRUE;
	if(read_error==TRUE)break;
	}
      if(fread(&facet_total,4,1,model_in)<1)read_error=TRUE;
      }
    if(read_error==TRUE)
      {
      printf("/nError - unable to properly read binary STL file heading!\n");
      return(FALSE);
      }

    // update progress bar info
    idle_start_time=time(NULL);						// reset idle time
    sprintf(scratch,"Step 1 of 3:  Loading %d facets...",facet_total);
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);

    // scan file for model boundaries
    vertex_used=0;							// counts number of unique vtxs added
    for(facet_ctr=1;facet_ctr<=facet_total;facet_ctr++)
      {
      
      // update status to user
      amt_done = (double)(facet_ctr)/(double)(facet_total);
      if((int)(amt_done*100)>(int)(old_done*100))
        {
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
        while (g_main_context_iteration(NULL, FALSE));
	if(debug_flag==368)printf("Tool temp = %6.2f \n",Tool[0].thrm.tempC);
	old_done=amt_done;
	}

      if(debug_flag==367)printf("Reading facet number %ld of %ld \n",facet_ctr,facet_total);

      // binary read in vertices
      if(stl_binary==TRUE)
	{
	read_error=FALSE;
	for(h=0;h<3;h++)
	  {
	  if(fread(&norm[h],4,1,model_in)<1)read_error=TRUE;			// get unit normal vector vertex data
	  }
	for(h=0;h<3;h++)							// get 3 vertex corners of facet
	  {
	  for(j=0;j<3;j++)
	    {
	    if(fread(&vert[h][j],4,1,model_in)<1)read_error=TRUE;		// load xyz of this vtx
	    }
	  vert[h][0] -= mptr->xmin[MODEL];					// re-orient to 0,0,0 so index marks align
	  vert[h][1] -= mptr->ymin[MODEL];
	  vert[h][2] -= mptr->zmin[MODEL];
	  } 
	if(read_error==TRUE)
	  {
	  printf("/nError - unable to properly read binary STL file data!\n");
	  return(FALSE);
	  }
	
	}
      // ascii read in vertices
      else 
	{
	h=0;
	while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
	  {
	  if(strstr(scratch,"facet normal")!=NULL)					
	    {
	    token=strtok(scratch," ");					// extract the word "facet"
	    token=strtok(NULL," ");					// extract the word "normal"
	    token=strtok(NULL," ");					// get X
	    norm[0]=atof(token);
	    token=strtok(NULL," ");					// get Y
	    norm[1]=atof(token);
	    token=strtok(NULL," ");					// get Z
	    norm[2]=atof(token);
	    continue;
	    }
	  if(strstr(scratch,"vertex")!=NULL)					
	    {
	    token=strtok(scratch," ");					// extract the word "vertex"
	    token=strtok(NULL," ");					// extract X value
	    vert[h][0]=atof(token);
	    token=strtok(NULL," ");					// extract Y value
	    vert[h][1]=atof(token);
	    token=strtok(NULL," ");					// extract Z value
	    vert[h][2]=atof(token);
	    
	    //vert[h][0]-=XMin;						// subtract off min values to move object to origin
	    //vert[h][1]-=YMin;
	    //vert[h][2]-=ZMin;
	    h++;							// increment vtx array
	    if(h>2)break;						// at this point facet norm and 3 vtx have been read
	    }
	  }
	if(ferror(model_in))
	  {
	  printf("/nError - unable to properly read ASCII STL file data!\n");
	  return(FALSE);
	  }
      }
		 
      // Make three new vertices and load with data.  Note these are NOT inserted into the linked list yet.
      vnewA=vertex_make();
      vnewA->x=vert[0][0]; vnewA->y=vert[0][1]; vnewA->z=vert[0][2]; vnewA->attr=1;
      vnewB=vertex_make();
      vnewB->x=vert[1][0]; vnewB->y=vert[1][1]; vnewB->z=vert[1][2]; vnewB->attr=1;
      vnewC=vertex_make();
      vnewC->x=vert[2][0]; vnewC->y=vert[2][1]; vnewC->z=vert[2][2]; vnewC->attr=1;
      
      if(debug_flag==367)
        {
        printf("  facet=%d\n",facet_ctr);
        printf("  vtxA=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewA,vnewA->x,vnewA->y,vnewA->z);
        printf("  vtxB=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewB,vnewB->x,vnewB->y,vnewB->z);
        printf("  vtxC=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewC,vnewC->x,vnewC->y,vnewC->z);
        while(!kbhit());
	}

      // Scan existing vertex list using vertex index pointer array.  The array values are based on the X axis length
      // in index increments across the model.  If a matching vertex is not found between index pointer values it is new
      // and will be added at its appropriate location.

      // Search for vtx A in list, add if unique
      vAfound=FALSE;
      vix=(int)(vnewA->x/index_convert_x);				// define array index value based on X value
      if(vix<0)vix=0;
      if(vix>(VTX_INDEX-2))vix=(VTX_INDEX-2);
      viy=(int)(vnewA->y/index_convert_y);				// define array index value based on Y value
      if(viy<0)viy=0;
      if(viy>(VTX_INDEX-2))viy=(VTX_INDEX-2);
      viz=(int)(vnewA->z/index_convert_z);				// define array index value based on Z value
      if(viz<0)viz=0;
      if(viz>(VTX_INDEX-2))viz=(VTX_INDEX-2);
      vptr=vtx_index[vix][viy][viz]->next;				// scan portion of linked list from this index value
      while(vptr!=vtx_index[vix][viy][viz+1])				// finish scan at next increment index value
	{
	if(vertex_compare(vptr,vnewA,mptr->facet_prox_tol)==TRUE)	// if a vtx already exists at this location...
	  {
	  vertex_destroy(vnewA);					// ... free memory of one that was created
	  vnewA=vptr;							// ... and instead have it point to the existing match
	  vAfound=TRUE;							// ... set flag to indicate match was found
	  break;							// ... get out of loop.  no need to keep scanning.
	  }
	vptr=vptr->next;						// increment to next vtx
	}
      if(vAfound==FALSE)						// if no match was found...
	{
	vertex_insert(mptr,vtx_index[vix][viy][viz],vnewA,MODEL);	// ... add to vtx list of model
	vertex_used++;							// ... increment the number of unique vtxs found
	}
	    
      // Search for vtx B in list, add if unique
      vBfound=FALSE;
      vix=(int)(vnewB->x/index_convert_x);				// define array index value based on X value
      if(vix<0)vix=0;
      if(vix>(VTX_INDEX-2))vix=(VTX_INDEX-2);
      viy=(int)(vnewB->y/index_convert_y);				// define array index value based on Y value
      if(viy<0)viy=0;
      if(viy>(VTX_INDEX-2))viy=(VTX_INDEX-2);
      viz=(int)(vnewB->z/index_convert_z);				// define array index value based on Z value
      if(viz<0)viz=0;
      if(viz>(VTX_INDEX-2))viz=(VTX_INDEX-2);
      vptr=vtx_index[vix][viy][viz]->next;				// set start point of scan to beginning of index		
      while(vptr!=vtx_index[vix][viy][viz+1])				// scan portion of linked list from this index value
	{
	if(vertex_compare(vptr,vnewB,mptr->facet_prox_tol)==TRUE)	// if a vtx already exists at this location...
	  {
	  vertex_destroy(vnewB);					// ... free memory of one that was created
	  vnewB=vptr;							// ... and instead have it point to the existing match
	  vBfound=TRUE;
	  break;
	  }
	vptr=vptr->next;
	}
      if(vBfound==FALSE)
	{
	vertex_insert(mptr,vtx_index[vix][viy][viz],vnewB,MODEL);	// if unique location... add to vtx list of model
	vertex_used++;
	}
	    
      // Search for vtx C in list, add if unique
      vCfound=FALSE;
      vix=(int)(vnewC->x/index_convert_x);				// define array index value based on X value
      if(vix<0)vix=0;
      if(vix>(VTX_INDEX-2))vix=(VTX_INDEX-2);
      viy=(int)(vnewC->y/index_convert_y);				// define array index value based on Y value
      if(viy<0)viy=0;
      if(viy>(VTX_INDEX-2))viy=(VTX_INDEX-2);
      viz=(int)(vnewC->z/index_convert_z);				// define array index value based on Z value
      if(viz<0)viz=0;
      if(viz>(VTX_INDEX-2))viz=(VTX_INDEX-2);
      vptr=vtx_index[vix][viy][viz]->next;				// scan portion of linked list from this index value
      while(vptr!=vtx_index[vix][viy][viz+1])
	{
	if(vertex_compare(vptr,vnewC,mptr->facet_prox_tol)==TRUE)	// if a vtx already exists at this location...
	  {
	  vertex_destroy(vnewC);					// ... free memory of one that was created
	  vnewC=vptr;							// ... and instead have it point to the existing match
	  vCfound=TRUE;
	  break;
	  }
	vptr=vptr->next;
	}
      if(vCfound==FALSE)
	{
	vertex_insert(mptr,vtx_index[vix][viy][viz],vnewC,MODEL);	// if unique location... add to vtx list of model
	vertex_used++;
	}
	  
      // In the case where positional tolerance is collapsing some facets, a check needs to be made
      // to ensure these three unique vtxs are actually far enough apart to stand as individuals.  If not,
      // they will collapse into one and only get added if one in near its coords does not already exist.
      // Obviously if that happens, no facet is added... just the vertex (because it may be needed by futrure facet).
      if(vnewA==vnewB || vnewA==vnewC || vnewB==vnewC)			// if any two are same... 
        {
        fread(&fattr,2,1,model_in);					// ... read facet attribute just to move file ptr
        continue;							// ... skip adding facet
        }
	  
      // At this point we now have vertices loaded, but no facet yet.
      // Now create a new facet, connect the vertexes in a sequence that matches the incoming unit normal
      // (i.e. so their cross product produces the same normal direction) and ensure this facet is unique.
      fnew=facet_make();						// allocate memory and set default values for facet
      vnewA->flist=facet_list_manager(vnewA->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
      vnewB->flist=facet_list_manager(vnewB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
      vnewC->flist=facet_list_manager(vnewC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
      fnew->vtx[0]=vnewA;						// assign which vtxs belong to this facet
      fnew->vtx[1]=vnewB;
      fnew->vtx[2]=vnewC;
      fnew->unit_norm->x=norm[0];					// define normal values as came off file
      fnew->unit_norm->y=norm[1];
      fnew->unit_norm->z=norm[2];
      fnew->member=0;
      facet_area(fnew);
      facet_insert(mptr,mptr->facet_last[MODEL],fnew, MODEL); 		// insert this new facet into the model's data structure
      fread(&fattr,2,1,model_in);					// get facet attribute
      fnew->attr=fattr;							// assign attribute to facet
      }

    // Now delete out the vtx_index values so they aren't considered part of geometry
    // update the progress bar
    sprintf(scratch,"  %s  ",mdl_name);
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Step 2 of 3:  Cleaning up data...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);
    
    // loop thru vtx list and delete index vtxs out of the model's vtx data linked list.  
    // do this because they are intimate with the model vtxs and they will just cause confusion 
    // downstream.  if a strong reason to keep them is realized they could be kept and the rest
    // of vtx management code could be fixed to account for them.  right now, they are kept
    // as a copy under mptr->vertex_index for reference.
    h=0;j=0;
    vold=NULL;
    vtx_idx_new=NULL;
    vtx_idx_old=NULL;
    vptr=mptr->vertex_first[MODEL];
    while(vptr!=NULL)
      {
      // update status to user
      idle_start_time=time(NULL);						// reset idle time
      amt_done = (double)(h)/(double)(mptr->vertex_qty[MODEL]);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));
      
      // delete if an index vtx
      if(vptr->attr==411)							// if an index vtx of hash table ...
        {
	// make copy of index list for future reference
	vtx_idx_new=vertex_copy(vptr,NULL);					// ... make a copy 411 vtx
	if(vtx_idx_new!=NULL)							// ... if copy was successful...
	  {
	  if(mptr->vertex_index[MODEL]==NULL)mptr->vertex_index[MODEL]=vtx_idx_new;		// ... if first one in list.
	  if(vtx_idx_old!=NULL)vtx_idx_old->next=vtx_idx_new;			// ... if there is one defined in list before this one.
	  vtx_idx_old=vtx_idx_new;						// ... save address of last one defined
	  }
	  
	if(vold==NULL)								// ... if first vtx in list
	  {mptr->vertex_first[MODEL]=vptr->next;}				// ... re-assign pointer to first vtx
	else 
	  {vold->next=vptr->next;}						// ... skip over vtx to be deleted
	vdel=vptr;								// ... set vtx to delete
	vptr=vptr->next;							// ... assign to next vtx in list
	vertex_destroy(vdel); j++;						// ... release from memory and update count in memory
	}
      else 
        {
        vold=vptr;								// ... save address of prev vtx
        vptr=vptr->next;							// ... move onto next vtx in list
	}
      h++;
      }
	
    // use previous min/max scan to set a preliminary size for this model
    mptr->xmin[mtyp]=0;mptr->xmax[mtyp]=(XMax-XMin);
    mptr->ymin[mtyp]=0;mptr->ymax[mtyp]=(YMax-YMin);
    mptr->zmin[mtyp]=0;mptr->zmax[mtyp]=(ZMax-ZMin);
    
    // update progress bar to display during geoemtry analysis
    sprintf(scratch,"  %s  ",mdl_name);
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Step 3 of 3:  Analyzing geometry...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);

    facet_find_all_neighbors(mptr->facet_first[MODEL]);			// create facet list for each vtx
    model_integrity_check(mptr);					// check for geometry flaws
    idle_start_time=time(NULL);						// reset idle time
    edge_display(mptr,set_view_edge_angle,MODEL);			// determine which sharp edges to display in 3D view
    edge_silhouette(mptr,MVdisp_spin,MVdisp_tilt);			// determine which silhouette edges to display in 3D view

    // hide progress bar
    gtk_widget_set_visible(win_info,FALSE);
    
    // debug - vertex dump
    /*
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      vnewA=fptr->vtx[0];
      vnewB=fptr->vtx[1];
      vnewC=fptr->vtx[2];
      printf("  fptr = %X \n",fptr);
      printf("  vtxA=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewA,vnewA->x,vnewA->y,vnewA->z);
      printf("  vtxB=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewB,vnewB->x,vnewB->y,vnewB->z);
      printf("  vtxC=%X  x=%6.3f y=%6.3f z=%6.3f \n",vnewC,vnewC->x,vnewC->y,vnewC->z);
      fptr=fptr->next;
      }
    */

    // Copy model vtxs to array format understood by GPU with OpenGL
    /*
    ndvtx=0;
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      for(i=0;i<3;i++)
	{
	dvtx[ndvtx].x=fptr->vtx[i]->x/100;
	dvtx[ndvtx].y=fptr->vtx[i]->y/100;
	dvtx[ndvtx].z=fptr->vtx[i]->z/100;
	dvtx[ndvtx].w=1.0;
	//printf("  n=%d  x=%6.3f y=%6.3f z=%6.3f \n",ndvtx,dvtx[ndvtx].x,dvtx[ndvtx].y,dvtx[ndvtx].z);

	dnrm[ndvtx].x=fptr->unit_norm->x;
	dnrm[ndvtx].y=fptr->unit_norm->y;
	dnrm[ndvtx].z=fptr->unit_norm->z;
	dnrm[ndvtx].w=0.0;
	
	ndvtx++;
	if(ndvtx>=MAX_VERTS)break;
	}
      fptr=fptr->next;
      if(ndvtx>=MAX_VERTS)break;
      }
    */
  
    //display_model_flag=TRUE;
    printf("Facets=%ld  VRead=%ld  VUsed=%ld  VMemory=%ld \n",facet_total,vertex_total,vertex_used,vertex_mem);
    printf("STL Load:  Success!  Exiting function...\n\n");
    
    //model_dump(mptr);
    
    return(1);
}


// Load STL file into raw facet format
int stl_raw_load(model *mptr)
{
    char	 	*valstart,*valend,*token;
    int			h,i,j,k,vix,viy,viz;
    int			bar_size,stl_binary;
    unsigned int	bucket[VTX_INDEX][VTX_INDEX][VTX_INDEX];
    double		amt_done=0.0;
    float		vert[3][3],norm[3];
    float 		index_convert_x,index_convert_y,index_convert_z;
    unsigned int	attr;
    unsigned long	facet_ctr,facet_total;
    unsigned long 	vertex_used,vertex_total;
    char		title[255];
    char		mdl_name[255];
    int			vAfound=0,vBfound=0,vCfound=0;			// flags to control vertex identification
    int			eAfound=0,eBfound=0,eCfound=0;			// flags to control edge identification

    vertex		*posptr;
    facet		*fctptr,*fctold;
    model		*mnew,*mpic;
    GtkWidget		*win_local_dialog;
    

    printf("STL RAW Load:  Entering function... \n");

    // scan the file first to get the max/min boundaries
    // we need to do this to ensure (move) the model into the positive quandrant, and to evenly
    // distribute the vertex index markers to provide consistent and reasonable loading speed.
    if(model_in==NULL)return(0);
    XMin=INT_MAX_VAL;YMin=INT_MAX_VAL;ZMin=INT_MAX_VAL;			// max/min boundaries of all models currently loaded
    XMax=INT_MIN_VAL;YMax=INT_MIN_VAL;ZMax=INT_MIN_VAL;
    
    // build simplified model name for info display
    {
      h=0;
      memset(mdl_name,0,sizeof(mdl_name));
      for(i=0;i<strlen(mptr->model_file);i++)
	{
	if(mptr->model_file[i]==41)h=i;					// find the last "/" in file name structure
	}
      if(h>0)
	{
	i=0;h++;
	while(h<strlen(mptr->model_file))
	  {
	  mdl_name[i]=mptr->model_file[h];
	  i++;h++;
	  if(i>40)break;
	  }
	}
      else 
	{
	strncpy(mdl_name,mptr->model_file,40);
	}
      printf("  model = %s\n",mdl_name);
    }

    // test if text or binary format
    stl_binary=TRUE;
    rewind(model_in);
    i=0;
    while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
      {
      if(strstr(scratch,"facet normal")!=NULL){stl_binary=FALSE;break;} // must have "facet normal"... always
      i++;
      if(i>25)break;
      }
    if(ferror(model_in))
      {
      printf("/nError - unable to properly read STL file data!\n");
      return(FALSE);
      }
    
    // load BINARY file
    if(stl_binary==TRUE)
      {
      // show the progress bar
      sprintf(scratch,"  %s  ",mdl_name);
      gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
      sprintf(scratch,"  Loading binary file...  ");
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
      gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
      gtk_widget_set_visible(win_info,TRUE);
      gtk_widget_queue_draw(win_info);
      while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
	
      rewind(model_in);
      for(j=0;j<40;j++)fread(&title[j],1,1,model_in);
      for(j=0;j<40;j++)fread(&title[j],1,1,model_in);
      fread(&facet_total,4,1,model_in);				/* Read number of facets. */
      printf("Loading binary file.  facets=%ld\n",facet_total);

      /* Verify that a reasonable number of facets is found. */
      if(facet_total<3 || facet_total>2.0e6)			// limit binary files to 1M facets
	{
	fclose(model_in);
	sprintf(scratch," \nThis file is in excess of 2M facets,\nwhich is too large.  Please reduce.\nthe file size.");
	sprintf(scratch,"Allowable facet count exceeded! \n2M max vs %ld facets\n",facet_total);
	aa_dialog_box(win_model,1,0,"STL Load Error",scratch);
	return(0);
	}

      // update progress bar info
      sprintf(scratch,"Loading %d facets...",facet_total);
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);

      // scan facet verticies to establish max/min boundaries of this model
      fctold=NULL;
      for(facet_ctr=1;facet_ctr<=facet_total;facet_ctr++)
	{
	// update status to user
	amt_done = (double)(facet_ctr)/(double)(facet_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));

	// create new facet in memory
	fctptr=facet_make();
	fctptr->next=NULL;
	if(fctold==NULL)
	  {mptr->facet_first[MODEL]=fctptr; fctptr->next=NULL;}
	else 
	  {fctold->next=fctptr;}
	
	// get unit normal vector vertex data
	for(h=0;h<3;h++)fread(&norm[h],4,1,model_in);
	fctptr->unit_norm=vertex_make();
	fctptr->unit_norm->x=norm[0];
	fctptr->unit_norm->y=norm[1];
	fctptr->unit_norm->z=norm[2];
	fctptr->unit_norm->attr=1;
	fctptr->attr=1;
	
	// get 3 vertex corners of facet
	for(h=0;h<3;h++)
	  {
	  for(j=0;j<3;j++)fread(&vert[h][j],4,1,model_in);
	  
	  fctptr->vtx[h]=vertex_make();
	  fctptr->vtx[h]->x=vert[h][0];
	  fctptr->vtx[h]->y=vert[h][1];
	  fctptr->vtx[h]->z=vert[h][2];
	  fctptr->vtx[h]->attr=1;
	  fctptr->vtx[h]->flist=facet_list_manager(fctptr->vtx[h]->flist,fctptr,ACTION_ADD);
	  }
	fread(&attr,2,1,model_in);					// get facet attribute
	fctold=fctptr;
	}
      }
      
    // load ASCII file
    else 
      {
	
      // show the progress bar
      sprintf(scratch,"  %s  ",mdl_name);
      gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
      sprintf(scratch,"  Scanning ASCII file...  ");
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
      gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
      gtk_widget_set_visible(win_info,TRUE);
      gtk_widget_queue_draw(win_info);
      while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly

      // first scan ASCII file for size to establish index values and number of facets
      rewind(model_in);
      facet_total=0;
      while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
	{
	if(strstr(scratch,"facet normal")!=NULL)			// count number of facets
	  {
	  facet_total++;
	  continue;
	  }
	if(strstr(scratch,"vertex")!=NULL)				// get vertex max/min values
	  {
	  token=strtok(scratch," ");				// extract the word "vertex"
	  token=strtok(NULL," ");					// get X
	  vert[0][0]=atof(token);
	  token=strtok(NULL," ");					// get Y
	  vert[0][1]=atof(token);
	  token=strtok(NULL," ");					// get Z
	  vert[0][2]=atof(token);
	  if(vert[0][0]<XMin)XMin=vert[0][0];
	  if(vert[0][0]>XMax)XMax=vert[0][0];
	  if(vert[0][1]<YMin)YMin=vert[0][1];
	  if(vert[0][1]>YMax)YMax=vert[0][1];
	  if(vert[0][2]<ZMin)ZMin=vert[0][2];
	  if(vert[0][2]>ZMax)ZMax=vert[0][2];
	  }
	}
      if(ferror(model_in))
	{
	printf("/nError - unable to properly read ASCII STL file data!\n");
	return(FALSE);
	}

      // update progress bar info
      sprintf(scratch,"Loading %d facets...",facet_total);
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
      printf("Loading ASCII file.  facets=%ld\n",facet_total);
      
      rewind(model_in);
      fctold=NULL;
      for(facet_ctr=1;facet_ctr<=facet_total;facet_ctr++)
	{
	
	// update status to user
	amt_done = (double)(facet_ctr)/(double)(facet_total);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
	while (g_main_context_iteration(NULL, FALSE));
	
	h=0;
	while(fgets(scratch,sizeof(scratch),model_in)!=NULL)
	  {
	  if(strstr(scratch,"facet normal")!=NULL)					
	    {
	    token=strtok(scratch," ");				// extract the word "facet"
	    token=strtok(NULL," ");					// extract the word "normal"
	    token=strtok(NULL," ");					// get X
	    norm[0]=atof(token);
	    token=strtok(NULL," ");					// get Y
	    norm[1]=atof(token);
	    token=strtok(NULL," ");					// get Z
	    norm[2]=atof(token);
	    continue;
	    }
	  if(strstr(scratch,"vertex")!=NULL)					
	    {
	    token=strtok(scratch," ");				// extract the word "vertex"
	    token=strtok(NULL," ");					// extract X value
	    vert[h][0]=atof(token);
	    token=strtok(NULL," ");					// extract Y value
	    vert[h][1]=atof(token);
	    token=strtok(NULL," ");					// extract Z value
	    vert[h][2]=atof(token);
	    h++;							// increment vtx array
	    if(h>2)break;						// at this point facet norm and 3 vtx have been read
	    }
	  }
	if(ferror(model_in))
	  {
	  printf("/nError - unable to properly read ASCII STL file data!\n");
	  return(FALSE);
	  }
	  
	// create new facet in memory
	fctptr=facet_make();
	fctptr->next=NULL;
	if(fctold==NULL){mptr->facet_first[MODEL]=fctptr; fctptr->next=NULL;}
	else {fctold->next=fctptr;}
	
	// load the unit normal vector vertex data into memory
	fctptr->unit_norm=vertex_make();
	fctptr->unit_norm->x=norm[0];
	fctptr->unit_norm->y=norm[1];
	fctptr->unit_norm->z=norm[2];
	fctptr->attr=1;
	
	// load the 3 vertex corners of facet into memory
	for(h=0;h<3;h++)
	  {
	  fctptr->vtx[h]=vertex_make();
	  fctptr->vtx[h]->x=vert[h][0];
	  fctptr->vtx[h]->y=vert[h][1];
	  fctptr->vtx[h]->z=vert[h][2];
	  fctptr->vtx[h]->attr=1;					// set to indicate read, but not structured
	  fctptr->vtx[h]->flist=facet_list_manager(fctptr->vtx[h]->flist,fctptr,ACTION_ADD);
	  }

	fctold=fctptr;
	}
      }
    
    // hide progress bar
    gtk_widget_set_visible(win_info,FALSE);
    
    // set finally tally of facets and vtxs
    mptr->facet_qty[MODEL]=facet_total-1;
    vertex_total=facet_total*3;
    printf("    Facets=%ld  Vertexes=%ld  \n",facet_total,vertex_total);


    printf("STL RAW Load:  Success!  Exiting function...\n");
    while(!kbhit());
    
    return(1);
}

