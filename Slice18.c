#include "Global.h"


// This function allocates space for a single link list element of type slice
slice *slice_make(void)
{
    int 	i;
    slice	*sptr;

    sptr=(slice *)malloc(sizeof(slice));				// Go get memory and new address
    if(sptr==NULL)return(NULL);						// Unable to allocate for some reason
    sptr->ID=(-1);
    sptr->msrc=NULL;							// ptr to source model
    for(i=0;i<MAX_LINE_TYPES;i++)					// init line types
      {
      sptr->pfirst[i]=NULL;						// ptr to first polygon in linked list
      sptr->plast[i]=NULL;						// ptr to last polygon in linked list
      sptr->pqty[i]=0;							// number of polygons held in linked list
      sptr->pdist[i]=0;
      sptr->ptime[i]=0;
      sptr->stype[i]=0;

      sptr->perim_vec_qty[i]=0;
      sptr->perim_vec_first[i]=NULL;
      sptr->perim_vec_last[i]=NULL;
    
      sptr->ss_vec_qty[i]=0;
      sptr->ss_vec_first[i]=NULL;
      sptr->ss_vec_last[i]=NULL;

      sptr->raw_vec_qty[i]=0;
      sptr->raw_vec_first[i]=NULL;
      sptr->raw_vec_last[i]=NULL;
      
      sptr->vec_list[i]=NULL;
      }
      
    // init other params
    sptr->stl_vec_first=NULL;
    sptr->gap_first=NULL;
    sptr->p_zero_ref=NULL;
    sptr->p_one_ref=NULL;
    sptr->xmin=0;sptr->ymin=0;
    sptr->xmax=0;sptr->ymax=0;
    sptr->sz_level=(-1);						// set as undefined
    sptr->time_estimate=0.0;
    sptr->eol_pause=FALSE;
    sptr->prev=NULL;
    sptr->next=NULL;
    slice_mem++;
    
    return(sptr);
}

// Function to return a linetype ID based on name
int slice_linetype_findID(slice *sptr, char *ltname)
{
  int		slot,lt_ID=(-1);
  linetype	*lptr;
  
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    lptr=Tool[slot].matl.lt;
    while(lptr!=NULL)
      {
      if(strstr(lptr->name,ltname)!=NULL){lt_ID=lptr->ID;break;}
      lptr=lptr->next;
      }
    }
  
  return(lt_ID);
}

// Function to insert new edge into linked list
int slice_insert(model *mptr, int slot, slice *local, slice *slicenew)
{
  // validate input
  if(mptr==NULL || slicenew==NULL)return(0);
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  
  slicenew->msrc=mptr;							// define source model from which slice came
  if(mptr->slice_qty[slot]==0)						// special case for first slice loaded
    {
    mptr->slice_first[slot]=slicenew;	
    mptr->slice_last[slot]=slicenew;
    }
  else 									// if NOT the first slice loaded then ...
    {
    if(local==mptr->slice_last[slot])					// ... test if insertion is at last element in list
      {
      local->next=slicenew;       
      slicenew->prev=local;	      				
      mptr->slice_last[slot]=slicenew;					
      slicenew->next=NULL;
      }
    else 								// if insertion is NOT at end of list
      {
      slicenew->next=local->next;
      (local->next)->prev=slicenew;
      local->next=slicenew;       	    	   			/* Add link from old to new element. */
      slicenew->prev=local;
      }
    }
  mptr->slice_qty[slot]++;						/* Increment number of elements. */
  return(1);								/* Exit with success. */
}

// Function to delete a single slice from its associated input model
int slice_delete(model *mptr, int slot, slice *sdel)
{
  int		i;
  slice		*sptr,*spre;
    
  if(mptr==NULL || sdel==NULL)return(0);

  // search linked list of slices for this model to find the previous slice to the one to be deleted
  sptr=mptr->slice_first[slot];
  while(sptr!=NULL)
    {
    if(sptr==sdel)break;
    spre=sptr;
    sptr=sptr->next;
    }
    
  // slice could not be found in this model
  if(sptr==NULL)return(0);						
  printf("Slice Delete Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  
  // if this happens to be the first slice...
  if(sptr==mptr->slice_first[slot])
    {
    // reset the pointer into the slice list for this model
    mptr->slice_first[slot]=sptr->next;
    }
  else  
    {
    // set link to skip over deleted slice
    spre->next=sdel->next;						
    }

  for(i=0;i<MAX_LINE_TYPES;i++)
    {
    // purge slice polygon data and their associated verticies
    if(sdel->pfirst[i]!=NULL)
      {
      polygon_purge(sdel->pfirst[i],0);
      sdel->pqty[i]=0;
      sdel->pfirst[i]=NULL;
      sdel->plast[i]=NULL;
      }
    // purge and straight skeleton data
    if(sdel->ss_vec_first[i]!=NULL)
      {
      vector_purge(sdel->ss_vec_first[i]);
      sdel->ss_vec_qty[i]=0;
      sdel->ss_vec_first[i]=NULL;
      sdel->ss_vec_last[i]=NULL;
      }
    }

  
  free(sdel);								// free memory space holding the slice itself
  mptr->slice_qty[slot]--;							// decrement the number of slices held in memory for this model
  slice_mem--;							// decrement the memory counter

  printf("Slice Delete Exit:   vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  return(1);
  }

// Function to copy a slice 
// this function will create and fill all entities needed to make an exact copy
slice *slice_copy(slice *ssrc)
{
  int		i;
  slice		*sdes;
  polygon	*psrc,*pdes,*ppre;

  if(ssrc==NULL)return(NULL);
  
  sdes=slice_make();
  if(sdes==NULL)return(NULL);						// Unable to allocate for some reason
  sdes->ID=(-1);
  sdes->msrc=ssrc->msrc;							// ptr to source model
  for(i=0;i<MAX_LINE_TYPES;i++)						// init line types
    {
    sdes->pdist[i]=ssrc->pdist[i];
    sdes->ptime[i]=ssrc->ptime[i];
    sdes->stype[i]=ssrc->stype[i];

    sdes->perim_vec_qty[i]=ssrc->perim_vec_qty[i];
    sdes->perim_vec_first[i]=ssrc->perim_vec_first[i];
    sdes->perim_vec_last[i]=ssrc->perim_vec_last[i];
  
    sdes->ss_vec_qty[i]=ssrc->ss_vec_qty[i];
    sdes->ss_vec_first[i]=ssrc->ss_vec_first[i];
    sdes->ss_vec_last[i]=ssrc->ss_vec_last[i];

    sdes->raw_vec_qty[i]=ssrc->raw_vec_qty[i];
    sdes->raw_vec_first[i]=ssrc->raw_vec_first[i];
    sdes->raw_vec_last[i]=ssrc->raw_vec_last[i];
    
    sdes->vec_list[i]=ssrc->vec_list[i];

    sdes->pfirst[i]=NULL;
    sdes->plast[i]=NULL;
    sdes->pqty[i]=0;
    if(ssrc->pfirst[i]==NULL)continue;
    sdes->pqty[i]=ssrc->pqty[i];
    ppre=NULL;
    psrc=ssrc->pfirst[i];
    while(psrc!=NULL)
      {
      pdes=polygon_copy(psrc);
      if(sdes->pfirst[i]==NULL)sdes->pfirst[i]=pdes;
      if(ppre!=NULL)ppre->next=pdes;
      ppre=pdes;
      psrc=psrc->next;
      }
    sdes->plast[i]=pdes;
    }
    
  // init other params
  sdes->stl_vec_first=ssrc->stl_vec_first;
  sdes->gap_first=ssrc->gap_first;
  sdes->p_zero_ref=ssrc->p_zero_ref;
  sdes->p_one_ref=ssrc->p_one_ref;
  sdes->xmin=ssrc->xmin; sdes->ymin=ssrc->ymin;
  sdes->xmax=ssrc->xmax; sdes->ymax=ssrc->ymax;
  sdes->sz_level=ssrc->sz_level;	
  sdes->time_estimate=ssrc->time_estimate;
  sdes->eol_pause=ssrc->eol_pause;
  sdes->prev=NULL;							// leave ptrs undefined
  sdes->next=NULL;
  
  return(sdes);
}

// Function to wipe out all of a specific line type at a specific offset distance out of a slice
int slice_wipe(slice *sptr, int ptyp, float odist)
{
  polygon 	*pptr;
  
  // ensure we have something to work with
  if(sptr==NULL)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  //printf("Slice Wipe:  entry   sptr=%X  ptyp=%d  odist=%f z=%f\n",sptr,ptyp,odist,sptr->sz_level);
      
  // purge slice fill polygon data and their associated verticies
  if(sptr->pfirst[ptyp]!=NULL)
    {
    sptr->pfirst[ptyp]=polygon_purge(sptr->pfirst[ptyp],odist);
    }
    
  // recheck qty and set pointer to last
  sptr->pqty[ptyp]=0;
  sptr->plast[ptyp]=NULL;
  pptr=sptr->pfirst[ptyp];
  while(pptr!=NULL)
    {
    sptr->pqty[ptyp]++;
    sptr->plast[ptyp]=pptr;
    pptr=pptr->next;
    }

  //printf("Slice Wipe Exit:   vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  return(1);
}

// Function to purge entire slice list associated to the input model
int slice_purge(model *mptr, int slot)
  {
  int		ptyp,qty;
  slice		*sptr,*sdel;
  vector	*vecptr,*vecdel,*veclast;
  vector_list	*vl_ptr;
  
  // ensure good inbound data
  if(mptr==NULL)return(0);						// don't try to process undefined model
  if(slot<0 || slot>=MAX_TOOLS)return(0);				// slot must be real and in bounds
  
  //printf("\nSlice Purge Entry:  vtx=%ld  vec=%ld  poly=%ld \n",vertex_mem,vector_mem,polygon_mem);
  sptr=mptr->slice_first[slot];
  while(sptr!=NULL)
    {
    sdel=sptr;
    sptr=sptr->next;
    
    //printf("   sptr=%X  sdel=%X   z=%6.3f \n",sptr,sdel,sdel->sz_level);
    
    // purge slice polygon data and vector data along with their associated verticies
    for(ptyp=0;ptyp<MAX_LINE_TYPES;ptyp++)
      {
      if(sdel->pfirst[ptyp]!=NULL)					// purge line type polygon data
	{
	qty=sdel->pqty[ptyp];
	polygon_purge(sdel->pfirst[ptyp],0);
	sdel->pqty[ptyp]=0;
	sdel->pfirst[ptyp]=NULL;
	sdel->plast[ptyp]=NULL;
	//printf("   pfirst[%d]:   qty=%d  vtx=%ld  vec=%ld  poly=%ld \n",ptyp,qty,vertex_mem,vector_mem,polygon_mem);
	}
      if(sdel->perim_vec_first[ptyp]!=NULL)			     	// purge perimeter vector data
	{
	qty=sdel->perim_vec_qty[ptyp];
	vector_purge(sdel->perim_vec_first[ptyp]);
	sdel->perim_vec_qty[ptyp]=0;
	sdel->perim_vec_first[ptyp]=NULL;
	sdel->perim_vec_last[ptyp]=NULL;
	//printf("   perime[%d]:   qty=%d  vtx=%ld  vec=%ld  poly=%ld \n",ptyp,qty,vertex_mem,vector_mem,polygon_mem);
	}
      if(sdel->ss_vec_first[ptyp]!=NULL)				// purge straight skeleton data
	{
	qty=sdel->ss_vec_qty[ptyp];
	vector_purge(sdel->ss_vec_first[ptyp]);
	sdel->ss_vec_qty[ptyp]=0;
	sdel->ss_vec_first[ptyp]=NULL;
	sdel->ss_vec_last[ptyp]=NULL;
	//printf("   ss_vec_fi[%d]:   qty=%d  vtx=%ld  vec=%ld  poly=%ld \n",ptyp,qty,vertex_mem,vector_mem,polygon_mem);
	}
      if(sdel->vec_list[ptyp]!=NULL)					// purge raw vector data
        {
	qty=sdel->raw_vec_qty[ptyp];
	vl_ptr=sdel->vec_list[ptyp];					// start with head node of vec list
	while(vl_ptr!=NULL)						// loop thru all nodes of list
	  {
	  vecptr=vl_ptr->v_item;					// get head vec node for this list
	  vector_purge(vecptr);
	  vl_ptr=vl_ptr->next;
	  }
	sdel->raw_vec_qty[ptyp]=0;
	sdel->raw_vec_first[ptyp]=NULL;
	sdel->raw_vec_last[ptyp]=NULL;
	vector_list_manager(sdel->vec_list[ptyp],NULL,ACTION_CLEAR);
	sdel->vec_list[ptyp]=NULL;
	//printf("   raw_vec_f[%d]:   qty=%d  vtx=%ld  vec=%ld  poly=%ld \n",ptyp,qty,vertex_mem,vector_mem,polygon_mem);
	}
      }
      
    // purge stl vector data
    if(sdel->stl_vec_first!=NULL)vector_purge(sdel->stl_vec_first);
    //printf("   stl_vec_firs:   vtx=%ld  vec=%ld  poly=%ld \n",vertex_mem,vector_mem,polygon_mem);

    free(sdel); slice_mem--;
    }
  mptr->slice_first[slot]=NULL;						// set to indicate this model no longer contains any slice date
  mptr->slice_last[slot]=NULL;
  mptr->slice_qty[slot]=0;

  // purge base layer data
  if(mptr->base_layer!=NULL)
    {
    sdel=mptr->base_layer;
    for(ptyp=0;ptyp<MAX_LINE_TYPES;ptyp++)
      {
      if(sdel->pfirst[ptyp]!=NULL)
	{
	polygon_purge(sdel->pfirst[ptyp],0);
	sdel->pqty[ptyp]=0;
	sdel->pfirst[ptyp]=NULL;
	sdel->plast[ptyp]=NULL;
	}
      }
    free(sdel); slice_mem--;
    mptr->base_layer=NULL;
    printf("   baselayer data purged... \n");
    }

  // purge platform 1 layer data
  if(mptr->plat1_layer!=NULL)
    {
    sdel=mptr->plat1_layer;
    for(ptyp=0;ptyp<MAX_LINE_TYPES;ptyp++)
      {
      if(sdel->pfirst[ptyp]!=NULL)
	{
	polygon_purge(sdel->pfirst[ptyp],0);
	sdel->pqty[ptyp]=0;
	sdel->pfirst[ptyp]=NULL;
	sdel->plast[ptyp]=NULL;
	}
      }
    free(sdel); slice_mem--;
    mptr->plat1_layer=NULL;
    printf("  platform 1 data purged... \n");
    }

  // purge platform 2 layer data
  if(mptr->plat2_layer!=NULL)
    {
    sdel=mptr->plat2_layer;
    for(ptyp=0;ptyp<MAX_LINE_TYPES;ptyp++)
      {
      if(sdel->pfirst[ptyp]!=NULL)
	{
	polygon_purge(sdel->pfirst[ptyp],0);
	sdel->pqty[ptyp]=0;
	sdel->pfirst[ptyp]=NULL;
	sdel->plast[ptyp]=NULL;
	}
      }
    free(sdel); slice_mem--;
    mptr->plat2_layer=NULL;
    printf("  platform 2 data purged... \n");
    }

  //printf("Slices purged for model %X \n",mptr);
  
  //printf("Slice Purge Exit:   vtx=%ld  vec=%ld  poly=%ld \n",vertex_mem,vector_mem,polygon_mem);
  return(1);
  }

// Function to display slice information for debug purposes
void slice_dump(slice *sptr)
{
  int 		i;
  polygon	*pptr;
  
  printf("SLICE DUMP OF %X ---------------------------------------------\n",sptr);
  for(i=0;i<MAX_LINE_TYPES;i++)
    {
    if(sptr->pfirst[i]!=NULL)
      {
      pptr=sptr->pfirst[i];
      printf("LT %d first.........: %X \n",i,sptr->pfirst[i]);		// pointer to first polygon in this slice
      printf("LT %d last..........: %X \n",i,sptr->plast[i]);		// pointer to last polygon in this slice
      printf("LT %d quantity......: %ld / %ld\n",i,sptr->pqty[i],pptr->vert_qty); // quantity of polygons in this slice, and vtxs in the first polygon
      printf("skeleton first......: %X \n",sptr->ss_vec_first[i]);	// pointer to first skeleton vector in this slice (note: open ended polygons)
      printf("skeleton last.......: %X \n",sptr->ss_vec_last[i]);	// pointer to last skeleton vector in this slice
      printf("skeleton qty........: %ld \n",sptr->ss_vec_qty[i]);	// quantity of skeleton vectors in this slice
      while(pptr!=NULL)
        {
	polygon_dump(pptr);
	pptr=pptr->next;
	}
      }
    }
  printf("xmin = %f   ymin = %f \n",sptr->xmin,sptr->ymin);		// minimum values for this slice
  printf("xmax = %f   ymax = %f \n",sptr->xmax,sptr->ymax);		// minimum values for this slice
  printf("sz_level........ = %f \n",sptr->sz_level);			// z_level for this slice
  printf("prev................: %X \n",sptr->prev);
  printf("next................: %X \n",sptr->next);
  
  printf("--------------------------------------------------------------\n");
  while(!kbhit());
  
  return;
}

// Function to scale the contents of a slice in xy about the center of its source model
void slice_scale(slice *ssrc, float scale)
{
    int		ptyp;
    float 	xcen,ycen;
    vertex 	*vptr;
    polygon	*pptr;
    slice 	*sptr;
    model 	*mptr;
    
    // validate inputs
    if(ssrc==NULL)return;
    
    // calc center point to scale about
    mptr=ssrc->msrc;
    xcen=(mptr->xmax[MODEL]-mptr->xmin[MODEL])/2;
    ycen=(mptr->ymax[MODEL]-mptr->ymin[MODEL])/2;
    
    // apply scale value
    for(ptyp=0;ptyp<MAX_LINE_TYPES;ptyp++)
      {
      pptr=ssrc->pfirst[ptyp];
      while(pptr!=NULL)
	{
	vptr=pptr->vert_first;
	while(vptr!=NULL)
	  {
	  vptr->x=(scale*(vptr->x-xcen)+xcen);
	  vptr->y=(scale*(vptr->y-ycen)+ycen);
	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  }
	pptr=pptr->next;
	}
      }
      
    
    return;
}

// Function to find a specific slice of a specific model at a specific z level
// note:  zlvl is the actual z position in the build volume, but models are all sliced as if their lowest point is at z=0
//        this allows vertical changes in the model to be done with mptr->zoff in a similar manner as x and y.
// 	  so to display/print the returned slice that is found, mptr->zoff has to be added back it's level
// note:  if include_btm_detail is FALSE then the first slice of the model (at z=0.000) will just be over ridden
//	  by either the baselayer or just not returned
slice *slice_find(model *mptr, int mtyp, int slot, float zlvl, float delta)
{
  slice	*sptr;
  
  //debug_flag=217;
  if(mptr==NULL)return(NULL);
  if(slot<0 || slot>=MAX_TOOLS)return(NULL);
  
  // if a vector marking job, there is only one slice
  if(job.type==MARKING){return(mptr->slice_first[MODEL]);}
  
  zlvl-=mptr->zoff[mtyp];						// adjust to bottom of model

  if(debug_flag==217)printf("\nSF:  mptr=%X  mtyp=%d  slot=%d  zoff=%f  lvl=%f  delta=%f \n\n",mptr,mtyp,slot,mptr->zoff[mtyp],zlvl,delta);

  if(zlvl<CLOSE_ENOUGH)							// if at very bottom...
    {
    if(job.baselayer_flag==TRUE)					// ... and baselayer active, return baselayer
      {
      return(mptr->base_layer);
      }
    }

  if(zlvl<mptr->zmin[mtyp] || zlvl>mptr->zmax[mtyp])
    {
    if(debug_flag==217)printf("SF: outside of model bounds  zmin=%f  zmax=%f  zlvl=%f \n\n",mptr->zmin[mtyp],mptr->zmax[mtyp],zlvl);
    return(NULL);	// check if z level within model z bounds
    }
  
  sptr=mptr->slice_first[slot];						// loop thru all slices of this model
  while(sptr!=NULL)
    {
    if(debug_flag==217)printf("SF: Checking: %X  slice_z=%f  against  z_cut=%f  with  delta=%f \n",sptr,sptr->sz_level,zlvl,delta);
    if(fabs(sptr->sz_level-zlvl)<delta)break;				// note both sz_level and zlvl are relative to z=0 here
    sptr=sptr->next;
    }
  if(debug_flag==217 && sptr!=NULL)printf("SF:  sptr=%X  sz_level=%5.3f  zlvl=%5.3f\n\n",sptr,sptr->sz_level,zlvl);

  return(sptr);
}

// Function to find the model that a slice belongs to
model *slice_get_model(slice *sinp)
{
    int		slot;
    slice 	*sptr;
    model	*mptr;
    
    // validate inputs
    if(sinp==NULL)return(NULL);
    
    // loop thru model list and compare slice addresses at z of input slice
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);			// get slot used to build this model
      sptr=slice_find(mptr,MODEL,slot,sinp->sz_level,LEVEL);			// get ptr to slice of this model at this z
      if(sptr==sinp)break;						// if they match... we are done
      mptr=mptr->next;
      }
  
    return(mptr);
}

// Function to recount polygons contained within it
int slice_polygon_count(slice *sptr)
{
  int		i,count;
  polygon	*pptr;
  
  if(sptr==NULL)return(0);
  
  for(i=0;i<MAX_LINE_TYPES;i++)
    {
    count=0;
    pptr=sptr->pfirst[i];
    while(pptr!=NULL)
      {
      count++;
      pptr=pptr->next;
      }
    sptr->pqty[i]=count;
    }
  
  return(1);
}

// Function to find the outermost polygon in a list of polygons
polygon *slice_find_outermost(polygon *pinpt)
{
  int		inside_count,min_count;
  polygon 	*pouter,*pptr,*pnxt;
  vertex 	*vptr;
 
  pouter=NULL;
  if(pinpt==NULL)return(NULL);
  
  // loop thru poly list and find the poly with vtxs that are not inside other polygons
  min_count=1000;
  pptr=pinpt;
  while(pptr!=NULL)
    {
    inside_count=0;
    vptr=pptr->vert_first;
    pnxt=pinpt;
    while(pnxt!=NULL)
      {
      if(pnxt!=pptr && polygon_contains_point(pnxt,vptr)==TRUE)inside_count++;
      pnxt=pnxt->next;
      }
    if(inside_count<min_count){min_count=inside_count;pouter=pptr;}
    pptr=pptr->next;
    }
  
  return(pouter);
}

