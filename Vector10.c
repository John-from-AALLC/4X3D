#include "Global.h"


// This function allocates space for a single link list element of type vector
vector *vector_make(vertex *A, vertex *B, int typ)
{
  vector	*vptr;

  vptr=(vector *)malloc(sizeof(vector));				/* Go get memory and new address. */
  if(vptr==NULL)return(NULL);						/* Exit with failure. */
  if(A==NULL)A=vertex_make();
  vptr->tip=A;
  if(B==NULL)B=vertex_make();
  vptr->tail=B;
  vptr->psrc=NULL;
  vptr->vsrcA=NULL;
  vptr->vsrcB=NULL;
  vptr->curlen=(-1);
  vptr->member=(-1);
  vptr->type=typ;
  vptr->status=(-1);
  vptr->crmin=(-1);
  vptr->crmax=(-1);
  vptr->wind_dir=(-1);
  vptr->wind_min=(-1);
  vptr->wind_max=(-1);
  vptr->next=NULL;
  vptr->prev=NULL;
  vector_mem++;
  return(vptr);
}

// Function to insert new vector into linked list of vectors
int vector_insert(slice *sptr, int ptyp, vector *vecnew)
{
  if(sptr==NULL || vecnew==NULL)return(0);
  if(ptyp<=0 || ptyp>=MAX_LINE_TYPES)return(0);
  
  if(sptr->ss_vec_first[ptyp]==NULL)						// special case for first vector loaded
    {
    sptr->ss_vec_first[ptyp]=vecnew;	
    vecnew->prev=NULL;
    }
  else
    {
    sptr->ss_vec_last[ptyp]->next=vecnew;
    vecnew->prev=sptr->ss_vec_last[ptyp];
    }
  sptr->ss_vec_last[ptyp]=vecnew;
  vecnew->next=NULL;
  sptr->ss_vec_qty[ptyp]++;							/* Increment number of elements. */
  return(1);								/* Exit with success. */
}

// Function to insert new vector into linked list of vectors
int vector_raw_insert(slice *sptr, int ptyp, vector *vecnew)
{
  if(sptr==NULL || vecnew==NULL)return(0);
  
  if(sptr->raw_vec_first[ptyp]==NULL){sptr->raw_vec_first[ptyp]=vecnew;} // special case for first vector loaded    
  else {sptr->raw_vec_last[ptyp]->next=vecnew;}				// otherwise add to end of list
  sptr->raw_vec_last[ptyp]=vecnew;					// update end of list ptr
  vecnew->next=NULL;							// ensure nothing afterwards
  return(TRUE);
}

// Function to delete a vector from the vector list and out of memory.
vector *vector_delete(vector *veclist, vector *vdel)
{
  vector *vptr,*newlist,*vpre,*vnxt,*veclast;

  // validate input
  if(veclist==NULL || vdel==NULL)return(veclist);
  
  if(vdel==veclist)							// if it happens to be the very first vector then ...
    {
    newlist=vdel->next;							// ... reassign the first vector to the next one
    veclast=veclist->prev;
    if(newlist!=NULL)newlist->prev=veclast;
    }
  else 
    {
    newlist=veclist;
    vpre=vdel->prev;
    vnxt=vdel->next;
    if(vpre!=NULL)vpre->next=vnxt;
    if(vnxt!=NULL)vnxt->prev=vpre;
    }
  if(vdel->tip!=NULL) free(vdel->tip);  vertex_mem--;
  if(vdel->tail!=NULL)free(vdel->tail); vertex_mem--;
  free(vdel); vector_mem--;	
  
  return(newlist);
}

// Function to delete vectors of specific status
vector *vector_wipe(vector *vec_list, int del_typ)
{
  vector	*new_vec_list,*vecptr,*vecdel;
  vertex	*vptr,*vdel;
  
  if(vec_list==NULL)return(NULL);					// validate input
  new_vec_list=vec_list;						// default return list address to same address and input list
  
  vecptr=vec_list;							// start at top of list
  while(vecptr!=NULL)							// loop thru whole list
    {
    if(vecptr->status==del_typ)						// if this vector matches the deletion status type...
      {
      if(vecptr==vec_list)new_vec_list=vecptr->next;			// if first vector in list, redefine start of list
      vecdel=vecptr;							// save address of vector to delete
      vecptr=vecptr->next;						// move our loop pointer forward
      vdel=vecdel->tip;							// delete the tip vertex
      free(vdel);vertex_mem--;
      vdel=vecdel->tail;						// delete the tail vertex
      free(vdel);vertex_mem--;			
      free(vecdel);vector_mem--;					// delete the vector itself
      if(vecptr=vec_list)break;
      }  
    else 								// otherwise, if not marked to delete...
      {  
      vecptr=vecptr->next;						// move onto next vector
      if(vecptr=vec_list)break;
      }
    }
  
  return(new_vec_list);
}

// Function to dump a vector list to consol
int vector_list_dump(vector *vec_list)
{
  vector 	*vecptr;
  
  if(vec_list==NULL)return(0);
  printf("\n\nVector list dump:  entry  vec_list=%X\n",vec_list);
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    printf("\n  %X  next=%X  type=%d  status=%d  len=%6.3f  psrc=%X \n",vecptr,vecptr->next,vecptr->type,vecptr->status,vecptr->curlen,vecptr->psrc);
    printf("  	Tip:  %X	x=%6.3f y=%6.3f z=%6.3f supp=%d\n",vecptr->tip,vecptr->tip->x,vecptr->tip->y,vecptr->tip->z,vecptr->tip->supp);
    printf("  	Tail: %X	x=%6.3f y=%6.3f z=%6.3f supp=%d\n",vecptr->tail,vecptr->tail->x,vecptr->tail->y,vecptr->tail->z,vecptr->tail->supp);
    vecptr=vecptr->next;
    if(vecptr==vec_list)break;
    }
    
  return(1);
}

// Function to compare two vectors.  returns true if same
int vector_compare(vector *A, vector *B, float tol)
{
  if(A==NULL || B==NULL)return(0);
  if(A->type == B->type)
    {
    if(vertex_compare(A->tip,B->tip,tol) && vertex_compare(A->tail,B->tail,tol))return(TRUE);
    if(vertex_compare(A->tip,B->tail,tol) && vertex_compare(A->tail,B->tip,tol))return(TRUE);
    }
  return(FALSE);
}

// Function to make a copy of a vector's vertexes and parameters.  Note that this does NOT
// copy items such as source polygons, source vectors, or vertex_index buckets.
vector *vector_copy(vector *vec_src)
{
  vertex	*new_tip,*new_tail;
  vector 	*vec_copy;
  
  new_tip=vertex_copy(vec_src->tip,NULL);
  new_tail=vertex_copy(vec_src->tail,NULL);
  vec_copy=vector_make(new_tip,new_tail,vec_src->type);
  vec_copy->curlen=vec_src->curlen;
  vec_copy->angle=vec_src->angle;
  vec_copy->member=vec_src->member;
  vec_copy->type=vec_src->type;
  vec_copy->status=vec_src->status;
  vec_copy->crmin=vec_src->crmin;
  vec_copy->crmax=vec_src->crmax;
  vec_copy->wind_dir=vec_src->wind_dir;
  vec_copy->wind_min=vec_src->wind_min;
  vec_copy->wind_max=vec_src->wind_max;

  return(vec_copy);
}

// Function to merge collinear vectors with less than "delta" angle, or if too short, together.
// this function relies on the vector list being sorted tip-to-tail before hand, and that all vectors are
// circularly linked in both directions (i.e. a prepolygon loop).
vector *vector_colinear_merge(vector *vec_first, int ptyp, float delta)
{
  int 		i,vec_cnt=0,del_cnt=0;
  int 		vecpre_ok=0,merge_pre=0,vecnxt_ok=0,merge_nxt=0;
  int		inbound_cnt,outbound_cnt;
  float 	inbound_len,outbound_len;
  float 	max_angle,pre_angle,nxt_angle,total_len;
  double 	dx0,dy0,dx1,dy1,dx2,dy2;
  vertex	*vptip,*vptail,*vctip,*vctail,*vntip,*vntail,*del_tail,*del_tip;
  vector 	*vecpre,*vecptr,*vecnxt,*veclast;

  // validate inputs
  if(vec_first==NULL)return(NULL);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(vec_first);
  
  // count the number of vectors and ensure it is a closed loop.  also add up the
  // total perimeter length so that the final result can be compared to it.
  inbound_cnt=0; inbound_len=0.0;
  vec_cnt=0;
  vecptr=vec_first;
  while(vecptr!=NULL)
    {
    inbound_cnt++;
    inbound_len += vecptr->curlen;
    vec_cnt++;
    vecptr=vecptr->next;
    if(vecptr==vec_first)break;
    }
  if(vecptr!=vec_first){vecptr->next=vec_first; vec_first->prev=vecptr;}
  if(vec_cnt<4)return(vec_first);
  
  // loop thru vector list comparing the angles of the current vector (vecptr) to the previous and next vectors.  if the current
  // vector is too short, first merge it to either the prev or next vector depending on which one is closest in angle.  and if not
  // merged due to magnitude, check if they are colinear enough to merge.
  i=0; del_cnt=0;
  max_angle=delta*PI/180.0;
  vecptr=vec_first;
  while(i<(vec_cnt+5))
    {
    vecpre=vecptr->prev;
    vecnxt=vecptr->next;
    if(vecpre==vecnxt)break;						// if list has diminished down to 1 vector...
    
    if(debug_flag==76)
      {
      printf(" \ni=%d\n",i);
      printf(" vecpre=%X  tip=%X x=%6.3f y=%6.3f  tail=%X x=%6.3f y=%6.3f  l=%6.4f\n",vecpre,vecpre->tip,vecpre->tip->x,vecpre->tip->y,vecpre->tail,vecpre->tail->x,vecpre->tail->y,vecpre->curlen);
      printf(" vecptr=%X  tip=%X x=%6.3f y=%6.3f  tail=%X x=%6.3f y=%6.3f  l=%6.4f\n",vecptr,vecptr->tip,vecptr->tip->x,vecptr->tip->y,vecptr->tail,vecptr->tail->x,vecptr->tail->y,vecptr->curlen);
      printf(" vecnxt=%X  tip=%X x=%6.3f y=%6.3f  tail=%X x=%6.3f y=%6.3f  l=%6.4f\n",vecnxt,vecnxt->tip,vecnxt->tip->x,vecnxt->tip->y,vecnxt->tail,vecnxt->tail->x,vecnxt->tail->y,vecnxt->curlen);
      }

    // test if consqueutive vectors share the same tip->tail endpts making them eligible for merging.  set flags as such.
    vecpre_ok = vertex_compare(vecpre->tail,vecptr->tip,CLOSE_ENOUGH);	// ok to merge with prev vec as is
    vecnxt_ok = vertex_compare(vecptr->tail,vecnxt->tip,CLOSE_ENOUGH);	// ok to merge with next vec as is
    if(debug_flag==76)printf(" vecpre=%d  vecnxt=%d  \n",vecpre_ok,vecnxt_ok);

    if(vecpre_ok==FALSE && vecnxt_ok==FALSE)				// if no viable neighbors... something went wrong.
      {
      printf("VCM:  discontinuous vector loop.\n");
      vecptr=vecptr->next;
      i++;
      continue;
      }
    
    // calculate angle differences between prev, current, and next accounting for vectors at same angle
    // but pointing in opposite directions.
    pre_angle=fabs(vecpre->angle - vecptr->angle);			// get difference in angle
    if(pre_angle>=PI)pre_angle-=PI;					// account for pointing opposite
    nxt_angle=fabs(vecptr->angle - vecnxt->angle);
    if(nxt_angle>=PI)nxt_angle-=PI;
    
    if(debug_flag==76)printf(" pre_angle=%5.1f  nxt_angle=%5.1f  \n",(180*pre_angle/PI),(180*nxt_angle/PI));
    
    // check if vecptr is so short that it needs to be merged (regardless of angle)
    //min_vector_length=0.0;						// DEBUG - take vec length merge out of equation for now
    if(vecptr->curlen<min_vector_length)
      {
      if(vecptr->curlen<TOLERANCE){vecpre_ok=FALSE;vecnxt_ok=TRUE;}	// if a near zero length vector ... just set to merge with next
      if(vecpre_ok==TRUE && vecnxt_ok==TRUE)				// if we can merge to either... chose the one with less angle
        {
	if(pre_angle<nxt_angle){vecnxt_ok=FALSE;}			// ... if prev is the better option
	else {vecpre_ok=FALSE;}						// ... if next is the better option
	}
      if(vecpre_ok==FALSE && vecnxt_ok==TRUE)				// if we can only merge to next...
	{
	if(debug_flag==76)printf(" length merge with next\n");
	if(vecnxt==vec_first)vec_first=vecptr;				// ... if changing our entry ptr into the list
	del_tail=vecptr->tail;						// ... save address of tail we'll need to free
	del_tip=vecnxt->tip;						// ... save address of tip we'll need to free
	vecptr->tail=vecnxt->tail;					// ... skip over old tip & tail
	vecptr->next=vecnxt->next;					// ... skip over next vector
	(vecnxt->next)->prev=vecptr;					// ... restore backwards list direction
	vector_magnitude(vecptr);					// ... re-calc new vector length
	vecptr->angle=vector_absangle(vecptr);				// ... re-calc new vector angle
	if(del_tip!=del_tail)						// ... ensure same vtx is not referenced for both
	  {
	  if(del_tail!=NULL && del_tail!=vecptr->tail){free(del_tail); vertex_mem--;}	// ... delete old tail
	  if(del_tip!=NULL && del_tip!=vecptr->tip){free(del_tip); vertex_mem--;}	// ... delete old tip
	  }
	else 
	  {
	  if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}  	// ... delete old tip/tail since same
	  }
	if(vecnxt!=NULL){free(vecnxt); vector_mem--;}					// ... delete next vector
	vecptr=vecpre;							// ... reset position in loop
	del_cnt++;
	vecptr->status=3;
	if(debug_flag==76)printf("LNXT %6.3f \n",vecptr->curlen);
	}
      else if(vecpre_ok==TRUE && vecnxt_ok==FALSE)			// if we can only merge to prev...
	{
	if(debug_flag==76)printf(" length merge with prev\n");
	if(vecptr==vec_first)vec_first=vecpre;				// ... if changing our entry ptr into the list
	del_tail=vecpre->tail;
	del_tip=vecptr->tip;
	vecpre->tail=vecptr->tail;					// ... skip over old tip & tail
	vecpre->next=vecnxt;						// ... skip over next vector
	vecnxt->prev=vecpre;						// ... restore backwards list direction
	vector_magnitude(vecpre);					// ... re-calc new vector length
	vecpre->angle=vector_absangle(vecpre);				// ... re-calc new vector angle
	if(del_tip!=del_tail)						// ... ensure same vtx is not referenced for both
	  {
	  if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}  	// ... delete old tail
	  if(del_tip!=NULL && del_tip!=vecnxt->tip){free(del_tip); vertex_mem--;}	// ... delete old tip
	  }
	else 
	  {
	  if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}  	// ... delete old tip/tail since same
	  }
	if(vecptr!=NULL){free(vecptr); vector_mem--;}					// ... delete next vector
	vecptr=vecpre;							// ... reset position in loop
	del_cnt++;
	vecptr->status=4;
	if(debug_flag==76)printf("LPRE %6.3f \n",vecptr->curlen);
	}
      else 								// if we cannot merge to either... something went wrong!
        {
	if(debug_flag==76)printf(" length merge with none\n");
	if(debug_flag==76)printf("LNONE %6.3f \n",vecptr->curlen);
	i++;
	vecptr=vecptr->next;						// ... just move onto next vector
	}
      }
    // if vecptr is long enough, and next angle is below delta, and next vector is a neighbor...
    else if(nxt_angle<max_angle && vecnxt_ok==TRUE)			
      {
      if(debug_flag==76)printf(" angle merge with next\n");
      if(vecnxt==vec_first)vec_first=vecptr;				// ... if changing our entry ptr into the list
      del_tail=vecptr->tail;
      del_tip=vecnxt->tip;
      vecptr->tail=vecnxt->tail;					// ... skip over old tip & tail
      vecptr->next=vecnxt->next;					// ... skip over next vector
      (vecnxt->next)->prev=vecptr;					// ... restore backwards list direction
      vector_magnitude(vecptr);						// ... re-calc new vector length
      vecptr->angle=vector_absangle(vecptr);				// ... re-calc new vector angle
      if(del_tip!=del_tail)						// ... ensure same vtx is not referenced for both
	{
	if(del_tail!=NULL && del_tail!=vecptr->tail){free(del_tail); vertex_mem--;}	// ... delete old tail
	if(del_tip!=NULL && del_tip!=vecptr->tip){free(del_tip); vertex_mem--;}		// ... delete old tip
	}
      else 
	{
	if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}  	// ... delete old tip/tail since same
	}
      if(vecnxt!=NULL){free(vecnxt); vector_mem--;}					// ... delete next vector
      vecptr=vecpre;							// ... reset position in loop
      del_cnt++;
      vecptr->status=5;
      if(debug_flag==76)printf("ANXT %6.3f \n",vecptr->curlen);
      }
    // if vecptr is long enough, and prev angle is below delta, and prev vector is a neighbor...
    else if(pre_angle<max_angle && vecpre_ok==TRUE)			
      {
      if(debug_flag==76)printf(" angle merge with prev\n");
      if(vecptr==vec_first)vec_first=vecpre;				// ... if changing our entry ptr into the list
      del_tail=vecpre->tail;
      del_tip=vecptr->tip;
      vecpre->tail=vecptr->tail;					// ... skip over old tip & tail
      vecpre->next=vecnxt;						// ... skip over next vector
      vecnxt->prev=vecpre;						// ... restore backwards list direction
      vector_magnitude(vecpre);						// ... re-calc new vector length
      vecpre->angle=vector_absangle(vecpre);				// ... re-calc new vector angle
      if(del_tip!=del_tail)						// ... ensure same vtx is not referenced for both
	{
	if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}	// ... delete old tail
	if(del_tip!=NULL && del_tip!=vecnxt->tip){free(del_tip); vertex_mem--;}		// ... delete old tip
	}
      else 
	{
	if(del_tail!=NULL && del_tail!=vecpre->tail){free(del_tail); vertex_mem--;}  	// ... delete old tip/tail since same
	}
      if(vecptr!=NULL){free(vecptr); vector_mem--;}					// ... delete next vector
      vecptr=vecpre;							// ... reset position in loop
      del_cnt++;
      vecptr->status=6;
      if(debug_flag==76)printf("APRE %6.3f \n",vecptr->curlen);
      }
    // if vecptr either should not, or cannot, be merged with either neighbor... happens if long enough and enough angle
    else 								// if NOT merging...
      {
      if(debug_flag==76)printf(" angle merge with none\n");
      i++;
      vecptr=vecptr->next;						// ... move onto next vector
      if(debug_flag==76)printf("ANONE\n");
      }
    }

  // count the number of vectors outbound.
  // total perimeter length so that the final result can be compared to it.
  outbound_cnt=0; outbound_len=0.0;
  vecptr=vec_first;
  while(vecptr!=NULL)
    {
    outbound_cnt++;
    outbound_len += vecptr->curlen;
    vecptr=vecptr->next;
    if(vecptr==vec_first)break;
    }
  //if(fabs(inbound_len-outbound_len)>1.0)printf("Vec Colinear Merge: length change=%6.3f  count change=%d \n",(inbound_len-outbound_len),(inbound_cnt-outbound_cnt));

  if(debug_flag==76)printf("Vector Colinear Merge exit:  %d vectors removed\n",del_cnt);
  //debug_flag=0;
  return(vec_first);
}

