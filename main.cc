#include "thread.h"
#include "led-matrix.h"

#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "ParticleSys.h"
#include "Particle_Bounce.h"
#include "Emitter_Spin.h"
#include "PartMatrix.h"

using std::min;
using std::max;

const byte numParticles = 60;

// Base-class for a Thread that does something with a matrix.
class RGBMatrixManipulator : public Thread {
public:
  RGBMatrixManipulator(RGBMatrix *m) : running_(true), matrix_(m) {}
  virtual ~RGBMatrixManipulator() { running_ = false; }

  // Run() implementation needs to check running_ regularly.

protected:
  volatile bool running_;  // TODO: use mutex, but this is good enough for now.
  RGBMatrix *const matrix_;
};

// Pump pixels to screen. Needs to be high priority real-time because jitter
// here will make the PWM uneven.
class DisplayUpdater : public RGBMatrixManipulator {
public:
  DisplayUpdater(RGBMatrix *m) : RGBMatrixManipulator(m) {}

  void Run() {
    while (running_) {
      matrix_->UpdateScreen();
    }
  }
};

// -- The following are demo image generators.

// Simple generator that pulses through RGB and White.
class ColorPulseGenerator : public RGBMatrixManipulator {
public:
  ColorPulseGenerator(RGBMatrix *m) : RGBMatrixManipulator(m) {}
  void Run() {
    const int width = matrix_->width();
    const int height = matrix_->height();
    uint32_t count = 0;
    while (running_) {
      usleep(5000);
      ++count;
      int color = (count >> 9) % 6;
      int value = count & 0xFF;
      if (count & 0x100) value = 255 - value;
      int r, g, b;
      switch (color) {
      case 0: r = value; g = b = 0; break;
      case 1: r = g = value; b = 0; break;
      case 2: g = value; r = b = 0; break;
      case 3: g = b = value; r = 0; break;
      case 4: b = value; r = g = 0; break;
      default: r = g = b = value; break;
      }
      for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
          matrix_->SetPixel(x, y, r, g, b);
    }
  }
};

class ParticleSpin: public RGBMatrixManipulator {
public:
	ParticleSpin(RGBMatrix *m) : RGBMatrixManipulator(m), 
		emitter(112, 112, 5, 7), pSys(numParticles, particles, &emitter) {}
	void Run() {
		pMatrix.reset();
		PartMatrix::isOverflow = true;
  
		emitter.oscilate = true;
		
		while (running_) {
			pSys.update();
			drawMatrix();
			usleep(30 * 1000);
		}
	}
private:
	Particle_Bounce particles[numParticles];
	Emitter_Spin emitter;
	ParticleSys pSys;
	PartMatrix pMatrix;
	/**
	 * Render the particles into a low-resolution matrix
	 */
	void drawMatrix(){
		pMatrix.fade();
		pMatrix.render(particles, numParticles);
		//update the actual LED matrix
		for (byte y=0;y<PS_PIXELS_Y;y++) {
			for(byte x=0;x<PS_PIXELS_X;x++) {
				matrix_->SetPixel(x, y, pMatrix.matrix[x][y].r, pMatrix.matrix[x][y].g, pMatrix.matrix[x][y].b);
			}
		}
	}
};

class Plasma : public RGBMatrixManipulator {
public:
	Plasma(RGBMatrix *m) : RGBMatrixManipulator(m) {}
	void Run() {
	  paletteShift=128000;
	  unsigned char bcolor;

	  //generate the plasma once
	  for(unsigned char y = 0; y < matrix_->height(); y++)
		for(unsigned char x = 0; x < matrix_->width(); x++)
		{
		  //the plasma buffer is a sum of sines
		  bcolor = (unsigned char)
		  (
				128.0 + (128.0 * sin(x*8.0 / 16.0))
			  + 128.0 + (128.0 * sin(y*8.0 / 16.0))
		  ) / 2;
		  plasma[x][y] = bcolor;
		}
		
		 // to adjust white balance you can uncomment this line
		 // and comment out the plasma_morph() in loop()
		 // and then experiment with whiteBalVal above
		 // ColorFill(255,255,255);
		 
		while(running_) { 
			plasma_morph();
			usleep(15 * 1000);
		}
	}
private:
	typedef struct
	{
	  unsigned char r;
	  unsigned char g;
	  unsigned char b;
	} ColorRGB;

