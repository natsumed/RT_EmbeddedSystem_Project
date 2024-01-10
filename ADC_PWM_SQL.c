/* README
This code is provided by Mohamed_FATHALLAH and Maissa_BOUZIRI 3AGE1
* In this code we tried to implement the values of the ADC from the BBB
to the Data_Base called ADC_Table using the sqlite3
* Also this code provides the values of the duty_cycle of the BBB as 
well as the values of pwm that we can visualize
* The commented parts are for the LED that turns on using the GPIOs
* In this code too we tried to utilize the notion of the BBB server by
using some functions that creates server
*/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <limits.h>
#include <signal.h> 
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h> 
#include <time.h>  
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sqlite3.h>

void init_timer(void);
//void toggle_GPIO(void);
//void init_GPIO(void);
int rt_init(void);
void init_pwm(void);
char kbhit(void);
u_int32_t T=1000000, c=0,duty=0;
int db_insert();
unsigned int db_countrow();
struct meter_param* db_read();
int db_create_table();
sqlite3 *db_open();

void *thread_led_func(void *);
void *thread_adc_func(void *);
void *thread_pwm_func(void *);

	int ret;   
	pthread_attr_t attr;                  
   timer_t timerid;
   struct sigevent sev;
   struct itimerspec trigger;
   //struct gpiod_line *lineRed;// Red LED
  // struct gpiod_chip *chip;

	pthread_cond_t timer;
	pthread_cond_t adc;
	pthread_cond_t pwm;
	pthread_mutex_t lock;
//First we create the data base
sqlite3 *DataBase; 

struct meter_param {
	//structure to hold energy meter parameters
	u_int32_t id;
	u_int32_t adc;
	time_t date;
};
time_t get_date_time(void) {
    return time(NULL); 
}

struct server_param{
	char   ip_address[16];
	u_int16_t port;
	u_int8_t max_clients;
};
struct meter_param m1;
//*******************************THREAD_SERVER*****************************************************************************//

void *server_func(void *s_param) {
    printf("IP_SERVER = %s\n", ((struct server_param *)s_param)->ip_address);
    printf("SERVEUR : Ports number = %u\n", ((struct server_param *)s_param)->port);
    printf("SERVEUR : MAX Client number = %u\n", ((struct server_param *)s_param)->max_clients);

    u_int32_t res, client_len, server_len;
    struct sockaddr_in server_adr, client_adr;

    server_adr.sin_family = AF_INET;
    server_adr.sin_addr.s_addr = inet_addr(((struct server_param *)s_param)->ip_address);
    server_adr.sin_port = ((struct server_param *)s_param)->port;

    server_len = sizeof(server_adr);
    client_len = sizeof(client_adr);

    // Here we will create a socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) {
        perror("SERVEUR : Socket OPEN DENIED : ");
        goto cl_bas;
    }
    
    
    
    printf("SERVEUR : Socket CREATED SUCCESSFULLY\n");

    uint8_t on = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    res = bind(sfd, (struct sockaddr *)&server_adr, server_len);
    if (res < 0) {
        perror("SERVEUR : READ SOCKET DENIED : ");
        goto cl_bas;
    }
    printf("SERVEUR : Socket LINKED SUCCESSFULLY\n");

    // Listening to a client
    res = listen(sfd, ((struct server_param *)s_param)->max_clients);
    if (res < 0) {
        perror("SERVEUR : READ CLIENT IMPOSSIBLE : ");
        goto cl_bas;
    }

    while (1) {
        printf("SERVEUR : SOCKET LISTEN\n");

        int cfd = accept(sfd, (struct sockaddr *)&client_adr, &client_len);
        if (cfd < 0) {
            perror("SERVEUR : ACCEPT CLIENT DENIED: ");
            goto cl_bas;
        }

        printf("CLIENT CONNECTED\n");
        printf("CLIENT IP ADDRESS: %s\n", inet_ntoa(client_adr.sin_addr));


        close(cfd);
    }

    close(sfd);
    pthread_exit("SERVEUR : THREAD FINISHED SUCCESSFULLY!\n");

cl_bas:
    close(sfd);
    pthread_exit("SERVEUR : SERVER DIAGNOSIS FAILED !\n");
}

