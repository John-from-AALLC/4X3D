#include "Global.h"

// Function to allocate space for a single link list element of type polygon
polygon *polygon_make(vertex *vtx, int typ, unsigned int memb)
  {
  polygon	*pptr;
    
  pptr=(polygon *)malloc(sizeof(polygon));
  if(pptr==NULL)return(NULL);		
  pptr->vert_first=vtx;
  pptr->vert_qty=1;
  pptr->ID=(-1);
  pptr->member=memb;							// 0=perim, 1=offset1, 2=offset2, ... N=offsetN
  pptr->mdl_num=(-1);
  pptr->type=typ;
  pptr->hole=(-1);							// init as undefined:  0=matl, 1=hole, 2=circle
  pptr->diam=0.0;
  pptr->prim=0.0;
  pptr->area=0.0;
  pptr->dist=0.0;							// distance this polygon is offset from perimeter
  pptr->centx=(-1);
  pptr->centy=(-1);
  pptr->status=(-1);
  pptr->perim_type=(-1);
  pptr->perim_lt=NULL;
  pptr->fill_type=(-1);							// init as undefined
  pptr->fill_lt=NULL;
  pptr->p_child_list=NULL;
  pptr->p_parent=NULL;
  pptr->next=NULL;
  polygon_mem++;
  
  return(pptr);
  }

// Function to add a polygon list onto the end of an existing linked list of polygons for a slice.
// Inputs:  sptr=slice containing the polygons, type=line type, plynew=pointer to first element in new list
int polygon_insert(slice *sptr, int ptyp, polygon *plynew)
{
  vertex	*vptr;
  polygon	*pptr,*plast;
  
  //printf("\nPolygonInsert:  entry\n");
  
  if(sptr==NULL || plynew==NULL)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  
  // create linkage
  if(sptr->pfirst[ptyp]==NULL)						// if first polygon...
    {
    sptr->pfirst[ptyp]=plynew;						// ... define as first
    }
  else 									// otherwise add link to last ...
    {
    if(sptr->plast[ptyp]==NULL)						// ... if plast has not yet been defined...
      {
      pptr=sptr->pfirst[ptyp];
      while(pptr!=NULL)
        {
	if(pptr->next==NULL)sptr->plast[ptyp]=pptr;
	pptr=pptr->next;
	}
      }
    sptr->plast[ptyp]->next=plynew;					// ... link end of existing to start of new list
    }

  // establish pointer to last polygon in slice
  sptr->pqty[ptyp]=0;
  pptr=sptr->pfirst[ptyp];
  while(pptr!=NULL)
    {
    if(pptr->next==NULL)sptr->plast[ptyp]=pptr;				// redefine end of linked list
    sptr->pqty[ptyp]++;							// increment quantity
    pptr->type=ptyp;
    pptr=pptr->next;
    }
    
  //printf("   sptr=%X  ptyp=%d  pfirst=%X plast=%X pnew=%X pqty=%d \n ",sptr,ptyp,sptr->pfirst[ptyp],sptr->plast[ptyp],plynew,plynew->vert_qty);
  //printf("PolygonInsert:  exit \n");

  return(1);
}

// Function to delete a single polygon, and all its contents, from a slice's linked list of polygons
int polygon_delete(slice *sptr, polygon *pdel)
{
  int		found=0;
  int		type=0;
  vertex 	*vptr,*vdel;
  polygon 	*pptr,*pold;
  
  // verify inbound data
  if(sptr==NULL || pdel==NULL)return(0);
  type=pdel->type;
  if(type<0 || type>MAX_LINE_TYPES)return(0);
  
  //printf("\nPolygon Delete:  sptr=%X  pdel=%X  pdel->vert_qty=%ld ",sptr,pdel,pdel->vert_qty);
  
  // check if this is the first polygon in the slice
  if(pdel==sptr->pfirst[type])
    {
    sptr->pfirst[type]=pdel->next;
    }
  else 
    {
    // search list to get address of polygon just before the one to be deleted
    pptr=sptr->pfirst[type];						// init to start of polygon list
    while(pptr!=NULL)							// search thru the whole list
      {
      if(pptr->next==pdel)break;					// if this polygons address matches the one to be deleted
      pptr=pptr->next;							// move onto next element in list
      }
    if(pptr!=NULL)
      {
      pptr->next=pdel->next;						// if found... then skip over it in the list
      }
    if(pptr==NULL)return(0);						// if not found... then return in error
    }
    
  // check if this is the last polygon in the slice
  if(pdel==sptr->plast[type])
    {
    sptr->plast[type]=pptr;
    }
 
  // get an accurate count of verticies in this polygon
  pdel->vert_qty=0;
  vptr=pdel->vert_first;
  while(vptr!=NULL)
    {
    pdel->vert_qty++;
    vptr=vptr->next;
    if(vptr==pdel->vert_first)break;
    }
      
  // free memory of all vertices held by the target polygon
  vptr=pdel->vert_first;
  while(vptr!=NULL)							// the polygon could be a loop or open segment
    {
    vdel=vptr;								// id the vertex to remove
    vptr=vptr->next;							// move the vptr to the next vertex
    free(vdel); vertex_mem--; vdel=NULL;				// remove the vertex
    pdel->vert_qty--;							// decrement the polygon counter as well
    if(pdel->vert_qty<=0)break;						// if it was a loop, we're done
    }
  
  free(pdel); polygon_mem--; pdel=NULL;					// ... free the memory it was using
  sptr->pqty[type]--;							// ... decrement qty of polygons in linked list
  
  //printf("... done.\n");

  return(1);								// ... and exit with success.
}

// Function to remove a stand alone polygon, along with its vtxs, from memory
int polygon_free(polygon *pdel)
{
  vertex	*vptr,*vdel;
  
  if(pdel==NULL)return(FALSE);
  
  // get an accurate count of verticies in this polygon
  pdel->vert_qty=0;
  vptr=pdel->vert_first;
  while(vptr!=NULL)
    {
    pdel->vert_qty++;
    vptr=vptr->next;
    if(vptr==pdel->vert_first)break;
    }

  // free memory of all vertices held by the target polygon
  vptr=pdel->vert_first;
  while(vptr!=NULL)							// the polygon could be a loop or open segment
    {
    vdel=vptr;								// id the vertex to remove
    vptr=vptr->next;							// move the vptr to the next vertex
    free(vdel);	vertex_mem--; vdel=NULL;				// remove the vertex
    pdel->vert_qty--;							// decrement the polygon counter as well
    if(pdel->vert_qty<=0)break;						// if it was a loop, we're done
    }
  
  free(pdel); polygon_mem--; pdel=NULL;					// ... free the memory it was using
  
  return(TRUE);
}

// Function to purge entire polygon of offsets and fill at a specific offset distance.
polygon *polygon_purge(polygon *pinpt, float odist)
{
  int		i=0;
  float 	deldist;
  polygon	*pptr,*pdel,*pold;
  vertex	*vptr,*vstr,*vdel,*vfst;
  
  // Verify inbound data
  if(pinpt==NULL)return(pinpt);
  if(debug_flag==201)printf("Polygon Purge Entry:  polygon=%ld \n",polygon_mem);

  // delete polygons and their verticies
  deldist=odist;							// set the distance we are looking for
  pold=NULL;								// init
  pptr=pinpt;								// start with input polygon
  while(pptr!=NULL)							// loop thru all subsequent polys
    {
    if(odist==0)deldist=pptr->dist;					// delete them all
    if(fabs(pptr->dist-deldist)<TOLERANCE)				// or delete only those at a specific distance
      {
      pdel=pptr;							// id the polygon to remove
      pptr=pptr->next;							// move the pptr to the next polygon
      
      if(pdel==pinpt)pinpt=pdel->next;					// reset head of list if deleting first element
      if(pold!=NULL)pold->next=pdel->next;				// link previous to current
  
      if(debug_flag==201 && pptr!=NULL)printf("PPurge:  pptr=%X next=%X \n",pptr,pptr->next);
      if(debug_flag==201)printf("PPurge:  pdel=%X  vfst=%X  qty=%d  ply_mem=%ld  vtx_mem=%ld \n",pdel,vfst,pdel->vert_qty,polygon_mem,vertex_mem);

      vertex_purge(pdel->vert_first);					// free memory of all vertices held by the target polygon
      free(pdel); polygon_mem--; pdel=NULL;				// free memory space holding pointers and type
      i++;								// increment del counter
      if(debug_flag==201 && pptr!=NULL)printf("         i=%d  pptr=%X next=%X  ply_mem=%ld  vtx_mem=%ld \n",i,pptr,pptr->next,polygon_mem,vertex_mem);
      }
    else 								// if NOT at correct offset distance...
      {
      pold=pptr;							// ... save address of prev poly ptr
      pptr=pptr->next;							// ... move onto next polygon
      }
    }
    
  if(odist==0)pinpt=NULL;
  
  if(debug_flag==201)printf("Polygon Purge Exit:   polygon=%ld \n",polygon_mem);
  return(pinpt);
}
  
// Function to make a copy of a polygon and its verts
polygon *polygon_copy(polygon *psrc)
{
  vertex	*vsrc,*vdes,*vpre,*vstart;
  polygon	*pdes;
  
  if(psrc==NULL)return(NULL);
  
  vpre=NULL;
  vstart=NULL;
  vsrc=psrc->vert_first;						// start at first vtx
  while(vsrc!=NULL)							// loop thru entire src linked list of vtxs
    {
    vdes=vertex_copy(vsrc,NULL);					// make copy of source vtx
    if(vstart==NULL)vstart=vdes;					// if first new vtx, save start address
    if(vpre!=NULL)vpre->next=vdes;					// if not first vtx, create link from previous vtx
    vpre=vdes;								// save address of previous vtx
    vsrc=vsrc->next;							// move onto next src vtx
    if(vsrc==psrc->vert_first)break;					// if back at beginning of loop, quit
    }
  if(vpre!=NULL)vpre->next=vstart;					// close the loop if src is a loop
  
  pdes=polygon_make(vstart,psrc->type,psrc->member);			// create the new polygon
  pdes->vert_qty=psrc->vert_qty;
  pdes->ID=psrc->ID;
  pdes->member=psrc->member;
  pdes->mdl_num=psrc->mdl_num;
  pdes->type=psrc->type;
  pdes->hole=psrc->hole;
  pdes->diam=psrc->diam;
  pdes->prim=psrc->prim;
  pdes->area=psrc->area;
  pdes->dist=psrc->dist;
  pdes->centx=psrc->centx;
  pdes->centy=psrc->centy;
  pdes->status=psrc->status;
  pdes->perim_type=psrc->perim_type;
  pdes->perim_lt=psrc->perim_lt;
  pdes->fill_type=psrc->fill_type;
  pdes->fill_lt=psrc->fill_lt;
  pdes->p_child_list=psrc->p_child_list;
  pdes->p_parent=psrc->p_parent;
  pdes->next=NULL;
    
  return(pdes);
}
  
// Function to establish parent-child polygon relationships within a slice based on linetype.
// the polygon list under a polygon provides pointers to all the polygons enclosed by this polygon.
// in other words, this function establishes the parent/child relationship between polygons.
int polygon_build_sublist(slice *ssrc, int ptyp)
{
  float 	max_area;
  polygon 	*pptr,*pmax;
  polygon_list	*plst_ptr;
  
  // init
  if(ssrc==NULL)return(FALSE);
  if(ptyp<0 || ptyp>=MAX_LINE_TYPES)return(FALSE);
  
  // since this may be a raw list of polys with zero organization, the first thing to do
  // is set the first poly in the slice's poly list to the one with the most area that
  // contains material
  pmax=NULL;
  max_area=0.0;
  pptr=ssrc->pfirst[ptyp];
  while(pptr!=NULL)
    {
    if(pptr->hole==0)
      {
      if(pptr->area > max_area){max_area=pptr->area; pmax=pptr;}
      }
    pptr=pptr->next;
    if(pptr==ssrc->pfirst[ptyp])break;
    }
  if(pmax!=NULL)polygon_swap_contents(pmax,ssrc->pfirst[ptyp]);
  
  
  // loop thru whole list and build relationships
  pmax=ssrc->pfirst[ptyp];						// get the first poly in the list
  while(pmax!=NULL)							// loop thru the whole list
    {
    pptr=ssrc->pfirst[ptyp];
    while(pptr!=NULL)							// loop thru rest of list
      {
      if(pptr==pmax){pptr=pptr->next; continue;}
      if(polygon_contains_point(pmax,pptr->vert_first)==TRUE)		// if pptr is inside pmax...
	{
	pmax->p_child_list=polygon_list_manager(pmax->p_child_list,pptr,ACTION_ADD);
	pptr->p_parent=pmax;
	}
      pptr=pptr->next;
      }
    pmax=pmax->next;
    }
    
  return(TRUE);
}
  
// Function to create a polygon list element
polygon_list *polygon_list_make(polygon *pinpt)
{
  polygon_list	*pl_ptr;
  
  pl_ptr=(polygon_list *)malloc(sizeof(polygon_list));
  if(pl_ptr!=NULL)
    {
    pl_ptr->p_item=pinpt;
    pl_ptr->next=NULL;
    }
  return(pl_ptr);
}
  
// Function to manage polygon lists - typically used as a "user pick list", but can be used for other things
// Note this does NOT delete any polygons... it just modifies the list linkages.
// actions:  ADD, DELETE, CLEAR
polygon_list *polygon_list_manager(polygon_list *plist, polygon *pinpt, int action)
{
  polygon	*pptr;
  polygon_list	*pl_ptr,*pl_pre,*pl_del;
  
  if(action==ACTION_ADD)
    {
    pl_ptr=polygon_list_make(pinpt);					// create a new element
    pl_ptr->next=plist;							// make link to first element in list
    plist=pl_ptr;							// redefine start of list as new element
    }

  if(action==ACTION_DELETE)
    {
    pl_pre=NULL;
    pl_ptr=plist;
    while(pl_ptr!=NULL)							// loop thru list to find pre node to delete node
      {
      if(pl_ptr->p_item==pinpt)break;
      pl_pre=pl_ptr;
      pl_ptr=pl_ptr->next;
      }
    if(pl_ptr!=NULL)							// if the delete node was found...
      {
      pl_del=pl_ptr;							// save address in temp ptr
      if(pl_pre==NULL)							// if head node, redefine head to be next node (which may be NULL if only one node in list)
        {plist=pl_ptr->next;}
      else 
        {pl_pre->next=pl_ptr->next;}					// otherwise skip over node to be deleted
      free(pl_del);
      }
    }
    
  if(action==ACTION_CLEAR)
    {
    pl_ptr=plist;
    while(pl_ptr!=NULL)
      {
      pl_del=pl_ptr;
      pl_ptr=pl_ptr->next;
      free(pl_del);
      }
    if(plist==p_pick_list)p_pick_list=NULL;
    plist=NULL;
    }
    
  return(plist);
}  
  
