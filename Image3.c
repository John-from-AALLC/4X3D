#include "Global.h"

// Function to pan camera around build volume to collect images for object scanning
int scan_build_volume(void)
{
  char		cam_scratch[128];
  char		img_name[255];
  int		status=FALSE;
  int		i,slot=0;
  int		max_diam,x_table_center,y_table_center;
  int		number_of_exposures;
  float 	radius,theta,theta_inc,x,y;
  vertex 	*vptr,*vstart,*vold,*vnew;
  polygon 	*pnew;
  
  printf("\n\nScan_build_volume:  entry\n");

  // configure
  job.type=SCANNING;
  number_of_exposures=10;
  
  // delete all the existing files (if any) in the tablescan subdirectory
  strcat(bashfn,"#!/bin/bash\nsudo rm -f /home/aa/Documents/4X3D/TableScan/*.*");	// delete all files in this folder
  system(bashfn);									// issue the command

  // determine max diameter of scan path
  max_diam=(BUILD_TABLE_MAX_X-BUILD_TABLE_MIN_X);
  if((BUILD_TABLE_MAX_Y-BUILD_TABLE_MIN_Y)<max_diam)max_diam=(BUILD_TABLE_MAX_Y-BUILD_TABLE_MIN_Y);
  radius=max_diam/2-10;
  
  // determine table center
  x_table_center=(BUILD_TABLE_MAX_X-BUILD_TABLE_MIN_X)/2;
  y_table_center=(BUILD_TABLE_MAX_Y-BUILD_TABLE_MIN_Y)/2;
  
  // build closed polygon based on diameter and number of exposures
  vstart=NULL; vold=NULL; vnew=NULL;
  theta=0;
  theta_inc=(2*PI)/number_of_exposures;
  while(theta<(2*PI))
    {
    x=x_table_center+radius*cos(theta);
    y=y_table_center+radius*sin(theta);
    
    printf("%d theta=%6.1f x=%6.3f y=%6.3f \n",i,theta,x,y);
    
    theta += theta_inc;
    vnew=vertex_make();
    vnew->x=x; vnew->y=y; vnew->z=150; vnew->k=5000;
    if(vstart==NULL)vstart=vnew;
    if(vold!=NULL)vold->next=vnew;
    vold=vnew;
    }
  vold->next=vstart;
  
  // initialize the camera gimbal servos
  // servos are driven by PWM duty cycle. 14% moves full ccw, 28% moves full cw.
  // AUX2_OUTPUT is used to switch the PWM signal (at the tool) between the pan and tilt servos.
  gpioWrite(AUX2_OUTPUT,ON);								// set to pan servo

  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=8;								// move full ccw
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);
  
  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=22;								// move full cw
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);
  
  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=15;								// move to middle
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);
  
  gpioWrite(AUX2_OUTPUT,OFF);								// set to tilt servo

  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=8;								// move full ccw
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);

  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=22;								// move full cw
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);

  Tool[slot].PWM_is_set=FALSE;
  Tool[slot].thrm.heat_duty=15;								// move to middle
  while(Tool[slot].PWM_is_set==FALSE);
  delay(1500);

  Tool[slot].PWM_is_set=FALSE;
  gpioWrite(AUX2_OUTPUT,ON);								// set to pan servo

  // move camera thru polygon pausing at each vtx to take a picture
  memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
  theta=8;								// set initial duty cycle for pan of camera gibal
  theta_inc=14/number_of_exposures;					// calc increment to each new position
  vptr=vstart;
  while(vptr!=NULL)
    {
    // move to new location
    print_vertex(vptr,slot,7,NULL);  
    tinyGSnd(gcode_burst);						// send string of commands to tinyG
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string
    motion_complete();							// wait for motion to complete
    delay(500);								// wait for camera to stabilize
    
    // rotate the camera gibal into position
    gpioWrite(AUX2_OUTPUT,ON);								// set to pan servo
    Tool[slot].PWM_is_set=FALSE;
    Tool[slot].thrm.heat_duty=theta;							// set duty cycle (aka pulse width)
    theta += theta_inc;									// increment to next position
    while(Tool[slot].PWM_is_set==FALSE);						// wait for thermal thread to acknowledge pwm setting
    delay(1500);
    
    // snap an image
    strcpy(bashfn,"#!/bin/bash\nsudo libcamera-jpeg -v 0 -n 1 -t 10 ");		// define base camera command
    sprintf(img_name,"-o /home/aa/Documents/4X3D/TableScan/x%000dy%000dz%000d.jpg ",(int)(PostG.x),(int)(PostG.y),(int)(PostG.z));
    strcat(bashfn,img_name);							// ... add the target file name
    strcat(bashfn,"--width 830 --height 574 --vflip 1 ");			// ... add the image size
    sprintf(cam_scratch,"--roi %4.2f,%4.2f,%4.2f,%4.2f ",cam_roi_x0,cam_roi_y0,cam_roi_x1,cam_roi_y1);
    strcat(bashfn,cam_scratch);							// ... add region of interest (zoom)
    strcat(bashfn,"--autofocus-mode continuous ");				// ... add focus control
    system(bashfn);								// issue the command to take a picture

    // debug
    printf("  vptr: x=%6.2f y=%6.2f z=%6.2f   theta=%f  theta_inc=%f  duty=%f  PWM=%d\n",vptr->x,vptr->y,vptr->z,theta,theta_inc,Tool[slot].thrm.heat_duty,Tool[slot].thrm.heat_duration);
    printf("  img_name=%s\n\n",img_name);

    // move onto the next location
    vptr=vptr->next;
    if(vptr==vstart)break;
    }
  
  goto_machine_home();
  
  status=TRUE;
  return(status);
}


// Function to load model images at scale.
// Nominal image size is 300 pixels in the y (height) direction.
int image_loader(model *mptr, int xpix, int ypix, int perserv_aspect)
{
  int	stat=FALSE;
  
  // validate input
  if(mptr==NULL)return(FALSE);
  printf("Image loader:  enter\n");
  
  // clear buffers if already full
  if(mptr->g_img_mdl_buff!=NULL){g_object_unref(mptr->g_img_mdl_buff); mptr->g_img_mdl_buff=NULL;}
  //if(mptr->g_img_act_buff!=NULL){g_object_unref(mptr->g_img_act_buff); mptr->g_img_act_buff=NULL;}
  
  // loads the image into model GDK buffer
  if(perserv_aspect==TRUE)
    {
    mptr->g_img_mdl_buff = gdk_pixbuf_new_from_file_at_scale(mptr->model_file, (-1), ypix, TRUE, NULL);
    }
  else 
    {
    mptr->g_img_mdl_buff = gdk_pixbuf_new_from_file_at_scale(mptr->model_file, xpix, ypix, FALSE, NULL); 
    }
  
  if(mptr->g_img_mdl_buff!=NULL)
    {
    // get color and pixels per row information of source image - this is kept as original
    mptr->mdl_n_ch = gdk_pixbuf_get_n_channels (mptr->g_img_mdl_buff);			// n channels - should always be 4
    mptr->mdl_cspace = gdk_pixbuf_get_colorspace (mptr->g_img_mdl_buff);		// color space - should be RGB
    mptr->mdl_alpha = gdk_pixbuf_get_has_alpha (mptr->g_img_mdl_buff);			// alpha - should be FALSE
    mptr->mdl_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (mptr->g_img_mdl_buff); 	// bits per pixel - should be 8
    mptr->mdl_width = gdk_pixbuf_get_width (mptr->g_img_mdl_buff);			// number of columns (image size)
    mptr->mdl_height = gdk_pixbuf_get_height (mptr->g_img_mdl_buff);			// number of rows (image size)
    mptr->mdl_rowstride = gdk_pixbuf_get_rowstride (mptr->g_img_mdl_buff);		// get pixels per row... may change tho
    mptr->mdl_pixels = gdk_pixbuf_get_pixels (mptr->g_img_mdl_buff);			// get pointer to pixel data
    
    // make copy of original image that is manipulated by user and is what is displayed
    mptr->g_img_act_buff=gdk_pixbuf_copy(mptr->g_img_mdl_buff);
    mptr->act_n_ch = gdk_pixbuf_get_n_channels (mptr->g_img_act_buff);			// n channels - should always be 4
    mptr->act_cspace = gdk_pixbuf_get_colorspace (mptr->g_img_act_buff);		// color space - should be RGB
    mptr->act_alpha = gdk_pixbuf_get_has_alpha (mptr->g_img_act_buff);			// alpha - should be FALSE
    mptr->act_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (mptr->g_img_act_buff); 	// bits per pixel - should be 8
    mptr->act_width = gdk_pixbuf_get_width (mptr->g_img_act_buff);			// number of columns (image size)
    mptr->act_height = gdk_pixbuf_get_height (mptr->g_img_act_buff);			// number of rows (image size)
    mptr->act_rowstride = gdk_pixbuf_get_rowstride (mptr->g_img_act_buff);		// get pixels per row... may change tho
    mptr->act_pixels = gdk_pixbuf_get_pixels (mptr->g_img_act_buff);			// get pointer to pixel data

    stat=TRUE;
    }
  else 
    {
    stat=FALSE;
    }
      
  printf("Image loader:  exit\n");
  return(stat);
}

