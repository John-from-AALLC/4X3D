#include "Global.h"
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

// Callback to destroy tool UI when user exits
void on_model_exit (GtkWidget *btn, gpointer dead_window)
{
  
  printf("OnModelUIExit:  job_regen=%d \n",job.regen_flag);
  gtk_window_close(GTK_WINDOW(win_model));
  gtk_widget_queue_draw(win_main);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
  
  if(job.regen_flag==TRUE)job_slice();

  idle_start_time=time(NULL);						// reset idle time
  win_modelUI_flag=FALSE;
  display_model_flag=TRUE;
  //memory_status();
  return;
}

// Callback to set response for model create error
void cb_model_create_error_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if(response_id==GTK_RESPONSE_OK)gtk_window_close(GTK_WINDOW(dialog));
    return;
}

// Callback to get response for file dialog
void cb_file_dialog_response(GtkNativeDialog *dialog, int response)
{
  int		h,i,slot,col_width,status=1;
  int		xpix,ypix;
  float 	xscale,yscale;
  float 	fval;
  model 	*mnew,*mptr,*mdel;
  model 	*old_active_model;
  char 		mdl_name[255];
  genericlist	*aptr;
  linetype 	*linetype_ptr;
  GtkWidget	*win_local_dialog;

  if(response==GTK_RESPONSE_ACCEPT)
    {
    GFile *file=gtk_file_chooser_get_file (GTK_FILE_CHOOSER(dialog));
    strcpy(new_mdl, g_file_get_path(file));
    printf("\nModel Selected: %s \n",new_mdl);
    
    display_model_flag=FALSE;
    if(strlen(new_mdl)<1)return;
    
    // if there is an active model, save its address as the target for images
    old_active_model=active_model;
    active_model=NULL;

    model_in=fopen(new_mdl,"rb");						// attempt file open
    if(model_in==NULL)							// if unable to open then ...
      {
      printf("ERROR: Unable to open model file.\n");
      return;
      }
  
    // build and load new model
    mnew=model_make();         						// allocate memory of a new model
    if(mnew==NULL)							// if unable to create new model...
      {
      // display dialog to user, await response
      sprintf(scratch," \nUnable to create new model in memory!\n");
      aa_dialog_box(win_model,1,0,"Model Creation Error",scratch);
      printf("ERROR: Unable to make new model.\n");
      return;
      }
  
    // load and define model based on extension type
    strcpy(mnew->model_file,new_mdl);					// copy over the new model file name
    if(strstr(mnew->model_file,".stl")!=NULL)mnew->input_type=STL;
    if(strstr(mnew->model_file,".Stl")!=NULL)mnew->input_type=STL;
    if(strstr(mnew->model_file,".STL")!=NULL)mnew->input_type=STL;
    if(strstr(mnew->model_file,".gocde")!=NULL)mnew->input_type=GCODE;
    if(strstr(mnew->model_file,".GCode")!=NULL)mnew->input_type=GCODE;
    if(strstr(mnew->model_file,".GCODE")!=NULL)mnew->input_type=GCODE;
    if(strstr(mnew->model_file,".art")!=NULL)mnew->input_type=GERBER;
    if(strstr(mnew->model_file,".ART")!=NULL)mnew->input_type=GERBER;
    if(strstr(mnew->model_file,".gbr")!=NULL)mnew->input_type=GERBER;
    if(strstr(mnew->model_file,".GBR")!=NULL)mnew->input_type=GERBER;
    if(strstr(mnew->model_file,".jpg")!=NULL)mnew->input_type=IMAGE;
    if(strstr(mnew->model_file,".JPG")!=NULL)mnew->input_type=IMAGE;
  
    // load STL files
    if(mnew->input_type==STL)
      {
      printf("\nLoading STL file...\n\n");
      // loads the vertices, edges, and facets into structured memory
      status=stl_model_load(mnew);
      fclose(model_in);
      if(status==FALSE)
	{
	// delete empty model pointer that was created prior to checking file
	model_delete(mnew);
	
	// let user know what's going on
	sprintf(scratch," \nUnable to load STL file!\n");
	aa_dialog_box(win_model,1,0,"STL Load Error",scratch);
	printf("ERROR: Unable to load STL model.\n");
	return;
	}
      
      // STL now in memory. It will be sliced and processed upon Model_UI exit
      //display_model_flag=FALSE;
      slot=0;
      if(Tool[slot].type==TC_LASER && job.model_count>0)
        {
	autoplacement_flag=FALSE;
	//set_center_build_job=FALSE;
	set_auto_zoom=FALSE;
	}
      z_cut=0.200;
      job.regen_flag=TRUE;
      mnew->reslice_flag=TRUE;
      model_auto_orient(mnew);
      mnew->MRedraw_flag=TRUE;
      }
  
    // load GCODE files
    if(mnew->input_type==GCODE)
      {
      printf("\nLoading GCode file...\n\n");
      //status=gcode_model_load(mnew);
      status=FALSE;
      if(status==FALSE)
	{
	// delete empty model pointer that was created prior to checking file
	model_delete(mnew);
	
	// let user know what's going on
	sprintf(scratch,"\nUnable to load GCode file!\n");
	aa_dialog_box(win_model,1,0,"GCode Load Error",scratch);
	printf("ERROR: Unable to load GCode model.\n");
	return;
	}
  
      }
  
    // load GERBER files
    if(mnew->input_type==GERBER)
      {
      printf("\nLoading GERBER file...\n\n");
      fclose(model_in);
      
      // create 3D model by extruding the gerber slice in z by 1mm
      
      
      // Gerbers are loaded directly into a slice upon Model_UI exit
      job.min_slice_thk=0.25;					// ... set min slice thickness for job
      z_cut=0.25;							// ... set current layer
      job.regen_flag=TRUE;					// ... set flag that job is ready
      mnew->reslice_flag=TRUE;
      printf("\nGERBER file successfully loaded.\n\n");
      }
 
    // load IMAGE files
    if(mnew->input_type==IMAGE)
      {
      // if a model is active, ask user if this image belongs with it
      if(old_active_model!=NULL)
        {
	aa_dialog_box(win_model,3,0,"Add Image","Add new image to the active model?");
	if(aa_dialog_result==TRUE)
	  {
	  model_convert_type(old_active_model,TARGET);			// convert from whatever to a TARGET type
	  mdel=mnew;
	  mnew=old_active_model;
	  free(mdel); model_mem--;					// since not "inserted" yet
	  }
	}
	
      status=FALSE;
      printf("\nLoading IMAGE file...\n\n");
      mnew->xscl[MODEL]=1.0;						// these need to be set befor image loading
      mnew->yscl[MODEL]=1.0;						// as they will scale the image to these values
      mnew->zscl[MODEL]=1.0;						// as they will scale the image to these values
      
      // find a loaded tool that can work with images to get likely line width
      // pix_size controls to how the image size is derived from a pixel count to size in mm
      mnew->pix_size=0.20;
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
	if(linetype_ptr!=NULL)mnew->pix_size=linetype_ptr->line_width;
	}
      if(mnew->pix_size<0.10)mnew->pix_size=0.10;			// ensure we are viable
      if(mnew->pix_size>25.0)mnew->pix_size=25.0;
      
      // check image size by loading the image in its natural resolution
      mnew->g_img_mdl_buff = gdk_pixbuf_new_from_file_at_scale(mnew->model_file, (-1), (-1), TRUE, NULL);
      if(mnew->g_img_mdl_buff!=NULL)
	{
	xpix = gdk_pixbuf_get_width (mnew->g_img_mdl_buff);		// number of columns (image size)
	ypix = gdk_pixbuf_get_height (mnew->g_img_mdl_buff);		// number of rows (image size)
	
	// ask user if they want to fit it to the existing target
	sprintf(scratch,"Image size: %d x %d \n Match scale with target?",xpix,ypix);
	aa_dialog_box(win_model,3,0,"Add Image",scratch);
    
	aa_dialog_result=(-1);						// wait for user to confirm change
	while(aa_dialog_result<0)
	  {
	  delay(100);
	  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
	  }
    
	// scale based on largest dimension
	if(aa_dialog_result==TRUE)
	  {
	  mnew->xmax[TARGET]=360.0;
	  mnew->ymax[TARGET]=360.0;
	  xscale=(xpix*mnew->pix_size)/mnew->xmax[TARGET];
	  yscale=(ypix*mnew->pix_size)/mnew->ymax[TARGET];
	  if(xscale>=yscale && xscale>1.0)
	    {
	    mnew->xscl[MODEL]=1/xscale;
	    mnew->yscl[MODEL]=1/xscale;
	    }
	  if(yscale>=xscale && yscale>1.0)
	    {
	    mnew->xscl[MODEL]=1/yscale;
	    mnew->yscl[MODEL]=1/yscale;
	    }
	  
	  xpix *= mnew->xscl[MODEL];
	  ypix *= mnew->yscl[MODEL];
	  if(xpix<10)xpix=10;
	  if(ypix<10)ypix=10;
	  //if(xpix>(BUILD_TABLE_MAX_X*mnew->pix_size))xpix=BUILD_TABLE_MAX_X*mnew->pix_size;
	  //if(ypix>(BUILD_TABLE_MAX_Y*mnew->pix_size))ypix=BUILD_TABLE_MAX_Y*mnew->pix_size;
	  }
	}

      // loads the image into model GDK buffer
      if(image_loader(mnew,xpix,ypix,TRUE)==TRUE)			// load at nominal 300 pix ht perserving aspect
        {
	// flip in y as artifact regarding how they are loaded
	mnew->ymir[MODEL]=1;
	model_mirror(mnew);									// apply mirroring

	// note the MODEL array descriptor.  the "image" is the model in this case.
	mnew->xmin[MODEL]=0;
	mnew->ymin[MODEL]=0;
	mnew->zmin[MODEL]=0;
	
	mnew->xmax[MODEL]=mnew->act_width*mnew->pix_size; 
	mnew->ymax[MODEL]=mnew->act_height*mnew->pix_size;
	mnew->zmax[MODEL]=cnc_z_height;
	
	mnew->xoff[MODEL]=(BUILD_TABLE_MAX_X-mnew->xmax[MODEL])/2;    	
	mnew->yoff[MODEL]=(BUILD_TABLE_MAX_Y-mnew->ymax[MODEL])/2;    	
	mnew->zoff[MODEL]=0;
	
	mnew->xorg[MODEL]=mnew->xoff[MODEL];    	
	mnew->yorg[MODEL]=mnew->yoff[MODEL];    	
	mnew->zorg[MODEL]=0;
	
	mnew->geom_type=MODEL;
	
	// if no block stl exists yet add in default block
	// do this after image load so block knows dimensions to build to
	if(mnew->facet_qty[TARGET]<=0)
	  {
	  model_build_target(mnew);
	  mnew->zmin[MODEL]=0.0;
	  mnew->zmax[MODEL]=2*mnew->slice_thick;
	  mnew->zoff[MODEL]=mnew->zmax[TARGET]-mnew->zmax[MODEL];
	  mnew->zorg[MODEL]=mnew->zoff[MODEL];
	  }
  
	status=TRUE;
	}
      //status=image_model_load(mnew);
      if(status==FALSE)
	{
	// delete empty model pointer that was created prior to checking file
	model_delete(mnew);
	
	// let user know what's going on
	sprintf(scratch," \nUnable to load IMAGE file!\n");
	aa_dialog_box(win_model,1,0,"IMAGE Load Error",scratch);
	printf("ERROR: Unable to load IMAGE model.\n");
	return;
	}
      
      // model now in memory. It will be sliced and processed upon Model_UI exit
      //display_model_flag=FALSE;
      z_cut=0.200;
      job.regen_flag=TRUE;
      mnew->reslice_flag=TRUE;
      mnew->MRedraw_flag=TRUE;
      }
    
    // if in the win_UI screen, update the display widgets...
    if(win_modelUI_flag==TRUE)
      {
      // update the model load button color
      //gtk_image_clear(GTK_IMAGE(img_model_load));				// MUST be cleared then re-loaded or it becomes severe memory leak
      //img_model_load=gtk_image_new_from_file("Model-Add-G.gif");
      //gtk_image_set_pixel_size(GTK_IMAGE(img_model_load),50);
      //gtk_button_set_child(GTK_BUTTON(btn_model_load),img_model_load);
      
      // update model in error label
      col_width=25;
      sprintf(scratch,"    ");
      if(mnew->error_status!=0){sprintf(scratch,"   Flawed");col_width=18;}
      while(strlen(scratch)<col_width)strcat(scratch," ");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      gtk_label_set_text(GTK_LABEL(lbl_mdl_error),scratch);
      gtk_label_set_markup(GTK_LABEL(lbl_mdl_error),markup);
      gtk_widget_queue_draw(lbl_mdl_error);
    
      // add the name of this new model to the combo box list
      sprintf(scratch,"%s",mnew->model_file);
      if(strlen(scratch)>70)
	{
	strcpy(mdl_name,&scratch[strlen(scratch)-70]);
	sprintf(scratch,"...");
	strcat(scratch,mdl_name);
	}
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mdls), NULL, scratch);
      
      // set the latest model added as the active pick in the combo box
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mdls), job.model_count-1);
      active_model=mnew;
      printf("Model UI: model id assigned\n");
    
      // update position widgets as result of auto-orient
      update_size_values(mnew);
      
      // finally turn off all previously selected boxes and activate operation check boxes for loaded tools
      for(h=1;h<MAX_OP_TYPES;h++)
	{
	for(i=0;i<MAX_TOOLS;i++)
	  {
	  gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[h][i]), FALSE);	// turn off all buttons
	  aptr=Tool[i].oper_list;
	  while(aptr!=NULL)
	    {
	    if(aptr->ID==h)break;
	    aptr=aptr->next;
	    }
	  if(aptr!=NULL)
	    {
	    gtk_widget_set_sensitive(btn_op[h][i],TRUE);
	    }
	  else 
	    {
	    gtk_widget_set_sensitive(btn_op[h][i],FALSE);
	    }
	  }
	}
      }		// end of if in win_UI
  
    if(mnew!=old_active_model)model_insert(mnew);			// insert it into linked list of models for the job
    active_model=mnew;
    active_model->MRedraw_flag=TRUE;					// set to update silhouette edges
    }

  // if we successfully loaded something, process it...
  if(active_model!=NULL)
    {
    model_rotate(active_model);						// auto-orient sets rot values, now need to execute them
    model_maxmin(active_model);						// re-establish xyz boundaries of this model after rotating
    job_maxmin();							// reset job size after rotation
    }
  // otherwise set back to previous active model...
  else 
    {
    active_model=old_active_model;
    }

  display_model_flag=TRUE;
  gtk_window_close(GTK_WINDOW(dialog));
  gtk_widget_queue_draw(g_model_area);
  while (g_main_context_iteration(NULL, FALSE));			// update display promptly

  //mptr=job.model_first;
  //while(mptr!=NULL)
  //  {
  //  model_dump(mptr);
  //  mptr=mptr->next;
  //  }

  return;
}