	//a color with 3 components: h, s and v
	typedef struct 
	{
	  unsigned char h;
	  unsigned char s;
	  unsigned char v;
	} ColorHSV;
	unsigned char plasma[64][16];
	long paletteShift;


	//Converts an HSV color to RGB color
	void HSVtoRGB(void *vRGB, void *vHSV) 
	{
	  float r, g, b, h, s, v; //this function works with floats between 0 and 1
	  float f, p, q, t;
	  int i;
	  ColorRGB *colorRGB=(ColorRGB *)vRGB;
	  ColorHSV *colorHSV=(ColorHSV *)vHSV;

	  h = (float)(colorHSV->h / 256.0);
	  s = (float)(colorHSV->s / 256.0);
	  v = (float)(colorHSV->v / 256.0);

	  //if saturation is 0, the color is a shade of grey
	  if(s == 0.0) {
		b = v;
		g = b;
		r = g;
	  }
	  //if saturation > 0, more complex calculations are needed
	  else
	  {
		h *= 6.0; //to bring hue to a number between 0 and 6, better for the calculations
		i = (int)(floor(h)); //e.g. 2.7 becomes 2 and 3.01 becomes 3 or 4.9999 becomes 4
		f = h - i;//the fractional part of h

		p = (float)(v * (1.0 - s));
		q = (float)(v * (1.0 - (s * f)));
		t = (float)(v * (1.0 - (s * (1.0 - f))));

		switch(i)
		{
		  case 0: r=v; g=t; b=p; break;
		  case 1: r=q; g=v; b=p; break;
		  case 2: r=p; g=v; b=t; break;
		  case 3: r=p; g=q; b=v; break;
		  case 4: r=t; g=p; b=v; break;
		  case 5: r=v; g=p; b=q; break;
		  default: r = g = b = 0; break;
		}
	  }
	  colorRGB->r = (int)(r * 255.0);
	  colorRGB->g = (int)(g * 255.0);
	  colorRGB->b = (int)(b * 255.0);
	}

	float
	dist(float a, float b, float c, float d) 
	{
	  return sqrt((c-a)*(c-a)+(d-b)*(d-b));
	}


	void
	plasma_morph()
	{
	  unsigned char x,y;
	  float value;
	  ColorRGB colorRGB;
	  ColorHSV colorHSV;

	  for(y = 0; y < matrix_->height(); y++)
		for(x = 0; x < matrix_->width(); x++) {
		  {
		value = sin(dist(x + paletteShift, y, 128.0, 128.0) / 8.0)
		  + sin(dist(x, y, 64.0, 64.0) / 8.0)
		  + sin(dist(x, y + paletteShift / 7, 192.0, 64) / 7.0)
		  + sin(dist(x, y, 192.0, 100.0) / 8.0);
		colorHSV.h=(unsigned char)((value) * 128)&0xff;
		colorHSV.s=255; 
		colorHSV.v=255;
		HSVtoRGB(&colorRGB, &colorHSV);
		
		matrix_->SetPixel(x, y, colorRGB.r, colorRGB.g, colorRGB.b);
		  }
	  }
	  paletteShift++;
	}

	/********************************************************
	Name: ColorFill
	Function: Fill the frame with a color
	Parameter:R: the value of RED.   Range:RED 0~255
			  G: the value of GREEN. Range:RED 0~255
			  B: the value of BLUE.  Range:RED 0~255
	********************************************************/
	void ColorFill(unsigned char R,unsigned char G,unsigned char B)
	{
	  for (unsigned char y=0;y<matrix_->width();y++) {
		for(unsigned char x=0;x<matrix_->height();x++) {
			matrix_->SetPixel(x, y, R, G, B);
		}
	  }
	  
	}
};

