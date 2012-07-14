#include "../glcd.h"
#include "LPC11Uxx.h"

#if defined(GLCD_DEVICE_LPC11UXX)

void glcd_init(void)
{

#if defined(GLCD_CONTROLLER_PCD8544)
	/*
	 * Set up SPI (SSP)
	 * Note: Max allowed SPI clock is 4 MHz from datasheet.
	 */

	/* Select SSP/SPI port */
	SSP_IOConfig( CONTROLLER_SPI_PORT_NUMBER );

	/* Initialise SSP/SPI port */
	SSP_Init( CONTROLLER_SPI_PORT_NUMBER );

	/* Above functions take care of SPI pins */

	/* Set SS, DC and RST pins to output */
	CONTROLLER_SS_PORT->DIR  |= (1 << CONTROLLER_SS_PIN);
	CONTROLLER_DC_PORT->DIR  |= (1 << CONTROLLER_DC_PIN);
	CONTROLLER_RST_PORT->DIR |= (1 << CONTROLLER_RST_PIN);

	/* Deselect LCD */
	GLCD_DESELECT();

	/* Reset the display */
	glcd_reset();

	/* Get into the EXTENDED mode! */
	glcd_command(PCD8544_FUNCTION_SET | PCD8544_EXTENDED_INSTRUCTION);

	/* LCD bias select (4 is optimal?) */
	glcd_command(PCD8544_SET_BIAS | 0x2);

	/* Set VOP */
	glcd_command(PCD8544_SET_VOP | 50); // Experimentally determined

	/* Back to standard instructions */
	glcd_command(PCD8544_FUNCTION_SET);

	/* Normal mode */
	glcd_command(PCD8544_DISPLAY_CONTROL | PCD8544_DISPLAY_NORMAL);

	glcd_select_screen((uint8_t *)&glcd_buffer,&glcd_bbox);

	glcd_clear();

#elif defined(GLCD_CONTROLLER_NT75451)
	/* Parallel interface controller used on NGX BlueBoards */
	
	/* Set 4x control lines pins as output */
	LPC_GPIO->DIR[CONTROLLER_LCD_EN_PORT] |= (1U<<CONTROLLER_LCD_EN_PIN);
	LPC_GPIO->DIR[CONTROLLER_LCD_RW_PORT] |= (1U<<CONTROLLER_LCD_RW_PIN);
	LPC_GPIO->DIR[CONTROLLER_LCD_RS_PORT] |= (1U<<CONTROLLER_LCD_RS_PIN);
	LPC_GPIO->DIR[CONTROLLER_LCD_CS_PORT] |= (1U<<CONTROLLER_LCD_CS_PIN);
	
	/* Don't worry about setting default RS/RW/CS/EN, they get set during use */
	
#ifdef CONTROLLER_LCD_DATA_PORT	
	/* Set data pins as output */
	LPC_GPIO->DIR[CONTROLLER_LCD_D0_PORT] |= GLCD_PARALLEL_MASK;
#else
	#error "Support of parallel data pins on different ports not supported."
#endif

	/* Initialise sequence - code by NGX Technologies */
	glcd_command(0xE2);  /*	S/W RESWT               */
	glcd_command(0xA0);  /*	ADC select              */
	glcd_command(0xC8);  /*	SHL Normal              */
	glcd_command(0xA3);  /*	LCD bias                */
	glcd_command(0x2F);  /*	Power control           */
	glcd_command(0x22);  /*	reg resistor select     */
	glcd_command(0x40);  /*	Initial display line 40 */
	glcd_command(0xA4);  /*	Normal display          */
	glcd_command(0xA6);  /*	Reverce display a7      */
	glcd_command(0x81);  /*	Ref vg select mode      */
	glcd_command(0x3f);  /*	Ref vg reg select       */
	glcd_command(0xB0);  /*	Set page address        */
	glcd_command(0x10);  /*	Set coloumn addr MSB    */
	glcd_command(0x00);  /*	Set coloumn addr LSB    */
	glcd_command(0xAF);  /*	Display ON              */

	/* Select default screen buffer */
	glcd_select_screen((uint8_t *)&glcd_buffer,&glcd_bbox);

	/* Clear the screen buffer */
	glcd_clear();
	
#else /* GLCD_CONTROLLER_PCD8544 */
	#error "Controller not supported by LPC111x"
#endif

}

#if defined(GLCD_USE_PARALLEL)

/** Write byte via parallel interface */
void glcd_parallel_write(uint8_t c)
{
	
	uint32_t port_output = \
		( ( (1U << 0) & c ? 1 : 0 ) << CONTROLLER_LCD_D0_PIN ) | \
		( ( (1U << 1) & c ? 1 : 0 ) << CONTROLLER_LCD_D1_PIN ) | \
		( ( (1U << 2) & c ? 1 : 0 ) << CONTROLLER_LCD_D2_PIN ) | \
		( ( (1U << 3) & c ? 1 : 0 ) << CONTROLLER_LCD_D3_PIN ) | \
		( ( (1U << 4) & c ? 1 : 0 ) << CONTROLLER_LCD_D4_PIN ) | \
		( ( (1U << 5) & c ? 1 : 0 ) << CONTROLLER_LCD_D5_PIN ) | \
		( ( (1U << 6) & c ? 1 : 0 ) << CONTROLLER_LCD_D6_PIN ) | \
		( ( (1U << 7) & c ? 1 : 0 ) << CONTROLLER_LCD_D7_PIN );

	volatile uint32_t parmask = GLCD_PARALLEL_MASK; // for debugging
	
	/* Perform the write */

	/* Clear data bits to zero and set required bits as needed */
	LPC_GPIO->CLR[CONTROLLER_LCD_D0_PORT] |= GLCD_PARALLEL_MASK;
	LPC_GPIO->SET[CONTROLLER_LCD_D0_PORT] |= port_output;
	
	GLCD_EN_HIGH();
	GLCD_CS_LOW();
	GLCD_RW_LOW();
	
	/* Add a delay here if we need to - determines on your freqs chip is running at */
	//glcd_delay(10);
	
	GLCD_RW_HIGH();
	GLCD_CS_HIGH();
	GLCD_EN_LOW();
}

#else

void glcd_spi_write(uint8_t c)
{
	GLCD_SELECT();
	SSP_Send(CONTROLLER_SPI_PORT_NUMBER,&c,1);
	GLCD_DESELECT();
}

#endif /* GLCD_USE_PARALLEL */

void glcd_reset(void)
{
#if defined(GLCD_CONTROLLER_PCD8544)
	/* Toggle RST low to reset. Minimum pulse 100ns on datasheet. */
	GLCD_SELECT();
	GLCD_RESET_LOW();
	_delay_ms(1);
	GLCD_RESET_HIGH();
	GLCD_DESELECT();
	
#elif defined(GLCD_CONTROLLER_NT75451)
	
#endif /* GLCD_CONTROLLER_PCD8544 */	
}

void glcd_delay(uint32_t count)
{
  uint16_t j=0,i=0;

  for(j=0;j<count;j++)
  {
    /* At 60Mhz, the below loop introduces
    delay of 10 us */
    for(i=0;i<35;i++);
  }
}

#endif /* GLCD_DEVICE_LPC11UXX */
