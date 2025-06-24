#include "Global.h"	


// Callback to turn on/off a checkbox value
void btn_endpts_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_endpts++;
    if(set_view_endpts>1)set_view_endpts=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_vecnum_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_vecnum++;
    if(set_view_vecnum>1)set_view_vecnum=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_veclen_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_veclen++;
    if(set_view_veclen>1)set_view_veclen=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_vecloc_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_vecloc++;
    if(set_view_vecloc>1)set_view_vecloc=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_poly_ids_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_poly_ids++;
    if(set_view_poly_ids>1)set_view_poly_ids=0;
    set_view_model_lt_width=FALSE;
    return;
}

// Callback to turn on/off a checkbox value
void btn_poly_fam_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_poly_fam++;
    if(set_view_poly_fam>1)set_view_poly_fam=0;
    set_view_model_lt_width=FALSE;
    return;
}

// Callback to turn on/off a checkbox value
void btn_poly_dist_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_poly_dist++;
    if(set_view_poly_dist>1)set_view_poly_dist=0;
    set_view_model_lt_width=FALSE;
    return;
}

// Callback to turn on/off a checkbox value
void btn_poly_perim_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_poly_perim++;
    if(set_view_poly_perim>1)set_view_poly_perim=0;
    set_view_model_lt_width=FALSE;
    return;
}

// Callback to turn on/off a checkbox value
void btn_poly_area_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_poly_area++;
    if(set_view_poly_area>1)set_view_poly_area=0;
    set_view_model_lt_width=FALSE;
    return;
}

// Callback to turn on/off a checkbox value
void btn_rawmdl_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_raw_mdl++;
    if(set_view_raw_mdl>1)set_view_raw_mdl=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_rawstl_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_raw_stl++;
    if(set_view_raw_stl>1)set_view_raw_stl=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_strskl_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_strskl++;
    if(set_view_strskl>1)set_view_strskl=0;
    return;
}


// Callback to turn on/off a linetype checkbox value
void btn_lt_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  // nothing to do here since toggle buttons take care of themselves
  // instead, set global variable upon exit of window (see below)
  return;
}

// Callback to destroy line type upon exit
void on_view_lt_exit (GtkWidget *btn, gpointer dead_window)
{
  int		i,btn_chk,slot;
  model		*mptr;
  operation	*optr;
  linetype 	*ext_lt;
  genericlist	*aptr,*lptr,*new_lt;
  
  // convert button checkbox states into global line type view flags

  // init checkbox state for MODEL
  for(i=MDL_PERIM;i<=MDL_LAYER_1;i++)
    {
    btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_linetype[i]) );
    if(btn_chk==TRUE) {set_view_lt[i]=TRUE;} else {set_view_lt[i]=FALSE;}
    }
  btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_model_lt_width) );
  if(btn_chk==TRUE) {set_view_model_lt_width=TRUE;} else {set_view_model_lt_width=FALSE;}

  // init checkbox state for SUPPORT
  for(i=SPT_PERIM;i<=SPT_LAYER_1;i++)
    {
    btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_linetype[i]) );
    if(btn_chk==TRUE) {set_view_lt[i]=TRUE;} else {set_view_lt[i]=FALSE;}
    }
  btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_support_lt_width) );
  if(btn_chk==TRUE) {set_view_support_lt_width=TRUE;} else {set_view_support_lt_width=FALSE;}

  // init checkbox state for BASE LAYERS
  for(i=BASELYR;i<=PLATFORMLYR2;i++)
    {
    btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_linetype[i]) );
    if(btn_chk==TRUE) {set_view_lt[i]=TRUE;} else {set_view_lt[i]=FALSE;}
    }

  // init checkbox state for OTHERS
  for(i=TRACE;i<=SURFTRIM;i++)
    {
    btn_chk=gtk_check_button_get_active( GTK_CHECK_BUTTON(btn_view_linetype[i]) );
    if(btn_chk==TRUE) {set_view_lt[i]=TRUE;} else {set_view_lt[i]=FALSE;}
    }
    
  // if user has selected a line type to view that is NOT part of the line type sequence for
  // this tool's specific operation, it must be temporarily added to the sequence
  set_view_lt_override=FALSE;						// view lt sequence matches tool's op sequence
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    aptr=mptr->oper_list;						// look thru all operations called out by this model
    while(aptr!=NULL)							// recall that aptr->ID=tool and aptr->name=operation
      {
      slot=aptr->ID;
      if(slot<0 || slot>=MAX_TOOLS){aptr=aptr->next;continue;}
      if(Tool[slot].state!=TL_READY || Tool[slot].state==TL_FAILED){aptr=aptr->next;continue;}
      optr=operation_find_by_name(slot,aptr->name);			// find the matching operation in the tool's abilities (operations)
      if(optr==NULL){aptr=aptr->next;continue;}				// if not found/defined... move onto next operation
      for(i=1;i<MAX_LINE_TYPES;i++)					// loop thru all line types and compare against view request
	{
	if(set_view_lt[i]==TRUE)					// if this line type is requested to be viewed...
	  {
	  lptr=optr->lt_seq;						// reset start of list to look thru
	  while(lptr!=NULL)						// loop thru all line types for this tool's operation
	    {
	    if(lptr->ID==i)break;					// if a match was found, break here
	    lptr=lptr->next;
	    }
	  if(lptr==NULL)						// if no match was found, add lt to list...
	    {
	    ext_lt=linetype_find(slot,i);				// get pointer to existing line type
	    if(ext_lt!=NULL)
	      {
	      new_lt=genericlist_make();				// create a new lt element
	      new_lt->ID=i;						// assign the lt to be viewed
	      strcpy(new_lt->name,ext_lt->name);			// assign the lt name to be viewed
	      genericlist_insert(optr->lt_seq,new_lt);			// insert the temporary lt into the op list
	      set_view_lt_override=TRUE;				// view lt sequence no longer matches tool's op sequence
	      }
	    }
	  }
	}
      aptr=aptr->next;
      }
    mptr=mptr->next;
    }
    
  gtk_window_close(GTK_WINDOW(dead_window));
  printf("STGS: on_lt_view_exit - exit\n");
  return;
}

// Callback to set line type viewing
void btn_view_lt_callback(GtkWidget *btn, gpointer src_window)
{
    int		ltyp;
    GtkWidget	*lt_settings;
    GtkWidget	*hbox, *vbox, *hbox_up, *hbox_dn;
    GtkWidget	*view_mdl_frame, *view_spt_frame, *view_other_frame, *view_subtract_frame;
    GtkWidget	*grd_view_mdl, *grd_view_spt, *grd_view_other, *grd_view_subtract;
    GtkWidget	*img_exit, *btn_exit;

    // set up line type settings window
    lt_settings = gtk_window_new();			
    gtk_window_set_transient_for (GTK_WINDOW(lt_settings), GTK_WINDOW(src_window));	// make subordinate to win_settings
    gtk_window_set_modal (GTK_WINDOW(lt_settings), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(lt_settings),LCD_WIDTH-150,LCD_HEIGHT-150);
    gtk_window_set_resizable(GTK_WINDOW(lt_settings),FALSE);				
    sprintf(scratch,"Line Type Settings");
    gtk_window_set_title(GTK_WINDOW(lt_settings),scratch);			
    
    // set up an hbox to divide the screen into a narrower segments
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    //gtk_box_set_spacing (GTK_BOX(hbox),20);
    gtk_window_set_child(GTK_WINDOW(lt_settings),hbox);

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
    g_signal_connect (btn_exit, "clicked", G_CALLBACK (on_view_lt_exit), lt_settings);
    img_exit = gtk_image_new_from_file("Back.gif");
    gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
    gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
    gtk_box_append(GTK_BOX(vbox),btn_exit);

    // set up line types for MODEL
    {
      view_mdl_frame = gtk_frame_new("Model Line Types");
      gtk_box_append(GTK_BOX(hbox_up),view_mdl_frame);
      
      GtkWidget *view_mdl_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(view_mdl_frame),view_mdl_box);
      gtk_widget_set_size_request(view_mdl_box,350,275);
      
      grd_view_mdl = gtk_grid_new ();
      gtk_box_append(GTK_BOX(view_mdl_box),grd_view_mdl);
      gtk_grid_set_row_spacing (GTK_GRID(grd_view_mdl),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_view_mdl),20);
      
      // create toggle check buttons
      btn_view_linetype[MDL_PERIM] = gtk_check_button_new_with_label ("Perimeter");
      btn_view_linetype[MDL_BORDER] = gtk_check_button_new_with_label ("Boarder");
      btn_view_linetype[MDL_OFFSET] = gtk_check_button_new_with_label ("Offset");
      btn_view_linetype[MDL_FILL] = gtk_check_button_new_with_label ("Fill");
      btn_view_linetype[MDL_LOWER_CO] = gtk_check_button_new_with_label ("Lower Close-off");
      btn_view_linetype[MDL_UPPER_CO] = gtk_check_button_new_with_label ("Upper Close-off");
      btn_view_linetype[MDL_LAYER_1] = gtk_check_button_new_with_label ("First Layer");
      btn_view_model_lt_width = gtk_check_button_new_with_label ("Show Line Type Width");

      // place them into the grid
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_PERIM], 0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_BORDER], 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_OFFSET], 0, 2, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_FILL], 0, 3, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_LOWER_CO], 0, 4, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_UPPER_CO], 0, 5, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_linetype[MDL_LAYER_1], 0, 6, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_mdl), btn_view_model_lt_width, 0, 7, 1, 1);
      
      // connect handlers
      ltyp=MDL_PERIM;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_BORDER;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_OFFSET;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_FILL;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_LOWER_CO;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_UPPER_CO;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=MDL_LAYER_1;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      g_signal_connect ( btn_view_model_lt_width, "toggled", G_CALLBACK (btn_lt_toggled_cb), NULL);
    }

    // set up line types for SUPPORT
    {
      view_spt_frame = gtk_frame_new("Support Line Types");
      gtk_box_append(GTK_BOX(hbox_dn),view_spt_frame);
      
      GtkWidget *view_spt_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(view_spt_frame),view_spt_box);
      gtk_widget_set_size_request(view_spt_box,350,275);
      
      grd_view_spt = gtk_grid_new ();
      gtk_box_append(GTK_BOX(view_spt_box),grd_view_spt);
      gtk_grid_set_row_spacing (GTK_GRID(grd_view_spt),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_view_spt),20);
      
      // create toggle check buttons
      btn_view_linetype[SPT_PERIM] = gtk_check_button_new_with_label ("Perimeter");
      btn_view_linetype[SPT_BORDER] = gtk_check_button_new_with_label ("Boarder");
      btn_view_linetype[SPT_OFFSET] = gtk_check_button_new_with_label ("Offset");
      btn_view_linetype[SPT_FILL] = gtk_check_button_new_with_label ("Fill");
      btn_view_linetype[SPT_LOWER_CO] = gtk_check_button_new_with_label ("Lower Close-off");
      btn_view_linetype[SPT_UPPER_CO] = gtk_check_button_new_with_label ("Upper Close-off");
      btn_view_linetype[SPT_LAYER_1] = gtk_check_button_new_with_label ("First Layer");
      btn_view_support_lt_width = gtk_check_button_new_with_label ("Show Line Type Width");

      // place them into the grid
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_PERIM], 0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_BORDER], 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_OFFSET], 0, 2, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_FILL], 0, 3, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_LOWER_CO], 0, 4, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_UPPER_CO], 0, 5, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_linetype[SPT_LAYER_1], 0, 6, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_spt), btn_view_support_lt_width, 0, 7, 1, 1);
      
      // connect handlers
      ltyp=SPT_PERIM;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_BORDER;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_OFFSET;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_FILL;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_LOWER_CO;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_UPPER_CO;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SPT_LAYER_1;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      g_signal_connect ( btn_view_support_lt_width, "toggled", G_CALLBACK (btn_lt_toggled_cb), NULL);
    }
    
    // set up line types for OTHER
    {
      view_other_frame = gtk_frame_new("Other Line Types");
      gtk_box_append(GTK_BOX(hbox_up),view_other_frame);
      
      GtkWidget *view_other_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(view_other_frame),view_other_box);
      gtk_widget_set_size_request(view_other_box,350,250);
      
      grd_view_other = gtk_grid_new ();
      gtk_box_append(GTK_BOX(view_other_box),grd_view_other);
      gtk_grid_set_row_spacing (GTK_GRID(grd_view_other),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_view_other),20);
      
      // create toggle check buttons
      btn_view_linetype[BASELYR] = gtk_check_button_new_with_label ("Base layer");
      btn_view_linetype[PLATFORMLYR1] = gtk_check_button_new_with_label ("Platform 1");
      btn_view_linetype[PLATFORMLYR2] = gtk_check_button_new_with_label ("Platform 2");
      btn_view_linetype[TRACE] = gtk_check_button_new_with_label ("Gerber trace");
      btn_view_linetype[DRILL] = gtk_check_button_new_with_label ("Gerber drill");

      // place them into the grid
      gtk_grid_attach (GTK_GRID (grd_view_other), btn_view_linetype[BASELYR], 0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_other), btn_view_linetype[PLATFORMLYR1], 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_other), btn_view_linetype[PLATFORMLYR2], 0, 2, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_other), btn_view_linetype[TRACE], 0, 3, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_other), btn_view_linetype[DRILL], 0, 4, 1, 1);
      
      // connect handlers
      ltyp=BASELYR;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=PLATFORMLYR1;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=PLATFORMLYR2;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=TRACE;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=DRILL;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
    }
    
    // set up line types for SUBTRACTIVE
    {
      view_subtract_frame = gtk_frame_new("Subtractive Line Types");
      gtk_box_append(GTK_BOX(hbox_dn),view_subtract_frame);
      
      GtkWidget *view_subtract_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_frame_set_child(GTK_FRAME(view_subtract_frame),view_subtract_box);
      gtk_widget_set_size_request(view_subtract_box,350,250);
      
      grd_view_subtract = gtk_grid_new ();
      gtk_box_append(GTK_BOX(view_subtract_box),grd_view_subtract);
      gtk_grid_set_row_spacing (GTK_GRID(grd_view_subtract),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_view_subtract),20);
      
      // create toggle check buttons
      btn_view_linetype[VERTTRIM] = gtk_check_button_new_with_label ("Vertical trimming");
      btn_view_linetype[HORZTRIM] = gtk_check_button_new_with_label ("Horizontal trimming");
      btn_view_linetype[SURFTRIM] = gtk_check_button_new_with_label ("Surface trimming");

      // place them into the grid
      gtk_grid_attach (GTK_GRID (grd_view_subtract), btn_view_linetype[VERTTRIM], 0, 0, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_subtract), btn_view_linetype[HORZTRIM], 0, 1, 1, 1);
      gtk_grid_attach (GTK_GRID (grd_view_subtract), btn_view_linetype[SURFTRIM], 0, 2, 1, 1);
      
      // connect handlers
      ltyp=VERTTRIM;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=HORZTRIM;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
      ltyp=SURFTRIM;
      g_signal_connect ( btn_view_linetype[ltyp], "toggled", G_CALLBACK (btn_lt_toggled_cb), GINT_TO_POINTER(ltyp));
    }
    
    // init checkbox state for MODEL
    for(ltyp=MDL_PERIM;ltyp<=MDL_LAYER_1;ltyp++)
      {
      if(set_view_lt[ltyp]==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), FALSE);}
      }
    if(set_view_model_lt_width==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_model_lt_width), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_model_lt_width), FALSE);}

    // init checkbox state for SUPPORT
    for(ltyp=SPT_PERIM;ltyp<=SPT_LAYER_1;ltyp++)
      {
      if(set_view_lt[ltyp]==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), FALSE);}
      }
    if(set_view_support_lt_width==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_support_lt_width), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_support_lt_width), FALSE);}

    // init checkbox state for BASE LAYERS
    for(ltyp=BASELYR;ltyp<=PLATFORMLYR2;ltyp++)
      {
      if(set_view_lt[ltyp]==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), FALSE);}
      }

    // init checkbox state for OTHERS
    for(ltyp=TRACE;ltyp<=SURFTRIM;ltyp++)
      {
      if(set_view_lt[ltyp]==TRUE) {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), TRUE);}
      else  {gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_view_linetype[ltyp]), FALSE);}
      }

    gtk_widget_set_visible(lt_settings,TRUE);
    
    return;
}


// Callback to turn on/off a checkbox value
void btn_blvl_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_build_lvl++;
    if(set_view_build_lvl>1)set_view_build_lvl=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_penup_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_penup_moves++;
    if(set_view_penup_moves>1)set_view_penup_moves=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_slice_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_mdl_slice++;
    if(set_view_mdl_slice>1)set_view_mdl_slice=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_toolpos_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_tool_pos++;
    if(set_view_tool_pos>1)set_view_tool_pos=0;
    return;
}


// Callback to turn on/off a checkbox value
void btn_show_only_current_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    slc_view_start=z_cut;
    slc_view_end=z_cut+CLOSE_ENOUGH;
    return;
}

// Callback to turn on/off a checkbox value
void btn_show_entire_stack_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    slc_view_start=ZMin;
    slc_view_end=ZMax;
    return;
}

// Callback to get slice view start height
void slc_view_start_value(GtkSpinButton *button, gpointer user_data)
{
  slc_view_start=gtk_spin_button_get_value(button);
  if(slc_view_start<0.0)slc_view_start=0.0;
  if(slc_view_start>ZMax)slc_view_end=ZMax;
  return;
}

// Callback to get slice view end height
void slc_view_end_value(GtkSpinButton *button, gpointer user_data)
{
  slc_view_end=gtk_spin_button_get_value(button);
  if(slc_view_end<0.0)slc_view_end=0.0;
  if(slc_view_end>ZMax)slc_view_end=ZMax;
  return;
}