// Function to dump polygon list to screen
int polygon_dump(polygon *pptr)
{
  vertex *vptr;
  
  if(pptr==NULL)return(0);
  
  printf("POLYGON DUMP FOR %X ------------------------------------------\n",pptr);
  printf("  next polgon........: %X \n",pptr->next);
  printf("  vert_first.........: %X \n",pptr->vert_first);
  
  // check if loop or open type of polygon
  vptr=pptr->vert_first;
  while(vptr!=NULL)
    {
    printf("  %X  %X  x=%6.3f  y=%6.3f  z=%6.3f  attr=%d \n",vptr,vptr->next,vptr->x,vptr->y,vptr->z,vptr->attr);
    vptr=vptr->next;
    if(vptr==pptr->vert_first)break;
    }
	
  if(vptr==NULL)
    {printf("\n  open or loop.......: OPEN \n");}
  else
    {printf("\n  open or loop.......: LOOP \n");}
  printf("  vert_qty...........: %d \n",pptr->vert_qty);
  printf("  ID.................: %d \n",pptr->ID);
  printf("  member.............: %d \n",pptr->member);
  printf("  type...............: %d \n",pptr->type);
  printf("  hole...............: %d \n",pptr->hole);
  printf("  area...............: %f \n",pptr->area);
  printf("  distance...........: %f \n",pptr->dist);
  printf("--------------------------------------------------------------\n");
  
  //while(!kbhit());
  
  return(1);
}

// Function to determine if a 2D point lies inside/on or outside a 2D polygon using simple winding number.
// This function does not care if the polygon direction is CW or CCW.  It will work for either case.
// Returns:  FALSE(0)=Outside  TRUE(1)=Inside  (-1)=Error
//
// This algorithm is credited to: https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
int polygon_contains_point(polygon *pptr, vertex *vtest)
{
  int		windnum=FALSE;
  vertex	*vptr,*vnxt; 
  
  // Verify inbound data
  if(pptr==NULL || vtest==NULL)return(-1);
  if(pptr->vert_qty<3)return(-1);
  
  // loop through all edges of the polygon
  vptr=pptr->vert_first;						// start with first vtx in polygon
  while(vptr!=NULL)							// loop thru entire polygon vtx set
    {
    vnxt=vptr->next;							// get next vtx
    if(vnxt==NULL)break;						// make sure it exists

    // The magic test based on ray tracing to right and counting crossings - see website for details.
    // In basic words:  if one of the edge pts is above the test pt and the other is below, or vice versa, and
    // test pt x is less than the start of the edge in x plus the slope of the edge in x up to the test pt y,
    // then the ray to the right from the test pt must cross over that edge, so toggle the crossing counter.
      if( ((vptr->y > vtest->y) != (vnxt->y > vtest->y)) &&
        (vtest->x < ((vnxt->x-vptr->x)*(vtest->y-vptr->y)/(vnxt->y-vptr->y)+vptr->x)) )windnum=!windnum;

    vptr=vptr->next;
    if(vptr==pptr->vert_first)break;
    }
    
  return(windnum);
}

// Fuction to determine the closest distance between a polygon perimeter and a point.
//
// Inputs:  ptest=test polygon, vtest=test vertex, vrtn=address of vtx to load distance values in
// Returns:  minDist = the shortest distance, if negative it is inside the poly
float polygon_to_vertex_distance(polygon *ptest, vertex *vtest, vertex *vrtn)
{
  int		in_out_status=FALSE;
  vertex 	*vtxA,*vtxB,*AB,*AV,*projection;
  float 	ab_len2,new_dist,min_dist,t;
  
  // check inputs
  if(ptest==NULL || vtest==NULL || vrtn==NULL)return(0);
  
  // test if inside or outside poly
  in_out_status=polygon_contains_point(ptest,vtest);			// get actual status
  if(ptest->hole!=0)in_out_status *= (-1);				// ok if in a hole

  AB=vertex_make();
  AV=vertex_make();
  projection=vertex_make();
  
  min_dist=1000;
  vtxA=ptest->vert_first;
  while(vtxA!=NULL)
    {
    vtxB=vtxA->next;
    if(vtxB==NULL)break;
    
    // distance between polygon vectors (edge length)
    AB->x=vtxB->x - vtxA->x;
    AB->y=vtxB->y - vtxA->y;
    AB->z=vtxB->z - vtxA->z;
    
    // distance from test vtx to first polygon vtx
    AV->x=vtest->x - vtxA->x;
    AV->y=vtest->y - vtxA->y;
    AV->z=vtest->z - vtxA->z;
  
    ab_len2=vertex_dotproduct(AB,AB);
    
    // test if A and B are the same point
    if(ab_len2<TOLERANCE)				  			
      {
      new_dist=vertex_distance(vtest,vtxA);
      if(new_dist<min_dist)
        {
	min_dist=new_dist;
	vrtn->x=vtxA->x;
	vrtn->y=vtxA->y;
	vrtn->z=vtxA->z;
	}
      }
    // otherwise the polygon edge has some length
    else 
      {
      // Project point onto line segment, computing parameter t
      t=vertex_dotproduct(AV,AB) / ab_len2;
    
      // Clamp t to [0, 1] to stay within segment
      if(t<0.0)t=0.0;
      if(t>1.0)t=1.0;
    
      // Compute the closest point on the polygon segment
      projection->x = (vtxA->x + (AB->x*t));
      projection->y = (vtxA->y + (AB->y*t));
      projection->z = (vtxA->z + (AB->z*t));
      
      // Check if this point on segment is closest to vtest
      new_dist=vertex_distance(vtest,projection);
      if(new_dist<min_dist)
        {
        min_dist=new_dist;
	vrtn->x=projection->x;
	vrtn->y=projection->y;
	vrtn->z=projection->z;
        }
      }
      
    //printf("      vtest: x=%6.3f y=%6.3f  vA: x=%6.3f y=%6.3f  vB: x=%6.3f y=%6.3f  new_dist=%6.3f \n",vtest->x,vtest->y,vtxA->x,vtxA->y,vtxB->x,vtxB->y,new_dist);

    vtxA=vtxA->next;
    if(vtxA==ptest->vert_first)break;
    }

  free(AB); vertex_mem--;
  free(AV); vertex_mem--;
  free(projection); vertex_mem--;

  if(in_out_status==TRUE)min_dist *= (-1);				// if inside make negative
  //printf("   poly2vtx dist:  %6.3f  vtest: x=%6.3f y=%6.3f  pf: x=%6.3f y=%6.3f \n",min_dist,vtest->x,vtest->y,ptest->vert_first->x,ptest->vert_first->y);
  //while(!kbhit());
  
  return(min_dist);
}

// Function to calculate the perimeter length of a polygon
float polygon_perim_length(polygon *pinpt)
{
  float 	dx,dy,dz,dist,total_dist;
  vertex	*vptr,*vnxt;
  
  total_dist=0;
  
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    vnxt=vptr->next;
    if(vnxt!=NULL)
      {
      dx=vptr->x-vnxt->x;
      dy=vptr->y-vnxt->y;
      dz=vptr->z-vnxt->z;
      dist=sqrt(dx*dx+dy*dy+dz*dz);
      total_dist+=dist;
      }
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  //pinpt->prim=total_dist;
  
  return(total_dist);
}

// Function to find the best possible start point of a polygon and set if viable.
// This is generally considered the vtx that is closest to being in a 90 corner AND is over the same material.
// Inputs:  pinpt=polygon to set starting pt on, sprev=previous slice of the same material (generally the slice below)
// Return:  0=failed, 1=success
int polygon_find_start(polygon *pinpt, slice *sprev)
{
  float 	cur_angle,min_angle;
  float 	dist,mindist,xd,yd;
  vertex	*vpre,*vptr,*vstr,*vnxt,*vstart;
  vector 	*A,*B;
  polygon 	*pslc,*pptr;
  
  if(pinpt==NULL)return(0);
  vstart=NULL;

  // if no previous slice was given... find a nice sharp angle to start from that is close to the previous
  // offset polygon start pt.
  if(sprev==NULL)
    {
    pptr=pinpt;
    while(pptr!=NULL)
      {
      mindist=10000;
      vstr=NULL;
      vptr=pptr->vert_first;
      while(vptr!=NULL)
	{
	xd=vptr->x; yd=vptr->y;
	dist=sqrt(xd*xd+yd*yd);
	vptr->k=dist;
	if(dist<mindist){mindist=dist;vstr=vptr;}
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      if(vstr!=NULL)
	{
	//printf(" prev start:  dist=%6.3f  %X  x=%6.3f  y=%6.3f \n",pptr->vert_first->k,pptr->vert_first,pptr->vert_first->x,pptr->vert_first->y);
	//printf(" new  start:  dist=%6.3f  %X  x=%6.3f  y=%6.3f \n\n",vstr->k,vstr,vstr->x,vstr->y);
	pptr->vert_first=vstr;
	}
      pptr=pptr->next;
      }

    /*
    min_angle=1000;
    vpre=pinpt->vert_first;
    vptr=vpre->next;
    while(vptr!=NULL)
      {
      vnxt=vptr->next;
      A=vector_make(vptr,vpre,0); B=vector_make(vptr,vnxt,0);
      cur_angle=fabs(vector_relangle(A,B));
      free(A); vector_mem--; free(B); vector_mem--;
      cur_angle=fabs(PI/2-cur_angle);					// calc how close to 90 deg
      if(cur_angle<min_angle){min_angle=cur_angle;vstart=vptr;}		// if closest, save new start
      vptr=vptr->next;
      if(vptr==pinpt->vert_first)break;
      }
    */
    
    }
  
  // if the previous slice was given... find a point over the same material to start from.
  else 
    {
    // loop thru all vtxs of input polygon.
    vptr=pinpt->vert_first;
    while(vptr!=NULL)
      {
      // check if this vtx falls within a polygon of the previous slice.  if so, this is a good place to start in
      // the event there are cantileavered overhangs.
      pslc=sprev->pfirst[pinpt->type];
      while(pslc!=NULL)
        {
	if(polygon_contains_point(pslc,vptr)==TRUE){vstart=vptr;break;}
	pslc=pslc->next;
	}
      vptr=vptr->next;
      if(vptr==pinpt->vert_first)break;
      if(vstart!=NULL)break;
      }
    }
  
  if(vstart!=NULL)pinpt->vert_first=vstart;
  return(1);
}

// Function to find the centroid of a polygon.
int polygon_find_center(polygon *pinpt)
{
  int		i;
  float 	dx,dy;
  vertex	*vptr;
  
  // Verify inbound data
  if(pinpt==NULL)return(0);
  
  dx=0;dy=0;i=0;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    i++;
    if(i>pinpt->vert_qty)break;
    dx+=vptr->x;
    dy+=vptr->y;
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  dx=dx/(float)i;
  dy=dy/(float)i;
  pinpt->centx=dx;
  pinpt->centy=dy;
  
  if(vptr!=NULL)return(1);						// indicate it is a CLOSED polygon
  if(vptr==NULL)return(2);						// indicate it is NOT a closed polygon
  return(0);
}

// Function to find area of a polygon as projected onto the XY plane (i.e. it does not account for z).
// Note that area will have a positive sign for CCW polygons, and negative for CW polygons.  Industry
// convention is that polygons containing material are CCW and holes are CW.
float polygon_find_area(polygon *pinpt)
{
  float 	area=0.0;
  float		det,det_sum;
  vertex	*vptr,*vnxt;
  
  if(pinpt!=NULL)
    {
    det_sum=0;
    vptr=pinpt->vert_first;
    while(vptr!=NULL)
      {
      vnxt=vptr->next;
      det=(vptr->x*vnxt->y-vptr->y*vnxt->x);
      det_sum+=det;
      vptr=vptr->next;
      if(vptr==pinpt->vert_first)break;
      }
    
    area=det_sum/2.0;
    }
  
  return(area);
}

// Function to return which slice the input polygon belongs to
slice *polygon_get_slice(polygon *pinpt)
{
  int		ptyp;
  polygon 	*pptr;
  slice 	*sptr,*sresult;
  model 	*mptr;
  
  sresult=NULL;
  if(pinpt==NULL)return(sresult);
   
  // loop thru all slices of all models looking for pinpt address
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    sptr=mptr->slice_first[MODEL];
    while(sptr!=NULL)
      {
      for(ptyp=1;ptyp<MAX_LINE_TYPES;ptyp++)
        {
	pptr=sptr->pfirst[ptyp];
	while(pptr!=NULL)
	  {
	  if(pptr==pinpt){sresult=sptr; break;}
	  pptr=pptr->next;
	  }
	if(sresult!=NULL)break;
	}
      if(sresult!=NULL)break;
      sptr=sptr->next;
      }
    if(sresult!=NULL)break;
    mptr=mptr->next;
    }
  
  return(sresult);
}

