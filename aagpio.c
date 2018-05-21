#include <stdio.h>
#include <sys/poll.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>


#define GPIO_MAX 63
#define GPIO_MIN 0

static	int led_gpio_num; // Number GPIO for led
static	int but_gpio_num; // Number GPIO for button
static	char led_gpio_str[4];
static	char but_gpio_str[4];

static const char * name_export = "/sys/class/gpio/export";
static const char * name_unexport = "/sys/class/gpio/unexport";

static int conf_des;
static int led_des;
static int but_des;

static volatile sig_atomic_t term_flag;

//SIGTERM and SIGINT handler
static void sig_handl(int sig)
{
	term_flag = 1;
}
                                                                            
void errExit(const char * msg)
{
	syslog(LOG_ERR,"Error: %s. errno = %d. err discription: %s", msg, errno, strerror(errno));
	//unexport gpio
	if ((conf_des=open(name_unexport,O_SYNC|O_WRONLY)) == -1){
		syslog(LOG_ERR,"error in open unexport file");
		closelog();
		exit(EXIT_FAILURE);
	}
	if (write(conf_des, led_gpio_str, strlen(led_gpio_str)) == -1)
		syslog(LOG_ERR,"error write in unexport file for led gpio");
	if (write(conf_des, but_gpio_str, strlen(but_gpio_str)) == -1)
		syslog(LOG_ERR,"error write in unexport file for button gpio");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"error close unexport file");

	closelog();
	exit(EXIT_FAILURE);
}

