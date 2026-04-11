#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// GAME SCREENS
typedef enum {
    SCREEN_MAIN_MENU, SCREEN_SURFACE, SCREEN_PLAYING, SCREEN_PENALTY,
    SCREEN_PAUSE, SCREEN_SHOP, SCREEN_STATS, SCREEN_INVENTORY,
    SCREEN_STATS_UPGRADE, SCREEN_COMBAT, SCREEN_QUESTION, SCREEN_GAMEOVER
} GameScreen;

// PLAYER
typedef struct {
    double oxygen; double oxygenMax;
    int gold; int depth; int questionCounter;
    int attack; int attacklevel;
    int defense; int defenselevel;
    int health; int healthMax; int healthlevel;
    int tanklevel; int weaponlevel; int armorlevel; int finlevel;
    int level; int skillpoints; int xp; int xpMax; int healthpotions;
    int monsterlevel; int enemyHealthmax; int enemyHealth;
    int enemyAttack; int enemyDefense; int enemyType; int timingilaDefeats;
    int encounters[7]; int defeats[7];
} Player;

// GLOBALS
#define MAX_BUBBLES 40
typedef struct { float x, y, speed, size; } Bubble;

GameScreen currentScreen = SCREEN_MAIN_MENU;
Bubble bubbles[MAX_BUBBLES];
Player p;
char logMessage[256] = "Welcome to Deep Sea Divers!"; 
bool shouldExitGame = false;

// Animation & Interaction Globals
float frameTimer = 0.0f;
int currentFrame = 0;
int diverAnimState = 0; 
int enemyAnimState = 0;
float diveAnimTimer = 0.0f;
float combatAnimTimer = 0.0f;
bool combatProcessing = false;

// TEXTURES & FONT
Font customFont;

Texture2D bgGameplay, bgShop, bgSurface, bgQuestion;
Texture2D btnBlank, btnStart, btnContinue, btnOption, btnExit, btnMenu, btnClose;
Texture2D icon_atlas, texDiver, texCrab, texShark, texWhale, texKraken, texLeviathan, texHydra, texTimingila;

// CRISP PIXEL ART LOADER
Texture2D LoadPixelTexture(const char* path) {
    Texture2D tex = LoadTexture(path);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    return tex;
}

// CUSTOM FONT WRAPPERS
void DrawTextCustom(const char *text, float x, float y, float fontSize, Color color) {
    DrawTextEx(customFont, text, (Vector2){x - 2, y}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x + 2, y}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x, y - 2}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x, y + 2}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x - 2, y - 2}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x + 2, y + 2}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x - 2, y + 2}, fontSize, 2.0f, BLACK);
    DrawTextEx(customFont, text, (Vector2){x + 2, y - 2}, fontSize, 2.0f, BLACK);
    
    DrawTextEx(customFont, text, (Vector2){x, y}, fontSize, 2.0f, color);
}

float MeasureTextCustom(const char *text, float fontSize) {
    return MeasureTextEx(customFont, text, fontSize, 2.0f).x;
}

// NEW: Word-Wrapping function to keep text inside the Question Slate!
void DrawTextWrappedCustom(const char *text, float centerX, float startY, float fontSize, float maxWidth, Color color) {
    char buffer[512];
    strncpy(buffer, text, 511);
    buffer[511] = '\0';
    
    char* lineStart = buffer;
    char* lastSpace = NULL;
    float currentY = startY;
    
    for (int i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == ' ') lastSpace = &buffer[i];
        
        char temp = buffer[i+1];
        buffer[i+1] = '\0';
        float currentWidth = MeasureTextCustom(lineStart, fontSize);
        buffer[i+1] = temp;
        
        if (currentWidth > maxWidth && lastSpace != NULL && lastSpace != lineStart) {
            *lastSpace = '\0'; 
            float w = MeasureTextCustom(lineStart, fontSize);
            DrawTextCustom(lineStart, centerX - w/2.0f, currentY, fontSize, color);
            
            lineStart = lastSpace + 1;
            lastSpace = NULL;
            currentY += fontSize + 10;
        }
    }
    float w = MeasureTextCustom(lineStart, fontSize);
    DrawTextCustom(lineStart, centerX - w/2.0f, currentY, fontSize, color);
}

// SPRITE BUTTON FUNCTIONS
bool SpriteButton(Rectangle destRect, Texture2D texture, const char *text) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, destRect);
    Color tint = hover ? LIGHTGRAY : WHITE;
    if (hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) tint = GRAY;

    Rectangle sourceRect = { 0, 0, (float)texture.width, (float)texture.height };
    DrawTexturePro(texture, sourceRect, destRect, (Vector2){0, 0}, 0.0f, tint);

    if (text != NULL && strlen(text) > 0) {
        float fontSize = destRect.height * 0.45f;
        float textWidth = MeasureTextCustom(text, fontSize);
        float textX = destRect.x + (destRect.width / 2.0f) - (textWidth / 2.0f);
        float textY = destRect.y + (destRect.height / 2.0f) - (fontSize / 2.0f);
        DrawTextCustom(text, textX, textY, fontSize, RAYWHITE);
    }
    return hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

bool AtlasButton(Texture2D atlas, Rectangle sourceRect, Rectangle destRect) {
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, destRect);
    Color tint = hover ? LIGHTGRAY : WHITE;
    if (hover && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) tint = GRAY;

    DrawTexturePro(atlas, sourceRect, destRect, (Vector2){0, 0}, 0.0f, tint);
    return hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

// DYNAMIC ROTATION SPRITE DRAWER
void DrawDiver(float x, float y, float size, float rotation) {
    int dFrameW = texDiver.width / 4;
    int dFrameH = texDiver.height / 4;
    Rectangle src = { currentFrame * dFrameW, diverAnimState * dFrameH, (float)dFrameW, (float)dFrameH };
    Rectangle dest = { x + size/2.0f, y + size/2.0f, size, size };
    Vector2 origin = { size/2.0f, size/2.0f };
    DrawTexturePro(texDiver, src, dest, origin, rotation, WHITE);
}

