// $Id: GestionLCD.cpp,v 1.6 2025/02/02 13:10:15 administrateur Exp $

#include "GestionLCD.h"

// Code tire de 'DEV_Config.cpp'
Config::Config()
{
	Serial.printf("Config::Config()\n");
}

Config::~Config()
{
	Serial.printf("Config::~Config()\n");
}

void Config::GPIO_Init()
{
	Serial.printf("Config::GPIO_Init()\n");

	pinMode(DEV_CS_PIN, OUTPUT);
	pinMode(DEV_RST_PIN, OUTPUT);
	pinMode(DEV_DC_PIN, OUTPUT);
	pinMode(DEV_BL_PIN, OUTPUT);

	analogWrite(DEV_BL_PIN, 140);
}

void Config::Config_Init()
{
	Serial.printf("Config::Config_Init()\n");

	GPIO_Init();

	// spi
	SPI.setDataMode(SPI_MODE3);
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.begin();
}
// Fin: Code tire de 'DEV_Config.cpp'

// Code tire de 'LCD_Driver.cpp'
/*******************************************************************************
function:
  Hardware reset
*******************************************************************************/
LCD::LCD()
{
	Serial.printf("LCD::LCD()\n");
}

LCD::~LCD()
{
	Serial.printf("LCD::~Config()\n");
}

void LCD::LCD_Reset(void)
{
  DEV_Digital_Write(DEV_CS_PIN,0);
  DEV_Delay_ms(20);
  DEV_Digital_Write(DEV_RST_PIN,0);
  DEV_Delay_ms(20);
  DEV_Digital_Write(DEV_RST_PIN,1);
  DEV_Delay_ms(20);
}

/*******************************************************************************
function:
  Setting backlight
parameter :
    value : Range 0~255   Duty cycle is value/255
*******************************************************************************/
void LCD::LCD_SetBacklight(UWORD Value)
{
  DEV_Set_BL(DEV_BL_PIN, Value);
}

/*******************************************************************************
function:
    Write register address and data
*******************************************************************************/
void LCD::LCD_WriteData_Byte(UBYTE da) 
{ 
  DEV_Digital_Write(DEV_CS_PIN,0);
  DEV_Digital_Write(DEV_DC_PIN,1);
  DEV_SPI_WRITE(da);  
  DEV_Digital_Write(DEV_CS_PIN,1);
}  

void LCD::LCD_WriteData_Word(UWORD da)
{
  UBYTE i=(da>>8)&0xff;
  DEV_Digital_Write(DEV_CS_PIN,0);
  DEV_Digital_Write(DEV_DC_PIN,1);
  DEV_SPI_WRITE(i);
  DEV_SPI_WRITE(da);
  DEV_Digital_Write(DEV_CS_PIN,1);
}   

void LCD::LCD_WriteReg(UBYTE da)  
{ 
  DEV_Digital_Write(DEV_CS_PIN,0);
  DEV_Digital_Write(DEV_DC_PIN,0);
  DEV_SPI_WRITE(da);
  //DEV_Digital_Write(DEV_CS_PIN,1);
}

