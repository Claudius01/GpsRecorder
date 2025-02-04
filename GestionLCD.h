// $Id: GestionLCD.h,v 1.7 2025/02/04 09:49:31 administrateur Exp $

#ifndef __GESTION_LCD__
#define __GESTION_LCD__

#include <stdint.h>
#include <stdio.h>
#include <pgmspace.h>
#include <stdlib.h>
#include <string.h>		//memset()
#include <math.h>

#include <Arduino.h>

#include "fonts.h"
#include <SPI.h>

#define LIGHTS_POSITION_X       10

// Positions des feux @ GPS
#define LIGHTS_POSITION_GPS               LIGHTS_POSITION_X
#define LIGHTS_POSITION_GPS_GREEN_X       LIGHTS_POSITION_GPS
#define LIGHTS_POSITION_GPS_YELLOW_X      (LIGHTS_POSITION_GPS + 25)
#define LIGHTS_POSITION_GPS_RED_X         (LIGHTS_POSITION_GPS + 50)

// Positions des feux @ SDCard
#define LIGHTS_POSITION_SDC               (LIGHTS_POSITION_X + 100)
#define LIGHTS_POSITION_SDC_YELLOW_X      LIGHTS_POSITION_SDC
#define LIGHTS_POSITION_SDC_RED_X         (LIGHTS_POSITION_SDC + 25)
#define LIGHTS_POSITION_SDC_BLUE_X        (LIGHTS_POSITION_SDC + 50)

#define LIGHTS_POSITION_Y       108

#define LIGHT_FULL_IDX          0
#define LIGHT_BORD_IDX          1

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

// Code tire de 'DEV_Config.h'
/**
 * GPIO config
**/
#define DEV_RST_PIN 9
#define DEV_DC_PIN  8
#define DEV_CS_PIN  10
#define DEV_BL_PIN  7

/**
 * GPIO read and write
**/
#define DEV_Digital_Write(_pin, _value) digitalWrite(_pin, _value == 0? LOW:HIGH)
#define DEV_Digital_Read(_pin) digitalRead(_pin)

/**
 * SPI
**/
#define DEV_SPI_WRITE(_dat)   SPI.transfer(_dat)

/**
 * delay x ms
**/
#define DEV_Delay_ms(__xms)    delay(__xms)

/**
 * PWM_BL
**/
 #define  DEV_Set_BL(_Pin, _Value)  analogWrite(_Pin, _Value)

class Config {
	private:
		void GPIO_Init();

	public:
		Config();
		~Config();

		void Config_Init();
};
// Fin: Code tire de 'DEV_Config.h'

// Code tire de 'LCD_Driver.h'
#define LCD_WIDTH   135     // LCD width
#define LCD_HEIGHT  240     // LCD height

#define LCD_X_SIZE_MAX    (max(LCD_WIDTH, LCD_HEIGHT))
#define LCD_Y_SIZE_MAX    (max(LCD_WIDTH, LCD_HEIGHT))

class LCD {
	private:

	protected:
		LCD();
		~LCD();

		void LCD_Reset(void);

		void LCD_WriteData_Byte(UBYTE da); 
		void LCD_WriteData_Word(UWORD da);
		void LCD_WriteReg(UBYTE da);

		void LCD_SetCursor(UWORD x1, UWORD y1, UWORD x2,UWORD y2);
		void LCD_SetUWORD(UWORD x, UWORD y, UWORD Color);

		void LCD_Init(void);
		void LCD_SetBacklight(UWORD Value);
		void LCD_Clear(UWORD Color);
		void LCD_ClearWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD UWORD);

		void LCD_SetWindowColor(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,UWORD  Color);
};
// Fin: Code tire de 'LCD_Driver.h'

// Code tire de 'GUI_Paint.h'
/**
 * Image attributes
**/
typedef struct {
    UBYTE *Image;
    UWORD Width;
    UWORD Height;
    UWORD WidthMemory;
    UWORD HeightMemory;
    UWORD Color;
    UWORD Rotate;
    UWORD Mirror;
    UWORD WidthByte;
    UWORD HeightByte;
} PAINT;

/**
 * Display rotate
**/
#define ROTATE_0            0
#define ROTATE_90           90
#define ROTATE_180          180
#define ROTATE_270          270

/**
 * Display Flip
**/
typedef enum {
    MIRROR_NONE  = 0x00,
    MIRROR_HORIZONTAL = 0x01,
    MIRROR_VERTICAL = 0x02,
    MIRROR_ORIGIN = 0x03,
} MIRROR_IMAGE;

#define MIRROR_IMAGE_DFT MIRROR_NONE