// Function to merge collinear vectors in a given polygon
int polygon_colinear_merge(slice *sptr, polygon *pptr, double delta)
{
  vertex	*v0,*v1,*v2;						// the 3 vertices to check if collinear
  double 	dx1,dy1,mag1,dx2,dy2,mag2,a,b,m1,m2;			// temp values used to calculate area
  int 		merge_flag=0,done_flag=0;

  // validate inputs
  if(sptr==NULL || pptr==NULL)return(0);
  
  //printf("  colinear merge enter\n");
  
  // use fast slope comparison test to determine if verticies are aligned
  // v0 and v1 make up the line and v2 is the test point
  // if v2 lies on the line then v1 is elimnated, v2 becomes v1, and v2 is loaded with the next vertex
  // note this must wrap around between the first and last vertex of the loop as this happens often.
  
  done_flag=pptr->vert_qty+2;						// save original number of vtx in this poly plus overlap
  v0=pptr->vert_first;							// start with first vertex in polygon list
  v1=v0->next;
  v2=v1->next;								// pointers to the next 2 verticies under examination
  while(TRUE)
    {
    // if the slope of v0->v1 equals the slope of v1->v2, then v0 v1 and v2 are on same line, but realize that this
    // method is sensative to the magnitudes of each segment (v0->v1 & v1->v2).
    dx1=v1->x-v0->x;
    dy1=v1->y-v0->y;
    dx2=v2->x-v1->x;
    dy2=v2->y-v1->y;
    a=dy1*dx2;
    b=dy2*dx1;
    
    v0->k=fabs(a-b);

    merge_flag=FALSE;
    if(fabs(a-b)<delta)merge_flag=TRUE;					// if slopes are about equal...
    
    if(merge_flag==TRUE)						// if merging...
      {
      // at this point, v1 has been found to lie on the line created by v0->v2, so eliminate v1, set v2 to next, and keep v0 same
      v0->next=v1->next;						// ... point v0 past the about to be deleted v1
      if(v1==pptr->vert_first)pptr->vert_first=v0;			// ... make sure v1 wasn't referenced as the start of this poly
      free(v1);vertex_mem--;						// ... release v1 from memory
      pptr->vert_qty--;							// ... decrement polygon vtx count
      // note v0 stays as is in this case
      }
    else 								// if not merging...
      {
      v0=v1;								// ... move everything over by 1 vtx
      }
    
    v1=v2;								// redefine the current v2 as the new v1
    v2=v1->next;							// move on to next vtx
    done_flag--;							// decrement the number of vtx we've looked at
    if(done_flag<=0)break;						// done if wrapped back around to beginning vtxs
    if(pptr->vert_qty<=3)break;						// done if not enough vtx left to compare
    }
    
  if(pptr->vert_qty<3)
    {
    //printf("Vector colinear merge:  Not enough non-linear verticies: %ld \n",pptr->vert_qty);
    return(0);								// if less than 3 verticies then it is not a viable polygon
    }
  
  //printf("  colinear merge exit\n");
  return(1);
}

// Function to subdivide vector segments around a polygon at a specific interval
int polygon_subdivide(polygon *pinpt, float interval)
{
  float 	org_dist,tot_step,cur_step,perc_along,step_dist;
  vertex 	*vptr,*vnxt,*vnew,*vold;
  
  if(pinpt==NULL)return(0);
  if(interval<LOOSE_CHECK)return(0);
  
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    vnxt=vptr->next;
    vold=vptr;								// init for linked list reference
    
    // create new vtxs along the line segment bt vptr and vnxt
    org_dist=vertex_distance(vptr,vnxt);
    tot_step=org_dist/interval;						// calc number of segments to make
    if(tot_step>1.1)							// note the 10% margin
      {
      step_dist=org_dist/tot_step;					// calc even interval distance
      cur_step=step_dist;						// init to first step interval
      while(cur_step<(org_dist-TOLERANCE))				// move along line segement at step_dist intervals
        {
        perc_along=cur_step/org_dist;					// get % along org dist for this step
	vnew=vertex_make();						// create a new vertex
	vnew->x=vptr->x+perc_along*(vnxt->x-vptr->x);			// calc x coord at this location
	vnew->y=vptr->y+perc_along*(vnxt->y-vptr->y);			// calc y coord at this location
	vnew->z=vptr->z+perc_along*(vnxt->z-vptr->z);			// calc z coord at this location
	vold->next=vnew;						// add into linked list
	vnew->next=vnxt;						// link to rest of list
	vold=vnew;							// save address to link on next pass thru loop
	cur_step += step_dist;						// increment distance down line segment
	}
      }
    vptr=vnxt;
    if(vptr==pinpt->vert_first)break;
    }
  
  // re-establish correct vtx count for this polygon
  pinpt->vert_qty=0;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    pinpt->vert_qty++;
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  
  return(1);
}