// Function to process either a single image or a pair of images against each other
// This function relies on the gdk_pixelbuf format.
// Input:	the requested process (see #defs), last_pass=defines the file name it will be saved under (see below)
int image_processor(GdkPixbuf *g_img_src1_buff, int img_proc_type)
{
    int		src1_n_ch;
    int		src1_cspace;
    int		src1_alpha;
    int		src1_bits_per_pixel;
    int 	src1_height;
    int 	src1_width;
    int 	src1_rowstride;
    guchar 	*src1_pixels;
    
    GdkPixbuf	*g_img_diff_buff;
    int		diff_n_ch;
    int		diff_cspace;
    int		diff_alpha;
    int		diff_bits_per_pixel;
    int 	diff_height;
    int 	diff_width;
    int 	diff_rowstride;
    guchar 	*diff_pixels;

    guchar 	*pix1,*pixdif,*pix_new;
    int		pix_neighbor[3][3][4];
    int		h,i,j,k,ci,xpix,ypix,curx,cury,pix_sum[4];
    int		gray_scale_val;
    float	x_filter[3][3],y_filter[3][3];
    int 	delta[4];

    float	RG_target,RB_target,GB_target;
    float 	RG_actual,RB_actual,GB_actual;
    float 	clr_tol;
    
    //printf("Image Processor:  entry\n");

    // verify image data availability
    if(g_img_src1_buff==NULL)return(FALSE);
    
    // define default filters
    for(h=0;h<3;h++){for(i=0;i<3;i++)x_filter[h][i]=1.0;}
    for(h=0;h<3;h++){for(i=0;i<3;i++)y_filter[h][i]=1.0;}

    // get color and pixels per row information of 1st source image
    src1_n_ch = gdk_pixbuf_get_n_channels (g_img_src1_buff);		// n channels - should always be 4
    src1_cspace = gdk_pixbuf_get_colorspace (g_img_src1_buff);		// color space - should be RGB
    src1_alpha = gdk_pixbuf_get_has_alpha (g_img_src1_buff);		// alpha - should be FALSE
    src1_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_src1_buff); // bits per pixel - should be 8
    src1_width = gdk_pixbuf_get_width (g_img_src1_buff);		// number of columns (image size)
    src1_height = gdk_pixbuf_get_height (g_img_src1_buff);		// number of rows (image size)
    src1_rowstride = gdk_pixbuf_get_rowstride (g_img_src1_buff);	// get pixels per row... may change tho
    src1_pixels = gdk_pixbuf_get_pixels (g_img_src1_buff);		// get pointer to pixel data

    // create new pixel buffer to hold processed image
    // this is done best by just copying the original source 1 image and overwriting the pixel values
    g_img_diff_buff = gdk_pixbuf_copy(g_img_src1_buff);	
    if(g_img_diff_buff==NULL)return(FALSE);
    diff_n_ch = gdk_pixbuf_get_n_channels (g_img_diff_buff);		// n channels - should always be 4
    diff_cspace = gdk_pixbuf_get_colorspace (g_img_diff_buff);		// color space - should be RGB
    diff_alpha = gdk_pixbuf_get_has_alpha (g_img_diff_buff);		// alpha - should be FALSE
    diff_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_diff_buff); // bits per pixel - should be 8
    diff_width = gdk_pixbuf_get_width (g_img_diff_buff);		// number of columns (image size)
    diff_height = gdk_pixbuf_get_height (g_img_diff_buff);		// number of rows (image size)
    diff_rowstride = gdk_pixbuf_get_rowstride (g_img_diff_buff);	// get pixels per row... may change tho
    diff_pixels = gdk_pixbuf_get_pixels (g_img_diff_buff);		// get pointer to pixel data
    
    // define edge filters - Sobel for now
    if(img_proc_type==IMG_EDGE_DETECT_RED || img_proc_type==IMG_EDGE_DETECT_ALL || img_proc_type==IMG_EDGE_DETECT_ADD)
      {
      x_filter[0][0]=1.0;  x_filter[0][1]=0.0;  x_filter[0][2]=(-1.0);
      x_filter[1][0]=2.0;  x_filter[1][1]=0.0;  x_filter[1][2]=(-2.0);
      x_filter[2][0]=1.0;  x_filter[2][1]=0.0;  x_filter[2][2]=(-1.0);

      y_filter[0][0]=1.0;  y_filter[0][1]=2.0;  y_filter[0][2]=1.0;
      y_filter[1][0]=0.0;  y_filter[1][1]=0.0;  y_filter[1][2]=0.0;
      y_filter[2][0]=(-1.0);  y_filter[2][1]=(-2.0);  y_filter[2][2]=(-1.0);
      }

    // define smoothing filter
    if(img_proc_type==IMG_SMOOTH)
      {
      // define smoothing weighting matrix
      x_filter[0][0]=0.50;  x_filter[0][1]=2.00;  x_filter[0][2]=0.50;
      x_filter[1][0]=2.00;  x_filter[1][1]=2.00;  x_filter[1][2]=2.00;
      x_filter[2][0]=0.50;  x_filter[2][1]=2.00;  x_filter[2][2]=0.50;
      }
    
    // define smoothing filter
    if(img_proc_type==IMG_DENOISE)
      {
      // define noise reduction weighting matrix
      x_filter[0][0]=1.00;  x_filter[0][1]=1.00;  x_filter[0][2]=1.00;
      x_filter[1][0]=1.00;  x_filter[1][1]=1.00;  x_filter[1][2]=1.00;
      x_filter[2][0]=1.00;  x_filter[2][1]=1.00;  x_filter[2][2]=1.00;
      }
    
    
    // loop thru each pixel and apply filtering/processing
    for(ypix=0; ypix<src1_height; ypix++)
      {
      for(xpix=0; xpix<src1_width; xpix++)
        {
	pix1 = src1_pixels + ypix * src1_rowstride + xpix * src1_n_ch;	// load pixel from src1
	pixdif = diff_pixels + ypix * diff_rowstride + xpix * diff_n_ch;// define target pixel for diff
	
	// apply edge detection
	if(img_proc_type==IMG_EDGE_DETECT_RED || img_proc_type==IMG_EDGE_DETECT_ALL || img_proc_type==IMG_EDGE_DETECT_ADD)
	  {
	      
	  // derive neighboring pixel values, apply filter, and sum value
	  for(ci=0;ci<4;ci++){pix_sum[ci]=0;}
	  for(h=(-1);h<2;h++)						// loop thru rows of filter
	    {
	    cury=ypix+h;
	    if(cury<0)cury=0;
	    if(cury>=src1_height)cury=src1_height-1;
	    for(i=(-1);i<2;i++)						// loop thru columns of filter
	      {
	      curx=xpix+i;
	      if(curx<0)curx=0;
	      if(curx>=src1_width)curx=src1_width-1;
	      pix_new = src1_pixels + cury * src1_rowstride + curx * src1_n_ch;	// RGBA array
	      
	      // filter detector to apply only to red laser light
	      //if(pix_new[0]<pix_new[1] || pix_new[0]<pix_new[2])	// if blue or green dominant over red...
	      if(img_proc_type==IMG_EDGE_DETECT_RED)
	        {
	        if(pix_new[0]<100 || pix_new[1]>80 || pix_new[2]>80)
		  {
		  pix_new[0]=0;						// ... make pixel black
		  pix_new[1]=0;
		  pix_new[2]=0;
		  }
	        }
    
	      pix_neighbor[h+1][i+1][0] = (pix_new[0]*0.3) + (pix_new[1]*0.59) + (pix_new[2]*0.11);	// convert to gray scale
	      
	      pix_sum[0] += (pix_neighbor[h+1][i+1][0] * x_filter[h+1][i+1]);	// apply x filter
	      pix_sum[1] += (pix_neighbor[h+1][i+1][0] * y_filter[h+1][i+1]);	// apply y filter
	      }
	    }

	  // calc magnitude
	  pixdif[0]=sqrt(pix_sum[0]*pix_sum[0] + pix_sum[1]*pix_sum[1]);
	  
	  // compare to threshold to force to black and white
	  if(edge_detect_threshold>0)
	    {
	    if(img_proc_type==IMG_EDGE_DETECT_RED)
	      {
	      if(pixdif[0]>edge_detect_threshold) {pixdif[0]=255;} else {pixdif[0]=0;}
	      }
	    if(img_proc_type==IMG_EDGE_DETECT_ALL)
	      {
	      if(pixdif[0]>edge_detect_threshold) {pixdif[0]=255;} else {pixdif[0]=0;}
	      }
	    if(img_proc_type==IMG_EDGE_DETECT_ADD)
	      {
	      gray_scale_val=(pix1[0]+pix1[1]+pix1[2])/3;
	      if(pixdif[0]>edge_detect_threshold)
	        {
		pixdif[0]=gray_scale_val-25;
		if(pixdif[0]<0)pixdif[0]=0;
		}
	      else 
	        {pixdif[0]=gray_scale_val;}
	      }
	    }
	  
	  // set other channels
	  pixdif[1]=pixdif[0];
	  pixdif[2]=pixdif[0];
	    
	  }

	// apply color detection
	if(img_proc_type==IMG_COLOR_DETECT)
	  {
	  // use RGB ratios in attempt to make immune to brightness/darkness of image
	  RG_target = ( (127.0/123.0) + (88.0/76.0) + (147.0/122.0) + (225.0/217.0) ) / 4.0;	// derived imperically... should be user pick on image
	  RB_target = ( (127.0/137.0) + (88.0/96.0) + (147.0/144.0) + (225.0/215.0) ) / 4.0;
	  GB_target = ( (137.0/123.0) + (96.0/76.0) + (144.0/122.0) + (215.0/217.0) ) / 4.0;
	  clr_tol = (float)(edge_detect_threshold)/255.0;
	  RG_actual = (float)(pixdif[0]) / (float)(pixdif[1]);
	  RB_actual = (float)(pixdif[0]) / (float)(pixdif[2]);
	  GB_actual = (float)(pixdif[1]) / (float)(pixdif[2]);
	  
	  //printf("RGt=%f RBt=%f GBt=%f   RGa=%f RBa=%f GBa=%f \n",RG_target,RB_target,GB_target,RG_actual,RB_actual,GB_actual);
	  
	  // if within color threshold... set to black
	  if((RG_actual > (RG_target-clr_tol)) && (RG_actual < (RG_target+clr_tol)))
	    {
	    if((RB_actual > (RB_target-clr_tol)) && (RB_actual < (RB_target+clr_tol)))
	      {
	      if((GB_actual > (GB_target-clr_tol)) && (GB_actual < (GB_target+clr_tol)))
	        {
		pixdif[0]=0; pixdif[1]=0; pixdif[2]=0;
	        }
	      }
	    }
	  
	  }

	// apply image filtering
	if(img_proc_type==IMG_FILTER)
	  {

	  }
	
	// apply image smoothing by averaging the central pixel with its 8 neighbors
	// the eight neighbors are defined via the 3x3 matrix where the center element is the target pixel.
	if(img_proc_type==IMG_SMOOTH)
	  {
	  // derive neighboring pixel values, apply filter, and sum value
	  for(ci=0;ci<4;ci++){pix_sum[ci]=0;}
	  for(h=(-1);h<2;h++)						// loop thru rows of filter
	    {
	    cury=ypix+h;
	    if(cury<0)cury=0;
	    if(cury>=src1_height)cury=src1_height-1;
	    for(i=(-1);i<2;i++)						// loop thru columns of filter
	      {
	      curx=xpix+i;
	      if(curx<0)curx=0;
	      if(curx>=src1_width)curx=src1_width-1;
	      pix_new = src1_pixels + cury * src1_rowstride + curx * src1_n_ch;	// RGBA array
	      for(ci=0;ci<4;ci++)
	        {
		pix_neighbor[h+1][i+1][ci] = (int)(x_filter[h+1][i+1] * (float)pix_new[ci]);
		pix_sum[ci] += pix_neighbor[h+1][i+1][ci];
		}
	      }
	    }
	  
	  // average sum
	  for(ci=0;ci<4;ci++)
	    {
	    pix_sum[ci]=floor(pix_sum[ci]/9);
	    if(pix_sum[ci]<0)pix_sum[ci]=0;
	    if(pix_sum[ci]>255)pix_sum[ci]=255;
	    pixdif[ci]=pix_sum[ci];
	    }
	  }	

	// apply image denoise by looking at the value of pixels around the one of interest and
	// tweaking its value if at an extreme.
	if(img_proc_type==IMG_DENOISE)
	  {
	  // derive neighboring pixel values, apply filter, and sum value
	  for(ci=0;ci<4;ci++){pix_sum[ci]=0;}
	  for(h=(-1);h<2;h++)						// loop thru rows of filter
	    {
	    cury=ypix+h;
	    if(cury<0)cury=0;
	    if(cury>=src1_height)cury=src1_height-1;
	    for(i=(-1);i<2;i++)						// loop thru columns of filter
	      {
	      curx=xpix+i;
	      if(curx<0)curx=0;
	      if(curx>=src1_width)curx=src1_width-1;
	      pix_new = src1_pixels + cury * src1_rowstride + curx * src1_n_ch;	// RGBA array
	      for(ci=0;ci<4;ci++)
	        {
		pix_neighbor[h+1][i+1][ci] = (int)(x_filter[h+1][i+1] * (float)pix_new[ci]);
		pix_sum[ci] += pix_neighbor[h+1][i+1][ci];
		}
	      }
	    }
	  
	  // average sum
	  for(ci=0;ci<4;ci++)
	    {
	    pix_sum[ci]=floor(pix_sum[ci]/9);
	    if(pix_sum[ci]<(pix_new[ci]-1))pix_sum[ci]=pix_new[ci];
	    if(pix_sum[ci]>(pix_new[ci]+1))pix_sum[ci]=pix_new[ci];
	    pixdif[ci]=pix_sum[ci];
	    }
	  }	

	// apply image grayscaling
	if(img_proc_type==IMG_GRAYSCALE)
	  {
	  gray_scale_val = (pixdif[0]*0.3) + (pixdif[1]*0.59) + (pixdif[2]*0.11);	// convert to gray scale
	
	  //if(gray_scale_val<100)gray_scale_val=0;				// set lower filter
	  //if(gray_scale_val>225)gray_scale_val=0;				// set upper filter
	
	  pixdif[0]=gray_scale_val;
	  pixdif[1]=gray_scale_val;
	  pixdif[2]=gray_scale_val;
	  }
	}
      }
    
    // save image to file
    gdk_pixbuf_save (g_img_diff_buff, "/home/aa/Documents/4X3D/tb_diff.jpg", "jpeg", &my_errno, "quality", "100", NULL);

    // clean up
    g_object_unref(g_img_diff_buff);
    
    //printf("Image Processor:  exit\n");

    return(TRUE);
}