// Function to establish the max/min boundaries of a slice
int slice_maxmin(slice *sptr)
{
  int			i;
  vertex		*vptr;
  polygon		*pptr;

  // Verify inbound data
  if(sptr==NULL)return(0);
  
  //printf("  slice Max/Min: enter\n");

  // re-establish max min boundaries of this specific slice
  sptr->xmin=10000;sptr->ymin=10000;
  sptr->xmax=(-10000);sptr->ymax=(-10000);
  for(i=0;i<MAX_LINE_TYPES;i++)						// loop thru all types of polygons
    {
    pptr=NULL;
    pptr=sptr->pfirst[i];						
    //printf("  pptr=%X  line_type=%d  ",pptr,i);
    while(pptr!=NULL)							// loop thru all polygons in this type
      {
      //printf(" first=%X  qty=%d \n",pptr->vert_first,pptr->vert_qty);
      vptr=pptr->vert_first;
      while(vptr!=NULL)							// loop thru all verticies in this polygon
	{
	if(vptr->x < sptr->xmin)sptr->xmin=vptr->x;
	if(vptr->y < sptr->ymin)sptr->ymin=vptr->y;
	if(vptr->x > sptr->xmax)sptr->xmax=vptr->x;
	if(vptr->y > sptr->ymax)sptr->ymax=vptr->y;
	//printf("  vptr=%X  next=%X  x=%f y=%f z=%f\n",vptr,vptr->next,vptr->x,vptr->y,vptr->z);
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      pptr=pptr->next;
      }
    }
  
  //printf("  xmin=%f  ymin=%f  xmax=%f  ymax=%f\n",sptr->xmin,sptr->ymin,sptr->xmax,sptr->ymax);
  //printf("  slice Max/Min: exit\n");

  return(1);
}

// Function to rotate only a specific line type of a single slice about its center by rotating the vertex
// coordinates about the slice's centroid.
// Inputs are in degrees.
int slice_rotate(slice *sptr, float rot_angle, int typ)
{
  int		mtyp=MODEL;
  vertex	*vptr;
  polygon 	*pptr;
  float 	xc,yc;							// calculated centroid of slice to be rotated
  double 	xv,yv;
  model		*mptr;
  
  //printf("\nSlice Rotation: sptr=%X  angle=%6.3f  type=%d \n",sptr,rot_angle,typ);
  
  // nothing to process
  if(sptr==NULL)return(0);
  if(typ<0 || typ>MAX_LINE_TYPES)return(0);
  if(sptr->pfirst[typ]==NULL)return(0);
  //printf("  pfirst=%X  pqty=%d \n",sptr->pfirst[typ],sptr->pqty[typ]);
  
  // use source model center as rotation point
  mptr=sptr->msrc;
  if(mptr!=NULL)
    {
    xc= (mptr->xmax[mtyp] + mptr->xmin[mtyp])/2.0;
    yc= (mptr->ymax[mtyp] + mptr->ymin[mtyp])/2.0;
    }
  else 
    {
    xc= (sptr->xmax + sptr->xmin)/2.0;
    yc= (sptr->ymax + sptr->ymin)/2.0;
    }
    
  // convert incoming degees into radians
  rot_angle *= PI/180.0;
	    
  // rotate the slice about z axis	  
  if(fabs(rot_angle)>TOLERANCE)
    {
    pptr=sptr->pfirst[typ];
    while(pptr!=NULL)
      {
      // rotate entire vertex list
      vptr=pptr->vert_first;
      while(vptr!=NULL)							// if an open-ended polygon...
	{
	vptr->x -= xc;							// shift center to zero
	vptr->y -= yc;

	xv = vptr->x*cos(rot_angle)-vptr->y*sin(rot_angle);		// perform rotation
	yv = vptr->x*sin(rot_angle)+vptr->y*cos(rot_angle);

	vptr->x = xv+xc;						// shift back to original location
	vptr->y = yv+yc;

	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;				// if a closed-loop polygon...
	}
      pptr=pptr->next;
      }
    }

  return(1);
}

// Function to set a specific slice's vertex attributes
int slice_set_attr(int slot, float zlvl, int newattr)
{
  int		i;
  vertex	*vptr;
  polygon	*pptr;
  slice		*sptr;
  model		*mptr;
  
  if(slot<0 || slot>=MAX_TOOLS)return(0);
  if(ZMin<BUILD_TABLE_MIN_Z || ZMax>BUILD_TABLE_MAX_Z)return(0);
  if(zlvl<ZMin || zlvl>ZMax)return(0);
  
  mptr=job.model_first;							// start at first model for this tool
  while(mptr!=NULL)							// loop thru all models of this tool
    {
    sptr=mptr->slice_first[slot];					// start at first slice of this model
    while(sptr!=NULL)							// loop thru all available slices
      {
      if(fabs(sptr->sz_level-zlvl)<TOLERANCE)				// if this one matches the requested z level...
        {
	for(i=0;i<MAX_LINE_TYPES;i++)
	  {
	  pptr=sptr->pfirst[i];						// start at the first polygon of the offset list
	  while(pptr!=NULL)
	    {
	    vptr=pptr->vert_first;
	    while(vptr!=NULL)						// loop thru all vertices and change attribute
	      {
	      vptr->attr=newattr;
	      vptr=vptr->next;
	      if(vptr==pptr->vert_first)break;
	      }
	    pptr=pptr->next;
	    }
	  }
	}
      sptr=sptr->next;
      }
    mptr=mptr->next;
    }
  
  
  return(1);
}
  
// Function to determine if a point is in material or in a hole via winding number method.
// It calls the polygon_contains_point function for all polygons in the slice, but will ignore the
// polygon that is the source of the test point (if set).
int slice_point_in_material(slice *sptr, vertex *vtest, int ptyp, float odist)
{
  int		i,pip,windnum;
  float 	dx,dy,lside;
  vertex	*vptr,*vnxt,*vinf,*vint; 
  polygon	*pptr;
  vector 	*vec_test_line,*vec_poly;
  
  // Verify inbound data
  if(sptr==NULL || vtest==NULL)return(FALSE);

  windnum=0;								// respresents the number of polygons winding around our test point
  pptr=sptr->pfirst[ptyp];
  while(pptr!=NULL)							// loop thru all polygons in this slice of this line type
    {
    if(pptr->dist!=odist){pptr=pptr->next;continue;}			// only look at polygons at this same offset disttance
    if(pptr->ID==vtest->supp){pptr=pptr->next;continue;}		// don't check against its originating polygon
    if(polygon_contains_point(pptr,vtest)==TRUE)windnum++;		// increment if inside polygon
    pptr=pptr->next;
    }
  
  vtest->attr=windnum;
  if(windnum%2==0)return(FALSE);					// if even, then inside hole
  return(TRUE);								// if odd, then inside material
}

// Function to determine the minimum distance from a point to the boundary of a slice.
// returns the minium 2D distance from input vtx to slice perimeter.  negative value
// means the point is inside the material of the slice.
float slice_point_near_material(slice *sinpt, vertex *vinpt, int ptyp)
  {
  int		inside_matl=FALSE;
  float 	crt_dist,min_dist;  
  vertex 	*vtx_dist;
  polygon 	*pptr;  
    
  // validate inputs
  if(sinpt==NULL || vinpt==NULL)return(0);
  if(ptyp<0 || ptyp>=MAX_LINE_TYPES)return(0);  
    
  min_dist=1000;
  vtx_dist=vertex_make();
  pptr=sinpt->pfirst[ptyp];
  while(pptr!=NULL)
    {
    crt_dist=polygon_to_vertex_distance(pptr, vinpt, vtx_dist);
    if(fabs(crt_dist)<min_dist)
      {
      min_dist=fabs(crt_dist);
      if(crt_dist<0){inside_matl=TRUE;}
      else {inside_matl=FALSE;}
      }
    pptr=pptr->next;
    }
  vertex_destroy(vtx_dist);
  
  if(inside_matl==TRUE)min_dist *= (-1);
  //printf("\n  Slice_point_near_material:  exit  dist=%6.3f \n",min_dist);
  
  return(min_dist);
  }

// Function to generate a slice that represents the bounds of a model.
slice *slice_model_bounds(model *mptr,int mtyp, int ptyp)
{
  vertex 	*vtx0,*vtx1,*vtx2,*vtx3;
  polygon 	*pptr;
  slice 	*sptr;
  
  // validate inputs 
  if(mptr==NULL)return(NULL);
  
  // generate slice 
  vtx0=vertex_make();
  vtx0->x=mptr->xmin[mtyp]; vtx0->y=mptr->ymin[mtyp]; vtx0->z=0;
  vtx1=vertex_make();
  vtx1->x=mptr->xmax[mtyp]; vtx1->y=mptr->ymin[mtyp]; vtx1->z=0;
  vtx2=vertex_make();
  vtx2->x=mptr->xmax[mtyp]; vtx2->y=mptr->ymax[mtyp]; vtx2->z=0;
  vtx3=vertex_make();
  vtx3->x=mptr->xmin[mtyp]; vtx3->y=mptr->ymax[mtyp]; vtx3->z=0;
  
  vtx0->next=vtx1;
  vtx1->next=vtx2;
  vtx2->next=vtx3;
  vtx3->next=vtx0;
  
  pptr=polygon_make(vtx0,ptyp,0);
  pptr->vert_qty=4;
  sptr=slice_make();
  sptr->pfirst[ptyp]=pptr;
  sptr->pqty[ptyp]=1;
  
  return(sptr);
}

// Function to fill a slice with straight lines
//
// inputs:
//	blw_sptr = ptr to slice below the current (used as boolean comparison)
//	crt_sptr = ptr to current z level slice (the one that will recieve fill vectors)
//	abv_sptr = ptr to slice above the current (used as boolean comparison)
//	scanstep = pitch between fill lines
//	scanangle = angle of fill lines relative to x-axis
//	ConTyp = contour line type that will be used as boundary for fill
// 	FillTyp = resulting line type for fill lines going into current z level slice
//	BoolAction = what to do with the slices (see code below)

int slice_linefill(slice *blw_sptr,slice *crt_sptr,slice *abv_sptr,float scanstep,float scanangle,int ConTyp,int FillTyp,int BoolAction)
{
  int		mtyp=MODEL;
  vertex	*hc_string, *int_string;				// pointer to crossing fill line and intersections generated
  vertex	*vptr,*vold,*vint,*vnew,*vbrk,*vint_pos,*vsrt;
  int		ix=0,iy=0,istatblw,istatcrt,istatabv,icros=0,int_flag=0;	// counters across hc string
  int 		sc_count=0;
  int		y_odd=0;						// counter to define honey comb direction
  float 	x_inc,y_inc;						// increment values
  float 	x_old,y_old;						// previous increment values
  float 	x_pos;							// horizontal position of hc string as it is generated
  float 	y_pos;							// vertical position of hc string as it sweeps across build table
  float 	tool_line_width=0.25;					// should be replace by proper reference from tool/material
  float 	fill_pitch;
  polygon	*pptr,*pnew;
  vector	*A,*B,*C;
  slice 	*slpr;
  model 	*mptr;
  int		slot;
  int		slice_ctr=0;						// counts the number of slices fill is taking into account
  int		swap=0;							// flag to indicate if vertex should be swapped
  int		status=0;
  int 		ltype;
  int		optyp;
  int		h,i,j,vint_result,in_flag=0,skip,steps;
  float 	scanlow,scanhigh,scanpos,mdelta;
  float   	maxdist,tempdist,blwmaxdist,blwmindist,crtmaxdist,crtmindist,abvmaxdist,abvmindist;
  float 	dx,dy,dist,veclen;
  linetype	*lptr;
  
  // ensure we have useable data coming in
  // note abv_sptr can possibly be NULL near tops of models
  if(crt_sptr==NULL || scanstep<TOLERANCE)return(0);

  // need to ensure a contour is available to work with
  status=0;
  if(crt_sptr->pfirst[ConTyp]!=NULL)status=1;
  if(!status)return(status);

  //printf("\nLinefill Entry:  crt_sptr=%X  abv_sptr=%X  z=%f\n",crt_sptr,abv_sptr,crt_sptr->sz_level);
  //printf("  scanstep=%6.3f  scanangle=%6.3f   ConTyp=%d FilTyp=%d Act=%d\n",scanstep,scanangle,ConTyp,FillTyp,BoolAction);
  
  // find the most "inside" offset polygon group to fill from and save that distance to use as a filter 
  // for which polygons to intersect with the scanline.  unless, intnernal/support which has no offsets OR subtractive
  // in which case the roles of maxdist/mindist are reversed.
  blwmaxdist=(-1000.0); blwmindist=1000.0;
  if(blw_sptr!=NULL)
    {
    pptr=blw_sptr->pfirst[ConTyp];
    while(pptr!=NULL)
      {
      if(pptr->dist>blwmaxdist)blwmaxdist=pptr->dist;
      if(pptr->dist<blwmindist)blwmindist=pptr->dist;
      pptr=pptr->next;
      }
    }
    
  // find the most "outside" offset polygon group to fill from and save that distance to use as a filter 
  // for which polygons to intersect with the scanline.  unless, internal/support which has no offsets OR subtractive
  // in which case the roles of maxdist/mindist are reversed.
  crtmaxdist=(-1000.0); crtmindist=1000.0;
  pptr=crt_sptr->pfirst[ConTyp];
  while(pptr!=NULL)
    {
    if(pptr->dist>crtmaxdist)crtmaxdist=pptr->dist;
    if(pptr->dist<crtmindist)crtmindist=pptr->dist;
    pptr=pptr->next;
    }
    
  // When applying which set of polygons (line types) to use, realize that it is a time saver to compare
  // the inner-most to the outer-most when doing close-offs.  That is, for upper close-off, the lower z
  // value slice should use the outer-most polygon set to compare against the higher z value slice using
  // the inner-most set.  Typically this means comparing the inner-most offset line type to the outer-most
  // border to generate the necessary fill.  Just the opposite for lower close-offs.
  abvmaxdist=(-1000.0); abvmindist=1000.0;
  if(abv_sptr!=NULL)
    {
    pptr=abv_sptr->pfirst[ConTyp];
    while(pptr!=NULL)
      {
      if(pptr->dist>abvmaxdist)abvmaxdist=pptr->dist;
      if(pptr->dist<abvmindist)abvmindist=pptr->dist;
      pptr=pptr->next;
      }
    }
    
  // we always sweep from bottom (ymin) to top (ymax) and rotate the contour to generate fills off horizontal
  slice_rotate(crt_sptr,scanangle,ConTyp);				// rotate the contour we're using
  if(blw_sptr!=NULL && blw_sptr!=crt_sptr)slice_rotate(blw_sptr,scanangle,ConTyp);
  if(abv_sptr!=NULL && abv_sptr!=crt_sptr)slice_rotate(abv_sptr,scanangle,ConTyp);
  
  // at this point the following control parameters have been defined to generate the fill:
  //	hc_string = the actual scan line made up of a linked-list of 2 verticies that span the entire slice
  //	scanstep = the increment to make in the sweep direction (i.e. perpendicular to the hc_string vector)
  //	scanlow = the lower limit of where to start the scan
  //	scanhigh = the upper limit of where the scan should stop
  
  // loop from bottom of build table Y to top of build table Y, but where we start should be relative to
  // the dimensions of the model.  this bit of code sets the starting positions relative to the center of
  // the model.  the idea is that this will provide the most logical structural support.
  veclen=0.25;								// set default minimum vector length
  mptr=crt_sptr->msrc;							// get source model
  if(mptr!=NULL)
    {
    mdelta=(mptr->ymax[mtyp] - mptr->ymin[mtyp])*2.83;			// get width of model turned at scanangle
    steps=(int)(mdelta/scanstep+0.5);					// calc number of scan steps needed
    scanlow = (mptr->ymax[mtyp]+mptr->ymin[mtyp])/2 - scanstep*(steps/2 + 1);	// calc start scan location
    scanhigh = scanlow + scanstep*(steps+2);				// calc stop scan location
    
    // set minimum vec length
    if(FillTyp>=MDL_PERIM && FillTyp<=MDL_LAYER_1)optyp=OP_ADD_MODEL_MATERIAL;
    if(FillTyp>=INT_PERIM && FillTyp<=INT_LAYER_1)optyp=OP_ADD_MODEL_MATERIAL;
    if(FillTyp>=BASELYR && FillTyp<=PLATFORMLYR2)optyp=OP_ADD_BASE_LAYER;
    if(FillTyp>=SPT_PERIM && FillTyp<=SPT_LAYER_1)optyp=OP_ADD_SUPPORT_MATERIAL;
    slot=model_get_slot(mptr,optyp);					// get source slot
    if(slot>=0 && slot<MAX_TOOLS)					// if a valid slot...
      {
      lptr=linetype_find(slot,FillTyp);					// ... get line type ptr
      if(lptr!=NULL)veclen=lptr->minveclen;				// ... if a valid ptr, set to proper min vec length
      }
    }
  else 
    {
    scanlow=0-BUILD_TABLE_MAX_Y;
    scanhigh=2*BUILD_TABLE_MAX_Y;
    }
  scanpos=scanlow;							// set starting scan position
  while(scanpos<scanhigh)						// loop across entire scan distance
    {
    // increment scan line position
    scanpos+=scanstep;
 
    // construct scan line vector
    hc_string=NULL;
    vptr=vertex_make();
    vptr->x=0-BUILD_TABLE_MAX_X;
    vptr->y=scanpos;
    vptr->z=0.0;
    vptr->next=NULL;
    hc_string=vptr;		
    vptr=vertex_make();
    vptr->x=2*BUILD_TABLE_MAX_X;
    vptr->y=scanpos;
    vptr->z=0.0;
    vptr->next=NULL;
    hc_string->next=vptr;	
      
    // at this point a straight line that runs across the slice has been formed and held in hc_string
    // now it must be intersected with the polygon group to figure out the actual fill segments
    // do this by looping thru the hc vertex string and finding intersections with each polygon segment then
    // sorting and counting winding numbers

    // find intersections of the scanline with polygon segments and create list of them stored in int_string
    int_string=NULL;
    vnew=vertex_make();							// temporary vertex to hold intersection values
    vold=hc_string;
    vptr=hc_string;
    do{									// loop thru all verticies in hc fill string
      A=vector_make(vptr,vptr->next,1);					// create scanline vector from hc vertex pair
      slice_ctr=1;							// init the slice counter
      while(slice_ctr<4)						// loop thru the selected slices
        {
	ltype=ConTyp;
	//if(slice_ctr==1){slpr=blw_sptr; maxdist=blwmaxdist;}
	if(slice_ctr==1){slpr=blw_sptr; maxdist=blwmindist;}
	if(slice_ctr==2){slpr=crt_sptr; maxdist=crtmaxdist;}
	if(slice_ctr==3){slpr=abv_sptr; maxdist=abvmaxdist;}
	slice_ctr++;							// increment the slice counter
	if(slpr==NULL)continue;						// if nothing for this slice, move onto next
	pptr=slpr->pfirst[ltype];					// start with all of the determined contour line type
	while(pptr!=NULL)						// loop thru all polys in this slice and the next slice
	  {
	  skip=TRUE;
	  if(fabs(pptr->dist-maxdist)<TOLERANCE)skip=FALSE;		// if this polygon is not at the maximum offset distance...
	  if(FillTyp==BASELYR || FillTyp==PLATFORMLYR1 || FillTyp==PLATFORMLYR2)skip=FALSE;
	  if(pptr->fill_type<=0)skip=TRUE;				// if fill undefined or "none", skip
	  if(skip==TRUE){pptr=pptr->next; continue;}			// then we are going to skip it.
	  vint=pptr->vert_first;					// if this polygon is at the max offset distance, init our first vertex...
	  do{
	    if((vint->y < scanpos) && (vint->next->y < scanpos))	// if there is no chance of intersection because too low...
	      {
	      vint=vint->next;						// just move onto next segment
	      continue;
	      }
	    if((vint->y > scanpos) && (vint->next->y > scanpos))	// if there is no chance of intersection because too high...
	      {
	      vint=vint->next;						// just move onto next segment
	      continue;
	      } 
	    B=vector_make(vint,vint->next,2);				// make a vector from this segment of polygon
	    vint_result=vector_intersect(A,B,vnew);			// determine intersection bt A (fill line) and B (polygon edge)
	    if(vint_result==204 || vint_result==206 || vint_result==220 || vint_result==221 || vint_result==222)
	      {			
	      //printf("             Intersection found.  lt=%d  scanpos=%f  int_string=%X\n",ConTyp,scanpos,int_string);
	      //printf("  vint=%d \n",vint_result);
	      vbrk=vertex_make();					// create a new vertex to hold this intersection's coords
	      vbrk->x=vnew->x;
	      vbrk->y=vnew->y;
	      vbrk->z=vnew->z;						
	      vbrk->attr=slice_ctr-1;					// set attribute to slice number that intersected it
	      vbrk->next=NULL;
	      if(int_string==NULL)					// if the intersection string is empty...
	        {
		int_string=vbrk;					// define this as the first vertex in the intersection string
		vint_pos=int_string;					// define pointer position along intersection string
		}
	      else 							// if the intersection string is not empty...
	        {
		vint_pos->next=vbrk;					// add new intersection to end of intersection string  
		vint_pos=vbrk;						// increment the vertex intersection pointer to last element
		}
	      }
	    vint=vint->next;
	    vector_destroy(B,FALSE);
	    }while(vint!=pptr->vert_first);
	  pptr=pptr->next;						// move onto the next polygon in this slice
	  }
	slpr=slpr->next;						// move onto the next slice above the current
	}
      vector_destroy(A,FALSE);

      // at this point we have a list of verticies for this one scanline that represent all the intersections found between
      // all the inner most polygons of all slices and the scanline.  this list is held in int_string, but is in random order.
      // the list needs to be sorted left to right so that odd/even counting can be done.
      //if(FillTyp==SPT_FILL)printf("  Linefill:  Starting intersection sort...\n");

      if(int_string!=NULL)						// if the intersection string is not empty...
	{
	// sort the entire int string by X value lowest to highest (i.e. orthagonal to scan sweep direction)
  	vsrt=int_string;
	while(vsrt!=NULL)
	  {
	  vint=vsrt->next;
	  while(vint!=NULL)
	    {
	    // note it is easier to swap values than addresses (swapping addresses will mess up the loops).
	    // note we swap all values except for the link to "next" so the list stays in tact.
	    if(vint->x < vsrt->x)
	      {
	      vold->x=vsrt->x; vold->y=vsrt->y; vold->z=vsrt->z; vold->attr=vsrt->attr;
	      vsrt->x=vint->x; vsrt->y=vint->y; vsrt->z=vint->z; vsrt->attr=vint->attr;
	      vint->x=vold->x; vint->y=vold->y; vint->z=vold->z; vint->attr=vold->attr;
	      }
	    vint=vint->next;
	    }
	  vint_pos=vsrt;						// this will catch the address of the last element in the intersection list before the loop ends
	  vsrt=vsrt->next;
	  }
	}
	
      vptr=vptr->next;							// move onto next line segment of hc fill line string (could be multi segmented)
      }while(vptr->next!=NULL);						// end when we've done all of the hc fill string segments
      
    vertex_destroy(vnew);						// delete temporary vertex from memory

    // since we are done with hc_string (the scanline) it can be removed
    vint=hc_string->next;
    vertex_destroy(hc_string);
    vertex_destroy(vint);
 
    // at this point the int_string is now broken into segments that are either completely inside polygons or completely outside.
    // now we can count the winding number from left to right and keep what is inside and throw away whats outside.
    // but it is important to consider which slice we are crossing and when. ultimately we want to know if we are
    // exclusively inside the area of an individual polygon or a shared area of polygons from different slices.

    //pnew=polygon_make(int_string,MDL_FILL,0);				// create a new polygon from it
    //polygon_insert(crt_sptr,MDL_FILL,pnew);  				// insert it into the linked list of FILL polygons
    
    //if(FillTyp==SPT_FILL)printf("  Linefill:  Starting winding number search...\n");

    // zero out winding number for each slice
    istatblw=0;
    istatcrt=0;
    istatabv=0;
    in_flag=0;
    
    //printf("\nSFill:  ");
    vptr=int_string;
    vold=int_string;
    while(vptr!=NULL)
      {
      // increment winding numbers
      h=0;
      if(vptr->attr==1)istatblw++;					// if this vtx was created as a result of intersecting blw_sptr polygons... increment winding number.
      if(vptr->attr==2)istatcrt++;					// if this vtx was created as a result of intersecting crt_sptr polygons... increment winding number.
      if(vptr->attr==3)istatabv++;					// if this vtx was created as a result of intersecting abv_sptr polygons... increment winding number.
      
      // based on "BoolAction" bit setting and the winding number a transition will be determined
      //	ACTION_OR  = fill everything inclusive of blw_sptr, crt_sptr, and abv_sptr
      //	ACTION_AND = fill only the parts of crt_sptr that DO overlap blw_sptr AND abv_sptr
      //	ACTION_XOR = fill only the parts of crt_sptr that do NOT overlap with blw_sptr or abv_sptr
      // an odd number of istat means that it is inside material
      if(BoolAction==ACTION_OR)
        {
	if(istatblw%2!=0 || istatcrt%2!=0 || istatabv%2!=0)h=1;		// if we are inside any of the slice polys, keep the fill line
	}
      if(BoolAction==ACTION_AND)
        {
	if(blw_sptr!=NULL && abv_sptr!=NULL)
	  {if(istatblw%2!=0 && istatcrt%2!=0 && istatabv%2!=0)h=1;}	// only keep what is in crt if also in blw and abv
	else if(blw_sptr==NULL && abv_sptr!=NULL)
	  {if(istatcrt%2!=0 && istatabv%2!=0)h=1;}			// only keep what is in crt if also in abv
	else if(blw_sptr!=NULL && abv_sptr==NULL)
	  {if(istatblw%2!=0 && istatcrt%2!=0)h=1;}			// only keep what is in crt if also in blw
	else if(blw_sptr==NULL && abv_sptr==NULL)
	  {if(istatcrt%2!=0)h=1;}					// only keep what is in crt
	}
      if(BoolAction==ACTION_XOR)
        {
	if(blw_sptr!=NULL && abv_sptr!=NULL)
	  {if(istatblw%2==0 && istatcrt%2!=0 && istatabv%2==0)h=1;}	// only keep what is in crt if also not in blw and abv
	else if(blw_sptr==NULL && abv_sptr!=NULL)
	  {if(istatcrt%2!=0 && istatabv%2==0)h=1;}			// only keep what is in crt and not in abv
	else if(blw_sptr!=NULL && abv_sptr==NULL)
	  {if(istatblw%2==0 && istatcrt%2!=0)h=1;}			// only keep what is in crt and not in blw
	else if(blw_sptr==NULL && abv_sptr==NULL)
	  {if(istatcrt%2!=0)h=1;}					// only keep what is in crt
	}
	
      // if this is a transition into what we want to keep...
      if(h==1 && in_flag==0)						
        {
	//printf(" Ti:%d:%d ",istat1,istat2);
	in_flag=1;							// ... set flag to indicate we are inside
	vold=vptr;							// ... save address of previous vertex
	vptr=vptr->next;						// ... move onto the next vertex
	continue;
	}
      
      // if this is a transition out of what we want to keep...
      if(h==0 && in_flag==1)						
        {
	//printf(" To:%d:%d ",istat1,istat2);
	in_flag=0;							// ... set flag to indicate we are outside
	vold=vptr;							// ... save address of previous vertex
	vptr=vptr->next;						// ... move onto the next vertex
	continue;
	}

      // at this point the vertex is NOT a transition.
      // we are either inside or outside, but either way we delete it.
      // just a little tricky to maintain the integrity of the linked list.
      
      // if this is the very first vertex in the linked list...
      //printf(" De:%d:%d ",istat1,istat2);
      if(vptr==int_string)						
        {
	int_string=vptr->next;						// ... set the start of the string to the next vtx
	vertex_destroy(vptr);						// ... get rid of this vtx
	vptr=int_string;						// ... reset our current pointer to the start of the string
	vold=vptr;							// ... save address of "previous" vertex (sorta)
	continue;
	}
	
      // if this is the very last vertex in the linked list...
      if(vptr->next==NULL)						
        {
	vold->next=NULL;						// ... set the prev vtx to terminate the string
	vertex_destroy(vptr);						// ... get rid of this vtx
	break;
	}

      // otherwise this vtx is in the middle of the string...
      vold->next=vptr->next;						// ... set prev vtx to skip over the one we are about to delete
      vertex_destroy(vptr);						// ... get rid of this vtx
      vptr=vold->next;							// ... reset our vertex to the next one in list, vold stays the same
      }
    
    //pnew=polygon_make(int_string,MDL_FILL,0);				// create a new polygon from it
    //polygon_insert(crt_sptr,MDL_FILL,pnew);  				// insert it into the linked list of FILL polygons


    // now make polygon segments out of every other pair of verticies provided the segment is long enough
    vptr=int_string;
    while(vptr!=NULL)
      {
      vint=vptr->next;							// get next vertex to make our pair (vptr, vint) for this segment
      if(vint==NULL)break;						// shouldn't happen if in pairs... but maybe not
      dx=vptr->x-vint->x;
      dy=vptr->y-vint->y;
      dist=sqrt(dx*dx+dy*dy);						// calculate length
      vold=vptr;							// save start address of current vtx
      vptr=vint->next;							// reset current vtx to next pair
      if(dist>veclen)							// if this vector is long enough...
	{
	vint->next=NULL;						// terminate this pair
	pnew=polygon_make(vold,FillTyp,0);				// create a new polygon from it
	polygon_insert(crt_sptr,FillTyp,pnew);  			// insert it into the linked list of polygons
	sc_count++;
	}
      else 								// if not long enough, delete the verticies
	{
	vertex_destroy(vint);
	vertex_destroy(vold);
	}
      }

    }  // end of WHILE incrementing scanpos thru Y axis
  
  // at this point we have loaded the slice's fill polygons with vectors, but everything may be rotatated.
  // so we gotta rotate them (and the newly created fill polygons) back to their orginal orientation
  scanangle*=(-1);							// invert the incoming scanangle
  slice_rotate(crt_sptr,scanangle,ConTyp);				// un-rotate the contour that was used
  if(blw_sptr!=crt_sptr)slice_rotate(blw_sptr,scanangle,ConTyp);
  if(abv_sptr!=crt_sptr)slice_rotate(abv_sptr,scanangle,ConTyp);
  slice_rotate(crt_sptr,scanangle,FillTyp);				// un-rotate newly created fill
  //if(FillTyp==BASELYR)printf("  Linefill: polygon count=%d  qty=%d \n",sc_count,crt_sptr->pqty[FillTyp]);
  
  // apply special exceptions to the support materials
  if(FillTyp==SPT_FILL)
    {
    // if the inbound polygon is too small to provid more than a couple fill lines, then
    // copy the support perimeter to fill type as well.
    pptr=crt_sptr->pfirst[SPT_PERIM];					
    while(pptr!=NULL)
      {
      //pptr->area=polygon_find_area(pptr);
      if(fabs(pptr->area)<3.0 && crt_sptr->pfirst[SPT_OFFSET]==NULL)
        {
	pnew=polygon_copy(pptr);					// make a copy of the boundary polygon
	polygon_insert(crt_sptr,SPT_FILL,pnew);				// insert it into the linked list of polygons
	}
      pptr=pptr->next;
      }
    }


  // finally, sort the polygons to minimize pen up moves
  polygon_sort(crt_sptr,1);						// sort tip-to-tail every other pass
  
  // make sure all vtxs in polygon are at correct z level
  pptr=crt_sptr->pfirst[FillTyp];
  while(pptr!=NULL)
    {
    vptr=pptr->vert_first;
    while(vptr!=NULL)
      {
      vptr->z=crt_sptr->sz_level;
      vptr=vptr->next;
      if(vptr==pptr->vert_first)break;
      }
    pptr=pptr->next;
    }

  //if(FillTyp==SPT_FILL)printf("  Linefill:  Exit \n");
 
  return(1);
  
}

// Function to fill a specific slice with honey comb pattern fill lines
// where "cell_size" is the cell size of the honeycomb in mm - 5.0 is typical reasonible size.
//
// Ftyp defines what result is desired in bitwise fashion.
// the four most significant bits pertain to end_sptr, the least four pertain to str_sptr.
//	end_sptr	str_sptr	dec value	result
//	1000		0000		128		Only what is inside end_sptr, ignoring str_sptr
//	0000		1000		8		Only what is inside str_sptr, ignoring end_sptr
//	1000		1000		136		OR - what is inside either including overlap areas
//	1100		1100		204		AND - only what is inside both (i.e. overlap areas only)
//	1010		1010		170		XOR - what is inside either, but NOT including overlap areas
//	1010		1000		168		Only what is inside end_sptr, and NOT inside str_sptr
//	1000		1010		138		Only what is inside str_sptr, and NOT inside end_sptr

int slice_honeycomb(slice *str_sptr, slice *end_sptr, float cell_size, int Ltyp, int Ftyp)
{
  vertex	*hc_string;						// pointer to string of verticies that define honey comb lines
  vertex	*int_string,*vint_pos;					// pointer to start of intersection string and position in that string
  vertex	*vptr,*vold,*vint,*vnew,*vbrk,*vsrt,*vlst;
  int		h,status,vint_result,ix=0,iy=0;
  int		slice_ctr=0,istat1,istat2,in_state,in_flag;
  int		y_odd=0;						// counter to define honey comb direction
  float 	x_inc,y_inc;						// increment values
  float 	x_old,y_old;						// previous increment values
  float 	x_pos;							// horizontal position of hc string as it is generated
  float 	y_pos;							// vertical position of hc string as it sweeps across build table
  float 	tool_line_width=0.25;					// should be replace by proper reference from tool/material
  polygon	*pptr,*pnew;
  vector	*A,*B,*C;
  slice		*slpr;
  float 	vec_len=0.0;
  long int 	vmemold;
  float 	maxdist,mindist;
  float 	dx,dy,dist,nxtdist,veclen;
  int		ifound,int_cnt=0,cur_attr;
  int		i,ltype;
  
  vmemold=vertex_mem;
  
  //printf("Slice honeycomb fill: Entry \n");

  // ensure we have useable data coming in
  if(str_sptr==NULL || cell_size<0.25)return(0);
  
  // need to ensure a contour is available to work with
  status=0;
  if(str_sptr->pfirst[MDL_PERIM]!=NULL)status=1;
  if(str_sptr->pfirst[MDL_BORDER]!=NULL)status=1;
  if(str_sptr->pfirst[MDL_OFFSET]!=NULL)status=1;
  if(str_sptr->pfirst[SPT_PERIM]!=NULL)status=1;
  if(str_sptr->pfirst[SPT_BORDER]!=NULL)status=1;
  if(str_sptr->pfirst[SPT_OFFSET]!=NULL)status=1;
  if(!status)return(status);
  
  // ensure the max/min boundaries of this slice are up to date
  //slice_maxmin(str_sptr);

  // count number of slices
  slice_ctr=1;
  if(end_sptr!=NULL)slice_ctr++;

  // find the most "inside" offset polygon group to fill from and save that distance to use as a filter 
  // for which polygons to intersect with the scanline.  unless, support which has no offsets.
  ltype=MDL_PERIM;							// default to filling model
  if(Ltyp==SPT_FILL || Ltyp==SPT_LOWER_CO || Ltyp==SPT_UPPER_CO)ltype=SPT_PERIM; // unless support is requested
  maxdist=BUILD_TABLE_MIN_Y;
  mindist=BUILD_TABLE_MAX_Y;
  for(i=ltype;i<(ltype+3);i++)						// loop thru first 3 line types:  PERIM, BORDER, & OFFSET
    {
    pptr=str_sptr->pfirst[i];
    while(pptr!=NULL)
      {
      if(pptr->dist>maxdist){maxdist=pptr->dist;ltype=i;}		// if this is the most "inner", set line type to it
      if(pptr->dist<mindist)mindist=pptr->dist;
      pptr=pptr->next;
      }
    }
    
  // loop from bottom of build table Y to top of build table Y
  y_odd=0;								// tracks if odd/even pass across X.  determins if Y goes up or down.
  y_pos=0-BUILD_TABLE_MAX_Y;						// note this MUST be referenced to the build table.  NOT model or slice.
  y_old=y_pos;
  while(y_pos<BUILD_TABLE_MAX_Y)
    {

    y_odd++;
    y_pos=y_old;
    if(y_odd%2==0)y_pos+=cell_size;
    y_old=y_pos;
    
    y_pos-=(tool_line_width*0.5);					// if odd y stroke, offset up by half a line width
    if(y_odd%2==0)y_pos+=tool_line_width;				// if even y stroke, offset down by half a line width

    if(y_pos < (str_sptr->ymin-(2*cell_size)) || y_pos > (str_sptr->ymax+(2*cell_size)))continue;	// skip if outside of this slice's y range
  
    ix=1;iy=1;
    x_inc=cell_size*0.5;
    x_pos=BUILD_TABLE_MIN_X;						// note this MUST be referenced to the build table.  NOT model or slice.
    hc_string=NULL;
    while(x_pos<BUILD_TABLE_MAX_X)
      {
  
      // figure out x coord based on odd/even steps
      x_old=x_inc;							// save what this last increment was
      x_inc=cell_size*0.5;							// assume even increment at full length
      if(ix%2==0)x_inc=cell_size*0.3;						// if odd, then reset to half length
      x_pos+=x_inc;							// increment position of x
      ix++;
    
      // figure out y coord based on x increment size.  a change in increment size means a change in y
      y_inc=0;
      if(x_inc<x_old)							// if this x increment is a raise or lower of y...
	{
	y_inc=cell_size*0.5-(tool_line_width);					// ... then y will change by half of l
	if(iy%2==0)y_inc=(-1)*y_inc;					// if x went from half to full then reverse y increment direction
	iy++;
	}
      if(y_odd%2!=0)
	{
	y_inc=(-1)*y_inc;						// if on an odd y pass, then reverse the y increment direction
	}
      y_pos+=y_inc;							// increment position of y up or down as a function of x

      // skip processing anything outside of slice's range
      if(x_pos < (str_sptr->xmin-(2*cell_size)) || x_pos > (str_sptr->xmax+(2*cell_size)))continue;
    

      // push new coords into new vertex
      vptr=vertex_make();
      vptr->x=x_pos;
      vptr->y=y_pos;
      vptr->z=0;							// note this value defaults to "outside"
      vptr->next=NULL;
    
      // adjust state of construction
      x_old=x_inc;
      if(hc_string==NULL)						// if first time thru loop ....
	{
	hc_string=vptr;							// ... save address of first vertex for future reference
	}
      else 								// if NOT first time thru loop ...
	{
	vold->next=vptr;						// ... add new vtx into linked list
	}
      vold=vptr;
      }
 
  
    // at this point a honeycomb line that runs across the slice has been formed in the hc_string.
    // now each segment must be intersected with the polygon group of each slice to figure out the actual fill.
    // do this by looping thru the hc vertex string segments and finding intersections and assigning the attr value
    // of each found intersection vertex the slice number that created it.  we need this later to figure
    // out the correct winding number.

    int_string=NULL;
    vnew=vertex_make();							// temporary vertex to hold intersection values
    vold=hc_string;
    vptr=hc_string;							// start with first segment of hc_string
    do{									// loop thru all verticies in hc fill string
      vptr->z=0;							// since all z is the same, use this variable to hold intersection slice count
      A=vector_make(vptr,vptr->next,1);					// create vector from hc fill string vertex pair (segment)
      slpr=str_sptr;							// start at the current slice
      h=0;								// init slice counter
      int_cnt=0;
      while(h<3)							// loop thru the current slice, until the last one
        {
	slpr=str_sptr;
	if(h==1)slpr=end_sptr;
	if(slpr==NULL)break;
	h++;								// increment the slice counter
	pptr=slpr->pfirst[ltype];					// start with offset polygons of nearest polygons
	while(pptr!=NULL)						// loop thru all polygons in this slice
	  {
	  if(fabs(pptr->dist - maxdist)>TOLERANCE)			// if this polygon is not at the maximum offset distance...
	    {
	    pptr=pptr->next;						// then we are going to skip it.
	    continue;
	    }
	  vint=pptr->vert_first;					// loop thru all vertex pairs in this polygon
	  do{
	    // check bounds of vtxs first.  if beyond, then just ignore.  may provide speed improvement.
	    if((vint->y < (y_pos-cell_size)) && (vint->next->y < (y_pos-cell_size)))	// if there is no chance of intersection because too low...
	      {
	      vint=vint->next;						// just move onto next segment
	      continue;
	      }
	    if((vint->y > (y_pos+cell_size)) && (vint->next->y > (y_pos+cell_size)))	// if there is no chance of intersection because too hight...
	      {
	      vint=vint->next;						// just move onto next segment
	      continue;
	      }
	  
	    B=vector_make(vint,vint->next,2);				// create vector out of vertex pair of polygon segment
	    vint_result=vector_intersect(A,B,vnew);			// intersect hc vector (A) with polygon vector (B)
	    
	    // if polygon vector (vecB) intersects hc fill line segment vector (vecA)...
	    ifound=0;
	    if(vint_result>=204 && vint_result<=205)ifound=1;		// all cases when on tail of polygon vector
	    if(vint_result>=220 && vint_result<=222)			// all cases when on tail of scanline vector
	      {
	      if(int_string==NULL)					// if there are no other intersections with this segment...
	        {
		if(polygon_contains_point(pptr,vptr))ifound=1;		// only count it if the tip is inside the polygon
		}
	      else 
	        {
		if(polygon_contains_point(pptr,vbrk))ifound=1;		// only count it if the previous segment is inside the polygon
		}
	      }
	    if(vint_result>=236 && vint_result<=238)			// all cases when on tip of scanline vector
	      {
	      if(int_string==NULL)					// if there are no other intersections with this segment...
	        {
	        if(polygon_contains_point(pptr,vptr->next))ifound=1;	// only count it if the tail is inside the polygon
		}
	      }
	    if(ifound)
	      {								
	      // check if new intersection created a vector long enough to keep
/*	      dx=vptr->x-vnew->x;
	      dy=vptr->y-vnew->y;
	      dist=sqrt(dx*dx+dy*dy);					// calculate length from current vertex in hc_string
	      if(vptr->next!=NULL)
	        {
		dx=vptr->next->x-vnew->x;
		dy=vptr->next->y-vnew->y;
		nxtdist=sqrt(dx*dx+dy*dy);				// calculate length to next vertex in hc_string
		veclen=MIN_VEC_LEN;
		if(current_tool>0)veclen=Tool[current_tool].matl.minveclen;	// if a tool is active and we have an actual min_vec_len...
		if(dist<nxtdist && dist<veclen)				// if this vector is too short... just terminate at closest vertex of hc_string (vint)
		  {
		  vnew->x=vptr->x;
		  vnew->y=vptr->y;
		  vnew->z=vptr->z;
		  }
		if(nxtdist<dist && nxtdist<veclen)
		  {
		  vnew->x=vptr->next->x;
		  vnew->y=vptr->next->y;
		  vnew->z=vptr->next->z;
		  }
		}
*/
	      vbrk=vertex_make();					// create a new vertex to be inserted at intersection point
	      vbrk->x=vnew->x;						// define its coordinates
	      vbrk->y=vnew->y;
	      vbrk->z=h;
	      vbrk->attr=vint_result;					// set to indicate intersection with this specific slice
	      vbrk->next=NULL;
	      int_cnt++;
	      if(int_string==NULL)					// if the intersection string is empty...
		{
		int_string=vbrk;					// define this as the first vertex in the intersection string
		vint_pos=int_string;					// define pointer position along intersection string
		}
	      else 							// if the intersection string is not empty...
		{
		vint_pos->next=vbrk;					// add new intersection to end of intersection string  
		vint_pos=vbrk;						// increment the vertex intersection pointer to last element
		}
	      }
	    vint=vint->next;						// get next vertex from current polygon
	    vector_destroy(B,FALSE);					// release temporary polygon segment vector
	    }while(vint!=pptr->vert_first);				// end when we've circled all the way back to start
	  pptr=pptr->next;						// move onto next polygon in this slice
	  }
	slpr=slpr->next;						// move onto next slice in the requested list
	}
      vector_destroy(A,FALSE);
      
      // at this point we have intersected this one hc string segment of fill line with all polygon lines of both slices.
      // we must now sort the intersections from left to right (low to high) in the X then insert that string into 
      // the definition of this one fill line vector.
      
      // now insert the sorted string of intersection verticies into the hc fill line segment
      if(int_cnt>0)
        {
	// sort the entire int string by X value lowest to highest.
	// note these are just the intersections found in this one segment of hc_string that we are working on here.
	// since the polygons occur in random order, the intersections will appear in random order in int_string until sorted.
	if(int_cnt>1)							// if the intersection string has more than 1 verticie...
	  {
/*	  vlst=vertex_make();
	  vsrt=int_string;
	  while(vsrt!=NULL)
	    {
	    vint=vsrt->next;
	    while(vint!=NULL)
	      {
	      if(vint->x < vsrt->x)
		{
		// note it is easier to swap values that addresses (swapping addresses will mess up the loops)
		vlst->x=vsrt->x; vlst->y=vsrt->y; vlst->z=vsrt->z; vlst->attr=vsrt->attr;
		vsrt->x=vint->x; vsrt->y=vint->y; vsrt->z=vint->z; vsrt->attr=vint->attr;
		vint->x=vlst->x; vint->y=vlst->y; vint->z=vlst->z; vint->attr=vlst->attr;
		
		//printf("Swap:  count=%d  vsrt->x=%f  vint->x=%f \n",int_cnt,vsrt->x,vint->x);
		}
	      vint=vint->next;
	      }
	    vint_pos=vsrt;						// this will catch the address of the last element in the intersection list before the loop ends
	    vsrt=vsrt->next;
	    }
	  vertex_destroy(vlst);
*/
	  }
	  
	// add the sorted int string into the hc string segment between vptr and vptr->next
	vint=vptr->next;						// save address of next vertex
	vptr->next=int_string;						// redefine inlet of hc string into int string
	vint_pos->next=vint;						// redefine exit of int string back into hc string
	vptr=vint;							// set hc string pointer to move on past the int string that was just added
	int_string=NULL;
	}
      else 								// if the intersection string is empty...
        {
	vptr=vptr->next;						// move onto next line segment of hc fill line string
	}
      }while(vptr->next!=NULL);						// end when we've done all of the hc fill string segments
    vertex_destroy(vnew);						// delete temporary vertex from memory

 
    // at this point the hc string is now broken into segments that are either completely inside polygons or completely outside.
    // now we can count the winding number from left to right and keep what is inside and throw away whats outside.
    // but it is important to consider which slice we are crossing and when. ultimately we want to know if we are
    // exclusively inside the area of an individual polygon or a shared area of polygons from different slices.


    istat1=0;
    istat2=0;
    cur_attr=0;
    vptr=hc_string;
    while(vptr!=NULL)
      {
      // increment winding numbers
      h=0;
      if(vptr->z==1)istat1++;						// if this vtx was created as a result of intersecting str_sptr polygons... increment winding number.
      if(vptr->z==2)istat2++;						// if this vtx was created as a result of intersecting end_sptr polygons... increment winding number.
      
      // based on input "Ftyp" bit setting and the winding number a transition will be determined
      if(Ftyp==8 && istat1%2!=0)h=1;					// Only what is inside str_sptr, ignoring end_sptr
      if(Ftyp==128 && istat2%2!=0)h=1;					// Only what is inside end_sptr, ignoring sptr_sptr
      if(Ftyp==138 && istat1%2!=0 && istat2%2==0)h=1;			// Only what is inside str_sptr, and NOT inside end_sptr
      if(Ftyp==168 && istat1%2==0 && istat2%2!=0)h=1;			// Only what is inside end_sptr, and NOT inside str_sptr
      if(Ftyp==204 && istat1%2!=0 && istat2%2!=0)h=1;			// AND - only what is inside both (i.e. overlap areas only)
      if(Ftyp==170)							// XOR - what is inside either, but NOT including overlap areas
	{
	if(istat1%2!=0 && istat2%2==0)h=1;
	if(istat1%2==0 && istat2%2!=0)h=1;
	}
      if(Ftyp==136)							// OR - what is inside either including overlap areas
	{
	if(istat1%2!=0 || istat2%2!=0)h=1;
	}
	
      // if this is a transition into what we want to keep...
      if(h==1 && in_flag==0)						
        {
	in_flag=1;							// ... set flag to indicate we are inside
	cur_attr=vptr->attr;						// ... save the attribute value which indicates we are inside
	vptr=vptr->next;						// ... move onto the next vertex
	continue;
	}
      
      // if this is a transition out of what we want to keep...
      if(h==0 && in_flag==1)						
        {
	in_flag=0;							// ... set flag to indicate we are outside
	cur_attr=0;							// ... set to indicate we are outside
	vptr=vptr->next;						// ... move onto the next vertex
	continue;
	}

      // at this point the vertex is NOT a transition.
      // we are either inside or outside, so we set the vertex attribute to the transition value
      vptr->attr=cur_attr;
      vptr=vptr->next;
      }


    //pnew=polygon_make(hc_string,MDL_FILL,0);				// create a new polygon from it
    //polygon_insert(str_sptr,MDL_FILL,pnew);  				// insert it into the linked list of FILL polygons


    // loop thru hc vertex string and make polygons out of the segments we want to keep, and toss the rest
    in_flag=0;
    vint=hc_string;
    vold=hc_string;
    vptr=hc_string;
    while(vptr!=NULL)
      {
      // if this vertex is on the outside and the previous one was on the outside...
      if(vptr->attr==0 && in_flag==0)				
	{
	in_flag=vptr->attr;						// save in/out status of vertex that's about to be deleted
	vold=vptr;							// save this address for deletion
	vptr=vptr->next;						// move our current pointer to next vertex
	hc_string=vptr;							// redefine beginning of rest of hc fill line string
	vertex_destroy(vold);						// delete this outside vertex
	vint=hc_string;
	vold=hc_string;
	continue;
	}
      // if this vertex is on the outside and the previous one was on the inside...
      if(vptr->attr==0 && in_flag!=0)				
        {
	in_flag=vptr->attr;
	vptr=vptr->next;						// increment out pointer to the next vertex
	vold->next=NULL;						// terminate this polygon string
	pnew=polygon_make(vint,Ltyp,0);					// create a new polygon from it
	polygon_insert(str_sptr,Ltyp,pnew);  				// insert it into the linked list of FILL polygons
	hc_string=vptr;							// redefine start as remainder of this string
	vint=hc_string;
	vold=hc_string;
	continue;
	}
      // if this vertex is on the inside and the previous one was on the outside...
      if(vptr->attr!=0 && in_flag==0)
        {
	in_flag=vptr->attr;
	vint=vptr;							// save this address as first vertex of new polygon
	vptr=vptr->next;
	continue;
	}
      // if this vertex is on the inside and the previous one was also on the inside...
      if(vptr->attr!=0 && in_flag!=0)
        {
	in_flag=vptr->attr;
	vold=vptr;							// save this address as last vertex of new polygon
	vptr=vptr->next;
	continue;
	}
      }


    }		// end of WHILE incrementing thru Y axis
  
  // now sort polygons tip-to-tail to improve printing efficiency  
  polygon_sort(str_sptr,1);

  // make sure all vtxs in polygon are at correct z level
  pptr=str_sptr->pfirst[Ltyp];
  while(pptr!=NULL)
    {
    vptr=pptr->vert_first;
    while(vptr!=NULL)
      {
      vptr->z=str_sptr->sz_level;
      vptr=vptr->next;
      if(vptr==pptr->vert_first)break;
      }
    pptr=pptr->next;
    }

  //printf("Slice honeycomb fill: Exit \n");

  
  return(1);
}


// Function to boolean two slices together
// this creates a new slice that is the boolean of the two input slices without modifying the input slices.  see
// the pound-defines for the types of actions.
slice *slice_boolean(model *mptr, slice *sA, slice *sB, int ltypA, int ltypB, int ltyp_save, int action)
{
  int		h,result;
  polygon	*ppre,*pptr,*pnpr,*pnxt,*pnew,*pdel;
  slice		*snew;
 
  if(mptr==NULL || sA==NULL || sB==NULL)return(NULL);
  if(ltypA<0 || ltypA>MAX_LINE_TYPES)return(NULL);
  if(ltypB<0 || ltypB>MAX_LINE_TYPES)return(NULL);
  if(ltyp_save<0 || ltyp_save>MAX_LINE_TYPES)return(NULL);
  
  printf("\nSB:  Entry...\n");
  printf("  mptr=%X  sA=%X  sB=%X  ltypA=%d  ltypB=%d  ltyp_save=%d  action=%d\n",mptr,sA,sB,ltypA,ltypB,ltyp_save,action);
  
  snew=slice_make();
  if(snew==NULL)return(NULL);
  
  
  // first copy everything from each of the two input slices into one new slice.
  // this serves the purpose of including polygons that do not intersect anything else,
  // and allows the delete/recursion necessary to find all intersections between all polygons
  // of a specific line types (versus all line types).
  pptr=sA->pfirst[ltypA];
  while(pptr!=NULL)
    {
    pnew=polygon_copy(pptr);
    pnew->next=NULL;
    polygon_insert(snew,ltyp_save,pnew);
    //printf("   copying  pptr=%X  from  sA=%X  to  pnew=%X  and inserting into  snew=%X  at  lt=%d \n",pptr,sA,pnew,snew,ltyp_save);
    pptr=pptr->next;
    }
  pptr=sB->pfirst[ltypB];
  while(pptr!=NULL)
    {
    pnew=polygon_copy(pptr);
    pnew->next=NULL;
    polygon_insert(snew,ltyp_save,pnew);
    //printf("   copying  pptr=%X  from  sB=%X  to  pnew=%X  and inserting into  snew=%X  at  lt=%d \n",pptr,sB,pnew,snew,ltyp_save);
    pptr=pptr->next;
    }
 
  /*
  // boolean support polygons together
  pptr=sptr->pfirst[ptyp];						// start with first spt polygon
  while(pptr!=NULL)							// main loop
    {
    pnxt=sptr->pfirst[ptyp];						// start with first spt polygon
    while(pnxt!=NULL)							// secondary loop
      {
      if(pnxt==pptr){pnxt=pnxt->next;continue;}				// don't add to itself
      result=polygon_intersect(pptr,pnxt,0.01);				// check if the polygons intersect
      printf("  pptr=%X  pnxt=%X  result=%d\n",pptr,pnxt,result);
      if(result>0)							// if they do intersect...
        {
	pnew=polygon_boolean2(pptr,pnxt,1);				// ... attempt to add them together
	if(pnew!=NULL)							// ... if it created a new polygon...
	  {
	  polygon_delete(sptr,pptr);					// ... delete the main loop polygon
	  polygon_delete(sptr,pnxt);					// ... delete the secondary loop polygon

	  polygon_insert(sptr,ptyp,pnew);				// ... insert the new polygon at end of linked list

	  pptr=sptr->pfirst[ptyp];					// ... restart main loop back at first polygon
	  pnxt=sptr->pfirst[ptyp];					// ... restart secondary loop back at first polygon
	  }
	}
      else 
        {
	pnxt=pnxt->next;						// only increment secondary loop if no intersection found
	}
      }
    pptr=pptr->next;
    }
  */

  // loop thru all the polygons of snew and compare each polygon against all other polygons
  ppre=NULL;
  pptr=snew->pfirst[ltyp_save];
  while(pptr!=NULL)
    {
    printf("  Checking pptr=%X (%d) intersections against: \n",pptr,pptr->vert_qty);
    pnpr=NULL;
    pnxt=snew->pfirst[ltyp_save];
    while(pnxt!=NULL)
      {
      if(pnxt==pptr){pnxt=pnxt->next;continue;}
      result=polygon_intersect(pptr,pnxt,0.01);
      printf("    pnxt=%X (%d) with result=%d \n",pnxt,pnxt->vert_qty,result);
      pnew=polygon_boolean2(snew,pptr,pnxt,action);
      if(pnew!=NULL)
	{
	pnew->type=ltyp_save;
	polygon_insert(snew,ltyp_save,pnew);
	polygon_delete(snew,pptr);
	if(ppre!=NULL){pptr=ppre;}else{pptr=snew->pfirst[ltyp_save];}
	polygon_delete(snew,pnxt);
	if(pnpr!=NULL){pnxt=pnpr;}else{pnxt=snew->pfirst[ltyp_save];}
	}
      pnpr=pnxt;
      pnxt=pnxt->next;
      }
    ppre=pptr;
    pptr=pptr->next;
    }
  
  // test if anything good came out of this...
  if(snew->pqty[ltyp_save]<=0){snew=NULL;}
  else {printf("\nSB:  Exit - snew=%X  qty=%d \n",snew,snew->pqty[ltyp_save]);}
  
  return(snew);
}

// Function to determine if a vertex falls within the 2D map of a light facet
int vertex_in_facet(vertex *vtest, facet *finpt)
{
  int 	status;
  float x,y,z;
  float x1,y1,z1,x2,y2,z2,x3,y3,z3;
  float l1,l2,l3;
  
  x=vtest->x; y=vtest->y; z=vtest->z;
  x1=finpt->vtx[0]->x; y1=finpt->vtx[0]->y; z1=finpt->vtx[0]->z;
  x2=finpt->vtx[1]->x; y2=finpt->vtx[1]->y; z2=finpt->vtx[1]->z;
  x3=finpt->vtx[2]->x; y3=finpt->vtx[2]->y; z3=finpt->vtx[2]->z;

  l1 = (x-x1)*(y3-y1) - (x3-x1)*(y-y1);
  l2 = (x-x2)*(y1-y2) - (x1-x2)*(y-y2);
  l3 = (x-x3)*(y2-y3) - (x2-x3)*(y-y3);
  
  status=FALSE;
  if(l1>0 && l2>0 && l3>0)status=TRUE;
  if(l1<0 && l2<0 && l3<0)status=TRUE;
  
  return(status);
}

// Function to slice each model based on tool and operation.
// the result is a processed slice deck of clean model and support polygons held in memory for each slot.
// note that filling happens dynamically at display and/or print time via slice_engine.
// Inputs:  mptr=pointer to model to be sliced  mtyp=model type (MODEL/SUPPORT/etc)  oper_ID=operation tool will perform on mptr
// Return:  0=Failure  1=Success  AND the mptr is loaded with slices for that tool/operation
int slice_deck(model *mptr, int mtyp, int oper_ID)
{
  int 		h,i,slot,ptyp,ntyp,all_ok,ptest,scount=1,cstat,pstat,result;
  int		offset_walls,border_walls,xctr,yctr,add_spt;
  char		mdl_name[255];
  float		xp,yp,dx,dy,dz,mx,my,len,area;
  float 	xstart,xend,ystart,yend;
  float 	actual_dist,border_dist,offset_dist,travel_dist;
  float 	fval,z_lvl;
  float 	old_min_polygon_area;
  vertex 	*v0,*v1,*v2;
  vertex 	*vptr,*vpre,*vtest;
  edge 		*eptr,*edel;
  facet 	*fptr;
  polygon 	*pptr,*pdel,*ppre,*pnxt,*pnpr,*pold,*pnew,*pspt,*pdia,*phf,*ptemp;
  slice 	*sptr,*snxt,*sold,*snew,*s_at_z,*s_above;
  float 	adjwidth,adjpitch;
  double 	amt_done=0;
  linetype	*lptr;
  genericlist	*aptr;
  operation 	*optr;

  // validate inputs
  if(mptr==NULL){printf("\nSliceDeck: no model\n");   return(0);}			// if the model has not yet been defined, abort.
  slot=model_get_slot(mptr,oper_ID);					// get the slot defined for this model
  if(slot<0 || slot>=MAX_TOOLS){printf("\nSliceDeck: bad slot\n");   return(0);}	// if an errant slot id
  if(Tool[slot].matl.lt==NULL){printf("\nSliceDeck: no linetypes\n");return(0);}	// if no line types
  if(mptr->input_type==GERBER){printf("\nSliceDeck: GERBER file\n"); return(1);}	// nothing to slice if a GERBER file
  //if(mptr->reslice_flag==FALSE)return(0);				// no need to process
  
  //debug_flag=200;
  if(debug_flag==200)printf("\nSlice Deck Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  if(debug_flag==200)printf("  Operations List\n");
  add_spt=FALSE;    
  aptr=mptr->oper_list;							// start with first operation for this model
  while(aptr!=NULL)							// loop thru all of this model's operations
    {
    if(debug_flag==200)printf(" %d  %s \n",aptr->ID,aptr->name);
    aptr=aptr->next;
    }
  if(debug_flag==200)printf("  Operation Requested: %d\n",oper_ID);
  
  // build simplified model name for display
  // this is neccessary to limit the name length to cleanly fit into the status window
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
  }

  // show the progress bar
  sprintf(scratch,"  Slicing %s...  ",mdl_name);
  gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
  if(mtyp==MODEL){sprintf(scratch,"  Generating %d model layers for tool %d  ",(int)((mptr->zmax[mtyp]-mptr->zmin[mtyp])/Tool[slot].matl.layer_height),(slot+1));}
  if(mtyp==INTERNAL){sprintf(scratch,"  Generating %d internal layers for tool %d  ",(int)((mptr->zmax[mtyp]-mptr->zmin[mtyp])/Tool[slot].matl.layer_height),(slot+1));}
  if(mtyp==SUPPORT){sprintf(scratch,"  Generating %d support layers for tool %d  ",(int)((mptr->zmax[mtyp]-mptr->zmin[mtyp])/Tool[slot].matl.layer_height),(slot+1));}
  gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
  gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
  amt_done = 0.0;
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
  gtk_widget_set_visible(win_info,TRUE);
  gtk_widget_queue_draw(win_info);
  while (g_main_context_iteration(NULL, FALSE));

  mptr->slice_thick=Tool[slot].matl.layer_height;
  old_min_polygon_area=min_polygon_area;
  fidelity_mode_flag=mptr->fidelity_mode;
  slice_has_many_polygons[mtyp]=0;					// init polygon counter for this model type
  
  // find starting slice at top of model that is an even increment of the model's slice thickness
  // in other words, this loop slices from the TOP down (not bottom up)
  //fval=(mptr->zmax - mptr->zmin) / mptr->slice_thick;			// get decimal number of slices
  //z_lvl=floor(fval)*mptr->slice_thick+TOLERANCE;			// get max z to start slicing at.  TOL keeps it off zero.
  z_lvl=mptr->zmax[MODEL]-(mptr->slice_thick/2.0);			// get max z to start slicing at.

  first_layer_flag=FALSE;
  sold=NULL;								// pointer to previous slice
  while(z_lvl > 0.000)							// loop until at build table
    {
    // update status to user
    amt_done = (double)(mptr->zmax[mtyp]-z_lvl)/(double)(mptr->zmax[mtyp]);
    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
    while (g_main_context_iteration(NULL, FALSE));
    
    // check if an slice at this z level already exists for this model and purge 
    // what is found (if any) for this operation and this operation only.  Note that this is a little
    // tricky because any given tool can provide BOTH model material and support material.
    /*
    sptr=slice_find(mptr,MODEL,slot,z_lvl,mptr->slice_thick);		// returns NULL if nothing found
    if(sptr!=NULL)							// if a slice does exist...
      {
      if(debug_flag==200)printf("  purging existing slice data... \n");
      optr=operation_find_by_ID(slot,oper_ID);				// ... get ptr to this operation
      aptr=optr->lt_seq;						// ... get first line type off operation's sequence list
      while(aptr!=NULL)							// ... loop thru all line types called out for this operation
	{
	if(sptr->pfirst[aptr->ID]!=NULL)slice_wipe(sptr,aptr->ID,0);	// ... wipe out any existing polygon data
	aptr=aptr->next;						// ... move onto next line type
	}
      }
    */

    // generate cross sections for MODEL
    if(oper_ID==OP_ADD_MODEL_MATERIAL || (oper_ID>=OP_MILL_OUTLINE && oper_ID<=OP_MILL_HOLES) || 
      (oper_ID>=OP_MARK_OUTLINE && oper_ID<=OP_MARK_CUT))
      {
      if(debug_flag==200)printf("  slicing model at z=%6.3f \n",z_lvl);
      if(mptr->input_type==STL && mtyp==MODEL)
        {
	ptyp=MDL_PERIM;							// define source line type from which to make target types
	sptr=model_raw_slice(mptr,slot,mtyp,ptyp,z_lvl);		// slice STL and insert sptr as last in linked list	
	}
      if(mptr->input_type==STL && mtyp==INTERNAL)
        {
	ptyp=INT_PERIM;
	sptr=model_raw_slice(mptr,slot,mtyp,ptyp,z_lvl);		// slice STL and insert sptr as last in linked lis	
	}
      if(mptr->input_type==GERBER)sptr=mptr->slice_first[slot];		// define first slice of GERBER file
      if(sptr!=NULL)
	{
	if(debug_flag==200)printf("  new slice %X generated... \n",sptr);
	sptr->ID=scount;
	scount++;
	//if(sold!=NULL)sptr->prev=sold;				// define the previous slice and add link
       
	// determine boundaries of new slice
	slice_maxmin(sptr);
    
	// boolean overlapping polygons together.  if two polygons do overlap, the pnxt polygon will be
	// "absorbed" by the pptr polygon.  note that nested polygons are not affected... edges must cross
	// for them to boolean.
	/*
	ppre=NULL;
	pptr=sptr->pfirst[ptyp];
	while(pptr!=NULL)
	  {
	  pnxt=sptr->pfirst[ptyp];
	  while(pnxt!=NULL)
	    {
	    // if polygons cross edges, then assume they both represent material and 
	    // should be merged.
	    if(polygon_intersect(pptr,pnxt,CLOSE_ENOUGH) > 0)
	      {
	      pnew=polygon_boolean2(pptr,pnxt,ACTION_ADD);
	      //pnew=NULL;
	      if(pnew!=NULL)
	        {
		polygon_swap_contents(pptr,pnew);			// replace pptr with pnew contents
		polygon_free(pnew);					// remove stand alone polygon
		polygon_delete(sptr,pnxt);				// remove pnxt from active slice
		pnxt=pptr;						// reset place in linked list
		}
	      }
	    pnxt=pnxt->next;
	    }
	  ppre=pptr;
	  pptr=pptr->next;
	  }
	*/
	
	// determine if each polygon encloses material or is a hole  
	// call these function only AFTER ALL polygons in the slice have been verified.
	if(fidelity_mode_flag==TRUE)min_polygon_area=0.0010;
	pptr=sptr->pfirst[ptyp];	
	while(pptr!=NULL)
	  {
	  slice_has_many_polygons[mtyp]++;					// increment polygon counter
	  polygon_contains_material(sptr,pptr,ptyp);				// deterimine if this poly encloses a hole or encloses material
	  //polygon_find_start(pptr,NULL);					// put start at a corner for now
	  pptr->fill_type=1;							// default fill to "from file"
	  if(oper_ID==OP_MARK_OUTLINE){pptr->fill_type=0;}			// if outline,set fill to "none"
	  pold=pptr;								// save address of this poly
	  pptr=pptr->next;							// move loop poly onto next poly
	  if(fabs(pold->area)<min_polygon_area)polygon_delete(sptr,pold);	// remove polygons that are too small
	  }
	if(debug_flag==200)printf("   sptr=%X  z=%5.3f  MPqty=%d \n",sptr,sptr->sz_level,sptr->pqty[MDL_PERIM]);
	if(debug_flag==200)printf("   %d model perim polygons made \n",sptr->pqty[ptyp]);
	
	// sort polygon sequence depending on nature of job
	//if(job.type==ADDITIVE)polygon_sort_perims(sptr,MDL_PERIM,0);		// sort by proximity to each other
	//if(job.type==SUBTRACTIVE)polygon_sort_perims(sptr,MDL_PERIM,1);	// sort by area
	//if(oper_ID==OP_MARK_CUT)polygon_sort_perims(sptr,MDL_PERIM,2);	// sort by perim length
	
	// build enclosure lists.  this identifies a parent polygon and then builds the pointer list of all
	// its children (i.e. all polygons that are enclosed by the parent)
	polygon_build_sublist(sptr,ptyp);
	//printf("poly relations established  z=%6.3f\n",z_lvl);

	// make border polygons from perimeter polygons
	// note that layer_1 still uses border polygons to define its perimeter.  layer_1 is only a fill, nor a perim
	border_dist=0.0;
	if(mtyp==MODEL)ntyp=MDL_BORDER;
	if(mtyp==INTERNAL)ntyp=INT_BORDER;
	if(model_linetype_active(mptr,ntyp)==TRUE)			// if this model uses this line type...
	  {
	  if(debug_flag==200)printf("   processing model borders with ntyp=%d ...\n",ntyp);
	  lptr=linetype_find(slot,ntyp);					// get pointer to line type
	  if(lptr!=NULL)							// if the pointer was found...
	    {
	    min_vector_length=lptr->minveclen;
	    border_walls=(int)floor(lptr->wall_width/lptr->line_pitch + 0.5); 	// calc number of integer passes required to build wall
	    border_dist=lptr->line_width*0.5+(border_walls-1)*lptr->line_pitch;	// calc initial position of first pass of wall
	    if(job.type==ADDITIVE && first_layer_flag==TRUE){border_walls=1;border_dist=lptr->line_width*1.5;} // adjust to elimnate "elephant foot" on first layer.  More dist = less foot.
	    if(fidelity_mode_flag==TRUE){border_walls=1;border_dist=0.001;}
	    if(fidelity_mode_flag==TRUE && first_layer_flag==FALSE)border_dist=lptr->fidelity_line_width;
	    while(border_walls>0)						// loop thru all passes required to build wall
	      {
	      actual_dist=border_dist;						// set to temporary variable
	      if(job.type==ADDITIVE && Tool[slot].type==SUBTRACTIVE)actual_dist *= (-1);
	      if(slice_has_many_polygons[mtyp] > slice_offset_type_qty[mtyp])
	        {
		if(first_layer_flag==TRUE){slice_offset_winding_by_polygon(sptr,ptyp,MDL_BORDER,actual_dist);}
		else {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,actual_dist);}	// generate the new border toward "inside"
		}
	      else 
	        {
		if(first_layer_flag==TRUE){slice_offset_winding(sptr,ptyp,MDL_BORDER,actual_dist);}
		else {slice_offset_winding(sptr,ptyp,ntyp,actual_dist);}		// generate the new border toward "inside"
		}
	      border_walls--;							// decrement the pass we just made
	      if(border_walls>0)border_dist-=lptr->line_pitch;			// decrement the distance by a line pitch
	      }
	    border_dist-=(lptr->line_width*0.5);				// subtract off other half of border line width
	    travel_dist=linetype_accrue_distance(sptr,ntyp);
	    if(travel_dist>0)sptr->pdist[ntyp]=travel_dist;
	    }
	  border_dist=lptr->wall_width;							// calc inside edge location of boarder once done
	  if(fidelity_mode_flag==TRUE)border_dist=lptr->fidelity_line_width-(lptr->line_width*0.5);
	  if(first_layer_flag==TRUE)border_dist=lptr->line_width*1.5;			// sepcial case to reduce excessive spread on first layer
	  if(oper_ID==OP_MARK_CUT)polygon_sort_perims(sptr,MDL_BORDER,2);		// sort by perim length
	  if(debug_flag==200)printf("   %d model borders polygons made \n",sptr->pqty[ntyp]);
	  }

	// make offset polygons from perimeter polygons
	if(fidelity_mode_flag==TRUE)min_polygon_area=3.0;
	offset_dist=0.0;
	if(mtyp==MODEL)ntyp=MDL_OFFSET;
	if(mtyp==INTERNAL)ntyp=INT_OFFSET;
	if(model_linetype_active(mptr,ntyp)==TRUE && first_layer_flag==FALSE)
	  {
	  if(debug_flag==200)printf("   processing model offsets... \n");
	  lptr=linetype_find(slot,ntyp);
	  if(lptr!=NULL)
	    {
	    min_vector_length=lptr->minveclen;
	    if(mptr->set_wall_thk<TOLERANCE){mptr->set_wall_thk=lptr->wall_width;}	// if not defined otherwise, use line type width
	    offset_walls=(int)floor(mptr->set_wall_thk/lptr->line_pitch + 0.5);		// round to closest integer
	    offset_dist=border_dist+mptr->set_wall_thk-(lptr->line_pitch*0.5);		// calc initial position of first pass of inner most wall
	    if(fidelity_mode_flag==TRUE){offset_walls=1; offset_dist=0.050;}
	    while(offset_walls>0)							// loop thru all passes required to build wall
	      {
	      actual_dist=offset_dist;							// set to temporary variable
	      if(job.type==ADDITIVE && Tool[slot].type==SUBTRACTIVE)actual_dist *= (-1);// invert direction if removing material
	      if(slice_has_many_polygons[mtyp] > slice_offset_type_qty[mtyp])
	        {
		if(first_layer_flag==TRUE){slice_offset_winding_by_polygon(sptr,ptyp,MDL_OFFSET,actual_dist);}
		else {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,actual_dist);}		// generate the new offset
		}
	      else 
	        {
		if(first_layer_flag==TRUE){slice_offset_winding(sptr,ptyp,MDL_OFFSET,actual_dist);}
		else {slice_offset_winding(sptr,ptyp,ntyp,actual_dist);}		// generate the new offset
		}
	      offset_walls--;							// decrement the pass we just made
	      if(offset_walls>0)offset_dist-=lptr->line_pitch;			// subtract a line width for each pass
	      }
	    offset_dist-=(lptr->line_width*0.5);				// subtract off other half of offset line width
	    travel_dist=linetype_accrue_distance(sptr,ntyp);
	    if(travel_dist>0)sptr->pdist[ntyp]=travel_dist;
	    }
	  if(oper_ID==OP_MARK_OUTLINE)polygon_sort_perims(sptr,MDL_OFFSET,0);	// sort by perim length
	  //if(debug_flag==200)printf("   %d model offset polygons made \n",sptr->pqty[ntyp]);
	  }
	}	// end of if sptr!=NULL...
      }		// end of if oper_ID...

    // generate cross sections for SUPPORT
    if(oper_ID==OP_ADD_SUPPORT_MATERIAL)
      {
      if(debug_flag==200)printf("  slicing support at z=%6.3f \n",z_lvl);
      mtyp=SUPPORT;
      ptyp=SPT_PERIM;							// define source line type from which to make target types
      sptr=model_raw_slice(mptr,slot,mtyp,ptyp,z_lvl);			// slice STL and insert sptr as last in linked lis	
      if(sptr!=NULL)
	{
	// determine if each polygon encloses material or is a hole  
	// call these function only AFTER ALL polygons in the slice have been cleaned up and added together.
	pptr=sptr->pfirst[ptyp];	
	while(pptr!=NULL)
	  {
	  polygon_contains_material(sptr,pptr,ptyp);			// deterimine if this poly encloses a hole or encloses material
	  polygon_find_start(pptr,sptr->prev);				// set starting vtx to one at corner, if available
	  pptr=pptr->next;
	  }
	polygon_sort(sptr,1);						// sort by proximity
	if(debug_flag==200)printf("   sptr=%X  z=%5.3f  MPqty=%d \n",sptr,sptr->sz_level,sptr->pqty[MDL_PERIM]);
	if(debug_flag==200)printf("   %d support perim polygons made \n",sptr->pqty[SPT_PERIM]);
	
	// convert any particularly small polygons directly to fill line type without actually moving them.
	// otherwise, the offsetting function will destroy them an small areas will be left unsupported.
	/*
	pptr=sptr->pfirst[ptyp];	
	while(pptr!=NULL)
	  {
	  if(fabs(pptr->area)>1.0 && fabs(pptr->area)<10.0)		// if this polygon is really small ...
	    {
	    if(pptr->hole==FALSE)					// ... and it is not a hole ...
	      {
	      pnew=polygon_copy(pptr);					// ... make a copy 
	      ntyp=SPT_FILL;						// ... define target line type
	      if(first_layer_flag==TRUE)ntyp=SPT_LAYER_1;		// ... special exception for first layer
	      polygon_insert(sptr,ntyp,pnew);				// ... and add it to the slice as target type
	      }
	    }
	  pptr=pptr->next;
	  }
	*/

	// make border polygons from support perimeter polygons
	// note: even if support borders/offsets are not an active line type for the tool, they must still be
	// generated as the offset and cosequently the fills & close-offs depend on them for positioning.
	border_dist=0;							// reset to 0 in case no borders are generated
	if(debug_flag==200)printf("   processing support borders... \n");
	ntyp=SPT_BORDER;
	//if(model_linetype_active(mptr,ntyp)==TRUE && first_layer_flag==FALSE)
	if(model_linetype_active(mptr,ntyp)==TRUE)
	  {
	  lptr=linetype_find(slot,ntyp);
	  if(lptr!=NULL)
	    {
	    min_vector_length=lptr->minveclen;
	    //border_walls=(int)(lptr->wall_width/lptr->line_pitch);
	    //border_dist=lptr->line_width*0.5+mptr->support_gap;		// line width compensation
	    border_walls=1;
	    border_dist=0.001;
	    while(border_walls>0)
	      {
	      if(slice_has_many_polygons[mtyp] > slice_offset_type_qty[mtyp])
	        {
		if(Tool[slot].type==SUBTRACTIVE)
		  {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,(-1)*border_dist);} 	// generate the new border toward "outside"
		else 
		  {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,border_dist);}	// generate the new border toward "inside"
		}
	      else 
	        {
		if(Tool[slot].type==SUBTRACTIVE)
		  {slice_offset_winding(sptr,ptyp,ntyp,(-1)*border_dist);} 	// generate the new border toward "outside"
		else 
		  {slice_offset_winding(sptr,ptyp,ntyp,border_dist);}		// generate the new border toward "inside"
		}
	      border_walls--;
	      if(border_walls>0)border_dist+=lptr->line_pitch;		// increment the distance by a line pitch
	      }
	    border_dist+=lptr->line_width*0.5;				// add back other half of border line width
	    travel_dist=linetype_accrue_distance(sptr,ntyp);
	    if(travel_dist>0)sptr->pdist[ntyp]=travel_dist;
	    if(debug_flag==200)printf("   %d support border polygons made \n",sptr->pqty[ntyp]);
	    }
	  }
	
	// make offset polygons from support perimeter polygons
	offset_dist=0;							// reset to 0 in case no offsets are generated
	if(debug_flag==200)printf("   processing support offsets... \n");
	ntyp=SPT_OFFSET;						// define target line type
	if(model_linetype_active(mptr,ntyp)==TRUE)
	  {
	  //if(first_layer_flag==TRUE)ntyp=SPT_LAYER_1;
	  lptr=linetype_find(slot,ntyp);
	  if(debug_flag==200)printf("   ntyp=%d  slot=%d  lptr=%X \n",ntyp,slot,lptr);
	  if(lptr!=NULL)
	    {
	    offset_walls=(int)(lptr->wall_width/lptr->line_pitch);
	    offset_dist=border_dist+lptr->line_width*0.5;		// compensate for line width
	    while(offset_walls>0)
	      {
	      if(slice_has_many_polygons[mtyp] > slice_offset_type_qty[mtyp])
	        {
		if(Tool[slot].type==SUBTRACTIVE)
		  {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,(-1)*offset_dist);}	// generate the offset
		else 
		  {slice_offset_winding_by_polygon(sptr,ptyp,ntyp,offset_dist);}	// generate the offset
		}
	      else 
	        {
		if(Tool[slot].type==SUBTRACTIVE)
		  {slice_offset_winding(sptr,ptyp,ntyp,(-1)*offset_dist);}	// generate the offset
		else 
		  {slice_offset_winding(sptr,ptyp,ntyp,offset_dist);}		// generate the offset
		}
	      offset_walls--;
	      if(offset_walls>0)offset_dist+=lptr->line_pitch;		// increment the distance by a line pitch
	      }
	    offset_dist+=lptr->line_width*0.5;
	    travel_dist=linetype_accrue_distance(sptr,ntyp);
	    if(travel_dist>0)sptr->pdist[ntyp]=travel_dist;
	    if(debug_flag==200)printf("   %d support offset polygons made \n",sptr->pqty[ntyp]);
	    }
	  }

	}
      }
      
    // generate tool paths for MEASUREMENT
    if(oper_ID>=OP_MEASURE_X && oper_ID<=OP_MEASURE_HOLES)
      {
      }
      
    // generate tool paths for MARKING
    //if(oper_ID==OP_MARK)
    //  {
    //  }

    // generate tool paths for CURING
    if(oper_ID==OP_CURE)
      {
      }
      
    // generate tool paths for PICK AND PLACE
    if(oper_ID==OP_PLACE)
      {
      }
      
    z_lvl-=Tool[slot].matl.layer_height;
    if(z_lvl<mptr->zmin[MODEL])break;
    //if(set_start_at_crt_z==TRUE && set_force_z_to_table==TRUE && z_lvl<z_cut)break;
    if(z_lvl<(mptr->zmin[MODEL]+(1.999*mptr->slice_thick)))first_layer_flag=TRUE;
    idle_start_time=time(NULL);						// reset idle time
    sold=sptr;
    
    // in the case of vector marking or vector cutting only one slice is needed
    if(oper_ID==OP_MARK_OUTLINE || oper_ID==OP_MARK_AREA || oper_ID==OP_MARK_CUT)
      {
      mptr->slice_first[MODEL]=sptr;
      mptr->slice_last[MODEL]=sptr;
      break;
      }
    }
  first_layer_flag=FALSE;
  fidelity_mode_flag=FALSE;
  min_polygon_area=old_min_polygon_area;

  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  //if(debug_flag==200)
  printf("\nSlice Deck: exit - slices=%d  for  tool=%d  of type=%d\n",scount,slot,mtyp);
  return(1);
}

