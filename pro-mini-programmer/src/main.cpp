// ************************************************************************
//  CCDebug communicate with TI CC2510 MUC in debug mode
//  Derived from UART_TEMPLATE for ATMega328P  / Arduino UNO
//
//  Some Uno to AVR conversion notes:
//  UNO Pin 0-7   = PORTD PD0-PD7     (Rx & Tx = Pins 0 & 1 = PD0 & PD1)
//  UNO Pin 8-13  = PORTB PB0-PB5     (PIN13/PB5 has on-board LED)
//  UNO Pin A0-A5 = PORTC PC0=PC5     (Analog Pins also work as PIN# 14-19)
//
// *********************** CC2510 Notes  **********************************
//  The CC2510 debug interface uses an SPI-like two-wire interface consisting
//  of the bi-directional Debug Data(P2_1) and Debug Clock(P2_2) input pin.
//  Data is driven on the bi-directional Debug Data pin at the positive edge
//  of Debug Clock and data is sampled on the negative edge of this clock.
//  Debug commands are sent by an external host and consist of 1 to 4 output
//  bytes from the host and an optional input byte read by the host.
// RESET (RS)    = Pin#5/PD5  -> RESET_N (CC2510 pin 31)
// Data Clock(DC)= Pin4/PD4   -> P2_2    (CC2510 Pin 16)
// Data I/O(DD)  = Pin3/PD3   -> P2_1    (CC2510 Pin 15)
// ************************************************************************
// This code is targeted for an Arduino UNO (ATMega325P)
// You must modify it for other Arduino boards/devices
// ************************************************************************
#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#define CCDB_PORT PORTD
#define CCDB_PIN PIND
#define CCDB_DDR DDRD
#define DD_PIN PD3
#define DC_PIN PD4
#define RS_PIN PD5

// CCDebug instruction set for CC2510
#define INSTR_VERSION 1;
#define I_HALT 0x48
#define I_RESUME 0x4C
#define I_RD_CONFIG 0x24
#define I_WR_CONFIG 0x1D
#define I_DEBUG_INSTR_1 0x55
#define I_DEBUG_INSTR_2 0x56
#define I_DEBUG_INSTR_3 0x57
#define I_GET_CHIP_ID 0x68
#define I_GET_PC 0x28
#define I_READ_STATUS 0x34
#define I_STEP_INSTR 0x5C
#define I_CHIP_ERASE 0x14
#define CHIP_ID 0x81

#define RW_DELAY_US 1

// ccdebug Function prototypes
void flashLED(uint8_t count);
void show_menu(void);
void process_menu_cmd(uint8_t cmd);
uint16_t getAddress(uint8_t bEcho);
uint16_t getDataBytes(uint8_t *buf, uint16_t count);
void dumpDataBuf(uint16_t addr);
uint8_t get_ihex_rec(uint8_t *buf, uint16_t *addr, uint16_t max);

// ************** CC DEBUGGER Functions ************************
void cc_init(void);
void cc_enter(void);
void cc_send(uint8_t data);
uint8_t cc_readyWait(void);
uint8_t cc_read(void);
uint16_t cc_readID(void);
uint8_t cc_readStatus(void);
uint8_t cc_eraseChip(void);
uint8_t cc_exec1(uint8_t cmd1);
uint8_t cc_exec2(uint8_t cmd1, uint8_t cmd2);
uint8_t cc_exec3(uint8_t cmd1, uint8_t cmd2, uint8_t cmd3);
void cc_read16f(uint16_t address, uint8_t bank, uint8_t *buf);
void cc_read16x(uint16_t addr, uint8_t *buf);
void cc_writeXD(uint16_t addr, uint8_t *buf, uint16_t size);
void cc_pageErase(uint16_t addr);
void cc_writeBuf(uint16_t addr, uint8_t *buf, uint16_t size);

// ************************************************************************
//  Basic UART functions for terminal display
// ************************************************************************
uint8_t UART_get_chr(void);
void UART_put_chr(uint8_t c);
void UART_put_str(uint8_t *str);
void UART_put_strP(char *str);
void UART_get_str(uint8_t *str, int max, uint8_t bEcho);
void UART_PutHexByte(uint8_t byte);
uint8_t UART_get_hex_byte(uint8_t echo);
uint8_t AsciiToHexVal(uint8_t b);
uint8_t HexStrToByte(uint8_t *buf);

void putcharBB(uint8_t c);
uint8_t getcharBB(void);

// Global variables
uint8_t errorNo, bank, addrH, addrL, do_echo = 1, debugMode = 0;
uint8_t dataBuf[258];
uint16_t address;
uint8_t msg[80], k;
uint8_t led;

