#include "Global.h"	
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

// Function to check temperature and turn heater on/off 
// This is intended to operate as a seperate thread so as to act in a more "real time" fashion.
// To be thread safe, this thread uses a locally declared version of thrm (local_thrm) to operate.
// Once a loop when we know data "is safe", it copies the data into Tool[].thrm for public use.
// Return:  none
void* ThermalControl(void *arg)
{
    int		i=0,h=0,j=0,k=0,m=0;	// scratch counters
    int		dvc,max_dvc,slot;
    int		status=1;		// future return value depending on conditions
    int		bad_temp_count=0;	// counts number of consistently bad temp readings
    int 	settings,autoInc;
    ssize_t 	numRead;		// size of 1wire data stream
    char 	buf[256];    		// data from device
    char 	tmpData[6]; 		// temp C * 1000 reported by device 
    float 	tempC=0.0;		// temporary holder for temperature
    float 	temp_diff=0.0;		// temporary holder for difference in temp
    int		temp_good=0;
    int		pwm_freq=500;
    float 	duty_cycle=0.0;
    int		on_ticks,off_ticks;
    int		pwm_on,pwm_off;
    int		pwm_okay_flag=TRUE;
    int		PWM_duration;
    float 	temp_prop,temp_intg,temp_derv;
    float 	temp_acum[MAX_THERMAL_DEVICES][10];
    struct 	timespec time_step[MAX_THERMAL_DEVICES][10];
    double	interval;
    struct	timespec device_active_time[MAX_THERMAL_DEVICES];	// the time at which a device becomes active
    struct 	timespec errchk_start[MAX_THERMAL_DEVICES],errchk_end[MAX_THERMAL_DEVICES];
    float 	errchk_temp[MAX_THERMAL_DEVICES];
    FILE	*thermal_log;						// handle to thermal error log file (human text of what's happening)
    char 	thrm_scratch[255];					// scratch text field for messages
    char	time_stamp[80];						// text field to hold time stamp
    time_t	curtime;						// used to create time stamp for log
    struct	tm *loc_time;						// used to create time stamp for log
    thermal 	local_thrm[MAX_THERMAL_DEVICES];			// local thermal values used for operation within this thread
    unsigned int	spiData=0xFF;


    printf("Thermal control thread launched... %d thermal devices.\n",MAX_THERMAL_DEVICES);
	
    // open log file for error recording/debug
    // note this file is thread specific.  writing "outside of" or "across" threads is dangerous because it takes too much time.
    thermal_log=fopen("thermal.log","w");
    
    // pre-define first 10 readings for integral values of PID and temp history
    for(i=0;i<MAX_THERMAL_DEVICES;i++)
      {
      for(j=0;j<10;j++)
	{
	temp_acum[i][j]=0.0;
	clock_gettime(CLOCK_REALTIME, &time_step[i][j]);
	}
      clock_gettime(CLOCK_REALTIME, &errchk_start[i]);
      }

    // Cycle through each device and apply any control adjustments
    while(TRUE)
      {
      
      // if still waiting for gpio initialization to complete... just skip rest
      if(gpio_ready_flag==FALSE)continue;

      // Kick the hardware watchdog with alternating high/low signal to show that this code is still alive.
      // Note that due to different types of sensors this will NOT be at a constant frequency.
      // The hardware requires that a high/low or low/high switch occur within ~3.5 seconds (0.28 hz)
      // before shutting the entire PWM module down (thus making all duty cycles 0).
      wdog = !wdog;
      gpioWrite(WATCH_DOG,wdog);
      thermal_thread_alive=1;

      // if still waiting for rest of initialization to complete... just skip rest
      if(init_done_flag==FALSE){delay(1000); continue;}
      
      // once init is done, make full copies of the thermal data structs to init local copies
      for(i=0;i<MAX_THERMAL_DEVICES;i++)local_thrm[i] = Tool[i].thrm;

      // if test settings window is active, skip thermal contol since those setting may conflict with this function
      if(win_testUI_flag==TRUE){delay(1000); continue;}
      
      // since this is the only thread "allowed" to communicate with the PWM module, use the section below this
      // comment to address non-thermal related PWM requests.  these may be to ramp up/down tool 24v or 48v when
      // thermal is not involved, control laser PWM, or even fans with PWM control.
      if(PWM_direct_control==TRUE)					// if global flag set for direct control of PWM...
        {
	PWM_duration=(int)(MAX_PWM*(float)(PWM_duty_cycle)/100.0);	// set based on duty cycle
	if(PWM_duration<0)PWM_duration=0;				// clamp low end
	if(PWM_duration>MAX_PWM)PWM_duration=MAX_PWM;			// clamp high end
	if(PWM_duration>0)						// set flag that PWM is on
	  {
	  pca9685PWMWrite(I2C_PWM_fd,PWM_direct_port,0,PWM_duration); 	// send to PWM chip via I2C
	  PWM_direct_status=ON;
	  }
	else 
	  {
	  pca9685PWMWrite(I2C_PWM_fd,PWM_direct_port,0,0);		// send to PWM chip via I2C for TOOL
	  PWM_direct_status=OFF;					// set flag that PWM is off
	  }								

	// verify by reading back and comparing against what was just sent
	delay(25);
	pca9685PWMRead(I2C_PWM_fd,PWM_direct_port,&pwm_on,&pwm_off);  	// read back what was just sent
	pwm_on = pwm_on & 0x0FFF;					// convert
	pwm_off = pwm_off & 0x0FFF;
	PWM_duty_report=(floor)(pwm_off/MAX_PWM+0.5);			// convert to integer duty cycle bt 0 -100
	if(pwm_on==0 && pwm_off==PWM_duration)				// if it matches what we sent...
	  {
	  if(pwm_okay_flag==FALSE)printf("  PWM function restored. \n"); // only post if was in error
	  pwm_okay_flag=TRUE;						// set flag that all is okay
	  }
	else 								// if it was in error...
	  {
	  pwm_okay_flag=FALSE;						// set flag that all not okay
	  sprintf(thrm_scratch,"\nPWM ERROR on TOOL %d:\n",slot);
	  printf("%s",thrm_scratch);
	  }
	PWM_direct_control=FALSE;					// only allows a single shot... must reset at point of control
	}

      // loop thru each device and determine what to do.  note this all happens within one watch dog cycle.
      // regardless of device status, this loop will attempt to read a temperature depending on how sensor is set.
      for(slot=0;slot<MAX_THERMAL_DEVICES;slot++)	
        {
	// Update thermal logic from public data struct - this basically syncs the local and public thermal structs
	if(Tool[slot].thrm.sync==TRUE)
	  {
	  local_thrm[slot].setpC = Tool[slot].thrm.setpC;
	  Tool[slot].thrm.sync = FALSE;					// reset flag to update no longer needed
	  }
	
	// PWM control for tools like lasers and camera gimbals.  This should be the only thread talking with the PWM hardware.
	// Therefore it makes sense to use it to control PWM inputs via the tool's "heat" variables.
	if(job.type==MARKING || job.type==SCANNING)
	  {
	  // turn off build table heaters if available
	  if(slot==BLD_TBL1)
	    {
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[BLD_TBL1].PWM_port,0,0);	// send to PWM chip via I2C for TOOL
	    local_thrm[BLD_TBL1].heat_status=OFF;
	    }
	  if(slot==BLD_TBL2)
	    {
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[BLD_TBL2].PWM_port,0,0);	// send to PWM chip via I2C for TOOL
	    local_thrm[BLD_TBL2].heat_status=OFF;
	    }
	  // turn chamber on fans on full
	  if(slot==CHAMBER)
	    {
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[CHAMBER].PWM_port,0,MAX_PWM);	// send to PWM chip via I2C for TOOL
	    local_thrm[CHAMBER].heat_status=ON;
	    }

	  // Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	  Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	  Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	  Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	  Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	  Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	  

	  // do not process rest of this loop if marking/scanning... all speed needs to go into toggling laser on/off
	  continue;			
	  }
	  
	
	// assume no sensor available... set to non-value
	tempC=(-100);							// set default
	  
	// read temperature from 1wire sensor
	if(local_thrm[slot].sensor==ONEWIRE)
	  {
	  // opening the device's file triggers new reading
	  local_thrm[slot].fd=open(local_thrm[slot].devPath,O_RDONLY);  	
	    
	  // if device is no longer still active ...
	  if(local_thrm[slot].fd<0)
	    {
	    status=0;   
	    }
	  // if device is still active ...
	  else
	    {
	    // get current temperature
	    if((numRead=read(local_thrm[slot].fd,buf,256))>0) 		// load string off data file
	      {
	      strncpy(tmpData,strstr(buf,"t=")+2,5); 			// locate temperature data
	      tempC=strtof(tmpData,NULL);				// covert to float
	      tempC=tempC/1000.0;					// apply conversion factor
	      }
	    else
	      {
	      status=0;
	      }
	    close(local_thrm[slot].fd);					// close 1wire
	    }
	  }
		
	// read temperature from RTD sensor
	// 100 ohm platinum RTDs will have a resistance of about 106 ohms at room temperature and be
	// linear up to a resistance of 180 ohms at 230C.
	if(local_thrm[slot].sensor==RTDH || local_thrm[slot].sensor==RTD)
	  {
	  tempC=read_RTD_sensor(local_thrm[slot].temp_port,3,1); 				// acquire temperature
	  local_thrm[slot].tempV=read_RTD_sensor(local_thrm[slot].temp_port,3,0);		// acquire voltage
	  //if(slot==0)printf("Thrm: tempC=%f  volt=%f \n",tempC,local_thrm[slot].tempV); 	// debug
	  }
	// 1000 ohm platinum RTDs  
	if(local_thrm[slot].sensor==RTDT)
	  {
	  tempC=read_RTD_sensor(local_thrm[slot].temp_port,3,2); 			// acquire temperature
	  local_thrm[slot].tempV=read_RTD_sensor(local_thrm[slot].temp_port,3,0);		// acquire voltage
	  //if(slot==0)printf("Thrm: tempC=%f  volt=%f \n",tempC,local_thrm[slot].tempV); // debug
	  }
	  
	// read temperature from thermistor
	if(local_thrm[slot].sensor==THERMISTER)
	  {
	  // since NTC thermistors have a rediculously high resistance (~100k ohms) at room temperature,
	  // they will register super high temperature value until they get close to their operational temperature.
	  // if the right thermistor is used, it should have a resistance of about 785ohms at 220C.
	  tempC=read_THERMISTOR_sensor(local_thrm[slot].temp_port,1,1);		// acquire temperature
	  
	  if(tempC<10)printf("\nlow value thermistor temp: %f \n",tempC);
	  /*
	  if(tempC>350)								// if no real reading yet, fake temp to value below target to force heating and keep watching
	    {
	    tempC=local_thrm[slot].operC-100;
	    local_thrm[slot].heat_duty=50.0;
	    }				
	  */
	  //delay(150);								// it takes ~100 ms to read and adjust, balance with 250 ms for other tools
	  }

	// read temperature from thermocouple via SPI bus
	if(local_thrm[slot].sensor==THERMOCOUPLE)
	  {
/*
	  int ret;
	  uint8_t tx[] = {0xF0, 0x0D};
	  uint8_t rx[] = {0x00, 0x00};
	  
	  struct spi_ioc_transfer tr = 
		  {
		  .tx_buf = (unsigned long)tx,
		  .rx_buf = (unsigned long)rx,
		  .len = 2,
		  .delay_usecs = txdelay,
		  .speed_hz = speed,
		  .bits_per_word = bits,
		  };
  
	  ret = ioctl(SPI_fd, SPI_IOC_MESSAGE(1), &tr);
	  if (ret < 1) printf("can't send spi message\n");
  
	  printf("%d \n",ret);
  
	  for (ret = 0; ret < 2; ret++) 
	    {
	    if (!(ret % 6)) puts("");
	    printf("%.2X ", rx[ret]);
	    }
	  puts("");
*/
/*
	  if(m>100)
	    {
	    m=0;
	    j=ioctl(SPI_fd, SPI_IOC_MESSAGE(1), &tr);			// execute transfer

	    printf("%d tx0=%x  rx0=%x  tx1=%x  rx1=%x \n",j,tx[0],rx[0],tx[1],rx[1]);
	    //printf("%d tx0=%x  rx0=%x  \n",j,tx,rx);

	    if(j==4)							// only if right number of bytes comes back
	      {
	      rx[1] >>= 2;						// shift high byte by 2 bits
	      tempC = (float)(rx[1] & 0x1FFF);				// bottom 13 bits
	      if((rx[1] & 0x2000)!=0)tempC=(-1)*tempC;			// last bit is for negative (or not)
	      }
	    else 
	      {
	      tempC=1600.0;
	      }
	    }
*/
	  if(m>10)
	    {
	    spiData=0xFAFA;
	    //j=wiringPiSPIDataRW (SPI_fd, (unsigned char *)&spiData, 4) ;
	    printf("%d  raw=%.4X  ",j,spiData);
	    spiData >>= 18;						// shift by 18 bits to get temp value
	    tempC = (float)(spiData & 0x1FFF);				// bottom 13 bits
	    if((spiData & 0x2000)!=0)tempC=(-1)*tempC;			// last bit is for negative (or not)
	    printf("%6.3f \n",tempC);
	    m=0;
	    }
	    
	  m++;
	  }
	  
	// if tool is not installed, skip to next.
	// note this is done AFTER reading temperatures.  this allows calibration loads to work without a tool.
	if(tempC>(-99.0))
	  {
	  tempC = tempC-local_thrm[slot].temp_offset;			// adjust relative to saved offset
	  local_thrm[slot].tempC=tempC;					// save off temp for calibrations
	  }
	if(Tool[slot].state<TL_LOADED)
	  {
	  // Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	  Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	  Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	  Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	  Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	  Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	  continue;								// don't continue with any further processing
	  }
	  
	
	// at this point we have acquired a temperature reading for this slot from some means
	
	// verify good temperature reading by comparing against last value
	// since the system runs fairly quickly (i.e. about 1 hz) thermal mass of even light weight components
	// will not allow an "instantaneous" change of more than a few degrees.
	temp_good=TRUE;							// default flag to indicate it is good
	
	// check to ensure temp value read is "not-a-number" which can happen in certain cases
	if(isnanf(tempC)==TRUE)
	  {
	  temp_good=FALSE;						// ... set flag to bad reading
	  pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0);	// ... send OFF to PWM to ensure heat is off for this slot
	  local_thrm[slot].heat_status=0;				// ... set flag that heat is off
	  Tool[slot].state=TL_FAILED;					// ... flag tool as failed
	  if(Tool[slot].pwr24==TRUE)Tool[slot].epwr24=TRUE;		// ... flag to indicate problem with thermal control
	  if(Tool[slot].pwr48==TRUE)Tool[slot].epwr48=TRUE;		// ... flag to indicate problem with thermal control

	  // build time stamp and make log entry
	  curtime=time(NULL);
	  loc_time=localtime(&curtime);
	  memset(time_stamp,0,sizeof(time_stamp));
	  strftime(time_stamp,80,"\n%x %X ",loc_time);
	  sprintf(thrm_scratch,"\n%s  ERROR:  Temperature not reading correctly on device %d ! \n",time_stamp,slot);
	  printf("%s",thrm_scratch);
	  if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}

	  // Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	  Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	  Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	  Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	  Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	  Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	  continue;								// don't continue with any further processing
	  }
	
	// check for temp not changing fast enough which could be caused by a detached temperature sensor.  
	// if heat power is at high duty cycle, but temp change is less than 1 deg C within the period dictated by
	// the tool "duration_per_C" time.  The delay in enabling this check is due to the fact that if the tool
	// was previously heated, then is unplugged from the unit, then plugged back in hot (but cooling rapidly), 
	// the temperature readings will be negative for a few cycles until the heater overcomes the tool's thermal
	// mass and starts pushing the temp back up.
	clock_gettime(CLOCK_REALTIME, &errchk_end[slot]);		// get end time for this slot
	if((errchk_end[slot].tv_sec-Tool[slot].active_time_stamp.tv_sec)>(local_thrm[slot].duration_per_C))	// if active for more than one duration...
	  {
	  if((errchk_end[slot].tv_sec-errchk_start[slot].tv_sec)>local_thrm[slot].duration_per_C)
	    {
	    temp_diff=tempC-errchk_temp[slot];
	    if(temp_diff>0.0 && temp_diff<1.0 && (Tool[slot].pwr24==TRUE || Tool[slot].pwr48==TRUE))  		// if minimal increasing temp change and slot has a heater...
	      {
	      if(local_thrm[slot].heat_duty>98.0)			// ... if duty cycle is high enough...
		{
		temp_good=FALSE;					// ... set flag to bad reading
		pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0); // ... send OFF to PWM on regular basis to prevent PWM from sleeping
		local_thrm[slot].heat_status=0;				// ... set flag that heat is off
		Tool[slot].state=TL_FAILED;				// ... flag tool as failed
		if(Tool[slot].pwr24==TRUE)Tool[slot].epwr24=TRUE;	// ... flag to indicate problem with thermal control
		if(Tool[slot].pwr48==TRUE)Tool[slot].epwr48=TRUE;	// ... flag to indicate problem with thermal control

		// build time stamp and make log entry
		curtime=time(NULL);
		loc_time=localtime(&curtime);
		memset(time_stamp,0,sizeof(time_stamp));
		strftime(time_stamp,80,"\n%x %X ",loc_time);
		sprintf(thrm_scratch,"\n%s  WARNING:  Temperature at %5.2f not changing on device %d ! \n",time_stamp,tempC,slot);
		printf("%s",thrm_scratch);
		if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
		sprintf(thrm_scratch,"          Temp change of %5.2f in %4.2f secs at duty_cycle=%6.3f.\n",(tempC-errchk_temp[slot]),local_thrm[slot].duration_per_C,local_thrm[slot].heat_duty);
		printf("%s",thrm_scratch);
		if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
		}
	      }
	    clock_gettime(CLOCK_REALTIME, &errchk_start[slot]);		// ... reset clock for this slot
	    errchk_temp[slot]=tempC;					// ... record temp at this time
	    }
	  }
	
	// check for temp changing too fast
	// this could be caused by a malfunctioning temperature sensor or bad PWM drive.
	if(fabs(tempC-local_thrm[slot].tempC)>10.0)			// if temp rate of change too fast, ignore this reading
	  {
	  temp_good=FALSE;						// ... set flag to bad reading
	  bad_temp_count++;						// ... increment bad reading count
	  if(bad_temp_count>20)						// ... if too many bad reads, shut down this channel
	    {
	    interval=(double)(time_step[slot][9].tv_nsec-time_step[slot][8].tv_nsec)/10000/10000;
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0);	// ... send OFF to PWM to ensure heat is off for this slot
	    local_thrm[slot].heat_status=0;				// ... set flag that heat is off
	    Tool[slot].state=TL_FAILED;					// ... flag tool as failed
	    if(Tool[slot].pwr24==TRUE)Tool[slot].epwr24=TRUE;		// ... flag to indicate problem with thermal control
	    if(Tool[slot].pwr48==TRUE)Tool[slot].epwr48=TRUE;		// ... flag to indicate problem with thermal control

	    // build time stamp and make log entry
	    curtime=time(NULL);
	    loc_time=localtime(&curtime);
	    memset(time_stamp,0,sizeof(time_stamp));
	    strftime(time_stamp,80,"\n%x %X ",loc_time);
	    sprintf(thrm_scratch,"\n%s  ERROR:  Unstable temperature readings on device %d ! \n",time_stamp,slot);
	    printf("%s",thrm_scratch);
	    if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	    sprintf(thrm_scratch,"        Temp change of %5.2f in %4.2f secs.\n",(tempC-local_thrm[slot].tempC),interval);
	    printf("%s",thrm_scratch);
	    if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	    }
	  if(bad_temp_count==1)local_thrm[slot].tempC=tempC;		// ... must initialize somehow... maybe tool was plugged in hot
	  if(bad_temp_count>25)bad_temp_count=0;			// ... don't allow to run away
	  }
	
	// check for temp out of reasonable range
	// this could be caused by poor calibration, or just drving it too hard.
	if(tempC<(-99.0) || tempC>(local_thrm[slot].maxtC+10.0))	// if temp value is out of range, ignore this reading...
	  {
	  temp_good=FALSE;						// temp out of normal operating range... set flag to bad reading
	  bad_temp_count++;						// increment bad reading count
	  if(bad_temp_count>5)						// if too many bad reads, shut down this channel
	    {
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0);	// ... send OFF to PWM to ensure heat is off for this slot
	    local_thrm[slot].heat_status=0;				// ... set flag that heat is off
	    Tool[slot].state=TL_FAILED;					// ... flag tool as failed
	    if(Tool[slot].pwr24==TRUE)Tool[slot].epwr24=TRUE;		// ... flag to indicate problem with thermal control
	    if(Tool[slot].pwr48==TRUE)Tool[slot].epwr48=TRUE;		// ... flag to indicate problem with thermal control
	    
	    // build time stamp and make log entry
	    curtime=time(NULL);
	    loc_time=localtime(&curtime);
	    memset(time_stamp,0,sizeof(time_stamp));
	    strftime(time_stamp,80,"\n%x %X ",loc_time);
	    sprintf(thrm_scratch,"\n%s  ERROR:  Unreasonable temperature on device %d ! \n",time_stamp,slot);
	    printf("%s",thrm_scratch);
	    if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	    sprintf(thrm_scratch,"        Last temp=%f     New temp=%f \n",local_thrm[slot].tempC,tempC);
	    printf("%s",thrm_scratch);
	    if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	    }
	  if(bad_temp_count==1)local_thrm[slot].tempC=tempC;		// must initialize somehow... maybe tool was plugged in hot
	  if(bad_temp_count>25) bad_temp_count=0;			// don't allow to run away
	  }
	  
	// if tempC value passed quality tests...
	if(temp_good==TRUE)						
	  {
	  // record history
	  if(slot<MAX_TOOLS)history[H_TOOL_TEMP][hist_ctr[HIST_TEMP]]=tempC;
	  if(slot==BLD_TBL1)history[H_TABL_TEMP][hist_ctr[HIST_TEMP]]=tempC;
	  if(slot==CHAMBER )
	    {
	    history[H_CHAM_TEMP][hist_ctr[HIST_TEMP]]=tempC;
	    
	    // increment history counter here, left shifting data if over max history limit
	    // do this under "CHAMBER" so it only happens once each time thru all themal devices
	    hist_ctr[HIST_TEMP]++;
	    if(hist_ctr[HIST_TEMP]>(MAX_HIST_COUNT-1))			// if history buffer full...
	      {
	      for(i=H_TOOL_TEMP;i<=H_CHAM_TEMP;i++)			// shift all elements to left by one
		{
		for(j=0;j<(hist_ctr[HIST_TEMP]-1);j++){history[i][j]=history[i][j+1];}
		}
	      hist_ctr[HIST_TEMP]=MAX_HIST_COUNT-1;
	      }
	    }
	  
	  //printf("  slot=%d  tempC=%6.2f \n",slot,tempC);
	  bad_temp_count=0;						// re-init bad temp count
	  local_thrm[slot].tempC=tempC;					// define this tools temp as what was read
	  if(Tool[slot].pwr24==FALSE && Tool[slot].pwr48==FALSE)	// if no heater on this slot, move onto next slot (tables may take 48v)
	    {
	    // Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	    Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	    Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	    Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	    Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	    Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	    continue;
	    }
	  if(Tool[slot].state==TL_FAILED)				// even if failed allow temp to update...
	    {
	    pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0);	// ... send OFF to PWM on regular basis to prevent PWM from sleeping
	    printf("Device=%d  state=FAILED  PWM=OFF\n",slot);
	    printf("SetPt=%6.3f CurPt=%6.3f Itvl=%6.3f Duty=%6.3f Dur=%d Stat=%d \n",
	      local_thrm[slot].setpC,local_thrm[slot].tempC,interval,
	      (temp_prop+temp_intg+temp_derv),local_thrm[slot].heat_duration,local_thrm[slot].heat_status);
	    printf("        Duty=%6.3f  TempP=%6.3f  TempI=%6.3f  TempD=%6.3f \n",(temp_prop+temp_intg+temp_derv),temp_prop,temp_intg,temp_derv);
	    printf("        T[9]=%6.3f  T[8]=%6.3f  Kd=%6.3f \n\n",temp_acum[slot][9],temp_acum[slot][8],local_thrm[slot].Kd);

	    // Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	    Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	    Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	    Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	    Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	    Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	    continue;
	    }
	
	  // at this point a new valid temperature has been obtained for this slot
	  // adjust PWM to respond to temp reading and drive heater
     
	  // calculate raw error and add steady state error required to provide convergence (needed to control thermal systems)
	  temp_diff=local_thrm[slot].setpC-local_thrm[slot].tempC+local_thrm[slot].errtC;	
	    	
	  // calculate proportional error
	  temp_prop=temp_diff*local_thrm[slot].Kp;			// apply Kp		
	    
	  // calculate intergal error - sum total of last 10 error readings
	  temp_intg=0.0;
	  for(j=1;j<10;j++)						// drop oldest error and make room for newest
	    {
	    temp_acum[slot][j-1]=temp_acum[slot][j];			// shift all temp values forward by one
	    time_step[slot][j-1]=time_step[slot][j];			// shift all time values forward by one
	    temp_intg+=temp_acum[slot][j];				// accumulate sum of error for last 9 readings
	    }
	  temp_acum[slot][9]=temp_diff;					// add newest error to list
	  clock_gettime(CLOCK_REALTIME, &time_step[slot][9]);		// measure time error was calculated
	  temp_intg+=temp_diff;						// add newest error to sum total
	  temp_intg*=local_thrm[slot].Ki;				// apply Ki	
	    
	  // calculate derivative error - slope (rate of change) of last two readings
	  interval=(double)(time_step[slot][9].tv_nsec-time_step[slot][8].tv_nsec)/10000/10000;
	  if(interval<0.50)interval=0.50;				// half sec limit - typically runs ~ 0.6 secs
	  temp_derv=(temp_acum[slot][9]-temp_acum[slot][8])/interval;	// calc slope
	  temp_derv*=local_thrm[slot].Kd;				// apply Kd
	   
	  // now apply Kp, Ki, Kd to error if tool is ready for it
	  if(Tool[slot].state>=TL_LOADED && Tool[slot].state!=TL_FAILED)
	    {
	    // Calculate new heater duty cycle based on PID error
	    local_thrm[slot].heat_duty = (temp_prop+temp_intg+temp_derv);			// apply PIDs to get total temp error amt
	    if(local_thrm[slot].heat_duty<0)local_thrm[slot].heat_duty=0.0;			// clamp low end
	    if(local_thrm[slot].heat_duty>100)local_thrm[slot].heat_duty=100.0;			// clamp high end
	    if(local_thrm[slot].tempC>local_thrm[slot].maxtC)local_thrm[slot].heat_duty=0.0;	// safety stop to avoid run-away

	    local_thrm[slot].heat_duration=(int)(MAX_PWM*local_thrm[slot].heat_duty/100);	// set based on duty cycle
	    if(local_thrm[slot].heat_duration<0)local_thrm[slot].heat_duration=0;		// clamp low end
	    if(local_thrm[slot].heat_duration>MAX_PWM)local_thrm[slot].heat_duration=MAX_PWM;	// clamp high end
	    
	    //if(slot==CHAMBER)local_thrm[slot].heat_duration=MAX_PWM;				// filter fan drive
	    
	    if(local_thrm[slot].heat_duration>0 && local_thrm[slot].heat_duration<=MAX_PWM)	// apply to PWM chip
	      {
	      pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,local_thrm[slot].heat_duration); // send to PWM chip via I2C for TOOL
	      local_thrm[slot].heat_status=ON;							// set flag that heat is on
	      }
	    else
	      {
	      pca9685PWMWrite(I2C_PWM_fd,local_thrm[slot].PWM_port,0,0);			// send to PWM chip via I2C for TOOL
	      local_thrm[slot].heat_status=OFF;							// set flag that heat is off
	      }
	     
	    // ensure the correct value was sent and the PWM is still functioning by reading back what we just sent
	    pca9685PWMRead(I2C_PWM_fd,local_thrm[slot].PWM_port,&pwm_on,&pwm_off);  		// read back what was just sent
	    pwm_on = pwm_on & 0x0FFF;								// convert
	    pwm_off = pwm_off & 0x0FFF;
	    if(pwm_on==0 && pwm_off==local_thrm[slot].heat_duration)				// if it matches what we sent...
	      {
	      if(pwm_okay_flag==FALSE)printf("  PWM function restored. \n");			// only post if was in error
	      pwm_okay_flag=TRUE;								// set flag that all is okay
	      }
	    else 										// if it was in error...
	      {
	      pwm_okay_flag=FALSE;								// set flag that all not okay

	      // build time stamp and make log entry
	      curtime=time(NULL);
	      loc_time=localtime(&curtime);
	      memset(time_stamp,0,sizeof(time_stamp));
	      strftime(time_stamp,80,"\n%x %X ",loc_time);
	      sprintf(thrm_scratch,"\n%s  PWM ERROR on TOOL %d:\n",time_stamp,slot);
	      printf("%s",thrm_scratch);
	      if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	      sprintf(thrm_scratch,"  PWM On= 0:%d   PWM Off= %d:%d \n",pwm_on,local_thrm[slot].heat_duration,pwm_off);
	      printf("%s",thrm_scratch);
	      if(thermal_log!=NULL){fwrite(thrm_scratch,1,strlen(thrm_scratch),thermal_log); fflush(thermal_log);}
	      //printf("  SetPt=%6.3f CurPt=%6.3f Itvl=%6.3f Duty=%6.3f Dur=%d Stat=%d \n",
	      //  local_thrm[slot].setpC,local_thrm[slot].tempC,interval,
	      //  (temp_prop+temp_intg+temp_derv),local_thrm[slot].heat_duration,local_thrm[slot].heat_status);
	      //printf("  Duty=%6.3f  TempP=%6.3f  TempI=%6.3f  TempD=%6.3f \n",(temp_prop+temp_intg+temp_derv),temp_prop,temp_intg,temp_derv);

	      // Re-setup the chip. Enable auto-increment of registers.
	      printf("  Attempting automatic reset... \n");
	      pca9685PWMReset(I2C_PWM_fd);							// reset to default values (know state)
	      pca9685Setup(0,I2C_PWM_fd,50);							// set frequency
	      pca9685PWMWrite(I2C_PWM_fd,PIN_BASE,0,0);						// turn off everything
	      }
	    /*
	   if(slot==BLD_TBL1 || slot==BLD_TBL2)
	    {
	    printf("\nTool=%d state=%d temp_port=%d PWM_port=%d\n",slot,Tool[slot].state,local_thrm[slot].temp_port,local_thrm[slot].PWM_port);
	    printf("SetPt=%6.3f CurPt=%6.3f Itvl=%6.3f Duty=%6.3f Dur=%d Stat=%d \n",
	      local_thrm[slot].setpC,local_thrm[slot].tempC,interval,local_thrm[slot].heat_duty,local_thrm[slot].heat_duration,local_thrm[slot].heat_status);
	    printf("        KSum=%6.3f  TempP=%6.3f  TempI=%6.3f  TempD=%6.3f \n",(temp_prop+temp_intg+temp_derv),temp_prop,temp_intg,temp_derv);
	    printf("        T[9]=%6.3f  T[8]=%6.3f  Kd=%6.3f \n\n",temp_acum[slot][9],temp_acum[slot][8],local_thrm[slot].Kd);
	    }
	    */
	    
	    } // end apply PID values
	  } // end if good temp readings

	// Update public data struct from local thermal logic - this basically syncs the local and public thermal structs
	Tool[slot].thrm.tempC = local_thrm[slot].tempC;
	Tool[slot].thrm.tempV = local_thrm[slot].tempV;
	Tool[slot].thrm.heat_status = local_thrm[slot].heat_status;
	Tool[slot].thrm.heat_duration = local_thrm[slot].heat_duration;
	Tool[slot].thrm.heat_duty = local_thrm[slot].heat_duty;
	
	} // end of slot for loop
      } // end of while loop
      
      
  } 