// Function to add a model file to the tool
model *model_file_load(GtkWidget *btn_call, gpointer src_window)
{
  GtkWidget 	*win_file_load;
  GtkWidget	*grd_file_load;
  GtkWidget	*label;
  GtkWidget	*button;
  GtkFileFilter	*ALLfilter,*STLfilter,*GERBERfilter,*GCODEfilter,*JPEGfilter;
  
  
  if(job.state>=JOB_RUNNING)return(0);

  STLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(STLfilter,"*.STL");
  gtk_file_filter_add_pattern(STLfilter,"*.stl");
  gtk_file_filter_set_name (STLfilter,"*.stl");
  JPEGfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(JPEGfilter,"*.JPG");
  gtk_file_filter_add_pattern(JPEGfilter,"*.jpg");
  gtk_file_filter_set_name (JPEGfilter,"*.jpg");
  GERBERfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(GERBERfilter,"*.ART");
  gtk_file_filter_add_pattern(GERBERfilter,"*.art");
  gtk_file_filter_set_name (GERBERfilter,"*.art");
  GCODEfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(GCODEfilter,"*.GCODE");
  gtk_file_filter_add_pattern(GCODEfilter,"*.gcode");
  gtk_file_filter_set_name (GCODEfilter,"*.gcode");
  ALLfilter=gtk_file_filter_new ();
  gtk_file_filter_add_pattern(ALLfilter,"*.*");
  gtk_file_filter_set_name (ALLfilter,"*.*");

  GtkFileChooserNative *file_dialog;
  file_dialog = gtk_file_chooser_native_new("Open File", GTK_WINDOW(win_main), GTK_FILE_CHOOSER_ACTION_OPEN, "_Open", "_Cancel");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), STLfilter);	
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), JPEGfilter);	
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), GERBERfilter);	
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER(file_dialog), ALLfilter);	
  g_signal_connect (file_dialog, "response", G_CALLBACK (cb_file_dialog_response), NULL);
  gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(file_dialog), TRUE);
  gtk_native_dialog_show (GTK_NATIVE_DIALOG (file_dialog));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}

  gtk_widget_queue_draw(GTK_WIDGET(src_window));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  printf("Model UI file load exit\n");

  return(NULL);
}

// Function to remove a model file from the tool
int model_file_remove(GtkWidget *btn_call, gpointer user_data)
{
  int		i,h,slot,op_typ,clear_item;
  char 		mdl_name[255];
  model		*mptr;
  GtkWidget 	*dialog;
  genericlist	*aptr;
  operation	*optr;
  
  printf("\nEntering model_file_remove...\n");

  // check if the model to be deleted makes sense
  if(job.state>=JOB_RUNNING)return(0);
  mptr=active_model;
  if(mptr==NULL)return(0);
  active_model=NULL;
  
  // if all good, purge everything it references
  model_delete(mptr);
 
  // if in the win_UI screen, update the display widgets...
  if(win_modelUI_flag==TRUE)
    {
    // update the model load button color
    gtk_image_clear(GTK_IMAGE(img_model_load));				// MUST be cleared then re-loaded or it becomes severe memory leak
    if(job.model_count==0)img_model_load=gtk_image_new_from_file("Model-Add-Y.gif");
    if(job.model_count>0)img_model_load=gtk_image_new_from_file("Model-Add-G.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_model_load),50);
    gtk_button_set_child(GTK_BUTTON(btn_model_load),img_model_load);
    
    // rebuild the combobox list
    gtk_list_store_clear(GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_mdls))));
    i=0;
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      sprintf(scratch,"%s",mptr->model_file);
      if(strlen(scratch)>70)
	{
	strcpy(mdl_name,&scratch[strlen(scratch)-70]);
	sprintf(scratch,"...");
	strcat(scratch,mdl_name);
	}
      gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mdls),NULL,scratch);
      i++;
      mptr=mptr->next;
      }
    if(i>0)gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mdls), i-1);  
    active_model=job.model_first;
    
    // update copies and autoplacement widgets
    //if(active_model!=NULL)gtk_adjustment_set_value(mdl_copies_adj,active_model->total_copies);
    
    // update position, rotation, scale, and mirror widgets
  
    // reset operational tool checkboxes
    aptr=NULL;
    if(active_model!=NULL)aptr=active_model->oper_list;
    if(aptr==NULL)							// if this model currently has no operations defined...
      {
      for(h=1;h<MAX_OP_TYPES;h++)
	{
	for(slot=0;slot<MAX_TOOLS;slot++)
	  {
	  gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[h][slot]), FALSE);	// turn off all buttons
	  aptr=Tool[slot].oper_list;
	  while(aptr!=NULL)
	    {
	    if(aptr->ID==h)break;
	    aptr=aptr->next;
	    }
	  if(aptr!=NULL)
	    {gtk_widget_set_sensitive(btn_op[h][slot],TRUE);}
	  else 
	    {gtk_widget_set_sensitive(btn_op[h][slot],FALSE);}
	  }
	}
      }
    else 									// otherwise, clear ops and turn on what is defined for this model...
      {
      while(aptr!=NULL)							// loop thru all operations currently requested for this model
	{
	for(slot=0;slot<MAX_TOOLS;slot++)
	  {
	  optr=operation_find_by_name(slot,aptr->name);			// determine if the tool in this slot can perform the operation
	  if(optr==NULL)continue;						// if it can't, NULL is returned so just skip it
	  op_typ=optr->ID;						// if it can, assign index value from ID
	  if(aptr->ID==slot)						// if this model's operation is currently using this slot(tool)....
	    {
	    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),TRUE);	// turn on the checkbox
	    for(i=0;i<MAX_TOOLS;i++)
	      {
	      if(i==slot){gtk_widget_set_sensitive(btn_op[op_typ][i],TRUE);}		// turn on the ability to check/de-check this box
	      else {gtk_widget_set_sensitive(btn_op[op_typ][i],FALSE);}			// turn off the ability to check/de-check this box
	      }
	    }
	  else 
	    {
	    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),FALSE);	// turn off the checkbox
	    }
	  }
	aptr=aptr->next;							// move onto next operation for this model
	}
      }
    printf("MUI:  operation selection buttons updated... \n");
    }

  // clear the history values
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
  
  printf("Model clear:  exit\n");
 
  return(1);
}

// Function to define which model to apply changes to
int model_select(GtkWidget *cmb_box, gpointer user_data)
{
  int		mtyp=MODEL;
  int		i,h,idx,op_typ,slot,col_width;
  model 	*mptr;
  model 	*mnew;
  char		new_mdl[255];
  GtkWidget 	*win_info;
  GtkWidget	*grd_info;
  GtkWidget	*label;
  GtkWidget	*button;
  GtkWidget 	*dialog;
  genericlist	*aptr;
  operation	*optr;
  
  if(job.state>=JOB_RUNNING)return(0);

  // find the pointer to the right model based on combo box index
  idx=gtk_combo_box_get_active(GTK_COMBO_BOX(combo_mdls));
  h=0;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    if(h==idx)break;
    mptr=mptr->next;
    h++;
    }
  if(mptr==NULL)return(0);
  printf("idx=%d  mptr: id=%d  name=%s \n",idx,mptr->model_ID,mptr->model_file);
  
  // update model error label
  col_width=25;
  sprintf(scratch,"    ");
  if(mptr->error_status!=0){sprintf(scratch,"   Flawed");col_width=18;}
  while(strlen(scratch)<col_width)strcat(scratch," ");
  markup = g_markup_printf_escaped (error_fmt,scratch);
  gtk_label_set_text(GTK_LABEL(lbl_mdl_error),scratch);
  gtk_label_set_markup(GTK_LABEL(lbl_mdl_error),markup);
  gtk_widget_queue_draw(lbl_mdl_error);

  // set active model to what's been selected so other functions know what it currently is
  active_model=mptr;
  
  // update number of copies
  //gtk_adjustment_set_value(mdl_copies_adj,active_model->total_copies);

  // update the orientation values to reflect selected model
  gtk_adjustment_set_value(xposadj,active_model->xoff[mtyp]);
  gtk_adjustment_set_value(yposadj,active_model->yoff[mtyp]);
  gtk_adjustment_set_value(zposadj,active_model->zoff[mtyp]);
  gtk_adjustment_set_value(xrotadj,active_model->xrot[mtyp]*180/PI);	// model value in rads, spin btn in degrees
  gtk_adjustment_set_value(yrotadj,active_model->yrot[mtyp]*180/PI);
  gtk_adjustment_set_value(zrotadj,active_model->zrot[mtyp]*180/PI);
  gtk_adjustment_set_value(xscladj,active_model->xscl[mtyp]);
  gtk_adjustment_set_value(yscladj,active_model->yscl[mtyp]);
  gtk_adjustment_set_value(zscladj,active_model->zscl[mtyp]);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(xmirbtn), active_model->xmir[mtyp]);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(ymirbtn), active_model->ymir[mtyp]);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(zmirbtn), active_model->zmir[mtyp]);

  // reset operational tool checkboxes
  aptr=NULL;
  if(active_model!=NULL)aptr=active_model->oper_list;
  if(aptr==NULL)							// if this model currently has no operations defined...
    {
    for(h=1;h<MAX_OP_TYPES;h++)
      {
      for(slot=0;slot<MAX_TOOLS;slot++)
	{
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[h][slot]), FALSE);	// turn off all buttons
	aptr=Tool[slot].oper_list;
	while(aptr!=NULL)
	  {
	  if(aptr->ID==h)break;
	  aptr=aptr->next;
	  }
	if(aptr!=NULL)
	  {gtk_widget_set_sensitive(btn_op[h][slot],TRUE);}
	else 
	  {gtk_widget_set_sensitive(btn_op[h][slot],FALSE);}
	}
      }
    }
  else 									// otherwise, clear ops and turn on what is defined for this model...
    {
    while(aptr!=NULL)							// loop thru all operations currently requested for this model
      {
      for(slot=0;slot<MAX_TOOLS;slot++)
	{
	optr=operation_find_by_name(slot,aptr->name);			// determine if the tool in this slot can perform the operation
	if(optr==NULL)continue;						// if it can't, NULL is returned so just skip it
	op_typ=optr->ID;						// if it can, assign index value from ID
	if(aptr->ID==slot)						// if this model's operation is currently using this slot(tool)....
	  {
	  gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),TRUE);	// turn on the checkbox
	  for(i=0;i<MAX_TOOLS;i++)
	    {
	    if(i==slot){gtk_widget_set_sensitive(btn_op[op_typ][i],TRUE);}		// turn on the ability to check/de-check this box
	    else {gtk_widget_set_sensitive(btn_op[op_typ][i],FALSE);}			// turn off the ability to check/de-check this box
	    }
	  }
	else 
	  {
	  gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),FALSE);	// turn off the checkbox
	  }
	}
      aptr=aptr->next;							// move onto next operation for this model
      }
    }

  return(1);
}