/******************************************************************************
function: 
    Common register initialization
******************************************************************************/
void LCD::LCD_Init(void)
{
  LCD_Reset();

  LCD_WriteReg(0x36); 
  LCD_WriteData_Byte(0x70);

  LCD_WriteReg(0x3A); 
  LCD_WriteData_Byte(0x05);

  LCD_WriteReg(0xB2);
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x00);
  LCD_WriteData_Byte(0x33);
  LCD_WriteData_Byte(0x33);

  LCD_WriteReg(0xB7); 
  LCD_WriteData_Byte(0x35);  

  LCD_WriteReg(0xBB);
  LCD_WriteData_Byte(0x19);

  LCD_WriteReg(0xC0);
  LCD_WriteData_Byte(0x2C);

  LCD_WriteReg(0xC2);
  LCD_WriteData_Byte(0x01);

  LCD_WriteReg(0xC3);
  LCD_WriteData_Byte(0x12);   

  LCD_WriteReg(0xC4);
  LCD_WriteData_Byte(0x20);  

  LCD_WriteReg(0xC6); 
  LCD_WriteData_Byte(0x0F);    

  LCD_WriteReg(0xD0); 
  LCD_WriteData_Byte(0xA4);
  LCD_WriteData_Byte(0xA1);

  LCD_WriteReg(0xE0);
  LCD_WriteData_Byte(0xD0);
  LCD_WriteData_Byte(0x04);
  LCD_WriteData_Byte(0x0D);
  LCD_WriteData_Byte(0x11);
  LCD_WriteData_Byte(0x13);
  LCD_WriteData_Byte(0x2B);
  LCD_WriteData_Byte(0x3F);
  LCD_WriteData_Byte(0x54);
  LCD_WriteData_Byte(0x4C);
  LCD_WriteData_Byte(0x18);
  LCD_WriteData_Byte(0x0D);
  LCD_WriteData_Byte(0x0B);
  LCD_WriteData_Byte(0x1F);
  LCD_WriteData_Byte(0x23);

  LCD_WriteReg(0xE1);
  LCD_WriteData_Byte(0xD0);
  LCD_WriteData_Byte(0x04);
  LCD_WriteData_Byte(0x0C);
  LCD_WriteData_Byte(0x11);
  LCD_WriteData_Byte(0x13);
  LCD_WriteData_Byte(0x2C);
  LCD_WriteData_Byte(0x3F);
  LCD_WriteData_Byte(0x44);
  LCD_WriteData_Byte(0x51);
  LCD_WriteData_Byte(0x2F);
  LCD_WriteData_Byte(0x1F);
  LCD_WriteData_Byte(0x1F);
  LCD_WriteData_Byte(0x20);
  LCD_WriteData_Byte(0x23);

  LCD_WriteReg(0x21); 

  LCD_WriteReg(0x11); 

  LCD_WriteReg(0x29);
} 

/******************************************************************************
function: Set the cursor position
parameter :
    Xstart:   Start UWORD x coordinate
    Ystart:   Start UWORD y coordinate
    Xend  :   End UWORD coordinates
    Yend  :   End UWORD coordinatesen
******************************************************************************/
void LCD::LCD_SetCursor(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD  Yend)
{ 
  LCD_WriteReg(0x2a);
  LCD_WriteData_Word(Xstart	+52);
  LCD_WriteData_Word(Xend	+52);
  LCD_WriteReg(0x2b);
  LCD_WriteData_Word(Ystart	+40);
  LCD_WriteData_Word(Yend	+40);

  LCD_WriteReg(0x2C);
}

/******************************************************************************
function: Clear screen function, refresh the screen to a certain color
parameter :
    Color :   The color you want to clear all the screen
******************************************************************************/
void LCD::LCD_Clear(UWORD Color)
{
  UWORD i,j;    
  LCD_SetCursor(0,0,LCD_WIDTH-1,LCD_HEIGHT-1);

  DEV_Digital_Write(DEV_DC_PIN,1);
  for(i = 0; i < LCD_WIDTH; i++){
    for(j = 0; j < LCD_HEIGHT; j++){
      LCD_WriteData_Word((Color>>8)&0xff);
      LCD_WriteData_Word(Color);
    }
   }
}

/******************************************************************************
function: Refresh a certain area to the same color
parameter :
    Xstart:   Start UWORD x coordinate
    Ystart:   Start UWORD y coordinate
    Xend  :   End UWORD coordinates
    Yend  :   End UWORD coordinates
    color :   Set the color
******************************************************************************/
void LCD::LCD_ClearWindow(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,UWORD color)
{          
  UWORD i,j; 
  LCD_SetCursor(Xstart, Ystart, Xend-1,Yend-1);
  for(i = Ystart; i <= Yend-1; i++){                                
    for(j = Xstart; j <= Xend-1; j++){
      LCD_WriteData_Word(color);
    }
  }                   
}