// ********* Flash Write 16 bytes Routine ****************
// ** 8051 assembly code to write to flash memory ********
uint8_t flashWrite16[] = {
    0x75, 0xAD, 0x00, // MOV FDAARH, #imm
    0x75, 0xAC, 0x00, // MOV FDAARL, #imm
    0x90, 0xF0, 0x00, //#MOV DPTR, #0F000H;
    0x75, 0xAE, 0x02, //#MOV FLC, #02H;  // WRITE
    0x7D, 0x08,       // MOV R5, #imm;  //8 words = 16bytes
    0xE0,             //#writeWordLoop: MOVX A, @DPTR;
    0xF5, 0xAF,       //#MOV FWDATA, A;
    0xA3,             //#INC DPTR;
    0xE0,             //MOVX A, @DPTR;
    0xF5, 0xAF,       //#MOV FWDATA, A;
    0xA3,             //#INC DPTR;
    0xE5, 0xAE,       //#writeWaitLoop: MOV A, FLC;
    0x20, 0xE6, 0xFB, //#JB ACC_SWBSY, writeWaitLoop;
    0xDD, 0xF1,       //#DJNZ R5, writeWordLoop;
    0xA5};            //#DB 0xA5; fake a breakpoint

// ************************************************************************
//
//
// ************************************************************************
void setup()
{
  int i;

  // Enable pins for use
  DDRB = 0x20;   // UNO LED on PIN#13  (PORTB BIT5 * PB5)
  PORTB &= 0xDF; // Start LED OFF

  flashLED(5);

  led = 1;

  // Initilize the UART for serial terminal display @9600 baud
  Serial.begin(9600);

  cc_init();

  // Send a message to the terminal
  UART_put_strP(PSTR("\r\n UNO CC Debug Ready \r\n"));
}

void loop()
{
  if (Serial.available()) // Any UART dara ready?
  {
    process_menu_cmd(UART_get_chr()); // Process it
  }

  static uint32_t change;
  uint32_t now = millis();
  if (now + 500 > change)
  {
    change = now;
    if (led)
    {
      PORTB &= 0xDF; // LED OFF
      led = 0;
    }
    else
    {
      PORTB |= 0x20; // LED ON
      led = 1;
    }
  }
}

// ***********************************************************
// show_menu()
//  Display the menu choices on terminal
// ***********************************************************
void show_menu(void)
{
  UART_put_strP(PSTR("\r\n\n  **** UART_TEMPLATE Main Menu ****\r\n"));
  UART_put_strP(PSTR("1. Read Chip ID             R. Toggel RESET \r\n"));
  UART_put_strP(PSTR("d. Read 16 flash bytes      D. Read 256 flash bytes\r\n"));
  UART_put_strP(PSTR("x. Read 16 XDATA bytes      X. Read 256 XDATA bytes\r\n"));
  UART_put_strP(PSTR("w. Write 16 XDATA bytes     W. Write 256 XDATA bytes\r\n"));
  UART_put_strP(PSTR("e. Erase Chip               :  Get iHEX Record\r\n"));
  UART_put_strP(PSTR("p. Page Erase               U. UART Test\r\n"));
  UART_put_strP(PSTR("2. Toggel Board LEDs (P1)   F. Flash The LED on PB5/PIN13 \r\n"));
  UART_put_strP(PSTR("M  Re-Display Menu          @  Attention (reply='$$$')\r\n"));
}