class SimpleSquare : public RGBMatrixManipulator {
public:
  SimpleSquare(RGBMatrix *m) : RGBMatrixManipulator(m) {}
  void Run() {
    const int width = matrix_->width();
    const int height = matrix_->height();
    // Diagonaly
    for (int x = 0; x < width; ++x) {
        matrix_->SetPixel(x, x, 255, 255, 255);
        matrix_->SetPixel(height -1 - x, x, 255, 0, 255);
    }
    for (int x = 0; x < width; ++x) {
      matrix_->SetPixel(x, 0, 255, 0, 0);
      matrix_->SetPixel(x, height - 1, 255, 255, 0);
    }
    for (int y = 0; y < height; ++y) {
      matrix_->SetPixel(0, y, 0, 0, 255);
      matrix_->SetPixel(width - 1, y, 0, 255, 0);
    }
  }
};

// Simple class that generates a rotating block on the screen.
class RotatingBlockGenerator : public RGBMatrixManipulator {
public:
  RotatingBlockGenerator(RGBMatrix *m) : RGBMatrixManipulator(m) {}

  uint8_t scale_col(int val, int lo, int hi) {
    if (val < lo) return 0;
    if (val > hi) return 255;
    return 255 * (val - lo) / (hi - lo);
  }

  void Run() {
    const int cent_x = matrix_->width() / 2;
    const int cent_y = matrix_->height() / 2;

    // The square to rotate (inner square + black frame) needs to cover the
    // whole area, even if diagnoal.
    const int rotate_square = min(matrix_->width(), matrix_->height()) * 1.41;
    const int min_rotate = cent_x - rotate_square / 2;
    const int max_rotate = cent_x + rotate_square / 2;

    // The square to display is within the visible area.
    const int display_square = min(matrix_->width(), matrix_->height()) * 0.7;
    const int min_display = cent_x - display_square / 2;
    const int max_display = cent_x + display_square / 2;

    const float deg_to_rad = 2 * 3.14159265 / 360;
    int rotation = 0;
    while (running_) {
      ++rotation;
      usleep(15 * 1000);
      rotation %= 360;
      for (int x = min_rotate; x < max_rotate; ++x) {
        for (int y = min_rotate; y < max_rotate; ++y) {
          float disp_x, disp_y;
          Rotate(x - cent_x, y - cent_y,
                 deg_to_rad * rotation, &disp_x, &disp_y);
          if (x >= min_display && x < max_display &&
              y >= min_display && y < max_display) { // within display square
            matrix_->SetPixel(disp_x + cent_x, disp_y + cent_y,
                              scale_col(x, min_display, max_display),
                              255 - scale_col(y, min_display, max_display),
                              scale_col(y, min_display, max_display));
          } else {
            // black frame.
            matrix_->SetPixel(disp_x + cent_x, disp_y + cent_y, 0, 0, 0);
          }
        }
      }
    }
  }

private:
  void Rotate(int x, int y, float angle,
              float *new_x, float *new_y) {
    *new_x = x * cosf(angle) - y * sinf(angle);
    *new_y = x * sinf(angle) + y * cosf(angle);
  }
};

class ImageScroller : public RGBMatrixManipulator {
public:
  ImageScroller(RGBMatrix *m)
    : RGBMatrixManipulator(m), image_(NULL), horizontal_position_(0) {
  }

