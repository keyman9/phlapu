/*
    FlappyNerd
    Author: Spencer Scott
*/

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 160

/* include the background image we are using */
#include "background.h"

/* include the sprite image we are using */
#include "dragon.h"

/* include the ground layer map we are using */
#include "groundlayermap.h"     //bg1

/* include the background tile map */
#include "layer0map.h"          //bg0

/* include the the score tile map */
#include "score.h"              //bg2

/* the tile mode flags needed for display control register */
#define MODE0 0x00
#define MODE1 0x01
#define MODE2 0x02
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400    
#define BG3_ENABLE 0x800    


/* flags to set sprite handling in display control register */
#define SPRITE_MAP_2D 0x0
#define SPRITE_MAP_1D 0x40
#define SPRITE_ENABLE 0x1000


/* the control registers for the four tile layers */
volatile unsigned short* bg0_control = (volatile unsigned short*) 0x4000008;
volatile unsigned short* bg1_control = (volatile unsigned short*) 0x400000a;
volatile unsigned short* bg2_control = (volatile unsigned short*) 0x400000c;
volatile unsigned short* bg3_control = (volatile unsigned short*) 0x400000e;


/* palette is always 256 colors */
#define PALETTE_SIZE 256

/* there are 128 sprites on the GBA */
#define NUM_SPRITES 128

/* the display control pointer points to the gba graphics register */
volatile unsigned long* display_control = (volatile unsigned long*) 0x4000000;

/* the memory location which controls sprite attributes */
volatile unsigned short* sprite_attribute_memory = (volatile unsigned short*) 0x7000000;

/* the memory location which stores sprite image data */
volatile unsigned short* sprite_image_memory = (volatile unsigned short*) 0x6010000;

/* the address of the color palettes used for backgrounds and sprites */
volatile unsigned short* bg_palette = (volatile unsigned short*) 0x5000000;
volatile unsigned short* sprite_palette = (volatile unsigned short*) 0x5000200;

/* the button register holds the bits which indicate whether each button has
 * been pressed - this has got to be volatile as well
 */
volatile unsigned short* buttons = (volatile unsigned short*) 0x04000130;

/* scrolling registers for backgrounds */
volatile short* bg0_x_scroll = (unsigned short*) 0x4000010;
volatile short* bg0_y_scroll = (unsigned short*) 0x4000012;
volatile short* bg1_x_scroll = (unsigned short*) 0x4000014;
volatile short* bg1_y_scroll = (unsigned short*) 0x4000016;
volatile short* bg2_x_scroll = (unsigned short*) 0x4000018;
volatile short* bg2_y_scroll = (unsigned short*) 0x400001a;
volatile short* bg3_x_scroll = (unsigned short*) 0x400001c;
volatile short* bg3_y_scroll = (unsigned short*) 0x400001e;

/* the bit positions indicate each button - the first bit is for A, second for
 * B, and so on, each constant below can be ANDED into the register to get the
 * status of any one button */
#define BUTTON_A (1 << 0)
#define BUTTON_B (1 << 1)
#define BUTTON_SELECT (1 << 2)
#define BUTTON_START (1 << 3)
#define BUTTON_RIGHT (1 << 4)
#define BUTTON_LEFT (1 << 5)
#define BUTTON_UP (1 << 6)
#define BUTTON_DOWN (1 << 7)
#define BUTTON_R (1 << 8)
#define BUTTON_L (1 << 9)

/* the scanline counter is a memory cell which is updated to indicate how
 * much of the screen has been drawn */
volatile unsigned short* scanline_counter = (volatile unsigned short*) 0x4000006;

/* wait for the screen to be fully drawn so we can do something during vblank */
void wait_vblank( ) {
    /* wait until all 160 lines have been updated */
    while (*scanline_counter < 160) { }
}

