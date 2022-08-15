
#include <genesis.h>
#include <resources.h>


typedef struct {
    int x;
    fix16 y;
    int w;
    int h;
    int velx;
    fix16 vely;
    int hugged;
    bool jumping;
    Sprite* sprite;
    char name[6];
    bool present;
    int direction;
    u16 timerPeople;
} Person;

Person biker = {0, FIX16(0), 32, 32, 0, FIX16(0), 0, FALSE, NULL, "BIKER", TRUE, 0, 0};
#define ANIM_BIKER_STOP 0
#define ANIM_BIKER_PEDAL 1
#define ANIM_BIKER_EMPTY 2

Person player = {0, FIX16(0), 32, 32, 0, FIX16(0), 0, FALSE, NULL, "PLAYER", TRUE, 0, 0};
#define ANIM_STAND 0
#define ANIM_JUMP 1
#define ANIM_FALL 2
#define ANIM_WALK 3
#define ANIM_HUG 4
#define ANIM_WAVE 5

Person bird = {0, FIX16(0), 24, 16, 0, FIX16(0), 0, FALSE, NULL, "BIRD", TRUE, 0, 0};
#define BIRD_LAND 0
#define BIRD_SIT 1
#define BIRD_CHIRP 2

#define MAX_PEOPLE 14
#define MAX_UNHUGGED 4
bool spawn_person;
int peopleRemaining = 0;
Person people[MAX_PEOPLE];
bool startDrop = FALSE;

Sprite* lamp_sparks_1a;
Sprite* lamp_sparks_1b;
Sprite* lamp_sparks_2a;
Sprite* lamp_sparks_2b;
Sprite* lamp_sparks_3a;
Sprite* lamp_sparks_3b;
Sprite* lamp_sparks_4a;
Sprite* lamp_sparks_4b;
Sprite* lamp_sparks_5a;
Sprite* lamp_sparks_5b;


enum STATE {
//  0     1     2          3              4             5  
    SEGA, MENU, RESOURCES, CLASSIC_INTRO, CLASSIC_GAME, CLASSIC_OUTRO
};

int GAME_STATE;

u16 ind = TILE_USERINDEX;

u16 timerSEGA = 0;

#define SONIC_COIN 64
#define DELAY_SEGA_TITLE 200            // Delay for SEGA on screen
#define DELAY_TITLE_START 120           // Delay for "Press Start" to appear
#define DELAY_TITLE_SMEAR 240           // Delay for title smear
#define DELAY_CLASSIC_GAME_INTRO 80     // Delay for intro animation in game
#define DELAY_CLASSIC_INTRO_JUMP 35     // Delay for jump off bike
#define DELAY_CLASSIC_GAME_LEN 8000      // Total length of song
#define DELAY_CLASSIC_OUTRO_BLACK 60    // Black screen at end of classic game (1 sec)
#define DELAY_CLASSIC_OUTRO_BIKE_ANIM 180   // Animation time of outro biking to middle

#define LEFT_EDGE 0
#define RIGHT_EDGE 320
#define TOP_EDGE 72
#define GROUND 152

fix16 GRAVITY = FIX16(.08);
fix16 JUMP_STRENGTH = FIX16(1.3);

// ---------------------------------- position () related flags and variables

int offsetA = 0;
int offsetB = 0;
int offset_velA = 0;
int offset_velB = 0;

#define HUG_TIME 32         // Duration of hug-action

u16 timingDropHelper = 0;

bool activateNPCs = 0;


// --------------------------------- hug text control

bool textActive = FALSE;
Sprite* textSprite1;
Sprite* textSprite2;
Sprite* textSprite3;
Sprite* textSprite4;
int text_x;
int personType;
int timerText = 0;
fix16 text_y;
fix16 text_vely = FIX16(0);


// --------------------------------- intro animation control

s16 hscroll_offset[224];
s16 vscroll_offset[20];
s16 hscroll_speed[224];
s16 vscroll_speed[20];
fix16 hscroll_speed_fix16[224];
fix16 vscroll_speed_fix16[20];

bool enable_stretch = FALSE;
u16 cur_line = 46; // current line
f16 v_offset = 0; // shift of the plane
f16 v_scroll_step = FIX16(0.5); // step to increase the displacement of the v_offset



// --------------------------------- main game background animation control