void grab_xpos_value(GtkSpinButton *button, gpointer user_data)
{
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->xoff[MODEL]=gtk_spin_button_get_value(button);
  
  return;
}

void grab_ypos_value(GtkSpinButton *button, gpointer user_data)
{
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->yoff[MODEL]=gtk_spin_button_get_value(button);
  
  return;
}

void grab_zpos_value(GtkSpinButton *button, gpointer user_data)
{
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->zoff[MODEL]=gtk_spin_button_get_value(button);
  
  return;
}

void grab_xrotate_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->xrot[mtyp]=gtk_spin_button_get_value(button)*PI/180;	// from spin btn degrees to model radians
  model_rotate(active_model);						// apply rotations
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);					// update size display
  
  return;
}

void grab_yrotate_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->yrot[mtyp]=gtk_spin_button_get_value(button)*PI/180;
  model_rotate(active_model);						// apply rotations
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);
    
  return;
}

void grab_zrotate_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->zrot[mtyp]=gtk_spin_button_get_value(button)*PI/180;
  model_rotate(active_model);						// apply rotations
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);

  return;
}


void grab_xscl_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->xscl[mtyp]=gtk_spin_button_get_value(button);
  if(active_model->xscl[mtyp]<0.01)active_model->xscl[mtyp]=0.01;
  if(active_model->xscl[mtyp]>100)active_model->xscl[mtyp]=100;
  model_scale(active_model,mtyp);					// apply user scaling
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);
  
  return;
}

void grab_yscl_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->yscl[mtyp]=gtk_spin_button_get_value(button);
  if(active_model->yscl[mtyp]<0.01)active_model->yscl[mtyp]=0.01;
  if(active_model->yscl[mtyp]>100)active_model->yscl[mtyp]=100;
  model_scale(active_model,mtyp);					// apply user scaling
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);

  return;
}

void grab_zscl_value(GtkSpinButton *button, gpointer user_data)
{
  int	mtyp=MODEL;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->zscl[mtyp]=gtk_spin_button_get_value(button);
  if(active_model->zscl[mtyp]<0.01)active_model->zscl[mtyp]=0.01;
  if(active_model->zscl[mtyp]>100)active_model->zscl[mtyp]=100;
  model_scale(active_model,mtyp);					// apply user scaling
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries
  update_size_values(active_model);

  return;
}


void xmirbtn_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int 	slot;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->xmir[MODEL]++;
  if(active_model->xmir[MODEL]>1)active_model->xmir[MODEL]=0;
  model_mirror(active_model);						// apply mirroring
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries

  // check if any tools have slice data for this model, if so purge it
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
    }

  //job.regen_flag=TRUE;
  //job_purge();								// purge support and slice data
  
  return;
}

void ymirbtn_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int 	slot;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->ymir[MODEL]++;
  if(active_model->ymir[MODEL]>1)active_model->ymir[MODEL]=0;
  model_mirror(active_model);						// apply mirroring
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries

  // check if any tools have slice data for this model, if so purge it
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
    }
  //job.regen_flag=TRUE;
  //job_purge();								// purge support and slice data
  
  return;
}

void zmirbtn_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	slot;
  
  if(job.state>=JOB_RUNNING)return;
  if(active_model==NULL)return;
  active_model->zmir[MODEL]++;
  if(active_model->zmir[MODEL]>1)active_model->zmir[MODEL]=0;
  model_mirror(active_model);						// apply mirroring
  model_maxmin(active_model);						// re-establish its boundaries
  job_maxmin();								// re-establish its boundaries

  // check if any tools have slice data for this model, if so purge it
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
    }
  //job.regen_flag=TRUE;
  //job_purge();								// purge support and slice data
  
  return;
}


void update_size_values(model *mnew)
{
  int		mtyp;
  float 	fval;

  //for(mtyp=MODEL;mtyp<MAX_MDL_TYPES;mtyp++)
  mtyp=MODEL;
    {
    // update size display
    fval=(mnew->xscl[mtyp]/mnew->xspr[mtyp])*(mnew->xmax[mtyp]-mnew->xmin[mtyp]);
    sprintf(scratch,"%6.3f",fval);
    if(fval>BUILD_TABLE_LEN_X){markup = g_markup_printf_escaped (error_fmt,scratch);}
    else {markup = g_markup_printf_escaped (norm_fmt,scratch);}
    gtk_label_set_text(GTK_LABEL(lbl_x_size),scratch);
    gtk_label_set_markup(GTK_LABEL(lbl_x_size),markup);
    gtk_widget_queue_draw(lbl_x_size);

    fval=(mnew->yscl[mtyp]/mnew->yspr[mtyp])*(mnew->ymax[mtyp]-mnew->ymin[mtyp]);
    sprintf(scratch,"%6.3f",fval);
    if(fval>BUILD_TABLE_LEN_Y){markup = g_markup_printf_escaped (error_fmt,scratch);}
    else {markup = g_markup_printf_escaped (norm_fmt,scratch);}
    gtk_label_set_text(GTK_LABEL(lbl_y_size),scratch);
    gtk_label_set_markup(GTK_LABEL(lbl_y_size),markup);
    gtk_widget_queue_draw(lbl_y_size);

    fval=(mnew->zscl[mtyp]/mnew->zspr[mtyp])*(mnew->zmax[mtyp]-mnew->zmin[mtyp]);
    sprintf(scratch,"%6.3f",fval);
    if(fval>BUILD_TABLE_LEN_Z){markup = g_markup_printf_escaped (error_fmt,scratch);}
    else {markup = g_markup_printf_escaped (norm_fmt,scratch);}
    gtk_label_set_text(GTK_LABEL(lbl_z_size),scratch);
    gtk_label_set_markup(GTK_LABEL(lbl_z_size),markup);
    gtk_widget_queue_draw(lbl_z_size);
    }
    
  mnew->reslice_flag=TRUE;
  printf("Model size update done: exiting \n");
  return;
}


void btn_op_add_model_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_ADD_MODEL_MATERIAL;
  set_operation_checkbutton(slot,op_typ);
  return;
}

void btn_op_add_supt_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_ADD_SUPPORT_MATERIAL;
  set_operation_checkbutton(slot,op_typ);
  return;
}

void btn_op_add_base_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_ADD_BASE_LAYER;
  set_operation_checkbutton(slot,op_typ);
  if(job.baselayer_flag==FALSE){job.baselayer_flag=TRUE;}
  else {job.baselayer_flag=FALSE;}
  job.regen_flag=TRUE;
  return;
}


void btn_op_mill_outline_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model	*mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MILL_OUTLINE;
  set_operation_checkbutton(slot,op_typ);

  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MILL_OUTLINE");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      model_build_target(mptr);						// ... add raw block that represents the target to cut into
      if(set_start_at_crt_z==0)set_start_at_crt_z=1;			// ... turn on start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      set_start_at_crt_z=0;						// ... turn off start at z
      }
    mptr=mptr->next;
    }

  return;
}

void btn_op_mill_area_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model *mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MILL_AREA;
  set_operation_checkbutton(slot,op_typ);

  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MILL_AREA");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      if(set_start_at_crt_z==0)set_start_at_crt_z=1;			// ... turn on start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      set_start_at_crt_z=0;						// ... turn off start at z
      }
    mptr=mptr->next;
    }

  return;
}

void btn_op_mill_profile_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MILL_PROFILE;
  set_operation_checkbutton(slot,op_typ);
  return;
}

void btn_op_mill_holes_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MILL_HOLES;
  set_operation_checkbutton(slot,op_typ);
  return;
}


void btn_op_meas_x_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MEASURE_X;
  set_operation_checkbutton(slot,op_typ);
  return;
}

void btn_op_meas_y_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MEASURE_Y;
  set_operation_checkbutton(slot,op_typ);
  return;
  
}

void btn_op_meas_z_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MEASURE_Z;
  set_operation_checkbutton(slot,op_typ);
  return;
  
}

void btn_op_meas_holes_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MEASURE_HOLES;
  set_operation_checkbutton(slot,op_typ);
  return;
}


void btn_op_mark_outline_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model	*mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MARK_OUTLINE;
  set_operation_checkbutton(slot,op_typ);
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MARK_OUTLINE");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      //if(set_start_at_crt_z==0)set_start_at_crt_z=1;			// ... turn on start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      set_start_at_crt_z=0;						// ... turn off start at z
      }
    mptr=mptr->next;
    }

  return;
}

void btn_op_mark_area_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model *mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MARK_AREA;
  set_operation_checkbutton(slot,op_typ);
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MARK_AREA");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      //if(set_start_at_crt_z==0)set_start_at_crt_z=1;			// ... turn on start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      set_start_at_crt_z=0;						// ... turn off start at z
      }
    mptr=mptr->next;
    }
  
  return;
}

void btn_op_mark_image_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model	*mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MARK_IMAGE;
  set_operation_checkbutton(slot,op_typ);
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MARK_IMAGE");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      set_start_at_crt_z=0;						// ... turn off start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      }
    mptr=mptr->next;
    }
  
  return;
}

void btn_op_mark_cut_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	i,op_typ,slot,mtyp=MODEL;
  model	*mptr;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_MARK_CUT;
  set_operation_checkbutton(slot,op_typ);
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    i=genericlist_find(mptr->oper_list,"MARK_CUT");
    if(i==slot)								// if this operation is selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// ... add base height to model z
      //if(set_start_at_crt_z==0)set_start_at_crt_z=1;			// ... turn on start at z
      job.current_z=z_cut;						// ... set job z to match
      }
    else 								// or if this operation is de-selected...
      {
      mptr->zoff[mtyp]=mptr->zorg[mtyp];				// ... reset model z to original location
      set_start_at_crt_z=0;						// ... turn off start at z
      }
    mptr=mptr->next;
    }

  return;
}

void btn_op_cure_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_CURE;
  set_operation_checkbutton(slot,op_typ);
  return;
}

void btn_op_place_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	op_typ,slot;
  
  slot=GPOINTER_TO_INT(user_data);
  op_typ=OP_PLACE;
  set_operation_checkbutton(slot,op_typ);
  return;
}

// Callback to set response for model assignment error
void cb_model_assignment_error_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if(response_id==GTK_RESPONSE_OK)gtk_window_close(GTK_WINDOW(dialog));
    return;
}

