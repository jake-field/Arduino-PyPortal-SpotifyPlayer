#define SERIAL_BAUDRATE 115200 //PyPortal default (9600 or 115200?)

//Pin setups
#define TFT_D0        34 // Data bit 0 pin (MUST be on PORT byte boundary)
#define TFT_WR        26 // Write-strobe pin (CCL-inverted timer output)
#define TFT_DC        10 // Data/command pin
#define TFT_CS        11 // Chip-select pin
#define TFT_RST       24 // Reset pin
#define TFT_RD         9 // Read-strobe pin
#define TFT_BACKLIGHT 25

//TS pins/defines
#define YP A4
#define XM A7
#define YM A6
#define XP A5
#define X_MIN  325
#define X_MAX  750
#define Y_MIN  840
#define Y_MAX  240