/******************************************************************************
function: Set the color of an area
parameter :
    Xstart:   Start UWORD x coordinate
    Ystart:   Start UWORD y coordinate
    Xend  :   End UWORD coordinates
    Yend  :   End UWORD coordinates
    Color :   Set the color
******************************************************************************/
void LCD::LCD_SetWindowColor(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend,UWORD  Color)
{
  LCD_SetCursor( Xstart,Ystart,Xend,Yend);
  LCD_WriteData_Word(Color);      
}

/******************************************************************************
function: Draw a UWORD
parameter :
    X     :   Set the X coordinate
    Y     :   Set the Y coordinate
    Color :   Set the color
******************************************************************************/
void LCD::LCD_SetUWORD(UWORD x, UWORD y, UWORD Color)
{
  LCD_SetCursor(x,y,x,y);
  LCD_WriteData_Word(Color);      
} 
// Fin: Code tire de 'LCD_Driver.cpp'

// Code tire de 'GUI_Paint.cpp'
Paint::Paint()
{
	Serial.printf("Paint::Paint()\n");

  // Initialisation du cache de couleur a 'BLACK'
  Serial.printf("-> Init. cache (%u x %u)\n", LCD_X_SIZE_MAX, LCD_Y_SIZE_MAX);

  for (UWORD y = 0; y < LCD_Y_SIZE_MAX; y++) {
    for (UWORD x = 0; x < LCD_X_SIZE_MAX; x++) {
      m__cache_color[x][y] = BLACK;
    }
  }
}

Paint::~Paint()
{
	Serial.printf("Paint::~GestionLCD()•\n");
}

/******************************************************************************
  function: Create Image
  parameter:
    image   :   Pointer to the image cache
    width   :   The width of the picture
    Height  :   The height of the picture
    Color   :   Whether the picture is inverted
******************************************************************************/
void Paint::Paint_NewImage(UWORD Width, UWORD Height, UWORD Rotate, UWORD Color)
{
  m__paint.WidthMemory = Width;
  m__paint.HeightMemory = Height;
  m__paint.Color = Color;
  m__paint.WidthByte = Width;
  m__paint.HeightByte = Height;
  
  m__paint.Rotate = Rotate;
  m__paint.Mirror = MIRROR_NONE;

  if (Rotate == ROTATE_0 || Rotate == ROTATE_180) {
    m__paint.Width = Width;
    m__paint.Height = Height;
  } else {
    m__paint.Width = Height;
    m__paint.Height = Width;
  }
}

/******************************************************************************
  function: Select Image Rotate
  parameter:
    Rotate   :   0,90,180,270
******************************************************************************/
void Paint::Paint_SetRotate(UWORD Rotate)
{
  if (Rotate == ROTATE_0 || Rotate == ROTATE_90 || Rotate == ROTATE_180 || Rotate == ROTATE_270) {
    //Debug("Set image Rotate %d\r\n", Rotate);
    m__paint.Rotate = Rotate;
  } else {
    //Debug("rotate = 0, 90, 180, 270\r\n");
    //  exit(0);
  }
}

/******************************************************************************
  function: Select Image mirror
  parameter:
    mirror   :       Not mirror,Horizontal mirror,Vertical mirror,Origin mirror
******************************************************************************/
void Paint::Paint_SetMirroring(UBYTE mirror)
{
  if (mirror == MIRROR_NONE || mirror == MIRROR_HORIZONTAL ||
      mirror == MIRROR_VERTICAL || mirror == MIRROR_ORIGIN) {
    //Debug("mirror image x:%s, y:%s\r\n", (mirror & 0x01) ? "mirror" : "none", ((mirror >> 1) & 0x01) ? "mirror" : "none");
    m__paint.Mirror = mirror;
  } else {
    //Debug("mirror should be MIRROR_NONE, MIRROR_HORIZONTAL, MIRROR_VERTICAL or MIRROR_ORIGIN\r\n");
    //exit(0);
  }
}