// Function to set operation checkbutton called from each of the btn_op callbacks
void set_operation_checkbutton(int slot, int op_typ)
{
  GtkWidget	*win_dialog;
  int		i,ok;
  genericlist	*gptr,*gdel,*aptr;
  operation	*optr;
  model		*mptr;
  linetype 	*lt_ptr;
  
  if(slot<0 || slot>=MAX_TOOLS)return;
  
  // find the actual tool operation that matches these inputs
  optr=Tool[slot].oper_first;
  while(optr!=NULL)
    {
    if(optr->ID==op_typ)break;
    optr=optr->next;
    }
  if(optr==NULL){printf("set_op error:  no op found!\n");return;}

  // if the user set the checkbox to active...
  ok=1;
  if(gtk_check_button_get_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot])))		
    {
    if(ok && active_model==NULL)
      {
      sprintf(scratch,"\nNo models have been selected.\nSelect a model first, then\nassign operations.\n");
      ok=0;
      }
    if(ok && Tool[slot].state<TL_LOADED)
      {
      sprintf(scratch,"\nUnable to assign model to\ntool that is not defined.\n");
      ok=0;
      }
    if(ok && Tool[slot].type!=optr->type)
      {
      sprintf(scratch,"\nUnable to perform that operation\nwith this kind of tool.\n");
      ok=0;
      }
    if(!ok)
      {
      aa_dialog_box(win_model,1,0,"Model Assignment Error",scratch);
      return;
      }
    else 
      {
      if(active_model==NULL)return;

      printf("\nSet_Operation: Add operation run - slot=%d\n",slot);
      // add the operation to the model's list of opers only if it is not already on it
      job.regen_flag=TRUE;
      Tool[slot].select_status=TRUE;
      mptr=job.model_first;						// ... start with first model
      if(set_assign_to_all==FALSE)mptr=active_model;			// ... if only the active model
      while(mptr!=NULL)							// ... loop thru all models
	{
	mptr->reslice_flag=TRUE;					// ... must re-slice if different operations selected
	gptr=mptr->oper_list;
	while(gptr!=NULL)
	  {
	  if(gptr->ID==slot && strstr(gptr->name,optr->name)!=NULL)break;
	  gptr=gptr->next;
	  }
	if(gptr==NULL)							// if no operations yet defined...
	  {
	  gptr=genericlist_make();					// ... create a new ops list
	  gptr->ID=slot;						// ... define which tool to use
	  sprintf(gptr->name,"%s",optr->name);				// ... define the operation by name
	  job.type=ADDITIVE;						// ... default job to additive
	  if(op_typ==OP_MILL_OUTLINE)job.type=SUBTRACTIVE;		// ... if op is milling
	  if(op_typ==OP_MILL_AREA)job.type=SUBTRACTIVE;			// ... if op is milling
	  if(op_typ==OP_MILL_PROFILE)job.type=SUBTRACTIVE;		// ... if op is milling
	  if(op_typ==OP_MILL_HOLES)job.type=SUBTRACTIVE;		// ... if op is milling
	  if(op_typ==OP_MARK_OUTLINE)job.type=MARKING;			// ... if op is marking
	  if(op_typ==OP_MARK_AREA)job.type=MARKING;			// ... if op is marking
	  if(op_typ==OP_MARK_IMAGE)job.type=MARKING;			// ... if op is marking
	  if(op_typ==OP_MARK_CUT)job.type=MARKING;			// ... if op is marking
	  if(op_typ==OP_MEASURE_HOLES)job.type=MEASUREMENT;		// ... if op is measuring
	  if(op_typ==OP_MEASURE_X)job.type=MEASUREMENT;			// ... if op is measuring
	  if(op_typ==OP_MEASURE_Y)job.type=MEASUREMENT;			// ... if op is measuring
	  if(op_typ==OP_MEASURE_Z)job.type=MEASUREMENT;			// ... if op is measuring
	  if(op_typ==OP_CURE)job.type=CURING;				// ... if op is curing
	  if(op_typ==OP_PLACE)job.type=PLACEMENT;			// ... if op is placement
	  if(op_typ==OP_SCAN)job.type=SCANNING;				// ... if op is scanning
	  if(mptr->oper_list==NULL)					// ... if first op for this model...
	    {mptr->oper_list=gptr;}					// ... define as such
	  else 								// ... otherwise if not the first op for this model
	    {genericlist_insert(mptr->oper_list,gptr);}			// ... add to the current list of ops for this model
	  }

	// at this point the tool (slot) that will be used on this model has been selected
	//lt_ptr=linetype_find(slot,MDL_OFFSET);				// ... get pointer to wall line type
	//if(lt_ptr!=NULL){mptr->set_wall_thk=lt_ptr->thickness;}		// ... if pointer exists, use its thickness
	//else {mptr->set_wall_thk=1.0;}					// ... otherwise set to no tool associated yet

	if(set_assign_to_all==FALSE)break;				// ... exit if only doing the active model
	mptr=mptr->next;
	}
  
      // de-activate checkboxes of other tools for this operation
      for(i=0;i<MAX_TOOLS;i++)
	{
	if(i==slot)continue;
	gtk_widget_set_sensitive(btn_op[op_typ][i],FALSE);
	}
      
      // update job information
      sprintf(scratch,"Undetermined");
      if(job.type==ADDITIVE)sprintf(scratch,"Additive");
      if(job.type==SUBTRACTIVE)sprintf(scratch,"Subtractive");
      if(job.type==MARKING)sprintf(scratch,"Marking");
      if(job.type==MEASUREMENT)sprintf(scratch,"Measuring");
      if(job.type==CURING)sprintf(scratch,"Curing");
      if(job.type==PLACEMENT)sprintf(scratch,"Placement");
      if(job.type==SCANNING)sprintf(scratch,"Scanning");
      gtk_label_set_text(GTK_LABEL(lbl_job_type),scratch);
      gtk_widget_queue_draw(lbl_job_type);
      
      job.min_slice_thk=job_find_min_thickness();
      sprintf(scratch,"%6.3f",job.min_slice_thk);
      gtk_label_set_text(GTK_LABEL(lbl_job_slice_thk),scratch);
      gtk_widget_queue_draw(lbl_job_slice_thk);
      }
    }
    
  // if the user just "unchecked" this operation...
  else 
    {
    printf("\nSet_Operation: Remove operation run\n");
      
    // find this operation in the model's list of opers and remove it
    if(active_model==NULL)return;
    mptr=job.model_first;
    if(set_assign_to_all==FALSE)mptr=active_model;			// if only doing the active model...
    while(mptr!=NULL)
      {
      mptr->reslice_flag=TRUE;						// must re-slice if different operations selected
      job.regen_flag=TRUE;
      Tool[slot].select_status=FALSE;
      gptr=mptr->oper_list;
      while(gptr!=NULL)
	{
	if(gptr->ID==slot && strstr(gptr->name,optr->name)!=NULL)break;
	gptr=gptr->next;
	}
      //printf("MUI: Removing %s from slot %d .\n",gptr->name,gptr->ID);
      if(gptr!=NULL)
	{
	if(gptr==mptr->oper_list)
	  {
	  mptr->oper_list=gptr->next;
	  free(gptr);
	  // reset job type to first operation remaining in job, if any
	  job.type=UNDEFINED;
	  gptr=mptr->oper_list;
	  if(gptr!=NULL)						// if at least one operation defined...
	    {
	    job.type=ADDITIVE;						// ... default job to additive
	    if(strstr(gptr->name,"SUB_MILL"))job.type=SUBTRACTIVE;	// ... unless first op is CNC milling
	    }
	  }
	else 
	  {
	  gdel=gptr;
	  gptr=mptr->oper_list;
	  while(gptr->next!=gdel)gptr=gptr->next;
	  gptr->next=gdel->next;
	  free(gdel);
	  }
	}
      
      if(set_assign_to_all==FALSE)break;				// if only one model, leave now
      mptr=mptr->next;
      }
    
    // finally adjust sensitivity of all checkboxes now that this element is gone
    for(i=0;i<MAX_TOOLS;i++)
      {
      if(Tool[i].state>=TL_LOADED){gtk_widget_set_sensitive(btn_op[op_typ][i],TRUE);}
      else {gtk_widget_set_sensitive(btn_op[op_typ][i],FALSE);}
      }

    // update job information
    sprintf(scratch,"Undetermined");
    if(job.type==ADDITIVE)sprintf(scratch,"Additive");
    if(job.type==SUBTRACTIVE)sprintf(scratch,"Subtractive");
    if(job.type==MARKING)sprintf(scratch,"Marking");
    gtk_label_set_text(GTK_LABEL(lbl_job_type),scratch);
    gtk_widget_queue_draw(lbl_job_type);
    
    job.min_slice_thk=job_find_min_thickness();
    sprintf(scratch,"%6.3f",job.min_slice_thk);
    if(job.min_slice_thk>=BUILD_TABLE_MAX_Z)sprintf(scratch,"Undetermined");
    gtk_label_set_text(GTK_LABEL(lbl_job_slice_thk),scratch);
    gtk_widget_queue_draw(lbl_job_slice_thk);
    }

  // display debug information
  if(active_model!=NULL)
    {
    printf("\n\nActive Model Ops List:  Call from slot=%d op_typ=%d\n",slot,op_typ);
    gptr=NULL;
    if(active_model!=NULL)gptr=active_model->oper_list;
    while(gptr!=NULL)
      {
      printf(" %d  %s \n",gptr->ID,gptr->name);
      gptr=gptr->next;
      }
    }

  return;
}


void drill_hole_value(GtkSpinButton *button, gpointer user_data)
{
  int	i;
  
  i=GPOINTER_TO_INT(user_data);	
  Tool[i].max_drill_diam=gtk_spin_button_get_value(button);
  
  return;
}

// Callback to get number of copies for this model
void get_model_copies(GtkSpinButton *button, gpointer user_data)
{
  int		old_copy_cnt;
  model		*mptr;
  
  if(active_model==NULL)return;
  mptr=active_model;
  old_copy_cnt=mptr->total_copies;
  mptr->total_copies=gtk_spin_button_get_value(button);
  new_copy_cnt_flag=FALSE;
  if(old_copy_cnt!=mptr->total_copies)new_copy_cnt_flag=TRUE;
  
  return;
}

// autplacement callback
void autoplacement_toggled_cb(GtkWidget *btn, gpointer user_data)
{

  autoplacement_flag++;
  if(autoplacement_flag>1)autoplacement_flag=0;
  if(autoplacement_flag==TRUE)job_maxmin();
  
  return;
}

// center on table callback
void center_toggled_cb(GtkWidget *btn, gpointer user_data)
{

  set_center_build_job++;
  if(set_center_build_job>1)set_center_build_job=0;
  job_maxmin();
  
  return;
}

// vase mode callback
void vase_mode_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int		slot;
  slice		*sptr;

  if(active_model!=NULL)
    {
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // check if any tools have slice data for this model, if so purge it
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
      }
    active_model->reslice_flag=TRUE;
      
    // set vase mode accordingly
    active_model->vase_mode++;
    if(active_model->vase_mode>1)active_model->vase_mode=0;
    }
  
  return;
}

// lower close-off for vase mode callback
void btm_co_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	slot;
  
  if(active_model!=NULL)
    {
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // check if any tools have slice data for this model, if so purge it
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
      }
    active_model->reslice_flag=TRUE;

    active_model->btm_co++;
    if(active_model->btm_co>1)active_model->btm_co=0;
    }
  
  return;
}

// upper close-off for vase mode callback
void top_co_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int	slot;
  
  if(active_model!=NULL)
    {
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // check if any tools have slice data for this model, if so purge it
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
      }
    active_model->reslice_flag=TRUE;

    active_model->top_co++;
    if(active_model->top_co>1)active_model->top_co=0;
    }
  
  return;
}

// shallow angle supports for vase mode callback
void internal_spt_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  int 	slot;
  
  if(active_model!=NULL)
    {
    if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
    
    // check if any tools have slice data for this model, if so purge it
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
      }
    active_model->reslice_flag=TRUE;

    active_model->internal_spt++;
    if(active_model->internal_spt>1)active_model->internal_spt=0;
    }
  
  return;
}

// Callback to get wall thickness over-ride
void wall_thk_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot;
  linetype	*lt_ptr;
  
  if(active_model==NULL)return;
  
  slot=model_get_slot(active_model,OP_ADD_MODEL_MATERIAL);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
  
  // check if any tools have slice data for this model, if so purge it
  for(slot=0;slot<MAX_TOOLS;slot++)
    {
    if(active_model->slice_first[slot]!=NULL){model_purge(active_model);break;}
    }
  active_model->reslice_flag=TRUE;
  active_model->set_wall_thk=gtk_spin_button_get_value(button);
  
  // subtract off border wall width to get new offset wall width
  lt_ptr=linetype_find(slot,MDL_BORDER);
  if(lt_ptr!=NULL)
    {
    active_model->set_wall_thk -= lt_ptr->wall_width;
    if(active_model->set_wall_thk < 0)active_model->set_wall_thk=0.0;
    }

  // now assign new wall width to offset line type
  //lt_ptr=linetype_find(slot,MDL_OFFSET);
  //if(lt_ptr!=NULL)
  //  {
  //  printf("new wall thickness assigned to line type.\n");
  //  lt_ptr->wall_width = active_model->set_wall_thk;
  //  }
  
  return;
}