// Function to determine if a polygon is a hole or contains material
// this routine sets the polygon->hole value as such:  0=encloses material, 1=encloses polyhole, 2=encloses drill hole
// if the input file comes from un-booleaned solids, some polygons are both.  in this case the assumption is made that
// the are intended to be material.  therefore, every vtx in each polygon is tested.  if any one of them defines defines
// an outer boundary then that entire polygon will represent material.
int polygon_contains_material(slice *sptr, polygon *pinpt, int ptyp)
{
  int		i,is_round=0,hole_cnt,matl_cnt;
  float 	dx,dy,xmin,dist,dmin,dmax,dsum,lside,old_dist;
  vertex	*vptr,*vold,*vnxt,*v1,*v2,*vmax,*vtest;
  polygon	*pptr; 

  // verify inbound data
  if(sptr==NULL || pinpt==NULL)return(0);
  if(pinpt->vert_qty<3)return(0);
  
  if(debug_flag==188)printf("Polygon Contains Material Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  
  // determine if round or not by consistancy of distance from centroid AND a similar segment length.
  // without checking segment length, slots would be considered round.
  polygon_find_center(pinpt);
  i=0;dsum=0;dmin=BUILD_TABLE_MAX_Y;dmax=BUILD_TABLE_MIN_Y;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)							// loop thru get dist from centroid
    {
    i++;
    dx=vptr->x-pinpt->centx;
    dy=vptr->y-pinpt->centy;
    dist=sqrt(dx*dx+dy*dy);
    dsum+=dist;
    if(dist<dmin)dmin=dist;
    if(dist>dmax)dmax=dist;
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  if(i>4 && (dmax-dmin)<(is_round_radii_tol*dmax))			// if at least 5 vtxs AND within 5% "round-ness"
    {
    is_round=1;								// default to round unless we prove otherwise
    pinpt->diam=2*dsum/(float)i;					// dsum/i = average radius, want to store diam
    vptr=pinpt->vert_first;
    vnxt=vptr->next;
    dx=vnxt->x-vptr->x;
    dy=vnxt->y-vptr->y;
    old_dist=sqrt(dx*dx+dy*dy);						// calc length of the first segment
    vptr=vptr->next;
    while(vptr!=NULL)							// loop thru and look for segments not similar to first in length
      {
      vnxt=vptr->next;
      dx=vnxt->x-vptr->x;
      dy=vnxt->y-vptr->y;
      dist=sqrt(dx*dx+dy*dy);						// calc length of this segment
      if(fabs(old_dist-dist)>(old_dist*is_round_length_tol))		// if they are more than x% different in length...
        {
	is_round=0;							// ... flag as not round
	pinpt->diam=0;							// ... reset diam as nill
	break;								// ... no need to check any further
	}
      vptr=vptr->next;
      if(vptr==pinpt->vert_first)break;
      }
    }
    
  if(debug_flag==188)printf("Polygon Contains Material:  is_round=%d\n",is_round);
  //if(debug_flag==188)debug_flag=199;
  
  // for each vtx in the input polygon, loop thru all perimeter polygons in this slice and accrue a crossing number
  // then verify they are the same.  if not, then polygons are overlapping (i.e. non-booleaned STL sorce)
  hole_cnt=0; matl_cnt=0;							// init in and out counters
  vold=NULL;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    // loop thru all polygons in this slice and calculate crossing number for this vtx
    vptr->supp=0;							// init to zero
    pptr=sptr->pfirst[ptyp];						// start with first polygon of this type in this slice
    while(pptr!=NULL)							// loop thru all polygons in this slice
      {
      if(pptr->dist!=pinpt->dist){pptr=pptr->next; continue;}		// skip polygons at different offsets
      if(pptr==pinpt){pptr=pptr->next; continue;}  			// skip the input polygon
      if(polygon_contains_point(pptr,vptr)==TRUE)vptr->supp++;		// if inside, increment crossing number
      pptr=pptr->next;
      }
    // accrue matl/hole counters.  note these are NOT crossings.  this is how many polygons
    // the input polygon is believed to be contained within.  thererore, odd=encloses hole, even=encloses material
    if(vptr->supp%2==0) {matl_cnt++;} else {hole_cnt++;}		// if even (mat'l) increment matl_cnt, else hole_cnt
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  
  // check in/out number to determine hole status.  if most of the vtxs of the input polygon are an
  // odd count in, then treat it as a hole.  this helps reduce ambiguity when polygons cross each other.
  pinpt->hole=0;							// default as contains material
  if(hole_cnt>=matl_cnt)pinpt->hole=1;					// more likely should be treated as a hole
  
  pinpt->prim=polygon_perim_length(pinpt);				// get perimeter length
  pinpt->area=polygon_find_area(pinpt);					// get area and direction 
  if(pinpt->hole>0 && pinpt->area<0)polygon_reverse(pinpt);		// if hole and CCW, reverse to make CW
  if(pinpt->hole==0 && pinpt->area>0)polygon_reverse(pinpt);		// if matl and CW, reverse to make CCW
  
  // check if this polygon meets criteria for a round hole and flag as such if so
  if(is_round==TRUE)
    {
    if(only_drill_small_holes_flag)
      {
      if(current_tool>=0 && current_tool<MAX_TOOLS)
        {
        if((dmax+dmin)<=Tool[current_tool].tip_diam)pinpt->hole=2;
	}
      }
    else 
      {
      pinpt->hole=2;
      }
    }

  if(debug_flag==199)debug_flag=188;
  if(debug_flag==188)printf("Polygon Contains Material Exit:   vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  if(pinpt->hole>0)return(FALSE);
  return(FALSE);
}


// Function to determine if a polygon is a hole or contains material
// this routine sets the polygon->hole value as such:  0=encloses material, 1=encloses polyhole, 2=encloses drill hole
// it works by creating a vtx at the polygon centroid then testing if this vtx is inside or outside that polygon.
// once it knows that, it test if it is inside or outside all the other polygons essentially creating a winding
// number.  combine the self status with the other status to determine if containing material.
int polygon_contains_material2(slice *sptr, polygon *pinpt, int ptyp)
{
  int		i,istat=0,is_round=0;
  float 	dx,dy,xmin,dist,dmin,dmax,dsum,lside,old_dist;
  vertex	*vptr,*vold,*vnxt,*v1,*v2,*vmax,*vtest;
  polygon	*pptr; 

  // verify inbound data
  if(sptr==NULL || pinpt==NULL)return(0);
  if(pinpt->vert_qty<3)return(0);
  
  if(debug_flag==188)printf("Polygon Contains Material Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  
  // find and set the centroid of each polygon
  polygon_find_center(pinpt);					

  // determine if round or not by consistancy of distance from centroid AND a similar segment length.
  // without checking segment length, slots would be considered round.
  i=0;dsum=0;dmin=BUILD_TABLE_MAX_Y;dmax=BUILD_TABLE_MIN_Y;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)							// loop thru get dist from centroid
    {
    i++;
    dx=vptr->x-pinpt->centx;
    dy=vptr->y-pinpt->centy;
    dist=sqrt(dx*dx+dy*dy);
    dsum+=dist;
    if(dist<dmin)dmin=dist;
    if(dist>dmax)dmax=dist;
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  if(i>4 && (dmax-dmin)<(is_round_radii_tol*dmax))			// if at least 5 vtxs AND within 5% "round-ness"
    {
    is_round=1;								// default to round unless we prove otherwise
    pinpt->diam=2*dsum/(float)i;					// dsum/i = average radius, want to store diam
    vptr=pinpt->vert_first;
    vnxt=vptr->next;
    dx=vnxt->x-vptr->x;
    dy=vnxt->y-vptr->y;
    old_dist=sqrt(dx*dx+dy*dy);						// calc length of the first segment
    vptr=vptr->next;
    while(vptr!=NULL)							// loop thru and look for segments not similar to first in length
      {
      vnxt=vptr->next;
      dx=vnxt->x-vptr->x;
      dy=vnxt->y-vptr->y;
      dist=sqrt(dx*dx+dy*dy);						// calc length of this segment
      if(fabs(old_dist-dist)>(old_dist*is_round_length_tol))		// if they are more than x% different in length...
        {
	is_round=0;							// ... flag as not round
	pinpt->diam=0;							// ... reset diam as nill
	break;								// ... no need to check any further
	}
      vptr=vptr->next;
      if(vptr==pinpt->vert_first)break;
      }
    }
    
  if(debug_flag==188)printf("Polygon Contains Material:  is_round=%d\n",is_round);
  //if(debug_flag==188)debug_flag=199;
  
  // create a vtx at the input polygon's centroid.  it doesn't matter whether this point is truly
  // inside the polygon or not.
  vptr=vertex_make();
  vptr->x=pinpt->centx;
  vptr->y=pinpt->centy;
  vptr->z=sptr->sz_level;
  vptr->supp=1;
  
  // loop thru all perimeter polygons in this slice and accrue a winding number
  pptr=sptr->pfirst[ptyp];						// start with first polygon of slice
  while(pptr!=NULL)							// loop thru all polygons in this slice
    {
    if(pptr->dist!=pinpt->dist){pptr=pptr->next; continue;}		// only consider polygons of same offset distance
    if(pptr==pinpt){pptr=pptr->next; continue;}  			// skip the input polygon
    if(polygon_contains_point(pptr,vptr)==TRUE)vptr->supp++;		// if inside, increment winding number
    pptr=pptr->next;
    }

  // check winding number to determine hole status
  pinpt->hole=0;							// default as contains material
  if(vptr->supp%2==0)pinpt->hole=1;					// if even, it's a hole
  free(vptr); vertex_mem--; vptr=NULL;
  
  pinpt->area=polygon_find_area(pinpt);
  if(pinpt->hole>0 && pinpt->area<0)polygon_reverse(pinpt);
  if(pinpt->hole==0 && pinpt->area>0)polygon_reverse(pinpt);
  
  // check if this polygon meets criteria for a round hole and flag as such if so
  if(is_round==TRUE)
    {
    if(only_drill_small_holes_flag)
      {
      if(current_tool>=0 && current_tool<4)
        {
        if((dmax+dmin)<=Tool[current_tool].tip_diam)pinpt->hole=2;
	}
      }
    else 
      {
      pinpt->hole=2;
      }
    }

  if(debug_flag==199)debug_flag=188;
  if(debug_flag==188)printf("Polygon Contains Material Exit:   vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  if(pinpt->hole>0)return(TRUE);
  return(FALSE);
}

// Function to apply custom offsets to a polygon.
// it works by making a temporary slice out of the input polygon and its subordinates then
// calls the slice_offset_winding function.
int polygon_make_offset(slice *sinpt, polygon *pinpt)
{
  int		i,ptyp=MDL_PERIM;
  int 		perim_walls;
  float 	perim_dist,perim_line_width=0.25;
  vertex 	*vptr;
  polygon 	*pptr,*pfirst,*pnew,*pold;
  slice 	*pslc;
  polygon_list 	*pl_ptr;
  linetype	*ltptr;
     
  // validate input
  if(sinpt==NULL || pinpt==NULL)return(FALSE);
  if(pinpt->perim_lt==NULL)return(FALSE);
  
  //printf("PO:  entry  sinpt=%X  pintp=%X  pID=%d \n",sinpt,pinpt,pinpt->ID);
  
  // make temporary slice and load it with copies of source poly and its subordinates.
  // we need to make copies since we are modifying their "next" list.  if we used a direct 
  // reference to the originals we would destroy their linkage.
  pslc=slice_make();							// make a temporary slice
  pslc->msrc=active_model;						// set source model to currently active
  pslc->sz_level=sinpt->sz_level;					// set z level from input slice
  pfirst=polygon_copy(pinpt);						// make a copy of source polygon
  pslc->pfirst[ptyp]=pfirst;						// define new poly as head node of temp slice
  pslc->pqty[ptyp]++;							// increment poly count in slice
  
  // copy over contour polygons of ptyp
  pold=pfirst;
  pl_ptr=pfirst->p_child_list;						// start with first item in source poly sub list
  while(pl_ptr!=NULL)							// loop thru all sub polygons
    {
    pptr=pl_ptr->p_item;  						// get pointer to sub polygon
    pnew=polygon_copy(pptr);						// make a copy
    pold->next=pnew;							// link to last in list
    pold=pnew;								// save address for next loop thru
    pslc->pqty[ptyp]++;							// increment poly count in slice
    pl_ptr=pl_ptr->next;						// get next item off sub list
    }
  pslc->plast[ptyp]=pold;						// save ptr to last poly in list

  /*
  printf("\nPolygons to be offset:\n");
  pptr=pslc->pfirst[ptyp];
  while(pptr!=NULL)
    {
    printf("  pptr=%X  qty=%d  area=%f hole=%d \n",pptr,pptr->vert_qty,pptr->area,pptr->hole);
    pptr=pptr->next;
    }
  printf("\n\n");
  */
  
  // offset the temporary slice
  // the offset function requires different input/output line types (hence the use of MDL_BORDER here)
  perim_walls=(int)floor((pinpt->perim_lt)->line_width/(pinpt->perim_lt)->line_pitch + 0.5); 	// calc number of integer passes required to build wall
  perim_dist=(pinpt->perim_lt)->line_width*0.5+(perim_walls-1)*(pinpt->perim_lt)->line_pitch;	// calc initial position of first pass of wall
  while(perim_walls>0)							// loop thru all passes required to build wall
    {
    //printf("   walls=%d  dist=%f  pqty=%d \n",perim_walls,perim_dist,pslc->pqty[ptyp]);
    slice_offset_winding(pslc,ptyp,MDL_BORDER,perim_dist);		// generate the new border toward "inside"
    perim_walls--;							// decrement the pass we just made
    if(perim_walls>0)perim_dist-=(pinpt->perim_lt)->line_pitch;		// decrement the distance by a line pitch
    }

  // tag the new perim polys while they still exist in the temporary slice
  i=0;
  pptr=pslc->pfirst[MDL_BORDER];
  while(pptr!=NULL)
    {
    pptr->vert_qty=0;
    vptr=pptr->vert_first;
    while(vptr!=NULL)
      {
      pptr->vert_qty++;
      vptr=vptr->next;
      if(vptr==pptr->vert_first)break;
      }
    pptr->member=pinpt->ID;						// define which polygon these fills belong to
    pptr->perim_type=2;							// set as custom offset
    pslc->plast[MDL_BORDER]=pptr;					// set last ptr for slice (increments with each pass thru loop)
    pptr=pptr->next;
    i++;
    }

  // move the new perim offsets over to the originating slice and change their line type in the process
  pptr=pslc->pfirst[MDL_BORDER];
  //printf("   pptr=%X  type=%d  ptyp=%d  dist=%f \n",pptr,pptr->type,pptr->perim_type,pptr->dist);
  //printf("   inserting pptr=%X into input slice %X \n",pptr,sinpt);
  polygon_insert(sinpt,MDL_PERIM,pptr);					// inserts head node (pptr), and its following link list
  polygon_build_sublist(sinpt,MDL_PERIM);				// rebuild subordinate list
  
  // delete copies of perims made earlier and free the temporary slice
  pptr=pslc->pfirst[ptyp];						
  while(pptr!=NULL)
    {
    pold=pptr;
    pptr=pptr->next;
    if(pold->perim_type<2)polygon_free(pold);				// only del base copies... not the offsets
    }
  free(pslc); pslc=NULL;
  
  /*
  pptr=sinpt->pfirst[MDL_PERIM];
  while(pptr!=NULL)
    {
    printf("  pptr=%X  %d  vqty=%ld  vfirst=%X \n",pptr,pptr->member,pptr->vert_qty,pptr->vert_first);
    pptr=pptr->next;
    }
  
  ptyp=MDL_PERIM;
  printf(" sinpt->ID=%d \n",sinpt->ID);
  printf(" sinpt->pqty[ptyp]=%d \n",sinpt->pqty[ptyp]);
  printf(" sinpt->pfirst[ptyp]=%X \n",sinpt->pfirst[ptyp]);
  printf(" sinpt->plast[ptyp] =%X \n",sinpt->plast[ptyp]);
  printf("PO:  exit  qty=%d \n",i);
  */
     
  return(TRUE);
}

// Function to wipe out all of a specific line type wintin a specific offset distance out of a single polygon.
// This is different than polygon_purge in that it applies only to the input polygon.
int polygon_wipe_offset(slice *sinpt, polygon *pinpt, int ptyp, float odist)
{
  int		did_delete;
  polygon 	*pptr;
  polygon_list	*pl_ptr,*pl_old,*pl_del;
  
  // ensure we have something to work with
  if(pinpt==NULL)return(0);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(0);
  //printf("Polygon Wipe Offset:  entry   pinpt=%X  ptyp=%d  odist=%f \n",pinpt,ptyp,odist);
      
  // purge polygon fill data since the fill will no longer meet the perimeter
  polygon_wipe_fill(sinpt,pinpt);
    
  // find subordinate polys that meet the distance filter.
  // note that we are modifying the polygon list as we are using it, hence
  // the extra loop wrapped around it.
  while(TRUE)
    {
    did_delete=FALSE;							// init
    pl_ptr=pinpt->p_child_list;						// start with input polys sub list
    while(pl_ptr!=NULL)							// loop thru the whole list
      {
      pptr=pl_ptr->p_item;						// get address of poly
      if(pptr->type==ptyp && pptr->perim_type==2)			// if the right type ...
	{
	polygon_delete(sinpt,pptr);					// delete the polygon itself
	did_delete=TRUE;						// set flag that we did so
	break;								// get out of list
	}
      pl_ptr=pl_ptr->next;						// move onto next element in list
      }
    if(did_delete==FALSE)break;						// if nothing was deleted, we are done
    pinpt->p_child_list=polygon_list_manager(pinpt->p_child_list,pptr,ACTION_DELETE);	// remove from list
    }
    
  // now rebuild the fill to match the remaining perimeter
  polygon_make_fill(sinpt,pinpt);

  //printf("Slice Wipe Exit:   vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);
  return(1);
}

// Function to fill a polygon with a specific pattern.
// it works by making a temporary slice out of the input polygon and its subordinates then
// calls the slice_fill function.
int polygon_make_fill(slice *sinpt, polygon *pinpt)
{
  int		i,ptyp=MDL_PERIM;
  polygon 	*pptr,*pfirst,*pnew,*pold;
  slice 	*pslc;
  polygon_list 	*pl_ptr;
     
  // validate input
  if(sinpt==NULL || pinpt==NULL)return(FALSE);
  if(pinpt->fill_lt==NULL)return(FALSE);
  
  //printf("PF:  entry  sinpt=%X  pintp=%X  pID=%d \n",sinpt,pinpt,pinpt->ID);
  
  // make temporary slice and load it with copies of source poly and its subordinates.
  // we need to make copies since we are modifying their "next" list.  if we used a direct 
  // reference to the originals we would destroy their linkage.
  pslc=slice_make();							// make a temporary slice
  pslc->msrc=sinpt->msrc;						// set source model
  pslc->sz_level=sinpt->sz_level;					// set z level from input slice
  pfirst=polygon_copy(pinpt);						// make a copy of source polygon
  pslc->pfirst[ptyp]=pfirst;						// define new poly as head node of temp slice
  pslc->pqty[ptyp]++;							// increment poly count in slice
  
  // copy over contour polygons of ptyp from input poly and its subordinates
  pold=pfirst;
  pl_ptr=pfirst->p_child_list;						// start with first item in source poly sub list
  while(pl_ptr!=NULL)							// loop thru all sub polygons
    {
    pptr=pl_ptr->p_item;  						// get pointer to sub polygon
    pnew=polygon_copy(pptr);						// make a copy
    pnew->fill_type=2;
    pold->next=pnew;							// link to last in list
    pold=pnew;								// save address for next loop thru
    pslc->pqty[ptyp]++;							// increment poly count in slice
    pl_ptr=pl_ptr->next;						// get next item off sub list
    }
  pslc->plast[ptyp]=pold;						// save ptr to last poly in list

  /*
  printf("\nPolygons to be filled:\n");
  pptr=pslc->pfirst[ptyp];
  while(pptr!=NULL)
    {
    printf("  pptr=%X  qty=%d  area=%f hole=%d \n",pptr,pptr->vert_qty,pptr->area,pptr->hole);
    pptr=pptr->next;
    }
  printf("\n\n");
  */
  
  // fill the slice
  if((pinpt->fill_lt)->line_pitch<TOLERANCE)(pinpt->fill_lt)->line_pitch=0.25;				// avoid divide by 0
  slice_linefill(NULL,pslc,NULL,(pinpt->fill_lt)->line_pitch,(pinpt->fill_lt)->p_angle,ptyp,MDL_FILL,ACTION_AND);

  // recall that fill lines are nothing more than open ended polygons.  since we may need to change/delete
  // the new fill lines we just created lets tag them so they can be identified later.  Also use this
  // loop to establish the "last" pointer for the new slice.
  i=0;
  pptr=pslc->pfirst[MDL_FILL];
  while(pptr!=NULL)
    {
    pptr->vert_qty=2;
    pptr->type=MDL_FILL;
    pptr->member=pinpt->ID;						// define which polygon these fills belong to
    pslc->plast[MDL_FILL]=pptr;						// set last ptr for slice
    pptr=pptr->next;
    i++;
    }

  // move the new fill data over to the originating slice
  pptr=sinpt->plast[MDL_FILL];						// save address of last fill poly in source slice
  if(pptr==NULL)							// if input slice has no fill polys...
    {sinpt->pfirst[MDL_FILL]=pslc->pfirst[MDL_FILL];}			// ... make this the head node of its fill polys
  else 									// otherwise...
    {pptr->next=pslc->pfirst[MDL_FILL];}				// ... link new fill data to end of old fill data
  sinpt->plast[MDL_FILL]=pslc->plast[MDL_FILL];				// in either case this is the new end
  sinpt->pqty[MDL_FILL] += i;						// add in the new qty of polys
  
  // delete copies of perims made earlier and free the temporary slice
  pptr=pslc->pfirst[ptyp];						
  while(pptr!=NULL)
    {
    pold=pptr;
    pptr=pptr->next;
    polygon_free(pold);
    }
  free(pslc); pslc=NULL;
  
  /*
  pptr=sinpt->pfirst[MDL_FILL];
  while(pptr!=NULL)
    {
    printf("  pptr=%X  %d  vqty=%ld  vfirst=%X \n",pptr,pptr->member,pptr->vert_qty,pptr->vert_first);
    pptr=pptr->next;
    }
  
  ptyp=MDL_FILL;
  printf(" sinpt->ID=%d \n",sinpt->ID);
  printf(" sinpt->pqty[ptyp]=%d \n",sinpt->pqty[ptyp]);
  printf(" sinpt->pfirst[ptyp]=%X \n",sinpt->pfirst[ptyp]);
  printf(" sinpt->plast[ptyp] =%X \n",sinpt->plast[ptyp]);
  printf("PF:  exit  qty=%d \n",i);
  */
     
  return(TRUE);
}

// Function to wipe out fill lines from a specific polygon
// It works by using the polygon->member number as a filter.
int polygon_wipe_fill(slice *sinpt, polygon *pinpt)
{
  int		ptyp=MDL_FILL;
  polygon 	*pptr,*pdel,*pold;
  
  // validate input
  if(sinpt==NULL || pinpt==NULL)return(FALSE);
  
  //printf("PW:  entry  sinpt=%X  pintp=%X  pID=%d \n",sinpt,pinpt,pinpt->ID);
  
  // loop thru all fill polys of this slice and delete those that pass the filter
  pold=NULL;
  pptr=sinpt->pfirst[ptyp]; 						// start with first poly in list of this type
  while(pptr!=NULL)							// loop thru whole list
    {
    if(pptr->member==pinpt->ID)						// if it belongs to input polygon...
      {
      pdel=pptr;							// ... set target to delete
      pptr=pptr->next;							// ... increment location in list
      if(pdel==sinpt->pfirst[ptyp])sinpt->pfirst[ptyp]=pptr;		// ... if deleting head node, reset head node
      if(pdel==sinpt->plast[ptyp])sinpt->plast[ptyp]=pptr;		// ... if deleting last node, reset last node
      if(pold!=NULL)pold->next=pptr;					// ... reconnect link skipping over pdel
      sinpt->pqty[ptyp]--;						// ... decrement count of polys in this slice of this type
      polygon_free(pdel);						// ... delete target polygon
      continue;
      }
    pold=pptr;								// save address of last non-filter poly
    pptr=pptr->next;							// move onto next polygon
    }
  sinpt->plast[ptyp]=pold;
  if(sinpt->pqty[ptyp]==0){sinpt->pfirst[ptyp]=NULL; sinpt->plast[ptyp]=NULL;}

  //printf("PW: exit  qty=%d \n",sinpt->pqty[ptyp]);
    
  return(TRUE);
}

// Function to reverse the vertex sequence in a polygon
// Notes:	In the case of a closed polygon, this has the net effect of changing it from clock-wise to CCW.
//              In the case of closed polygons, clock-wise means material is inside it, CCW means material is outside it.
//		In the case of an open polygon, this has the net effect of just reversing its direction.
int polygon_reverse(polygon *pptr)
{
  int 		closed_poly;
  vertex	*vtxptr,*vtxprev,*vtxnext;
 
  // if nothing to process just leave
  if(pptr==NULL)return(0);	

  //printf("Polygon Reverse:  Entering\n");

  // test if NULL ending polygon string or closed loop polygon
  //printf("Current sequence:  ");
  closed_poly=0;							// default to open polygon (vs closed loop)
  vtxptr=pptr->vert_first;
  while(vtxptr!=NULL)
    {
    //printf(" %X ->",vtxptr);
    vtxprev=vtxptr;							// if closed poly, then "prev" will init as last vtx in list
    vtxptr=vtxptr->next;
    if(vtxptr==pptr->vert_first)break;
    }
  if(vtxptr!=NULL)
    {
    closed_poly=1;							// turn flag on if closed loop
    //printf(" %X\n",vtxptr);
    }
  else 
    {
    //printf(" NULL\n");
    }
  
  // reverse the linked list
  if(!closed_poly)vtxprev=NULL;						// if open poly, then "prev" will init as NULL
  vtxnext=NULL;
  vtxptr=pptr->vert_first;						// start at first vtx in list
  while(vtxptr!=NULL)							// quit if open loop and at end of list
    {
    vtxnext=vtxptr->next;						// save address of next vtx
    vtxptr->next=vtxprev;						// swap direction of "next" for current vtx
    vtxprev=vtxptr;							// move "prev" forward one element
    vtxptr=vtxnext;							// move "current" forward one element
    if(vtxptr==pptr->vert_first)break;					// if back to first vtx, then quit
    }
  pptr->vert_first=vtxprev;						// reset pointer into vtx list
  
  //printf("New sequence:      ");
  vtxptr=pptr->vert_first;
  while(vtxptr!=NULL)
    {
    //printf(" %X ->",vtxptr);
    vtxprev=vtxptr;
    vtxptr=vtxptr->next;
    if(vtxptr==pptr->vert_first)break;
    }
  if(vtxptr!=NULL)
    {
    //printf(" %X\n",vtxptr);
    }
  else 
    {
    //printf(" NULL\n");
    }

  // finally switch the sign of its area to match CW or CCW
  pptr->area*=(-1);

  //printf("Polygon Reverse:  Exiting\n");
  return(1);
}

// Function swap contents, not addresses nor position in linked list, of polygons
int polygon_swap_contents(polygon *pA, polygon *pB)
{
    polygon 	*pswap;
    
    if(pA==NULL || pB==NULL)return(0);
    
    pswap=polygon_make(pA->vert_first,pA->type,pA->member);
    
    pswap->vert_first=pA->vert_first;
    pswap->vert_qty=pA->vert_qty;
    pswap->ID=pA->ID;
    pswap->member=pA->member;
    pswap->mdl_num=pA->mdl_num;
    pswap->type=pA->type;
    pswap->hole=pA->hole;
    pswap->diam=pA->diam;
    pswap->prim=pA->prim;
    pswap->area=pA->area;
    pswap->dist=pA->dist;
    pswap->area=pA->area;
    pswap->centx=pA->centx;
    pswap->centy=pA->centy;
    pswap->status=pA->status;
    pswap->perim_type=pA->perim_type;
    pswap->perim_lt=pA->perim_lt;
    pswap->fill_type=pA->fill_type;
    pswap->fill_lt=pA->fill_lt;
    pswap->p_child_list=pA->p_child_list;
    pswap->p_parent=pA->p_parent;
    
    pA->vert_first=pB->vert_first;
    pA->vert_qty=pB->vert_qty;
    pA->ID=pB->ID;
    pA->member=pB->member;
    pA->mdl_num=pB->mdl_num;
    pA->type=pB->type;
    pA->hole=pB->hole;
    pA->diam=pB->diam;
    pA->prim=pB->prim;
    pA->area=pB->area;
    pA->dist=pB->dist;
    pA->area=pB->area;
    pA->centx=pB->centx;
    pA->centy=pB->centy;
    pA->status=pB->status;
    pA->perim_type=pB->perim_type;
    pA->perim_lt=pB->perim_lt;
    pA->fill_type=pB->fill_type;
    pA->fill_lt=pB->fill_lt;
    pA->p_child_list=pB->p_child_list;
    pA->p_parent=pB->p_parent;
 
    pB->vert_first=pswap->vert_first;
    pB->vert_qty=pswap->vert_qty;
    pB->ID=pswap->ID;
    pB->member=pswap->member;
    pB->mdl_num=pswap->mdl_num;
    pB->type=pswap->type;
    pB->hole=pswap->hole;
    pB->diam=pswap->diam;
    pB->prim=pswap->prim;
    pB->area=pswap->area;
    pB->dist=pswap->dist;
    pB->area=pswap->area;
    pB->centx=pswap->centx;
    pB->centy=pswap->centy;
    pB->status=pswap->status;
    pB->perim_type=pswap->perim_type;
    pB->perim_lt=pswap->perim_lt;
    pB->fill_type=pswap->fill_type;
    pB->fill_lt=pswap->fill_lt;
    pB->p_child_list=pswap->p_child_list;
    pB->p_parent=pswap->p_parent;
 
    free(pswap);polygon_mem--;
    
    return(1);
}

// Function to sort perimeter polygons to miniumize pen up travel between them.
// sort_typ:  0=nearest (position), 1=min area to max area, 2=min perim to max perim
int polygon_sort_perims(slice *sinpt, int line_typ, int sort_typ)
{
  int		pcnt;
  float 	dx,dy,delta,min_delta;
  polygon 	*pptr,*pnxt,*pold,*ppre,*pswap;
  
  if(sinpt==NULL)return(FALSE);
  if(line_typ<=0 || line_typ>=MDL_FILL)return(FALSE);
  if(sinpt->pfirst[line_typ]==NULL)return(FALSE);
  
  printf("\nPSP:  Entry  z=%6.3f lt=%d  st=%d \n",sinpt->sz_level,line_typ,sort_typ);
  
  // count polys are re-affirm plast is in place
  pcnt=0;
  pold=NULL;
  pptr=sinpt->pfirst[line_typ];
  while(pptr!=NULL)
    {
    pcnt++;
    pold=pptr;
    pptr=pptr->next;
    }
  sinpt->plast[line_typ]=pold;
  printf("   pcnt=%d  pold=%X  plast=%X \n",pcnt,pold,sinpt->plast[line_typ]);
 
  // execute sort
  pptr=sinpt->pfirst[line_typ];
  while(pptr!=NULL)
    {
    if(sort_typ==1)min_delta=pptr->area;
    if(sort_typ==2)min_delta=pptr->prim;
    pnxt=pptr->next;
    while(pnxt!=NULL)
      {
      if(sort_typ==0)							// sort by nearest position
        {
	dx=(pptr->vert_first)->x - (pnxt->vert_first)->x;
	dy=(pptr->vert_first)->y - (pnxt->vert_first)->y;
	delta=sqrt(dx*dx+dy*dy);					// calc distance
	if(pnxt==pptr->next)min_delta=delta;				// init to first distance found
	}
      if(sort_typ==1)delta=pnxt->area;					// sort by area - smallest to largest
      if(sort_typ==2)delta=pnxt->prim;					// sort by perimeter length - smallest to largest
      if(delta < min_delta){polygon_swap_contents(pptr,pnxt); min_delta=delta;}		// swap contents, not addresses
      pnxt=pnxt->next;
      }
    pptr=pptr->next;
    }
 
  // re-count polys and re-establish sinpt->plast
  pcnt=0;
  pold=NULL;
  pptr=sinpt->pfirst[line_typ];
  while(pptr!=NULL)
    {
    pcnt++;
    printf("  pptr=%X  %d  %6.2f \n",pptr,pcnt,pptr->prim);
    sinpt->plast[line_typ]=pptr;
    pold=pptr;
    pptr=pptr->next;
    }
  printf("PSP:  exit   sinpt=%X  line_typ=%d  pcnt=%d  pold=%X  plast=%X \n",sinpt,line_typ,pcnt,pold,sinpt->plast[line_typ]);
  
  return(TRUE);
}


// Function to sort fill/close-off polygons within a slice to minimize pen up travel between them.
// typ:  1=tip-to-tail every pass  2=tip-to-tail every other pass  3=by hole diameter
int polygon_sort(slice *sinpt, int typ)
{
  int		open_poly,i,colinear_error_cnt;
  int		rv_dir=0;
  float  	dist,mindist,nxtdist,xd,yd;
  float 	crt_diam=(-1);
  polygon 	*pptr,*pinpt,*ptest,*pswap;
  vertex 	*vptr,*vstr,*vend,*ustr,*uend;
  
  //printf("Polygon Sort Entry:  vtx=%ld  vec=%ld ply=%ld \n",vertex_mem,vector_mem,polygon_mem);

  // Verify inbound data
  if(sinpt==NULL)return(0);
  
  // this is a HOLE sort for drilling
  // it's a two level sort that first sorts all polys from smallest diam to largest diam to accomodate bit changes
  // it then sorts each diam grouping by proximity to minimize pen up movements
  if(typ==3)								
    {
    // first sort by diameter
    pinpt=sinpt->pfirst[MDL_BORDER];
    pptr=pinpt;
    while(pptr!=NULL)
      {
      ptest=pptr->next;
      while(ptest!=NULL)
	{
	if(ptest->diam < pptr->diam)					// note: if not a hole, then diam=0
	  {
	  polygon_swap_contents(ptest,pptr);  
	  }
	ptest=ptest->next;
	}
      pptr=pptr->next;
      }
    
    // at this point they are sorted by diams only
    // now need to sort each diam group by centroid proximity which makes it substantially easier
    // than the sort for non-typ 3 sorts (as done below)
    crt_diam=(-1);
    pptr=pinpt;
    while(pptr!=NULL)
      {
      if(pptr->diam>0)
        {
	if(crt_diam<0)crt_diam=pptr->diam;
	pswap=NULL;
	mindist=2*BUILD_TABLE_MAX_Y;
	ptest=pptr->next;
	while(ptest!=NULL)
	  {
	  xd=pptr->centx-ptest->centx;
	  yd=pptr->centy-ptest->centy;
	  dist=sqrt(xd*xd+yd*yd);
	  if(dist<mindist && fabs(ptest->diam-crt_diam)<CLOSE_ENOUGH)
	    {
	    mindist=dist;
	    pswap=ptest;
	    }
	  ptest=ptest->next;
	  }
	if(pswap!=NULL)
	  {
	  polygon_swap_contents(pswap,pptr->next);  
	  }
	}
      crt_diam=pptr->diam;
      pptr=pptr->next;
      }
    return(1);
    }
  
  // sort polgon strings tip to tail to reduce pen up time when printing
  // do this with a typical distance sort and reversing poly vertex strings as necessary
  colinear_error_cnt=0;
  for(i=MDL_FILL;i<MAX_LINE_TYPES;i++)
    {
    pptr=sinpt->pfirst[i];
    while(pptr!=NULL)
      {
      // find vertex at end of this polygon and refresh vertex count
      pptr->vert_qty=1;							// must account for vertices at each end of string
      vstr=pptr->vert_first;						// starting vertex
      vend=pptr->vert_first;						// ending vertex
      while(vend!=NULL)						
	{
	if(vend->next==vstr)break;					// if continuous loop...
	if(vend->next==NULL)break;					// if open loop...
	pptr->vert_qty++;
	vend=vend->next;	
	}
      open_poly=0;
      if(vend->next==NULL)open_poly=1;					// if not pointing back to first vtx, then open polygon
  
      // now loop thru remaining polygons to find closest start/end vtx to the vend we just found
      mindist=2*BUILD_TABLE_MAX_Y;
      ptest=pptr->next;
      if(ptest==NULL)break;
      pswap=NULL;
      rv_dir=0;
      while(ptest!=NULL)
	{
	ustr=ptest->vert_first;						// starting vertex of polygon
	uend=ptest->vert_first;						// ending vertex of polygon, more applicable for open ended polys
	while(uend->next!=NULL)
	  {
	  if(uend->next==ustr)break;					// if continuous loop, otherwise just stops at vtx before NULL
	  uend=uend->next;
	  }
	
	// compare start & end points of ptest with end point of pptr
	xd=vend->x - ustr->x;
	yd=vend->y - ustr->y;
	dist=sqrt(xd*xd + yd*yd);
	if(dist<mindist)
	  {
	  mindist=dist;
	  pswap=ptest;
	  rv_dir=0;
	  }
	xd=vend->x - uend->x;
	yd=vend->y - uend->y;
	dist=sqrt(xd*xd + yd*yd);
	if(dist<mindist)
	  {
	  mindist=dist;
	  pswap=ptest;
	  rv_dir=1;
	  }
	
	ptest=ptest->next;
	}
	
      // at this point we've found the next polygon that is closest to the end of the first
      if(pswap!=NULL)
	{
	if(open_poly==TRUE)
	  {
	  /*
	  // test if a pair of colinear pendown fills have a penup in between
	  // if left unaddressed this will cause a tinyG motion error!  it is an active tinyG bug.
	  // it causes the 2nd pendown to miss its S curve acceleration ramp, thus causing an error.
	  if(vertex_colinear_test(pptr->vert_first,(pptr->vert_first)->next,pswap->vert_first)==TRUE)
	    {
	    colinear_error_cnt++;
	    rv_dir++;							// force the start/end of this segment to reverse
	    if(rv_dir>1)rv_dir=0;					// which changes the sequence.
	    //pswap->vert_first->x += 0.1;				// force non-linearity in x
	    //pswap->vert_first->y += 0.1;				// force non-linearity in y
	    //printf("possible tinyg error condition %d at z=%6.3f \n",colinear_error_cnt,sinpt->sz_level);
	    if(pptr->ID==9999 || pptr->ID==9998){pptr->ID=9998;}
	    else {pptr->ID=9999;}
	    }
	  */
	  if(rv_dir)polygon_reverse(pswap);				// reverse the vertex sequence if found to be needed
	  }
	polygon_swap_contents(pptr->next,pswap);			// swap contents of polygons (easier than swapping pointers)
	}
  
      pptr=pptr->next;
      }	// end of pptr loop
    }		// end of for loop
    
  //printf("Polygon Sort Exit:   vtx=%ld  vec=%ld ply=%ld \n",vertex_mem,vector_mem,polygon_mem);
  return(1);
}

// Function to make a vector list from a polygon list without deleting the poly list
// input:  pinpt=pointer to a polygon, ptyp=line type
// return: pointer to the head node of a vector linked list
vector *polylist_2_veclist(polygon *pinpt, int ptyp)
{
  vertex 	*vptr,*vnxt,*vA,*vB;
  vector	*veclist,*vecnew,*vecpre;
  
  if(pinpt==NULL)return(NULL);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(NULL);

  // convert to vectors
  veclist=NULL;
  vecpre=NULL;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    vnxt=vptr->next;
    vA=vertex_copy(vptr,NULL);						// make new vtx for vector tip
    vB=vertex_copy(vnxt,NULL);						// make new vtx for vector tail
    vecnew=vector_make(vA,vB,ptyp);					// create actual vector of same type as polygon
    vecnew->type=ptyp;							// designate as input line type
    vecnew->member=pinpt->member;					// copy over member number
    vecnew->psrc=pinpt;							// define source polygon
    vecnew->vsrcA=NULL;							// no source vectors
    vecnew->vsrcB=NULL;
    vecnew->status=1;							// flag as just created
    vector_magnitude(vecnew);						// define vector magnitude/length
    if(veclist==NULL)veclist=vecnew;					// if first vector created... save as head of list
    if(vecpre!=NULL){vecpre->next=vecnew;vecnew->prev=vecpre;}		// insert into linked list
    vecpre=vecnew;							// save address for next pass thru loop
    vptr=vptr->next;							// move onto next vtx in polygon
    if(vptr==pinpt->vert_first)break;					// if back at start... quit
    }
  
  return(veclist);
}

// Function to check the integrity of a polygon
// Identify the following:
//	1 - Verify that it is a closed loop polygon.
//	2 - Get an accurate vertex count.
//	3 - Eliminate redundant vtxs.
//	4 - Verify the polygon have a minimum of 3 vtxs.
//	5 - Verify the polygon meets the mimimum area requirement.

int polygon_verify(polygon *pinpt)
{
  int		vtx_ctr,vtx_max,outofbounds=0;
  float 	area;
  vertex	*vptr,*vnxt,*vpre,*vdel;
  
  // verify input data
  if(pinpt==NULL)return(0);
  //printf("PV: entry... ");
  
  // verify that it is a closed polygon and count vtxs
  vpre=NULL;
  vtx_ctr=0;
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    vpre=vptr;								// save address of current vtx (last one before end)
    vtx_ctr++;								// increment vtx count
    if(vtx_ctr>max_vtxs_per_polygon)break;				// safety valve
    if(pinpt->vert_qty>0 && vtx_ctr>pinpt->vert_qty)break;		// if loop ptr not set for some reason
    vptr=vptr->next;
    if(vptr==pinpt->vert_first)break;
    }
  if(vtx_ctr>max_vtxs_per_polygon)return(FALSE);			// something wrong... polygon too big
  if(pinpt->vert_qty<3)return(FALSE);					// not enough vtxs... polygon too small
  if(pinpt->vert_qty>0 && vtx_ctr>pinpt->vert_qty)vpre->next=pinpt->vert_first;	// end not pointing back to start... fix it
  if(vptr==NULL)vpre->next=pinpt->vert_first;				// end not pointing back to start... fix it
  if(pinpt->vert_qty<=0)pinpt->vert_qty=vtx_ctr;			// if qty not set... set it now
  
  // eliminate any redundant sequential vtxs
  vptr=pinpt->vert_first;
  while(vptr!=NULL)
    {
    vnxt=vptr->next;
    if(vnxt==NULL)break;
    if(vertex_compare(vptr,vnxt,TIGHTCHECK)==TRUE)
      {
      vdel=vnxt;
      vptr=vdel->next;
      if(vdel==pinpt->vert_first)
        {
	pinpt->vert_first=vptr;
	vpre->next=vptr;
	}
      free(vdel);vertex_mem--;
      pinpt->vert_qty--;
      }  
    else 
      {
      vptr=vptr->next;
      }
    if(pinpt->vert_qty<3)break;
    if(vptr==pinpt->vert_first)break;
    }

  // verify that what remains is enough vtxs to make a closed 2D area
  if(pinpt->vert_qty<3)return(FALSE);
  
  // verify big enough area
  area=polygon_find_area(pinpt);					// calc area
  pinpt->area=area;							// set area
  polygon_perim_length(pinpt);						// calc perim length
  polygon_find_center(pinpt);						// find the centroid of polygon
  if(fabs(area)<min_polygon_area)return(FALSE);
  
  //printf("exit\n");
  return(TRUE);
}

// Function to identify and fix self intersecting polygons
int polygon_selfintersect_check(polygon *pA)
{
  int		h,i,vint_result,seq_num;
  vertex	*vptr,*vnxt,*nptr,*nnxt;
  vertex	*vnew,*vint,*vbrk;
  vector	*A,*B;
  polygon	*pptr,*pnew,*pcrt,*pnewlist;
  
  
  // validate input
  if(pA==NULL)return(0);
  if(pA->vert_first==NULL)return(0);
  printf("Polygon Self Intersect:  entry  pA=%X qty=%d\n",pA,pA->vert_qty);

  // Loop thru all the sides of pA comparing each side against all other vectors in pA.  Collect intersections vtx in a linked list.
  // When done, sort the linked list of intersections from vec_pA->tip to vec_pA->tial and subdivide vec_pA
  // at each intersection.
  seq_num=0;								// init intersection counter
  vnew=vertex_make();							// create an empty vertex for reusable storage space
  vptr=pA->vert_first;							// start with first vtx in pA
  while(vptr!=NULL)							// loop thru all vtxs of pA
    {
    vnxt=vptr->next;							// grab the next vtx
    A=vector_make(vptr,vnxt,1);						// create pA vector from current vtx to next vtx
    nptr=vnxt;
    while(nptr!=NULL)							// now compare against all vectors in pB
      {
      nnxt=nptr->next;
      B=vector_make(nptr,nnxt,2);					// create pB vector from current vtx to next vtx
      vint_result=vector_intersect(A,B,vnew);				// determine intersection, load intersection into temp storage
      if(vint_result==204)						// if a pure crossing intersection with no endpoints involved
	{
	seq_num++;							// increment number of intersections found
	vint=vertex_random_insert(vptr,vnew,CLOSE_ENOUGH);		// insert copy of vnew vtx in middle of vec A just after vptr
	vbrk=vertex_random_insert(nptr,vnew,CLOSE_ENOUGH);		// insert copy of vnew vtx in middle of vec B just after nptr
	}
      free(B);vector_mem--;
      nptr=nptr->next;		
      if(nptr==pA->vert_first)break;
      }
    free(A);vector_mem--;
    vptr=vptr->next;
    if(vptr==pA->vert_first)break;
    }
  free(vnew);vertex_mem--;

  // at this point we have no crossing vectors... they only cross where intersection vtxs exist
  printf("Polygon Self Intersect:  exit  %d intersections found\n",seq_num);

  return(seq_num);
}

// Function to test if two polygons overlap or touch each other.
// This only provides a true/false answer versus actually breaking them at intersection locations.
int polygon_overlap(polygon *pA, polygon *pB)
{
  int		status=FALSE,int_stat;
  vertex	*vptrA,*vptrB,*vtxint; 
  vector 	*vecnewA,*vecnewB;
  
  if(pA==NULL || pB==NULL)return(FALSE);
  
  // test if any vtx of pB is inside pA, if any vtx of pA is inside pB, and if any edge of pA crosses any edge of pB
  vtxint=vertex_make();
  vptrA=pA->vert_first;							// start with first vtx of pA
  while(vptrA!=NULL)							// loop thru all vtxs of pA
    {
    if(polygon_contains_point(pB,vptrA)==TRUE){status=TRUE;break;}	// test if inside pB
    vecnewA=vector_make(vptrA,vptrA->next,0);				// define vector of pA
    vptrB=pB->vert_first;						// start with first vtx of pB
    while(vptrB!=NULL)							// loop thru all vtxs of pB
      {
      if(polygon_contains_point(pA,vptrB)==TRUE){status=TRUE;break;}	// test if inside pA
      vecnewB=vector_make(vptrB,vptrB->next,0);				// define vector of pB
      int_stat=vector_intersect(vecnewA,vecnewB,vtxint);		// test if vectors intersect
      free(vecnewB); vector_mem--; vecnewB=NULL;			// clean up memory
      if(int_stat>=204){status=TRUE; break;}				// check intersection status
      vptrB=vptrB->next;						// move onto next vtx in pB
      if(vptrB==pB->vert_first)break;					// if back at start of pB, done
      }
    free(vecnewA); vector_mem--; vecnewA=NULL;				// clean up memory
    if(status==TRUE)break;
    vptrA=vptrA->next;							// move onto next vtx of pA
    if(vptrA==pA->vert_first)break;					// if back at start of pA, done
    }
  free(vtxint); vertex_mem--; vtxint=NULL;				// free memory
  
  return(status);
}

// Function to intersect two polygons
// if any portion of them cross, this function will subdivide the crossing vectors and add matching
// vertex attributes to them (altho they could also be identified by position)
int polygon_intersect(polygon *pA, polygon *pB, double tolerance)
{
  int		h,i,j,seq_num=0,vint_result,qty_found;
  int		old_debug_flag,mtyp=MODEL;
  int		loopAexit,loopBexit,Acount,Bcount;
  float 	pA_xmin,pA_xmax,pA_ymin,pA_ymax;
  float 	pB_xmin,pB_xmax,pB_ymin,pB_ymax;
  float 	dx,dy,dist_tip,dist_tail;
  float 	Adx,Ady,Bdx,Bdy;
  polygon	*pptr;
  vertex	*vptr,*vold,*vnxt;
  vertex	*nptr,*nold,*nnxt;
  vertex	*vnew,*vint,*vbrk,*vtxc;
  vertex 	*Avtxptr,*Avtxnxt,*Bvtxptr,*Bvtxnxt;
  vertex 	*vAloop_next,*vBloop_next;
  vector	*A,*B;
  model		*mptrA,*mptrB;
  
  //debug_flag=0;
  old_debug_flag=debug_flag;
  //debug_flag=198;
  if(debug_flag==198)printf("\nPolygon Intersect Entry:  pA=%X  pB=%X  tol=%f\n",pA,pB,tolerance);
  
  // validate inputs
  if(pA==NULL || pB==NULL)						// both must exist
    {
    if(debug_flag==198)printf("Polygon Intersect Exit:   Invalid Inputs   vtxs added=%d \n",seq_num);
    debug_flag=old_debug_flag;
    return(seq_num);
    }
  if(pA==pB)								// can't compare against self
    {
    if(debug_flag==198)printf("Polygon Intersect Exit:   pA==pB   vtxs added=%d \n",seq_num);
    debug_flag=old_debug_flag;
    return(seq_num);
    }
  if(pA->vert_qty<3 || pB->vert_qty<3)
    {
    if(debug_flag==198)printf("Polygon Intersect Exit:   pA->aty=%d pB->qty=%d   vtxs added=%d \n",seq_num,pA->vert_qty,pB->vert_qty);
    debug_flag=old_debug_flag;
    return(seq_num);			// both must be valid polygons
    }
  
  // init bounding boxes of incoming polygons
  pA_xmin=BUILD_TABLE_MAX_X; pA_xmax=BUILD_TABLE_MIN_X;
  pA_ymin=BUILD_TABLE_MAX_Y; pA_ymax=BUILD_TABLE_MIN_Y;
  pB_xmin=BUILD_TABLE_MAX_X; pB_xmax=BUILD_TABLE_MIN_X;
  pB_ymin=BUILD_TABLE_MAX_Y; pB_ymax=BUILD_TABLE_MIN_Y;
  
  // verify that pA is a closed loop polygon
  if(debug_flag==198)printf("  Verifying pA... vert_qty=%d\n",pA->vert_qty);
  i=0;									// set independent vtx counter
  vptr=pA->vert_first;						
  vold=NULL;
  while(vptr!=NULL)							// if NULL hit, then open poly and flag will not be set otherwise
    {
    i++;
    if(vptr->x<pA_xmin)pA_xmin=vptr->x;
    if(vptr->x>pA_xmax)pA_xmax=vptr->x;
    if(vptr->y<pA_ymin)pA_ymin=vptr->y;
    if(vptr->y>pA_ymax)pA_ymax=vptr->y;
    vptr->supp=0;							// reset in case previous run left matching IDs
    vold=vptr;
    vptr=vptr->next;							// goto next vtx
    if(vptr==pA->vert_first)break;					// does last vtx equal first vtx ... if so, close polygon
    if(i>(pA->vert_qty+2))break;
    }
  if(i<3)
    {
    if(debug_flag==198)printf("Polygon Intersect Exit:   Incomplete pA Polygon   vtxs added=%d \n",seq_num);
    debug_flag=old_debug_flag;
    return(seq_num);
    }
  if(i>(pA->vert_qty+2))
    {
    if(debug_flag==198)
      {
      printf("Polygon Intersect Exit:   No end to pA Polygon   vtxs added=%d \n",seq_num);
      printf("  pA=%X  next=%X  ID=%d  mem=%d  mnum=%d  type=%d  hole=%d\n",pA,pA->next,pA->ID,pA->member,pA->mdl_num,pA->type,pA->hole);
      printf("  diam=%6.3f  perim=%6.3f  area=%6.3f  dist=%6.3f  x=%6.3f y=%6.3f\n",pA->diam,pA->prim,pA->area,pA->dist,pA->centx,pA->centy);
      while(!kbhit());
      }
    
    //vptr=pA->vert_first;
    //for(i=0;i<pA->vert_qty;i++)vptr=vptr->next;
    //vptr->next=pA->vert_first;
    
    debug_flag=old_debug_flag;
    return(seq_num);
    }
  if(vptr==NULL)
    {
    if(debug_flag==198)printf("\nPI Entry:  pA is an open polygon!\n");
    vold->next=pA->vert_first;
    }
  if(debug_flag==198)printf("  ... done.\n");

  // verify that pB is a closed loop polygon
  if(debug_flag==198)printf("  Verifying pB... vert_qty=%d\n",pB->vert_qty);
  i=0;									// set flag to say "open polygon"
  vptr=pB->vert_first;						
  vold=NULL;
  while(vptr!=NULL)							// if NULL hit, then open poly and flag will not be set otherwise
    {
    i++;
    if(vptr->x<pB_xmin)pB_xmin=vptr->x;
    if(vptr->x>pB_xmax)pB_xmax=vptr->x;
    if(vptr->y<pB_ymin)pB_ymin=vptr->y;
    if(vptr->y>pB_ymax)pB_ymax=vptr->y;
    vptr->supp=0;							// reset in case previous run left matching IDs
    vold=vptr;
    vptr=vptr->next;							// goto next vtx
    if(vptr==pB->vert_first)break;					// does last vtx equal first vtx ... if so, close polygon
    if(i>(pB->vert_qty+2))break;
    }
  if(i<3)
    {
    if(debug_flag==198)printf("Polygon Intersect Exit:   Incomplete pB Polygon   vtxs added=%d \n",seq_num);
    return(seq_num);
    }
  if(i>(pB->vert_qty+2))
    {
    if(debug_flag==198)
      {
      printf("Polygon Intersect Exit:   No end to pB Polygon   vtxs added=%d \n",seq_num);
      printf("  pB=%X  next=%X  ID=%d  mem=%d  mnum=%d  type=%d  hole=%d\n",pB,pB->next,pB->ID,pB->member,pB->mdl_num,pB->type,pB->hole);
      printf("  diam=%6.3f  perim=%6.3f  area=%6.3f  dist=%6.3f  x=%6.3f y=%6.3f\n",pB->diam,pB->prim,pB->area,pB->dist,pB->centx,pB->centy);
      while(!kbhit());
      }
    
    //vptr=pB->vert_first;
    //for(i=0;i<pB->vert_qty;i++)vptr=vptr->next;
    //vptr->next=pB->vert_first;
    
    debug_flag=old_debug_flag;
    return(seq_num);
    }
  if(vptr==NULL)
    {
    if(debug_flag==198)printf("\nPI Entry:  pB is an open polygon!\n");
    vold->next=pB->vert_first;
    }
  if(debug_flag==198)printf("  ... done.\n");

  // identify the source models for each incoming polygon.  this is needed because we need to apply the model offsets
  // when cacluating vector intersections
  mptrA=model_get_pointer(pA->mdl_num);
  mptrB=model_get_pointer(pB->mdl_num);

  // determine if polygons overlap at all by bounding boxes, if not just exit
  if(mptrA!=NULL){pA_xmin+=mptrA->xoff[mtyp]; pA_xmax+=mptrA->xoff[mtyp]; pA_ymin+=mptrA->yoff[mtyp]; pA_ymax+=mptrA->yoff[mtyp];}
  if(mptrB!=NULL){pB_xmin+=mptrB->xoff[mtyp]; pB_xmax+=mptrB->xoff[mtyp]; pB_ymin+=mptrB->yoff[mtyp]; pB_ymax+=mptrB->yoff[mtyp];}
  if((pA_xmax<pB_xmin || pA_ymax<pB_ymin) || (pB_xmax<pA_xmin || pB_ymax<pA_ymin))
    {
    if(debug_flag==198)
      {
      printf("Polygon Intersect Exit:  pA does not overlap pB\n");
      printf("  pA:  xmin=%6.3f  ymin=%6.3f  xmax=%6.3f  ymax=%6.3f\n",pA_xmin,pA_ymin,pA_xmax,pA_ymax);
      printf("  pB:  xmin=%6.3f  ymin=%6.3f  xmax=%6.3f  ymax=%6.3f\n",pB_xmin,pB_ymin,pB_xmax,pB_ymax);
      }
    return(seq_num);
    }

  // Loop thru all the sides of pA comparing each side against all vectors in pB.  When an intersection is found, break each
  // of the intersecting vectors at the intersection point, add in the new vtx(s), and assign them matching "supp" IDs so they
  // can be paired up by both XY location and/or by unique ID.
  i=0; Acount=0;
  vnew=vertex_make();
  Avtxptr=pA->vert_first;
  while(Avtxptr!=NULL)
    {
    Acount++;
    Avtxnxt=Avtxptr->next;
    if(Avtxnxt==NULL)break;
    A=vector_make(Avtxptr,Avtxnxt,1);					// create pA vector from current Avtxptr to Avtxnxt
    Adx=Avtxnxt->x - Avtxptr->x;					// delta x of A - used later to determine direction
    Ady=Avtxnxt->y - Avtxptr->y;					// delta y of A
    if(fabs(Adx)<TOLERANCE && Ady<0)Adx=(-1);				// if x near zero, apply y value to x
    if(fabs(Adx)<TOLERANCE && Ady>0)Adx=1;
    
    Bcount=0;
    Bvtxptr=pB->vert_first;
    while(Bvtxptr!=NULL)						// now compare vecA against all vectors in pB
      {
      Bcount++;
      Bvtxnxt=Bvtxptr->next;
      if(Bvtxnxt==NULL)break;
      B=vector_make(Bvtxptr,Bvtxnxt,2);					// create pB vector from current Bvtxptr to Bvtxnxt
      Bdx=Bvtxnxt->x - Bvtxptr->x;					// delta x of B - used later to determine direction
      Bdy=Bvtxnxt->y - Bvtxptr->y;
      
      vint_result=vector_intersect(A,B,vnew);				// determine intersection, load intersection into temp storage
      vnew->z=Avtxptr->z;
      
      if(debug_flag==199)
        {
	printf("\nPI: case %d - intersection results\n",vint_result);
	printf("    Acnt=%d  Aqty=%d   Bcnt=%d  Bqty=%d \n",Acount,pA->vert_qty,Bcount,pB->vert_qty);
	printf("    A: x0=%6.3f  y0=%6.3f   x1=%6.3f  y1=%6.3f\n",A->tip->x,A->tip->y,A->tail->x,A->tail->y);
	printf("    B: x0=%6.3f  y0=%6.3f   x1=%6.3f  y1=%6.3f\n",B->tip->x,B->tip->y,B->tail->x,B->tail->y);
	printf("  int: x0=%6.3f  y0=%6.3f   i0=%6.3f  j0=%6.3f supp=%d\n",vnew->x,vnew->y,vnew->i,vnew->j,vnew->supp);
	if(i%20==0){while(!kbhit()); i=0;}
	i++;
	}
      
      // check if the vectors are identical (or close enough), then nothing needs to be done.
      if(vector_compare(A,B,CLOSE_ENOUGH)==TRUE)
        {
	// do nothing since they already have their own endpoints in the right places
	}
	
      // process the case where polygon edges cross, inclusive of all tip intersections ...
      // create a pair of new verticies and insert them into both polygons with matching "supp" values
      if(vint_result==204 || vint_result==206 || vint_result==236)	// if intersection inclusive of tip endpts found ...
	{
	// insert the new intersection vertex into pA
	vbrk=vertex_random_insert(Avtxptr,vnew,tolerance);		// insert intersection pt after Avtxptr
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Avtxptr && vbrk!=Avtxnxt)pA->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  //Avtxptr=vbrk;							// move loop ptr onto next element
	  Avtxnxt=Avtxptr->next;					// make sure we have the correct "next" element (it may be vbrk)
	  A->tip=Avtxptr;						// make sure our definition of vector A is up to date
	  A->tail=Avtxnxt;
	  }
	
	// insert the new intersection vertex into pB
	vbrk=vertex_random_insert(Bvtxptr,vnew,tolerance);
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Bvtxptr && vbrk!=Bvtxnxt)pB->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  //Bvtxptr=vbrk;							// move loop ptr onto next element
	  Bvtxnxt=Bvtxptr->next;					// make sure we have the correct "next" element
	  B->tip=Bvtxptr;						// make sure our definition of vector B is up to date
	  B->tail=Bvtxnxt;
	  }
	gerber_vtx_inter++;						// increment unique ID counter
	seq_num++;							// increment number of intersections found
	}	// end of "if intersection 204-206 found"
	
      // process the special case where polygon edges actually overlap ... in which case the intersecton pts will be
      // a pair of the vector coords.  use the return value of the vector intersect function to determine which pair.
      // once the overlap is identified and and the two vectors are broken appropriately the result is 2 vectors where
      // the overlap occurs.  in this function, it leaves a pair of exactly overlapping vectors - one for each polygon.
      
      // colinear - if all of A falls inside of B
      // we leave A alone, break B at A's end pts leaving 3 unique segments
      else if(vint_result==262 || vint_result==263)
	{
	// break B at 1st pt
	if(vint_result==262){vtxc=A->tip; if(Adx<0)vtxc=A->tail;}
	if(vint_result==263){vtxc=A->tip; if(Adx<0)vtxc=A->tail;}
	vbrk=vertex_random_insert(Bvtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL && vbrk!=Bvtxptr)					// if the insertion function return a successful result...
	  {
	  Avtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Bvtxptr && vbrk!=Bvtxnxt)pB->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Bvtxptr=vbrk;							// increment our current position to new vtx
	  Bvtxnxt=Bvtxptr->next;					// make sure we have the correct "next" element
	  B->tip=Bvtxptr;						// make sure our definition of vector B is up to date
	  B->tail=Bvtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	// break what's left of B again at 2nd pt
	if(vint_result==262){vtxc=A->tail; if(Adx<0)vtxc=A->tip;}
	if(vint_result==263){vtxc=A->tail; if(Adx<0)vtxc=A->tip;}
	vbrk=vertex_random_insert(Bvtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL && vbrk!=Bvtxptr)					// if the insertion function return a successful result...
	  {
	  Avtxnxt->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Bvtxptr && vbrk!=Bvtxnxt)pB->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Bvtxptr=vbrk;							// increment out current position to new vtx
	  Bvtxnxt=Bvtxptr->next;					// make sure we have the correct "next" element
	  B->tip=Bvtxptr;						// make sure our definition of vector B is up to date
	  B->tail=Bvtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	}

      // colinear - if all of B falls inside of A
      // we delete B entirely, break A at B's end pts leaving 3 unique segments
      else if(vint_result==264 || vint_result==265)
	{
	// break A at 1st pt - need to check with way A is pointing
	if(vint_result==264){vtxc=B->tip;  if(Adx<0)vtxc=B->tail;}
	if(vint_result==265){vtxc=B->tail; if(Adx<0)vtxc=B->tip;}
	vbrk=vertex_random_insert(Avtxptr,vtxc,tolerance);		// insert intersection pt after Avtxptr
	if(vbrk!=NULL && vbrk!=Avtxptr)					// if the insertion function return a successful result...
	  {
	  Bvtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Avtxptr && vbrk!=Avtxnxt)pA->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Avtxptr=vbrk;							// increment out current position to new vtx
	  Avtxnxt=Avtxptr->next;					// make sure we have the correct "next" element
	  A->tip=Avtxptr;						// make sure our definition of vector A is up to date
	  A->tail=Avtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	// break what's left of A again at 2nd pt
	if(vint_result==264){vtxc=B->tail; if(Adx<0)vtxc=B->tip;}
	if(vint_result==265){vtxc=B->tip;  if(Adx<0)vtxc=B->tail;}
	vbrk=vertex_random_insert(Avtxptr,vtxc,tolerance);		// insert intersection pt after Avtxptr
	if(vbrk!=NULL && vbrk!=Avtxptr)					// if the insertion function return a successful result...
	  {
	  Bvtxnxt->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Avtxptr && vbrk!=Avtxnxt)pA->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Avtxptr=vbrk;							// increment out current position to new vtx
	  Avtxnxt=Avtxptr->next;					// make sure we have the correct "next" element
	  A->tip=Avtxptr;						// make sure our definition of vector B is up to date
	  A->tail=Avtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	}

      // colinear - segment falls between A and B with A as 1st pt, B as 2nd pt
      // we break A then break B at the pts given in vnew
      else if(vint_result>=266 && vint_result<=269)
	{
	// break A at 1st pt
	if(vint_result==266){vtxc=A->tip;  if(Adx<0)vtxc=B->tip;}
	if(vint_result==267){vtxc=A->tip;  if(Adx<0)vtxc=B->tail;}
	if(vint_result==268){vtxc=A->tail; if(Adx<0)vtxc=B->tip;}
	if(vint_result==269){vtxc=A->tail; if(Adx<0)vtxc=B->tail;}
	vbrk=vertex_random_insert(Avtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  Bvtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Avtxptr && vbrk!=Avtxnxt)pA->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Avtxptr=vbrk;							// increment out current position to new vtx
	  Avtxnxt=Avtxptr->next;					// make sure we have the correct "next" element
	  A->tip=Avtxptr;						// make sure our definition of vector B is up to date
	  A->tail=Avtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	// break B at 2nd pt
	if(vint_result==266){vtxc=B->tip;  if(Adx<0)vtxc=A->tip;}
	if(vint_result==267){vtxc=B->tail; if(Adx<0)vtxc=A->tip;}
	if(vint_result==268){vtxc=B->tip;  if(Adx<0)vtxc=A->tail;}
	if(vint_result==269){vtxc=B->tail; if(Adx<0)vtxc=A->tail;}
	vbrk=vertex_random_insert(Bvtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  Avtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Bvtxptr && vbrk!=Bvtxnxt)pB->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Bvtxptr=vbrk;							// increment out current position to new vtx
	  Bvtxnxt=Bvtxptr->next;					// make sure we have the correct "next" element
	  B->tip=Bvtxptr;						// make sure our definition of vector B is up to date
	  B->tail=Bvtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	}

      // colinear - segment falls between A and B with B as 1st pt, A as 2nd pt
      // we break B then break A at the pts given in vnew
      else if(vint_result>=270 && vint_result<=273)
	{
	// break B at 1st pt
	if(vint_result==270){vtxc=B->tip;  if(Adx<0)vtxc=A->tip;}
	if(vint_result==271){vtxc=B->tip;  if(Adx<0)vtxc=A->tail;}
	if(vint_result==272){vtxc=B->tail; if(Adx<0)vtxc=A->tip;}
	if(vint_result==273){vtxc=B->tail; if(Adx<0)vtxc=A->tail;}
	vbrk=vertex_random_insert(Bvtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  Avtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Bvtxptr && vbrk!=Bvtxnxt)pB->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Bvtxptr=vbrk;							// increment out current position to new vtx
	  Bvtxnxt=Bvtxptr->next;					// make sure we have the correct "next" element
	  B->tip=Bvtxptr;						// make sure our definition of vector B is up to date
	  B->tail=Bvtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	// break A at 2nd pt
	if(vint_result==270){vtxc=A->tip;  if(Adx<0)vtxc=B->tip;}
	if(vint_result==271){vtxc=A->tail; if(Adx<0)vtxc=B->tip;}
	if(vint_result==272){vtxc=A->tip;  if(Adx<0)vtxc=B->tail;}
	if(vint_result==273){vtxc=A->tail; if(Adx<0)vtxc=B->tail;}
	vbrk=vertex_random_insert(Avtxptr,vtxc,tolerance);		// insert intersection pt after Bvtxptr
	if(vbrk!=NULL)							// if the insertion function return a successful result...
	  {
	  Bvtxptr->supp=gerber_vtx_inter;
	  vbrk->supp=gerber_vtx_inter;					// assign unique intersection ID to vtx regardless if new or Bvtxptr or Bvtxnxt
	  if(vbrk!=Avtxptr && vbrk!=Avtxnxt)pA->vert_qty++;		// if a new vtx was created... increment polygon vtx qty
	  Avtxptr=vbrk;							// increment out current position to new vtx
	  Avtxnxt=Avtxptr->next;					// make sure we have the correct "next" element
	  A->tip=Avtxptr;						// make sure our definition of vector B is up to date
	  A->tail=Avtxnxt;
	  gerber_vtx_inter++;						// increment unique ID counter
	  seq_num++;							// increment number of intersections found
	  }
	}
	
      free(B);vector_mem--;
      Bvtxptr=Bvtxptr->next;	
      if(Bvtxptr==pB->vert_first)break;
      if(Bcount>pB->vert_qty)break;
      }
    
    free(A);vector_mem--;
    Avtxptr=Avtxptr->next;
    if(Avtxptr==pA->vert_first)break;
    if(Acount>pA->vert_qty)break;
    }
    
  free(vnew);vertex_mem--;
  
  if(debug_flag==198)printf("Polygon Intersect Exit:   vtxs added=%d \n",seq_num);
  //if(seq_num%2!=0)printf("  WARNING:  Odd number of intersections found!\n");

  debug_flag=old_debug_flag;
  return(seq_num);
}

