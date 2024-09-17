/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "portmacro.h"
#include "string.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"


//defing the UART_ports for ESP32
#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

//receive buffer
static const int RX_BUF_SIZE = 512;
uint8_t RxData[1024];

//transmit handle for resume and suspending task
TaskHandle_t txtaskhandle;

//variable declaration for measuring data per second transmit ,receive speed
static TimerHandle_t byte_rate_time;
static volatile uint32_t bytes_transmitted = 0;
static volatile uint32_t bytes_received = 0;


//UART initialization
void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 2400,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // RX buffer is only used, Tx is not buffered
    uart_driver_install(UART_NUM_2, RX_BUF_SIZE*2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void SPIFFS_init(void)
{
	static const char *TAG = "FILESYSTEM";//log for filesystem
	
		//creating partition structure
	esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

//passing partitions structure into SPIFFS register
esp_err_t ret = esp_vfs_spiffs_register(&conf);

//indicates error in the flash memory allocation
 if (ret != ESP_OK) 
 {
	 ESP_LOGE(TAG, "ERROR IN FILESYSTEM with %s	",esp_err_to_name(ret));
	 return;
 }
 //gives info about how much of space is used in SPIFFS flash
   size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    
    //reading exisitiing file in the flaash
    ESP_LOGI(TAG, "reading file");
    FILE* f = fopen("/spiffs/TEST.txt", "r");//'r' for read operation
    
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");//logging error condition
        return;
        }
        //printing the existing content in the flash
	 char line[64];//array to hold the contents in flasj
    fgets(line, sizeof(line), f);//data from the file to array
    fclose(f);//closing the file for future use
    ESP_LOGI(TAG, "Read from file: '%s'", line);//logging the contents of the file
	
}

//Transmission task
static void tx_task(void *arg)
{
	//terminal tags for displaying commands of TX
	 static const char *TX_TASK_TAG = "TX_TASK";
	 esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
	 
   // Buffer to hold chunks of data
        char buffer[128]; // Chunk size for transmission
        size_t bytesRead = 0;
        
    while (1) {
		
		//opening the file in SPIFFS for sending over UART transmit
		FILE *f = fopen("/spiffs/TEST.txt", "r");
        if (f == NULL) {
            ESP_LOGE(TX_TASK_TAG, "Failed to open file for reading");
            return;
            }else ESP_LOGE(TX_TASK_TAG, "Opened file from flash");
            
            
      //reading data from the file and checking reception of data      
    	while ((bytesRead = fread(buffer, 1, sizeof(buffer), f)) > 0) 
    	{
        const int txBytes = uart_write_bytes(UART_NUM_2, buffer, bytesRead);//transmitting data in buffer
        
          bytes_transmitted += txBytes; //updating for per second count
         //transmission longs 
          ESP_LOGI(TX_TASK_TAG, "Wrote %d bytes", txBytes);
        //  vTaskDelay(2000/portTICK_PERIOD_MS);
        }
        
    /* clearing the file in the flash post usage    
       f = fopen("/spiffs/TEST.txt", "w"); // Open file in write mode to clear it
        if (f != NULL) {
          fclose(f); // Closing immediately clears the file
           ESP_LOGI(TX_TASK_TAG, "File cleared after transmission");
       }*/
       vTaskSuspend(txtaskhandle);//resumes when there is an incoming data in rXbuffer
    }
    
}


//reception task
static void rx_task(void *arg)
{
	//terminal tags for displaying commands of TX
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
 
    while (1) {
		
		//receiving data in the buffer
        const int rxBytes = uart_read_bytes(UART_NUM_2, RxData, RX_BUF_SIZE, 200 / portTICK_PERIOD_MS);
        //if there is received data
        if (rxBytes > 0) {
            RxData[rxBytes] = 0;//ending the last element in the buffer end line character '0'
            bytes_received += rxBytes;//updating for per second count
           
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes", rxBytes);//receptionlogs
            
            
            //opening file system to update data
             FILE *f = fopen("/spiffs/TEST.txt", "a"); //type is "a" for appending data
            if (f == NULL) {
                ESP_LOGE(RX_TASK_TAG, "Failed to open file for writing");//file not found condition
            } else {
                fwrite(RxData, sizeof(char), rxBytes, f);//writing received data in flash 
                fclose(f); //closing the file
                   ESP_LOGI(RX_TASK_TAG, "Data written to SPIFFS");//file write logs as a part of Recepion
            }
            vTaskResume(txtaskhandle);//resuming the tx task post reception of data
        }
         
    }
}


//data per second counter 
void report_timer_callback(TimerHandle_t xTimer)
{
//static variable to hold data out of function call	
    static uint32_t last_bytes_transmitted = 0;
    static uint32_t last_bytes_received = 0;
    
    
    //updated transist and receive rates 
    uint32_t transmitted_rate = bytes_transmitted - last_bytes_transmitted;
    uint32_t received_rate = bytes_received - last_bytes_received;
    
    //logs of data per sec
ESP_LOGI("DATA_RATE", "Transmission_rate/sec: %" PRIu32 ", Reception_rate/sec: %" PRIu32, transmitted_rate, received_rate);
    
    //updating count to get per second count the next rotation
    last_bytes_transmitted = bytes_transmitted;
    last_bytes_received = bytes_received;
}


/*
//idle state file system delete
static void idle_task(void *arg)
{
    while (1) {
        if ((eTaskGetState(txtaskhandle) == eSuspended)&&(!file_flag))
        {
            // Check if destination file exists before deleting
    struct stat st;
    if (stat("/spiffs/TEST.txt", &st) == 0) {
        // Delete it if it exists
        unlink("/spiffs/TEST.txt");
        file_flag = 1;//changing flag
    }else  ESP_LOGI("IDLE_TASK","file deleted");
                      
            }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
}
*/


void app_main(void)
{
	
	SPIFFS_init();//flash partiton creation and checking

    init();//UART init
 
 //Receive task with high priorty than TX task
    xTaskCreate(rx_task, "uart_rx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, &txtaskhandle);
     // xTaskCreate(idle_task, "idle_task", 1024, NULL, tskIDLE_PRIORITY, NULL);//idle task.when no tx or RX happens
     
   //task to check data per second
   byte_rate_time = xTimerCreate("ReportTimer", pdMS_TO_TICKS(1000), pdTRUE, 0, report_timer_callback);
    if (byte_rate_time != NULL) {
        xTimerStart(byte_rate_time, 0);
    } else {
        ESP_LOGE("APP_MAIN", "Failed to create the report timer");
    }
}