/**
 * image color
**/
#define WHITE         0xFFFF
#define BLACK         0x0000    
#define BLUE          0x001F  
#define BRED          0XF81F
#define GRED          0XFFE0
#define GBLUE         0X07FF
#define RED           0xF800
#define MAGENTA       0xF81F
#define GREEN         0x07E0
#define CYAN          0x7FFF
#define YELLOW        0xFFE0
#define BROWN         0XBC40 
#define BRRED         0XFC07 
#define GRAY          0X8430 
#define DARKBLUE      0X01CF  
#define LIGHTBLUE     0X7D7C   
#define GRAYBLUE      0X5458 
#define LIGHTGREEN    0X841F 
#define LGRAY         0XC618 
#define LGRAYBLUE     0XA651
#define LBBLUE        0X2B12 

#define IMAGE_BACKGROUND    WHITE
#define FONT_FOREGROUND     BLACK
#define FONT_BACKGROUND     WHITE

/**
 * The size of the point
**/
typedef enum {
    DOT_PIXEL_1X1  = 1,   // 1 x 1
    DOT_PIXEL_2X2  ,    // 2 X 2
    DOT_PIXEL_3X3  ,    // 3 X 3
    DOT_PIXEL_4X4  ,    // 4 X 4
    DOT_PIXEL_5X5  ,    // 5 X 5
    DOT_PIXEL_6X6  ,    // 6 X 6
    DOT_PIXEL_7X7  ,    // 7 X 7
    DOT_PIXEL_8X8  ,    // 8 X 8
} DOT_PIXEL;

#define DOT_PIXEL_DFT  DOT_PIXEL_1X1  //Default dot pilex

/**
 * Point size fill style
**/
typedef enum {
    DOT_FILL_AROUND  = 1,   // dot pixel 1 x 1
    DOT_FILL_RIGHTUP  ,     // dot pixel 2 X 2
} DOT_STYLE;

#define DOT_STYLE_DFT  DOT_FILL_AROUND  //Default dot pilex

/**
 * Line style, solid or dashed
**/
typedef enum {
    LINE_STYLE_SOLID = 0,
    LINE_STYLE_DOTTED,
} LINE_STYLE;

/**
 * Whether the graphic is filled
**/
typedef enum {
    DRAW_FILL_EMPTY = 0,
    DRAW_FILL_FULL,
} DRAW_FILL;

/**
 * Custom structure of a time attribute
**/
typedef struct {
    UWORD Year;	// 0000
    UBYTE  Month; // 1 - 12
    UBYTE  Day;   // 1 - 30
    UBYTE  Hour;  // 0 - 23
    UBYTE  Min;   // 0 - 59
    UBYTE  Sec;   // 0 - 59
} PAINT_TIME;

class Paint : public LCD {
	private:
		volatile PAINT    m__paint;     // TBC: 'volatile'

    UWORD             m__cache_color[LCD_X_SIZE_MAX][LCD_Y_SIZE_MAX];    // m__cache_color[x-1][y-1] after rotation

	public:
		Paint();
		~Paint();

		// Init and Clear
		void Paint_SelectImage(UBYTE *image);
		void Paint_SetRotate(UWORD Rotate);
		void Paint_SetMirroring(UBYTE mirror);
		void Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color);

		void Paint_Clear(UWORD Color);
		void Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color);

		// Drawing
		void Paint_DrawPoint(UWORD Xpoint, UWORD Ypoint, UWORD Color, DOT_PIXEL Dot_Pixel, DOT_STYLE Dot_FillWay);
		void Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style);
		void Paint_DrawRectangle(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Filled );
		void Paint_DrawCircle(UWORD X_Center, UWORD Y_Center, UWORD Radius, UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill );

		// Display string
		void Paint_DrawChar(UWORD Xstart, UWORD Ystart, const char Acsii_Char, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);
		void Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);
		void Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);
		void Paint_DrawFloatNum(UWORD Xpoint, UWORD Ypoint, double Nummber,  UBYTE Decimal_Point, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);
		void Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);

		// Pic
		void Paint_NewImage(UWORD Width, UWORD Height, UWORD Rotate, UWORD Color);
		void Paint_DrawImage(const unsigned char *image, UWORD Startx, UWORD Starty, UWORD Endx, UWORD Endy);

		void Paint_DrawSymbol(UWORD Xpoint, UWORD Ypoint, const char Num_Symbol, sFONT* Font, UWORD Color_Background, UWORD Color_Foreground);

    // Analyse du cache
    bool Paint_GetCache(UWORD i__x, UWORD i__y) { return (m__cache_color[i__x][i__y] != BLACK ? true : false); };
};
// Fin: Code tire de 'GUI_Paint.h'

class GestionLCD : public Paint {
	private:
		class Config m__config;

	public:
		GestionLCD();
		~GestionLCD();

		void init(UWORD i__val_back_light) {
			m__config.Config_Init();

			LCD_Init();
			LCD_SetBacklight(i__val_back_light);
		};

		void clear(UWORD i__color) {
			LCD_Clear(i__color);
		};
};

extern GestionLCD   *g__gestion_lcd;
#endif