// Function to purge entire vector list regardless if terminated with NULL or a loop
int vector_purge(vector *vpurge)
{
  int		vec_count=0;
  vector	*vecptr,*vecdel;
  
  if(vpurge==NULL)return(FALSE);
  //printf("Vector Purge Entry:  vector=%ld \n",vector_mem);
  
  // calc exact count of vecs we are deleting
  vecptr=vpurge;
  while(vecptr!=NULL)
    {
    vec_count++;
    vecptr=vecptr->next;
    if(vecptr==vpurge)break;
    }
    
  // perform deletion
  vecptr=vpurge;
  while(vecptr!=NULL)
    {
    vecdel=vecptr;				// set element to be deleted as current one in loop
    vecptr=vecptr->next;			// get pointer to next element BEFORE deleting it
    if(vecdel->tip!=NULL)			// if polygons exist, they reference these vertices - don't delete the tips!
      {
      free(vecdel->tip);			// free memory space holding first vertex
      vertex_mem--;				// decrement vertex memory counter
      }
    if(vecdel->tail!=NULL)			// polygons do not reference tails... delete away
      {
      free(vecdel->tail);			// free memory space holding second vertex
      vertex_mem--;				// decrement vertex memory counter
      }
    free(vecdel);				// free memory space holding pointers and type
    vector_mem--;				// decrement the number of vectors in memory
    vec_count--;
    if(vec_count<=0)break;
    }
  vpurge=NULL;
  //printf("Vector Purge Exit:   vector=%ld \n",vector_mem);
  return(TRUE);
}