// REUSABLE HUD SYSTEM
void DrawHUD(Player *p, const char* logMsg) {
    DrawRectangle(650, 20, 620, 100, Fade(BLACK, 0.85f));
    DrawTextCustom(TextFormat("Depth: %d m", p->depth), 700, 35, 30, WHITE);
    DrawTextCustom(TextFormat("O2: %d / %d", (int)p->oxygen, (int)p->oxygenMax), 950, 35, 30, (p->oxygen < 20) ? RED : LIME);
    DrawTextCustom(TextFormat("HP: %d / %d", p->health, p->healthMax), 700, 75, 30, GREEN);
    DrawTextCustom(TextFormat("Gold: %d", p->gold), 950, 75, 30, GOLD);
    
    DrawRectangle(30, 930, 800, 120, Fade(BLACK, 0.85f));
    DrawTextCustom("Captain's Log:", 50, 950, 28, GRAY);
    DrawTextCustom(logMsg, 50, 990, 26, WHITE);
}

// FULL TERMINAL KNOWLEDGE GATE ARRAYS
#define TOTAL_EASY_QUESTIONS 50
#define TOTAL_MEDIUM_QUESTIONS 25
#define TOTAL_HARD_QUESTIONS 10

struct MCQ { char question[200]; char options[4][100]; char answer; };

struct MCQ mcqeasy[TOTAL_EASY_QUESTIONS] = {
    {"Who developed C language?", {"A) Dennis Ritchie","B) James Gosling","C) Bjarne Stroustrup","D) Guido van Rossum"},'A'},
    {"C language was developed at?", {"A) Microsoft","B) AT&T Bell Labs","C) IBM","D) Google"},'B'},
    {"Which symbol ends a statement in C?", {"A) :","B) ;","C) .","D) ,"},'B'},
    {"Which header file is used for printf()?", {"A) stdio.h","B) math.h","C) string.h","D) conio.h"},'A'},
    {"Which data type stores integers?", {"A) float","B) char","C) int","D) double"},'C'},
    {"Which function is used to take input?", {"A) input()","B) scanf()","C) gets()","D) read()"},'B'},
    {"Which operator is used for addition?", {"A) +","B) -","C) *","D) /"},'A'},
    {"Which loop executes at least once?", {"A) for","B) while","C) do-while","D) none"},'C'},
    {"Array index starts from?", {"A) 0","B) 1","C) -1","D) 2"},'A'},
    {"Which operator is assignment?", {"A) =","B) ==","C) !=","D) <="},'A'},
    {"Which operator is comparison?", {"A) =","B) ==","C) +","D) %"},'B'},
    {"Which data type stores single character?", {"A) char","B) int","C) float","D) string"},'A'},
    {"Which keyword is used for loop?", {"A) for","B) loop","C) repeat","D) iterate"},'A'},
    {"Which keyword exits a loop?", {"A) stop","B) break","C) exit","D) quit"},'B'},
    {"Which keyword skips iteration?", {"A) continue","B) skip","C) pass","D) next"},'A'},
    {"Which header file is for math functions?", {"A) stdio.h","B) math.h","C) stdlib.h","D) string.h"},'B'},
    {"Which operator is modulus?", {"A) %","B) /","C) *","D) //"},'A'},
    {"Which keyword defines structure?", {"A) class","B) struct","C) record","D) object"},'B'},
    {"Which keyword defines union?", {"A) combine","B) union","C) mix","D) group"},'B'},
    {"Which function calculates string length?", {"A) len()","B) strlen()","C) size()","D) length()"},'B'},
    {"Which function compares strings?", {"A) strcmp()","B) compare()","C) strcomp()","D) equal()"},'A'},
    {"Which function copies string?", {"A) strcpy()","B) strcopy()","C) copy()","D) memcpy()"},'A'},
    {"Which keyword is used to return value?", {"A) give","B) send","C) return","D) output"},'C'},
    {"Which keyword prevents modification?", {"A) static","B) const","C) auto","D) register"},'B'},
    {"Which function allocates memory?", {"A) malloc()","B) alloc()","C) new()","D) create()"},'A'},
    {"Which function frees memory?", {"A) delete()","B) remove()","C) free()","D) clear()"},'C'},
    {"Which header file contains exit()?", {"A) stdio.h","B) stdlib.h","C) math.h","D) string.h"},'B'},
    {"Which operator is logical AND?", {"A) &","B) &&","C) |","D) ||"},'B'},
    {"Which operator is logical OR?", {"A) &","B) &&","C) |","D) ||"},'D'},
    {"Which operator is NOT logical?", {"A) &&","B) ||","C) !","D) %"},'D'},
    {"Which operator increments value?", {"A) ++","B) ","C) +=","D) =="},'A'},
    {"Which operator decrements value?", {"A) ++","B) ","C) -=","D) =="},'B'},
    {"Which operator checks inequality?", {"A) !=","B) ==","C) >=","D) <="},'A'},
    {"Which operator is less than?", {"A) >","B) <","C) =","D) !"},'B'},
    {"Which operator is greater than?", {"A) >","B) <","C) =","D) !"},'A'},
    {"Which function prints output?", {"A) print()","B) printf()","C) display()","D) show()"},'B'},
    {"Which function reads character?", {"A) getc()","B) input()","C) read()","D) scan()"},'A'},
    {"Which data type has highest precision?", {"A) float","B) int","C) double","D) char"},'C'},
    {"Which type stores large integers?", {"A) short","B) long","C) char","D) float"},'B'},
    {"Which symbol is address operator?", {"A) *","B) &","C) %","D) #"},'B'},
    {"Which symbol is dereference operator?", {"A) &","B) *","C) %","D) #"},'B'},
    {"Which is valid variable name?", {"A) 1num","B) num_1","C) num-1","D) @num"},'B'},
    {"Which storage class is default?", {"A) auto","B) static","C) extern","D) register"},'A'},
    {"Which keyword is used in switch?", {"A) case","B) option","C) select","D) choose"},'A'},
    {"Which statement is used for decision making?", {"A) if","B) loop","C) iterate","D) run"},'A'},
    {"Which loop checks condition first?", {"A) do-while","B) while","C) none","D) both"},'B'},
    {"Which comment is single line?", {"A) //","B) /* */","C) #","D) "},'A'},
    {"Which comment is multi-line?", {"A) //","B) /* */","C) #","D) "},'B'},
    {"Which header file handles strings?", {"A) string.h","B) stdio.h","C) math.h","D) stdlib.h"},'A'},
    {"Which keyword is used to define constant macro?", {"A) #define","B) const","C) static","D) auto"},'A'}
};