// function to read RTD sensor via A2D on I2C
// inputs:  channel=thermal device, readings=desired number of readings to average, typ: 0=raw voltage 1=device temp
// return:  volts or temp depending on typ value.  if <(-1) then an error occured.
float read_RTD_sensor(int channel, int readings, int typ) 
{
	int i,cnt,dvc;
	uint8_t buf[4];
	int16_t val;
	float slope=1.0;
	float ylow=0.0,yhigh=0.0;
	float yint;
	float tempX=0.0,voltX=0.0;
	float avg_temp=0.0;
	float avg_volt=0.0;
	
	// validate inputs
	if(readings<1)readings=1;					// must take at least one reading
	if(channel<0 || channel>=MAX_THERMAL_DEVICES)return(-2.0);	// must be a valid channel
	
	// calculate slope and y intercept
	// in this case, Y=volts coming into A2D  and   X=corrisponding temperature
	if(typ==2)
	  {
	  slope = (4.03 - 3.42) / (250 - 19);
	  //yint  = (3.42 - (slope * 19));
	  yint=3.21;
	  }
	else 
	  {
	  slope = (Tool[channel].thrm.calib_v_high - Tool[channel].thrm.calib_v_low) / (Tool[channel].thrm.calib_t_high - Tool[channel].thrm.calib_t_low);
	  //yint = Tool[channel].thrm.calib_v_low - (slope * Tool[channel].thrm.calib_t_low);
	  yint=0.75;
	  }

	for(i=0;i<readings;i++)
	  {

	  // set config register for ADS1115 and start conversion
	  // first 8 bits
	  //		Bit		Value 	Comment
	  //		15		1	Start Conversion
	  //		14:12		100	Singled-ended input AIN0 (CHANNEL 0)
	  //				101	Singled-ended input AIN1 (CHANNEL 1)
	  //				110	Singled-ended input AIN2 (CHANNEL 2)
	  //				111	Singled-ended input AIN3 (CHANNEL 3)
	  //		11:9		000	FS 6.144v to cover the active range of interest of thermistor/RTD
	  //		8		1	Single shot mode
	  // next 8 bits
	  //		7:5		000	8 Samples per second (or 1 sample takes 1/8 of a second)
	  //		4:2		000	Not relevant
	  //		1:0		11	Disable comparator	
	  //
	  // 		Resulting in 1 100 0001  00000011  or 0xc1 0x03  (for channel 0) as set below
	
	  buf[0] = 1;   						// config register is 1
	  buf[1] = 0xc1;						// first set default channel to 0 (see above)
	  if(channel==0){dvc=0;buf[1]=0xc1;}				// tool slot 0
	  if(channel==1){dvc=0;buf[1]=0xd1;}				// tool slot 1
	  if(channel==2){dvc=0;buf[1]=0xe1;}				// tool slot 2
	  if(channel==3){dvc=0;buf[1]=0xf1;}				// tool slot 3
	  if(channel==4){dvc=1;buf[1]=0xc1;}				// channel 1 of build table
	  if(channel==5){dvc=1;buf[1]=0xd1;}				// channel 2 of build table
	  if(channel==6){dvc=1;buf[1]=0xe1;}				// chamber
	  if(channel==7){dvc=1;buf[1]=0xf1;}				// (empty) spare
	  buf[2] = 0x03;						// next set of params (see above)
	  if(write(I2C_A2D_fd[dvc],buf,3) != 3)return(-4.0);		// failed to write to register
	  delay(15);							// greater delay seems help quality of read

	  // wait for conversion complete
	  cnt=0;
	  do
	    {
	    if(read(I2C_A2D_fd[dvc],buf,2) != 2)return(-5.0);		// failed to read conversion
	    delay(15);							// greater delay seems help quality of read
	    cnt++;
	    if(cnt>30)break;						// bail out if stuck otherwise RPi will lock process status
	    }while ((buf[0] & 0x80) == 0);				// keep reading until none left
	  if(cnt>30)return(-5.0);					// fail if too many attempt to read

	  // read conversion register
	  buf[0]=0;							// conversion register is at 0
	  if(write(I2C_A2D_fd[dvc],buf,1) != 1)return(-6.0);		// failed write register select
	  if( read(I2C_A2D_fd[dvc],buf,2) != 2)return(-6.0);		// failed to read conversion
 
	  // convert output and collect results
	  val = (int16_t)buf[0]*256 + (uint16_t)buf[1];			// calc value from both bytes
	  voltX=(float)val*6.144/32768.0;				// values based on A2D's FS param setting
	  if(voltX>6.000)return(-7.0);					// out of range on high side
	  if(voltX<0.001)return(-7.0);					// out of range on low side
	  tempX=(voltX-yint)/slope;
	  avg_volt+=voltX;
	  avg_temp+=tempX;

	  // Kick the hardware watchdog with alternating high/low signal to show that this code is still alive.
	  // This is needed in this loop if more than 3 readings are being averaged.
	  wdog = !wdog;
	  gpioWrite(WATCH_DOG,wdog);
	  thermal_thread_alive=1;
	  }

	avg_volt=avg_volt/(float)(readings);
	avg_temp=avg_temp/(float)(readings);

	if(typ==0)return(avg_volt);
	return(avg_temp);
}