// Function to sort the raw vector list into tip-to-tail fashion
// Inputs:  vec_list=head pointer to an unorganized group of vectors that may form multiple polygons
//          tolerance=distance between vtxs to be considered at the same geometeric positiion
//	    mem_flag: 0=ignore vector member value, 1=only sort vectors of same member value
// Output:  a head pointer to a list of vector lists where each list represents a polygon of vectors.
//          additionally, vec_group will be sorted by tip-to-tail, but remain as one big group.
//
// The objective of sort is to simply re-organize the vectors so that they form the most complete
// tip to tail chain possible.  In principle it should not be adding or subtracing length or quantity
// to the inbound vector list.
//
// Note that vec_group will no longer be one contiguous linked list after this routine.  The only access into
// it will be via the vector list pointers (other than the first loop).  Also note that the two lists share
// vectors.  That is, if you delete via one list, you have also deleted parts of the other list.
vector_list *vector_sort(vector *vec_list, float tolerance, int mem_flag)
{
  int		i,j,polygon_count,vec_count,end_swap=FALSE,add_end=TRUE;
  int		inbound_cnt,outbound_cnt;
  float 	dx,dy,dist_tip,dist_tail,mindist,inbound_len,outbound_len;
  vertex	*vtxtemp,*vtxfirst,*vnew,*vold,*vtx_A,*vtx_B;
  vector 	*vecptr,*vecnxt,*vecpre,*vecmin,*nvecptr,*vec_test;
  vector	*vec_first,*vec_last,*vc_ptr,*vecnew,*vecdel;
  polygon	*pnew;
  vector_list	*vl_first,*vl_ptr,*vl_nxt;				// ptr to sorted vec lists
  
  // nothing to process
  if(vec_list==NULL)return(NULL);

  //printf("  Vector sort:  Enter\n");
  // count the number of incoming vectors and ensure link list integrity in both directions.
  inbound_len=0.0;
  inbound_cnt=0;
  vec_last=NULL;
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    vecptr->prev=vec_last;
    vec_last=vecptr;
    inbound_len += vector_magnitude(vecptr);
    inbound_cnt++;
    vecptr=vecptr->next;
    if(vecptr==vec_list)break;
    }
  vec_last->next=NULL;
  
  // loop thru vector list finding closest to either end of vector polygon, then swapping contents.
  // once a vector polygon is closed (i.e. end matches front) it is made circular and moved into
  // a vector list data element.
  vl_first=NULL;							// init to no list
  vec_count=1;
  polygon_count=1;							// use member count to segragate separate polygons of vectors
  vec_first=vec_list;							// init first vec in polygon to start of list
  vecptr=vec_list;							// init starting vector
  vecptr->member=polygon_count;						// define vec poly member
  while(vecptr!=NULL)
    {
    
    // search remainder of veclist to find vector(s) with a matching endpoint(s).  usually there should only be one,
    // and sometimes none, but sometimes also several, which makes sorting ambiguous.  so the right answer is to 
    // create a list of all vectors that share the same end pt, then use the one that originates from the same
    // polygon (if that data exists).  if that data does not exist, follow the longer path.
    add_end=TRUE;							// init to add at end (vs front) of vec loop
    end_swap=FALSE;							// init to no swap of tip-to-tail
    vecmin=NULL;							// init to no matching vector found
    vec_last=vecptr;							// vec_last is last vector in current loop (opposite of vec_first)
    vecnxt=vec_last->next;						// start at vector just past vec_last
    while(vecnxt!=NULL)							// loop thru remainder of vector group
      {
      // if vec member numbers must match by request, but they don't, then skip this vector
      if(mem_flag==TRUE && vecnxt->member!=vec_last->member){vecnxt=vecnxt->next;continue;}
	
      // if the tip of next vector matches the tail of the last vector in the loop...
      if(vertex_compare(vec_last->tail,vecnxt->tip,tolerance)==TRUE)	
	{vecmin=vecnxt;	end_swap=FALSE; add_end=TRUE; break;}
	
      // if the tail of next vector matches the tail of the last vector in the loop...
      if(vertex_compare(vec_last->tail,vecnxt->tail,tolerance)==TRUE)
	{vecmin=vecnxt;	end_swap=TRUE; add_end=TRUE; break;}
	
      // if the tip of next vector matches the tip of the first vector in the loop...
      if(vertex_compare(vec_first->tip,vecnxt->tip,tolerance)==TRUE)
	{vecmin=vecnxt;	end_swap=TRUE; add_end=FALSE; break;}
	
      // if the tail of next vector matches the tip of the last vector in the loop...
      if(vertex_compare(vec_first->tip,vecnxt->tail,tolerance)==TRUE)
	{vecmin=vecnxt;	end_swap=FALSE; add_end=FALSE; break;}
	
      vecnxt=vecnxt->next;						// move onto next vector in group
      }
      
    // if a matching end pt was NOT found in the remainder of the vector list...
    if(vecmin==NULL)
      {
      // check if this last vtx of the loop matches the first vtx of the loop...
      if(vertex_compare(vec_first->tip,vec_last->tail,tolerance)==TRUE)
        {
	// redefine start of remaining vec list as next vector beyound the list that we are about to create.  
	// it is the head node of the remaining collection of unsorted vectors.
	vec_list=vec_last->next;
	vec_last->next=vec_first;					// make this vec list circular
	vec_first->prev=vec_last;					// perserve both directions of list
	vl_first=vector_list_manager(vl_first,vec_first,ACTION_ADD);	// add this loop to our vec list
  
	if(vec_list!=NULL)vec_list->prev=NULL;				// break link to newly created vec list
	vec_first=vec_list;						// define first node of next vec list (loop)
	vecptr=vec_list;						// define vector to start searching from
	vec_count=1;							// reset vector count for next loop
	polygon_count++;						// increment polygon count
	}
      // if the end pt did NOT match within tolerance...
      else 								
        {
	// so the end pts did not match, but they are very close... make a bridge vector to enable closure
	if(vertex_distance(vec_first->tip,vec_last->tail)<=min_vector_length)	
	  {
	  vtx_A=vertex_copy(vec_last->tail,NULL);
	  vtx_B=vertex_copy(vec_first->tip,NULL);
	  vecnew=vector_make(vtx_A,vtx_B,0);				// create a vector to fill the gap
	  vecnew->type=vec_last->type;					// assign likely type
	  vecnew->psrc=vec_last->psrc;					// assign likely source polygon
	  vecnew->member=polygon_count;					// assign likely polygon count
	  vec_list=vec_last->next;					// save address of start of remaining list
	  vecnew->next=vec_first;					// make circular, point vecnew back to first vec in list
	  vec_first->prev=vecnew;					// have vec first point to vecnew
	  vecnew->prev=vec_last;					// add linkage
	  vec_last->next=vecnew;					// add linkage
	  vec_count++;							// increment vec count of this loop by one
	  vl_first=vector_list_manager(vl_first,vec_first,ACTION_ADD);	// add this loop to our vec list
	  if(vec_list!=NULL)vec_list->prev=NULL;			// break link to newly created vec list
	  vec_first=vec_list;						// define first node of next vec list (loop)
	  vecptr=vec_list;						// define vector to start searching from
	  vec_count=1;							// reset vector count for next loop
	  polygon_count++;						// increment polygon count
	  }
	// but if the end pts are far apart, then we need to find the next closest end pt out of the
	// remaining vector list, bridge a new vector to that, but do not close the loop - keep going.
	else 								
	  {
	  vtx_B=vertex_make();
	  vtx_B->x=vec_first->tip->x;
	  vtx_B->y=vec_first->tip->y;
	  vtx_B->z=vec_first->tip->z;
	  mindist=1000;
	  vec_test=vec_last->next;					// start with first vec in remainder of list
	  while(vec_test!=NULL)						// loop thru all the remaining vectors
	    {
	    dist_tip=vertex_distance(vec_last->tail,vec_test->tip);	// get distance to tip of this vector
	    if(dist_tip<mindist)					// if it is the closest so far...
	      {
	      mindist=dist_tip;
	      vtx_B->x=vec_test->tip->x;
	      vtx_B->y=vec_test->tip->y;
	      vtx_B->z=vec_test->tip->z;
	      }
	    dist_tail=vertex_distance(vec_last->tail,vec_test->tail);	// get distance to tail of this vector
	    if(dist_tail<mindist)					// if it is the closest so far...
	      {
	      mindist=dist_tail;
	      vtx_B->x=vec_test->tail->x;
	      vtx_B->y=vec_test->tail->y;
	      vtx_B->z=vec_test->tail->z;
	      }
	    vec_test=vec_test->next;					// move onto next vector
	    }
	    
	  if(mindist<10*min_vector_length)
	    {
	    //printf("\nVS: bridging gap %6.4f in loop %d at z=%6.3f \n",mindist,polygon_count,vec_list->tip->z);
	    
	    vtx_A=vertex_copy(vec_last->tail,NULL);
	    vecnew=vector_make(vtx_A,vtx_B,0);				// create a vector to fill the gap
	    vecnew->type=vec_last->type;					// assign likely type
	    vecnew->psrc=vec_last->psrc;					// assign likely source polygon
	    vecnew->member=polygon_count;					// assign likely polygon ID
	    vecnxt=vec_last->next;					// insert vecnew after vec_last
	    if(vecnxt!=NULL)vecnxt->prev=vecnew;
	    vecnew->next=vecnxt;
	    vecnew->prev=vec_last;
	    vec_last->next=vecnew;
	    vecptr=vecnew;						// reset vecptr to new end node
	    vec_count++;							// increment vec count of this loop by one
	    }
	  else 
	    {
	    //printf("\nVS: bad gap %6.4f in loop %d at z=%6.3f \n",mindist,polygon_count,vec_list->tip->z);
	    
	    // redefine start of remaining vec list as next vector beyound the list that we are about to create.  
	    // it is the head node of the remaining collection of unsorted vectors.
	    vec_list=vec_last->next;
	    vec_last->next=vec_first;					// make this vec list circular
	    vec_first->prev=vec_last;					// perserve both directions of list
	    vl_first=vector_list_manager(vl_first,vec_first,ACTION_ADD);	// add this loop to our vec list
      
	    if(vec_list!=NULL)vec_list->prev=NULL;				// break link to newly created vec list
	    vec_first=vec_list;						// define first node of next vec list (loop)
	    vecptr=vec_list;						// define vector to start searching from
	    vec_count=1;							// reset vector count for next loop
	    polygon_count++;						// increment polygon count
	    }
	  }
	}
      continue;								// restart search for next vector
      }
      
    // if we found a vector with a matching endpt and we are not at the end of the vector list... 
    // swap contents bt the one with matching end pt (vecmin) and the next vector in the list (vecnxt) after
    // ensuring it is a viable vector.
    else 
      {
      // swap ends of vecmin so that we perserve direction (i.e. we want tip-to-tail alignment, not tail-to-tail)
      if(end_swap==TRUE)
        {
	vtxtemp=vecmin->tail;
	vecmin->tail=vecmin->tip;
	vecmin->tip=vtxtemp;
	}

      // remove vecmin from its current location in vec_list linkage
      vecpre=vecmin->prev;
      vecnxt=vecmin->next;
      if(vecpre!=NULL)vecpre->next=vecnxt;
      if(vecnxt!=NULL)vecnxt->prev=vecpre;

      // check the length of vecmin.  if too small, delete it and tweak test vec_last's (or vec_first's) end pt.
      // so that it exactly matches.
      vector_magnitude(vecmin);						// get vector length
      if(vecmin->curlen<tolerance)					// if shorter than our tolerance ...
        {
	if(add_end==TRUE)						// if adding to end of list (vs front of list) ...
	  {
	  if(end_swap==TRUE)						// if swapping tip for tail...
	    {
	    vec_last->tail->x=vecmin->tip->x; 				// ... just make prev vec tail values same as these tip vals
	    vec_last->tail->y=vecmin->tip->y; 
	    vec_last->tail->z=vecmin->tip->z;
	    }
	  else 								// if NOT swapping tip for tail...
	    {
	    vec_last->tail->x=vecmin->tail->x; 				// ... just make prev vec tail values same as these tail vals
	    vec_last->tail->y=vecmin->tail->y; 
	    vec_last->tail->z=vecmin->tail->z;
	    }
	  }
	else 								// if adding to the front of list (vs the end)... 
	  {
	  if(end_swap==TRUE)
	    {
	    vec_first->tip->x=vecmin->tail->x; 
	    vec_first->tip->y=vecmin->tail->y; 
	    vec_first->tip->z=vecmin->tail->z;
	    }
	  else 
	    {
	    vec_first->tip->x=vecmin->tip->x; 
	    vec_first->tip->y=vecmin->tip->y; 
	    vec_first->tip->z=vecmin->tip->z;
	    }
	  }
	free(vecmin->tip);  vertex_mem--;
	free(vecmin->tail); vertex_mem--;
	free(vecmin); vector_mem--;
	continue;							// keep vecptr where it is, search again
	}
      
      // assign properties
      vecmin->member=polygon_count;
	
      // if adding this vector to the end of the vector loop...
      if(add_end==TRUE)
        {
	// insert vecmin after vec_last so in becomes last element in this vector loop
	vecnxt=vec_last->next;
	if(vecnxt!=NULL)vecnxt->prev=vecmin;
	vec_last->next=vecmin;
	vecmin->prev=vec_last;
	vecmin->next=vecnxt;
	vec_last=vecmin;
	vecptr=vec_last->prev;
	}
      // otherwise adding this vector to the front of the vector loop...
      else 
        {
	// insert vecmin before vec_first in vec_list linkage
	vecpre=vec_first->prev;		
	if(vecpre!=NULL)vecpre->next=vecmin;
	vec_first->prev=vecmin;
	vecmin->prev=vecpre;
	vecmin->next=vec_first;
	vec_first=vecmin;
	vecptr=vec_last->prev;
	}
	
      vec_count++;
      }

    // move onto next base vector
    vecptr=vecptr->next;
    }

  //printf("  first part of sort done...\n");
  //while(!kbhit());
  
  // loop thru freshly sorted lists and ensure they are viable.  they must contain at least 3 vectors.  they must
  // meet a minimum perimeter length and a minimum area.
  polygon_count=1;
  outbound_len=0;
  outbound_cnt=0;
  vl_ptr=vl_first;
  while(vl_ptr!=NULL)
    {
    // count vecs and sum up length
    vec_count=0;
    vecptr=vl_ptr->v_item;
    while(vecptr!=NULL)
      {
      outbound_len += vector_magnitude(vecptr);				// calculate current vector magnitude
      outbound_cnt++;
      vecptr->angle=vector_absangle(vecptr);				// calculate current vector angle relative to x axis
      vecptr->member=polygon_count;
      vec_count++;
      vecptr=vecptr->next;
      if(vecptr==vl_ptr->v_item)break;
      }
    
    //printf("  count=%d  len=%f  vl_first=%X  vl_ptr=%X  vl_nxt=%X \n",vec_count,total_len,vl_first,vl_ptr,vl_ptr->next);
    
    // if there are an insufficient number of vectors, or the polygon is just too small... delete it and its contents <==!!!!!
    if(vec_count<3 || outbound_len<min_perim_length)
      {
      vl_nxt=vl_ptr->next;
      vl_first=vector_list_manager(vl_first,vl_ptr->v_item,ACTION_DELETE);	// ... delete this vec list node
      vl_ptr=vl_nxt;
      continue;
      }
    
    polygon_count++;
    vl_ptr=vl_ptr->next;
    }

  //if(fabs(inbound_len-outbound_len)>1.0)
    //{
    //printf("Vector merge:  inbound: cnt=%d len=%6.3f   outbound: cnt=%d len=%6.3f \n",inbound_cnt,inbound_len,outbound_cnt,outbound_len);
    //}
    
  // DEBUG
  /*
  polygon_count=1;
  vl_ptr=vl_first;
  while(vl_ptr!=NULL)
    {
    printf("\n   Polygon count: %d  vl_ptr=%X  vl_nxt=%X\n",polygon_count,vl_ptr,vl_ptr->next);
    vec_count=0;
    vecptr=vl_ptr->v_item;
    while(vecptr!=NULL)
      {
      vec_count++;
      printf("      vecptr=%X  next=%X   x=%6.3f y=%6.3f   x=%6.3f y=%6.3f \n",vecptr,vecptr->next,vecptr->tip->x,vecptr->tip->y,vecptr->tail->x,vecptr->tail->y);
      vecptr=vecptr->next;
      if(vecptr==vl_ptr->v_item)break;
      }
    polygon_count++;
    vl_ptr=vl_ptr->next;
    }
  */
  
  //printf("\n  Vector sort:  Exit with %d vector lists.\n", polygon_count);
  return(vl_first);
}

// Function to calculate how many edge crossings each vector within a vector list has relative to its own list.
// Used to determine if vector holds material or is part of a hole.
int vector_crossings(vector *vec_list, int save_in_new)
{
  int		crossings;
  float 	np_del_x,np_del_y,tp_del_x,tp_del_y;
  double 	vt;
  vector	*vecptr,*vecnxt;
  vertex	*vtip,*vtail,*vtest;
  
  if(vec_list==NULL)return(0);
  
  vtest=vertex_make();
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    crossings=0;
    vtest->x=(vecptr->tip->x+vecptr->tail->x)/2;
    vtest->y=(vecptr->tip->y+vecptr->tail->y)/2;
    vecnxt=vec_list;
    while(vecnxt!=NULL)
      {  
      if(vecnxt!=vecptr)
        {
	np_del_x=vecnxt->tail->x-vecnxt->tip->x;
	np_del_y=vecnxt->tail->y-vecnxt->tip->y;
	tp_del_x=vtest->x-vecnxt->tip->x;
	tp_del_y=vtest->y-vecnxt->tip->y;
	
	if(((vecnxt->tip->y<=vtest->y) && (vecnxt->tail->y>vtest->y)) || ((vecnxt->tip->y>vtest->y) && (vecnxt->tail->y<=vtest->y)))
	  {
	  if(fabs(np_del_y)>TOLERANCE)
	    {
	    vt = tp_del_y / np_del_y;
	    if( vtest->x < (vecnxt->tip->x + vt*np_del_x) )crossings++;
	    }
	  }
	}
      vecnxt=vecnxt->next;
      if(vecnxt==vec_list)break;
      }
    //if(save_in_new==TRUE){vecptr->new_cross=crossings;}
    //else {vecptr->old_cross=crossings;}
    vecptr=vecptr->next;
    if(vecptr==vec_list)break;
    }
  
  free(vtest);vertex_mem--;
  
  return(1);
}

// Function to determine if a vector crosses any polygons in a slice
// Input vector can be in 3D, but it will only evaluate the XY components of it.
// (in other words, it does not work in 3D)
int vector_crosses_polygon(slice *sinpt, vector *vecA, int ptyp)
{
  int 		state=FALSE;
  int		vint_result;
  polygon	*pptr;
  vertex	*vptr,*vnxt,*vnew;
  vector 	*vecB;
   
  // validate inputs
  if(sinpt==NULL || vecA==NULL)return(FALSE);
  
  // loop thru all polygons in the slice
  vnew=vertex_make();
  pptr=sinpt->pfirst[ptyp];						// start with first polygon in slice of ptyp
  while(pptr!=NULL)							// loop thru all polys in slice
    {
    vptr=pptr->vert_first;						// start with first vtx in poly
    while(vptr!=NULL)							// loop thru all vtxs in poly
      {
      vnxt=vptr->next;							// get address of next vtx
      if(vnxt==NULL)break;						// ensure not a problem
      vecB=vector_make(vptr,vnxt,0);					// make a test vector bt the two
      vint_result=vector_intersect(vecA,vecB,vnew);			// check if test vector crosses input vector
      free(vecB); vector_mem--; vecB=NULL;				// release memory
      if(vint_result==204 || vint_result==206 || vint_result==220 || vint_result==221 || vint_result==222)
        {
	state=TRUE;							// ... set to "they DO intersect"
	break;								// ... no need to test any others
	}	
      vptr=vptr->next;							// get next vtx in poly
      if(vptr==pptr->vert_first)break;					// poly linked lists are circular, make sure not back at start
      }
    if(state==TRUE)break;						// if one was found, no need to test others
    pptr=pptr->next;							// get next polygon
    }
  free(vnew); vertex_mem--;						// free temp intersection location vtx
  
  return(state);
}