// Callback to get slice view increment
void slc_view_increment_value(GtkSpinButton *button, gpointer user_data)
{
  slc_view_inc=gtk_spin_button_get_value(button);
  if(slc_view_inc<job.min_slice_thk)slc_view_inc=job.min_slice_thk;
  if(slc_view_inc>ZMax)slc_view_inc=ZMax;
  return;
}

// Callback to turn on/off a checkbox value
void btn_show_normals_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_normals++;
    if(set_view_normals>1)set_view_normals=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_view_patches_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_patches++;
    if(set_view_patches>1)set_view_patches=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_view_bounds_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    show_view_bound_box++;
    if(show_view_bound_box>1)show_view_bound_box=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_show_models_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_models++;
    if(set_view_models>1)set_view_models=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_show_supports_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_supports++;
    if(set_view_supports>1)set_view_supports=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_show_internal_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_internal++;
    if(set_view_internal>1)set_view_internal=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_show_target_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_target++;
    if(set_view_target>1)set_view_target=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_facets_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_facet_display++;
    if(set_facet_display>1)set_facet_display=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_wireframe_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_edge_display++;
    if(set_edge_display>1)set_edge_display=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_image_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_view_image++;
    if(set_view_image>1)set_view_image=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_superimpose_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    Superimpose_flag++;
    if(Superimpose_flag>1)Superimpose_flag=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_per_scale_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    Perspective_flag++;
    if(Perspective_flag>1)Perspective_flag=0;
    if(Perspective_flag==TRUE)
      {
      pers_scale=0.10;
      if(MVview_scale<0.2)MVview_scale=0.2;
      if(MVview_scale>12.0)MVview_scale=12.0;
      }
    return;
}

// Callback to get edge viewing angle
void edge_angle_value(GtkSpinButton *button, gpointer user_data)
{
  model		*mptr;
  
  set_view_edge_angle=gtk_spin_button_get_value(button);
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    edge_display(mptr,set_view_edge_angle,MODEL);			// determine which edges to display in 3D view
    edge_display(mptr,set_view_edge_angle,SUPPORT);			// determine which edges to display in 3D view
    mptr=mptr->next;
    }
  
  return;
}

// Callback to get max facet qty to display
void facet_qty_value(GtkSpinButton *button, gpointer user_data)
{
  
  set_view_max_facets=gtk_spin_button_get_value(button);
  if(set_view_max_facets<1)set_view_max_facets=1;
  if(set_view_max_facets>99)set_view_max_facets=99;
  
  return;
}

// Callback to select what entity type a mouse pick will select
void btn_pick_entity_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    int		user_request;
    
    user_request=GPOINTER_TO_INT(user_data);
    set_mouse_pick_entity=user_request;
    
    return;
}

// Callback to select if connected neighbors will be highlighted
void btn_pick_neighbors_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_show_pick_neighbors++;
    if(set_show_pick_neighbors>1)set_show_pick_neighbors=0;
    
    return;
}


// Callback to select what to track for graph during build
void btn_graph_type_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    hist_type++;
    if(hist_type>1)hist_type=0;
    return;
}

// Callback to get material feed time for purging
void matl_feed_value(GtkSpinButton *button, gpointer user_data)
{
  max_feed_time=gtk_spin_button_get_value(button);
  if(max_feed_time<5)max_feed_time=5;
  if(max_feed_time>59)max_feed_time=59;
  
  return;
}

// Callback to get edge detection threshold for image processing
void edge_detect_threshold_cb(GtkSpinButton *button, gpointer user_data)
{
  edge_detect_threshold=gtk_spin_button_get_value(button);
  if(edge_detect_threshold<0)edge_detect_threshold=0;
  if(edge_detect_threshold>255)edge_detect_threshold=255;
  
  return;
}

// Callback to get acceptable temperature tolerance
void temp_tol_value(GtkSpinButton *button, gpointer user_data)
{
  int	slot;
  
  acceptable_temp_tol=gtk_spin_button_get_value(button);
  if(acceptable_temp_tol<1)acceptable_temp_tol=1;
  if(acceptable_temp_tol>20)acceptable_temp_tol=20;
  
  for(slot=0;slot<MAX_THERMAL_DEVICES;slot++)
    {
    Tool[slot].thrm.tolrC=acceptable_temp_tol;
    }
  
  return;
}

// Callback to get which device for temperature calibration
void grab_device_for_temp_cal(GtkSpinButton *button, gpointer user_data)
{
  
  device_being_calibrated=gtk_spin_button_get_value(button);
  
  if(device_being_calibrated>=0 && device_being_calibrated<MAX_TOOLS)
    {
    sprintf(scratch," Tool %d ",device_being_calibrated+1);
    }
  
  if(device_being_calibrated==BLD_TBL1)
    {
    sprintf(scratch," Build Table 1 ");
    }
  
  if(device_being_calibrated==BLD_TBL2)
    {
    sprintf(scratch," Build Table 2 ");
    }
  
  if(device_being_calibrated==CHAMBER)
    {
    sprintf(scratch," Chamber ");
    }
    
  gtk_label_set_text(GTK_LABEL(lbl_device_being_calibrated),scratch);
  gtk_adjustment_set_value(temp_cal_offset,Tool[device_being_calibrated].thrm.temp_offset);
  
  if(device_being_calibrated==BLD_TBL1)printf("\nBLD_TBL1: toff=%f\n",Tool[BLD_TBL1].thrm.temp_offset);
  if(device_being_calibrated==BLD_TBL2)printf("\nBLD_TBL2: toff=%f\n",Tool[BLD_TBL2].thrm.temp_offset);
  if(device_being_calibrated==CHAMBER)printf("\nCHAMBER: toff=%f\n",Tool[CHAMBER].thrm.temp_offset);

  return;
}

// Callback to get slot temperature calibration
void temp_slot_lo_cal_value(GtkWidget *button, gpointer user_data)
{
  int		slot;
  
  slot=GPOINTER_TO_INT(user_data);
  if(set_temp_cal_load==FALSE)
    {
    sprintf(scratch," \nInsert LOW calibarion load\n into slot %d then try again. \n",slot);
    aa_dialog_box(win_settings,1,0,"Temperature Calibration",scratch);
    return;
    }
  Tool[slot].thrm.sensor=RTDH;						// force type to RTD for calib loads
  Tool[slot].thrm.temp_offset=0.0;					// remove any offset to avoid confusion
  sprintf(scratch," \nReading low temperature calibration value... \n");
  aa_dialog_box(win_settings,0,50,"Temperature Calibration",scratch);
  pthread_mutex_lock(&thermal_lock);      
  Tool[slot].thrm.tempC=0.0;
  pthread_mutex_unlock(&thermal_lock);      
  crgslot[slot].calib_t_low=0.0;					// value defined by precision resistor
  crgslot[slot].calib_v_low=Tool[slot].thrm.tempV;			// get the voltage reading at this temperature
  
  Tool[slot].thrm.calib_t_low=crgslot[slot].calib_t_low;		// setting these ought to bring to correct low temp
  Tool[slot].thrm.calib_v_low=crgslot[slot].calib_v_low;
  
  sprintf(scratch,"%6.2fC %6.3fv",Tool[slot].thrm.calib_t_low,Tool[slot].thrm.calib_v_low);
  gtk_label_set_text(GTK_LABEL(lbl_lo_values),scratch);
  
  printf("\nDevice %d: Temp=%5.3f  Volt=%5.3f \n\n",slot,crgslot[slot].calib_t_low,crgslot[slot].calib_v_low);

  return;
}

void temp_slot_hi_cal_value(GtkWidget *button, gpointer user_data)
{
  int		slot;
  
  slot=GPOINTER_TO_INT(user_data);
  if(set_temp_cal_load==FALSE)
    {
    sprintf(scratch," \nInsert HIGH calibarion load\n into slot %d then try again. \n",slot);
    aa_dialog_box(win_settings,1,0,"Temperature Calibration",scratch);
    return;
    }
  Tool[slot].thrm.sensor=RTDH;						// force type to RTD for calib loads
  Tool[slot].thrm.temp_offset=0.0;					// remove any offset to avoid confusion
  sprintf(scratch," \nReading high temperature calibration value... \n");
  aa_dialog_box(win_settings,0,50,"Temperature Calibration",scratch);
  pthread_mutex_lock(&thermal_lock);      
  Tool[slot].thrm.tempC=152.0;
  pthread_mutex_unlock(&thermal_lock);      
  crgslot[slot].calib_t_high=152.0;					// value defined by precision resistor
  crgslot[slot].calib_v_high=Tool[slot].thrm.tempV;			// get the voltage reading at this temperature
  
  Tool[slot].thrm.calib_t_high=crgslot[slot].calib_t_high;		// settting these ought to bring temp to 152.0
  Tool[slot].thrm.calib_v_high=crgslot[slot].calib_v_high;
  
  sprintf(scratch,"%6.2fC %6.3fv",Tool[slot].thrm.calib_t_high,Tool[slot].thrm.calib_v_high);
  gtk_label_set_text(GTK_LABEL(lbl_hi_values),scratch);
  
  printf("\nDevice %d: Temp=%5.3f  Volt=%5.3f \n\n",slot,crgslot[slot].calib_t_high,crgslot[slot].calib_v_high);

  return;
}

// Callback to get temp offset of device for temperature calibration
void grab_temp_cal_offset(GtkSpinButton *button, gpointer user_data)
{
  int		slot;
  
  //slot=GPOINTER_TO_INT(user_data);
  slot=device_being_calibrated;
  
  Tool[slot].thrm.temp_offset=gtk_spin_button_get_value(button);
  crgslot[slot].temp_offset=Tool[slot].thrm.temp_offset;
  
  return;
}

// Callback to save calibration values
void save_temp_cal_value(GtkWidget *button, gpointer user_data)
{
  save_unit_calibration();
  sprintf(scratch," \nCalibration values saved. \n");
  aa_dialog_box(win_settings,0,50,"Temperature Calibration",scratch);
  return;
}


// Callback to get chamber temperature override
void temp_chm_or_value(GtkSpinButton *button, gpointer user_data)
{
  // get new temp
  Tool[CHAMBER].thrm.setpC=gtk_spin_button_get_value(button);
  if(Tool[CHAMBER].thrm.setpC<1)Tool[CHAMBER].thrm.setpC=1;
  if(Tool[CHAMBER].thrm.setpC>Tool[CHAMBER].thrm.setpC)Tool[CHAMBER].thrm.setpC=Tool[CHAMBER].thrm.setpC;

  // flag as over ride enabled if not same as default
  Tool[CHAMBER].thrm.temp_override_flag=FALSE;
  if(Tool[CHAMBER].thrm.setpC != Tool[CHAMBER].thrm.bedtC)Tool[CHAMBER].thrm.temp_override_flag=TRUE;
  
  return;
}

// Callback to get table temperature override
void temp_tbl_or_value(GtkSpinButton *button, gpointer user_data)
{
  // get new temp
  Tool[BLD_TBL1].thrm.setpC=gtk_spin_button_get_value(button);
  if(Tool[BLD_TBL1].thrm.setpC<1)Tool[BLD_TBL1].thrm.setpC=1;
  if(Tool[BLD_TBL1].thrm.setpC>Tool[BLD_TBL1].thrm.maxtC)Tool[BLD_TBL1].thrm.setpC=Tool[BLD_TBL1].thrm.maxtC;

  // flag as over ride enabled if not same as default
  Tool[BLD_TBL1].thrm.temp_override_flag=FALSE;
  if(Tool[BLD_TBL1].thrm.setpC != Tool[BLD_TBL1].thrm.bedtC)Tool[BLD_TBL1].thrm.temp_override_flag=TRUE;

  #ifdef BETA_UNIT
    Tool[BLD_TBL2].thrm.setpC=Tool[BLD_TBL1].thrm.setpC;
    Tool[BLD_TBL2].thrm.temp_override_flag = Tool[BLD_TBL1].thrm.temp_override_flag;
  #endif
  #ifdef GAMMA_UNIT
    Tool[BLD_TBL2].thrm.setpC=Tool[BLD_TBL1].thrm.setpC;
    Tool[BLD_TBL2].thrm.temp_override_flag = Tool[BLD_TBL1].thrm.temp_override_flag;
  #endif
  
  return;
}

// Callback to get facet proximity tolerance
void facet_tol_value(GtkSpinButton *button, gpointer user_data)
{
  model	*mptr;
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->facet_prox_tol=gtk_spin_button_get_value(button);
    mptr=mptr->next;
    }
  job.regen_flag=TRUE;
  
  return;
}

// Callback to get colinear angle value
void colinear_angle_value(GtkSpinButton *button, gpointer user_data)
{
  max_colinear_angle=gtk_spin_button_get_value(button);
  if(max_colinear_angle<TOLERANCE)max_colinear_angle=TOLERANCE;
  if(max_colinear_angle>45.0)max_colinear_angle=45.0;
  job.regen_flag=TRUE;
  
  return;
}

// Callback to get wavefront increment value
void wavefront_inc_value(GtkSpinButton *button, gpointer user_data)
{
  ss_wavefront_increment=gtk_spin_button_get_value(button);
  if(ss_wavefront_increment<0.001)ss_wavefront_increment=0.001;
  if(ss_wavefront_increment>1.0)ss_wavefront_increment=1.0;
  job.regen_flag=TRUE;
  
  return;
}

// Callback to get image pixel size value
void pix_size_value(GtkSpinButton *button, gpointer user_data)
{
  model	*mptr;
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->pix_size=gtk_spin_button_get_value(button);
    mptr=mptr->next;
    }
  job.regen_flag=TRUE;
  
  return;
}

// Callback to get image pixel increment value
void pix_increment_value(GtkSpinButton *button, gpointer user_data)
{
  model	*mptr;
  
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->pix_increment=gtk_spin_button_get_value(button);
    mptr=mptr->next;
    }
  job.regen_flag=TRUE;
  
  return;
}

// Callback to turn on/off a checkbox value
void btn_center_job_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_center_build_job++;
    if(set_center_build_job>1)set_center_build_job=0;
    if(set_center_build_job)job_maxmin();
    return;
}

// Callback to turn on/off a checkbox value
void btn_ignore_bit_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    ignore_tool_bit_change++;
    if(ignore_tool_bit_change>1)ignore_tool_bit_change=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_ignore_drill_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    drill_holes_done_flag++;
    if(drill_holes_done_flag>1)drill_holes_done_flag=0;
    return;
}

// Callback to turn on/off a checkbox value
void btn_small_drill_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    only_drill_small_holes_flag++;
    if(only_drill_small_holes_flag>1)only_drill_small_holes_flag=0;
    return;
}

// Callback to turn on/off z build table contouring
void btn_z_tbl_comp_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    z_tbl_comp++;
    if(z_tbl_comp>1)z_tbl_comp=0;
    return;
}

// Callback to get z contour layer quantity
void zcontour_value(GtkSpinButton *button, gpointer user_data)
{
  
  set_z_contour=gtk_spin_button_get_value(button);
  if(set_z_contour<0)set_z_contour=0;
  if(set_z_contour>20)set_z_contour=20;
  
  return;
}

// Callback to turn on/off a checkbox value
void btn_profile_cut_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  model 	*mptr;
  
  cnc_profile_cut_flag++;
  if(cnc_profile_cut_flag>1)cnc_profile_cut_flag=0;
  job.regen_flag=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->reslice_flag=TRUE;
    mptr=mptr->next;
    }
    return;
}

// Callback to get vertex overlap quantity
void vtx_overlap_value(GtkSpinButton *button, gpointer user_data)
{
  
  set_vtx_overlap=gtk_spin_button_get_value(button);
  
  return;
}

// Callback to turn on/off a checkbox value
void btn_add_block_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  model 	*mptr;
  
  cnc_add_block_flag++;
  if(cnc_add_block_flag>1)cnc_add_block_flag=0;
  job.regen_flag=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->reslice_flag=TRUE;
    mptr=mptr->next;
    }

    return;
}

// Callback to get cnc block x margin
void cnc_block_x_value(GtkSpinButton *button, gpointer user_data)
{
  model		*mptr;
  
  cnc_x_margin=gtk_spin_button_get_value(button);
  if(cnc_x_margin<0)cnc_x_margin=0;
  if(cnc_x_margin>20)cnc_x_margin=20;
  job.regen_flag=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->reslice_flag=TRUE;
    mptr=mptr->next;
    }
  
  return;
}

// Callback to get cnc block y margin
void cnc_block_y_value(GtkSpinButton *button, gpointer user_data)
{
  model 	*mptr;
  
  cnc_y_margin=gtk_spin_button_get_value(button);
  if(cnc_y_margin<0)cnc_y_margin=0;
  if(cnc_y_margin>20)cnc_y_margin=20;
  job.regen_flag=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->reslice_flag=TRUE;
    mptr=mptr->next;
    }
  
  return;
}

// Callback to get cnc block z height
void cnc_block_z_value(GtkSpinButton *button, gpointer user_data)
{
  int		mtyp=MODEL;
  model 	*mptr;
  
  cnc_z_height=gtk_spin_button_get_value(button);
  if(cnc_z_height<0)cnc_z_height=0;
  if(cnc_z_height>152)cnc_z_height=152;
  job.regen_flag=TRUE;
  mptr=job.model_first;
  while(mptr!=NULL)
    {
    mptr->zoff[mtyp]=mptr->zorg[mtyp]+cnc_z_height;			// add to z offset for all models
    mptr->reslice_flag=TRUE;
    mptr=mptr->next;
    }
  
  return;
}