// function to read THERMISTOR sensor via A2D on I2C
// 12 bit A2D used with a voltage range of 0 - 5v
// inputs:  channel=thermal device, readings=desired number of readings to average, typ: 0=raw voltage 1=device temp
// design:  the power driver CCA provides 5vdc thru a 560ohm resistor, after which is the A2D pickup, then the
// thermistor which then connects to ground
float read_THERMISTOR_sensor(int channel, int readings, int typ) 
{
	int i,cnt,dvc;
	uint8_t buf[4];
	int16_t val;
	float slope=1.0;
	float ylow=0.0,yhigh=0.0;
	float yint=0.0;
	float in_vdc;
	float A0,A1,A2;
	float tempX=0.0,voltX=0.0,restX=0.0;
	float avg_temp=0.0;
	float avg_volt=0.0;
	
	// validate inputs
	if(readings<1)readings=1;
	if(channel<0 || channel>=MAX_THERMAL_DEVICES)return(-1);
	
	in_vdc=5.1000;							// input voltage of system - ideally not fixed and read by A2D input instead
	A0=0.00080561;							// Stienhart-Hart constants as calculated by test points
	A1=0.00020225;
	A2=1.5327E-7;

	for(i=0;i<readings;i++)
	  {

	  // set config register for ADS1115 and start conversion
	  // first 8 bits
	  //		Bit		Value 	Comment
	  //		15		1	Start Conversion
	  //		14:12		100	Singled-ended input AIN0 (CHANNEL 0)
	  //				101	Singled-ended input AIN1 (CHANNEL 1)
	  //				110	Singled-ended input AIN2 (CHANNEL 2)
	  //				111	Singled-ended input AIN3 (CHANNEL 3)
	  //		11:9		000	FS 6.144v to cover the active range of interest of thermistor/RTD
	  //		8		1	Single shot mode
	  // next 8 bits
	  //		7:5		000	8 Samples per second (or 1 sample takes 1/8 of a second)
	  //		4:2		000	Not relevant
	  //		1:0		11	Disable comparator	
	  //
	  // 		Resulting in 1 100 0001  00000011  or 0xc1 0x03  (for channel 0) as set below
	
	  buf[0] = 1;   						// config register is 1
	  buf[1] = 0xc1;						// first set default channel to 0 (see above)
	  if(channel==0){dvc=0;buf[1]=0xc1;}				// tool slot 0
	  if(channel==1){dvc=0;buf[1]=0xd1;}				// tool slot 1
	  if(channel==2){dvc=0;buf[1]=0xe1;}				// tool slot 2
	  if(channel==3){dvc=0;buf[1]=0xf1;}				// tool slot 3
	  if(channel==4){dvc=1;buf[1]=0xc1;}				// channel 1 of build table
	  if(channel==5){dvc=1;buf[1]=0xd1;}				// channel 2 of build table
	  if(channel==6){dvc=1;buf[1]=0xe1;}				// chamber
	  if(channel==7){dvc=1;buf[1]=0xf1;}				// (empty) spare
	  buf[2] = 0x03;						// next set of params (see above)
	  if(write(I2C_A2D_fd[dvc],buf,3) != 3)return(-3.0);		// failed to write to register
	  delay(15);

	  // wait for conversion complete
	  cnt=0;
	  do
	    {
	    if(read(I2C_A2D_fd[dvc],buf,2) != 2)return(-3.0);		// failed to read conversion
	    delay(15);							// greater delay seems help quality of read
	    if(cnt>30)break;						// bail out if stuck otherwise RPi will lock process status
	    cnt++;
	    }while ((buf[0] & 0x80) == 0);				// keep reading until none left
	  if(cnt>30)return(-3);						// fail if too many attempt to read

	  // read conversion register
	  buf[0]=0;							// conversion register is 0
	  if(write(I2C_A2D_fd[dvc],buf,1) != 1)return(-3.0);		// failed write register select
	  if( read(I2C_A2D_fd[dvc],buf,2) != 2)return(-3.0);		// failed to read conversion
 
	  // convert output and collect results
	  val = (int16_t)buf[0]*256 + (uint16_t)buf[1];
	  voltX=(float)val*6.144/32768.0;				// values based on A2D's FS param setting
	  if(voltX>6.000)return(-2.0);					// out of range on high side
	  if(voltX<0.001)return(-2.0);					// out of range on low side
	  if(voltX>(in_vdc-0.1))voltX=(in_vdc-0.1);			// in event ref voltage exactly matches
	  restX=(voltX*560)/(in_vdc-voltX);				// resistance of thermistor
	  tempX=(1/( A0 + (A1*log(restX)) + (A2*(log(restX)*log(restX)*log(restX))) ))-273.15;	// Stienhart-Hart equation
	  avg_volt+=voltX;
	  avg_temp+=tempX;

	  // Kick the hardware watchdog with alternating high/low signal to show that this code is still alive.
	  // This is needed in this loop if more than 3 readings are being averaged.
	  wdog = !wdog;
	  gpioWrite(WATCH_DOG,wdog);
	  thermal_thread_alive=1;

	  //printf(" restX=%f  voltX=%f  tempX=%f \n",restX,voltX,tempX);
	  }

	avg_volt=avg_volt/(float)readings;
	avg_temp=avg_temp/(float)readings;

	if(typ==0)return(avg_volt);
	return(avg_temp);
}