// Function to convert index of table images into STL model(s)
// It works by running an edge detect on each image to pick up the laser scan line.
// The file name contains the x position of the line.  The left 20mm is used to establish
// the build table z.  The camera location and viewing params establish the trajectory
// that grid line takes across the table.  The code then steps down each grid line in
// the y direction and determines if the laser line is on the grid line or above it,
// and if so, by how much.  At that point we have x, y, and z to set a facet vertex.
int laser_scan_images_to_STL(void)
{
    GdkPixbuf	*g_img_new,*g_img_diff_buff;				// difference image
    int		diff_n_ch;
    int		diff_cspace;
    int		diff_alpha;
    int		diff_bits_per_pixel;
    int 	diff_height;
    int 	diff_width;
    int 	diff_rowstride;
    guchar 	*diff_pixels;
    guchar 	*pix_new,*pix_lf,*pix_rt,*pix_up,*pix_dn;
    float	pix_sum,pix_sum_lf,pix_sum_rt,pix_sum_up,pix_sum_dn;
    float 	pix_avg;
    
    char	img_name[255];
    int		h,i,j;
    int		xtbl,ytbl;
    int		xmin,ymin,zmin,xmax,ymax,zmax;
    int		origin_found=FALSE;
    int		x_edge_found=FALSE;
    int 	img_org=0;
    int		x_crnt,x_prev,x_next;
    float 	vl_inc,vl_delta;
    float 	vert_offset,vert_cnt,vert_total;
    float 	vangle,oss,osc,ots,otc;
    float 	pix_per_mm,pixel_height;
    float 	hl_org,vl_org,zl_org;
    float	hl,vl,zl,x1,y1,z1;
    float	tbl_inc,xlim,ylim;
    float	xstart,ystart;
    int		in_object,on_wall;
    double	amt_total=0,amt_done=0.0;
    GdkRGBA 	color;

    int 	x,y,step_size;
    int		vtx_cnt=0,fct_cnt=0;
    float 	Czmax,Dzmax;
    model 	*mptr;
    facet 	*fptr,*fnew,*fbtm,*fngh;
    vertex 	*vtxnew[4],*vtx_ptr,*vtx_new;
    vertex 	*vtxyA,*vtxyB,*vtxyC,*vtxyD;
    
    // init params
    pix_per_mm = 0.1;
    edge_detect_threshold=90;
    //debug_flag=603;
    
    // set orientation values so that the xy table grid can be superimposed
    // onto the images.  this allows image checking at known xy positons.
    MVdisp_spin=-0.800;							// more spins clockwise on image
    MVdisp_tilt= 3.640;							// less makes grid flatter
    MVview_scale=2.300;							// more makes grid bigger on image
    MVdisp_cen_x=(BUILD_TABLE_LEN_X/2);
    MVdisp_cen_y=0-(BUILD_TABLE_LEN_Y/2);
    MVdisp_cen_z=0.0;
    MVgxoffset=415;							// more pushes grid to right of image
    MVgyoffset=195;							// more pushes grid down on image
    pers_scale=0.65;							// more bends axis inward

    // init viewing angles
    vangle=set_view_edge_angle*PI/180;
    oss=sin(MVdisp_spin);						// pre-calculate to save time downstream
    osc=cos(MVdisp_spin);
    ots=sin(MVdisp_tilt);
    otc=cos(MVdisp_tilt);

    // show the progress bar
    if(debug_flag==0)
      {
      sprintf(scratch,"  Build Table Scan ");
      gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
      sprintf(scratch,"  Step 1 of 2:  Processing scan data...  ");
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
      gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
      gtk_widget_set_visible(win_info,TRUE);
      gtk_widget_queue_draw(win_info);
      while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
      }

    // loop thru all the scan images using the index as our guide
    x_prev=0; x_crnt=0; x_next=0;
    for(i=0;i<scan_image_count;i++)
      {
      // update status to user
      if(debug_flag==0)
        {
        amt_done = (double)(i)/(double)(scan_image_count);
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
        while (g_main_context_iteration(NULL, FALSE));
	}

      // load scan image
      sprintf(img_name,"/home/aa/Documents/4X3D/TableScan/x_%000d.jpg",scan_img_index[i]); 	// build image name
      g_img_new = gdk_pixbuf_new_from_file_at_scale(img_name, (-1),600, TRUE, NULL);		// load new buffer with image
      if(g_img_new==NULL)continue;
      image_processor(g_img_new,IMG_EDGE_DETECT_ALL);

      // load image with edge detected objects
      g_img_diff_buff = gdk_pixbuf_new_from_file_at_scale("/home/aa/Documents/4X3D/tb_diff.jpg", (-1), 600, TRUE, NULL);	// load new buffer with image
      if(g_img_diff_buff==NULL)continue;
      diff_n_ch = gdk_pixbuf_get_n_channels (g_img_diff_buff);		// n channels - should always be 4
      diff_cspace = gdk_pixbuf_get_colorspace (g_img_diff_buff);	// color space - should be RGB
      diff_alpha = gdk_pixbuf_get_has_alpha (g_img_diff_buff);		// alpha - should be FALSE
      diff_bits_per_pixel = gdk_pixbuf_get_bits_per_sample (g_img_diff_buff); // bits per pixel - should be 8
      diff_width = gdk_pixbuf_get_width (g_img_diff_buff);		// number of columns (image size)
      diff_height = gdk_pixbuf_get_height (g_img_diff_buff);		// number of rows (image size)
      diff_rowstride = gdk_pixbuf_get_rowstride (g_img_diff_buff);	// get pixels per row... may change tho
      diff_pixels = gdk_pixbuf_get_pixels (g_img_diff_buff);		// get pointer to pixel data
      if(diff_pixels==NULL)continue;

      // search for the x position of this image by scanning along the y=0 grid line from x=415 to x=0
      // and checking to see if we hit a pixel (along with a couple neighbors) that is lit up.  this value
      // ought to be close to the x value represented in the file name, but not exact.
      x_edge_found=FALSE;
      y1=0.0; z1=0.0;
      xstart=x_prev+25;
      //if(origin_found==FALSE){xstart=BUILD_TABLE_MAX_X;} else {xstart=x_prev+25;}
      if(xstart>BUILD_TABLE_MAX_X)xstart=BUILD_TABLE_MAX_X;
      while(origin_found==FALSE)
        {
	  for(x1=xstart;x1>(-10);x1--)
	    {
	    zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(0-y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	    hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	    vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	    if(hl<0)hl=0; if(hl>=diff_width)hl=diff_width-1;
	    if(vl<0)vl=0; if(vl>=diff_height)vl=diff_height-1;
	
	    pix_new = diff_pixels + (int)(vl) * diff_rowstride + (int)(hl) * diff_n_ch;	// target pixel
	    pix_lf  = diff_pixels + (int)(vl) * diff_rowstride + (int)(hl-1) * diff_n_ch;	// left of target pixel
	    pix_rt  = diff_pixels + (int)(vl) * diff_rowstride + (int)(hl+1) * diff_n_ch;	// right of target pixel
	    pix_up  = diff_pixels + (int)(vl-1) * diff_rowstride + (int)(hl) * diff_n_ch;	// above target pixel
	    pix_dn  = diff_pixels + (int)(vl+1) * diff_rowstride + (int)(hl) * diff_n_ch;	// below of target pixel
	    pix_sum = (float)(pix_new[0] + pix_new[1] + pix_new[2])/3.0;			// get average level of target
	    pix_sum_lf = (float)(pix_lf[0] + pix_lf[1] + pix_lf[2])/3.0;			// get average level of left
	    pix_sum_rt = (float)(pix_rt[0] + pix_rt[1] + pix_rt[2])/3.0;			// get average level of right
	    pix_sum_up = (float)(pix_up[0] + pix_up[1] + pix_up[2])/3.0;			// get average level of above
	    pix_sum_dn = (float)(pix_dn[0] + pix_dn[1] + pix_dn[2])/3.0;			// get average level of below
		  
	    if(debug_flag==603)								// if debugging, change color to see it
	      {
	      pix_new[0]=200; pix_new[1]=50; pix_new[2]=50; pix_new[3]=100;
	      pix_lf[0] =200; pix_lf[1] =50; pix_lf[2] =50; pix_lf[3] =100;
	      pix_rt[0] =200; pix_rt[1] =50; pix_rt[2] =50; pix_rt[3] =100;
	      pix_up[0] =200; pix_up[1] =50; pix_up[2] =50; pix_up[3] =100;
	      pix_dn[0] =200; pix_dn[1] =50; pix_dn[2] =50; pix_dn[3] =100;
	      gdk_pixbuf_save (g_img_diff_buff, "/home/aa/Documents/4X3D/tb_diff.jpg", "jpeg", &my_errno, "quality", "100", NULL);
	      while (g_main_context_iteration(NULL, FALSE));				// ... update display promptly
	      }
		  
	    pix_avg=(pix_sum + pix_sum_dn + pix_sum_rt)/3.0;				// check if a laser line hit
	    if(pix_avg>100.0)								// if at laser scan line threshold...
	      {
	      x_edge_found=TRUE;
	      if(debug_flag==604)printf("X found:  x1=%f  scan number = %d   index_val=%d \n",x1,i,scan_img_index[i]);
	      if(origin_found==FALSE)
		{
		img_org=scan_img_index[i];							// ... save offset
		hl_org=hl; vl_org=vl; zl_org=zl;						// ... save pixel position
		origin_found=TRUE; 								// ... set flag as such
		}
	      break;									// ... and process rest
	      }
	    }
	// if we did not hit the scan line, move slight along y axis and try again
	y1+=2;
	if(y1>20)break;
	}
      if(origin_found==FALSE)continue;
      
      if(x_edge_found==FALSE)
        {
	if(debug_flag==604)printf("Warning: x edge not found! \n");
	x1=scan_img_index[i]-img_org;
	}

      // since the xy matrix that holds z values is 1x1mm and the scan may be at a lower resolution, say
      // every 3mm there is an image, we need to copy the last known x-row to fill in the gaps
      if(i>0)								// if we are past the first image...
	{
	x_crnt=x1;							// get current x value at current index
	x_next=x_prev+1;						// set x value of next row to fill
	while(x_next<x_crnt)						// while the next row is blank...
	  {
	  // copy the previous x row to the blank row
	  if(debug_flag==601)printf("scan processing: i=%d  crt=%d  pre=%d  nxt=%d\n",i,x_crnt,x_prev,x_next);
	  for(h=0;h<415;h++)
	    {
	    xy_on_table[x_next][h]=xy_on_table[x_prev][h];		// copy last know good row to blank row
	    if(debug_flag==602)printf("  h=%d  zpre=%d  znxt=%d \n",h,xy_on_table[x_prev][h],xy_on_table[x_next][h]);
	    }
	  x_next++;
	  }
	}  
      x_prev=x1;							// save x value for next time thru loop
      if(debug_flag==601)printf("scan processing: i=%d  x_%d.jpg \n",i,scan_img_index[i]);

      vert_cnt=0;							// init y step counter for offset averaging
      vert_total=0;							// init vert offset accumulator
      vert_offset=0;							// init average vertical offset
      tbl_inc=2;
      y1=0; z1=(-5);							// use z offset to ensure we are below scan line when we start
      while(y1<(BUILD_TABLE_LEN_Y-25))
	{
	// calc pixel position in image at x1, y1, and z1=0.  this gives the xy position on the table (i.e. the grid line).
	zl=1+(600 + (MVview_scale*(x1-MVdisp_cen_x))*oss - (MVview_scale*(0-y1-MVdisp_cen_y))*osc + (MVview_scale*(z1-MVdisp_cen_z))*ots)/600*pers_scale;
	hl=((MVview_scale*(x1-MVdisp_cen_x))*osc+(MVview_scale*(0-y1-MVdisp_cen_y))*oss)/zl+MVgxoffset;
	vl=((MVview_scale*(x1-MVdisp_cen_x))*oss*ots-(MVview_scale*(0-y1-MVdisp_cen_y))*osc*ots+(MVview_scale*(z1-MVdisp_cen_z))*otc)/zl+MVgyoffset;
	if(hl<0)hl=0; if(hl>=diff_width)hl=diff_width-1;
	if(vl<0)vl=0; if(vl>=diff_height)vl=diff_height-1;
	
	// now move up in vl (which means toward max image ht) to find where the pixel value is above black (zero).
	vl_inc=vl;
	while(vl_inc>0)
	  {
	  pix_new = diff_pixels + (int)(vl_inc) * diff_rowstride + (int)(hl) * diff_n_ch;	// target pixel
	  pix_lf  = diff_pixels + (int)(vl_inc) * diff_rowstride + (int)(hl-1) * diff_n_ch;	// left of target pixel
	  pix_rt  = diff_pixels + (int)(vl_inc) * diff_rowstride + (int)(hl+1) * diff_n_ch;	// right of target pixel
	  pix_sum = (float)(pix_new[0] + pix_new[1] + pix_new[2])/3.0;				// get average level of target
	  pix_sum_lf = (float)(pix_lf[0] + pix_lf[1] + pix_lf[2])/3.0;				// get average level of left
	  pix_sum_rt = (float)(pix_rt[0] + pix_rt[1] + pix_rt[2])/3.0;				// get average level of right
	  
	  if(debug_flag==603)pix_new[0]=0; pix_new[1]=150; pix_new[2]=150; pix_new[3]=0;
	  
	  pix_avg=(pix_sum + pix_sum_lf + pix_sum_rt)/3.0;
	  if(pix_avg>100.0)break;
	  vl_inc--;
	  }
	  
	// if we had to move more than a bit, there is something there
	xy_on_table[(int)(x1)][(int)(y1)]=0;				// set to 0 height
	vl_delta=vl-vl_inc;						// number of pixels moved
	
	// calc average vertical offset using pixels along the scan from y=0 to y=10
	vert_cnt++;							// increment y step counter
	if(vert_cnt<10)							// if still near left edge...
	  {
	  vert_total += vl_delta;					// add delta to total accumulator
	  vert_offset = vert_total/vert_cnt;				// calc average
	  }
	vl_delta -= vert_offset;					// remove offset from reading
	if(vl_delta<0)vl_delta=0;					// nothing should be below table
	
	// if vertical scan ran all the way to the top of the image we hit a blind spot (i.e. no laser edge to stop
	// the scan).  the default is to just reset these values to "on table".
	if(vl_inc<5)vl_delta=0;						// if scan ran to top of image, set to on table

	//pixel_height=10*vl_delta/pix_per_mm - 5.0;			// calc z height.  note the 10X for better resolution and z offset correction
	pixel_height=10*vl_delta/pix_per_mm;				// calc z height.  note the 10X for better resolution and z offset correction
	if(pixel_height>10.0)						// filter on sensativity.  a value of 10 = 1mm
	  {
	  xtbl=(int)(x1);						// adjust offsets out of scan elements
	  ytbl=(int)(y1);
	  if(xtbl<0)xtbl=0;  if(xtbl>414)xtbl=414;			// insure withing array bounds
	  if(ytbl<0)ytbl=0;  if(ytbl>414)ytbl=414;
	  xy_on_table[xtbl][ytbl]=(int)(pixel_height/10+0.5);		// save height (rounded to nearest mm)
	  j=1;
	  while(j<tbl_inc)						// if not checking every y value... fill in blanks
	    {
	    xy_on_table[xtbl][ytbl+j]=(int)(pixel_height/10+0.5);
	    j++;
	    }
	  }

        y1+=tbl_inc;
	if(x1<0 || x1>414)break;
	if(y1<0 || y1>414)break;
	}

      if(debug_flag==603)
        {
	gdk_pixbuf_save (g_img_diff_buff, "/home/aa/Documents/4X3D/tb_diff.jpg", "jpeg", &my_errno, "quality", "100", NULL);
        while (g_main_context_iteration(NULL, FALSE));			// update display promptly
	}
      
      //if(x1>200 && x1<220)while(!kbhit());

      if(g_img_diff_buff!=NULL)g_object_unref(g_img_diff_buff);
      if(g_img_new!=NULL)g_object_unref(g_img_new);
      }

    // each call applies more smoothing
    laser_scan_matrix();
    laser_scan_matrix();


    
    // now that we have a matrix where the indexes give us x & y and the value is z we need
    // to covert this to an STL object. the upper surface is made by subdividing neighboring matrix
    // cells into triangles at their xy&z.  the lower stl surface that sits on the build table can
    // be made at the same time but with z=0.
    
    display_model_flag=FALSE;
    
    if(debug_flag==0)
      {
      sprintf(scratch,"  Step 2 of 2:  Generating models...  ");
      gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
      gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
      gtk_widget_set_visible(win_info,TRUE);
      gtk_widget_queue_draw(win_info);
      while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
      amt_total = (double)(BUILD_TABLE_MAX_X);
      }

    // create a new model to load data into
    mptr=model_make();
    
    // loop thru xy matrix and build top and btm surface facets
    xmin=BUILD_TABLE_MAX_X; ymin=BUILD_TABLE_MAX_Y; zmin=BUILD_TABLE_MAX_Z;
    xmax=0; ymax=0; zmax=0;
    step_size=2;
    for(x=0;x<(BUILD_TABLE_MAX_X-1);x+=step_size)
      {
      if(debug_flag==0)
        {
        amt_done = (double)(x) / amt_total;  
        gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
        while (g_main_context_iteration(NULL, FALSE));
	}
      for(y=0;y<(BUILD_TABLE_MAX_Y-1);y+=step_size)
        {
        // update status to user
	//printf("x=%d y=%d  fct_cnt=%d\n",x,y,fct_cnt);

	for(i=0;i<4;i++)vtxnew[i]=NULL;					// init to null
	
	// if this point has a z value above zero...
	if(xy_on_table[x][y]>0 && xy_on_table[x+step_size][y]>0 && xy_on_table[x][y+step_size]>0)	
	  {
	  // make the first vtx
	  vtxnew[0]=vertex_make();						// ... make its vtx
	  vtxnew[0]->x=x; vtxnew[0]->y=y; vtxnew[0]->z=xy_on_table[x][y];	// ... load its xyz value
	  vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[0],MODEL,CLOSE_ENOUGH);
	  if(vtx_ptr!=vtxnew[0]){free(vtxnew[0]); vertex_mem--; vtxnew[0]=vtx_ptr;}
	  if(debug_flag==611)printf("  vtx0 made \n");
	  
	  // check for 2nd vtx
	  if(xy_on_table[x+step_size][y]>0)
	    {
	    vtxnew[1]=vertex_make();
	    vtxnew[1]->x=x+step_size; vtxnew[1]->y=y; vtxnew[1]->z=xy_on_table[x+step_size][y];
	    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[1],MODEL,CLOSE_ENOUGH);
	    if(vtx_ptr!=vtxnew[1]){free(vtxnew[1]); vertex_mem--; vtxnew[1]=vtx_ptr;}
	    if(debug_flag==611)printf("  vtx1 made \n");
	    }
	    
	  // check for 3rd vtx
	  if(xy_on_table[x][y+step_size]>0)
	    {
	    vtxnew[2]=vertex_make();
	    vtxnew[2]->x=x; vtxnew[2]->y=y+step_size; vtxnew[2]->z=xy_on_table[x][y+step_size];
	    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[2],MODEL,CLOSE_ENOUGH);
	    if(vtx_ptr!=vtxnew[2]){free(vtxnew[2]); vertex_mem--; vtxnew[2]=vtx_ptr;}
	    if(debug_flag==611)printf("  vtx2 made \n");
	    }
	    
	  // check for 4th vtx
	  if(xy_on_table[x+step_size][y+step_size]>0)
	    {
	    vtxnew[3]=vertex_make();
	    vtxnew[3]->x=x+step_size; vtxnew[3]->y=y+step_size; vtxnew[3]->z=xy_on_table[x+step_size][y+step_size];
	    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[3],MODEL,CLOSE_ENOUGH);
	    if(vtx_ptr!=vtxnew[3]){free(vtxnew[3]); vertex_mem--; vtxnew[3]=vtx_ptr;}
	    if(debug_flag==611)printf("  vtx3 made \n");
	    }

	  // determine if there are enough vtxs to form a full facet (or two)
	  vtx_cnt=0;
	  for(i=0;i<4;i++){if(vtxnew[i]!=NULL)vtx_cnt++;}
	  if(vtx_cnt<3)							// if not enough...
	    {
	    for(i=0;i<4;i++)								// ... check all 4
	      {
	      if(vtxnew[i]!=NULL){free(vtxnew[i]); vertex_mem--; vtxnew[i]=NULL;}	// ... remove from memory
	      }
	    if(debug_flag==611)printf("  not enough vtxs... continuing. \n");
	    xy_on_table[x][y]=0;							// ... zero out
	    continue;									// ... move onto next xy pt
	    }
	  if(debug_flag==611)printf("  %d vtxs found \n",vtx_cnt);
		
	  // determine which vtxs to use for this facet
	  fct_cnt++;
	  fnew=facet_make();
	  if(vtxnew[0]==NULL && vtxnew[1]!=NULL && vtxnew[2]!=NULL && vtxnew[3]!=NULL)
	    {
	    fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[3]; fnew->vtx[2]=vtxnew[1];
	    }
	  else if(vtxnew[0]!=NULL && vtxnew[1]==NULL && vtxnew[2]!=NULL && vtxnew[3]!=NULL)
	    {
	    fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[3]; fnew->vtx[2]=vtxnew[0];
	    }
	  else if(vtxnew[0]!=NULL && vtxnew[1]!=NULL && vtxnew[2]==NULL && vtxnew[3]!=NULL)
	    {
	    fnew->vtx[0]=vtxnew[3]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
	    }
	  else if(vtxnew[0]!=NULL && vtxnew[1]!=NULL && vtxnew[2]!=NULL && vtxnew[3]==NULL)
	    {
	    fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
	    }
	  else if(vtxnew[0]!=NULL && vtxnew[1]!=NULL && vtxnew[2]!=NULL && vtxnew[3]!=NULL)
	    {
	    fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
	    }
	  if(debug_flag==611)printf("  facet vtx assignment done. \n");
	    
	  // determine if min/max vals
	  for(i=0;i<3;i++)
	    {
	    if(fnew->vtx[i]->x<xmin)xmin=fnew->vtx[i]->x;
	    if(fnew->vtx[i]->x>xmax)xmax=fnew->vtx[i]->x;
	    if(fnew->vtx[i]->y<ymin)ymin=fnew->vtx[i]->y;
	    if(fnew->vtx[i]->y>ymax)ymax=fnew->vtx[i]->y;
	    if(fnew->vtx[i]->z<zmin)zmin=fnew->vtx[i]->z;
	    if(fnew->vtx[i]->z>zmax)zmax=fnew->vtx[i]->z;
	    }

	  // add the first facet at z height to MODEL structure
	  facet_unit_normal(fnew);
	  for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
	  facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);
	  //printf("  upper facet inserted at:  %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
	  
	  // copy this new facet less the z value to form the lower surface
	  fct_cnt++;
	  fbtm=facet_make();
	  for(i=0;i<3;i++)
	    {
	    vtx_new=vertex_copy(fnew->vtx[i],NULL);
	    vtx_new->z=0;
	    vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtx_new,MODEL,CLOSE_ENOUGH);
	    if(vtx_ptr!=vtx_new){free(vtx_new); vertex_mem--; vtx_new=vtx_ptr;}
	    fbtm->vtx[i]=vtx_new;
	    fbtm->vtx[i]->flist=facet_list_manager(fbtm->vtx[i]->flist,fbtm,ACTION_ADD);
	    }
	  fbtm->unit_norm->x=0;
	  fbtm->unit_norm->y=0;
	  fbtm->unit_norm->z=(-1);
	  facet_insert(mptr,mptr->facet_last[MODEL],fbtm,MODEL);
	  //printf("  lower facet inserted at:   %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
		  
	  // add second new upper facet if all 4 vtx are above z=0
	  if(vtxnew[0]!=NULL && vtxnew[1]!=NULL && vtxnew[2]!=NULL && vtxnew[3]!=NULL)
	    {
	    fct_cnt++;
	    fnew=facet_make();
	    fnew->vtx[0]=vtxnew[2];
	    fnew->vtx[1]=vtxnew[3];
	    fnew->vtx[2]=vtxnew[1];
	    facet_unit_normal(fnew);
	    for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
	    facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);
	    //printf("  bonus upper facet inserted at:  %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);

	    // copy this new facet less the z value to form the lower surface
	    fct_cnt++;
	    fbtm=facet_make();
	    for(i=0;i<3;i++)
	      {
	      vtx_new=vertex_copy(fnew->vtx[i],NULL);
	      vtx_new->z=0;
	      vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtx_new,MODEL,CLOSE_ENOUGH);
	      if(vtx_ptr!=vtx_new){free(vtx_new); vertex_mem--; vtx_new=vtx_ptr;}
	      fbtm->vtx[i]=vtx_new;
	      fbtm->vtx[i]->flist=facet_list_manager(fbtm->vtx[i]->flist,fbtm,ACTION_ADD);
	      }
	    fbtm->unit_norm->x=0;
	    fbtm->unit_norm->y=0;
	    fbtm->unit_norm->z=(-1);
	    facet_insert(mptr,mptr->facet_last[MODEL],fbtm,MODEL);
	    //printf("  bonus lower facet inserted at:   %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
	    }
		
	  }  	// end of if this xy_on_table has z value
	  
	// zero out spent xy grid values
	for(h=0;h<step_size;h++)
	  {
	  for(i=0;i<step_size;i++){xy_on_table[x+h][y+i]=0;}
	  }
	  
	}	// end of y loop
      }		// end of x loop

    // re-associate the entire block facet set 
    facet_find_all_neighbors(mptr->facet_first[MODEL]);
    
    // init status of all the new facets
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      fptr->status=(-1);
      fptr=fptr->next;
      }
    
    // finally the upper and lower surfaces are stiched together with vertical facets.
    // check if upper facet has a null neighbor edge.
    // if so, build a pair of wall facets between it and the same edge of the lower facet. 
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      if(fptr->status>0){fptr=fptr->next; continue;}
      for(i=0;i<3;i++)							// loop thru each of this facet's neighbors
        {
        fngh=fptr->fct[i];						// get pointer to neighbor
        if(fngh==NULL)							// if no neighbor exists, we found an edge
          {
          // at this point we know that fptr is a free edge facet.  and we know the free edge
	  // exists between vtx[i] and vtx[h] on this upper facet.  now we need to find
	  // the matching free edge vtxs in the matching lower facet that have the same xy.
	  h=i+1; if(h>2)h=0;
	  vtxyA=fptr->vtx[i]; 						// define vtx addresses of the upper free edge
	  vtxyB=fptr->vtx[h];
	  vtxyC=NULL;							// vtx addresses of lower facet not known yet
	  vtxyD=NULL;
      
	  // search facets with same xy but z at 0 to find lower facet with matching free edge
	  fbtm=mptr->facet_first[MODEL];
	  while(fbtm!=NULL)
	    {
	    if(fbtm->status==1){fbtm=fbtm->next; continue;}
	    if((fbtm->vtx[0]->z+fbtm->vtx[1]->z+fbtm->vtx[2]->z)>CLOSE_ENOUGH){fbtm=fbtm->next; continue;}
	    if(fbtm==fptr){fbtm=fbtm->next; continue;}
	    for(j=0;j<3;j++)
	      {
	      if(vtxyC==NULL){if(vertex_xy_compare(vtxyA,fbtm->vtx[j],TOLERANCE)==TRUE)vtxyC=fbtm->vtx[j];}
	      if(vtxyD==NULL){if(vertex_xy_compare(vtxyB,fbtm->vtx[j],TOLERANCE)==TRUE)vtxyD=fbtm->vtx[j];}
	      }
	    if(vtxyC!=NULL && vtxyD!=NULL)break;			// if found, exit early to save time
	    fbtm=fbtm->next;
	    }
      
	  // at this point we should have 2 top and 2 btm vtxs define... enough for 2 new wall facets
          if(vtxyA!=NULL && vtxyB!=NULL && vtxyC!=NULL && vtxyD!=NULL)
	    {
	    fbtm->status=1;
	    
	    //printf("\nfptr=%X  fbtm=%X  fngh=%X \n",fptr,fbtm,fngh);
	    //printf(" A=%X  B=%X  C=%X  D=%X \n",vtxyA,vtxyB,vtxyC,vtxyD);
	    //printf(" Ax=%d  Bx=%d  Cx=%d  Dx=%d \n", vtxyA->x,vtxyB->x,vtxyC->x,vtxyD->x);
	    //printf(" Ay=%d  By=%d  Cy=%d  Dy=%d \n", vtxyA->y,vtxyB->y,vtxyC->y,vtxyD->y);
	    //printf(" Az=%d  Bz=%d  Cz=%d  Dz=%d \n", vtxyA->z,vtxyB->z,vtxyC->z,vtxyD->z);
		
	    // build two new vertical facets between fptr and fbtm and add to support list from
	    // the two free edge vtxs.
	    fnew=facet_make();							// allocate memory and set default values for facet
	    vtxyA->flist=facet_list_manager(vtxyA->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    fnew->vtx[0]=vtxyA;							// assign which vtxs belong to this facet
	    fnew->vtx[1]=vtxyB;
	    fnew->vtx[2]=vtxyC;
	    facet_unit_normal(fnew);
	    facet_area(fnew);
	    fnew->status=4;							// flag this as a lower facet
	    fnew->member=fptr->member;						// keep track of which body it belongs to
	    facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);		// adds new facet to facet list
	
	    // make new facet between vtxB, vtxC, and vtxD
	    fnew=facet_make();							// allocate memory and set default values for facet
	    vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyD->flist=facet_list_manager(vtxyD->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    fnew->vtx[0]=vtxyB;							// assign which vtxs belong to this facet
	    fnew->vtx[1]=vtxyD;
	    fnew->vtx[2]=vtxyC;
	    facet_unit_normal(fnew);
	    facet_area(fnew);
	    fnew->status=4;							// flag this as a lower facet
	    fnew->member=fptr->member;						// keep track of which body it belongs to
	    facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);		// adds new facet to facet list
	    }
          }		// end of if neighbor NULL
	}		// end of for(i=0...
	
      fptr->status=1;
      fptr=fptr->next;
      }			// end of while(fptr!=NULL)
    
    // hide progress bar
    if(debug_flag==0)
      {
      gtk_widget_set_visible(win_info,FALSE);
      while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
      }

    // define model boundaries
    //printf("\n xmin=%d ymin=%d zmin=%d  xmax=%d ymax=%d zmax=%d\n",xmin,ymin,zmin,xmax,ymax,zmax);
    mptr->xoff[MODEL]=xmin; 	 mptr->yoff[MODEL]=ymin;	mptr->zoff[MODEL]=0;
    mptr->xorg[MODEL]=xmin; 	 mptr->yorg[MODEL]=ymin;	mptr->zorg[MODEL]=0;
    mptr->xmax[MODEL]=xmax-xmin; mptr->ymax[MODEL]=ymax-ymin;	mptr->zmax[MODEL]=zmax;
    mptr->xmin[MODEL]=0;     	 mptr->ymin[MODEL]=0;		mptr->zmin[MODEL]=0;
    
    // adjust vtx positions to match model min/max/off data
    vtx_ptr=mptr->vertex_first[MODEL];
    while(vtx_ptr!=NULL)
      {
      vtx_ptr->x -= mptr->xorg[MODEL];
      vtx_ptr->y -= mptr->yorg[MODEL];
      vtx_ptr=vtx_ptr->next;
      }
    
    facet_find_all_neighbors(mptr->facet_first[MODEL]);			// create facet list for each vtx
    //model_integrity_check(mptr);					// check for geometry flaws
    edge_display(mptr,set_view_edge_angle,MODEL);			// determine which sharp edges to display in 3D view
    model_insert(mptr);							// add to job
    set_center_build_job=FALSE;						// do not center, it is where it is
    autoplacement_flag=FALSE;						// since fixed on table, do not allow move
    job_maxmin();

    display_model_flag=TRUE;
    Superimpose_flag=FALSE;
    printf("\n\nDone laser scan processing!\n");
    
    return(TRUE);
}