// ***********************************************************
// process_menu_cmd(uint8_t cmd)
//  execute various menu commands
// ***********************************************************
void process_menu_cmd(uint8_t cmd)
{
  int x, count;
  uint8_t val;
  uint16_t id, data_len;

  switch (cmd)
  {
  case 'M': // *** Re-Display the menu ***
  case 'm':
    show_menu();
    break;

  case '1':
    UART_put_strP(PSTR("\r\n Chip ID:"));
    cc_enter();
    id = cc_readID();
    UART_PutHexByte(id >> 8);
    UART_PutHexByte(id & 0xFF);
    UART_put_strP(PSTR("\r\n"));
    debugMode = 1;
    break;
  case '2': // Using debug mode issue commands to toggel 4 bits on P1
    cc_enter();
    cc_exec3(0x75, 0xFE, 0x0F); // MOV DIRP1, 0x0F;
    cc_exec3(0x75, 0x90, 0x0C); // MOV P1, 0x0F;
    UART_put_strP(PSTR("\r\n LEDs ON"));
    _delay_ms(1000);
    cc_exec3(0x75, 0x90, 0x03); // MOV P1, 0x00;
    UART_put_strP(PSTR(" LEDs OFF"));
    debugMode = 1;
    break;

  case 'R':
    CCDB_PORT &= 0xE7;         // PD3 & PD4 = LOW (NoPullups)
    CCDB_DDR &= 0xE7;          // PD3 & PD4 = INPUT
    CCDB_PORT &= ~_BV(RS_PIN); //Reset = LOW
    _delay_ms(10);             // 1/10 second
    CCDB_PORT |= _BV(RS_PIN);  //Reset = HIGH

    UART_put_strP(PSTR(" Chip RESET\r\n"));

    do_echo = 1; // turn echo back on if it's off
    debugMode = 0;
    break;

  case 'd': // Dump 16 bytes of flash memory
    UART_put_strP(PSTR("\r\n Read 16 Flash"));
    address = getAddress(1);
    bank = 0;
    cc_enter();
    cc_read16f(address, bank, dataBuf);
    dumpDataBuf(address);
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case 'D': // Dump 256 bytes of flash memory
    UART_put_strP(PSTR("\r\n Read 256 Flash"));
    address = getAddress(1);
    bank = 0;
    cc_enter();
    for (x = 0; x < 16; x++)
    {
      cc_read16f(address, bank, dataBuf);
      dumpDataBuf(address);
      address += 16;
    }
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case 'x': // Dump 16 bytes of XDATA memory
    UART_put_strP(PSTR("\r\n Read 16 XDATA"));
    address = getAddress(1);
    cc_enter();
    cc_read16x(address, dataBuf);
    dumpDataBuf(address);
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case 'X': // Dump 256 bytes of XDATA memory
    UART_put_strP(PSTR("\r\n Read 256 XDATA"));
    address = getAddress(1);
    cc_enter();
    for (x = 0; x < 16; x++)
    {
      cc_read16x(address, dataBuf);
      dumpDataBuf(address);
      address += 16;
    }
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case 'w': // Write 16 bytes of XDATA memory
    UART_put_strP(PSTR("\r\n Write 16 XDATA"));
    address = getAddress(1);

    memset(dataBuf, 0, 16);
    UART_put_strP(PSTR("\r\n Enter Data: "));
    getDataBytes(dataBuf, 16);

    cc_enter();
    cc_writeXD(address, dataBuf, 16);
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case 'p': // Erase flash page
    UART_put_strP(PSTR("\r\n Flash Page Erase - Enter Address: "));
    address = getAddress(1);
    cc_enter();
    cc_pageErase(address);
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;

  case 'f': // write 16 bytes of flash memory
    UART_put_strP(PSTR("\r\n Write 16 FLASH"));
    address = getAddress(1);
    UART_put_strP(PSTR("\r\n Enter Data: "));

    memset(dataBuf, 0xFF, sizeof(dataBuf));
    x = 0;
    count = 16;
    if (address & 0x0001) // Start address MUST be even (on a word boundary)
    {                     // If not adjust by making 1st byte = 0xFF
                          // NOTE:(0xFF will have no effect on an already programed byte)
      address--;          // Back up start address 1 byte
      x = 1;              // Advance buffer index
      count++;            // Increase size by 1
    }

    getDataBytes(&dataBuf[x], count);

    cc_enter();
    cc_writeBuf(address, dataBuf, count);
    debugMode = 1;
    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;

  case 'e': // Erase the entire chip (requires 'Y' response)
    UART_put_strP(PSTR("\r\n ERASE CHIP ?:"));
    val = UART_get_chr();
    UART_put_chr(val);
    if (val == 'Y')
    {
      cc_enter();
      if (cc_eraseChip())
        UART_put_strP(PSTR("\r\n Done\r\n"));
      else
        UART_put_strP(PSTR(" ERROR\r\n"));
    }
    debugMode = 1;
    break;
  case ':':            // Get Intel Hex Record & Program it
    UART_put_chr(cmd); // echo the ":" only
    cc_enter();
    memset(dataBuf, 0xFF, sizeof(dataBuf));
    data_len = get_ihex_rec(dataBuf, &address, 257);
    if (data_len != 0)
    {
      if (data_len & 0x0001) //Data MUST end on an even byte
        data_len++;

      cc_writeBuf(address, dataBuf, data_len);
      UART_put_strP(PSTR("*\r\n"));
    }
    else
    {
      UART_put_strP(PSTR("\r\nE*CS:"));
      UART_PutHexByte(address & 0x00FF);
      UART_put_strP(PSTR("\r\n"));
    }
    debugMode = 1;
    break;
  case 'U': // User Terminal Mode

    if (debugMode)
    {
      CCDB_PORT &= ~_BV(RS_PIN); //Reset = LOW
      _delay_ms(100);            // 1/10 second
      CCDB_PORT |= _BV(RS_PIN);  //Reset = HIGH
      debugMode = 0;
    }

    UART_put_strP(PSTR("\r\n Terminal Mode   Press [Esc] to exit\r\n"));
    PORTD &= 0xF7; // PD3 = High (pull-up)
    DDRD &= 0xF7;  // PD3 = RXD = INPUT

    val = 0;
    _delay_ms(100);
    putcharBB('\r');
    _delay_ms(100);
    putcharBB('\n');
    _delay_ms(100);
    putcharBB('M');

    while (val != 0x1B)
    {
      if ((PIND & 0x08) == 0)
      {
        val = getcharBB();
        UART_put_chr(val);
        //UART_PutHexByte(val);
      }

      if (Serial.available())
      {
        putcharBB(Serial.read());
      }
    }

    UART_put_strP(PSTR("\r\n Done\r\n"));
    break;
  case '@': // System attention command - just reply with '$$$'
    UART_put_strP(PSTR("\r\n$$$\r\n"));
    do_echo = 0; //Clear the do_echo flag - we are(most likely) talking to a PC
    break;

  case 'F': // Blink the UNO on-board LED just to prove we're working
    UART_put_strP(PSTR("\r\n Flashing the LED 10x fast..."));
    flashLED(10);
    do_echo = 1;
    UART_put_strP(PSTR("Done\r\n"));
    break;
  }
}