// Callback to get fill density over-ride
void fill_density_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot,i;
  linetype	*lt_ptr;
  
  if(active_model==NULL)return;
  
  slot=model_get_slot(active_model,OP_ADD_MODEL_MATERIAL);
  if(slot<0 || slot>=MAX_TOOLS)return;
  if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
  
  // check if any tools have slice data for this model, if so purge it
  for(i=0;i<MAX_TOOLS;i++)
    {
    if(active_model->slice_first[i]!=NULL){model_purge(active_model);break;}
    }
  active_model->reslice_flag=TRUE;
  lt_ptr=linetype_find(slot,MDL_FILL);
  if(lt_ptr==NULL)return;
  
  active_model->set_fill_density=gtk_spin_button_get_value(button)/100;
  if(active_model->set_fill_density<=0)active_model->set_fill_density=0.001;
  lt_ptr->wall_pitch=lt_ptr->wall_width / active_model->set_fill_density;
  
  return;
}

// fidelity mode callback
void fidelity_mode_toggled_cb(GtkWidget *btn, gpointer user_data)
{

  if(active_model==NULL)return;
  if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
  
  // if sliced, purge slice data
  if(active_model->reslice_flag==FALSE)model_purge(active_model);

  // set the flag
  active_model->fidelity_mode++;
  if(active_model->fidelity_mode>1)active_model->fidelity_mode=0;
  
  // adjust global settings based on state
  if(active_model->fidelity_mode==FALSE)min_vector_length=0.250;
  if(active_model->fidelity_mode==TRUE)min_vector_length=0.050;
  
  
  return;
}

// image invert callback
void img_invert_toggled_cb(GtkWidget *btn, gpointer user_data)
{

  if(active_model==NULL)return;
  if(active_model->reslice_flag==FALSE){if(on_job_will_need_regen()==FALSE)return;}
  
  // if sliced, purge slice data
  if(active_model->reslice_flag==FALSE)model_purge(active_model);

  // set the flag
  active_model->image_invert++;
  if(active_model->image_invert>1)active_model->image_invert=0;
  
  return;
}

// assign tools to all models callback
void do_all_btn_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  slice		*sptr;

  if(active_model==NULL)return;
  
  set_assign_to_all++;
  if(set_assign_to_all>1)set_assign_to_all=0;
  
  sprintf(scratch," ALL LOADED MODELS ");
  if(set_assign_to_all==FALSE)						// change lable from "ALL LOADED MODELS" back to file name...
    {
    if(strlen(active_model->model_file)>50)				// if it is a long name...
      {
      sprintf(scratch,"...");						// ... add the dots
      strcat(scratch,&active_model->model_file[strlen(active_model->model_file)-50]);	// ... add the last part of name
      }
    else 								// else if it is a short name...
      {sprintf(scratch,"%s",active_model->model_file);}			// ... just copy it over
    }
    
  gtk_label_set_text(GTK_LABEL(lbl_model_name),scratch);
  gtk_widget_queue_draw(lbl_model_name);
  
  
  return;
}


// Destroy callback for build options window
void build_options_done_callback (GtkWidget *btn, gpointer dead_window)
{

  // can't just do math (i.e. ratio of new|old copy count) because the job may have different models and
  // because the graphs for time/matl get down to line type times.  Must just re-run job time simulator.
  if(new_copy_cnt_flag==TRUE)
    {
    job_maxmin();							// assign x & y copies based on new total copy value
    job_time_estimate_simulator();					// run with new copy values
    new_copy_cnt_flag=FALSE;						// clear flag
    }

  gtk_window_close(GTK_WINDOW(dead_window));
  if(win_modelUI_flag==TRUE)gtk_widget_queue_draw(GTK_WIDGET(win_model));
  display_model_flag=TRUE;
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}