// Function to smooth the resulting image matrix before STL conversion
int laser_scan_matrix(void)
{
    int 	h,i,all_lo,all_hi;
    int		xpos,ypos,xval,yval;
    int		xy_nbr[3][3],delta[3][3];
    float 	avg_all,avg_nbr;
    
    for(xpos=0;xpos<415;xpos++)
      {
      for(ypos=0;ypos<415;ypos++)
        {
	// skip 0 values
	if(xy_on_table[xpos][ypos]==0)continue;
	    
	// init params
	avg_all=0;
	avg_nbr=0;
	
	// load the smoothing matrix
	for(h=(-1);h<2;h++)
	  {
	  xval=xpos+h;
	  if(xval<0)xval=0; if(xval>414)xval=414;
	  for(i=(-1);i<2;i++)
	    {
	    yval=ypos+i;
	    if(yval<0)yval=0; if(yval>414)yval=414;
	    xy_nbr[h+1][i+1]=xy_on_table[xval][yval];			// save neighbor's value
	    avg_all += xy_nbr[h+1][i+1];				// accumulate total for averaging all
	    delta[h+1][i+1]=xy_on_table[xpos][ypos]-xy_nbr[h+1][i+1];	// delta bt center pt and each neighbor
	    if(h!=0 && i!=0)avg_nbr += xy_nbr[h+1][i+1];		// accumulate total for averaging perimeter
	    }
	  }
	
	// calc the average of the matrix
	avg_all /= 9.0;							// avg of all including central pt
	avg_nbr /= 8.0;							// avg of perimeter values excluding central pt

	// look for up/dn spike at central location of neighbor matrix
	all_lo=TRUE;							// init to all delta values less than 0
	all_hi=TRUE;							// init to all delta values greater than 0
	for(h=0;h<2;h++)
	  {
	  for(i=0;i<2;i++)
	    {
	    if(h==1 && i==1)continue;					// skip the central pt
	    if(all_hi==TRUE && delta[h][i]<0)all_hi=FALSE;		// if all pos and one found negative...
	    if(all_lo==TRUE && delta[h][i]>0)all_lo=FALSE;		// if all neg and one found positive...
	    }
	  }
	if(all_hi==TRUE || all_lo==TRUE)
	  {
	  //printf("Spike x=%d y=%d from %d to %d \n",xpos,ypos,xy_on_table[xpos][ypos],(int)floor(avg_nbr+0.5));
	  xy_on_table[xpos][ypos]=(int)floor(avg_nbr+0.5);
	  }

	// look for out of norm value as compared to neighbors
	if(fabs(xy_on_table[xpos][ypos]-avg_nbr)>3)
	  {
	  //printf("Bad val x=%d y=%d from %d to %d \n",xpos,ypos,xy_on_table[xpos][ypos],(int)floor(avg_nbr+0.5));
	  xy_on_table[xpos][ypos]=(int)floor(avg_nbr+0.5);
	  }

	}
      }
    
    return(TRUE);
}