/******************************************************************************
  function: Draw Pixels
  parameter:
    Xpoint  :   At point X
    Ypoint  :   At point Y
    Color   :   Painted colors
******************************************************************************/
void Paint::Paint_SetPixel(UWORD Xpoint, UWORD Ypoint, UWORD Color)
{
  if (Xpoint > m__paint.Width || Ypoint > m__paint.Height) {
    //Debug("Exceeding display boundaries\r\n");
    return;
  }

  // Use the cache before rotation ;-)
  if (m__cache_color[Xpoint-1][Ypoint-1] == Color) {
    return;
  }
  m__cache_color[Xpoint-1][Ypoint-1] = Color;
  // End: Use the cache before rotation ;-)

  UWORD X, Y;

  switch (m__paint.Rotate) {
    case 0:
      X = Xpoint;
      Y = Ypoint;
      break;
    case 90:
      X = m__paint.WidthMemory - Ypoint - 1;
      Y = Xpoint;
      break;
    case 180:
      X = m__paint.WidthMemory - Xpoint - 1;
      Y = m__paint.HeightMemory - Ypoint - 1;
      break;
    case 270:
      X = Ypoint;
      Y = m__paint.HeightMemory - Xpoint - 1;
      break;

    default:
      return;
  }

  switch (m__paint.Mirror) {
    case MIRROR_NONE:
      break;
    case MIRROR_HORIZONTAL:
      X = m__paint.WidthMemory - X - 1;
      break;
    case MIRROR_VERTICAL:
      Y = m__paint.HeightMemory - Y - 1;
      break;
    case MIRROR_ORIGIN:
      X = m__paint.WidthMemory - X - 1;
      Y = m__paint.HeightMemory - Y - 1;
      break;
    default:
      return;
  }

  // printf("x = %d, y = %d\r\n", X, Y);
  if (X > m__paint.WidthMemory || Y > m__paint.HeightMemory) {
    //Debug("Exceeding display boundaries\r\n");
    return;
  }

  // UDOUBLE Addr = X / 8 + Y * m__paint.WidthByte;
  LCD_SetUWORD(X, Y, Color);
}

/******************************************************************************
  function: Clear the color of the picture
  parameter:
    Color   :   Painted colors
******************************************************************************/
void Paint::Paint_Clear(UWORD Color)
{
  LCD_SetCursor(0, 0, m__paint.WidthByte-1 , m__paint.HeightByte-1);
  for (UWORD Y = 0; Y < m__paint.HeightByte; Y++) {
    for (UWORD X = 0; X < m__paint.WidthByte; X++ ) {//8 pixel =  1 byte
      LCD_WriteData_Word(Color);
    }
  }
}

/******************************************************************************
  function: Clear the color of a window
  parameter:
    Xstart :   x starting point
    Ystart :   Y starting point
    Xend   :   x end point
    Yend   :   y end point
******************************************************************************/
void Paint::Paint_ClearWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, UWORD Color)
{
  UWORD X, Y;
  for (Y = Ystart; Y < Yend; Y++) {
    for (X = Xstart; X < Xend; X++) {//8 pixel =  1 byte
      Paint_SetPixel(X, Y, Color);
    }
  }
}

/******************************************************************************
function:	Draw Point(Xpoint, Ypoint) Fill the color
parameter:
    Xpoint		:   The Xpoint coordinate of the point
    Ypoint		:   The Ypoint coordinate of the point
    Color		:   Set color
    Dot_Pixel	:	point size
******************************************************************************/
void Paint::Paint_DrawPoint( UWORD Xpoint,       UWORD Ypoint, UWORD Color,
                      DOT_PIXEL Dot_Pixel,DOT_STYLE Dot_FillWay)
{
    if (Xpoint > m__paint.Width || Ypoint > m__paint.Height) {
        //Debug("Paint_DrawPoint Input exceeds the normal display range\r\n");
        return;
    }

    int16_t XDir_Num , YDir_Num;
    if (Dot_FillWay == DOT_FILL_AROUND) {
        for (XDir_Num = 0; XDir_Num < 2*Dot_Pixel - 1; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num < 2 * Dot_Pixel - 1; YDir_Num++) {
                if(Xpoint + XDir_Num - Dot_Pixel < 0 || Ypoint + YDir_Num - Dot_Pixel < 0)
                    break;
                // printf("x = %d, y = %d\r\n", Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel);
                Paint_SetPixel(Xpoint + XDir_Num - Dot_Pixel, Ypoint + YDir_Num - Dot_Pixel, Color);
            }
        }
    } else {
        for (XDir_Num = 0; XDir_Num <  Dot_Pixel; XDir_Num++) {
            for (YDir_Num = 0; YDir_Num <  Dot_Pixel; YDir_Num++) {
                Paint_SetPixel(Xpoint + XDir_Num - 1, Ypoint + YDir_Num - 1, Color);
            }
        }
    }
}