struct MCQ mcqmedium[TOTAL_MEDIUM_QUESTIONS] = {
    {"What will be the output of printf(\"%d\", 10 > 9 > 8);", {"A) 1", "B) 0", "C) Garbage value", "D) Error"}, 'B'},
    {"What is the size of int in C (generally)?", {"A) 2 bytes", "B) 4 bytes", "C) 8 bytes", "D) Depends on compiler"}, 'D'},
    {"What will sizeof('A') return?", {"A) 1", "B) 2", "C) 4", "D) Error"}, 'C'},
    {"What is the output?\nint x=5; printf(\"%d\", x++);", {"A) 5", "B) 6", "C) Error", "D) Garbage"}, 'A'},
    {"Which is correct pointer declaration?", {"A) int p;", "B) int *p;", "C) pointer p;", "D) *int p;"}, 'B'},
    {"What will be output?\nint a=5; printf(\"%d\", ++a);", {"A) 5", "B) 6", "C) 7", "D) Error"}, 'B'},
    {"Which operator has highest precedence?", {"A) *", "B) +", "C) =", "D) &&"}, 'A'},
    {"What is NULL pointer?", {"A) Pointer with value 0", "B) Empty pointer", "C) Unused pointer", "D) Random pointer"}, 'A'},
    {"Which function dynamically allocates memory?", {"A) calloc()", "B) create()", "C) alloc()", "D) new()"}, 'A'},
    {"What will be output?\nprintf(\"%d\", 5 & 3);", {"A) 1", "B) 2", "C) 3", "D) 0"}, 'A'},
    {"What is default return type of function if not specified?", {"A) void", "B) int", "C) float", "D) char"}, 'B'},
    {"Which storage class retains value between function calls?", {"A) auto", "B) register", "C) static", "D) extern"}, 'C'},
    {"Which operator is used for bitwise OR?", {"A) ||", "B) |", "C) &&", "D) &"}, 'B'},
    {"What will be output?\nint a=3,b=4; printf(\"%d\", a^b);", {"A) 7", "B) 1", "C) 0", "D) 2"}, 'A'},
    {"Which keyword is used to access global variable inside function?", {"A) extern", "B) static", "C) global", "D) auto"}, 'A'},
    {"Which function releases dynamically allocated memory?", {"A) remove()", "B) free()", "C) delete()", "D) clear()"}, 'B'},
    {"What is the output?\nprintf(\"%d\", 5<<1);", {"A) 5", "B) 10", "C) 2", "D) 1"}, 'B'},
    {"Which header file is required for malloc()?", {"A) stdio.h", "B) string.h", "C) stdlib.h", "D) math.h"}, 'C'},
    {"Which loop is best when iterations unknown?", {"A) for", "B) while", "C) do-while", "D) switch"}, 'B'},
    {"What will be output?\nint a=5; printf(\"%d\", a);", {"A) 4", "B) 5", "C) 6", "D) Error"}, 'B'},
    {"Which symbol is used for structure member access (pointer)?", {"A) .", "B) ->", "C) :", "D) *"}, 'B'},
    {"What is recursion?", {"A) Loop inside loop", "B) Function calling itself", "C) Infinite loop", "D) Memory allocation"}, 'B'},
    {"Which function reads formatted input?", {"A) scanf()", "B) gets()", "C) getchar()", "D) read()"}, 'A'},
    {"What will be output?\nprintf(\"%d\", 7%4);", {"A) 3", "B) 2", "C) 1", "D) 0"}, 'A'},
    {"Which operator is used to access value at address?", {"A) &", "B) *", "C) %", "D) #"}, 'B'}
};

struct MCQ mcqhard[TOTAL_HARD_QUESTIONS] = {
    {"What will be output?\nint a=5; printf(\"%d %d %d\", a, a++, ++a);", {"A) 5 5 7", "B) 5 6 7", "C) Undefined behavior", "D) 7 6 5"}, 'C'},
    {"What is the output?\nprintf(\"%d\", sizeof(5.5));", {"A) 4", "B) 8", "C) 2", "D) Depends on compiler"}, 'D'},
    {"What will be output?\nint x=10; if(x=0) printf(\"Hi\"); else printf(\"Hello\");", {"A) Hi", "B) Hello", "C) Error", "D) Garbage"}, 'B'},
    {"Which is correct about 'register' keyword?", {"A) Stored in RAM", "B) Stored in CPU register", "C) Global variable", "D) Constant variable"}, 'B'},
    {"What will be output?\nint a=3; printf(\"%d\", a<<2);", {"A) 6", "B) 12", "C) 9", "D) 5"}, 'B'},
    {"What will be output?\nint a=5; printf(\"%d\", ~a);", {"A) -6", "B) 6", "C) -5", "D) 5"}, 'A'},
    {"What is a dangling pointer?", {"A) Pointer with NULL value", "B) Pointer pointing to freed memory", "C) Uninitialized pointer", "D) Global pointer"}, 'B'},
    {"Which function allocates contiguous memory for array?", {"A) malloc()", "B) calloc()", "C) realloc()", "D) free()"}, 'B'},
    {"What will be output?\nint a[]={1,2,3}; printf(\"%d\", *(a+1));", {"A) 1", "B) 2", "C) 3", "D) Error"}, 'B'},
    {"Which operator has lowest precedence?", {"A) +", "B) *", "C) =", "D) &&"}, 'C'}
};

struct MCQ currentActiveQuestion;

void setupQuestion(Player *p) {
    if (p->questionCounter <= 25) { 
        currentActiveQuestion = mcqeasy[rand() % TOTAL_EASY_QUESTIONS];
    } else if (p->questionCounter <= 40) { 
        currentActiveQuestion = mcqmedium[rand() % TOTAL_MEDIUM_QUESTIONS];
    } else { 
        currentActiveQuestion = mcqhard[rand() % TOTAL_HARD_QUESTIONS];
    }
}

// TERMINAL LOGIC FUNCTIONS
void resetPlayer(Player *p) {
    p->oxygenMax = 100; p->oxygen = p->oxygenMax; p->healthMax = 100; p->health = p->healthMax;
    p->depth = 0; p->questionCounter = 1; p->gold = 100; p->attack = 10;
    p->attacklevel = 0; p->defense = 5; p->defenselevel = 0; p->healthlevel = 0;
    p->level = 1; p->skillpoints = 2; p->xpMax = 100; p->xp = 0;
    p->finlevel = 0; p->tanklevel = 0; p->weaponlevel = 0; p->armorlevel = 0;
    p->healthpotions = 3; p->timingilaDefeats = 0;
    for(int i=0; i<7; i++) { p->encounters[i]=0; p->defeats[i]=0; }
    sprintf(logMessage, "Welcome to Deep Sea Divers!");
    diveAnimTimer = 0.0f; combatAnimTimer = 0.0f; combatProcessing = false;
    diverAnimState = 0; enemyAnimState = 0;
}

