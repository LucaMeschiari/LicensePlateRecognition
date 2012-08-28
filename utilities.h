
#define ESC 27

#define PI 3.14159

#define RED_CH 2
#define GREEN_CH 1
#define BLUE_CH 0

#define GETPIXELMACRO( ptr_name, _image, column, row, width_step, pixel_step) \
  unsigned char* ptr_name = (((unsigned char*) _image->imageData)+(row)*(width_step)+(column)*(pixel_step))

#define GETPIXELPTRMACRO( _image, column, row, width_step, pixel_step) \
  (((unsigned char*) _image->imageData)+(row)*(width_step)+(column)*(pixel_step))

#define COPYPIXELMACRO( ptr_name, _image, column, row, width_step, pixel_step, size) \
  memcpy(ptr_name,(unsigned char*) _image->imageData+(row)*(width_step)+(column)*(pixel_step),size)

#define PUTPIXELMACRO( _image, column, row, new_pixel_values, width_step, pixel_step, number_channels) \
  memcpy((_image)->imageData+(row)*(width_step)+(column)*(pixel_step),new_pixel_values,number_channels)

void write_text_on_image(IplImage* image, int top_row, int top_column, char* text);

extern IplImage* image_for_on_mouse_show_values;
extern char* window_name_for_on_mouse_show_values;
void on_mouse_show_values( int event, int x, int y, int flags, void* );