// Function to subdivide a vector into two seperate vectors at a specific pt.
//
// Note that the input vertex does not have to lie on the input vector.  In effect, this function changes the
// tail of the input vector to the input vertex and then creates a new vector as the next vector in the linked
// list that with its tip at the input vertex and tail at the input vector's original tail.
int vector_subdivide(vector *vecinpt, vertex *vtxinpt)
{
  vector	*vecptr,*vecnxt,*vecnew,*vecold;
  
  //printf("Vector subdivide:  entry vec_list=%X\n",vec_list);

  if(vecinpt==NULL || vtxinpt==NULL)return(0);				// validate input
  
  vecnew=vector_make(vtxinpt,vecinpt->tail,vecinpt->type);		// create new vector
  vecnxt=vecnew->next;							// get address of vector after input vector
  vecnew->next=vecnxt;							// add forward linkage
  if(vecnxt!=NULL)vecnxt->prev=vecnew;					// add backward linkage if appropriate
  vecnew->prev=vecinpt;							// add backward linkage
  vecinpt->tail=vtxinpt;						// assign new tail
  vecinpt->next=vecnew;							// add linkage
  
  //printf("Vector subdivide: exit\n");
  return(1);
}
 
// Function to determine the bisector DIRECTION between two vectors that may, or may not, touch.
// it returns a vertex with x and y providing direction.  by default, magnitude is one (unit).
// note this is a "true vector" in the sense that it only gives direction.  it has no origin.
int vector_bisector_get_direction(vector *vA, vector *vB, vertex *vnew)
{
  int		vint_result;
  double 	dxA,dyA,magvecA,dxB,dyB,magvecB,swapx,swapy;
  vertex	*vint,*vAend,*vBend,*vtxA,*vtxB;
  vector 	*vecA,*vecB;
  
  // validate inputs
  if(vA==NULL || vB==NULL || vnew==NULL)return(FALSE);

  // ensure we don't have a zero length situation
  if(vector_compare(vA,vB,TOLERANCE)==TRUE)return(FALSE);

  // init
  vint=vertex_make();
  vtxA=vertex_make();
  vtxB=vertex_make();

  // find the intersection point bt the source vectors
  vint_result=vector_intersect(vA,vB,vint);				// vint_result tells us how they intersect
  vint->z=vA->tip->z;							// ensure z at same level

  // handle special case when vectors are parallel, so the bisector is either parallel or perpendicular to them.
  // if parallel with equal distance between the endpoints (i.e. there would be no intersection even if infinitely long),
  // then the bisector is parallel to them.  if the parallel lines are colinear, even if endpoints do not touch, 
  // then the bisector is perpendicular to them.
  if(vint_result>=255)
    {
    // normalize vA (could also use vB... either one here)
    dxA=vA->tip->x-vA->tail->x;
    dyA=vA->tip->y-vA->tail->y;
    magvecA=sqrt(dxA*dxA+dyA*dyA);
    if(magvecA<CLOSE_ENOUGH)magvecA=CLOSE_ENOUGH;
    vtxA->x=dxA/magvecA/2;	
    vtxA->y=dyA/magvecA/2;
    vnew->x=vtxA->x;
    vnew->y=vtxA->y;
    vnew->z=vA->tip->z;
    
    // if colinear, then we want the negative recipocal which is perpendicular
    if(vint_result>=256)
      {
      vnew->x=vtxA->y;							// swap both values ...
      vnew->y=(-1)*vtxA->x;						// ... but only negate one.
      // unless perfectly vertical (i.e. no x value)
      if(fabs(dxA)<CLOSE_ENOUGH && dyA>0){vnew->x=1.0;vnew->y=0.0;}
      if(fabs(dxA)<CLOSE_ENOUGH && dyA<0){vnew->x=(-1.0);vnew->y=0.0;}
      }
    }

  // now create unit vector from intersection pt to average of vector ends away from intersection pt.
  else 
    {
    // check if the intersection point matches a vector vtx - common when vectors are sorted tip-to-tail
    // if so, we need to use the other end of the vector so it has enough magnitude to define direction
    vAend=vA->tail;
    if(vertex_compare(vint,vAend,CLOSE_ENOUGH)==TRUE)vAend=vA->tip;
    vBend=vB->tip;
    if(vertex_compare(vint,vBend,CLOSE_ENOUGH)==TRUE)vBend=vB->tail;
    
    // normalize vector A which is now from vAend to intersection pt.
    dxA=vAend->x-vint->x;
    dyA=vAend->y-vint->y;
    magvecA=sqrt(dxA*dxA+dyA*dyA);
    if(magvecA<CLOSE_ENOUGH)magvecA=CLOSE_ENOUGH;
    vtxA->x=dxA/magvecA;	
    vtxA->y=dyA/magvecA;
    
    // normalize vector B which is now from vBend to intersection pt.
    dxB=vBend->x-vint->x;
    dyB=vBend->y-vint->y;
    magvecB=sqrt(dxB*dxB+dyB*dyB);
    if(magvecB<CLOSE_ENOUGH)magvecB=CLOSE_ENOUGH;
    vtxB->x=dxB/magvecB;	
    vtxB->y=dyB/magvecB;
    
    // create direction of the bisector vector by adding the two unit vectors and averaging the result
    vnew->x=(vtxA->x+vtxB->x)/2;
    vnew->y=(vtxA->y+vtxB->y)/2;
    vnew->z=vA->tail->z;
    }

  // free the temporary intersection vertex
  free(vint);vertex_mem--;
  free(vtxA);vertex_mem--;
  free(vtxB);vertex_mem--;

  return(vint_result);
}
  
// Function to SET the bisector LENGTH by the value of newdist based on parent vector angle
// note that the vec->curlen value MUST be available.  it is set by the vector_magnitude function.
int vector_bisector_set_length(vector *vA, float newdist)
{
  float		odist;
  double 	dx,dy,mag,alpha,sa;
  
  // validate input
  if(vA==NULL)return(FALSE);
  
  // get existing magnitude of input vector
  dx=vA->tip->x-vA->tail->x;
  dy=vA->tip->y-vA->tail->y;
  vector_magnitude(vA);							// this also sets vA->curlen
  mag=vA->curlen;
  if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;
  
  // determine new length of bisector 
  // need to do this because we are trying to move the wavefront the input value, but our
  // bisectors are at angles to the wavefront, so they must be further tweeked
  alpha=vector_relangle(vA,vA->vsrcA);					// get angle bt bisector and parent
  sa=1;
  if(alpha>=0 && alpha<=PI)sa=sin(alpha);				// apply trig
  if(sa>=0 && sa<(newdist/10))sa=newdist/10;
  if(sa<0 && sa>(newdist/(-10)))sa=newdist/(-10);
  vA->tip->k=sa;
  odist=newdist;
  if(fabs(sa)>CLOSE_ENOUGH)odist=newdist/sa;				// adjust offset distance
  if(odist<newdist)odist=newdist;
  if(odist>(4*newdist))odist=newdist;
  vA->tip->x=vA->tail->x+odist*dx/mag;					// set new length
  vA->tip->y=vA->tail->y+odist*dy/mag;
  vA->curlen=odist;

  return(TRUE);
}
  
// Function to INCREMENT the bisector LENGTH by the value of newdist based on parent vector angle
int vector_bisector_inc_length(vector *vA, float incdist)
{
  float		odist;
  double 	dx,dy,mag,alpha,sa;
  
  // validate input
  if(vA==NULL)return(FALSE);
  
  // get existing magnitude of input vector
  dx=vA->tip->x-vA->tail->x;
  dy=vA->tip->y-vA->tail->y;
  vector_magnitude(vA);
  mag=vA->curlen;
  if(mag<CLOSE_ENOUGH)mag=CLOSE_ENOUGH;

  // determine new length of bisector 
  // need to do this because we are trying to move the wavefront the input value, but our
  // bisectors are at angles to the wavefront, so they must be further tweeked
  alpha=vector_relangle(vA,vA->vsrcA);					// get angle bt bisector and parent
  sa=1;
  if(alpha>=(-PI) && alpha<=PI)sa=sin(alpha);				// apply trig
  if(sa>=0 && sa<(incdist/10))sa=(incdist/10);
  if(sa<0 && sa>(incdist/(-10)))sa=(incdist/(-10));
  odist=incdist;
  if(fabs(sa)>CLOSE_ENOUGH)odist=incdist/sa;				// adjust offset distance
  if(odist<incdist)odist=incdist;
  if(odist>(4*incdist))odist=incdist;					// vectors near parallel
  vA->tip->x=vA->tail->x+(vA->curlen+odist)*dx/mag;			// set new length
  vA->tip->y=vA->tail->y+(vA->curlen+odist)*dy/mag;
  vA->curlen=mag+odist;

  return(TRUE);
}

// Function to extract a single polygon out of the straight skeleton vector list.
polygon *ss_veclist_2_single_polygon(vector *vec_list, int ptyp)
{
  int		i,add_vtx;
  vertex 	*vptr,*vold,*vtip,*vtail;
  vector	*vecptr,*vecold,*vecdel;
  polygon	*pnew,*psource;
  
  // validate input
  if(vec_list==NULL)return(NULL);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(NULL);
  
  if(debug_flag==68)printf("SS_VEC2POLY:  entry\n");

  // scan vec list for first un-used vector to start polygon with.  if none, return NULL
  // this routine will only connect vectors with status==1
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    if(vecptr->status==1)break;
    vecptr=vecptr->next;
    }
  if(vecptr==NULL)return(NULL);
  if(debug_flag==68)printf("SS_VEC2POLY:  first vector found  %X\n",vecptr);
  
  // copy the tip of the first vector in the list to a new polygon
  vecptr->status=3;							// set status as used
  psource=vecptr->psrc;							// define source polygon
  vptr=vertex_copy(vecptr->tip, NULL);					// copy first vtx
  pnew=polygon_make(vptr,ptyp,0);					// create new polygon
  if(pnew==NULL)return(NULL);
  pnew->vert_qty=1;
  vold=vptr;								// save ptr to last vtx
  if(debug_flag==68)printf("SS_VEC2POLY:  first vtx %X  qty=%d  pnew=%X\n",vptr,pnew->vert_qty,pnew);
  
  // loop thru vector list adding a copy of the next vector's tip vtx until the polygon source changes
  // note that this is relying upon the ss vectors being in sequencial order as derived from the
  // perimeter order.
  while(vecptr!=NULL)
    {
    // if different source polygon...
    if(vecptr->psrc!=psource)break;

    // ensure the new vtx is sufficiently spaced from prev vtx
    //if(vertex_distance(vold,vecptr->tip)>0.1)
    
    if(vecptr->status==1)
      {
      add_vtx=TRUE;
      // if the polygon contains material, but the new point is not inside...
      //if(psource->hole==FALSE && polygon_contains_point(psource,vecptr->tip)==FALSE)add_vtx=FALSE;
      //if(psource->hole==TRUE && polygon_contains_point(psource,vecptr->tip)==TRUE)add_vtx=FALSE;
      if(add_vtx==TRUE)
        {
	vptr=vertex_copy(vecptr->tip,NULL);
	vptr->attr=psource->ID;
	vold->next=vptr;
	vold=vptr;
	pnew->vert_qty++;
	if(debug_flag==68)printf("SS_VEC2POLY:  vtx add  %X  %d\n",vptr,pnew->vert_qty);
	}
      vecptr->status=3;
      }
    vecptr=vecptr->next;
    //if(vecptr==vec_list)break;
    }

  // close the polygon
  if(vold!=NULL)vold->next=pnew->vert_first;
  
  // copy over source polygon's attributes to the new polygon
  pnew->member=psource->member;
  pnew->mdl_num=psource->mdl_num;
  pnew->type=psource->type;
  pnew->hole=psource->hole;
  if(polygon_verify(pnew)==FALSE)
    {
    //pnew->dist=99;
    //polygon_purge(pnew,pnew->dist);					// ... delete polygon and its contents
    //pnew=NULL;
    }
  if(debug_flag==68 && pnew==NULL)printf("SS_VEC2POLY:  exit  pnew=NULL \n");
  if(debug_flag==68 && pnew!=NULL)printf("SS_VEC2POLY:  exit  pnew=%X  vqty=%d  vfst=%X\n",pnew,pnew->vert_qty,pnew->vert_first);
  
  return(pnew);
}

// Function to generate a single polygon out of a singular circular vector list.  This function is
// dependent upon a SORTED vector list input!  The input vector list remains unaffected.  The resulting
// polygons are made from COPIES of the vector vtxs.
polygon *veclist_2_single_polygon(vector *vec_first, int ptyp)
{
  int		i;
  vertex 	*vptr,*vold,*vtip,*vtail;
  vector	*vecptr,*vecold,*vecdel;
  polygon	*pnew;
  
  // validate input
  if(debug_flag==201)printf("    veclist_2_single_polygon:  entry   %X  %d \n",vec_first,ptyp);
  if(vec_first==NULL)return(NULL);
  if(ptyp<0 || ptyp>MAX_LINE_TYPES)return(NULL);
  
  // copy the tip of the first vector in the list to a new polygon
  vecptr=vec_first;
  vptr=vertex_copy(vecptr->tip, NULL);					// copy tip of un-spent vector to nex vtx
  pnew=polygon_make(vptr,ptyp,0);					// make new poly from tip vtx
  pnew->vert_qty=1;							// set qty
  if(vecptr->member>=0)pnew->member=vecptr->member;			// set poly to match vector source
  vold=vptr;								// save address of this vtx
  
  // loop thru vector list adding a copy of the next vector's tail vtx until end points meet
  while(vecptr!=NULL)
    {
    // check if back at start of this polygon.  if so, we are done.
    if(vecptr->next==vec_first)break;

    // if not back as start, add the tail of this vector to the polygon's vtx list
    vptr=vertex_copy(vecptr->tail,NULL);				// ... make copy of its tail
    
    vold->next=vptr;							// ... add link from previous vtx
    vold=vptr;								// ... save address of this vtx
    pnew->vert_qty++;							// ... increment qty of vtxs in this poly
    vecptr->psrc=pnew;							// ... backwards assign this polygon to this vector
    vecptr->status=2;							// ... mark vector as spent
      
    vecptr=vecptr->next;
    }

  if(vold!=NULL)vold->next=pnew->vert_first;				// close the new polygon
  
  if(debug_flag==201)printf("    veclist_2_single_polygon:  exit  pnew=%X  qty=%d \n",pnew,pnew->vert_qty);
  return(pnew);
}