int main(int argc,char *  argv[])
{	
	int cnt;
	char filename[PATH_MAX];
	char but_val;
	//opensyslog
	openlog("GPIO", LOG_PID|LOG_PERROR, LOG_USER);
	// get GPIO number
	if (argc != 3){
		syslog(LOG_ERR,"Need gpio numbers. Usage: gpio led_gpio_number button_gpio_number");
		closelog();
		exit(EXIT_FAILURE);
	}
	cnt = sscanf(argv[1],"%d", &led_gpio_num);
	if (cnt !=1 || led_gpio_num < GPIO_MIN || led_gpio_num > GPIO_MAX){
		syslog(LOG_ERR,"bad led gpio number");
		closelog();
		exit(EXIT_FAILURE);
	}
	cnt = sscanf(argv[2],"%d", &but_gpio_num);
	if (cnt !=1 || led_gpio_num == but_gpio_num || but_gpio_num < GPIO_MIN || but_gpio_num > GPIO_MAX){
		syslog(LOG_ERR,"bad button gpio number");
		closelog();
		exit(EXIT_FAILURE);
	}
	
	//set new handler for SIGTERM and SIGINT                                                              
	struct sigaction act;                                                                                 
	sigemptyset(&act.sa_mask);                                                                            
	act.sa_handler = &sig_handl;                                                                          
	act.sa_flags = 0;                                                                                     
	if (sigaction(SIGTERM, &act, NULL) == -1){                                                             
		syslog(LOG_ERR,"sigaction");
		exit(EXIT_FAILURE);
	}		
	if (sigaction(SIGINT, &act, NULL) == -1){                                                              
		syslog(LOG_ERR,"sigaction");
		exit(EXIT_FAILURE);
	}

	//export GPIO for led and button
	if (snprintf(led_gpio_str,4,"%d",led_gpio_num) < 0){
		syslog(LOG_ERR,"error snprintf");
		closelog();
		exit(EXIT_FAILURE);
	}	
	if (snprintf(but_gpio_str,4,"%d",but_gpio_num) < 0){
		syslog(LOG_ERR,"error snprintf");
		closelog();
		exit(EXIT_FAILURE);
	}	
	if ((conf_des = open(name_export,O_SYNC|O_WRONLY)) == -1){
		syslog(LOG_ERR,"error in open export file");
		closelog();
		exit(EXIT_FAILURE);
	}
	if (write(conf_des, led_gpio_str, strlen(led_gpio_str)) == -1){
		syslog(LOG_ERR,"error write in export file");
		closelog();
		exit(EXIT_FAILURE);
	}

	if (write(conf_des,but_gpio_str, strlen(but_gpio_str)) == -1){
		syslog(LOG_ERR,"error write in export file");
		int des;
		if ((des = open(name_unexport, O_SYNC|O_WRONLY)) != -1)
			write(des, led_gpio_str, strlen(led_gpio_str)); 
		close(des);
		closelog();
		exit(EXIT_FAILURE);
	}

	if (close(conf_des) == -1)
		syslog(LOG_ERR,"error close export file. errno=%d. %s",errno, strerror(errno));
	
	sleep(1);// wait for creat files GPIO
	//configure gpio direction for led
	char * ldir_val = "out";
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/direction", led_gpio_num) <= 0)
		errExit("snprintf");
	if ((conf_des = open(filename,O_SYNC|O_WRONLY)) == -1)
		errExit("open direction for led");
	if (write(conf_des, ldir_val, strlen(ldir_val)) == -1)
		errExit("write in file direction for led");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"close file diraction for led");

	//configure gpio direction for button
	char * bdir_val = "in";
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/direction", but_gpio_num) <= 0)
		errExit("snprintf");
	if ((conf_des = open(filename,O_SYNC|O_WRONLY)) == -1)
		errExit("open direction for button");
	if (write(conf_des, bdir_val, strlen(bdir_val)) == -1)
		errExit("write in file direction for button");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"close file diraction for button");

	//configure active_low parametr for button
	char * act_val = "1";
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/active_low", but_gpio_num) <= 0)
		errExit("snprintf");
	if ((conf_des = open(filename,O_SYNC|O_WRONLY)) == -1)
		errExit("open active_low for button");
	if (write(conf_des, act_val, strlen(act_val)) == -1)
		errExit("write in file active_low for button");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"close file active_low for button");

	//configure edge for button
	char * edge_val = "both";
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/edge", but_gpio_num) <= 0)
		errExit("snprintf");
	if ((conf_des = open(filename,O_SYNC|O_WRONLY)) == -1)
		errExit("open edge for button");
	if (write(conf_des, edge_val, strlen(edge_val)) == -1)
		errExit("write in file edge for button");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"close file edge for button");

	//open value for led
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/value", led_gpio_num) <= 0)
		errExit("snprintf");
	if ((led_des = open(filename,O_SYNC|O_WRONLY)) == -1)
		errExit("open value for led");
	//open value for button and do first read (need for edge)
	if (snprintf(filename,sizeof(filename),"/sys/class/gpio/gpio%d/value", but_gpio_num) <= 0)
		errExit("snprintf");
	if ((but_des = open(filename,O_RDONLY)) == -1)
		errExit("open value for button");
	if (read(but_des,&but_val, sizeof(but_val)) == -1)
		errExit("read from value for button");

	struct pollfd pollfd[1];
	pollfd[0].fd = but_des;
	pollfd[0].events = POLLPRI | POLLERR;
	pollfd[0].revents = 0;

	//main cycle
	while(term_flag == 0){

printf("Kyyyyyy\n");
		
		//waiting cick for button 
		if (poll(pollfd, 1, -1) == -1){
			if (term_flag == 1){
				break;
			}else{
				errExit("poll");
			}
		}
printf("Kuuuuuu\n");
		
		// handle the event
		if (lseek(but_des, 0, SEEK_SET) == -1)
			errExit("lseek file value for button");
	  	if (read(but_des, &but_val, sizeof(but_val)) != sizeof(but_val))
			errExit("read from file value for button");
		if (write(led_des, &but_val, sizeof(but_val)) == -1)
			errExit("write in file value for led");
	}//while(term_flag == 0)
	//deallocate recource
	if (close(led_des) == -1)
		syslog(LOG_ERR,"close file value for led");
	if (close(but_des) == -1)
		syslog(LOG_ERR,"close file value for button");
	//unexport gpio
	if ((conf_des=open(name_unexport,O_SYNC|O_WRONLY)) == -1){
		syslog(LOG_ERR,"error in open unexport file");
		closelog();
		exit(EXIT_FAILURE);
	}
	if (write(conf_des, led_gpio_str, strlen(led_gpio_str)) == -1)
		syslog(LOG_ERR,"error write in unexport file for led gpio");
	if (write(conf_des, but_gpio_str, strlen(but_gpio_str)) == -1)
		syslog(LOG_ERR,"error write in unexport file for button gpio");
	if (close(conf_des) == -1)
		syslog(LOG_ERR,"error close unexport file");

	closelog();
	exit(EXIT_SUCCESS);
}