// ***********************************************************
// ***** Utility Routines ************************************
// ***********************************************************

// ***********************************************************
//  getAddress(bEcho)
//  get a 16 bit HEX address
// ***********************************************************
uint16_t getAddress(uint8_t bEcho)
{
  uint8_t addBuf[5];
  if (bEcho)
    UART_put_strP(PSTR("\r\n Enter Address: "));

  UART_get_str(addBuf, 2, bEcho);
  addrH = HexStrToByte(addBuf);
  UART_get_str(addBuf, 2, bEcho);
  addrL = HexStrToByte(addBuf);

  return ((uint16_t)addrH << 8) + addrL;
}

// ***********************************************************
//  getDataBytes( uint8_t *buf, uint16_t count)
// ***********************************************************
uint16_t getDataBytes(uint8_t *buf, uint16_t count)
{
  uint16_t x = 0;
  while (x < count)
  {
    UART_get_str(msg, 2, 1);
    if (msg[0] < ' ') // User entered CR/LF/Esc/Tab/Space?
      break;          // This means we're done with input

    buf[x] = HexStrToByte(msg); //Convert & store it
    if (++x < count)            // if not the last entry
      UART_put_chr(',');        // output a comma
  }

  return x;
}

// ***********************************************************
//  dumpDataBuf(uint16_t addr)
// ***********************************************************
void dumpDataBuf(uint16_t addr)
{
  uint8_t x, c;
  UART_put_strP(PSTR("\r\n "));
  UART_PutHexByte(addr >> 8);
  UART_PutHexByte(addr & 0x00FF);
  UART_put_chr(':');
  UART_put_chr(' ');

  for (x = 0; x < 16; x++)
  {
    UART_PutHexByte(dataBuf[x]);
    if (x == 7)
      UART_put_chr('-');
    else
      UART_put_chr(' ');
  }

  UART_put_chr(' ');
  for (x = 0; x < 16; x++)
  {
    c = dataBuf[x];

    if ((c < ' ') || (c > 0x7f))
      c = '.';

    UART_put_chr(c);
  }
}

// ***********************************************************
//  get_ihex_rec(unsigned uint8_t *buf, unsigned int *addr, uint8_t max)
//  input and store data from Intel Hex File record
// iHex record format:
// recLen(2bytes)address(4bytes)type(2bytes)data(recLen*2bytes)checksum(2bytes)
// ***********************************************************
uint8_t get_ihex_rec(uint8_t *buf, uint16_t *addr, uint16_t max)
{
  uint16_t addr16 = 0, cs_sum = 0;
  uint8_t p, dp, data, recLen, recType, addr_hi, addr_lo, cs1, cs2, csRL;

  // Get Record Length
  recLen = UART_get_hex_byte(do_echo);
  if (recLen > (max - 1))
    return 0;

  csRL = recLen; //Save the unmodified recLen for checksum calculation.

  // Get Address
  addr_hi = UART_get_hex_byte(do_echo);
  addr_lo = UART_get_hex_byte(do_echo);
  addr16 = (uint16_t)(addr_hi << 8) + addr_lo;

  dp = 0;
  if (addr16 & 0x0001) // Start address MUST be even (on a word boundary)
  {                    // If not then adjust by making 1st byte = 0xFF
    addr16--;          // Back up start address 1 byte
    dp = 1;            // Advance buffer index
    recLen++;          // Increase recLen by 1
    buf[0] = 0xFF;     // Set 1st byte to 0xFF
  }                    // NOTE:(0xFF will have no effect on an already programed byte)

  // Get Record Type
  recType = UART_get_hex_byte(do_echo);

  // Get Data Bytes
  for (p = 0; p < csRL; p++)
  {
    data = UART_get_hex_byte(do_echo); // get input data
    buf[dp] = data;                    // store it
    cs_sum += data;                    // add it's value to checksum count
    dp++;                              // increment index
  }

  // Get Record Checksum compare to checksum calculated
  cs1 = UART_get_hex_byte(do_echo);
  cs_sum += (csRL + addr_hi + addr_lo + recType);
  cs2 = (uint8_t)((~cs_sum) & 0x00FF) + 1;

  if ((cs2 != cs1) && (cs1 != 0)) //Test data has zero checksum - ignore it
  {
    recLen = 0;
    addr16 = cs2;
  }

  *addr = addr16; // Return the (adjusted) start address
  return recLen;  // Return the (adjusted) record length
}

// ***********************************************************
// ******* CC DEBUGGER FUNCTIONS *****************************
// ***********************************************************

// ***********************************************************
// cc_init()
// ***********************************************************
void cc_init(void)
{
  CCDB_PORT = ~(_BV(RS_PIN) | _BV(DC_PIN) | _BV(DD_PIN) | _BV(PD3)); //ALL PINS LOW
  CCDB_DDR = 0;                                                      //All PINS INPUT
  CCDB_DDR |= _BV(RS_PIN);                                           // RESET = OUTPUT
  _delay_ms(1);
  CCDB_PORT |= _BV(RS_PIN); //Reset = HIGH
}