s16 vcolumns_offset[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
s16 vcolumns_speed[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//s16 vcolumns_speed[20] = { 1, 2, 1, 3, 1, 1, 2, 1, 1, 1,
//                            1, 2, 1, 1, 2, 1, 3, 1, 1, 1 };
u8 starting_flowers[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


// --------------------------------- 


clearScreen() {
    VDP_clearPlane(BG_A, FALSE);
    VDP_clearPlane(BG_B, FALSE);
    VDP_clearTextArea(0, 0, 40, 28);
}


// Transitions -------------------------------------------------- Transitions

void transitionSEGA() {
    VDP_drawImageEx(BG_B, &sega_splash, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
    XGM_startPlayPCM(SONIC_COIN, 1, SOUND_PCM_CH2);
    GAME_STATE = SEGA;
}

void transitionSEGAtoMENU() {
    clearScreen();
    timerSEGA = 0;
    VDP_drawImageEx(BG_B, &classic_title, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
    //XGM_setLoopNumber(-1);
    //XGM_startPlay(&drums);
    GAME_STATE = MENU;
}

void transitionMENUtoRESOURCES() {
    clearScreen();
    VDP_drawImageEx(BG_B, &classic_bg_resources, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
    VDP_drawText("Press START to return.", 9, 26);
    GAME_STATE = RESOURCES;
}

void transitionMENUtoCLASSIC_INTRO() {
    //clearScreen(); // Keep title on screen for upcoming animation
    VDP_clearTextArea(0, 0, 40, 28);

    timerSEGA = 0;
    

    biker.x = -biker.w;
    biker.velx = 0;
    biker.y = FIX16(GROUND - biker.h);
    biker.sprite = SPR_addSprite(&spr_biker, biker.x, fix16ToInt(biker.y), TILE_ATTR(PAL0, 0, FALSE, FALSE) );

    player.x = -player.w;
    player.velx = 0;
    player.y = FIX16(GROUND - player.h);
    player.sprite = SPR_addSprite(&spr_player, player.x, fix16ToInt(player.y), TILE_ATTR(PAL0, 0, FALSE, FALSE) );
    
    GAME_STATE = CLASSIC_INTRO;
}

void transitionCLASSIC_INTROtoCLASSIC_GAME() {
    timerSEGA = 0;

    //PAL_setColor(13, RGB24_TO_VDPCOLOR(0xcade00));  // DELETE THIS - The big colour switch 
    //           Switch this to 1 (priority) ----> | <---
    VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 1, FALSE, FALSE, 2), 0, 0, 40, 9);
    //VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 19, 40, 9);

    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_COLUMN);
    
    bird.x = 88;
    bird.velx = 0;
    bird.y = FIX16(TOP_EDGE - bird.h);
    bird.sprite = SPR_addSprite(&spr_bird, bird.x, fix16ToInt(bird.y), TILE_ATTR(PAL0, 0 * 30, FALSE, FALSE) );
 
    
    GAME_STATE = CLASSIC_GAME;
}

void transitionCLASSIC_GAMEtoCLASSIC_OUTRO() {
    clearScreen();
    timerSEGA = 0;
    VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
    GAME_STATE = CLASSIC_OUTRO;
}


void transitionTEMPLATE() {
    clearScreen();
    GAME_STATE = 0;
}


// State Prototypes ---------------------------------------- State Prototypes

void stateSEGA();
void stateMENU();
void stateRESOURCES();
void stateCLASSIC_INTRO();
void stateCLASSIC_GAME();
void stateCLASSIC_OUTRO();


// Joy Handler -------------------------------------------------- Joy Handler

void gameJoyHandler(u16 joy, u16 changed, u16 state) {
    if (joy == JOY_1)
    {
        // FOR DEBUG ---------------------------------------------- FOR DEBUG
        if (state & BUTTON_C & changed) {
            player.y = FIX16(GROUND);
        }
        // State: MENU
        if (GAME_STATE == MENU) {
            if (state & BUTTON_START & changed) {
                XGM_stopPlay();
                transitionMENUtoCLASSIC_INTRO();
            }
            if (state & BUTTON_B & changed) {
                XGM_stopPlay();
                transitionMENUtoRESOURCES();
            }
        }
        // State: RESOURCES
        if (GAME_STATE == RESOURCES) {
            if (state & BUTTON_START & changed) {
                transitionSEGAtoMENU();
            }
        }
        // State: CLASSIC_GAME
        if (GAME_STATE == CLASSIC_GAME && player.hugged == 0) {
            /* REMOVED
            if (state & BUTTON_START & changed)
            {
                transitionSEGAtoMENU();
            } */
            if (state & BUTTON_LEFT & changed)
            {
                player.velx = -1;
                player.direction = -1;
            }
            else if (state & BUTTON_RIGHT & changed)
            {
                player.velx = 1;
                player.direction = 1;
            }
            else if (state & BUTTON_UP & changed)
            {
                if (player.jumping == FALSE && player.hugged == 0) {
                    player.jumping = TRUE;
                    player.vely = -fix16Add(JUMP_STRENGTH, FIX16(0.1));
                }
            }
            else if (state & BUTTON_A & changed)
            {
                if (player.jumping == FALSE && player.hugged == 0) {
                    player.hugged = HUG_TIME;
                    hug();
                }
            }
            else
            {
                if( (changed & BUTTON_RIGHT) | (changed & BUTTON_LEFT) )
                {
                    player.velx = 0;
                }

            }

        }
        
    }

}



// Position Functions ------------------------------------ Position Functions

void positionBG() {
    VDP_setVerticalScroll(BG_A, offsetA += offset_velA);
    VDP_setVerticalScroll(BG_B, offsetB += offset_velB);
}

void positionBiker() {
    biker.x += biker.velx;
    SPR_setPosition(biker.sprite, biker.x, fix16ToInt(biker.y));
}

void positionPlayer() {
    player.x += player.velx;
    player.y = fix16Add(player.y, player.vely);

    if (GAME_STATE == CLASSIC_GAME) {
        if (player.x < LEFT_EDGE) player.x = LEFT_EDGE;
    }
    if (player.x + player.w > RIGHT_EDGE) player.x = RIGHT_EDGE - player.w;

    if (player.jumping == TRUE) player.vely = fix16Add(player.vely, GRAVITY);
    
    if (player.jumping == TRUE && fix16ToInt(player.y) + player.h >= GROUND) {
        player.jumping = FALSE;
        player.vely = FIX16(0);
        player.y = FIX16(GROUND - player.h);
        SPR_setAnim(player.sprite, ANIM_STAND);
    }

    if (player.hugged == 0) {
        if (player.vely < FIX16(-.2) && (player.y <= FIX16(GROUND - player.h))) SPR_setAnim(player.sprite, ANIM_JUMP);
        if (player.vely > FIX16(-.2) && (player.y < FIX16(GROUND - player.h))) SPR_setAnim(player.sprite, ANIM_FALL);
        if (player.y == FIX16(GROUND - player.h) && player.velx == 0) {
            SPR_setAnim(player.sprite, ANIM_STAND);
        }
        if (player.y == FIX16(GROUND - player.h) && player.velx < 0) {
            SPR_setAnim(player.sprite, ANIM_WALK);
                    SPR_setHFlip(player.sprite, TRUE);
        }
        if (player.y == FIX16(GROUND - player.h) && player.velx > 0) {
            SPR_setAnim(player.sprite, ANIM_WALK);
                    SPR_setHFlip(player.sprite, FALSE);
        }
    }
    if (player.hugged > 0) {
        player.velx = 0;
        if (player.hugged == HUG_TIME) SPR_setAnim(player.sprite, ANIM_HUG);
        if (player.hugged == 1) SPR_setAnim(player.sprite, ANIM_STAND);
        player.hugged--;
    }

    SPR_setPosition(player.sprite, player.x, fix16ToInt(player.y));
}

void positionBird() {
    bird.y = fix16Add(bird.y, bird.vely);

    SPR_setPosition(bird.sprite, bird.x, fix16ToInt(bird.y));
}

void positionPeople() {
    u16 i = 0;
    u16 choose_drop = MAX_PEOPLE + 1;

    if (timingDropHelper == 0) {
        if (peopleRemaining != 0 && startDrop == TRUE) choose_drop = random() % peopleRemaining;
        timingDropHelper = random() % 70 + 80; //120
    }
    timingDropHelper--;

    for (i = 0; i < peopleRemaining; i++) {
        Person* p = &people[i];
        p->x += (timerSEGA % 2) * p->velx; // % is slow-down ratio
        p->y = fix16Add(p->y, p->vely);

        // Setting upper out of play boundary for people to drop into.
        if (p->present == FALSE) {
            if (p->x < LEFT_EDGE + 27) {
                p->direction = 1;
                p->velx = 1;
                p->x = LEFT_EDGE + 27;
            } 
            if (p->x + p->w > RIGHT_EDGE - 23) {
                p->direction = -1;
                p->velx = -1;
                p->x = RIGHT_EDGE - p->w - 23;
            }

            if (p->velx < 0) {
                SPR_setAnim(p->sprite, ANIM_WALK);
                SPR_setHFlip(p->sprite, TRUE);
            }
            if (p->velx > 0) {
                SPR_setAnim(p->sprite, ANIM_WALK);
                SPR_setHFlip(p->sprite, FALSE);
            }

            p->y = FIX16(2);
        } 


        // Qualifying and selecting a person to drop into play
        if (p->present == FALSE && i == choose_drop ) {
            if (( p->x < 240 && p->velx > 0 ) || ( p->x > 80 && p->velx < 0 )) {
                p->present = TRUE;
                p->timerPeople = random() % 180 + 120; // Sets timer to walk before waving
            }
        }


        // Setting edge boundaries on ground
        if ( p->present == TRUE && p->hugged == 0 ) {
            if (p->x < LEFT_EDGE) {
                p->direction = 1; 
                p->velx = 1;
                p->x = LEFT_EDGE;
            }
            if (p->x + p->w > RIGHT_EDGE) {
                p->direction = -1;
                p->velx = -1;
                p->x = RIGHT_EDGE - p->w;
            }
        }

        // All the time that the Person is in play on the screen.
        if (p->present == TRUE) {
            
            if (p->jumping == TRUE) p->vely = fix16Add(p->vely, GRAVITY);
        
            if (p->jumping == TRUE && fix16ToInt(p->y) + p->h >= GROUND) {
                p->jumping = FALSE;
                p->vely = FIX16(0);
                p->y = FIX16(GROUND - p->h);
                SPR_setAnim(p->sprite, ANIM_STAND);
            }
    
            // Passives        
            // (p->vely < FIX16(-0.2)
            if (p->vely < FIX16(0) && (p->y <= FIX16(GROUND - p->h))) SPR_setAnim(p->sprite, ANIM_JUMP);
            if (p->vely > FIX16(0) && (p->y < FIX16(GROUND - p->h))) SPR_setAnim(p->sprite, ANIM_FALL);
            if (p->y == FIX16(GROUND - p->h) && p->velx == 0 && p->timerPeople > 0) {
                SPR_setAnim(p->sprite, ANIM_STAND);
            }
            if (p->y == FIX16(GROUND - p->h) && p->velx < 0) {
                SPR_setAnim(p->sprite, ANIM_WALK);
                SPR_setHFlip(p->sprite, TRUE);
            }
            if (p->y == FIX16(GROUND - p->h) && p->velx > 0) {
                SPR_setAnim(p->sprite, ANIM_WALK);
                SPR_setHFlip(p->sprite, FALSE);
            }

            // Landing and time to turn green
            if (p->hugged == 0 && p->timerPeople > 0) {
                p->timerPeople--;
            }
            else if (p->hugged == 0 && p->timerPeople == 0) {
                p->velx = 0;
                SPR_setPalette(p->sprite, PAL1);
                SPR_setAnim(p->sprite, ANIM_WAVE);
            }

            // Intermediate hugging animation timeframe
            if (p->hugged == 2 && p->timerPeople > 0) {
                p->timerPeople--;
                SPR_setAnim(p->sprite, ANIM_HUG);
            }
            else if (p->hugged == 2 && p->timerPeople == 0) {
                p->velx = p->direction;
                SPR_setPalette(p->sprite, PAL0);
                SPR_setAnim(p->sprite, ANIM_WAVE);
                p->hugged = 1;
            }

            // Been hugged and leaving screen
            if (p->hugged == 1 && p->timerPeople > 0) {
                p->timerPeople--;
            }
            else if (p->hugged == 1 && p->timerPeople == 0) {
                p->jumping = TRUE;
                p->vely = -JUMP_STRENGTH;
                p->timerPeople = 40;
                //p->timerPeople = random() % 10 + 35;
            }

            //Reset person condition
            if (p->hugged == 1 && ( p->x < LEFT_EDGE - p->w || p->x > RIGHT_EDGE ) ) {
                p->jumping = TRUE;
                p->present = FALSE;
                p->hugged = 0;
                p->timerPeople = 0;
                p->y = FIX16(2);
                p->vely = FIX16(0);

            }

        }


        SPR_setPosition(p->sprite, p->x, fix16ToInt(p->y) );
    }
}


void textController() {
    timerText++;
    text_y = fix16Add(text_y, text_vely);
    text_vely = fix16Add(text_vely, fix16Div(GRAVITY, FIX16(2.51)) );

    switch (timerText) {
        case(1):
        {
            text_y = FIX16(GROUND - 48); // Positions text above the people

            if (personType == 1) {
                int randText = random() % 7;
                if (randText < 2) {
                    textSprite1 = SPR_addSprite(&spr_text_empty, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_hugz_L, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_hugz_R, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_empty, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                } else if (randText < 4) {
                    textSprite1 = SPR_addSprite(&spr_text_empty, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_cute_L, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_cute_R, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_empty, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                } else if (randText < 6) {
                    textSprite1 = SPR_addSprite(&spr_text_geth_1, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_geth_2, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_geth_3, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_geth_4, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                } else {
                    textSprite1 = SPR_addSprite(&spr_text_empty, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_3you_L, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_3you_R, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_empty, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                }
            }
            else if (personType == 2) {
                int randText = random() % 2;
                if (randText < 1) {
                    textSprite1 = SPR_addSprite(&spr_text_empty, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_meow_L, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_meow_R, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_empty, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                } else {
                    textSprite1 = SPR_addSprite(&spr_text_khugs_1, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite2 = SPR_addSprite(&spr_text_khugs_2, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite3 = SPR_addSprite(&spr_text_khugs_3, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                    textSprite4 = SPR_addSprite(&spr_text_khugs_4, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                }
            }
            else if (personType == 3) {
                textSprite1 = SPR_addSprite(&spr_text_trad_1, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite2 = SPR_addSprite(&spr_text_trad_2, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite3 = SPR_addSprite(&spr_text_trad_3, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite4 = SPR_addSprite(&spr_text_trad_4, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
            }
            else if (personType == 4) {
                textSprite1 = SPR_addSprite(&spr_text_gohy_1, text_x, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite2 = SPR_addSprite(&spr_text_gohy_2, text_x + 32, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite3 = SPR_addSprite(&spr_text_gohy_3, text_x + 64, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
                textSprite4 = SPR_addSprite(&spr_text_gohy_4, text_x + 96, fix16ToInt(text_y), TILE_ATTR(PAL0, 1, FALSE, FALSE) );
            }
            //choose which sprite to generate
            text_vely = -fix16Div(JUMP_STRENGTH, FIX16(1.3) );
            break;
        }
        case(65):{
            SPR_releaseSprite(textSprite1);
            SPR_releaseSprite(textSprite2);
            SPR_releaseSprite(textSprite3);
            SPR_releaseSprite(textSprite4);
            textActive = FALSE;
            break;
        }
    } 
    
    if (timerText < 65) {
        SPR_setPosition(textSprite1, text_x, fix16ToInt(text_y));
        SPR_setPosition(textSprite2, text_x + 32, fix16ToInt(text_y));
        SPR_setPosition(textSprite3, text_x + 64, fix16ToInt(text_y));
        SPR_setPosition(textSprite4, text_x + 96, fix16ToInt(text_y));
    }
}


int collidePlayer(Person* a) {
    return ( player.direction * (a->x - player.x) < player.w * 6 / 8 && player.direction * (a->x - player.x) > player.w / 16 && a->timerPeople == 0 );
}

void hug() {
    Person* p;
    int i = 0;
    int j = 0;
    //for (j = 0; j < MAX_PEOPLE; j++) {     // Coutning downwards prefers the Cat in a stack to be hit first.
    for (j = MAX_PEOPLE - 1; j >= 0; j--) {
        p = &people[j];
        if (p->present == TRUE && p->hugged == 0) {
            if (collidePlayer( p )) {
                p->timerPeople = HUG_TIME;
                p->hugged = 2;
                if (!textActive) {
                    textActive = TRUE;
                    text_x = p->x - 48;
                    if (p->name[0] == 'P') personType = 1;
                    if (p->name[0] == 'C') personType = 2;
                    if (p->name[0] != 'P' && p->name[0] != 'C') personType = 0;
                    timerText = 0;
                }
            }
        }
    }
}


u8 adjustCOLUMNS(u8 i) {
    switch (i) {
        case 0:
            return 0;
        case 1:
            return 3;
        case 2:
            return 4;
        case 3:
            return 7;
        case 4:
            return 8;
        case 5:
            return 11;
        case 6:
            return 12;
        case 7:
            return 15;
        case 8:
            return 16;
        case 9:
            return 19;
    }
}

void animateCOLUMNS(s16 *offset, s16 *speed) {

    
    for (u8 i = 0; i < 10; i++) {
        if(offset[adjustCOLUMNS(i)] >= 96) {
            offset[adjustCOLUMNS(i)] = 96;
            speed[adjustCOLUMNS(i)] = 0;
        }


            if (timerSEGA % 3 == 0) offset[adjustCOLUMNS(i)] += speed[adjustCOLUMNS(i)];
        }

    VDP_setVerticalScrollTile(BG_B, 0, offset, 20, DMA);
}




void HIntHandler() {
    if(enable_stretch) {
        VDP_setVerticalScrollTile( BG_B, 0, vscroll_offset, 20, DMA);
        for (int i = 0; i < 20; i++) { 
            vscroll_offset[i] -= ( (timerSEGA - 100) % 15 == 1 ) ? (timerSEGA - 100) / 15 : 0 ;
        }
        //v_offset = fix16Sub(v_offset, v_scroll_step); // change the displacement
    }
}

void VIntHandler() {
    if(enable_stretch) {
        // after the frame is drawn:
        //v_scroll_step = FIX16(0.5); // resets this var to initial value
        //v_offset = 0; // remove the accumulated shift
    //    for (int i = 0; i < 20; i++) { 
    //        vscroll_offset[i] = vscroll_speed[i];
    //    }
    }
}


/*
 *  Used personRamaining for intermediate MAX_PEOPLE
 */

// int main() ---------------------------------------------------- int main()

int main()
{
    // Game setup

    JOY_init();
    JOY_setEventHandler( &gameJoyHandler );
    XGM_setPCM(SONIC_COIN, sonic_coin, sizeof(sonic_coin) );

    SPR_init();


    VDP_loadTileSet(&tile_ind0,1,DMA);
    VDP_loadTileSet(&tile_ind13,2,DMA);

    PAL_setPalette(PAL0, sega_splash.palette->data, DMA);
    PAL_setColor(15, PAL_getColor(1) );
    PAL_setColor(17, PAL_getColor(6) );
    
    SYS_disableInts();
    {
        VDP_setHIntCounter(0);
        VDP_setHInterrupt(1);
        SYS_setHIntCallback(HIntHandler);
        SYS_setVIntCallback(VIntHandler);
    }
    SYS_enableInts();
    


    //transitionSEGA();
    transitionSEGAtoMENU();

// main() while(1) ------------------------------------------ main() while(1)

    while(1)
    {
        // State Machine
        switch (GAME_STATE) {
            case SEGA: {
                stateSEGA();
                break;
            }
            case MENU: {
                stateMENU();
                break;
            }
            case RESOURCES: {
                stateRESOURCES();
                break;
            }
            case CLASSIC_INTRO: {
                stateCLASSIC_INTRO();
                break;
            }
            case CLASSIC_GAME: {
                stateCLASSIC_GAME();
                break;
            }
            case CLASSIC_OUTRO: {
                stateCLASSIC_OUTRO();
                break;
            }
            default: {
                VDP_drawText("State Machine: default is hit.", 2, 3);
                break;
            }
        }



        if (offset_velA || offset_velB) positionBG();
        if (biker.velx) positionBiker();
        if (GAME_STATE == CLASSIC_INTRO || GAME_STATE == CLASSIC_GAME) positionPlayer();
        if (GAME_STATE == CLASSIC_GAME) {
            if(activateNPCs) positionBird();
            if(activateNPCs) positionPeople();
        }
        if(textActive) textController();

        SPR_update();  // This is not in each respective STATE (that's good)
        SYS_doVBlankProcess();
    }
    return (0);
}



// States ------------------------------------------------------------ States

void stateSEGA() {
    timerSEGA++;
    if (timerSEGA > DELAY_SEGA_TITLE) {
        transitionSEGAtoMENU();
    }
}

void stateMENU() {
    timerSEGA++; 
    switch (timerSEGA) {
        case DELAY_TITLE_START: {
            VDP_drawText("Press START", 14, 18);
            VDP_drawText("Press B for extras.", 10, 20);
            break;
        }
    }
}

void stateRESOURCES() {
    VDP_drawText("ROCK PAPER SHOTGUN", 15, 5);
    VDP_drawText("Piles of Smiles: HUGPUNX", 15, 7);
    VDP_drawText("FOREST AMBASSADOR", 8, 17);
    VDP_drawText("a game about hugging", 5, 19);

}

void stateCLASSIC_INTRO() {
    timerSEGA++;
    switch (timerSEGA) {
        case 1:
        {
            // Code to animate title smear
            VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_COLUMN);
            for (int i = 0; i < 224; i++) {
                hscroll_offset[i] = 0;
                hscroll_speed[i] = 0;
            }
            for (int i = 0; i < 20; i++) {
                vscroll_offset[i] = 0;
                vscroll_speed[i] = 0;
            }

            // Set background image
            PAL_setColor(13, RGB24_TO_VDPCOLOR(0x000000));
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 0, 40, 9);
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 19, 40, 9);
            //VDP_drawText("State: CLASSIC_INTRO Menu Titl Smear", 2, 24);
            break;
        }
        case 100:
        {
            //enable_stretch = TRUE;
            break;
        }
        case 101:
        {
            break;
        }
        case 240:
        {
            // Clear screen wait for small delay to game intro animation
            VDP_setVerticalScroll( BG_B, 0);
            
            VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_COLUMN);
            clearScreen();
            for (int i = 0; i < 224; i++) {
                hscroll_offset[i] = 0;
                hscroll_speed[i] = 0;
            }
            for (int i = 0; i < 20; i++) {
                vscroll_offset[i] = 0;
                vscroll_speed[i] = 0;
            }
            enable_stretch = FALSE;
            v_scroll_step = FIX16(3.0); // resets this var to initial value
            v_offset = 0; // remove the accumulated shift
            break;
        }
        case 280:
        {
            VDP_drawImageEx(BG_B, &classic_bg, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 19, 40, 9);
            //VDP_drawText("State: CLASSIC_INTRO Game Intro", 2, 25);
            SPR_setAnim(biker.sprite, ANIM_BIKER_PEDAL);
            biker.velx = 1;
            break;
        }
        case (280 + DELAY_CLASSIC_GAME_INTRO):
        {
            SPR_setAnim(biker.sprite, ANIM_BIKER_EMPTY);
            biker.velx = 0;

            player.x = biker.x + 2;
            player.y = fix16Sub(player.y, FIX16(3));
            player.velx = 1;
            player.vely = -JUMP_STRENGTH;
            player.jumping = TRUE;
            
            break;
        }
        case (280 + DELAY_CLASSIC_GAME_INTRO + DELAY_CLASSIC_INTRO_JUMP):
        {
            player.velx = 0;
            transitionCLASSIC_INTROtoCLASSIC_GAME();
            break;
        }

    }
    
    // Animation control
    if (1 <= timerSEGA && timerSEGA < 100) { // 240
        VDP_setHorizontalScrollLine(BG_B, 0 , &hscroll_offset, 224, DMA);
        VDP_setVerticalScrollTile(BG_B, 0, &vscroll_offset, 20, DMA);

        // Calculate new fix16 positions
        int angle_MAX = 85; // 30deg in 1024ians

        // Update speed and offset
        for (int i = 0; i < 224; i++) {
            // Angle Math
            //                       = ( Destination Angle * vertical distance from middle ) * ( Frame / Total Frames ) * Scale Coefficient
            //hscroll_speed_fix16[i] = fix16Mul( fix16Mul( fix16Mul( sinFix16( angle_MAX ), FIX16(i - 112) ), fix16Div(FIX16(timerSEGA), FIX16(240)) ), FIX16(0.5) );
            hscroll_speed_fix16[i] = fix16Mul( FIX16(i - 112), fix16Div(FIX16(timerSEGA), FIX16(100 * 2)) ); // was: 240 * 2
            // Offset updating
            hscroll_speed[i] = -fix16ToInt( hscroll_speed_fix16[i] );
            hscroll_offset[i] = hscroll_speed[i];
        }
        for (int i = 0; i < 20; i++) {
            // Angle Math
            //                       = ( Destination Angle * vertical distance from middle ) * ( Frame / Total Frames ) * Scale Coefficient
            //vscroll_speed_fix16[i] = fix16Mul( fix16Mul( fix16Mul( sinFix16( angle_MAX ), FIX16(i - 10) ), fix16Div(FIX16(timerSEGA), FIX16(240)) ), FIX16(2) );
            vscroll_speed_fix16[i] = fix16Mul( FIX16(i - 10), fix16Div(FIX16(timerSEGA), FIX16(75)) ); // new: 180 ; old: 240 * 1
            // Offset updating
            vscroll_speed[i] = -fix16ToInt( vscroll_speed_fix16[i] );
            vscroll_offset[i] = vscroll_speed[i];
        }
    }

    if (100 <= timerSEGA && timerSEGA < 240) {
        VDP_setHorizontalScrollLine(BG_B, 0 , &hscroll_offset, 224, DMA);
        VDP_setVerticalScrollTile(BG_B, 0, &vscroll_offset, 20, DMA);

        
        for (int i = 0; i < 224; i++) {

            hscroll_speed_fix16[i] = fix16Add(hscroll_speed_fix16[i], FIX16(-0.36) );

            hscroll_speed[i] = -fix16ToInt( hscroll_speed_fix16[i] );
            hscroll_offset[i] = hscroll_speed[i];
        }
        
        for (int i = 0; i < 20; i++) {

            vscroll_speed_fix16[i] = fix16Add(vscroll_speed_fix16[i], FIX16(0.8) );

            vscroll_speed[i] = -fix16ToInt( vscroll_speed_fix16[i] );
            vscroll_offset[i] = vscroll_speed[i];
        }
    }

    if (timerSEGA >= 240) {
            for (int i = 0; i < 224; i++) {
                hscroll_offset[i] = 0;
                hscroll_speed[i] = 0;
            }
            for (int i = 0; i < 40; i++) {
                vscroll_offset[i] = 0;
                vscroll_speed[i] = 0;
            }
        VDP_setHorizontalScrollLine(BG_B, 0 , &hscroll_offset, 224, DMA);
        VDP_setVerticalScrollTile(BG_B, 0, &vscroll_offset, 40, DMA);
    }

}


void stateCLASSIC_GAME() {

    //VDP_drawText("State: CLASSIC_GAME", 2, 26);
    Person* p = people;
    
    u16 i = 0;
    timerSEGA++;
    switch (timerSEGA) {
        case (1):
        {
            for (i = 0; i < MAX_PEOPLE - 5; i++) {
                p->x = i * 16;
                p->y = FIX16(2);
                p->w = 32;
                p->h = 32;
                p->velx = 0;
                p->vely = FIX16(0);
                p->hugged = 0;
                p->jumping = TRUE;
                int j = random() % 3;
                if (j == 0) p->sprite = SPR_addSprite(&spr_person1, p->x, p->y, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
                if (j == 1) p->sprite = SPR_addSprite(&spr_person2, p->x, p->y, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
                if (j == 2) p->sprite = SPR_addSprite(&spr_person3, p->x, p->y, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
                p->present = FALSE;
                p->direction = ( 2 * ( random() % 2 ) ) - 1;
                p->velx = p->direction;
                sprintf(p->name, "Per%d", i);
                p->timerPeople = 0;
                peopleRemaining++;
                p++;

            }
            activateNPCs = 1;
            break;
        }
        case (420 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // People start falling
            startDrop = TRUE;
            break;
        }
        case (480 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Plants start growing
            u8 started_flowers = 0;
            while (started_flowers < 3) {
                for (i = 0; i < 10; i++) {
                    if (started_flowers < 3) {
                            int j = random() % 9;
                            if (j == 0 && starting_flowers[i] == 0) {
                                vcolumns_speed[adjustCOLUMNS(i)] = 1;
                                starting_flowers[i] = 1;
                                started_flowers++;
                            }
                    }
                }
                
            }
            break;
        }
        case (528 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Stop initial plants growing 
            for (i = 0; i < 10; i++) {
                vcolumns_speed[adjustCOLUMNS(i)] = 0;
            }
            break;
        }
        case (1360 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Secondary plants start growing
            u8 started_flowers = 0;
            while (started_flowers < 3) {
                for (i = 0; i < 10; i++) {
                    if (started_flowers < 3) {
                            int j = random() % 9;
                            if (j == 0 && starting_flowers[i] == 0) {
                                vcolumns_speed[adjustCOLUMNS(i)] = 1;
                                starting_flowers[i] = 1;
                                started_flowers++;
                            }
                    }
                }
                
            }
            break;
        }
        case (1410 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Stop secondary plants growing 
            for (i = 0; i < 10; i++) {
                vcolumns_speed[adjustCOLUMNS(i)] = 0;
                starting_flowers[i] = 0;  // Clears for future use later in State
            }
            break;
        }
        case (2640 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            SPR_setAnim(bird.sprite, BIRD_LAND);
            bird.vely = FIX16(0.25);
            break;
        }
        case (2704 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            SPR_setAnim(bird.sprite, BIRD_SIT);
            bird.vely = FIX16(0);
            break;
        }
        case (4204 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            for (i = 0; i < MAX_PEOPLE - 5; i++) { p++; }

            for (i = MAX_PEOPLE - 5; i < MAX_PEOPLE; i++) {
                p->x = i * 16;
                p->y = FIX16(2);
                p->w = 32;
                p->h = 32;
                p->velx = 0;
                p->vely = FIX16(0);
                p->hugged = 0;
                p->jumping = TRUE;
                p->sprite = SPR_addSprite(&spr_cat, p->x, p->y, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
                p->present = FALSE;
                p->direction = ( 2 * ( random() % 2 ) ) - 1;
                p->velx = p->direction;
                sprintf(p->name, "Cat%d", i);
                p->timerPeople = 0;
                peopleRemaining++;
                p++;

            }
            break;
        }
        case (5704 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            lamp_sparks_1a = SPR_addSprite(&spr_particles, 6 + 64 * 0, 94, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
            lamp_sparks_2a = SPR_addSprite(&spr_particles, 6 + 64 * 1, 94, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
            lamp_sparks_3a = SPR_addSprite(&spr_particles, 6 + 64 * 2, 94, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
            lamp_sparks_4a = SPR_addSprite(&spr_particles, 6 + 64 * 3, 94, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
            lamp_sparks_5a = SPR_addSprite(&spr_particles, 6 + 64 * 4, 94, TILE_ATTR(PAL0, 0, FALSE, FALSE) );
            break;
        }
        case (5716 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            lamp_sparks_1b = SPR_addSprite(&spr_particles, 30 + 64 * 0, 94, TILE_ATTR(PAL0, 0, FALSE, TRUE) );
            lamp_sparks_2b = SPR_addSprite(&spr_particles, 30 + 64 * 1, 94, TILE_ATTR(PAL0, 0, FALSE, TRUE) );
            lamp_sparks_3b = SPR_addSprite(&spr_particles, 30 + 64 * 2, 94, TILE_ATTR(PAL0, 0, FALSE, TRUE) );
            lamp_sparks_4b = SPR_addSprite(&spr_particles, 30 + 64 * 3, 94, TILE_ATTR(PAL0, 0, FALSE, TRUE) );
            lamp_sparks_5b = SPR_addSprite(&spr_particles, 30 + 64 * 4, 94, TILE_ATTR(PAL0, 0, FALSE, TRUE) );
            break;
        }
        case (5752 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            SPR_setAnim(lamp_sparks_1a, 1);
            SPR_setAnim(lamp_sparks_2a, 1);
            SPR_setAnim(lamp_sparks_3a, 1);
            SPR_setAnim(lamp_sparks_4a, 1);
            SPR_setAnim(lamp_sparks_5a, 1);
            break;
        }
        case (5764 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            SPR_setAnim(lamp_sparks_1b, 1);
            SPR_setAnim(lamp_sparks_2b, 1);
            SPR_setAnim(lamp_sparks_3b, 1);
            SPR_setAnim(lamp_sparks_4b, 1);
            SPR_setAnim(lamp_sparks_5b, 1);
            break;
        }
        case (6360 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Select random plants for partial full growth
            u8 started_flowers = 0;
            while (started_flowers < 5) {
                for (i = 0; i < 10; i++) {
                    if (started_flowers < 5) {
                            int j = random() % 9;
                            if (j == 0 && starting_flowers[i] == 0) {
                                vcolumns_speed[adjustCOLUMNS(i)] = 1;
                                starting_flowers[i] = 1;
                                started_flowers++;
                            }
                    }
                }
                
            }
            break;
        }
        case (6460 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Allow non-selected plants to grow fully
            for (i = 0; i < 10; i++) {
                //if (starting_flowers[i] == 0) {
                    vcolumns_speed[adjustCOLUMNS(i)] = 1;
                //}
            }
                
            break;
        }
        case (6520 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Stop partial growth plants early.
            for (i = 0; i < 10; i++) {
                if (starting_flowers[i] == 1) {
                    vcolumns_speed[adjustCOLUMNS(i)] = 0;
                }
            }
            break;
        }
        case (6700 - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            // Allow all plants to grow fully
            for (i = 0; i < 10; i++) {
                    vcolumns_speed[adjustCOLUMNS(i)] = 1;
            }
                
            break;
        }
        // End of CLASSIC_GAME
        case (DELAY_CLASSIC_GAME_LEN - DELAY_CLASSIC_GAME_INTRO - DELAY_CLASSIC_INTRO_JUMP):
        {
            biker.x = -biker.w;
            SPR_setAnim(biker.sprite, ANIM_BIKER_PEDAL);
            positionBiker();
            SPR_releaseSprite(player.sprite);
            SPR_releaseSprite(bird.sprite);

            SPR_releaseSprite(lamp_sparks_1a);
            SPR_releaseSprite(lamp_sparks_1b);
            SPR_releaseSprite(lamp_sparks_2a);
            SPR_releaseSprite(lamp_sparks_2b);
            SPR_releaseSprite(lamp_sparks_3a);
            SPR_releaseSprite(lamp_sparks_3b);
            SPR_releaseSprite(lamp_sparks_4a);
            SPR_releaseSprite(lamp_sparks_4b);
            SPR_releaseSprite(lamp_sparks_5a);
            SPR_releaseSprite(lamp_sparks_5b);
            
            for (i = 0; i < MAX_PEOPLE; i++) {
                Person* p = &people[i];
                SPR_releaseSprite( p->sprite );
                peopleRemaining--;
            }
            activateNPCs = 0;
            startDrop = FALSE;

            // Background Animation variable reset.
            for (i = 0; i < 10; i++) {
                vcolumns_offset[adjustCOLUMNS(i)] = 0;
                vcolumns_speed[adjustCOLUMNS(i)] = 0;
                starting_flowers[i] = 0;
            }

            transitionCLASSIC_GAMEtoCLASSIC_OUTRO();
            break;
        }
    }

    
    animateCOLUMNS(&vcolumns_offset, &vcolumns_speed);

    /*
    int playy = p->velx;
    char str_var[10] = "0";
    sprintf(str_var, "%d", playy);
    VDP_clearText(30, 10, 10);
    VDP_drawText(str_var, 30, 10);
    */

}


void stateCLASSIC_OUTRO() {
    // Screen cleared from transition
    timerSEGA++;
    switch(timerSEGA) {
        case DELAY_CLASSIC_OUTRO_BLACK:
        {
            //VDP_drawText("Start bike in from left", 2, 4);
            biker.velx = 1;
            break;
        }
        case (DELAY_CLASSIC_OUTRO_BIKE_ANIM + DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Stop Bike", 2, 5);
            biker.velx = 0;
            SPR_setAnim(biker.sprite, ANIM_BIKER_STOP);
            break;
        }
        case (DELAY_CLASSIC_OUTRO_BIKE_ANIM + 3 *DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("'yr totally rad!' - animation initiation", 2, 6);
            textActive = TRUE;
            text_x = biker.x - 48;
            personType = 3;
            timerText = 0;
            break;
        }
        case (DELAY_CLASSIC_OUTRO_BIKE_ANIM + 5 *DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("'go hug yrself!' - animation initiation", 2, 7);
            textActive = TRUE;
            text_x = biker.x - 48;
            personType = 4;
            timerText = 0;
            break;
        }
        case (DELAY_CLASSIC_OUTRO_BIKE_ANIM + 8 *DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Start bike leave to right (same rate as in)", 2, 8);
            biker.velx = 1;
            SPR_setAnim(biker.sprite, ANIM_BIKER_PEDAL);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 12 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            clearScreen();
            SPR_releaseSprite(biker.sprite);
            //VDP_drawText("Start credit anim", 2, 9);
            PAL_setColor(13, RGB24_TO_VDPCOLOR(0x000000));  // The big colour switch
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 0, 40, 9);
            VDP_fillTileMapRect(BG_A, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, 2), 0, 20, 40, 8);
            VDP_drawImageEx(BG_B, &classic_credits_0, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, -21, FALSE, DMA);
            //offsetB = 168; // +?
            offset_velB = -1;
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 12 * DELAY_CLASSIC_OUTRO_BLACK +60):
        {
            VDP_clearTileMapRect(BG_A, 0, 20, 40, 8);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 12 * DELAY_CLASSIC_OUTRO_BLACK + 182):
        {
            //VDP_drawText("Credits land", 2, 10);
            VDP_clearPlane(BG_A, TRUE);
            offset_velB = 0;
            offsetB = 0;
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 17 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Add music attrib", 2, 11);
            VDP_drawImageEx(BG_A, &classic_credits_1, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
            VDP_clearPlane(BG_B, TRUE);
            VDP_setVerticalScroll(BG_B, 0);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 19 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Add game attrib", 2, 12);
            VDP_drawImageEx(BG_A, &classic_credits_2, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 21 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Add port attrib", 2, 13);
            VDP_drawImageEx(BG_A, &classic_credits_3, TILE_ATTR_FULL(PAL0, 0, FALSE, FALSE, ind), 0, 0, FALSE, DMA);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 23 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            //VDP_drawText("Fade out", 2, 14);
            PAL_fadeOutPalette(PAL0, 240, FALSE);
            break;
        }
        case(DELAY_CLASSIC_OUTRO_BIKE_ANIM + 27 * DELAY_CLASSIC_OUTRO_BLACK):
        {
            transitionSEGAtoMENU();
            PAL_setPalette(PAL0, sega_splash.palette->data, DMA);
            PAL_setColor(15, PAL_getColor(1) );
            break;
        }
    }
    
    //SPR_update();
}