/* this function checks whether a particular button has been pressed */
unsigned char button_pressed(unsigned short button) {
    /* and the button register with the button constant we want */
    unsigned short pressed = *buttons & button;

    /* if this value is zero, then it's not pressed */
    if (pressed == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* return a pointer to one of the 4 character blocks (0-3) */
volatile unsigned short* char_block(unsigned long block) {
    /* they are each 16K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x4000));
}

/* return a pointer to one of the 32 screen blocks (0-31) */
volatile unsigned short* screen_block(unsigned long block) {
    /* they are each 2K big */
    return (volatile unsigned short*) (0x6000000 + (block * 0x800));
}

/* flag for turning on DMA */
#define DMA_ENABLE 0x80000000

/* flags for the sizes to transfer, 16 or 32 bits */
#define DMA_16 0x00000000
#define DMA_32 0x04000000

/* pointer to the DMA source location */
volatile unsigned int* dma_source = (volatile unsigned int*) 0x40000D4;

/* pointer to the DMA destination location */
volatile unsigned int* dma_destination = (volatile unsigned int*) 0x40000D8;

/* pointer to the DMA count/control */
volatile unsigned int* dma_count = (volatile unsigned int*) 0x40000DC;

/* copy data using DMA */
void memcpy16_dma(unsigned short* dest, unsigned short* source, int amount) {
    *dma_source = (unsigned int) source;
    *dma_destination = (unsigned int) dest;
    *dma_count = amount | DMA_16 | DMA_ENABLE;
}

/* function to setup background 0 for this program */
void setup_background() {

    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) bg_palette, (unsigned short*) background_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) char_block(0), (unsigned short*) background_data,
            (background_width * background_height) / 2);

    /* back */
    *bg0_control = 2 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (16 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    /* set all control bits in 

    /* set all control the bits in ground layer */
    *bg1_control = 1 |    /* priority, 0 is highest, 3 is lowest */
        (0 << 2)  |       /* the char block the image data is stored in */
        (0 << 6)  |       /* the mosaic flag */
        (1 << 7)  |       /* color mode, 0 is 16 colors, 1 is 256 colors */
        (21 << 8) |       /* the screen block the tile data is stored in */
        (1 << 13) |       /* wrapping flag */
        (0 << 14);        /* bg size, 0 is 256x256 */

    /* score page */
    *bg2_control = 0 |
        (0 << 2) |
        (0 << 6) |
        (1 << 7) |
        (26 << 8)|
        (1 << 13) |
        (0 << 14);

    /* load the score page */
    memcpy16_dma((unsigned short*) screen_block(26), (unsigned short*) score, score_width * score_height);

    /* load the ground layer tile data into screen block 21 */
    memcpy16_dma((unsigned short*) screen_block(21), (unsigned short*) groundlayermap, groundlayermap_width * groundlayermap_height);

    /* load in back layer */
    memcpy16_dma((unsigned short*) screen_block(16), (unsigned short*)
        layer0map, layer0map_width * layer0map_height);
}

/* just kill time */
void delay(unsigned int amount) {
    for (int i = 0; i < amount * 10; i++);
}

///////////////////Sprites

/* a sprite is a moveable image on the screen */
struct Sprite {
    unsigned short attribute0;
    unsigned short attribute1;
    unsigned short attribute2;
    unsigned short attribute3;
};

/* array of all the sprites available on the GBA */
struct Sprite sprites[NUM_SPRITES];
int next_sprite_index = 0;

/* the different sizes of sprites which are possible */
enum SpriteSize {
    SIZE_8_8,
    SIZE_16_16,
    SIZE_32_32,
    SIZE_64_64,
    SIZE_16_8,
    SIZE_32_8,
    SIZE_32_16,
    SIZE_64_32,
    SIZE_8_16,
    SIZE_8_32,
    SIZE_16_32,
    SIZE_32_64
};

/* function to initialize a sprite with its properties, and return a pointer */
struct Sprite* sprite_init(int x, int y, enum SpriteSize size,
        int horizontal_flip, int vertical_flip, int tile_index, int priority) {

    /* grab the next index */
    int index = next_sprite_index++;

    /* setup the bits used for each shape/size possible */
    int size_bits, shape_bits;
    switch (size) {
        case SIZE_8_8:   size_bits = 0; shape_bits = 0; break;
        case SIZE_16_16: size_bits = 1; shape_bits = 0; break;
        case SIZE_32_32: size_bits = 2; shape_bits = 0; break;
        case SIZE_64_64: size_bits = 3; shape_bits = 0; break;
        case SIZE_16_8:  size_bits = 0; shape_bits = 1; break;
        case SIZE_32_8:  size_bits = 1; shape_bits = 1; break;
        case SIZE_32_16: size_bits = 2; shape_bits = 1; break;
        case SIZE_64_32: size_bits = 3; shape_bits = 1; break;
        case SIZE_8_16:  size_bits = 0; shape_bits = 2; break;
        case SIZE_8_32:  size_bits = 1; shape_bits = 2; break;
        case SIZE_16_32: size_bits = 2; shape_bits = 2; break;
        case SIZE_32_64: size_bits = 3; shape_bits = 2; break;
    }

    int h = horizontal_flip ? 1 : 0;
    int v = vertical_flip ? 1 : 0;

    /* set up the first attribute */
    sprites[index].attribute0 = y |             /* y coordinate */
                            (0 << 8) |          /* rendering mode */
                            (0 << 10) |         /* gfx mode */
                            (0 << 12) |         /* mosaic */
                            (1 << 13) |         /* color mode, 0:16, 1:256 */
                            (shape_bits << 14); /* shape */

    /* set up the second attribute */
    sprites[index].attribute1 = x |             /* x coordinate */
                            (0 << 9) |          /* affine flag */
                            (h << 12) |         /* horizontal flip flag */
                            (v << 13) |         /* vertical flip flag */
                            (size_bits << 14);  /* size */

    /* setup the second attribute */
    sprites[index].attribute2 = tile_index |   // tile index */
                            (priority << 10) | // priority */
                            (0 << 12);         // palette bank (only 16 color)*/

    /* return pointer to this sprite */
    return &sprites[index];
}

/* update all of the spries on the screen */
void sprite_update_all() {
    /* copy them all over */
    memcpy16_dma((unsigned short*) sprite_attribute_memory, (unsigned short*) sprites, NUM_SPRITES * 4);
}

/* setup all sprites */
void sprite_clear() {
    /* clear the index counter */
    next_sprite_index = 0;

    /* move all sprites offscreen to hide them */
    for(int i = 0; i < NUM_SPRITES; i++) {
        sprites[i].attribute0 = SCREEN_HEIGHT;
        sprites[i].attribute1 = SCREEN_WIDTH;
    }
}

/* set a sprite postion */
void sprite_position(struct Sprite* sprite, int x, int y) {
    /* clear out the y coordinate */
    sprite->attribute0 &= 0xff00;

    /* set the new y coordinate */
    sprite->attribute0 |= (y & 0xff);

    /* clear out the x coordinate */
    sprite->attribute1 &= 0xfe00;

    /* set the new x coordinate */
    sprite->attribute1 |= (x & 0x1ff);
}

/* move a sprite in a direction */
void sprite_move(struct Sprite* sprite, int dx, int dy) {
    /* get the current y coordinate */
    int y = sprite->attribute0 & 0xff;

    /* get the current x coordinate */
    int x = sprite->attribute1 & 0x1ff;

    /* move to the new location */
    sprite_position(sprite, x + dx, y + dy);
}

/* change the vertical flip flag */
void sprite_set_vertical_flip(struct Sprite* sprite, int vertical_flip) {
    if (vertical_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x2000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xdfff;
    }
}

/* change the vertical flip flag */
void sprite_set_horizontal_flip(struct Sprite* sprite, int horizontal_flip) {
    if (horizontal_flip) {
        /* set the bit */
        sprite->attribute1 |= 0x1000;
    } else {
        /* clear the bit */
        sprite->attribute1 &= 0xefff;
    }
}

/* change the tile offset of a sprite */
void sprite_set_offset(struct Sprite* sprite, int offset) {
    /* clear the old offset */
    sprite->attribute2 &= 0xfc00;

    /* apply the new one */
    sprite->attribute2 |= (offset & 0x03ff);
}

/* setup the sprite image and palette */
void setup_sprite_image() {
    /* load the palette from the image into palette memory*/
    memcpy16_dma((unsigned short*) sprite_palette, (unsigned short*) dragon_palette, PALETTE_SIZE);

    /* load the image into char block 0 */
    memcpy16_dma((unsigned short*) sprite_image_memory, (unsigned short*) dragon_data, (dragon_width * dragon_height) / 2);

}

/////////////////Dragon

/* a struct for the dragon's logic and behavior */
struct Dragon {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion, in 1/256 pixels */
    int x, y;

    /* the dragon's y velocity in 1/256 pixels/second */
    int yvel;

    /* the dragon's y acceleration in 1/256 pixels/second^2 */
    int gravity; 

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* whether the dragon is moving right now or not */
    int move;

    /* the number of pixels away from the edge of the screen the dragon stays */
    int border;

    /* if the dragon is currently falling */
    int falling;

    /* if the dragon is alive */
    int alive;
};

/* initialize the dragon */
void dragon_init(struct Dragon* dragon) {
    dragon->x = 40 << 8;
    dragon->y = 40 << 8;
    dragon->yvel = 0;
    dragon->gravity = 40;
    dragon->border = 40;
    dragon->frame = 0;
    dragon->move = 1;
    dragon->counter = 0;
    dragon->falling = 0;
    dragon->alive = 1;
    dragon->animation_delay = 8;
    dragon->sprite = sprite_init(dragon->x >> 8, dragon->y >> 8, SIZE_16_16, 0, 0, dragon->frame, 0);
}

/* move the dragon left or right returns if it is at edge of the screen */
int dragon_left(struct Dragon* dragon) {
    /* face left */
    sprite_set_horizontal_flip(dragon->sprite, 1);
    dragon->move = 1;

    /* if we are at the left end, just scroll the screen */
    if ((dragon->x >> 8) < dragon->border) {
        return 1;
    } else {
        /* else move left */
        dragon->x -= 256;
        return 0;
    }
}
int dragon_right(struct Dragon* dragon) {
    /* face right */
    sprite_set_horizontal_flip(dragon->sprite, 0);
    dragon->move = 1;

    /* if we are at the right end, just scroll the screen */
    if ((dragon->x >> 8) > (SCREEN_WIDTH - 16 - dragon->border)) {
        return 1;
    } else {
        /* else move right */
        dragon->x += 256;
        return 0;
    }
}

/* stop the dragon from walking left/right */
void dragon_stop(struct Dragon* dragon) {
    dragon->move = 0;
    dragon->frame = 0;
    dragon->counter = 7;
    sprite_set_offset(dragon->sprite, dragon->frame);
}

/* flap */
void flap(struct Dragon* dragon) {
        dragon->frame = 0;
        dragon->yvel -= 1500;
        dragon->y -= 40;
}

/* finds which tile a screen coordinate maps to, taking scroll into account */
unsigned short tile_lookup(int x, int y, int xscroll, int yscroll,
        const unsigned short* tilemap, int tilemap_w, int tilemap_h) {

    /* adjust for the scroll */
    x += xscroll;
    y += yscroll;

    /* convert from screen coordinates to tile coordinates */
    x >>= 3;
    y >>= 3;

    /* account for wraparound */
    while (x >= tilemap_w) {
        x -= tilemap_w;
    }
    while (y >= tilemap_h) {
        y -= tilemap_h;
    }
    while (x < 0) {
        x += tilemap_w;
    }
    while (y < 0) {
        y += tilemap_h;
    }

    /* lookup this tile from the map */
    int index = y * tilemap_w + x;

    /* return the tile */
    return tilemap[index];
}

/* function to set text on the screen at a given location */
void set_text(char* str, int row, int col) {                    
    /* find the index in the texmap to draw to */
    int index = row * 32 + col;

    /* the first 32 characters are missing from the map (controls etc.) */
    int missing = 32; 

    /* pointer to text map */
    volatile unsigned short* ptr = screen_block(26);

    /* for each character */
    while (*str) {
        /* place this character in the map */
        ptr[index] = *str - missing;

        /* move onto the next character */
        index++;
        str++;
    }   
}

/* declaration of assembly function to uppercase a string */
void uppercase(char* s);


/////////////SCORE
/* a struct for the dragon's logic and behavior */
struct Score {
    /* the actual sprite attribute info */
    struct Sprite* sprite;

    /* the x and y postion, in 1/256 pixels */
    int x, y;

    /* which frame of the animation he is on */
    int frame;

    /* the number of frames to wait before flipping */
    int animation_delay;

    /* the animation counter counts how many frames until we flip */
    int counter;

    /* the number of pixels away from the edge of the screen the dragon stays */
    int border;

    /* total score */
    int total;

    /* lap number */
    int lap;

};

/* initialize the score */
void score_init(struct Score* score) {
    score->x = 116 << 8;
    score->y = 30 << 8;
    score->border = 110;
    score->frame = 24;
    score->counter = 0;
    score->animation_delay = 8;
    score->sprite = sprite_init(score->x >> 8, score->y >> 8, SIZE_8_8, 0, 0, score->frame, 0);
}

////////////Sprite Updates


void score_update(struct Score* score, struct Dragon* dragon, int xscroll){
    /*check if dragon has passed key tile*/
   // unsigned short begin = tile_lookup((dragon->x >> 8)+8, 0, xscroll, 0, groundlayermap, groundlayermap_width, groundlayermap_height);
    unsigned short above = tile_lookup((dragon->x >> 8)-1,0,xscroll,0,groundlayermap,groundlayermap_width,groundlayermap_height);
    unsigned short passed = 0;
    if(above == 11){
        passed = 1;
    }
    if(passed == 1){
        score->counter++;
        if (score->counter >= score->animation_delay) {
            score->frame += 1;
            score->total += 1;
            score->lap = (score->total/3)+1;
            if (score->frame > 45) {
                score->frame = 24;
            }
            sprite_set_offset(score->sprite, score->frame);
            score->counter = 0;
        }
    }
}

//accelerate Assembly function
void accelerate(int y, int yvel, int grav);

//gameScore Assembly function
int gameScore(int total, int lap);

/* update the dragon */
void dragon_update(struct Dragon* dragon, struct Score* score, int xscroll) {
    /* update y position and speed if falling */
    if (dragon->falling) {
        accelerate(dragon->y,dragon->yvel,dragon->gravity);
        //dragon->y += dragon->yvel;
        //dragon->yvel += dragon->gravity;
    }

    /* check which tile the dragon's feet are over */
    unsigned short below = tile_lookup((dragon->x >> 8) + 8, (dragon->y >> 8) + 16, xscroll, 0, groundlayermap, groundlayermap_width, groundlayermap_height);
    /* top right of dragon */
    unsigned short topRight = tile_lookup((dragon->x >> 8)+13,(dragon->y >> 8), xscroll, 0, groundlayermap, groundlayermap_width,groundlayermap_height);
    /* right of dragon */
    unsigned short right = tile_lookup((dragon->x>>8)+16,(dragon->y >> 8) + 8, xscroll, 0, groundlayermap, groundlayermap_width, groundlayermap_height);
    /* bottom right */
    unsigned short botRight = tile_lookup((dragon->x>>8)+15,(dragon->y >>8)+15, xscroll, 0, groundlayermap, groundlayermap_width, groundlayermap_height);
    //dragon stops if he runs into a block (2,3,4,5,12,13,14,15)
    unsigned short above = tile_lookup((dragon->x>>8)+8,(dragon->y>>8),xscroll,0,groundlayermap,groundlayermap_width,groundlayermap_height);
    /* if it's block tile
     * these numbers refer to the tile indices of the blocks the dragon can walk on */
    
    dragon->falling = 1;
        dragon->move = 1;

    if(dragon->x == 240){
        dragon->alive = 0;
    }
    if((above == 21) || 
        (above >= 1 && above <= 6) ||
        (above >= 12 && above <= 17)){
        dragon->alive = 0;
    }
    else if((right == 21) ||
        (right >= 1 && right <= 6) ||
        (right >= 12 && right <= 17)) {
        dragon->alive = 0;
    }
    else if((topRight == 21) ||
            (topRight >= 1 && topRight <= 6) ||
            (topRight >= 12 && topRight <= 17)){
        dragon->alive = 0;
    }
    else if ((botRight == 21) ||
             (botRight >= 1 && botRight <= 6) ||
             (botRight >= 12 && botRight <= 17)){
        dragon->alive = 0;
    }
    else if((below == 21) ||
            (below >= 1 && below <= 6) ||
            (below >= 12 && below <= 17)){
        dragon->alive = 0;
    }
    /*
    else {
        /* he is falling now */
        //dragon->falling = 1;
      //  dragon->move = 1;
    //}

    //if collided = 1
    if(dragon->alive == 0){
        dragon->x = 240;
        dragon->y = 160;
        score->x = 120;
        score->y = 40;
        sprite_position(dragon->sprite,240, 160);
        sprite_position(score->sprite,240,160);
        //*display_control |= BG2_ENABLE;
        dragon->yvel = 0;
        dragon->falling = 0;
    }
    else{
        /* update animation if moving */
        if (dragon->move) {
            dragon->counter++;
            if (dragon->counter >= dragon->animation_delay) {
                dragon->frame += 1;
                if (dragon->frame > 16) {
                    dragon->frame = 0;
                }
                sprite_set_offset(dragon->sprite, dragon->frame);
                dragon->counter = 0;
            }
        }
        /* set on screen position */
        sprite_position(dragon->sprite, dragon->x >> 8, dragon->y >> 8);
    }
}


/* the main function */
int main( ) {
    /* we set the mode to mode 0 with bg0 on */
    *display_control = MODE0 | BG0_ENABLE | BG1_ENABLE |SPRITE_ENABLE | SPRITE_MAP_1D;

    /* setup the background 0 */
    setup_background();

    /* setup the sprite image data */
    setup_sprite_image();

    /* clear all the sprites on screen now */
    sprite_clear();

    /* create the dragon */
    struct Dragon dragon;
    dragon_init(&dragon);

    /* create the score */
    struct Score score;
    score_init(&score);

    /* set initial scroll to 1 */
    int xscroll = 1;

    /* loop forever */
    while (dragon.alive) {
        /*scroll continuously*/
        xscroll++;

        /* update the dragon */
        dragon_update(&dragon, &score, xscroll);
        /* update the score */
        score_update(&score, &dragon, xscroll);

        /* check if they're flapping*/
        if (button_pressed(BUTTON_A)) {
            flap(&dragon);
        }
        /* wait for vblank before scrolling and moving sprites */
        wait_vblank();
        *bg0_x_scroll = xscroll * 1.2;
        *bg1_x_scroll = xscroll;
        sprite_update_all();

        /* delay some */
        delay(300);
    }
    

    while(dragon.alive == 0){
        *display_control |= BG2_ENABLE;
        dragon.x = 240;
        dragon.y = 160;
        score.x = 120;
        score.y = 40;
        //char grats[16] = "Score: ";
        //char youScored[12] = "you scored: ";
        //uppercase(grats);
        //uppercase(youScored);
        //int playerScore = gameScore(score.total,score.lap);
        //int printX = 60;
        //set_text(grats, printX, 60);
        //set_text(youScored, printX, 90);     
    }
        
}





/* the game boy advance uses "interrupts" to handle certain situations
 * for now we will ignore these */
void interrupt_ignore( ) {
    /* do nothing */
}

/* this table specifies which interrupts we handle which way
 * for now, we ignore all of them */
typedef void (*intrp)( );
const intrp IntrTable[13] = {
    interrupt_ignore,   /* V Blank interrupt */
    interrupt_ignore,   /* H Blank interrupt */
    interrupt_ignore,   /* V Counter interrupt */
    interrupt_ignore,   /* Timer 0 interrupt */
    interrupt_ignore,   /* Timer 1 interrupt */
    interrupt_ignore,   /* Timer 2 interrupt */
    interrupt_ignore,   /* Timer 3 interrupt */
    interrupt_ignore,   /* Serial communication interrupt */
    interrupt_ignore,   /* DMA 0 interrupt */
    interrupt_ignore,   /* DMA 1 interrupt */
    interrupt_ignore,   /* DMA 2 interrupt */
    interrupt_ignore,   /* DMA 3 interrupt */
    interrupt_ignore,   /* Key interrupt */
};

