#include "Global.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>


// Function to proceed with axis homing if rest of system checked out okay
static void on_init_okay (void)
{
  // show initialization window
  sprintf(scratch,"\nHoming system axes ... please wait.\n(This may take a minute or two)");
  aa_dialog_box(win_main,0,(-1),"Homing Axes",scratch);
    
  // load and execute config.json instructions
  load_system_config();

  // hide status window
  sprintf(scratch,"\nHoming complete!");
  aa_dialog_box(win_main,0,50,"Homing Axes",scratch);

  idle_start_time=time(NULL);						// reset idle time
	init_done_flag=TRUE;							// set flag that tool load is done
  return;
}

// Function to post status if init failed
static void on_init_failure (void)
{

  //sprintf(job_status_msg,"Status:  Not ready ");
  //gtk_window_close(GTK_WINDOW(winit));
  return;
}

// Function to control the initialization of system
int init_system(void)
{
    int		i=0;							// scratch
    int		RPi_check,TinyG_check,PWM_check,w1_check,matsdb_check,tooldb_check;
    GtkWidget	*win_init;
    GtkWidget	*grd_init;
    GtkWidget 	*lbl_init,*lbl_istat,*lbl_rpi,*lbl_i2c,*lbl_thrml,*lbl_1wire;
    GtkWidget	*lbl_tinyg,*lbl_pwm,*lbl_matdb,*lbl_tooldb,*lbl_separator;
    GtkWidget 	*button;
    
{
    // define formats for GTK messages
    strcpy(error_fmt,"<span foreground='red'>%s</span>");
    strcpy(okay_fmt,"<span foreground='green'>%s</span>");
    strcpy(norm_fmt,"<span foreground='black'>%s</span>");
    strcpy(unkw_fmt,"<span foreground='orange'>%s</span>");
    strcpy(stat_fmt,"<span foreground='blue'>%s</span>");
    
    // set up initialization window
    win_init = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(win_init), GTK_WINDOW(win_main));
    gtk_window_set_modal(GTK_WINDOW(win_init),TRUE);
    gtk_window_set_default_size(GTK_WINDOW(win_init),400,350);
    gtk_window_set_resizable(GTK_WINDOW(win_init),FALSE);
    gtk_window_set_title(GTK_WINDOW(win_init),"Nx3D - System Check");			
    
    // create a grid to hold label information
    grd_init = gtk_grid_new ();						// grid will hold cascading label information
    gtk_window_set_child(GTK_WINDOW(win_init),grd_init);
    gtk_grid_set_row_spacing (GTK_GRID(grd_init),15);
    gtk_grid_set_column_homogeneous(GTK_GRID(grd_init), TRUE);		// set columns to same
    gtk_widget_set_visible(win_init,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    
    // setup RPi GPIO
    // pass criterion:  select GPIO pins get set without error
    {
    RPi_check=init_device_RPi();
    if(RPi_check==0)
      {
      sprintf(scratch,"Error setting up GPIO communication!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"RPi GPIO communication working!");
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    lbl_rpi = gtk_label_new(scratch);
    gtk_label_set_markup(GTK_LABEL(lbl_rpi),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_rpi, 0, 1, 1, 1);		// add in controller status on row 1
    gtk_widget_set_visible(win_init,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    printf("RPi init done.\n\n");
    }

    // turn on chamber lighting
    gpioWrite(CHAMBER_LED,!ON);

    // check for tinyG and make appropriate label
    // pass criterion: the tinyG replies with its serial number
    {
    sprintf(scratch,"Initializing motion controller... please wait.");
    lbl_tinyg = gtk_label_new (scratch);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_tinyg, 0, 2, 1, 1);		// add in controller status on row 1
    gtk_widget_set_visible(win_init,TRUE);
    gtk_widget_queue_draw(lbl_tinyg);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    TinyG_check=init_device_tinyG();
    if(TinyG_check==0)
      {
      sprintf(scratch,"Error initializing motion controller!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"Motion controller ID = %s ",tinyG_id);
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_text(GTK_LABEL(lbl_tinyg),scratch);
    gtk_label_set_markup(GTK_LABEL(lbl_tinyg),markup);
    gtk_widget_set_visible(win_init,TRUE);
    while(g_main_context_pending(NULL)){g_main_context_iteration(NULL, FALSE);}
    printf("tinyG init done.\n\n");
    }

    // before checking any further the watch-dog must be started.
    // it is part of the thermal control loop and operates as an independent thread.
    // pass criterion:  the global variable "thermal_thread_alive" is set to TRUE
    {
    lbl_thrml = gtk_label_new (NULL);
    if(thermal_thread_alive<=0)
      {
      sprintf(scratch,"Error starting thermal control thread!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"Thermal control thread alive!");
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      delay(250);							// give PWM chance to "boot up"
      }
    gtk_label_set_markup(GTK_LABEL(lbl_thrml),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_thrml, 0, 3, 1, 1);
    gtk_widget_set_visible(win_init,TRUE);
    printf("thermal thread alive.\n\n");		
    }

    // check for pwm controller
    // dependent upon the thermal thread being functional (it kicks the watchdog)
    // pass criterion: PWM receives and replies (is talking)
    {
    PWM_check=init_device_PWM();
    lbl_pwm = gtk_label_new (NULL);
    if(PWM_check==0)
      {
      sprintf(scratch,"Error locating the PWM controller!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"PWM controller found!");
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_markup(GTK_LABEL(lbl_pwm),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_pwm, 0, 4, 1, 1);		
    gtk_widget_set_visible(win_init,TRUE);
    printf("PWM init done.\n\n");
    }

    // check for I2C devices
    // pass criterion: at least 2 I2C devices could be opened with handles
    {
    init_device_I2C();
    lbl_i2c = gtk_label_new (NULL);
    if(i2c_devices_found<2)
      {
      sprintf(scratch,"Error locating enough I2C devices!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"%d I2C devices found!",i2c_devices_found);
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_markup(GTK_LABEL(lbl_i2c),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_i2c, 0, 5, 1, 1);		
    gtk_widget_set_visible(win_init,TRUE);
    printf("I2C init done.\n\n");
    }

    // check spi bus
    {
      init_device_SPI();
      printf("SPI bus init done.\n");
    }

    // check for 1wire devices
    // pass criterion: 1wire master bus found
    {
    w1_check=init_device_1Wire();
    lbl_1wire = gtk_label_new (NULL);
    if(w1_check<1)
      {
      sprintf(scratch,"Error locating 1wire system bus!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"1wire system bus found!",memory_devices_found);
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_markup(GTK_LABEL(lbl_1wire),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_1wire, 0, 6, 1, 1);
    gtk_widget_set_visible(win_init,TRUE);
    printf("1wire init done.\n\n");
    }
    
    // check for presence of materials.xls database
    // pass criterion: file able to be opened (does NOT check integrity of contents)
    {
    matsdb_check=build_material_index(0);
    lbl_matdb = gtk_label_new (NULL);
    if(matsdb_check==0)
      {
      sprintf(scratch,"Error locating the MATERIALS database!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"Materials database found!");
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_markup(GTK_LABEL(lbl_matdb),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_matdb, 0, 7, 1, 1);
    gtk_widget_set_visible(win_init,TRUE);
    printf("Materials index init done.\n");
    }
    
    // check for tool database and build index of what is available
    // pass criterion:  file able to be opened AND indexed
    {
    tooldb_check=build_tool_index();
    lbl_tooldb = gtk_label_new (NULL);
    if(tooldb_check==0)
      {
      sprintf(scratch,"Error locating any TOOL databases!");
      markup = g_markup_printf_escaped (error_fmt,scratch);
      }
    else 
      {
      sprintf(scratch,"Tool databases found!");
      markup = g_markup_printf_escaped (norm_fmt,scratch);
      }
    gtk_label_set_markup(GTK_LABEL(lbl_tooldb),markup);
    gtk_grid_attach (GTK_GRID (grd_init), lbl_tooldb, 0, 8, 1, 1);
    gtk_widget_set_visible(win_init,TRUE);
    printf("Tool index init done.\n");
    }
    
    // init a few general system variables
    init_general();
    printf("General init done.\n\n");
    
    lbl_separator=gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);    
    gtk_grid_attach (GTK_GRID (grd_init), lbl_separator, 0, 9, 1, 1);
    
    
    // ADD MORE INITIALIZATION ELEMENTS HERE
    // Note that tools are initialized by the "on idle" call once the main window is up
    printf(" RPi_check...........=%s\n",RPi_check ? "PASS" : "FAIL");
    printf(" TinyG_check.........=%s\n",TinyG_check ? "PASS" : "FAIL");
    printf(" thermal_thread......=%s\n",thermal_thread_alive ? "PASS" : "FAIL");
    printf(" PWM_check...........=%s\n",PWM_check ? "PASS" : "FAIL");
    printf(" i2c_devices_found...=%d\n",i2c_devices_found);
    printf(" w1_check............=%s\n",w1_check ? "PASS" : "FAIL");
    printf(" materials db........=%s\n",matsdb_check ? "PASS" : "FAIL");
    printf(" tools db............=%s\n",tooldb_check ? "PASS" : "FAIL");
}    
    // Conditionally create a button to appropriate callback
    if(RPi_check>0 && 
       TinyG_check>0 && 
       thermal_thread_alive>0 && 
       PWM_check>0 && 
       i2c_devices_found>0 && 
       w1_check>0 && 
       matsdb_check>0 && 
       tooldb_check>0)
	{
	printf("Initialization success.\n\n");
	sprintf(scratch,"System Check - Good!");
	entry_system_log(scratch);
	lbl_istat = gtk_label_new (scratch);
	markup = g_markup_printf_escaped (okay_fmt,scratch);
	gtk_label_set_markup(GTK_LABEL(lbl_istat),markup);
	gtk_grid_attach (GTK_GRID (grd_init), lbl_istat, 0, 10, 1, 1);
	gtk_widget_set_visible(win_init,TRUE);
	i=0;
	while(i<25)							// display loop for win_init
	  {
	  g_main_context_iteration(NULL, FALSE);
	  delay(100);
	  i++;
	  }
	gtk_widget_set_visible(win_init,FALSE);
	gtk_window_close(GTK_WINDOW(win_init));
	gtk_widget_queue_draw(GTK_WIDGET(win_main));
	on_init_okay();
	return(1);
	}
    else
	{
	printf("Initialization failure.\n\n");
	sprintf(scratch,"Error Initializing - Shutting Down!");
	entry_system_log(scratch);
	lbl_istat = gtk_label_new (scratch);
	markup = g_markup_printf_escaped (error_fmt,scratch);
	gtk_label_set_markup(GTK_LABEL(lbl_istat),markup);
	gtk_grid_attach (GTK_GRID (grd_init), lbl_istat, 0, 10, 1, 1);
	gtk_widget_set_visible(win_init,TRUE);
	i=0;
	while(i<25)
	  {
	  g_main_context_iteration(NULL, FALSE);
	  delay(100);
	  i++;
	  }
	gtk_widget_set_visible(win_init,FALSE);
	gtk_window_close(GTK_WINDOW(win_init));
	return(0);
	}
	
    return(0);
}

int init_device_RPi(void)
{
    int result=0;

    i2c_devices_found=0;
    thermal_devices_found=0;
    memory_devices_found=0;

    // use pigpio to setup and control GPIO/I2C/SPI
    if(gpioInitialise()<0)
      {
      printf("\n\nGPIO Initialisation Failure! \n");
      return(result);
      }

    // init tinyG reset
    gpioSetMode(TINYG_RESET,PI_OUTPUT);
    gpioWrite(TINYG_RESET,ON);

    // init auxiliary multi-tool GPIO - currently used for input from tool
    gpioSetMode(AUX1_INPUT,PI_INPUT);

    // init auxiliary multi-tool GPIO - currently used for output to tool
    gpioSetMode(AUX2_OUTPUT,PI_OUTPUT);
    gpioWrite(AUX2_OUTPUT,ON);

    // tool tip switches mounted on the build table
    gpioSetMode(TOOL_TIP,PI_INPUT);	

    #ifdef GAMMA_UNIT
      // init chamber LED lighting
      gpioSetMode(CHAMBER_LED,PI_OUTPUT);
      gpioWrite(CHAMBER_LED,ON);

      // setup tool air
      gpioSetMode(TOOL_AIR,PI_OUTPUT);
      gpioWrite(TOOL_AIR,ON);						// backwards?
      tool_air_status_flag=OFF;

      // setup filament out detector
      gpioSetMode(FILAMENT_OUT,PI_INPUT);

      // init carriage LED lighting
      gpioSetMode(CARRIAGE_LED,PI_OUTPUT);
      gpioWrite(CARRIAGE_LED,ON);

      // setup 24v direct for 80w tool laser
      gpioSetMode(LASER_ENABLE,PI_OUTPUT);
      gpioWrite(LASER_ENABLE,1);
    #endif

    // init tool slot "limit" switches
    // all tools feed into this one switch, combine with selected tool
    gpioSetMode(TOOL_LIMIT,PI_INPUT);	
    
    // init watchdog GPIO, ALSO requires THERMAL THREAD to be active
    gpioSetMode(WATCH_DOG,PI_OUTPUT);
    gpioWrite(WATCH_DOG,OFF);

    // setup loop backs, tool selects, and carriage actuaters relative to location on carriage
    // ALPHA and BETA units use 4 tools concurrently, hence their high use of GPIOs.
    // GAMMA units use only one tool so the GPIOs are used for other things.
    #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	// slot 0
	gpioSetMode(TOOL_A_LOOP,PI_INPUT);
	gpioSetMode(TOOL_A_ACT,PI_OUTPUT);						
	gpioWrite(TOOL_A_ACT,1);  
	gpioSetMode(TOOL_A_SEL,PI_OUTPUT);
	gpioWrite(TOOL_A_SEL,1); 
	// slot 1
	gpioSetMode(TOOL_B_LOOP,PI_INPUT);
	gpioSetMode(TOOL_B_ACT,PI_OUTPUT);
	gpioWrite(TOOL_B_ACT,1);  
	gpioSetMode(TOOL_B_SEL,PI_OUTPUT);
	gpioWrite(TOOL_B_SEL,1); 
	// slot 2
	gpioSetMode(TOOL_C_LOOP,PI_INPUT);
	gpioSetMode(TOOL_C_ACT,PI_OUTPUT);
	gpioWrite(TOOL_C_ACT,1);  
	gpioSetMode(TOOL_C_SEL,PI_OUTPUT);
	gpioWrite(TOOL_C_SEL,1); 
	// slot 3
	gpioSetMode(TOOL_D_LOOP,PI_INPUT);
	gpioSetMode(TOOL_D_ACT,PI_OUTPUT);
	gpioWrite(TOOL_D_ACT,1);  
	gpioSetMode(TOOL_D_SEL,PI_OUTPUT);
	gpioWrite(TOOL_D_SEL,1);  
    #endif
    
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	// slot 0A - the primary slot in which this machine has
	gpioSetMode(TOOL_A_LOOP,PI_INPUT);				// used to check if tool is installed
	gpioSetMode(TOOL_A_ACT,PI_OUTPUT);
	gpioWrite(TOOL_A_ACT,1);  
	gpioSetMode(TOOL_A_SEL,PI_OUTPUT);				// used to turn tool on/off
	gpioWrite(TOOL_A_SEL,1); 
	// slot 0B - same physical slot as 0A, but allows drive of 2nd heater, 2nd RTD, and 2nd stepper (i.e. another FDM)
	gpioSetMode(TOOL_B_LOOP,PI_INPUT);
	gpioSetMode(TOOL_B_ACT,PI_OUTPUT);
	gpioWrite(TOOL_B_ACT,1);  
	gpioSetMode(TOOL_B_SEL,PI_OUTPUT);
	gpioWrite(TOOL_B_SEL,1); 
	// slot 2
	//gpioSetMode(TOOL_C_LOOP,PI_INPUT);
	//gpioSetMode(TOOL_C_ACT,PI_OUTPUT);
	//gpioWrite(TOOL_C_ACT,1);  
	//gpioSetMode(TOOL_C_SEL,PI_OUTPUT);
	//gpioWrite(TOOL_C_SEL,1); 
	// slot 3
	//gpioSetMode(TOOL_D_LOOP,PI_INPUT);
	//gpioSetMode(TOOL_D_ACT,PI_OUTPUT);
	//gpioWrite(TOOL_D_ACT,1);  
	//gpioSetMode(TOOL_D_SEL,PI_OUTPUT);
	//gpioWrite(TOOL_D_SEL,1);  
    #endif
      
    // set flag that gpio is operational
    gpio_ready_flag=TRUE;
    result=1;

    return(result);
}

int init_device_tinyG(void)
{
    int		retry_count=0;
    char	CtrlC;
    int		i,result=0;
    
    CtrlC=(char)03;

    // init tracking status of tinyG command buffer
    bufferAvail=MAX_BUFFER;						// tracks buffering status into tinyG controller, max=32
    bufferAdded=0;
    bufferRemoved=0;

    // init command buffer
    for(i=0;i<MAX_BUFFER;i++)
      {
      memset(cmd_buffer[i].cmd,0,sizeof(cmd_buffer[i]));
      cmd_buffer[i].tid=0;
      cmd_buffer[i].vid=0;
      cmd_buffer[i].vptr=NULL;
      }
    TId=1;								// set transaction ID to 1
    VId=1;								// set vertex ID to 1
    cmdCount=0;								// keeps track of the number of commands queued with tinyG
    cmdBurst=1;								// sets the limit for pending commands with the tinyG
    cmdControl=TRUE;							// flag to indicate if counting cmds or not
    oldState=cmdControl;
    memset(gcode_burst,0,sizeof(gcode_burst));				// null out burst string

    // send hard reset via GPIO to tinyG to start in known state
    gpioWrite(TINYG_RESET,ON);						// high keeps system on
    delay(50);
    gpioWrite(TINYG_RESET,OFF);						// pull low to reset
    delay(50);
    gpioWrite(TINYG_RESET,ON);						// high keeps system on
    sleep(6);								// tinyG takes ~4 sec to come back up
    
    // open output to tinyG and configurate comm protocol
    retry_count=0;
    while(retry_count<2)						// needs to be set twice
      {
      USB_fd=serOpen("/dev/ttyUSB0",115200,0);				// 115200 baud
      retry_count++;
      delay(1000);
      }
    if(USB_fd<0)
      {
      printf("tinyG USB Initialisation Failure!\n");
      printf("unable to acquire USB port after 5 attempts!\n");
      return(result);				
      }
    delay(250);

    // send a command to test if tinyG is responding by asking for its id and firmware version
    cmdControl=TRUE;
    memset(tinyG_id,0,sizeof(tinyG_id));

    tinyGSnd("{id:NULL} \n");						// get id of this tinyG
    result=tinyGRcv(1);
    
    if(result<1)
      {
      tinyGSnd("{fb:NULL} \n");						// get firmware build
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      }
    
    if(result<1)
      {
      tinyGSnd("{fv:NULL} \n");						// get firmware version
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      }
    
    if(result<1)
      {
      tinyGSnd("{hp:NULL} \n");						// get hardware build
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      }
    
    if(result<1)
      {
      tinyGSnd("{hv:NULL} \n");						// get hardware version
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      }
    
    if(result<1)
      {
      tinyGSnd("{\"baud\":5}\n");					// ensure baud (115,200) is properly set
      //while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      //if(result>0){printf("\nBaud shift glitch on baud:5 ... no problem.\n"); result=0;}

      // ensure we have collected all there is after baud shift... it typically causes ascii read errors
      delay(100);							// give time for shift to occur before any further com
      while(tinyGRcv(1)<0)delay(50);
      }
    
    
    //tinyGSnd("{\"ex\":1}\n");						// ensure software (XON/XOFF) float control is ON
    //while(cmdCount>0)tinyGRcv(1);					// wait for cmd to complete
    
    if(result<1)
      {
      tinyGSnd("{\"ex\":2}\n");						// ensure hardware (RTS/CTS) float control is ON
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      if(result>0){printf("\nBaud shift glitch on ex:2 ... no problem.\n"); result=0;}
      }
    
    if(result<1)
      {
      tinyGSnd("{\"ec\":1}\n");						// send LF & CR on TX
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      if(result>0){printf("\nBaud shift glitch on ec:1 ... no problem.\n"); result=0;}
      }
    
    if(result<1)
      {
      tinyGSnd("{\"ej\":1}\n");						// enable json mode
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      if(result>0){printf("\nBaud shift glitch on ej:1 ... no problem.\n"); result=0;}
      }
    
    if(result<1)
      {
      tinyGSnd("{\"ee\":0}\n");						// turn character echo OFF
      while((result=tinyGRcv(1))<0)delay(50);				// wait for cmd to complete
      if(result>0){printf("\nBaud shift glitch on ee:0 ... no problem.\n"); result=0;}
      }
      
    //while(tinyGRcv(1)<0)delay(50);					// one last chance to clear responses

    if(result<1)
      {
      if(strlen(tinyG_id)>0)result=1;
      tinyG_state=0;
      tinyG_in_motion=0;
      }
    else 
      {
      result=0;
      }
    
    return(result);
}

// Function to initialize I2C devices
int init_device_I2C(void)
{
    // open 1st A2D device on I2C port /dev/i2c-1 (the default on Raspberry Pi B)
    if((I2C_A2D_fd[0]=open("/dev/i2c-1",O_RDWR))<0) 
      {
      return(0);
      }
    else
      {
      // check for ADS1115 A2D with ADDR pin grounded as I2C slave at address 48H - typically RTDs
      if(ioctl(I2C_A2D_fd[0],I2C_SLAVE,0x48)<0) 
	{
	return(0);
	}
      else
	{
	printf("A2D #1 handle: %d \n",I2C_A2D_fd[0]);
	i2c_devices_found++;
	}
      }

    // open 2nd A2D device on I2C port /dev/i2c-1 (the default on Raspberry Pi B)
    if((I2C_A2D_fd[1]=open("/dev/i2c-1",O_RDWR))<0) 
	{
	return(0);
	}
    else
	{
	// check for ADS1115 A2D with ADDR pin @ Vcc as I2C slave at address 49H - typically other sensors
	if(ioctl(I2C_A2D_fd[1],I2C_SLAVE,0x49)<0) 
	    {
	    return(0);
	    }
	else
	    {
	    printf("A2D #2 handle: %d \n",I2C_A2D_fd[1]);
	    i2c_devices_found++;
	    }
	  }

    // open Distance Sensor device on I2C port /dev/i2c-1 (the default on Raspberry Pi B)
    //vl6180 handle = vl6180_initialise(1);
    //printf("Distance sensor handle: %d \n",handle);
    //if(handle>0)i2c_devices_found++;
    
    /*
    if((I2C_DS_fd=open("/dev/i2c-1",O_RDWR))<0) 
	{
	return(0);
	}
    else
	{
	// check for ADS1115 A2D with ADDR pin @ Vcc as I2C slave at address 49H - typically other sensors
	if(ioctl(I2C_DS_fd,I2C_SLAVE,0x29)<0) 
	    {
	    return(0);
	    }
	else
	    {
	    i2c_devices_found++;
	    }
	  }
    */
    
    return(1);
}

// Function to check if the PWM controller is alive
// Note that the watchdog MUST be active for the PWM controller to receive power and I2C must be working to communicate
int init_device_PWM(void)
{
    int		result=0;
    int		write_on,write_off,read_on,read_off;


    // open 1st PWM device on I2C port /dev/i2c-1 (the default on Raspberry Pi B)
    if((I2C_PWM_fd=open("/dev/i2c-1",O_RDWR))<0) 
      {
      return(0);
      }
    else
      {
      // check for PWM I2C slave at address 40H
      if(ioctl(I2C_PWM_fd,I2C_SLAVE,0x40)<0) 
	{
	return(0);
	}
      else
	{
	printf("PWM handle: %d \n",I2C_PWM_fd);
	i2c_devices_found++;
	}
      }

    // init PWM contorller
    pca9685PWMReset(I2C_PWM_fd);					// reset to default values (know state)
    pca9685Setup(0,I2C_PWM_fd,100);					// set pwwm freq to 100 hz
    //pca9685Setup(0,I2C_PWM_fd,10);					// set pwwm freq to 10 hz
    pca9685PWMWrite(I2C_PWM_fd,PIN_BASE,0,0);				// turn off everything
    
    // write data to a pin
    write_on=4000; write_off=100;
    pca9685PWMWrite(I2C_PWM_fd,0,write_on,write_off);			// send to PWM chip via I2C for pin 0
    printf("  test: write_on=%d  write_off=%d \n",write_on,write_off);

    // read back data from pin
    read_on=0; read_off=0;
    pca9685PWMRead(I2C_PWM_fd,0,&read_on,&read_off);			// read data from PWM via I2C
    printf("  test: read_on =%d read_off =%d \n",read_on,read_off);

    // check if operational
    if(read_on==write_on && read_off==write_off)result=1;
    pca9685PWMWrite(I2C_PWM_fd,PIN_BASE,0,0);				// turn off everything
    
    return(result);
}

// Function to open SPI bus
int init_device_SPI(void)
{
    
    #define AUX_SPI (1<<8)
    #define AUX_BITS(x) ((x)<<16)
    if(SPI_fd=spiOpen(0,1115000,AUX_SPI)<0)return(0);
    spiClose(SPI_fd);
   
/*    static uint8_t mode = SPI_CPHA;
    static uint8_t bits = 8;
    static uint32_t speed = 1000000;
    static uint16_t delay = 0;
    int	ret=0;
  
    SPI_fd=open("/dev/spidev0.0",O_RDWR);
    if(SPI_fd<0)
	{
	printf("SPI device init failed! \n");
	while(!kbhit());
	return(0);
	}

    ret += ioctl(SPI_fd, SPI_IOC_WR_MODE, &mode);
    ret += ioctl(SPI_fd, SPI_IOC_RD_MODE, &mode);
    ret += ioctl(SPI_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    ret += ioctl(SPI_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
    ret += ioctl(SPI_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    ret += ioctl(SPI_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
    if(ret<0)
	{
	printf("SPI param init failed! \n");
	while(!kbhit());
	return(0);
	}
*/
    return(1);
}

// Function to open all available 1wire devices via RPi subsystem (vs directly)
// When a 1wire device is found, this function will load the substructure md.dev with its ID via Open1Wire.
// md.dev will consequently be used to determine which memory device is associated with which tool.
int init_device_1Wire(void)
{
    int	slot,new_state;
    int	result=0;
    
    thermal_devices_found=0;
    memory_devices_found=0;

    // check for 1wire bus
    result=Open1Wire(0);

    return(result);
}

// Function to initialize tool variables
int init_general(void)
{
    int 	h=0,i=0,j=0,slot;
    int		read_error;
    float 	maxval;
    char	inval[16];
    FILE 	*tbl_data;

    // init log file pointer to NULL until they are opened
    system_log=NULL;							// contains high level system actions (launch, shut-down, job start, etc.)
    proc_log=NULL;							// contains job processing actions
    memset(rcv_cmd,0,sizeof(rcv_cmd));					// null init response from tinyG
 
    // init system level variables
    XMin=INT_MAX_VAL;YMin=INT_MAX_VAL;ZMin=INT_MAX_VAL;			// max/min boundaries of all models currently loaded
    XMax=INT_MIN_VAL;YMax=INT_MIN_VAL;ZMax=INT_MIN_VAL;
    for(i=0;i<VTX_INDEX;i++)						// init index for searching vtxs
      {
      for(h=0;h<VTX_INDEX;h++)
        {
        for(j=0;j<VTX_INDEX;j++)vtx_index[i][h][j]=NULL;
	}
      }
      
    // init any model slicing structures
    for(i=0;i<MAX_MDL_TYPES;i++)
      {
      slice_has_many_polygons[i]=0;
      slice_offset_type_qty[i]=15;					// threshold to shift over to different offsetting function by model type
      }
    slice_offset_type_qty[INTERNAL]=5;					// use lower value since offset errors are more tolerated
    slice_offset_type_qty[SUPPORT]=5;					// use lower value since offset errors are more tolerated
    //slc_spt=slice_make();						// create data space for the support slice
    
    #if defined(ALPHA_UNIT) || defined(BETA_UNIT)
	// init carriage slot parameters
	crgslot[0].x_offset=0.0;					// offset values will be overwritten by calibration values
	crgslot[0].y_offset=0.0;					// that are specific to this particular 4X3D machine
	crgslot[0].z_offset=0.0;
	crgslot[0].x_center= 38.600;					// was 40.400;	
	crgslot[0].y_center= 32.000;					// was 33.020;
	crgslot[0].camera_focal_dist=40.0;
	crgslot[0].camera_z_offset=53.0;
	crgslot[0].temp_offset=0.0;					// temp offset due to connections up to carriage
	crgslot[0].volt_offset=0.0;
	crgslot[0].calib_t_low=50.0;					// values based on meter readings
	crgslot[0].calib_v_low=0.893;
	crgslot[0].calib_t_high=200.0;
	crgslot[0].calib_v_high=1.259;
	
	crgslot[1].x_offset=0.0;
	crgslot[1].y_offset=0.0;
	crgslot[1].z_offset=0.0;
	crgslot[1].x_center=-38.600;					// was -40.400;
	crgslot[1].y_center= 32.000;					// was 33.020;
	crgslot[1].camera_focal_dist=40.0;
	crgslot[1].camera_z_offset=53.0;
	crgslot[1].temp_offset=0.0;
	crgslot[1].volt_offset=0.0;
	crgslot[1].calib_t_low=50.0;					// values based on meter readings
	crgslot[1].calib_v_low=0.893;
	crgslot[1].calib_t_high=200.0;
	crgslot[1].calib_v_high=1.259;
	
	crgslot[2].x_offset=0.0;
	crgslot[2].y_offset=0.0;
	crgslot[2].z_offset=0.0;
	crgslot[2].x_center= 38.600;					// was 40.400;
	crgslot[2].y_center=-32.000;					// was -33.020;
	crgslot[2].camera_focal_dist=40.0;
	crgslot[2].camera_z_offset=53.0;
	crgslot[2].temp_offset=0.0;
	crgslot[2].volt_offset=0.0;
	crgslot[2].calib_t_low=50.0;					// values based on meter readings
	crgslot[2].calib_v_low=0.893;
	crgslot[2].calib_t_high=200.0;
	crgslot[2].calib_v_high=1.259;
	
	crgslot[3].x_offset=0.0;
	crgslot[3].y_offset=0.0;
	crgslot[3].z_offset=0.0;
	crgslot[3].x_center=-38.600;					// was -40.400;
	crgslot[3].y_center=-32.000;					// was -33.020;
	crgslot[3].camera_focal_dist=40.0;
	crgslot[3].camera_z_offset=53.0;
	crgslot[3].temp_offset=0.0;
	crgslot[3].volt_offset=0.0;
	crgslot[3].calib_t_low=50.0;					// values based on meter readings
	crgslot[3].calib_v_low=0.893;
	crgslot[3].calib_t_high=200.0;
	crgslot[3].calib_v_high=1.259;
    #endif
    
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	// init carriage slot parameters - gamma units only have one slot
	crgslot[0].x_offset=0.0;					// offset values will be overwritten by calibration values
	crgslot[0].y_offset=0.0;					// that are specific to this particular 4X3D machine
	crgslot[0].z_offset=0.0;
	crgslot[0].x_center= 0.0;					// was 40.400;	
	crgslot[0].y_center= 0.0;					// was 33.020;
	crgslot[0].camera_focal_dist=40.0;
	crgslot[0].camera_z_offset=53.0;
	crgslot[0].temp_offset=0.0;					// temp offset due to connections up to carriage
	crgslot[0].volt_offset=0.0;
	crgslot[0].calib_t_low=50.0;					// values based on meter readings
	crgslot[0].calib_v_low=0.893;
	crgslot[0].calib_t_high=200.0;
	crgslot[0].calib_v_high=1.259;
    #endif
    
    // define positions of x/y tool tip calib switches
    xsw_pos=vertex_make();
    xsw_pos->x=(-28.000); xsw_pos->y=109.000; xsw_pos->z=(-1.000);
    ysw_pos=vertex_make();
    ysw_pos->x=(-20.000); ysw_pos->y=70.000; ysw_pos->z=(-1.000);
    
    // init tools - alpha & beta units have 4 tools slots, gamma has one tool slot
    // note that build table only receives one RTD, but drives two heaters
    for(slot=0;slot<MAX_TOOLS;slot++)
      {
      // default to empty slot params
      Tool[slot].state=TL_EMPTY;					// init to empty
      Tool[slot].sernum=(-1);						// no serial number yet
      Tool[slot].step_ID=0;						// no stepper motor
      Tool[slot].swch_pos=vertex_make();				// create ptr to switch vtx which defines its location
      Tool[slot].swch_pos->x=xsw_pos->x;				// note x value is from x switch
      Tool[slot].swch_pos->y=ysw_pos->y;				// note y value is from y switch
      Tool[slot].swch_pos->z=xsw_pos->z;				// note z value is from x switch
      sprintf(Tool[slot].name,"Empty");					// set name as empty
      Tool[slot].thrm.start_t_low=(-1);					// no low temp value calib data
      Tool[slot].thrm.start_v_low=(-1);					// no low volt value calib data
      Tool[slot].thrm.sensor=NONE;					// no temp measurement sensor
      Tool[slot].thrm.temp_port=slot;					// sensor input port to slot value
      Tool[slot].thrm.PWM_port=slot;					// PWM output port to slot value
      Tool[slot].thrm.sync=TRUE;					// force thermal thread to update to public values
      win_tool_flag[slot]=FALSE;					// tool window for this tool is not displayed
      linet_state[slot]=UNDEFINED;					// no linetype in use with this tool
      }
      
    // treat the build table and chamber as "psuedo" tools... beyond MAX_TOOLS but less than MAX_THERMAL_DEVICES
    // alpha units have up to 4 tools, one build table heater and one RTD measuring its temp, and one chamber RTD
    #ifdef ALPHA_UNIT
	load_tool_defaults(BLD_TBL1);
	Tool[BLD_TBL1].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[BLD_TBL1].active_time_stamp);
	Tool[BLD_TBL1].thrm.sync=FALSE;
	Tool[BLD_TBL1].thrm.sensor=RTDH;
	Tool[BLD_TBL1].thrm.maxtC=150.0;
	Tool[BLD_TBL1].pwr24=TRUE;
	Tool[BLD_TBL1].thrm.temp_port=MAX_TOOLS;
	Tool[BLD_TBL1].thrm.PWM_port=MAX_TOOLS;
	Tool[BLD_TBL1].thrm.duration_per_C=90.0;
	Tool[BLD_TBL1].thrm.tolrC=5.0;
	Tool[BLD_TBL1].thrm.Kp=3.00;
	Tool[BLD_TBL1].thrm.Ki=0.30;
	Tool[BLD_TBL1].thrm.Kd=9.00;
	Tool[BLD_TBL1].thrm.calib_t_low=0.0;
	Tool[BLD_TBL1].thrm.calib_v_low=0.788;
	Tool[BLD_TBL1].thrm.calib_t_high=250.0;
	Tool[BLD_TBL1].thrm.calib_v_high=1.338;

	load_tool_defaults(CHAMBER);
	Tool[CHAMBER].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[CHAMBER].active_time_stamp);
	Tool[CHAMBER].thrm.sync=FALSE;
	Tool[CHAMBER].thrm.sensor=RTDH;
	Tool[CHAMBER].thrm.maxtC=50.0;
	Tool[CHAMBER].pwr24=FALSE;
	Tool[CHAMBER].thrm.temp_port=MAX_TOOLS+1;
	Tool[CHAMBER].thrm.PWM_port=(-1);
	Tool[CHAMBER].thrm.duration_per_C=120.0;
	Tool[CHAMBER].thrm.tolrC=5.0;					// very broad since no control
	Tool[CHAMBER].thrm.Kp=3.00;
	Tool[CHAMBER].thrm.Ki=0.30;
	Tool[CHAMBER].thrm.Kd=9.00;
	Tool[CHAMBER].thrm.calib_t_low=0.0;
	Tool[CHAMBER].thrm.calib_v_low=0.788;
	Tool[CHAMBER].thrm.calib_t_high=250.0;
	Tool[CHAMBER].thrm.calib_v_high=1.338;
    #endif

    // beta units have up to 4 tools, 2 heaters AND 2 RTDs on their larger build table, and one chamber RTD
    #ifdef BETA_UNIT
	load_tool_defaults(BLD_TBL1);
	Tool[BLD_TBL1].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[BLD_TBL1].active_time_stamp);
	Tool[BLD_TBL1].thrm.sync=FALSE;
	Tool[BLD_TBL1].thrm.sensor=RTDH;
	Tool[BLD_TBL1].thrm.maxtC=150.0;
	Tool[BLD_TBL1].pwr24=TRUE;
	Tool[BLD_TBL1].thrm.temp_port=MAX_TOOLS;
	Tool[BLD_TBL1].thrm.PWM_port=MAX_TOOLS;
	Tool[BLD_TBL1].thrm.duration_per_C=90.0;
	Tool[BLD_TBL1].thrm.tolrC=5.0;
	Tool[BLD_TBL1].thrm.Kp=3.00;
	Tool[BLD_TBL1].thrm.Ki=0.30;
	Tool[BLD_TBL1].thrm.Kd=9.00;
	Tool[BLD_TBL1].thrm.calib_t_low=0.0;
	Tool[BLD_TBL1].thrm.calib_v_low=0.788;
	Tool[BLD_TBL1].thrm.calib_t_high=250.0;
	Tool[BLD_TBL1].thrm.calib_v_high=1.338;

	load_tool_defaults(BLD_TBL2);
	Tool[BLD_TBL2].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[BLD_TBL2].active_time_stamp);
	Tool[BLD_TBL2].thrm.sync=FALSE;
	Tool[BLD_TBL2].thrm.sensor=RTDH;
	Tool[BLD_TBL2].thrm.maxtC=120.0;
	Tool[BLD_TBL2].pwr24=TRUE;
	Tool[BLD_TBL2].thrm.temp_port=MAX_TOOLS+1;
	Tool[BLD_TBL2].thrm.PWM_port=MAX_TOOLS+1;
	Tool[BLD_TBL2].thrm.duration_per_C=90.0;
	Tool[BLD_TBL2].thrm.tolrC=5.0;
	Tool[BLD_TBL2].thrm.Kp=3.00;
	Tool[BLD_TBL2].thrm.Ki=0.30;
	Tool[BLD_TBL2].thrm.Kd=9.00;
	Tool[BLD_TBL2].thrm.calib_t_low=0.0;
	Tool[BLD_TBL2].thrm.calib_v_low=0.788;
	Tool[BLD_TBL2].thrm.calib_t_high=250.0;
	Tool[BLD_TBL2].thrm.calib_v_high=1.338;

	load_tool_defaults(CHAMBER);
	Tool[CHAMBER].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[CHAMBER].active_time_stamp);
	Tool[CHAMBER].thrm.sync=FALSE;
	Tool[CHAMBER].thrm.sensor=RTDH;
	Tool[CHAMBER].thrm.maxtC=50.0;
	Tool[CHAMBER].pwr24=FALSE;
	Tool[CHAMBER].thrm.temp_port=MAX_TOOLS+2;
	Tool[CHAMBER].thrm.PWM_port=(-1);
	Tool[CHAMBER].thrm.duration_per_C=120.0;
	Tool[CHAMBER].thrm.tolrC=5.0;					// very broad since no control
	Tool[CHAMBER].thrm.Kp=3.00;
	Tool[CHAMBER].thrm.Ki=0.30;
	Tool[CHAMBER].thrm.Kd=9.00;
	Tool[CHAMBER].thrm.calib_t_low=0.0;
	Tool[CHAMBER].thrm.calib_v_low=0.788;
	Tool[CHAMBER].thrm.calib_t_high=250.0;
	Tool[CHAMBER].thrm.calib_v_high=1.338;
    #endif

    // gamma units have only 1 tool, 2 heaters and only 1 RTD on their larger build table, and 1 chamber RTD
    #if defined(GAMMA_UNIT) || defined(DELTA_UNIT)
	load_tool_defaults(BLD_TBL1);
	Tool[BLD_TBL1].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[BLD_TBL1].active_time_stamp);
	Tool[BLD_TBL1].thrm.sync=FALSE;
	Tool[BLD_TBL1].thrm.sensor=RTDH;
	Tool[BLD_TBL1].thrm.maxtC=150.0;
	Tool[BLD_TBL1].pwr48=TRUE;
	Tool[BLD_TBL1].thrm.temp_port=1;
	Tool[BLD_TBL1].thrm.PWM_port=1;
	Tool[BLD_TBL1].thrm.duration_per_C=6000.0;
	Tool[BLD_TBL1].thrm.errtC=6.0;
	Tool[BLD_TBL1].thrm.tolrC=5.0;
	Tool[BLD_TBL1].thrm.Kp=5.00;
	Tool[BLD_TBL1].thrm.Ki=0.50;
	Tool[BLD_TBL1].thrm.Kd=6.00;
	Tool[BLD_TBL1].thrm.calib_t_low=0.0;
	Tool[BLD_TBL1].thrm.calib_v_low=0.788;
	Tool[BLD_TBL1].thrm.calib_t_high=250.0;
	Tool[BLD_TBL1].thrm.calib_v_high=1.338;

	load_tool_defaults(BLD_TBL2);
	Tool[BLD_TBL2].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[BLD_TBL2].active_time_stamp);
	Tool[BLD_TBL2].thrm.sync=FALSE;
	Tool[BLD_TBL2].thrm.sensor=RTDH;
	Tool[BLD_TBL2].thrm.maxtC=150.0;
	Tool[BLD_TBL2].pwr48=TRUE;
	Tool[BLD_TBL2].thrm.temp_port=1;
	Tool[BLD_TBL2].thrm.PWM_port=2;
	Tool[BLD_TBL2].thrm.duration_per_C=6000.0;
	Tool[BLD_TBL2].thrm.errtC=6.0;
	Tool[BLD_TBL2].thrm.tolrC=5.0;
	Tool[BLD_TBL2].thrm.Kp=5.00;
	Tool[BLD_TBL2].thrm.Ki=0.50;
	Tool[BLD_TBL2].thrm.Kd=6.00;
	Tool[BLD_TBL2].thrm.calib_t_low=0.0;
	Tool[BLD_TBL2].thrm.calib_v_low=0.788;
	Tool[BLD_TBL2].thrm.calib_t_high=250.0;
	Tool[BLD_TBL2].thrm.calib_v_high=1.338;

	load_tool_defaults(CHAMBER);
	Tool[CHAMBER].state=TL_READY;
	clock_gettime(CLOCK_REALTIME, &Tool[CHAMBER].active_time_stamp);
	Tool[CHAMBER].thrm.sync=FALSE;
	Tool[CHAMBER].thrm.sensor=RTDH;
	Tool[CHAMBER].thrm.maxtC=100.0;
	Tool[CHAMBER].pwr24=TRUE;					// drive filter fan speed
	Tool[CHAMBER].thrm.temp_port=2;
	Tool[CHAMBER].thrm.PWM_port=4;
	Tool[CHAMBER].thrm.duration_per_C=120.0;
	Tool[CHAMBER].thrm.tolrC=15.0;					// very broad since no control
	Tool[CHAMBER].thrm.Kp=3.00;
	Tool[CHAMBER].thrm.Ki=0.30;
	Tool[CHAMBER].thrm.Kd=9.00;
	Tool[CHAMBER].thrm.calib_t_low=0.0;
	Tool[CHAMBER].thrm.calib_v_low=0.788;
	Tool[CHAMBER].thrm.calib_t_high=250.0;
	Tool[CHAMBER].thrm.calib_v_high=1.338;
    #endif
    
    
    // read unit calibration file after defaults have been loaded
    printf("\nReading unit calibration file...\n");
    read_error=FALSE;
    sprintf(calib_file_rev,"20230725");					// set current rev of calib file
    unit_calib=fopen("4X3D.calib","r");
    if(unit_calib!=NULL)
      {
      if(fread(&scratch,sizeof(calib_file_rev),1,unit_calib)!=1)read_error=TRUE;	// get rev of saved file
      //if(strstr(scratch,calib_file_rev)!=NULL)					// if rev matches this code base... read rest of file
        {
	for(slot=0;slot<MAX_TOOLS;slot++)
	  {
	  if(fread(&crgslot[slot].x_center,sizeof(crgslot[slot].x_center),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].y_center,sizeof(crgslot[slot].y_center),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].x_offset,sizeof(crgslot[slot].x_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].y_offset,sizeof(crgslot[slot].y_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].z_offset,sizeof(crgslot[slot].z_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].camera_focal_dist,sizeof(crgslot[slot].camera_focal_dist),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].camera_z_offset,sizeof(crgslot[slot].camera_z_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].temp_offset,sizeof(crgslot[slot].temp_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].volt_offset,sizeof(crgslot[slot].volt_offset),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].calib_t_low,sizeof(crgslot[slot].calib_t_low),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].calib_v_low,sizeof(crgslot[slot].calib_v_low),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].calib_t_high,sizeof(crgslot[slot].calib_t_high),1,unit_calib)!=1)read_error=TRUE;
	  if(fread(&crgslot[slot].calib_v_high,sizeof(crgslot[slot].calib_v_high),1,unit_calib)!=1)read_error=TRUE;
	  }

	if(fread(&Tool[BLD_TBL1].thrm.calib_t_low,sizeof(Tool[BLD_TBL1].thrm.calib_t_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL1].thrm.calib_v_low,sizeof(Tool[BLD_TBL1].thrm.calib_v_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL1].thrm.calib_t_high,sizeof(Tool[BLD_TBL1].thrm.calib_t_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL1].thrm.calib_v_high,sizeof(Tool[BLD_TBL1].thrm.calib_v_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL1].thrm.temp_offset,sizeof(Tool[BLD_TBL1].thrm.temp_offset),1,unit_calib)!=1)read_error=TRUE;
	
	if(fread(&Tool[BLD_TBL2].thrm.calib_t_low,sizeof(Tool[BLD_TBL2].thrm.calib_t_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL2].thrm.calib_v_low,sizeof(Tool[BLD_TBL2].thrm.calib_v_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL2].thrm.calib_t_high,sizeof(Tool[BLD_TBL2].thrm.calib_t_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL2].thrm.calib_v_high,sizeof(Tool[BLD_TBL2].thrm.calib_v_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[BLD_TBL2].thrm.temp_offset,sizeof(Tool[BLD_TBL2].thrm.temp_offset),1,unit_calib)!=1)read_error=TRUE;
	
	if(fread(&Tool[CHAMBER].thrm.calib_t_low,sizeof(Tool[CHAMBER].thrm.calib_t_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[CHAMBER].thrm.calib_v_low,sizeof(Tool[CHAMBER].thrm.calib_v_low),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[CHAMBER].thrm.calib_t_high,sizeof(Tool[CHAMBER].thrm.calib_t_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[CHAMBER].thrm.calib_v_high,sizeof(Tool[CHAMBER].thrm.calib_v_high),1,unit_calib)!=1)read_error=TRUE;
	if(fread(&Tool[CHAMBER].thrm.temp_offset,sizeof(Tool[CHAMBER].thrm.temp_offset),1,unit_calib)!=1)read_error=TRUE;
	  
	unit_calib_required=FALSE;
	printf("   SLOT calib_t_lo=%f  calib_v_lo=%f  toff=%f\n",crgslot[0].calib_t_low,crgslot[0].calib_v_low,crgslot[0].temp_offset);
	printf("   BLDT calib_t_lo=%f  calib_v_lo=%f  toff=%f\n",Tool[BLD_TBL1].thrm.calib_t_low,Tool[BLD_TBL1].thrm.calib_v_low,Tool[BLD_TBL1].thrm.temp_offset);
	printf("   CHMB calib_t_lo=%f  calib_v_lo=%f  toff=%f\n",Tool[CHAMBER].thrm.calib_t_low,Tool[CHAMBER].thrm.calib_v_low,Tool[CHAMBER].thrm.temp_offset);
	printf("... unit calibration data successfully read from file.\n");
	}
      fclose(unit_calib);
      if(read_error==TRUE)
        {
	printf("/nError - could not properly read the tool file while initializing!\n");
	}
      }
    else 
      {
      printf("... error reading unit calibration file.\n\n");
      }

    
    // define deault colors for models loaded by tools in each slot
    // colors are defined by slot number, not tool type
    mcolor[0].red=0.204;mcolor[0].blue=0.643;mcolor[0].green=0.396;mcolor[0].alpha=0.8;	// med blue
    mcolor[1].red=0.306;mcolor[1].blue=0.024;mcolor[1].green=0.604;mcolor[1].alpha=0.8;	// drk green
    mcolor[2].red=0.757;mcolor[2].blue=0.067;mcolor[2].green=0.490;mcolor[2].alpha=0.8;	// med brown
    mcolor[3].red=0.459;mcolor[3].blue=0.482;mcolor[3].green=0.314;mcolor[3].alpha=0.8;	// med violet
    printf("Tools init done.\n");
    
    // init job
    job.prev_state=UNDEFINED;
    job.state=UNDEFINED;
    job.regen_flag=FALSE;
    job.baselayer_flag=FALSE;    
    job.baselayer_type=1;
    job.type=UNDEFINED;
    job.min_slice_thk=0.20;
    job.current_z=0.20;
    job.model_count=0;
    job.max_layer_count=0;
    job.model_first=NULL;
    job.current_dist=0.0;
    job.penup_dist=0.0;
    job.pendown_dist=0.0;
    job.total_dist=0.0;
    job.support_tree=NULL;
    job.lfacet_upfacing[MODEL]=NULL;
    job.lfacet_upfacing[INTERNAL]=NULL;
    job.lfacet_upfacing[SUPPORT]=NULL;
    
    // init layer display
    slc_view_inc=0.25;
    slc_view_start=0.0;
    slc_view_end=0.0;
    slc_just_one=TRUE;
    slc_show_all=FALSE;

    // init possible operation descriptions - actual operation flags are held with each model
    sprintf(op_description[OP_NONE],"None");
    sprintf(op_description[OP_ADD_MODEL_MATERIAL],"Add model material");	// add model material
    sprintf(op_description[OP_ADD_SUPPORT_MATERIAL],"Add support material");	// add support structure material
    sprintf(op_description[OP_ADD_BASE_LAYER],"Add base layer");		// add base layer material
    sprintf(op_description[OP_MILL_OUTLINE],"Mill an outline on a surface");	// mill outlines
    sprintf(op_description[OP_MILL_AREA],"Mill an area of a surface");		// mill areas
    sprintf(op_description[OP_MILL_PROFILE],"Mill using profile cutting.");	// mill with profile cuts
    sprintf(op_description[OP_MILL_HOLES],"Mill (drill) holes.");		// mill holes
    sprintf(op_description[OP_MEASURE_X],"Measure x dimensions");		// measure x dimensions
    sprintf(op_description[OP_MEASURE_Y],"Measure y dimensions");		// measure y dimensions
    sprintf(op_description[OP_MEASURE_Z],"Measure z dimensions");		// measure z dimensions
    sprintf(op_description[OP_MEASURE_HOLES],"Measure hole diameters");		// measure hole diameters
    sprintf(op_description[OP_MARK_OUTLINE],"Vector draw an outline");	  	// mark outline
    sprintf(op_description[OP_MARK_AREA],"Vector fill an outline & area");	// mark area
    sprintf(op_description[OP_MARK_IMAGE],"Raster draw in grayscale");		// mark image
    sprintf(op_description[OP_MARK_CUT],"Vector cut an outline");		// mark cut
    sprintf(op_description[OP_CURE],"Cure material");				// curing of added material
    sprintf(op_description[OP_PLACE],"Placement");				// placement of material
	    
    // init possible line type descriptions
    sprintf(lt_description[MDL_PERIM],"Modle Perimeter");
    sprintf(lt_description[MDL_BORDER],"Model Border");
    sprintf(lt_description[MDL_OFFSET],"Model Offset");
    sprintf(lt_description[MDL_FILL],"Model Fill");
    sprintf(lt_description[MDL_LOWER_CO],"Model Lower close-off");
    sprintf(lt_description[MDL_UPPER_CO],"Model Upper close-off");
    sprintf(lt_description[MDL_LAYER_1],"Model First layer");
    sprintf(lt_description[BASELYR],"Base layer");
    sprintf(lt_description[PLATFORMLYR1],"Platform layer 1");
    sprintf(lt_description[PLATFORMLYR2],"Platform layer 2");
    sprintf(lt_description[SPT_PERIM],"Support Perim");
    sprintf(lt_description[SPT_BORDER],"Support Border");
    sprintf(lt_description[SPT_OFFSET],"Support Offset");
    sprintf(lt_description[SPT_FILL],"Support Fill");
    sprintf(lt_description[SPT_LOWER_CO],"Support Lower close-off");
    sprintf(lt_description[SPT_UPPER_CO],"Support Upper close-off");
    sprintf(lt_description[SPT_LAYER_1],"Support First layer");
    sprintf(lt_description[TRACE],"Trace");
    sprintf(lt_description[DRILL],"Drill");
    
    // init system
    current_tool=(-1);							// no tool currently active
    current_step=0;							// no stepper motor currently active
    vtx_pick_new=vertex_make();						// create a mem location for vtx picking
    vtx_pick_new->x=0;vtx_pick_new->y=0;vtx_pick_new->z=0;		// set init location to origin
    vtx_pick_ref=vertex_make();						// create a mem location for vtx picking
    vtx_pick_ref->x=0;vtx_pick_ref->y=0;vtx_pick_ref->z=0;		// set init location to origin
    vtx_zero_ref=vertex_make();						// make a vertex for global storage/reference
    vtx_zero_ref->x=0;vtx_zero_ref->y=0;vtx_zero_ref->z=0;		// set init location to origin
    fct_pick_ref=NULL;							// no facet in pick list
    active_model=NULL;							// no model is active
    set_start_at_crt_z=FALSE;						// start at model top/btm
    
    // init debugging params
    vtx_debug=vertex_make();
    vmid=vertex_make();
    vlow=vertex_make();
    vhgh=vertex_make();
    vec_debug=vector_make(vmid,vlow,0);
    vec_debug->next=vector_make(vmid,vhgh,0);
    (vec_debug->next)->next=NULL;
    vec_debug->prev=NULL;
    (vec_debug->next)->prev=vec_debug;
    vtx_pick_list=NULL;
    e_pick_list=NULL;
    vec_pick_list=NULL;
    p_pick_list=NULL;
    f_pick_list=NULL;
    set_mouse_pick_entity=1;
    MDisplay_edges=NULL;
    MDisplay_facets=NULL;
    

    // init history tracking
    for(i=0;i<MAX_HIST_TYPES;i++)hist_ctr[i]=0;				// init counter values
    for(i=0;i<MAX_HIST_FIELDS;i++)
      {
      for(h=0;h<MAX_HIST_COUNT;h++){history[i][h]=0.0;}
      memset(hist_name[i],0,sizeof(hist_name[i]));
      hist_use_flag[i]=FALSE;						// default them to off
      //if(i<MAX_TOOLS)hist_use_flag[i]=FALSE;				// idle loop will turn them on if installed
      hist_tool[i]=(-1);
      }
      
    hist_use_flag[H_TOOL_TEMP]=TRUE;					// turn thrermal tracking on
    hist_use_flag[H_TABL_TEMP]=TRUE;
    hist_use_flag[H_CHAM_TEMP]=TRUE;
    
    hist_color[H_TOOL_TEMP]=DK_BLUE;
    hist_color[H_TABL_TEMP]=DK_VIOLET;
    hist_color[H_CHAM_TEMP]=DK_GREEN;
    
    hist_color[H_MOD_MAT_EST]=LT_BLUE;
    hist_color[H_MOD_MAT_ACT]=DK_BLUE;
    hist_color[H_SUP_MAT_EST]=LT_RED;
    hist_color[H_SUP_MAT_ACT]=DK_RED;
    
    hist_color[H_MOD_TIM_EST]=LT_BLUE;
    hist_color[H_MOD_TIM_ACT]=DK_BLUE;
    hist_color[H_SUP_TIM_EST]=LT_RED;
    hist_color[H_SUP_TIM_ACT]=DK_RED;
    hist_color[H_OHD_TIM_EST]=LT_GRAY;
    hist_color[H_OHD_TIM_ACT]=DK_GRAY;

    hist_type=HIST_TEMP;						// default to thermal tracking
    sprintf(hist_name[H_TOOL_TEMP],"TOOL");
    sprintf(hist_name[H_TABL_TEMP],"BUILD TABLE");
    sprintf(hist_name[H_CHAM_TEMP],"CHAMBER");
    
    sprintf(hist_name[H_MOD_MAT_EST],"MODEL ESTIMATE");
    sprintf(hist_name[H_SUP_MAT_EST],"SUPPORT ESTIMATE");

    sprintf(hist_name[H_MOD_MAT_ACT],"MODEL ACTUAL");
    sprintf(hist_name[H_SUP_MAT_ACT],"SUPPORT ACTUAL");
    
    sprintf(hist_name[H_MOD_TIM_EST],"MODEL ESTIMATE");
    sprintf(hist_name[H_SUP_TIM_EST],"SUPPORT ESTIMATE");
    sprintf(hist_name[H_OHD_TIM_EST],"OVERHEAD ESTIMATE");

    sprintf(hist_name[H_MOD_TIM_ACT],"MODEL ACTUAL");
    sprintf(hist_name[H_SUP_TIM_ACT],"SUPPORT ACTUAL");
    sprintf(hist_name[H_OHD_TIM_ACT],"OVERHEAD ACTUAL");


    // init camera to live view
    cam_roi_x0=0.0; cam_roi_y0=0.0;
    cam_roi_x1=1.0; cam_roi_y1=1.0;
    g_camera_buff=NULL;
    cam_image_mode=CAM_LIVEVIEW;
    //pixfilter[0]=127;
    //pixfilter[1]=123;
    //pixfilter[2]=137;
    //pixfilter[3]=142;

    // init build table level
    zoffset=0.0;
    for(i=0;i<SCAN_X_MAX;i++)
      {
      for(h=0;h<SCAN_Y_MAX;h++)
        {
	Pz_raw[i][h]=(-1);
	Pz_att[i][h]=(-1);
	}
      }
      
    // search for valid BldTblZData file that contains the previous build table level scan data. 
    // note the values in this data are the deltas of the table as measured from 0.000
    tbl_data=fopen("/home/aa/Documents/4X3D/BldTblZData.txt","r");
    if(tbl_data!=NULL)
      {
      // read in the value of scan_dist used to create this data set
      read_error=FALSE;
      memset(inval,0,sizeof(inval));							// set whole string to NULL
      if(fread(&inval,7,1,tbl_data)!=1)read_error=TRUE;	    				// read in value
      scan_dist=atof(inval);
      if(fread(&inval,1,1,tbl_data)!=1)read_error=TRUE;					// account for line feed character
      // init scanning variables used throughout the code based on the scan_dist.
      // note these must match the values used when writing BldTblZData.txt in the build_table_scan functions.
      scan_xmax=(int)(BUILD_TABLE_LEN_X/scan_dist)-1;					// number of readings in X
      if(scan_xmax<0)scan_xmax=0;if(scan_xmax>=SCAN_X_MAX)scan_xmax=SCAN_X_MAX-1;	// ensure it does not exceed array space
      scan_x_remain=(BUILD_TABLE_LEN_X-(scan_xmax*scan_dist))/2;			// remainder in X
    
      scan_ymax=(int)(BUILD_TABLE_LEN_Y/scan_dist)-1;					// number of readings in Y
      if(scan_ymax<0)scan_ymax=0;if(scan_ymax>=SCAN_Y_MAX)scan_ymax=SCAN_Y_MAX-1;	// ensure it does not exceed array space
      scan_y_remain=(BUILD_TABLE_LEN_Y-(scan_ymax*scan_dist))/2;			// remainder in Y
    
      // now read in the values at each table point
      for(h=0;h<=scan_xmax;h++)
        {
	for(i=0;i<=scan_ymax;i++)
	  {
	  memset(inval,0,sizeof(inval));						// set whole string to NULL
	  if(fread(&inval,7,1,tbl_data)!=1)read_error=TRUE;   				// read in value
	  Pz_raw[h][i]=atof(inval);
	  Pz[h][i]=Pz_raw[h][i];
	  Pz_att[h][i]=1.0;
	  }
	if(fread(&inval,1,1,tbl_data)!=1)read_error=TRUE;     				// account for line feed character
	}
      build_table_scan_flag=TRUE;							// indicated model needs to be mapped to table
      fclose(tbl_data);
      printf("\nBldTblZData - done reading build table Z displacement data.\n\n");
      }
     
    /*
    // scan for lowest z in data set
    maxval=10.0;
    for(h=0;h<=scan_xmax;h++)
      {
      for(i=0;i<=scan_ymax;i++)
        {
	if(Pz_raw[h][i]==0)continue;
	if(Pz_raw[h][i]<maxval)maxval=Pz_raw[h][i];
	}
      }
      
    // normalize to the lowest z in data set, making that value zero
    for(h=0;h<=scan_xmax;h++)
      {
      for(i=0;i<=scan_xmax;i++)
        {
	Pz[h][i]=Pz_raw[h][i]+fabs(maxval);
	}
      }
    */
    printf("Table data loaded.\n");
   
    // search for valid 4X3D.stat file that contains various system state variables from previous run
    //load_unit_state();
    printf("4X3D state data loaded.\n");

    // init fabrication stats display values
    PostGVel=0.0; PostGVAvg=0.0;
    PostGFlow=0.0; PostGFAvg=0.0;

    read_prev_time=time(NULL);
    read_cur_time=time(NULL);
    
    vtest_array=NULL;							// pointer to linked list of vtx for debugging
   
    return(1);
}