  // _very_ simplified. Can only read binary P6 PPM. Expects newlines in headers
  // Not really robust. Use at your own risk :)
  bool LoadPPM(const char *filename) {
    if (image_) {
      delete [] image_;
      image_ = NULL;
    }
    FILE *f = fopen(filename, "r");
    if (f == NULL) return false;
    char header_buf[256];
    const char *line = ReadLine(f, header_buf, sizeof(header_buf));
#define EXIT_WITH_MSG(m) { fprintf(stderr, "%s: %s |%s", filename, m, line); \
      fclose(f); return false; }
    if (sscanf(line, "P6 ") == EOF)
      EXIT_WITH_MSG("Can only handle P6 as PPM type.");
    line = ReadLine(f, header_buf, sizeof(header_buf));
    if (!line || sscanf(line, "%d %d ", &width_, &height_) != 2)
      EXIT_WITH_MSG("Width/height expected");
    int value;
    line = ReadLine(f, header_buf, sizeof(header_buf));
    if (!line || sscanf(line, "%d ", &value) != 1 || value != 255)
      EXIT_WITH_MSG("Only 255 for maxval allowed.");
    const size_t pixel_count = width_ * height_;
    image_ = new Pixel [ pixel_count ];
    assert(sizeof(Pixel) == 3);   // we make that assumption.
    if (fread(image_, sizeof(Pixel), pixel_count, f) != pixel_count) {
      line = "";
      EXIT_WITH_MSG("Not enough pixels read.");
    }
#undef EXIT_WITH_MSG
    fclose(f);
    fprintf(stderr, "Read image with %dx%d\n", width_, height_);
    horizontal_position_ = 0;
    return true;
  }

  void Run() {
    const int screen_height = matrix_->height();
    const int screen_width = matrix_->width();
    while (running_) {
      if (image_ == NULL) {
        usleep(100 * 1000);
        continue;
      }
      usleep(30 * 1000);
      for (int x = 0; x < screen_width; ++x) {
        for (int y = 0; y < screen_height; ++y) {
          const Pixel &p = getPixel((horizontal_position_ + x) % width_, y);
          // Display upside down on my desk. Lets flip :)
          int disp_x = screen_width - x;
          int disp_y = screen_height - y;
          matrix_->SetPixel(disp_x, disp_y, p.red, p.green, p.blue);
        }
      }
      ++horizontal_position_;
    }
  }

private:
  struct Pixel {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
  };

  // Read line, skip comments.
  char *ReadLine(FILE *f, char *buffer, size_t len) {
    char *result;
    do {
      result = fgets(buffer, len, f);
    } while (result != NULL && result[0] == '#');
    return result;
  }

  const Pixel &getPixel(int x, int y) {
    static Pixel dummy;
    if (x < 0 || x > width_ || y < 0 || y > height_) return dummy;
    return image_[x + width_ * y];
  }

  int width_;
  int height_;
  Pixel *image_;
  uint32_t horizontal_position_;
};

int main(int argc, char *argv[]) {
  int demo = 0;
  if (argc > 1) {
    demo = atoi(argv[1]);
  }
  fprintf(stderr, "Using demo %d\n", demo);

  GPIO io;
  if (!io.Init())
    return 1;

  RGBMatrix m(&io);
    
  RGBMatrixManipulator *image_gen = NULL;
  switch (demo) {
  case 0:
    image_gen = new RotatingBlockGenerator(&m);
    break;

  case 1:
    if (argc > 2) {
      ImageScroller *scroller = new ImageScroller(&m);
      if (!scroller->LoadPPM(argv[2]))
        return 1;
      image_gen = scroller;
    } else {
      fprintf(stderr, "Demo %d Requires PPM image as parameter", demo);
      return 1;
    }
    break;

  case 2:
    image_gen = new SimpleSquare(&m);
    break;

  case 3:
	image_gen = new ParticleSpin(&m);
	break;
	
  case 4:
	image_gen = new Plasma(&m);
	break;
	
  default:
    image_gen = new ColorPulseGenerator(&m);
    break;
  }

  if (image_gen == NULL)
    return 1;

  RGBMatrixManipulator *updater = new DisplayUpdater(&m);
  updater->Start(10);  // high priority

  image_gen->Start();

  // Things are set up. Just wait for <RETURN> to be pressed.
  printf("Press <RETURN> to exit and reset LEDs\n");
  getchar();

  // Stopping threads and wait for them to join.
  delete image_gen;
  delete updater;

  // Final thing before exit: clear screen and update once, so that
  // we don't have random pixels burn
  m.ClearScreen();
  m.UpdateScreen();

  return 0;
}