// ***********************************************************
// cc_enter()
// ***********************************************************
void cc_enter(void)
{
  CCDB_PORT &= ~_BV(RS_PIN); //Reset = LOW
  _delay_us(200);            //Wait 200us

  CCDB_DDR |= _BV(DC_PIN);   //Clock = OUTPUT
  CCDB_PORT |= _BV(DC_PIN);  //Clock = High
  _delay_us(10);             //Wait 10us
  CCDB_PORT &= ~_BV(DC_PIN); //Clock = LOW
  _delay_us(10);             //Wait 10us
  CCDB_PORT |= _BV(DC_PIN);  //Clock = High
  _delay_us(10);             //Wait 10us
  CCDB_PORT &= ~_BV(DC_PIN); //Clock = LOW

  _delay_us(100);           //Wait 100us
  CCDB_PORT |= _BV(RS_PIN); //Reset = High
  _delay_us(10);            //Wait 10us
}

// ***********************************************************
// void cc_send(uint8_t data)
// ***********************************************************
void cc_send(uint8_t data)
{
  uint8_t bct;

  CCDB_DDR |= (_BV(DD_PIN) | _BV(DC_PIN)); // Make sure DD & DC = output

  // Sent bytes
  for (bct = 8; bct; bct--)
  {
    if (data & 0x80)
      CCDB_PORT |= _BV(DD_PIN); //Data = High
    else
      CCDB_PORT &= ~_BV(DD_PIN); //Data = Low

    CCDB_PORT |= _BV(DC_PIN); //Clock = High
    data = data << 1;         // Shift data left
    _delay_us(RW_DELAY_US);   //Wait 10us

    CCDB_PORT &= ~_BV(DC_PIN); //Clock = LOW
    _delay_us(RW_DELAY_US);    //Wait 10us
  }

  CCDB_DDR &= ~(_BV(DD_PIN) | _BV(DC_PIN)); // set DD&DC back to INPUT
}

// ***********************************************************
// uint8_t cc_readyWait(void)
// ***********************************************************
uint8_t cc_readyWait(void)
{
  uint8_t bct = 255; //set bcnt for max wait count

  CCDB_PORT &= ~_BV(DD_PIN); // No Pullup = LOW
  CCDB_DDR &= ~_BV(DD_PIN);  // set DD to INPUT
  CCDB_DDR |= _BV(DC_PIN);   // Set Clock=OUTPUT
  CCDB_PORT &= ~_BV(DC_PIN); // Make sure Clock = LOW
  //_delay_us(1);  // Wait at least 83 ns before checking state t(dir_change)

  while (--bct) // Wait for timeout ot data pin to go LOW  (Chip is READY)
  {
    if ((CCDB_PIN & _BV(DD_PIN)) == 0)
      break;
  }

  return bct;
}

// ***********************************************************
// void cc_read(uint8_t data)
// NOTE: assumes readyWait was successful
// ***********************************************************
uint8_t cc_read(void)
{
  uint8_t data = 0, bct;

  CCDB_PORT &= ~_BV(DD_PIN); // No Pullup = LOW
  CCDB_DDR &= ~_BV(DD_PIN);  // set DD to INPUT
  CCDB_DDR |= _BV(DC_PIN);   // Set Clock=OUTPUT

  for (bct = 8; bct; bct--)
  {
    CCDB_PORT |= _BV(DC_PIN); //Clock = High
    _delay_us(RW_DELAY_US);   //Wait 10us

    data = data << 1; //Shift data

    if (CCDB_PIN & _BV(DD_PIN))
      data |= 0x01;

    CCDB_PORT &= ~_BV(DC_PIN); //Clock = LOW
    _delay_us(RW_DELAY_US);    //Wait 10us
  }

  CCDB_DDR &= ~(_BV(DD_PIN) | _BV(DC_PIN)); // set DD&DC back to INPUT
  return data;
}

// ***********************************************************
// uint16_t cc_readID(void)
// assumes cc_enter() was successful
// ***********************************************************
uint16_t cc_readID(void)
{
  uint16_t id = 0xFFFF;
  cc_send(I_GET_CHIP_ID); //CMD_GET_CHIP_ID 0x68
  if (cc_readyWait())     // Switch  DD pin to input and wait for ready response.
  {                       // If no timeout detected
    id = cc_read();       // Get ID  (0x81)
    id = id << 8;
    id += cc_read(); // Get Version
  }

  return id;
}

// ***********************************************************
// cc_readStatus(void)
// Read status byte
// Assumes cc_enter() was successful
// 0x08 - HALT_STATUS          0x80 - CHIP_ERASE_DONE
// 0x04 - DEBUG_LOCKED         0x40 - PCON_IDLE
// 0x02 - OSCILLATOR_STABLE    0x20 - CPU_HALTED
// 0x01 - STACK_OVERFLOW       0x10 - POWER_MODE_0
// ***********************************************************
uint8_t cc_readStatus(void)
{
  uint8_t status = 0xFF;

  cc_send(I_READ_STATUS); // Send status cmd (0x34)

  if (cc_readyWait())   // Switch  DD pin to input and wait for ready response.
    status = cc_read(); // Read status byte

  return status;
}