// build options anytime callback - these options can be set before or after slicing/tool selection
void build_options_anytime_cb(GtkWidget *btn, gpointer src_window)
  {
  GtkWidget	*win_bo, *btn_exit, *img_exit, *lbl_new, *win_local_dialog;
  GtkWidget	*hbox, *hbox_up, *hbox_dn, *vbox;
  GtkWidget	*copies_frame, *grd_copies;
  GtkWidget	*lbl_mdl_copies, *autoplacement_btn, *center_btn;
  
  
  int		slot;
  float 	fval;
  linetype	*lt_ptr;

  // first make sure we have a model to work with
  if(active_model==NULL)
    {
    sprintf(scratch," \nBuild options are model specific.\n");
    strcat(scratch,"Open a model first, then set options.");
    aa_dialog_box(win_model,1,0,"Model Assignment Error",scratch);
    return;
    }

  // turn off model display to speed up response
  //display_model_flag=FALSE;
  
  // now make sure that model is associated with a specific tool
  slot=(-1);
  slot=model_get_slot(active_model,OP_ADD_MODEL_MATERIAL);
  
  // set up primary window
  win_bo = gtk_window_new ();				
  gtk_window_set_transient_for (GTK_WINDOW(win_bo), GTK_WINDOW(src_window));
  gtk_window_set_modal (GTK_WINDOW(win_bo),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_bo),300,200);
  gtk_window_set_resizable(GTK_WINDOW(win_bo),FALSE);			
  sprintf(scratch,"Build Options");
  gtk_window_set_title(GTK_WINDOW(win_bo),scratch);			

  // set up an hbox to divide the screen into a narrower segments
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
  //gtk_box_set_spacing (GTK_BOX(hbox),20);
  gtk_window_set_child (GTK_WINDOW (win_bo), hbox);

  // now divide left side into segments for buttons
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
  gtk_box_append(GTK_BOX(hbox),vbox);
    
  // and further divide the right side into quads - two "up" and two "dn"
  hbox_up=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_up);
  hbox_dn=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_dn);

  // set up EXIT button
  btn_exit = gtk_button_new ();
  g_signal_connect (btn_exit, "clicked", G_CALLBACK (build_options_done_callback), win_bo);
  img_exit = gtk_image_new_from_file("Back.gif");
  gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
  gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
  gtk_box_append(GTK_BOX(vbox),btn_exit);

  
  // set up copies button and autoplace checkbox
  {
    copies_frame = gtk_frame_new("Copies & Placement");
    GtkWidget *copies_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(copies_frame), copies_box);
    gtk_widget_set_size_request(copies_frame,350,100);
    gtk_box_append(GTK_BOX(hbox_up),copies_frame);

    grd_copies = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(copies_box),grd_copies);
    gtk_grid_set_row_spacing (GTK_GRID(grd_copies),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_copies),15);
  
    fval=active_model->total_copies;
    mdl_copies_adj=gtk_adjustment_new(fval, 0, 99, 1, 1, 10);
    mdl_copies_btn=gtk_spin_button_new(mdl_copies_adj, 3, 0);
    lbl_mdl_copies=gtk_label_new("Number of copies:");
    gtk_grid_attach (GTK_GRID (grd_copies), lbl_mdl_copies, 0, 0, 1, 1);
    gtk_grid_attach (GTK_GRID (grd_copies), mdl_copies_btn, 1, 0, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(mdl_copies_btn),"value-changed",G_CALLBACK(get_model_copies),NULL);
    
    autoplacement_btn=gtk_check_button_new_with_label ("AutoPlacement ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(autoplacement_btn), autoplacement_flag);
    g_signal_connect (GTK_CHECK_BUTTON (autoplacement_btn), "toggled", G_CALLBACK (autoplacement_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_copies),autoplacement_btn, 0, 1, 1, 1);
    
    center_btn=gtk_check_button_new_with_label ("Center on Table ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(center_btn), set_center_build_job);
    g_signal_connect (GTK_CHECK_BUTTON (center_btn), "toggled", G_CALLBACK (center_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_copies),center_btn, 1, 1, 1, 1);
  }

  gtk_widget_set_visible(win_bo,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}

// build options preslice callback - these options must be set before slicing and after tool selection
void build_options_preslice_cb(GtkWidget *btn, gpointer src_window)
  {
  GtkWidget	*win_bo, *btn_exit, *img_exit, *lbl_new, *win_local_dialog;
  GtkWidget	*hbox, *hbox_up, *hbox_dn, *vbox;
  GtkWidget	*vase_mode_frame, *grd_vase_mode;
  GtkWidget 	*vase_mode_btn, *btm_co_btn, *top_co_btn, *internal_spt_btn;
  GtkWidget	*lbl_wall_thk, *wall_thk_btn,*lbl_fill_density,*fill_density_btn;
  GtkAdjustment	*wall_thk_adj,*fill_density_adj;
  GtkWidget	*fidelity_mode_frame, *grd_fidelity_mode;
  GtkWidget 	*fidelity_mode_btn,*img_invert_btn;
  int		slot;
  float 	fval,min_val;
  linetype	*lt_ptr_off,*lt_ptr_bor,*lt_ptr_fill;

  // first make sure we have a model to work with
  if(active_model==NULL)
    {
    sprintf(scratch," \nBuild options are model specific.\n");
    strcat(scratch,"Open a model first, then set options.");
    aa_dialog_box(win_model,1,0,"Model Assignment Error",scratch);
    return;
    }

  // we will need tool/mat'l information to set up these options
  // so make sure that model is associated with a specific tool
  slot=model_get_slot(active_model,OP_ADD_MODEL_MATERIAL);
  if(slot<0)slot=model_get_slot(active_model,OP_ADD_SUPPORT_MATERIAL);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_OUTLINE);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_AREA);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_IMAGE);
  if(slot<0)slot=model_get_slot(active_model,OP_MARK_CUT);
  if(slot<0)slot=model_get_slot(active_model,OP_MILL_OUTLINE);
  if(slot<0)slot=model_get_slot(active_model,OP_MILL_AREA);
  if(slot<0)slot=model_get_slot(active_model,OP_MILL_HOLES);
  if(slot<0)slot=model_get_slot(active_model,OP_MILL_PROFILE);
  if(slot<0 || slot>=MAX_TOOLS)
    {
    sprintf(scratch," \nThe model needs to be associated with a tool.\n");
    strcat(scratch,"Check off which tool will operate on this model\n");
    strcat(scratch,"first, then select Build Options.");
    aa_dialog_box(win_model,1,0,"Model Assignment Error",scratch);
    return;
    }

  // turn off model display to speed up response
  display_model_flag=FALSE;
  
  // set up primary window
  win_bo = gtk_window_new();				
  gtk_window_set_transient_for (GTK_WINDOW(win_bo), GTK_WINDOW(src_window));
  gtk_window_set_modal (GTK_WINDOW(win_bo),TRUE);
  gtk_window_set_default_size(GTK_WINDOW(win_bo),300,250);
  gtk_window_set_resizable(GTK_WINDOW(win_bo),FALSE);			
  sprintf(scratch,"Pre-Slice Build Options");
  gtk_window_set_title(GTK_WINDOW(win_bo),scratch);			

  // set up an hbox to divide the screen into a narrower segments
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
  //gtk_box_set_spacing (GTK_BOX(hbox),20);
  gtk_window_set_child (GTK_WINDOW (win_bo), hbox);

  // now divide left side into segments for buttons
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
  gtk_box_append(GTK_BOX(hbox),vbox);
    
  // and further divide the right side into quads - two "up" and two "dn"
  hbox_up=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_up);
  hbox_dn=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
  gtk_box_append(GTK_BOX(hbox),hbox_dn);

  // set up EXIT button
  btn_exit = gtk_button_new ();
  g_signal_connect (btn_exit, "clicked", G_CALLBACK (build_options_done_callback), win_bo);
  img_exit = gtk_image_new_from_file("Back.gif");
  gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
  gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
  gtk_box_append(GTK_BOX(vbox),btn_exit);
  
  // set up perimeter and in fill options
  {
    vase_mode_frame = gtk_frame_new("Internal Perim and Fill Options");
    GtkWidget *vase_mode_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(vase_mode_frame), vase_mode_box);
    gtk_widget_set_size_request(vase_mode_frame,350,150);
    gtk_box_append(GTK_BOX(hbox_up),vase_mode_frame);

    grd_vase_mode = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(vase_mode_box),grd_vase_mode);
    gtk_grid_set_row_spacing (GTK_GRID(grd_vase_mode),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_vase_mode),15);
  
    vase_mode_btn=gtk_check_button_new_with_label ("Enable Vase Mode ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(vase_mode_btn), active_model->vase_mode);
    g_signal_connect (GTK_CHECK_BUTTON (vase_mode_btn), "toggled", G_CALLBACK (vase_mode_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_vase_mode),vase_mode_btn, 0, 1, 1, 1);
    
    internal_spt_btn=gtk_check_button_new_with_label ("Internal support ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(internal_spt_btn), active_model->internal_spt);
    g_signal_connect (GTK_CHECK_BUTTON (internal_spt_btn), "toggled", G_CALLBACK (internal_spt_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_vase_mode),internal_spt_btn, 1, 1, 1, 1);

    btm_co_btn=gtk_check_button_new_with_label ("Bottom close-off ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(btm_co_btn), active_model->btm_co);
    g_signal_connect (GTK_CHECK_BUTTON (btm_co_btn), "toggled", G_CALLBACK (btm_co_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_vase_mode),btm_co_btn, 0, 2, 1, 1);
    
    top_co_btn=gtk_check_button_new_with_label ("Top close-off ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(top_co_btn), active_model->top_co);
    g_signal_connect (GTK_CHECK_BUTTON (top_co_btn), "toggled", G_CALLBACK (top_co_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_vase_mode),top_co_btn, 1, 2, 1, 1);
    
    // set up wall thickness over-ride spin box
    lt_ptr_bor=linetype_find(slot,MDL_BORDER);				// ... get pointer to border wall line type
    lt_ptr_off=linetype_find(slot,MDL_OFFSET);				// ... get pointer to offset wall line type
    active_model->set_wall_thk=0.0;						// init to nothing
    if(lt_ptr_bor!=NULL)active_model->set_wall_thk += lt_ptr_bor->wall_width;	// if border in use, add it in
    if(lt_ptr_off!=NULL)active_model->set_wall_thk += lt_ptr_off->wall_width;	// if offset in use, add it in
    min_val=0.0;
    if(lt_ptr_bor!=NULL){min_val=lt_ptr_bor->wall_width;}			// don't allow going lower than border if it is in use
    lbl_wall_thk=gtk_label_new("Perim thickness:");
    gtk_grid_attach (GTK_GRID (grd_vase_mode), lbl_wall_thk, 0, 3, 1, 1);
    wall_thk_adj=gtk_adjustment_new(active_model->set_wall_thk, min_val, 10, 0.1, 1, 2);
    wall_thk_btn=gtk_spin_button_new(wall_thk_adj, 4, 1);
    gtk_grid_attach (GTK_GRID (grd_vase_mode), wall_thk_btn, 1, 3, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(wall_thk_btn),"value-changed",G_CALLBACK(wall_thk_value),NULL);

    // set up interior fill desnity over-ride spin box
    // fill density is the ratio of line_width / line_pitch where line_width is the priority value
    lt_ptr_fill=linetype_find(slot,MDL_FILL);				// ... get pointer to wall line type
    if(lt_ptr_fill!=NULL){active_model->set_fill_density=(lt_ptr_fill->wall_width/lt_ptr_fill->wall_pitch);}	 // ... if pointer exists, use to calc density
    else {active_model->set_fill_density=0.5;}				// ... otherwise set to no tool associated yet
    lbl_fill_density=gtk_label_new("Fill density %:");
    gtk_grid_attach (GTK_GRID (grd_vase_mode), lbl_fill_density, 0, 4, 1, 1);
    fill_density_adj=gtk_adjustment_new((active_model->set_fill_density*100), 0, 125, 1, 1, 2);
    fill_density_btn=gtk_spin_button_new(fill_density_adj, 3, 0);
    gtk_grid_attach (GTK_GRID (grd_vase_mode), fill_density_btn, 1, 4, 1, 1);
    g_signal_connect(GTK_SPIN_BUTTON(fill_density_btn),"value-changed",G_CALLBACK(fill_density_value),NULL);

  }


  // set up high fidelity mode params
  {
    fidelity_mode_frame = gtk_frame_new("Miscellaneous");
    GtkWidget *fidelity_mode_box=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_frame_set_child(GTK_FRAME(fidelity_mode_frame), fidelity_mode_box);
    gtk_widget_set_size_request(fidelity_mode_frame,350,100);
    gtk_box_append(GTK_BOX(hbox_up),fidelity_mode_frame);

    grd_fidelity_mode = gtk_grid_new ();						
    gtk_box_append(GTK_BOX(fidelity_mode_box),grd_fidelity_mode);
    gtk_grid_set_row_spacing (GTK_GRID(grd_fidelity_mode),15);
    gtk_grid_set_column_spacing (GTK_GRID(grd_fidelity_mode),15);
  
    fidelity_mode_btn=gtk_check_button_new_with_label ("Enable fidelity mode ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(fidelity_mode_btn), active_model->fidelity_mode);
    g_signal_connect (GTK_CHECK_BUTTON (fidelity_mode_btn), "toggled", G_CALLBACK (fidelity_mode_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_fidelity_mode),fidelity_mode_btn, 0, 1, 1, 1);
  }
  
  // set up image options
  if(active_model->g_img_act_buff!=NULL)
    {
    img_invert_btn=gtk_check_button_new_with_label ("Invert image colors ");
    gtk_check_button_set_active(GTK_CHECK_BUTTON(img_invert_btn), active_model->image_invert);
    g_signal_connect (GTK_CHECK_BUTTON (img_invert_btn), "toggled", G_CALLBACK (img_invert_toggled_cb), NULL);
    gtk_grid_attach(GTK_GRID (grd_fidelity_mode),img_invert_btn, 0, 2, 1, 1);
    }

  gtk_widget_set_visible(win_bo,TRUE);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}


// MODEL UI
int model_UI(GtkWidget *btn_call, gpointer user_data)
{
    GtkWidget		*win_local_dialog, *hbox, *vboxleft, *vboxright, *h_separator;
    GtkWidget		*jobbox,*frame_job_area,*grid_job_area;
    GtkWidget		*filebox, *frame_file_area, *grid_file_area;
    GtkWidget 		*orientbox, *frame_orient_area, *grid_orient_area;
    GtkWidget		*operbox, *frame_oper_area, *grid_oper_upper, *grid_oper_lower;

    GtkWidget		*build_options_btn, *do_all_btn;
    GtkWidget		*lbl_op_top,*lbl_op_mid,*lbl_op_btm;
    GtkWidget		*oper_win, *op_upper_box, *op_lower_box;
    GtkAdjustment	*oper_win_vert;

    GtkWidget		*btn_view, *btn_model, *btn_print, *img_exit, *btn_exit;
    GtkWidget		*btn_test,*img_test,*btn_chng,*img_chng;
    GtkWidget		*btn_file,*btn_clear;
    GtkWidget		*lbl_info;
    GtkAdjustment	*drill_hole_adj;
    GtkWidget		*drill_hole_btn,*btn_base_layer;
    
    char 		mdl_name[255];
    int			win_width,win_height;
    int			h,i,op_typ,slot,all_ok,col_width=35,vrow;
    int			ival,mtyp=MODEL;
    float 		fval;
    model		*mptr;
    genericlist		*aptr;
    operation		*optr;

    idle_start_time=time(NULL);						// reset idle time
    //memory_status();
    
    if(job.state==JOB_RUNNING)
      {
      sprintf(scratch," \nChanging assignments during a job\nis forbidden.  Abort the job first.\n\n");
      aa_dialog_box(win_main,1,0,"Model Assignment Error",scratch);
      return(0);
      }

    if(job.model_first==NULL)
      {
      sprintf(scratch," \nA model must be loaded before\ntool operations can be assigned to it.\n\n");
      aa_dialog_box(win_main,1,0,"Model Assignment Error",scratch);
      return(0);
      }
      
    if(active_model==NULL)
      {
      active_model=job.model_first;
      }
 
    all_ok=0;
    for(slot=0;slot<MAX_TOOLS;slot++)if(Tool[slot].state>=TL_READY)all_ok=1;
    if(!all_ok)
      {
      sprintf(scratch," \nA tool must be loaded\nbefore assigning to models.\n\n");
      aa_dialog_box(win_main,1,0,"Model Assignment Error",scratch);
      return(0);
      }
   
    //if(job.model_first!=NULL)active_model=job.model_first;
    win_modelUI_flag=1;
    display_model_flag=FALSE;   
    win_width=LCD_WIDTH-150;
    win_height=LCD_HEIGHT-50;
      
    
    // set up model interface window
    {
    win_model = gtk_window_new();	
    gtk_window_set_transient_for (GTK_WINDOW(win_model), GTK_WINDOW(win_main));	
    gtk_window_set_default_size(GTK_WINDOW(win_model),win_width,win_height);
    gtk_window_set_resizable(GTK_WINDOW(win_model),FALSE);			
    sprintf(scratch,"Model Control");
    gtk_window_set_title(GTK_WINDOW(win_model),scratch);			
    
    // set up an hbox to divide the screen into a narrower segments
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    //gtk_box_set_spacing (GTK_BOX(hbox),20);
    gtk_window_set_child(GTK_WINDOW(win_model),hbox);

    // now divide left side into segments for buttons
    vboxleft = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vboxleft);

    // set up EXIT button
    btn_exit = gtk_button_new ();
    g_signal_connect (btn_exit, "clicked", G_CALLBACK (on_model_exit), win_model);
    img_exit = gtk_image_new_from_file("Back.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
    gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
    gtk_box_append(GTK_BOX(vboxleft),btn_exit);

    // set up vboxright and divide it down to file select, orientation, and operations boxes
    vboxright = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vboxright);
    jobbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),jobbox);
    filebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),filebox);
    orientbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),orientbox);
    operbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,5);
    gtk_box_append(GTK_BOX(vboxright),operbox);
    }

    // set up job information area
    {
      sprintf(scratch,"Job");
      frame_job_area=gtk_frame_new(scratch);
      GtkWidget *box_job_area=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
      gtk_frame_set_child(GTK_FRAME(frame_job_area), box_job_area);
      gtk_widget_set_size_request(frame_job_area,win_width,85);
      gtk_box_append(GTK_BOX(jobbox),frame_job_area);
      
      grid_job_area=gtk_grid_new();
      gtk_box_append(GTK_BOX(box_job_area),grid_job_area);
      gtk_grid_set_row_spacing (GTK_GRID(grid_job_area),10);
      gtk_grid_set_column_spacing (GTK_GRID(grid_job_area),10);
      
      lbl_info=gtk_label_new(" Type:");
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_info, 0, 0, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"Undetermined");
      if(job.type==ADDITIVE)sprintf(scratch,"Additive");
      if(job.type==SUBTRACTIVE)sprintf(scratch,"Subtractive");
      if(job.type==MARKING)sprintf(scratch,"Marking");
      lbl_job_type=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_job_type, 1, 0, 2, 1);
      
      lbl_info=gtk_label_new(" Qty Models Loaded: ");
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_info, 0, 1, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%3d",job.model_count);
      lbl_job_mdl_qty=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_job_mdl_qty, 1, 1, 2, 1);

      lbl_info=gtk_label_new(" Min Slice Thickness:");
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_info, 0, 2, 1, 1);
      gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
      sprintf(scratch,"%6.3f",job.min_slice_thk);
      if(job.min_slice_thk>=BUILD_TABLE_MAX_Z)sprintf(scratch,"Undetermined");
      lbl_job_slice_thk=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_job_area), lbl_job_slice_thk, 1, 2, 2, 1);

    }
    
    // set up file select area
    {
      sprintf(scratch,"Active Model");
      frame_file_area=gtk_frame_new(scratch);
      GtkWidget *box_file_area=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
      gtk_frame_set_child(GTK_FRAME(frame_file_area), box_file_area);
      gtk_widget_set_size_request(frame_file_area,win_width,55);
      gtk_box_append(GTK_BOX(filebox),frame_file_area);
      
      grid_file_area=gtk_grid_new();
      gtk_box_append(GTK_BOX(box_file_area),grid_file_area);
      gtk_grid_set_row_spacing (GTK_GRID(grid_file_area),12);
      gtk_grid_set_column_spacing (GTK_GRID(grid_file_area),10);

      col_width=25;

      // add apply to all checkbox
      do_all_btn=gtk_check_button_new_with_label ("Apply to All");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(do_all_btn), set_assign_to_all);
      g_signal_connect (GTK_CHECK_BUTTON (do_all_btn), "toggled", G_CALLBACK (do_all_btn_toggled_cb), NULL);
      gtk_grid_attach(GTK_GRID (grid_file_area),do_all_btn, 0, 1, 1, 1);
      //g_object_set (do_all_btn, "margin-left", 20, NULL);
	
      // if model in error, post label in first grid location
      sprintf(scratch,"     ");
      if(active_model!=NULL)
        {
	if(active_model->error_status!=0){sprintf(scratch,"   Flawed"); col_width=18;}
	}
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_mdl_error=gtk_label_new (scratch);
      markup = g_markup_printf_escaped (error_fmt,scratch);
      gtk_label_set_markup(GTK_LABEL(lbl_mdl_error),markup);
      gtk_grid_attach(GTK_GRID(grid_file_area),lbl_mdl_error,0,2,1,1);
      gtk_label_set_xalign(GTK_LABEL(lbl_mdl_error),0.5);

      // pad remaining grid 0 column to provide consistent spacing for other elements
      sprintf(scratch,"    ");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      for(i=1;i<6;i++)
        {
        lbl_info = gtk_label_new (scratch);
        gtk_grid_attach (GTK_GRID (grid_file_area), lbl_info, i, 1, 1, 1);
        }
	
      // display name of currently selected model or all models
      sprintf(scratch," ALL LOADED MODELS ");
      if(set_assign_to_all==FALSE)
        {
	if(strlen(active_model->model_file)>50)				// if it is a long name...
	  {
	  sprintf(scratch,"...");					// ... add the dots
	  strcat(scratch,&active_model->model_file[strlen(active_model->model_file)-50]);	// ... add the last part of name
	  }
	else 								// else if it is a short name...
	  {sprintf(scratch,"%s",active_model->model_file);}		// ... just copy it over
	}
      lbl_model_name=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_file_area), lbl_model_name, 1, 1, 5, 1);
      gtk_label_set_xalign(GTK_LABEL(lbl_mdl_error),0.0);

      // add button for tool specific options
      build_options_btn = gtk_button_new_with_label ("Build Options");
      g_signal_connect (build_options_btn, "clicked", G_CALLBACK (build_options_preslice_cb), win_model);
      gtk_grid_attach(GTK_GRID (grid_file_area),build_options_btn, 7, 1, 1, 1);

      /*
      // set up drop down menu
      combo_mdls = gtk_combo_box_text_new();
      i=0;
      mptr=job.model_first;					
      while(mptr!=NULL)
	{
	printf("\n%d %s\n\n",strlen(mptr->model_file),mptr->model_file);
	sprintf(scratch,"%s",mptr->model_file);
	if(strlen(scratch)>70)
	  {
	  strcpy(mdl_name,&scratch[strlen(scratch)-70]);
	  sprintf(scratch,"...");
	  strcat(scratch,mdl_name);
	  }
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_mdls), NULL, scratch);
	i++;
	mptr=mptr->next;
	}
      gtk_combo_box_set_active(GTK_COMBO_BOX(combo_mdls), 0);    
      gtk_grid_attach (GTK_GRID (grid_file_area), combo_mdls, 1, 1, 5, 1);
      g_signal_connect (combo_mdls, "changed", G_CALLBACK (model_select), NULL);
      
      */
       
    }
      
    // set up orientation box and grid axis labels
    {
      col_width=23;
      sprintf(scratch,"Oreintation");
      frame_orient_area=gtk_frame_new(scratch);
      GtkWidget *box_orient_area=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
      gtk_frame_set_child(GTK_FRAME(frame_orient_area), box_orient_area);
      gtk_widget_set_size_request(frame_orient_area,win_width,188);
      gtk_box_append(GTK_BOX(orientbox),frame_orient_area);

      grid_orient_area=gtk_grid_new();
      gtk_box_append(GTK_BOX(box_orient_area),grid_orient_area);
      gtk_grid_set_row_spacing (GTK_GRID(grid_orient_area),12);
      gtk_grid_set_column_spacing (GTK_GRID(grid_orient_area),10);
      
      sprintf(scratch,"      X:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 0, 2, 1, 1);
      sprintf(scratch,"      Y:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 0, 3, 1, 1);
      sprintf(scratch,"      Z:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 0, 4, 1, 1);
      sprintf(scratch,"    ");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      //lbl_info = gtk_label_new (scratch);
      //gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 0, 5, 1, 1);

      // set up button separator
      h_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
      gtk_grid_attach (GTK_GRID (grid_orient_area), h_separator, 0, 5, 6, 1);
    }
  
    // set up ROTATION input
    {
      sprintf(scratch,"Rotation:");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 2, 1, 1, 1);
      gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->xrot[mtyp]*180/PI;
      xrotadj=gtk_adjustment_new(fval, -360, 360, 45, 90, 180);
      xrotbtn=gtk_spin_button_new(xrotadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),xrotbtn, 2, 2, 1, 1);
      g_signal_connect(xrotbtn,"value-changed",G_CALLBACK(grab_xrotate_value),NULL);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->yrot[mtyp]*180/PI;
      yrotadj=gtk_adjustment_new(fval, -360, 360, 45, 90, 180);
      yrotbtn=gtk_spin_button_new(yrotadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),yrotbtn, 2, 3, 1, 1);
      g_signal_connect(yrotbtn,"value-changed",G_CALLBACK(grab_yrotate_value),NULL);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->zrot[mtyp]*180/PI;
      zrotadj=gtk_adjustment_new(fval, -360, 360, 45, 90, 180);
      zrotbtn=gtk_spin_button_new(zrotadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),zrotbtn, 2, 4, 1, 1);
      g_signal_connect(zrotbtn,"value-changed",G_CALLBACK(grab_zrotate_value),NULL);
    }
   
    // set up POSITION input
    {
      sprintf(scratch,"Position:");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 3, 1, 1, 1);
      gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->xoff[mtyp];
      xposadj=gtk_adjustment_new(fval, (0-BUILD_TABLE_LEN_X), BUILD_TABLE_LEN_X, 1, 15, 90);
      xposbtn=gtk_spin_button_new(xposadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),xposbtn, 3, 2, 1, 1);
      g_signal_connect(xposbtn,"value-changed",G_CALLBACK(grab_xpos_value),NULL);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->yoff[mtyp];
      yposadj=gtk_adjustment_new(fval, (0-BUILD_TABLE_LEN_Y), BUILD_TABLE_LEN_Y, 1, 15, 90);
      yposbtn=gtk_spin_button_new(yposadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),yposbtn, 3, 3, 1, 1);
      g_signal_connect(yposbtn,"value-changed",G_CALLBACK(grab_ypos_value),NULL);
      fval=0.0;
      if(active_model!=NULL)fval=active_model->zoff[mtyp];
      zposadj=gtk_adjustment_new(fval, (0-BUILD_TABLE_LEN_Z), BUILD_TABLE_LEN_Z, 1, 15, 90);
      zposbtn=gtk_spin_button_new(zposadj, 5, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),zposbtn, 3, 4, 1, 1);
      g_signal_connect(zposbtn,"value-changed",G_CALLBACK(grab_zpos_value),NULL);
    }
  
    // set up SCALE input
    {
      sprintf(scratch,"Scale:");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 4, 1, 1, 1);
      gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);
      fval=1.0;
      if(active_model!=NULL)fval=active_model->xscl[mtyp];
      xscladj=gtk_adjustment_new(fval, -100, 100, 0.1, 1, 10);
      xsclbtn=gtk_spin_button_new(xscladj, 6, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),xsclbtn, 4, 2, 1, 1);
      g_signal_connect(xsclbtn,"value-changed",G_CALLBACK(grab_xscl_value),NULL);
      fval=1.0;
      if(active_model!=NULL)fval=active_model->yscl[mtyp];
      yscladj=gtk_adjustment_new(fval, -100, 100, 0.1, 1, 10);
      ysclbtn=gtk_spin_button_new(yscladj, 6, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),ysclbtn, 4, 3, 1, 1);
      g_signal_connect(ysclbtn,"value-changed",G_CALLBACK(grab_yscl_value),NULL);
      fval=1.0;
      if(active_model!=NULL)fval=active_model->zscl[mtyp];
      zscladj=gtk_adjustment_new(fval, -100, 100, 0.1, 1, 10);
      zsclbtn=gtk_spin_button_new(zscladj, 6, 3);
      gtk_grid_attach(GTK_GRID(grid_orient_area),zsclbtn, 4, 4, 1, 1);
      g_signal_connect(zsclbtn,"value-changed",G_CALLBACK(grab_zscl_value),NULL);
    }
  
    // set up MIRROR check boxes
    {
      sprintf(scratch,"  Mirror:  ");
      //while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 5, 1, 1, 1);
      gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);
      
      xmirbtn=gtk_check_button_new_with_label (" ");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(xmirbtn), 0);
      if(active_model!=NULL)gtk_check_button_set_active(GTK_CHECK_BUTTON(xmirbtn), active_model->xmir[mtyp]);
      g_signal_connect (GTK_CHECK_BUTTON (xmirbtn), "toggled", G_CALLBACK (xmirbtn_toggled_cb), NULL);
      gtk_grid_attach(GTK_GRID (grid_orient_area),xmirbtn, 5, 2, 1, 1);
      gtk_widget_set_halign(xmirbtn, GTK_ALIGN_CENTER);
	
      ymirbtn=gtk_check_button_new_with_label (" ");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(ymirbtn), 0);
      if(active_model!=NULL)gtk_check_button_set_active(GTK_CHECK_BUTTON(ymirbtn), active_model->ymir[mtyp]);
      g_signal_connect (GTK_CHECK_BUTTON (ymirbtn), "toggled", G_CALLBACK (ymirbtn_toggled_cb), NULL);
      gtk_grid_attach(GTK_GRID (grid_orient_area),ymirbtn, 5, 3, 1, 1);
      gtk_widget_set_halign(ymirbtn, GTK_ALIGN_CENTER);
      
      zmirbtn=gtk_check_button_new_with_label (" ");
      gtk_check_button_set_active(GTK_CHECK_BUTTON(zmirbtn), 0);
      if(active_model!=NULL)gtk_check_button_set_active(GTK_CHECK_BUTTON(zmirbtn), active_model->zmir[mtyp]);
      g_signal_connect (GTK_CHECK_BUTTON (zmirbtn), "toggled", G_CALLBACK (zmirbtn_toggled_cb), NULL);
      gtk_grid_attach(GTK_GRID (grid_orient_area),zmirbtn, 5, 4, 1, 1);
      gtk_widget_set_halign(zmirbtn, GTK_ALIGN_CENTER);
    }

    // set up SIZE dimensions (passive)
    {
      sprintf(scratch,"Size:");
      while(strlen(scratch)<col_width)strcat(scratch," ");
      lbl_info = gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_orient_area), lbl_info, 6, 1, 1, 1);
      
      sprintf(scratch,"  ");
      if(active_model!=NULL)sprintf(scratch,"%6.3f",(active_model->xmax[MODEL]-active_model->xmin[MODEL]));
      lbl_x_size=gtk_label_new(scratch); 
      gtk_grid_attach(GTK_GRID(grid_orient_area),lbl_x_size,6,2,1,1);
      gtk_label_set_xalign (GTK_LABEL(lbl_x_size),0.0);

      sprintf(scratch,"  ");
      if(active_model!=NULL)sprintf(scratch,"%6.3f",(active_model->ymax[MODEL]-active_model->ymin[MODEL]));
      lbl_y_size=gtk_label_new(scratch); 
      gtk_grid_attach(GTK_GRID(grid_orient_area),lbl_y_size,6,3,1,1);
      gtk_label_set_xalign (GTK_LABEL(lbl_y_size),0.0);

      sprintf(scratch,"  ");
      if(active_model!=NULL)sprintf(scratch,"%6.3f",(active_model->zmax[MODEL]-active_model->zmin[MODEL]));
      lbl_z_size=gtk_label_new(scratch); 
      gtk_grid_attach(GTK_GRID(grid_orient_area),lbl_z_size,6,4,1,1);
      gtk_label_set_xalign (GTK_LABEL(lbl_z_size),0.0);
      
    }

    // set up OPERATIONS
    {
      sprintf(scratch,"Operations");
      frame_oper_area=gtk_frame_new(scratch);
      GtkWidget *box_oper_area=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(frame_oper_area), box_oper_area);		// define box inside of frame
      gtk_widget_set_size_request(box_oper_area,win_width,250);
      gtk_box_append(GTK_BOX(operbox),frame_oper_area);

      // divide the operations frame into an upper section for tool labels, and a lower section for operations
      op_upper_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
      gtk_box_append(GTK_BOX(box_oper_area),op_upper_box);
      grid_oper_upper=gtk_grid_new();
      gtk_box_append(GTK_BOX(op_upper_box),grid_oper_upper);		
      gtk_grid_set_row_spacing (GTK_GRID(grid_oper_upper),2);
      gtk_grid_set_column_spacing (GTK_GRID(grid_oper_upper),10);
      
      oper_win=gtk_scrolled_window_new();
      gtk_widget_set_size_request(oper_win,win_width,180);
      gtk_box_append(GTK_BOX(box_oper_area),oper_win);			
      grid_oper_lower=gtk_grid_new();
      gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(oper_win),grid_oper_lower);
      gtk_grid_set_row_spacing (GTK_GRID(grid_oper_lower),10);
      gtk_grid_set_column_spacing (GTK_GRID(grid_oper_lower),10);
  
      // setup available tools for operations across heading
      col_width=25;
      for(i=0;i<MAX_TOOLS;i++)
	{
	if(Tool[i].state==TL_EMPTY)
	  {
	  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	    sprintf(scratch,"Tool %d ",(i+1));
	  #endif
	  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	    sprintf(scratch,"  ");
	  #endif
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  lbl_op_top=gtk_label_new (scratch);
	  markup = g_markup_printf_escaped (norm_fmt,scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_top),markup);
	  
	  sprintf(scratch," Empty ");
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (norm_fmt,scratch);
	  lbl_op_mid=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_mid),markup);
	  
	  sprintf(scratch," - ");
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (norm_fmt,scratch);
	  lbl_op_btm=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_btm),markup);
	  }
	if(Tool[i].state==TL_UNKNOWN)
	  {
	  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	    sprintf(scratch,"Tool %d ",(i+1));
	  #endif
	  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	    sprintf(scratch,"   ");
	  #endif
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  lbl_op_top=gtk_label_new (scratch);
	  markup = g_markup_printf_escaped (unkw_fmt,scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_top),markup);
	  
	  sprintf(scratch,"UNKNOWN ");
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (unkw_fmt,scratch);
	  lbl_op_mid=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_mid),markup);
	  
	  sprintf(scratch," - ");
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (unkw_fmt,scratch);
	  lbl_op_btm=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_btm),markup);
	  }
	if(Tool[i].state>=TL_LOADED)
	  {
	  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	    sprintf(scratch,"Tool %d ",(i+1));
	  #endif
	  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	    sprintf(scratch,"   ");
	  #endif
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  lbl_op_top=gtk_label_new (scratch);
	  markup = g_markup_printf_escaped (okay_fmt,scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_top),markup);
	  
	  sprintf(scratch," %s ",Tool[i].name);
	  if(strlen(Tool[i].name)>10)
	    {
	    memset(scratch,0,sizeof(scratch));
	    strncpy(scratch,Tool[i].name,7);
	    strcat(scratch,"...");
	    }
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (okay_fmt,scratch);
	  lbl_op_mid=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_mid),markup);

	  sprintf(scratch," - ");
	  if(Tool[i].state>=TL_LOADED)sprintf(scratch," %s ",Tool[i].matl.name);
	  if(strlen(Tool[i].matl.name)>10)
	    {
	    memset(scratch,0,sizeof(scratch));
	    strncpy(scratch,Tool[i].matl.name,7);
	    strcat(scratch,"...");
	    }
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  markup = g_markup_printf_escaped (okay_fmt,scratch);
	  lbl_op_btm=gtk_label_new (scratch);
	  gtk_label_set_markup(GTK_LABEL(lbl_op_btm),markup);
	  }
	gtk_widget_set_halign(lbl_op_top,GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid_oper_upper),lbl_op_top,(i+3),1,1,1);
	gtk_widget_set_halign(lbl_op_mid,GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid_oper_upper),lbl_op_mid,(i+3),2,1,1);
	gtk_widget_set_halign(lbl_op_btm,GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid_oper_upper),lbl_op_btm,(i+3),3,1,1);
	}
	
      // to get display formatting right, we need to find the longest operation description
      // and match the title above it in length.. gtk thing
      col_width=24;							// set default length
      for(op_typ=1;op_typ<MAX_OP_TYPES;op_typ++)
	{
	// loop thru all the tools
	for(slot=0;slot<MAX_TOOLS;slot++)				
	  {
	  // find the actual tool operation that matches the current operation
	  optr=Tool[slot].oper_first;
	  while(optr!=NULL)
	    {
	    if(optr->ID==op_typ)break;
	    optr=optr->next;
	    }
	  if(optr==NULL)continue;					// no match, move onto next tool

	  // build operation description
	  if(strlen(op_description[op_typ])>col_width)col_width=strlen(op_description[op_typ]);
	  }
	}

      sprintf(scratch," Description");
      while(strlen(scratch)<(col_width+5))strcat(scratch," ");
      lbl_info=gtk_label_new (scratch);
      gtk_label_set_xalign(GTK_LABEL(lbl_info),0.5);
      gtk_grid_attach (GTK_GRID (grid_oper_upper),lbl_info,0,3,2,1);
      
      gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);
      GtkWidget *lbl_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
      gtk_grid_attach (GTK_GRID (grid_oper_upper),lbl_separator,0,4,3,1);
      
      sprintf(scratch,"             ");
      lbl_info=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_oper_lower),lbl_info,0,0,2,1);
      sprintf(scratch,"                     ");
      lbl_info=gtk_label_new (scratch);
      gtk_grid_attach (GTK_GRID (grid_oper_lower),lbl_info,3,0,1,1);
      
 
      // loop thru all possible operations and display the ones that match the loaded tools
      vrow=0;
      for(op_typ=1;op_typ<MAX_OP_TYPES;op_typ++)
	{
	// loop thru all the tools
	for(slot=0;slot<MAX_TOOLS;slot++)				
	  {
	  // find the actual tool operation that matches the current operation
	  optr=Tool[slot].oper_first;
	  while(optr!=NULL)
	    {
	    if(optr->ID==op_typ)break;
	    optr=optr->next;
	    }
	  if(optr==NULL)continue;					// no match, move onto next tool

	  // build operation description
	  sprintf(scratch,"   %s",op_description[op_typ]);
	  while(strlen(scratch)<col_width)strcat(scratch," ");
	  lbl_info=gtk_label_new (scratch);
	  gtk_label_set_xalign (GTK_LABEL(lbl_info),1.0);
	  gtk_grid_attach (GTK_GRID (grid_oper_lower),lbl_info,0,vrow,2,1);
	  gtk_widget_set_halign(lbl_info, GTK_ALIGN_CENTER);

	  sprintf(scratch,"      ");
	  lbl_info=gtk_label_new (scratch);
	  gtk_grid_attach (GTK_GRID (grid_oper_lower),lbl_info,2,vrow,2,1);

	  // build operation checkbox button
	  btn_op[op_typ][slot] = gtk_check_button_new();
	  gtk_widget_set_halign(btn_op[op_typ][slot],GTK_ALIGN_CENTER);
	  gtk_grid_attach (GTK_GRID (grid_oper_lower),btn_op[op_typ][slot],(slot+3),vrow,1,1);
	  gtk_widget_set_sensitive(btn_op[op_typ][slot],TRUE);

	  // these were done as indpendent call backs to allow customization for each operation
	  // not to mention binding all these buttons to one callback looked like a nitemare

	  // additive operations 
	  if(op_typ==OP_ADD_MODEL_MATERIAL)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_add_model_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_ADD_SUPPORT_MATERIAL)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_add_supt_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_ADD_BASE_LAYER)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_add_base_toggled_cb),GINT_TO_POINTER(slot));}

	  // subtractive operations
	  if(op_typ==OP_MILL_OUTLINE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mill_outline_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MILL_AREA)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mill_area_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MILL_PROFILE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mill_profile_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MILL_HOLES)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mill_holes_toggled_cb),GINT_TO_POINTER(slot));}
	  
	  // measurement operations  
	  if(op_typ==OP_MEASURE_X)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_meas_x_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MEASURE_Y)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_meas_y_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MEASURE_Z)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_meas_z_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MEASURE_HOLES)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_meas_holes_toggled_cb),GINT_TO_POINTER(slot));}
	    
	  // miscellaneous operations
	  if(op_typ==OP_MARK_OUTLINE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mark_outline_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MARK_AREA)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mark_area_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MARK_IMAGE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mark_image_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_MARK_CUT)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_mark_cut_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_CURE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_cure_toggled_cb),GINT_TO_POINTER(slot));}
	  if(op_typ==OP_PLACE)
	    {g_signal_connect( btn_op[op_typ][slot],"toggled",G_CALLBACK(btn_op_place_toggled_cb),GINT_TO_POINTER(slot));}

	  //if(Tool[slot].state>=TL_LOADED && optr!=NULL)			// if tool loaded and includes this operation...
	  //  {gtk_widget_set_sensitive(btn_op[op_typ][slot],TRUE);}	// ... set the checkbox as check-able
	  //else 
	  //  {gtk_widget_set_sensitive(btn_op[op_typ][slot],FALSE);}	// ... otherwise make it un-check-able

	  vrow++;							// increment grid row to place next operation
	  }
	}

      // if in-bound with a model loaded, then set buttons to match its state
      if(active_model!=NULL)
	{
	aptr=active_model->oper_list;					// loop thru all operations currently requested for this model
	while(aptr!=NULL)
	  {
	  for(slot=0;slot<MAX_TOOLS;slot++)				// loop thru all tool slots
	    {
	    optr=operation_find_by_name(slot,aptr->name);		// get pointer to operation based on operation name
	    if(optr==NULL)continue;					// if no operation found, just skip to next slot
	    op_typ=optr->ID;						// set index based on operation ID
	    if(aptr->ID==slot)						// if this model's operation is requesting to use this tool...
	      {
	      gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),TRUE);		// check the box
	      for(i=0;i<MAX_TOOLS;i++)								// loop thru other tools 
	        {
		if(i==slot){gtk_widget_set_sensitive(btn_op[op_typ][i],TRUE);}			// turn on the ability to check/de-check this box
		else {gtk_widget_set_sensitive(btn_op[op_typ][i],FALSE);}			// turn off the ability to check/de-check this box
		}
	      }
	    else 							// otherwise...
	      {
	      gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_op[op_typ][slot]),FALSE);	// un-check the box
	      }
	    }
	  aptr=aptr->next;						// move onto this model's next operation
	  }
	}

      // add some blank lines after legit operations to pad bottom of scroll window otherwise bottom op gets cropped
      //for(i=0;i<2;i++)
	//{
	//sprintf(scratch," -- ");
	//lbl_info=gtk_label_new(scratch);
	//gtk_grid_attach (GTK_GRID (grid_oper_lower), lbl_info, 1, (i+4+MAX_OP_TYPES), 1, 1);
	//}
	
      }
      
    // because setting checkboxes actually calls the checkbox callbacks, we need to "un-do" the model reslice flag settings.
    // the reslice flag will get set to TRUE if the user actually checks/un-checks any boxes once the window is up
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      mptr->reslice_flag=FALSE;
      mptr=mptr->next;
      }

    gtk_widget_set_visible(win_model,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

    return(1);
}

G_GNUC_END_IGNORE_DEPRECATIONS