void updateStats(Player *p) {
    p->oxygenMax = 100 + (20 * p->tanklevel);
    p->healthMax = 100 + (20 * p->healthlevel);
    p->attack = 10 + (5 * p->attacklevel);
    p->defense = 5 + (3 * p->defenselevel);
}

void levelup(Player *p) {
    while (p->xp >= p->xpMax) {
        p->xp -= p->xpMax; p->level++; p->xpMax = 100 * p->level; p->skillpoints += 2;
        sprintf(logMessage, "*** LEVEL %d! *** You have %d skill points.", p->level, p->skillpoints);
    }
}

double divingcost(Player *p) {
    double cost = (p->depth + 2) / (1 + 0.1 * p->finlevel);
    return cost < 1 ? 1 : cost;
}

void setupEnemy(Player *p) {
    p->monsterlevel = floor(p->depth / 10.0);
    if (p->depth <= 10) p->enemyType = 1; else if (p->depth <= 20) p->enemyType = (rand() % 2) + 1;
    else if (p->depth <= 40) p->enemyType = (rand() % 3) + 1; else if (p->depth <= 80) p->enemyType = (rand() % 4) + 1;
    else if (p->depth <= 130) p->enemyType = (rand() % 5) + 1; else if (p->depth <= 180) p->enemyType = (rand() % 6) + 1;
    else p->enemyType = (rand() % 7) + 1;

    if (p->depth > 0 && p->depth % 250 == 0) p->enemyType = 7; 

    int m = p->monsterlevel;
    switch(p->enemyType) {
        case 1: p->enemyHealthmax = 20 + 5*m;   p->enemyAttack = 5 + 2*m;   p->enemyDefense = 2 + m; break;
        case 2: p->enemyHealthmax = 40 + 10*m;  p->enemyAttack = 10 + 4*m;  p->enemyDefense = 4 + 2*m; break;
        case 3: p->enemyHealthmax = 60 + 15*m;  p->enemyAttack = 15 + 6*m;  p->enemyDefense = 6 + 3*m; break;
        case 4: p->enemyHealthmax = 80 + 20*m;  p->enemyAttack = 20 + 8*m;  p->enemyDefense = 8 + 4*m; break;
        case 5: p->enemyHealthmax = 100 + 25*m; p->enemyAttack = 25 + 10*m; p->enemyDefense = 10 + 5*m; break;
        case 6: p->enemyHealthmax = 150 + 30*m; p->enemyAttack = 30 + 12*m; p->enemyDefense = 12 + 6*m; break;
        case 7: p->enemyHealthmax = 500 + 50*m; p->enemyAttack = 50 + 20*m; p->enemyDefense = 20 + 10*m; break;
    }
    p->enemyHealth = p->enemyHealthmax; 
    p->encounters[p->enemyType - 1]++;
    enemyAnimState = 0; 
}

int getDamageDealt(Player *p) { return ((p->attack * (1.0 + 0.1 * p->weaponlevel)) - p->enemyDefense) < 1 ? 1 : ((p->attack * (1.0 + 0.1 * p->weaponlevel)) - p->enemyDefense); }
int getDamageTaken(Player *p) { return (p->enemyAttack - (p->defense * (1.0 + 0.1 * p->armorlevel))) < 1 ? 1 : (p->enemyAttack - (p->defense * (1.0 + 0.1 * p->armorlevel))); }

void positiveEvents(Player *p) {
    int event = rand() % 10;
    if (event == 0 || event == 3 || event == 6 || event == 9) { sprintf(logMessage,"Found gold pouch! Gain %d", 5*(p->depth + 1)); p->gold += 5*(p->depth + 1); }
    else if (event == 1 || event == 4) { sprintf(logMessage,"Found Chest! Gain %d gold!", 10*(p->depth + 1)); p->gold += 10*(p->depth + 1); }
    else if (event == 2 || event == 5 || event == 7) { sprintf(logMessage,"Found potion! Healed %d HP!", 10 + 5*p->healthlevel); p->health += 10 + 5*p->healthlevel; if(p->health > p->healthMax) p->health = p->healthMax; }
    else if (event == 8) { sprintf(logMessage,"Found O2 tank! Filled %d O2!", 5 + 5*p->tanklevel); p->oxygen += 5 + 5*p->tanklevel; if(p->oxygen > p->oxygenMax) p->oxygen = p->oxygenMax; }
}

void negativeEvents(Player *p) {
    int event = rand() % 10;
    if (event == 0 || event == 3 || event == 6 || event == 9) { sprintf(logMessage,"Current slams you! Lost 5 O2."); p->oxygen -= 5 * p->tanklevel; if (p->gold > 10) { p->gold -= 10 * p->questionCounter; strcat(logMessage, " Dropped 10 gold!"); } }
    else if (event == 1 || event == 4 || event == 7) { sprintf(logMessage,"Squid squeezes tank! Lost 10 O2."); p->oxygen -= 10 * p->tanklevel; }
    else if (event == 2 || event == 5) { sprintf(logMessage,"Jellyfish sting! Lost 10 HP."); p->health -= 10 * p->healthlevel; }
    else if (event == 8) { sprintf(logMessage,"Nitrogen panic! Ascend 10m."); p->depth -= 10; if (p->depth < 0) p->depth = 0; }
}

// VISUAL EFFECTS
void InitBubbles() {
    for(int i=0;i<MAX_BUBBLES;i++){
        bubbles[i].x = GetRandomValue(0, 1920); bubbles[i].y = GetRandomValue(0, 1080);
        bubbles[i].speed = GetRandomValue(20, 60)/10.0f; bubbles[i].size = GetRandomValue(2,6);
    }
}
void UpdateBubbles() {
    for(int i=0;i<MAX_BUBBLES;i++){
        bubbles[i].y -= bubbles[i].speed;
        if(bubbles[i].y < -10){ bubbles[i].y = 1100; bubbles[i].x = GetRandomValue(0, 1920); }
    }
}
void DrawBubbles() {
    for(int i=0;i<MAX_BUBBLES;i++) DrawCircle(bubbles[i].x, bubbles[i].y, bubbles[i].size, Fade(SKYBLUE,0.4f));
}