// ***********************************************************
// uint16_t cc_eraseChip(void)
// assumes cc_enter() was successful
// returns zero if timeout
// ***********************************************************
uint8_t cc_eraseChip(void)
{
  uint8_t x = 255, status = 0x00;
  cc_send(I_CHIP_ERASE); //I_CHIP_ERASE = 0x14
  while (x--)
  {
    status = cc_readStatus(); // get status
    if (status & 0x80)        // if CHIP_ERASE_DONE flag set
      break;                  // exit
  }

  return x; // x=0 = timeout error
}

// ***********************************************************
// uint8_t cc_exec1(uint8_t cmd1)
// Execute a 1 byte 8051 instruction
// NOTE: assumes cc_enter() was successful
// ***********************************************************
uint8_t cc_exec1(uint8_t cmd1)
{
  uint8_t rct, ret = 0;

  cc_send(I_DEBUG_INSTR_1); // Send I_DEBUG_INSTR_1
  cc_send(cmd1);            // Send cmd1
  rct = cc_readyWait();
  if (rct)
  {
    ret = cc_read(); // read A
    errorNo = 0;
  }
  else
    errorNo = 1;

  return ret;
}

// ***********************************************************
// uint8_t cc_exec2(uint8_t cmd1, uint8_t cmd2)
// Execute a 2 byte 8051 instruction
// NOTE: assumes cc_enter() was successful
// ***********************************************************
uint8_t cc_exec2(uint8_t cmd1, uint8_t cmd2)
{
  uint8_t rct, ret = 0;

  cc_send(I_DEBUG_INSTR_2); // Send I_DEBUG_INSTR_2
  cc_send(cmd1);            // Send cmd1
  cc_send(cmd2);            // Send cmd2
  rct = cc_readyWait();
  if (rct)
  {
    ret = cc_read(); // read A
    errorNo = 0;
  }
  else
    errorNo = 1;

  return ret;
}

// ***********************************************************
// uint8_t cc_exec3(uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
// Execute a 3 byte 8051 instruction
// NOTE: assumes cc_enter() was successful
// ***********************************************************
uint8_t cc_exec3(uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
  uint8_t rct, ret = 0;

  cc_send(I_DEBUG_INSTR_3); // Send I_DEBUG_INSTR_2
  cc_send(cmd1);            // Send cmd1
  cc_send(cmd2);            // Send cmd2
  cc_send(cmd3);            // Send cmd3
  rct = cc_readyWait();
  if (rct)
  {
    ret = cc_read(); // read A
    errorNo = 0;
  }
  else
    errorNo = 1;

  return ret;
}

// ***********************************************************
// cc_read16f(address, bank, buf)
// Read 16 bytes from flash memory
// ***********************************************************
void cc_read16f(uint16_t addr, uint8_t bank, uint8_t *buf)
{
  uint8_t x, adrH, adrL;
  adrH = (addr >> 8) & 0x00FF;
  adrL = addr & 0x00FF;

  cc_exec3(0x75, 0xC7, (bank * 16) + 1); // MOV MEMCTR, (bank *16)+1;
  cc_exec3(0x90, adrH, adrL);            // MOV DPTR, address;
  for (x = 0; x < 16; x++)
  {
    cc_exec1(0xE4);          // CLR A;
    buf[x] = cc_exec1(0x93); // MOV A, @DPTR;
    cc_exec1(0xA3);          // INC DPTR;
  }
}
// ***********************************************************
// cc_pageErase(address)
// Assumed enter was successful
// ***********************************************************
void cc_pageErase(uint16_t addr)
{
  uint8_t adrH, adrL, flc;
  adrH = ((addr >> 8) / 2) & 0x7E;
  adrL = 0;

  cc_exec3(0x75, 0xAD, adrH); // MOV FADDRH, #AddrH;
  cc_exec3(0x75, 0xAC, adrL); // MOV FADDRL, #0;
  cc_exec3(0x75, 0xAE, 0x01); // MOV FLC, #01H; //ERASE
  flc = 0x80;
  while (flc & 0x80) //Test BUSY flag
  {
    flc = cc_exec2(0xE5, 0xAE); // MOV A, FLC;
  }
}