/*
// Function to close small gaps in vector loops.  the input vector list should already be sorted.
// this function just makes sure endpoints within the tolerance are adjusted to have the same values.
int veclist_close_gaps(vector *vec_list, float tolerance)
{
  float 	dx,dy,dist,min_dist;
  vector 	*vecptr,*vecnxt;
  
  if(vec_list==NULL)return(0);
  
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    vecnxt=vecptr->next;
    if(vertex_compare(vecptr->tail,vecnxt->tip,tolerance)==FALSE)
      {
      if(vertex_distance(vecptr->tail,vecnxt->tip)<tolerance)
        {
	vecnxt->tip->x=vecptr->tail->x;
	vecnxt->tip->y=vecptr->tail->y;
	vecnxt->tip->z=vecptr->tail->z;
	}
      }
    vecptr=vecptr->next;
    }
  
  return(1);
}
*/

// Function to determine a particular vector's winding number as it exists in a set of vectors.
// note this is highly dependent on the direction of the vector.  in other words, tip/tail assignment
// is critical for this function to work properly.
int vector_winding_number(vector *vec_list, vector *vec_inpt)
{
  int		win_res;
  int		valid,win_xmin,win_xmax,win_ymin,win_ymax;
  float 	dx,dy,dx_inpt,dy_inpt,dxn,dyn,dxp,dyp;
  vertex 	*vtxint,*vtxctr,*vtx0,*vtx1,*vtx2,*vtx3;
  vector 	*vecptr,*vecpre,*vecnxt,*vec_xmin,*vec_xmax,*vec_ymin,*vec_ymax;
  
  // validate inputs
  if(vec_list==NULL || vec_inpt==NULL)return(0);
  
  // use 4 independent vectors from the center of vec_inpt to each extreme in X,-X,Y,-Y to
  // determine winding number.  evaluate 2 of 4 at end of loop to arrive at consensus.
  vtxint=vertex_make();
  
  vtxctr=vertex_make();
  vtxctr->x=(vec_inpt->tip->x + vec_inpt->tail->x)/2;
  vtxctr->y=(vec_inpt->tip->y + vec_inpt->tail->y)/2;
  vtxctr->z=vec_inpt->tip->z;
  
  vtx0=vertex_make();							// vector to minimum x from vec_inpt center
  vtx0->x=(-10);
  vtx0->y=vtxctr->y;
  vtx0->z=vec_inpt->tip->z;
  vec_xmin=vector_make(vtxctr,vtx0,0);
  vec_xmin->type=vec_inpt->type;
  
  vtx1=vertex_make();							// vector to maxium x from vec_inpt center
  vtx1->x=BUILD_TABLE_LEN_X;
  vtx1->y=vtxctr->y;
  vtx1->z=vec_inpt->tip->z;
  vec_xmax=vector_make(vtxctr,vtx1,0);
  vec_xmax->type=vec_inpt->type;
  
  vtx2=vertex_make();							// vector to minimum y from vec_inpt center
  vtx2->x=vtxctr->x;
  vtx2->y=(-10);
  vtx2->z=vec_inpt->tip->z;
  vec_ymin=vector_make(vtxctr,vtx2,0);
  vec_ymin->type=vec_inpt->type;
  
  vtx3=vertex_make();							// vector to maximum y from vec_inpt center
  vtx3->x=vtxctr->x;
  vtx3->y=BUILD_TABLE_LEN_Y;
  vtx3->z=vec_inpt->tip->z;
  vec_ymax=vector_make(vtxctr,vtx3,0);
  vec_ymax->type=vec_inpt->type;
  
  dx_inpt=vec_inpt->tip->x - vec_inpt->tail->x;
  dy_inpt=vec_inpt->tip->y - vec_inpt->tail->y;
  
  // this part of the function is a little bit like polygon_contains_point function in that we
  // use vtx position compares to determine if each vector is pointing up/down or left/right.
  win_xmin=0; win_xmax=0;
  win_ymin=0; win_ymax=0;
  vecptr=vec_list;
  while(vecptr!=NULL)
    {
    if(vecptr->type!=vec_inpt->type){vecptr=vecptr->next;continue;}
    if(vecptr!=vec_inpt)
      {
      dx=vecptr->tip->x - vecptr->tail->x;
      dy=vecptr->tip->y - vecptr->tail->y;
      if(fabs(dy_inpt)>fabs(dx_inpt))					// if more of a vertical line, check using x crossings...
        {
	// check xmin direction
	{
	win_res=vector_intersect(vec_xmin,vecptr,vtxint);			
	if(win_res==204)						// if pure crossing... 
	  {		
	  if(dy<0)win_xmin++; 						// ... if down increment
	  if(dy>0)win_xmin--;						// ... if up decrement
	  }
	//if(win_res==236)						// if on end pt ...
	if(win_res==206 || win_res==236 || win_res==238)		// if on end pt ...
	// note we have to check for intersection at ends of vecA due to how close some of the offsets are
	// to other vectors hence checking 236 and 238 types of intersections.  see vector_intersect function for more details.
	  {
	  dyp=0;
	  vecpre=vecptr->prev;						// ... get prev vec, which may or may not be location neighbor
	  if(vecpre!=NULL)						// ... if it exists
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... check if location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... if not, find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyp)<TOLERANCE)dyp=0;					// ... make sure not near horizonal
	  if(dy<0 && dyp<0)win_xmin++;					// ... if both going down then increment
	  if(dy>0 && dyp>0)win_xmin--;					// ... if both going up then decrement
	  }
	if(win_res>257)							// if vec is colinear...
	  {
	  dyp=0; dyn=0;
	  vecpre=vecptr->prev;						// ... get prev vec
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyp)<TOLERANCE)dyp=0;					// ... make sure not near horizonal
	  vecnxt=vecptr->next;						// ... get next vec
	  if(vecnxt!=NULL)
	    {
	    if(vertex_compare(vecptr->tail,vecnxt->tip,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecnxt=vector_find_neighbor(vec_list,vecptr,1);		// ... find neighbor that matches the tail of vecptr
	     }
	    if(vecnxt!=NULL)dyn=vecnxt->tip->y - vecnxt->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyn)<TOLERANCE)dyn=0;					// ... make sure not near horizonal
	  if(dyn<0 && dyp<0)win_xmin++;					// ... if both going down then increment
	  if(dyn>0 && dyp>0)win_xmin--;					// ... if both going up then decrement
	  }
	}
	// check xmax direction
	{
	win_res=vector_intersect(vec_xmax,vecptr,vtxint);			
	if(win_res==204)
	  {
	  if(dy>0)win_xmax++; 
	  if(dy<0)win_xmax--;
	  }
	if(win_res==206|| win_res==236 || win_res==238)
	//if(win_res==236)
	  {
	  dyp=0;
	  vecpre=vecptr->prev;
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyp)<TOLERANCE)dyp=0;
	  if(dy>0 && dyp>0)win_xmax++;
	  if(dy<0 && dyp<0)win_xmax--;
	  }
	if(win_res>257)							// if vec is colinear...
	  {
	  dyp=0; dyn=0;
	  vecpre=vecptr->prev;						// ... get prev vec
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dyp=vecpre->tip->y - vecpre->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyp)<TOLERANCE)dyp=0;
	  vecnxt=vecptr->next;						// ... get next vec
	  if(vecnxt!=NULL)
	    {
	    if(vertex_compare(vecptr->tail,vecnxt->tip,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecnxt=vector_find_neighbor(vec_list,vecptr,1);		// ... find neighbor that matches the tail of vecptr
	      }
	    if(vecnxt!=NULL)dyn=vecnxt->tip->y - vecnxt->tail->y;	// ... get direction of prev vec
	    }
	  if(fabs(dyn)<TOLERANCE)dyn=0;
	  if(dyn>0 && dyp>0)win_xmax++;					// ... if both going up then increment
	  if(dyn<0 && dyp<0)win_xmax--;					// ... if both going down then decrement
	  }
	}
	}
      else 								// otherwise, is more of a horizontal so use y direction instead
        {
	// check ymin direction
	{
	win_res=vector_intersect(vec_ymin,vecptr,vtxint);			
	if(win_res==204)
	  {
	  if(dx<0)win_ymin--; 
	  if(dx>0)win_ymin++;
	  }
	if(win_res==206 || win_res==236 || win_res==238)
	//if(win_res==236)
	  {
	  dxp=0;
	  vecpre=vecptr->prev;
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dxp=vecpre->tip->x - vecpre->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxp)<TOLERANCE)dxp=0;
	  if(dx<0 && dxp<0)win_ymin--;
	  if(dx>0 && dxp>0)win_ymin++;
	  }
	if(win_res>257)							// if vec is colinear...
	  {
	  dxp=0; dxn=0;
	  vecpre=vecptr->prev;						// ... get prev vec
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dxp=vecpre->tip->x - vecpre->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxp)<TOLERANCE)dxp=0;
	  vecnxt=vecptr->next;						// ... get next vec
	  if(vecnxt!=NULL)
	    {
	    if(vertex_compare(vecptr->tail,vecnxt->tip,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecnxt=vector_find_neighbor(vec_list,vecptr,1);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecnxt!=NULL)dxn=vecnxt->tip->x - vecnxt->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxn)<TOLERANCE)dxn=0;
	  if(dxn<0 && dxp<0)win_ymin--;					// ... if both going left then decrement
	  if(dxn>0 && dxp>0)win_ymin++;					// ... if both going right then increment
	  }
	}
	// check ymax direction
	{
	win_res=vector_intersect(vec_ymax,vecptr,vtxint);			
	if(win_res==204)						// if pure crossing... 
	  {
	  if(dx>0)win_ymax--; 						// ... if right decrement
	  if(dx<0)win_ymax++;						// ... if left increment
	  }
	if(win_res==206 || win_res==236 || win_res==238)		// if endpt crossing...
	//if(win_res==236)						// if endpt crossing...
	  {
	  dxp=0;
	  vecpre=vecptr->prev;
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dxp=vecpre->tip->x - vecpre->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxp)<TOLERANCE)dxp=0;					// ... if close to vertical, make vertical
	  if(dx>0 && dxp>0)win_ymax--;					// ... if both right, decrement
	  if(dx<0 && dxp<0)win_ymax++;					// ... if both left, increment
	  }
	if(win_res>257)							// if colinear...
	  {
	  dxp=0; dxn=0;
	  vecpre=vecptr->prev;
	  if(vecpre!=NULL)
	    {
	    if(vertex_compare(vecptr->tip,vecpre->tail,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecpre=vector_find_neighbor(vec_list,vecptr,0);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecpre!=NULL)dxp=vecpre->tip->x - vecpre->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxp)<TOLERANCE)dxp=0;					// ... if close to vertical, make vertical
	  vecnxt=vecptr->next;
	  if(vecnxt!=NULL)
	    {
	    if(vertex_compare(vecptr->tail,vecnxt->tip,TOLERANCE)!=TRUE) // ... possible not actual location neighbors
	      {
	      vecnxt=vector_find_neighbor(vec_list,vecptr,1);		// ... find neighbor that matches the tip of vecptr
	      }
	    if(vecnxt!=NULL)dxn=vecnxt->tip->x - vecnxt->tail->x;	// ... get direction of prev vec
	    }
	  if(fabs(dxn)<TOLERANCE)dxn=0;					// ... if close to vertical, make vertical
	  if(dxn>0 && dxp>0)win_ymax--;					// ... if both right, decrement
	  if(dxn<0 && dxp<0)win_ymax++;					// ... if both left, increment
	  }
	}
	}
      }
    vecptr=vecptr->next;
    }
  
  // process results
  valid=(-1);								// default to invalid vector
  if(fabs(dy_inpt)>fabs(dx_inpt))					// if input vec has more y delta
    {
    vec_inpt->wind_dir=0;						// set as Y direction check
    vec_inpt->wind_min=win_xmin;
    vec_inpt->wind_max=win_xmax;
    vec_inpt->crmin=win_xmin;
    vec_inpt->crmax=win_xmax;
    if(dy_inpt>0 && win_xmin==1 && win_xmax==0)valid=1;			// valid if pntg down and left is no mat'l and right is mat'l
    if(dy_inpt<0 && win_xmin==0 && win_xmax==1)valid=1;			// valid if pntg up and left is mat'l and right is no mat'l
    
    if(fabs(dx_inpt)<TOLERANCE && dy_inpt>0 && win_xmax==0)valid=1;	// valid if vertical and pntg down and nothing to left
    if(fabs(dx_inpt)<TOLERANCE && dy_inpt<0 && win_xmin==0)valid=1;	// valid if vertical and pntg up and nothing to right
    }
  else 									// otherwise, it must have more x delta
    {
    vec_inpt->wind_dir=1;						// set as X direction check
    vec_inpt->wind_min=win_ymin;
    vec_inpt->wind_max=win_ymax;
    vec_inpt->crmin=100+win_ymin;
    vec_inpt->crmax=100+win_ymax;
    if(dx_inpt<0 && win_ymin==1 && win_ymax==0)valid=1;			// valid if pntg left and down is mat'l and up is no mat'l
    if(dx_inpt>0 && win_ymin==0 && win_ymax==1)valid=1;			// valid if pntg right and down is no mat'l and up is mat'l

    if(fabs(dy_inpt)<TOLERANCE && dx_inpt>0 && win_ymax==0)valid=1;	// valid if horizontal and pntg right and nothing above
    if(fabs(dy_inpt)<TOLERANCE && dx_inpt<0 && win_ymin==0)valid=1;	// valid if horizontal and pntg left and nothing below
    }
  
  // clean up before leaving
  free(vtxint); vertex_mem--; vtxint=NULL;
  free(vtxctr); vertex_mem--; vtxctr=NULL;
  free(vtx0); vertex_mem--; vtxctr=NULL;
  free(vtx1); vertex_mem--; vtxctr=NULL;
  free(vtx2); vertex_mem--; vtxctr=NULL;
  free(vtx3); vertex_mem--; vtxctr=NULL;
  free(vec_xmin); vector_mem--; vec_xmin=NULL;
  free(vec_xmax); vector_mem--; vec_xmax=NULL;
  free(vec_ymin); vector_mem--; vec_ymin=NULL;
  free(vec_ymax); vector_mem--; vec_ymax=NULL;
  
  return(valid);
}