// MAIN
int main(void)
{
    InitWindow(1920, 1080, "Deep Sea Divers");
    SetTargetFPS(60);
    srand(time(NULL));

    customFont = LoadFontEx("assets/font/Star Crush.ttf", 64, 0, 250);
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_POINT);

    bgGameplay = LoadPixelTexture("assets/background/Gameplay.png");
    bgShop = LoadPixelTexture("assets/background/Shop.png"); 
    bgSurface = LoadPixelTexture("assets/background/Surface.png");
    bgQuestion = LoadPixelTexture("assets/slates/question_slate.png"); 
    
    btnBlank = LoadPixelTexture("assets/icon/Blank_Button.png");
    btnStart = LoadPixelTexture("assets/icon/start.png");
    btnContinue = LoadPixelTexture("assets/icon/continue.png");
    btnOption = LoadPixelTexture("assets/icon/option.png");
    btnExit = LoadPixelTexture("assets/icon/exit button.png");
    btnMenu = LoadPixelTexture("assets/icon/Menu_Button.png");
    btnClose = LoadPixelTexture("assets/icon/Close.png");
    
    icon_atlas = LoadPixelTexture("assets/stat/icon_atlas.png"); 
    
    texDiver = LoadPixelTexture("assets/diver/diver.png");
    texCrab = LoadPixelTexture("assets/enemy/crab.png");
    texShark = LoadPixelTexture("assets/enemy/shark.png");
    texWhale = LoadPixelTexture("assets/enemy/whale.png");
    texKraken = LoadPixelTexture("assets/enemy/kraken.png");
    texLeviathan = LoadPixelTexture("assets/enemy/leviathan.png");
    texHydra = LoadPixelTexture("assets/enemy/hydra.png");
    texTimingila = LoadPixelTexture("assets/enemy/timingila.png");

    resetPlayer(&p);
    InitBubbles();

    while (!WindowShouldClose() && !shouldExitGame)
    {
        frameTimer += GetFrameTime();
        if (frameTimer >= 0.15f) {
            currentFrame = (currentFrame + 1) % 4;
            frameTimer = 0.0f;
        }

        if (combatAnimTimer > 0) {
            combatAnimTimer -= GetFrameTime();
            if (combatAnimTimer > 0.4f) {
                diverAnimState = 2; // Player Attack 
                enemyAnimState = 3; // Enemy Hurt
            } else {
                diverAnimState = 3; // Player Hurt 
                enemyAnimState = 2; // Enemy Attack 
            }
        } else {
            if (currentScreen == SCREEN_COMBAT || currentScreen == SCREEN_PLAYING) {
                diverAnimState = 0; 
                enemyAnimState = 0; 
            }
            
            if (combatProcessing) {
                combatProcessing = false;
                if (p.health <= 0) currentScreen = SCREEN_GAMEOVER;
                else if (p.enemyHealth <= 0) {
                    if (p.enemyType == 7) p.timingilaDefeats++; p.defeats[p.enemyType - 1]++;
                    p.xp += 20 * p.enemyType * (1 + 0.1 * p.monsterlevel); 
                    p.gold += 10 * p.enemyType + (5 * p.monsterlevel);
                    levelup(&p); currentScreen = SCREEN_PLAYING;
                }
            }
        }

        if (!combatProcessing && currentScreen != SCREEN_GAMEOVER && (p.health <= 0 || p.oxygen <= 0)) {
            currentScreen = SCREEN_GAMEOVER;
        }

        UpdateBubbles();
        BeginDrawing();
        ClearBackground(BLACK);

        if (currentScreen == SCREEN_MAIN_MENU) {
            DrawTexturePro(bgSurface, (Rectangle){0,0,bgSurface.width,bgSurface.height}, (Rectangle){0,0,1920,1080}, (Vector2){0,0}, 0, WHITE);
        } else if (currentScreen == SCREEN_SHOP) {
            DrawTexturePro(bgShop, (Rectangle){0,0,bgShop.width,bgShop.height}, (Rectangle){0,0,1920,1080}, (Vector2){0,0}, 0, WHITE);
        } else {
            if (p.depth == 0) {
                DrawTexturePro(bgSurface, (Rectangle){0,0,bgSurface.width,bgSurface.height}, (Rectangle){0,0,1920,1080}, (Vector2){0,0}, 0, WHITE);
            } else {
                Color depthTint = { 255 - (p.depth), 255 - (p.depth), 255 - (p.depth/2), 255 }; 
                DrawTexturePro(bgGameplay, (Rectangle){0,0,bgGameplay.width,bgGameplay.height}, (Rectangle){0,0,1920,1080}, (Vector2){0,0}, 0, depthTint);
                DrawBubbles();
            }

            if (currentScreen == SCREEN_PAUSE || currentScreen == SCREEN_STATS || currentScreen == SCREEN_STATS_UPGRADE || currentScreen == SCREEN_INVENTORY || currentScreen == SCREEN_PENALTY || currentScreen == SCREEN_QUESTION) {
                DrawRectangle(0, 0, 1920, 1080, Fade(BLACK, 0.7f)); 
            }
        }

        switch(currentScreen)
        {
            case SCREEN_MAIN_MENU:
                DrawTextCustom("DEEP SEA DIVERS", 960 - MeasureTextCustom("DEEP SEA DIVERS", 100)/2, 280, 100, WHITE);
                if (SpriteButton((Rectangle){1920/2 - 140, 480, 280, 70}, btnStart, "")) { resetPlayer(&p); currentScreen = SCREEN_SURFACE; }
                if (SpriteButton((Rectangle){1920/2 - 40, 600, 80, 80}, btnExit, "")) shouldExitGame = true;
                break;
            
            case SCREEN_SURFACE:
                DrawHUD(&p, logMessage); 
                
                if (SpriteButton((Rectangle){40, 40, 200, 60}, btnBlank, "MENU")) currentScreen = SCREEN_PAUSE;

                if (SpriteButton((Rectangle){1920/2 - 140, 450, 280, 70}, btnBlank, "DIVE IN!")) { 
                    double cost = divingcost(&p);
                    if (p.oxygen >= cost) {
                        p.oxygen -= cost; p.depth++; positiveEvents(&p); 
                        diveAnimTimer = 1.0f; currentScreen = SCREEN_PLAYING; 
                    } else {
                        sprintf(logMessage, "Not enough oxygen to dive!");
                    }
                }
                if (SpriteButton((Rectangle){1920/2 - 140, 560, 280, 70}, btnBlank, "SHOP")) currentScreen = SCREEN_SHOP;
                break;   

            case SCREEN_PLAYING:
                float diverRot = 90.0f; 
                if (diveAnimTimer > 0) {
                    diveAnimTimer -= GetFrameTime();
                    diverRot = 00.0f; 
                    diverAnimState = 1; 
                }
                DrawDiver(1920/2 - 64, 1080/2 - 64, 128, diverRot);

                DrawHUD(&p, logMessage); 

                if (SpriteButton((Rectangle){40, 40, 200, 60}, btnBlank, "MENU") && !combatProcessing) currentScreen = SCREEN_PAUSE;

                if (SpriteButton((Rectangle){1600, 750, 280, 70}, btnBlank, "Dive Deeper")) {
                    double cost = divingcost(&p);
                    if(p.oxygen >= cost) {
                        p.oxygen -= cost; p.depth++; diveAnimTimer = 1.0f; sprintf(logMessage, "Diving deeper...");
                        if (p.depth > 0 && p.depth % (5 * p.questionCounter) == 0) { setupQuestion(&p); currentScreen = SCREEN_QUESTION; } 
                        else if (p.depth > 0 && p.depth % 250 == 0) { setupEnemy(&p); currentScreen = SCREEN_COMBAT; } 
                        else if (rand() % 2 == 0) { 
                            int ev = rand() % 10;
                            if (ev == 0 || ev == 2 || ev == 4 || ev == 8) positiveEvents(&p);
                            else if(ev == 1 || ev == 3 || ev == 5) negativeEvents(&p);
                            else if (ev == 6 || ev == 7) { setupEnemy(&p); currentScreen = SCREEN_COMBAT; }
                        }
                    } else {
                        p.oxygen -= cost/2; p.depth; if (p.depth < 0) p.depth = 0; sprintf(logMessage, "Not enough oxygen! Ascending.");
                    }
                }
                
                if (SpriteButton((Rectangle){1600, 850, 280, 70}, btnBlank, "Stay at Depth")) {
                    double cost = divingcost(&p)/2;
                    if(p.oxygen >= cost){
                        p.oxygen -= cost; diverAnimState = 0; sprintf(logMessage, "Catching breath.");
                        if (rand() % 4 == 0) { 
                            int ev = rand() % 10;
                            if (ev == 0 || ev == 2 || ev == 4 || ev == 8) positiveEvents(&p);
                            else if(ev == 1 || ev == 3 || ev == 5) negativeEvents(&p);
                            else if (ev == 6 || ev == 7) { setupEnemy(&p); currentScreen = SCREEN_COMBAT; }
                        }
                    } else { p.oxygen -= divingcost(&p)/4; p.depth; if (p.depth < 0) p.depth = 0; sprintf(logMessage, "Not enough oxygen. You ascend."); }
                }
                
                if (SpriteButton((Rectangle){1600, 950, 280, 70}, btnBlank, "Surface")) {
                    if (p.gold >= (p.depth * 5)) { p.gold -= (p.depth * 5); p.depth = 0; p.oxygen = p.oxygenMax; currentScreen = SCREEN_SURFACE; } 
                    else currentScreen = SCREEN_PENALTY;
                }
                break;

            case SCREEN_COMBAT:
                DrawDiver(400, 450, 160, 00.0f);

                Texture2D activeEnemyTex;
                switch(p.enemyType) {
                    case 1: activeEnemyTex = texCrab; break; case 2: activeEnemyTex = texShark; break;
                    case 3: activeEnemyTex = texWhale; break; case 4: activeEnemyTex = texKraken; break;
                    case 5: activeEnemyTex = texLeviathan; break; case 6: activeEnemyTex = texHydra; break;
                    case 7: activeEnemyTex = texTimingila; break;
                }
                
                int eFrameW = activeEnemyTex.width / 4;
                int eFrameH = activeEnemyTex.height / 4;
                Rectangle eSrc = { currentFrame * eFrameW, enemyAnimState * eFrameH, (float)-eFrameW, (float)eFrameH };
                DrawTexturePro(activeEnemyTex, eSrc, (Rectangle){ 1300, 450 - (p.enemyType == 7 ? 80 : 0), 160, 160 }, (Vector2){0,0}, 0, WHITE);

                char monsterNames[7][20] = {"Crab", "Shark", "Whale", "Kraken", "Leviathan", "Hydra", "Timingila"};
                DrawTextCustom("COMBAT!", 850, 100, 60, RED);
                DrawTextCustom(TextFormat("%s (Lv %d)", monsterNames[p.enemyType - 1], p.monsterlevel), 1200, 310, 40, WHITE);
                DrawTextCustom(TextFormat("HP: %d / %d", p.enemyHealth, p.enemyHealthmax), 1200, 360, 36, RED);
                DrawTextCustom(TextFormat("Your HP: %d / %d", p.health, p.healthMax), 300, 360, 36, LIME);

                if (SpriteButton((Rectangle){820, 800, 280, 70}, btnBlank, "Fight!") && !combatProcessing) {
                    combatProcessing = true; combatAnimTimer = 0.8f; 
                    int dealt = getDamageDealt(&p); p.enemyHealth -= dealt;
                    if (p.enemyHealth > 0) { 
                        int taken = getDamageTaken(&p); p.health -= taken; 
                        sprintf(logMessage, "Dealt %d and took %d damage!", dealt, taken); 
                    } else { sprintf(logMessage, "Dealt %d damage! Enemy defeated!", dealt); }
                }
                
                if (SpriteButton((Rectangle){820, 890, 280, 70}, btnBlank, "Use Potion") && !combatProcessing) {
                    if (p.healthpotions > 0) { 
                        p.healthpotions; int healAmt = 50 + 5 * p.healthlevel; 
                        p.health += healAmt; if(p.health > p.healthMax) p.health = p.healthMax; 
                        int taken = getDamageTaken(&p); p.health -= taken; 
                        sprintf(logMessage, "Healed %d HP but took %d damage!", healAmt, taken); 
                    } else sprintf(logMessage, "No potions left!");
                }
                
                if (SpriteButton((Rectangle){40, 40, 80, 80}, btnExit, "") && !combatProcessing) {
                    sprintf(logMessage, "Ran away!"); currentScreen = SCREEN_PLAYING;
                }
                break;

            case SCREEN_SHOP:
                DrawTextCustom("ABYSS SHOP", 1920/2 - MeasureTextCustom("ABYSS SHOP", 70)/2, 120, 70, ORANGE);
                char goldStr[32]; sprintf(goldStr, "Gold: %d", p.gold);
                DrawTextCustom(goldStr, 1920/2 - MeasureTextCustom(goldStr, 40)/2, 200, 40, GOLD);
                
                if (SpriteButton((Rectangle){1920/2 - 180, 920, 360, 70}, btnBlank, "Return to Game")) currentScreen = (p.depth > 0) ? SCREEN_PLAYING : SCREEN_SURFACE;

                int costs[5] = { 100, 100, 100, 100, 50 };
                int levels[4] = { p.tanklevel, p.weaponlevel, p.armorlevel, p.finlevel };

                float atlasW = icon_atlas.width / 5.0f;
                float atlasH = icon_atlas.height / 3.0f;
                int atlasCols[5] = {0, 2, 3, 4, 1}; 
                
                int startY = 280;
                int spacingY = 120;
                int iconSize = 90; 
                int startX = 1920/2 - 120; 

                for (int i = 0; i < 5; i++) {
                    int col = atlasCols[i];
                    Rectangle src = { col * atlasW, 0.0f, atlasW, atlasH };
                    Rectangle dest = { startX, startY + (i * spacingY), iconSize, iconSize };
                    bool canBuy = p.gold >= costs[i];
                    
                    if (AtlasButton(icon_atlas, src, dest)) {
                        if (canBuy) {
                            p.gold -= costs[i];
                            switch(i) { case 0: p.tanklevel++; p.oxygen = p.oxygenMax; break; case 1: p.weaponlevel++; break; case 2: p.armorlevel++; break; case 3: p.finlevel++; break; case 4: p.healthpotions++; break; }
                            updateStats(&p);
                        }
                    }
                    
                    if (i < 4) {
                        DrawTextCustom(TextFormat("Lv %d", levels[i]), dest.x + 5, dest.y + 5, 20, WHITE);
                    } else {
                        DrawTextCustom(TextFormat("Own: %d", p.healthpotions), dest.x + 5, dest.y + 5, 20, WHITE);
                    }

                    char costStr[16]; sprintf(costStr, "%d G", costs[i]);
                    DrawTextCustom(costStr, dest.x + 130, dest.y + 25, 40, canBuy ? GOLD : RED);
                }
                break;

            case SCREEN_PENALTY:
                DrawTextCustom("SURFACE PENALTY", 1920/2 - MeasureTextCustom("SURFACE PENALTY", 60)/2, 260, 60, ORANGE);
                DrawTextCustom(TextFormat("Need %d Gold to surface safely.", p.depth * 5), 1920/2 - MeasureTextCustom(TextFormat("Need %d Gold to surface safely.", p.depth * 5), 36)/2, 360, 36, WHITE);
                DrawTextCustom("Take penalty? (Lose 50% HP & Gold)", 1920/2 - MeasureTextCustom("Take penalty? (Lose 50% HP & All Gold)", 36)/2, 420, 36, RED);
                
                if (SpriteButton((Rectangle){1920/2 - 280, 520, 240, 70}, btnBlank, "ACCEPT")) {
                    p.health /= 2; p.gold = 0; p.depth = 0; p.oxygen = p.oxygenMax;
                    sprintf(logMessage, "Surfaced safely but took damage!"); currentScreen = SCREEN_SURFACE;
                }
                if (SpriteButton((Rectangle){1920/2 + 40, 520, 240, 70}, btnBlank, "CANCEL")) currentScreen = SCREEN_PLAYING;
                break;

            case SCREEN_QUESTION:
                DrawTexturePro(bgQuestion, (Rectangle){0,0,bgQuestion.width,bgQuestion.height}, (Rectangle){1920/2 - 400, 50, 800, 950}, (Vector2){0,0}, 0, WHITE);
                DrawTextCustom("KNOWLEDGE GATE", 1920/2 - MeasureTextCustom("KNOWLEDGE GATE", 50)/2, 160, 50, ORANGE);
                
                // Dynamic wrapped white text with black outline so it pops perfectly!
                DrawTextWrappedCustom(currentActiveQuestion.question, 1920/2, 250, 36, 650, WHITE);
                
                for(int i = 0; i < 4; i++) {
                    if (SpriteButton((Rectangle){1920/2 - 300, 420 + (i * 90), 600, 70}, btnBlank, currentActiveQuestion.options[i])) {
                        if (('A' + i) == currentActiveQuestion.answer) {
                            sprintf(logMessage, "Correct! The gate opens."); p.questionCounter++; currentScreen = SCREEN_PLAYING;
                        } else {
                            sprintf(logMessage, "Wrong! Forced to surface. Answer: %c", currentActiveQuestion.answer);
                            p.depth = 0; p.oxygen = p.oxygenMax; currentScreen = SCREEN_SURFACE;
                        }
                    }
                }
                break;

            case SCREEN_PAUSE:
                DrawTextCustom("GAME PAUSED", 1920/2 - MeasureTextCustom("GAME PAUSED", 60)/2, 150, 60, WHITE);

                int btnW = 320;
                int btnH = 70;
                int bX = 1920/2 - btnW/2;

                if (SpriteButton((Rectangle){bX, 300, btnW, btnH}, btnBlank, "Return to Game")) currentScreen = (p.depth > 0) ? SCREEN_PLAYING : SCREEN_SURFACE;
                if (SpriteButton((Rectangle){bX, 400, btnW, btnH}, btnBlank, "Stats")) currentScreen = SCREEN_STATS;
                if (SpriteButton((Rectangle){bX, 500, btnW, btnH}, btnBlank, "Upgrade Stats")) currentScreen = SCREEN_STATS_UPGRADE;
                if (SpriteButton((Rectangle){bX, 600, btnW, btnH}, btnBlank, "Inventory")) currentScreen = SCREEN_INVENTORY;
                if (SpriteButton((Rectangle){bX, 700, btnW, btnH}, btnBlank, "Return to Title")) {
                    resetPlayer(&p);
                    currentScreen = SCREEN_MAIN_MENU;
                }
                break;

            case SCREEN_STATS:
            case SCREEN_STATS_UPGRADE:
            case SCREEN_INVENTORY:
                
                // Content Sidebar logic for the 3 sub-menus
                if (currentScreen == SCREEN_STATS) {
                    DrawTextCustom("PLAYER STATISTICS", 1920/2 - MeasureTextCustom("PLAYER STATISTICS", 60)/2, 100, 60, ORANGE);
                    
                    int pX = 1920/2 - 350; 
                    DrawTextCustom(TextFormat("Level: %d  |  XP: %d / %d", p.level, p.xp, p.xpMax), pX, 220, 36, WHITE);
                    DrawTextCustom(TextFormat("Health: %d / %d", p.health, p.healthMax), pX, 310, 36, LIME);
                    DrawTextCustom(TextFormat("Attack: %d (Lv %d)", p.attack, p.attacklevel), pX, 400, 36, ORANGE);
                    DrawTextCustom(TextFormat("Defense: %d (Lv %d)", p.defense, p.defenselevel), pX, 490, 36, SKYBLUE);
                    
                    int eX = 1920/2 + 100;
                    DrawTextCustom("- ENEMY ENCOUNTERS -", eX, 200, 36, RED);
                    char eNames[7][20] = {"Crab", "Shark", "Whale", "Kraken", "Leviathan", "Hydra", "Timingila"};
                    for (int i=0; i<7; i++) {
                        DrawTextCustom(TextFormat("%s: Defeats %d | Encounters %d", eNames[i], p.defeats[i], p.encounters[i]), eX, 260 + (i*45), 28, WHITE);
                    }
                } 
                else if (currentScreen == SCREEN_STATS_UPGRADE) {
                    DrawTextCustom("UPGRADE STATS", 1920/2 - MeasureTextCustom("UPGRADE STATS", 60)/2, 100, 60, ORANGE);
                    DrawTextCustom(TextFormat("Skill Points: %d", p.skillpoints), 1920/2 - MeasureTextCustom(TextFormat("Skill Points: %d", p.skillpoints), 40)/2, 200, 40, GOLD);
                    
                    int uLevels[3] = {p.healthlevel, p.attacklevel, p.defenselevel};
                    float aW = icon_atlas.width / 5.0f;
                    float aH = icon_atlas.height / 3.0f;
                    int cx = 1920/2 - 150; 

                    for (int i = 0; i < 3; i++) {
                        Rectangle src = { i * aW, aH * 2.0f, aW, aH }; 
                        Rectangle dest = { cx, 300 + (i * 140), 120, 120 }; 
                        bool canUp = p.skillpoints > 0;
                        
                        if (AtlasButton(icon_atlas, src, dest)) {
                            if (canUp) {
                                if(i==0) { p.healthlevel++; p.skillpoints--; }
                                else if(i==1) { p.attacklevel++; p.skillpoints--; } 
                                else { p.defenselevel++; p.skillpoints--; }
                                updateStats(&p);
                            }
                        }
                        
                        DrawTextCustom(TextFormat("Lv %d", uLevels[i]), dest.x + 8, dest.y + 8, 24, WHITE);
                        DrawTextCustom("Cost: 1 SP", dest.x + 150, dest.y + 40, 45, canUp ? GOLD : GRAY);
                    }
                }
                else if (currentScreen == SCREEN_INVENTORY) {
                    DrawTextCustom("INVENTORY", 1920/2 - MeasureTextCustom("INVENTORY", 60)/2, 100, 60, ORANGE);

                    float aW = icon_atlas.width / 5.0f;
                    float aH = icon_atlas.height / 3.0f;
                    int cx = 1920/2 - 200;

                    DrawTextureRec(icon_atlas, (Rectangle){aW*1, aH*1, aW, aH}, (Vector2){cx, 300}, WHITE); 
                    DrawTextCustom(TextFormat("Health Potions: %d", p.healthpotions), cx + 150, 340, 45, LIME);
                    
                    DrawTextureRec(icon_atlas, (Rectangle){aW*4, aH*2, aW, aH}, (Vector2){cx, 450}, WHITE); 
                    DrawTextCustom(TextFormat("Gold: %d", p.gold), cx + 150, 490, 45, GOLD);
                }

                // BOTTOM CENTERED CLEAR RETURN BUTTON FOR ALL SUB-MENUS
                if (SpriteButton((Rectangle){1920/2 - 160, 800, 320, 70}, btnBlank, "Return to Menu")) {
                    currentScreen = SCREEN_PAUSE;
                }
                break;

            case SCREEN_GAMEOVER:
                DrawTextCustom("YOU DIED", 960 - MeasureTextCustom("YOU DIED", 100)/2, 300, 100, RED);
                DrawTextCustom(TextFormat("Depth Reached: %d m", p.depth), 750, 450, 36, WHITE);
                DrawTextCustom(TextFormat("Monsters Defeated: %d", p.defeats[0]+p.defeats[1]+p.defeats[2]+p.defeats[3]+p.defeats[4]+p.defeats[5]), 750, 500, 36, WHITE);
                DrawTextCustom(TextFormat("Bosses Defeated: %d", p.timingilaDefeats), 750, 550, 36, ORANGE);
                
                if (SpriteButton((Rectangle){1920/2 - 140, 650, 280, 70}, btnBlank, "MAIN MENU")) {
                    resetPlayer(&p);
                    currentScreen = SCREEN_MAIN_MENU;
                }
                break;
        }
        EndDrawing();
    }
    
    //  FREE MEMORY 
    UnloadFont(customFont);
    UnloadTexture(bgGameplay); UnloadTexture(bgShop); 
    UnloadTexture(bgSurface); UnloadTexture(bgQuestion); UnloadTexture(btnBlank);
    UnloadTexture(btnStart); UnloadTexture(btnContinue); UnloadTexture(btnOption); 
    UnloadTexture(btnExit); UnloadTexture(btnMenu); UnloadTexture(btnClose);
    UnloadTexture(icon_atlas); UnloadTexture(texDiver); UnloadTexture(texTimingila);
    UnloadTexture(texCrab); UnloadTexture(texShark); UnloadTexture(texWhale);
    UnloadTexture(texKraken); UnloadTexture(texLeviathan); UnloadTexture(texHydra);
    
    CloseWindow();
    return 0;
}