/******************************************************************************
function:	Draw a line of arbitrary slope
parameter:
    Xstart ：Starting Xpoint point coordinates
    Ystart ：Starting Xpoint point coordinates
    Xend   ：End point Xpoint coordinate
    Yend   ：End point Ypoint coordinate
    Color  ：The color of the line segment
******************************************************************************/
void Paint::Paint_DrawLine(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, 
                    UWORD Color, DOT_PIXEL Line_width, LINE_STYLE Line_Style)
{
    Serial.printf("Entering in 'Paint_DrawLine(%u, %u, %u, %u, %u, %u, %u)'\n",
    	Xstart, Ystart, Xend, Yend, Color, Line_width, Line_Style);

    if (Xstart > m__paint.Width || Ystart > m__paint.Height ||
        Xend > m__paint.Width || Yend > m__paint.Height) {
        Serial.printf("Paint_DrawLine Input exceeds the normal display range\n");
        return;
    }

    UWORD Xpoint = Xstart;
    UWORD Ypoint = Ystart;
    int dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
    int dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;

    // Increment direction, 1 is positive, -1 is counter;
    int XAddway = Xstart < Xend ? 1 : -1;
    int YAddway = Ystart < Yend ? 1 : -1;

    //Cumulative error
    int Esp = dx + dy;
    char Dotted_Len = 0;

    for (;;) {
        Dotted_Len++;
        //Painted dotted line, 2 point is really virtual
        if (Line_Style == LINE_STYLE_DOTTED && Dotted_Len % 3 == 0) {
            //Debug("LINE_DOTTED\r\n");
            Paint_DrawPoint(Xpoint, Ypoint, IMAGE_BACKGROUND, Line_width, DOT_STYLE_DFT);
            Dotted_Len = 0;
        } else {
            Paint_DrawPoint(Xpoint, Ypoint, Color, Line_width, DOT_STYLE_DFT);
        }
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}

/******************************************************************************
function:	Draw a rectangle
parameter:
    Xstart ：Rectangular  Starting Xpoint point coordinates
    Ystart ：Rectangular  Starting Xpoint point coordinates
    Xend   ：Rectangular  End point Xpoint coordinate
    Yend   ：Rectangular  End point Ypoint coordinate
    Color  ：The color of the Rectangular segment
    Filled : Whether it is filled--- 1 solid 0：empty
******************************************************************************/
void Paint::Paint_DrawRectangle( UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend, 
                          UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Filled )
{
    if (Xstart > m__paint.Width || Ystart > m__paint.Height ||
        Xend > m__paint.Width || Yend > m__paint.Height) {
        //Debug("Input exceeds the normal display range\r\n");
        return;
    }

    if (Filled ) {
        UWORD Ypoint;
        for(Ypoint = Ystart; Ypoint < Yend; Ypoint++) {
            Paint_DrawLine(Xstart, Ypoint, Xend, Ypoint, Color ,Line_width, LINE_STYLE_SOLID);
        }
    } else {
        Paint_DrawLine(Xstart, Ystart, Xend, Ystart, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xstart, Ystart, Xstart, Yend, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xend, Ystart, Color ,Line_width, LINE_STYLE_SOLID);
        Paint_DrawLine(Xend, Yend, Xstart, Yend, Color ,Line_width, LINE_STYLE_SOLID);
    }
}

/******************************************************************************
function:	Use the 8-point method to draw a circle of the
            specified size at the specified position->
parameter:
    X_Center  ：Center X coordinate
    Y_Center  ：Center Y coordinate
    Radius    ：circle Radius
    Color     ：The color of the ：circle segment
    Filled    : Whether it is filled: 1 filling 0：Do not
******************************************************************************/
void Paint::Paint_DrawCircle(  UWORD X_Center, UWORD Y_Center, UWORD Radius, 
                        UWORD Color, DOT_PIXEL Line_width, DRAW_FILL Draw_Fill )
{
    if (X_Center > m__paint.Width || Y_Center >= m__paint.Height) {
        //Debug("Paint_DrawCircle Input exceeds the normal display range\r\n");
        return;
    }

    //Draw a circle from(0, R) as a starting point
    int16_t XCurrent, YCurrent;
    XCurrent = 0;
    YCurrent = Radius;

    //Cumulative error,judge the next point of the logo
    int16_t Esp = 3 - (Radius << 1 );

    int16_t sCountY;
    if (Draw_Fill == DRAW_FILL_FULL) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_DrawPoint(X_Center + XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//1
                Paint_DrawPoint(X_Center - XCurrent, Y_Center + sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//2
                Paint_DrawPoint(X_Center - sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//3
                Paint_DrawPoint(X_Center - sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//4
                Paint_DrawPoint(X_Center - XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//5
                Paint_DrawPoint(X_Center + XCurrent, Y_Center - sCountY, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//6
                Paint_DrawPoint(X_Center + sCountY, Y_Center - XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);//7
                Paint_DrawPoint(X_Center + sCountY, Y_Center + XCurrent, Color, DOT_PIXEL_DFT, DOT_STYLE_DFT);
            }
            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_DrawPoint(X_Center + XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//1
            Paint_DrawPoint(X_Center - XCurrent, Y_Center + YCurrent, Color, Line_width, DOT_STYLE_DFT);//2
            Paint_DrawPoint(X_Center - YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//3
            Paint_DrawPoint(X_Center - YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//4
            Paint_DrawPoint(X_Center - XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//5
            Paint_DrawPoint(X_Center + XCurrent, Y_Center - YCurrent, Color, Line_width, DOT_STYLE_DFT);//6
            Paint_DrawPoint(X_Center + YCurrent, Y_Center - XCurrent, Color, Line_width, DOT_STYLE_DFT);//7
            Paint_DrawPoint(X_Center + YCurrent, Y_Center + XCurrent, Color, Line_width, DOT_STYLE_DFT);//0

            if (Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
  function: Show English characters
  parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint::Paint_DrawChar(UWORD Xpoint, UWORD Ypoint, const char Acsii_Char,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground)
{

  UWORD Page, Column;

  if (Xpoint > m__paint.Width || Ypoint > m__paint.Height) {
    //Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
    return;
  }
  uint32_t Char_Offset = (Acsii_Char - ' ') * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
  const unsigned char *ptr = &Font->table[Char_Offset];

  for ( Page = 0; Page < Font->Height; Page ++ ) {
    for ( Column = 0; Column < Font->Width; Column ++ ) {

      //To determine whether the font background color and screen background color is consistent
      if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
        if (pgm_read_byte(ptr) & (0x80 >> (Column % 8)))
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Foreground );
      } else {
        if (pgm_read_byte(ptr) & (0x80 >> (Column % 8))) {
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Foreground );
        } else {
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Background );
        }
      }
      //One pixel is 8 bits
      if (Column % 8 == 7) {
        ptr++;
      }
    }/* Write a line */
    if (Font->Width % 8 != 0) {
      ptr++;
    }
  }/* Write all */
}

/******************************************************************************
  function: Show Symbol characters
  parameter:
    Xpoint           ：X coordinate
    Ypoint           ：Y coordinate
    Acsii_Char       ：To display the English characters
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint::Paint_DrawSymbol(UWORD Xpoint, UWORD Ypoint, const char Num_Symbol,
                    sFONT* Font, UWORD Color_Background, UWORD Color_Foreground)
{

  UWORD Page, Column;

  if (Xpoint > m__paint.Width || Ypoint > m__paint.Height) {
    //Debug("Paint_DrawChar Input exceeds the normal display range\r\n");
    return;
  }
  uint32_t Char_Offset = Num_Symbol * Font->Height * (Font->Width / 8 + (Font->Width % 8 ? 1 : 0));
  const unsigned char *ptr = &Font->table[Char_Offset];

  for ( Page = 0; Page < Font->Height; Page ++ ) {
    for ( Column = 0; Column < Font->Width; Column ++ ) {

      //To determine whether the font background color and screen background color is consistent
      if (FONT_BACKGROUND == Color_Background) { //this process is to speed up the scan
        if (pgm_read_byte(ptr) & (0x80 >> (Column % 8)))
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Foreground );
      } else {
        if (pgm_read_byte(ptr) & (0x80 >> (Column % 8))) {
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Foreground );
        } else {
          Paint_SetPixel (Xpoint + Column, Ypoint + Page, Color_Background );
        }
      }
      //One pixel is 8 bits
      if (Column % 8 == 7) {
        ptr++;
      }
    }/* Write a line */
    if (Font->Width % 8 != 0) {
      ptr++;
    }
  }/* Write all */
}

/******************************************************************************
  function: Display the string
  parameter:
    Xstart           ：X coordinate
    Ystart           ：Y coordinate
    pString          ：The first address of the English string to be displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
void Paint::Paint_DrawString_EN(UWORD Xstart, UWORD Ystart, const char * pString,
                         sFONT* Font, UWORD Color_Background, UWORD Color_Foreground )
{
  UWORD Xpoint = Xstart;
  UWORD Ypoint = Ystart;

  if (Xstart > m__paint.Width || Ystart > m__paint.Height) {
    //Debug("Paint_DrawString_EN Input exceeds the normal display range\r\n");
    return;
  }

  while (* pString != '\0') {
    //if X direction filled , reposition to(Xstart,Ypoint),Ypoint is Y direction plus the Height of the character
    if ((Xpoint + Font->Width ) > m__paint.Width ) {
      Xpoint = Xstart;
      Ypoint += Font->Height;
    }

    // If the Y direction is full, reposition to(Xstart, Ystart)
    if ((Ypoint  + Font->Height ) > m__paint.Height ) {
      Xpoint = Xstart;
      Ypoint = Ystart;
    }
    Paint_DrawChar(Xpoint, Ypoint, * pString, Font, Color_Background, Color_Foreground);

    //The next character of the address
    pString ++;

    //The next word of the abscissa increases the font of the broadband
    Xpoint += Font->Width;
  }
}

/******************************************************************************
  function: Display nummber
  parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The number displayed
    Font             ：A structure pointer that displays a character size
    Color_Background : Select the background color of the English character
    Color_Foreground : Select the foreground color of the English character
******************************************************************************/
#define  ARRAY_LEN 50
void Paint::Paint_DrawNum(UWORD Xpoint, UWORD Ypoint, int32_t Nummber,
                   sFONT* Font, UWORD Color_Background, UWORD Color_Foreground )
{

  int16_t Num_Bit = 0, Str_Bit = 0;
  uint8_t Str_Array[ARRAY_LEN] = {0}, Num_Array[ARRAY_LEN] = {0};
  uint8_t *pStr = Str_Array;

  if (Xpoint > m__paint.Width || Ypoint > m__paint.Height) {
    //Debug("Paint_DisNum Input exceeds the normal display range\r\n");
    return;
  }

  //Converts a number to a string
  do{
    Num_Array[Num_Bit] = Nummber % 10 + '0';
    Num_Bit++;
    Nummber /= 10;
  }while (Nummber);

  //The string is inverted
  while (Num_Bit > 0) {
    Str_Array[Str_Bit] = Num_Array[Num_Bit - 1];
    Str_Bit ++;
    Num_Bit --;
  }

  //show
  Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Background, Color_Foreground);
}

/******************************************************************************
function:	Display float number
parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    Nummber          : The float data that you want to display
	Decimal_Point	 : Show decimal places
    Font             ：A structure pointer that displays a character size
    Color            : Select the background color of the English character
******************************************************************************/
void Paint::Paint_DrawFloatNum(UWORD Xpoint, UWORD Ypoint, double Nummber,  UBYTE Decimal_Point, 
                        sFONT* Font,  UWORD Color_Background, UWORD Color_Foreground)
{
  char Str[ARRAY_LEN] = {0};
  dtostrf(Nummber,0,Decimal_Point+2,Str);
  char * pStr= (char *)malloc((strlen(Str))*sizeof(char));
  memcpy(pStr,Str,(strlen(Str)-2));
  * (pStr+strlen(Str)-1)='\0';
  if((*(pStr+strlen(Str)-3))=='.')
  {
	*(pStr+strlen(Str)-3)='\0';
  }
  //show
  Paint_DrawString_EN(Xpoint, Ypoint, (const char*)pStr, Font, Color_Foreground, Color_Background);
  free(pStr);
  pStr=NULL;
}

/******************************************************************************
  function: Display time
  parameter:
    Xstart           ：X coordinate
    Ystart           : Y coordinate
    pTime            : Time-related structures
    Font             ：A structure pointer that displays a character size
    Color            : Select the background color of the English character
******************************************************************************/
void Paint::Paint_DrawTime(UWORD Xstart, UWORD Ystart, PAINT_TIME *pTime, sFONT* Font,
                    UWORD Color_Background, UWORD Color_Foreground)
{
  uint8_t value[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

  UWORD Dx = Font->Width;

  //Write data into the cache
  Paint_DrawChar(Xstart                           , Ystart, value[pTime->Hour / 10], Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx                      , Ystart, value[pTime->Hour % 10], Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx  + Dx / 4 + Dx / 2   , Ystart, ':'                    , Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx * 2 + Dx / 2         , Ystart, value[pTime->Min / 10] , Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx * 3 + Dx / 2         , Ystart, value[pTime->Min % 10] , Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx * 4 + Dx / 2 - Dx / 4, Ystart, ':'                    , Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx * 5                  , Ystart, value[pTime->Sec / 10] , Font, Color_Background, Color_Foreground);
  Paint_DrawChar(Xstart + Dx * 6                  , Ystart, value[pTime->Sec % 10] , Font, Color_Background, Color_Foreground);
}

/******************************************************************************
  function: Display image
  parameter:
    image            ：Image start address
    xStart           : X starting coordinates
    yStart           : Y starting coordinates
    xEnd             ：Image width
    yEnd             : Image height
******************************************************************************/
void Paint::Paint_DrawImage(const unsigned char *image, UWORD xStart, UWORD yStart, UWORD W_Image, UWORD H_Image)
{
  int i, j;
  for (j = 0; j < H_Image; j++) {
    for (i = 0; i < W_Image; i++) {
      if (xStart + i < LCD_WIDTH  &&  yStart + j < LCD_HEIGHT) //Exceeded part does not display
        Paint_SetPixel(xStart + i, yStart + j, (pgm_read_byte(image + j * W_Image * 2 + i * 2 + 1)) << 8 | (pgm_read_byte(image + j * W_Image * 2 + i * 2)));
      //Using arrays is a property of sequential storage, accessing the original array by algorithm
      //j*W_Image*2          Y offset
      //i*2                  X offset
      //pgm_read_byte()
    }
  }

}
// Fin: Code tire de 'GUI_Paint.cpp'

GestionLCD::GestionLCD()
{
	Serial.printf("GestionLCD::GestionLCD()\n");
}

GestionLCD::~GestionLCD()
{
	Serial.printf("GestionLCD::~GestionLCD()•\n");
}