void Tstimer_thread(union sigval sv) 
{        
        puts("100ms elapsed.");
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&timer);
		pthread_mutex_unlock(&lock);
        timer_settime(timerid, 0, &trigger, NULL);
}
/* // We used this function for the LED
void *thread_led_func(void *v)
{
	for (;;)
	{
		//attente msg de la part de timer
		pthread_mutex_lock(&lock);
		pthread_cond_wait(&blink_led,&lock);
		pthread_mutex_unlock(&lock);
		//--------------------toggle GPIO
       toggle_GPIO();
		//------------------------------
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&adc);
		pthread_mutex_unlock(&lock);
	}
}*/
void *thread_adc_func(void *v) {
	int f;
	char a[5];
	while(1)
	{
		pthread_mutex_lock(&lock);
		pthread_cond_wait(&timer,&lock);
		pthread_mutex_unlock(&lock);
		//This is the PATH of the ADC value
		f=open("/sys/bus/iio/devices/iio:device0/in_voltage5_raw",O_RDONLY);
		read(f,&a,sizeof(a));
		c=atoi(a);
		printf( "the value of adc is %d \n",c);
		close(f);
		
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&adc);
		pthread_mutex_unlock(&lock);
	}

	return NULL;
}

void *thread_pwm_func(void *v) {
	int fd;
	char str[10];
	while(1)
	{

	pthread_mutex_lock(&lock);
	pthread_cond_wait(&pwm,&lock);
	pthread_mutex_unlock(&lock);
	//THis is the PATH for the duty_cycle value
	fd=open("/sys/class/pwm/pwmchip5/pwm1/duty_cycle",O_WRONLY);
	duty= (2 * T / 10 ) + (8 * T / 10 * c >> 12 );
	sprintf(str, "%d",duty);
	printf("%s \n" , str);
	write(fd,str,sizeof(str));
	close(fd);	
	
	}
	return NULL;
	
}

int main(void) 
{
	//pthread_t *thread_led;
	pthread_t *thread_adc;
	pthread_t *thread_pwm;
	    struct server_param myserver = {"127.0.0.0",1880,5};
		sqlite3 *db_open();
	    db_create_table(DataBase, "ADC_Table");
		init_timer();
        init_pwm();
        DataBase = db_open("ADC_Table"); 

    if (DataBase == NULL) {
        printf("Error while opening the Data_Base.\n");
        return EXIT_FAILURE;
    }

    
    if (db_create_table(DataBase, "ADC_Table") != 0) {
        printf("Error while creating the Data_Base.\n");
        sqlite3_close(DataBase);  
        return EXIT_FAILURE;
    }
     u_int32_t row_to_read = 1; 
    struct meter_param *data = db_read(DataBase, "ADC_Table", row_to_read); 
    if (data) {
        printf("ID : %u\n", data->id);
        printf("ADC : %u\n", data->adc);
        printf("date : %u\n", data->adc);
        
       
        free(data);
    }

		//init_timer();
		//init_GPIO();
		//init_pwm();
		if(rt_init()) exit(0);
        /* Wait 10 seconds under the main thread. When the timer expires,
         * a message will be printed to the standard output by the newly
         * created notification thread.
         */
         
		//thread_led= (pthread_t *) malloc(sizeof(pthread_t));
		//pthread_create(thread_led, &attr, thread_led_func, NULL);
		thread_adc= (pthread_t *) malloc(sizeof(pthread_t));
		pthread_create(thread_adc, NULL, thread_adc_func, NULL);
		thread_pwm= (pthread_t *) malloc(sizeof(pthread_t));
		pthread_create(thread_pwm, &attr, thread_pwm_func, NULL);
		
        while (kbhit()!='q');

        /* Delete (destroy) the timer */
        timer_delete(timerid);
       // gpiod_line_release(lineRed);
		//gpiod_chip_close(chip);
		pthread_mutex_destroy(&lock);
		pthread_cond_destroy(&pwm);
       sqlite3_close(DataBase);

        return EXIT_SUCCESS;

}
/*
void init_GPIO(void)
{
    const char *chipname = "gpiochip2";
	struct gpiod_chip *chip;
	
	// Open GPIO chip
	chip = gpiod_chip_open_by_name(chipname);
	// Open GPIO lines
	lineRed = gpiod_chip_get_line(chip, 4);

	// Open LED lines for output
	gpiod_line_request_output(lineRed, "gpio_timer", 0);	
}

void toggle_GPIO(void)
{
	gpiod_line_set_value(lineRed, ! gpiod_line_get_value(lineRed));
}
*/
int rt_init(void)
{
     
        struct sched_param param;
     
        /* Lock memory */
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }
 
        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) printf("init pthread attributes failed\n");
 
        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) printf("pthread setstacksize failed\n");
           
        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) printf("pthread setschedpolicy failed\n");
              
        param.sched_priority = 80;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) printf("pthread setschedparam failed\n");
                
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) printf("pthread setinheritsched failed\n");
        
        return ret;
        
}
void init_pwm(void)
{
	char str[10];
	int fd=open("/sys/devices/platform/ocp/ocp:P9_16_pinmux/state",O_WRONLY);
	write(fd,"pwm",sizeof("pwm"));
	close(fd);
	
	fd=open("/sys/class/pwm/pwmchip5/pwm1/period",O_WRONLY);
	sprintf(str, "%d",T);
	printf("%s",str);
	write(fd,str,sizeof(str));
	close(fd);	
	
	fd=open("/sys/class/pwm/pwmchip5/pwm1/duty_cycle",O_WRONLY);
	sprintf(str, "%d",(T*2)/10);
	write(fd,str,sizeof(str));
	close(fd);	
	
	fd=open("/sys/class/pwm/pwmchip5/pwm1/enable",O_WRONLY);
	write(fd,"1",sizeof("1"));
	close(fd);	
}
void init_timer(void)
{
        /* Set all `sev` and `trigger` memory to 0 */
        memset(&sev, 0, sizeof(struct sigevent));
        memset(&trigger, 0, sizeof(struct itimerspec));

        /* 
         * Set the notification method as SIGEV_THREAD:
         *
         * Upon timer expiration, `sigev_notify_function` (thread_handler()),
         * will be invoked as if it were the start function of a new thread.
         *
         */
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = &Tstimer_thread;
        
        /* Create the timer. In this example, CLOCK_REALTIME is used as the
         * clock, meaning that we're using a system-wide real-time clock for
         * this timer.
         */
        timer_create(CLOCK_REALTIME, &sev, &timerid);

        /* Timer expiration will occur withing 100ms seconds after being armed
         * by timer_settime().
         */
        trigger.it_value.tv_sec = 0;
        trigger.it_value.tv_nsec = 100000000;  // 100ms

        /* Arm the timer. No flags are set and no old_value will be retrieved.
         */
        timer_settime(timerid, 0, &trigger, NULL);
        
}



