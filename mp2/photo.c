/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/*My struct*/

/* 
 * One node of the second of fourth level of the Octree. 
 * It will store a the index of RR-GG-BB or RRRR-GGGG-BBBB color.
 * the number of this color in a photo 
 * the index after the quicksort
 * the sum of each this color in red, green, blue seperately 
 * the average color of the corrosponding color
 */
typedef struct Octree_node{
	uint32_t RGB;
	uint32_t index_after_qs;
	uint32_t count;
	uint32_t Sum_R, Sum_G, Sum_B;
}Octree_node;


/*
 * The whole Octree structure. It contains
 * a level2 array of 64 L2 nodes, and
 * a level4 array of 4096 L4 nodes.
 */
typedef struct Octree{
	Octree_node L2[L2_Size];
	Octree_node L4[L4_Size];
}Octree;


/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 


/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
	set_palette(room_photo(r)->palette);
    /* Record the current room. */
    cur_room = r;
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB. You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */



    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

	/*We set up and initialize a Octree before we iterate the whole photo*/
	Octree The_tree = init_Octree();
	uint32_t color12;
	uint32_t color6;
	uint32_t L4_index;
	uint32_t Red;
	uint32_t Green;
	uint32_t Blue;
	/*creat an array contains each pixel. */
	uint32_t img_array[p->hdr.width*p->hdr.height];
    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = p->hdr.height; y-- > 0; ) {
	/* Loop over columns from left to right. */
	for (x = 0; p->hdr.width > x; x++) {

	    /* 
	     * Try to read one 16-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (p->img);
		free (p);
	        (void)fclose (in);
		return NULL;
	    }

		/* store each pixel*/
		img_array[p->hdr.width * y + x] = pixel;

		/* conver the pixel color from 16 to 12*/
		color12 = convert16_12(pixel);

		/* Get R(5bits),G(6bits),B(5bits) seperately from the 16 bits color
		   and set them all to 6 bit type*/
		Red = Red_of_16(pixel);
		Green = Green_of_16(pixel);
		Blue = Blue_of_16(pixel);

		/* Update the data in the Octree*/
		The_tree.L4[color12].count +=1;
		The_tree.L4[color12].Sum_R += Red;
		The_tree.L4[color12].Sum_G += Green;
		The_tree.L4[color12].Sum_B += Blue;


	    /* 
	     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
	     * and 6 bits blue).  We change to 2:2:2, which we've set for the
	     * game objects.  You need to use the other 192 palette colors
	     * to specialize the appearance of each photo.
	     *
	     * In this code, you need to calculate the p->palette values,
	     * which encode 6-bit RGB as arrays of three uint8_t's.  When
	     * the game puts up a photo, you should then change the palette 
	     * to match the colors needed for that photo.
	     */
		/*We don't need the old color anymore since we have an optimized palette*/
	    // p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
		// 			    (((pixel >> 9) & 0x3) << 2) |
		// 			    ((pixel >> 3) & 0x3));
	}
    }

	/* use quick sort method to sort the L4 level*/
	qsort((Octree_node*)The_tree.L4,L4_Size,sizeof(The_tree.L4[0]),comparator);

	int i;
	uint32_t L2_index;
	for (i=0; i<L4_Size;i++){
		/* Record the index after the quick sort. */
		The_tree.L4[The_tree.L4[i].RGB].index_after_qs = i;
		/* Set up the first 128 color of the palette (L4)*/
		if(i < Max_L4){
			/* If the color is not been used, then continue*/
			if (The_tree.L4[i].count ==0){
				break;
			}
			p->palette[i][0] = The_tree.L4[i].Sum_R / The_tree.L4[i].count;
			p->palette[i][1] = The_tree.L4[i].Sum_G / The_tree.L4[i].count;
			p->palette[i][2] = The_tree.L4[i].Sum_B / The_tree.L4[i].count;
		}
		/* Use the remaining L4 color to set up L2*/
		else{
			L2_index = convert12_6(The_tree.L4[i].RGB);
			The_tree.L2[L2_index].Sum_R += The_tree.L4[i].Sum_R;
			The_tree.L2[L2_index].Sum_G += The_tree.L4[i].Sum_G;
			The_tree.L2[L2_index].Sum_B += The_tree.L4[i].Sum_B;
			The_tree.L2[L2_index].count += The_tree.L4[i].count;
		}
	}

	/* Set up the last 64 color of the palette (L2)*/
	for (i=0; i<L2_Size; i++){
		/* If the color is not been used, then continue*/
		if(The_tree.L2[i].count ==0 ){
			continue;
		}
		p->palette[i+Max_L4][0] = The_tree.L2[i].Sum_R / The_tree.L2[i].count;
		p->palette[i+Max_L4][1] = The_tree.L2[i].Sum_G / The_tree.L2[i].count;
		p->palette[i+Max_L4][2] = The_tree.L2[i].Sum_B / The_tree.L2[i].count;
	}

	for (y = p->hdr.height; y-- > 0; ) {
		/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) {
			/*get the pixel we stored earlier*/
			pixel = img_array[p->hdr.width * y + x];

			/* convert the pixel into 12 and 6 bit color type*/
			color12 = convert16_12(pixel);
			color6 = convert12_6(color12);

			L4_index = The_tree.L4[color12].index_after_qs;

			/* If the index of the color is less than 128, we need find the color
				in the range of 64 ~ 193, otherwise, find in the range of 194 ~ 255*/
			if(L4_index < Max_L4){
				p->img[p->hdr.width * y + x] = (char)(base_color_count+L4_index);
			}else{
				p->img[p->hdr.width * y + x] = (char)(base_color_count+Max_L4+color6);
			}
		}
	}

    /* All done.  Return success. */
    (void)fclose (in);
    return p;
}


