#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include <lmic.h>
#include <hal/hal.h>

//Function prototypes
uint8_t getFirstValueFromCSV(const char * fileName, bool *error = nullptr);

static const u1_t PROGMEM APPEUI[8] = {0x00, 0x6F, 0x6E, 0x69, 0x67, 0x61, 0x72, 0x64};
void os_getArtEui (u1_t* buf) {  memcpy_P(buf, APPEUI, 8);}
static const u1_t PROGMEM DEVEUI[8] = {0x31, 0x6F, 0x6E, 0x69, 0x67, 0x61, 0x72, 0x64};
void os_getDevEui (u1_t* buf) {  memcpy_P(buf, DEVEUI, 8);}
static const u1_t PROGMEM APPKEY[16] = {0x5F, 0xAC, 0x47, 0x0F, 0x83, 0x6C, 0x53, 0xF6, 0xD5, 0xEE, 0x1A, 0x6F, 0xF4, 0x78, 0xC1, 0x83};
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}

static uint8_t mydata[] = "1";
static osjob_t sendjob;
const unsigned TX_INTERVAL = 2;

//Flag for Ctrl-C
volatile sig_atomic_t force_exit = 0;

// Dragino Raspberry PI hat (no onboard led)
// see https://github.com/dragino/Lora
#define RF_CS_PIN  RPI_V2_GPIO_P1_22 // Slave Select on GPIO25 so P1 connector pin #22
#define RF_IRQ_PIN RPI_V2_GPIO_P1_07 // IRQ on GPIO4 so P1 connector pin #7
#define RF_RST_PIN RPI_V2_GPIO_P1_11 // Reset on GPIO17 so P1 connector pin #11

// Pin mapping
const lmic_pinmap lmic_pins = {
  .nss  = RF_CS_PIN,
  .rxtx = LMIC_UNUSED_PIN,
  .rst  = RF_RST_PIN,
  .dio  = {LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
};

#ifndef RF_LED_PIN
#define RF_LED_PIN NOT_A_PIN
#endif

void do_send(osjob_t* j) {
  char strTime[16];
  uint8_t readValue(0);
  bool error(false);

  getSystemTime(strTime , sizeof(strTime));
  printf("%s: ", strTime);
  readValue = getFirstValueFromCSV("error_state.csv", &error);

  printf("We read from file the value : %u, got error ? %s\n", readValue, error ? "yes" : "no");

  // Save 
  mydata[0] = readValue;


  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    printf("OP_TXRXPEND, not sending\n");
  } else {
    digitalWrite(RF_LED_PIN, HIGH);
    // Prepare upstream data transmission at the next possible time.
    printf("mydata to be sent is : %u \n", *mydata);


    LMIC_setTxData2(38, mydata, sizeof(mydata) - 1, 0);
    printf("Packet queued\n");
  }
  // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
  char strTime[16];
  getSystemTime(strTime , sizeof(strTime));
  printf("%s: ", strTime);

  switch (ev) {
    case EV_SCAN_TIMEOUT:
      printf("EV_SCAN_TIMEOUT\n");
      break;
    case EV_BEACON_FOUND:
      printf("EV_BEACON_FOUND\n");
      break;
    case EV_BEACON_MISSED:
      printf("EV_BEACON_MISSED\n");
      break;
    case EV_BEACON_TRACKED:
      printf("EV_BEACON_TRACKED\n");
      break;
    case EV_JOINING:
      printf("EV_JOINING\n");
      break;
    case EV_JOINED:
      printf("EV_JOINED\n");
      digitalWrite(RF_LED_PIN, LOW);
      // Disable link check validation
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      printf("EV_RFU1\n");
      break;
    case EV_JOIN_FAILED:
      printf("EV_JOIN_FAILED\n");
      break;
    case EV_REJOIN_FAILED:
      printf("EV_REJOIN_FAILED\n");
      break;
    case EV_TXCOMPLETE:
      printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
      if (LMIC.txrxFlags & TXRX_ACK)
        printf("%s Received ack\n", strTime);
      if (LMIC.dataLen) {
        printf("%s Received %d bytes of payload\n", strTime, LMIC.dataLen);
      }
      digitalWrite(RF_LED_PIN, LOW);
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      printf("EV_LOST_TSYNC\n");
      break;
    case EV_RESET:
      printf("EV_RESET\n");
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      printf("EV_RXCOMPLETE\n");
      break;
    case EV_LINK_DEAD:
      printf("EV_LINK_DEAD\n");
      break;
    case EV_LINK_ALIVE:
      printf("EV_LINK_ALIVE\n");
      break;
    default:
      printf("Unknown event\n");
      break;
  }
}

/* ======================================================================
  Function: sig_handler
  Purpose : Intercept CTRL-C keyboard to close application
  Input   : signal received
  ====================================================================== */
void sig_handler(int sig)
{
  printf("\nBreak received, exiting!\n");
  force_exit = true;
}

/* ======================================================================
  Function: getFirstValueFromCSV
  Purpose : Extract the first value of CSV file to send by LoRa
  Input   : test.csv
  Output  : first line of the csv file
  Comments: -
  ====================================================================== */
uint8_t getFirstValueFromCSV(const char * fileName, bool *error)
{
  unsigned int value(0);
  char buffer[10] = "";
  FILE *file = fopen(fileName, "r");
  if (!file)
  {
    printf("Failed to open file : %s\n", fileName);
    if (error) *error = true;
    return 0;
  }

  fscanf(file, "%u", &value);
  //fgets(buffer, sizeof(buffer), file);
  //value = strtoul(buffer,NULL,10);

  if (fclose(file) != 0)
    printf("Failed to close file : %s\n", fileName);

  return value;
}

/* ======================================================================
  Function: main
  Purpose : not sure ;)
  Input   : command line parameters
  Output  : -
  Comments: -
  ====================================================================== */
int main(void)
{
  // caught CTRL-C to do clean-up
  signal(SIGINT, sig_handler);

  printf("%s Starting\n", __BASEFILE__);

  // Init GPIO bcm
  if (!bcm2835_init()) {
    fprintf( stderr, "bcm2835_init() Failed\n\n" );
    return 1;
  }

  // Show board config
  printConfig(RF_LED_PIN);
  printKeys();

  // Light off on board LED
  pinMode(RF_LED_PIN, OUTPUT);
  digitalWrite(RF_LED_PIN, HIGH);

  // LMIC init
  os_init();
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();

  // Start job (sending automatically starts OTAA too)
  do_send(&sendjob);

  while (!force_exit) {
    os_runloop_once();

    // We're on a multitasking OS let some time for others
    // Without this one CPU is 99% and with this one just 3%
    // On a Raspberry PI 3
    usleep(1000);
  }

  // We're here because we need to exit, do it clean

  // Light off on board LED
  digitalWrite(RF_LED_PIN, LOW);

  // module CS line High
  digitalWrite(lmic_pins.nss, HIGH);
  printf( "\n%s, done my job!\n", __BASEFILE__ );
  bcm2835_close();
  return 0;
}