// Function to process model and support FILLs and CLOSE-OFFs for a single slice for display or printing by model reference
// Note that zlvl is in build volume z level here.  In other words zlvl = mptr->zoff[MODEL] + slice_into_model_z_depth
int slice_fill_model(model *mptr, float zlvl)
{
  int		mtyp=MODEL;
  int		i,slot,slot_check[MAX_TOOLS];
  int		mdl_aco_lyrs,mdl_bco_lyrs;
  int		int_aco_lyrs,int_bco_lyrs;
  int		spt_aco_lyrs,spt_bco_lyrs;
  int		mdl_ctr=0;
  int		contour_ref;
  float 	lc_angle,infill_angle,uc_angle;
  float 	travel_dist;
  slice 	*sptr;
  slice		*slc_mdl_below,*slc_mdl_zlvl,*slc_mdl_above;
  slice		*slc_int_below,*slc_int_zlvl,*slc_int_above;
  slice		*slc_spt_below,*slc_spt_zlvl,*slc_spt_above;
  genericlist	*aptr;
  operation	*optr;
  linetype	*mdl_lower_ptr,*mdl_upper_ptr,*mdl_fill_ptr,*mdl_layer1_ptr;
  linetype	*int_lower_ptr,*int_upper_ptr,*int_fill_ptr,*int_layer1_ptr;
  linetype	*spt_lower_ptr,*spt_upper_ptr,*spt_fill_ptr,*spt_layer1_ptr;
  linetype 	*lptr;

  // check model inputs
  if(mptr==NULL)return(0);						// if here without a model, leave
  if(mptr->input_type==GERBER)return(0);				// can't do Gerber files *yet*
  if(mptr->grayscale_mode>0)return(0);
  if(cnc_profile_cut_flag==TRUE)return(0);				// no filling when profile cut
  //if(zlvl<(mptr->zmin+mptr->zoff) || zlvl>(mptr->zmax+mptr->zoff))return(0);	// if z level out of bounds, leave

  if(debug_flag==1099)printf("\nSlice Fill by MODEL Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  if(debug_flag==1099)printf("\nEntering slice engine by model -----------------------------\n");
    
  // loop thru all operations to find all tools/materials/linetypes
  for(i=0;i<MAX_TOOLS;i++){slot_check[i]=FALSE;}
  aptr=mptr->oper_list;
  while(aptr!=NULL)
    {
    //printf("  processing operation: ID=%d Name=%s\n",aptr->ID,aptr->name);
    slot=aptr->ID;
    if(slot<0 || slot>=MAX_TOOLS)return(0);				// if tool location not reasonable, leave
    if(Tool[slot].state<TL_LOADED)return(0);				// if tool not yet defined, leave
    //if(slot_check[slot]==TRUE){aptr=aptr->next;continue;}		// if already filled for this slot
    //slot_check[slot]=TRUE;
    
    // find the tool operation that matches the current model operation
    optr=Tool[slot].oper_first;
    while(optr!=NULL)
      {
      if(strstr(aptr->name,optr->name)!=NULL)break;
      optr=optr->next;
      }
    if(optr==NULL)return(0);						// if no tool operation was found...
    
    // certain operations don't want to be filled... check them here
    if(optr->ID==OP_MARK_OUTLINE){aptr=aptr->next;continue;}
    if(optr->ID==OP_MARK_CUT){aptr=aptr->next;continue;}
    
    // init model slice data
    mdl_ctr++;
    slc_mdl_below=NULL; slc_mdl_zlvl=NULL; slc_mdl_above=NULL;

    mdl_bco_lyrs=0;
    mdl_lower_ptr=linetype_find(slot,MDL_LOWER_CO);
    if(mdl_lower_ptr!=NULL)mdl_bco_lyrs=(int)(mdl_lower_ptr->thickness/Tool[slot].matl.layer_height);
    if(mdl_bco_lyrs>10)mdl_bco_lyrs=10;

    mdl_aco_lyrs=0;
    mdl_upper_ptr=linetype_find(slot,MDL_UPPER_CO);
    if(mdl_upper_ptr!=NULL)mdl_aco_lyrs=(int)(mdl_upper_ptr->thickness/Tool[slot].matl.layer_height);
    if(mdl_aco_lyrs>10)mdl_aco_lyrs=10;

    mdl_fill_ptr=linetype_find(slot,MDL_FILL);
    
    // init internal structure slice data
    slc_int_below=NULL; slc_int_zlvl=NULL; slc_int_above=NULL;

    int_bco_lyrs=0;
    int_lower_ptr=linetype_find(slot,MDL_LOWER_CO);
    if(int_lower_ptr!=NULL)int_bco_lyrs=(int)(int_lower_ptr->thickness/Tool[slot].matl.layer_height);
    if(int_bco_lyrs>10)int_bco_lyrs=10;

    int_aco_lyrs=0;
    int_upper_ptr=linetype_find(slot,MDL_UPPER_CO);
    if(int_upper_ptr!=NULL)int_aco_lyrs=(int)(int_upper_ptr->thickness/Tool[slot].matl.layer_height);
    if(int_aco_lyrs>10)int_aco_lyrs=10;

    // init support slice data
    // note that these are NOT common with the mdl variables as support may require more/less close off layers
    slc_spt_below=NULL; slc_spt_zlvl=NULL; slc_spt_above=NULL;

    spt_bco_lyrs=0;
    spt_lower_ptr=linetype_find(slot,SPT_LOWER_CO);
    //if(spt_lower_ptr!=NULL)spt_bco_lyrs=(int)(spt_lower_ptr->thickness/Tool[slot].matl.layer_height);
    if(spt_lower_ptr!=NULL)spt_bco_lyrs=1;
    if(spt_bco_lyrs>10)spt_bco_lyrs=10;

    spt_aco_lyrs=0;
    spt_upper_ptr=linetype_find(slot,SPT_UPPER_CO);
    if(spt_upper_ptr!=NULL)spt_aco_lyrs=(int)(spt_upper_ptr->thickness/Tool[slot].matl.layer_height);
    if(spt_aco_lyrs>10)spt_aco_lyrs=10;

    spt_fill_ptr=linetype_find(slot,SPT_FILL);

    //printf("  slice find\n");
    slc_mdl_zlvl=slice_find(mptr,mtyp,slot,zlvl,mptr->slice_thick);	// find this model's slice at this z level
    if(slc_mdl_zlvl==NULL)return(0);					// make sure we found something to work with
    slc_int_zlvl=slc_mdl_zlvl;						// model and interal for current slice are same, above/below may be different
    slc_spt_zlvl=slc_mdl_zlvl;						// model and support for current slice are same, above/below may be different
    
    // identify the rest of the slices we will be working with.
    // note, they are already created but not associated to the local slice pointers
    slc_mdl_below=slc_mdl_zlvl;
    slc_mdl_above=slc_mdl_zlvl;
    for(i=0;i<mdl_bco_lyrs;i++)if(slc_mdl_below!=NULL)slc_mdl_below=slc_mdl_below->next;
    for(i=0;i<mdl_aco_lyrs;i++)if(slc_mdl_above!=NULL)slc_mdl_above=slc_mdl_above->prev;

    slc_int_below=slc_int_zlvl;
    slc_int_above=slc_int_zlvl;
    for(i=0;i<int_bco_lyrs;i++)if(slc_int_below!=NULL)slc_int_below=slc_int_below->next;
    for(i=0;i<int_aco_lyrs;i++)if(slc_int_above!=NULL)slc_int_above=slc_int_above->prev;

    slc_spt_below=slc_spt_zlvl;
    slc_spt_above=slc_spt_zlvl;
    for(i=0;i<spt_bco_lyrs;i++)if(slc_spt_below!=NULL)slc_spt_below=slc_spt_below->next;
    for(i=0;i<spt_aco_lyrs;i++)if(slc_spt_above!=NULL)slc_spt_above=slc_spt_above->prev;
  
    // display slice status  
    //printf("  slot=%d  slc_mdl_zlvl=%X  zlvl=%f \n",slot,slc_mdl_zlvl,zlvl);
    //printf("  bco_lyrs=%d  aco_lyrs=%d  slot=%d \n",bco_lyrs,aco_lyrs,slot);
    //if(slc_mdl_below!=NULL)printf("  slice below .: %X	P=%ld O=%ld F=%ld Z=%6.3f \n",slc_mdl_below,slc_mdl_below->pqty[MDL_PERIM],slc_mdl_below->pqty[MDL_OFFSET],slc_mdl_below->pqty[MDL_FILL],slc_mdl_below->sz_level);
    //if(slc_mdl_zlvl!=NULL)printf("  slice at zlvl: %X	P=%ld O=%ld F=%ld Z=%6.3f \n",slc_mdl_zlvl,slc_mdl_zlvl->pqty[MDL_PERIM],slc_mdl_zlvl->pqty[MDL_OFFSET],slc_mdl_zlvl->pqty[MDL_FILL],slc_mdl_zlvl->sz_level);
    //if(slc_mdl_above!=NULL)printf("  slice above .: %X	P=%ld O=%ld F=%ld Z=%6.3f \n",slc_mdl_above,slc_mdl_above->pqty[MDL_PERIM],slc_mdl_above->pqty[MDL_OFFSET],slc_mdl_above->pqty[MDL_FILL],slc_mdl_above->sz_level);
  
    // process surface close offs by comparing the current slice with the N below and N above slices to meet thickness request
    // offset the perimeter based on the number of walls requested
    // note that the order the offsets are generated are the order they are printed, generally from the inside-out.
    // note that a value of 0 for wall_count will still offset the perimeter by some portion of line width for accuracy
    
    // generate MODEL UPPER CLOSE OFFS
    // the portion of the current slice that does not overlap the above slice
    // find the unique area of the current slice from the above slice
    if(mdl_upper_ptr!=NULL && mdl_lower_ptr!=NULL && model_linetype_active(mptr,MDL_UPPER_CO)==TRUE)
      {
      // if upper close offs are being used and we are above lower close-offs ...
      if(mptr->top_co==TRUE && slc_mdl_zlvl->sz_level>mdl_lower_ptr->thickness)
        {
	// determine which contour is available to fill
	contour_ref=(-1);
	if(model_linetype_active(mptr,MDL_PERIM) ==TRUE){contour_ref=MDL_PERIM;}
	if(model_linetype_active(mptr,MDL_BORDER)==TRUE){contour_ref=MDL_BORDER;}
	if(model_linetype_active(mptr,MDL_OFFSET)==TRUE){contour_ref=MDL_OFFSET;}
	if(contour_ref>0)						// only fill if a contour is present
	  {
	  slice_wipe(slc_mdl_zlvl,MDL_UPPER_CO,0);
	  lc_angle=mdl_upper_ptr->p_angle;
	  if((int)(slc_mdl_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)lc_angle+=90;	// rotate fill angle on every other slice
	  slice_linefill(NULL,slc_mdl_zlvl,slc_mdl_above,mdl_upper_ptr->wall_pitch,lc_angle,contour_ref,MDL_UPPER_CO,ACTION_XOR);	// fill lower close off
	  }
	travel_dist=linetype_accrue_distance(slc_mdl_zlvl,MDL_UPPER_CO);
	if(travel_dist>0)slc_mdl_zlvl->pdist[MDL_UPPER_CO]=travel_dist;
	}
      }
    
    // generate MODEL FILL
    // the portion of the current slice that overlaps both the below slice and the above slice
    // find the common area bt the current slice, the slice below, and the slice above
    if(mdl_fill_ptr!=NULL && model_linetype_active(mptr,MDL_FILL))
      {
      if(slc_mdl_above!=NULL && slc_mdl_below!=NULL)
	{
	if(mptr->vase_mode==FALSE)
	  {
	  // determine which contour is available to fill
	  contour_ref=(-1);
	  if(model_linetype_active(mptr,MDL_PERIM) ==TRUE){contour_ref=MDL_PERIM;}
	  if(model_linetype_active(mptr,MDL_BORDER)==TRUE){contour_ref=MDL_BORDER;}
	  if(model_linetype_active(mptr,MDL_OFFSET)==TRUE){contour_ref=MDL_OFFSET;}
	  if(contour_ref>0)						// only fill if a contour is present
	    {
	    slice_wipe(slc_mdl_zlvl,MDL_FILL,0);
	    mdl_fill_ptr=linetype_find(slot,MDL_FILL);
	    if(strstr(mdl_fill_ptr->pattern,"HONEYCOMB")!=NULL)
	      {
	      slice_honeycomb(slc_mdl_zlvl,slc_mdl_above,mdl_fill_ptr->wall_pitch,MDL_FILL,ACTION_AND);					
	      }
	    if(strstr(mdl_fill_ptr->pattern,"SQUARE")!=NULL)
	      {
	      infill_angle=0;
	      if((int)(slc_mdl_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)infill_angle+=90;	// rotate fill angle on every other slice
	      slice_linefill(slc_mdl_below,slc_mdl_zlvl,slc_mdl_above,mdl_fill_ptr->wall_pitch,infill_angle,contour_ref,MDL_FILL,ACTION_AND);
	      }
	    }
	  travel_dist=linetype_accrue_distance(slc_mdl_zlvl,MDL_FILL);
	  if(travel_dist>0)slc_mdl_zlvl->pdist[MDL_FILL]=travel_dist;
	  }
	}
      }
      
    // generate MODEL LOWER CLOSE OFFS
    // the portion of the current slice that does not overlap the below slice
    // find the unique area of the current slice from the below slice
    if(mdl_lower_ptr!=NULL && model_linetype_active(mptr,MDL_LOWER_CO))
      {
      // if bottom close offs are being used and not on thed base layer...
      if(mptr->btm_co==TRUE && slc_mdl_zlvl!=mptr->base_layer)
        {
	// if baselayer is NOT being used OR we are above layer 1 ....
	if(job.baselayer_flag==FALSE || fabs(zlvl-mptr->slice_thick)>CLOSE_ENOUGH)
	  {
	  // determine which contour is available to fill
	  contour_ref=(-1);
	  if(model_linetype_active(mptr,MDL_PERIM) ==TRUE){contour_ref=MDL_PERIM;}
	  if(model_linetype_active(mptr,MDL_BORDER)==TRUE){contour_ref=MDL_BORDER;}
	  if(model_linetype_active(mptr,MDL_OFFSET)==TRUE){contour_ref=MDL_OFFSET;}
	  if(contour_ref>0)						// only fill if a contour is present
	    {
	    slice_wipe(slc_mdl_zlvl,MDL_LOWER_CO,0);
	    uc_angle=mdl_lower_ptr->p_angle;								// note lc_anlge and uc_angle out of phase by 90 deg
	    if((int)(slc_mdl_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)uc_angle-=90;		// uc flips back to 45 when lc flips up to 135
	    slice_linefill(slc_mdl_below,slc_mdl_zlvl,NULL,mdl_lower_ptr->wall_pitch,uc_angle,contour_ref,MDL_LOWER_CO,ACTION_XOR);		// fill upper close off
	    }
	  travel_dist=linetype_accrue_distance(slc_mdl_zlvl,MDL_LOWER_CO);
	  if(travel_dist>0)slc_mdl_zlvl->pdist[MDL_LOWER_CO]=travel_dist;
	  }
	}
      }

    // generate MODEL LAYER 1
    // the interior portion of the first layer that falls on top of the build table.
    // it has unique parameters to aid in separation from the build table.
    int_layer1_ptr=linetype_find(slot,MDL_LAYER_1);
    if(int_layer1_ptr!=NULL && model_linetype_active(mptr,MDL_LAYER_1) && slc_mdl_zlvl!=mptr->base_layer)
      {
      // if bottom close offs are being used...
      //if(mptr->btm_co==TRUE)
        {
	// if z heigth is at first layer...
	if(fabs(zlvl-mptr->zoff[mtyp]) < (1.5*mptr->slice_thick))
	  {
	  // determine which contour is available to fill
	  contour_ref=(-1);
	  if(model_linetype_active(mptr,MDL_PERIM) ==TRUE){contour_ref=MDL_PERIM;}
	  if(model_linetype_active(mptr,MDL_BORDER)==TRUE){contour_ref=MDL_BORDER;}
	  if(contour_ref>0)						// only fill if a contour is present
	    {
	    slice_wipe(slc_mdl_zlvl,MDL_LAYER_1,0);
	    uc_angle=int_layer1_ptr->p_angle;								// note lc_anlge and uc_angle out of phase by 90 deg
	    if((int)(slc_mdl_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)uc_angle-=90;		// uc flips back to 45 when lc flips up to 135
	    slice_linefill(NULL,slc_mdl_zlvl,NULL,int_layer1_ptr->wall_pitch,uc_angle,contour_ref,MDL_LAYER_1,ACTION_OR);		// fill upper close off
	    }
	  travel_dist=linetype_accrue_distance(slc_mdl_zlvl,MDL_LAYER_1);
	  if(travel_dist>0)slc_mdl_zlvl->pdist[MDL_LAYER_1]=travel_dist;
	  }
	}
      }

    // generate INTERNAL UPPER CLOSE OFFS
    if(mptr->vase_mode==TRUE && mptr->internal_spt==TRUE)
      {
      if(int_upper_ptr!=NULL && model_linetype_active(mptr,INT_UPPER_CO) && slc_int_zlvl->sz_level>int_lower_ptr->thickness)
	{
	if(zlvl>(mptr->zmax[mtyp]+mptr->zoff[mtyp]-mptr->slice_thick*3))
	  {
	  // determine which contour is available to fill
	  contour_ref=INT_PERIM;
	  if(model_linetype_active(mptr,INT_BORDER)==TRUE){contour_ref=INT_BORDER;}
	  if(model_linetype_active(mptr,INT_OFFSET)==TRUE){contour_ref=INT_OFFSET;}
	  slice_wipe(slc_int_zlvl,INT_UPPER_CO,0);
	  lc_angle=int_upper_ptr->p_angle;
	  if((int)(slc_int_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)lc_angle+=90;	// rotate fill angle on every other slice
	  slice_linefill(NULL,slc_int_zlvl,slc_int_above,int_upper_ptr->wall_pitch,lc_angle,contour_ref,INT_UPPER_CO,ACTION_XOR);	// fill lower close off
	  travel_dist=linetype_accrue_distance(slc_int_zlvl,INT_UPPER_CO);
	  if(travel_dist>0)slc_int_zlvl->pdist[INT_UPPER_CO]=travel_dist;
	  }
	}
      }
      
    // generate INTERNAL FILL
    if(mptr->vase_mode==TRUE && mptr->internal_spt==TRUE)
      {
      int_fill_ptr=linetype_find(slot,INT_FILL);
      if(int_fill_ptr!=NULL && model_linetype_active(mptr,INT_FILL))
	{
	if(slc_int_above!=NULL && slc_int_below!=NULL)
	  {
	  // determine which contour is available to fill
	  contour_ref=INT_PERIM;
	  if(model_linetype_active(mptr,INT_BORDER)==TRUE){contour_ref=INT_BORDER;}
	  if(model_linetype_active(mptr,INT_OFFSET)==TRUE){contour_ref=INT_OFFSET;}
	  // clear any existing fill data
	  slice_wipe(slc_int_zlvl,INT_FILL,0);
	  int_fill_ptr=linetype_find(slot,INT_FILL);
	  // go fill the slice
	  if(strstr(int_fill_ptr->pattern,"HONEYCOMB")!=NULL)
	    {
	    slice_honeycomb(slc_int_zlvl,slc_int_above,int_fill_ptr->wall_pitch,INT_FILL,ACTION_AND);					
	    }
	  if(strstr(int_fill_ptr->pattern,"SQUARE")!=NULL)
	    {
	    infill_angle=0;
	    if((int)(slc_int_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)infill_angle+=90;	// rotate fill angle on every other slice
	    slice_linefill(slc_int_below,slc_int_zlvl,slc_int_above,int_fill_ptr->wall_pitch,infill_angle,contour_ref,INT_FILL,ACTION_AND);
	    }
	  travel_dist=linetype_accrue_distance(slc_int_zlvl,INT_FILL);
	  if(travel_dist>0)slc_int_zlvl->pdist[INT_FILL]=travel_dist;
	  }
	}
      }
    
    // generate INTERNAL LOWER CLOSE OFFS
    if(mptr->vase_mode==TRUE && mptr->internal_spt==TRUE)
      {
      if(int_lower_ptr!=NULL && model_linetype_active(mptr,INT_LOWER_CO) && slc_int_zlvl!=mptr->base_layer)
	{
	if(zlvl<(mptr->zoff[mtyp]+mptr->slice_thick*3))
	  {
	  // if baselayer is being used, use layer_1 params to aid in separation...
	  if(fabs(zlvl-mptr->slice_thick)<CLOSE_ENOUGH && job.baselayer_flag==TRUE)int_lower_ptr=linetype_find(slot,INT_LAYER_1);
	
	  // determine which contour is available to fill
	  contour_ref=INT_PERIM;
	  if(model_linetype_active(mptr,INT_BORDER)==TRUE){contour_ref=INT_BORDER;}
	  if(model_linetype_active(mptr,INT_OFFSET)==TRUE){contour_ref=INT_OFFSET;}

	  slice_wipe(slc_int_zlvl,INT_LAYER_1,0);
	  slice_wipe(slc_int_zlvl,INT_LOWER_CO,0);
	  uc_angle=int_lower_ptr->p_angle;								// note lc_anlge and uc_angle out of phase by 90 deg
	  if((int)(slc_int_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)uc_angle-=90;		// uc flips back to 45 when lc flips up to 135
	  slice_linefill(slc_int_below,slc_int_zlvl,NULL,int_lower_ptr->wall_pitch,uc_angle,contour_ref,INT_LOWER_CO,ACTION_XOR);		// fill upper close off
	  travel_dist=linetype_accrue_distance(slc_int_zlvl,INT_LOWER_CO);
	  if(travel_dist>0)slc_int_zlvl->pdist[INT_LOWER_CO]=travel_dist;
	  }
	}
      }
  
    // generate BASE & PLATFORM LAYERS
    if(optr->ID==OP_ADD_BASE_LAYER && slc_mdl_zlvl==mptr->base_layer)
      {
      printf("  Slice fill: filling %d mdl perim layer polygons for base layer\n",slc_mdl_zlvl->pqty[MDL_PERIM]);
	
      slice_wipe(slc_mdl_zlvl,BASELYR,0);
      slice_wipe(slc_mdl_zlvl,PLATFORMLYR1,0);
      slice_wipe(slc_mdl_zlvl,PLATFORMLYR2,0);

      lptr=linetype_find(slot,BASELYR);
      if(lptr!=NULL)slice_linefill(NULL,slc_mdl_zlvl,NULL,lptr->wall_pitch,lptr->p_angle,MDL_OFFSET,BASELYR,ACTION_AND);    
      
      lptr=linetype_find(slot,PLATFORMLYR1);
      if(lptr!=NULL)slice_linefill(NULL,slc_mdl_zlvl,NULL,lptr->wall_pitch,lptr->p_angle,MDL_OFFSET,PLATFORMLYR1,ACTION_AND);    
      
      lptr=linetype_find(slot,PLATFORMLYR2);
      if(lptr!=NULL)slice_linefill(NULL,slc_mdl_zlvl,NULL,lptr->wall_pitch,lptr->p_angle,MDL_OFFSET,PLATFORMLYR2,ACTION_AND);    
      
      printf("  Slice fill: BLqty=%d  P1qty=%d  P2qty=%d \n",slc_mdl_zlvl->pqty[BASELYR],slc_mdl_zlvl->pqty[PLATFORMLYR1],slc_mdl_zlvl->pqty[PLATFORMLYR2]);
      }
  
    // generate SUPPORT UPPER CLOSE OFFS
    if(spt_upper_ptr!=NULL && model_linetype_active(mptr,SPT_UPPER_CO))
      {
      // determine which contour is available to fill
      contour_ref=SPT_PERIM;
      if(model_linetype_active(mptr,SPT_BORDER)==TRUE){contour_ref=SPT_BORDER;}
      if(model_linetype_active(mptr,SPT_OFFSET)==TRUE){contour_ref=SPT_OFFSET;}
      slice_wipe(slc_spt_zlvl,SPT_UPPER_CO,0);
      lc_angle=spt_upper_ptr->p_angle;
      if((int)(slc_spt_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)lc_angle+=90;	// rotate fill angle on every other slice
      slice_linefill(NULL,slc_spt_zlvl,slc_spt_above,spt_upper_ptr->wall_pitch,lc_angle,contour_ref,SPT_UPPER_CO,ACTION_XOR);	// fill lower close off
      travel_dist=linetype_accrue_distance(slc_spt_zlvl,SPT_UPPER_CO);
      if(travel_dist>0)slc_spt_zlvl->pdist[SPT_UPPER_CO]=travel_dist;
      }

    // generate SUPPORT FILL
    if(optr->ID==OP_ADD_SUPPORT_MATERIAL && spt_fill_ptr!=NULL)
      {
      // determine which contour is available to fill
      contour_ref=SPT_PERIM;
      if(model_linetype_active(mptr,SPT_BORDER)==TRUE){contour_ref=SPT_BORDER;}
      if(model_linetype_active(mptr,SPT_OFFSET)==TRUE){contour_ref=SPT_OFFSET;}
      slice_wipe(slc_spt_zlvl,SPT_FILL,0);
      if(strstr(spt_fill_ptr->pattern,"LINE")!=NULL)
	{
	infill_angle=spt_fill_ptr->p_angle;
	slice_linefill(slc_spt_below,slc_spt_zlvl,slc_spt_above,spt_fill_ptr->wall_pitch,infill_angle,contour_ref,SPT_FILL,ACTION_AND);
	}
      if(strstr(spt_fill_ptr->pattern,"SQUARE")!=NULL)
	{
	infill_angle=spt_fill_ptr->p_angle;
	if((int)(slc_spt_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)infill_angle+=90;	// rotate fill angle on every other slice
	slice_linefill(slc_spt_below,slc_spt_zlvl,slc_spt_above,spt_fill_ptr->wall_pitch,infill_angle,contour_ref,SPT_FILL,ACTION_AND);
	}
      if(strstr(spt_fill_ptr->pattern,"HONEYCOMB")!=NULL)
	{
	slice_honeycomb(slc_spt_zlvl,slc_spt_above,spt_fill_ptr->wall_pitch,SPT_FILL,ACTION_AND);	
	}
      travel_dist=linetype_accrue_distance(slc_spt_zlvl,SPT_FILL);
      if(travel_dist>0)slc_spt_zlvl->pdist[SPT_FILL]=travel_dist;
      }

    // generate SUPPORT LOWER CLOSE OFFS
    if(spt_lower_ptr!=NULL && model_linetype_active(mptr,SPT_LOWER_CO) && slc_mdl_zlvl!=mptr->base_layer)
      {
      // if baselayer is being used, use layer_1 params to aid in separation...
      if(fabs(zlvl-mptr->slice_thick)<CLOSE_ENOUGH && job.baselayer_flag==TRUE)spt_lower_ptr=linetype_find(slot,SPT_LAYER_1);

      // determine which contour is available to fill
      contour_ref=SPT_PERIM;
      if(model_linetype_active(mptr,SPT_BORDER)==TRUE){contour_ref=SPT_BORDER;}
      if(model_linetype_active(mptr,SPT_OFFSET)==TRUE){contour_ref=SPT_OFFSET;}
      slice_wipe(slc_spt_zlvl,SPT_LAYER_1,0);
      slice_wipe(slc_spt_zlvl,SPT_LOWER_CO,0);
      uc_angle=spt_lower_ptr->p_angle;								// note lc_anlge and uc_angle out of phase by 90 deg
      if((int)(slc_spt_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)uc_angle-=90;		// uc flips back to 45 when lc flips up to 135
      //if(slc_spt_zlvl->sz_level<(2*Tool[slot].matl.layer_height))slc_spt_below=NULL;		// use lc on bottom layer
      slice_linefill(slc_spt_below,slc_spt_zlvl,NULL,spt_lower_ptr->wall_pitch,uc_angle,contour_ref,SPT_LOWER_CO,ACTION_XOR);
      travel_dist=linetype_accrue_distance(slc_spt_zlvl,SPT_LOWER_CO);
      if(travel_dist>0)slc_spt_zlvl->pdist[SPT_LOWER_CO]=travel_dist;
      }
  
    // generate SUPPORT LAYER 1
    // the interior portion of the first layer that falls on top of the baselayer/platform layers.
    // it has unique parameters to aid in separation from the baselayer/platform layers.
    int_layer1_ptr=linetype_find(slot,SPT_LAYER_1);
    if(int_layer1_ptr!=NULL && model_linetype_active(mptr,SPT_LAYER_1) && slc_spt_zlvl!=mptr->base_layer)
      {
      // if baselayer is being used and on first layer then use layer_1 params to aid in separation...
      //if(job.baselayer_flag==TRUE && fabs(zlvl-mptr->zoff[mtyp])<(1.5*mptr->slice_thick))
      if(fabs(zlvl-mptr->zoff[mtyp])<(1.5*mptr->slice_thick))
	{
	// determine which contour is available to fill
	contour_ref=SPT_PERIM;
	if(model_linetype_active(mptr,SPT_BORDER)==TRUE){contour_ref=SPT_BORDER;}
	//if(model_linetype_active(mptr,SPT_OFFSET)==TRUE){contour_ref=SPT_OFFSET;}
	slice_wipe(slc_spt_zlvl,SPT_LAYER_1,0);
	uc_angle=int_layer1_ptr->p_angle;								// note lc_anlge and uc_angle out of phase by 90 deg
	if((int)(slc_spt_zlvl->sz_level/Tool[slot].matl.layer_height)%2==0)uc_angle-=90;		// uc flips back to 45 when lc flips up to 135
	slice_linefill(NULL,slc_spt_zlvl,NULL,int_layer1_ptr->wall_pitch,uc_angle,contour_ref,SPT_LAYER_1,ACTION_OR);		// fill upper close off
	travel_dist=linetype_accrue_distance(slc_spt_zlvl,SPT_LAYER_1);
	if(travel_dist>0)slc_spt_zlvl->pdist[SPT_LAYER_1]=travel_dist;
	}
      }
  
    aptr=aptr->next;
    }
  
  //printf("  Model: %X  Slice 1st: %X  Slice Qty: %d \n",mptr,mptr->slice_first[slot],mptr->slice_qty[slot]);
  //printf("Exiting slice engine by model --------------------------------\n");
  //printf("Slice Fill Exit:   vtx=%ld  vec=%ld \n\n",vertex_mem,vector_mem);

  return(1);
}

// Function to process FILL, CLOSE-OFFS, for MODEL and SUPPORT for a single slice for display or printing 
int slice_fill_all(float zlvl)
{
  model 	*mptr;

  // check inputs
  if(job.model_first==NULL)return(0);					// if here without a model, leave
  //if(zlvl<ZMin || zlvl>ZMax)return(0);				// if z level out of bounds, leave

  // loop thru models and apply filling based on the requested operations and available tool functions
  mptr=job.model_first;							// start with first model
  while(mptr!=NULL)							// loop thru all models
    {
    if(mptr->reslice_flag==FALSE)					// if it does NOT require reslicing...
      {
      slice_fill_model(mptr,zlvl);
      }
    mptr=mptr->next;
    }

  return(1);
}

// Function to create a straight skeleton (used for offsetting) of the input slice
int slice_skeleton(slice *sinpt, int source_typ, int ptyp, float odist)
{
    int		i,vec_int,psv_cnt;
    int		done_incrementing,bi_result,vecnew_doa,intersect_flag;
    int 	cA_to_nA_count,cA_to_nB_count,cB_to_nA_count,cB_to_nB_count;
    float 	inc_dist,vdist0,vdist1,vdist2,vdist3,perp_dist,distxy;
    double 	xn,x1,x2,x3,yn,y1,y2,y3,s,t,area;
    vertex	*vptr,*vnew,*vtail,*vtip,*vtxA,*vtxB;
    vector	*vec_ss,*vec_start,*vec_crt,*vec_nxt;
    vector 	*vecptr,*vecnxt,*vecpre,*vecdel;
    vector	*vecpoly,*vecold,*vecnew,*vecNptr;
    polygon 	*pinpt,*pptr,*pout;

    // ensure valid input
    if(sinpt==NULL)return(0);
    pout=NULL;
    
    //debug_flag=0;
    //if(sinpt->sz_level>0.00 && sinpt->sz_level<0.800)debug_flag=202;
    if(debug_flag==202)printf("\nSS: entry\n");

    // create copy of source perimeter polygons into a perimeter vector list.  the reason for doing it this way
    // (versus using the sinpt->vec_list[typ] vector lists) is because the processed polygons offer more useful
    // information such as whether the vectors are part of a hole or not, etc.
    if(sinpt->perim_vec_first[source_typ]==NULL)
      {
      vecold=NULL;
      pptr=sinpt->pfirst[source_typ];
      while(pptr!=NULL)
        {
	vecpoly=polylist_2_veclist(pptr,source_typ);
	if(vecpoly!=NULL)
	  {
	  if(sinpt->perim_vec_first[source_typ]==NULL)sinpt->perim_vec_first[source_typ]=vecpoly;
	  if(vecold!=NULL){vecold->next=vecpoly;vecpoly->prev=vecold;}	// add linkage bt polygons
	  vecptr=vecpoly;
	  while(vecptr!=NULL)						// find last vector from this polygon
	    {
	    vecold=vecptr;						// save last vector as vecold
	    vecptr=vecptr->next;
	    if(vecptr==vecpoly)break;
	    }
	  }
	pptr=pptr->next;
	}
      }
    if(debug_flag==202)printf("  perimeter vector list created...\n");


    // calculate bisector between adjacent vectors of perimeter vectors and strore in straight skeleton.
    // initially use only the ss_wavefront_increment value of offset to help ensure we are offsetting to the right direction.
    // the next loop will adjust bisector length depending on what it intersects (i.e. other bisectors) as the wave
    // front moves forward.
    if(sinpt->ss_vec_first[ptyp]==NULL)					// if the skeleton for this slice is not yet made
      {
      vnew=vertex_make();						// make a reusable scratch vtx
      vec_crt=sinpt->perim_vec_first[source_typ];			// start with first vector in whole list
      vec_start=vec_crt;						// save the starting vector FOR THIS POLYGON
      while(vec_crt!=NULL)						// loop thru entire perim vec list which may consist of multiple polygons
	{	
	// identify the next vector FOR THIS POLYGON
	vec_nxt=vec_crt->next;
	if(vec_nxt==NULL)vec_nxt=vec_start;
	if(vec_nxt->psrc!=vec_crt->psrc)				// if the next vector didn't come from the same perim polygon...
	  {
	  vec_nxt=vec_start;						// ... point it back to the first vector in this polygon
	  vec_start=vec_crt->next;					// ... and save what will be the first vector in the next polygon
	  }

	// determine bisector vector direction
	bi_result=vector_bisector_get_direction(vec_crt,vec_nxt,vnew);
	if(bi_result<1)							// if returns in error...
	  {
	  vec_crt=vec_crt->next;
	  continue;
	  }
	
	// add bisector direction to base (tail) to get complete vector
	vnew->x=vec_crt->tail->x+vnew->x;
	vnew->y=vec_crt->tail->y+vnew->y;
	vnew->z=vec_crt->tail->z;

	// make the ss vector
	vtip=vertex_copy(vnew,NULL);
	vtail=vertex_copy(vec_crt->tail,NULL);
	vec_ss=vector_make(vtip,vtail,0);
	
	// set ss vector params
	pinpt=vec_crt->psrc;
	if(pinpt!=NULL)
	  {
	  //vec_ss->type=pinpt->ID;
	  vec_ss->type=bi_result;
	  vec_ss->member=pinpt->ID;
	  vec_ss->psrc=pinpt;						// save address of source polygon
	  }
	vec_ss->vsrcA=vec_crt;						// note they share their TAIL vtx
	vec_ss->vsrcB=vec_nxt;						// note they share their TIP vtx
	vec_ss->status=1;
	
	// now adjust the bisector vector length down to the wave_front size so that this point is
	// more likely to stay within the bounds of polygons with fine details.
	vector_bisector_set_length(vec_ss,ss_wavefront_increment);	// adjust length to compensate for angle
	
	// if the polygon contains material, but the new point is not inside... or vice versa
	vtip=vec_ss->tip;
	if(pinpt!=NULL)
	  {
	  if(pinpt->hole==0 && polygon_contains_point(pinpt,vtip)==FALSE)vector_rotate(vec_ss,PI);
	  if(pinpt->hole>0 && polygon_contains_point(pinpt,vtip)==TRUE)vector_rotate(vec_ss,PI);
	  }
	
	// insert into ss veclist of the slice
	vector_insert(sinpt,ptyp,vec_ss);

	// move onto next vector in source type vec list
	vec_crt=vec_crt->next;
	}
      vertex_destroy(vnew);
      ss_wavefront_crtdist=ss_wavefront_increment;
      }
    if(debug_flag==202)printf("  bisectors created...\n");

    // at this point we should have a straight skeleton vector list that is simply the extended bisectors of all
    // polygon vtxs pointing in toward material and each with a length of ss_wavefront_increment.

    // run an increment event loop where we'll increment the length of the bisectors by some small amount, check for
    // edge events and split events, process any if found, then increment again.  we will only increment as far as the
    // maximum offset (as loaded from the materials.xlm file) dictates.
    i=0;
    psv_cnt=0;
    if(odist>ss_wavefront_maxdist)odist=ss_wavefront_maxdist;
    inc_dist=ss_wavefront_crtdist;
    vnew=vertex_make();
    done_incrementing=FALSE;
    while(done_incrementing==FALSE)					// increment out only as far as odist
      {
      if(debug_flag==202)printf("  inc_dist=%f odist=%f \n",inc_dist,odist);
	
      // build list of offset vectors at this location (needed to detect split events)
      
      
      // loop thru entire ss veclist to find any intersections with another skeleton vector (edge event) or any
      // intersections with offset vectors (split event).  assume that each skeleton vector will only have at most
      // one intersection.
      vecptr=sinpt->ss_vec_first[ptyp];
      while(vecptr!=NULL)
	{
	vecptr->tip->z=sinpt->sz_level;
	vecptr->tail->z=sinpt->sz_level;
 	if(vecptr->status!=1)
	  {
	  vecptr=vecptr->next;
	  if(vecptr==sinpt->ss_vec_first[ptyp])break;
	  continue;
	  }
	  
	// loop thru offset list to find any split events which are defined by skeleton vectors intersection with the wavefront
	
	  
	// loop thru all of veclist to find any edge events which are defined by skeleton vectors intersecting
	vecnxt=sinpt->ss_vec_first[ptyp];
	while(vecnxt!=NULL)
	  {
	  if(vecnxt==vecptr){vecnxt=vecnxt->next;continue;}
	  if(vecnxt->psrc==vecptr->psrc)
	    {
	    // test if actual intersection occurs
	    intersect_flag=FALSE;
	    vec_int=vector_intersect(vecptr,vecnxt,vnew);		// get intersection pt of skeleton vectors, if any
	    if(vec_int==204 || vec_int==206 || vec_int==236 || vec_int==238)	// if actual intersection...
	      {
	      vnew->z=sinpt->sz_level;
	      if(vecnxt->status==1){break;}				// if intersecting with a live vector... process them.
	      else {vecptr->status=2;vecnxt=NULL;break;}		// if intersecting with another dead vector... kill this one
	      }
	    if(vec_int==140 || vec_int==141 || vec_int==142)		// if a "near" intersection of vecA...
	      {
	      distxy=vertex_distance(vnew,vecptr->tip);		
	      if(distxy<ss_wavefront_increment)intersect_flag=TRUE;	//... within a wave front increment
	      distxy=vertex_distance(vnew,vecptr->tail);
	      if(distxy<ss_wavefront_increment)intersect_flag=TRUE;
	      }
	    if(vec_int==200 || vec_int==216 || vec_int==232)		// if a "near" intersection of vecB...
	      {
	      distxy=vertex_distance(vnew,vecnxt->tip);
	      if(distxy<ss_wavefront_increment)intersect_flag=TRUE;	//... within a wave front increment
	      distxy=vertex_distance(vnew,vecnxt->tail);
	      if(distxy<ss_wavefront_increment)intersect_flag=TRUE;
	      }
	    if(intersect_flag==TRUE)
	      {
	      // do nothing at the moment
	      }
	    }
	  vecnxt=vecnxt->next;
	  if(vecnxt==sinpt->ss_vec_first[ptyp])break;
	  }
	  
	// adjust to its intersection point if found (which should remove it from hitting any more intersections),
	// and create a new vector that is the bisector of the two intersecting bisectors with adjustments... all below.
	if(vecnxt!=NULL)
	  {
	  if(debug_flag==202)printf("   vec_int=%d \n",vec_int);

	  // adjust the tips of vecptr and vecnxt to end at their intersection pt.
	  vecptr->tip->x=vnew->x;vecptr->tip->y=vnew->y;vecptr->tip->z=vnew->z;
	  vecnxt->tip->x=vnew->x;vecnxt->tip->y=vnew->y;vecnxt->tip->z=vnew->z;
	  vector_magnitude(vecptr);					// reset mag since end pt adjusted
	  vector_magnitude(vecnxt);

	  // identify the correct polygon edges to use as parents to the new bisector.
	  // the correct pair of polygon parents are the pair that are farthest apart out of the four
	  // possible candidates: vecptr->vsrcA, vecptr->vsrcB, vecnxt->vsrcA, and vecnxt->vsrcB
	  // count the number of vectors between each possible combination to find right ones.
	  // first check if bisectors are neighbors and if they share one common polygon vector.
	  vec_crt=vecptr->vsrcA; 
	  vec_nxt=vecnxt->vsrcB;
	  i=0; cA_to_nA_count=0; cA_to_nB_count=0;
	  vecNptr=vecptr->vsrcA;
	  while(vecNptr!=NULL)
	    {
	    i++;
	    vecNptr=vecNptr->next;
	    if(vecNptr==vecnxt->vsrcA)cA_to_nA_count=i;
	    if(vecNptr==vecnxt->vsrcB){cA_to_nB_count=i;break;}
	    }
	  i=0; cB_to_nA_count=0; cB_to_nB_count=0;
	  vecNptr=vecptr->vsrcB;
	  while(vecNptr!=NULL)
	    {
	    i++;
	    vecNptr=vecNptr->next;
	    if(vecNptr==vecnxt->vsrcA)cB_to_nA_count=i;
	    if(vecNptr==vecnxt->vsrcB){cB_to_nB_count=i;break;}
	    }
	  if(cA_to_nA_count>cA_to_nB_count && cA_to_nA_count>cB_to_nA_count && cA_to_nA_count>cB_to_nB_count)
	    {vec_crt=vecptr->vsrcA; vec_nxt=vecnxt->vsrcA;}
	  if(cA_to_nB_count>cA_to_nA_count && cA_to_nB_count>cB_to_nA_count && cA_to_nB_count>cB_to_nB_count)
	    {vec_crt=vecptr->vsrcA; vec_nxt=vecnxt->vsrcB;}
	  if(cB_to_nA_count>cA_to_nA_count && cB_to_nA_count>cA_to_nB_count && cB_to_nA_count>cB_to_nB_count)
	    {vec_crt=vecptr->vsrcB; vec_nxt=vecnxt->vsrcA;}
	  if(cB_to_nB_count>cA_to_nA_count && cB_to_nB_count>cA_to_nB_count && cB_to_nB_count>cB_to_nA_count)
	    {vec_crt=vecptr->vsrcB; vec_nxt=vecnxt->vsrcB;}

	  // figure out the new bisector direction - from polygon parent intersection pt. to ss vector intersection pt.
	  vecnew_doa=FALSE;						// set flag that vecnew is NOT dead on arrival
	  vtail=vertex_make();						// create a place to hold parent intersection pt.
	  vec_int=vector_intersect(vec_crt,vec_nxt,vtail);		// get intersection pt of polygon parents gives tail pt.
	  //if(vec_int==136 || vec_int==255)				// if vectors are parallel and NONcolinear...
	  if(vec_int==255)						// if vectors are parallel and NONcolinear...
	    {
	    // ... we want to know if the vectors are far enough apart to keep this skeleton vector, or to
	    // mark it as spent.  do this by creating a perpendicular line to vec_crt then intersecting it with
	    // vec_nxt, calculating that distance, and compare it to our current wavefront offset.
	    perp_dist=10*inc_dist;
	    vdist0=vertex_distance(vec_crt->tail,vec_nxt->tail);
	    if(vdist0<perp_dist)perp_dist=vdist0;
	    vdist1=vertex_distance(vec_crt->tail,vec_nxt->tip);
	    if(vdist1<perp_dist)perp_dist=vdist1;
	    vdist2=vertex_distance(vec_crt->tip,vec_nxt->tail);
	    if(vdist2<perp_dist)perp_dist=vdist2;
	    vdist3=vertex_distance(vec_crt->tip,vec_nxt->tip);
	    if(vdist3<perp_dist)perp_dist=vdist3;
	    if(perp_dist<(2.1*inc_dist)){vecnew_doa=TRUE;}		// set flag that the new vec about to be created is dead on arrival
	    else {vecnew_doa=FALSE;vec_int=256;}			// else not d.o.a. and process as if colinear
	    }
	  if(vec_int>256)						// if vectors are parallel AND colinear...
	    {
	    // ... we want to determine which pair of parent end pts are closest, the pt we want is their average.
	    vtxA=vec_crt->tail; vtxB=vec_nxt->tail;
	    vdist0=vertex_distance(vec_crt->tail,vec_nxt->tail);
	    vdist1=vertex_distance(vec_crt->tail,vec_nxt->tip);
	    vdist2=vertex_distance(vec_crt->tip,vec_nxt->tail);
	    vdist3=vertex_distance(vec_crt->tip,vec_nxt->tip);
	    if(vdist0<vdist1 && vdist0<vdist2 && vdist0<vdist3) {vtxA=vec_crt->tail; vtxB=vec_nxt->tail;}
	    if(vdist1<vdist0 && vdist1<vdist2 && vdist1<vdist3) {vtxA=vec_crt->tail; vtxB=vec_nxt->tip;}
	    if(vdist2<vdist0 && vdist2<vdist1 && vdist2<vdist3) {vtxA=vec_crt->tip;  vtxB=vec_nxt->tail;}
	    if(vdist3<vdist0 && vdist3<vdist1 && vdist3<vdist2) {vtxA=vec_crt->tip;  vtxB=vec_nxt->tip;}
	    vtail->x=(vtxA->x+vtxB->x)/2;
	    vtail->y=(vtxA->y+vtxB->y)/2;
	    vtail->z=sinpt->sz_level;
	    }
	  vtip=vertex_copy(vnew,NULL);					// set tip as bisector intersection pt.
	  
	  // make the vector, assign properties, and increment its source polygon vectors further around the perimeter
	  vecnew=vector_make(vtip,vtail,0);
	  if(vecptr->psrc!=NULL)
	    {
	    vecnew->member=vecptr->psrc->ID;
	    vecnew->psrc=vecptr->psrc;					// save address of source polygon
	    }
	  vecnew->type=vec_int;						// type of intersection they have
	  vecnew->vsrcA=vec_crt;					// source polygon edge to left
	  vecnew->vsrcB=vec_nxt;					// source polygon edge to right
	  vecnew->status=1;
	  //vector_magnitude(vecnew);
	  
	  // now adjust the bisector vector length so the perim vector moves out to the current wavefront.
	  // this is the distance from the intersection pt of the polygon parent vectors out to the current
	  // wavefront distance inclusive of the angle adjustment.  this should provide a vector from the
	  // intersection point of the two source polygon edges out to the current wavefront location.
	  vector_bisector_set_length(vecnew,inc_dist);
	  
	  // move the new bisector tail up to the intersection pt of the two intersecting bisectors
	  vecnew->tail->x=vecptr->tip->x;
	  vecnew->tail->y=vecptr->tip->y;
	  vecnew->tip->z=sinpt->sz_level;
	  vecnew->tail->z=vecnew->tip->z;
	  //vector_magnitude(vecnew);
	  vector_bisector_set_length(vecnew,ss_wavefront_increment);
	  
	  // direction check
	  // by definition, the new bisector tip should always be further away from averaged parent bisector tail
	  x1=vecnew->tail->x+(vecnew->tip->x-vecnew->tail->x)*0.01;
	  y1=vecnew->tail->y+(vecnew->tip->y-vecnew->tail->y)*0.01;
	  x1=x1 - (vecptr->tail->x+vecnxt->tail->x)/2;
	  y1=y1 - (vecptr->tail->y+vecnxt->tail->y)/2;
	  x2=vecnew->tail->x - (vecptr->tail->x+vecnxt->tail->x)/2;
	  y2=vecnew->tail->y - (vecptr->tail->y+vecnxt->tail->y)/2;
	  s=sqrt(x1*x1+y1*y1);						// dist from tip to avg parent bisector tail
	  t=sqrt(x2*x2+y2*y2);						// dist from tail to avg parent bisector tail
	  if(s<t)vector_rotate(vecnew,PI);				// if tip is closer than tail, rotate 180 deg.

	  // insert the new bisector after vecptr, and flag vecptr and vecnxt as spent
	  vecptr->status=2;
	  vecnxt->status=2;
	  vecnxt=vecptr->next;
	  vecnxt->prev=vecnew;
	  vecptr->next=vecnew;
	  vecnew->next=vecnxt;
	  vecnew->prev=vecptr;
	  if(vecnew_doa==TRUE)vecnew->status=2;
	  }
	else 
	  {
	  vecptr=vecptr->next;
	  //if(vecptr==sinpt->ss_vec_first[ptyp])break;
	  }
	}
  
      // increment ss wavefront
      inc_dist+=ss_wavefront_increment;
      if(inc_dist>(odist+ss_wavefront_increment))
        {
	inc_dist=odist;
	done_incrementing=TRUE;
	}
      else 
        {
	vecptr=sinpt->ss_vec_first[ptyp];				// start at top of list
	while(vecptr!=NULL)						// loop thru whole list (closed or open versions)
	  {
	  if(vecptr->status==1)vector_bisector_inc_length(vecptr,ss_wavefront_increment);	// if an active vector...
	  vecptr=vecptr->next;
	  if(vecptr==sinpt->ss_vec_first[ptyp])break;			// handle closed loop vec list as well as open
	  }
	}
      }
    vertex_destroy(vnew);
    ss_wavefront_crtdist=inc_dist;
    if(debug_flag==202)printf("\nSS: exit\n");
    //debug_flag=0;
    
  return(1);
}

// Function to offset all polygons in a slice - straight skeleton method
int slice_offset_skeleton(slice *sinpt, int source_typ, int ptyp, float odist)
{
  int		i,pim,ply_cnt,old_debug_flag;
  vertex 	*vptr,*vnxt,*vtest,*vA,*vB;
  polygon	*pptr,*pnxt,*pold,*pdel,*pnew,*new_poly_list;
  vector	*vecptr,*vecdel,*vecnew,*vecold;
  vector_list	*vl_ptr;
  
  // validate input
  if(sinpt==NULL)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  if(fabs(odist)<TOLERANCE)return(0);

  old_debug_flag=debug_flag;
  //if(sinpt->sz_level<40.80 && sinpt->sz_level>40.50)debug_flag=201;

  if(debug_flag==201)printf("\nSO: entry:  sinpt=%X  ptyp=%d  odist=%f z=%6.3f\n",sinpt,ptyp,odist,sinpt->sz_level);
  
  // increment ss vector tips out to odist
  slice_skeleton(sinpt,source_typ,ptyp,odist);
  if(debug_flag==201)printf("  slice skeleton done...\n");

  // wipe out any previous polygons at this distance
  slice_wipe(sinpt,ptyp,odist);
  if(debug_flag==201)printf("  slice wipe done...\n");
  
  // convert ss vector tips into new polygons at odist
  {
    i=0;
    pold=NULL;
    pnew=NULL;
    new_poly_list=NULL;
    while(TRUE)
      {
      pnew=ss_veclist_2_single_polygon(sinpt->ss_vec_first[ptyp],ptyp);
      if(pnew==NULL)break;
      if(pnew!=NULL)
	{
	pnew->dist=odist;						// ... save its offset distance
	if(new_poly_list==NULL)new_poly_list=pnew;			// ... set head node if not yet defined
	if(pold!=NULL)pold->next=pnew;					// ... add linkage into list
	pold=pnew;							// ... save its address for next time thru loop
	i++;								// ... increment polygon count
	}
      }
    if(debug_flag==201)printf("  ss vector tips converted to polygons...\n");
  }
    
  // add in the new polygons freshly generated from the ss to the slice
  {
    if(sinpt->pfirst[ptyp]==NULL)
      {
      sinpt->pfirst[ptyp]=new_poly_list; 
      sinpt->pqty[ptyp]=i;
      }
    else
      {
      sinpt->plast[ptyp]->next=new_poly_list;
      sinpt->pqty[ptyp] += i;
      }
    if(debug_flag==201)printf("  %d new polygons added.  total=%d ...\n",i,sinpt->pqty[ptyp]);
  }
  
  // loop thru ss veclist and reset status for next time thru this function
  {
    vecptr=sinpt->ss_vec_first[ptyp];
    while(vecptr!=NULL)
      {
      if(vecptr->status==3)vecptr->status=1;
      vecptr=vecptr->next;
      }
  }
  
  // get accurate count of new polygons (offsetting may return multiples)
  {
    sinpt->pqty[ptyp]=0;
    pptr=sinpt->pfirst[ptyp];
    while(pptr!=NULL)
      {
      sinpt->pqty[ptyp]++;
      pptr->ID=sinpt->pqty[ptyp];
      sinpt->plast[ptyp]=pptr;
      pptr=pptr->next;
      }
    if(debug_flag==201)printf("  polygon count done.  total=%d ...\n",sinpt->pqty[ptyp]);
  }
   
  // break any polygon edges that cross other polygons at this distance
  {
    if(debug_flag==201)
      {
      printf("  checking polygon intersections: z=%f  ptyp=%d  odist=%f\n",sinpt->sz_level,ptyp,odist);
      //debug_flag=198;
      }
    ply_cnt=0;
    pptr=sinpt->pfirst[ptyp];
    while(pptr!=NULL)
      {
      if(pptr->dist==odist)
	{
	pnxt=pptr->next;
	while(pnxt!=NULL)
	  {
	  if(fabs(pnxt->dist-odist)<TOLERANCE)polygon_intersect(pptr,pnxt,TOLERANCE);
	  pnxt=pnxt->next;
	  }
	}
      ply_cnt++;
      pptr->ID=ply_cnt;
      pptr=pptr->next;
      }
    if(debug_flag==198)debug_flag=201;
  }

  // validate integrity of intersected polygons
  {
    if(debug_flag==201)printf("  checking polygon integrity...\n");
    pold=NULL;
    pptr=sinpt->pfirst[ptyp];
    while(pptr!=NULL)
      {
      if(polygon_verify(pptr)==FALSE)
	{
	if(debug_flag==201)printf("    polgyon %X failed validation... \n",pptr);
	pdel=pptr;
	pptr=pptr->next;
	if(pdel==sinpt->pfirst[ptyp]){sinpt->pfirst[ptyp]=pptr;}
	if(pold!=NULL){pold->next=pdel->next;}
	pdel->next=NULL;
	polygon_purge(pdel,pdel->dist);
	sinpt->pqty[ptyp]--;
	continue;
	}
      pold=pptr;
      pptr=pptr->next;
      }
  }

  // clear any residual slice offset vectors from previous operations
  {
    if(debug_flag==201)printf("  clearing residual offset vectors...\n");
    if(sinpt->vec_list[ptyp]!=NULL)
      {
      vl_ptr=sinpt->vec_list[ptyp];
      while(vl_ptr!=NULL)
	{
	vecptr=vl_ptr->v_item;
	while(vecptr!=NULL)
	  {
	  vecdel=vecptr;
	  vecptr=vecptr->next;
	  vector_destroy(vecdel,TRUE);
	  if(vecptr==vl_ptr->v_item)break;
	  }
	vl_ptr=vl_ptr->next;
	}
      sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
      sinpt->raw_vec_qty[ptyp]=0;
      sinpt->raw_vec_first[ptyp]=NULL;
      sinpt->raw_vec_last[ptyp]=NULL;
      }
  }
  
  // convert all newly offset polygon edges into one long vector list filtering out those vectors
  // whose "hole status" flipped from their original state.
  {
    if(debug_flag==201)printf("  converting polygons to vector list...\n");
    vecold=NULL;
    vtest=vertex_make();						// create temporary vtx storage
    pptr=sinpt->pfirst[ptyp];						// start with first polygon of this type in slice
    while(pptr!=NULL)							// loop thru all polygons
      {
      if(debug_flag==201)
        {
	printf("    ptyp=%d  pptr=%X  pnxt=%X  vfirst=%X  qty=%d  dist=%6.3f  mem=%d \n",ptyp,pptr,pptr->next,pptr->vert_first,pptr->vert_qty,pptr->dist,pptr->member);
	}
      if(fabs(pptr->dist-odist)>TOLERANCE){pptr=pptr->next;continue;}	// skip polygons not at this offset distance
      i=0;
      vptr=pptr->vert_first;						// start with first vtx in this polygon
      while(vptr!=NULL)							// loop thru all vtxs
	{
	vnxt=vptr->next;						// grab next vtx
	if(vnxt==NULL)break;
	vtest->x=(vptr->x+vnxt->x)/2;					// create test point half way in between
	vtest->y=(vptr->y+vnxt->y)/2;
	vtest->z=sinpt->sz_level;
	vtest->supp=pptr->ID;						// save source polygon ID... needed for slice_point_in_material
	if(debug_flag==201)
	  {
	  //printf("\nSO:  pptr=%X  pnxt=%X  pqty=%d  vfst=%X  \n",pptr,pptr->next,pptr->vert_qty,pptr->vert_first);
	  //printf("SO:  vptr=%X  vnxt=%X  x=%6.3f y=%6.3f z=%6.3f   i=%d \n",vptr,vnxt,vptr->x,vptr->y,vptr->z,i);
	  //printf("SO:  vtst=%X  x=%6.3f y=%6.3f z=%6.3f  sup=%d  odist=%6.3f\n",vtest,vtest->x,vtest->y,vtest->z,vtest->supp,odist);
	  //while(!kbhit());
	  }
	pim=slice_point_in_material(sinpt,vtest,ptyp,odist);		// test if in material or not
	
	// if the mid-pt of this vector is in material and the originally polygon held material (or vice-versa)... make the vector
	if((pim==FALSE && pptr->hole==0) || (pim==TRUE && pptr->hole>0))
	  {
	  // ensure we are not creating a zero length vector...
	  if(vertex_compare(vptr,vnxt,TOLERANCE)==FALSE)
	    {
	    vA=vertex_copy(vptr,NULL);
	    vB=vertex_copy(vnxt,NULL);
	    vecnew=vector_make(vA,vB,pptr->type);
	    vecnew->psrc=pptr;
	    if(sinpt->raw_vec_first[ptyp]==NULL)sinpt->raw_vec_first[ptyp]=vecnew;
	    sinpt->raw_vec_qty[ptyp]++;
	    if(vecold!=NULL)vecold->next=vecnew;
	    vecold=vecnew;
	    }
	  }
	i++;
	if(i>pptr->vert_qty)
	  {
	  vptr->next=pptr->vert_first;
	  break;
	  }
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      pptr=pptr->next;
      }
    vertex_destroy(vtest);
  }

  // sort the new vector list tip to tail.  note that this action will also break the vector list
  // into separate pre-polygons.
  {
    if(debug_flag==201)printf("  sorting vectors tip-to-tail...\n");
    sinpt->vec_list[ptyp]=vector_sort(sinpt->raw_vec_first[ptyp],TOLERANCE,0);	// sort vector list tip-to-tail into vec lists
  }

  // before re-assembling, reset vector status as downstream functions rely upon it
  {
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      while(vecptr!=NULL)
	{
	vecptr->status=1;
	vecptr=vecptr->next;
	if(vecptr==vl_ptr->v_item)break;
	}
      vl_ptr=vl_ptr->next;
      }
  }

  // wipe out the existing polygons of this line type at this offset distance
  // since this entire polygon set is now held in vectors
  {
    if(debug_flag==201)printf("  wiping out old polygon data...\n");
    slice_wipe(sinpt,ptyp,odist);
  }

  // reassemble the modified veclist back into polygons
  {
    if(debug_flag==201)printf("  converting vectors to polygons...\n");
    i=0;
    pold=NULL;
    new_poly_list=NULL;
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      pptr=veclist_2_single_polygon(vl_ptr->v_item,ptyp);
      //if(debug_flag==201)printf("    new polygon %X made from vl_ptr=%X at v_item=%X  vlnxt=%X \n",pptr,vl_ptr,vl_ptr->v_item,vl_ptr->next);
      if(pptr!=NULL)
	{
	//pptr->member=i;
	pptr->dist=odist;
	if(new_poly_list==NULL)new_poly_list=pptr;
	if(pold!=NULL)pold->next=pptr;
	pold=pptr;
	i++;
	}
      vl_ptr=vl_ptr->next;
      }
  }
    
  // add the new set of polygons to the slice
  {
    if(sinpt->pfirst[ptyp]==NULL)
      {
      sinpt->pfirst[ptyp]=new_poly_list; 
      sinpt->pqty[ptyp]=i;
      }
    else
      {
      if(sinpt->plast[ptyp]==NULL)
        {
	pptr=sinpt->pfirst[ptyp];
	while(pptr!=NULL)
	  {
	  sinpt->plast[ptyp]=pptr;
	  pptr=pptr->next;
	  }
	}
      sinpt->plast[ptyp]->next=new_poly_list;
      sinpt->pqty[ptyp] += i;
      }
    if(debug_flag==201)printf("  %d new polygons added.  total=%d ...\n",i,sinpt->pqty[ptyp]);
  }
  
  // process new polygons
  {
    if(debug_flag==201)printf("  processing polygons...\n");
    sinpt->pqty[ptyp]=0;
    sinpt->plast[ptyp]=NULL;
    pptr=new_poly_list;
    while(pptr!=NULL)
      {
      polygon_contains_material(sinpt,pptr,ptyp);			// deterimine if this poly encloses a hole or encloses material
      polygon_find_start(pptr,NULL);					// just put the start at a corner for now
      sinpt->pqty[ptyp]++;
      sinpt->plast[ptyp]=pptr;
      pptr=pptr->next;
      }
    polygon_sort(sinpt,1);						// ... sort by proximity
  }

  // delete the raw vec list and its contents as it's no longer needed since we have the polygons
  {
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      while(vecptr!=NULL)
	{
	vecdel=vecptr;
	vecptr=vecptr->next;
	if(vecptr==vl_ptr->v_item)break;
	if(vecdel!=NULL){vector_destroy(vecdel,TRUE);}
	}
      vl_ptr=vl_ptr->next;
      }
    sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
    sinpt->raw_vec_qty[ptyp]=0;
    sinpt->raw_vec_first[ptyp]=NULL;
    sinpt->raw_vec_last[ptyp]=NULL;
    if(debug_flag==201)printf("  vec_list removed from memory...\n");
  }
  
  if(debug_flag==201)printf("SO: exit:  ptyp=%d   raw_vec_qty=%d\n",ptyp,sinpt->raw_vec_qty[ptyp]);
  debug_flag=old_debug_flag;

  return(1);
}

// Function to offset all polygons in a slice - winding number method
int slice_offset_winding(slice *sinpt, int source_typ, int ptyp, float odist)
{
  int 		h,i,k,status=TRUE,vint_result,max_mem,is_hole,is_dialation;
  float 	dx,dy,mag,distx,disty,dist,hole_sum,old_area;
  vertex 	*vtx_debug_old;
  vertex 	*vtxcrs,*vtxnew,*vtxA,*vtxB,*vtxC,*vtxD,*vtxint;
  vector	*vecptr,*vecnxt,*vecfst,*vecnew,*vecold,*veccpy;
  vector 	*vecdel,*vecset,*veczup,*veclast,*vecnext,*vecdir;
  vector_list 	*vl_ptr;
  polygon	*pptr,*pold,*new_poly_list;
  
  //printf("\n\nSlice_offset_winding entry:  sinpt=%X  source=%d  ptyp=%d  odist=%f \n",sinpt,source_typ,ptyp,odist);
  //printf("\n\nSlice_offset_winding entry:  vec count = %ld \n",vector_mem);
  //printf("  raw_vec_qty[source_typ]=%ld \n",sinpt->raw_vec_qty[source_typ]);
  //printf("  raw_vec_qty[ptyp]......=%ld \n",sinpt->raw_vec_qty[ptyp]);
  
  // validate input
  if(sinpt==NULL)return(0);
  if(source_typ==ptyp)return(0);					// can't generate same line types
  if(source_typ<0 || source_typ>MAX_LINE_TYPES)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  if(fabs(odist)<TOLERANCE)return(0);
  
  // wipe out any existing vector lists for this target line type on this slice
  {
    //printf("   entering wipe of pre-existing vecs:  vec_qty=%ld \n",vector_mem);
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      vector_purge(vecptr);
      vl_ptr=vl_ptr->next;
      }
    sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
    sinpt->raw_vec_qty[ptyp]=0;
    sinpt->raw_vec_first[ptyp]=NULL;
    sinpt->raw_vec_last[ptyp]=NULL;
    //printf("   exiting wipe of pre-existing vecs:   vec_qty=%ld \n",vector_mem);
  }

  // convert source polygons to vectors if they do not yet exist
  // these will be used as reference during each re-entry into this routine
  {
    //printf("   entering poly->vec conversion:  vec_qty=%ld \n",vector_mem);
    if(sinpt->perim_vec_first[source_typ]==NULL)
      {
      sinpt->perim_vec_qty[source_typ]=0;
      veclast=NULL;
      vecnext=NULL;
      max_mem=sinpt->pqty[source_typ];
      pptr=sinpt->pfirst[source_typ];					// start with first poly of this type in slice
      while(pptr!=NULL)							// search each poly and all children polys
	{
	// if polygon is already too small to offset, don't include it in the vector set
	if(fabs(pptr->area)<min_polygon_area)
	  {
	  //printf("SWO: skipping pptr->area=%5.3f  min_area=%5.3f \n",fabs(pptr->area),min_polygon_area);
	  pptr=pptr->next; 
	  continue;
	  }
	
	vecnext=polylist_2_veclist(pptr,source_typ);			// convert polygon to vectors
	if(vecnext!=NULL)						// if vectors were generated...
	  {
	  vecptr=vecnext;
	  while(vecptr!=NULL)						// count how many vectors were made
	    {
	    sinpt->perim_vec_qty[source_typ]++;
	    vecptr=vecptr->next;
	    }
	  if(sinpt->perim_vec_first[source_typ]==NULL)sinpt->perim_vec_first[source_typ]=vecnext;	// ... if current list is empty, set it to new list
	  if(veclast!=NULL)						// ... if prev list exist
	    {
	    veclast->next=vecnext;					// ... connect next
	    vecnext->prev=veclast;					// ... connect prev
	    }
	  // find last vector in list to connect
	  while(vecnext!=NULL)
	    {
	    veclast=vecnext;
	    vecnext=vecnext->next;
	    }
	  sinpt->perim_vec_last[source_typ]=veclast;
	  //printf("polygon %d converted to vecs.  veclast=%X \n",i,veclast);
	  }
	pptr=pptr->next;
	}
      }
    //printf("  perim_vec_qty[ptyp]=%ld  vectors made from source polygons\n",sinpt->perim_vec_qty[ptyp]);
    //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->perim_vec_first[ptyp],ACTION_ADD);
    //printf("   exiting poly->vec conversion:   vec_qty=%ld \n",vector_mem);
  }

  // move & copy all the vectors by offset distance using cross product for direction
  {
    //printf("   entering move and copy:  vec_qty=%ld \n",vector_mem);
    i=0;
    vecold=NULL;
    veclast=NULL;
    vtxcrs=vertex_make();
    vecptr=sinpt->perim_vec_first[source_typ];
    while(vecptr!=NULL)
      {
      // get perpendicular direction by creating a z vector off the tip (start pt) and then
      // getting the cross product.  since hole boundaries move opposite (ccw) to material boundaries (cw)
      // just adding positive z will automatically provide correct offset direction.
      vtxnew=vertex_copy(vecptr->tip,NULL);				// make copy of tip vtx
      vtxnew->z += 1.0;							// give it some z value
      veczup=vector_make(vecptr->tip,vtxnew,0);				// create vector pointing up in z
      vector_crossproduct(vecptr,veczup,vtxcrs);			// get cross product for offset direction
      vector_destroy(veczup,FALSE); veczup=NULL;			// release from memory
      vertex_destroy(vtxnew); vtxnew=NULL;				// release from memory
      
      // normalize direction vector
      dx=vtxcrs->x;
      dy=vtxcrs->y;
      mag=sqrt(dx*dx + dy*dy);
      if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;
      distx=odist*dx/mag;
      disty=odist*dy/mag;

      // load ss vector array with these for debug
      if(debug_flag==78)
        {
	vtxA=vertex_make();
	vtxA->x=(vecptr->tip->x + vecptr->tail->x)/2;
	vtxA->y=(vecptr->tip->y + vecptr->tail->y)/2;
	vtxB=vertex_make();
	vtxB->x=vtxA->x+distx;
	vtxB->y=vtxA->y+disty;
	vecnew=vector_make(vtxB,vtxA,source_typ);
	if(sinpt->ss_vec_first[source_typ]==NULL){sinpt->ss_vec_first[source_typ]=vecnew;}
	if(vecold!=NULL){vecold->next=vecnew;vecnew->prev=vecold;}
	vecold=vecnew;
	}
      
      // create a copy of the source vector into the target vector type
      veccpy=vector_copy(vecptr);
      sinpt->raw_vec_qty[ptyp]++;
      if(sinpt->raw_vec_first[ptyp]==NULL)sinpt->raw_vec_first[ptyp]=veccpy;
      if(veclast!=NULL){veclast->next=veccpy;veccpy->prev=veclast;}
      veclast=veccpy;
      sinpt->raw_vec_last[ptyp]=veclast;
      
      // offset tip and tail of vecptr in direction of vtxcrs
      veccpy->tip->x += distx;
      veccpy->tip->y += disty;
      veccpy->tail->x += distx;
      veccpy->tail->y += disty;

      // ensure tip and tail vtx attr is cleared.  needed in following steps.
      veccpy->type=ptyp;
      veccpy->status=0;
      veccpy->member=vecptr->member;
      veccpy->psrc=vecptr->psrc;
      veccpy->tip->attr=0;
      veccpy->tail->attr=0;
      
      // copy over winding number information and save for future comparison
      veccpy->wind_dir=vecptr->wind_dir;
      veccpy->pwind_min=vecptr->wind_min;
      veccpy->pwind_max=vecptr->wind_max;
      
      // save source vector address for reference later in this function
      veccpy->vsrcA=vecptr;
      veccpy->vsrcB=NULL;
      
      //printf("vector tip %X offset by %6.3f in x, and %6.3f in y\n",vecptr->tip,(odist*dx/mag),(odist*dy/mag));
  
      i++;
      vecptr=vecptr->next;
      if(vecptr==sinpt->raw_vec_first[source_typ])break;
      }
    vertex_destroy(vtxcrs); vtxcrs=NULL;
    //printf("   exiting move and copy:   vec_qty=%ld \n",vector_mem);
  }

  // DEBUG - uncomment to check source vector status
  //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
  //return(1);
 
  // connect corners or trim overlaps of freshly moved vectors
  // vectors with gaps between them (i.e. those that were dialated) get an elbow if far enough aprat.  vectors
  // that intersect (i.e. those that were contracted) get trimmed.  if the new vector end pts are so close
  // together that they are within tolerance, just merge them into one vtx.
  {
    //printf("   entering corner connect:  vec_qty=%ld \n",vector_mem);
    vtx_debug_old=vtx_debug;
    vtxint=vertex_make();
    vtxA=NULL; vtxB=NULL;
    vecfst=sinpt->raw_vec_first[ptyp];					// save address of first vec in this polygon
    vecptr=sinpt->raw_vec_first[ptyp];					// start at first vec in list
    while(vecptr!=NULL)							// loop thru entire list
      {
      vecnxt=vecptr->next;						// get next vec
      if(vecnxt==NULL)vecnxt=vecfst;					// if this was last vec in list... redefine next as first in this polygon...
      if(vecnxt->psrc != vecfst->psrc)					// if they are NOT from the same polygon...
	{
	vecnxt=vecfst;							// ... redefine next as the first one in this poly
	vecfst=vecptr->next;						// ... save address of first vec in next poly
	}
      vtxA=vertex_copy(vecptr->tail,NULL);				// define tip vtx of new vec as tail of current
      vtxB=vertex_copy(vecnxt->tip,NULL);				// define tail vtx of new vec as tip of next

      // find where they intersect. note, they do not have to cross to produce a result
      vint_result=vector_intersect(vecptr,vecnxt,vtxint);
      vtxint->z=(vtxA->z+vtxB->z)/2;					// since intersect only provides xy pt
      
      // DEBUG - add intersection pt to vtx debug list
      //vtx_debug_old->next=vertex_copy(vtxint,NULL);
      //vtx_debug_old=vtx_debug_old->next;

      // determine type of vector joint we are dealing with - see vector_intersect function
      is_dialation=TRUE;
      if((vint_result & (1<<2)) && (vint_result & (1<<6)))is_dialation=FALSE;	// if on vA and on vB... then they touch
      
      //if(vertex_distance(vtxA,vtxint)<(2*min_vector_length))is_dialation=FALSE;
      //if(vertex_distance(vtxB,vtxint)<(2*min_vector_length))is_dialation=FALSE;

      // if they do NOT touch each other AND the vector end pts are far enough apart, 
      // then they are dialated... add in an elbow
      if(is_dialation==TRUE)						
	{
	// find midpt vtx and scale out to offset distance
	vtxC=vertex_make();						// make vtx C midway bt A and B
	vtxC->x=(vtxA->x+vtxB->x)/2;
	vtxC->y=(vtxA->y+vtxB->y)/2;
	vtxC->z=(vtxA->z+vtxB->z)/2;
	
	//vtxC->x=(vtxC->x+vtxint->x)/2;				// just use mid-pt bt average and intersect
	//vtxC->y=(vtxC->y+vtxint->y)/2;
	
	distx= (vecptr->vsrcA)->tail->x - vtxint->x;			// get vector from original intersect to new intersect
	disty= (vecptr->vsrcA)->tail->y - vtxint->y;
	mag=sqrt(distx*distx + disty*disty);				// normalize the vector
	if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;
	vtxC->x = (vecptr->vsrcA)->tail->x - fabs(odist)*(distx/mag);		// new pt is original intersect plus offset distance
	vtxC->y = (vecptr->vsrcA)->tail->y - fabs(odist)*(disty/mag);
	
	if(vertex_distance(vtxA,vtxC)>(min_vector_length))
	  {
	  // add in new vector from vtxA to vtxC
	  vecnew=vector_make(vtxA,vtxC,vecptr->type);			// create new vec of same type as current
	  sinpt->raw_vec_qty[ptyp]++;
	  vecnew->psrc=vecptr->psrc;
	  vecnew->type=vecptr->type;
	  vecnew->status=99;
	  vector_magnitude(vecnew);
	  if(vecptr->next!=NULL)					// insert into bi-directional linked list, but
	    {								// don't use vecnxt as this may be the old vecfst
	    vecnew->next=vecptr->next; 					// which is out of linked list order
	    (vecptr->next)->prev=vecnew;
	    }
	  else 
	    {
	    vecnew->next=NULL;
	    }
	  vecptr->next=vecnew; 
	  vecnew->prev=vecptr;
	  vecnew->wind_dir=vecptr->wind_dir;
	  vecnew->pwind_min=vecptr->wind_min;
	  vecnew->pwind_max=vecptr->wind_max;
	  vecptr=vecnew;						// redefine position in our loop as new vec
	  
	  // add in new vector from vtxD to vtxB
	  vtxD=vertex_copy(vtxC,NULL);
	  vecnew=vector_make(vtxD,vtxB,vecptr->type);			// create new vec of same type as current
	  sinpt->raw_vec_qty[ptyp]++;
	  vecnew->psrc=vecptr->psrc;
	  vecnew->type=vecptr->type;
	  vecnew->status=99;
	  vector_magnitude(vecnew);
	  if(vecptr->next!=NULL)					// insert into bi-directional linked list, but
	    {								// don't use vecnxt as this may be the old vecfst
	    vecnew->next=vecptr->next; 					// which is out of linked list order
	    (vecptr->next)->prev=vecnew;
	    }
	  else 
	    {
	    vecnew->next=NULL;
	    }
	  vecptr->next=vecnew; 
	  vecnew->prev=vecptr;
	  vecnew->wind_dir=vecptr->wind_dir;
	  vecnew->pwind_min=vecptr->wind_min;
	  vecnew->pwind_max=vecptr->wind_max;
	  vecptr=vecnew;						// redefine position in our loop as new vec
	  }
	else 
	  {
	  // redefine (trim) end pts to offset pt
	  vecptr->tail->x=vtxC->x; vecptr->tail->y=vtxC->y;
	  vecnxt->tip->x=vtxC->x;  vecnxt->tip->y=vtxC->y;  
	  vertex_destroy(vtxA); vtxA=NULL;
	  vertex_destroy(vtxB); vtxB=NULL;
	  vertex_destroy(vtxC); vtxC=NULL;
	  vector_magnitude(vecptr);
	  vector_magnitude(vecnxt);
	  }
	}
      // otherwise they DO touch each other OR they are so close together that we want to
      // make them one pt, then they are either contracted or too close... just trim them to their intersection
      else 								
	{
	// redefine (trim) end pts to intersection pt
	vecptr->tail->x=vtxint->x; vecptr->tail->y=vtxint->y;
	vecnxt->tip->x=vtxint->x;  vecnxt->tip->y=vtxint->y;  
	vertex_destroy(vtxA); vtxA=NULL;
	vertex_destroy(vtxB); vtxB=NULL;
	vector_magnitude(vecptr);
	vector_magnitude(vecnxt);
	}

      vecptr=vecptr->next;						// move onto next vector
      }
    vertex_destroy(vtxint); vtxint=NULL;
    //printf("  raw_vec_qty[ptyp]=%ld  vectors after offset connections made\n",sinpt->raw_vec_qty[ptyp]);
    //printf("   exiting corner connect:   vec_qty=%ld \n",vector_mem);
  }
  
  // DEBUG - uncomment to check source vector status
  //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
  //return(1);

  // break vectors that intersect into new vectors
  {
    //printf("   entering break intersects:  vec_qty=%ld \n",vector_mem);
    vtxint=vertex_make();
    vecptr=sinpt->raw_vec_first[ptyp];					// start with first vec in list
    while(vecptr!=NULL)							// loop thru entire list
      {
      total_vector_count++;
      vecnxt=vecptr->next;						// start with the next vec in list
      while(vecnxt!=NULL)						// loop thru rest of list
        {
	vint_result=vector_intersect(vecptr,vecnxt,vtxint);		// check if they intersect
	if(vint_result==204)						// if a true cross-over type intersection...
	  {
	  // break vecptr at intersection pt into two vectors
	  vtxnew=vertex_copy(vtxint,NULL);				// make a copy of the intersection vtx
	  vtxnew->attr=11;						// identify this vtx as an intersection vtx
	  vecnew=vector_make(vtxnew,vecptr->tail,vecptr->type);		// create a new vec from intersection pt to vecptr tail
	  sinpt->raw_vec_qty[ptyp]++;
	  vecnew->psrc=vecptr->psrc;
	  vecnew->wind_dir=vecptr->wind_dir;
	  vecnew->wind_min=vecptr->wind_min;
	  vecnew->wind_max=vecptr->wind_max;
	  vecptr->tail=vertex_copy(vtxint,NULL);			// redefine vecptr tail as the intersection vtx
	  vecnew->next=vecptr->next;
	  vecnew->prev=vecptr;
	  vecptr->next=vecnew;
	  if(vecnew->next!=NULL)(vecnew->next)->prev=vecnew;

	  // break vecnxt at interseciton pt into two vectors
	  vtxnew=vertex_copy(vtxint,NULL);				// make another copy for the other vector loop
	  vtxnew->attr=11;						// identify this vtx as an intersection vtx
	  vecnew=vector_make(vtxnew,vecnxt->tail,vecnxt->type);		// create new vec from intersection to tail of vecnxt
	  sinpt->raw_vec_qty[ptyp]++;
	  vecnew->psrc=vecnxt->psrc;
	  vecnew->wind_dir=vecnxt->wind_dir;
	  vecnew->wind_min=vecnxt->wind_min;
	  vecnew->wind_max=vecnxt->wind_max;
	  vecnxt->tail=vertex_copy(vtxint,NULL);
	  vecnew->next=vecnxt->next;
	  vecnew->prev=vecnxt;
	  vecnxt->next=vecnew;
	  if(vecnew->next!=NULL)(vecnew->next)->prev=vecnew;
	  }
	vecnxt=vecnxt->next;
	}
      vecptr=vecptr->next;
      }
    vertex_destroy(vtxint); vtxint=NULL;
    //printf("  raw_vec_qty[ptyp]=%ld  vectors after intersecitons found\n",sinpt->raw_vec_qty[ptyp]);
    //printf("   enxiting break intersects:   vec_qty=%ld \n",vector_mem);
  }

  // loop around vec list and identify invalid vectors
  // this is the most accurate way of determining vector validity, but it is N^2 time intensive.
  // (i.e. every vector must be compared with every other vector.  so if 100 vecs, that is 100^2 comparisons)
  // it can be sped up by on comparing top level polygons with their children given the assumption the top
  // level poly will contract.
  {
    //printf("   entering invalid detection:  vec_qty=%ld \n",vector_mem);
    vecptr=sinpt->raw_vec_first[ptyp];
    while(vecptr!=NULL)
      {
      vecptr->status=vector_winding_number(sinpt->raw_vec_first[ptyp],vecptr);
      vecptr=vecptr->next;
      }
    //printf("   exiting invalid detection:   vec_qty=%ld \n",vector_mem);
  }
 
  // DEBUG - uncomment to check source vector status
  //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
  //return(1);
 
  // remove invalid vectors
  {
    //printf("   entering removing invalid vecs:  vec_qty=%ld \n",vector_mem);
    vecptr=sinpt->raw_vec_first[ptyp];
    while(vecptr!=NULL)
      {
      //printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X  ",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
      
      // set vector z height here
      if(vecptr->tip!=NULL)vecptr->tip->z=sinpt->sz_level;
      if(vecptr->tail!=NULL)vecptr->tail->z=sinpt->sz_level;
	
      if(vecptr->status<0)
        {
	//printf(" delete \n");
	if(vecptr==sinpt->raw_vec_first[ptyp])sinpt->raw_vec_first[ptyp]=vecptr->next;
	if(vecptr->prev!=NULL)(vecptr->prev)->next=vecptr->next;	// adjust list to skip over this vector going forward
	if(vecptr->next!=NULL)(vecptr->next)->prev=vecptr->prev;	// adjust list to skip over this vector going backward
	
	vecdel=vecptr;
	vecptr=vecptr->next;
	vector_destroy(vecdel,TRUE); vecdel=NULL;
	sinpt->raw_vec_qty[ptyp]--;
	
	if(vecptr==NULL)break;
	//printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X    new\n",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
	}
      else 
        {
	//printf(" next\n");
	vecptr=vecptr->next;
	}
      }
    //printf("   exiting removing invalid vecs:   vec_qty=%ld \n",vector_mem);
    //printf("  raw_vec_qty[ptyp]=%ld  vectors after invalid vectors removed\n",sinpt->raw_vec_qty[ptyp]);
  }


  // DEBUG
  //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
  //return(1);

  // sort vectors to ensure proper sequencing.  each vec_list node will corrispond to a new polygon.
  {
    //printf("   entering vec sort:   vec_qty=%ld \n",vector_mem);
    sinpt->vec_list[ptyp]=NULL;
    sinpt->vec_list[ptyp]=vector_sort(sinpt->raw_vec_first[ptyp],TOLERANCE,0);	// sort vector list tip-to-tail into vec lists
    //printf("   exiting vec sort:    vec_qty=%ld \n",vector_mem);
  }

  // colinear merge any overtly short/shallow vectors
  {
    //printf("   entering colinear merge:   vec_qty=%ld \n",vector_mem);
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vl_ptr->v_item=vector_colinear_merge(vl_ptr->v_item,ptyp,max_colinear_angle);
      vl_ptr=vl_ptr->next;
      }
    //printf("   exiting colinear merge:    vec_qty=%ld \n",vector_mem);
  }

  // reassemble the modified vec_list back into polygons
  {
    //printf("   entering re-assemble:   vec_qty=%ld \n",vector_mem);
    new_poly_list=NULL;
    if(sinpt->vec_list[ptyp]!=NULL)
      {
      i=0;
      pold=NULL;
      vl_ptr=sinpt->vec_list[ptyp];
      while(vl_ptr!=NULL)
	{
	pptr=veclist_2_single_polygon(vl_ptr->v_item,ptyp);
	if(pptr!=NULL)
	  {
	  pptr->area=polygon_find_area(pptr);
	  if(fabs(pptr->area)>min_polygon_area)
	    {
	    //pptr->member=i;
	    pptr->dist=odist;
	    if(new_poly_list==NULL)new_poly_list=pptr;
	    if(pold!=NULL)pold->next=pptr;
	    pold=pptr;
	    i++;
	    }
	  else 								// otherwise, remove from memory
	    {
	    polygon_free(pptr);
	    }
	  }
	vl_ptr=vl_ptr->next;
	}
      }
    //printf("   exiting re-assemble:    vec_qty=%ld \n",vector_mem);
  }

  // add the new set of polygons to the slice
  {
  if(new_poly_list!=NULL)
    {
    if(sinpt->pfirst[ptyp]==NULL)
      {
      sinpt->pfirst[ptyp]=new_poly_list; 
      sinpt->pqty[ptyp]=i;
      }
    else
      {
      if(sinpt->plast[ptyp]==NULL)
        {
	pptr=sinpt->pfirst[ptyp];
	while(pptr!=NULL)
	  {
	  sinpt->plast[ptyp]=pptr;
	  pptr=pptr->next;
	  }
	}
      sinpt->plast[ptyp]->next=new_poly_list;
      sinpt->pqty[ptyp] += i;
      }
    }
    //printf("%d polygons added to slice %X of type %d\n",i,sinpt,ptyp);
  }
  
  // process new polygons
  {
    if(new_poly_list!=NULL)
      {
      sinpt->pqty[ptyp]=0;
      sinpt->plast[ptyp]=NULL;
      pptr=new_poly_list;
      while(pptr!=NULL)
	{
	polygon_contains_material(sinpt,pptr,ptyp);			// deterimine if this poly encloses a hole or encloses material
	polygon_find_start(pptr,NULL);					// just put the start at a corner for now
	pptr->dist=odist;
	pptr->perim_type=1;
	pptr->fill_type=1;
	sinpt->pqty[ptyp]++;
	sinpt->plast[ptyp]=pptr;
	pptr=pptr->next;
	}
      polygon_sort(sinpt,1);						// sort by proximity
      }
  }

  // delete the raw vec list and its contents as it's no longer needed since we have the polygons
  {
    //printf("   entering raw vec purge:  vec_qty=%ld \n",vector_mem);
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      vector_purge(vecptr);
      vl_ptr=vl_ptr->next;
      }
    sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
    sinpt->raw_vec_qty[ptyp]=0;
    sinpt->raw_vec_first[ptyp]=NULL;
    sinpt->raw_vec_last[ptyp]=NULL;
    //printf("   exiting raw vec purge:   vec_qty=%ld \n",vector_mem);
  }
  
  //printf("Slice_offset_winding exit:   vec count = %ld \n",vector_mem);
  //while(!kbhit());
  
  return(status);
}


// Function to offset all polygons in a slice - winding number method
int slice_offset_winding_by_polygon(slice *sinpt, int source_typ, int ptyp, float odist)
{
  int 		h,i,k,status=TRUE,vint_result,max_mem,is_hole,is_dialation;
  int		new_poly_count;
  float 	dx,dy,mag,distx,disty,dist,hole_sum,old_area;
  vertex 	*vtx_debug_old;
  vertex 	*vtxcrs,*vtxnew,*vtxA,*vtxB,*vtxC,*vtxD,*vtxint;
  vector	*vecptr,*vecnxt,*vecfst,*vecnew,*vecold,*veccpy;
  vector 	*vecdel,*vecset,*veczup,*veclast,*vecnext,*vecdir;
  vector_list 	*vl_ptr;
  polygon 	*pmax,*psub;
  polygon	*pptr,*pold,*new_poly_list;
  polygon_list 	*pl_ptr;
  
  //printf("\n\nSlice_offset_winding entry:  sinpt=%X  source=%d  ptyp=%d  odist=%f \n",sinpt,source_typ,ptyp,odist);
  //printf("\n\nSlice_offset_winding entry:  vec count = %ld \n",vector_mem);
  //printf("  raw_vec_qty[source_typ]=%ld \n",sinpt->raw_vec_qty[source_typ]);
  //printf("  raw_vec_qty[ptyp]......=%ld \n",sinpt->raw_vec_qty[ptyp]);
  
  // validate input
  if(sinpt==NULL)return(0);
  if(source_typ==ptyp)return(0);					// can't generate same line types
  if(source_typ<0 || source_typ>MAX_LINE_TYPES)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  if(fabs(odist)<TOLERANCE)return(0);
  
  // this function works by looping thru the parent polygons of each slice.  it relies on the fact that parent polygons
  // will offset toward inward thus not having the potential of colliding into another parent.  if a parent polygon
  // encloses any child polygons, they will be converted to vectors at the same time as collisons between them are
  // probable.
  
  new_poly_list=NULL;							// init ptr to new list of polys for this slice
  new_poly_count=0;							// init count of new polygons made by this function
  pmax=sinpt->pfirst[source_typ];					// start with first poly in slice which shold be largest outer
  while(pmax!=NULL)							// loop thru all polys in slice
    {
    // skip any poly that is NOT at the highest level parent
    if(pmax->p_parent!=NULL){pmax=pmax->next; continue;}
    
    // wipe out any existing vector lists for this TARGET line type on this slice
    {
      //printf("   entering wipe of pre-existing target vecs:  vec_qty=%ld \n",vector_mem);
      vl_ptr=sinpt->vec_list[ptyp];
      while(vl_ptr!=NULL)
	{
	vecptr=vl_ptr->v_item;
	vector_purge(vecptr);
	vl_ptr=vl_ptr->next;
	}
      sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
      sinpt->raw_vec_qty[ptyp]=0;
      sinpt->raw_vec_first[ptyp]=NULL;
      sinpt->raw_vec_last[ptyp]=NULL;
      //printf("   exiting wipe of pre-existing target vecs:   vec_qty=%ld \n",vector_mem);
    }

    // wipe out any existing vector lists for this SOURCE line type on this slice
    {
      //printf("   entering wipe of pre-existing source vecs:  vec_qty=%ld \n",vector_mem);
      vector_purge(sinpt->perim_vec_first[source_typ]);
      sinpt->perim_vec_first[source_typ]=NULL;
      //printf("   exiting wipe of pre-existing source vecs:   vec_qty=%ld \n",vector_mem);
    }

    // convert source polygons to vectors if they do not yet exist
    {
      //printf("   entering poly->vec conversion:  vec_qty=%ld \n",vector_mem);
      sinpt->perim_vec_qty[source_typ]=0;
      veclast=NULL;
      vecnext=NULL;
      max_mem=sinpt->pqty[source_typ];
      pptr=pmax;							// start with parent poly and convert it to vecs
      pl_ptr=pmax->p_child_list;					// set start pointer to child polys of pmax
      while(pptr!=NULL)						// search each poly and all children polys
	{
	// if polygon is big enough, offset it.  otherwise, skip it
	if(fabs(pptr->area)>=min_polygon_area)
	  {
	  vecnext=polylist_2_veclist(pptr,source_typ);		// convert polygon to vectors
	  if(vecnext!=NULL)						// if vectors were generated...
	    {
	    vecptr=vecnext;
	    while(vecptr!=NULL)						// count how many vectors were made
	      {
	      sinpt->perim_vec_qty[source_typ]++;
	      vecptr=vecptr->next;
	      }
	    if(sinpt->perim_vec_first[source_typ]==NULL)sinpt->perim_vec_first[source_typ]=vecnext;	// ... if current list is empty, set it to new list
	    if(veclast!=NULL)						// ... if prev list exist
	      {
	      veclast->next=vecnext;					// ... connect next
	      vecnext->prev=veclast;					// ... connect prev
	      }
	    // find last vector in list to connect
	    while(vecnext!=NULL)
	      {
	      veclast=vecnext;
	      vecnext=vecnext->next;
	      }
	    sinpt->perim_vec_last[source_typ]=veclast;
	    //printf("polygon %d converted to vecs.  veclast=%X \n",i,veclast);
	    }
	  }
	if(pl_ptr==NULL)break;					// break when no more children
	pptr=pl_ptr->p_item;						// set target for next iteration of loop
	pl_ptr=pl_ptr->next;						// move onto next child 
	}
      //printf("  perim_vec_qty[ptyp]=%ld  vectors made from source polygons\n",sinpt->perim_vec_qty[ptyp]);
      //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->perim_vec_first[ptyp],ACTION_ADD);
      //printf("   exiting poly->vec conversion:   vec_qty=%ld \n",vector_mem);
    }
  
    // move & copy all the vectors by offset distance using cross product for direction
    {
      //printf("   entering move and copy:  vec_qty=%ld \n",vector_mem);
      i=0;
      vecold=NULL;
      veclast=NULL;
      vtxcrs=vertex_make();
      vecptr=sinpt->perim_vec_first[source_typ];
      while(vecptr!=NULL)
	{
	// get perpendicular direction by creating a z vector off the tip (start pt) and then
	// getting the cross product.  since hole boundaries move opposite (ccw) to material boundaries (cw)
	// just adding positive z will automatically provide correct offset direction.
	vtxnew=vertex_copy(vecptr->tip,NULL);				// make copy of tip vtx
	vtxnew->z += 1.0;						// give it some z value
	veczup=vector_make(vecptr->tip,vtxnew,0);			// create vector pointing up in z
	vector_crossproduct(vecptr,veczup,vtxcrs);			// get cross product for offset direction
	vector_destroy(veczup,FALSE); veczup=NULL;			// release from memory
	vertex_destroy(vtxnew); vtxnew=NULL;				// release from memory
	
	// normalize direction vector
	dx=vtxcrs->x;
	dy=vtxcrs->y;
	mag=sqrt(dx*dx + dy*dy);
	if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;
	distx=odist*dx/mag;
	disty=odist*dy/mag;
  
	// load ss vector array with these for debug
	if(debug_flag==78)
	  {
	  vtxA=vertex_make();
	  vtxA->x=(vecptr->tip->x + vecptr->tail->x)/2;
	  vtxA->y=(vecptr->tip->y + vecptr->tail->y)/2;
	  vtxB=vertex_make();
	  vtxB->x=vtxA->x+distx;
	  vtxB->y=vtxA->y+disty;
	  vecnew=vector_make(vtxB,vtxA,source_typ);
	  if(sinpt->ss_vec_first[source_typ]==NULL){sinpt->ss_vec_first[source_typ]=vecnew;}
	  if(vecold!=NULL){vecold->next=vecnew;vecnew->prev=vecold;}
	  vecold=vecnew;
	  }
	
	// create a copy of the source vector into the target vector type
	veccpy=vector_copy(vecptr);
	sinpt->raw_vec_qty[ptyp]++;
	if(sinpt->raw_vec_first[ptyp]==NULL)sinpt->raw_vec_first[ptyp]=veccpy;
	if(veclast!=NULL){veclast->next=veccpy;veccpy->prev=veclast;}
	veclast=veccpy;
	sinpt->raw_vec_last[ptyp]=veclast;
	
	// offset tip and tail of vecptr in direction of vtxcrs
	veccpy->tip->x += distx;
	veccpy->tip->y += disty;
	veccpy->tail->x += distx;
	veccpy->tail->y += disty;
  
	// ensure tip and tail vtx attr is cleared.  needed in following steps.
	veccpy->type=ptyp;
	veccpy->status=0;
	veccpy->member=vecptr->member;
	veccpy->psrc=vecptr->psrc;
	veccpy->tip->attr=0;
	veccpy->tail->attr=0;
	
	// save source vector address for reference later in this function
	veccpy->vsrcA=vecptr;
	veccpy->vsrcB=NULL;
	
	//printf("vector tip %X offset by %6.3f in x, and %6.3f in y\n",vecptr->tip,(odist*dx/mag),(odist*dy/mag));
    
	i++;
	vecptr=vecptr->next;
	if(vecptr==sinpt->raw_vec_first[source_typ])break;
	}
      vertex_destroy(vtxcrs); vtxcrs=NULL;
      //printf("   exiting move and copy:   vec_qty=%ld \n",vector_mem);
    }
  
    // DEBUG - uncomment to check source vector status
    //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
    //return(1);
   
    // connect corners or trim overlaps of freshly moved vectors
    // vectors with gaps between them (i.e. those that were dialated) get an elbow if far enough aprat.  vectors
    // that intersect (i.e. those that were contracted) get trimmed.  if the new vector end pts are so close
    // together that they are within tolerance, just merge them into one vtx.
    {
      //printf("   entering corner connect:  vec_qty=%ld \n",vector_mem);
      vtx_debug_old=vtx_debug;
      vtxint=vertex_make();
      vtxA=NULL; vtxB=NULL;
      vecfst=sinpt->raw_vec_first[ptyp];					// save address of first vec in this polygon
      vecptr=sinpt->raw_vec_first[ptyp];					// start at first vec in list
      while(vecptr!=NULL)							// loop thru entire list
	{
	vecnxt=vecptr->next;						// get next vec
	if(vecnxt==NULL)vecnxt=vecfst;					// if this was last vec in list... redefine next as first in this polygon...
	if(vecnxt->psrc != vecfst->psrc)					// if they are NOT from the same polygon...
	  {
	  vecnxt=vecfst;							// ... redefine next as the first one in this poly
	  vecfst=vecptr->next;						// ... save address of first vec in next poly
	  }
	vtxA=vertex_copy(vecptr->tail,NULL);				// define tip vtx of new vec as tail of current
	vtxB=vertex_copy(vecnxt->tip,NULL);				// define tail vtx of new vec as tip of next
  
	// find where they intersect. note, they do not have to cross to produce a result
	vint_result=vector_intersect(vecptr,vecnxt,vtxint);
	vtxint->z=(vtxA->z+vtxB->z)/2;					// since intersect only provides xy pt
	
	// DEBUG - add intersection pt to vtx debug list
	//vtx_debug_old->next=vertex_copy(vtxint,NULL);
	//vtx_debug_old=vtx_debug_old->next;
  
	// determine type of vector joint we are dealing with - see vector_intersect function
	is_dialation=TRUE;
	if((vint_result & (1<<2)) && (vint_result & (1<<6)))is_dialation=FALSE;	// if on vA and on vB... then they touch
	
	//if(vertex_distance(vtxA,vtxint)<(2*min_vector_length))is_dialation=FALSE;
	//if(vertex_distance(vtxB,vtxint)<(2*min_vector_length))is_dialation=FALSE;
  
	// if they do NOT touch each other AND the vector end pts are far enough apart, 
	// then they are dialated... add in an elbow
	if(is_dialation==TRUE)						
	  {
	  // find midpt vtx and scale out to offset distance
	  vtxC=vertex_make();						// make vtx C midway bt A and B
	  vtxC->x=(vtxA->x+vtxB->x)/2;
	  vtxC->y=(vtxA->y+vtxB->y)/2;
	  vtxC->z=(vtxA->z+vtxB->z)/2;
	  
	  //vtxC->x=(vtxC->x+vtxint->x)/2;				// just use mid-pt bt average and intersect
	  //vtxC->y=(vtxC->y+vtxint->y)/2;
	  
	  distx= (vecptr->vsrcA)->tail->x - vtxint->x;			// get vector from original intersect to new intersect
	  disty= (vecptr->vsrcA)->tail->y - vtxint->y;
	  mag=sqrt(distx*distx + disty*disty);				// normalize the vector
	  if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;
	  vtxC->x = (vecptr->vsrcA)->tail->x - fabs(odist)*(distx/mag);	// new pt is original intersect plus offset distance
	  vtxC->y = (vecptr->vsrcA)->tail->y - fabs(odist)*(disty/mag);
	  
	  if(vertex_distance(vtxA,vtxC)>(min_vector_length))
	    {
	    // add in new vector from vtxA to vtxC
	    vecnew=vector_make(vtxA,vtxC,vecptr->type);			// create new vec of same type as current
	    sinpt->raw_vec_qty[ptyp]++;
	    vecnew->psrc=vecptr->psrc;
	    vecnew->type=vecptr->type;
	    vecnew->status=99;
	    vector_magnitude(vecnew);
	    if(vecptr->next!=NULL)					// insert into bi-directional linked list, but
	      {								// don't use vecnxt as this may be the old vecfst
	      vecnew->next=vecptr->next; 				// which is out of linked list order
	      (vecptr->next)->prev=vecnew;
	      }
	    else 
	      {
	      vecnew->next=NULL;
	      }
	    vecptr->next=vecnew; 
	    vecnew->prev=vecptr;
	    vecptr=vecnew;						// redefine position in our loop as new vec
	    
	    // add in new vector from vtxD to vtxB
	    vtxD=vertex_copy(vtxC,NULL);
	    vecnew=vector_make(vtxD,vtxB,vecptr->type);			// create new vec of same type as current
	    sinpt->raw_vec_qty[ptyp]++;
	    vecnew->psrc=vecptr->psrc;
	    vecnew->type=vecptr->type;
	    vecnew->status=99;
	    vector_magnitude(vecnew);
	    if(vecptr->next!=NULL)					// insert into bi-directional linked list, but
	      {								// don't use vecnxt as this may be the old vecfst
	      vecnew->next=vecptr->next; 				// which is out of linked list order
	      (vecptr->next)->prev=vecnew;
	      }
	    else 
	      {
	      vecnew->next=NULL;
	      }
	    vecptr->next=vecnew; 
	    vecnew->prev=vecptr;
	    vecptr=vecnew;						// redefine position in our loop as new vec
	    }
	  else 
	    {
	    // redefine (trim) end pts to offset pt
	    vecptr->tail->x=vtxC->x; vecptr->tail->y=vtxC->y;
	    vecnxt->tip->x=vtxC->x;  vecnxt->tip->y=vtxC->y;  
	    vertex_destroy(vtxA); vtxA=NULL;
	    vertex_destroy(vtxB); vtxB=NULL;
	    vertex_destroy(vtxC); vtxC=NULL;
	    vector_magnitude(vecptr);
	    vector_magnitude(vecnxt);
	    }
	  }
	// otherwise they DO touch each other OR they are so close together that we want to
	// make them one pt, then they are either contracted or too close... just trim them to their intersection
	else 								
	  {
	  // redefine (trim) end pts to intersection pt
	  vecptr->tail->x=vtxint->x; vecptr->tail->y=vtxint->y;
	  vecnxt->tip->x=vtxint->x;  vecnxt->tip->y=vtxint->y;  
	  vertex_destroy(vtxA); vtxA=NULL;
	  vertex_destroy(vtxB); vtxB=NULL;
	  vector_magnitude(vecptr);
	  vector_magnitude(vecnxt);
	  }
  
	vecptr=vecptr->next;						// move onto next vector
	}
      vertex_destroy(vtxint); vtxint=NULL;
      //printf("  raw_vec_qty[ptyp]=%ld  vectors after offset connections made\n",sinpt->raw_vec_qty[ptyp]);
      //printf("   exiting corner connect:   vec_qty=%ld \n",vector_mem);
    }
    
    // DEBUG - uncomment to check source vector status
    //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
    //return(1);
  
    // break vectors that intersect into new vectors
    {
      //printf("   entering break intersects:  vec_qty=%ld \n",vector_mem);
      vtxint=vertex_make();
      vecptr=sinpt->raw_vec_first[ptyp];					// start with first vec in list
      while(vecptr!=NULL)							// loop thru entire list
	{
	total_vector_count++;
	vecnxt=vecptr->next;						// start with the next vec in list
	while(vecnxt!=NULL)						// loop thru rest of list
	  {
	  vint_result=vector_intersect(vecptr,vecnxt,vtxint);		// check if they intersect
	  if(vint_result==204)						// if a true cross-over type intersection...
	    {
	    // break vecptr at intersection pt into two vectors
	    vtxnew=vertex_copy(vtxint,NULL);				// make a copy of the intersection vtx
	    vtxnew->attr=11;						// identify this vtx as an intersection vtx
	    vecnew=vector_make(vtxnew,vecptr->tail,vecptr->type);		// create a new vec from intersection pt to vecptr tail
	    sinpt->raw_vec_qty[ptyp]++;
	    vecnew->psrc=vecptr->psrc;
	    vecptr->tail=vertex_copy(vtxint,NULL);			// redefine vecptr tail as the intersection vtx
	    vecnew->next=vecptr->next;
	    vecnew->prev=vecptr;
	    vecptr->next=vecnew;
	    if(vecnew->next!=NULL)(vecnew->next)->prev=vecnew;
  
	    // break vecnxt at interseciton pt into two vectors
	    vtxnew=vertex_copy(vtxint,NULL);				// make another copy for the other vector loop
	    vtxnew->attr=11;						// identify this vtx as an intersection vtx
	    vecnew=vector_make(vtxnew,vecnxt->tail,vecnxt->type);		// create new vec from intersection to tail of vecnxt
	    sinpt->raw_vec_qty[ptyp]++;
	    vecnew->psrc=vecnxt->psrc;
	    vecnxt->tail=vertex_copy(vtxint,NULL);
	    vecnew->next=vecnxt->next;
	    vecnew->prev=vecnxt;
	    vecnxt->next=vecnew;
	    if(vecnew->next!=NULL)(vecnew->next)->prev=vecnew;
	    }
	  vecnxt=vecnxt->next;
	  }
	vecptr=vecptr->next;
	}
      vertex_destroy(vtxint); vtxint=NULL;
      //printf("  raw_vec_qty[ptyp]=%ld  vectors after intersecitons found\n",sinpt->raw_vec_qty[ptyp]);
      //printf("   enxiting break intersects:   vec_qty=%ld \n",vector_mem);
    }
  
    // loop around vec list and identify invalid vectors  <-- N^2 TIME SINK PER PARENT POLYGON!!!
    {
      //printf("   entering invalid detection:  vec_qty=%ld \n",vector_mem);
      vecptr=sinpt->raw_vec_first[ptyp];
      while(vecptr!=NULL)
	{
	vecptr->status=vector_winding_number(sinpt->raw_vec_first[ptyp],vecptr);
	vecptr=vecptr->next;
	}
      //printf("   exiting invalid detection:   vec_qty=%ld \n",vector_mem);
    }
   
    // DEBUG - uncomment to check source vector status
    //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
    //return(1);
   
    // remove invalid vectors
    {
      //printf("   entering removing invalid vecs:  vec_qty=%ld \n",vector_mem);
      vecptr=sinpt->raw_vec_first[ptyp];
      while(vecptr!=NULL)
	{
	//printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X  ",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
	
	// set vector z height here
	if(vecptr->tip!=NULL)vecptr->tip->z=sinpt->sz_level;
	if(vecptr->tail!=NULL)vecptr->tail->z=sinpt->sz_level;
	  
	if(vecptr->status<0)
	  {
	  //printf(" delete \n");
	  if(vecptr==sinpt->raw_vec_first[ptyp])sinpt->raw_vec_first[ptyp]=vecptr->next;
	  if(vecptr->prev!=NULL)(vecptr->prev)->next=vecptr->next;	// adjust list to skip over this vector going forward
	  if(vecptr->next!=NULL)(vecptr->next)->prev=vecptr->prev;	// adjust list to skip over this vector going backward
	  
	  vecdel=vecptr;
	  vecptr=vecptr->next;
	  vector_destroy(vecdel,TRUE); vecdel=NULL;
	  sinpt->raw_vec_qty[ptyp]--;
	  
	  if(vecptr==NULL)break;
	  //printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X    new\n",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
	  }
	else 
	  {
	  //printf(" next\n");
	  vecptr=vecptr->next;
	  }
	}
      //printf("   exiting removing invalid vecs:   vec_qty=%ld \n",vector_mem);
      //printf("  raw_vec_qty[ptyp]=%ld  vectors after invalid vectors removed\n",sinpt->raw_vec_qty[ptyp]);
    }
  
  
    // DEBUG
    //sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],sinpt->raw_vec_first[ptyp],ACTION_ADD);
    //return(1);
  
    // sort vectors to ensure proper sequencing.  each vec_list node will corrispond to a new polygon.
    {
      //printf("   entering vec sort:   vec_qty=%ld \n",vector_mem);
      sinpt->vec_list[ptyp]=NULL;
      sinpt->vec_list[ptyp]=vector_sort(sinpt->raw_vec_first[ptyp],TOLERANCE,0);	// sort vector list tip-to-tail into vec lists
      //printf("   exiting vec sort:    vec_qty=%ld \n",vector_mem);
    }
  
    // colinear merge any overtly short/shallow vectors
    {
      //printf("   entering colinear merge:   vec_qty=%ld \n",vector_mem);
      vl_ptr=sinpt->vec_list[ptyp];
      while(vl_ptr!=NULL)
	{
	vl_ptr->v_item=vector_colinear_merge(vl_ptr->v_item,ptyp,max_colinear_angle);
	vl_ptr=vl_ptr->next;
	}
      //printf("   exiting colinear merge:    vec_qty=%ld \n",vector_mem);
    }
  
    // reassemble the modified vec_list back into polygons and add to new_poly_list
    {
      //printf("   entering re-assemble:   vec_qty=%ld \n",vector_mem);
      //new_poly_list=NULL;
      if(sinpt->vec_list[ptyp]!=NULL)
	{
	// find last element of new_poly_list
	pold=new_poly_list;
	if(pold!=NULL)
	  {
	  while(pold->next!=NULL){pold=pold->next;}
	  }
	
	// add new polys to existing list
	vl_ptr=sinpt->vec_list[ptyp];
	while(vl_ptr!=NULL)
	  {
	  pptr=veclist_2_single_polygon(vl_ptr->v_item,ptyp);
	  if(pptr!=NULL)
	    {
	    pptr->area=polygon_find_area(pptr);
	    if(fabs(pptr->area)>min_polygon_area)			// if the polygon is big enough... process it
	      {
	      //pptr->member=i;
	      pptr->dist=odist;
	      if(new_poly_list==NULL)new_poly_list=pptr;
	      if(pold!=NULL)pold->next=pptr;
	      pold=pptr;
	      new_poly_count++;
	      }
	    else 							// ... otherwise, remove from memory
	      {
	      polygon_free(pptr);
	      }
	    }
	  vl_ptr=vl_ptr->next;
	  }
	}
      //printf("   exiting re-assemble:    vec_qty=%ld \n",vector_mem);
    }
  
  pmax=pmax->next;							// increment to next parent level polygon
  } 	// end of p_parent!=NULL loop
     
  // redefine the new set of polygons to be the slice
  {
  if(new_poly_list!=NULL)
    {
    if(sinpt->pfirst[ptyp]==NULL)
      {
      sinpt->pfirst[ptyp]=new_poly_list; 
      sinpt->pqty[ptyp]=new_poly_count;
      }
    else
      {
      if(sinpt->plast[ptyp]==NULL)
	{
	pptr=sinpt->pfirst[ptyp];
	while(pptr!=NULL)
	  {
	  sinpt->plast[ptyp]=pptr;
	  pptr=pptr->next;
	  }
	}
      sinpt->plast[ptyp]->next=new_poly_list;
      sinpt->pqty[ptyp] += new_poly_count;
      }
    }
    //printf("%d polygons added to slice %X of type %d\n",i,sinpt,ptyp);
  }
  
  // process new polygons
  {
    if(sinpt->pfirst[ptyp]!=NULL)
      {
      pptr=sinpt->pfirst[ptyp];
      while(pptr!=NULL)
	{
	if(pptr->dist!=odist){pptr=pptr->next; continue;}		// skip polys not at this same offset distance
	polygon_contains_material(sinpt,pptr,ptyp);			// deterimine if this poly encloses a hole or encloses material
	polygon_find_start(pptr,NULL);					// just put the start at a corner for now
	pptr->perim_type=1;
	pptr->fill_type=1;
	pptr->ID=sinpt->pqty[ptyp];
	sinpt->pqty[ptyp]++;
	sinpt->plast[ptyp]=pptr;
	pptr=pptr->next;
	}
      polygon_sort(sinpt,1);						// sort by proximity
      }
  }

  // delete the raw vec list and its contents as it's no longer needed since we have the polygons
  {
    //printf("   entering raw vec purge:  vec_qty=%ld \n",vector_mem);
    vl_ptr=sinpt->vec_list[ptyp];
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      vector_purge(vecptr);
      vl_ptr=vl_ptr->next;
      }
    sinpt->vec_list[ptyp]=vector_list_manager(sinpt->vec_list[ptyp],NULL,ACTION_CLEAR);
    sinpt->raw_vec_qty[ptyp]=0;
    sinpt->raw_vec_first[ptyp]=NULL;
    sinpt->raw_vec_last[ptyp]=NULL;
    //printf("   exiting raw vec purge:   vec_qty=%ld \n",vector_mem);
  }
   
  //printf("Slice_offset_winding exit:   vec count = %ld \n",vector_mem);
  //while(!kbhit());
  
  return(status);
}

// Function to add a specific vector to a slice vector list
vector *vector_add_to_list(vector *veclist, vector *vecadd)
{
  vector 	*vecptr;
  vector	*head_of_list,*end_of_list;
  
  // validate input
  // note veclist is allowed to be NULL in which case we are creating a new list
  if(vecadd==NULL)return(0);
  head_of_list=veclist;
  end_of_list=NULL;
  
  // find end of list vector
  vecptr=head_of_list;
  while(vecptr!=NULL)
    {
    end_of_list=vecptr;
    vecptr=vecptr->next;
    }
  
  // add new vector to END of the list
  if(head_of_list==NULL)
    {
    head_of_list=vecadd;
    end_of_list=vecadd;
    vecadd->prev=NULL;
    vecadd->next=NULL;
    }
  else 
    {
    end_of_list->next=vecadd;
    vecadd->prev=end_of_list;
    end_of_list=vecadd;
    vecadd->next=NULL;
    }

  return(head_of_list);
}

// Function to delete a specific vector out of a slice vector list
vector *vector_del_from_list(vector *veclist, vector *vecdel)
{
  vector 	*head_of_list,*end_of_list;
  vector 	*vecptr,*vecnxt,*vecpre;
  
  // validate input and init
  if(veclist==NULL || vecdel==NULL)return(NULL);
  head_of_list=veclist;
  end_of_list=NULL;
  
  // assume veclist does NOT wrap around
  // then need to find last vector in list (i.e. end_of_list)
  vecptr=veclist;
  while(vecptr!=NULL)
    {
    end_of_list=vecptr;
    vecptr=vecptr->next;
    }
  
  // delete from support vector list
  vecnxt=vecdel->next;							// for clarity, define next vector
  vecpre=vecdel->prev;							// define previous vector
  if(vecdel==head_of_list)						// if deleting the very first vector in the list...
    {
    head_of_list=vecnxt;						// ... redefine list start as next vector
    if(vecnxt!=NULL)vecnxt->prev=NULL;					// ... redefine list start prev as null
    }
  else if(vecdel==end_of_list)						// if deleting the very last vector in the list...
    {
    end_of_list=vecpre;							// ... redefine list end as previous vector
    if(vecpre!=NULL)vecpre->next=NULL;					// ... redefine previous' next as null
    }
  else 									// if not deleting first or last vector in list ...
    {
    if(vecnxt!=NULL)vecnxt->prev=vecpre;				// ... redefine next's prev to skip over vecdel
    if(vecpre!=NULL)vecpre->next=vecnxt;				// ... redefine prev's next to skip over vecdel
    }
  vector_destroy(vecdel,TRUE);
    
  return(head_of_list);
}

// Function to find a specific vector in a vector list
vector *vector_search(vector *veclist, vector *vecinp)
{
  vector	*vecptr;

  // validate input
  if(veclist==NULL || vecinp==NULL)return(NULL);
  
  vecptr=veclist;
  while(vecptr!=NULL)
    {
    if(vector_compare(vecptr,vecinp,CLOSE_ENOUGH)==TRUE)break;
    vecptr=vecptr->next;
    if(vecptr==veclist)break;
    }
  
  return(vecptr);
}

// Function to convert all polygons of a specific type in a slice to a slice vector list
vector *slice_to_veclist(slice *sinpt, int ptyp)
{
  vector 	*veclist,*vecptr,*vecold;
  polygon	*pptr;
  
  if(sinpt==NULL)return(NULL);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(NULL);
  
  veclist=NULL;
  vecold=NULL;
  pptr=sinpt->pfirst[ptyp];
  while(pptr!=NULL)
    {
    vecptr=polylist_2_veclist(pptr,ptyp);
    if(veclist==NULL)veclist=vecptr;
    if(vecold!=NULL)vecold->next=vecptr;
    vecold=vecptr;
    pptr=pptr->next;
    }
  
  return(veclist);
}