//The functions that will help us create a DATABASE

sqlite3 *db_open(char *dbname)
{
	sqlite3 *db;
	char dbfile[25];
	int rc;
	
	sprintf(dbfile,"%s.db",dbname);
	
	rc = sqlite3_open(dbfile,&db);
	if(rc){
	     printf("XXXX : Can't open database: %s\n", sqlite3_errmsg(db));
	     return NULL;
	}
    	
	printf("@=> Database \"%s\" created/opened successfully\n",dbfile);
	return db;
}

int db_create_table(sqlite3 *db,char *table_name)
{
	char *zErrMsg = NULL,sql[300];
	int rc;

	
   	sprintf(sql,"CREATE TABLE %s (ID INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT," \
   	"ADC INTEGER,"\
         "Time TEXT); " ,table_name);

	
	rc = sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK){
		printf("XXXX : SQL error (Table = %s) - %s\n",table_name,zErrMsg);
	    	sqlite3_free(zErrMsg);
		return -1;
	}

	printf("@=> Table \"%s\" created successfully!!\n",table_name);
	return 0;
}
//Insertion in a DataBase
int db_insert(sqlite3 *db,char *table_name,struct meter_param *m1)
{
	char *zErrMsg = NULL,sql[300];
	int rc;

	sprintf(sql, "INSERT INTO %s (ADC,Time) " "VALUES (%d,%ld);", table_name, m1->adc,m1->date);
	
	rc = sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		return -1;
	}

	printf("@=> Database values inserted successfully!!\n");
	return 0;
}
//Read the values in DataBase
struct meter_param* db_read(sqlite3 *db,char *table_name,unsigned int row)
{
	struct meter_param *x= (struct meter_param*)malloc(sizeof(struct meter_param));
	char sql[100],*zErrMsg = NULL;
	int rc;
	sprintf(sql,"SELECT * FROM %s",table_name);
	rc = sqlite3_exec(db, sql, NULL, &x, &zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		free(x);
		return NULL;
	}

	printf("@=> Values read successfully!!\n");
	return x; 
}


//Line numbers in DataBase
unsigned int db_countrow(sqlite3 *db,char* table_name)
{
	char sql[100],*zErrMsg = NULL;
	int rc;
	sprintf(sql,"SELECT count(*) FROM %s",table_name);
	rc = sqlite3_exec(db, sql, NULL,NULL, &zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		return 0; 
	}

	
	return 0;

}