// Callback to turn on/off start at current z
void btn_start_at_z_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  // set flag
  set_start_at_crt_z++;
  if(set_start_at_crt_z>1)set_start_at_crt_z=0;
  
  // if turning this on, then job will start/resume where ever current z slider is set to
  if(set_start_at_crt_z==TRUE)
    {
    job.current_z=z_cut;
    }
  
  return;
}

// Callback to turn on/off forcing current z to build table
void btn_force_z_to_table_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  set_force_z_to_table++;
  if(set_force_z_to_table>1)set_force_z_to_table=0;
  return;
}

// Callback to turn on/off ignoring line type temperature changes
void btn_ignore_lt_temps_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    set_ignore_lt_temps++;
    if(set_ignore_lt_temps>1)set_ignore_lt_temps=0;
    return;
}

// Callback to turn on/off including bottom layer details
void btn_btm_lyr_details_toggled_cb(GtkWidget *btn, gpointer user_data)
{
    int		i,old_state;
    slice 	*sptr;
    model 	*mptr;
    
    old_state=set_include_btm_detail;
    set_include_btm_detail++;
    if(set_include_btm_detail>1)set_include_btm_detail=0;
    
    // if a model has already been sliced, we need to adjust the position of the slice deck
    // by the thickness of the base layer... which becomes the first slice.
    mptr=job.model_first;
    while(mptr!=NULL)
      {
      for(i=0;i<MAX_MDL_TYPES;i++)
        {
	sptr=mptr->slice_first[i];
	while(sptr!=NULL)
	  {
	  if(old_state==FALSE && set_include_btm_detail==TRUE)sptr->sz_level+=mptr->slice_thick;	// move up by one slice
	  if(old_state==TRUE && set_include_btm_detail==FALSE)sptr->sz_level-=mptr->slice_thick;	// move down by one slice
	  sptr=sptr->next;
	  }
	}
      mptr=mptr->next;
      }
      
    return;
}

// Callback to turn on/off saving image history
void btn_save_image_hist_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  // set flag
  save_image_history++;
  if(save_image_history>1)save_image_history=0;
  
  return;
}

// Callback to turn on/off saving job log files
void btn_save_job_hist_toggled_cb(GtkWidget *btn, gpointer user_data)
{
  // set flag
  save_logfile_flag++;
  if(save_logfile_flag>1)save_logfile_flag=0;
  
  return;
}


// Callback to destroy tool UI when user exits
void on_settings_exit (GtkWidget *btn, gpointer dead_window)
{

  // get rid of any previous slice data for models in the job that need reslicing
  if(job.regen_flag==TRUE)
    {
    job_purge();		// only purges slices where mptr->reslice_flag==TRUE
    job_slice();		// only slices models where mptr->reslice_flag==TRUE
    }

  gtk_window_close(GTK_WINDOW(dead_window));
  if(carriage_at_home==FALSE)goto_machine_home();
  win_testUI_flag=FALSE;
  win_settingsUI_flag=FALSE;
  gtk_widget_queue_draw(GTK_WIDGET(win_main));
  return;

}



void slot_done_callback(GtkWidget *btn, gpointer dead_window)
{

  gpioWrite(TOOL_A_SEL,1); 
  gpioWrite(TOOL_A_ACT,1); 

  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    gpioWrite(TOOL_B_SEL,1); 
    gpioWrite(TOOL_C_SEL,1); 
    gpioWrite(TOOL_D_SEL,1); 
    gpioWrite(TOOL_B_ACT,1); 
    gpioWrite(TOOL_C_ACT,1); 
    gpioWrite(TOOL_D_ACT,1); 
  #endif
  
  gtk_window_close(GTK_WINDOW(dead_window));
  
  return;
}

void grab_slot_value(GtkSpinButton *button, gpointer user_data)
{
  int tool_sel,tool_fan;
  
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    gpioWrite(TOOL_A_SEL,1); 						// turn everything off
    gpioWrite(TOOL_B_SEL,1); 
    gpioWrite(TOOL_C_SEL,1); 
    gpioWrite(TOOL_D_SEL,1); 
    gpioWrite(TOOL_A_ACT,1); 
    gpioWrite(TOOL_B_ACT,1); 
    gpioWrite(TOOL_C_ACT,1); 
    gpioWrite(TOOL_D_ACT,1); 
    if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
    if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
    if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
    if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
    if(test_element==1)gpioWrite(tool_sel,0); 				// if set to carriage
    if(test_element==2)gpioWrite(tool_fan,0); 				// if set to tool
  #endif
  
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  #endif
  
  previous_tool=current_tool;
  current_tool=test_slot;
  
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}

void grab_cdist_value(GtkSpinButton *button, gpointer user_data)
{
  
  move_cdist=gtk_spin_button_get_value(button);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void slot_chome_callback(GtkWidget *btn, gpointer user_data)
{
  int slot,h;
  int tool_sel,tool_fan;

  slot=GPOINTER_TO_INT(user_data);

  gpioWrite(TOOL_A_ACT,1); 
  gpioWrite(TOOL_B_ACT,1); 
  gpioWrite(TOOL_C_ACT,1); 
  gpioWrite(TOOL_D_ACT,1); 

  // get tool default values
  load_tool_defaults(test_slot);

  // move all tools up a nudge to ensure they are not on their limit switches
  printf("\n\nStatus: Homing slot %d...",test_slot);
  if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
  if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
  if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
  gpioWrite(tool_sel,0); 
  tool_tip_home(test_slot);
  //gpioWrite(tool_sel,1); 
  
  sprintf(scratch,"LimitSw OFF");
  if(RPi_GPIO_Read(TOOL_LIMIT))sprintf(scratch,"LimitSw ON ");
  gtk_label_set_text(GTK_LABEL(lbl_slot_stat),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(lbl_slot_stat));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}

void slot_cmove_callback(GtkWidget *btn, gpointer user_data)
{
  int slot,h;
  int tool_sel,tool_fan;

  slot=GPOINTER_TO_INT(user_data);

  gpioWrite(TOOL_A_ACT,1); 
  gpioWrite(TOOL_B_ACT,1); 
  gpioWrite(TOOL_C_ACT,1); 
  gpioWrite(TOOL_D_ACT,1); 

  // get tool default values
  load_tool_defaults(test_slot);

  // move all tools up a nudge to ensure they are not on their limit switches
  printf("\n\nStatus: Moving slot %d...",test_slot);
  if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
  if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
  if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
  gpioWrite(tool_sel,0); 
  sprintf(gcode_cmd,"G28.3 A0.0 \n");	
  tinyGSnd(gcode_cmd);
  sprintf(gcode_cmd,"G1 A%07.3f F250 \n",move_cdist);			// move head up off switch into retracted position
  tinyGSnd(gcode_cmd);
  motion_complete();
  //gpioWrite(tool_sel,1); 

  sprintf(scratch,"LimitSw OFF");
  if(RPi_GPIO_Read(TOOL_LIMIT))sprintf(scratch,"LimitSw ON ");
  gtk_label_set_text(GTK_LABEL(lbl_slot_stat),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(lbl_slot_stat));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void grab_tdist_value(GtkSpinButton *button, gpointer user_data)
{
  
  move_tdist=gtk_spin_button_get_value(button);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void slot_thome_callback(GtkWidget *btn, gpointer user_data)
{
  int slot,h;
  int tool_sel,tool_fan;

  slot=GPOINTER_TO_INT(user_data);

  gpioWrite(TOOL_A_ACT,1); 
  gpioWrite(TOOL_B_ACT,1); 
  gpioWrite(TOOL_C_ACT,1); 
  gpioWrite(TOOL_D_ACT,1); 

  // get tool default values
  load_tool_defaults(test_slot);

  // move all tools up a nudge to ensure they are not on their limit switches
  printf("\n\nStatus: Homing slot %d...",test_slot);
  if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
  if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
  if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
  gpioWrite(tool_fan,0); 
  //tool_tip_home(test_slot);
  //gpioWrite(tool_fan,1); 
  
  sprintf(scratch,"LimitSw OFF");
  if(RPi_GPIO_Read(AUX1_INPUT))sprintf(scratch,"LimitSw ON ");
  gtk_label_set_text(GTK_LABEL(lbl_slot_stat),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(lbl_slot_stat));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  
  return;
}

void slot_tmove_callback(GtkWidget *btn, gpointer user_data)
{
  int slot,h;
  int tool_sel,tool_fan;

  slot=GPOINTER_TO_INT(user_data);

  gpioWrite(TOOL_A_ACT,1); 
  gpioWrite(TOOL_B_ACT,1); 
  gpioWrite(TOOL_C_ACT,1); 
  gpioWrite(TOOL_D_ACT,1); 

  // get tool default values
  load_tool_defaults(test_slot);

  // move all tools up a nudge to ensure they are not on their limit switches
  printf("\n\nStatus: Moving tool %d...",slot);
  if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
  if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
  if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
  gpioWrite(tool_fan,0); 
  //sprintf(gcode_cmd,"G28.3 A0.0 \n");	
  //tinyGSnd(gcode_cmd);
  sprintf(gcode_cmd,"G1 A%07.3f F250 \n",move_tdist);			// drive tool stepper
  tinyGSnd(gcode_cmd);
  motion_complete();
  //gpioWrite(tool_fan,1); 

  sprintf(scratch,"LimitSw OFF");
  if(RPi_GPIO_Read(AUX1_INPUT))sprintf(scratch,"LimitSw ON ");
  gtk_label_set_text(GTK_LABEL(lbl_slot_stat),scratch);
  gtk_widget_queue_draw(GTK_WIDGET(lbl_slot_stat));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}

  return;
}


// Callback to get scan distance resolution
void scan_dist_value(GtkSpinButton *button, gpointer user_data)
{
  float 	min_scan_dist,max_scan_dist;
  
  // determine the minimum allowable scan distance based on table sizes and array sizes
  min_scan_dist=(BUILD_TABLE_LEN_X/SCAN_X_MAX);
  if((BUILD_TABLE_LEN_Y/SCAN_Y_MAX) > min_scan_dist) min_scan_dist=(BUILD_TABLE_LEN_Y/SCAN_Y_MAX);
  if(min_scan_dist < 1.0)min_scan_dist=1.0;
  
  // determine the maximum allowable scan distance based on table sizes
  max_scan_dist=BUILD_TABLE_LEN_X;
  if(BUILD_TABLE_LEN_Y < max_scan_dist) max_scan_dist=BUILD_TABLE_LEN_Y;
  if(max_scan_dist > 250.0)max_scan_dist=250.0;
  
  // get new scan dist value
  scan_dist=gtk_spin_button_get_value(button);
  if(scan_dist<min_scan_dist)scan_dist=min_scan_dist;
  if(scan_dist>max_scan_dist)scan_dist=max_scan_dist;
  
  // calc new array size and edge remainders so that the scan is centered on the table
  scan_xmax=(int)(BUILD_TABLE_LEN_X/scan_dist);				// number of readings in X at current scan dist
  if(scan_xmax < 0)scan_xmax=0;
  if(scan_xmax >= SCAN_X_MAX)scan_xmax=SCAN_X_MAX-1;			// ensure it does not exceed array space
  scan_x_remain=(BUILD_TABLE_LEN_X-(scan_xmax*scan_dist))/2;		// remainder in X at each extreme
  
  scan_ymax=(int)(BUILD_TABLE_LEN_Y/scan_dist);				// number of readings in Y at current scan dist
  if(scan_ymax < 0)scan_ymax=0;
  if(scan_ymax >= SCAN_Y_MAX)scan_ymax=SCAN_Y_MAX-1;			// ensure it does not exceed array space
  scan_y_remain=(BUILD_TABLE_LEN_Y-(scan_ymax*scan_dist))/2;		// remainder in Y at each extreme
  
  return;
}

void scan_callback(GtkWidget *btn, gpointer dead_window)
{

  gtk_window_close(GTK_WINDOW(dead_window));
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  set_view_build_lvl=TRUE;
  job.state=JOB_TABLE_SCAN;
  build_table_step_scan(0,1);						// scan using physical probe tool
  //build_table_sweep_scan(current_tool,2);				// scan using noncontact sensor on laser tool
  job.state=UNDEFINED;

  return;
}

static void slotbtn_toggled_cb (GtkWidget *button, gpointer user_data)
{
  int 	tool_sel,tool_fan;

  test_element=GPOINTER_TO_INT(user_data);
  
  #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
    gpioWrite(TOOL_A_SEL,1); 						// turn everything off
    gpioWrite(TOOL_B_SEL,1); 
    gpioWrite(TOOL_C_SEL,1); 
    gpioWrite(TOOL_D_SEL,1); 
    gpioWrite(TOOL_A_ACT,1); 
    gpioWrite(TOOL_B_ACT,1); 
    gpioWrite(TOOL_C_ACT,1); 
    gpioWrite(TOOL_D_ACT,1); 
    if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
    if(test_slot==1){tool_sel=TOOL_B_SEL;tool_fan=TOOL_B_ACT;}
    if(test_slot==2){tool_sel=TOOL_C_SEL;tool_fan=TOOL_C_ACT;}
    if(test_slot==3){tool_sel=TOOL_D_SEL;tool_fan=TOOL_D_ACT;}
    if(test_element==1)gpioWrite(tool_sel,0); 				// if set to carriage
    if(test_element==2)gpioWrite(tool_fan,0); 				// if set to tool
  #endif
  
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    if(test_slot==0){tool_sel=TOOL_A_SEL;tool_fan=TOOL_A_ACT;}		// define which GPIO addresses to use
  #endif
  
  previous_tool=current_tool;
  current_tool=test_slot;
  
  return;
}

void make_test_move(int slot, vertex *vptr)
{
  if(carriage_at_home==TRUE)
    {
    comefrom_machine_home(slot,vptr);
    }
  else
    {
    sprintf(gcode_cmd,"G0 X%7.3f Y%7.3f Z%7.3f \n",vptr->x,vptr->y,vptr->z);
    tinyGSnd(gcode_cmd);
    while(tinyGRcv(1)>=0);
    motion_complete();
    }
  
  return;
}

void grab_xdist_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot=0;
  vertex 	*vptr;
  
  move_x=gtk_spin_button_get_value(button);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  vptr=vertex_make();
  vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
  make_test_move(slot,vptr);
  free(vptr); vertex_mem--;
    
  return;
}

void grab_ydist_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot=0;
  vertex 	*vptr;
  
  move_y=gtk_spin_button_get_value(button);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  vptr=vertex_make();
  vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
  make_test_move(slot,vptr);
  free(vptr); vertex_mem--;
    
  return;
}

void grab_zdist_value(GtkSpinButton *button, gpointer user_data)
{
  int		slot=0;
  vertex 	*vptr;
  
  move_z=gtk_spin_button_get_value(button);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
    if(Tool[0].state>=TL_LOADED)
      {
      if(move_z<Tool[0].tip_dn_pos)move_z=Tool[0].tip_dn_pos;
      }
  #endif
  vptr=vertex_make();
  vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
  make_test_move(slot,vptr);
  free(vptr); vertex_mem--;
    
  return;
}

// Function to home an axis
int on_home_callback(GtkWidget *btn_call, gpointer user_data)
{
    int	axis;
    
    // 1==X, 10==Y, 100==Z, 11==X&Y, 101==X&Z, 110==Y&Z, 111==X&Y&Z
    axis=GPOINTER_TO_INT(user_data);

    if(axis==1)
      {
      sprintf(gcode_cmd,"G28.2 X0 \n");
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      move_x=0;
      gtk_adjustment_set_value (xdistadj,move_x);
      }
    if(axis==10)
      {
      sprintf(gcode_cmd,"G28.2 Y0 \n");
      tinyGSnd(gcode_cmd);
      while(tinyGRcv(1)>=0);
      move_y=0;
      gtk_adjustment_set_value (ydistadj,move_y);
      }
    if(axis==100)
      {
      // code to adjust z such that build table is near "zero"
      #if defined(ALPHA_UNIT) || defined(BETA_UNIT)			// alpha and beta units have home near z=minimum
	sprintf(gcode_cmd,"G1 Z%7.3f F300.0 \n",ZHOMESET);
	tinyGSnd(gcode_cmd);
	while(tinyGRcv(1)>=0);
	sprintf(gcode_cmd,"G28.3 Z0.0 \n");
	tinyGSnd(gcode_cmd);
	while(tinyGRcv(1)>=0);
	move_z=0.0;
      #endif
    
      #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)			// gamma and delta units have home near z=maximum
	sprintf(gcode_burst,"G28.2 Z0 \n");				// request move to hit limit switch
	tinyGSnd(gcode_burst);
	while(tinyGRcv(1)>=0);
	sprintf(gcode_cmd,"G28.3 Z%7.3f \n",ZHOMESET);			// set this value as the 
	tinyGSnd(gcode_cmd);
	while(tinyGRcv(1)>=0);
	PosIs.z=PostG.z;
	move_z=ZHOMESET;
      #endif
      
      gtk_adjustment_set_value (zdistadj,move_z);
    
      }

    return(1);
}

// Function to put axis at minium in build volue
int on_min_callback(GtkWidget *btn_call, gpointer user_data)
{
    int		axis,slot=0;
    vertex 	*vptr;
    
    // 1==X, 10==Y, 100==Z, 11==X&Y, 101==X&Z, 110==Y&Z, 111==X&Y&Z
    axis=GPOINTER_TO_INT(user_data);

    if(axis==1)
      {
      move_x=BUILD_TABLE_MIN_X;
      gtk_adjustment_set_value (xdistadj,move_x);
      }
    if(axis==10)
      {
      move_y=BUILD_TABLE_MIN_Y;
      gtk_adjustment_set_value (ydistadj,move_y);
      }
    if(axis==100)
      {
      move_z=BUILD_TABLE_MIN_Z;
      #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	if(Tool[0].state>=TL_LOADED)
	  {
	  if(move_z<Tool[0].tip_dn_pos)move_z=Tool[0].tip_dn_pos;
	  }
      #endif
      gtk_adjustment_set_value (zdistadj,move_z);
      }

    vptr=vertex_make();
    vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
    make_test_move(slot,vptr);
    free(vptr); vertex_mem--;
    
    return(1);
}