/*
// Function to determine if a vector is valid or not by counting crossings and the vector's direction.
// Inputs:  vec_list=pointer to head node of linked list of vectors to test against,  vec_inpt=test vector
// Return:  FALSE(0)=Invalid  TRUE(1)=Valid  (-1)=Error
//
// This algorithm is an extension of the point in polygon algorithm 
// credited to: https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
//
// Unlike point in polygon, this function relies on the input vectors all being sorted and organized into
// CW or CCW loops to determine if the vector now points the wrong way and is therefore invalid.  This 
// organization is what makes it more of a winding number algorithm rather than a crossing algorithm.
int vector_winding_number(vector *vec_list, vector *vec_inpt)
{
  int		win_xmax,win_xmin,valid;
  float 	dx_inpt,dy_inpt,dx,dy;
  float 	tolerance;
  vertex	*vptr,*vnxt,*vtest; 
  vector 	*vecptr;
  
  // Verify inbound data
  if(vec_list==NULL || vec_inpt==NULL)return(-1);
  
  tolerance=TOLERANCE;
  
  // set test vtx as mid pt of input vector
  vtest=vertex_make();
  vtest->x=(vec_inpt->tip->x+vec_inpt->tail->x)/2;
  vtest->y=(vec_inpt->tip->y+vec_inpt->tail->y)/2;
  vtest->z=(vec_inpt->tip->z+vec_inpt->tail->z)/2;
  dx_inpt=vec_inpt->tip->x - vec_inpt->tail->x;
  dy_inpt=vec_inpt->tip->y - vec_inpt->tail->y;
  
  // loop through all vectors of the in bound list
  win_xmin=FALSE;
  win_xmax=FALSE;
  vecptr=vec_list;							// start with first vec in list
  while(vecptr!=NULL)							// loop thru entire list
    {
    if(vecptr==vec_inpt){vecptr=vecptr->next;continue;}
    vptr=vecptr->tip;
    vnxt=vecptr->tail;							// get vtxs
    if(vptr==NULL || vnxt==NULL)break;					// make sure they exist
    dx=vecptr->tip->x - vecptr->tail->x;
    dy=vecptr->tip->y - vecptr->tail->y;

    // In words:  if one of the edge pts is above the test pt and the other is below, or vice versa, and
    // test pt x is left/right of the start of the edge in x plus the slope of the edge in x up to the test pt y,
    // then the ray to the left/right from the test pt must cross over that edge, so toggle the crossing counter.
    if((vptr->y>vtest->y)!=(vnxt->y>vtest->y))
      {
      // The magic test based on ray tracing to RIGHT and counting crossings - see website for details.
      if((vtest->x<(vnxt->x-vptr->x)*(vtest->y-vptr->y)/(vnxt->y-vptr->y)+vptr->x))
	{
	if(dy>0)win_xmax++;						// if vec going down... then moving into mat'l
	if(dy<0)win_xmax--;						// if vec going up... then moving out of mat'l
	}
  
      // the magic test based on ray tracing to LEFT and counting crossings - see website for details
      if((vtest->x>(vnxt->x-vptr->x)*(vtest->y-vptr->y)/(vnxt->y-vptr->y)+vptr->x))
	{
	if(dy<0)win_xmin++;
	if(dy>0)win_xmin--;
	}
      }

    vecptr=vecptr->next;
    if(vecptr==vec_list)break;
    }

  valid=(-1);								// default to invalid vector
  vec_inpt->crmin=win_xmin;
  vec_inpt->crmax=win_xmax;
  if(dy_inpt>0 && win_xmin==1 && win_xmax==0)valid=1;			// valid if pntg down and left is no mat'l and right is mat'l
  if(dy_inpt<0 && win_xmin==0 && win_xmax==1)valid=1;			// valid if pntg up and left is mat'l and right is no mat'l
    
  return(valid);
}
*/


// Function to find a vector neighbor by location
// Inputs:  veclist=vector list to search, vecinpt=input vector, endpt: 0=TIP, 1=TAIL
// Return:  pointer to the neighboring vector, NULL if none within tolerance
vector *vector_find_neighbor(vector *veclist, vector *vecinpt, int endpt)
{
  vertex	*vtxinpt;
  vector 	*vecptr;
  
  if(veclist==NULL || vecinpt==NULL)return(NULL);
  if(endpt<0 || endpt>1)return(NULL);
  
  vtxinpt=NULL;
  if(endpt==0)vtxinpt=vecinpt->tip;
  if(endpt==1)vtxinpt=vecinpt->tail;
  
  vecptr=veclist;
  while(veclist!=NULL)
    {
    if(vecptr!=vecinpt)
      {
      if(endpt==1 && vertex_compare(vtxinpt,vecptr->tip,TOLERANCE)==TRUE)break;
      if(endpt==0 && vertex_compare(vtxinpt,vecptr->tail,TOLERANCE)==TRUE)break;
      }
    vecptr=vecptr->next;
    if(vecptr==veclist){vecptr=NULL;break;}
    }

  return(vecptr);
}


// Function to delete duplicate vectors from a vector list
int vector_duplicate_delete(vector *veclist, float tolerance)
{
  int		delqty=0;
  vector 	*vecptr,*vecnxt,*vecdel;
  
  if(veclist==NULL)return(-1);
  
  vecptr=veclist;
  while(vecptr!=NULL)
    {
    vecnxt=vecptr->next;
    while(vecnxt!=NULL)
      {
      if(vector_compare(vecptr,vecnxt,tolerance)==TRUE)
        {
	//printf(" delete \n");
	if(vecnxt==veclist)veclist=vecnxt->next;
	if(vecnxt->prev!=NULL)(vecnxt->prev)->next=vecnxt->next;	// adjust list to skip over this vector going forward
	if(vecnxt->next!=NULL)(vecnxt->next)->prev=vecnxt->prev;	// adjust list to skip over this vector going backward
	
	vecdel=vecnxt;
	vecnxt=vecnxt->next;
	free(vecdel->tip);  vertex_mem--; vecdel->tip=NULL;
	free(vecdel->tail); vertex_mem--; vecdel->tail=NULL;
	free(vecdel); vector_mem--; vecdel=NULL;
	delqty++;
	
	if(vecnxt==NULL)break;
	//printf(" vecnxt=%X  prev=%X  next=%X   tip=%X tail=%X    new\n",vecnxt,vecnxt->prev,vecnxt->next,vecnxt->tip,vecnxt->tail);
	}
      else 
        {
	//printf(" next\n");
	vecnxt=vecnxt->next;
	}
      }
    vecptr=vecptr->next;
    }
  
  return(delqty);
}

// Function to calculate the intersection point of two vectors in 2D space (i.e. z values ignored)
// input:  pointers to the two vectors and pointer to vertex where to store intersection coordinates
// return: 	intvtx is loaded with the xyz coords of the intersection
//		return value is the intersection type and encoded in bits of the rval.  
//		the first (low/right) 4 bits pertain to vecA, next 4 bits to vecB
//		in each group of 4 bits, the left most bit indicates if the vector is skew
//		next bit indicates if the intersection lands on the vector
//		next bit indicates if the intersection happens to land on the tip
//		next bit indicates if the intersection happens to land on the tail
//		after "255", integer values are used to represent overlap types
//
//		Some examples:
//		Binary		Decimal		Meaning
//		0000 0000  	0		Undefined

//		when vB is horizontal and vA is skew
//		0000 1000
//		.... 1...	N		Many other cases, but can be "cheated" by forcing both skew and using what's below

//		when vB is skew and vA is horizontal
//		1000 0000
//		1... ....	N		Many other cases, but can be "cheated" by forcing both skew and using what's below

//		examples for reference... but not a complete list
//		1000 1000 	136		Both skew, but may be parallel, no intersection within either vector segment
//		1000 1100	140		Intersection within vecA, but not vecB
//		1000 1101	141		Intersection on the tail of vecA, but not vecB
//		1000 1110	142		Intersection on the tip of vecA, but not vecB
//		1100 1000 	200		Intersection within vecB, but not vecA
//		1100 1100	204		*Intersection within vecB, and within vecA
//		1100 1101	205		Intersection within vecB, and on the tail of vecA
//		1100 1110	206		*Intersection within vecB, and on the tip of vecA
//		1101 1000	216		Intersection on the tail of vecB, but not vecA
//		1101 1100	220		Intersection on the tail of vecB, and within vecA
//		1101 1101	221		Intersection on the tail of vecB, and on the tail of vecA
//		1101 1110	222		Intersection on the tail of vecB, and on the tip of vecA
//		1110 1000	232		Intersection on the tip of vecB, but not vecA
//		1110 1100	236		*Intersection on the tip of vecB, and within vecA
//		1110 1101	237		Intersection on the tip of vecB, and on the tail of vecA
//		1110 1110	238		*Intersection on the tip of vecB, and on the tip of vecA
//
//		Parallel and colinear vectors... more ways two parallel lines can intersect than one can imagine
//		This function tells you how and where they overlap in painful detail.  It is up to the calling function
//		to deal with the results accordingly.
//		1111 1111	255		Parallel - and NOT colinear
//		N/A		256		Colinear - and may or may not overlap
//		N/A		257		Colinear - and NOT overlapping
//		N/A		258		Colinear and end pts overlap - tail of A touching tip of B
//		N/A		259		Colinear and end pts overlap - tip of A touching tip of B
//		N/A		260		Colinear and end pts overlap - tail of A touching tail of B
//		N/A		261		Colinear and end pts overlap - tip of A touching tail of B
//		N/A		262		Colinear and segment overlap - 1st pt tip of A, 2nd pt tail of A
//		N/A		263		Colinear and segment overlap - 1st pt tail of A, 2nd pt tip of A
//		N/A		264		Colinear and segment overlap - 1st pt tip of B, 2nd pt tail of B
//		N/A		265		Colinear and segment overlap - 1st pt tail of B, 2nd pt tip of B
//		N/A		266		Colinear and segment overlap - 1st pt tip of A, 2nd pt tip of B
//		N/A		267		Colinear and segment overlap - 1st pt tip of A, 2nd pt tail of B
//		N/A		268		Colinear and segment overlap - 1st pt tail of A, 2nd pt tip of B
//		N/A		269		Colinear and segment overlap - 1st pt tail of A, 2nd pt tail of B
//		N/A		270		Colinear and segment overlap - 1st pt tip of B, 2nd pt tip of A
//		N/A		271		Colinear and segment overlap - 1st pt tip of B, 2nd pt tail of A
//		N/A		272		Colinear and segment overlap - 1st pt tail of B, 2nd pt tip of A
//		N/A		273		Colinear and segment overlap - 1st pt tail of B, 2nd pt tail of A
//		N/A		274		Colinear and segment overlap - tip of A matches tip of B AND tail of A matches tail of B
//		N/A		275		Colinear and segment overlap - tip of A matches tail of B AND tail of A matches tip of B
//
// the "*" in the descriptions above denote the results that provide a "half edge" combination (includes tips, ignores tails)
// in other words, they include any where on vA except the tail AND any where on vB except the tail.