// Function to boolean two polygons
//
// Inputs:  pA, pB = input polygons,  action = ADD, SUBTRACT, AND, OR, XOR
// Return:  A pointer to the head of a polygon linked list of resutls.
polygon *polygon_boolean2(slice *sinpt, polygon *pA, polygon *pB, int action)
{
  int		i,ptyp,vint_result;
  vertex	*vptr,*vtest,*vtxint,*vtxnew;
  vector	*veclistA,*veclistB,*vA,*vB,*veclastA,*veclastB;
  vector	*vecptr,*vecnew,*vecdel,*vecpre,*vecnxt,*vecold,*veclist;
  vector 	*vecptrA,*vecoldA,*vecptrB,*vecoldB;
  polygon	*pptr,*pnew,*pold,*pnew_first,*new_poly_list;
  model		*mptr;
  vector_list	*vec_list,*vl_first,*vl_ptr,*vl_del;

  // validate input
  if(pA==NULL || pB==NULL)return(NULL);
  debug_flag=199;
  
  // test if all of pB vtxs lie inside of pA
  {
    if(debug_flag==199)printf("Boolean2:  pB inside pA? \n");
    vint_result=TRUE;
    vptr=pB->vert_first;
    while(vptr!=NULL)
      {
      i=polygon_contains_point(pA,vptr);
      if(debug_flag==199)printf("  x=%10.8f  y=%10.8f  %s \n",vptr->x,vptr->y,i?"IN":"OUT");
      if(i==FALSE)vint_result=FALSE;
      vptr=vptr->next;
      if(vptr==pB->vert_first)break;
      }
    if(debug_flag==199)printf("\n");
    if(vint_result==TRUE)
      {
      if(debug_flag==199)printf("Boolean2:  All of pB inside pA.\n");
      new_poly_list=pA;
      return(new_poly_list);
      }
  }

  // test if all of pA vtxs lie inside of pB
  {
    if(debug_flag==199)printf("Boolean2:  pA inside pB? \n");
    vint_result=TRUE;
    vptr=pA->vert_first;
    while(vptr!=NULL)
      {
      i=polygon_contains_point(pB,vptr);
      if(debug_flag==199)printf("  x=%10.8f  y=%10.8f  %s \n",vptr->x,vptr->y,i?"IN":"OUT");
      if(i==FALSE)vint_result=FALSE;
      vptr=vptr->next;
      if(vptr==pA->vert_first)break;
      }
    if(debug_flag==199)printf("\n");
    if(vint_result==TRUE)
      {
      if(debug_flag==199)printf("Boolean2:  All of pA inside pB.\n");
      new_poly_list=pB;
      return(new_poly_list);
      }
  }

  // break polygon vectors where they cross and/or overlap
  printf("testing intersections...\n");
  debug_flag=0;
  vint_result=polygon_intersect(pA,pB,TOLERANCE);
  if(vint_result==0)return(NULL);					// if no intersections, nothing to boolean
  
  // copy both polygons into vector lists
  {
  printf("creating vector list A...\n");
    ptyp=pA->type;							// set type to be same
    veclistA=polylist_2_veclist(pA,ptyp);				// convert pA to vector list
    veclastA=NULL;
    vecptr=veclistA;
    while(vecptr!=NULL)
      {
      veclastA=vecptr;							// save address of last vec in list A
      vecptr=vecptr->next;
      if(vecptr==veclistA)break;
      }
      
  printf("creating vector list B...\n");
    veclistB=polylist_2_veclist(pB,ptyp);				// convert pB to vector list
    veclastB=NULL;
    vecptr=veclistB;
    while(vecptr!=NULL)
      {
      veclastB=vecptr;							// save address of last vec in list B
      vecptr=vecptr->next;
      if(vecptr==veclistB)break;
      }
  }
  
  // apply boolean ADD/OR logic - area encompassed by both pA and pB
  printf("performing boolean:  action=%d \n",action);
  vtest=vertex_make();
  if(action==ACTION_ADD || action==ACTION_OR)
    {
    // merge the A and B vec lists together
    veclastA->next=veclistB;
    veclistB->prev=veclastA;
  
    // identify invalid vectors
    i=0;
    vecptr=veclistA;
    while(vecptr!=NULL)
      {
      i++;
      vecptr->status=vector_winding_number(veclistA,vecptr);
      //printf("  vecptr=%X  status=%d \n",vecptr,vecptr->status);
      vecptr=vecptr->next;
      }
    
    // remove invalid vector loops
    vecptr=veclistA;
    while(vecptr!=NULL)
      {
      //printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X  ",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
      
      if(vecptr->status<0)
        {
	//printf(" delete \n");
	if(vecptr==veclistA)veclistA=vecptr->next;
	if(vecptr->prev!=NULL)(vecptr->prev)->next=vecptr->next;	// adjust list to skip over this vector going forward
	if(vecptr->next!=NULL)(vecptr->next)->prev=vecptr->prev;	// adjust list to skip over this vector going backward
	
	vecdel=vecptr;
	vecptr=vecptr->next;
	free(vecdel->tip);  vertex_mem--; vecdel->tip=NULL;
	free(vecdel->tail); vertex_mem--; vecdel->tail=NULL;
	free(vecdel); vector_mem--; vecdel=NULL;
	total_vector_count--;
	
	if(vecptr==NULL)break;
	//printf(" vecptr=%X  prev=%X  next=%X   tip=%X tail=%X    new\n",vecptr,vecptr->prev,vecptr->next,vecptr->tip,vecptr->tail);
	}
      else 
        {
	//printf(" next\n");
	vecptr=vecptr->next;
	}
      }

    }
  free(vtest);vertex_mem--;
  
  // apply boolean SUBTRACT logic -  pA=Source, pB=What's removed
  vtest=vertex_make();
  if(action==ACTION_SUBTRACT)
    {
    // loop thru all of pA and remove any vectors INSIDE of pB
    // loop thru all of pB and remove any vectors OUTSIDE of pA
    }
  free(vtest);vertex_mem--;
  
  
  // sort vectors to ensure proper sequencing
  {
    vec_list=NULL;
    vec_list=vector_sort(veclistA,TOLERANCE,0);				// sort vector list tip-to-tail into vec lists
  }

  // colinear merge any overtly short/shallow vectors
  {
    vl_ptr=vec_list;
    while(vl_ptr!=NULL)
      {
      vl_ptr->v_item=vector_colinear_merge(vl_ptr->v_item,ptyp,max_colinear_angle);
      vl_ptr=vl_ptr->next;
      }
  }

  // reassemble the modified veclist back into polygons
  {
    new_poly_list=NULL;
    if(vec_list!=NULL)
      {
      i=0;
      pold=NULL;
      vl_ptr=vec_list;
      while(vl_ptr!=NULL)
	{
	pptr=veclist_2_single_polygon(vl_ptr->v_item,ptyp);
	if(pptr!=NULL)
	  {
	  pptr->area=polygon_find_area(pptr);
	  if(fabs(pptr->area)>min_polygon_area)
	    {
	    pptr->dist=pA->dist;
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
  }

  // process new polygons
  {
    if(new_poly_list!=NULL)
      {
      pptr=new_poly_list;
      while(pptr!=NULL)
	{
	pptr->area=polygon_find_area(pptr);
	polygon_find_center(pptr);					// find the centroid of polygon
	polygon_find_start(pptr,NULL);					// just put the start at a corner for now
	pptr=pptr->next;
	}
      }
  }

  return(new_poly_list);						// return head pointer to new polygon(s)

  // delete the raw vec list and its contents as it's no longer needed since we have the polygons
  {
    vl_ptr=vec_list;
    while(vl_ptr!=NULL)
      {
      vecptr=vl_ptr->v_item;
      while(vecptr!=NULL)
	{
	vecdel=vecptr;
	vecptr=vecptr->next;
	if(vecptr==vl_ptr->v_item)break;
	if(vecdel->tip!=NULL){free(vecdel->tip);vertex_mem--;}
	if(vecdel->tail!=NULL){free(vecdel->tail);vertex_mem--;}
	if(vecdel!=NULL){free(vecdel);vector_mem--;}
	}
      vl_ptr=vl_ptr->next;
      }
  }

  return(new_poly_list);						// return head pointer to new polygon(s)
}

// Function to generate true polygons (outlines) from psuedo polygons (grids) via edge detection.
// the inbound psuedo polygon is a single polygon with vtx values for x,y, and material presence indicator
// in one long linked list.  this function converts that singular list into actual seperate contour polygons.
//
// this function requires that the polygons to be created are padded with at least one layer of 0's around the 
// perimeter of the polygon.  it other words, it relies on starting off on a 0 (not it material) to work properly.
int polygon_edge_detection(slice *sptr, int ptyp)
{
  int		vtx_cnt;
  vertex	*vptr,*vnew,*vnxt,*vpre,*vplp,*vstart;
  polygon 	*pptr,*pnew;
  
  if(sptr->pqty[ptyp]!=1)return(0);					// if in grid array format, only one polygon should exist
  if(sptr->stype[ptyp]>0)return(0);					// this slice has already been processed
  pptr=sptr->pfirst[ptyp];						// init pointer to psuedo grid polygon
  if(pptr==NULL)return(0);						// if psuedo grid does not exist
  if(pptr->vert_qty<3)return(0);					// ensure it contains enough to process
  if(pptr->vert_first==NULL)return(0);					// nothing to process
  
  //printf("\nPolygon Edge Detection entry...\n");

  // find an unspent pair of verticies indicating material state change.  when found loop around
  // the state change boundary in clockwise fashion to build a polygon.  mark each vertex that goes
  // into the polygon as "spent" so it won't be used to generate redundant polygons.  at the end of this
  // loop should be a collection of polygons similar to what we get when slicing a model.
  vtx_cnt=0;
  vpre=pptr->vert_first;
  vptr=vpre;
  vnxt=vptr;
  while(vptr!=NULL)							// loop thru all vtxs of psuedo polygon
    {
    //printf(" %d/%d ",vtx_cnt,pptr->vert_qty);
    vtx_cnt++;  
      
    vptr->z=1;
    if(vpre->k!=1 && vptr->k!=1 && vptr->supp!=vpre->supp)		// if neither spent and a material state change is found ...
      {
      // create the starting vtx of the new polygon
      vnew=vertex_make();
      vnew->x=vptr->x;
      vnew->y=vptr->y;
      pnew=polygon_make(vnew,1,1);
      polygon_insert(sptr,ptyp,pnew);
      pnew->vert_qty=1;
      
      // loop clockwise from starting vplp searching for same state
      // this will ultimately trace around all "on" vtx in clockwise fashion until back at start
      //printf("\nSearching for next polygon loop...\n");
      if(vpre->supp==1)vplp=vpre;					// if a hole, trace from vpre
      if(vptr->supp==1)vplp=vptr;					// if a solid, trace from vptr
      vstart=vplp;
      
      //printf("  vpre=%X  vptr=%X  vplp=%X \n",vpre,vptr,vplp);
      
      if(vstart!=NULL)
        {
	while(TRUE)
	  {
	  vnxt=vertex_find_in_grid(pptr,vplp);				// find next neighbor with same state
	  if(vnxt==vstart)break;					// if back at first vtx, this polygon is done
	  if(vnxt==NULL)break;
	  vnew->x=vnxt->x;
	  vnew->y=vnxt->y;
	  pnew->vert_qty++;						// increment vtx qty
	  vnew->next=vertex_make();					// make another new vtx
	  vplp=vnxt;							// set the next vtx to check as current
	  vnew=vnew->next;						// point to next vtx to be loaded
	  }
	}
      //polygon_dump(pnew);
	
      // at this point we should have an open polygon with one extra vtx at the end
      // get rid of the extra vtx and close the polygon loop.  this gives us a true closed loop polygon.
      if(pnew->vert_qty>3)
	{
	//printf("New polygon created @ vtx=%d  qty=%d\n",vtx_cnt,pnew->vert_qty);
	vplp=pnew->vert_first;						// use vplp at "previous vtx" tracker
	vnxt=vplp->next;						// start with next one in list
	while(vnxt->next!=NULL)						// loop thru to last one in list
	  {
	  vplp=vnxt;							// save as "previous"
	  vnxt=vnxt->next;						// move onto next vtx
	  }
	vplp->next=pnew->vert_first;					// point the vtx before the last one (vplp) back to the first
	if(vnxt!=NULL){free(vnxt);vertex_mem--;vnxt=NULL;}		// delete the last one in the list
	}
      else
        {
	//printf("New polygon deleted @ vtx=%d\n",vtx_cnt);
	polygon_delete(sptr,pnew);					// if not enough vtxs were found...
	}
      }
      
    vptr->z=0;
    vpre=vptr;
    vptr=vptr->next;
    }

  //printf("\nPolygon Edge Detection exit...\n\n");
  return(1);
}