// Function to put axis in middle of build volue
int on_mid_callback(GtkWidget *btn_call, gpointer user_data)
{
    int		axis,slot=0;
    vertex 	*vptr;
    
    // 1==X, 10==Y, 100==Z, 11==X&Y, 101==X&Z, 110==Y&Z, 111==X&Y&Z
    axis=GPOINTER_TO_INT(user_data);
    
    if(axis==1)
      {
      move_x=(BUILD_TABLE_MAX_X-BUILD_TABLE_MIN_X)/2;
      gtk_adjustment_set_value (xdistadj,move_x);
      }
    if(axis==10)
      {
      move_y=(BUILD_TABLE_MAX_Y-BUILD_TABLE_MIN_Y)/2;
      gtk_adjustment_set_value (ydistadj,move_y);
      }
    if(axis==100)
      {
      move_z=(BUILD_TABLE_MAX_Z-BUILD_TABLE_MIN_Z)/2;
      gtk_adjustment_set_value (zdistadj,move_z);
      }

    vptr=vertex_make();
    vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
    make_test_move(slot,vptr);
    free(vptr); vertex_mem--;
    
    return(1);
}

// Function to put axis at maximum in build volue
int on_max_callback(GtkWidget *btn_call, gpointer user_data)
{
    int		axis,slot=0;
    vertex 	*vptr;
    
    // 1==X, 10==Y, 100==Z, 11==X&Y, 101==X&Z, 110==Y&Z, 111==X&Y&Z
    axis=GPOINTER_TO_INT(user_data);

    if(axis==1)
      {
      move_x=BUILD_TABLE_MAX_X;
      gtk_adjustment_set_value (xdistadj,move_x);
      }
    if(axis==10)
      {
      move_y=BUILD_TABLE_MAX_Y;
      gtk_adjustment_set_value (ydistadj,move_y);
      }
    if(axis==100)
      {
      move_z=ZHOMESET;
      gtk_adjustment_set_value (zdistadj,move_z);
      }

    vptr=vertex_make();
    vptr->x=move_x; vptr->y=move_y; vptr->z=move_z;
    make_test_move(slot,vptr);
    free(vptr); vertex_mem--;
    
    return(1);
}