unsigned int vector_intersect(vector *vA, vector *vB, vertex *intvtx)
{
  unsigned int	rval=0;
  int 		vec[4],vtemp;
  int		i,j;
  double 	px[4],py[4],ptemp;
  double	dx,dy,lenAB,lenA,lenB;
  double 	A_x,A_y,B_x,B_y;
  double	A,B,C,s,t;
  float 	tolerance,vec_angle;
  
  if(vA==NULL || vB==NULL || intvtx==NULL)return(0);
  
  // init
  tolerance=TOLERANCE;
  px[0]=vA->tip->x;  py[0]=vA->tip->y;  vec[0]=0;
  px[1]=vA->tail->x; py[1]=vA->tail->y; vec[1]=1;
  A_x=px[1]-px[0];   A_y=py[1]-py[0];
  
  px[2]=vB->tip->x;  py[2]=vB->tip->y;  vec[2]=2;
  px[3]=vB->tail->x; py[3]=vB->tail->y; vec[3]=3;
  B_x=px[3]-px[2];   B_y=py[3]-py[2];
  
  // test if parallel...
  C=(A_x*B_y)-(A_y*B_x);						// difference in slopes
  if(fabs(C)<tolerance)							// if within desired threshold...
    {
    rval=255;								// set rval as parallel... may or may not be overlapping
    
    // test if colinear...
    // since we know they are parallel, just check if p2 lies on the line made by p0->p1 by
    // comparing slopes of p0->p1 to p0->p2
    A=(py[2]-py[0])*(px[1]-px[0]);
    B=(py[1]-py[0])*(px[2]-px[0]);
    if(fabs(A-B)<CLOSE_ENOUGH)						// if colinear... determine if/how they overlap each other
      {
      // at this point we know the segments are colinear.  we now need to figure out if they
      // overlap, and if they do what those points are understanding that those points would
      // have to be two of the four total points defining the segments.
      //
      // do this by computing the length of each segment independently.  then lexicographically
      // sort the points and compute the overall length.  if the overall length is longer than
      // the sum of the segment lengths they do not overlap.  if the overall length is equal
      // then they share end pts.  if the overall length is shorter, the middle two points tell
      // how much they overlap.
      rval=256;								// set rval as colinear... may or may not overlap

      // get lengths of segments independently
      lenA=vA->curlen;
      if(lenA<tolerance)lenA=vector_magnitude(vA);
      lenB=vB->curlen;
      if(lenB<tolerance)lenB=vector_magnitude(vB);
      
      // sort the four sets of points using X as priority, then Y when X's are equal
      // this should make a staight line of points from lowest X to highest X and any angle including vertical
      for(i=0;i<3;i++)							// sort X values: loop thru entire data set
        {
	for(j=i+1;j<4;j++)						// loop thru remainder of set to find smallest value
	  {
	  if(px[j]>px[i])continue;					// if greater than ...
	  if(px[j]<px[i])						// if less than ...
	    {
	    ptemp=px[i]; px[i]=px[j];  px[j]=ptemp;			// ... swap x values
	    ptemp=py[i]; py[i]=py[j];  py[j]=ptemp;			// ... swap y values
	    vtemp=vec[i];vec[i]=vec[j];vec[j]=vtemp;			// ... swap source vec flag
	    }
	  }
	}
      for(i=0;i<3;i++)							// Sort Y values: loop thru entire data set
        {
	for(j=i+1;j<4;j++)						// loop thru remainder of set to find smallest value
	  {
	  if(fabs(px[j]-px[i])<CLOSE_ENOUGH && py[j]<py[i])		// if X equal and Y is less...
	    {
	    ptemp=px[i]; px[i]=px[j];  px[j]=ptemp;			// ... swap x values (only to cover tolerance diff)
	    ptemp=py[i]; py[i]=py[j];  py[j]=ptemp;			// ... swap y values
	    vtemp=vec[i];vec[i]=vec[j];vec[j]=vtemp;			// ... swap source vec flag
	    }
	  }
	}

      // compute overall length from p0 -> p3
      dx=px[3]-px[0];
      dy=py[3]-py[0];
      lenAB=sqrt(dx*dx+dy*dy);
      
      vtemp=debug_flag;
      //debug_flag=70;
      if(debug_flag==70)
	{
	printf("\nCollinear Vectors:  C=%f  tolerance=%f \n",C,tolerance);
	printf(" vA:  x0=%6.3f y0=%6.3f  x1=%6.3f y1=%6.3f \n",px[0],py[0],px[1],py[1]);
	printf(" vB:  x2=%6.3f y2=%6.3f  x3=%6.3f y3=%6.3f \n",px[2],py[2],px[3],py[3]);
	vec_angle=vector_relangle(vA,vB);
	printf(" A=%f B=%f vec_angle=%f\n",A,B,(vec_angle*180/PI));
	printf(" Overall len=%6.3f  Alen=%6.3f  Blen=%6.3f \n",lenAB,lenA,lenB);
	printf(" p0x=%6.5f p0y=%6.5f \n",px[0],py[0]);
	printf(" p1x=%6.5f p1y=%6.5f \n",px[1],py[1]);
	printf(" p2x=%6.5f p2y=%6.5f \n",px[2],py[2]);
	printf(" p3x=%6.5f p3y=%6.5f \n",px[3],py[3]);
	}
      debug_flag=vtemp;
      
      // compare overall length with sum of independent lengths to determine amount, if any, of overlap
      if(lenAB>(lenA+lenB+tolerance))					// if they do NOT overlap ...
        {
	rval=257;
	}
      if(fabs(lenAB-lenA-lenB)<=tolerance)				// if they share end points there are 6 possibilities...
        {
	intvtx->x=px[1];						// define the return 1st pt as 1st inside pt
	intvtx->y=py[1];
	intvtx->i=px[2];						// define the return 2nd pt as 2nd inside pt
	intvtx->j=py[2];
	if(vertex_compare(vA->tip,vB->tip,tolerance)==TRUE)		// if tip of A matches tip of B...
	  {
	  if(vertex_compare(vA->tail,vB->tail,tolerance)==TRUE)		// ... and tail of A matches tail of B (they are duplicates)
	    {
	    rval=274;
	    return(rval);
	    }
	  }
	if(vertex_compare(vA->tip,vB->tail,tolerance)==TRUE)		// if tip of A matches tail of B...
	  {
	  if(vertex_compare(vA->tail,vB->tip,tolerance)==TRUE)		// ... and tail of A matches tip of B (they are reversed duplicates)
	    {
	    rval=275;
	    return(rval);
	    }
	  }
	if(vec[1]==1 && vec[2]==2)rval=258;				// if tail of A touching tip of B but no other overlap...
	if(vec[1]==0 && vec[2]==2)rval=259;				// if tip of A touching tip of B but no other overlap...
	if(vec[1]==1 && vec[2]==3)rval=260;				// if tail of A touching tail of B but no other overlap...
	if(vec[1]==0 && vec[2]==3)rval=261;				// if tip of A touching tail of B but no other overlap...
	}
      if(lenAB<(lenA+lenB+tolerance))					// they overlap by some amount there are 10 possiblities...
        {
	intvtx->x=px[1];						// define the return 1st pt as 1st inside pt
	intvtx->y=py[1];
	intvtx->i=px[2];						// define the return 2nd pt as 2nd inside pt
	intvtx->j=py[2];
	if(vec[1]==0 && vec[2]==1)rval=262;				// all of A inside B since 1st pt tip of A, 2nd pt tail of A
	if(vec[1]==1 && vec[2]==0)rval=263;				// all of A inside B since 1st pt tail of A, 2nd pt tip of A
	if(vec[1]==2 && vec[2]==3)rval=264;				// all of B inside A since 1st pt tip of B, 2nd pt tail of B
	if(vec[1]==3 && vec[2]==2)rval=265;				// all of B inside A since 1st pt tail of B, 2nd pt tip of B
	if(vec[1]==0 && vec[2]==2)rval=266;				// tip of A overlaps tip of B since 1st "inside" pt is tip of vec A, 2nd from tip of vec B
	if(vec[1]==0 && vec[2]==3)rval=267;				// tip of A overlaps tail of B since 1st "inside" pt is tip of vec A, 2nd from tail of vec B
	if(vec[1]==1 && vec[2]==2)rval=268;				// tail of A overlaps tip of B since 1st "inside" pt is tail of vec A, 2nd from tip of vec B
	if(vec[1]==1 && vec[2]==3)rval=269;				// tail of A overlaps tail of B since 1st "inside" pt is tail of vec A, 2nd from tail of vec B
	if(vec[1]==2 && vec[2]==0)rval=270;				// tip of B overlaps tip of A since 1st "inside" pt is tip of vec B, 2nd from tip of vec A
	if(vec[1]==2 && vec[2]==1)rval=271;				// tip of B overlaps tail of A since 1st "inside" pt is tip of vec B, 2nd from tail of vec A
	if(vec[1]==3 && vec[2]==0)rval=272;				// tail of B overlaps tip of A since 1st "inside" pt is tail of vec B, 2nd from tip of vec A
	if(vec[1]==3 && vec[2]==1)rval=273;				// tail of B overlaps tail of A since 1st "inside" pt is tail of vec B, 2nd from tail of vec A
	}
      
      if(debug_flag==70)printf(" rval=%d  \n",rval);
      }

    return(rval);		
    }

  // calculate intersection point parameters
  // t = % location from p0 on vA therefore, t=0 then t=p0, t=1 then t=p1
  // s = % location from p2 on vB therefore, s=0 then s=p2, s=1 then s=p3
  t = ( B_x * (py[0] - py[2]) - B_y * (px[0] - px[2])) / C;
  s = (-A_y * (px[0] - px[2]) + A_x * (py[0] - py[2])) / C;
  intvtx->x=px[0]+(t*A_x);
  intvtx->y=py[0]+(t*A_y);

  // init return value to nothing
  rval = 0;
/*
  // if an intersection on vA, test if it landed on an endpoint of A
  rval |= 1 << 3;
  if(t>(0-tolerance) && t<(1+tolerance))rval |= 1 << 2;			// set to indicate on vecA
  if(fabs(t)<=tolerance)rval |= 1 << 1;					// on tip of A
  if(fabs(1-t)<=tolerance)rval |= 1 << 0;				// on tail of A

  // if an intersection on vB, test if it landed on an endpoint of B
  rval |= 1 << 7;
  if(s>(0-tolerance) && s<(1+tolerance))rval |= 1 << 6;			// set to indicate on vecB
  if(fabs(s)<=tolerance)rval |= 1 << 5;					// on tip of B
  if(fabs(1-s)<=tolerance)rval |= 1 << 4;				// on tail of B
*/

  // if an intersection on vA, test if it landed on an endpoint of A
  rval |= 1 << 3;
  if(t>=0 && t<=1)rval |= 1 << 2;					// set to indicate on vecA
  if(t==0)rval |= 1 << 1;						// on tip of A
  if(t==1)rval |= 1 << 0;						// on tail of A

  // if an intersection on vB, test if it landed on an endpoint of B
  rval |= 1 << 7;
  if(s>=0 && s<=1)rval |= 1 << 6;					// set to indicate on vecB
  if(s==0)rval |= 1 << 5;						// on tip of B
  if(s==1)rval |= 1 << 4;						// on tail of B

  intvtx->i=t;
  intvtx->j=s;
  intvtx->k=tolerance;
  intvtx->attr=rval;							// load the attribute with rval
  
  // DEBUG
  //if(fabs(vB->tip->x-3.079781)<TOLERANCE && fabs(vB->tip->y-30.807394)<TOLERANCE)
  //  {printf("\nVI - Match Vtx: x=%f y=%f  t=%f  s=%f  rval=%d \n",vB->tip->x,vB->tip->y,t,s,rval);}

  return(rval);
}

// Function to test if two vectors are parallel.
int vector_parallel(vector *vA, vector *vB)
{
  double 	Ax,Ay,Bx,By;						// deltas between vector ends
  double	C;							

  // initialize
  if(vA==NULL || vB==NULL)return(-1);
	
  // calculate
  Ax=vA->tail->x - vA->tip->x;
  Ay=vA->tail->y - vA->tip->y;
  Bx=vB->tail->x - vB->tip->x;
  By=vB->tail->y - vB->tip->y;
  C=(Ax*By)-(Ay*Bx);
	
  // test if parallel
  if(fabs(C)<TIGHTCHECK)return(TRUE);
  
  return(FALSE);
}

// Function to caclculate the magnitude of a 3D vector
double vector_magnitude(vector *vA)
{
  double 	dx,dy,dz,dc;
  
  if(vA==NULL)return(0);
  
  dx = vA->tip->x - vA->tail->x;
  dy = vA->tip->y - vA->tail->y;
  dz = vA->tip->z - vA->tail->z;
  
  dc=sqrt(dx*dx + dy*dy + dz*dz);
  
  vA->curlen=(float)dc;

  return(dc);

}

// Function to calculate the dot product of two vectors in 3D.
// Basic formula:  dot product = a1 * b1 + a2 * b2 + a3 * b3
// if you want a 2D dot product, the input z values should all be zero.
// note that a negative dot product result means vectors are pointing more than 90deg apart in
// opposite directions.
double vector_dotproduct(vector *A, vector *B)
  {
  double 	dxA,dyA,dzA;						// scratch
  double	dxB,dyB,dzB;						// scratch
  double 	dprod=0;
  
  // only process valid requests
  if(A==NULL || B==NULL)return(0);
  if(A->tip==NULL || A->tail==NULL)return(0);
  if(B->tip==NULL || B->tail==NULL)return(0);

  // default condition where A->tip == B->tip
  dxA=A->tail->x-A->tip->x;
  dyA=A->tail->y-A->tip->y;
  dzA=A->tail->z-A->tip->z;
  dxB=B->tail->x-B->tip->x;
  dyB=B->tail->y-B->tip->y;
  dzB=B->tail->z-B->tip->z;
  dprod=((dxA*dxB)+(dyA*dyB)+(dzA*dzB));

  if(vertex_compare(A->tip,B->tail,TOLERANCE)==TRUE)
    {
    dxA=A->tail->x-A->tip->x;
    dyA=A->tail->y-A->tip->y;
    dzA=A->tail->z-A->tip->z;
    dxB=B->tip->x-B->tail->x;
    dyB=B->tip->y-B->tail->y;
    dzB=B->tip->z-B->tail->z;
    dprod=((dxA*dxB)+(dyA*dyB)+(dzA*dzB));
    }
  if(vertex_compare(A->tail,B->tip,TOLERANCE)==TRUE)
    {
    dxA=A->tip->x-A->tail->x;
    dyA=A->tip->y-A->tail->y;
    dzA=A->tip->z-A->tail->z;
    dxB=B->tail->x-B->tip->x;
    dyB=B->tail->y-B->tip->y;
    dzB=B->tail->z-B->tip->z;
    dprod=((dxA*dxB)+(dyA*dyB)+(dzA*dzB));
    }
  if(vertex_compare(A->tail,B->tail,TOLERANCE)==TRUE)
    {
    dxA=A->tip->x-A->tail->x;
    dyA=A->tip->y-A->tail->y;
    dzA=A->tip->z-A->tail->z;
    dxB=B->tip->x-B->tail->x;
    dyB=B->tip->y-B->tail->y;
    dzB=B->tip->z-B->tail->z;
    dprod=((dxA*dxB)+(dyA*dyB)+(dzA*dzB));
    }

  return(dprod);
  }

// Function to calculate the dot product of two vertex in 3D
// Basic formula:  dot product = a1 * b1 + a2 * b2 + a3 * b3
double vertex_dotproduct(vertex *A, vertex *B)
  {
  double 	dprod;
  
  // only process valid requests
  if(A==NULL || B==NULL)return(0);
  
  // calculate magnitude
  dprod=( (A->x*B->x) + (A->y*B->y) + (A->z*B->z) );

  return(dprod);
  }
  
// Function to calculate the cross product of two vertexes
// Return: vptr is loaded with result
// Basic formula:  cross product = (a2 * b3 – a3 * b2) * i + (a1 * b3 – a3 * b1) * j + (a1 * b1 – a2 * b1) * k
int vertex_crossproduct(vertex *A, vertex *B, vertex *vptr)
{

  if(A==NULL || B==NULL || vptr==NULL)return(0);			// make sure we have something to work with

  vptr->x= (A->y*B->z) - (A->z*B->y);
  vptr->y= (A->z*B->x) - (A->x*B->z);
  vptr->z= (A->x*B->y) - (A->y*B->x);

  return(1);
}
  