// ***********************************************************
// cc_writeBuf(address, buf, size)
// Write up to 256 bytes to flash memory
// Assumes flash is in erased state and enter was successful
// ***********************************************************
void cc_writeBuf(uint16_t addr, uint8_t *buf, uint16_t size)
{
  uint8_t status, adrH, adrL;

  addr = addr / 2;
  adrH = addr >> 8 & 0x00FF;
  adrL = addr & 0x00FF;

  flashWrite16[2] = adrH;      // Update routine with adrH
  flashWrite16[5] = adrL;      // Update routine with adrL
  flashWrite16[13] = size / 2; // Update routine with # of words to write

  cc_writeXD(0xF000, buf, size); // Move buf to address 0xF000 in SRAM
  // Move flashWrite16 routine to address 0xF100 in SRAM
  cc_writeXD(0xF100, flashWrite16, sizeof(flashWrite16));

  cc_exec3(0x75, 0xC7, 0x51); // MOV MEMCTR, (bank *16)+1;
  cc_exec3(0x02, 0xF1, 0x00); // Set PC = 0xF100
  cc_send(I_RESUME);          //send RESUME 0x4C
  status = 0;
  while (!status & 0x08) // Wait for halt status
  {
    status = cc_readStatus();
  }
}

// ***********************************************************
// cc_read16x(address, buf)
// Read 16 bytes from XDATA memory (SRAM)
// ***********************************************************
void cc_read16x(uint16_t addr, uint8_t *buf)
{
  uint8_t x, adrH, adrL;
  adrH = (addr >> 8) & 0x00FF;
  adrL = addr & 0x00FF;

  cc_exec3(0x90, adrH, adrL); // MOV DPTR, address;
  for (x = 0; x < 16; x++)
  {
    buf[x] = cc_exec1(0xE0); // MOV A, @DPTR;
    cc_exec1(0xA3);          // INC DPTR;
  }
}

// ***********************************************************
// cc_writeXD(address, buf)
// Write size bytes to XDATA memory (SRAM)
// ***********************************************************
void cc_writeXD(uint16_t addr, uint8_t *buf, uint16_t size)
{
  uint16_t x;
  uint8_t adrH, adrL;
  adrH = (addr >> 8) & 0x00FF;
  adrL = addr & 0x00FF;

  cc_exec3(0x90, adrH, adrL); // MOV DPTR, address;
  for (x = 0; x < size; x++)
  {
    cc_exec2(0x74, buf[x]); // MOV A, #buf[x];
    cc_exec1(0xF0);         // MOVX @DPTR, A;
    cc_exec1(0xA3);         // INC DPTR;
  }
}

void flashLED(uint8_t count)
{
  uint8_t x;

  for (x = 0; x < count; x++)
  {
    PORTB &= ~_BV(PB5); // LED OFF
    _delay_ms(125);     // delay
    PORTB |= _BV(PB5);  //  LED ON
    _delay_ms(125);     // delay
  }
}

// ***********************************************************
// UART_get_chr(void)
//  Wait for a character to be recieved and return it's value
// ***********************************************************
uint8_t UART_get_chr(void)
{
  while (true)
  {
    int c = Serial.read();
    if (c == -1)
      continue;
    return c;
  };
}

// ***********************************************************
//  UART_put_chr(uint8_t c)
//  Wait for output buffer to be empty then send the character.
// ***********************************************************
void UART_put_chr(uint8_t c)
{
  Serial.write(c);
}

// ***********************************************************
//  UART_put_str(uint8_t *str)
//  Send characters to USART (str) until NULL is encountered.
// ***********************************************************
void UART_put_str(uint8_t *str)
{
  int p = 0; // character pointer set to start of string

  while (str[p] != 0)       // While char is not = 0
    UART_put_chr(str[p++]); // send it and inc. pointer
}

// ***********************************************************
//  UART_put_strP(char *str)
//  Send characters from PROGRAM Memory(Flash) to USART
//  HOTE: be sure to #include <avr/pgmspace.h>
// ***********************************************************
void UART_put_strP(char *str)
{
  while (pgm_read_byte(&(*str)))                     // While char is not = 0
    UART_put_chr((uint8_t)pgm_read_byte(&(*str++))); // send it and inc. pointer
}

// ***********************************************************
//  UART_get_str(uint8_t *str, int max, bEcho)
//  Get characters from USART and store them in (str) until
//  newline '\n' or (max) number of characters recieved.
//  echo input if bEcho = TRUE
// ***********************************************************
void UART_get_str(uint8_t *str, int max, uint8_t bEcho)
{
  uint8_t c;
  int p = 0; // Character pointer set to start of string.

  while (p < max) // Until max input recieved
  {
    c = UART_get_chr(); // Get 1 char.

    if (c == '\r') // if it's [Enter]
      break;       // we're done..
    else           // otherwise
    {
      str[p++] = c; // store it and inc. pointer
      if (bEcho)
        UART_put_chr(c);
    }
  }
  str[p] = 0; // terminate the string.
}

// ***********************************************************************
// UART_PutHexByte(uint8_t byte)
// ***********************************************************************
void UART_PutHexByte(uint8_t byte)
{
  uint8_t n = (byte >> 4) & 0x0F;
  // Write high order digit
  if (n < 10)
    UART_put_chr(n + '0');
  else
    UART_put_chr(n - 10 + 'A');

  // Write low order digit
  n = (byte & 0x0F);
  if (n < 10)
    UART_put_chr(n + '0');
  else
    UART_put_chr(n - 10 + 'A');
}

