#include "Global.h"


// This function returns the amount of memory currently in use by element and byte consupmtion
int memory_status(void)
{
  int		h,i,slot;
  
  long int	md_geo_vtx[4],md_geo_edg[4],md_geo_fct[4],md_geo_sli[4];	// model: vtx, edge, fct, slices
  long int	sp_geo_vtx[4],sp_geo_edg[4],sp_geo_fct[4],sp_geo_sli[4];	// support
  long int	bl_geo_vtx[4],bl_geo_edg[4],bl_geo_fct[4],bl_geo_sli[4];	// baselayers
  long int	ao_geo_vtx[4],ao_geo_edg[4],ao_geo_fct[4],ao_geo_sli[4];	// all else
  
  long int 	md_ply_vtx[4],bl_ply_vtx[4],sp_ply_vtx[4],ao_ply_vtx[4];
  long int 	md_sli_ply[4],bl_sli_ply[4],sp_sli_ply[4],ao_sli_ply[4];
  long int 	md_sli_vec[4],bl_sli_vec[4],sp_sli_vec[4],ao_sli_vec[4];
  
  long int 	md_geo_vtxt=0,md_geo_edgt=0,md_geo_fact=0,md_geo_slit=0;
  long int 	sp_geo_vtxt=0,sp_geo_edgt=0,sp_geo_fact=0,sp_geo_slit=0;
  long int 	bl_geo_vtxt=0,bl_geo_edgt=0,bl_geo_fact=0,bl_geo_slit=0;
  long int 	ao_geo_vtxt=0,ao_geo_edgt=0,ao_geo_fact=0,ao_geo_slit=0;
  
  long int	md_ply_vtxt=0,bl_ply_vtxt=0,sp_ply_vtxt=0,ao_ply_vtxt=0;
  long int 	md_sli_plyt=0,bl_sli_plyt=0,sp_sli_plyt=0,ao_sli_plyt=0;
  long int 	md_sli_vect=0,bl_sli_vect=0,sp_sli_vect=0,ao_sli_vect=0;
  
  vertex	*vptr;
  polygon	*pptr;
  vector	*vecptr;
  edge		*eptr;
  facet		*ftpr;
  slice		*sptr;
  model		*mptr;
  genericlist	*aptr;
  
  // loop thru all model of all slot to accumulate data
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    md_geo_vtx[slot]=0; md_geo_edg[slot]=0; md_geo_fct[slot]=0; md_geo_sli[slot]=0;
    sp_geo_vtx[slot]=0; sp_geo_edg[slot]=0; sp_geo_fct[slot]=0; sp_geo_sli[slot]=0;
    bl_geo_vtx[slot]=0; bl_geo_edg[slot]=0; bl_geo_fct[slot]=0; bl_geo_sli[slot]=0;
    ao_geo_vtx[slot]=0; ao_geo_edg[slot]=0; ao_geo_fct[slot]=0; ao_geo_sli[slot]=0;
    
    md_ply_vtx[slot]=0; bl_ply_vtx[slot]=0; sp_ply_vtx[slot]=0; ao_ply_vtx[slot]=0;
    md_sli_ply[slot]=0; bl_sli_ply[slot]=0; sp_sli_ply[slot]=0; ao_sli_ply[slot]=0;
    md_sli_vec[slot]=0; bl_sli_vec[slot]=0; sp_sli_vec[slot]=0; ao_sli_vec[slot]=0;
  
    // loop thru all models in the job associated to this slot
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      // accumulate pre slice geometry related elements
      md_geo_vtx[slot] += mptr->vertex_qty[MODEL];			// note += to add from multiple models on one tool
      md_geo_edg[slot] += mptr->edge_qty[MODEL];
      md_geo_fct[slot] += mptr->facet_qty[MODEL];
      md_geo_sli[slot] += mptr->slice_qty[slot];
  
      sp_geo_vtx[slot] += mptr->vertex_qty[SUPPORT];
      sp_geo_edg[slot] += mptr->edge_qty[SUPPORT];
      sp_geo_fct[slot] += mptr->facet_qty[SUPPORT];
  
      // loop thru the model's operation list and accumulate slot data
      aptr=mptr->oper_list;
      while(aptr!=NULL)
	{
	slot=aptr->ID;
	if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
	
	// loop thru the slices for this operation and accumulate line type and polygon data
	sptr=mptr->slice_first[slot];
	while(sptr!=NULL)
	  {
	  for(i=0;i<MAX_LINE_TYPES;i++)
	    {
	    pptr=sptr->pfirst[i];
	    while(pptr!=NULL)
	      {
	      // vertex qty = polygon vtxs + 2*perimeter vecs + 2*straight skeleton vecs + 2*raw vecs
	      // because there are two independent verticies per vector for perims, skeletons, and raws.
	      if(i>=MDL_PERIM && i<=INT_LAYER_1) md_ply_vtx[slot] += (pptr->vert_qty + 2*sptr->perim_vec_qty[i] + 2*sptr->ss_vec_qty[i] + 2*sptr->raw_vec_qty[i]);	// model line types
	      if(i>=BASELYR && i<=PLATFORMLYR2)  bl_ply_vtx[slot] += (pptr->vert_qty + 2*sptr->perim_vec_qty[i] + 2*sptr->ss_vec_qty[i] + 2*sptr->raw_vec_qty[i]);	// baselayer line types
	      if(i>=SPT_PERIM && i<=SPT_LAYER_1) sp_ply_vtx[slot] += (pptr->vert_qty + 2*sptr->perim_vec_qty[i] + 2*sptr->ss_vec_qty[i] + 2*sptr->raw_vec_qty[i]);	// support line types
	      if(i>=TRACE) ao_ply_vtx[slot] += (pptr->vert_qty + 2*sptr->perim_vec_qty[i] + 2*sptr->ss_vec_qty[i] + 2*sptr->raw_vec_qty[i]);		// all other line types
	      pptr=pptr->next;
	      }
	    if(i>=MDL_PERIM && i<=INT_LAYER_1)				// model polygon and vector counts
	      {
	      md_sli_ply[slot] += sptr->pqty[i];
	      md_sli_vec[slot] += sptr->perim_vec_qty[i];
	      md_sli_vec[slot] += sptr->ss_vec_qty[i];
	      md_sli_vec[slot] += sptr->raw_vec_qty[i];
	      }
	    if(i>=BASELYR && i<=PLATFORMLYR2)				// baselayer polygon and vector counts
	      {
	      bl_sli_ply[slot] += sptr->pqty[i];
	      bl_sli_vec[slot] += sptr->perim_vec_qty[i];
	      bl_sli_vec[slot] += sptr->ss_vec_qty[i];
	      bl_sli_vec[slot] += sptr->raw_vec_qty[i];
	      }
	    if(i>=SPT_PERIM && i<=SPT_LAYER_1)				// support polygon and vector counts
	      {
	      sp_sli_ply[slot] += sptr->pqty[i];
	      sp_sli_vec[slot] += sptr->perim_vec_qty[i];
	      sp_sli_vec[slot] += sptr->ss_vec_qty[i];
	      sp_sli_vec[slot] += sptr->raw_vec_qty[i];
	      }
	    if(i>=TRACE)							// all other polygon and vector counts
	      {
	      ao_sli_ply[slot] += sptr->pqty[i];
	      ao_sli_vec[slot] += sptr->perim_vec_qty[i];
	      ao_sli_vec[slot] += sptr->ss_vec_qty[i];
	      ao_sli_vec[slot] += sptr->raw_vec_qty[i];
	      }
	    }
	  sptr=sptr->next;
	  }
	aptr=aptr->next;
	}
      mptr=mptr->next;
      }		// end of mptr loop
      
    }		// end of slot loop
    
  // sum slot totals
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    md_geo_vtxt += md_geo_vtx[slot];
    md_geo_edgt += md_geo_edg[slot];
    md_geo_fact += md_geo_fct[slot];
    md_geo_slit += md_geo_sli[slot];
    md_ply_vtxt += md_ply_vtx[slot];
    md_sli_plyt += md_sli_ply[slot];
    md_sli_vect += md_sli_vec[slot];
    sp_geo_vtxt += sp_geo_vtx[slot];
    sp_geo_edgt += sp_geo_edg[slot];
    sp_geo_fact += sp_geo_fct[slot];
    sp_geo_slit += sp_geo_sli[slot];
    sp_ply_vtxt += sp_ply_vtx[slot];
    sp_sli_plyt += sp_sli_ply[slot];
    sp_sli_vect += sp_sli_vec[slot];
    }
  
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    printf("\n\nMEMORY IN USE: \n");
    printf("Item		Tool 1	Tool 2	Tool 3	Tool 4	Total	Bytes \n");
    printf("model geometry \n");
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_geo_vtx[0],md_geo_vtx[1],md_geo_vtx[2],md_geo_vtx[3],md_geo_vtxt,(md_geo_vtxt*sizeof(vertex)));
    printf("  edge		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_geo_edg[0],md_geo_edg[1],md_geo_edg[2],md_geo_edg[3],md_geo_edgt,(md_geo_edgt*sizeof(edge)));
    printf("  facet		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_geo_fct[0],md_geo_fct[1],md_geo_fct[2],md_geo_fct[3],md_geo_fact,(md_geo_fact*sizeof(facet)));
    printf("support geometry \n");
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_geo_vtx[0],sp_geo_vtx[1],sp_geo_vtx[2],sp_geo_vtx[3],sp_geo_vtxt,(sp_geo_vtxt*sizeof(vertex)));
    printf("  edge		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_geo_edg[0],sp_geo_edg[1],sp_geo_edg[2],sp_geo_edg[3],sp_geo_edgt,(sp_geo_edgt*sizeof(edge)));
    printf("  facet		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_geo_fct[0],sp_geo_fct[1],sp_geo_fct[2],sp_geo_fct[3],sp_geo_fact,(sp_geo_fact*sizeof(facet)));
    printf("model slice data \n");
    printf("  slice		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_geo_sli[0],md_geo_sli[1],md_geo_sli[2],md_geo_sli[3],md_geo_slit,(md_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_sli_vec[0],md_sli_vec[1],md_sli_vec[2],md_sli_vec[3],md_sli_vect,(md_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_sli_ply[0],md_sli_ply[1],md_sli_ply[2],md_sli_ply[3],md_sli_plyt,(md_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",md_ply_vtx[0],md_ply_vtx[1],md_ply_vtx[2],md_ply_vtx[3],md_ply_vtxt,(md_ply_vtxt*sizeof(vertex)));
    printf("support slice data \n");
    printf("  slice		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_geo_sli[0],sp_geo_sli[1],sp_geo_sli[2],sp_geo_sli[3],sp_geo_slit,(sp_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_sli_vec[0],sp_sli_vec[1],sp_sli_vec[2],sp_sli_vec[3],sp_sli_vect,(sp_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_sli_ply[0],sp_sli_ply[1],sp_sli_ply[2],sp_sli_ply[3],sp_sli_plyt,(sp_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",sp_ply_vtx[0],sp_ply_vtx[1],sp_ply_vtx[2],sp_ply_vtx[3],sp_ply_vtxt,(sp_ply_vtxt*sizeof(vertex)));
    printf("baselayer slice data \n");
    printf("  slice		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",bl_geo_sli[0],bl_geo_sli[1],bl_geo_sli[2],bl_geo_sli[3],bl_geo_slit,(bl_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",bl_sli_vec[0],bl_sli_vec[1],bl_sli_vec[2],bl_sli_vec[3],bl_sli_vect,(bl_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",bl_sli_ply[0],bl_sli_ply[1],bl_sli_ply[2],bl_sli_ply[3],bl_sli_plyt,(bl_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",bl_ply_vtx[0],bl_ply_vtx[1],bl_ply_vtx[2],bl_ply_vtx[3],bl_ply_vtxt,(bl_ply_vtxt*sizeof(vertex)));
    printf("all other slice data \n");
    printf("  slice		%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",ao_geo_sli[0],ao_geo_sli[1],ao_geo_sli[2],ao_geo_sli[3],ao_geo_slit,(ao_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",ao_sli_vec[0],ao_sli_vec[1],ao_sli_vec[2],ao_sli_vec[3],ao_sli_vect,(ao_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",ao_sli_ply[0],ao_sli_ply[1],ao_sli_ply[2],ao_sli_ply[3],ao_sli_plyt,(ao_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld	%7ld	%7ld	%7ld \n",ao_ply_vtx[0],ao_ply_vtx[1],ao_ply_vtx[2],ao_ply_vtx[3],ao_ply_vtxt,(ao_ply_vtxt*sizeof(vertex)));
    printf("\nMemory tracking data\n");
    printf("Item		Qty		Bytes \n");
    printf("   vertex.:	%9ld	%9ld \n",vertex_mem,(vertex_mem*sizeof(vertex)));
    printf("   edge...:	%9ld	%9ld \n",edge_mem,(edge_mem*sizeof(edge)));
    printf("   vector.:	%9ld	%9ld \n",vector_mem,(vector_mem*sizeof(vector)));
    printf("   polygon:	%9ld	%9ld \n",polygon_mem,(polygon_mem*sizeof(polygon)));
    printf("   facet..:	%9ld	%9ld \n",facet_mem,(facet_mem*sizeof(facet)));
    printf("   slice..:	%9ld	%9ld \n",slice_mem,(slice_mem*sizeof(slice)));
    printf("   model..:	%9ld	%9ld \n",job.model_count,(job.model_count*sizeof(model)));
    printf("   tools..: 	%9ld	%9ld \n",MAX_TOOLS,(MAX_TOOLS*sizeof(toolx)));
    printf("   job....: 	%9ld	%9ld \n",1,(sizeof(jobx)));
    printf("   \n");
  #endif
    
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    printf("\n\nMEMORY IN USE: \n");
    printf("Item		Tool 1	Total	Bytes \n");
    printf("model geometry \n");
    printf("  vertex	%7ld	%7ld	%7ld \n",md_geo_vtx[0],md_geo_vtxt,(md_geo_vtxt*sizeof(vertex)));
    printf("  edge		%7ld	%7ld	%7ld \n",md_geo_edg[0],md_geo_edgt,(md_geo_edgt*sizeof(edge)));
    printf("  facet		%7ld	%7ld	%7ld \n",md_geo_fct[0],md_geo_fact,(md_geo_fact*sizeof(facet)));
    printf("support geometry \n");
    printf("  vertex	%7ld	%7ld	%7ld \n",sp_geo_vtx[0],sp_geo_vtxt,(sp_geo_vtxt*sizeof(vertex)));
    printf("  edge		%7ld	%7ld	%7ld \n",sp_geo_edg[0],sp_geo_edgt,(sp_geo_edgt*sizeof(edge)));
    printf("  facet		%7ld	%7ld	%7ld \n",sp_geo_fct[0],sp_geo_fact,(sp_geo_fact*sizeof(facet)));
    printf("model slice data \n");
    printf("  slice		%7ld	%7ld	%7ld \n",md_geo_sli[0],md_geo_slit,(md_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld \n",md_sli_vec[0],md_sli_vect,(md_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld \n",md_sli_ply[0],md_sli_plyt,(md_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld \n",md_ply_vtx[0],md_ply_vtxt,(md_ply_vtxt*sizeof(vertex)));
    printf("support slice data \n");
    printf("  slice		%7ld	%7ld	%7ld \n",sp_geo_sli[0],sp_geo_slit,(sp_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld \n",sp_sli_vec[0],sp_sli_vect,(sp_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld \n",sp_sli_ply[0],sp_sli_plyt,(sp_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld \n",sp_ply_vtx[0],sp_ply_vtxt,(sp_ply_vtxt*sizeof(vertex)));
    printf("baselayer slice data \n");
    printf("  slice		%7ld	%7ld	%7ld \n",bl_geo_sli[0],bl_geo_slit,(bl_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld \n",bl_sli_vec[0],bl_sli_vect,(bl_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld \n",bl_sli_ply[0],bl_sli_plyt,(bl_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld \n",bl_ply_vtx[0],bl_ply_vtxt,(bl_ply_vtxt*sizeof(vertex)));
    printf("all other slice data \n");
    printf("  slice		%7ld	%7ld	%7ld \n",ao_geo_sli[0],ao_geo_slit,(ao_geo_slit*sizeof(slice)));
    printf("  vector	%7ld	%7ld	%7ld \n",ao_sli_vec[0],ao_sli_vect,(ao_sli_vect*sizeof(vector)));
    printf("  polygon	%7ld	%7ld	%7ld \n",ao_sli_ply[0],ao_sli_plyt,(ao_sli_plyt*sizeof(polygon)));
    printf("  vertex	%7ld	%7ld	%7ld \n",ao_ply_vtx[0],ao_ply_vtxt,(ao_ply_vtxt*sizeof(vertex)));
    printf("\nMemory tracking data\n");
    printf("Item		Qty		Bytes \n");
    printf("   vertex.:	%9ld	%9ld \n",vertex_mem,(vertex_mem*sizeof(vertex)));
    printf("   edge...:	%9ld	%9ld \n",edge_mem,(edge_mem*sizeof(edge)));
    printf("   vector.:	%9ld	%9ld \n",vector_mem,(vector_mem*sizeof(vector)));
    printf("   polygon:	%9ld	%9ld \n",polygon_mem,(polygon_mem*sizeof(polygon)));
    printf("   facet..:	%9ld	%9ld \n",facet_mem,(facet_mem*sizeof(facet)));
    printf("   slice..:	%9ld	%9ld \n",slice_mem,(slice_mem*sizeof(slice)));
    printf("   model..:	%9ld	%9ld \n",job.model_count,(job.model_count*sizeof(model)));
    printf("   tools..: 	%9ld	%9ld \n",MAX_TOOLS,(MAX_TOOLS*sizeof(toolx)));
    printf("   job....: 	%9ld	%9ld \n",1,(sizeof(jobx)));
    printf("   \n");
  #endif
    
  return(1);
}

// This function allocates space for a single link list element of type vertex
vertex *vertex_make(void)
  {
  vertex 	*vptr;

  vptr=(vertex *)malloc(sizeof(vertex));
  if(vptr==NULL)return(NULL);		
  vptr->x=0;
  vptr->y=0;
  vptr->z=0;
  vptr->i=0;
  vptr->j=0;
  vptr->k=0;
  vptr->supp=0;
  vptr->attr=0;
  vptr->flist=NULL;
  vptr->next=NULL;
  vertex_mem++;								// increment number of vertices in memory
  return(vptr);
  }
	
// Function to insert new vertex into linked list just AFTER local location, which means the
// head node will never get replaced (or conversely, you cannot add to the front of the list).
int vertex_insert(model *mptr, vertex *local, vertex *vertexnew, int typ)
{
  if(mptr==NULL || vertexnew==NULL)return(FALSE);
  if(typ<1 || typ>MAX_MDL_TYPES)return(FALSE);
  
  if(mptr->vertex_qty[typ]==0)						// special case for first vertex loaded
    {
    mptr->vertex_first[typ]=vertexnew;	
    mptr->vertex_last[typ]=vertexnew;
    }
  else if(local==mptr->vertex_last[typ])				// test if insertion is at last element in list
    {
    local->next=vertexnew;     	    					// ... add link from old to new element
    mptr->vertex_last[typ]=vertexnew;					// ... define new end of list
    vertexnew->next=NULL;
    }
  else
    {
    vertexnew->next=local->next;					// point to new next vertex
    local->next=vertexnew;     	    					// add link from old to new element
    }
  mptr->vertex_qty[typ]++;						// increment number of elements
  
  return(TRUE);
}

// Function to insert new vertex into linked list just after local location ONLY IF UNIQUE in xyz position and SUPP.
// if not unique, it returns the address of the existing vtx at that location.
vertex *vertex_unique_insert(model *mptr, vertex *local, vertex *vtx_new, int typ, float tol)
{
  vertex 	*vptr;
  
  if(mptr==NULL || vtx_new==NULL)return(NULL);
  if(typ<1 || typ>MAX_MDL_TYPES)return(NULL);

  vptr=mptr->vertex_first[typ];
  while(vptr!=NULL)
    {
    if(vertex_compare(vtx_new,vptr,tol)==TRUE && vtx_new->supp==vptr->supp)break;
    vptr=vptr->next;
    }
  
  if(vptr==NULL)
    {
    vertex_insert(mptr,local,vtx_new,typ);
    vptr=vtx_new;
    }
  
  return(vptr);
}

// Function to insert a vertex into a random string of verticies provided it is not physically too
// close to either the previous, or next vertex.  note this creates a copy of vnew if distances are sufficient.
// Inputs:  vpre=vtx before location of insertion, vnew=new vtx to be inserted, tolerance=xy range to search
// Return:  address of existing vtx if one found within xy tolerance range, or address of new vtx if not
vertex *vertex_random_insert(vertex *vpre, vertex *vnew, double tolerance)
{
  float 	dx,dy,dist_pre,dist_nxt;
  vertex	*vptr,*vnxt,*vint;
  
  // ensure we have valid inputs
  if(vpre==NULL || vnew==NULL)return(NULL);
  
  // check to make sure the new vertex is far enough away from previous vtx
  dist_pre=vertex_distance(vnew,vpre);
  
  // check to make sure the new vertex is far enough away from next vtx
  vnxt=vpre->next;
  if(vnxt!=NULL)
    {dist_nxt=vertex_distance(vnew,vnxt);}
  else
    {dist_nxt=tolerance;}

  if(debug_flag==199)printf("  dist_pre=%f  dist_nxt=%f  tolerance=%f \n",dist_pre,dist_nxt,tolerance);

  // if the distance away from pre/nxt is far enough... return the newly inserted vtx as result
  if(dist_pre>tolerance && dist_nxt>tolerance)
    {
    // make a copy of vnew and define vnxt as ->next
    vint=vertex_copy(vnew,vnxt);
    
    // insert the between vpre and vnxt
    vpre->next=vint;
    vptr=vint;
    }

  // if the distance is too close to vpre... return vpre as result
  else if(dist_pre<tolerance && dist_nxt>tolerance)
    {
    vptr=vpre;
    }
    
  // if the distance is too close to vnxt... return vnxt as result
  else if(dist_pre>tolerance && dist_nxt<tolerance)
    {
    vptr=vnxt;
    }
    
  // if the distance is too close to both... return NULL as result
  else
    {
    vptr=NULL;
    }
  
  return(vptr);
}

// Function to delete a vertex from the list and out of memory
int vertex_delete(model *mptr, vertex *vdel, int typ)
  {
  vertex *vptr;

  if(mptr==NULL || vdel==NULL)return(0);
  if(typ<1 || typ>MAX_MDL_TYPES)return(0);

  if(vdel==mptr->vertex_first[typ])					// If the very first vtx is the delete target...
    {
    mptr->vertex_first[typ]=vdel->next;					// ... reset the model reference into the list
    free(vdel);	vertex_mem--; vdel=NULL;				// ... free the memory it was using
    mptr->vertex_qty[typ]--;						// ... decrement qty of vertices in linked list
    return(1);								// ... and exit with success.
    }
  vptr=mptr->vertex_first[typ];
  while(vptr!=NULL)							// search list for "next" link to element to be deleted
    {
    if(vptr->next==vdel)break;						// compare by memory address, break if matched
    vptr=vptr->next;							// move onto next element in list
    }
  if(vptr!=NULL)							// if a reference was found...
    {
    vptr->next=vdel->next;						// ... then skip over it in the list
    free(vdel);	vertex_mem--; vdel=NULL;				// ... free the memory it was using
    mptr->vertex_qty[typ]--;						// ... decrement qty of vertices in linked list
    return(1);								// ... and exit with success.
    }

  return(0);								// otherwise exit with failure
  }

// Function to compare two vertices by XYZ position (NOT memory address).
// Inputs:  A & B = verticies to compare, tol = tolerance threshold
int vertex_compare(vertex *A, vertex *B, float tol)
  {
  if(A==NULL || B==NULL)return(FALSE);
  if(fabs(A->x-B->x)>tol)return(FALSE);
  if(fabs(A->y-B->y)>tol)return(FALSE);
  if(fabs(A->z-B->z)>tol)return(FALSE);
  return(TRUE);
  }

// Function to compare two vertices by XY (No Z) by position.
// Inputs:  A & B = verticies to compare, tol = tolerance threshold
int vertex_xy_compare(vertex *A, vertex *B, float tol)
  {
  if(A==NULL || B==NULL)return(FALSE);
  if(fabs(A->x-B->x)>tol)return(FALSE);
  if(fabs(A->y-B->y)>tol)return(FALSE);
  return(TRUE);
  }

// Function to search for redundant vtxs by position
int vertex_redundancy_check(vertex *vtxlist)
{
  int		vtx_cnt=0;
  vertex	*vptr,*vnxt;
  
  vptr=vtxlist;
  while(vptr!=NULL)
    {
    vnxt=vptr->next;
    while(vnxt!=NULL)
      {
      if(vertex_compare(vptr,vnxt,TOLERANCE)==TRUE)
        {
	vtx_cnt++;
	printf("Redundant vtx %d found at:  x=%6.3f y=%6.3f z=%6.3f \n",vtx_cnt,vptr->x,vptr->y,vptr->z);
	}
      vnxt=vnxt->next;
      }
    vptr=vptr->next;
    }
  printf("Redundant vtx count: %d \n\n",vtx_cnt);
  
  return(vtx_cnt);
}

// Function to copy an existing vertex into a newly created one
vertex *vertex_copy(vertex *vptr, vertex *vnxt)
{
    int		i;
    vertex	*vnew;
    
    if(vptr==NULL)return(NULL);
    
    vnew=vertex_make();
    if(vnew==NULL)return(NULL);
    vnew->x=vptr->x;
    vnew->y=vptr->y;
    vnew->z=vptr->z;
    vnew->i=vptr->i;
    vnew->j=vptr->j;
    vnew->k=vptr->k;
    vnew->supp=vptr->supp;
    vnew->attr=vptr->attr;
    //vnew->flist=vptr->flist;		// <== would have to "make" these elements to make them independent
    vnew->flist=NULL;
    vnew->next=vnxt;

    return(vnew);
}

// Function to swap the contents of two verticies but NOT their addresses
int vertex_swap(vertex *A, vertex *B)
  {
  vertex	*vtx_temp;  
    
  vtx_temp=vertex_copy(A,NULL);
  if(vtx_temp==NULL)return(0);
  
  A->x=B->x; A->y=B->y; A->z=B->z;
  A->i=B->i; A->j=B->j; A->k=B->k;
  A->supp=B->supp; A->attr=B->attr;
  A->flist=B->flist;
  
  B->x=vtx_temp->x; B->y=vtx_temp->y; B->z=vtx_temp->z;
  B->i=vtx_temp->i; B->j=vtx_temp->j; B->k=vtx_temp->k;
  B->supp=vtx_temp->supp; B->attr=vtx_temp->attr;
  B->flist=vtx_temp->flist;
  
  free(vtx_temp); vertex_mem--; vtx_temp=NULL;
  
  return(1);
  }
  
// Function to purge entire vertex list regardless if NULL terminated or a loop
int vertex_purge(vertex *vlist)
  {
  int			vtx_count=0,i=0;
  vertex		*vptr,*vdel;
  facet_list		*fl_ptr,*fl_del;
  
  if(vlist==NULL)return(0);
  //printf("\nVertex Purge Entry:  vertex=%ld \n",vertex_mem);
  
  // establish exact count of number of vtxs to delete
  vptr=vlist;
  while(vptr!=NULL)
    {
    vtx_count++;
    vptr=vptr->next;
    if(vptr==vlist)break;						// if back at start (i.e. a loop) then exit
    }


  // loop thru list deleting as we go
  vptr=vlist;
  while(vptr!=NULL)
    {
    vdel=vptr;
    vptr=vptr->next;
    
    // clear facet list ptrs, but not the facets they point to
    fl_ptr=vdel->flist;							
    while(fl_ptr!=NULL)
      {
      fl_del=fl_ptr;
      fl_ptr=fl_ptr->next;
      if(fl_del!=NULL)free(fl_del);
      }
    
    // delete vtx
    if(vdel!=NULL)
      {
      free(vdel); vertex_mem--; vdel=NULL; 
      vtx_count--;
      i++;
      }
      
    if(vtx_count<=0)break;
    }
    
  //printf("Vertex Purge Exit:   vertex=%ld \n",vertex_mem);
  return(1);
  }

// Function to calculate the 2D distance between two verticies
float vertex_2D_distance(vertex *vtxA, vertex *vtxB)
{
  float 	dx,dy,dist;
  
  if(vtxA==NULL || vtxB==NULL)return(0);
  dx = vtxA->x - vtxB->x;
  dy = vtxA->y - vtxB->y;
  dist = sqrt(dx*dx + dy*dy);
  
  return(dist);
}

// Function to calculate the 3D distance between two verticies
float vertex_distance(vertex *vtxA, vertex *vtxB)
{
  float 	dx,dy,dz,dist;
  
  if(vtxA==NULL || vtxB==NULL)return(0);
  dx = vtxA->x - vtxB->x;
  dy = vtxA->y - vtxB->y;
  dz = vtxA->z - vtxB->z;
  dist = sqrt(dx*dx + dy*dy + dz*dz);
  
  return(dist);
}

// Function to linearly interpolate between to 3D verticies
// inputs:  f_dist=how far between A & B, vtxA=start vtx, vtxB=end vtx, vresult=vtx to hold answer
// note that this will NOT give values beyond vtxB
int vertex_3D_interpolate(float f_dist,vertex *vtxA,vertex *vtxB, vertex *vresult)
{
  float 	vtx_dist,scale;
  
  // validate inputs
  if(f_dist<TOLERANCE)return(FALSE);
  if(vtxA==NULL || vtxB==NULL || vresult==NULL)return(FALSE);
  
  vtx_dist=vertex_distance(vtxA,vtxB);
  if(vtx_dist<TOLERANCE)return(FALSE);
  
  scale = f_dist/vtx_dist;
  if(scale>1.0)return(FALSE);
  
  vresult->x = vtxA->x + scale * (vtxA->x - vtxB->x);
  vresult->y = vtxA->y + scale * (vtxA->y - vtxB->y);
  vresult->z = vtxA->z + scale * (vtxA->z - vtxB->z);
  
  return(TRUE);
}

// Function to check if three verticies are collinear in 2D space
int vertex_colinear_test(vertex *v1, vertex *v2, vertex *v3)
{
  int		status=FALSE;
  float 	result;
  float 	dx1,dy1,dx2,dy2;
  
  if(v1==NULL || v2==NULL || v3==NULL)return(status);

  // triagle area of the vtxs equals zero test
  result = v1->x*(v2->y - v3->y) + v2->x*(v3->y - v1->y) + v3->x*(v1->y - v2->y);
  if(fabs(result)<TIGHTCHECK)status=TRUE;

  // slope of both lines from one vtx are equal test
  //dx1=v2->x-v1->x;  dy1=v2->y-v1->y;
  //dx2=v3->x-v1->x;  dy2=v3->y-v1->y;
  //if(fabs((dx2*dy1)-(dx1*dy2))<TOLERANCE)status=TRUE;
  
  return(status);
}

// Function to calculate a vector cross-product from three verticies
// Basic formula:  cross product = (a2 * b3 – a3 * b2) * i + (a1 * b3 – a3 * b1) * j + (a1 * b2 – a2 * b1) * k
int vertex_cross_product(vertex *v1, vertex *v2, vertex *v3, vertex *vresult)
{
  float 	Ax,Ay,Az,Bx,By,Bz;
  
  if(v1==NULL || v2==NULL || v3==NULL)return(0);			// make sure we have something to work with
  
  Ax=v2->x-v1->x;
  Ay=v2->y-v1->y;
  Az=v2->z-v1->z;
  
  Bx=v2->x-v3->x;
  By=v2->y-v3->y;
  Bz=v2->z-v3->z;

  vresult->x = (Ay*Bz)-(Az*By);
  vresult->y = (Ax*Bz)-(Az*Bx);
  vresult->z = (Ax*By)-(Ay*Bx);
  
  return(1);
}

// Function to find a matching vertex by proximity to another vertex
vertex *vertex_match_XYZ(vertex *vtest,polygon *ptest)
{
    vertex	*vptr;
    float 	tol;
    
    if(vtest==NULL || ptest==NULL)return(NULL);

    tol=CLOSE_ENOUGH;
    vptr=ptest->vert_first;						// start with first vtx in ptest
    while(vptr!=NULL)							// loop thru entire list
      {
      if(fabs(vptr->x-vtest->x)<tol && fabs(vptr->y-vtest->y)<tol)return(vptr);
      vptr=vptr->next;							// move onto next vtx
      if(vptr==ptest->vert_first)break;					// if back at beginning, then quit
      }
    return(NULL);							// return that nothing was found
}

// Function to find a matching vertex by "supp" parameter in another polygon 
vertex *vertex_match_ID(vertex *vtest,polygon *ptest)
{
    vertex	*vptr;
    
    if(vtest==NULL || ptest==NULL)return(NULL);

    vptr=ptest->vert_first;						// start with first vtx in pA
    while(vptr!=NULL)							// loop thru entire list
      {
      if(vptr->supp>999 && vptr->supp==vtest->supp)return(vptr);	// if a matching ID is found, return that vtx address
      vptr=vptr->next;							// move onto next vtx
      if(vptr==ptest->vert_first)break;					// if back at beginning, then quit
      }
    return(NULL);							// return that nothing was found
}

// Function to find vertex previous to one specified in input
vertex *vertex_previous(vertex *vtest,polygon *pA)
{
    int		i;
    vertex	*vptr,*vpre;
    
    if(vtest==NULL || pA==NULL)return(NULL);

    i=0;
    vpre=NULL;
    vptr=pA->vert_first;
    while(vptr!=NULL)
      {
      i++;
      vpre=vptr;
      vptr=vptr->next;
      if(vptr==vtest)break;
      if(vptr==pA->vert_first)break;
      if(i>pA->vert_qty)break;
      }
    return(vpre);
}
  



// Function to create a vertex list element
vertex_list *vertex_list_make(vertex *vinpt)
{
  vertex_list	*vl_ptr;
  
  vl_ptr=(vertex_list *)malloc(sizeof(vertex_list));
  if(vl_ptr!=NULL)
    {
    vl_ptr->v_item=vinpt;
    vl_ptr->next=NULL;
    }
  return(vl_ptr);
}
  
// Function to manage vertex lists - typically used as a "user pick list", but can be used for other things
// actions:  ADD, DELETE, CLEAR
vertex_list *vertex_list_manager(vertex_list *vlist, vertex *vinpt, int action)
{
  vertex	*vptr;
  vertex_list	*vl_ptr,*vl_pre,*vl_del;
  
  if(action==ACTION_ADD)						// add new element at front of list...
    {
    vl_ptr=vertex_list_make(vinpt);					// create a new element
    vl_ptr->v_item=vinpt;						// defines its contents
    vl_ptr->next=vlist;							// add linkage
    vlist=vl_ptr;							// redefine head of list as new element

    /*
    // scan existing list to ensure the input vertex is not already on the list.
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
    if(vl_ptr!=NULL){return(vlist);}
      //{
      //vl_del=vl_ptr;							// save address of element to delete
      //if(vl_pre==NULL){vlist=vl_ptr->next;}				// if first element in list...
      //else {vl_pre->next=vl_ptr->next;}					// but if NOT first element... skip over element to be deleted
      //free(vl_del);							// finally delete the target element
      //}
    // if NOT already on list, create a new element and add to end of list
    else
      {
      vl_ptr=vertex_list_make(vinpt);					// create a new element
      vl_ptr->v_item=vinpt;						// defines its contents
      vl_ptr->next=NULL;
      if(vlist==NULL)vlist=vl_ptr;					// if a new list is being created
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
    if(vlist==vtx_pick_list)vtx_pick_list=NULL;
    vlist=NULL;
    }
    
  return(vlist);
}  

// Function to add on unique vtxs to a vertex list
vertex *vertex_list_add_unique(vertex_list *vl_inpt, vertex *vtx_inpt)
{
  vertex 	*vptr,*vtx_match;
  vertex_list	*vl_ptr,*vl_last;
  
  // validate inputs
  if(vl_inpt==NULL || vtx_inpt==NULL)return(NULL);
  
  // loop thru current list and see if a positional match can be found, and
  // use this opportunity to find the address of the last element in the source
  // vertex list
  vtx_match=NULL;
  vl_last=NULL;
  vl_ptr=vl_inpt;
  while(vl_ptr!=NULL)							// loop thru entire source vtx list
    {
    vptr=vl_ptr->v_item;						// get address of actual vtx
    if(vertex_compare(vptr,vtx_inpt,CLOSE_ENOUGH)==TRUE)vtx_match=vptr;	// compare.  if a match, save match's address
    vl_last=vl_ptr;							// save address of last vtx list element
    vl_ptr=vl_ptr->next;						// move onto next vtx list element
    }

  // if a matching vtx was not found in the current list... 
  if(vtx_match==NULL)
    {
    vl_ptr=vertex_list_make(vtx_inpt);					// ... create new element pointing to source vtx
    vl_last->next=vl_ptr;						// ... add linkage
    vl_ptr->next=NULL;							// ... ensure end is set
    }
  
  return(vtx_match);							// return address of match if found
}

// Function to return the next perimeter vertex when searching clockwise from the inboud xy location of vcenter
// The idea is to look at vcenter's 8 neighbors and figure out which of them is the next step in generating the
// perimeter.  This is done by a crude winding number test on each "on" neighbor found.
//
// Inputs:	pgrid = pointer to the entire grid (of unknown dimensions)
//		vcenter = pointer to vertex that in the center of all neighbors
//			note vcenter->k = spent status
//			note vcenter->attr = inbound direction (0=from E, 1=NE, 2=N, 3=NW, 4=W, 5=SW, 6=S, 7=SE)
//
// Return:	a pointer to the vertex that satisfies the search criterea
//		NULL is returned if a "dead end" is hit (happens if polygons overlap)
//
vertex 	*vertex_find_in_grid(polygon *pgrid, vertex *vcenter)
{
  int		h,i,icnt,pos;
  int 		nwind,ewind,wwind,swind;				// winding numbers for each direction
  float 	old_x;
  vertex	*vptr,*v[8],*vdump,*vtest,*vlst,*vpre_ew,*vpre_ns;
  
  //printf("\nVertex Find in Grid entry...\n");
  //printf("  pgrid=%X  vqty=%d  vcenter=%X \n",pgrid,pgrid->vert_qty,vcenter);
  
  for(i=0;i<8;i++)v[i]=NULL;
  vcenter->k=1;

  // find the east/west neighbors
  pos=(-1);
  vptr=pgrid->vert_first;
  while(vptr!=NULL)
    {
    if(vptr==vcenter)pos=0;
    if(fabs(vptr->y-vcenter->y)<TOLERANCE && pos<0)v[0]=vptr;
    if(fabs(vptr->y-vcenter->y)<TOLERANCE && pos>0){v[4]=vptr;break;}
    vptr=vptr->next;
    if(pos==0)pos=1;
    }
  
  // find the north/south neighbors
  pos=(-1);
  vptr=pgrid->vert_first;
  while(vptr!=NULL)
    {
    if(vptr==vcenter)pos=0;
    if(fabs(vptr->x-vcenter->x)<TOLERANCE && pos<0)v[2]=vptr;
    if(fabs(vptr->x-vcenter->x)<TOLERANCE && pos>0){v[6]=vptr;break;}
    vptr=vptr->next;
    if(pos==0)pos=1;
    }

  // find the NE/SE neighbors
  pos=(-1);
  vptr=pgrid->vert_first;
  while(vptr!=NULL)
    {
    if(vptr==v[0])pos=0;
    if(fabs(vptr->x-v[0]->x)<TOLERANCE && pos<0)v[1]=vptr;
    if(fabs(vptr->x-v[0]->x)<TOLERANCE && pos>0){v[7]=vptr;break;}
    vptr=vptr->next;
    if(pos==0)pos=1;
    }

  // find the NW/SW neighbors
  pos=(-1);
  vptr=pgrid->vert_first;
  while(vptr!=NULL)
    {
    if(vptr==v[4])pos=0;
    if(fabs(vptr->x-v[4]->x)<TOLERANCE && pos<0)v[3]=vptr;
    if(fabs(vptr->x-v[4]->x)<TOLERANCE && pos>0){v[5]=vptr;break;}
    vptr=vptr->next;
    if(pos==0)pos=1;
    }
  
  //printf("\nVFG: vcenter=%X  x=%6.3f  y=%6.3f  s=%d  p=%d\n",vcenter,vcenter->x,vcenter->y,vcenter->supp,vcenter->attr);
  
  // now starting from the inbound direction, search for the first un-spent neighbor that is "on" after
  // one is "off" in clockwise fashion AND demonstrates the proper winding number in all four directions.
  vptr=NULL;								// init to nothing found
  if(vcenter->attr<0 || vcenter->attr>7)vcenter->attr=0;		// ensure our vcenter references an inbound direction
  icnt=0;								// init loop counter
  i=vcenter->attr;							// init inbound direction counter to compliment of inbound direction
  if(i==0)vlst=v[7];
  if(i>0)vlst=v[i-1];
  while(icnt<8)								// loop thru all neighbors
    {
    //printf("     vlst=%X   x=%6.3f  y=%6.3f  s=%d  k=%2.0f\n",vlst,vlst->x,vlst->y,vlst->supp,vlst->k);
    //printf("     v[%d]=%X   x=%6.3f  y=%6.3f  s=%d  k=%2.0f\n",i,v[i],v[i]->x,v[i]->y,v[i]->supp,v[i]->k);
    if(v[i]->k<0 && v[i]->supp==1 && vlst->supp==0)			// if not spent and current vtx is on, test if on outside perim
      {
      nwind=0;swind=0;
      ewind=0;wwind=0;
      vpre_ew=pgrid->vert_first;
      vpre_ns=pgrid->vert_first;
      vtest=pgrid->vert_first;
      while(vtest!=NULL)						// loop thru the entire grid list...
	{
	if(vtest->x==v[i]->x && vtest->y==v[i]->y)
	  {
	  vpre_ew=vtest;
	  vpre_ns=vtest;
	  wwind++;							// need to increment due to nature of linked list
	  swind++;							// since these two will count this vtx as an "on"
	  }  

	if(vtest->x==v[i]->x && vtest->y < v[i]->y)			// count east crossings including current vtx
	  {
	  if(vpre_ew->supp==0 && vtest->supp==1)ewind++;		// if crossing into solid, increment
	  if(vpre_ew->supp==1 && vtest->supp==0)ewind--;		// if crossing into hole, decrement
	  vpre_ew=vtest;
	  //printf("     EAST    vt->y=%6.3f  v[%d]->y=%6.3f  ew=%d\n",vtest->y,i,v[i]->y,ewind);
	  }
	if(vtest->x==v[i]->x && vtest->y > v[i]->y)			// count west crossings NOT including current vtx
	  {
	  if(vpre_ew->supp==0 && vtest->supp==1)wwind++;		// if crossing into solid, increment
	  if(vpre_ew->supp==1 && vtest->supp==0)wwind--;		// if crossing into hole, decrement
	  vpre_ew=vtest;
	  //printf("     WEST    vt->y=%6.3f/%d  v[%d]->y=%6.3f/%d  ww=%d\n",vtest->y,vtest->supp,i,v[i]->y,v[i]->supp,wwind);
	  }
	
	if(vtest->y==v[i]->y && vtest->x < v[i]->x)			// count north crossings including current vtx
	  {
	  if(vpre_ns->supp==0 && vtest->supp==1)nwind++;		// if crossing into solid, increment
	  if(vpre_ns->supp==1 && vtest->supp==0)nwind--;		// if crossing into hole, decrement
	  vpre_ns=vtest;
	  //printf("     NORTH   vt->x=%6.3f  v[%d]->x=%6.3f  nw=%d\n",vtest->x,i,v[i]->x,nwind);
	  }
	if(vtest->y==v[i]->y && vtest->x > v[i]->x)			// count south crossings NOT including current vtx
	  {
	  if(vpre_ns->supp==0 && vtest->supp==1)swind++;		// if crossing into solid, increment
	  if(vpre_ns->supp==1 && vtest->supp==0)swind--;		// if crossing into hole, decrement
	  vpre_ns=vtest;
	  //printf("     SOUTH   vt->x=%6.3f  v[%d]->x=%6.3f  sw=%d\n",vtest->x,i,v[i]->x,swind);
	  }
	  
	vtest=vtest->next;
	}
	
      // to be a valid boundary, the winding numbers must compliment each other.
      // that is, one must be 1 and the opposite direction must be -1, or vice-versa.
      // otherwise it indicates we are completely inside/outside material.
      //printf("     E=%d  W=%d  N=%d  S=%d \n",ewind,wwind,nwind,swind);
      if(ewind==0 || wwind==0 || nwind==0 || swind==0){vptr=v[i];break;}
      }
    vlst=v[i];
    i--;								// move onto next neighbor
    if(i<0)i=7;								// wrap around as needed
    icnt++;								// increment loop counter
    }

  if(vptr!=NULL)
    {
    vptr->k=1;								// mark newly found vtx as spent
    vptr->attr=(i-4);							// save prev inbound direction
    if(vptr->attr<0)vptr->attr+=8;
/*
    // dump polygon grid
    printf("     vptr=%X     x=%6.3f  y=%6.3f  s=%d  p=%d\n",vptr,vptr->x,vptr->y,vptr->supp,vptr->attr);
    printf("\nGrid Dump:\n");
    vdump=pgrid->vert_first;
    old_x=vdump->x;
    while(vdump!=NULL)
      {
      if(fabs(vdump->x-old_x)>CLOSE_ENOUGH)printf("\n");
      if(vdump->z!=1)
        {
	if(vdump==vcenter)printf("X");
	if(vdump==vptr)printf("x");
	if(vdump!=vcenter && vdump!=vptr)printf("%d",vdump->supp);
	}
      else 
        {
	printf("#");
	}
      old_x=vdump->x;
      vdump=vdump->next;
      }
    while(!kbhit());
*/
    }
  
  //printf("\nVertex Find in Grid exit...\n");
  return(vptr);
}



// This function allocates space for a single link list element of type edge
edge *edge_make(void)
  {
  edge	*eptr;

  eptr=(edge *)malloc(sizeof(edge));
  if(eptr==NULL)return(NULL);
  eptr->tip=NULL;
  eptr->tail=NULL;
  eptr->Af=NULL;
  eptr->Bf=NULL;
  eptr->status=(-1);
  eptr->display=FALSE;
  eptr->type=0;
  eptr->next=NULL;
  edge_mem++;
  return(eptr);
  }

// Function to copy an edge
// Note - does NOT copy vertices (uses same pointers), also does NOT assign "next"
edge *edge_copy(edge *einp)
  {
  edge		*eptr;
  
  if(einp==NULL)return(NULL);
  eptr=edge_make();
  eptr->tip=einp->tip;
  eptr->tail=einp->tail;
  eptr->Af=einp->Af;
  eptr->Bf=einp->Bf;
  eptr->status=einp->status;
  eptr->display=einp->display;
  eptr->type=einp->type;  
    
  return(eptr);
  }
  
// Function to insert new edge into linked list
int edge_insert(model *mptr, edge *local, edge *edgenew, int typ)
  {

  if(mptr==NULL)return(0);
  if(edgenew==NULL)return(0);
  if(typ<1 || typ>MAX_MDL_TYPES)return(0);

  if(mptr->edge_qty[typ]==0)						// special case for first edge loaded
    {
    mptr->edge_first[typ]=edgenew;	
    mptr->edge_last[typ]=edgenew;
    }
  else if(local==mptr->edge_last[typ])				// test if insertion is at last element in list
    {
    local->next=edgenew;       	   
    mptr->edge_last[typ]=edgenew;	
    edgenew->next=NULL;
    }
  else 									// otherwise insert after local
    {			
    edgenew->next=local->next;
    local->next=edgenew;       	
    }
  mptr->edge_qty[typ]++;		
  return(1);			
  }

// Function to delete a edge from the list and out of memory
int edge_delete(model *mptr, edge *edel, int typ)
  {
  edge *eptr;

  if(mptr==NULL || edel==NULL)return(0);
  if(typ<1 || typ>MAX_MDL_TYPES)return(0);

  if(edel==mptr->edge_first[typ])					// If the very first edge is the delete target...
    {
    mptr->edge_first[typ]=edel->next;					// ... reset the model reference into the list
    free(edel);								// ... free the memory it was using
    mptr->edge_qty[typ]--;						// ... decrement qty of edges in linked list
    edge_mem--;								// ... decrement qty of edges in memory
    return(1);								// ... and exit with success.
    }
  eptr=mptr->edge_first[typ];
  while(eptr!=NULL)							// search list for "next" link to element to be deleted
    {
    if(eptr->next==edel)break;
    eptr=eptr->next;							// move onto next element in list
    }
  if(eptr!=NULL)							// if a reference was found...
    {
    if(edel==mptr->edge_last[typ])mptr->edge_last[typ]=eptr;	// ... reset end of list ptr
    eptr->next=edel->next;						// ... then skip over it in the list
    free(edel);								// ... free the memory it was using
    mptr->edge_qty[typ]--;						// ... decrement qty of edges in linked list
    edge_mem--;								// ... decrement qty of edges in memory
    return(1);								// ... and exit with success.
    }
  return(0);								// otherwise exist with failure
  }

// Function to compare two edges by vertex coordinates.  returns true if same
int edge_compare(edge *A, edge *B)
  {
  if(A->tip == B->tip && A->tail == B->tail)return(1);		// compare by memory address of member vertices
  if(A->tip == B->tail && A->tail == B->tip)return(1);
  return(0);
  }

// Function to create a list of edges for display.  This function identifies edges of facets that
// have a specific amount of angle between them.
int edge_display(model *mptr, float angle, int typ)
{
  int		h,i;
  float 	vangle,alpha,min_edge_len;
  double 	amt_done;
  edge		*eptr,*enew;
  facet 	*fptr;
  vector 	*vecptr;
  
  printf("\nEdge display:  entry\n");
  
  if(mptr==NULL)return(0);
  if(typ<1 || typ>MAX_MDL_TYPES)return(0);

  // clear edge display list if already exists
  edge_purge(mptr->edge_first[typ]);
  mptr->edge_first[typ]=NULL;
  mptr->edge_last[typ]=NULL;
  mptr->edge_qty[typ]=0;
  
  printf("   purge done\n");
  
  // reset facet status
  fptr=mptr->facet_first[typ];
  while(fptr!=NULL)
    {
    fptr->status=0;
    fptr=fptr->next;
    }
    
  printf("   facet reset done\n");

  // identify new edges that exceed the visible angle setting
  h=0;
  vangle=angle*PI/180;
  fptr=mptr->facet_first[typ];
  while(fptr!=NULL)
    {
    // update status to user
    if(typ==MODEL || typ==TARGET)
      {
      h++;
      amt_done = (double)(h)/(double)(mptr->facet_qty[typ]);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));
      }

    // if this facet is missing any parts (i.e. a free edge) ... skip it.
    if(fptr->fct[0]==NULL || fptr->fct[1]==NULL || fptr->fct[2]==NULL)
      {
      //printf("  fct[0]=%X  fct[1]=%X  fct[2]=%X \n",fptr->fct[0],fptr->fct[1],fptr->fct[2]);
      fptr=fptr->next;
      continue;
      }
    
    // loop thru all three neighboring facets to the current facet
    for(i=0;i<3;i++)
      {
      // check angle between current facet and its neighbor
      if((fptr->fct[i])->status==0)					// if this neighbor has not already been checked...
	{
	alpha=fabs(facet_angle(fptr,fptr->fct[i]));			// recall fct[0] is the neighbor along vtx[0]->vtx[1] of fptr
	//printf("  alpha=%f  vangle=%f \n",alpha,(PI-vangle));
	if(alpha>vangle && alpha<(PI-vangle))
	  {
	  enew=edge_make();
	  if(i==0){enew->tip=fptr->vtx[0]; enew->tail=fptr->vtx[1];}
	  if(i==1){enew->tip=fptr->vtx[1]; enew->tail=fptr->vtx[2];}
	  if(i==2){enew->tip=fptr->vtx[2]; enew->tail=fptr->vtx[0];}
	  enew->Af=fptr;
	  enew->Bf=fptr->fct[i];
	  enew->status=1;
	  enew->display=TRUE;
	  edge_insert(mptr,mptr->edge_last[typ],enew,typ);
	  }
	}
      }
      
    fptr->status=1;							// set status to spent
    fptr=fptr->next;
    }
  
  printf("   %d edges found\n",mptr->edge_qty[typ]);

  // attempt to reduce the number of edges in set by combining overly short ones and co-linear ones
  min_edge_len=set_view_edge_min_length;
  if(mptr->edge_qty[typ]<5000)min_edge_len=CLOSE_ENOUGH;
  eptr=mptr->edge_first[typ];
  if(eptr!=NULL)
    {
    vecptr=vector_make(eptr->tip,eptr->tail,43);
    while(eptr!=NULL)
      {
      vecptr->tip=eptr->tip;
      vecptr->tail=eptr->tail;
      if(vector_magnitude(vecptr)<min_edge_len)eptr->display=FALSE;
      eptr=eptr->next;
      }
    free(vecptr);vector_mem--;vecptr=NULL;
    }
    
  printf("Edge display: %ld edges made\n",mptr->edge_qty[typ]);
  
  return(1);
 }

// Function to create a list of edges that form a model's silhouette from a specific angle
// Note that this list differs from the edge display list in that this list is view angle dependent.
int edge_silhouette(model *mptr, float spin, float tilt)
{
  int		i,add_edge;
  float 	oss,osc,ots,otc;
  float 	Adot,Ndot;
  vertex 	*vwpt,*vctr,*Vvtx,*Avtx,*vtx1,*vtx2;
  edge		*eptr,*edel,*enew,*elast;
  facet 	*fptr,*fnbr;
  vector	*ViewVec,*Avec,*vecptr;
  
  
  // validate inputs
  if(mptr==NULL)return(FALSE);
  if(mptr->input_type==GERBER){mptr->MRedraw_flag=FALSE; return(FALSE);}
  
  // blow away old edge list
  if(mptr->silho_edges!=NULL)
    {
    edge_purge(mptr->silho_edges);
    mptr->silho_edges=NULL;
    }

  // resolve spin and tilt into trig values
  oss=sin(MVdisp_spin);							// X & Y spin about Z
  osc=cos(MVdisp_spin);
  ots=sin(MVdisp_tilt);							// Z is driven by tilt (but also effects X & Y)
  otc=cos(MVdisp_tilt);
  
  // create view point vector that is dependent upon spin and tilt of view
  // this is basically a unit normal vector of the plane defined by the viewing screen
  vwpt=vertex_make();
  vwpt->x=oss*otc*(-1);
  vwpt->y=osc*otc*(-1);
  vwpt->z=ots;	
  
  // find the edges in which facet normals switch from + to - relative to the view angle
  elast=NULL;
  vctr=vertex_make();
  Vvtx=vertex_make();
  Avtx=vertex_make();
  fptr=mptr->facet_first[MODEL];
  while(fptr!=NULL)
    {
    // if this facet is missing any parts (i.e. a free edge) ... skip it.
    if(fptr->fct[0]==NULL || fptr->fct[1]==NULL || fptr->fct[2]==NULL){fptr=fptr->next;continue;}
    if(fptr->unit_norm==NULL){fptr=fptr->next;continue;}
    if((fptr->fct[0])->unit_norm==NULL){fptr=fptr->next;continue;}
    if((fptr->fct[1])->unit_norm==NULL){fptr=fptr->next;continue;}
    if((fptr->fct[2])->unit_norm==NULL){fptr=fptr->next;continue;}
    
    
    // determine if current facet is pointing toward view or away from view.  skip if pointing away.
    add_edge=FALSE;
    facet_centroid(fptr,vctr);
    
    Vvtx->x=(vctr->x+vwpt->x);
    Vvtx->y=(vctr->y+vwpt->y);
    Vvtx->z=(vctr->z+vwpt->z);
    ViewVec=vector_make(vctr,Vvtx,99);					// define view vector at facet center

    Avtx->x=(vctr->x+fptr->unit_norm->x);
    Avtx->y=(vctr->y+fptr->unit_norm->y);
    Avtx->z=(vctr->z+fptr->unit_norm->z);
    Avec=vector_make(vctr,Avtx,99);					// define facet unit normal vector at facet center
    
    Adot=vector_dotproduct(ViewVec,Avec);				// + if pointing away, - if pointing toward
    
    free(ViewVec); vector_mem--;
    free(Avec); vector_mem--;
    
    if(Adot<0)								// if this facet is facing forward...
      {
      
      // loop thru all three neighboring facets of the current facet
      for(i=0;i<3;i++)
	{
	add_edge=FALSE;
	fnbr=fptr->fct[i];
	facet_centroid(fnbr,vctr);

	Vvtx->x=(vctr->x+vwpt->x);
	Vvtx->y=(vctr->y+vwpt->y);
	Vvtx->z=(vctr->z+vwpt->z);
	ViewVec=vector_make(vctr,Vvtx,99);				// define view vector at facet center
    
	Avtx->x=(vctr->x+fnbr->unit_norm->x);
	Avtx->y=(vctr->y+fnbr->unit_norm->y);
	Avtx->z=(vctr->z+fnbr->unit_norm->z);
	Avec=vector_make(vctr,Avtx,99);					// define facet unit normal vector at facet center
	
	Ndot=vector_dotproduct(ViewVec,Avec);				// + if pointing away, - if pointing toward
	
	free(ViewVec); vector_mem--;
	free(Avec); vector_mem--;

	if(Ndot>0)							// if signs flip... add edge
	  {
	  enew=edge_make();
	  if(i==0){enew->tip=fptr->vtx[0]; enew->tail=fptr->vtx[1];}
	  if(i==1){enew->tip=fptr->vtx[1]; enew->tail=fptr->vtx[2];}
	  if(i==2){enew->tip=fptr->vtx[2]; enew->tail=fptr->vtx[0];}
	  enew->Af=fptr;
	  enew->Bf=fptr->fct[i];
	  enew->status=1;
	  enew->display=TRUE;
	  enew->next=NULL;
	  if(mptr->silho_edges==NULL)mptr->silho_edges=enew;
	  if(elast!=NULL)elast->next=enew;
	  elast=enew;
	  }
	}
      }
    fptr=fptr->next;
    }
  
  // attempt to reduce the number of edges in set by only displaying every other edge and/or particularly long edges
  /*
  i=0;
  eptr=mptr->silho_edges;
  if(eptr!=NULL)
    {
    vecptr=vector_make(eptr->tip,eptr->tail,43);
    while(eptr!=NULL)
      {
      vecptr->tip=eptr->tip;
      vecptr->tail=eptr->tail;
      if(i%2==0)eptr->display=FALSE;					// display every other edge
      if(vector_magnitude(vecptr)>5.0)eptr->display=TRUE;		// unless the edge is so long it needs to be shown
      i++;
      eptr=eptr->next;
      }
    free(vecptr);vector_mem--;vecptr=NULL;
    }
  */

  // clean up
  free(Avtx); vertex_mem--;
  free(Vvtx); vertex_mem--;
  free(vctr); vertex_mem--;
  free(vwpt); vertex_mem--;
  
  mptr->MRedraw_flag=FALSE;
  
  return(1);
}

// Function to determine the angle between two facets (i.e. how sharp of a corner the edge represents)
float facet_angle(facet *Af, facet *Bf)
{
  float 	alpha,dotp;
  
  // verify inputs
  alpha=0.0;
  if(Af==NULL || Bf==NULL)return(alpha);
  if(Af->unit_norm==NULL || Bf->unit_norm==NULL)return(alpha);
  
  // check for verticals
  if(Af->unit_norm->z==1 && Bf->unit_norm->z==1)return(alpha);
  if(Af->unit_norm->z==(-1) && Bf->unit_norm->z==(-1))return(alpha);
  
  // calculate angle
  dotp=vertex_dotproduct(Af->unit_norm,Bf->unit_norm);			// calculate dot product
  if(dotp<=(-1.00)){alpha=PI;}
  else if(dotp>=(1.00)){alpha=0;}
  else {alpha=acos(dotp);}	
  
  return(alpha);
}

// Function to purge entire edge list 
int edge_purge(edge *elist)
  {
  edge		*eptr,*edel;
  
  if(elist==NULL)return(0);
  //printf("\nEdge Purge Entry:  edge=%ld \n",edge_mem);

  eptr=elist;
  while(eptr!=NULL)
    {
    edel=eptr;
    eptr=eptr->next;
    free(edel);	edge_mem--;
    }

  //printf("Edge Purge Exit:   edge=%ld \n",edge_mem);
  return(1);
  }



// Function to create an edge list element
edge_list *edge_list_make(edge *einpt)
{
  edge_list	*el_ptr;
  
  el_ptr=(edge_list *)malloc(sizeof(edge_list));
  if(el_ptr!=NULL)
    {
    el_ptr->e_item=einpt;
    el_ptr->next=NULL;
    }
  return(el_ptr);
}
  
// Function to manage edge lists - typically used as a "user pick list", but can be used for other things
// actions:  ADD, DELETE, CLEAR
edge_list *edge_list_manager(edge_list *elist, edge *einpt, int action)
{
  edge		*eptr;
  edge_list	*el_ptr,*el_pre,*el_del;
  
  if(action==ACTION_ADD)
    {
    el_ptr=edge_list_make(einpt);					// create a new element
    el_ptr->e_item=einpt;						// defines its contents
    el_ptr->next=elist;
    elist=el_ptr;

    /*
    // scan existing list to ensure the input edge is not already on the list
    el_pre=NULL;
    el_ptr=elist;
    while(el_ptr!=NULL)
      {
      if(el_ptr->e_item==einpt)break;
      el_pre=el_ptr;
      el_ptr=el_ptr->next;
      }
    // if already on list, then remove it from list (i.e. user selected it again to remove it)
    if(el_ptr!=NULL)
      {
      el_del=el_ptr;							// save address of element to delete
      if(el_pre==NULL)							// if first element in list...
        {elist=el_ptr->next;}
      else 								// but if NOT first element...
	{el_pre->next=el_ptr->next;}					// skip over element to be deleted
      free(el_del);							// finally delete the target element
      }
    // if NOT already on list, create a new element and add to end of list
    else
      {
      el_ptr=edge_list_make(einpt);					// create a new element
      el_ptr->e_item=einpt;						// defines its contents
      el_ptr->next=NULL;
      if(elist==NULL)elist=el_ptr;
      if(el_pre!=NULL)el_pre->next=el_ptr;				// add it to the end of the list
      }
    */
    }

  if(action==ACTION_DELETE)
    {
    el_pre=NULL;
    el_ptr=elist;
    while(el_ptr!=NULL)							// loop thru list to find pre node to delete node
      {
      if(el_ptr->e_item==einpt)break;
      el_pre=el_ptr;
      el_ptr=el_ptr->next;
      }
    if(el_ptr!=NULL)							// if the delete node was found...
      {
      el_del=el_ptr;							// save address in temp ptr
      if(el_pre==NULL)							// if head node, redefine head to be next node (which may be NULL if only one node in list)
        {elist=el_ptr->next;}
      else 
        {el_pre->next=el_ptr->next;}					// otherwise skip over node to be deleted
      free(el_del);
      }
    }
    
  if(action==ACTION_CLEAR)
    {
    el_ptr=elist;
    while(el_ptr!=NULL)
      {
      el_del=el_ptr;
      el_ptr=el_ptr->next;
      free(el_del);
      }
    if(elist==e_pick_list)e_pick_list=NULL;
    elist=NULL;
    }
    
  printf("\n  Edge Pick List: \n");
  el_ptr=elist;
  while(el_ptr!=NULL)
    {
    printf("   el_ptr=%X  el_next=%X  e_item=%X  type=%d \n",el_ptr,el_ptr->next,el_ptr->e_item,el_ptr->e_item->type);
    el_ptr=el_ptr->next;
    }
  printf("  --\n");
  
  return(elist);
}  

// Function to identify a collection of edges based on neighboring facet angles.
//
// This creates the "support edge list" identifying the boundaries of support.  Note that this list is NOT
// ordered.  That is, the edges are randomly added to it as they are found.
// This also creates the "e_base_list" for baselayers of each model.
edge *edge_angle_id(edge *inp_edge_list, float inp_angle)
{
  int 		edge_count=0;
  float 	max_angle=(-0.1);
  float 	nA,nB;
  edge		*ret_edge_list;
  edge		*eptr,*esptup,*enewup;
  
  printf("\nEntering edge_angle_id function:\n");
  
  // build edge support list from support areas
  if(inp_edge_list==NULL)return(NULL);
  
  // init
  max_angle=cos(inp_angle*PI/180);
  ret_edge_list=NULL;
  esptup=NULL;
  enewup=NULL;
  
  // cycle through entire edge list and both mark edges that separate facet pairs that cross the print angle threshold,
  // and add them to the return edge list.  the edge lists tend to be substantially smaller than the entire model edge
  // lists and are therefore much quicker to process in other functions that use them.
  eptr=inp_edge_list;
  while(eptr!=NULL)
    {
    eptr->type=0;
    if(eptr->Af==NULL || eptr->Bf==NULL){eptr=eptr->next;continue;}
    if(eptr->Af->unit_norm==NULL || eptr->Bf->unit_norm==NULL){eptr=eptr->next;continue;}
    nA=eptr->Af->unit_norm->z;						// normal of facet A
    nB=eptr->Bf->unit_norm->z;						// normal of facet B
    if(fabs(nA)<TOLERANCE && fabs(nB)<TOLERANCE){eptr=eptr->next;continue;}	// don't consider pairs of near horizontal normals
    if(nA<max_angle && nB>=max_angle){eptr->type=2;edge_count++;}	// if norm A points down and norm B is horiz or up...
    if(nA>=max_angle && nB<max_angle){eptr->type=2;edge_count++;}	// if norm A is horiz or up and norm B points down...
    if(eptr->type==2)							// if tagged as a viable edge for the ret list... 
      {
      enewup=edge_copy(eptr);						// ... create a copy
      if(ret_edge_list==NULL)						// if return edge list is empty...
        {
	ret_edge_list=enewup;						// ... define the copy as the first element in list
	esptup=enewup;							// ... save address of last element in list
	}
      else 								// if ret edge list already has elements...
        {
	esptup->next=enewup;						// ... point last element to new copy
	esptup=enewup;							// ... and redefine last element as new copy
	}
      }
    eptr=eptr->next;
    }
    
  printf("   %d edges identified at %f angle\n",edge_count,inp_angle);
  printf("Exiting edge_angle_id function.\n\n");
  
  return(ret_edge_list);
}

// Function to dump edge list to screen
int edge_dump(model *mptr)
{
    edge	*eptr;
    FILE	*test_ptr;
    char	output[255];
    
    test_ptr=fopen("test.gcode","w");
    eptr=mptr->edge_first[MODEL];
    while(eptr!=NULL)
      {
      if(eptr->type>0)
	{
	sprintf(output,"G1 X%f Y%f \n",eptr->tip->x,eptr->tip->y);
	fwrite(output,1,strlen(output),test_ptr);
	sprintf(output,"G1 X%f Y%f \n",eptr->tail->x,eptr->tail->y);
	fwrite(output,1,strlen(output),test_ptr);
	}
      eptr=eptr->next;
      }
    fflush(test_ptr);
    fclose(test_ptr);
    
    return(1);
}



// This function allocates space for a single link list element of type facet
facet *facet_make(void)
{
    int		i;
    facet 	*fptr;
	    
    fptr=(facet *)malloc(sizeof(facet));
    if(fptr==NULL)return(NULL);
    for(i=0;i<3;i++)
      {
      fptr->vtx[i]=NULL;
      fptr->fct[i]=NULL;
      }
    fptr->unit_norm=vertex_make();
    fptr->member=(-1);
    fptr->area=0.0;
    fptr->status=(-1);
    fptr->attr=(-1);
    fptr->supp=(-1);
    fptr->next=NULL;
    facet_mem++;
    return(fptr);
}

// Function to insert new facet, or list of facets, into linked list
// Inputs:  mptr=model, local=ptr to location in existing facet list, facetnew=new facet(s) to insert, typ=model type
// Return:  0=failed, 1=success
int facet_insert(model *mptr, facet *local, facet *facetnew, int typ)
{
  int 	fcount=0;
  facet	*fptr,*fold;
  
  if(mptr==NULL || facetnew==NULL)return(FALSE);
  if(typ<1 || typ>MAX_MDL_TYPES)return(FALSE);

  // determine how many facets are being added and the address of the last one
  fold=NULL;
  fptr=facetnew;
  while(fptr!=NULL)
    {
    fold=fptr; 
    fcount++; 
    fptr=fptr->next;
    }

  if(mptr->facet_first[typ]==NULL)					// special case for first facet loaded
    {
    mptr->facet_first[typ]=facetnew;	
    if(fold!=NULL){mptr->facet_last[typ]=fold;}
    else {mptr->facet_last[typ]=facetnew;}
    }
  else if(local==mptr->facet_last[typ])					// test if insertion is at last element in list
    {
    local->next=facetnew;      
    if(fold!=NULL){mptr->facet_last[typ]=fold;}
    else {mptr->facet_last[typ]=facetnew;}
    }
  else 									// otherwise insert after local
    {
    if(fold!=NULL)fold->next=local->next;
    local->next=facetnew; 
    }
  mptr->facet_qty[typ] += fcount;
  return(TRUE);		
}

// Function to delete a facet from the list and out of memory
int facet_delete(model *mptr, facet *fdel, int typ)
{
  facet *fptr;

  if(mptr==NULL || fdel==NULL)return(0);
  if(typ<1 || typ>MAX_MDL_TYPES)return(0);

  fptr=mptr->facet_first[typ];
  while(fptr!=NULL)							// search list for link to element to be deleted
    {
    if(facet_compare(fptr->next,fdel))break;				// break at element where it is referenced
    fptr=fptr->next;							// move onto next element in list
    }
  if(fptr!=NULL)							// if a reference was found...
    {
    fptr->next=fdel->next;						// ... then skip over it in the list
    free(fdel->unit_norm); vertex_mem--; fdel->unit_norm=NULL;		// ... free the memory containing the unit normal
    free(fdel); facet_mem--; fdel=NULL;					// ... free the memory it was using to hold pointers
    mptr->facet_qty[typ]--;						// ... decrement qty of facets in linked list
    return(1);								// ... and exit with success.
    }
  return(0);								// otherwise exist with failure
}

// Function to compare two facets - all 9 possibilities
int facet_compare(facet *A, facet *B)
{
  int		vtx_check;
  float 	tol;
  
  if(A->attr!=B->attr)return(0);
  
  vtx_check=0;
  tol=CLOSE_ENOUGH;

  // compare 1st vtx location against all 3 vtxs of B
  if(vertex_compare(A->vtx[0],B->vtx[0],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[0],B->vtx[1],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[0],B->vtx[2],tol)==TRUE)vtx_check++;

  // compare 2nd vtx location against all 3 vtxs of B
  if(vertex_compare(A->vtx[1],B->vtx[0],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[1],B->vtx[1],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[1],B->vtx[2],tol)==TRUE)vtx_check++;

  // compare 3rd vtx location against all 3 vtxs of B
  if(vertex_compare(A->vtx[2],B->vtx[0],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[2],B->vtx[1],tol)==TRUE)vtx_check++;
  if(vertex_compare(A->vtx[2],B->vtx[2],tol)==TRUE)vtx_check++;
  
  if(vtx_check==3)return(TRUE);
  return(FALSE);
}

// Function to purge entire facet list including its normal, but not its vtxs
int facet_purge(facet *flist)
  {
  facet		*fptr,*fdel;
  
  if(flist==NULL)return(0);
  //printf("\nFacet Purge Entry:  facet=%ld \n",facet_mem);
  
  fptr=flist;
  while(fptr!=NULL)
    {
    fdel=fptr;
    fptr=fptr->next;
    if(fdel->unit_norm!=NULL){free(fdel->unit_norm); vertex_mem--; fdel->unit_norm=NULL;}
    if(fdel!=NULL){free(fdel); facet_mem--; fdel=NULL;}
    }
    
  //printf("Facet Purge Exit:   facet=%ld \n",facet_mem);
  return(1);
  }

// Function to calculate a facet's unit normal vector
int facet_normal(facet *finp)
  {
  double 	vmag;
  vertex	*vtxA,*vtxB,*vtxC,*vnorm;
  vector	*vA,*vB,*vN;
  
  if(finp==NULL)return(0);
  if(finp->vtx[0]==NULL || finp->vtx[1]==NULL || finp->vtx[2]==NULL)return(0);
  
  vtxA=finp->vtx[0];
  vtxB=finp->vtx[1];
  vtxC=finp->vtx[2];
  
  vA=vector_make(vtxA,vtxB,0);
  vB=vector_make(vtxA,vtxC,0);
  vnorm=vertex_make();
  vector_crossproduct(vA,vB,vnorm);
  vN=vector_make(vtxA,vnorm,0);
  vmag=vector_magnitude(vN);
  
  finp->unit_norm->x=vnorm->x/vmag;
  finp->unit_norm->y=vnorm->y/vmag;
  finp->unit_norm->z=vnorm->z/vmag;
  
  free(vN);vector_mem--;vN=NULL;
  free(vnorm);vertex_mem--;vnorm=NULL;
  free(vB);vector_mem--;vB=NULL;
  free(vA);vector_mem--;vA=NULL;
  
  return(1);  
  }

// Function to calculate a facet's centroid
int facet_centroid(facet *fptr,vertex *vctr)
  {
  
  if(fptr==NULL || vctr==NULL)return(0);
  if(fptr->vtx[0]==NULL || fptr->vtx[1]==NULL || fptr->vtx[2]==NULL)return(0);
  
  vctr->x=(fptr->vtx[0]->x + fptr->vtx[1]->x + fptr->vtx[2]->x) / 3.0;
  vctr->y=(fptr->vtx[0]->y + fptr->vtx[1]->y + fptr->vtx[2]->y) / 3.0;
  vctr->z=(fptr->vtx[0]->z + fptr->vtx[1]->z + fptr->vtx[2]->z) / 3.0;
  
  return(1);
  }

// Function to calculate a facet's area
float facet_area(facet *fptr)
{
  float 	area;
  vector 	*vA,*vB,*vC;
  vertex	*vcrs;
  
  vcrs=vertex_make();
  vA=vector_make(fptr->vtx[0],fptr->vtx[1],0);
  vB=vector_make(fptr->vtx[0],fptr->vtx[2],0);
  
  vector_crossproduct(vA,vB,vcrs);
  vC=vector_make(fptr->vtx[0],vcrs,0);
  area=vector_magnitude(vC)/2.0;
  
  free(vC); vector_mem--; vC=NULL;
  free(vB); vector_mem--; vB=NULL;
  free(vA); vector_mem--; vA=NULL;
  free(vcrs); vertex_mem--; vcrs=NULL;
  
  fptr->area=area;
  
  return(area);
}

// Function to find facet neighbors
// note this only works IF the facets are sharing vtxs because it relies on the vtx list.
int facet_find_all_neighbors(facet *flist)
{
  int		body_cnt;
  vertex 	*vtxA,*vtxB,*vtxC;
  facet_list	*flA,*flB,*flC;
  facet 	*fptr,*fnbr[3],*fA,*fB,*fC;
  
  // loop thru facet list.  look at the three vtx facets lists.  when two vtxs have the same
  // facet address (beyond the facet we are on), then that is the neighbor.
  body_cnt=0;
  fptr=flist;
  while(fptr!=NULL)
    {
    //if(fptr->status==1){fptr=fptr->next;continue;}			// skip if set before hand
    vtxA=fptr->vtx[0]; vtxB=fptr->vtx[1]; vtxC=fptr->vtx[2];
    
    // check for connection bt vtxA and vtxB.  find the neighboring facet address by
    // comparing vtxA's facet list against vtxB's facet list.  they should have only
    // two in common... the two facets that share the edge formed by vtxA and vtxB.
    fnbr[0]=NULL;
    flA=vtxA->flist;
    while(flA!=NULL)
      {
      fA=flA->f_item;
      flB=vtxB->flist;
      while(flB!=NULL)
        {
	fB=flB->f_item;
	if(fA==fB && fA!=fptr){fnbr[0]=fA;break;}
	flB=flB->next;
	}
      if(fnbr[0]!=NULL)break;
      flA=flA->next;
      }
    
    // check for connection bt vtxB and vtxC
    fnbr[1]=NULL;
    flB=vtxB->flist;
    while(flB!=NULL)
      {
      fB=flB->f_item;
      flC=vtxC->flist;
      while(flC!=NULL)
        {
	fC=flC->f_item;
	if(fB==fC && fB!=fptr){fnbr[1]=fB;break;}
	flC=flC->next;
	}
      if(fnbr[1]!=NULL)break;
      flB=flB->next;
      }
    
    // check for connection bt vtxC and vtxA
    fnbr[2]=NULL;
    flC=vtxC->flist;
    while(flC!=NULL)
      {
      fC=flC->f_item;
      flA=vtxA->flist;
      while(flA!=NULL)
        {
	fA=flA->f_item;
	if(fC==fA && fC!=fptr){fnbr[2]=fC;break;}
	flA=flA->next;
	}
      if(fnbr[2]!=NULL)break;
      flC=flC->next;
      }
    
    fptr->fct[0]=fnbr[0];
    fptr->fct[1]=fnbr[1];
    fptr->fct[2]=fnbr[2];
    
    fptr=fptr->next;
    }
  //printf("Facet find neighbors success.\n");
  return(TRUE);
}

// Function to find a facet's neighbor that shares the two input vtxs
// Inputs:  fstart=pointer to start of list to search, finpt=facet we are trying to match to
//	vtx0=first vertex of common edge,  vtx1= second vertex of common edge
// Return:  fptr=pointer to neighboring facet
facet *facet_find_vtx_neighbor(facet *fstart, facet *finpt, vertex *vtx0, vertex *vtx1)
{
  int		found=0;
  vertex 	*vtxA,*vtxB,*vtxC;
  facet 	*fptr,*fngh,*fA,*fB,*fC;
  
  // validate inputs
  if(fstart==NULL || finpt==NULL)return(NULL);
  if(vtx0==NULL || vtx1==NULL)return(NULL);
  
  // loop thru facet linked list and find another facet with vtx in the same position
  fngh=NULL;
  fptr=fstart;
  while(fptr!=NULL)
    {
    if(fptr==finpt){fptr=fptr->next;continue;}
    found=0;
    vtxA=fptr->vtx[0]; vtxB=fptr->vtx[1]; vtxC=fptr->vtx[2];
    
    if(vertex_compare(vtx0,vtxA,TOLERANCE)==TRUE)found++;
    if(vertex_compare(vtx0,vtxB,TOLERANCE)==TRUE)found++;
    if(vertex_compare(vtx0,vtxC,TOLERANCE)==TRUE)found++;
    
    if(vertex_compare(vtx1,vtxA,TOLERANCE)==TRUE)found++;
    if(vertex_compare(vtx1,vtxB,TOLERANCE)==TRUE)found++;
    if(vertex_compare(vtx1,vtxC,TOLERANCE)==TRUE)found++;
    
    if(found==2){fngh=fptr;break;}
    
    fptr=fptr->next;
    }
    
  //printf("Facet find neighbors success.\n");
  return(fngh);
}

// Function to test if a point is within the XY bounds of a facet
// Return:  FALSE if point not within facet,  TRUE if within AND sets vtest->z to value at vtest->x/y
//
// This algorithm is credited to: https://wrf.ecse.rpi.edu//Research/Short_Notes/pnpoly.html
int facet_contains_point(facet *finpt, vertex *vtest)
{
  int		h,i,windnum=FALSE;
  vertex 	*vptr,*vnxt;
  float 	Ax,Ay,Az,Bx,By,Bz,Cx,Cy,Cz;
  float 	zpos,minz,maxz;
  
  if(finpt==NULL || vtest==NULL)return(FALSE);
  
  // loop through all three edges of the facet
  for(h=0;h<3;h++)							// loop thru all three vtxs
    {
    vptr=finpt->vtx[h];							// define first vtx of line segment
    i=h+1; if(i>2)i=0;
    vnxt=finpt->vtx[i];							// define second vtx of line segment

    // The magic test based on ray tracing to right and counting crossings - see website for details.
    // In basic words:  if one of the edge pts is above the test pt and the other is below, or vice versa, and
    // test pt x is less than the start of the edge in x plus the slope of the edge in x up to the test pt y,
    // then the ray to the right from the test pt must cross over that edge, so toggle the crossing counter.
    // An odd number of crossings (i.e. TRUE value) means the test pt is inside the facet.
    if( ((vptr->y > vtest->y) != (vnxt->y > vtest->y)) &&
      (vtest->x < ((vnxt->x-vptr->x)*(vtest->y-vptr->y)/(vnxt->y-vptr->y)+vptr->x)) )windnum=!windnum;

    }
  
  // if point is found inside, solve the determinant to find z value
  if(windnum==TRUE)
    {
    // use the facet normal as the definition of the plane 
    vtest->z=(1/finpt->unit_norm->z)*(finpt->unit_norm->x*finpt->vtx[0]->x+finpt->unit_norm->y*finpt->vtx[0]->y+
	     finpt->unit_norm->z*finpt->vtx[0]->z-finpt->unit_norm->x*vtest->x-finpt->unit_norm->y*vtest->y);

    // ensure calculated z value is within limits of the facet values
    minz=BUILD_TABLE_LEN_Z; maxz=0.0;
    for(h=0;h<3;h++)
      {
      if(finpt->vtx[h]->z < minz)minz=finpt->vtx[h]->z;
      if(finpt->vtx[h]->z > maxz)maxz=finpt->vtx[h]->z;
      }
    if(vtest->z < minz)vtest->z=minz;
    if(vtest->z > maxz)vtest->z=maxz;
    }

  //printf("testing facet %X  windnum=%d \n",finpt,windnum);
  
  return(windnum);
}

// Function to copy a facet - completely independent from original with new vtxs, normal, and params.
facet *facet_copy(facet *fptr)
{
  facet	*new_fptr;
  
  if(fptr==NULL)return(NULL);
  
  new_fptr=facet_make();
  if(new_fptr==NULL)return(NULL);
  
  new_fptr->vtx[0]=vertex_copy(fptr->vtx[0],NULL);
  new_fptr->vtx[1]=vertex_copy(fptr->vtx[1],NULL);
  new_fptr->vtx[2]=vertex_copy(fptr->vtx[2],NULL);

  new_fptr->unit_norm->x=fptr->unit_norm->x;				// just values... see facet_make()
  new_fptr->unit_norm->y=fptr->unit_norm->y;
  new_fptr->unit_norm->z=fptr->unit_norm->z;
  
  new_fptr->fct[0]=fptr->fct[0];
  new_fptr->fct[1]=fptr->fct[1];
  new_fptr->fct[2]=fptr->fct[2];
  
  new_fptr->member=fptr->member;
  new_fptr->attr=fptr->attr;
  new_fptr->supp=fptr->supp;
  new_fptr->status=fptr->status;
  new_fptr->area=fptr->area;
  new_fptr->next=NULL;
  
  return(new_fptr);
}
    
// Function to calculate a facet unit normal based on vertex sequence
int facet_unit_normal(facet *fptr)
{
  float 	r0,n01,n12,n20;
  vertex	*vtxA,*vtxB,*vtxC,*vtxN,*vtx_org;
  vector	*N;
  
  if(fptr==NULL)return(0);
  
  // at the expense of 3 vtxs, simplify the three facet corners to A, B, and C
  vtxA=vertex_make();
  vtxA->x=fptr->vtx[0]->x; vtxA->y=fptr->vtx[0]->y; vtxA->z=fptr->vtx[0]->z;
  vtxB=vertex_make();
  vtxB->x=fptr->vtx[1]->x; vtxB->y=fptr->vtx[1]->y; vtxB->z=fptr->vtx[1]->z;
  vtxC=vertex_make();
  vtxC->x=fptr->vtx[2]->x; vtxC->y=fptr->vtx[2]->y; vtxC->z=fptr->vtx[2]->z;
  
  // convert to true vector (to magnitude from position) making A the center pt
  vtxB->x-=vtxA->x;  vtxB->y-=vtxA->y;  vtxB->z-=vtxA->z;		
  vtxC->x-=vtxA->x;  vtxC->y-=vtxA->y;  vtxC->z-=vtxA->z;
  
  // calculate the cross product  N = B X C
  vtxN=vertex_make();
  vertex_crossproduct(vtxB,vtxC,vtxN);
  
  // normalize and save in facet
  vtx_org=vertex_make();
  N=vector_make(vtx_org,vtxN,0);
  vector_magnitude(N);
  fptr->unit_norm->x=vtxN->x/N->curlen;
  fptr->unit_norm->y=vtxN->y/N->curlen;
  fptr->unit_norm->z=vtxN->z/N->curlen;
  
  // free memory
  free(N); vector_mem--; N=NULL;
  free(vtx_org); vertex_mem--; vtx_org=NULL;
  free(vtxN); vertex_mem--; vtxN=NULL;
  free(vtxC); vertex_mem--; vtxC=NULL;
  free(vtxB); vertex_mem--; vtxB=NULL;
  free(vtxA); vertex_mem--; vtxA=NULL;
  
  // compare against neighbors to see if going same general direction
  if(fptr->fct[0]!=NULL && fptr->fct[1]!=NULL && fptr->fct[2]!=NULL)
    {
    if(fptr->fct[0]->unit_norm!=NULL && fptr->fct[1]->unit_norm!=NULL && fptr->fct[2]->unit_norm!=NULL)
      {
      // compare dot products
      // first determine which general direction the base facet's neighbors are going
      n01=vertex_dotproduct(fptr->fct[0]->unit_norm,fptr->fct[1]->unit_norm);
      n12=vertex_dotproduct(fptr->fct[1]->unit_norm,fptr->fct[2]->unit_norm);
      n20=vertex_dotproduct(fptr->fct[2]->unit_norm,fptr->fct[0]->unit_norm);
      
      // now determine which general direction the base facet is pointing relative to neighbors
      r0=vertex_dotproduct(fptr->unit_norm,fptr->fct[0]->unit_norm);
      if((r0*n01*n12*n20)<0)facet_flip_normal(fptr);
      }
    }
  
  return(1);
}

// Function to reverse the direction of a facet normal
int facet_flip_normal(facet *fptr)
{
  if(fptr==NULL)return(0);
  if(fptr->unit_norm==NULL)return(0);
  
  fptr->unit_norm->x *= (-1);
  fptr->unit_norm->y *= (-1);
  fptr->unit_norm->z *= (-1);
  
  return(1);
}

// Function to swap vertex values between two facets (i.e. maintains addresses)
int facet_swap_value(facet *fA, facet *fB)
{
  facet 	*ftemp;
  
  if(fA==NULL || fB==NULL)return(0);
  
  ftemp=facet_make();
  
  ftemp->vtx[0]=fA->vtx[0]; 
  ftemp->vtx[1]=fA->vtx[1]; 
  ftemp->vtx[2]=fA->vtx[2];
  
  fA->vtx[0]=fB->vtx[0];    
  fA->vtx[1]=fB->vtx[1];    
  fA->vtx[2]=fB->vtx[2];
  
  fB->vtx[0]=ftemp->vtx[0]; 
  fB->vtx[1]=ftemp->vtx[1]; 
  fB->vtx[2]=ftemp->vtx[2];
  
  free(ftemp);
  
  return(1);
}

// Function to check if two facets share a vertex
// inputs: fA=ptr to first facet, fB=pointer to other facet, vtx_test=typically a vertex of one of the two facets
// return: TRUE if they do, FALSE if not
int facet_share_vertex(facet *fA, facet *fB, vertex *vtx_test)
{
  int		h;
  int		fA_test=FALSE, fB_test=FALSE;
  
  if(fA==NULL || fB==NULL || vtx_test==NULL)return(FALSE);
  
  for(h=0;h<3;h++)
    {
    if(vertex_compare(fA->vtx[h],vtx_test,CLOSE_ENOUGH)==TRUE)fA_test=TRUE;
    }
    
  for(h=0;h<3;h++)
    {
    if(vertex_compare(fB->vtx[h],vtx_test,CLOSE_ENOUGH)==TRUE)fB_test=TRUE;
    }

  if(fA_test==TRUE && fB_test==TRUE)return(TRUE);
  return(FALSE);
}

// Function to check if two facets share an edge (a.k.a. vertex pair)
// returns NULL if not, or a vector between the two vtxs if shared
vector *facet_share_edge(facet *fA, facet *fB)
{
  int		share01,share12,share20;
  float 	tol;
  vector 	*vA,*vB,*vresult;
  
  if(fA==NULL || fB==NULL)return(0);
  
  share01=FALSE; share12=FALSE; share20=FALSE;
  tol=CLOSE_ENOUGH;
  
  // compare edge 0-1 of fA with each edge of fB
  vA=vector_make(fA->vtx[0],fA->vtx[1],0);
  vB=vector_make(fB->vtx[0],fB->vtx[1],0);
  if(vector_compare(vA,vB,tol)==TRUE)share01=TRUE;			// edge 0-1 of fB
  vB->tip=fB->vtx[1]; vB->tail=fB->vtx[2];
  if(vector_compare(vA,vB,tol)==TRUE)share01=TRUE;			// edge 1-2 of fB
  vB->tip=fB->vtx[2]; vB->tail=fB->vtx[0];
  if(vector_compare(vA,vB,tol)==TRUE)share01=TRUE;			// edge 2-0 of fB
  
  // compare edge 1-2 of fA with each edge of fB
  if(share01==FALSE)
    {
    vA->tip=fA->vtx[1]; vA->tail=fA->vtx[2];
    vB->tip=fB->vtx[0]; vB->tail=fB->vtx[1];
    if(vector_compare(vA,vB,tol)==TRUE)share12=TRUE;
    vB->tip=fB->vtx[1]; vB->tail=fB->vtx[2];
    if(vector_compare(vA,vB,tol)==TRUE)share12=TRUE;
    vB->tip=fB->vtx[2]; vB->tail=fB->vtx[0];
    if(vector_compare(vA,vB,tol)==TRUE)share12=TRUE;
    }
  
  // compare edge 2-0 of fA with each edge of fB
  if(share01==FALSE && share12==FALSE)
    {
    vA->tip=fA->vtx[2]; vA->tail=fA->vtx[0];
    vB->tip=fB->vtx[0]; vB->tail=fB->vtx[1];
    if(vector_compare(vA,vB,tol)==TRUE)share20=TRUE;
    vB->tip=fB->vtx[1]; vB->tail=fB->vtx[2];
    if(vector_compare(vA,vB,tol)==TRUE)share20=TRUE;
    vB->tip=fB->vtx[2]; vB->tail=fB->vtx[0];
    if(vector_compare(vA,vB,tol)==TRUE)share20=TRUE;
    }
  
  free(vA); vector_mem--;
  free(vB); vector_mem--;

  vresult=NULL;
  if(share01==TRUE)vresult=vector_make(fA->vtx[0],fA->vtx[1],0);
  if(share12==TRUE)vresult=vector_make(fA->vtx[1],fA->vtx[2],0);
  if(share20==TRUE)vresult=vector_make(fA->vtx[2],fA->vtx[0],0);
  
  return(vresult);
}

// Function to subdivide a facet into a group of smaller facets.
// this works by subdividing each side, then connecting them to form four smaller
// facets until they are below the input length threshold.
// Inputs: mptr=model, finp=facet to divide, edgelen=max distance between vtxs
// Return: NULL=failed, Matching ptr to finp=success (with new facets in between)
facet *facet_subdivide(model *mptr, facet *finp, float edgelen)
{
  float 	dx,dy,dz,len0,len1,len2,maxlen;
  vertex 	*vA,*vB,*vC,*v01,*v12,*v20,*vadd,*vdel;
  facet 	*fA,*fB,*fC,*fcptr,*fptr;
  
  // verify input
  if(mptr==NULL || finp==NULL)return(NULL);
  //printf("  facet subdivide:  entry\n");
  
  // init.  put the input facet into a facet list.  the facet list will contain all the newly
  // created subfacets that are the product of subdividing this one.
  fA=NULL; fB=NULL; fC=NULL;
  
  // loop thru this facet and its results until all results are smaller than input threshold
  fptr=finp;
  vA=fptr->vtx[0]; vB=fptr->vtx[1]; vC=fptr->vtx[2];
  
  // determine length of longest side of facet
  dx=vB->x-vA->x;  dy=vB->y-vA->y; dz=vB->z-vA->z;
  len0=sqrt(dx*dx+dy*dy+dz*dz);
  dx=vC->x-vB->x;  dy=vC->y-vB->y; dz=vC->z-vB->z;
  len1=sqrt(dx*dx+dy*dy+dz*dz);
  dx=vA->x-vC->x;  dy=vA->y-vC->y; dz=vA->z-vC->z;
  len2=sqrt(dx*dx+dy*dy+dz*dz);
  maxlen=len0;
  if(len1>maxlen)maxlen=len1;
  if(len2>maxlen)maxlen=len2;
  
  // loop thru ever growing collection of facets until all have sides shorter than edgelen
  while(maxlen>edgelen)
    {
    // subdivide each side into two sections with a new vtx in the middle of each
    v01=vertex_make();
    v01->x=(vA->x + vB->x) / 2.0;
    v01->y=(vA->y + vB->y) / 2.0;
    v01->z=(vA->z + vB->z) / 2.0;
    v01->supp=vA->supp;
    v01->attr=vA->attr;
    
    v12=vertex_make();
    v12->x=(vB->x + vC->x) / 2.0;
    v12->y=(vB->y + vC->y) / 2.0;
    v12->z=(vB->z + vC->z) / 2.0;
    v12->supp=vB->supp;
    v12->attr=vB->attr;
    
    v20=vertex_make();
    v20->x=(vC->x + vA->x) / 2.0;
    v20->y=(vC->y + vA->y) / 2.0;
    v20->z=(vC->z + vA->z) / 2.0;
    v20->supp=vC->supp;
    v20->attr=vC->attr;
  
    // make three new facets that cover each of the corners of the original facet.
    // don't use facet_copy here... creates redundent vtxs that get lost in memory.
    fA=facet_make();
    fA->vtx[0]=vA;
    fA->vtx[1]=v01;
    fA->vtx[2]=v20;
    fA->unit_norm->x=fptr->unit_norm->x;
    fA->unit_norm->y=fptr->unit_norm->y;
    fA->unit_norm->z=fptr->unit_norm->z;
    fA->member=fptr->member;
    fA->attr=fptr->attr;
    fA->status=fptr->status;
    fA->area=fptr->area/4.0;
    
    fB=facet_make();
    fB->vtx[0]=vertex_copy(v01,NULL);
    fB->vtx[1]=vB;
    fB->vtx[2]=v12;
    fB->unit_norm->x=fptr->unit_norm->x;
    fB->unit_norm->y=fptr->unit_norm->y;
    fB->unit_norm->z=fptr->unit_norm->z;
    fB->member=fptr->member;
    fB->attr=fptr->attr;
    fB->status=fptr->status;
    fB->area=fptr->area/4.0;
  
    fC=facet_make();
    fC->vtx[0]=vertex_copy(v12,NULL);
    fC->vtx[1]=vC;
    fC->vtx[2]=vertex_copy(v20,NULL);
    fC->unit_norm->x=fptr->unit_norm->x;
    fC->unit_norm->y=fptr->unit_norm->y;
    fC->unit_norm->z=fptr->unit_norm->z;
    fC->member=fptr->member;
    fC->attr=fptr->attr;
    fC->status=fptr->status;
    fC->area=fptr->area/4.0;
  
    // redefine fptr to be the center facet
    fptr->vtx[0]=vertex_copy(v01,NULL);	
    fptr->vtx[1]=vertex_copy(v12,NULL);
    fptr->vtx[2]=vertex_copy(v20,NULL);
    fptr->area=fptr->area/4.0;
  
    // create linkage to insert new facets between fptr and fptr->next
    fC->next=fptr->next;  
    fptr->next=fA;
    fA->next=fB;
    fB->next=fC;
    
    // at this point we've subdived one facet down to four.  now loop thru the entire
    // list again further subdividing if necessary.
    maxlen=edgelen*2;							// re-init as large value
    fptr=finp;								// reset to start of facet list
    while(fptr!=NULL)							// loop thru whole facet list
      {
      vA=fptr->vtx[0]; vB=fptr->vtx[1]; vC=fptr->vtx[2];
      
      // determine length of longest side of facet
      dx=vB->x-vA->x;  dy=vB->y-vA->y; dz=vB->z-vA->z;
      len0=sqrt(dx*dx+dy*dy+dz*dz);
      dx=vC->x-vB->x;  dy=vC->y-vB->y; dz=vC->z-vB->z;
      len1=sqrt(dx*dx+dy*dy+dz*dz);
      dx=vA->x-vC->x;  dy=vA->y-vC->y; dz=vA->z-vC->z;
      len2=sqrt(dx*dx+dy*dy+dz*dz);
      maxlen=len0;
      if(len1>maxlen)maxlen=len1;
      if(len2>maxlen)maxlen=len2;
      
      if(maxlen>edgelen)break;						// if longer... then process this one
      fptr=fptr->next;
      }
    }

  //printf("  facet subdivide:  exit\n");
  fptr=finp;
  return(fptr);
}

/*
// Function to subdivide a facet into a group of smaller facets.
// this works by subdividing each side, then connecting them to form four smaller
// facets until they are below the input length threshold.
// Inputs: mptr=model, finp=facet to divide, edgelen=max distance between vtxs
// Return: NULL=failed, Matching ptr to finp=success (with new facets in between)
facet *facet_subdivide(model *mptr, facet *finp, float edgelen)
{
  float 	dx,dy,dz,len0,len1,len2,maxlen;
  vertex 	*vA,*vB,*vC,*v01,*v12,*v20,*vadd,*vdel;
  facet 	*fA,*fB,*fC,*fcptr,*fptr;
  facet_list	*pfptr,*plist;
  
  // verify input
  if(mptr==NULL || finp==NULL)return(NULL);
  
  // init.  put the input facet into a facet list.  the facet list will contain all the newly
  // created subfacets that are the product of subdividing this one.
  fA=NULL; fB=NULL; fC=NULL;
  plist=NULL;
  plist=facet_list_manager(plist,finp,ACTION_ADD);
  
  // loop thru this facet and its results until all results are smaller than input threshold
  pfptr=plist;
  while(pfptr!=NULL)
    {
    fptr=pfptr->f_item;
    vA=fptr->vtx[0]; vB=fptr->vtx[1]; vC=fptr->vtx[2];
    
    // determine length of longest side of facet
    dx=vB->x-vA->x;  dy=vB->y-vA->y; dz=vB->z-vA->z;
    len0=sqrt(dx*dx+dy*dy+dz*dz);
    dx=vC->x-vB->x;  dy=vC->y-vB->y; dz=vC->z-vB->z;
    len1=sqrt(dx*dx+dy*dy+dz*dz);
    dx=vA->x-vC->x;  dy=vA->y-vC->y; dz=vA->z-vC->z;
    len2=sqrt(dx*dx+dy*dy+dz*dz);
    maxlen=len0;
    if(len1>maxlen)maxlen=len1;
    if(len2>maxlen)maxlen=len2;
    
    // when all sides of all facets are shorter than input threshold, it will exit here
    if(maxlen<=edgelen){pfptr=pfptr->next;continue;}		

    // subdivide each side into two sections with a new vtx in the middle of each
    v01=vertex_make();
    v01->x=(vA->x + vB->x) / 2.0;
    v01->y=(vA->y + vB->y) / 2.0;
    v01->z=(vA->z + vB->z) / 2.0;
    v01->supp=vA->supp;
    v01->attr=vA->attr;
    vadd=vertex_unique_insert(mptr,mptr->vertex_last[SUPPORT],v01,SUPPORT,CLOSE_ENOUGH);
    if(vadd!=v01)
      {
      vdel=v01;
      v01=vadd;
      free(vdel); vertex_mem--;
      }
    
    v12=vertex_make();
    v12->x=(vB->x + vC->x) / 2.0;
    v12->y=(vB->y + vC->y) / 2.0;
    v12->z=(vB->z + vC->z) / 2.0;
    v12->supp=vB->supp;
    v12->attr=vB->attr;
    vadd=vertex_unique_insert(mptr,mptr->vertex_last[SUPPORT],v12,SUPPORT,CLOSE_ENOUGH);
    if(vadd!=v12)
      {
      vdel=v12;
      v12=vadd;
      free(vdel); vertex_mem--;
      }
    
    v20=vertex_make();
    v20->x=(vC->x + vA->x) / 2.0;
    v20->y=(vC->y + vA->y) / 2.0;
    v20->z=(vC->z + vA->z) / 2.0;
    v20->supp=vC->supp;
    v20->attr=vC->attr;
    vadd=vertex_unique_insert(mptr,mptr->vertex_last[SUPPORT],v20,SUPPORT,CLOSE_ENOUGH);
    if(vadd!=v20)
      {
      vdel=v20;
      v20=vadd;
      free(vdel); vertex_mem--;
      }
  
    // make three new facets that cover each of the corners of the original facet.
    // don't use facet_copy here... creates redundent vtxs that get lost in memory.
    fA=facet_make();
    fA->vtx[0]=vA;
    fA->vtx[1]=v01;
    fA->vtx[2]=v20;
    fA->unit_norm->x=fptr->unit_norm->x;
    fA->unit_norm->y=fptr->unit_norm->y;
    fA->unit_norm->z=fptr->unit_norm->z;
    fA->attr=fptr->attr;
    fA->status=fptr->status;
    fA->area=fptr->area/4.0;
    plist=facet_list_manager(plist,fA,ACTION_ADD);
    
    fB=facet_make();
    fB->vtx[0]=v01;
    fB->vtx[1]=vB;
    fB->vtx[2]=v12;
    fB->unit_norm->x=fptr->unit_norm->x;
    fB->unit_norm->y=fptr->unit_norm->y;
    fB->unit_norm->z=fptr->unit_norm->z;
    fB->attr=fptr->attr;
    fB->status=fptr->status;
    fB->area=fptr->area/4.0;
    plist=facet_list_manager(plist,fB,ACTION_ADD);
  
    fC=facet_make();
    fC->vtx[0]=v12;
    fC->vtx[1]=vC;
    fC->vtx[2]=v20;
    fC->unit_norm->x=fptr->unit_norm->x;
    fC->unit_norm->y=fptr->unit_norm->y;
    fC->unit_norm->z=fptr->unit_norm->z;
    fC->attr=fptr->attr;
    fC->status=fptr->status;
    fC->area=fptr->area/4.0;
    plist=facet_list_manager(plist,fC,ACTION_ADD);
  
    // remove fptr from the old vtxs' facet lists
    //vA->flist=facet_list_manager(vA->flist,fptr,ACTION_DELETE);
    //vB->flist=facet_list_manager(vB->flist,fptr,ACTION_DELETE);
    //vC->flist=facet_list_manager(vC->flist,fptr,ACTION_DELETE);
    
    // redefine fptr to be the center facet
    fptr->vtx[0]=v01;							
    fptr->vtx[1]=v12;
    fptr->vtx[2]=v20;
    fptr->area=fptr->area/4.0;
  
    // create linkage to insert new facets between fptr and fptr->next
    fC->next=fptr->next;  
    fptr->next=fA;
    fA->next=fB;
    fB->next=fC;
  
    // add facets to the new vtxs' facet lists
    v01->flist=facet_list_manager(v01->flist,fptr,ACTION_ADD);
    v12->flist=facet_list_manager(v12->flist,fptr,ACTION_ADD);
    v20->flist=facet_list_manager(v20->flist,fptr,ACTION_ADD);
    
    vA->flist=facet_list_manager(vA->flist,fA,ACTION_ADD);
    v01->flist=facet_list_manager(v01->flist,fA,ACTION_ADD);
    v20->flist=facet_list_manager(v20->flist,fA,ACTION_ADD);
  
    vB->flist=facet_list_manager(vB->flist,fB,ACTION_ADD);
    v01->flist=facet_list_manager(v01->flist,fB,ACTION_ADD);
    v12->flist=facet_list_manager(v12->flist,fB,ACTION_ADD);
  
    vC->flist=facet_list_manager(vC->flist,fC,ACTION_ADD);
    v12->flist=facet_list_manager(v12->flist,fC,ACTION_ADD);
    v20->flist=facet_list_manager(v20->flist,fC,ACTION_ADD);
    
    // update facets' neighbors lists
    //fA->fct[0]=fptr->fct[0]; fA->fct[1]=fptr; fA->fct[2]=fptr->fct[2];
    //fB->fct[0]=fptr->fct[0]; fB->fct[1]=fptr->fct[1]; fB->fct[2]=fptr;
    //fC->fct[0]=fptr; fC->fct[1]=fptr->fct[1]; fC->fct[2]=fptr->fct[2];
    //fptr->fct[0]=fA; fptr->fct[1]=fB; fptr->fct[2]=fC;
    
    //fptr->attr=10;
    //fA->attr=11;
    //fB->attr=12;
    //fC->attr=13;
    
    // start over at beginning of list after each subdivision
    pfptr=plist;							
    }
  
  // get rid of the list to free up memory before leaving
  plist=facet_list_manager(plist,NULL,ACTION_CLEAR);
  
  return(fptr);
}
*/



// Function to create a facet list element
facet_list *facet_list_make(facet *finpt)
{
  facet_list	*fl_ptr;
  
  fl_ptr=(facet_list *)malloc(sizeof(facet_list));
  if(fl_ptr!=NULL)
    {
    fl_ptr->f_item=finpt;
    fl_ptr->next=NULL;
    }
  return(fl_ptr);
}
  
// Function to manage facet lists - typically used as a "user pick list", but can be used for other things
// actions:  ADD, DELETE, CLEAR
facet_list *facet_list_manager(facet_list *flist, facet *finpt, int action)
{
  facet		*fptr;
  facet_list	*fl_ptr,*fl_pre,*fl_del;
  
  // note input checking is dependent on the requested action
  if(action==ACTION_ADD && finpt==NULL)return(flist);
  if(action==ACTION_DELETE && (flist==NULL || finpt==NULL))return(flist);
  if(action==ACTION_CLEAR && flist==NULL)return(flist);
  
  if(action==ACTION_ADD)
    {
    // add new facet to front of list and update ptr to head of list
    fl_ptr=facet_list_make(finpt);					// create a new element
    fl_ptr->f_item=finpt;						// defines its contents
    fl_ptr->next=flist;
    flist=fl_ptr;
    
    /*  
    // scan existing list to ensure the input facet is not already on the list
    fl_pre=NULL;
    fl_ptr=flist;
    while(fl_ptr!=NULL)
      {
      if(fl_ptr->f_item==finpt)break;
      fl_pre=fl_ptr;
      fl_ptr=fl_ptr->next;
      }
    // if already on list, then remove it from list (i.e. user selected it again to remove it)
    if(fl_ptr!=NULL)
      {
      fl_del=fl_ptr;							// save address of element to delete
      if(fl_pre==NULL){flist=fl_ptr->next;}				// if first element in list...
      else {fl_pre->next=fl_ptr->next;}					// but if NOT first element... skip over element to be deleted
      free(fl_del);							// finally delete the target element
      }
    // if NOT already on list, create a new element and add to end of list
    else
      {
      fl_ptr=facet_list_make(finpt);					// create a new element
      fl_ptr->f_item=finpt;						// defines its contents
      fl_ptr->next=NULL;
      if(flist==NULL)flist=fl_ptr;
      if(fl_pre!=NULL)fl_pre->next=fl_ptr;				// add it to the end of the list
      }
    */
    }

  if(action==ACTION_DELETE)
    {
    fl_pre=NULL;
    fl_ptr=flist;
    while(fl_ptr!=NULL)							// loop thru list to find pre node to delete node
      {
      if(fl_ptr->f_item==finpt)break;
      fl_pre=fl_ptr;
      fl_ptr=fl_ptr->next;
      }
    if(fl_ptr!=NULL)							// if the delete node was found...
      {
      fl_del=fl_ptr;							// save address in temp ptr
      if(fl_pre==NULL)							// if head node, redefine head to be next node (which may be NULL if only one node in list)
        {flist=fl_ptr->next;}
      else 
        {fl_pre->next=fl_ptr->next;}					// otherwise skip over node to be deleted
      free(fl_del);
      }
    }
    
  if(action==ACTION_CLEAR)
    {
    fl_ptr=flist;
    while(fl_ptr!=NULL)
      {
      fl_del=fl_ptr;
      fl_ptr=fl_ptr->next;
      free(fl_del);
      }
    flist=NULL;
    }
    
  return(flist);
}  



// Function to allocate space for a single link list element of type model
model *model_make(void)
{
  int		i,h,j,slot;
  vertex	*vptr;
  edge 		*eptr;
  model		*mptr;
	
  // find space in memory
  mptr=(model *)malloc(sizeof(model));
  if(mptr==NULL)return(NULL);
	
  // initialize new model parameters
  mptr->model_ID=(-1);
  for(i=MODEL;i<MAX_MDL_TYPES;i++)					// geometry oriented objects of model
    {
    mptr->vertex_index[i]=NULL;
      
    mptr->vertex_qty[i]=0;
    mptr->vertex_first[i]=NULL;
    mptr->vertex_last[i]=NULL;

    mptr->edge_qty[i]=0;
    mptr->edge_first[i]=NULL;
    mptr->edge_last[i]=NULL;

    mptr->facet_qty[i]=0;
    mptr->facet_first[i]=NULL;
    mptr->facet_last[i]=NULL;

    mptr->xmin[i]=INT_MAX_VAL;mptr->ymin[i]=INT_MAX_VAL;mptr->zmin[i]=INT_MAX_VAL;
    mptr->xmax[i]=(INT_MIN_VAL);mptr->ymax[i]=(INT_MIN_VAL);mptr->zmax[i]=(INT_MIN_VAL);
    mptr->xorg[i]=0;mptr->yorg[i]=0;mptr->zorg[i]=0;
    mptr->xoff[i]=0;mptr->yoff[i]=0;mptr->zoff[i]=0;
    mptr->xrpr[i]=0;mptr->yrpr[i]=0;mptr->zrpr[i]=0;
    mptr->xrot[i]=0;mptr->yrot[i]=0;mptr->zrot[i]=0;
    mptr->xscl[i]=1.0;mptr->yscl[i]=1.0;mptr->zscl[i]=1.0;
    mptr->xspr[i]=1.0;mptr->yspr[i]=1.0;mptr->zspr[i]=1.0;
    mptr->xmir[i]=0;mptr->ymir[i]=0;mptr->zmir[i]=0;
    }

  mptr->reslice_flag=TRUE;
  mptr->MRedraw_flag=TRUE;
  mptr->silho_edges=NULL;
  mptr->patch_model=NULL;
  mptr->patch_first_upr=NULL;
  mptr->patch_first_lwr=NULL;
  
  mptr->g_img_mdl_buff=NULL;
  mptr->mdl_pixels=NULL;
  mptr->g_img_act_buff=NULL;
  mptr->act_pixels=NULL;
  mptr->pix_size=0.28;
  mptr->pix_increment=3;
  mptr->show_image=TRUE;
  mptr->image_invert=FALSE;
  mptr->grayscale_mode=0;
  mptr->grayscale_zdelta=5.0;
  mptr->grayscale_velmin=5.0;
  
  for(slot=0;slot<MAX_TOOLS;slot++)					// tool oriented objects of model
    {
    mptr->slice_qty[slot]=0;
    mptr->slice_first[slot]=NULL;
    mptr->slice_last[slot]=NULL;
    }
  mptr->oper_list=NULL;
  mptr->base_layer=NULL;
  mptr->plat1_layer=NULL;
  mptr->plat2_layer=NULL;
  mptr->ghost_slice=NULL;
  mptr->geom_type=0;
  mptr->input_type=0;
  mptr->error_status=0;
  mptr->total_copies=1;
  mptr->xcopies=1;
  mptr->ycopies=1;
  mptr->xstp=5.0;
  mptr->ystp=5.0;
  mptr->current_z=0.0;
  mptr->base_lyr_zoff=0.0;
  mptr->target_x_margin=cnc_x_margin;
  mptr->target_y_margin=cnc_y_margin;
  mptr->target_z_height=cnc_z_height;
  mptr->mdl_has_target=FALSE;
  mptr->vase_mode=FALSE;
  mptr->btm_co=TRUE;
  mptr->top_co=TRUE;
  mptr->internal_spt=FALSE;
  mptr->set_wall_thk=0.0;
  mptr->set_fill_density=0.5;
  mptr->fidelity_mode=FALSE;
  mptr->support_gap=0.5;
  mptr->slice_thick=0.5;
  mptr->facet_prox_tol=0.0;
  mptr->time_estimate=0.0;
  mptr->vert_surf_val=0;
  mptr->botm_surf_val=0;
  mptr->lwco_surf_val=0;
  mptr->upco_surf_val=0;
  memset(mptr->model_file,0,sizeof(mptr->model_file));
  memset(mptr->cmt,0,sizeof(mptr->cmt));
  mptr->next=NULL;
  model_mem++;

  return(mptr);
}

// Function to insert new model into tail of linked list
int model_insert(model *modelnew)
{
  model 	*mptr;

  if(job.model_first==NULL)					// special case of first model in linked list
    {
    job.model_first=modelnew;
    job.model_count=0;
    }
  else
    {
    mptr=job.model_first;
    while(mptr->next!=NULL)
      {
      mptr=mptr->next;
      }
    mptr->next=modelnew;
    }
  job.model_count++;
  modelnew->model_ID=job.model_count;
  modelnew->next=NULL;	
  return(1);							
}

// Function to delete a model from the job list and out of memory
// Note this will delete all elements under the model structure.
int model_delete(model *mdl_del)
{
  int		i;
  vertex	*ltvdel,*mvptr,*svptr;
  edge		*eptr,*edel;
  facet 	*ltfptr,*ltfdel,*fptr;
  polygon	*pptr,*pdel;
  patch 	*patptr,*patdel;
  model 	*mptr;
  genericlist	*aptr,*dptr;

  if(mdl_del==NULL)return(0);
  //printf("\nModel Delete Entry:  vtx=%ld  vec=%ld \n",vertex_mem,vector_mem);

  // test if first model in list
  if(mdl_del==job.model_first)
    {
    job.model_first=mdl_del->next;					// redefine first node into list
    }
  else 
    {
    mptr=job.model_first;
    while(mptr!=NULL)							// search list for link to element to be deleted
      {
      if(mptr->next==mdl_del)break;					// break at element where it is referenced
      mptr=mptr->next;							// move onto next element in list
      }
    if(mptr==NULL)return(0);						// if not found in list... exit with failure
    mptr->next=mdl_del->next;						// skip list over delete model
    }
  //active_model=job.model_first;
    
  // resequence the model IDs still in the job
  job.model_count=0;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    job.model_count++;
    mptr->model_ID=job.model_count;
    mptr=mptr->next;
    }

  // at this point we have adjusted the job's model list, but we now need to clear out all the data/memory
  // associated with the model to be deleted

  // delete view specific data
  edge_purge(mdl_del->silho_edges);

  // delete orientation specific data - internal facets, support facets, slices, patches, images
  model_purge(mdl_del);

  // free the model's structured geometery
  printf("   claearing geometry data ... \n");
  if(mdl_del->input_type==STL)
    {
    printf("\n    Facet Purge Entry:  facet=%ld \n",facet_mem);
    facet_purge(mdl_del->facet_first[MODEL]);
    mdl_del->facet_first[MODEL]=NULL;  mdl_del->facet_last[MODEL]=NULL;
    mdl_del->facet_qty[MODEL]=0;
    printf("    Facet Purge Exit:   facet=%ld \n",facet_mem);
    
    printf("\n    Edge Purge Entry:  edge=%ld \n",edge_mem);
    edge_purge(mdl_del->edge_first[MODEL]);
    mdl_del->edge_first[MODEL]=NULL; mdl_del->edge_last[MODEL]=NULL;
    mdl_del->edge_qty[MODEL]=0;
    printf("    Edge Purge Exit:  edge=%ld \n",edge_mem);

    printf("\n    Vertex Purge Entry:  vertex=%ld \n",vertex_mem);
    vertex_purge(mdl_del->vertex_first[MODEL]);
    mdl_del->vertex_first[MODEL]=NULL; mdl_del->vertex_last[MODEL]=NULL;
    mdl_del->vertex_qty[MODEL]=0;
    printf("    Vertex Purge Exit:   vertex=%ld \n",vertex_mem);
    
    printf("\n    Vertex Index Purge Entry:  vertex_index=%X \n",mdl_del->vertex_index[MODEL]);
    vertex_purge(mdl_del->vertex_index[MODEL]);
    mdl_del->vertex_index[MODEL]=NULL;
    printf("     Vertex Index Purge Exit. \n");
    }
    
  // free the model itself
  free(mdl_del); mdl_del=NULL;
  
  // if nothing else loaded, then reset some viewing params
  if(job.model_count==0)
    {
    autoplacement_flag=TRUE;
    set_center_build_job=TRUE;
    set_auto_zoom=TRUE;
    MVview_scale=1.2; 
    MVgxoffset=425; 
    MVgyoffset=375; 
    MVdisp_spin=(-37*PI/180); 
    MVdisp_tilt=(210*PI/180);
    LVview_scale=1.00;
    LVgxoffset=155;
    LVgyoffset=(-490);
    }
  
  //printf("\nModel Delete Exit:  count=%d  vtx=%ld  vec=%ld \n",job.model_count,vertex_mem,vector_mem);

  return(1);
}

// Function to purge a model of its slice specific data (supports, slices, etc.)
// this function will NOT clear:  STL/AMF geometry or images
int model_purge(model *mdl_del)
{
  int		i;
  patch 	*patptr,*patdel;
  genericlist	*aptr,*dptr;
  
  // validate input
  if(mdl_del==NULL)return(0);
  
  // DEBUG
  if(mdl_del->input_type==GERBER)return(0);
  
  //printf("Model Purge:  entry\n");

  // free slice data held for each operation/tool/line_type
  // this will free straight skeleton and base layer data as well
  printf("   clearing orientation dependent slice data ... \n");
  aptr=mdl_del->oper_list;
  while(aptr!=NULL)
    {
    slice_purge(mdl_del,aptr->ID);
    dptr=aptr;
    aptr=aptr->next;
    free(dptr);
    }
  mdl_del->oper_list=NULL;
  
  // clear existing model and support patches if they exist
  printf("   clearing model patch data ... \n");
  if(mdl_del->patch_model!=NULL)
    {
    patptr=mdl_del->patch_model;
    while(patptr!=NULL)
      {
      patdel=patptr;
      patptr=patptr->next;
      patch_delete(patdel);
      }
    mdl_del->patch_model=NULL;
    }
  printf("   clearing lower patch data ... \n");
  if(mdl_del->patch_first_lwr!=NULL)
    {
    patptr=mdl_del->patch_first_lwr;
    while(patptr!=NULL)
      {
      patdel=patptr;
      patptr=patptr->next;
      patch_delete(patdel);
      }
    mdl_del->patch_first_lwr=NULL;
    }
  printf("   clearing upper patch data ... \n");
  if(mdl_del->patch_first_upr!=NULL)
    {
    patptr=mdl_del->patch_first_upr;
    while(patptr!=NULL)
      {
      patdel=patptr;
      patptr=patptr->next;
      patch_delete(patdel);
      }
    mdl_del->patch_first_upr=NULL;
    }

  // free the model's internal and support geometery
  printf("   claearing orientation dependent geometry data ... \n");
  if(mdl_del->input_type==STL)
    {
    for(i=SUPPORT;i<MAX_MDL_TYPES;i++)					// 2=support, 3=internal
      {
      if(i==SUPPORT)printf("   supports...");
      if(i==INTERNAL)printf("   internal...");
      if(i==TARGET)printf("   target...");
      facet_purge(mdl_del->facet_first[i]);
      mdl_del->facet_first[i]=NULL;  mdl_del->facet_last[i]=NULL;
      mdl_del->facet_qty[i]=0;
      
      edge_purge(mdl_del->edge_first[i]);
      mdl_del->edge_first[i]=NULL; mdl_del->edge_last[i]=NULL;
      mdl_del->edge_qty[i]=0;
  
      vertex_purge(mdl_del->vertex_first[i]);
      mdl_del->vertex_first[i]=NULL; mdl_del->vertex_last[i]=NULL;
      mdl_del->vertex_qty[i]=0;
      printf(" done.\n");
      }
    }

  mdl_del->reslice_flag=TRUE;
  //printf("Model Purge:  exit\n");
  return(1);
}

// Function to return a pointer to the model that matches the number sequence from when it was loaded
// Input:  mnum = the sequence number in which the model was loaded (i.e. first model, 2nd model, etc.)
// Return: mptr = pointer to the actual model
model *model_get_pointer(int mnum)
{
  int	mctr=0;
  model	*mptr;
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mctr==mnum)break;
    mptr=mptr->next;
    mctr++;
    }
    
  return(mptr);
}

// Function to return tool/slot for specific operation on a model.  Recall, models have a list of operations the user
// selects to build them.  Those operations are selected from the tool operations made available thru the TOOLS.XML file.
// Input:  op_id = operation ID, such as OP_ADD_MODEL_MATERIAL etc.
// Return: slot ID which is set to perform this operation
int model_get_slot(model *mptr, int op_id)
{
  int		slot;
  genericlist	*aptr;
  operation 	*optr;
  
  // validate input
  if(mptr==NULL)return(-1);
  if(op_id<0 || op_id>=MAX_OP_TYPES)return(-1);
  
  aptr=mptr->oper_list;							// look thru all operations called out by this model
  while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation name
    {
    slot=aptr->ID;							// get slot (aka tool) for this operation
    if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}		// make sure valid array value
    optr=Tool[slot].oper_first;						// look thru all operations this tool can perform
    while(optr!=NULL)
      {
      if(optr->ID==op_id)						// if this operation matches our request...
        {
	if(strstr(aptr->name,optr->name)!=NULL)return(slot);		// ... and if model operation = tool operation then match is found
	}
      optr=optr->next;							// move onto next tool operation
      }
    aptr=aptr->next;							// move onto next model operation
    }
  
  return(-1);								// return "match not found"
}

// Function to dump model information to the console for debugging
int model_dump(model *mptr)
{
  int	i,slot;
  
  printf("MODEL DUMP of %X ---------------------------------------------\n",mptr);
  printf("ID..................: %d \n",mptr->model_ID);
  printf("Name of file........: %s \n",mptr->model_file);
  printf("Input format type...: %d \n",mptr->input_type);
  printf("Input geometry type.: %d \n",mptr->geom_type);
  printf("Slice thickenss.....: %d \n",mptr->slice_thick);
  printf("Current z level.....: %d \n",mptr->current_z);
  for(i=MODEL;i<MAX_MDL_TYPES;i++)
    {
    if(i==MODEL)printf("\nGeometry type = MODEL\n");
    if(i==SUPPORT)printf("\nGeometry type = SUPPORT\n");
    if(i==INTERNAL)printf("\nGeometry type = INTERNAL\n");
    if(i==TARGET)printf("\nGeometry type = TARGET\n");
    printf("  Vertex qty..........: %d \n",mptr->vertex_qty[i]);
    printf("  Vertex first pointer: %p \n",mptr->vertex_first[i]);
    printf("  Vertex last pointer.: %p \n",mptr->vertex_last[i]);
    printf("  Edge qty............: %d \n",mptr->edge_qty[i]);
    printf("  Edge first pointer..: %p \n",mptr->edge_first[i]);
    printf("  Edge last pointer...: %p \n",mptr->edge_last[i]);
    printf("  Facet qty...........: %d \n",mptr->facet_qty[i]);
    printf("  Facet first pointer.: %p \n",mptr->facet_first[i]);
    printf("  Facet last pointer..: %p \n",mptr->facet_last[i]);
    printf("  xmin=%6.3f   ymin=%6.3f   zmin=%6.3f \n",mptr->xmin[i],mptr->ymin[i],mptr->zmin[i]);
    printf("  xmax=%6.3f   ymax=%6.3f   zmax=%6.3f \n",mptr->xmax[i],mptr->ymax[i],mptr->zmax[i]);
    printf("  xoff=%6.3f   yoff=%6.3f   zoff=%6.3f \n",mptr->xoff[i],mptr->yoff[i],mptr->zoff[i]);
    printf("  xorg=%6.3f   yorg=%6.3f   zorg=%6.3f \n",mptr->xorg[i],mptr->yorg[i],mptr->zorg[i]);
    }

  printf("Oper list pointer...: %p \n",mptr->oper_list);
  printf("Next model pointer.: %p \n",mptr->next);
  printf("\nJob Dims:\n");
  printf("  XMin=%6.3f   YMin=%6.3f   ZMin=%6.3f \n",XMin,YMin,ZMin);
  printf("  XMax=%6.3f   YMax=%6.3f   ZMax=%6.3f \n\n",XMax,YMax,ZMax);
  printf("--------------------------------------------------------------\n");
  while(!kbhit());
  
  return(1);
  
}

// Function to check if a particular line type is active in a model's operation list
// note:  linetypes report to materials which report to tools
int model_linetype_active(model *mptr, int lineType)
{
  int		slot,lt_check=FALSE;
  operation	*optr;
  genericlist	*lptr,*aptr;
  
  aptr=mptr->oper_list;							// start with first operation for this model
  while(aptr!=NULL)							// loop thru all of this model's operations
    {
    slot=aptr->ID;							// get tool that will perform this operation
    if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}		// check if a viable tool
    if(Tool[slot].state<TL_LOADED)return(FALSE);
    optr=Tool[slot].oper_first;						// start with first operation this tool can perform
    while(optr!=NULL)							// loop thru all of this tool's available operations
      {
      if(strstr(aptr->name,optr->name)!=NULL)				// if this model's operation is being performed by this tool...
        {
	lptr=optr->lt_seq;						// start with the first line type in this opereration
	while(lptr!=NULL)						// loop thru all line types of this operation
	  {
	  if(lptr->ID==lineType)return(TRUE);				// if a match is found...
	  lptr=lptr->next;						// otherwise, move onto next line type
	  }
	}
      optr=optr->next;
      }
    aptr=aptr->next;
    }
    
  return(FALSE);
}

// Function to mirror the model by translating vertex coordinates
// assume 0/0/0 establish the coordinate mirror planes
// this function will just flip the sign of the coordinate being mirrored then restablish max/min bounds
// bringing the model back into the first quadrant and onto the build table
int model_mirror(model *mptr)
{
  int		mtyp=MODEL;
  float 	xc,yc,zc;
  vertex	*vptr;
  int		xpix,xdest,ypix,ydest;
  GdkPixbuf	*g_img_copy_buff;
  int		copy_n_ch;
  int		copy_cspace;
  int		copy_alpha;
  int		copy_bits_per_pixel;
  int 		copy_height;
  int 		copy_width;
  int 		copy_rowstride;
  guchar 	*pix1,*pix2,*copy_pixels;
  
  //printf("\nModel Mirror: entry\n");

  // nothing to process
  if(mptr==NULL)return(0);
  if(mptr->xmir[mtyp]==0 && mptr->ymir[mtyp]==0 && mptr->zmir[mtyp]==0)return(0);
  mptr->reslice_flag=TRUE;
  
  // calculate center for this model
  if((mptr->xmin[mtyp] + mptr->ymin[mtyp] + mptr->zmin[mtyp])>TOLERANCE)model_maxmin(mptr);
  xc=mptr->xmax[mtyp]/2.0;
  yc=mptr->ymax[mtyp]/2.0;
  zc=mptr->zmax[mtyp]/2.0;
  
  //printf("  Min = %06.3f %06.3f %06.3f \n",mptr->xmin,mptr->ymin,mptr->zmin);
  //printf("  Max = %06.3f %06.3f %06.3f \n",mptr->xmax,mptr->ymax,mptr->zmax);
  //printf("  Ctr = %06.3f %06.3f %06.3f \n",xc,yc,zc);
  //printf("  Mir = %6d %6d %6d \n",mptr->xmir,mptr->ymir,mptr->zmir);

  // cycle through entire model facet list and mirror vertexes
  vptr=mptr->vertex_first[mtyp];
  while(vptr!=NULL)
    {
    if(mptr->xmir[mtyp]!=0)vptr->x=((-1)*(vptr->x-xc)+xc);		// flip x if requested
    if(mptr->ymir[mtyp]!=0)vptr->y=((-1)*(vptr->y-yc)+yc);		// flip y if requested
    if(mptr->zmir[mtyp]!=0)vptr->z=((-1)*(vptr->z-zc)+zc);		// flip z if requested
    vptr=vptr->next;
    }
  
  // if this model contians an image...
  if(mptr->g_img_act_buff!=NULL)
    {
    // make a copy of act_buff to use as source for this operation
    g_img_copy_buff=gdk_pixbuf_copy(mptr->g_img_act_buff);
    copy_n_ch = gdk_pixbuf_get_n_channels (g_img_copy_buff);			// n channels - should always be 4
    copy_cspace = gdk_pixbuf_get_colorspace (g_img_copy_buff);			// color space - should be RGB
    copy_alpha = gdk_pixbuf_get_has_alpha (g_img_copy_buff);			// alpha - should be FALSE
    copy_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_copy_buff); 	// bits per pixel - should be 8
    copy_width = gdk_pixbuf_get_width (g_img_copy_buff);			// number of columns (image size)
    copy_height = gdk_pixbuf_get_height (g_img_copy_buff);			// number of rows (image size)
    copy_rowstride = gdk_pixbuf_get_rowstride (g_img_copy_buff);		// get pixels per row... may change tho
    copy_pixels = gdk_pixbuf_get_pixels (g_img_copy_buff);			// get pointer to pixel data
    
    for(ypix=0; ypix<copy_height; ypix++)
      {
      for(xpix=0; xpix<copy_width; xpix++)
	{
	// load pixel from source copy
	pix1 = copy_pixels + ypix * copy_rowstride + xpix * copy_n_ch;
	
	// determine where to write copy of pix1 in act
	xdest=xpix; ydest=ypix;						// init to same
	if(mptr->xmir[mtyp]!=0)xdest=copy_width-xpix;			// if mirror in x, flip x
	if(mptr->ymir[mtyp]!=0)ydest=copy_height-ypix-1;		// if mirror in y, flip y
	if(xdest<0)xdest=0;
	if(ydest<0)ydest=0;
	
	// save contents of pix1 in new location of act at pix2
	pix2 = mptr->act_pixels + ydest * mptr->act_rowstride + xdest * mptr->act_n_ch;
	pix2[0]=pix1[0];
	pix2[1]=pix1[1];
	pix2[2]=pix1[2];
	pix2[3]=pix1[3];
	}	// end of xpix loop
      }		// end of ypix loop
      
    g_object_unref(g_img_copy_buff);
    }		// end of if model has image
  
  // reset flags
  mptr->xmir[mtyp]=0; mptr->ymir[mtyp]=0; mptr->zmir[mtyp]=0;
  
  //printf("Model Mirror: exit\n");
  
  return(1);
}

// Function to scale the model by translating vertex coordinates
// assume scalar center to be center of model
int model_scale(model *mptr, int mtyp)
{
  int		dest_width,dest_height;
  int		xpix,ypix,xinc,yinc;
  double  	xc,yc,zc;
  vertex	*vptr;
  GdkPixbuf	*g_img_copy_buff;
  int		copy_n_ch;
  int		copy_cspace;
  int		copy_alpha;
  int		copy_bits_per_pixel;
  int 		copy_height;
  int 		copy_width;
  int 		copy_rowstride;
  guchar 	*pix1,*pix2,*copy_pixels;
  int		xdest,ydest,xdestmax,ydestmax;
  
  //printf("\nModel Scale: entry\n");

  // validate inputs
  if(mptr==NULL)return(0);
  if(mtyp<1 || mtyp>MAX_MDL_TYPES)return(0);
  mptr->reslice_flag=TRUE;
  
  // calculate center for this model
  if((mptr->xmin[mtyp] + mptr->ymin[mtyp] + mptr->zmin[mtyp])>TOLERANCE)model_maxmin(mptr);
  xc=(mptr->xmax[mtyp] - mptr->xmin[mtyp])/2.0;
  yc=(mptr->ymax[mtyp] - mptr->ymin[mtyp])/2.0;
  zc=(mptr->zmax[mtyp] - mptr->zmin[mtyp])/2.0;
  
  //printf("  Min = %06.3f %06.3f %06.3f \n",mptr->xmin,mptr->ymin,mptr->zmin);
  //printf("  Max = %06.3f %06.3f %06.3f \n",mptr->xmax,mptr->ymax,mptr->zmax);
  //printf("  Ctr = %06.3f %06.3f %06.3f \n",xc,yc,zc);
  //printf("  Scl = %06.3f %06.3f %06.3f \n",mptr->xscl,mptr->yscl,mptr->zscl);
  
  // cycle through entire model vertex list of all materials and scale by ratio of new scale to old scale
  // other model attributes (like facets) will scale automatically since they are based on these vtxs
  vptr=mptr->vertex_first[mtyp];
  while(vptr!=NULL)
    {
    if(mptr->xscl[mtyp]!=1)vptr->x=((mptr->xscl[mtyp]/mptr->xspr[mtyp])*(vptr->x-xc)+xc);
    if(mptr->yscl[mtyp]!=1)vptr->y=((mptr->yscl[mtyp]/mptr->yspr[mtyp])*(vptr->y-yc)+yc);
    if(mptr->zscl[mtyp]!=1)vptr->z=((mptr->zscl[mtyp]/mptr->zspr[mtyp])*(vptr->z-zc)+zc);
    vptr=vptr->next;
    }
      
  // if this model has an image associated to it...
  if(mptr->g_img_act_buff!=NULL)
    {
    image_loader(mptr,mptr->act_width,mptr->act_height,TRUE);
    
/*
    // make a copy of act_buff to use as source for this operation
    g_img_copy_buff=gdk_pixbuf_copy(mptr->g_img_act_buff);
    copy_n_ch = gdk_pixbuf_get_n_channels (g_img_copy_buff);			// n channels - should always be 4
    copy_cspace = gdk_pixbuf_get_colorspace (g_img_copy_buff);			// color space - should be RGB
    copy_alpha = gdk_pixbuf_get_has_alpha (g_img_copy_buff);			// alpha - should be FALSE
    copy_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_copy_buff); 	// bits per pixel - should be 8
    copy_width = gdk_pixbuf_get_width (g_img_copy_buff);			// number of columns (image size)
    copy_height = gdk_pixbuf_get_height (g_img_copy_buff);			// number of rows (image size)
    copy_rowstride = gdk_pixbuf_get_rowstride (g_img_copy_buff);		// get pixels per row... may change tho
    copy_pixels = gdk_pixbuf_get_pixels (g_img_copy_buff);			// get pointer to pixel data

    //dest_width=floor(copy_width * mptr->xscl[MODEL]/mptr->xspr[MODEL]);
    //dest_height=floor(copy_height * mptr->yscl[MODEL]/mptr->yspr[MODEL]);
    dest_width=floor(copy_width * mptr->xscl[MODEL]);
    dest_height=floor(copy_height * mptr->yscl[MODEL]);
    if(dest_width<50)dest_width=25;
    if(dest_width>BUILD_TABLE_LEN_X)dest_width=BUILD_TABLE_LEN_X;
    if(dest_height<50)dest_height=25;
    if(dest_height>BUILD_TABLE_LEN_Y)dest_height=BUILD_TABLE_LEN_Y;
    mptr->g_img_act_buff=gdk_pixbuf_scale_simple(g_img_copy_buff,dest_width,dest_height,GDK_INTERP_BILINEAR);
    //gdk_pixbuf_scale(g_img_copy_buff,mptr->g_img_act_buff,0,0,copy_width,copy_height,0,0,mptr->xscl[MODEL],mptr->yscl[MODEL],GDK_INTERP_BILINEAR);

    // reload params as image has changed
    mptr->act_n_ch = gdk_pixbuf_get_n_channels (mptr->g_img_act_buff);			// n channels - should always be 4
    mptr->act_cspace = gdk_pixbuf_get_colorspace (mptr->g_img_act_buff);		// color space - should be RGB
    mptr->act_alpha = gdk_pixbuf_get_has_alpha (mptr->g_img_act_buff);			// alpha - should be FALSE
    mptr->act_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (mptr->g_img_act_buff); 	// bits per pixel - should be 8
    mptr->act_width = gdk_pixbuf_get_width (mptr->g_img_act_buff);			// number of columns (image size)
    mptr->act_height = gdk_pixbuf_get_height (mptr->g_img_act_buff);			// number of rows (image size)
    mptr->act_rowstride = gdk_pixbuf_get_rowstride (mptr->g_img_act_buff);		// get pixels per row... may change tho
    mptr->act_pixels = gdk_pixbuf_get_pixels (mptr->g_img_act_buff);			// get pointer to pixel data
*/
/*
    if(mptr->xscl[MODEL]<=1 && mptr->yscl[MODEL]<=1)
      {
      xinc=floor(1.0/mptr->xscl[MODEL]);
      yinc=floor(1.0/mptr->yscl[MODEL]);
      if(xinc>10)xinc=10;
      if(yinc>10)yinc=10;
      mptr->act_rowstride=floor(mptr->act_rowstride/xinc);
      
      ydest=0;
      for(ypix=0; ypix<copy_height; ypix+=yinc)
	{
	xdest=0;
	for(xpix=0; xpix<copy_width; xpix+=xinc)
	  {
	  // load pixel from source copy
	  pix1 = copy_pixels + ypix * copy_rowstride + xpix * copy_n_ch;
	  
	  // save contents of pix1 in new location of act at pix2
	  pix2 = mptr->act_pixels + ydest * mptr->act_rowstride + xdest * mptr->act_n_ch;
	  pix2[0]=pix1[0];
	  pix2[1]=pix1[1];
	  pix2[2]=pix1[2];
	  pix2[3]=pix1[3];
	  
	  if(xdest>xdestmax)xdestmax=xdest;
	  xdest++;
	  }	// end of xpix loop
	if(ydest>ydestmax)ydestmax=ydest;
	ydest++;
	}		// end of ypix loop
	
      mptr->act_width = xdestmax;
      mptr->act_height = ydestmax;
      }
*/

    //printf("active image resized\n");
    //g_object_unref(g_img_copy_buff);
    }
  
  // save old values
  mptr->xspr[mtyp]=mptr->xscl[mtyp];
  mptr->yspr[mtyp]=mptr->yscl[mtyp];
  mptr->zspr[mtyp]=mptr->zscl[mtyp];

  //printf("Model Scale: exit\n");
  return(1);
}

// Function to rotate a model about its center.
// The assumption is that all types (MODEL, SUPPORT, etc.) get rotated since they form a unit.
// The exception is images.  Typically one wants to rotate it slightly relative to the target onto
// which it will be applied.  So even tho they are of type MODEL, the rotation occurs dynamically
// at display time and print time.  Also note that this rotation is typically independent of the 
// TARGET type.  Also note that TARGET models are usually fixed (i.e. input from scan etc.) so 
// rotating them makes no sense.
//
// Inputs from model rotation params are in radians.
int model_rotate(model *mptr)
{
  int		mtyp;
  vertex 	*vptr;
  facet		*fptr;
  float 	xdelta,ydelta,zdelta;
  float 	crx,srx,cry,sry,crz,srz;
  float		xp,yp,zp;						// current angle formed by vertex from each axis
  float 	xc,yc,zc;						// calculated centroid of file to be rotated
  GdkPixbuf 	*img_copy;
  
  // nothing to process
  if(mptr==NULL)return(0);
  
  // determine the model center based on MODEL type only and rotate all types about that center
  mtyp=MODEL;
  
  //printf("\nModel Rotate: entry\n");
  //printf("   mptr=%X  fqty=%ld  xp=%f  yp=%f  zp=%f \n",mptr,mptr->facet_qty[mtyp],mptr->xrpr[mtyp]*180/PI,mptr->yrpr[mtyp]*180/PI,mptr->zrpr[mtyp]*180/PI);
  //printf("                      xr=%f  yr=%f  zr=%f \n",mptr->xrot[mtyp]*180/PI,mptr->yrot[mtyp]*180/PI,mptr->zrot[mtyp]*180/PI);

  // calculate delta between where model is, and what is now wanted
  // if no different, then just exit now
  xdelta=mptr->xrot[mtyp]-mptr->xrpr[mtyp];
  ydelta=mptr->yrot[mtyp]-mptr->yrpr[mtyp];
  zdelta=mptr->zrot[mtyp]-mptr->zrpr[mtyp];
  //printf("                      xd=%f  yd=%f  zd=%f \n\n",xdelta*180/PI,ydelta*180/PI,zdelta*180/PI);
  //if(xdelta<CLOSE_ENOUGH && ydelta<CLOSE_ENOUGH && zdelta<CLOSE_ENOUGH)return(0);
  
  // save previous rotation values
  mptr->xrpr[mtyp]=mptr->xrot[mtyp];
  mptr->yrpr[mtyp]=mptr->yrot[mtyp];
  mptr->zrpr[mtyp]=mptr->zrot[mtyp];
  
  // calculate center for this model
  model_maxmin(mptr);
  xc=mptr->xmax[mtyp]/2.0;
  yc=mptr->ymax[mtyp]/2.0;
  zc=mptr->zmax[mtyp]/2.0;
  
  //printf("\nMODEL ROTATE:  mtyp=%d  \n",mtyp);
  //printf("  Min = %06.3f %06.3f %06.3f \n",mptr->xmin[mtyp],mptr->ymin[mtyp],mptr->zmin[mtyp]);
  //printf("  Max = %06.3f %06.3f %06.3f \n",mptr->xmax[mtyp],mptr->ymax[mtyp],mptr->zmax[mtyp]);
  //printf("  Ctr = %06.3f %06.3f %06.3f \n",xc,yc,zc);
  //printf("  Rot = %06.3f %06.3f %06.3f \n",mptr->xrot[mtyp],mptr->yrot[mtyp],mptr->zrot[mtyp]);
    
  // loop thru all model types
  for(mtyp=MODEL;mtyp<=INTERNAL;mtyp++)
    {
    if(mptr->vertex_first[mtyp]==NULL)continue;				// skip none vtx models (GERBER, IMAGES, etc.)
      
    // cycle thru entire vertex list and shift model centroid to zero.
    vptr=mptr->vertex_first[mtyp];
    while(vptr!=NULL)
      {
      vptr->x -= xc;
      vptr->y -= yc;
      vptr->z -= zc;
      vptr=vptr->next;
      }
      
    // rotate about x axis	  
    if(fabs(xdelta)>TOLERANCE)
      {
      mptr->reslice_flag=TRUE;
      //printf("  rotating about X\n");
      // pre-caclulate trig functions
      crx=cos(xdelta);
      srx=sin(xdelta);
      // rotate entire vertex list
      vptr=mptr->vertex_first[mtyp];
      while(vptr!=NULL)
	{
	yp = vptr->y*crx - vptr->z*srx;
	zp = vptr->y*srx + vptr->z*crx;
	vptr->y=yp;
	vptr->z=zp;
	vptr=vptr->next;
	}
      // must also rotate facet normals
      fptr=mptr->facet_first[mtyp];
      while(fptr!=NULL)
	{
	yp = fptr->unit_norm->y*crx - fptr->unit_norm->z*srx;
	zp = fptr->unit_norm->y*srx + fptr->unit_norm->z*crx;
	fptr->unit_norm->y=yp;
	fptr->unit_norm->z=zp;
	fptr=fptr->next;
	}  
      }
    
    // rotate about y axis	  
    if(fabs(ydelta)>TOLERANCE)
	{
	mptr->reslice_flag=TRUE;
	//printf("  rotating about Y\n");
	// pre-caclulate trig functions
	cry=cos(ydelta);
	sry=sin(ydelta);
	// rotate entire vertex list
	vptr=mptr->vertex_first[mtyp];
	while(vptr!=NULL)
	  {
	  zp=vptr->z*cry-vptr->x*sry;
	  xp=vptr->z*sry+vptr->x*cry;
	  vptr->z=zp;
	  vptr->x=xp;
	  vptr=vptr->next;
	  }
	// must also rotate facet normals
	fptr=mptr->facet_first[mtyp];
	while(fptr!=NULL)
	  {
	  zp=fptr->unit_norm->z*cry-fptr->unit_norm->x*sry;
	  xp=fptr->unit_norm->z*sry+fptr->unit_norm->x*cry;
	  fptr->unit_norm->z=zp;
	  fptr->unit_norm->x=xp;
	  fptr=fptr->next;
	  }
	}
		
    // rotate about z axis	  
    if(fabs(zdelta)>TOLERANCE)
	{
	mptr->reslice_flag=TRUE;
	//printf("  rotating about Z\n");
	// pre-caclulate trig functions
	crz=cos(zdelta);
	srz=sin(zdelta);
	// rotate entire vertex list
	vptr=mptr->vertex_first[mtyp];
	while(vptr!=NULL)
	  {
	  xp=vptr->x*crz-vptr->y*srz;
	  yp=vptr->x*srz+vptr->y*crz;
	  vptr->x=xp;
	  vptr->y=yp;
	  vptr=vptr->next;
	  }
	// must also rotate facet normals
	fptr=mptr->facet_first[mtyp];
	while(fptr!=NULL)
	  {
	  xp=fptr->unit_norm->x*crz-fptr->unit_norm->y*srz;
	  yp=fptr->unit_norm->x*srz+fptr->unit_norm->y*crz;
	  fptr->unit_norm->x=xp;
	  fptr->unit_norm->y=yp;
	  fptr=fptr->next;
	  }
	}
  
    // cycle thru entire vertex list and shift model back to its original location
    vptr=mptr->vertex_first[mtyp];
    while(vptr!=NULL)
      {
      vptr->x += xc;
      vptr->y += yc;
      vptr->z += zc;
      vptr=vptr->next;
      }
    } 	// end of for mtyp loop
	
  //printf("Model Rotate: exit\n");
  return(1);
}

// Function to establish the max/min boundaries of a model.
// The assumption is that this applies to all model types (i.e. MODEL, SUPPORT, etc.)
// When complete, this funciton always moves the model to 0,0,0 so that xmax,ymax,zmax give
// the actual size of the model as well.  The xoff,yoff,zoff params move the model within the
// build volume.
int model_maxmin(model *mptr)
{
  int			h,i,slot,mtyp;
  float 		xp,yp,zp;
  float 		sx,sy;
  vertex		*vptr;
  vector		*ssptr;
  facet			*fptr;
  polygon		*pptr;
  slice			*sptr;
  genericlist		*aptr;
  linetype		*linetype_ptr;

  
  if(mptr==NULL)return(0);
  mtyp=MODEL;
  
  mdlmm_count++;
  printf("Model MaxMin: entry#=%d  mptr=%X  \n", mdlmm_count,mptr);
  //printf("Model Off = %06.3f %06.3f %06.3f \n",mptr->xoff[mtyp],mptr->yoff[mtyp],mptr->zoff[mtyp]);
  //printf("Model Min = %06.3f %06.3f %06.3f \n",mptr->xmin[mtyp],mptr->ymin[mtyp],mptr->zmin[mtyp]);
  //printf("Model Max = %06.3f %06.3f %06.3f \n",mptr->xmax[mtyp],mptr->ymax[mtyp],mptr->zmax[mtyp]);
  
  // find extents of all types for this model
  //for(mtyp=MODEL;mtyp<MAX_MDL_TYPES;mtyp++)
  
    {
  
    // reset this models max/min values
    mptr->xmin[mtyp]=10000;    mptr->ymin[mtyp]=10000;    mptr->zmin[mtyp]=10000;
    mptr->xmax[mtyp]=(-10000); mptr->ymax[mtyp]=(-10000); mptr->zmax[mtyp]=(-10000);
    xp=0; yp=0; zp=0;
  
    // manage images based on image size and the tool being used to "print" it
    // images are typcially defined as MODEL mtyp since they are the user's targeted input
    if(mptr->input_type==IMAGE && mtyp==MODEL)
      {
      if(mptr->g_img_act_buff!=NULL)
	{
	// check if a tool capable of marking image is loaded.  if so, assume it will do the work.
	// if no tool assigned/available then default to 0.25 pixel size on table.
	for(i=0;i<MAX_TOOLS;i++)
	  {
	  if(Tool[i].state>TL_UNKNOWN)
	    {
	    if(genericlist_find(Tool[i].oper_list,"MARK_IMAGE")==OP_MARK_IMAGE)break;	// get tool id doing the work
	    }
	  }
	if(i<MAX_TOOLS)
	  {
	  linetype_ptr=linetype_find(i,MDL_FILL);
	  if(linetype_ptr!=NULL)mptr->pix_size=linetype_ptr->line_width;
	  }
	  
	mptr->act_width = gdk_pixbuf_get_width (mptr->g_img_act_buff);	
	mptr->act_height = gdk_pixbuf_get_height (mptr->g_img_act_buff);	
	mptr->xmin[mtyp]=0.0; mptr->xmax[mtyp]=mptr->act_width*mptr->pix_size;
	mptr->ymin[mtyp]=0.0; mptr->ymax[mtyp]=mptr->act_height*mptr->pix_size;
	mptr->zmin[mtyp]=0.0; mptr->zmax[mtyp]=0.0;
	xp=mptr->xmin[mtyp];
	yp=mptr->ymin[mtyp];
	zp=mptr->zmin[mtyp];
	}
      }
  
    // manage gerber models which are loaded as a slice
    if(mptr->input_type==GERBER && mtyp==MODEL)
      {
      // get slot id of for this model
      slot=(-1);
      aptr=mptr->oper_list;
      while(aptr!=NULL)
	{
	if(strlen(aptr->name)>1){slot=aptr->ID;break;}
	aptr=aptr->next;
	}
      if(slot<0 || slot>=MAX_TOOLS)return(0);
      if(mptr->slice_first[slot]==NULL)return(0);
      
      slice_maxmin(mptr->slice_first[slot]);
    
      mptr->xmin[mtyp]=(mptr->slice_first[slot])->xmin;
      mptr->xmax[mtyp]=(mptr->slice_first[slot])->xmax;
      mptr->ymin[mtyp]=(mptr->slice_first[slot])->ymin;
      mptr->ymax[mtyp]=(mptr->slice_first[slot])->ymax;
      mptr->zmin[mtyp]=0.00;
      mptr->zmax[mtyp]=0.50;
  
      // adjust model position so that it is at 0,0,0 or centered
      xp=mptr->xmin[mtyp];
      yp=mptr->ymin[mtyp];
      zp=mptr->zmin[mtyp];
      sptr=mptr->slice_first[slot];
      pptr=sptr->pfirst[TRACE];
      while(pptr!=NULL)
	{
	vptr=pptr->vert_first;
	while(vptr!=NULL)
	  {
	  vptr->x-=xp;
	  vptr->y-=yp;
	  vptr->z-=zp;
	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  }
	pptr=pptr->next;
	}
      }
      
    // manage all other model types
    if(mptr->input_type==STL || mtyp==TARGET)
      {
      // loop thru all verticies within the model and find min/max in each axis
      vptr=mptr->vertex_first[mtyp];
      while(vptr!=NULL)
	{
	// set max/min for this particular model
	if(vptr->x < mptr->xmin[mtyp])mptr->xmin[mtyp]=vptr->x;			
	if(vptr->y < mptr->ymin[mtyp])mptr->ymin[mtyp]=vptr->y;
	if(vptr->z < mptr->zmin[mtyp])mptr->zmin[mtyp]=vptr->z;
	
	if(vptr->x > mptr->xmax[mtyp])mptr->xmax[mtyp]=vptr->x;
	if(vptr->y > mptr->ymax[mtyp])mptr->ymax[mtyp]=vptr->y;
	if(vptr->z > mptr->zmax[mtyp])mptr->zmax[mtyp]=vptr->z;
	
	vptr=vptr->next;
	}
	  
      // adjust model position so that it is at 0,0,0 by subtracting off min values
      xp=mptr->xmin[mtyp];
      yp=mptr->ymin[mtyp];
      zp=mptr->zmin[mtyp];
      for(i=MODEL;i<MAX_MDL_TYPES;i++)
	{
	vptr=mptr->vertex_first[i];
	while(vptr!=NULL)
	  {
	  vptr->x-=xp;
	  vptr->y-=yp;
	  if(i!=TARGET)vptr->z-=zp;
	  vptr=vptr->next;
	  }
	}
      }
      
    mptr->xmin[mtyp]-=xp; mptr->xmax[mtyp]-=xp;
    mptr->ymin[mtyp]-=yp; mptr->ymax[mtyp]-=yp;
    mptr->zmin[mtyp]-=zp; mptr->zmax[mtyp]-=zp;
    }		// end of for mtyp loop
  
  if(mptr->mdl_has_target==FALSE)
    {
    mptr->zmin[TARGET]=0.0;
    mptr->zmax[TARGET]=0.0;
    }
  
  //printf("\n\nModel MaxMin: exit\n");
  //printf("  Model Off = %06.3f %06.3f %06.3f \n",mptr->xoff[MODEL],mptr->yoff[MODEL],mptr->zoff[MODEL]);
  //printf("  Model Min = %06.3f %06.3f %06.3f \n",mptr->xmin[MODEL],mptr->ymin[MODEL],mptr->zmin[MODEL]);
  //printf("  Model Max = %06.3f %06.3f %06.3f \n",mptr->xmax[MODEL],mptr->ymax[MODEL],mptr->zmax[MODEL]);
  
  return(1);
}

// function to verify the integrity of a model
int model_integrity_check(model *mptr)
{
  int		i,facet_ok;
  float 	r0,n01,n12,n20;
  facet 	*fptr;
  vertex 	*vptr;
  facet_list	*fl_ptr;
  
  if(mptr==NULL)return(FALSE);

  printf("\nModel integrity:  entry\n");
  
  // loop thru vertex list and ensure each vtx anchors at least 3 facets
  printf("  checking vertex relationships...\n");
  vptr=mptr->vertex_first[MODEL];
  while(vptr!=NULL)
    {
    i=0;
    fl_ptr=vptr->flist;
    while(fl_ptr!=NULL)							// loop thru facets in list
      {
      fptr=fl_ptr->f_item;
      i++;								// increment facet count
      fl_ptr=fl_ptr->next;
      }
    if(i<3){mptr->error_status=1; printf("  ... vertex errors found.\n");}
    vptr=vptr->next;
    }
  
  // loop thru facet list and ensure each facet has 3 unique neighbors
  printf("  checking facet relationships...\n");
  fptr=mptr->facet_first[MODEL];
  while(fptr!=NULL)
    {
    facet_ok=TRUE;
    for(i=0;i<3;i++){if(fptr->fct[i]==NULL)facet_ok=FALSE;}
    //if((fptr->fct[0])==(fptr->fct[1]))facet_ok=FALSE;
    //if((fptr->fct[1])==(fptr->fct[2]))facet_ok=FALSE;
    //if((fptr->fct[2])==(fptr->fct[0]))facet_ok=FALSE;
    if(facet_ok==FALSE){mptr->error_status=2; printf("  ... facet errors found.\n");}
    fptr=fptr->next;
    }
    
  // verify facet normals.  many poorly generated files are not consistent.
  // this method regenerates each normal based on the sequence the vtxs were loaded off the file.
  /*
  printf("  checking facet normals...\n");
  fptr=mptr->facet_first[MODEL];
  while(fptr!=NULL)
    {
    // ensure base face is good
    if(fptr->unit_norm==NULL)facet_unit_normal(fptr);

    // compare against neighbors to see if going same general direction
    if(fptr->fct[0]!=NULL && fptr->fct[1]!=NULL && fptr->fct[2]!=NULL)
      {
      // make all facets have a unit normal defined (even if wrong)
      if(fptr->fct[0]->unit_norm==NULL)facet_unit_normal(fptr->fct[0]);
      if(fptr->fct[1]->unit_norm==NULL)facet_unit_normal(fptr->fct[1]);
      if(fptr->fct[2]->unit_norm==NULL)facet_unit_normal(fptr->fct[2]);
      
      // compare dot products
      // first determine which general direction the base facet's neighbors are going
      n01=vertex_dotproduct(fptr->fct[0]->unit_norm,fptr->fct[1]->unit_norm);
      n12=vertex_dotproduct(fptr->fct[1]->unit_norm,fptr->fct[2]->unit_norm);
      n20=vertex_dotproduct(fptr->fct[2]->unit_norm,fptr->fct[0]->unit_norm);
      
      // now determine which general direction the base facet is pointing relative to neighbors
      r0=vertex_dotproduct(fptr->unit_norm,fptr->fct[0]->unit_norm);
      if((r0*n01*n12*n20)<0)facet_flip_normal(fptr);
      }

    fptr=fptr->next;
    }
  */
  
  if(mptr->error_status==0){printf("Model integrity: exit status = CLEAN");}
  else {printf("Model integrity: exit status = ERRORS");}
  
  return(mptr->error_status);
}

// function to map model to build table z offsets
int model_map_to_table(model *mptr)
{
  int			h,i;
  float 		xp,yp,zp;
  float 		x_max,y_max,x_remain,y_remain;
  float 		minPz;
  
  //printf("  Adjusting build table scan to model.\n");
  
  // re-int the processed array
  for(h=0;h<SCAN_X_MAX;h++)
    {
    for(i=0;i<SCAN_Y_MAX;i++)
      {
      Pz[h][i]=(-1);
      }
    }
    
  // note that array Pz[0][0] will corrispond to table location x_remain,y_remain -- see scanning function
  x_max=(int)(BUILD_TABLE_LEN_X/scan_dist);				// set number of points in x
  x_remain=(BUILD_TABLE_LEN_X-(x_max*scan_dist))/2;			// calculate actual position of x=0 in model coords
  y_max=(int)(BUILD_TABLE_LEN_Y/scan_dist);
  y_remain=(BUILD_TABLE_LEN_Y-(y_max*scan_dist))/2;
  printf("\nx_max=%f  x_remain=%f    y_max=%f  y_remain=%f \n",x_max,x_remain,y_max,y_remain);

  // find minimum z offset value within model bounds of RAW data
  minPz=BUILD_TABLE_MAX_Z;
  xp=XMin;
  while(xp<XMax)
    {
    i=floor((xp-x_remain)/scan_dist);
    if(i<0)i=0;  if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;
    yp=YMin;
    while(yp<YMax)
      {
      h=floor((yp-y_remain)/scan_dist);
      if(h<0)h=0;  if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;
      if(Pz_raw[i][h]<minPz)minPz=Pz_raw[i][h];
      yp+=scan_dist;
      }
    xp+=scan_dist;
    }

  // normalize all to lowest value found in raw array and load into processed array
  // note all other values of processed array outside of model bounds will be (-1)
  xp=XMin;
  while(xp<XMax)
    {
    i=floor((xp-x_remain)/scan_dist);
    if(i<0)i=0; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;
    yp=YMin;
    while(yp<YMax)
      {
      h=floor((yp-y_remain)/scan_dist);
      if(h<0)h=0;  if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;
      Pz[i][h]=Pz_raw[i][h]-minPz;
      yp+=scan_dist;
      }
    xp+=scan_dist;
    }
    
  // the last thing to do is now echo the processed array values found just INSIDE model bounds to just OUTSIDE model bounds
  // this keeps things from getting out of hand around the edges
  xp=XMin;
  while(xp<XMax)
    {
    i=floor((xp-x_remain)/scan_dist); if(i<0)i=0; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;
    h=floor((YMin-y_remain)/scan_dist); if(h<1)h=1; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;
    Pz[i][h-1]=Pz[i][h];
    h=floor((YMax+y_remain)/scan_dist); if(h<0)h=0; if(h>=(SCAN_Y_MAX-1))h=SCAN_Y_MAX-2;
    Pz[i][h+1]=Pz[i][h];
    xp+=scan_dist;
    }
  yp=YMin-y_remain;
  while(yp<YMax-y_remain)
    {
    h=floor((yp-y_remain)/scan_dist); if(h<0)h=0; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-1;
    i=floor((XMin-x_remain)/scan_dist); if(i<1)i=1; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-1;
    Pz[i-1][h]=Pz[i][h];
    i=floor((XMax-x_remain)/scan_dist); if(i<0)i=0; if(h>=(SCAN_Y_MAX-1))h=SCAN_Y_MAX-2;
    Pz[i+1][h]=Pz[i][h];
    yp+=scan_dist;
    }
  i=floor((XMin-x_remain)/scan_dist); if(i<1)i=1; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-2;
  h=floor((YMin-y_remain)/scan_dist); if(h<1)h=1; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-2;
  Pz[i-1][h-1]=Pz[i][h];
  i=floor((XMin-x_remain)/scan_dist); if(i<1)i=1; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-2;
  h=floor((YMax-y_remain)/scan_dist); if(h<1)h=1; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-2;
  Pz[i-1][h+1]=Pz[i][h];
  i=floor((XMax-x_remain)/scan_dist); if(i<1)i=1; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-2;
  h=floor((YMax-y_remain)/scan_dist); if(h<1)h=1; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-2;
  Pz[i+1][h+1]=Pz[i][h];
  i=floor((XMax-x_remain)/scan_dist); if(i<1)i=1; if(h>=SCAN_Y_MAX)h=SCAN_Y_MAX-2;
  h=floor((YMin-y_remain)/scan_dist); if(h<1)h=1; if(i>=SCAN_X_MAX)i=SCAN_X_MAX-2;
  Pz[i+1][h-1]=Pz[i][h];
  
  build_table_scan_flag=FALSE;						// turn "need to map" flag off
      
  return(1);
}

// Fuction to slice a model via plane facet intersection method by cycling through entire facet list
// output is an unsorted list of raw vectors.  Note: this is the non-planar slicer version.  That is,
// the vectors of the slices it generates have varying Z values.
slice *model_raw_slice(model *mptr, int slot, int mtyp, int ptyp, float z_level)
{
	int	z_flag1=0,z_flag2=0,z_flag3=0;
	int	i,j,vec_cnt=0;
	long 	old_vec_mem;
	float 	plane[3][3];						// slicing plane
	double 	t;
	double 	a[3][3],ainv[3][3];
	double 	determinant=0.0;

	vertex		*vptr1,*vptr2,*vptr3;
	vector 		*vecptr,*vecold,*vecnew,*vecdel;
	vector_list	*vl_ptr,*vl_del;
	facet 		*fptr;
	polygon 	*pptr,*pnew,*pold;
	slice 		*sptr;
	
	//printf("   Model raw slice:  Enter:  slot=%d  mtyp=%d  ptyp=%d  z=%6.3f \n",slot,mtyp,ptyp,z_level);
	//printf("\n\nModel raw slice entry:  vec count = %ld \n",vector_mem);
	if(mptr==NULL)return(NULL);
	if(mptr->input_type!=STL)return(NULL);
	if(fabs(z_level)<mptr->zmin[mtyp])return(NULL);
	if(fabs(z_level)>mptr->zmax[mtyp])return(NULL);
	if(mtyp<=0 || mtyp>=MAX_MDL_TYPES)return(NULL);
	if(ptyp<=0 || ptyp>=MAX_LINE_TYPES)return(NULL);
	
	// define slicing plane with three arbitrary points all at cutting level.
	// may want to set the XY points to the max and min of the available build table envelope?
	// that way the "u" and "v" results will tell if points land within build volume.
	// for now, they are merely set to a 10 inch triangle
	plane[0][0]=0.0;   plane[0][1]=0.0;   plane[0][2]=z_level;			// first point on plane
	plane[1][0]=250.0; plane[1][1]=0.0;   plane[1][2]=z_level;			// second point on plane
	plane[2][0]=0.0;   plane[2][1]=250.0; plane[2][2]=z_level;			// third point on plane
	
	// starting with the first facet in the linked list, loop through the whole list
	// looking for facet edges that cross the z plane.
	sptr=NULL;
	fptr=mptr->facet_first[mtyp];
	while(fptr!=NULL)
	  {
	  // If nothing crosses then skip to next facet
	  if(fptr->vtx[0]->z<z_level && fptr->vtx[1]->z<z_level && fptr->vtx[2]->z<z_level){fptr=fptr->next; continue;}
	  if(fptr->vtx[0]->z>z_level && fptr->vtx[1]->z>z_level && fptr->vtx[2]->z>z_level){fptr=fptr->next; continue;}
	  
	  // either find or create a slice to save data in if one does not yet exist  
	  if(sptr==NULL)
	    {
	    sptr=slice_find(mptr,mtyp,slot,z_level,job.min_slice_thk);	// get ptr to slice that was previously made for model
	    if(sptr==NULL)						// if nothing was found... make a new slice
	      {
	      sptr=slice_make();					// create a new slice
	      sptr->sz_level=z_level;					// note this is relative to btm of mdl (z=0).  it does NOT include mptr->zoff
	      slice_insert(mptr,slot, mptr->slice_last[slot],sptr);	// add it to the linked list of slices at end of list
	      }
	    }

	  //if(vec_cnt>0)printf("  vct=%d fptr=%X vtx[1]=%X vtx[2]=%X vtx[3]=%X\n",vec_cnt,fptr,fptr->vtx[0],fptr->vtx[1],fptr->vtx[2]);
	  vptr1=NULL; vptr2=NULL; vptr3=NULL;
	  z_flag1=0; z_flag2=0; z_flag3=0;
	  vec_cnt++;
	  
	  // check if this edge is exactly aligned with slice plane, but only include if facet is below
	  if(fptr->vtx[0]->z==z_level && fptr->vtx[1]->z==z_level && fptr->vtx[2]->z<z_level)
	    {
	    vptr1=vertex_make();
	    vptr1->x=fptr->vtx[0]->x;
	    vptr1->y=fptr->vtx[0]->y;
	    vptr1->z=z_level;
	    vptr1->attr=1;
	    z_flag1=1;
	    vptr2=vertex_make();
	    vptr2->x=fptr->vtx[1]->x;
	    vptr2->y=fptr->vtx[1]->y;
	    vptr2->z=z_level;
	    vptr2->attr=1;
	    z_flag2=1;
	    }
	  
	  // check if this edge is exactly aligned with slice plane
	  if(fptr->vtx[1]->z==z_level && fptr->vtx[2]->z==z_level && fptr->vtx[0]->z<z_level)
	    {
	    vptr2=vertex_make();
	    vptr2->x=fptr->vtx[1]->x;
	    vptr2->y=fptr->vtx[1]->y;
	    vptr2->z=z_level;
	    vptr2->attr=1;
	    z_flag2=1;
	    vptr3=vertex_make();
	    vptr3->x=fptr->vtx[2]->x;
	    vptr3->y=fptr->vtx[2]->y;
	    vptr3->z=z_level;
	    vptr3->attr=1;
	    z_flag3=1;
	    }
	  
	  // check if this edge is exactly aligned with slice plane
	  if(fptr->vtx[2]->z==z_level && fptr->vtx[0]->z==z_level && fptr->vtx[1]->z<z_level)
	    {
	    vptr3=vertex_make();
	    vptr3->x=fptr->vtx[2]->x;
	    vptr3->y=fptr->vtx[2]->y;
	    vptr3->z=z_level;
	    vptr3->attr=1;
	    z_flag3=1;
	    vptr1=vertex_make();
	    vptr1->x=fptr->vtx[0]->x;
	    vptr1->y=fptr->vtx[0]->y;
	    vptr1->z=z_level;
	    vptr1->attr=1;
	    z_flag1=1;
	    }
	  
	  // check if this edge is crossing... if so, process intersection
	  if((fptr->vtx[0]->z<=z_level && fptr->vtx[1]->z>z_level) || (fptr->vtx[0]->z>=z_level && fptr->vtx[1]->z<z_level))
	    {
	    // load edge and surface params into 2d array
	    a[0][0]=fptr->vtx[0]->x - fptr->vtx[1]->x;
	    a[0][1]=plane[1][0]-plane[0][0];
	    a[0][2]=plane[2][0]-plane[0][0];
	    a[1][0]=fptr->vtx[0]->y - fptr->vtx[1]->y;
	    a[1][1]=plane[1][1]-plane[0][1];
	    a[1][2]=plane[2][1]-plane[0][1];
	    a[2][0]=fptr->vtx[0]->z - fptr->vtx[1]->z;
	    a[2][1]=plane[1][2]-plane[0][2];
	    a[2][2]=plane[2][2]-plane[0][2];
	    // caculate the determinant
	    determinant=0;
	    for(i=0;i<3;i++)
	      {
	      determinant = determinant + (a[0][i]*(a[1][(i+1)%3]*a[2][(i+2)%3] - a[1][(i+2)%3]*a[2][(i+1)%3]));
	      }
	    // perform inversion
	    for(i=0;i<3;i++)
	      {
	      ainv[i][0]=( (a[(i+1)%3][1%3] * a[(i+2)%3][2%3]) - (a[(i+1)%3][2%3]*a[(i+2)%3][1%3]) ) / determinant;
	      }
	    // multiply inverted matrix with singular matrix to get intersection point
	    t=(ainv[0][0]*(fptr->vtx[0]->x - plane[0][0]))+(ainv[1][0]*(fptr->vtx[0]->y - plane[0][1]))+(ainv[2][0]*(fptr->vtx[0]->z - plane[0][2]));

	    vptr1=vertex_make();
	    vptr1->x=fptr->vtx[0]->x + (fptr->vtx[1]->x - fptr->vtx[0]->x)*t;
	    vptr1->y=fptr->vtx[0]->y + (fptr->vtx[1]->y - fptr->vtx[0]->y)*t;
	    vptr1->z=z_level;
	    vptr1->attr=1;
	    z_flag1=1;
	    }	
	    
	  // check if this edge is crossing... if so, process intersection
	  if((fptr->vtx[1]->z<=z_level && fptr->vtx[2]->z>z_level) || (fptr->vtx[1]->z>=z_level && fptr->vtx[2]->z<z_level))
	    {
	    // load edge and surface params into 2d array
	    a[0][0]=fptr->vtx[1]->x - fptr->vtx[2]->x;
	    a[0][1]=plane[1][0]-plane[0][0];
	    a[0][2]=plane[2][0]-plane[0][0];
	    a[1][0]=fptr->vtx[1]->y - fptr->vtx[2]->y;
	    a[1][1]=plane[1][1]-plane[0][1];
	    a[1][2]=plane[2][1]-plane[0][1];
	    a[2][0]=fptr->vtx[1]->z - fptr->vtx[2]->z;
	    a[2][1]=plane[1][2]-plane[0][2];
	    a[2][2]=plane[2][2]-plane[0][2];
	    // caculate the determinant
	    determinant=0;
	    for(i=0;i<3;i++)
	      {
	      determinant = determinant + (a[0][i]*(a[1][(i+1)%3]*a[2][(i+2)%3] - a[1][(i+2)%3]*a[2][(i+1)%3]));
	      }
	    // perform inversion
	    for(i=0;i<3;i++)
	      {
	      ainv[i][0]=( (a[(i+1)%3][1%3] * a[(i+2)%3][2%3]) - (a[(i+1)%3][2%3]*a[(i+2)%3][1%3]) ) / determinant;
	      }
	    // multiply inverted matrix with singular matrix to get intersection point
	    t=(ainv[0][0]*(fptr->vtx[1]->x - plane[0][0]))+(ainv[1][0]*(fptr->vtx[1]->y - plane[0][1]))+(ainv[2][0]*(fptr->vtx[1]->z - plane[0][2]));

	    vptr2=vertex_make();
	    vptr2->x=fptr->vtx[1]->x + (fptr->vtx[2]->x - fptr->vtx[1]->x)*t;
	    vptr2->y=fptr->vtx[1]->y + (fptr->vtx[2]->y - fptr->vtx[1]->y)*t;
	    vptr2->z=z_level;
	    vptr2->attr=1;
	    z_flag2=1;
	    }	

	  // check if this edge is crossing... if so, process intersection
	  if((fptr->vtx[2]->z<=z_level && fptr->vtx[0]->z>z_level) || (fptr->vtx[2]->z>=z_level && fptr->vtx[0]->z<z_level))
	    {
	    // load edge and surface params into 2d array
	    a[0][0]=fptr->vtx[2]->x - fptr->vtx[0]->x;
	    a[0][1]=plane[1][0]-plane[0][0];
	    a[0][2]=plane[2][0]-plane[0][0];
	    a[1][0]=fptr->vtx[2]->y - fptr->vtx[0]->y;
	    a[1][1]=plane[1][1]-plane[0][1];
	    a[1][2]=plane[2][1]-plane[0][1];
	    a[2][0]=fptr->vtx[2]->z - fptr->vtx[0]->z;
	    a[2][1]=plane[1][2]-plane[0][2];
	    a[2][2]=plane[2][2]-plane[0][2];
	    // caculate the determinant
	    determinant=0;
	    for(i=0;i<3;i++)
	      {
	      determinant = determinant + (a[0][i]*(a[1][(i+1)%3]*a[2][(i+2)%3] - a[1][(i+2)%3]*a[2][(i+1)%3]));
	      }
	    // perform singular inversion
	    for(i=0;i<3;i++)
	      {
	      ainv[i][0]=( (a[(i+1)%3][1%3] * a[(i+2)%3][2%3]) - (a[(i+1)%3][2%3]*a[(i+2)%3][1%3]) ) / determinant;
	      }
	    // multiply inverted matrix with singular matrix to get intersection point
	    t=(ainv[0][0]*(fptr->vtx[2]->x - plane[0][0]))+(ainv[1][0]*(fptr->vtx[2]->y - plane[0][1]))+(ainv[2][0]*(fptr->vtx[2]->z - plane[0][2]));

	    vptr3=vertex_make();
	    vptr3->x=fptr->vtx[2]->x + (fptr->vtx[0]->x - fptr->vtx[2]->x)*t;
	    vptr3->y=fptr->vtx[2]->y + (fptr->vtx[0]->y - fptr->vtx[2]->y)*t;
	    vptr3->z=z_level;
	    vptr3->attr=1;
	    z_flag3=1;
	    }	

	  //printf("    facet=%X z1=%d z2=%d z3=%d\n",fptr,z_flag1,z_flag2,z_flag3);

	  // At this point we have hopefully discovered 2 edges that cross the z plane.
	  // These two edges have created 2 vertices that define the vector where the facet intersects the plane.
	  // Load this vector into the raw vector list which will be processed into polygons later.
	  if(z_flag2==1 && z_flag3==1)					// ... then vptr2 and vptr3 must be edges that cross
	    {
	    if(vertex_compare(vptr2,vptr3,ALMOST_ZERO)==FALSE)		// if this is not a zero length vector...
	      {
	      vecptr=vector_make(vptr2,vptr3,0);			// make a new vector list element of model material type
	      vecptr->member=fptr->member;				// keep track of which body this vector originated from
	      vector_raw_insert(sptr,ptyp,vecptr);			// insert it into the linked list of vectors
	      if(vptr1!=NULL){free(vptr1); vertex_mem--;vptr1=NULL;}	// decrement vertex in memory counter
	      }
	    else 							// otherwise, do not create the vector
	      {
	      if(vptr2!=NULL){free(vptr2); vertex_mem--;vptr2=NULL;}
	      if(vptr3!=NULL){free(vptr3); vertex_mem--;vptr3=NULL;}
	      }
	    }
	  if(z_flag1==1 && z_flag3==1)					// ... then vptr1 and vptr3 must be edges that cross
	    {
	    if(vertex_compare(vptr1,vptr3,ALMOST_ZERO)==FALSE)		// if this is not a zero length vector...
	      {
	      vecptr=vector_make(vptr1,vptr3,0);			// make a new vector list element of type build
	      vecptr->member=fptr->member;				// keep track of which body this vector originated from
	      vector_raw_insert(sptr,ptyp,vecptr);			// insert it into the linked list of vectors
	      if(vptr2!=NULL){free(vptr2); vertex_mem--;vptr2=NULL;}	// decrement vertex in memory counter
	      }
	    else 							// otherwise, do not create the vector
	      {
	      if(vptr1!=NULL){free(vptr1); vertex_mem--;vptr1=NULL;}
	      if(vptr3!=NULL){free(vptr3); vertex_mem--;vptr3=NULL;}
	      }
	    }
	  if(z_flag1==1 && z_flag2==1)					// ... then vptr1 and vptr2 must be edges that cross
	    {
	    if(vertex_compare(vptr1,vptr2,ALMOST_ZERO)==FALSE)		// if this is not a zero length vector...
	      {
	      vecptr=vector_make(vptr1,vptr2,0);			// make a new vector list element of type build
	      vecptr->member=fptr->member;				// keep track of which body this vector originated from
	      vector_raw_insert(sptr,ptyp,vecptr);			// insert it into the linked list of vectors
	      if(vptr3!=NULL){free(vptr3); vertex_mem--;vptr3=NULL;}	// decrement vertex in memory counter
	      }
	    else 							// otherwise, do not create the vector
	      {
	      if(vptr1!=NULL){free(vptr1); vertex_mem--;vptr1=NULL;}
	      if(vptr2!=NULL){free(vptr2); vertex_mem--;vptr2=NULL;}
	      }
	    }

	  // move onto next facet in linked list
	  fptr=fptr->next;
	  }
	
	// if a slice was generated, convert vectors to polygons...
	if(sptr!=NULL)
	  {
	  //printf("    new slice:  sptr=%X  veclist=%X \n",sptr,sptr->raw_vec_first[ptyp]);
	  
	  // save a copy raw stl vectors for debug
	  /*
	  if(mtyp==MODEL && ptyp==MDL_PERIM)
	    {
	    old_vec_mem=vector_mem;
	    vecold=NULL;
	    sptr->stl_vec_first=NULL;
	    vecptr=sptr->raw_vec_first[ptyp];
	    while(vecptr!=NULL)
	      {
	      vecnew=vector_copy(vecptr);
	      if(sptr->stl_vec_first==NULL)sptr->stl_vec_first=vecnew;
	      if(vecold!=NULL)vecold->next=vecnew;
	      vecnew->type=ptyp;
	      vecold=vecnew;
	      vecptr=vecptr->next;
	      }
	    printf("  ... raw stl vector qty = %ld \n",(vector_mem-old_vec_mem));
	    }
	  */
	  
	  // sort the huge list of resulting vectors tip-to-tail into prepolygon groups held in vec_list.  vec_list
	  // is a list of pointers where each node points to the head vector in a prepolygon loop.  note that calling
	  // this function will destroy the linkage of the input vector list.  the only way to reach all elements of
	  // the input list will be thru the pointers of the vec_list.
	  //printf("    enter vector sort.\n");
	  sptr->vec_list[ptyp]=vector_sort(sptr->raw_vec_first[ptyp],TOLERANCE,FALSE); // new and only access ptr into vectors
	  //printf("    exit vector sort.\n");
	  
	  //if(vector_list_check(sptr->vec_list[ptyp])<1)while(!kbhit());
	  
	  // loop thru the list of prepolygons (i.e. vec_list) and merge together vectors that fall within
	  // angular tolerance or are shorter than the minimum vector length.
	  //printf("    enter vector merge.\n");
	  vl_ptr=sptr->vec_list[ptyp];
	  while(vl_ptr!=NULL)
	    {
	    if(vl_ptr->v_item!=NULL)vl_ptr->v_item=vector_colinear_merge(vl_ptr->v_item,ptyp,max_colinear_angle);
	    vl_ptr=vl_ptr->next;
	    }
	  //printf("    exit vector merge.\n");
	    
	  // ensure vector status is set properly for down stream processing
	  vl_ptr=sptr->vec_list[ptyp];
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
	  //printf("    vectors status reset...\n");
	  
	  // copy sorted/merged vector list loops to polygons of specific line type
	  pold=NULL;
	  sptr->pqty[ptyp]=0;
	  vl_ptr=sptr->vec_list[ptyp];
	  while(vl_ptr!=NULL)
	    {
	    vecptr=vl_ptr->v_item;
	    if(vecptr!=NULL)pnew=veclist_2_single_polygon(vecptr,ptyp);
	    if(pnew!=NULL)
	      {
	      sptr->pqty[ptyp]++;
	      pnew->ID=sptr->pqty[ptyp];				// assign unique ID to each polygon with this slice
	      pnew->perim_type=1;					// set to "from file"
	      pnew->fill_type=1;					// set to "from file"
	      if(sptr->pfirst[ptyp]==NULL)sptr->pfirst[ptyp]=pnew;
	      if(pold!=NULL)pold->next=pnew;
	      pold=pnew;
	      }
	    vl_ptr=vl_ptr->next;
	    }
	    
	  // purge vector list now that polygons are made
	  if(sptr->vec_list[ptyp]!=NULL)
	    {
	    vl_ptr=sptr->vec_list[ptyp];
	    while(vl_ptr!=NULL)
	      {
	      vecptr=vl_ptr->v_item;
	      vector_purge(vecptr);
	      vl_ptr=vl_ptr->next;
	      if(vl_ptr==sptr->vec_list[ptyp])break;
	      }
	    sptr->vec_list[ptyp]=vector_list_manager(sptr->vec_list[ptyp],NULL,ACTION_CLEAR);
	    sptr->raw_vec_qty[ptyp]=0;
	    sptr->raw_vec_first[ptyp]=NULL;
	    sptr->raw_vec_last[ptyp]=NULL;
	    }
	    
	  //if(ptyp==MDL_PERIM)printf("    %X %d model polygons made.\n",sptr,sptr->pqty[ptyp]);
	  //if(ptyp==SPT_PERIM)printf("    %X %d support polygons made.\n",sptr,sptr->pqty[ptyp]);
	  sptr->plast[ptyp]=pold;
	  
	  }

	//printf("   Model raw slice:  Exit  sptr=%X\n",sptr);
	//printf("Model raw slice exit:   vec count = %ld \n",vector_mem);
	return(sptr);
}

// Function to quickly estimate the amount of support needed
// this works by crudely estimating the volume of support.  the volume is caclulated by
// summing the area * height of each downward facing facet in each direction.  the model
// is not rotated to do this.  it is done by "rotating the build plate".
// return value is volume in mm^3
// Definitions:	orient[0] = z min    	orient[1] = y min	orient[2] = x min
//              orient[3] = z max	orient[4] = y max	orient[5] = x max
int model_support_estimate(model *mptr)
{
  int		i,mtyp=MODEL;
  float 	max_angle,sli_thk;
  float 	x01,x12,x20,y01,y12,y20,z01,z12,z20;
  float 	A,B,C,S,area;
  facet 	*fptr;
  vertex 	*vctr;
  
  if(mptr==NULL)return(0);
  model_maxmin(mptr);							// ensure bounds are known and vtxs are shifted accordingly
  max_angle=cos(-45*PI/180);						// normalizes angle (PI/2 to -PI/2) to match unit normals (1 to -1 range)
  sli_thk=1.0;								// mat'l not yet assigned, so use typical value
  
  // clear any existing values
  for(i=0;i<6;i++)mptr->svol[i]=0;

  vctr=vertex_make();							// holds the facet centroid (re-used)
  fptr=mptr->facet_first[MODEL];
  while(fptr!=NULL)
    {
    facet_centroid(fptr,vctr);
    x01=fptr->vtx[0]->x-fptr->vtx[1]->x; x12=fptr->vtx[1]->x-fptr->vtx[2]->x; x20=fptr->vtx[2]->x-fptr->vtx[0]->x;
    y01=fptr->vtx[0]->y-fptr->vtx[1]->y; y12=fptr->vtx[1]->y-fptr->vtx[2]->y; y20=fptr->vtx[2]->y-fptr->vtx[0]->y;
    z01=fptr->vtx[0]->z-fptr->vtx[1]->z; z12=fptr->vtx[1]->z-fptr->vtx[2]->z; z20=fptr->vtx[2]->z-fptr->vtx[0]->z;
    
    // check zmin direction
    if((fptr->unit_norm)->z<0 && (fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
      {
      if(vctr->z>(mptr->zmin[mtyp]+sli_thk))     				// ignore facets that would be on build table
        {
	// calc area of facets in XY plane only (ignore Z)
	A=sqrt(x01*x01+y01*y01);
	B=sqrt(x12*x12+y12*y12);
	C=sqrt(x20*x20+y20*y20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[0] += (area*(vctr->z-mptr->zmin[mtyp]));
	}
      }

    // check ymin direction
    if((fptr->unit_norm)->y<0 && (fptr->unit_norm->y*fptr->unit_norm->y)>fabs(max_angle))
      {
      if(vctr->y>(mptr->ymin[mtyp]+sli_thk))     			// only consider facets that are above zmin
        {
	// calc area of facets in XZ plane only (ignore Y)
	A=sqrt(x01*x01+z01*z01);
	B=sqrt(x12*x12+z12*z12);
	C=sqrt(x20*x20+z20*z20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[1] += (area*(vctr->y-mptr->ymin[mtyp]));
	}
      }

    // check xmin direction
    if((fptr->unit_norm)->x<0 && (fptr->unit_norm->x*fptr->unit_norm->x)>fabs(max_angle))
      {
      if(vctr->x>(mptr->xmin[mtyp]+sli_thk))     			// only consider facets that are above zmin
        {
	// calc area of facets in YZ plane only (ignore X)
	A=sqrt(y01*y01+z01*z01);
	B=sqrt(y12*y12+z12*z12);
	C=sqrt(y20*y20+z20*z20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[2] += (area*(vctr->x-mptr->xmin[mtyp]));
	}
      }

    // check zmax direction
    if((fptr->unit_norm)->z>0 && (fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
      {
      if(vctr->z<(mptr->zmax[mtyp]-sli_thk))     			// only consider facets that are below zmax
        {
	// calc area of facets in XY plane only (ignore Z)
	A=sqrt(x01*x01+y01*y01);
	B=sqrt(x12*x12+y12*y12);
	C=sqrt(x20*x20+y20*y20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[3] += (area*(mptr->zmax[mtyp]-vctr->z));
	}
      }
    
    // check ymax direction
    if((fptr->unit_norm)->y>0 && (fptr->unit_norm->y*fptr->unit_norm->y)>fabs(max_angle))
      {
      if(vctr->y<(mptr->ymax[mtyp]-sli_thk))			// only consider facets that are above zmin
        {
	// calc area of facets in XZ plane only (ignore Y)
	A=sqrt(x01*x01+z01*z01);
	B=sqrt(x12*x12+z12*z12);
	C=sqrt(x20*x20+z20*z20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[4] += (area*(mptr->ymax[mtyp]-vctr->y));
	}
      }

    // check xmax direction
    if((fptr->unit_norm)->x>0 && (fptr->unit_norm->x*fptr->unit_norm->x)>fabs(max_angle))
      {
      if(vctr->x<(mptr->xmax[mtyp]-sli_thk))     			// only consider facets that are above zmin
        {
	// calc area of facets in YZ plane only (ignore X)
	A=sqrt(y01*y01+z01*z01);
	B=sqrt(y12*y12+z12*z12);
	C=sqrt(y20*y20+z20*z20);
	S=(A+B+C)/2;
	area=sqrt(fabs(S*(S-A)*(S-B)*(S-C)));
	mptr->svol[5] += (area*(mptr->xmax[mtyp]-vctr->x));
	}
      }

    fptr=fptr->next;
    }
    
  return(TRUE);
}

// Function to determine the best build orientation and scale for a model
int model_auto_orient(model *mptr)
{
  int		i,j,layers_id,support_id,mtyp=MODEL;
  float 	def_slice_thk;
  float 	layers_min,support_min;
  float 	xscale,yscale,zscale,allscale;
  
  if(mptr==NULL)return(0);
  def_slice_thk=0.200;
  
  // get data on given orientation
  model_maxmin(mptr);	
  
  // if too big, automatically scale to fit
  xscale=mptr->xmax[MODEL]/BUILD_TABLE_LEN_X;				// determine fit in each axis
  yscale=mptr->ymax[MODEL]/BUILD_TABLE_LEN_Y;
  zscale=mptr->zmax[MODEL]/BUILD_TABLE_LEN_Z;
  if(xscale>1 || yscale>1 || zscale>1)
    {
    allscale=xscale;							// determine max out of bounds value
    if(yscale>allscale)allscale=yscale;
    if(zscale>allscale)allscale=zscale;
    allscale = 1/allscale;						// set to reduce size to fit
    mptr->xscl[MODEL]=allscale;						// set scale values
    mptr->yscl[MODEL]=allscale;
    mptr->zscl[MODEL]=allscale;
    model_scale(mptr,MODEL);						// execute scale
    }
  
  // get support volume estimate at this (the loaded default) orientation
  model_support_estimate(mptr);
  
  // calculate layer requirements for each orientation
  mptr->lyrs[0]=(mptr->zmax[mtyp]-mptr->zmin[mtyp])/def_slice_thk;			// across z
  mptr->lyrs[1]=(mptr->ymax[mtyp]-mptr->ymin[mtyp])/def_slice_thk;			// across y
  mptr->lyrs[2]=(mptr->xmax[mtyp]-mptr->xmin[mtyp])/def_slice_thk;			// across x

  // find orientation with minimal layers
  layers_id=0;
  layers_min=mptr->lyrs[layers_id];
  for(i=1;i<3;i++)
    {
    if(mptr->lyrs[i]<layers_min)
      {
      layers_min=mptr->lyrs[i];
      layers_id=i;
      }
    }
  //printf("layers_min=%f  layers_id=%d \n",layers_min,layers_id);

  // find orientation with minimal support
  support_id=0;
  support_min=mptr->svol[support_id];
  for(i=1;i<6;i++)
    {
    if(mptr->svol[i]<support_min)
      {
      support_min=mptr->svol[i];
      support_id=i;
      }
    }
  printf("support_min=%f  support_id=%d \n",support_min,support_id);
  
  printf("\nLeast Layers = %d   Least Support = %d\n",layers_id,support_id);
  for(i=0;i<6;i++)
    {
    j=i;
    if(j>2)j-=3;
    if(i==0)printf(" %d  dZ=%6.1f  lyrs=%6.1f  svol=%8.1f   none\n",i,(mptr->zmax[mtyp]-mptr->zmin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    if(i==1)printf(" %d  dY=%6.1f  lyrs=%6.1f  svol=%8.1f   90 x\n",i,(mptr->ymax[mtyp]-mptr->ymin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    if(i==2)printf(" %d  dX=%6.1f  lyrs=%6.1f  svol=%8.1f   90 y\n",i,(mptr->xmax[mtyp]-mptr->xmin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    if(i==3)printf(" %d  dZ=%6.1f  lyrs=%6.1f  svol=%8.1f  180 x\n",i,(mptr->zmax[mtyp]-mptr->zmin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    if(i==4)printf(" %d  dY=%6.1f  lyrs=%6.1f  svol=%8.1f  -90 x\n",i,(mptr->ymax[mtyp]-mptr->ymin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    if(i==5)printf(" %d  dX=%6.1f  lyrs=%6.1f  svol=%8.1f  -90 y\n",i,(mptr->xmax[mtyp]-mptr->xmin[mtyp]),mptr->lyrs[j],mptr->svol[i]);
    }

  // set the model rotations from original (orient=0) to the optimize orientation values
  if(support_id==1){mptr->xrot[mtyp]=90*PI/180;    mptr->yrot[mtyp]=0.0;         }	// if miny, rotate about x
  if(support_id==2){mptr->xrot[mtyp]=0.0;          mptr->yrot[mtyp]=90*PI/180;   }	// if minx, rotate about y
  if(support_id==3){mptr->xrot[mtyp]=180*PI/180;   mptr->yrot[mtyp]=0.0;         }	// flip z upside down
  if(support_id==4){mptr->xrot[mtyp]=(-90)*PI/180; mptr->yrot[mtyp]=0.0;         }
  if(support_id==5){mptr->xrot[mtyp]=0.0;          mptr->yrot[mtyp]=(-90)*PI/180;}
  
  // finally, rotate about z such that longer dimension is aligned with y axis
  #ifdef BETA_UNIT
    if((mptr->xmax[mtyp]-mptr->xmin[mtyp])>(mptr->ymax[mtyp]-mptr->ymin[mtyp]))mptr->zrot[mtyp]=90*PI/180;
  #endif

  // if model UI screen is up, adjust size values to match suggested orientation
  if(win_modelUI_flag==TRUE)update_size_values(mptr);

  mptr->reslice_flag=TRUE;
  
  //printf("Model auto-orient done: exiting \n");
  
  return(1);
}

// Function to determine if a model overlaps any other models in the same 3D space
int model_overlap(model *minp)
{
  int		mtyp=MODEL;
  int		status=FALSE;
  int		xin,yin,zin;
  float 	dix,diy,diz,dmx,dmy,dmz;
  model 	*mptr;
  
  dix=(minp->xmax[mtyp] - minp->xmin[mtyp] + minp->xstp)*minp->xcopies - minp->xstp;
  diy=(minp->ymax[mtyp] - minp->ymin[mtyp] + minp->ystp)*minp->ycopies - minp->ystp;
  diz= minp->zmax[mtyp] - minp->zmin[mtyp];

  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(mptr==minp){mptr=mptr->next; continue;}
    
    xin=FALSE;
    yin=FALSE;
    zin=FALSE;
  
    dmx=(mptr->xmax[mtyp] - mptr->xmin[mtyp] + mptr->xstp)*mptr->xcopies - mptr->xstp;
    dmy=(mptr->ymax[mtyp] - mptr->ymin[mtyp] + mptr->ystp)*mptr->ycopies - mptr->ystp;
    dmz= mptr->zmax[mtyp] - mptr->zmin[mtyp];
  
    if( minp->xorg[mtyp] >=    mptr->xorg[mtyp] &&  minp->xorg[mtyp] <=    (mptr->xorg[mtyp]+dmx))xin=TRUE;	// is xmax of input model is inside test model...
    if((minp->xorg[mtyp]+dix)>=mptr->xorg[mtyp] && (minp->xorg[mtyp]+dix)<=(mptr->xorg[mtyp]+dmx))xin=TRUE;	// is xmin of input model is inside test model...
    if( minp->xorg[mtyp] <=    mptr->xorg[mtyp] && (minp->xorg[mtyp]+dix)>=(mptr->xorg[mtyp]+dmx))xin=TRUE;	// is all of test model inside input model...
    if( minp->xorg[mtyp] >=    mptr->xorg[mtyp] && (minp->xorg[mtyp]+dix)<=(mptr->xorg[mtyp]+dmx))xin=TRUE;	// is all of input model inside test model...
  
    if(xin==TRUE)
      {
      if( minp->yorg[mtyp] >=    mptr->yorg[mtyp] &&  minp->yorg[mtyp] <=    (mptr->yorg[mtyp]+dmy))yin=TRUE;		// is xmax of input model is inside test model...
      if((minp->yorg[mtyp]+diy)>=mptr->yorg[mtyp] && (minp->yorg[mtyp]+diy)<=(mptr->yorg[mtyp]+dmy))yin=TRUE;	// is xmin of input model is inside test model...
      if( minp->yorg[mtyp] <=    mptr->yorg[mtyp] && (minp->yorg[mtyp]+diy)>=(mptr->yorg[mtyp]+dmy))yin=TRUE;		// is all of test model inside input model...
      if( minp->yorg[mtyp] >=    mptr->yorg[mtyp] && (minp->yorg[mtyp]+diy)<=(mptr->yorg[mtyp]+dmy))yin=TRUE;		// is all of input model inside test model...
      }
  
    // if xy boxes do not overlap, check if they overlap in z 
    if(xin==FALSE && yin==FALSE)
      {
      if( minp->zorg[mtyp] >=    mptr->zorg[mtyp] &&  minp->zorg[mtyp] <=    (mptr->zorg[mtyp]+dmz))zin=TRUE;		// is xmax of input model is inside test model...
      if((minp->zorg[mtyp]+diz)>=mptr->zorg[mtyp] && (minp->zorg[mtyp]+diz)<=(mptr->zorg[mtyp]+dmz))zin=TRUE;	// is xmin of input model is inside test model...
      if( minp->zorg[mtyp] <=    mptr->zorg[mtyp] && (minp->zorg[mtyp]+diz)>=(mptr->zorg[mtyp]+dmz))zin=TRUE;		// is all of test model inside input model...
      if( minp->zorg[mtyp] >=    mptr->zorg[mtyp] && (minp->zorg[mtyp]+diz)<=(mptr->zorg[mtyp]+dmz))zin=TRUE;		// is all of input model inside test model...
      }
    
    if(xin==TRUE && yin==TRUE && zin==TRUE)status=TRUE;
    if(status==TRUE)break;
    mptr=mptr->next;
    }
  
  //printf("Model overlap: %d \n",status);
  return(status);
}

// Function to determine if a model falls outside the build volume
int model_out_of_bounds(model *mptr)
{
  int		status=FALSE,mtyp=MODEL;
  int		xo,yo,zo;
  float 	dx,dy,dz;
  
  dx=(mptr->xmax[mtyp] - mptr->xmin[mtyp] + mptr->xstp)*mptr->xcopies - mptr->xstp;
  dy=(mptr->ymax[mtyp] - mptr->ymin[mtyp] + mptr->ystp)*mptr->ycopies - mptr->ystp;
  dz=mptr->zmax[mtyp]-mptr->zmin[mtyp];

  xo=FALSE; yo=FALSE; zo=FALSE;
  if(mptr->xorg[mtyp]<BUILD_TABLE_MIN_X || (mptr->xorg[mtyp]+dx)>BUILD_TABLE_LEN_X){xo=TRUE;status=TRUE;}
  if(mptr->yorg[mtyp]<BUILD_TABLE_MIN_Y || (mptr->yorg[mtyp]+dy)>BUILD_TABLE_LEN_Y){yo=TRUE;status=TRUE;}
  if(mptr->zorg[mtyp]<BUILD_TABLE_MIN_Z || (mptr->zorg[mtyp]+dz)>BUILD_TABLE_LEN_Z){zo=TRUE;status=TRUE;}

  //printf(" x=%f \n",mptr->xorg[mtyp]);
  //printf(" y=%f \n",mptr->yorg[mtyp]);
  //printf(" z=%f \n",mptr->zorg[mtyp]);
  //printf("Model out of bounds: x=%d y=%d z=%d \n",xo,yo,zo);
  return(status);
}

// Function to convert data from one model geometry type to another model goemetery type
// for example:  move the facets that were generated as SUPPORT to become MODEL
// it is easy to confuse this with input type (STL, AMF, IMAGE, etc.)... don't do that.
int model_convert_type(model *mptr,int target_type)
{
  int	i,current_type;
  
  // validate inputs
  if(mptr==NULL)return(FALSE);
  if(target_type<1 || target_type>=MAX_MDL_TYPES)return(FALSE);
  
  // determine what the existing model type is by first checking for facets in STL
  if(mptr->input_type==STL)
    {
    for(current_type=MODEL;current_type<MAX_MDL_TYPES;current_type++)
      {
      if(mptr->facet_qty[current_type]>0)break;
      }
    if(current_type==MAX_MDL_TYPES)return(FALSE);			// nothing was found
    }
    
  // make sure current and target are different types
  if(current_type==target_type)return(FALSE);
    
  // copy model data from current to target type
  mptr->vertex_qty[target_type]=mptr->vertex_qty[current_type];		// vertexes
  mptr->vertex_first[target_type]=mptr->vertex_first[current_type];
  mptr->vertex_last[target_type]=mptr->vertex_last[current_type];

  mptr->facet_qty[target_type]=mptr->facet_qty[current_type];		// facets
  mptr->facet_first[target_type]=mptr->facet_first[current_type];
  mptr->facet_last[target_type]=mptr->facet_last[current_type];

  mptr->edge_qty[target_type]=mptr->edge_qty[current_type];		// edges
  mptr->edge_first[target_type]=mptr->edge_first[current_type];
  mptr->edge_last[target_type]=mptr->edge_last[current_type];
  
  mptr->xmin[target_type]=mptr->xmin[current_type];			// min bounds
  mptr->ymin[target_type]=mptr->ymin[current_type];
  mptr->zmin[target_type]=mptr->zmin[current_type];
  
  mptr->xmax[target_type]=mptr->xmax[current_type];			// max bounds
  mptr->ymax[target_type]=mptr->ymax[current_type];
  mptr->zmax[target_type]=mptr->zmax[current_type];

  mptr->xoff[target_type]=mptr->xoff[current_type];			// current position offsets
  mptr->yoff[target_type]=mptr->yoff[current_type];
  mptr->zoff[target_type]=mptr->zoff[current_type];

  mptr->xorg[target_type]=mptr->xorg[current_type];			// original position offsets
  mptr->yorg[target_type]=mptr->yorg[current_type];
  mptr->zorg[target_type]=mptr->zorg[current_type];

  mptr->xrot[target_type]=mptr->xrot[current_type];			// current rotation
  mptr->yrot[target_type]=mptr->yrot[current_type];
  mptr->zrot[target_type]=mptr->zrot[current_type];

  mptr->xrpr[target_type]=mptr->xrpr[current_type];			// previous rotation
  mptr->yrpr[target_type]=mptr->yrpr[current_type];
  mptr->zrpr[target_type]=mptr->zrpr[current_type];

  mptr->xscl[target_type]=mptr->xscl[current_type];			// current scale
  mptr->yscl[target_type]=mptr->yscl[current_type];
  mptr->zscl[target_type]=mptr->zscl[current_type];

  mptr->xspr[target_type]=mptr->xspr[current_type];			// previous scale
  mptr->yspr[target_type]=mptr->yspr[current_type];
  mptr->zspr[target_type]=mptr->zspr[current_type];

  mptr->xmir[target_type]=mptr->xmir[current_type];			// mirror
  mptr->ymir[target_type]=mptr->ymir[current_type];
  mptr->zmir[target_type]=mptr->zmir[current_type];

  // zero out current target type values
  mptr->vertex_qty[current_type]=0;					// vertexes
  mptr->vertex_first[current_type]=NULL;
  mptr->vertex_last[current_type]=NULL;

  mptr->facet_qty[current_type]=0;					// facets
  mptr->facet_first[current_type]=NULL;
  mptr->facet_last[current_type]=NULL;

  mptr->edge_qty[current_type]=0;					// edges
  mptr->edge_first[current_type]=NULL;
  mptr->edge_last[current_type]=NULL;
  
  mptr->xmin[current_type]=INT_MAX_VAL;					// min bounds
  mptr->ymin[current_type]=INT_MAX_VAL;
  mptr->zmin[current_type]=INT_MAX_VAL;
  
  mptr->xmax[current_type]=(INT_MIN_VAL);				// max bounds
  mptr->ymax[current_type]=(INT_MIN_VAL);
  mptr->zmax[current_type]=(INT_MIN_VAL);

  mptr->xoff[current_type]=0;						// current position offsets
  mptr->yoff[current_type]=0;
  mptr->zoff[current_type]=0;

  mptr->xorg[current_type]=0;						// original position offsets
  mptr->yorg[current_type]=0;
  mptr->zorg[current_type]=0;

  mptr->xrot[current_type]=0;						// current rotation
  mptr->yrot[current_type]=0;
  mptr->zrot[current_type]=0;

  mptr->xrpr[current_type]=0;						// previous rotation
  mptr->yrpr[current_type]=0;
  mptr->zrpr[current_type]=0;

  mptr->xscl[current_type]=1.0;						// current scale
  mptr->yscl[current_type]=1.0;
  mptr->zscl[current_type]=1.0;

  mptr->xspr[current_type]=1.0;						// previous scale
  mptr->yspr[current_type]=1.0;
  mptr->zspr[current_type]=1.0;

  mptr->xmir[current_type]=0;						// mirror
  mptr->ymir[current_type]=0;
  mptr->zmir[current_type]=0;
  
  return(TRUE);
}

// Function to build outer block around model for cnc milling
// One of the "tricks" emplyed here is to pad the block size in the Z by the amount the user wants
// to lift the model when cutting.  That is, the facets built here are the z height of the model, plus
// the margin, plus the extra height they want it lifted.
int model_build_target(model *mptr)
{
  int		mtyp;
  float 	zadd,zdelta,old_y;
  vertex	*vptr,*vtx_btm[4],*vtx_top[4];
  facet		*fptr;

  // if block already added to model
  if(mptr->facet_first[TARGET]!=NULL)return(FALSE);
  
  mtyp=TARGET;	
  mptr->target_x_margin=cnc_x_margin;
  mptr->target_y_margin=cnc_y_margin;
  mptr->target_z_height=cnc_z_height;

  // add four vtx around xy boundaries at offset distance below build table
  vtx_btm[0]=vertex_make();
  vtx_btm[0]->x=mptr->xmin[MODEL]-mptr->target_x_margin;
  vtx_btm[0]->y=mptr->ymin[MODEL]-mptr->target_y_margin;
  vtx_btm[0]->z=0;							// zero because it always sits on the z table
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_btm[0],TARGET);
  
  vtx_btm[1]=vertex_make();
  vtx_btm[1]->x=mptr->xmin[MODEL]-mptr->target_x_margin;
  vtx_btm[1]->y=mptr->ymax[MODEL]+mptr->target_y_margin;
  vtx_btm[1]->z=0;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_btm[1],TARGET);

  vtx_btm[2]=vertex_make();
  vtx_btm[2]->x=mptr->xmax[MODEL]+mptr->target_x_margin;
  vtx_btm[2]->y=mptr->ymax[MODEL]+mptr->target_y_margin;
  vtx_btm[2]->z=0;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_btm[2],TARGET);

  vtx_btm[3]=vertex_make();
  vtx_btm[3]->x=mptr->xmax[MODEL]+mptr->target_x_margin;
  vtx_btm[3]->y=mptr->ymin[MODEL]-mptr->target_y_margin;
  vtx_btm[3]->z=0;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_btm[3],TARGET);

  // add four vtx around xy boundaries just above z-max level
  // check if user wants to subtract the model from a base block, the block will run from
  // mptr->zmax down to the table with z_height.  if the user wants to cut the positive, the
  // block will run from mptr-zmin down to the table with z height.
  zadd=cnc_z_height;
  
  vtx_top[0]=vertex_make();
  vtx_top[0]->x=mptr->xmin[MODEL]-mptr->target_x_margin;
  vtx_top[0]->y=mptr->ymin[MODEL]-mptr->target_y_margin;
  vtx_top[0]->z=zadd;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_top[0],TARGET);
  
  vtx_top[1]=vertex_make();
  vtx_top[1]->x=mptr->xmin[MODEL]-mptr->target_x_margin;
  vtx_top[1]->y=mptr->ymax[MODEL]+mptr->target_y_margin;
  vtx_top[1]->z=zadd;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_top[1],TARGET);

  vtx_top[2]=vertex_make();
  vtx_top[2]->x=mptr->xmax[MODEL]+mptr->target_x_margin;
  vtx_top[2]->y=mptr->ymax[MODEL]+mptr->target_y_margin;
  vtx_top[2]->z=zadd;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_top[2],TARGET);

  vtx_top[3]=vertex_make();
  vtx_top[3]->x=mptr->xmax[MODEL]+mptr->target_x_margin;
  vtx_top[3]->y=mptr->ymin[MODEL]-mptr->target_y_margin;
  vtx_top[3]->z=zadd;
  vertex_insert(mptr,mptr->vertex_last[TARGET],vtx_top[3],TARGET);
  
  // build top facets that connect top 4 vtxs.  not needed for slicing, but good for visualization.
  fptr=facet_make();
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  vtx_top[1]->flist=facet_list_manager(vtx_top[1]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_top[0]; fptr->vtx[1]=vtx_top[2]; fptr->vtx[2]=vtx_top[1];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  vtx_top[3]->flist=facet_list_manager(vtx_top[3]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_top[0]; fptr->vtx[1]=vtx_top[3]; fptr->vtx[2]=vtx_top[2];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  // build sidewall facets that connect btm 4 vtxs to top 4 vtxs
  fptr=facet_make();
  vtx_btm[0]->flist=facet_list_manager(vtx_btm[0]->flist,fptr,ACTION_ADD);
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  vtx_btm[1]->flist=facet_list_manager(vtx_btm[1]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[0]; fptr->vtx[1]=vtx_top[0]; fptr->vtx[2]=vtx_btm[1];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[1]->flist=facet_list_manager(vtx_btm[1]->flist,fptr,ACTION_ADD);
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  vtx_top[1]->flist=facet_list_manager(vtx_top[1]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[1]; fptr->vtx[1]=vtx_top[0]; fptr->vtx[2]=vtx_top[1];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[1]->flist=facet_list_manager(vtx_btm[1]->flist,fptr,ACTION_ADD);
  vtx_top[1]->flist=facet_list_manager(vtx_top[1]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[1]; fptr->vtx[1]=vtx_top[1]; fptr->vtx[2]=vtx_top[2];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[2]->flist=facet_list_manager(vtx_btm[2]->flist,fptr,ACTION_ADD);
  vtx_btm[1]->flist=facet_list_manager(vtx_btm[1]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[2]; fptr->vtx[1]=vtx_btm[1]; fptr->vtx[2]=vtx_top[2];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[2]->flist=facet_list_manager(vtx_btm[2]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  vtx_btm[3]->flist=facet_list_manager(vtx_btm[3]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[2]; fptr->vtx[1]=vtx_top[2]; fptr->vtx[2]=vtx_btm[3];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[3]->flist=facet_list_manager(vtx_btm[3]->flist,fptr,ACTION_ADD);
  vtx_top[2]->flist=facet_list_manager(vtx_top[2]->flist,fptr,ACTION_ADD);
  vtx_top[3]->flist=facet_list_manager(vtx_top[3]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[3]; fptr->vtx[1]=vtx_top[2]; fptr->vtx[2]=vtx_top[3];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[3]->flist=facet_list_manager(vtx_btm[3]->flist,fptr,ACTION_ADD);
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  vtx_btm[0]->flist=facet_list_manager(vtx_btm[0]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[3]; fptr->vtx[1]=vtx_top[0]; fptr->vtx[2]=vtx_btm[0];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[3]->flist=facet_list_manager(vtx_btm[3]->flist,fptr,ACTION_ADD);
  vtx_top[3]->flist=facet_list_manager(vtx_top[3]->flist,fptr,ACTION_ADD);
  vtx_top[0]->flist=facet_list_manager(vtx_top[0]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[3]; fptr->vtx[1]=vtx_top[3]; fptr->vtx[2]=vtx_top[0];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  // build bottom facets that connect btm 4 vtxs.  not needed for slicing, but good for visualization.
  fptr=facet_make();
  vtx_btm[0]->flist=facet_list_manager(vtx_btm[0]->flist,fptr,ACTION_ADD);
  vtx_btm[2]->flist=facet_list_manager(vtx_btm[2]->flist,fptr,ACTION_ADD);
  vtx_btm[1]->flist=facet_list_manager(vtx_btm[1]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[0]; fptr->vtx[1]=vtx_btm[2]; fptr->vtx[2]=vtx_btm[1];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  fptr=facet_make();
  vtx_btm[0]->flist=facet_list_manager(vtx_btm[0]->flist,fptr,ACTION_ADD);
  vtx_btm[3]->flist=facet_list_manager(vtx_btm[3]->flist,fptr,ACTION_ADD);
  vtx_btm[2]->flist=facet_list_manager(vtx_btm[2]->flist,fptr,ACTION_ADD);
  fptr->vtx[0]=vtx_btm[0]; fptr->vtx[1]=vtx_btm[3]; fptr->vtx[2]=vtx_btm[2];
  facet_unit_normal(fptr);
  facet_insert(mptr,mptr->facet_last[TARGET],fptr,TARGET);
  
  // adjust model max/min boundaries to match new geometry
  mptr->xmin[TARGET] = mptr->xmin[MODEL] - mptr->target_x_margin;
  mptr->xmax[TARGET] = mptr->xmax[MODEL] + mptr->target_x_margin;
  mptr->xorg[TARGET] = mptr->xorg[MODEL];
  mptr->xoff[TARGET] = mptr->xoff[MODEL];
  
  mptr->ymin[TARGET] = mptr->ymin[MODEL] - mptr->target_y_margin;
  mptr->ymax[TARGET] = mptr->ymax[MODEL] + mptr->target_y_margin;
  mptr->yorg[TARGET] = mptr->yorg[MODEL];
  mptr->yoff[TARGET] = mptr->yoff[MODEL];
  
  mptr->zmin[TARGET] = 0.0;						// block is alway on table
  mptr->zmax[TARGET] = cnc_z_height;

  mptr->zoff[MODEL] = mptr->zmax[TARGET];				// lift model to top of block
  mptr->zorg[MODEL] = mptr->zoff[MODEL];
  
  // process new model data
  facet_find_all_neighbors(mptr->facet_first[TARGET]);			// create facet list for each vtx
  model_integrity_check(mptr);						// check for geometry flaws
  edge_display(mptr,set_view_edge_angle,TARGET);			// determine which sharp edges to display in 3D view
  
  mptr->mdl_has_target=TRUE;
  set_view_target=TRUE;	
  
  return(TRUE);
}

// Function to generate profile cuts for cnc milling and laser marking
// this function basically slices the model in the YZ plane vs in the traditional XY plane.
int model_profile_cut(model *mptr)
{
  int		slot,old_add_block,mtyp=MODEL;
  float 	old_z_ht,old_slice_thk,old_border_thk,old_offset_thk,old_x;
  float 	block_z_offset,old_xmax,z1,z2;
  float 	focus_scale=2.0;
  linetype	*lt_ptr;
  vertex 	*vptr,*vstart;
  polygon	*pptr;
  slice 	*sptr;
  operation	*op_ptr;
  genericlist	*aptr,*lt_new;
  
  debug_flag=302;
  if(debug_flag==302)printf("MPC:  Entry:  mptr=%X\n",mptr);
  
  // get tool info
  slot=model_get_slot(mptr,OP_MILL_PROFILE);
  if(slot<0)slot=model_get_slot(mptr,OP_MARK_IMAGE);
  if(slot<0 || slot>=MAX_TOOLS)return(FALSE);
  if(debug_flag==302)printf("   using slot %d \n",slot);
  
  // set mptr slice thk to fill pitch
  lt_ptr=linetype_find(slot,MDL_FILL);
  if(lt_ptr!=NULL)
    {
    old_slice_thk=mptr->slice_thick;	
    mptr->slice_thick=lt_ptr->line_pitch;				// use fill pitch as psuedo layer thickness
    Tool[slot].matl.layer_height=lt_ptr->line_pitch;
    job.min_slice_thk=lt_ptr->line_pitch;
    if(debug_flag==302)printf("   slice_thick reset \n");
    }
    
  // turn off any boarders
  lt_ptr=linetype_find(slot,MDL_BORDER);
  if(lt_ptr!=NULL)
    {
    old_border_thk=lt_ptr->thickness;
    lt_ptr->thickness=0.0;
    if(debug_flag==302)printf("   border thick reset \n");
    }
  
  // turn off any offsets
  lt_ptr=linetype_find(slot,MDL_OFFSET);
  if(lt_ptr!=NULL)
    {
    old_offset_thk=lt_ptr->thickness;
    lt_ptr->thickness=0.0;
    if(debug_flag==302)printf("   offset thick reset \n");
    }

  // apply a extra scaling factor in z to exaggerate values
  mptr->zscl[mtyp]=focus_scale;
  model_scale(mptr,mtyp);
  model_maxmin(mptr);
  job_maxmin();

  // rotate about Y by 90 deg so that we slice up what was the X-axis
  mptr->yrot[mtyp]=90*PI/180;
  model_rotate(mptr);
  model_maxmin(mptr);
  job_maxmin();

  // at this point what was X is now Z.  slice to generate perimeters only
  if(debug_flag==302)printf("\n   slicing... \n\n");
  if(Tool[slot].tool_ID==TC_ROUTER)slice_deck(mptr,MODEL,OP_MILL_PROFILE);
  if(Tool[slot].tool_ID==TC_LASER)slice_deck(mptr,MODEL,OP_MARK_IMAGE);
  if(debug_flag==302)printf("\n   ... done.\n");

  // unrotate model about Y so that the bottom is back on the table
  mptr->yrot[mtyp]=0;
  model_rotate(mptr);
  model_maxmin(mptr);
  job_maxmin();
  
  // rotate slices by swapping X values with Z values, remove the vector that is on the z table,
  // then move the slice up to match the model in z
  if(debug_flag==302)printf("\n   rotating slices... \n");
  sptr=mptr->slice_first[slot];
  if(debug_flag==302)printf("   sptr_first=%X\n",sptr);
  while(sptr!=NULL)
    {
    // first rotate
    // entire slice must be done before attempting to remove table level vector
    pptr=sptr->pfirst[MDL_PERIM];
    while(pptr!=NULL)
      {
      vptr=pptr->vert_first;
      while(vptr!=NULL)
        {
	old_x=vptr->x;
	vptr->x=vptr->z;
	vptr->z=old_x;
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      pptr=pptr->next;
      }
    
    // remove table level vector  
    // do this by simply breaking the link between the two vtxs on the table and
    // then setting the "next" one as the start of an open polygon loop
    pptr=sptr->pfirst[MDL_PERIM];
    while(pptr!=NULL)
      {
      vstart=pptr->vert_first;
      vptr=pptr->vert_first;
      while(vptr!=NULL)							// loop thru all vtxs in this polygon
	{
	if(vptr->z<CLOSE_ENOUGH && (vptr->next)->z<CLOSE_ENOUGH)	// if both vtxs are on the table...
	  {
	  pptr->vert_first=vptr->next;					// ... set start as next vtx
	  vptr->next=NULL;						// ... make break at current vtx
	  break;							// ... exit and move onto next poly
	  }
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      pptr=pptr->next;							// move onto next polygon
      }
     
    sptr=sptr->next;
    }
  if(debug_flag==302)printf("\n   ... done.\n\n");

  // swap x and z maxmins so that we can scroll thru all slices in other functions
  old_x = mptr->zmax[mtyp];
  mptr->zmax[mtyp] = mptr->xmax[mtyp];
  mptr->xmax[mtyp] = old_x;
  old_x = mptr->zmin[mtyp];
  mptr->zmin[mtyp] = mptr->xmin[mtyp];
  mptr->xmin[mtyp] = old_x;
  mptr->target_z_stop=mptr->zmin[mtyp];
  cnc_z_stop=mptr->target_z_stop;
  
  job.XMax=mptr->xmax[mtyp];
  job.XMin=mptr->xmin[mtyp];
  job.YMax=mptr->ymax[mtyp];
  job.YMin=mptr->ymin[mtyp];
  job.ZMax=mptr->zmax[mtyp]+mptr->zoff[mtyp];
  job.ZMin=mptr->zmin[mtyp]+mptr->zoff[mtyp];
    
  // move the model bottom from the table into the block where the top of the model
  // is coincident with the top of the block
  block_z_offset=cnc_z_height-(mptr->zmax-mptr->zmin);			// z distance of bottom of model from table
  if(block_z_offset<0)block_z_offset=0;					// can't be negative to keep from damaging table
  vptr=mptr->vertex_first[MODEL];
  while(vptr!=NULL)							//  move all model vtxs up by block offset
    {
    vptr->z += block_z_offset;
    vptr=vptr->next;
    }
  
  // reset z maxmin to account for block offset add
  mptr->target_z_stop = cnc_z_height-mptr->zmax[mtyp];
  cnc_z_stop=mptr->target_z_stop;
  mptr->zmax[mtyp] += block_z_offset;
  mptr->zmin[mtyp] = 0.0;
  job.ZMax=mptr->zmax[mtyp];
  job.ZMin=mptr->zmin[mtyp];
    
  // if etching, we want to reverse the z component (closer burns more)
  if(Tool[slot].type==MARKING)
    {
    int needs_reverse=TRUE;
    sptr=mptr->slice_first[slot];
    while(sptr!=NULL)
      {
      pptr=sptr->pfirst[MDL_PERIM];
      while(pptr!=NULL)
	{
	if(needs_reverse==TRUE)polygon_reverse(pptr); 
	if(needs_reverse==FALSE){needs_reverse=TRUE;} else {needs_reverse=FALSE;}
	vptr=pptr->vert_first;
	while(vptr!=NULL)
	  {
	  vptr->z *= (-1);						// flip curve (which puts it below table)
	  vptr->z += (mptr->xmax[MODEL]-mptr->xmin[MODEL]);		// move from below table to above table
	  vptr->z += block_z_offset;					// move from on table up to where stl file is
	  vptr=vptr->next;
	  if(vptr==pptr->vert_first)break;
	  }
	pptr=pptr->next;
	}
      sptr=sptr->next;
      }
    }


  // flip slice such that contour is in material and "max" is now out of material.  also
  // resequence vtx order such that we always start with an "out of material" vtx.
  if(debug_flag==302)printf("\n   flipping slices... \n");
  sptr=mptr->slice_first[slot];
  while(sptr!=NULL)
    {
    pptr=sptr->pfirst[MDL_PERIM];
    while(pptr!=NULL)
      {
      vstart=pptr->vert_first;
      vptr=pptr->vert_first;
      while(vptr!=NULL)
	{
	//z1=fabs(vptr->z-block_z_offset);
	//z2=fabs((vptr->next)->z-block_z_offset);
	//if(z1<CLOSE_ENOUGH && z2<CLOSE_ENOUGH)
	if(vptr->z<0 && (vptr->next)->z<0)
	  {
	  //vptr->z=cnc_z_height;
	  //(vptr->next)->z=cnc_z_height;
	  vptr->z=0;
	  (vptr->next)->z=0;
	  vstart=vptr->next;
	  vptr->next=NULL;
	  break;
	  }
	if(vptr->z<0)vptr->z=0;
	vptr=vptr->next;
	if(vptr==pptr->vert_first)break;
	}
      if(vstart!=pptr->vert_first)pptr->vert_first=vstart;
      pptr=pptr->next;
      }
    sptr=sptr->next;
    }
  if(debug_flag==302)printf("\n   ... done.\n\n");


  // force tool operations sequence to just perimeters
  if(debug_flag==302)printf("\n   resetting line type sequence... \n");
  aptr=mptr->oper_list;
  while(aptr!=NULL)
    {
    if(strstr(aptr->name,"SUB_MILL")!=NULL || strstr(aptr->name,"MARK")!=NULL)
      {
      op_ptr=operation_find_by_name(slot,aptr->name);			// get pointer to next operation in model's list
      lt_new=genericlist_make();
      lt_new->ID=MDL_PERIM;
      sprintf(lt_new->name,"MDL_PERIM");
      lt_new->next=NULL;
      if(op_ptr!=NULL)op_ptr->lt_seq=lt_new;
      }
    aptr=aptr->next;
    }
  if(debug_flag==302)printf("\n   ... done.\n\n");
  
  
  if(debug_flag==302)printf("MPC:  Exit:  mptr=%X\n",mptr);
  return(TRUE);
}

// Function to generate tool paths for grayscale marking.  There are serveral ways to make this happen:
//   grayscale_mode<=0 means that grayscale is not active for this image
//   grayscale_mode==1 works by holding the z at optimum focus and passing the laser duty cycle value 
//                     thru each vtx->i variable.
//   grayscale_mode==2 works by holding the laser duty cycle to the line_type->flowrate found in the matl
//                     file and modulating the z up and down thus changing the focus and dwell over each pixel.
//   grayscale_mode==3 works by holding the laser duty cycle to the line_type->flowrate found in the matl
//                     file and adding a millisecond delay via the vtx->i variable thus dwelling different
//                     times over each pixel to achieve more or less burn.
// In all methods, the target image is mapped onto the base block that was either established by scanning
// or as a loaded model or built by default.  Like profile_cut, a polygon now corrisponds to a scan line across
// the image which is composed of a string of vtxs to form an open ended polygon (similar to a fill line).  
// The entire image is considered as one slice.
int model_grayscale_cut(model *mptr)
{

    int		h,i,slot,xpix,ypix,xend,yend;
    int 	vtx_ctr=0,ply_ctr=0,merge_vtx;
    int		N_shades;
    guchar 	*pix0;
    int 	x_step_size,y_step_size;
    float  	gray_scale_val;
    float 	z_threshold;
    float 	x0,y0,z0;
    double	amt_total=0,amt_done=0.0;
    vertex 	*vnew,*vold,*vfst;
    vertex 	*vptr,*vnxt;
    polygon 	*pnew;
    slice 	*sold,*sptr;
    linetype	*linetype_ptr;
    int		bi_directional;
    int		slc_copies;
    
    printf("model_grayscale_cut: entry\n");

    // validate input
    if(mptr==NULL)return(FALSE);					// if no model...
    if(mptr->g_img_act_buff==NULL)return(FALSE);			// if model does not have an active image...
    
    // set params
    mptr->grayscale_mode=2;						// z modulation
    bi_directional=FALSE;						// mark in both directions
    z_threshold=0.05;							// tolerance to consider the same z level
    x_step_size=1;							// every Nth pixel will be converted
    y_step_size=1;							// every Nth pixel will be converted
    slot=model_get_slot(mptr,OP_MARK_IMAGE);				// get tool id doing the work
    mptr->slice_thick=Tool[slot].matl.layer_height;			// make sure model slice thick matches tool
    linetype_ptr=linetype_find(slot,MDL_FILL);  			// get line type pointer

    
    // refresh the image control params so we know we have the right values
    mptr->act_n_ch = gdk_pixbuf_get_n_channels (mptr->g_img_act_buff);			// n channels - should always be 4
    mptr->act_cspace = gdk_pixbuf_get_colorspace (mptr->g_img_act_buff);		// color space - should be RGB
    mptr->act_alpha = gdk_pixbuf_get_has_alpha (mptr->g_img_act_buff);			// alpha - should be FALSE
    mptr->act_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (mptr->g_img_act_buff); 	// bits per pixel - should be 8
    mptr->act_width = gdk_pixbuf_get_width (mptr->g_img_act_buff);			// number of columns (image size)
    mptr->act_height = gdk_pixbuf_get_height (mptr->g_img_act_buff);			// number of rows (image size)
    mptr->act_rowstride = gdk_pixbuf_get_rowstride (mptr->g_img_act_buff);		// get pixels per row... may change tho
    mptr->act_pixels = gdk_pixbuf_get_pixels (mptr->g_img_act_buff);			// get pointer to pixel data

    // calc end conditions
    xend=(int)((float)(mptr->act_width)/(float)(x_step_size))*x_step_size-x_step_size;
    yend=(int)((float)(mptr->act_height)/(float)(y_step_size))*y_step_size-y_step_size;
    
    // convert image to grayscale with edge detect enhancement
    //image_processor(mptr->g_img_act_buff,IMG_EDGE_DETECT_ADD);
  
    // re-load image with edge detected objects
    //sprintf(mptr->model_file,"/home/aa/Documents/4X3D/tb_diff.jpg");
    //image_loader(mptr,mptr->act_width,mptr->act_height,TRUE);
    
    // display progress bar
    sprintf(scratch,"  Slice Image ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Processing image data...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_set_visible(win_info,TRUE);
    gtk_widget_queue_draw(win_info);
    while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
    

    // make a few slices from image data
    // this allows the user to re-burn the slice if it isn't dark enough
    slc_copies=4;
    sold=NULL;
    for(i=1;i<slc_copies;i++)
      {
      // create the first slice and only slice
      sptr=slice_make();
      sptr->sz_level=i*mptr->slice_thick;
      sptr->xmin=mptr->xmin[MODEL];
      sptr->ymin=mptr->ymin[MODEL];
      sptr->xmax=mptr->xmax[MODEL];
      sptr->ymax=mptr->ymax[MODEL];
      sptr->msrc=mptr;
  
      amt_done = (double)(i)/(double)(slc_copies);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      // loop thru each pixel and apply conversion
      // each y row equates to a polygon
      ypix=1; 
      while(ypix<yend)
	{
	y0=ypix;								// def float values
	vfst=NULL;							// init head node addr
	vold=NULL;							// no existing vtx at start
	vtx_ctr=0;							// init count of vtxs generated in this row
	
	// sweep from left (low x) to right (high x) for this row
	for(xpix=1; xpix<xend; xpix+=x_step_size)
	  {
	  x0=xpix;							// def float values
  
	  // get pixel data and convert to grayscale
	  pix0 = mptr->act_pixels + ypix * mptr->act_rowstride + xpix * mptr->act_n_ch;
	  gray_scale_val=floor((pix0[0]+pix0[1]+pix0[2])/3.0+0.5);	// convert to grayscale value bt 0 - 255
	  gray_scale_val /= 255.0;					// higher the value, whiter it gets		
  
	  // create a new vertex and add to list
	  vnew=vertex_make();						// create a new vertex
	  vtx_ctr++;
  
	  // map pixel data to vertex on model
	  vnew->attr=MDL_FILL;
	  vnew->x=(x0+2)*mptr->pix_size;				// shift to right
	  vnew->y=y0*mptr->pix_size;
	  if(mptr->grayscale_mode==1)					// if using PWM method...
	    {
	    vnew->z=mptr->slice_thick;
	    vnew->i=1.0-gray_scale_val;	
	    }
	  else 								// if using Z focus method...
	    {
	    vnew->z=mptr->grayscale_zdelta*gray_scale_val;		// gray_scale_val == 0.0 should be perfect focus
	    vnew->i=0.0;
	    }
	  
	  if(vfst==NULL)vfst=vnew;					// save addr of head node
	  if(vold!=NULL)vold->next=vnew;					// add to linked list
	  vold=vnew;
	  }	// end of xpix loop
	  
	// merge neighboring vectors with same grayscale value
	h=0;
	vptr=vfst;
	while(vptr!=NULL)
	  {
	  vnxt=vptr->next;
	  if(vnxt==NULL)break;
	  merge_vtx=FALSE;
	  if(mptr->grayscale_mode==1 && fabs(vptr->i - vnxt->i)<0.05)merge_vtx=TRUE;
	  if(mptr->grayscale_mode==2 && fabs(vptr->z - vnxt->z)<z_threshold)merge_vtx=TRUE;
	  if(merge_vtx==TRUE)
	    {
	    vptr->next=vnxt->next;
	    free(vnxt); vertex_mem--;
	    merge_vtx=FALSE;
	    }
	  else 
	    {
	    vptr=vptr->next;
	    }
	  }
	  
	pnew=polygon_make(vfst,MDL_FILL,0);				// create a new polygon
	pnew->vert_qty=vtx_ctr;
	polygon_insert(sptr,MDL_FILL,pnew);
	
	ypix += y_step_size;						// increment to next row of pixels
	y0=ypix;								// def float value
	
    
	if(bi_directional==TRUE)
	  {
	  vfst=NULL;							// init head node addr
	  vold=NULL;							// no existing vtx at start
	  vtx_ctr=0;
    
	  // sweep from right (high x) to left (low x)
	  for(xpix=xend-1; xpix>0; xpix-=x_step_size)
	    {
	    x0=xpix;							// def float values
    
	    // get pixel data and convert to grayscale
	    pix0 = mptr->act_pixels + ypix * mptr->act_rowstride + xpix * mptr->act_n_ch;
	    gray_scale_val=floor((pix0[0]+pix0[1]+pix0[2])/3.0+0.5);	// convert to grayscale value bt 0 - 255
	    gray_scale_val /= 255.0;			
    
	    // create a new vertex and add to list
	    vnew=vertex_make();						// create a new vertex
	    vtx_ctr++;
    
	    // map pixel data to vertex on model
	    vnew->attr=MDL_FILL;
	    vnew->x=x0*mptr->pix_size;
	    vnew->y=y0*mptr->pix_size;
	    if(mptr->grayscale_mode==1)						// if using PWM method...
	      {
	      vnew->z=mptr->slice_thick;
	      vnew->i=1.0-gray_scale_val;	
	      }
	    else 								// if using Z focus method...
	      {
	      vnew->z=mptr->grayscale_zdelta*gray_scale_val;
	      vnew->i=0.0;
	      }
	    
	    if(vfst==NULL)vfst=vnew;					// save addr of head node
	    if(vold!=NULL)vold->next=vnew;					// add to linked list
	    vold=vnew;
	    }	// end of xpix loop
	    
	  // merge neighboring vectors with same grayscale value
	  h=0;
	  vptr=vfst;
	  while(vptr!=NULL)
	    {
	    vnxt=vptr->next;
	    if(vnxt==NULL)break;
	    merge_vtx=FALSE;
	    if(mptr->grayscale_mode==1 && fabs(vptr->i - vnxt->i)<0.05)merge_vtx=TRUE;
	    if(mptr->grayscale_mode==2 && fabs(vptr->z - vnxt->z)<z_threshold)merge_vtx=TRUE;
	    if(merge_vtx==TRUE)
	      {
	      vptr->next=vnxt->next;
	      free(vnxt); vertex_mem--;
	      merge_vtx=FALSE;
	      }
	    else 
	      {
	      vptr=vptr->next;
	      }
	    }
	    
	  pnew=polygon_make(vfst,MDL_FILL,0);				// create a new polygon
	  pnew->vert_qty=vtx_ctr;
	  polygon_insert(sptr,MDL_FILL,pnew);
	  
	  ypix+=y_step_size;
	  y0=ypix;
	  }
	
	}		// end of ypix loop
  
  
    // sort for efficiency
    //polygon_sort(sptr,1);							// sort tip-to-tail every other pass
    
    if(mptr->slice_first[slot]==NULL)mptr->slice_first[slot]=sptr;
    if(sold!=NULL)sold->next=sptr;
    sptr->prev=sold;
    sptr->next=NULL;
    sold=sptr;
    }
  
  mptr->slice_last[slot]=sold;
  mptr->slice_qty[slot]=slc_copies;
  mptr->geom_type=MODEL;
  
  // set z bounds
  mptr->zmin[MODEL]=0.0;
  mptr->zmax[MODEL]=slc_copies*mptr->slice_thick;
  mptr->zoff[MODEL]=mptr->zmax[TARGET]; 
  mptr->zorg[MODEL]=mptr->zoff[MODEL];
  
  job.min_slice_thk=Tool[slot].matl.layer_height;
  set_start_at_crt_z=TRUE;
  
  gtk_widget_set_visible(win_info,FALSE);
  while(g_main_context_iteration(NULL, FALSE));				// update display so it goes away promptly
  
  printf("model_grayscale_cut: exit\n");

  return(1);
}


// Function to generate minimal internal build fill structure for a model.  This function is very similar
// to the job_build_support function with the exception that it is applied to the inside of a single model
// with the idea that it only puts a wide xy grid support where needed vs thruoughout the entire interior
// (which is what normal line_fill does).
//
// Basic order of events:
// 1. Identify model facets that will require internal support and turn them into upper patches.
// 2. Identify upward facing model facets that fall directly under the upper patches.  Store these
//    facets in child patches of the upper patch.
// 3. Step thru each upper support patch in a pre-defined XY grid.  Use the Z at each grid point to
//    create a vector that projects down to the build table.  Find the heighest facet it intersects with
//    and use that for the bottom Z that exists within each child patch.
// 4. Build the vertical facet walls by connecting the upper and lower meshes.
//
int model_build_internal_support(model *mptr)
{
  int		h,i,j,k,slot,ptyp,mtyp=MODEL;
  int		vtst,vint_resA,vint_resB,vint_resC;
  int		patch_cnt,mfacet_dn_cnt,mfacet_up_cnt;
  int		add_spt,vint_count;
  int	 	ixcnt,iycnt,grid_xcnt,grid_ycnt;
  int		vtx_in[4];
  float 	ix,iy,grid_xinc,grid_yinc;
  float 	max_angle,vz0,vz1,vz2;
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
  
  polygon 	*polyedge,*pptr;
  patch		*patptr,*pchptr,*patdel,*patnxt,*pat_last,*patold,*pchold,*pchnew,*pupptr,*patspt,*pchspt;
  vertex_list	*vl_ptr,*vl_list;
  facet_list	*pfptr,*pfnxt,*pfset,*ptest;
  slice 	*sptr;
  
  printf("\nModel Build Internal Support:  entry\n");

  // validate inputs
  if(mptr==NULL)return(FALSE);
  if(mptr->vase_mode==FALSE)return(FALSE);
  if(mptr->internal_spt==FALSE)return(FALSE);
  
  // initialize
  {
    // clear existing model and internal patches if they exist
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
  
    // clear existing internal facets if they exist
    if(mptr->facet_first[INTERNAL]!=NULL)
      {
      facet_purge(mptr->facet_first[INTERNAL]);
      mptr->facet_first[INTERNAL]=NULL;  mptr->facet_last[INTERNAL]=NULL; mptr->facet_qty[INTERNAL]=0;
      vertex_purge(mptr->vertex_first[INTERNAL]);
      mptr->vertex_first[INTERNAL]=NULL; mptr->vertex_last[INTERNAL]=NULL; mptr->vertex_qty[INTERNAL]=0;
      }
      
    // set all model facet and vertex attributes to 0.  this is needed because we'll be using the attr value as a
    // status check in the subsequent operation.
    mfptr=mptr->facet_first[MODEL];
    while(mfptr!=NULL)
      {
      mfptr->status=0;
      mfptr->attr=0;
      for(j=0;j<3;j++){(mfptr->vtx[j])->attr=0; mfptr->vtx[j]->supp=0;}
      mfptr=mfptr->next;
      }

    amt_total = mptr->facet_qty[MODEL];					// accrue total number of facets to examine
    
    // purge the upward facing facet list
    job.lfacet_upfacing[MODEL]=facet_list_manager(job.lfacet_upfacing[MODEL],NULL,ACTION_CLEAR);
    job.lfacet_upfacing[MODEL]=NULL;
      
    // show the progress bar
    sprintf(scratch,"  Generating Internal Structure...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Identifying areas that need structure  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);
    
    printf("  initialization complete.\n");
  }
  
  
  // step 1 - identify patches (contiguous groups) of model facets that will need structure.  these will
  // become the upper internal patches.  do this by scanning thru the model facet list, finding
  // a facet that is internally pointing up, creating a new patch for it, then traversing thru its neighoring
  // facet ptrs to add to the patch.  once all neighbors that meet the upward criterion are found,
  // resume scanning thru the model facet list to find a new "seed" facet for the next patch.
  //
  // at the same time create a list of only downward facing model facets.  these would be the facets
  // that would stop internal structure (i.e. the structure would "land" on).  this shortened list speeds things up later.
  //
  // also search the model facet list for local minimum vtxs.  this would be a vtx that has ALL the facets around it
  // face upward but are not so shallow as to be identified as being "upward enough" AND has all the vtxs
  // around it higher, or equal to it, in z.  in other words, it's a local minium point.  build upper
  // support patches from those cases as well with the same logic.
  {
    amt_done=0; amt_current=0;
    patch_cnt=0; mfacet_dn_cnt=0; mfacet_up_cnt=0;

    // get tool (slot) and allowable overhang angle
    slot=(-1);								// default no tool selected
    max_angle=cos(150*PI/180);						// default if no tool selected yet
    slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);			// determine which tool is adding build
    if(slot>=0 && slot<MAX_TOOLS)					// if a viable tool is specified...
      {
      max_angle=cos((180-Tool[slot].matl.overhang_angle)*PI/180);	// ... puts in terms of normal vector value (note, this is a negative value)
      mptr->slice_thick=Tool[slot].matl.layer_height;			// ... reset to keep in sync
      }
      
    // loop thru all facets of this model and determine which need structure, or which could have
    // structure land on them.
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
    
      h++;
    
      // skip any model facets that have already been added to a patch
      // status meaning:  0=not yet checked, 1=checked and not added to patch, 2=checked and added to patch
      if(mfptr->status>0){mfptr=mfptr->next; continue;}
      
      // skip all but upward facing facets.  tolerance eliminates near vertical facets.
      if((mfptr->unit_norm)->z < (0-TOLERANCE))
	{
	// only add facet to upward facing list if not vertical AND has other facets above it
	//if((mfptr->unit_norm)->z >= (0-TOLERANCE))
	  {
	  mfacet_up_cnt++;
	  job.lfacet_upfacing[INTERNAL]=facet_list_manager(job.lfacet_upfacing[INTERNAL],mfptr,ACTION_ADD);
	  }
	mfptr->status=1;
	mfptr=mfptr->next; 
	continue;
	}
  
      // skip facets that are too close to the build table.  we cannot add support to anything
      // thinner than a slice thickness.
      vz0=mptr->zoff[mtyp]+mfptr->vtx[0]->z-mptr->slice_thick;
      vz1=mptr->zoff[mtyp]+mfptr->vtx[1]->z-mptr->slice_thick;
      vz2=mptr->zoff[mtyp]+mfptr->vtx[2]->z-mptr->slice_thick;
      if((vz0+vz1+vz2)<0){mfptr=mfptr->next; mfptr->status=1; continue;}	// if the facet avg z is too thin... skip it
    
      // include internal downward facing facets that are more shallow than max angle which is defined by the material
      // properties of the support material.  this needs to happen before checking for a minimum vtx otherwise
      // we'll get redundant structures.
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
	//printf("new patch created %d \n",patch_cnt);
	
	// add the seed facet to the new patch.  note that patch_add_facet only creates a
	// pointer to the model facet.
	mfptr->status=2;						// set status of facet to "added to patch"
	mfptr->member=patch_cnt;
	patptr->pfacet=facet_list_manager(patptr->pfacet,mfptr,ACTION_ADD);
	//patptr->area = mfptr->area;
	//printf("   seed facet added\n");
  
	// loop thru list of facets already added to the patch and check the three neighbors 
	// for any that have not yet been checked.  if not checked, see if they should be added.
	test_fptr=mfptr;						// set to seed facet at entry
	while(test_fptr!=NULL)						// loop until no other facets qualify
	  {
	  // test first neighbor
	  fptr=test_fptr->fct[0];					// set facet to first neighbor of seed
	  if(fptr!=NULL)						// if a facet exists ...
	    {
	    if(fptr->status==0)						// ... if not yet checked
	      {
	      fptr->status=1;						// ... set to checked
	      //if((fptr->unit_norm)->z < (0-TOLERANCE))			// ... if facing downward
		{
		vz0=mptr->zoff[mtyp]+fptr->vtx[0]->z-mptr->slice_thick;	
		vz1=mptr->zoff[mtyp]+fptr->vtx[1]->z-mptr->slice_thick;
		vz2=mptr->zoff[mtyp]+fptr->vtx[2]->z-mptr->slice_thick;
		if((vz0+vz1+vz2)>0)					// ... if z is off of the build table
		  {
		  if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))  // ... if steeper than material allows
		    {
		    fptr->member=patptr->ID;				// ... identify which patch it went to
		    fptr->status=2;					// ... set status to "added to patch"
		    patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD); // ... add it to the patch's facet list
		    //patptr->area += fptr->area;			// ... accrue the patch area
		    //printf("   n0 facet added\n");
		    }
		  }
		}
	      }
	    }
	  // test second neighbor
	  fptr=test_fptr->fct[1];
	  if(fptr!=NULL)
	    {
	    if(fptr->status==0)						// ... if not yet checked
	      {
	      fptr->status=1;						// ... set to checked
	      //if((fptr->unit_norm)->z < (0-TOLERANCE))			// ... if facing downward
		{
		vz0=mptr->zoff[mtyp]+fptr->vtx[0]->z-mptr->slice_thick;	
		vz1=mptr->zoff[mtyp]+fptr->vtx[1]->z-mptr->slice_thick;
		vz2=mptr->zoff[mtyp]+fptr->vtx[2]->z-mptr->slice_thick;
		if((vz0+vz1+vz2)>0)					// ... if off of the build table
		  {
		  if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
		    {
		    fptr->member=patptr->ID;
		    fptr->status=2;
		    patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD);
		    //patptr->area += fptr->area;
		    //printf("   n1 facet added\n");
		    }
		  }
		}
	      }
	    }
	  // test third neighbor
	  fptr=test_fptr->fct[2];
	  if(fptr!=NULL)
	    {
	    if(fptr->status==0)						// ... if not yet checked
	      {
	      fptr->status=1;						// ... set to checked
	      //if((fptr->unit_norm)->z < (0-TOLERANCE))			// ... if facing downward
		{
		vz0=mptr->zoff[mtyp]+fptr->vtx[0]->z-mptr->slice_thick;	
		vz1=mptr->zoff[mtyp]+fptr->vtx[1]->z-mptr->slice_thick;
		vz2=mptr->zoff[mtyp]+fptr->vtx[2]->z-mptr->slice_thick;
		if((vz0+vz1+vz2)>0)					// ... if off of the build table
		  {
		  if((fptr->unit_norm->z*fptr->unit_norm->z)>fabs(max_angle))
		    {
		    fptr->member=patptr->ID;
		    fptr->status=2;
		    patptr->pfacet=facet_list_manager(patptr->pfacet,fptr,ACTION_ADD);
		    //patptr->area += fptr->area;
		    //printf("   n2 facet added\n");
		    }
		  }
		}
	      }
	    }
	  // find a facet in this patch that has a neighbor that has not been checked.  note this will
	  // eventually check all facets within the bounds of our conditions (i.e. downward, above zero,
	  // and material angle) along with the ring of facets (neighbors) around the patch that are
	  // the ones that fail the conditions.
	  test_fptr=NULL;						// undefine the seed facet
	  pfptr=patptr->pfacet;						// starting with first facet in this patch
	  while(pfptr!=NULL)						// loop thru all facets in this patch
	    {
	    fptr=pfptr->f_item;						// get address of actual facet
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

    // this section now finds the perimeter vertex list for each patch.  that is, it determines the list of vtxs
    // that form the free edge(s) around and inside (holes) for each patch and create a 3D polygon(s) out of them.
    // this same function also establishes the minz and maxz of the patch.  that is, the highest and lowest vtxs in it.
    patptr=mptr->patch_model;
    while(patptr!=NULL)
      {
      // find out which facets do not have neighbors
      patch_find_free_edge(patptr);
      patptr=patptr->next;
      }
      
    printf("  upper patches and upward facing facets complete.\n");
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
    sprintf(scratch,"  Generating Internal Structure...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Analyzing model intersection areas  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    amt_done=0;
    amt_current=0;
    amt_total=mfacet_up_cnt;
    
    vtxint=vertex_make();
    vtxD=vertex_make();							// scratch vtx used to hold facet centroid
    pfptr=job.lfacet_upfacing[INTERNAL];				// start with first upward facing model facet
    while(pfptr!=NULL)							// loop thru all upward facing model facets
      {
      // update status to user
      amt_current++;
      amt_done = (double)(amt_current)/(double)(amt_total);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      mfptr=pfptr->f_item;						// get address of actual facet
      
      // find lowest point of this facet
      minz=mfptr->vtx[0]->z;						// check first vtx				
      if(mfptr->vtx[1]->z<minz)minz=mfptr->vtx[1]->z;			// check second vtx
      if(mfptr->vtx[2]->z<minz)minz=mfptr->vtx[2]->z;			// check third vtx

      // load test vtx with facet centroid location
      facet_centroid(mfptr,vtxD);
	
      //minz += mptr->slice_thick;					// can't support anything thinner than a slice
      
      // loop thru upper patches and determine if this facet is within its free edge polygon.  if so,
      // add this facet to its child patch.  note that model facets may belong to multiple child patches.
      patold=NULL;
      patptr=mptr->patch_model;						// start with first model upper patch
      while(patptr!=NULL)						// loop thru all model upper patches
	{
	if(minz > patptr->maxz){patptr=patptr->next; continue;}		// skip if this facet is above this patch
	polyedge=patptr->free_edge;
	
	// determine if this facet has anything inside the free edge polygon.  there are three
	// tests that need to be done to determine this most efficiently.
	
	// first test if any facet vtxs or the facet centroid fall within the polyedge.
	vtst=polygon_contains_point(polyedge,vtxD);			// check if facet centroid inside patch
	if(vtst==TRUE)							// if centroid is in patch...
	  {
	  vtxint->x=vtxD->x; vtxint->y=vtxD->y; vtxint->z=0.0;		// ... set xy test location to centroid
	  }
	else 								// if centroid not in patch...
	  {
	  for(i=0;i<3;i++)						// ... check each corner vtx
	    {
	    vtst=polygon_contains_point(polyedge,mfptr->vtx[i]);
	    if(vtst==TRUE)						// if this vtx is inside the patch...
	      {
	      vtxint->x=mfptr->vtx[i]->x; 				// ... set xy test location to vtxs that is in
	      vtxint->y=mfptr->vtx[i]->y; 
	      vtxint->z=0.0;
	      break;
	      }
	    }
	  }
	  
	// second test the reverse.  it is possible that this a huge facet bigger in area than
	// the polyedge itself.  so test if any polyedge vtxs fall with the facet.
	if(vtst==FALSE)
	  {
	  vptr=polyedge->vert_first;
	  while(vptr!=NULL)
	    {
	    vtxint->x=vptr->x; vtxint->y=vptr->y; vtxint->z=vptr->z;
	    vtst=facet_contains_point(mfptr,vtxint);
	    if(vtst==TRUE)break;					// vtxint xy already set to xy of polyedge vtx that is inside facet
	    vptr=vptr->next;
	    if(vptr==polyedge->vert_first)break;
	    }
	  }
	  
	// third and lastly, test if any of the polyedge edges cross any of the facet edges.
	if(vtst==FALSE)
	  {
	  // make three vectors out of the facet edges
	  vec[0]=vector_make(mfptr->vtx[0],mfptr->vtx[1],0);
	  vec[1]=vector_make(mfptr->vtx[1],mfptr->vtx[2],0);
	  vec[2]=vector_make(mfptr->vtx[2],mfptr->vtx[0],0);

	  // loop thru polyedges and determine if any cross any of the three facet vectors
	  vptr=polyedge->vert_first;
	  while(vptr!=NULL)
	    {
	    vecnew=vector_make(vptr,vptr->next,0);
	    for(i=0;i<3;i++){vtx_in[i]=vector_intersect(vecnew,vec[i],vtxint);}
	    free(vecnew); vector_mem--; vecnew=NULL;
	    if(vtx_in[0]>=204 || vtx_in[1]>=204 || vtx_in[2]>=204)
	      {
	      vtst=TRUE; 
	      break;
	      }
	    vptr=vptr->next;
	    if(vptr==polyedge->vert_first)break;
	    }
	  
	  // clean up memory
	  for(i=0;i<3;i++){free(vec[i]); vector_mem--; vec[i]=NULL;}
	  }
	
	// if any of this model facet falls within the patch bounds... 
	if(vtst==TRUE)
	  {
	  patch_find_z(patptr,vtxint);					// get z height at this xy of the upper patch
	  // test if this facet is below the upper patch ... if so, include it in the child patch
	  //if(vtxD->z < vtxint->z)
	  if(minz < vtxint->z)
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

      pfptr=pfptr->next;						// move onto next upward facing model facet
      }
      
    // clean up
    free(vtxD); vertex_mem--; vtxD=NULL;
    free(vtxint); vertex_mem--; vtxint=NULL;

    printf("  lower model patches identified.\n");
  }
  
  // step 3 - superimpose xy grid over each patch to generate upper support facet mesh.  at each grid
  // point, build a vertical vector down to the build table and look for the highest upward facing facet
  // intersection.  use that z value to build the lower facet mesh.  Also create upper and lower support
  // patches while doing this step.
  {
    // initialize
    sprintf(scratch,"  Generating Internal Structure...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Building structure facets  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    amt_done=0;
    amt_current=0;
    amt_total=patch_cnt;
    vtxint=vertex_make();
    
    slot=(-1);								// default no tool selected
    slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);			// determine which tool is adding support
    if(slot<0 || slot>=MAX_TOOLS)return(0);				// if this model does not require support... skip it.

    // loop thru each patch and apply grid
    patold=NULL;
    patptr=mptr->patch_model;						// start with first patch
    while(patptr!=NULL)							// loop thru all found in model
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
      while(vnew!=NULL)							// loop thru all vtxs in free edge list
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
      
      // calculate xy offsets and count qty based on patch size and grid increment sizes
      grid_xinc=(maxx-minx)/10;						// set x grid based on size
      if(grid_xinc<1.0)grid_xinc=1.0;					// lower threshold
      if(grid_xinc>5.0)grid_xinc=5.0;					// upper threshold
       
      grid_yinc=(maxy-miny)/10;						// set y grid based on size
      if(grid_yinc<1.0)grid_yinc=1.0;
      if(grid_yinc>5.0)grid_yinc=5.0;
      
      //if(patptr->area < 1.0){grid_xinc=0.1; grid_yinc=0.1;}		// if a thin narrow, but diagonal area... use tight grid
      
      grid_xcnt=floor((maxx-minx)/grid_xinc)-1;				// calc count of steps in x less one
      grid_ycnt=floor((maxy-miny)/grid_yinc)-1;				// calc count of steps in y less one
      minx += ((maxx-minx)-(grid_xinc*grid_xcnt))/2;			// adjust xmin to center under patch in x
      miny += ((maxy-miny)-(grid_yinc*grid_ycnt))/2;			// adjust ymin to center under patch in y
      
      // sweep across x from start to stop
      for(ixcnt=0;ixcnt<grid_xcnt;ixcnt++)
	{
	ix=minx+ixcnt*grid_xinc;
	  
	// sweep across y from start to stop
	for(iycnt=0;iycnt<grid_ycnt;iycnt++)
	  {
	  iy=miny+iycnt*grid_yinc;
	  
	  // create 4 test vtxs and check if all are within patch and not inside holes of that patch
	  vtxnew[0]=vertex_make(); vtxnew[0]->x=ix;           vtxnew[0]->y=iy;
	  vtxnew[1]=vertex_make(); vtxnew[1]->x=ix+grid_xinc; vtxnew[1]->y=iy;
	  vtxnew[2]=vertex_make(); vtxnew[2]->x=ix;           vtxnew[2]->y=iy+grid_yinc;
	  vtxnew[3]=vertex_make(); vtxnew[3]->x=ix+grid_xinc; vtxnew[3]->y=iy+grid_yinc;
	  for(i=0;i<4;i++)
	    {
	    vtx_in[i]=polygon_contains_point(polyedge,vtxnew[i]);
	    pptr=polyedge->next;					// starting with the first hole (if there)
	    while(pptr!=NULL)						// loop thru all holes in this polyedge
	      {
	      if(pptr->hole>0)						// if this polygon is a hole of any type
		{
		if(polygon_contains_point(pptr,vtxnew[i])==TRUE)vtx_in[i]=FALSE; // if the grid pt falls inside the hole, reverse "in" status
		}
	      pptr=pptr->next;						// move onto next hole in this polygedge
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
		vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[INTERNAL],vtxnew[i],INTERNAL,CLOSE_ENOUGH);
		if(vtx_ptr!=vtxnew[i]){free(vtxnew[i]); vertex_mem--; vtxnew[i]=vtx_ptr;}
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
	    facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);
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
	      facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);
	      patspt->pfacet=facet_list_manager(patspt->pfacet,fnew,ACTION_ADD);
	      }
	      
	    
	    // build the LOWER support surface for this patch.
	    // make 4 more vtxs that will be the lower compliment to the upper four and add to support structure.
	    // add them with z=0 so that they default to being on the build table.
	    for(i=0;i<4;i++)
	      {
	      vtxlow[i]=vertex_make();				// use make, not copy, to avoid carrying over flist
	      vtxlow[i]->x=vtxnew[i]->x;	      
	      vtxlow[i]->y=vtxnew[i]->y;
	      vtxlow[i]->z=0.0;
	      }
	      
	    // scan thru upward facing facets to see if any of the new facet vertical vectors intersect with them to
	    // establish their correct z.
	    
	    ptest=job.lfacet_upfacing[INTERNAL];
	    while(ptest!=NULL)
	      {
	      ftest=ptest->f_item;
	      minz=BUILD_TABLE_LEN_Z;
	      for(i=0;i<3;i++){if(ftest->vtx[i]->z < minz)minz=ftest->vtx[i]->z;} // get lowest z of facet
	      
	      for(i=0;i<4;i++)					// loop thru the four vtx we are checking
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
		  vtxlow[i]->i=vtxint->i;				// debug
		  }
		}
	      ptest=ptest->next;
	      }
	      
	    /*
	    pchptr=patptr->pchild;					// start with first child patch
	    while(pchptr!=NULL)					// loop thru all child patches
	      {
	      for(i=0;i<4;i++)					// loop thru the four vtx we are checking
		{
		if(vtx_in[i]==TRUE)
		  {
		  vtxint->x=vtxlow[i]->x;				// set the temp vtx to their xy values
		  vtxint->y=vtxlow[i]->y;
		  vtxint->z=vtxlow[i]->z;
		  if(patch_find_z(pchptr,vtxint)==TRUE)		// check if they fall within patch, gets z if it does
		    {
		    if(vtxint->z > vtxlow[i]->z)vtxlow[i]->z=vtxint->z; // if it is the highest intersection... set perm vtx to it
		    }
		  }
		}
	      pchptr=pchptr->next;
	      }
	    */
	      
	    
	    /*
	    // if there is a sharp transition in z between any of the grid vtxs, extrapolate between them to
	    // find the point of the transition.  just adjust the x or y of the vtx to match.  this keeps the
	    // support from cutting thru the outer edge of the model.
	    
	    if((vtxlow[0]->z-vtxlow[2]->z)>1.0)			// if 2 is lower than 0 ...
	      {
	      vtxlow[2]->z=vtxlow[0]->z;				// ... bring 2 up to z level of 0
	      }
	    if((vtxlow[2]->z-vtxlow[0]->z)>1.0)			// if 0 is lower than 2 ...
	      {
	      vtxlow[0]->z=vtxlow[2]->z;				// ... bring 0 up to z level of 2
	      }
	    if((vtxlow[1]->z-vtxlow[3]->z)>1.0)			// if 3 is lower than 1 ...
	      {
	      vtxlow[3]->z=vtxlow[1]->z;
	      }
	    if((vtxlow[3]->z-vtxlow[1]->z)>1.0)			// if 1 is lower than 3 ...
	      {
	      vtxlow[1]->z=vtxlow[3]->z;
	      }
	      
	    if((vtxlow[0]->z-vtxlow[1]->z)>1.0)			// if 1 is lower than 0 ...
	      {
	      vtxlow[1]->z=vtxlow[0]->z;
	      }
	    if((vtxlow[1]->z-vtxlow[0]->z)>1.0)			// if 0 is lower than 1 ...
	      {
	      vtxlow[0]->z=vtxlow[1]->z;
	      }
	    if((vtxlow[2]->z-vtxlow[3]->z)>1.0)			// if 3 is lower than 2 ...
	      {
	      vtxlow[3]->z=vtxlow[2]->z;
	      }
	    if((vtxlow[3]->z-vtxlow[2]->z)>1.0)			// if 2 is lower than 3 ...
	      {
	      vtxlow[2]->z=vtxlow[3]->z;
	      }
	    */
	    
	    // add them to support structure only after their new z is established or they will not
	    // have connectivity
	    for(i=0;i<4;i++)
	      {
	      if(vtx_in[i]==TRUE)
		{
		vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[INTERNAL],vtxlow[i],INTERNAL,CLOSE_ENOUGH);
		if(vtx_ptr!=vtxlow[i]){free(vtxlow[i]); vertex_mem--; vtxlow[i]=vtx_ptr;}
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
	    facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);
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
	      facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);
	      pchspt->pfacet=facet_list_manager(pchspt->pfacet,fnew,ACTION_ADD);
	      }
	    
	    // free any unused vtxs that did not become part of the support structure
	    for(i=0;i<4;i++)
	      {
	      if(vtx_in[i]==FALSE)
		{
		free(vtxnew[i]); vertex_mem--; vtxnew[i]=NULL;
		free(vtxlow[i]); vertex_mem--; vtxlow[i]=NULL;
		}
	      }
	      
	    }
	  else 
	    {
	    // delete all test vtxs since all not in patch
	    for(i=0;i<4;i++){free(vtxnew[i]); vertex_mem--; vtxnew[i]=NULL;}
	    }
	    
	  }	// end of iy increment loop
	}	// end of ix increment loop

      patptr=patptr->next;
      }
    
    // re-associate the entire support facet set 
    facet_find_all_neighbors(mptr->facet_first[INTERNAL]);
    
    // clean up
    free(vtxint); vertex_mem--; vtxint=NULL;
    
    printf("  upper and lower support facets generated.\n");

  }
 

  // step 4 - build vertical facets between upper and lower patches
  // use the upper support facet set as the guide to building the vertical walls.
  {
    sprintf(scratch,"Creating wall structure facets");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_widget_queue_draw(win_info);
    gtk_widget_set_visible(win_info,TRUE);
    amt_done=0;
    amt_current=0;
    vtxD=vertex_make();
  
    slot=(-1);								// default no tool selected
    slot=model_get_slot(mptr,OP_ADD_MODEL_MATERIAL);			// determine which tool is adding support
    if(slot<0 || slot>=MAX_TOOLS)return(0);				// if this model does not require support... skip it.
    
    amt_total=mptr->facet_qty[INTERNAL];

    patptr=mptr->patch_first_upr;					// start with first upper support patch
    while(patptr!=NULL)							// loop thru all upper support patches
      {
      pchptr=patptr->pchild;						// get lower support patch (child patch)
      if(pchptr==NULL){patptr=patptr->next;continue;}			// ensure there is a bottom patch to connect with
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
	for(i=0;i<3;i++)
	  {
	  //fngh=facet_find_vtx_neighbor(mptr->facet_first[INTERNAL],sfptr,sfptr->vtx[i],sfptr->vtx[h]);
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
	    Czmax=vtxyA->z;
	    Dzmax=vtxyB->z;
	    
	    // search facets in child patch for matching vtxs. if two vtx in the child patch share the
	    // same xy, use the vtx with the lowest z.
	    pfnxt=pchptr->pfacet;
	    while(pfnxt!=NULL)
	      {
	      fptr=pfnxt->f_item;
	      for(j=0;j<3;j++)
		{
		if(vertex_xy_compare(vtxyA,fptr->vtx[j],TOLERANCE)==TRUE)
		  {
		  //if(fptr->vtx[j]->z<vtxyA->z && fptr->vtx[j]->z>=Czmax){Czmax=fptr->vtx[j]->z;vtxyC=fptr->vtx[j];}
		  if(fptr->vtx[j]->z<vtxyA->z && fptr->vtx[j]->z<Czmax){Czmax=fptr->vtx[j]->z;vtxyC=fptr->vtx[j];}
		  }
		if(vertex_xy_compare(vtxyB,fptr->vtx[j],TOLERANCE)==TRUE)
		  {
		  //if(fptr->vtx[j]->z<vtxyB->z && fptr->vtx[j]->z>=Dzmax){Dzmax=fptr->vtx[j]->z;vtxyD=fptr->vtx[j];}
		  if(fptr->vtx[j]->z<vtxyB->z && fptr->vtx[j]->z<Dzmax){Dzmax=fptr->vtx[j]->z;vtxyD=fptr->vtx[j];}
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
	      facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);	// adds new facet to support facet list
	      
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
	      facet_insert(mptr,mptr->facet_last[INTERNAL],fnew,INTERNAL);	// adds new facet to support facet list
	      }
	    }		// end of if neighbor NULL
	  }		// end of for i=...
	 
	pfptr=pfptr->next;
	}
      patptr=patptr->next;
      }

    // re-associate the entire support facet set 
    facet_find_all_neighbors(mptr->facet_first[INTERNAL]);
    
    // clean up
    free(vtxD); vertex_mem--; vtxD=NULL;
    
    printf("  wall support facets made.\n");
  }

  // hide progress bar
  gtk_widget_set_visible(win_info,FALSE);
  printf("  %d  model facet qty = %d    internal facet qty =%d \n",mptr->model_ID,mptr->facet_qty[MODEL],mptr->facet_qty[INTERNAL]);
  printf("\nModel Build Internal Structure:  exit \n");
  return(1);
}

// Function to create a vertex list of neighbors to a vertex
vertex_list *vertex_neighbor_list(vertex *vinpt)
{
  int		j;
  vertex	*vptr;
  vertex_list 	*vlist,*vl_ptr;
  facet 	*fptr;
  facet_list 	*fl_ptr;


  vlist=NULL;
  if(vinpt==NULL)return(vlist);
  if(vinpt->flist==NULL)return(vlist);
  
  // loop thru input vertex's facet list and add all of their vtxs as
  // the input vtx's neighbors
  fl_ptr=vinpt->flist;
  while(fl_ptr!=NULL)
    {
    fptr=fl_ptr->f_item;
    if(fptr!=NULL)
      {
      for(j=0;j<3;j++)
	{
	vptr=fptr->vtx[j];
	if(vptr!=NULL && vptr!=vinpt)vlist=vertex_list_manager(vlist,vptr,ACTION_ADD);
	}
      }
    fl_ptr=fl_ptr->next;
    }
    
  return(vlist);
}

// Function to calculate the intersection pt between a 3D line segment and a 3D facet
// Inputs:  fptr=ptr to facet, vecptr=ptr to vector, vint=ptr to vertex that will hold intersection pt if found
// Return:  <(-1) = failed, 0 = does not intersect, 1 = does intersect
int vector_facet_intersect(facet *fptr, vector *vecptr, vertex *vint)
{
    float 	a, f, u, v, t;
    vertex 	*segdir, *ed1,*ed2;
    vertex 	*vtx_origin, *vtx_cp;
    vertex 	*sval,*qval;
    vector 	*vec_dir, *vec_edge1, *vec_edge2, *vec_cp;
    vector 	*svec,*qvec;

    //printf("vector_facet_intersection:  enter\n");
   
    if(fptr==NULL || vecptr==NULL || vint==NULL)return(FALSE);

    // Direction of the segment
    segdir=vertex_make();
    segdir->x = vecptr->tail->x - vecptr->tip->x;
    segdir->y = vecptr->tail->y - vecptr->tip->y;
    segdir->z = vecptr->tail->z - vecptr->tip->z;
  
    // Triangle edges
    ed1=vertex_make();
    ed1->x = fptr->vtx[1]->x - fptr->vtx[0]->x;
    ed1->y = fptr->vtx[1]->y - fptr->vtx[0]->y;
    ed1->z = fptr->vtx[1]->z - fptr->vtx[0]->z;
  
    ed2=vertex_make();
    ed2->x = fptr->vtx[2]->x - fptr->vtx[0]->x;
    ed2->y = fptr->vtx[2]->y - fptr->vtx[0]->y;
    ed2->z = fptr->vtx[2]->z - fptr->vtx[0]->z;
  
    // cp = dir × edge2
    vtx_origin=vertex_make();
    vtx_origin->x=0.000; vtx_origin->y=0.000; vtx_origin->z=0.000;
    vtx_cp=vertex_make();
    
    vec_dir=vector_make(segdir,vtx_origin,0);
    vec_edge2=vector_make(ed2,vtx_origin,0); 
    vector_crossproduct(vec_dir, vec_edge2, vtx_cp);
  
    // a = edge1 · cp
    vec_edge1=vector_make(ed1,vtx_origin,0);
    vec_cp=vector_make(vtx_cp,vtx_origin,0);
    a=vector_dotproduct(vec_edge1,vec_cp);
  
    if(fabs(a)<TOLERANCE)	  					// Segment is parallel to triangle
      {
      // clean up
      free(segdir); vertex_mem--;
      free(ed1); vertex_mem--;
      free(ed2); vertex_mem--;
      free(vtx_origin); vertex_mem--;
      free(vtx_cp); vertex_mem--;
      free(vec_dir); vector_mem--;
      free(vec_edge1); vector_mem--;
      free(vec_edge2); vector_mem--;
      return(FALSE);
      }
    

    f=1.0/a;
  
    // s = seg->tip - fptr->vtx[0]
    sval=vertex_make();
    sval->x = segdir->x - fptr->vtx[0]->x;
    sval->y = segdir->y - fptr->vtx[0]->y;
    sval->z = segdir->z - fptr->vtx[0]->z;
  
    // u = f * (s · h)
    svec=vector_make(sval,vtx_origin,0);
    u=f*vector_dotproduct(svec,vec_cp);
    if(u<0.0 || u>1.0)							// not on segment
      {
      // clean up
      free(sval); vertex_mem--;
      free(segdir); vertex_mem--;
      free(ed1); vertex_mem--;
      free(ed2); vertex_mem--;
      free(vtx_origin); vertex_mem--;
      free(vtx_cp); vertex_mem--;
      free(svec); vector_mem--;
      free(vec_dir); vector_mem--;
      free(vec_edge1); vector_mem--;
      free(vec_edge2); vector_mem--;
      return(FALSE);
      }
  
    // q = s × edge1
    qval=vertex_make();
    vector_crossproduct(svec,vec_edge1, qval);
  
    // v = f * (dir · q)
    qvec=vector_make(qval,vtx_origin,0);
    v=f*vector_dotproduct(vec_dir,qvec);
  
    if(v<0.0 || (u+v)>1.0)						// outside max/min box of facet
      {
      // clean up
      free(qval); vertex_mem--;
      free(sval); vertex_mem--;
      free(segdir); vertex_mem--;
      free(ed1); vertex_mem--;
      free(ed2); vertex_mem--;
      free(vtx_origin); vertex_mem--;
      free(vtx_cp); vertex_mem--;
      free(qvec); vector_mem--;
      free(svec); vector_mem--;
      free(vec_dir); vector_mem--;
      free(vec_edge1); vector_mem--;
      free(vec_edge2); vector_mem--;
      return(FALSE);
      }
  
    // t = f * (edge2 · q)
    t=f*vector_dotproduct(vec_edge2,qvec);
  
    if(t<0.0 || t>1.0)							// not within triangle side of facet
      {
      // clean up
      free(qval); vertex_mem--;
      free(sval); vertex_mem--;
      free(segdir); vertex_mem--;
      free(ed1); vertex_mem--;
      free(ed2); vertex_mem--;
      free(vtx_origin); vertex_mem--;
      free(vtx_cp); vertex_mem--;
      free(qvec); vector_mem--;
      free(svec); vector_mem--;
      free(vec_dir); vector_mem--;
      free(vec_edge1); vector_mem--;
      free(vec_edge2); vector_mem--;
      return(FALSE);
      }

    
    // clean up
    free(qval); vertex_mem--;
    free(sval); vertex_mem--;
    free(segdir); vertex_mem--;
    free(ed1); vertex_mem--;
    free(ed2); vertex_mem--;
    free(vtx_origin); vertex_mem--;
    free(vtx_cp); vertex_mem--;
    free(qvec); vector_mem--;
    free(svec); vector_mem--;
    free(vec_dir); vector_mem--;
    free(vec_edge1); vector_mem--;
    free(vec_edge2); vector_mem--;

   /*    
   // calculate the parameters for the plane
   d = - fptr->unit_norm->x * fptr->vtx[0]->x - fptr->unit_norm->y * fptr->vtx[0]->y - fptr->unit_norm->z * fptr->vtx[0]->z;

   // calculate the position on the line that intersects the plane
   denom = fptr->unit_norm->x * (vecptr->tail->x-vecptr->tip->x) + 
           fptr->unit_norm->y * (vecptr->tail->y-vecptr->tip->y) +
	   fptr->unit_norm->z * (vecptr->tail->z-vecptr->tip->z);
   
   // test if parallel
   if (fabs(denom) < TOLERANCE)
     {
     //printf("vector_facet_intersection:  exit FALSE parallel\n"); 
     return(FALSE);
     }
   
   // test if intersection lies beyond line segment
   mu = - (d + fptr->unit_norm->x * vecptr->tip->x + fptr->unit_norm->y * vecptr->tip->y + fptr->unit_norm->z * vecptr->tip->z) / denom;
   if (mu < 0 || mu > 1)
     {
     //printf("vector_facet_intersection:  exit FALSE not on vector\n"); 
     return(FALSE);
     }

   // calculate the vector - plane intersection point (still may not be on facet)
   vint->x = vecptr->tip->x + mu * (vecptr->tail->x - vecptr->tip->x);
   vint->y = vecptr->tip->y + mu * (vecptr->tail->y - vecptr->tip->y);
   vint->z = vecptr->tip->z + mu * (vecptr->tail->z - vecptr->tip->z);
   
   // test if intersection lies within the bounds of the facet
   vtx1=vertex_make();
   vec1=vector_make(fptr->vtx[0],vint,0);
   vector_unit_normal(vec1,vtx1);
   free(vec1); vector_mem--; vec1=NULL;
   vtx2=vertex_make();
   vec2=vector_make(fptr->vtx[1],vint,0);
   vector_unit_normal(vec2,vtx2);
   free(vec2); vector_mem--; vec2=NULL;
   vtx3=vertex_make();
   vec3=vector_make(fptr->vtx[2],vint,0);
   vector_unit_normal(vec3,vtx3);
   free(vec3); vector_mem--; vec3=NULL;

   a1=vertex_dotproduct(vtx1,vtx2);
   a2=vertex_dotproduct(vtx2,vtx3);
   a3=vertex_dotproduct(vtx3,vtx1);
   total = (acos(a1) + acos(a2) + acos(a3));
   
   // release memory
   free(vtx1); vertex_mem--; vtx1=NULL;
   free(vtx2); vertex_mem--; vtx2=NULL;
   free(vtx3); vertex_mem--; vtx3=NULL;
   
   // test if all angles don't add up to 2PI
   if (fabs(total) < (2*PI-5*CLOSE_ENOUGH))
     {
     //printf("vector_facet_intersection:  exit FALSE angles\n"); 
     return(FALSE);
     }
   
   // since we are now going to return a "legit" z value, ensure it is good
   minz=fptr->vtx[0]->z;
   if(fptr->vtx[1]->z>minz)minz=fptr->vtx[1]->z;
   if(fptr->vtx[2]->z>minz)minz=fptr->vtx[2]->z;
   maxz=fptr->vtx[0]->z;
   if(fptr->vtx[1]->z>maxz)maxz=fptr->vtx[1]->z;
   if(fptr->vtx[2]->z>maxz)maxz=fptr->vtx[2]->z;
   if(vint->z<minz)vint->z=minz;
   if(vint->z>maxz)vint->z=maxz;
   */
    
   //printf("vector_facet_intersection:  exit TRUE\n");
   return(TRUE);
}


// Function to create a new patch
patch *patch_make(void)
{
  patch	*patptr;
  
  patptr=(patch *)malloc(sizeof(patch));
  if(patptr!=NULL)
    {
    patptr->pfacet=NULL;
    patptr->pvertex=NULL;
    patptr->pchild=NULL;
    patptr->free_edge=NULL;
    patptr->area=0.0;
    patptr->maxx=0.0;
    patptr->minx=0.0;
    patptr->maxy=0.0;
    patptr->miny=0.0;
    patptr->maxz=0.0;
    patptr->minz=0.0;
    patptr->next=NULL;
    }
  return(patptr);
}

// Function to destroy a patch
int patch_delete(patch *patdel)
{
  facet_list	*pfdel;
  vertex_list	*pvdel;
  patch		*patptr,*patrmv;
  
  if(patdel==NULL)return(0);
  
  // delete any associated child patches first
  // note that we are only deleting the patch and the lists it uses to reference facets and vertexes, 
  // NOT the actual facets/vtxs that they point to.
  //printf("      deleting child patches... \n");
  patptr=patdel->pchild;						// start with first child patch
  while(patptr!=NULL)							// loop thru all of them
    {
    pfdel=patptr->pfacet;
    facet_list_manager(pfdel,NULL,ACTION_CLEAR);			// release patch facet list pointers
    pvdel=patptr->pvertex;
    vertex_list_manager(pvdel,NULL,ACTION_CLEAR);			// release boundary vertex list pointers
    
    // delete the free edge polygon
    if(patdel->free_edge!=NULL)polygon_free(patdel->free_edge);

    // delete the child patch itself
    patrmv=patptr;
    patptr=patptr->next;						// could be multiple children
    free(patrmv);
    }
   
  // delete the patch itself along with its facet/vtx lists
  //printf("      deleting target patch... \n");
  pfdel=patdel->pfacet;
  facet_list_manager(pfdel,NULL,ACTION_CLEAR);
  pvdel=patdel->pvertex;
  vertex_list_manager(pvdel,NULL,ACTION_CLEAR);

  free(patdel);
  //printf("      done.\n");
  
  return(1);
}

// Function to copy the facets and vtxs a patch points to and create a new patch pointing to the new stuff.
// This function will make a copy of the facet and vtx lists from the source patch.
// It will NOT copy the child patch.  Call this function again to that if necessary.
patch *patch_copy(model *mptr, patch *pat_src, int target_mdl_typ)
{
  int		h,i,k;
  vertex 	*vptr,*vtest;
  facet		*fptr,*fnew;
  patch		*pat_cpy;
  vertex_list	*vlist;
  facet_list	*pfptr;
  
  //printf("Patch Copy:  enter\n");
      
  // validate input
  if(pat_src==NULL)return(NULL);
  if(target_mdl_typ<=0 || target_mdl_typ>=MAX_MDL_TYPES)return(NULL);
 
  // create a new patch
  pat_cpy=patch_make();
  if(pat_cpy==NULL)return(NULL);
  pat_cpy->ID=pat_src->ID;
  pat_cpy->area=pat_src->area;

  // copy facets and vtxs which is tricky because we want to preserve their association in the new copy.
  // do this by looping thru the source patch's facet list.  copy that facet over then check each of its
  // vtxs if they have already been added to the copy's structure or not.  if they have, then their address
  // is used to tie this new facet to the others that already exist.
  pfptr=pat_src->pfacet;						// get first element in facet list of source patch
  while(pfptr!=NULL)							// loop thru entire facet list of source patch
    {
    fptr=pfptr->f_item;							// get address of actual facet
    
    // create a new support facet from the source facet
    fnew=facet_make();							// create the facet
    for(i=0;i<3;i++)							// create its three vertexes
      {
      fnew->vtx[i]=vertex_make();
      fnew->vtx[i]->x=fptr->vtx[i]->x; 					// copy over the same coordinates
      fnew->vtx[i]->y=fptr->vtx[i]->y; 
      fnew->vtx[i]->z=fptr->vtx[i]->z;
      }
    fnew->unit_norm->x=fptr->unit_norm->x;				// define normal values as same as source
    fnew->unit_norm->y=fptr->unit_norm->y;
    fnew->unit_norm->z=fptr->unit_norm->z;
    fnew->area=fptr->area;						// same coords, then same area
    fnew->member=pat_src->ID;						// define body ID
    
    // add new facet to target material facet list
    facet_insert(mptr,mptr->facet_last[target_mdl_typ],fnew,target_mdl_typ);	
      
    // add the new facet to the copy patch
    pat_cpy->pfacet=facet_list_manager(pat_cpy->pfacet,fnew,ACTION_ADD);	
    
    // insert each of the three facet vtxs into the copy's structure.  do this by searching what has
    // alread been inserted into the copy's vertex list and making sure each vtx is unqiue.  if it is
    // not unique, use the address of the one that already exists at that xyz position.
    for(i=0;i<3;i++)
      {
      if(pat_cpy->pvertex==NULL)					// if the list does not exist yet...
        {
	vptr=NULL; 							// ... define that no match was found
	pat_cpy->pvertex=vertex_list_manager(pat_cpy->pvertex,fnew->vtx[i],ACTION_ADD); // ... create list with first vtx
	vertex_insert(mptr,mptr->vertex_last[target_mdl_typ],fnew->vtx[i],target_mdl_typ);
	}
      else 								// otherwise the list does exist...
        {
	vptr=vertex_list_add_unique(pat_cpy->pvertex,fnew->vtx[i]);	// ... search list for matching xyz vtx
	}
      if(vptr!=NULL)							// if a match was found ...
	{
	free(fnew->vtx[i]); vertex_mem--; 				// ... free the one previously created for this facet
	fnew->vtx[i]=vptr;						// ... define the existing one as this facet's vtx
	}
      else 								// otherwise add the new vtx to the support vertex list
        {
	vertex_insert(mptr,mptr->vertex_last[target_mdl_typ],fnew->vtx[i],target_mdl_typ);
	}
      fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);	// add the new facet to the vtx facet list
      }
      
    pfptr=pfptr->next;							// move onto next facet in source patch facet list
    }
    
  //printf("Patch Copy:  exit  %X\n",pat_cpy);
  return(pat_cpy);
  }

// Function to flip the unit normal vectors of all facets listed in a patch
int patch_flip_normals(patch *patptr)
{
    facet	*fptr;
    facet_list	*pfptr;
    
    if(patptr==NULL)return(0);
    
    pfptr=patptr->pfacet;
    while(pfptr!=NULL)
      {
      fptr=pfptr->f_item;
      fptr->unit_norm->x*=(-1);
      fptr->unit_norm->y*=(-1);
      fptr->unit_norm->z*=(-1);
      pfptr=pfptr->next;
      }
      
    return(1);
}

// Function to find the z value of a patch given xy coordinates
// Return:  FALSE if vtx not in patch, TRUE if in patch AND vinpt->z will have z value.
int patch_find_z(patch *patptr, vertex *vinpt)
{
  float 	maxz;
  vertex 	*vint;
  facet		*fptr;
  facet_list	*fl_ptr;
  
  // validate inputs
  if(patptr==NULL || vinpt==NULL)return(FALSE);
  
  // loop thru all facets of the input patch.  find the facet that contains the input xy coord and get z.
  maxz=vinpt->z;
  vint=vertex_copy(vinpt,NULL);
  fl_ptr=patptr->pfacet;
  while(fl_ptr!=NULL)
    {
    fptr=fl_ptr->f_item;
    if(facet_contains_point(fptr,vint)==TRUE)
      {
      if(vint->z>maxz)maxz=vint->z;
      }
    fl_ptr=fl_ptr->next;
    }
  vinpt->z=maxz;
  
  free(vint); vertex_mem--;
  
  if(fl_ptr==NULL)return(FALSE);
  return(TRUE);
}

// Function to identify all facets along the edges of the patch
// This function will set the supp flag of the facet and its edge vtxs to indicate that they will need perimeter support.
int patch_find_edge_facets(patch *patptr)
{
  int		h,i,status=TRUE;
  int		f_main[3],f_nebr[3];
  int		ptr_unshrd=(-1),tst_unshrd=(-1);
  facet_list	*pfptr;
  facet		*fptr,*ftst;
  
  if(patptr==NULL)return(FALSE);
  
  // clear status of facets in the patch
  pfptr=patptr->pfacet;							// starting with first facet in list
  while(pfptr!=NULL)							// loop thru entire facet list of this patch
    {
    fptr=pfptr->f_item;							// get facet address
    fptr->supp=0;							// clear its status
    pfptr=pfptr->next;
    }
    
  // test and set status as appropriate
  pfptr=patptr->pfacet;							// starting with first facet in list
  while(pfptr!=NULL)							// loop thru entire facet list of this patch
    {
    fptr=pfptr->f_item;							// get facet address
    // test if any neighbor is outside of patch.  if so, this an edge facet
    // recall that facet neighbors always share an edge (i.e. a pair of vtxs)
    for(i=0;i<3;i++)
      {
      ftst=fptr->fct[i];
      if(patch_contains_facet(patptr,ftst)==FALSE)			// if this neighboring facet is NOT part of the patch...
        {
	// DEBUG
	//fptr->supp=10;
	//ftst->supp=11;
	  
	// determine which vtxs they do and do NOT share
	for(h=0;h<3;h++)							// loop thru 3 vtx of main facet
	  {
	  f_main[h]=0;								// set flag to NOT shared
	  if(facet_share_vertex(fptr,ftst,fptr->vtx[h])==TRUE){f_main[h]=1;}	// test if shared, if so set to shared
	  else {ptr_unshrd=h;}							// save index of vtx that is not shared
	  }
	for(h=0;h<3;h++)							// loop thru 3 vtx of neighboring facet
	  {
	  f_nebr[h]=0;								// set flag to NOT shared
	  if(facet_share_vertex(fptr,ftst,ftst->vtx[h])==TRUE){f_nebr[h]=1;}	// test if shared, if so set to shared
	  else {tst_unshrd=h;}
	  }
	// now evaluate z location of main versus neighbor using non-shared vtx z value.  if the neighbor folds
	// upward then this edge will require support.  if the neighbor folds downward, then the model will act
	// as support along this edge.
	if(ptr_unshrd<0 || ptr_unshrd>2 || tst_unshrd<0 || tst_unshrd>2)	// ensure correct index was found
	  {
	  printf("Warning - facet vertex index out of range while attempting to detect free edge! \n");
	  }
	else 									// if unshared vtx indexes look good ...
	  {
	  if(fptr->vtx[ptr_unshrd]->z < ftst->vtx[tst_unshrd]->z)		// if neighbor folds above patch, it will need support
	    {
	    //fptr->supp=1;							// set to facet will need perimeter support
	    for(h=0;h<3;h++){if(f_main[h]==1)fptr->vtx[h]->supp=1;}		// set the two vtxs forming the free edge to will need support
	    }
	  }
	}
      }
    
    pfptr=pfptr->next;
    }
  
  return(status);
}

// Function to test if a facet is a member of a patch
int patch_contains_facet(patch *patptr, facet *fptr)
{
  facet_list	*pfptr;
  
  // validate inputs
  if(patptr==NULL || fptr==NULL)return(FALSE);
  
  // search thru list
  pfptr=patptr->pfacet;
  while(pfptr!=NULL)
    {
    if(pfptr->f_item==fptr)break;
    pfptr=pfptr->next;
    }

  if(pfptr!=NULL)return(TRUE);
  return(FALSE);
}

// Function to test if a vertex is on the edge of a patch
int patch_vertex_on_edge(patch *patptr, vertex *vptr)
{
  int		status=FALSE;
  facet 	*fptr,*fnxt;
  facet_list	*vflptr,*pflptr;
  
  // validate input
  if(patptr==NULL || vptr==NULL)return(FALSE);
  
  // get vertex facet list, then check to see if any of those facets fall outside the patch
  vflptr=vptr->flist; 
  while(vflptr!=NULL)							// loop thru all facets connected to this vtx
    {
    fptr=vflptr->f_item;						// get address of facet connected to this vtx
    pflptr=patptr->pfacet;						// starting with first facet in this patch
    while(pflptr!=NULL)							// loop thru all facets in this patch
      {
      fnxt=pflptr->f_item;						// get address of facet
      if(fnxt==fptr)break;						// if patch facet matches vtx facet ... break, move onto next vtx facet
      pflptr=pflptr->next;						// move on to next patch facet
      }
    if(pflptr==NULL){status=TRUE;break;}				// if no match was found... then vtx on edge
    vflptr=vflptr->next;						// move on to next vtx facet
    }
  
  return(status);
}

// Function to define patch minz and maxz
int patch_find_z_extremes(patch *patptr)
{
  float 	newminz,newmaxz;
  facet 	*fptr;
  facet_list	*pfptr;
  

  newminz=BUILD_TABLE_LEN_Z; newmaxz=0;
  pfptr=patptr->pfacet;							// start with first facet in patch
  while(pfptr!=NULL)							// loop thru all facets in patch
    {
    fptr=pfptr->f_item;							// get address of actual facet
    if(fptr->vtx[0]==NULL || fptr->vtx[1]==NULL || fptr->vtx[2]==NULL){pfptr=pfptr->next;continue;}
    
    // set minz and maxz
    if(fptr->vtx[0]->z>newmaxz)newmaxz=fptr->vtx[0]->z;
    if(fptr->vtx[1]->z>newmaxz)newmaxz=fptr->vtx[1]->z;
    if(fptr->vtx[2]->z>newmaxz)newmaxz=fptr->vtx[2]->z;
    
    if(fptr->vtx[0]->z<newminz)newminz=fptr->vtx[0]->z;
    if(fptr->vtx[1]->z<newminz)newminz=fptr->vtx[1]->z;
    if(fptr->vtx[2]->z<newminz)newminz=fptr->vtx[2]->z;
    
    pfptr=pfptr->next;
    }
    
  patptr->minz=newminz;
  patptr->maxz=newmaxz;
  
  return(1);
}

// Function to generate a patch vertex list from the patch's current facet list.
// This function will generate a sequenced vertex list of the vtxs that outline each patch.
// Inputs: pat_inpt = pointer to input patch (i.e. contiguious collection of facets that
// may or may not have holes in it)
int patch_find_free_edge(patch *pat_inpt)
{
  int		h,i;
  int 		test[3];
  float 	max_area,area,max_len,length;
  vertex	*vptr,*vnew,*vold,*vtxA,*vtxB;
  vertex_list	*vtxlptr;
  vector 	*new_vec[3],*match_vec[3];
  vector 	*vecptr,*vec_first,*vec_last,*vec_next,*vec_prev;
  vector_list	*vlptr,*vlist;
  facet		*fptr,*nptr,*ftst;
  facet_list	*pfptr;
  polygon 	*pptr,*pold,*polymax;
  slice 	*sptr;
  patch		*patptr;
  
  if(pat_inpt==NULL)return(0);
  //printf("\nPatch find free edge - entry:  ID=%d  patptr=%X  \n",pat_inpt->ID,pat_inpt);

  // DEBUG - count and show what patches we're dealing with
  i=0;
  patptr=pat_inpt;
  while(patptr!=NULL)
    {
    //printf("%d patch=%X \n",i,patptr);
    pfptr=patptr->pfacet;
    while(pfptr!=NULL)
      {
      fptr=pfptr->f_item;
      //printf("  fptr=%X  v0=%X v1=%X v2=%X  n0=%X n1=%X n2=%X \n",fptr,fptr->vtx[0],fptr->vtx[1],fptr->vtx[2],fptr->fct[0],fptr->fct[1],fptr->fct[2]);
      pfptr=pfptr->next;
      }
    i++;
    patptr=patptr->next;
    }

  // dump all facet edges into one unsequenced vector list.  if a facet edge (vector) is identical to
  // one that already exists in the list, delete them both from the list.  what is left are the free
  // edges.  this works on the premiss that every two neighboring facets share exactly only one edge.

  patptr=pat_inpt;	

  // init min and max of patch
  patptr->minx=1000; patptr->maxx=(-1000);
  patptr->miny=1000; patptr->maxy=(-1000);
  patptr->minz=1000; patptr->maxz=(-1000);
  
  h=0;									// init vector counter
  vec_prev=NULL; vec_next=NULL; vec_first=NULL; vec_last=NULL;		// init ptrs into our new vector list
  pfptr=patptr->pfacet;							// start with first facet in patch
  while(pfptr!=NULL)							// loop thru all facets in patch
    {
    fptr=pfptr->f_item;							// get address of actual facet
    if(fptr->vtx[0]==NULL || fptr->vtx[1]==NULL || fptr->vtx[2]==NULL)	// if any data missing, skip this facet
      {pfptr=pfptr->next;continue;}
    
    // find min and max of patch
    for(i=0;i<3;i++)
      {
      if(fptr->vtx[i]->x > patptr->maxx)patptr->maxx = fptr->vtx[i]->x;
      if(fptr->vtx[i]->y > patptr->maxy)patptr->maxy = fptr->vtx[i]->y;
      if(fptr->vtx[i]->z > patptr->maxz)patptr->maxz = fptr->vtx[i]->z;
      
      if(fptr->vtx[i]->x < patptr->minx)patptr->minx = fptr->vtx[i]->x;
      if(fptr->vtx[i]->y < patptr->miny)patptr->miny = fptr->vtx[i]->y;
      if(fptr->vtx[i]->z < patptr->minz)patptr->minz = fptr->vtx[i]->z;
      }
    
    // create a copies of the facet vtxs.  down stream in this function vector_sort will manipulate vtx values
    // which we do not want to do with the original vtxs for facets.  So make copies.
    vtxA=vertex_make();					
    vtxA->x=fptr->vtx[0]->x; vtxA->y=fptr->vtx[0]->y; vtxA->z=fptr->vtx[0]->z;
    vtxB=vertex_make();							
    vtxB->x=fptr->vtx[1]->x; vtxB->y=fptr->vtx[1]->y; vtxB->z=fptr->vtx[1]->z;
    new_vec[0]=vector_make(vtxA,vtxB,0);				

    vtxA=vertex_make();							
    vtxA->x=fptr->vtx[1]->x; vtxA->y=fptr->vtx[1]->y; vtxA->z=fptr->vtx[1]->z;
    vtxB=vertex_make();							
    vtxB->x=fptr->vtx[2]->x; vtxB->y=fptr->vtx[2]->y; vtxB->z=fptr->vtx[2]->z;
    new_vec[1]=vector_make(vtxA,vtxB,0);				

    vtxA=vertex_make();							
    vtxA->x=fptr->vtx[2]->x; vtxA->y=fptr->vtx[2]->y; vtxA->z=fptr->vtx[2]->z;
    vtxB=vertex_make();							
    vtxB->x=fptr->vtx[0]->x; vtxB->y=fptr->vtx[0]->y; vtxB->z=fptr->vtx[0]->z;
    new_vec[2]=vector_make(vtxA,vtxB,0);				
    
    // loop thru the three new vectors we just created from this facet and see if a copy of them
    // (even if reversed direction) already exists on the list we are building.  If a copy does
    // exist, delete the copy found and do not add this vector.  If no copy found, add this vec
    // to the list.
    for(i=0;i<3;i++)
      {
      // search vector list for matches to any of the this facet's vectors
      match_vec[i]=NULL;						// init matching vector addresses
      vecptr=vec_first;							// start with first vector in list
      while(vecptr!=NULL)						// loop thru the entire vector list
	{
	// if match found for any of the new vectors, save address of match found in list
	if(vector_compare(vecptr,new_vec[i],TOLERANCE)==TRUE){match_vec[i]=vecptr;break;}
	vecptr=vecptr->next;
	}

      // if match was NOT found... add new vector to the list
      if(match_vec[i]==NULL)
	{
	new_vec[i]->tip->supp=fptr->supp;				// set status of if this edge will require perimeter support
	new_vec[i]->tail->supp=fptr->supp;
	if(vec_first==NULL)vec_first=new_vec[i];			// if head node not defined yet, define it
	if(vec_last!=NULL)vec_last->next=new_vec[i];			// if a prev vector exists, create link from it to new vector
	new_vec[i]->prev=vec_last;					// define link from new vector to prev vector (if any)
	new_vec[i]->next=NULL;
	vec_last=new_vec[i];						// redefine the last vector added to list
	h++;								// increment vector counter
	}
      // if a match was found... then delete the match and do not add new vector
      else 
	{
	if(match_vec[i]==vec_first)vec_first=match_vec[i]->next;	// if deleting the head node, redefine it as next node
	if(match_vec[i]==vec_last)vec_last=match_vec[i]->prev;		// if deleting the last node, redefine it as prev node
	vec_prev=match_vec[i]->prev;
	vec_next=match_vec[i]->next;
	if(vec_prev!=NULL)vec_prev->next=vec_next;			// add link around node about to be deleted
	if(vec_next!=NULL)vec_next->prev=vec_prev;			// add link around node about to be deleted in other direction
	free(match_vec[i]->tip); vertex_mem--;
	free(match_vec[i]->tail); vertex_mem--;
	free(match_vec[i]); vector_mem--; match_vec[i]=NULL;		// delete the matching vector from the list
	free(new_vec[i]->tip); vertex_mem--;
	free(new_vec[i]->tail); vertex_mem--;
	free(new_vec[i]); vector_mem--; new_vec[i]=NULL;		// delete the new vec that matched it
	h--;								// decrement vector counter
	}
      }

    pfptr=pfptr->next;							// move onto next facet in patch
    }
      
  //printf("  patch vector list created... %d vectors \n",h);
  
  // sort the vector list tip to tail
  vlist=vector_sort(vec_first,CLOSE_ENOUGH,0);
  if(vlist==NULL)return(FALSE);
  //printf("  patch vectors sorted...\n");
  
  // create 3D polygon(s) from vector list. note that multiple polygons may be created here,
  // because the input patch may have holes in it which creates more free boundary polygons.
  patptr->free_edge=NULL;
  i=0;
  pold=NULL;
  vlptr=vlist;
  while(vlptr!=NULL)
    {
    pptr=veclist_2_single_polygon(vlptr->v_item,SPT_PERIM);
    if(pptr!=NULL)
      {
      pptr->dist=0.0;
      if(patptr->free_edge==NULL)patptr->free_edge=pptr;
      if(pold!=NULL)pold->next=pptr;
      pold=pptr;
      pptr->next=NULL;
      i++;
      }
    vlptr=vlptr->next;
    }
  //printf("  patch %d polygon created...\n",i);
  
  // create a temporary slice and put all the polygon in it so we can figure out which one is
  // the largest (i.e. outer boundary) and which ones are holes.
  patptr->area=0.0;
  sptr=slice_make();
  sptr->pfirst[SPT_PERIM]=patptr->free_edge;
  sptr->pqty[SPT_PERIM]=i;
  pptr=patptr->free_edge;
  polymax=pptr;								// init biggest poly to first in list

  //printf("  sptr=%X  pfirst=%X  pqty=%d   pptr=%X  polymax=%X \n",sptr,sptr->pfirst[SPT_PERIM],sptr->pqty[SPT_PERIM],pptr,polymax);

  while(pptr!=NULL)							// loop thru all polys in free edge list
    {
    polygon_contains_material(sptr,pptr,SPT_PERIM);
    pptr->area=fabs(pptr->area);					// ensure all areas are of positive sign
    pptr->prim=polygon_perim_length(pptr);				// get perimeter length
    //if(pptr->hole==FALSE && pptr->area>polymax->area)polymax=pptr;	// find largest outer polygon
    if(pptr->area > polymax->area)polymax=pptr;				// find largest outer polygon
    patptr->area += pptr->area;						// add up matl and holes to get net area
    pptr=pptr->next;
    }
  free(sptr); slice_mem--; sptr=NULL;
  polymax->hole=0;
  //printf("  patch polygons holes identified...\n");
  
  // re-sort the polygon edge list with the first one (the head node) being the largest outer
  // polygon that encloses material and all the other smaller holes being after it in the linked list
  if(polymax!=patptr->free_edge)
    {
    pptr=patptr->free_edge;
    while(pptr->next!=polymax)pptr=pptr->next;				// find node just before polymax
    pptr->next=polymax->next;						// add link to jump over polymax
    polymax->next=patptr->free_edge;					// move polymax to first node of list
    patptr->free_edge=polymax;						// define it as first node for patch
    }
  //printf("  patch polygons sorted...\n");

  // delete the vector list and its working vectors and vertexes.  this can all be deleted since
  // the free edge polygons were made with copies of these vertexes.
  vlptr=vlist;
  while(vlptr!=NULL)
    {
    vecptr=vlptr->v_item;
    free(vecptr->tip); vertex_mem--;
    free(vecptr->tail); vertex_mem--;
    free(vecptr); vector_mem--;
    vlptr=vlptr->next;
    }
  vlist=vector_list_manager(vlist,NULL,ACTION_CLEAR);

  //printf("Patch find free edge - exit: \n");
  if(patptr->free_edge==NULL)return(FALSE);
  return(TRUE);
}


// Function to create a new support branch
branch *branch_make(void)
{
  branch	*brptr;
  
  brptr=(branch *)malloc(sizeof(branch));
  if(brptr!=NULL)
    {
    brptr->node=NULL;
    brptr->goal=NULL;
    brptr->wght=0.0;
    brptr->status=(-1);
    brptr->pfirst=NULL;
    brptr->patbr=NULL;
    brptr->next=NULL;
    }
  return(brptr);
}

// Function to delete a branch and its contents from memory
int branch_delete(branch *brdel)
{
  vertex 	*vptr,*vdel;
  polygon 	*pptr,*pdel;
  
  if(brdel==NULL)return(FALSE);
  
  // free linked list of nodes
  vptr=brdel->node;							
  while(vptr!=NULL)
    {
    vdel=vptr;
    vptr=vptr->next;
    free(vdel); 
    vertex_mem--;
    }
  
  // free linked list of goals
  vptr=brdel->goal;							
  while(vptr!=NULL)
    {
    vdel=vptr;
    vptr=vptr->next;
    free(vdel); 
    vertex_mem--;
    }

  // free linked list of polygons
  pptr=brdel->pfirst;							
  while(pptr!=NULL)
    {
    pdel=pptr;
    pptr=pptr->next;
    polygon_free(pdel);
    polygon_mem--;
    }
  
  return(TRUE);
}

// Function to sort the branch linked list by node->z value
int branch_sort(branch *br_list)
{
    float 	test_z;
    branch 	*brptr,*brnxt,*brtemp;
  
    if(br_list==NULL)return(FALSE);
    brtemp=branch_make();
    
    // sort by z level
    brptr=br_list;
    while(brptr!=NULL)
      {
      // push all finished branches to end of list
      test_z=brptr->node->z;
      if(brptr->status==1)test_z=(-1.0);
      
      brnxt=brptr->next;
      while(brnxt!=NULL)
        {
	// if the z value of brnxt is more than the z value of brptr, then
	// swap their contents.  swapping pointers messes up the loops.
	if(brnxt->node->z > test_z)				
	  {
	  brtemp->node=brptr->node;					// save brptr in temp
	  brtemp->goal=brptr->goal;
	  brtemp->wght=brptr->wght;
	  brtemp->status=brptr->status;
	  brtemp->pfirst=brptr->pfirst;
	  brtemp->patbr=brptr->patbr;
	  
	  brptr->node=brnxt->node;					// redefine brptr as brnxt
	  brptr->goal=brnxt->goal;
	  brptr->wght=brnxt->wght;
	  brptr->status=brnxt->status;
	  brptr->pfirst=brnxt->pfirst;
	  brptr->patbr=brnxt->patbr;
	  
	  brnxt->node=brtemp->node;					// put brtemp into brnxt
	  brnxt->goal=brtemp->goal;
	  brnxt->wght=brtemp->wght;
	  brnxt->status=brtemp->status;
	  brnxt->pfirst=brtemp->pfirst;
	  brnxt->patbr=brtemp->patbr;
	  }
	brnxt=brnxt->next;
	}
      brptr=brptr->next;
      }
    free(brtemp);
  
  return(TRUE);
}

// Function to create a branch list element
branch_list *branch_list_make(branch *br_inpt)
{
  branch_list	*bl_ptr;
  
  bl_ptr=(branch_list *)malloc(sizeof(branch_list));
  if(bl_ptr!=NULL)
    {
    bl_ptr->b_item=br_inpt;
    bl_ptr->next=NULL;
    }
  return(bl_ptr);
}
  
// Function to manage branch lists - typically used as a "user pick list", but can be used for other things
// actions:  ADD, INSERT, DELETE, CLEAR
branch_list *branch_list_manager(branch_list *bl_list, branch *br_inpt, branch *br_loc, int action)
{
  branch	*brptr;
  branch_list	*bl_ptr,*bl_pre,*bl_del,*bl_new;
  
  // note input checking is dependent on the requested action
  if(action==ACTION_ADD && br_inpt==NULL)return(bl_list);
  if(action==ACTION_INSERT && (br_inpt==NULL || bl_list==NULL))return(bl_list);
  if(action==ACTION_DELETE && (bl_list==NULL || br_inpt==NULL))return(bl_list);
  if(action==ACTION_CLEAR && bl_list==NULL)return(bl_list);
  
  // add new element contianing br_inpt to front of list
  if(action==ACTION_ADD)
    {
    // add new branch to front of list and update ptr to head of list
    bl_ptr=branch_list_make(br_inpt);					// create a new element
    bl_ptr->next=bl_list;
    bl_list=bl_ptr;
    }
    
  // insert new element contianing br_inpt BEFORE element contianing br_loc
  if(action==ACTION_INSERT)
    {
    // if an insert was requested and br_loc==NULL, then add to end of list
    if(br_loc==NULL)
      {
      bl_pre=NULL;
      bl_ptr=bl_list;
      while(bl_ptr!=NULL)
	{
	bl_pre=bl_ptr;
	bl_ptr=bl_ptr->next;
	}
      bl_new=branch_list_make(br_inpt);
      bl_pre->next=bl_new;
      bl_new->next=NULL;
      }  
    else 
      {
      // scan existing list to find br_loc
      bl_pre=NULL;
      bl_ptr=bl_list;
      while(bl_ptr!=NULL)
	{
	if(bl_ptr->b_item==br_loc)break;
	bl_pre=bl_ptr;
	bl_ptr=bl_ptr->next;
	}
      bl_new=branch_list_make(br_inpt);
      if(bl_pre==NULL)
	{
	bl_new->next=bl_list;
	bl_list=bl_new;
	}
      else
	{
	bl_pre->next=bl_new;
	bl_new->next=bl_ptr;
	}
      }
    }

  // delete list element contianing br_inpt
  if(action==ACTION_DELETE)
    {
    bl_pre=NULL;
    bl_ptr=bl_list;
    while(bl_ptr!=NULL)							// loop thru list to find pre node to delete node
      {
      if(bl_ptr->b_item==br_inpt)break;
      bl_pre=bl_ptr;
      bl_ptr=bl_ptr->next;
      }
    if(bl_ptr!=NULL)							// if the delete node was found...
      {
      bl_del=bl_ptr;							// save address in temp ptr
      if(bl_pre==NULL)							// if head node, redefine head to be next node (which may be NULL if only one node in list)
        {bl_list=bl_ptr->next;}
      else 
        {bl_pre->next=bl_ptr->next;}					// otherwise skip over node to be deleted
      free(bl_del);
      }
    }
   
  // clear the entire list (does not clear branches, just list elements)
  if(action==ACTION_CLEAR)
    {
    bl_ptr=bl_list;
    while(bl_ptr!=NULL)
      {
      bl_del=bl_ptr;
      bl_ptr=bl_ptr->next;
      free(bl_del);
      }
    bl_list=NULL;
    }
    
  return(bl_list);
}  