// Function to calculate the absolute angle of a vector in 2D space relative to x+ axis
// since in 2D the z value of the input vertex is ignored
// return value is angle in radians between -PI/2 and PI/2.  errors produce negative return values out of range - see below.
float vector_absangle(vector *A)
  {
  float 	dx,dy;
  float 	alpha;
  
  // only process valid requests
  if(A==NULL || A->tip==NULL || A->tail==NULL)return(-10);
  
  // calculate vector angle relative to x axis
  dx=A->tail->x - A->tip->x;						// vectors start at tip and end at tail
  dy=A->tail->y - A->tip->y;
  alpha=-PI/2;								// default to pointing down
  if(dy<0)alpha=PI/2;							// reverse if pointing up (recall, display is upside-down)
  if(fabs(dx)>CLOSE_ENOUGH)alpha=atan(dy/dx);				// if delta x available... calc actual angle

  return(alpha);
  }


// Function to calculate the relative angle between two vectors in 3D space.
// if you want a 2D angle, set the z values to zero first.
// valid return is the value of the angle between them between 0 and PI
// the calling routine must then decide if they want that, or its compliment
// failed returns are negative values beyond what acos can legitimately produce.
// note - the vectors do NOT have to share a common end point, they just have to intersect
float vector_relangle(vector *vA, vector *vB)
  {
  double 	dotp;							// dot product of vectors
  double 	arc;
  float 	alpha;							// angles between the vectors

  // only process valid requests
  if(vA==NULL || vB==NULL)return(-1);
  if(vA->tip==NULL || vA->tail==NULL)return(-2);
  if(vB->tip==NULL || vB->tail==NULL)return(-3);
  if(vA->curlen<TIGHTCHECK)return(-4);
  if(vB->curlen<TIGHTCHECK)return(-5);
  
  // calculate angle alpha based on formula: vector_dotproduct(A*B)=magA*magB*cos(alpha)
  dotp=vector_dotproduct(vA,vB);	
  arc=dotp/(vA->curlen*vB->curlen);
  if(arc<=(-1.00)){alpha=PI;}
  else if(arc>=(1.00)){alpha=0;}
  else {alpha=acos(arc);}	

  return(alpha);
  }


// Function to calculate the cross product of two vectors
// Inputs: vector A and vector B
// Return: vptr is loaded with result
// Basic formula:  x=(ay * bz – az * by),   y=(az * bx – ax * bz),   z=(ax * by – ay * bx)
int vector_crossproduct(vector *A, vector *B, vertex *vptr)
  {

  if(A==NULL || B==NULL || vptr==NULL)return(0);		// make sure we have something to work with

  // first determine which endpoint they share tip-tip, tip-tail, tail-tip, or tail-tail then find product
  // do so by comparing addresses of vertices referenced by each edge
  if(vertex_compare(A->tip,B->tip,CLOSE_ENOUGH)==TRUE) 
    {
    vptr->x= (A->tail->y-A->tip->y)*(B->tail->z-B->tip->z) - (A->tail->z-A->tip->z)*(B->tail->y-B->tip->y);
    vptr->y= (A->tail->z-A->tip->z)*(B->tail->x-B->tip->x) - (A->tail->x-A->tip->x)*(B->tail->z-B->tip->z);
    vptr->z= (A->tail->x-A->tip->x)*(B->tail->y-B->tip->y) - (A->tail->y-A->tip->y)*(B->tail->x-B->tip->x);
    return(1);
    }
  if(vertex_compare(A->tip,B->tail,CLOSE_ENOUGH)==TRUE) 
    {
    vptr->x= (A->tail->y-A->tip->y)*(B->tip->z-B->tail->z) - (A->tail->z-A->tip->z)*(B->tip->y-B->tail->y);
    vptr->y= (A->tail->z-A->tip->z)*(B->tip->x-B->tail->x) - (A->tail->x-A->tip->x)*(B->tip->z-B->tail->z);
    vptr->z= (A->tail->x-A->tip->x)*(B->tip->y-B->tail->y) - (A->tail->y-A->tip->y)*(B->tip->x-B->tail->x);
    return(1);
    }
  if(vertex_compare(A->tail,B->tip,CLOSE_ENOUGH)==TRUE) 
    {
    vptr->x= (A->tip->y-A->tail->y)*(B->tail->z-B->tip->z) - (A->tip->z-A->tail->z)*(B->tail->y-B->tip->y);
    vptr->y= (A->tip->z-A->tail->z)*(B->tail->x-B->tip->x) - (A->tip->x-A->tail->x)*(B->tail->z-B->tip->z);
    vptr->z= (A->tip->x-A->tail->x)*(B->tail->y-B->tip->y) - (A->tip->y-A->tail->y)*(B->tail->x-B->tip->x);
    return(1);
    }
  if(vertex_compare(A->tail,B->tail,CLOSE_ENOUGH)==TRUE) 
    {
    vptr->x= (A->tip->y-A->tail->y)*(B->tip->z-B->tail->z) - (A->tip->z-A->tail->z)*(B->tip->y-B->tail->y);
    vptr->y= (A->tip->z-A->tail->z)*(B->tip->x-B->tail->x) - (A->tip->x-A->tail->x)*(B->tip->z-B->tail->z);
    vptr->z= (A->tip->x-A->tail->x)*(B->tip->y-B->tail->y) - (A->tip->y-A->tail->y)*(B->tip->x-B->tail->x);
    return(1);
    }
	  
  return(0);
  }
	
// Function to provide a unit normal vtx from a vector
// Inputs: vecin = incoming vector
// Return: vtx_norm is loaded with xyz result
int vector_unit_normal(vector *vecin, vertex *vtx_norm)
{
  double 	vecmag;
  
  if(vecin==NULL || vtx_norm==NULL)return(FALSE);
  if(vecin->tip==NULL || vecin->tail==NULL)return(FALSE);
  
  vecmag=vector_magnitude(vecin);
  vtx_norm->x=(vecin->tail->x-vecin->tip->x)/vecmag;
  vtx_norm->y=(vecin->tail->y-vecin->tip->y)/vecmag;
  vtx_norm->z=(vecin->tail->z-vecin->tip->z)/vecmag;
  
  return(TRUE);
}

// Function to dump vector list to screen
int vector_dump(vector *vptr)
{
	int		i=0;
	
	while(vptr!=NULL)
	  {
	  printf("VECTOR DUMP ------------------------------------------\n");
	  printf("  Vec:  %X  Next=%X  Cnt=%d Type=%d \n",vptr,vptr->next,i,vptr->type);
	  printf("  Tip:  Vtx=%X  x=%f  y=%f  z=%f  attr=%d \n",vptr->tip,vptr->tip->x,vptr->tip->y,vptr->tip->z,vptr->tip->attr);
	  printf("  Tail: Vtx=%X  x=%f  y=%f  z=%f  attr=%d \n",vptr->tail,vptr->tail->x,vptr->tail->y,vptr->tail->z,vptr->tail->attr);

	  vptr=vptr->next;
	  i++;
	  }
	
	return(1);
}

// Function to rotate a vector about its TAIL in the XY plane
// vinpt must be existing vectors with existing verticies at tip and tail
// theta provided in radians
int vector_rotate(vector *vinpt, float theta)
{
  double	dx,dy;

  // screen inputs for acceptable data
  if(theta<TOLERANCE)return(0);
  if(vinpt==NULL)return(0);
  if(vinpt->tip==NULL || vinpt->tail==NULL)return(0);
  
  // calculate lengths
  dx=vinpt->tip->x-vinpt->tail->x;
  dy=vinpt->tip->y-vinpt->tail->y;
  
  // define rotate vector values
  vinpt->tip->x=vinpt->tail->x+(dx*cos(theta)-dy*sin(theta));
  vinpt->tip->y=vinpt->tail->y+(dx*sin(theta)+dy*cos(theta));
  
  // recalc new vec angle
  vinpt->angle=vector_absangle(vinpt);
  
  return(1);
}

// Function to swap tip and tail of a vector
// equivelent to rotating 180 deg but likely faster.
int vector_tip_tail_swap(vector *vinpt)
{
  vertex	*vptr;
  
  if(vinpt==NULL)return(0);
  
  vptr=vinpt->tip;							// save address of tip vertex
  vinpt->tip=vinpt->tail;						// redefine tip as tail by address
  vinpt->tail=vptr;							// redefine tail as tip
  return(1);
}

// Function to create a vector list element
vector_list *vector_list_make(vector *vinpt)
{
  vector_list	*vl_ptr;
  
  vl_ptr=(vector_list *)malloc(sizeof(vector_list));
  if(vl_ptr!=NULL)
    {
    vl_ptr->v_item=vinpt;
    vl_ptr->next=NULL;
    }
  return(vl_ptr);
}
  
// Function to manage vector lists - typically used as a "user pick list", but can be used for other things
// actions:  ADD, DELETE, CLEAR
vector_list *vector_list_manager(vector_list *vlist, vector *vinpt, int action)
{
  vector	*vptr;
  vector_list	*vl_ptr,*vl_pre,*vl_del;
  
  if(action==ACTION_ADD)
    {
    vl_ptr=vector_list_make(vinpt);					// create a new element
    vl_ptr->v_item=vinpt;						// defines its contents
    vl_ptr->next=vlist;
    vlist=vl_ptr;

    /*
    // scan existing list to ensure the input vector is not already on the list.
    // this loop also sets vl_pre to point to the last item in the existing list.
    vl_pre=NULL;
    vl_ptr=vlist;
    while(vl_ptr!=NULL)
      {
      if(vl_ptr->v_item==vinpt)break;
      vl_pre=vl_ptr;
      vl_ptr=vl_ptr->next;
      }
    // if already on list, then remove it from list (i.e. user selected it again to remove it)
    if(vl_ptr!=NULL){printf("VLM: item %X already on list!\n",vl_ptr->v_item);return(vlist);}
      //{
      //vl_del=vl_ptr;							// save address of element to delete
      //if(vl_pre==NULL){vlist=vl_ptr->next;}				// if first element in list...
      //else {vl_pre->next=vl_ptr->next;}					// but if NOT first element... skip over element to be deleted
      //free(vl_del);							// finally delete the target element
      //}
    // if NOT already on list, create a new element and add to end of list
    else
      {
      vl_ptr=vector_list_make(vinpt);					// create a new element
      vl_ptr->v_item=vinpt;						// defines its contents
      vl_ptr->next=NULL;
      if(vlist==NULL)vlist=vl_ptr;
      if(vl_pre!=NULL)vl_pre->next=vl_ptr;				// add it to the end of the list
      }
    */
    }

  if(action==ACTION_DELETE)
    {
    vl_pre=NULL;
    vl_ptr=vlist;
    while(vl_ptr!=NULL)							// loop thru list to find pre node to delete node
      {
      if(vl_ptr->v_item==vinpt)break;
      vl_pre=vl_ptr;
      vl_ptr=vl_ptr->next;
      }
    if(vl_ptr!=NULL)							// if the delete node was found...
      {
      vl_del=vl_ptr;							// save address in temp ptr
      if(vl_pre==NULL)							// if head node, redefine head to be next node (which may be NULL if only one node in list)
        {vlist=vl_ptr->next;}
      else 
        {vl_pre->next=vl_ptr->next;}					// otherwise skip over node to be deleted
      free(vl_del);
      }
    }
    
  if(action==ACTION_CLEAR)
    {
    vl_ptr=vlist;
    while(vl_ptr!=NULL)
      {
      vl_del=vl_ptr;
      vl_ptr=vl_ptr->next;
      free(vl_del);
      }
    if(vlist==vec_pick_list)vec_pick_list=NULL;
    vlist=NULL;
    }
    
  // DEBUG - dump vector list
  /*
  printf("\nVLM:  vector list dump\n");
  int vec_count=1;
  vptr=vl_ptr->v_item;
  while(vptr!=NULL)  
    {
    printf("  %d  vptr=%X  next=%X  x=%6.3f y=%6.3f   x=%6.3f y=%6.3f \n",vec_count,vptr,vptr->next,vptr->tip->x,vptr->tip->y,vptr->tail->x,vptr->tail->y);
    vptr=vptr->next;
    if(vptr==vl_ptr->v_item)break;
    vec_count++;
    if(vec_count>500)break;
    }
  printf("-----------------------\n");
  */  
    
  return(vlist);
}  

// Function to verify the integrity of a vector list
int vector_list_check(vector_list *vlist)
{
  int		status=1, polygon_cnt=1, vector_cnt=1; 
  float 	total_len,min_len;
  vector_list 	*vl_ptr;
  vector 	*vecptr;
  vertex 	*vtxptr;
  
  if(vlist==NULL)return(0);
  
  vl_ptr=vlist;
  while(vl_ptr!=NULL)
    {
    printf("\npolygon %d   vlist=%X vl_ptr=%X vl_nxt=%X --------------------\n",polygon_cnt,vlist,vl_ptr,vl_ptr->next);
    vector_cnt=1;
    total_len=0.0;
    min_len=1000;
    vecptr=vl_ptr->v_item;
    while(vecptr!=NULL)
      {
      if(vector_cnt<10)
        {
	printf("  vec=%X  nxt=%X  pre=%X   mem=%d   x1=%6.3f y1=%6.3f   x2=%6.3f y2=%6.3f \n",vecptr,vecptr->next,vecptr->prev,vecptr->member,vecptr->tip->x,vecptr->tip->y,vecptr->tail->x,vecptr->tail->y);
	}
      if(vecptr->curlen<TOLERANCE)status=0;
      if(vecptr->curlen<min_len)min_len=vecptr->curlen;
      total_len += vecptr->curlen;
      vector_cnt++;
      vecptr=vecptr->next;
      if(vecptr==vl_ptr->v_item)break;
      }
      
    if(vecptr==vl_ptr->v_item)
      {printf("  %d vectors   closed-loop  len=%6.4f  min=%6.4f ",vector_cnt,total_len,min_len);}
    else 
      {printf("  %d vectors   open-loop  len=%6.4f  min=%6.4f ",vector_cnt,total_len,min_len);}
    if(status==0)printf(" zero length found ");
    printf("\n---------------------------------------------------------------\n");

    polygon_cnt++;
    vl_ptr=vl_ptr->next;
    }
    
  return(status);
}