// ***********************************************************
//  UART_get_hex_byte(uint8_t echo)
//  Get characters from USART and convert hex values to byte
//  call with echo = 1 to echo input bytes
//  RETURN: value of 2 converted hex bytes.
//  NOTE: no error checking. assumes valuid hex character input
// ***********************************************************
uint8_t UART_get_hex_byte(uint8_t echo)
{
  uint8_t i, buf[3];
  for (i = 0; i < 2; i++)
  {
    buf[i] = UART_get_chr();
    if (echo)
      UART_put_chr(buf[i]);
  }
  buf[2] = 0;
  return HexStrToByte(buf);
}

//************************************************************************
// AsciiToHexVal(b)
// Convert a single ASCII character to it's 4 bit value 0-9 or A-F
// Note: no value error checking is done. Valid HEX characters is assumed
//************************************************************************
uint8_t AsciiToHexVal(uint8_t b)
{
  uint8_t v = b & 0x0F; // '0'-'9' simply mask high 4 bits
  if (b > '9')
    v += 9; // 'A'-'F' = low 4 bits +9
  return v;
}
//************************************************************************
// HexStrToByte(uint8_t *buf)
// Convert a 2 character ASCII string to a 8 bit unsigned value
//************************************************************************
uint8_t HexStrToByte(uint8_t *buf)
{
  uint8_t v;
  v = AsciiToHexVal(buf[0]) * 16;
  v += AsciiToHexVal(buf[1]);
  return v;
}

// ************************************************************************
//  *** Bit -Bang serial I/O for CCdebug terminal emulation
// Use Data Clock pin for TXD and Data I/O pin for RXD
// Data Clock(DC)= Pin4/PD4   -> P2_2    (CC2510 Pin 16)
// Data I/O(DD)  = Pin3/PD3   -> P2_1    (CC2510 Pin 15)
// ************************************************************************
//==============================================================================
// putcharBB( uint8_t c )
// Output 1 serial character @ 9600 baud
//
// Example below shows tranmission of "U" (0x55) OR 01010101
// Bits are recieved LSB->MSB
//      |1T | 1T| 1T| 1T| 1T| 1T| 1T| 1T| 1T| 1T| 1T|
//idle.-     ---     ---     ---     ---     ---  --- ...
//      |   |   |   |   |   |   |   |   |   |
//      |   |   |   |   |   |   |   |   |   |         idle...
//       ---     ---     ---     ---     ---
//      Start B0  B1  B2  B3  B4  B5  B6  B7 Stop Stop
//      LSB-> 1   0   1   0   1   0   1   0 <-MSB
//==============================================================================
void putcharBB(uint8_t c)
{
  uint8_t bit;
  DDRD |= 0x10;  // PD4 = TXD = OUTPUT
  PORTD &= 0xEF; // Set TXD LOW

  _delay_us(104); // Send Start Bit (1 Bit Delay @ 9600 baud = 104.66us)

  for (bit = 8; bit > 0; bit--) // Send 8 Data Bits
  {
    if (c & 0x01)    //if LSB=1, Set Output to match
      PORTD |= 0x10; // Set TXD HIGH
    else
      PORTD &= 0xEF; // Set TXD LOW

    _delay_us(104); // 9600 baud   1 Bit Delay for 1MHZ clock

    c = (c >> 1);
  }

  PORTD |= 0x10;  // Set TXD HIGH
  _delay_us(208); // Send 2 Stop Bits
}

//==============================================================================
// int getcharBB()
// Recieve 1 serial character at a rate determined by (baud_divider)
//
// Example below shows tranmission of "U" (0x55) OR 01010101
// Bits are recieved LSB->MSB
//    0.5T| 1T| 1T| 1T| 1T| 1T| 1T| 1T| 1T|
//idle.-     ---     ---     ---     ---     --- --- ...
//      |   |   |   |   |   |   |   |   |   |
//      |   |   |   |   |   |   |   |   |   |                idle...
//       ---     ---     ---     ---     ---
//      Start B0  B1  B2  B3  B4  B5  B6  B7 Stop Stop
//      LSB-> 1   0   1   0   1   0   1   0 <-MSB
//=============================================================================
uint8_t getcharBB(void)
{
  uint8_t c = 0;
  uint8_t bt = 0;

  PORTD &= 0xF7; // PD3 = LOW (no pull-up)
  DDRD &= 0xF7;  // PD3 = RXD = INPUT

  //Just wait for capture edge from start bit on RXD pin (PD3) to go LOW
  while (PIND & 0x08)
    ;

  _delay_us(50); // Delay 1/2 bit time (1 bit @9600 baud = 104us)

  for (bt = 8; bt > 0; bt--) // Get 8 Data Bits
  {
    c = c >> 1; //Shift all the bits left one bit.

    _delay_us(104); // Delay 1 bit time (1 bit @9600 baud = 104us)

    //Read the input signal pin. If it's a "1", then OR a "1" onto result values MSB.
    if (PIND & 0x08)
    {
      c |= 0x80;
    }
  }

  _delay_us(85); //Delay 1/2 bit (stop bit)
  return (c);    //Return the byte recieved.
}