// Function to covert an edge detected black and white image to STL
// The basic gist is to convert the grayscale values to z height.  This
// allows the image to then be sliced like a normal STL yielding what
// looks like a topographical map of the image.
int image_to_STL(model *mptr)
{
    int		h,i,j,xpix,ypix,xend,yend;
    int		pix_sum;
    guchar 	*pix0,*pix1,*pix2,*pix3;
    int 	step_size, z_range;
    long int 	facet_count;
    float 	gray_scale_val;
    float	fct_inc;
    float 	x0,y0,z0;
    float 	x1,y1,z1;
    float 	x2,y2,z2;
    float 	x3,y3,z3;
    int 	xstart,ystart;
    int		in_object,on_wall;
    double	amt_total=0,amt_done=0.0;
    linetype 	*linetype_ptr;
    GdkRGBA 	color;

    int		color_depth=8;
    int 	color_threshold=100;

    int		xmin,ymin,zmin,xmax,ymax,zmax;
    int		vtx_cnt=0,fct_cnt=0;
    facet 	*fptr,*fnew,*fbtm,*fngh;
    vertex 	*vtxnew[4],*vtx_ptr,*vtx_new;
    vertex 	*vtxyA,*vtxyB,*vtxyC,*vtxyD;
    
    //debug_flag=621;

    // validate input
    if(mptr==NULL)return(FALSE);					// if no model...
    if(mptr->g_img_act_buff==NULL)return(FALSE);			// if model does not have image...
    
    // set params
    xmin=BUILD_TABLE_MAX_X; ymin=BUILD_TABLE_MAX_Y; zmin=BUILD_TABLE_MAX_Z;
    xmax=0; ymax=0; zmax=0;
    step_size=mptr->pix_increment;					// every Nth pixel will be converted
    z_range=cnc_z_height;						// range of z values
    
    // calc end conditions
    xend=(int)((float)(mptr->act_width)/(float)(step_size))*step_size-step_size;
    yend=(int)((float)(mptr->act_height)/(float)(step_size))*step_size-step_size;

    // display progress bar
    sprintf(scratch,"  Image to STL Conversion ");
    gtk_label_set_text(GTK_LABEL(lbl_info1),scratch);
    sprintf(scratch,"  Step 1 of 2: Processing image data...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_window_set_transient_for(GTK_WINDOW(win_info),GTK_WINDOW(win_main));
    gtk_widget_set_visible(win_info,TRUE);
    gtk_widget_queue_draw(win_info);
    while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly

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

    // loop thru each pixel and apply conversion
    for(ypix=0; ypix<=yend; ypix+=step_size)
      {
      amt_done = (double)(ypix)/(double)(yend);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      y0=ypix*mptr->pix_size;								// def float values
      y1=ypix*mptr->pix_size;
      y2=(ypix+step_size)*mptr->pix_size;
      y3=(ypix+step_size)*mptr->pix_size;
      
      for(xpix=0; xpix<=xend; xpix+=step_size)
        {
	x0=xpix*mptr->pix_size;							// def float values
	x1=(xpix+step_size)*mptr->pix_size;
	x2=xpix*mptr->pix_size;
	x3=(xpix+step_size)*mptr->pix_size;
	
	//printf(" Y=%d  X=%d \n",ypix,xpix);

	pix0 = mptr->act_pixels + ypix * mptr->act_rowstride + xpix * mptr->act_n_ch;	// load pixel from src
	pix1 = mptr->act_pixels + ypix * mptr->act_rowstride + (xpix+(int)(step_size)) * mptr->act_n_ch;	// load pixel from src
	pix2 = mptr->act_pixels + (ypix+(int)(step_size)) * mptr->act_rowstride + xpix * mptr->act_n_ch;	// load pixel from src
	pix3 = mptr->act_pixels + (ypix+(int)(step_size)) * mptr->act_rowstride + (xpix+(int)(step_size)) * mptr->act_n_ch;	// load pixel from src

	// STEP 1 - derive z hieght based on user specified params
	{
	gray_scale_val=255-(pix0[0]+pix0[1]+pix0[2])/3;
	if(mptr->image_invert==TRUE)gray_scale_val=255-gray_scale_val;
	if(color_depth==2)
	  {
	  if(gray_scale_val<color_threshold){gray_scale_val=0;}
	  else {gray_scale_val=255;}
	  }
	z0=gray_scale_val/255*z_range;
	
	gray_scale_val=255-(pix1[0]+pix1[1]+pix1[2])/3;
	if(mptr->image_invert==TRUE)gray_scale_val=255-gray_scale_val;
	if(color_depth==2)
	  {
	  if(gray_scale_val<color_threshold){gray_scale_val=0;}
	  else {gray_scale_val=255;}
	  }
	z1=gray_scale_val/255*z_range;

	gray_scale_val=255-(pix2[0]+pix2[1]+pix2[2])/3;
	if(mptr->image_invert==TRUE)gray_scale_val=255-gray_scale_val;
	if(color_depth==2)
	  {
	  if(gray_scale_val<color_threshold){gray_scale_val=0;}
	  else {gray_scale_val=255;}
	  }
	z2=gray_scale_val/255*z_range;

	gray_scale_val=255-(pix3[0]+pix3[1]+pix3[2])/3;
	if(mptr->image_invert==TRUE)gray_scale_val=255-gray_scale_val;
	if(color_depth==2)
	  {
	  if(gray_scale_val<color_threshold){gray_scale_val=0;}
	  else {gray_scale_val=255;}
	  }
	z3=gray_scale_val/255*z_range;

	//z0 = ((pix0[0]*0.3) + (pix0[1]*0.59) + (pix0[2]*0.11))/255*z_range;
	//z1 = ((pix1[0]*0.3) + (pix1[1]*0.59) + (pix1[2]*0.11))/255*z_range;
	//z2 = ((pix2[0]*0.3) + (pix2[1]*0.59) + (pix2[2]*0.11))/255*z_range;
	//z3 = ((pix3[0]*0.3) + (pix3[1]*0.59) + (pix3[2]*0.11))/255*z_range;

	if(z0<(z_range*0.1))z0=z_range*0.1;
	if(z1<(z_range*0.1))z1=z_range*0.1;
	if(z2<(z_range*0.1))z2=z_range*0.1;
	if(z3<(z_range*0.1))z3=z_range*0.1;
	}
	
	// STEP 2 - add facet pair with new z values
	{
	for(i=0;i<4;i++)vtxnew[i]=NULL;					// init to null
	
	// make the first vtx
	vtxnew[0]=vertex_make();					// ... make its vtx
	vtxnew[0]->x=x0; vtxnew[0]->y=y0; vtxnew[0]->z=z0;		// ... load its xyz value
	vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[0],MODEL,CLOSE_ENOUGH);
	if(vtx_ptr!=vtxnew[0]){free(vtxnew[0]); vertex_mem--; vtxnew[0]=vtx_ptr;}
	  
	// check for 2nd vtx
	vtxnew[1]=vertex_make();
	vtxnew[1]->x=x1; vtxnew[1]->y=y1; vtxnew[1]->z=z1;
	vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[1],MODEL,CLOSE_ENOUGH);
	if(vtx_ptr!=vtxnew[1]){free(vtxnew[1]); vertex_mem--; vtxnew[1]=vtx_ptr;}
	    
	// check for 3rd vtx
	vtxnew[2]=vertex_make();
	vtxnew[2]->x=x2; vtxnew[2]->y=y2; vtxnew[2]->z=z2;
	vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[2],MODEL,CLOSE_ENOUGH);
	if(vtx_ptr!=vtxnew[2]){free(vtxnew[2]); vertex_mem--; vtxnew[2]=vtx_ptr;}
	    
	// check for 4th vtx
	vtxnew[3]=vertex_make();
	vtxnew[3]->x=x3; vtxnew[3]->y=y3; vtxnew[3]->z=z3;
	vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxnew[3],MODEL,CLOSE_ENOUGH);
	if(vtx_ptr!=vtxnew[3]){free(vtxnew[3]); vertex_mem--; vtxnew[3]=vtx_ptr;}

	// make facet
	fct_cnt++;
	fnew=facet_make();
	fnew->vtx[0]=vtxnew[2]; fnew->vtx[1]=vtxnew[1]; fnew->vtx[2]=vtxnew[0];
	    
	// determine if min/max vals
	for(i=0;i<3;i++)
	  {
	  if(fnew->vtx[i]->x<xmin)xmin=fnew->vtx[i]->x;
	  if(fnew->vtx[i]->x>xmax)xmax=fnew->vtx[i]->x;
	  if(fnew->vtx[i]->y<ymin)ymin=fnew->vtx[i]->y;
	  if(fnew->vtx[i]->y>ymax)ymax=fnew->vtx[i]->y;
	  if(fnew->vtx[i]->z<zmin)zmin=fnew->vtx[i]->z;
	  if(fnew->vtx[i]->z>zmax)zmax=fnew->vtx[i]->z;
	  }

	// add the first facet at z height to MODEL structure
	facet_unit_normal(fnew);
	for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
	facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);
	//printf("  1st upper facet inserted at:  %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
	  
	// copy this new facet less the z value to form the lower surface
	
	fct_cnt++;
	fbtm=facet_make();
	for(i=0;i<3;i++)
	  {
	  vtx_new=vertex_copy(fnew->vtx[i],NULL);
	  vtx_new->z=0;
	  vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtx_new,MODEL,CLOSE_ENOUGH);
	  if(vtx_ptr!=vtx_new){free(vtx_new); vertex_mem--; vtx_new=vtx_ptr;}
	  fbtm->vtx[i]=vtx_new;
	  fbtm->vtx[i]->flist=facet_list_manager(fbtm->vtx[i]->flist,fbtm,ACTION_ADD);
	  }
	fbtm->unit_norm->x=0;
	fbtm->unit_norm->y=0;
	fbtm->unit_norm->z=(-1);
	facet_insert(mptr,mptr->facet_last[MODEL],fbtm,MODEL);
	//printf("  1st lower facet inserted at:   %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
	
		  
	// add second new upper facet if all 4 vtx are above z=0
	fct_cnt++;
	fnew=facet_make();
	fnew->vtx[0]=vtxnew[2];
	fnew->vtx[1]=vtxnew[3];
	fnew->vtx[2]=vtxnew[1];
	facet_unit_normal(fnew);
	for(i=0;i<3;i++){fnew->vtx[i]->flist=facet_list_manager(fnew->vtx[i]->flist,fnew,ACTION_ADD);}
	facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);
	//printf("  2nd upper facet inserted at:  %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);

	// copy this new facet less the z value to form the lower surface
	/*
	fct_cnt++;
	fbtm=facet_make();
	for(i=0;i<3;i++)
	  {
	  vtx_new=vertex_copy(fnew->vtx[i],NULL);
	  vtx_new->z=0;
	  vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtx_new,MODEL,CLOSE_ENOUGH);
	  if(vtx_ptr!=vtx_new){free(vtx_new); vertex_mem--; vtx_new=vtx_ptr;}
	  fbtm->vtx[i]=vtx_new;
	  fbtm->vtx[i]->flist=facet_list_manager(fbtm->vtx[i]->flist,fbtm,ACTION_ADD);
	  }
	fbtm->unit_norm->x=0;
	fbtm->unit_norm->y=0;
	fbtm->unit_norm->z=(-1);
	facet_insert(mptr,mptr->facet_last[MODEL],fbtm,MODEL);
	//printf("  2nd lower facet inserted at:   %3.0f/%3.0f/%3.0f \n",fnew->vtx[0]->x,fnew->vtx[0]->y,fnew->vtx[0]->z);
	*/
	}	// end of STEP 2 
	  
	}	// end of xpix loop
      }		// end of ypix loop


    // re-associate the entire block facet set 
    facet_find_all_neighbors(mptr->facet_first[MODEL]);
    
    // init status of all the new facets
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      fptr->status=(-1);
      fptr=fptr->next;
      }
 
    // update status display
    sprintf(scratch,"  Step 2 of 2: Associating facets...  ");
    gtk_label_set_text(GTK_LABEL(lbl_info2),scratch);
    gtk_widget_queue_draw(win_info);
    while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly

    // finally the upper and lower surfaces are stiched together with vertical facets.
    // check if upper facet has a null neighbor edge.
    // if so, build a pair of wall facets between it and the same edge of the lower facet. 
    facet_count=0;
    fptr=mptr->facet_first[MODEL];
    while(fptr!=NULL)
      {
      if(fptr->status>0){fptr=fptr->next; continue;}

      // update status display
      facet_count++;
      amt_done = (double)(facet_count)/(double)(mptr->facet_qty[MODEL]);
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR(info_progress), amt_done);
      while (g_main_context_iteration(NULL, FALSE));

      for(i=0;i<3;i++)							// loop thru each of this facet's neighbors
        {
        fngh=fptr->fct[i];						// get pointer to neighbor
        if(fngh==NULL)							// if no neighbor exists, we found an edge
          {
          // at this point we know that fptr is a free edge facet.  and we know the free edge
	  // exists between vtx[i] and vtx[h] on this upper facet.  now we need to find
	  // the matching free edge vtxs in the matching lower facet that have the same xy.
	  h=i+1; if(h>2)h=0;
	  vtxyA=fptr->vtx[i]; 						// define vtx addresses of the upper free edge
	  vtxyB=fptr->vtx[h];
	  vtxyC=NULL;							// vtx addresses of lower facet not known yet
	  vtxyD=NULL;
	  
	  // instead of searching for lower facets that were previously made, just
	  // create two matching vtxs at z=0 and connect to them
	  vtxyC=vertex_copy(vtxyA,NULL);
	  vtxyC->z=0.0;
	  vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxyC,MODEL,CLOSE_ENOUGH);
	  if(vtx_ptr!=vtxyC){free(vtxyC); vertex_mem--; vtxyC=vtx_ptr;}
	  vtxyD=vertex_copy(vtxyB,NULL);
	  vtxyD->z=0.0;
	  vtx_ptr=vertex_unique_insert(mptr,mptr->vertex_last[MODEL],vtxyD,MODEL,CLOSE_ENOUGH);
	  if(vtx_ptr!=vtxyD){free(vtxyD); vertex_mem--; vtxyD=vtx_ptr;}
      
	  // search facets with same xy but z at 0 to find lower facet with matching free edge
	  /*
	  fbtm=mptr->facet_first[MODEL];
	  while(fbtm!=NULL)
	    {
	    if(fbtm->status==1){fbtm=fbtm->next; continue;}
	    if((fbtm->vtx[0]->z+fbtm->vtx[1]->z+fbtm->vtx[2]->z)>CLOSE_ENOUGH){fbtm=fbtm->next; continue;}
	    if(fbtm==fptr){fbtm=fbtm->next; continue;}
	    for(j=0;j<3;j++)
	      {
	      if(vtxyC==NULL){if(vertex_xy_compare(vtxyA,fbtm->vtx[j],TOLERANCE)==TRUE)vtxyC=fbtm->vtx[j];}
	      if(vtxyD==NULL){if(vertex_xy_compare(vtxyB,fbtm->vtx[j],TOLERANCE)==TRUE)vtxyD=fbtm->vtx[j];}
	      }
	    if(vtxyC!=NULL && vtxyD!=NULL)break;			// if found, exit early to save time
	    fbtm=fbtm->next;
	    }
	  */
      
	  // at this point we should have 2 top and 2 btm vtxs define... enough for 2 new wall facets
          if(vtxyA!=NULL && vtxyB!=NULL && vtxyC!=NULL && vtxyD!=NULL)
	    {
	    fbtm->status=1;
	    
	    //printf("\nfptr=%X  fbtm=%X  fngh=%X \n",fptr,fbtm,fngh);
	    //printf(" A=%X  B=%X  C=%X  D=%X \n",vtxyA,vtxyB,vtxyC,vtxyD);
	    //printf(" Ax=%d  Bx=%d  Cx=%d  Dx=%d \n", vtxyA->x,vtxyB->x,vtxyC->x,vtxyD->x);
	    //printf(" Ay=%d  By=%d  Cy=%d  Dy=%d \n", vtxyA->y,vtxyB->y,vtxyC->y,vtxyD->y);
	    //printf(" Az=%d  Bz=%d  Cz=%d  Dz=%d \n", vtxyA->z,vtxyB->z,vtxyC->z,vtxyD->z);
		
	    // build two new vertical facets between fptr and fbtm and add to support list from
	    // the two free edge vtxs.
	    fnew=facet_make();							// allocate memory and set default values for facet
	    vtxyA->flist=facet_list_manager(vtxyA->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    fnew->vtx[0]=vtxyA;							// assign which vtxs belong to this facet
	    fnew->vtx[1]=vtxyB;
	    fnew->vtx[2]=vtxyC;
	    facet_unit_normal(fnew);
	    facet_area(fnew);
	    fnew->status=4;							// flag this as a lower facet
	    fnew->member=fptr->member;						// keep track of which body it belongs to
	    facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);		// adds new facet to facet list
	
	    // make new facet between vtxB, vtxC, and vtxD
	    fnew=facet_make();							// allocate memory and set default values for facet
	    vtxyB->flist=facet_list_manager(vtxyB->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyC->flist=facet_list_manager(vtxyC->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    vtxyD->flist=facet_list_manager(vtxyD->flist,fnew,ACTION_ADD);	// add this facet to the vtxs facet list
	    fnew->vtx[0]=vtxyB;							// assign which vtxs belong to this facet
	    fnew->vtx[1]=vtxyD;
	    fnew->vtx[2]=vtxyC;
	    facet_unit_normal(fnew);
	    facet_area(fnew);
	    fnew->status=4;							// flag this as a lower facet
	    fnew->member=fptr->member;						// keep track of which body it belongs to
	    facet_insert(mptr,mptr->facet_last[MODEL],fnew,MODEL);		// adds new facet to facet list
	    }
          }		// end of if neighbor NULL
	}		// end of for(i=0...
	
      fptr->status=1;
      fptr=fptr->next;
      }			// end of while(fptr!=NULL)

    // define model boundaries
    printf("\n xmin=%d ymin=%d zmin=%d  xmax=%d ymax=%d zmax=%d\n",xmin,ymin,zmin,xmax,ymax,zmax);
    mptr->xoff[MODEL]=xmin; 	 mptr->yoff[MODEL]=ymin;	mptr->zoff[MODEL]=0;
    mptr->xorg[MODEL]=xmin; 	 mptr->yorg[MODEL]=ymin;	mptr->zorg[MODEL]=0;
    mptr->xmax[MODEL]=xmax-xmin; mptr->ymax[MODEL]=ymax-ymin;	mptr->zmax[MODEL]=zmax;
    mptr->xmin[MODEL]=0;     	 mptr->ymin[MODEL]=0;		mptr->zmin[MODEL]=0;
    
    // adjust vtx positions to match model min/max/off data
    vtx_ptr=mptr->vertex_first[MODEL];
    while(vtx_ptr!=NULL)
      {
      vtx_ptr->x -= mptr->xorg[MODEL];
      vtx_ptr->y -= mptr->yorg[MODEL];
      vtx_ptr=vtx_ptr->next;
      }
    
    facet_find_all_neighbors(mptr->facet_first[MODEL]);			// create facet list for each vtx
    model_integrity_check(mptr);					// check for geometry flaws
    edge_display(mptr,set_view_edge_angle,MODEL);			// determine which sharp edges to display in 3D view
    job_maxmin();							// reset job boundaries

    gtk_widget_set_visible(win_info,FALSE);				// hide status bar
    while(g_main_context_iteration(NULL, FALSE));			// update display so it goes away promptly
    
    return(TRUE);
}