// Function to set a tools 48v duty cycle (i.e. voltage)
void grab_t48v_value(GtkSpinButton *button, gpointer user_data)
{
  int		on_ticks;
  float 	duty_cycle;
  
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 48v ADJ: No tool set.\n"); return;}

  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"48v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t48v_duty=gtk_spin_button_get_value(button);
  if(t48v_duty<0)t48v_duty=0.0;
  if(t48v_duty>100)t48v_duty=100.0;
  Tool[test_slot].powr.PWM_port=test_slot+8;
  printf("TEST ADJ: 48v duty cycle being set to %f on PWM pin %d \n",t48v_duty,Tool[test_slot].powr.PWM_port);
  if(t48v_duty>0)
    {
    duty_cycle=0.001;
    while(duty_cycle<(t48v_duty/100))
      {
      on_ticks=(int)(MAX_PWM*duty_cycle);			
      pca9685PWMWrite(I2C_PWM_fd,Tool[test_slot].powr.PWM_port,0,on_ticks);
      duty_cycle+=0.05;
      delay(50);
      }
    }
  else 
    {
    pca9685PWMWrite(I2C_PWM_fd,Tool[test_slot].powr.PWM_port,0,0);
    }
  printf("TEST ADJ: 48v duty cycle set to %f - done.\n",t48v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}

// Function to put 48v at minium duty cycle
void on_t48v_off_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 48v MIN: No tool set.\n"); return;}

  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"48v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t48v_duty=0.0;
  gtk_adjustment_set_value (t48vadj,t48v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  printf("TEST MIN: 48v duty cycle set to %f\n",t48v_duty);
  return;
}

// Function to put 48v at maximum duty cycle
void on_t48v_full_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 48v MAX: No tool set.\n"); return;}

  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"48v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t48v_duty=100.0;
  gtk_adjustment_set_value (t48vadj,t48v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  printf("TEST MAX: 48v duty cycle set to %f\n",t48v_duty);
  return;
}


// Function to set a tools 24v duty cycle (i.e. voltage)
void grab_t24v_value(GtkSpinButton *button, gpointer user_data)
{
  int		on_ticks;
  float 	duty_cycle;
  
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 24v ADJ: No tool set.\n"); return;}

  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"24v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t24v_duty=gtk_spin_button_get_value(button);
  if(t24v_duty<0.0)t24v_duty=0.0;
  if(t24v_duty>100.0)t24v_duty=100.0;
  Tool[test_slot].thrm.PWM_port=test_slot;
  printf("TEST ADJ: 24v duty cycle being set to %f on PWM pin %d \n",t24v_duty,Tool[test_slot].thrm.PWM_port);
  if(t24v_duty>0)
    {
    duty_cycle=0.001;
    while(duty_cycle<(t24v_duty/100))
      {
      on_ticks=(int)(MAX_PWM*duty_cycle);			
      pca9685PWMWrite(I2C_PWM_fd,Tool[test_slot].thrm.PWM_port,0,on_ticks);
      duty_cycle+=0.05;
      delay(50);
      }
    }
  else 
    {
    pca9685PWMWrite(I2C_PWM_fd,Tool[test_slot].thrm.PWM_port,0,0);
    }
  printf("TEST ADJ: 24v duty cycle set to %f - done.\n",t24v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}

// Function to put 24v at minium duty cycle
void on_t24v_off_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 24v MIN: No tool set.\n"); return;}
  
  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"24v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t24v_duty=0.0;
  gtk_adjustment_set_value (t24vadj,t24v_duty);
  printf("TEST MIN: 24v duty cycle set to %f\n",t24v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}

// Function to put 24v at maximum duty cycle
void on_t24v_full_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST 24v MAX: No tool set.\n"); return;}
  
  // must turn off conflict with thermal thread if using this...
  if(win_testUI_flag==FALSE)
    {
    sprintf(scratch," \nThis will turn off thermal control. ");
    strcat (scratch," \nContinue? \n");
    aa_dialog_box(win_settings,3,0,"24v Test",scratch);
    while(aa_dialog_result<0){g_main_context_iteration(NULL, FALSE);}
    if(aa_dialog_result==FALSE)return;
    }
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");

  t24v_duty=100.0;
  gtk_adjustment_set_value (t24vadj,t24v_duty);
  printf("TEST MAX: 24v duty cycle set to %f\n",t24v_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}


// Function to set a tools PWM duty cycle (i.e. AUX3 pin)
void grab_PWMv_value(GtkSpinButton *button, gpointer user_data)
{
  int		on_ticks;
  float 	duty_cycle;
  
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST PWM ADJ: No tool set.\n"); return;}
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");
  PWMv_duty=gtk_spin_button_get_value(button);
  if(PWMv_duty<0.0)PWMv_duty=0.0;
  if(PWMv_duty>100.0)PWMv_duty=100.0;
  //Tool[test_slot].thrm.PWM_port=test_slot;
  printf("TEST ADJ: PWM duty cycle being set to %f on pin %d \n",PWMv_duty,aux3_port);
  if(PWMv_duty>0)
    {
    duty_cycle=0.001;
    while(duty_cycle<(PWMv_duty/100))
      {
      on_ticks=(int)(MAX_PWM*duty_cycle);			
      pca9685PWMWrite(I2C_PWM_fd,aux3_port,0,on_ticks);
      duty_cycle+=0.05;
      delay(50);
      }
    }
  else 
    {
    pca9685PWMWrite(I2C_PWM_fd,aux3_port,0,0);
    }
  win_testUI_flag=FALSE; printf("\nTEST:  Thermal thread set to active.\n");
  printf("TEST ADJ: PWM duty cycle set to %f - done.\n",PWMv_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}

// Function to put PWM at minium duty cycle
void on_PWMv_off_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST PWM MIN: No tool set.\n"); return;}
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");
  PWMv_duty=0.0;
  gtk_adjustment_set_value (PWMvadj,PWMv_duty);
  win_testUI_flag=FALSE; printf("\nTEST:  Thermal thread set to active.\n");
  printf("TEST MIN: PWM duty cycle set to %f\n",PWMv_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}

// Function to put PWM at maximum duty cycle
void on_PWMv_full_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(test_slot<0 || test_slot>=MAX_TOOLS){printf("TEST PWM MAX: No tool set.\n"); return;}
  win_testUI_flag=TRUE; printf("\nTEST:  Thermal thread set to inactive.\n");
  PWMv_duty=100.0;
  gtk_adjustment_set_value (PWMvadj,PWMv_duty);
  win_testUI_flag=FALSE; printf("\nTEST:  Thermal thread set to active.\n");
  printf("TEST MAX: PWM duty cycle set to %f\n",PWMv_duty);
  while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
  return;
}


// Function to turn tool cooling fan on
void on_tool_air_on_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(current_tool<0 || current_tool>=MAX_TOOLS){printf("TEST Cooling Fan: No tool set.\n"); return;}
  tool_air(current_tool,ON);
  return;
}

// Function to turn tool cooling fan off
void on_tool_air_off_callback(GtkWidget *btn_call, gpointer user_data)
{
  if(current_tool<0 || current_tool>=MAX_TOOLS){printf("TEST Cooling Fan: No tool set.\n"); return;}
  tool_air(current_tool,OFF);
  return;
}

// Function to turn line laser on
void on_line_laser_on_callback(GtkWidget *btn_call, gpointer user_data)
{
  //gpioWrite(LINE_LASER,!ON);						// turn on line laser
  gpioWrite(CARRIAGE_LED,!ON);						// turn on carriage led
  return;
}

// Function to turn line laser off
void on_line_laser_off_callback(GtkWidget *btn_call, gpointer user_data)
{
  //gpioWrite(LINE_LASER,!OFF);						// turn off line laser
  gpioWrite(CARRIAGE_LED,!OFF);						// turn off carriage led
  return;
}

// Function to test aux input
void on_aux_input_test_callback(GtkWidget *btn_call, gpointer user_data)
{
  time_t	tbegin,tend;
  
  tbegin=time(NULL);
  tend=time(NULL);
  while(RPi_GPIO_Read(AUX1_INPUT)==FALSE && (tend-tbegin)<20)
    {
    tend=time(NULL);
    printf("Listening for aux input trigger... %d secs left\n",(20-(tend-tbegin)));
    }
  if((tend-tbegin)<20){printf("\nSensor trigger detected!\n");}
  else {printf("\nSensor NOT detected!\n\n");}
  return;
}

// Function to test globals
void on_global_test_cb(GtkWidget *btn_call, gpointer user_data)
{
  printf("\n\nGlobal test clicked!\n");
  return;
}

// setting notebook page switch
static void on_setnb_page_switch (GtkWidget *g_notebook, GtkWidget *page, gint page_num)
{
    int		slot;

    //printf("SETTINGS:  Notebook page switch to %d \n",page_num);

    return;
}


// SETTINGS UI main
int settings_UI(GtkWidget *btn_call, gpointer user_data)
{
    int		i,ival,lbl_length=32;
    int		slot,axis,active_tab;
    float 	fval,minzpos;
    GtkWidget	*hbox, *vbox, *vboxmid, *hbox_up, *hbox_dn;
    GtkWidget	*settings_notebook;
    GtkWidget	*img_exit, *btn_exit;
    GtkWidget	*img_test,*btn_chng,*img_chng;
    GtkWidget	*label;
    
    // widgets for 2D viewing
    GtkWidget	*view2D_frame,*grd_view2D,*lbl_2Dview;
    GtkWidget	*lbl_start_ht, *lbl_end_ht, *lbl_inc_ht;
    GtkWidget	*btn_view_endpts, *btn_view_vecnum, *btn_view_raw_mdl, *btn_view_raw_stl;
    GtkWidget	*btn_view_strskl, *btn_view_lt, *btn_show_only_current, *btn_show_entire_stack;
    GtkWidget	*btn_build_lvl, *btn_view_blvl, *btn_view_penup, *btn_view_toolpos;
    GtkWidget	*btn_poly_ids, *btn_poly_fam, *btn_poly_dist, *btn_poly_perim, *btn_poly_area;
    
    // widgets for 3D viewing
    GtkWidget 	*view3D_frame,*grd_view3D,*lbl_3Dview;
    GtkWidget	*btn_show_normals,*btn_show_models,*btn_show_supports;
    GtkWidget	*btn_view_patches, *btn_view_bounds, *btn_view_facets, *btn_view_wireframe;
    GtkWidget	*btn_view_image,*btn_view_slice, *btn_superimpose, *btn_per_scale;
    
    // widgets for graphing
    GtkWidget	*graph_frame,*grd_graph,*lbl_graph_params;
    GtkWidget	*btn_set_in_use[10],*btn_set_as_Y[10];
    GtkWidget 	*btn_graph_type;

    // widgets for model parameters
    GtkWidget 	*mdl_frame,*grd_mdl,*lbl_model_params;
    GtkWidget	*colinear_angle_btn,*wavefront_inc_btn;

    // widgets for image parameters
    GtkWidget 	*img_frame,*grd_img,*lbl_image_params;

    // widgets for fabrication/printing parameters
    GtkWidget 	*fab_frame,*grd_fab,*lbl_print_params;
    GtkWidget 	*fab_left_box,*genrl_frame,*grd_genrl,*cnc_frame,*grd_cnc;
    GtkWidget 	*btn_center_job,*btn_start_at_z,*btn_ignore_lt_temps,*btn_btm_lyr_details;
    GtkWidget 	*btn_ignore_bit, *btn_ignore_drill,*btn_small_drill,*btn_add_block,*btn_profile_cut;
    GtkWidget	*btn_force_z_to_table;
    GtkWidget	*lbl_general,*lbl_cnc,*lbl_block_dims;
    
    // widgets for system parameters
    GtkWidget 	*unt_frame,*unt_left_box,*lbl_system_params;
    GtkWidget 	*unt_genr_frame,*grd_unt_genr;
    GtkWidget 	*unt_thrm_frame,*grd_unt_thrm;
    GtkWidget	*btn_save_image_hist, *btn_save_job_hist;

    // widgets for calibration parameters
    GtkWidget	*cal_left_box,*lbl_system_calib;
    GtkWidget 	*btn_scan;
    GtkWidget 	*bld_tbl_cal_frame,*grd_bld_tbl_cal;
    GtkWidget 	*carriage_cal_frame,*grd_carriage_cal;
    GtkWidget	*btn_calib_slots,*btn_calib_zlevel;

    // widgets for test parameters
    GtkWidget 	*test_left_box,*lbl_system_test;
    GtkWidget 	*slot_frame,*grd_slot,*slotbtn;
    GtkAdjustment	*slotadj;
    GtkWidget	*artic_frame,*grd_artic;
    GtkWidget	*lbl_scratch;
    GtkWidget 	*cdistbtn,*btn_ctest,*btn_chome;
    GtkAdjustment	*cdistadj;
    GtkWidget	*tfeed_frame,*grd_tfeed;
    GtkWidget	*tdistbtn,*btn_ttest,*btn_thome;
    GtkAdjustment	*tdistadj;
    GtkWidget	*xyz_frame,*grd_xyz;
    GtkWidget	*btn_xhome,*btn_yhome,*btn_zhome;
    GtkWidget	*btn_xmin,*btn_ymin,*btn_zmin;
    GtkWidget 	*btn_xmid,*btn_ymid,*btn_zmid;
    GtkWidget	*btn_xmax,*btn_ymax,*btn_zmax;
    GtkWidget	*xdistbtn,*ydistbtn,*zdistbtn;
    GtkWidget	*tio_frame,*grd_tio;
    GtkWidget	*btn_t48v_off,*btn_t48v_full;
    GtkWidget	*btn_t24v_off,*btn_t24v_full;
    GtkWidget	*btn_fan_on,*btn_fan_off;
    GtkWidget	*btn_line_laser_on,*btn_line_laser_off;
    GtkWidget 	*btn_aux_input_test;
	
    GtkWidget 	*rbtn_carg, *rbtn_tool, *rbtn_none;
    
    // set global flag to indicate we are in settings UI
    win_settingsUI_flag=TRUE;
    //win_testUI_flag=TRUE;
    
    // get target tab to display
    // switch made below after window and notebook are built
    //active_tab=GPOINTER_TO_INT(user_data);
    //if(active_tab<0)active_tab=0;
    //if(active_tab>7)active_tab=7;
    if(main_view_page==VIEW_MODEL)active_tab=1;	
    if(main_view_page==VIEW_TOOL)active_tab=4;	
    if(main_view_page==VIEW_LAYER)active_tab=0;	
    if(main_view_page==VIEW_GRAPH)active_tab=2;	
    if(main_view_page==VIEW_CAMERA)active_tab=5;
  
    // set up settings interface window
    win_settings = gtk_window_new ();
    gtk_window_set_transient_for(GTK_WINDOW(win_settings), GTK_WINDOW(win_main));
    gtk_window_set_modal(GTK_WINDOW(win_settings),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_settings),LCD_WIDTH-20,LCD_HEIGHT-20);
    gtk_window_set_resizable(GTK_WINDOW(win_settings),TRUE);
    gtk_window_set_title(GTK_WINDOW(win_settings),"Nx3D - Settings");			
    
    // set up an hbox to divide the screen into a narrower segments
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL,2);
    gtk_window_set_child(GTK_WINDOW(win_settings),hbox);
    //gtk_box_set_spacing (GTK_BOX(hbox),20);

    // now divide left side into segments for buttons
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
    gtk_box_append(GTK_BOX(hbox),vbox);
    
    // set up box on right side for notebook tabs
    vboxmid = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
    gtk_box_append(GTK_BOX(hbox),vboxmid);
    
    // set up left side buttons
    {
      // set up EXIT button
      btn_exit = gtk_button_new ();
      g_signal_connect (btn_exit, "clicked", G_CALLBACK(on_settings_exit),win_settings);
      img_exit = gtk_image_new_from_file("Back.gif");
      gtk_image_set_pixel_size(GTK_IMAGE(img_exit),50);
      gtk_button_set_child(GTK_BUTTON(btn_exit),img_exit);
      gtk_box_append(GTK_BOX(vbox),btn_exit);
    }

    // set up notebook
    {
      settings_notebook = gtk_notebook_new ();
      gtk_box_append(GTK_BOX(vboxmid),settings_notebook);
      g_signal_connect (settings_notebook, "switch_page",G_CALLBACK (on_setnb_page_switch), NULL);
      gtk_widget_set_size_request(settings_notebook,LCD_WIDTH-150,LCD_HEIGHT-130);
      gtk_notebook_set_show_border(GTK_NOTEBOOK(settings_notebook),TRUE);
    }

    // set up a view2D_frame on left side of screen to encapsulate 2D view settings
    {
      GtkWidget *view2D_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(view2D_box,LCD_WIDTH-170,LCD_HEIGHT-150);
      lbl_2Dview=gtk_label_new("2D View");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),view2D_box,lbl_2Dview);

      // set up vector types
      {
	GtkWidget *vec_type_frame = gtk_frame_new("Vector Type");
	gtk_box_append(GTK_BOX(view2D_box),vec_type_frame);
	gtk_widget_set_size_request(vec_type_frame,50,75);
	
	GtkWidget *vec_type_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(vec_type_frame),vec_type_box);
	
	GtkWidget *grd_vec_type = gtk_grid_new ();
	gtk_box_append(GTK_BOX(vec_type_box),grd_vec_type);
	gtk_grid_set_row_spacing (GTK_GRID(grd_vec_type),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_vec_type),20);
	
	sprintf(scratch,"Show stl vectors");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_raw_stl = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_type), btn_view_raw_stl, 0, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_raw_stl), set_view_raw_stl);
	g_signal_connect(btn_view_raw_stl, "toggled", G_CALLBACK (btn_rawstl_toggled_cb), NULL);
	
	sprintf(scratch,"Show raw vectors");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_raw_mdl = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_type), btn_view_raw_mdl, 0, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_raw_mdl), set_view_raw_mdl);
	g_signal_connect(btn_view_raw_mdl, "toggled", G_CALLBACK (btn_rawmdl_toggled_cb), NULL);
	
	sprintf(scratch,"Show straight skeleton");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_strskl  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_type), btn_view_strskl,  1, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_strskl),  set_view_strskl);
	g_signal_connect(btn_view_strskl,  "toggled", G_CALLBACK (btn_strskl_toggled_cb), NULL);
  
	sprintf(scratch,"Show tool up moves");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_penup   = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_type), btn_view_penup,   1, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_penup),   set_view_penup_moves);
	g_signal_connect(btn_view_penup,   "toggled", G_CALLBACK (btn_penup_toggled_cb), NULL);
      }

      // set up vector information
      {
	GtkWidget *vec_info_frame = gtk_frame_new("Vector Information");
	gtk_box_append(GTK_BOX(view2D_box),vec_info_frame);
	gtk_widget_set_size_request(vec_info_frame,50,75);
	
	GtkWidget *vec_info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(vec_info_frame),vec_info_box);
	
	GtkWidget *grd_vec_info = gtk_grid_new ();
	gtk_box_append(GTK_BOX(vec_info_box),grd_vec_info);
	gtk_grid_set_row_spacing (GTK_GRID(grd_vec_info),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_vec_info),20);
  
	sprintf(scratch,"Show endpoints");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_endpts  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_info), btn_view_endpts,  0, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_endpts),  set_view_endpts);
	g_signal_connect(btn_view_endpts,  "toggled", G_CALLBACK (btn_endpts_toggled_cb), NULL);
  
	sprintf(scratch,"Show sequence numbers");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_vecnum  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_info), btn_view_vecnum,  1, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_vecnum),  set_view_vecnum);
	g_signal_connect(btn_view_vecnum,  "toggled", G_CALLBACK (btn_vecnum_toggled_cb), NULL);

	sprintf(scratch,"Show length");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	GtkWidget *btn_view_veclen  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_info), btn_view_veclen,  0, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_veclen),  set_view_veclen);
	g_signal_connect(btn_view_veclen,  "toggled", G_CALLBACK (btn_veclen_toggled_cb), NULL);
	
	sprintf(scratch,"Show location");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	GtkWidget *btn_view_vecloc  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_vec_info), btn_view_vecloc,  1, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_vecloc),  set_view_vecloc);
	g_signal_connect(btn_view_vecloc,  "toggled", G_CALLBACK (btn_vecloc_toggled_cb), NULL);
	
      }

      // set up polygon information
      {
	GtkWidget *poly_info_frame = gtk_frame_new("Polygon Information");
	gtk_box_append(GTK_BOX(view2D_box),poly_info_frame);
	gtk_widget_set_size_request(poly_info_frame,50,75);
	
	GtkWidget *poly_info_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(poly_info_frame),poly_info_box);
	
	GtkWidget *grd_poly_info = gtk_grid_new ();
	gtk_box_append(GTK_BOX(poly_info_box),grd_poly_info);
	gtk_grid_set_row_spacing (GTK_GRID(grd_poly_info),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_poly_info),20);
  
	sprintf(scratch,"Show IDs");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_poly_ids  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_poly_info), btn_poly_ids,  0, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_poly_ids),  set_view_poly_ids);
	g_signal_connect(btn_poly_ids,  "toggled", G_CALLBACK (btn_poly_ids_toggled_cb), NULL);
  
	sprintf(scratch,"Show relationships");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_poly_fam  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_poly_info), btn_poly_fam,  1, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_poly_fam),  set_view_poly_fam);
	g_signal_connect(btn_poly_fam,  "toggled", G_CALLBACK (btn_poly_fam_toggled_cb), NULL);

	sprintf(scratch,"Show offset distance");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_poly_dist  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_poly_info), btn_poly_dist,  2, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_poly_dist),  set_view_poly_dist);
	g_signal_connect(btn_poly_dist,  "toggled", G_CALLBACK (btn_poly_dist_toggled_cb), NULL);

	sprintf(scratch,"Show perimeter length");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_poly_perim  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_poly_info), btn_poly_perim,  0, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_poly_perim),  set_view_poly_perim);
	g_signal_connect(btn_poly_perim,  "toggled", G_CALLBACK (btn_poly_perim_toggled_cb), NULL);
	
	sprintf(scratch,"Show area");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_poly_area  = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_poly_info), btn_poly_area,  1, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_poly_area),  set_view_poly_area);
	g_signal_connect(btn_poly_area,  "toggled", G_CALLBACK (btn_poly_area_toggled_cb), NULL);
	
      }

      // set up layer selects
      {
	GtkWidget *layer_type_frame = gtk_frame_new("Layer Type");
	gtk_box_append(GTK_BOX(view2D_box),layer_type_frame);
	gtk_widget_set_size_request(layer_type_frame,50,75);
	
	GtkWidget *layer_type_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(layer_type_frame),layer_type_box);
	
	GtkWidget *grd_layer_type = gtk_grid_new ();
	gtk_box_append(GTK_BOX(layer_type_box),grd_layer_type);
	gtk_grid_set_row_spacing (GTK_GRID(grd_layer_type),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_layer_type),20);

	// set up slice start view height
	lbl_start_ht=gtk_label_new("Distance below current Z to show:");
	gtk_grid_attach (GTK_GRID (grd_layer_type), lbl_start_ht, 0, 0, 1, 1);
	slc_view_start_adj=gtk_adjustment_new(slc_view_start, 0, ZMax, job.min_slice_thk, 1, 5);
	slc_view_start_btn=gtk_spin_button_new(slc_view_start_adj, job.min_slice_thk, 3);
	gtk_grid_attach (GTK_GRID (grd_layer_type), slc_view_start_btn, 1, 0, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(slc_view_start_btn),"value-changed",G_CALLBACK(slc_view_start_value),NULL);
    
	// set up slice end view height
	lbl_end_ht=gtk_label_new("Distance above current Z to show:");
	gtk_grid_attach (GTK_GRID (grd_layer_type), lbl_end_ht, 0, 1, 1, 1);
	slc_view_end_adj=gtk_adjustment_new(slc_view_end, 0, ZMax, job.min_slice_thk, 1, 5);
	slc_view_end_btn=gtk_spin_button_new(slc_view_end_adj, job.min_slice_thk, 3);
	gtk_grid_attach (GTK_GRID (grd_layer_type), slc_view_end_btn, 1, 1, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(slc_view_end_btn),"value-changed",G_CALLBACK(slc_view_end_value),NULL);
    
	// set up slice view increment
	lbl_inc_ht=gtk_label_new("Finish height to show:");
	gtk_grid_attach (GTK_GRID (grd_layer_type), lbl_inc_ht, 0, 2, 1, 1);
	slc_view_inc_adj=gtk_adjustment_new(slc_view_inc, 0, ZMax, job.min_slice_thk, 1, 5);
	slc_view_inc_btn=gtk_spin_button_new(slc_view_inc_adj, job.min_slice_thk, 3);
	gtk_grid_attach (GTK_GRID (grd_layer_type), slc_view_inc_btn, 1, 2, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(slc_view_inc_btn),"value-changed",G_CALLBACK(slc_view_increment_value),NULL);
    
	// set up button to view only current slice
	sprintf(scratch,"Show only current");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_show_only_current   = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_layer_type), btn_show_only_current,   4, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_show_only_current),   set_show_above_layer);
	g_signal_connect(btn_show_only_current,   "toggled", G_CALLBACK (btn_show_only_current_toggled_cb), NULL);
	
	// set up button to view all slices together
	sprintf(scratch,"Show  entire stack");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_show_entire_stack   = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_layer_type), btn_show_entire_stack,   4, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_show_entire_stack),   set_show_below_layer);
	g_signal_connect(btn_show_entire_stack,   "toggled", G_CALLBACK (btn_show_entire_stack_toggled_cb), NULL);

      }
      
      // set up misc selects
      {
	GtkWidget *misc2D_frame = gtk_frame_new("Miscellaneous");
	gtk_box_append(GTK_BOX(view2D_box),misc2D_frame);
	gtk_widget_set_size_request(misc2D_frame,50,75);
	
	GtkWidget *misc2D_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(misc2D_frame),misc2D_box);
	
	GtkWidget *grd_misc2D = gtk_grid_new ();
	gtk_box_append(GTK_BOX(misc2D_box),grd_misc2D);
	gtk_grid_set_row_spacing (GTK_GRID(grd_misc2D),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_misc2D),20);
	
	sprintf(scratch,"Show build table level");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_blvl = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_misc2D), btn_view_blvl,    0, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_blvl),    set_view_build_lvl);
	g_signal_connect(btn_view_blvl,    "toggled", G_CALLBACK (btn_blvl_toggled_cb), NULL);
  
	sprintf(scratch,"Show tool position");
	while(strlen(scratch)<lbl_length){strcat(scratch," ");}
	btn_view_toolpos = gtk_check_button_new_with_label (scratch);
	gtk_grid_attach(GTK_GRID(grd_misc2D), btn_view_toolpos, 1, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_view_toolpos), set_view_tool_pos);
	g_signal_connect(btn_view_toolpos, "toggled", G_CALLBACK (btn_toolpos_toggled_cb), NULL);

	btn_view_lt = gtk_button_new_with_label ("Line Types");
	gtk_grid_attach(GTK_GRID(grd_misc2D), btn_view_lt, 0, 3, 1, 1);
	g_signal_connect(btn_view_lt,"clicked", G_CALLBACK (btn_view_lt_callback),win_settings);
      }
    }
    
    // set up a view3D_frame on left side of screen to encapsulate 3D view settings
    {
      GtkWidget *view3D_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(view3D_box,LCD_WIDTH-170,LCD_HEIGHT-150);
      lbl_3Dview=gtk_label_new("3D View");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),view3D_box,lbl_3Dview);
      
      // set up what geometry to show
      {
	GtkWidget *view3D_geo_frame = gtk_frame_new("Geometry");
	gtk_box_append(GTK_BOX(view3D_box),view3D_geo_frame);
	gtk_widget_set_size_request(view3D_geo_frame,50,75);
	
	GtkWidget *view3D_geo_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(view3D_geo_frame),view3D_geo_box);
	
	GtkWidget *grd_view3D_geo = gtk_grid_new ();
	gtk_box_append(GTK_BOX(view3D_geo_box),grd_view3D_geo);
	gtk_grid_set_row_spacing (GTK_GRID(grd_view3D_geo),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_view3D_geo),20);
	
	btn_show_models   = gtk_check_button_new_with_label ("Show model geometry");
	gtk_grid_attach(GTK_GRID(grd_view3D_geo), btn_show_models, 0, 0, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_show_models), set_view_models);
	g_signal_connect(btn_show_models,   "toggled", G_CALLBACK (btn_show_models_toggled_cb), NULL);
  
	btn_show_supports = gtk_check_button_new_with_label ("Show support geometry");
	gtk_grid_attach(GTK_GRID(grd_view3D_geo), btn_show_supports, 0, 1, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_show_supports), set_view_supports);
	g_signal_connect(btn_show_supports, "toggled", G_CALLBACK (btn_show_supports_toggled_cb), NULL);

	btn_show_supports = gtk_check_button_new_with_label ("Show internal geometry");
	gtk_grid_attach(GTK_GRID(grd_view3D_geo), btn_show_supports, 3, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_show_supports), set_view_supports);
	g_signal_connect(btn_show_supports, "toggled", G_CALLBACK (btn_show_internal_toggled_cb), NULL);

	btn_show_supports = gtk_check_button_new_with_label ("Show target geometry");
	gtk_grid_attach(GTK_GRID(grd_view3D_geo), btn_show_supports, 3, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_show_supports), set_view_supports);
	g_signal_connect(btn_show_supports, "toggled", G_CALLBACK (btn_show_target_toggled_cb), NULL);

      }

      // set up how to display geometry
      {
	GtkWidget *view3D_display_frame = gtk_frame_new("Display");
	gtk_box_append(GTK_BOX(view3D_box),view3D_display_frame);
	gtk_widget_set_size_request(view3D_display_frame,50,75);
	
	GtkWidget *view3D_display_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(view3D_display_frame),view3D_display_box);
	
	GtkWidget *grd_view3D_display = gtk_grid_new ();
	gtk_box_append(GTK_BOX(view3D_display_box),grd_view3D_display);
	gtk_grid_set_row_spacing (GTK_GRID(grd_view3D_display),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_view3D_display),20);
	
	btn_view_facets   = gtk_check_button_new_with_label ("Show facet model");
	gtk_grid_attach(GTK_GRID(grd_view3D_display), btn_view_facets, 0, 0, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_facets), set_facet_display);
	g_signal_connect(btn_view_facets,   "toggled", G_CALLBACK (btn_facets_toggled_cb), NULL);
  
	btn_view_wireframe= gtk_check_button_new_with_label ("Show wireframe model");
	gtk_grid_attach(GTK_GRID(grd_view3D_display), btn_view_wireframe, 0, 1, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_wireframe), set_edge_display);
	g_signal_connect(btn_view_wireframe,"toggled", G_CALLBACK (btn_wireframe_toggled_cb), NULL);

	// set up max facet display quantity spin box
	lbl_facet_qty=gtk_label_new("Max Facet Quantity X1000:");
	gtk_grid_attach (GTK_GRID (grd_view3D_display), lbl_facet_qty, 3, 0, 1, 1);
	facet_qty_adj=gtk_adjustment_new(set_view_max_facets, 1, 99, 1, 1, 15);
	facet_qty_btn=gtk_spin_button_new(facet_qty_adj, 4, 0);
	gtk_grid_attach (GTK_GRID (grd_view3D_display), facet_qty_btn, 4, 0, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(facet_qty_btn),"value-changed",G_CALLBACK(facet_qty_value),NULL);

	// set up edge view angle spin box
	lbl_edge_angle=gtk_label_new("Display Edge Angle:");
	gtk_grid_attach (GTK_GRID (grd_view3D_display), lbl_edge_angle, 3, 1, 1, 1);
	edge_angle_adj=gtk_adjustment_new(set_view_edge_angle, 0, 90, 1, 1, 15);
	edge_angle_btn=gtk_spin_button_new(edge_angle_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_view3D_display), edge_angle_btn, 4, 1, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(edge_angle_btn),"value-changed",G_CALLBACK(edge_angle_value),NULL);
    
      }
      
      // set up display details
      {
	GtkWidget *view3D_options_frame = gtk_frame_new("Options");
	gtk_box_append(GTK_BOX(view3D_box),view3D_options_frame);
	gtk_widget_set_size_request(view3D_options_frame,50,75);
	
	GtkWidget *view3D_options_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(view3D_options_frame),view3D_options_box);
	
	GtkWidget *grd_view3D_options = gtk_grid_new ();
	gtk_box_append(GTK_BOX(view3D_options_box),grd_view3D_options);
	gtk_grid_set_row_spacing (GTK_GRID(grd_view3D_options),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_view3D_options),20);

	btn_per_scale   = gtk_check_button_new_with_label ("Show perspective");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_per_scale, 0, 0, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_per_scale), Perspective_flag);
	g_signal_connect(btn_per_scale,   "toggled", G_CALLBACK (btn_per_scale_toggled_cb), NULL);
  
	btn_view_bounds   = gtk_check_button_new_with_label ("Show bounds on drag");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_view_bounds, 0, 1, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_bounds), show_view_bound_box);
	g_signal_connect(btn_view_bounds,   "toggled", G_CALLBACK (btn_view_bounds_toggled_cb), NULL);
  
	btn_view_image    = gtk_check_button_new_with_label ("Show model source image");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_view_image, 0, 2, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_image), set_view_image);
	g_signal_connect(btn_view_image,    "toggled", G_CALLBACK (btn_image_toggled_cb), NULL);
  
	btn_superimpose   = gtk_check_button_new_with_label ("Superimpose build image");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_superimpose, 0, 3, 2, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_superimpose), Superimpose_flag);
	g_signal_connect(btn_superimpose,   "toggled", G_CALLBACK (btn_superimpose_toggled_cb), NULL);
  
	btn_view_slice    = gtk_check_button_new_with_label ("Show slice in 3D view");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_view_slice, 3, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_slice), set_view_mdl_slice);
	g_signal_connect(btn_view_slice,    "toggled", G_CALLBACK (btn_slice_toggled_cb), NULL);

	btn_show_normals  = gtk_check_button_new_with_label ("Show facet normals");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_show_normals, 3, 1, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_show_normals), set_view_normals);
	g_signal_connect(btn_show_normals,  "toggled", G_CALLBACK (btn_show_normals_toggled_cb), NULL);
  
	btn_view_patches  = gtk_check_button_new_with_label ("Show support patches");
	gtk_grid_attach(GTK_GRID(grd_view3D_options), btn_view_patches, 3, 2, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_view_patches), set_view_patches);
	g_signal_connect(btn_view_patches,  "toggled", G_CALLBACK (btn_view_patches_toggled_cb), NULL);

      }
  
      // set up mouse pick options
      {
	GtkWidget *view3D_pick_frame = gtk_frame_new("Mouse Pick Entity");
	gtk_box_append(GTK_BOX(view3D_box),view3D_pick_frame);
	gtk_widget_set_size_request(view3D_pick_frame,50,75);
	
	GtkWidget *view3D_pick_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(view3D_pick_frame),view3D_pick_box);
	
	GtkWidget *grd_view3D_pick = gtk_grid_new ();
	gtk_box_append(GTK_BOX(view3D_pick_box),grd_view3D_pick);
	gtk_grid_set_row_spacing (GTK_GRID(grd_view3D_pick),5);
	gtk_grid_set_column_spacing (GTK_GRID(grd_view3D_pick),20);

	GtkWidget *btn_picks = gtk_check_button_new();
	
	btn_pick_vertex   = gtk_check_button_new_with_label ("Vertex");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (btn_pick_vertex),GTK_CHECK_BUTTON (btn_picks));
	gtk_grid_attach(GTK_GRID(grd_view3D_pick), btn_pick_vertex, 0, 0, 2, 1);
	if(set_mouse_pick_entity==1){gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_vertex), 1);}
	else {gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_vertex), 0);}
	g_signal_connect(btn_pick_vertex,   "toggled", G_CALLBACK (btn_pick_entity_toggled_cb), GINT_TO_POINTER(1));
	
	btn_pick_facet   = gtk_check_button_new_with_label ("Facet");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (btn_pick_facet),GTK_CHECK_BUTTON (btn_picks));
	gtk_grid_attach(GTK_GRID(grd_view3D_pick), btn_pick_facet, 0, 1, 2, 1);
	if(set_mouse_pick_entity==2){gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_facet), 1);}
	else {gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_facet), 0);}
	g_signal_connect(btn_pick_facet,   "toggled", G_CALLBACK (btn_pick_entity_toggled_cb), GINT_TO_POINTER(2));
	
	btn_pick_patch   = gtk_check_button_new_with_label ("Patch");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (btn_pick_patch),GTK_CHECK_BUTTON (btn_picks));
	gtk_grid_attach(GTK_GRID(grd_view3D_pick), btn_pick_patch, 0, 2, 2, 1);
	if(set_mouse_pick_entity==3){gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_patch), 1);}
	else {gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_patch), 0);}
	g_signal_connect(btn_pick_patch,   "toggled", G_CALLBACK (btn_pick_entity_toggled_cb), GINT_TO_POINTER(3));
	
	btn_pick_neighbors   = gtk_check_button_new_with_label ("Show neighbors");
	gtk_grid_attach(GTK_GRID(grd_view3D_pick), btn_pick_neighbors, 3, 0, 1, 1);
	gtk_check_button_set_active(GTK_CHECK_BUTTON (btn_pick_neighbors), set_show_pick_neighbors);
	g_signal_connect(btn_pick_patch,   "toggled", G_CALLBACK (btn_pick_neighbors_toggled_cb), NULL);
	
      }
    }

    // set up a graph_frame next to encapsulate graph settings
    {
      //graph_frame = gtk_frame_new("Graph Settings");
      graph_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
      gtk_widget_set_size_request(graph_frame,LCD_WIDTH-170,LCD_HEIGHT-150);
      
      lbl_graph_params=gtk_label_new("Graph");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),graph_frame,lbl_graph_params);
      
      grd_graph = gtk_grid_new ();
      gtk_box_append(GTK_BOX(graph_frame),grd_graph);				
      gtk_grid_set_row_spacing (GTK_GRID(grd_graph),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_graph),20);

      btn_graph_type = gtk_check_button_new_with_label ("Track layer time during job");
      gtk_grid_attach (GTK_GRID (grd_graph), btn_graph_type, 0, 1, 1, 1);
      gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_graph_type), hist_type);
      g_signal_connect (btn_graph_type, "toggled", G_CALLBACK (btn_graph_type_toggled_cb), NULL);
    }
    
    // set up a mdl_frame next to encapsulate model settings
    {
      //mdl_frame = gtk_frame_new("Model Settings");
      mdl_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
      gtk_widget_set_size_request(mdl_frame,LCD_WIDTH-170,LCD_HEIGHT-150);
      
      lbl_model_params=gtk_label_new("Model");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),mdl_frame,lbl_model_params);
      
      grd_mdl = gtk_grid_new ();
      gtk_box_append(GTK_BOX(mdl_frame),grd_mdl);				
      gtk_grid_set_row_spacing (GTK_GRID(grd_mdl),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_mdl),20);

      label=gtk_label_new("Facet proximity:");
      gtk_grid_attach (GTK_GRID (grd_mdl), label, 0, 0, 1, 1);
      fval=0.001;
      if(job.model_first!=NULL)fval=job.model_first->facet_prox_tol;
      facet_tol_adj=gtk_adjustment_new(fval, 0.001, 1.000, 0.010, 0.10, 0.5);
      facet_tol_btn=gtk_spin_button_new(facet_tol_adj, 5, 3);
      gtk_grid_attach (GTK_GRID (grd_mdl), facet_tol_btn, 1, 0, 1, 1);
      g_signal_connect(GTK_SPIN_BUTTON(facet_tol_btn),"value-changed",G_CALLBACK(facet_tol_value),NULL);

      label=gtk_label_new("Colinear Tolerance:");
      gtk_grid_attach (GTK_GRID (grd_mdl), label, 0, 1, 1, 1);
      fval=max_colinear_angle;
      colinear_angle_adj=gtk_adjustment_new(fval, 0.1, 30.0, 0.5, 1.0, 10.0);
      colinear_angle_btn=gtk_spin_button_new(colinear_angle_adj, 5, 1);
      gtk_grid_attach (GTK_GRID (grd_mdl), colinear_angle_btn, 1, 1, 1, 1);
      g_signal_connect(GTK_SPIN_BUTTON(colinear_angle_btn),"value-changed",G_CALLBACK(colinear_angle_value),NULL);

      label=gtk_label_new("Wavefront Increment:");
      gtk_grid_attach (GTK_GRID (grd_mdl), label, 0, 2, 1, 1);
      fval=ss_wavefront_increment;
      wavefront_inc_adj=gtk_adjustment_new(fval, 0.001, 0.100, 0.005, 0.01, 0.05);
      wavefront_inc_btn=gtk_spin_button_new(wavefront_inc_adj, 5, 3);
      gtk_grid_attach (GTK_GRID (grd_mdl), wavefront_inc_btn, 1, 2, 1, 1);
      g_signal_connect(GTK_SPIN_BUTTON(wavefront_inc_btn),"value-changed",G_CALLBACK(wavefront_inc_value),NULL);

    }
    
    // set up a img_frame next to encapsulate image settings
    {
      img_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL,5);
      gtk_widget_set_size_request(img_frame,LCD_WIDTH-170,LCD_HEIGHT-150);
      
      lbl_image_params=gtk_label_new("Image");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),img_frame,lbl_image_params);
      
      grd_img = gtk_grid_new ();
      gtk_box_append(GTK_BOX(img_frame),grd_img);				
      gtk_grid_set_row_spacing (GTK_GRID(grd_img),10);
      gtk_grid_set_column_spacing (GTK_GRID(grd_img),20);

      lbl_pix_size=gtk_label_new("Pixel size:");
      gtk_grid_attach (GTK_GRID (grd_img), lbl_pix_size, 0, 0, 1, 1);
      fval=0.28;
      if(job.model_first!=NULL)fval=job.model_first->pix_size;
      pix_size_adj=gtk_adjustment_new(fval, 0.010, 25.000, 0.010, 0.10, 0.5);
      pix_size_btn=gtk_spin_button_new(pix_size_adj, 5, 3);
      gtk_grid_attach (GTK_GRID (grd_img), pix_size_btn, 1, 0, 1, 1);
      g_signal_connect(GTK_SPIN_BUTTON(pix_size_btn),"value-changed",G_CALLBACK(pix_size_value),NULL);

      lbl_pix_incr=gtk_label_new("Pixel increment:");
      gtk_grid_attach (GTK_GRID (grd_img), lbl_pix_incr, 0, 1, 1, 1);
      ival=2;
      if(job.model_first!=NULL)ival=job.model_first->pix_increment;
      pix_increment_adj=gtk_adjustment_new(ival, 1, 25, 1, 1, 5);
      pix_increment_btn=gtk_spin_button_new(pix_increment_adj, 5, 1);
      gtk_grid_attach (GTK_GRID (grd_img), pix_increment_btn, 1, 1, 1, 1);
      g_signal_connect(GTK_SPIN_BUTTON(pix_increment_btn),"value-changed",G_CALLBACK(pix_increment_value),NULL);

    }
    
    // set up a fab_frame next to encapsulate print controls
    {
      fab_left_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(fab_left_box,150,75);

      lbl_print_params=gtk_label_new("Fabrication");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),fab_left_box,lbl_print_params);
      
      // set up which carriage or tool to work with
      {
	sprintf(scratch,"General");
	genrl_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(fab_left_box),genrl_frame);
	
	GtkWidget *genrl_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(genrl_frame),genrl_box);
	gtk_widget_set_size_request(genrl_frame,50,75);
	
	grd_genrl=gtk_grid_new();
	gtk_box_append(GTK_BOX(genrl_box),grd_genrl);
	gtk_grid_set_row_spacing (GTK_GRID(grd_genrl),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_genrl),12);
	
	btn_start_at_z  = gtk_check_button_new_with_label ("(Re)Start at current Z");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_start_at_z, 0, 1, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_start_at_z), set_start_at_crt_z);
	g_signal_connect(btn_start_at_z, "toggled", G_CALLBACK (btn_start_at_z_toggled_cb), NULL);

	btn_force_z_to_table = gtk_check_button_new_with_label ("Force current Z to table");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_force_z_to_table, 0, 2, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_force_z_to_table), set_force_z_to_table);
	g_signal_connect(btn_force_z_to_table, "toggled", G_CALLBACK (btn_force_z_to_table_toggled_cb), NULL);

	btn_z_tbl_comp = gtk_check_button_new_with_label ("Use Z Contouring");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_z_tbl_comp, 0, 3, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_z_tbl_comp), z_tbl_comp);
	g_signal_connect(btn_z_tbl_comp, "toggled", G_CALLBACK (btn_z_tbl_comp_toggled_cb), NULL);
  
  	zcontour_lbl=gtk_label_new("Z Contour Height:");
	gtk_grid_attach (GTK_GRID (grd_genrl),zcontour_lbl, 0, 4, 1, 1);
	zcontour_adj=gtk_adjustment_new(set_z_contour, 0, 20, job.min_slice_thk, 5, 0);
	zcontour_btn=gtk_spin_button_new(zcontour_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_genrl),zcontour_btn, 1, 4, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(zcontour_btn),"value-changed",G_CALLBACK(zcontour_value),NULL);

	//label = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	//gtk_grid_attach (GTK_GRID (grd_genrl), label, 1, 0, 1, 4);

	btn_center_job = gtk_check_button_new_with_label ("Center job on build table");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_center_job, 2, 1, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_center_job), set_center_build_job);
	g_signal_connect(btn_center_job, "toggled", G_CALLBACK (btn_center_job_toggled_cb), NULL);
  
	btn_btm_lyr_details = gtk_check_button_new_with_label ("Include bottom layer details");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_btm_lyr_details, 2, 2, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_btm_lyr_details), set_include_btm_detail);
	g_signal_connect(btn_btm_lyr_details, "toggled", G_CALLBACK (btn_btm_lyr_details_toggled_cb), NULL);
  
	btn_ignore_lt_temps = gtk_check_button_new_with_label ("Ignore line type temps");
	gtk_grid_attach (GTK_GRID (grd_genrl), btn_ignore_lt_temps, 2, 3, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_ignore_lt_temps), set_ignore_lt_temps);
	g_signal_connect(btn_ignore_lt_temps, "toggled", G_CALLBACK (btn_ignore_lt_temps_toggled_cb), NULL);
	
	vtx_overlap_lbl=gtk_label_new("Vertex Overlap Qty:");
	gtk_grid_attach (GTK_GRID (grd_genrl),vtx_overlap_lbl, 2, 4, 1, 1);
	vtx_overlap_adj=gtk_adjustment_new(set_vtx_overlap, 0, 20, 1, 5, 0);
	vtx_overlap_btn=gtk_spin_button_new(vtx_overlap_adj, 3, 0);
	gtk_grid_attach (GTK_GRID (grd_genrl),vtx_overlap_btn, 3, 4, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(vtx_overlap_btn),"value-changed",G_CALLBACK(vtx_overlap_value),NULL);

      }
  
      // set up CNC router frame
      {
	sprintf(scratch,"Cutting & Marking");
	cnc_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(fab_left_box),cnc_frame);
	
	GtkWidget *cnc_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(cnc_frame),cnc_box);
	gtk_widget_set_size_request(cnc_frame,50,160);
	
	grd_cnc=gtk_grid_new();
	gtk_box_append(GTK_BOX(cnc_box),grd_cnc);
	gtk_grid_set_row_spacing (GTK_GRID(grd_cnc),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_cnc),12);
    
	btn_ignore_bit  = gtk_check_button_new_with_label ("Ignore tool bit changes ");
	gtk_grid_attach (GTK_GRID (grd_cnc), btn_ignore_bit, 0, 5, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_ignore_bit), ignore_tool_bit_change);
	g_signal_connect(btn_ignore_bit, "toggled", G_CALLBACK (btn_ignore_bit_toggled_cb), NULL);
     
	btn_ignore_drill  = gtk_check_button_new_with_label ("Ignore hole drilling");
	gtk_grid_attach (GTK_GRID (grd_cnc), btn_ignore_drill, 0, 6, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_ignore_drill), drill_holes_done_flag);
	g_signal_connect(btn_ignore_drill, "toggled", G_CALLBACK (btn_ignore_drill_toggled_cb), NULL);
     
	btn_small_drill  = gtk_check_button_new_with_label ("Only drill small holes");
	gtk_grid_attach (GTK_GRID (grd_cnc), btn_small_drill, 0, 7, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_small_drill), only_drill_small_holes_flag);
	g_signal_connect(btn_small_drill, "toggled", G_CALLBACK (btn_small_drill_toggled_cb), NULL);
  
	btn_profile_cut = gtk_check_button_new_with_label ("Profile cut/mark");
	gtk_grid_attach (GTK_GRID (grd_cnc), btn_profile_cut, 0, 8, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_profile_cut), cnc_profile_cut_flag);
	g_signal_connect(btn_profile_cut, "toggled", G_CALLBACK (btn_profile_cut_toggled_cb), NULL);
  
	btn_add_block = gtk_check_button_new_with_label ("Subtract from target");
	gtk_grid_attach (GTK_GRID (grd_cnc), btn_add_block, 0, 9, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_add_block), cnc_add_block_flag);
	g_signal_connect(btn_add_block, "toggled", G_CALLBACK (btn_add_block_toggled_cb), NULL);
  
	lbl_block_dims=gtk_label_new("Base size:");
	gtk_grid_attach (GTK_GRID (grd_cnc), lbl_block_dims, 0, 10, 1, 1);
	
	lbl_cnc_block_x=gtk_label_new("  X margin:");
	gtk_grid_attach (GTK_GRID (grd_cnc), lbl_cnc_block_x, 0, 11, 1, 1);
	cnc_block_x_adj=gtk_adjustment_new(cnc_x_margin, 1, 20, 1, 1, 5);
	cnc_block_x_btn=gtk_spin_button_new(cnc_block_x_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_cnc), cnc_block_x_btn, 1, 11, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(cnc_block_x_btn),"value-changed",G_CALLBACK(cnc_block_x_value),NULL);
  
	lbl_cnc_block_y=gtk_label_new("  Y margin:");
	gtk_grid_attach (GTK_GRID (grd_cnc), lbl_cnc_block_y, 0, 12, 1, 1);
	cnc_block_y_adj=gtk_adjustment_new(cnc_y_margin, 1, 20, 1, 1, 5);
	cnc_block_y_btn=gtk_spin_button_new(cnc_block_y_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_cnc), cnc_block_y_btn, 1, 12, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(cnc_block_y_btn),"value-changed",G_CALLBACK(cnc_block_y_value),NULL);
  
	lbl_cnc_block_z=gtk_label_new("  Z height:");
	gtk_grid_attach (GTK_GRID (grd_cnc), lbl_cnc_block_z, 0, 13, 1, 1);
	cnc_block_z_adj=gtk_adjustment_new(cnc_z_height, 1, 152, 1, 1, 20);
	cnc_block_z_btn=gtk_spin_button_new(cnc_block_z_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_cnc), cnc_block_z_btn, 1, 13, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(cnc_block_z_btn),"value-changed",G_CALLBACK(cnc_block_z_value),NULL);
      }
    }

    // set up a unt_frame next to encapsulate unit settings
    {
      unt_left_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(unt_left_box,150,75);

      lbl_system_params=gtk_label_new("System");
      gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),unt_left_box,lbl_system_params);

      // general stuff
      {
	unt_genr_frame = gtk_frame_new("General");
	gtk_box_append(GTK_BOX(unt_left_box),unt_genr_frame);
	
	GtkWidget *unt_genr_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(unt_genr_frame),unt_genr_box);
	gtk_widget_set_size_request(unt_genr_frame,50,75);
	
	grd_unt_genr = gtk_grid_new ();				
	gtk_box_append(GTK_BOX(unt_genr_box),grd_unt_genr);
	gtk_grid_set_row_spacing (GTK_GRID(grd_unt_genr),10);
	gtk_grid_set_column_spacing (GTK_GRID(grd_unt_genr),20);
	
	lbl_matl_feed_time=gtk_label_new("Material feed time:");
	gtk_grid_attach (GTK_GRID (grd_unt_genr), lbl_matl_feed_time, 0, 2, 1, 1);
	matl_feed_time_adj=gtk_adjustment_new(max_feed_time, 5, 59, 1, 5, 0);
	matl_feed_time_btn=gtk_spin_button_new(matl_feed_time_adj, 3, 0);
	gtk_grid_attach (GTK_GRID (grd_unt_genr), matl_feed_time_btn, 1, 2, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(matl_feed_time_btn),"value-changed",G_CALLBACK(matl_feed_value),NULL);
	
	lbl_img_edge_thld=gtk_label_new("Edge detect threshold:");
	gtk_grid_attach (GTK_GRID (grd_unt_genr), lbl_img_edge_thld, 0, 3, 1, 1);
	img_edge_thld_adj=gtk_adjustment_new(edge_detect_threshold, 0, 255, 1, 5, 0);
	img_edge_thld_btn=gtk_spin_button_new(img_edge_thld_adj, 4, 0);
	gtk_grid_attach (GTK_GRID (grd_unt_genr), img_edge_thld_btn, 1, 3, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(img_edge_thld_btn),"value-changed",G_CALLBACK(edge_detect_threshold_cb),NULL);

	btn_save_image_hist = gtk_check_button_new_with_label ("Save image history");
	gtk_grid_attach (GTK_GRID (grd_unt_genr), btn_save_image_hist, 0, 4, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_save_image_hist), save_image_history);
	g_signal_connect(btn_save_image_hist, "toggled", G_CALLBACK (btn_save_image_hist_toggled_cb), NULL);

	btn_save_job_hist = gtk_check_button_new_with_label ("Save job history");
	gtk_grid_attach (GTK_GRID (grd_unt_genr), btn_save_job_hist, 0, 5, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (btn_save_job_hist), save_logfile_flag);
	g_signal_connect(btn_save_job_hist, "toggled", G_CALLBACK (btn_save_job_hist_toggled_cb), NULL);

	// camera stuff
	/*
	  save_image_history++;
	  if(save_image_history>1)save_image_history=0;
	  
	  gtk_image_clear(GTK_IMAGE(img_camera_timelapse));			// MUST be cleared then re-loaded or it becomes severe memory leak
	  if(save_image_history==1)
	    {
	    img_camera_timelapse = gtk_image_new_from_file("Time_Lapse_On.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_timelapse),50);
	    gtk_button_set_child(GTK_BUTTON(btn_camera_timelapse),img_camera_timelapse);
	    }
	  else 
	    {
	    img_camera_timelapse = gtk_image_new_from_file("Time_Lapse_Off.gif");
	    gtk_image_set_pixel_size(GTK_IMAGE(img_camera_timelapse),50);
	    gtk_button_set_child(GTK_BUTTON(btn_camera_timelapse),img_camera_timelapse);
	    }
	*/
      }

      // thermal stuff
      {
	unt_thrm_frame = gtk_frame_new("Thermal");
	gtk_box_append(GTK_BOX(unt_left_box),unt_thrm_frame);
	
	GtkWidget *unt_thrm_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(unt_thrm_frame),unt_thrm_box);
	gtk_widget_set_size_request(unt_thrm_frame,50,75);
	
	grd_unt_thrm = gtk_grid_new ();				
	gtk_box_append(GTK_BOX(unt_thrm_box),grd_unt_thrm);
	gtk_grid_set_row_spacing (GTK_GRID(grd_unt_thrm),10);
	gtk_grid_set_column_spacing (GTK_GRID(grd_unt_thrm),20);
	
	lbl_temp_tol=gtk_label_new("Temperature Tolerance:");
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), lbl_temp_tol, 0, 2, 1, 1);
	temp_tol_adj=gtk_adjustment_new(acceptable_temp_tol, 1, 20, 0.2, 5, 0);
	temp_tol_btn=gtk_spin_button_new(temp_tol_adj, 4, 1);
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), temp_tol_btn, 1, 2, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(temp_tol_btn),"value-changed",G_CALLBACK(temp_tol_value),NULL);
  
	lbl_chmbr_temp_or=gtk_label_new("Chamber Temp Override:");
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), lbl_chmbr_temp_or, 0, 3, 1, 1);
	temp_chm_or_adj=gtk_adjustment_new(Tool[CHAMBER].thrm.tempC, 1, 100, 0.2, 1, 5);
	temp_chm_or_btn=gtk_spin_button_new(temp_chm_or_adj, 5, 1);
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), temp_chm_or_btn, 1, 3, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(temp_chm_or_btn),"value-changed",G_CALLBACK(temp_chm_or_value),NULL);
  
	lbl_table_temp_or=gtk_label_new("Table Temp Override:");
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), lbl_table_temp_or, 0, 4, 1, 1);
	temp_tbl_or_adj=gtk_adjustment_new(Tool[BLD_TBL1].thrm.setpC, 1, 150, 1, 1, 5);
	temp_tbl_or_btn=gtk_spin_button_new(temp_tbl_or_adj, 5, 1);
	gtk_grid_attach (GTK_GRID (grd_unt_thrm), temp_tbl_or_btn, 1, 4, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(temp_tbl_or_btn),"value-changed",G_CALLBACK(temp_tbl_or_value),NULL);
      }
      
    }

    // set up a cal_frame next to encapsulate system calibration
    {
      cal_left_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(fab_left_box,150,175);
      lbl_system_calib=gtk_label_new("Calibration");

      // only allow access when NOT running a job
      if(job.state<JOB_RUNNING || job.state>JOB_TABLE_SCAN_PAUSED)
        {gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),cal_left_box,lbl_system_calib);}

      // set up build table calibration
      {
	bld_tbl_cal_frame = gtk_frame_new("Build Table Z Calibration");
	gtk_box_append(GTK_BOX(cal_left_box),bld_tbl_cal_frame);
	gtk_widget_set_size_request(bld_tbl_cal_frame,50,75);
	
	GtkWidget *bld_tbl_cal_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(bld_tbl_cal_frame),bld_tbl_cal_box);
	
	grd_bld_tbl_cal = gtk_grid_new ();				
	gtk_box_append(GTK_BOX(bld_tbl_cal_box),grd_bld_tbl_cal);
	gtk_grid_set_row_spacing (GTK_GRID(grd_bld_tbl_cal),10);
	gtk_grid_set_column_spacing (GTK_GRID(grd_bld_tbl_cal),20);
	
	// add button to XY scan build table
	label=gtk_label_new(" Scan build table Z: ");
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal), label, 0, 0, 1, 1);
	btn_scan = gtk_button_new_with_label ("Scan");
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal), btn_scan, 1, 0, 1, 1);
	g_signal_connect (btn_scan, "clicked", G_CALLBACK(scan_callback),win_settings);
	
	// add spin button to set scanning interval distance
  	bld_tbl_res_lbl=gtk_label_new("Scan resolution (mm): ");
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal),bld_tbl_res_lbl, 0, 1, 1, 1);
	bld_tbl_res_adj=gtk_adjustment_new(scan_dist, 1, 250, 1, 5, 0);
	bld_tbl_res_btn=gtk_spin_button_new(bld_tbl_res_adj, 4, 0);
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal),bld_tbl_res_btn, 1, 1, 1, 1);
	g_signal_connect(GTK_SPIN_BUTTON(bld_tbl_res_btn),"value-changed",G_CALLBACK(scan_dist_value),NULL);
  
	// add button to calibrate build table level
	label=gtk_label_new(" Level XY plane to build table: ");
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal), label, 3, 0, 1, 1);
	btn_calib_zlevel = gtk_button_new_with_label ("Calib");
	gtk_grid_attach (GTK_GRID (grd_bld_tbl_cal), btn_calib_zlevel, 4, 0, 1, 1);
	g_signal_connect (btn_calib_zlevel, "clicked", G_CALLBACK(ztable_calibration),win_settings);
      }
      
      // set up carriage calibration
      {
	carriage_cal_frame = gtk_frame_new("Carriage Calibration");
	gtk_box_append(GTK_BOX(cal_left_box),carriage_cal_frame);
	
	GtkWidget *carriage_cal_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(carriage_cal_frame),carriage_cal_box);
	gtk_widget_set_size_request(carriage_cal_frame,50,150);
	
	grd_carriage_cal = gtk_grid_new ();				
	gtk_box_append(GTK_BOX(carriage_cal_box),grd_carriage_cal);
	gtk_grid_set_row_spacing (GTK_GRID(grd_carriage_cal),10);
	gtk_grid_set_column_spacing (GTK_GRID(grd_carriage_cal),20);
	
	// display current calibration values
	label=gtk_label_new("Slot     Temp      X        Y        Z");
	gtk_grid_attach (GTK_GRID (grd_carriage_cal), label, 0, 1, 2, 1);
	for(i=0;i<MAX_TOOLS;i++)
	  {
	  sprintf(scratch," %d      %+4.1f    %+8.3f  %+8.3f  %+8.3f",(i+1),crgslot[i].temp_offset,crgslot[i].x_offset,crgslot[i].y_offset,crgslot[i].z_offset);
	  label=gtk_label_new(scratch);
	  gtk_grid_attach (GTK_GRID (grd_carriage_cal), label, 0, (2+i), 2, 1);
	  }
	
	// add button to calibrate slots
	label=gtk_label_new(" Set tool slot offsets:  ");
	gtk_grid_attach (GTK_GRID (grd_carriage_cal), label, 4, 1, 1, 1);
	btn_calib_slots = gtk_button_new_with_label ("Calibrate");
	gtk_grid_attach (GTK_GRID (grd_carriage_cal), btn_calib_slots, 5, 1, 1, 1);
	g_signal_connect (btn_calib_slots, "clicked", G_CALLBACK(unit_calibration),win_settings);
      }
      
      // set up thermal calibration
      {
	GtkWidget *thermal_cal_frame = gtk_frame_new("Temperature Calibration");
	gtk_box_append(GTK_BOX(cal_left_box),thermal_cal_frame);
	
	GtkWidget *thermal_cal_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(thermal_cal_frame),thermal_cal_box);
	gtk_widget_set_size_request(thermal_cal_frame,50,150);
	
	GtkWidget *grd_thermal_cal = gtk_grid_new ();				
	gtk_box_append(GTK_BOX(thermal_cal_box),grd_thermal_cal);
	gtk_grid_set_row_spacing (GTK_GRID(grd_thermal_cal),10);
	gtk_grid_set_column_spacing (GTK_GRID(grd_thermal_cal),20);

	// row 0
	sprintf(scratch,"Device:");
	GtkWidget *lbl_cal_device=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_cal_device, 0, 0, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_cal_device),0.5);

	sprintf(scratch,"Tool 1");
	lbl_device_being_calibrated = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_device_being_calibrated, 1, 0, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_device_being_calibrated),0.0);

	temp_cal_device=gtk_adjustment_new(device_being_calibrated, 0, (MAX_THERMAL_DEVICES+1), 1, 1, 2);
	temp_cal_btn=gtk_spin_button_new(temp_cal_device, 2, 0);
	gtk_grid_attach(GTK_GRID(grd_thermal_cal),temp_cal_btn, 2, 0, 1, 1);
	g_signal_connect(temp_cal_btn,"value-changed",G_CALLBACK(grab_device_for_temp_cal),GINT_TO_POINTER(device_being_calibrated));

	label=gtk_label_new("          ");
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), label, 3, 0, 1, 1);
	
	sprintf(scratch,"Save");
	GtkWidget *save_temp_calib_btn = gtk_button_new_with_label(scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), save_temp_calib_btn, 4, 0, 1, 1);
	g_signal_connect (save_temp_calib_btn, "clicked", G_CALLBACK(save_temp_cal_value),GINT_TO_POINTER(device_being_calibrated));
	
	// row 1
	sprintf(scratch,"Current reading: ");
	GtkWidget *lbl_current_reading=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_current_reading, 0, 1, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_current_reading),0.0);
	
	sprintf(scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.tempC,Tool[device_being_calibrated].thrm.tempV);
	temp_tool_disp=gtk_label_new(scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), temp_tool_disp, 1, 1, 1, 1);

	// row 2
	sprintf(scratch,"Low cal point: ");
	GtkWidget *lbl_lo_current=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_lo_current, 0, 2, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_lo_current),0.0);
	
	sprintf(scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.calib_t_low,Tool[device_being_calibrated].thrm.calib_v_low);
	lbl_lo_values=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_lo_values, 1, 2, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_lo_values),0.0);
	
	sprintf(scratch,"Low Temp");
	GtkWidget *slot_lo_temp_btn = gtk_button_new_with_label(scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), slot_lo_temp_btn, 2, 2, 1, 1);
	g_signal_connect (slot_lo_temp_btn, "clicked", G_CALLBACK(temp_slot_lo_cal_value),GINT_TO_POINTER(device_being_calibrated));
	
	// row 3
	sprintf(scratch,"High cal point: ");
	GtkWidget *lbl_hi_current=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_hi_current, 0, 3, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_hi_current),0.0);
	
	sprintf(scratch,"%6.2fC %6.3fv",Tool[device_being_calibrated].thrm.calib_t_high,Tool[device_being_calibrated].thrm.calib_v_high);
	lbl_hi_values=gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_hi_values, 1, 3, 1, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_hi_values),0.0);
	
	sprintf(scratch,"High Temp");
	GtkWidget *slot_hi_temp_btn = gtk_button_new_with_label(scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), slot_hi_temp_btn, 2, 3, 1, 1);
	g_signal_connect (slot_hi_temp_btn, "clicked", G_CALLBACK(temp_slot_hi_cal_value),GINT_TO_POINTER(device_being_calibrated));

	// row 4
	sprintf(scratch,"Temperature offset: ");
	GtkWidget *lbl_temp_offset = gtk_label_new (scratch);
	gtk_grid_attach (GTK_GRID (grd_thermal_cal), lbl_temp_offset, 0, 4, 2, 1);
	gtk_label_set_xalign (GTK_LABEL(lbl_temp_offset),0.0);

	temp_cal_offset=gtk_adjustment_new(Tool[device_being_calibrated].thrm.temp_offset, -300, 300, 1, 2, 5);
	temp_cal_offset_btn=gtk_spin_button_new(temp_cal_offset, 3, 1);
	gtk_grid_attach(GTK_GRID(grd_thermal_cal),temp_cal_offset_btn, 2, 4, 1, 1);
	g_signal_connect(temp_cal_offset_btn,"value-changed",G_CALLBACK(grab_temp_cal_offset),GINT_TO_POINTER(device_being_calibrated));

      }
      
    }
    
    // set up a test frame next to encapsulate system test/debug operations
    {
      test_left_box=gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
      gtk_widget_set_size_request(test_left_box,150,75);
      lbl_system_test=gtk_label_new("Test");

      // only allow access when NOT running a job
      if(job.state<JOB_RUNNING || job.state>JOB_TABLE_SCAN_PAUSED)
        {gtk_notebook_append_page (GTK_NOTEBOOK(settings_notebook),test_left_box,lbl_system_test);}

      // set up which carriage or tool to work with
      {
	sprintf(scratch,"Active Carriage/Tool");
	GtkWidget *frame_slot_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(test_left_box),frame_slot_frame);

	slot_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(frame_slot_frame), slot_frame);
	gtk_widget_set_size_request(slot_frame,50,75);

	grd_slot=gtk_grid_new();
	gtk_box_append(GTK_BOX(slot_frame),grd_slot);
	gtk_grid_set_row_spacing (GTK_GRID(grd_slot),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_slot),12);
	
	lbl_scratch=gtk_label_new("ID:");
	gtk_grid_attach (GTK_GRID(grd_slot),lbl_scratch, 0, 3, 1, 1);
	slotadj=gtk_adjustment_new(test_slot, 0, MAX_TOOLS, 1, 1, 1);
	slotbtn=gtk_spin_button_new(slotadj, 2, 0);
	gtk_grid_attach(GTK_GRID(grd_slot),slotbtn, 1, 3, 1, 1);
	g_signal_connect(slotbtn,"value-changed",G_CALLBACK(grab_slot_value),NULL);
  
	GtkWidget *btn_rbtn_group=gtk_check_button_new();

	rbtn_none = gtk_check_button_new_with_label("None");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (rbtn_none),GTK_CHECK_BUTTON(btn_rbtn_group));
	gtk_grid_attach (GTK_GRID(grd_slot),rbtn_none, 2, 3, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (rbtn_none), TRUE);
	g_signal_connect (rbtn_none, "toggled", G_CALLBACK (slotbtn_toggled_cb), GINT_TO_POINTER(0));

	rbtn_carg = gtk_check_button_new_with_label("Carriage");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (rbtn_carg),GTK_CHECK_BUTTON(btn_rbtn_group));
	gtk_grid_attach (GTK_GRID(grd_slot),rbtn_carg, 3, 3, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (rbtn_carg), FALSE);
	g_signal_connect (rbtn_carg, "toggled", G_CALLBACK (slotbtn_toggled_cb), GINT_TO_POINTER(1));

	rbtn_tool = gtk_check_button_new_with_label("Tool");
	gtk_check_button_set_group(GTK_CHECK_BUTTON (rbtn_tool),GTK_CHECK_BUTTON(btn_rbtn_group));
	gtk_grid_attach (GTK_GRID(grd_slot),rbtn_tool, 4, 3, 1, 1);
	gtk_check_button_set_active (GTK_CHECK_BUTTON (rbtn_tool), FALSE);
	g_signal_connect (rbtn_tool, "toggled", G_CALLBACK (slotbtn_toggled_cb), GINT_TO_POINTER(2));

      }
      
      // set up carriage up/down movement tests
      {
	sprintf(scratch,"Carriage Move");
	artic_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(test_left_box),artic_frame);
	
	GtkWidget *artic_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(artic_frame),artic_box);
	gtk_widget_set_size_request(artic_frame,50,75);

	grd_artic=gtk_grid_new();
	gtk_box_append(GTK_BOX(artic_box),grd_artic);
	gtk_grid_set_row_spacing (GTK_GRID(grd_artic),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_artic),12);
  
	lbl_scratch=gtk_label_new("Distance:");
	gtk_grid_attach (GTK_GRID(grd_artic),lbl_scratch, 0, 1, 1, 1);
	cdistadj=gtk_adjustment_new(move_cdist, -15, 15, 1, 1, 5);
	cdistbtn=gtk_spin_button_new(cdistadj, 5, 1);
	gtk_grid_attach(GTK_GRID(grd_artic),cdistbtn, 1, 1, 1, 1);
	g_signal_connect(cdistbtn,"value-changed",G_CALLBACK(grab_cdist_value),NULL);
	lbl_scratch=gtk_label_new(" +=UP -=DN");
	gtk_grid_attach (GTK_GRID(grd_artic),lbl_scratch, 2, 1, 1, 1);
	
	btn_chome = gtk_button_new_with_label ("Home");
	g_signal_connect (btn_chome, "clicked", G_CALLBACK (slot_chome_callback), GINT_TO_POINTER(test_slot));
	gtk_grid_attach (GTK_GRID (grd_artic), btn_chome, 3, 1, 1, 1);
      
	btn_ctest = gtk_button_new_with_label ("Move");
	g_signal_connect (btn_ctest, "clicked", G_CALLBACK (slot_cmove_callback), GINT_TO_POINTER(test_slot));
	gtk_grid_attach (GTK_GRID (grd_artic), btn_ctest, 4, 1, 1, 1);

	sprintf(scratch,"LimitSw OFF");
	if(RPi_GPIO_Read(TOOL_LIMIT))sprintf(scratch,"LimitSw ON ");
	lbl_slot_stat=gtk_label_new(scratch);
	gtk_grid_attach (GTK_GRID(grd_artic),lbl_slot_stat, 5, 1, 1, 1);
      
      }
      
      // set up tool move/feed tests
      {
	sprintf(scratch,"Tool Feed");
	tfeed_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(test_left_box),tfeed_frame);
	
	GtkWidget *tfeed_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(tfeed_frame),tfeed_box);
	gtk_widget_set_size_request(tfeed_frame,50,75);
	
	grd_tfeed=gtk_grid_new();
	gtk_box_append(GTK_BOX(tfeed_box),grd_tfeed);
	gtk_grid_set_row_spacing (GTK_GRID(grd_tfeed),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_tfeed),12);
  
	lbl_scratch=gtk_label_new("Distance:");
	gtk_grid_attach (GTK_GRID(grd_tfeed),lbl_scratch, 0, 1, 1, 1);
	tdistadj=gtk_adjustment_new(move_tdist, -15, 15, 1, 1, 5);
	tdistbtn=gtk_spin_button_new(tdistadj, 5, 1);
	gtk_grid_attach(GTK_GRID(grd_tfeed),tdistbtn, 1, 1, 1, 1);
	g_signal_connect(tdistbtn,"value-changed",G_CALLBACK(grab_tdist_value),NULL);
	lbl_scratch=gtk_label_new(" +=UP -=DN");
	gtk_grid_attach (GTK_GRID(grd_tfeed),lbl_scratch, 2, 1, 1, 1);
	
	btn_thome = gtk_button_new_with_label ("Home");
	g_signal_connect (btn_thome, "clicked", G_CALLBACK (slot_thome_callback), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (grd_tfeed), btn_thome, 3, 1, 1, 1);
      
	btn_ttest = gtk_button_new_with_label ("Move");
	g_signal_connect (btn_ttest, "clicked", G_CALLBACK (slot_tmove_callback), GINT_TO_POINTER(slot));
	gtk_grid_attach (GTK_GRID (grd_tfeed), btn_ttest, 4, 1, 1, 1);

	sprintf(scratch,"LimitSw OFF");
	if(RPi_GPIO_Read(TOOL_LIMIT))sprintf(scratch,"LimitSw ON ");
	lbl_slot_stat=gtk_label_new(scratch);
	gtk_grid_attach (GTK_GRID(grd_tfeed),lbl_slot_stat, 5, 1, 1, 1);
      
      }

      // set up XYZ movement control
      {
	sprintf(scratch,"XYZ Control");
	xyz_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(test_left_box),xyz_frame);
	
	GtkWidget *xyz_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(xyz_frame),xyz_box);
	gtk_widget_set_size_request(xyz_frame,50,150);

	grd_xyz=gtk_grid_new();
	gtk_box_append(GTK_BOX(xyz_box),grd_xyz);
	gtk_grid_set_row_spacing (GTK_GRID(grd_xyz),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_xyz),12);
	
	lbl_scratch=gtk_label_new("  X:");
	gtk_grid_attach (GTK_GRID(grd_xyz),lbl_scratch, 0, 1, 1, 1);
	move_x=PosIs.x;
	xdistadj=gtk_adjustment_new(move_x, BUILD_TABLE_MIN_X, BUILD_TABLE_MAX_X, 1, 1, 5);
	xdistbtn=gtk_spin_button_new(xdistadj, 5, 1);
	gtk_grid_attach(GTK_GRID(grd_xyz),xdistbtn, 1, 1, 1, 1);
	g_signal_connect(xdistbtn,"value-changed",G_CALLBACK(grab_xdist_value),NULL);
	btn_xhome = gtk_button_new_with_label ("Home");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_xhome, 3, 1, 1, 1);
	axis=1;
	g_signal_connect (btn_xhome, "clicked", G_CALLBACK (on_home_callback), GINT_TO_POINTER(axis));
	btn_xmin = gtk_button_new_with_label ("Min");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_xmin, 4, 1, 1, 1);
	g_signal_connect (btn_xmin, "clicked", G_CALLBACK (on_min_callback), GINT_TO_POINTER(axis));
	btn_xmid = gtk_button_new_with_label ("Mid");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_xmid, 5, 1, 1, 1);
	g_signal_connect (btn_xmid, "clicked", G_CALLBACK (on_mid_callback), GINT_TO_POINTER(axis));
	btn_xmax = gtk_button_new_with_label ("Max");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_xmax, 6, 1, 1, 1);
	g_signal_connect (btn_xmax, "clicked", G_CALLBACK (on_max_callback), GINT_TO_POINTER(axis));
	
	lbl_scratch=gtk_label_new("  Y:");
	gtk_grid_attach (GTK_GRID(grd_xyz),lbl_scratch, 0, 2, 1, 1);
	move_y=PosIs.y;
	ydistadj=gtk_adjustment_new(move_y, BUILD_TABLE_MIN_Y, BUILD_TABLE_MAX_Y, 1, 1, 5);
	ydistbtn=gtk_spin_button_new(ydistadj, 5, 1);
	gtk_grid_attach(GTK_GRID(grd_xyz),ydistbtn, 1, 2, 1, 1);
	g_signal_connect(ydistbtn,"value-changed",G_CALLBACK(grab_ydist_value),NULL);
	btn_yhome = gtk_button_new_with_label ("Home");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_yhome, 3, 2, 1, 1);
	axis=10;
	g_signal_connect (btn_yhome, "clicked", G_CALLBACK (on_home_callback), GINT_TO_POINTER(axis));
	btn_ymin = gtk_button_new_with_label ("Min");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_ymin, 4, 2, 1, 1);
	g_signal_connect (btn_ymin, "clicked", G_CALLBACK (on_min_callback), GINT_TO_POINTER(axis));
	btn_ymid = gtk_button_new_with_label ("Mid");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_ymid, 5, 2, 1, 1);
	g_signal_connect (btn_ymid, "clicked", G_CALLBACK (on_mid_callback), GINT_TO_POINTER(axis));
	btn_ymax = gtk_button_new_with_label ("Max");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_ymax, 6, 2, 1, 1);
	g_signal_connect (btn_ymax, "clicked", G_CALLBACK (on_max_callback), GINT_TO_POINTER(axis));
	
	lbl_scratch=gtk_label_new("  Z:");
	gtk_grid_attach (GTK_GRID(grd_xyz),lbl_scratch, 0, 3, 1, 1);
	move_z=PosIs.z;
	minzpos=BUILD_TABLE_MIN_Z;
	#if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	  if(Tool[0].state>=TL_LOADED)minzpos=Tool[0].tip_dn_pos;
	#endif
	zdistadj=gtk_adjustment_new(move_z, minzpos, BUILD_TABLE_MAX_Z, 1, 1, 5);
	zdistbtn=gtk_spin_button_new(zdistadj, 5, 1);
	gtk_grid_attach(GTK_GRID(grd_xyz),zdistbtn, 1, 3, 1, 1);
	g_signal_connect(zdistbtn,"value-changed",G_CALLBACK(grab_zdist_value),NULL);
	btn_zhome = gtk_button_new_with_label ("Home");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_zhome, 3, 3, 1, 1);
	axis=100;
	g_signal_connect (btn_zhome, "clicked", G_CALLBACK (on_home_callback), GINT_TO_POINTER(axis));
	btn_zmin = gtk_button_new_with_label ("Min");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_zmin, 4, 3, 1, 1);
	g_signal_connect (btn_zmin, "clicked", G_CALLBACK (on_min_callback), GINT_TO_POINTER(axis));
	btn_zmid = gtk_button_new_with_label ("Mid");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_zmid, 5, 3, 1, 1);
	g_signal_connect (btn_zmid, "clicked", G_CALLBACK (on_mid_callback), GINT_TO_POINTER(axis));
	btn_zmax = gtk_button_new_with_label ("Max");
	gtk_grid_attach (GTK_GRID (grd_xyz), btn_zmax, 6, 3, 1, 1);
	g_signal_connect (btn_zmax, "clicked", G_CALLBACK (on_max_callback), GINT_TO_POINTER(axis));
      }

      // set up tool i/o test frame
      {
	sprintf(scratch,"Tool I/O Control");
	tio_frame=gtk_frame_new(scratch);
	gtk_box_append(GTK_BOX(test_left_box),tio_frame);
	
	GtkWidget *tio_box = gtk_box_new(GTK_ORIENTATION_VERTICAL,2);
	gtk_frame_set_child(GTK_FRAME(tio_frame),tio_box);
	gtk_widget_set_size_request(tio_frame,50,150);

	grd_tio=gtk_grid_new();
	gtk_box_append(GTK_BOX(tio_box),grd_tio);
	gtk_grid_set_row_spacing (GTK_GRID(grd_tio),6);
	gtk_grid_set_column_spacing (GTK_GRID(grd_tio),12);
	
	lbl_scratch=gtk_label_new("  48v:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 0, 1, 1, 1);
	t48vadj=gtk_adjustment_new(t48v_duty, 0, 105, 1, 1, 5);
	t48vbtn=gtk_spin_button_new(t48vadj, 4, 0);
	gtk_grid_attach(GTK_GRID(grd_tio),t48vbtn, 1, 1, 1, 1);
	g_signal_connect(t48vbtn,"value-changed",G_CALLBACK(grab_t48v_value),NULL);
	
	btn_t48v_off = gtk_button_new_with_label ("Off");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_t48v_off, 2, 1, 1, 1);
	g_signal_connect (btn_t48v_off, "clicked", G_CALLBACK (on_t48v_off_callback), NULL);
	
	btn_t48v_full = gtk_button_new_with_label ("Full");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_t48v_full, 3, 1, 1, 1);
	g_signal_connect (btn_t48v_full, "clicked", G_CALLBACK (on_t48v_full_callback), NULL);

	lbl_scratch=gtk_label_new("  24v:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 0, 2, 1, 1);
	t24vadj=gtk_adjustment_new(t24v_duty, 0, 105, 1, 1, 5);
	t24vbtn=gtk_spin_button_new(t24vadj, 4, 0);
	gtk_grid_attach(GTK_GRID(grd_tio),t24vbtn, 1, 2, 1, 1);
	g_signal_connect(t24vbtn,"value-changed",G_CALLBACK(grab_t24v_value),NULL);
	
	btn_t24v_off = gtk_button_new_with_label ("Off");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_t24v_off, 2, 2, 1, 1);
	g_signal_connect (btn_t24v_off, "clicked", G_CALLBACK (on_t24v_off_callback), NULL);
	
	btn_t24v_full = gtk_button_new_with_label ("Full");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_t24v_full, 3, 2, 1, 1);
	g_signal_connect (btn_t24v_full, "clicked", G_CALLBACK (on_t24v_full_callback), NULL);

	lbl_scratch=gtk_label_new("  PWM:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 0, 3, 1, 1);
	PWMvadj=gtk_adjustment_new(PWMv_duty, 0, 105, 1, 1, 5);
	PWMvbtn=gtk_spin_button_new(PWMvadj, 4, 0);
	gtk_grid_attach(GTK_GRID(grd_tio),PWMvbtn, 1, 3, 1, 1);
	g_signal_connect(PWMvbtn,"value-changed",G_CALLBACK(grab_PWMv_value),NULL);
	
	GtkWidget *btn_PWMv_off = gtk_button_new_with_label ("Off");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_PWMv_off, 2, 3, 1, 1);
	g_signal_connect (btn_PWMv_off, "clicked", G_CALLBACK (on_PWMv_off_callback), NULL);
	
	GtkWidget *btn_PWMv_full = gtk_button_new_with_label ("Full");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_PWMv_full, 3, 3, 1, 1);
	g_signal_connect (btn_PWMv_full, "clicked", G_CALLBACK (on_PWMv_full_callback), NULL);

	lbl_scratch=gtk_label_new("  Air:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 5, 1, 1, 1);
	btn_fan_on = gtk_button_new_with_label ("On");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_fan_on, 6, 1, 1, 1);
	g_signal_connect (btn_fan_on, "clicked", G_CALLBACK (on_tool_air_on_callback), NULL);
	btn_fan_off = gtk_button_new_with_label ("Off");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_fan_off, 7, 1, 1, 1);
	g_signal_connect (btn_fan_off, "clicked", G_CALLBACK (on_tool_air_off_callback), NULL);
	
	lbl_scratch=gtk_label_new("  Line laser:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 5, 2, 1, 1);
	btn_line_laser_on = gtk_button_new_with_label ("On");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_line_laser_on, 6, 2, 1, 1);
	g_signal_connect (btn_line_laser_on, "clicked", G_CALLBACK (on_line_laser_on_callback), NULL);
	btn_line_laser_off = gtk_button_new_with_label ("Off");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_line_laser_off, 7, 2, 1, 1);
	g_signal_connect (btn_line_laser_off, "clicked", G_CALLBACK (on_line_laser_off_callback), NULL);

	lbl_scratch=gtk_label_new("  Aux Input:");
	gtk_grid_attach (GTK_GRID(grd_tio),lbl_scratch, 5, 3, 1, 1);
	btn_aux_input_test = gtk_button_new_with_label ("Test");
	gtk_grid_attach (GTK_GRID (grd_tio), btn_aux_input_test, 6, 3, 1, 1);
	g_signal_connect (btn_aux_input_test, "clicked", G_CALLBACK (on_aux_input_test_callback), NULL);
      }

    }
    
    // set active tab to start in
    gtk_notebook_set_current_page(GTK_NOTEBOOK(settings_notebook),active_tab);
    
    gtk_widget_set_visible(win_settings,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL,FALSE);}
    return(1);
}