/* 
 * comparator
 *   DESCRIPTION: The comparator for the quick sort funciton. There
 * 				  we define a descending order.
 *   INPUTS:  p, q default pointers. There they will represent
 * 			  two pointer to the structure of Octree_node
 *   OUTPUTS: none
 *   RETURN VALUE: q_count - p_count
 *   SIDE EFFECTS: It will set the comparator for the qsort function
 * 				   which will then used to sort the L4 nodes.
 */
int comparator(const void *p, const void *q){
	int p_count = ((Octree_node*)p) -> count;
	int q_count = ((Octree_node*)q) -> count;
	return (q_count - p_count);
}

/* 
 * init_Octree
 *   DESCRIPTION: Initialize the Octree we will use later
 * 				  set each level to the initial value, sum of each
 * 				  R,G,B to 0 and count number to 0 as well 
 *   INPUTS:  none
 *   OUTPUTS: none
 *   RETURN VALUE: Octree, a initialized Octree structure
 *   SIDE EFFECTS: set up a initialized Octree for later use.
 */
Octree init_Octree(){
	Octree The_tree;
	int i;
	/* Initialize the level 2 of Octree*/
	for(i=0; i < L2_Size; i++){
		The_tree.L2[i].count = 0;
		The_tree.L2[i].Sum_R = 0;
		The_tree.L2[i].Sum_G = 0;
		The_tree.L2[i].Sum_B = 0;
		The_tree.L2[i].RGB = i;
	}
	/* Initialize the level 4 of Octree*/
	for(i=0; i < L4_Size; i++){
		The_tree.L4[i].count = 0;
		The_tree.L4[i].Sum_R = 0;
		The_tree.L4[i].Sum_G = 0;
		The_tree.L4[i].Sum_B = 0;
		The_tree.L4[i].RGB = i;
	}
	return The_tree;
}

/* 
 * convert16_12
 *   DESCRIPTION: conver a 16-RGB into a 12-bit RGB 
 *   INPUTS:  pixel - a RRRRR-GGGGGG-BBBBB (16bit) data of a color
 *   OUTPUTS: none
 *   RETURN VALUE: a RRRR-GGGG-BBBB (12bit) data of a clolor corrosponding to the input color
 *   SIDE EFFECTS: conver the pixel color into the L4 type of the Octree data
 */
int convert16_12(int pixel){
	/* Get the first 4 bits for each color*/
	int Red = (pixel >> Red_16to12_off) & four_mask;
	int Green = (pixel >> Green_16to12_off) & four_mask;
	int Blue = (pixel >> Blue_16to12_off) & four_mask;
	Red <<= Red_12off;
	Green <<= Green_12off;
	return (Red|Green|Blue);
}

/* 
 * convert12_6
 *   DESCRIPTION: conver a 12-RGB into a 6-bit RGB 
 *   INPUTS:  pixel - a RRRR-GGGG-BBBB (12bit) data of a color
 *   OUTPUTS: none
 *   RETURN VALUE: a RR-GG-BB (6bit) data of a clolor corrosponding to the input color
 *   SIDE EFFECTS: conver the pixel color into the L2 type of the Octree data
 */
int convert12_6(int pixel){
	/* Get the first 4 bits for each color*/
	int Red = (pixel >> Red_12to6_off) & two_mask;
	int Green = (pixel >> Green_12to6_off) & two_mask;
	int Blue = (pixel >> Blue_12to6_off) & two_mask;
	Red <<= Red_6off;
	Green <<= Green_6off;
	return (Red|Green|Blue);
}

/* 
 * Red_of_16
 *   DESCRIPTION: Get the red color from a 16bit type color
 *   INPUTS:  pixel - a RRRRR-GGGGGG-BBBBB (16bit) data of a color
 *   OUTPUTS: none
 *   RETURN VALUE: Red- the red part of the 16bit data of the color
 * 						it should be 6 bits - XXXXX0
 *   SIDE EFFECTS: none
 */
int Red_of_16(int pixel){
	/* Get the red color(5 bits) of the 16 bits color 
	   and change it to 6 bits*/
	int Red = ((pixel >> Red_16off) & five_mask)<<1;
	return (Red);
}

/* 
 * Green_of_16
 *   DESCRIPTION: Get the green color from a 16bit type color
 *   INPUTS:  pixel - a RRRRR-GGGGGG-BBBBB (16bit) data of a color
 *   OUTPUTS: none
 *   RETURN VALUE: green- the green part of the 16bit data of the color
 * 				   It should be 6 bits - XXXXXX
 *   SIDE EFFECTS: none
 */
int Green_of_16(int pixel){
	/* Get the green color of the 16 bits color */
	int green = (pixel >> Green_16off) & six_mask;
	return (green);
}

/* 
 * Blue_of_16
 *   DESCRIPTION: Get the blue color from a 16bit type color
 *   INPUTS:  pixel - a RRRRR-GGGGGG-BBBBB (16bit) data of a color
 *   OUTPUTS: none
 *   RETURN VALUE: blue- the blue part of the 16bit data of the color
 * 				   It should be 6 bits - XXXXX0
 *   SIDE EFFECTS: none
 */
int Blue_of_16(int pixel){
	/* Get the green color(5 bits) of the 16 bits color 
	   and change it to 6 bits*/
	int blue = (pixel & five_mask)<<1;
	return (blue);
}



