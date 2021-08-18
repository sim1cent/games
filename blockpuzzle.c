#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 420
#define HEIGHT 450
#define BLOCKSIZE 20

#define GRID_WIDTH 10
#define GRID_HEIGHT 22

#define X(n) (n%5)
#define Y(n) (n/5)

enum ROTATION{LEFT=-1,RIGHT=1};
enum STATUS{NONE=0,ACTIVE=1,PROJECTION=2};

struct Match{
    uint score;
    uint time;
    uint dropped;
};

struct Block{
    enum STATUS status;
    Color color;
};

typedef struct{
    int blocks[4];
    int pivot;
    Color color;
}Piece;

static inline void init_stuff();
static void init_game();
static inline void game_loop();
static inline void clear_lines();
static inline bool check_line(int);
static bool put_piece(Piece,enum STATUS,int,int);
static void rotate_piece(Piece*,enum ROTATION);
static void del_blocks(Piece,int,int,int);
static inline void del_piece(Piece,int,int);
static Piece *get_from_queue();
static inline void draw_queue();
static inline void draw_hold();
static int put_projection(Piece,bool,int,int);
static inline void check_input(Piece*,int*,int*,int,Piece**);
static inline void draw_grid();
static inline void draw_text();
static inline void end_game();

static Piece I_piece = {{10,11,12,13},12,SKYBLUE};
static Piece J_piece = {{5,10,11,12},11,BLUE};
static Piece L_piece = {{7,10,11,12},11,ORANGE};
static Piece O_piece = {{6,7,11,12},0,YELLOW};
static Piece T_piece = {{6,10,11,12},11,PURPLE};
static Piece S_piece = {{6,7,10,11},11,GREEN};
static Piece Z_piece = {{5,6,11,12},11,RED};

static Piece *pieces[] = {&I_piece,&J_piece,&L_piece,&O_piece,
    &T_piece,&S_piece,&Z_piece};

static Texture2D block_texture;
static struct Match current_match;
static struct Block grid[GRID_WIDTH][GRID_HEIGHT];
static Piece *hold_piece;
static Piece *piece_queue[6];
static bool next_piece,can_swap;
static int cycle;

static inline void init_stuff(){
    InitWindow(WIDTH, HEIGHT, "Block Puzzle");
    Image image = LoadImage("assets/block.png");
    ImageResize(&image,BLOCKSIZE,BLOCKSIZE);
    block_texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTargetFPS(60);
}

static void init_game(){
    cycle=0;
    hold_piece=NULL;
    next_piece=true;
    memset(grid,0,sizeof grid);
    memset(piece_queue,0,sizeof piece_queue);
    current_match.score = current_match.time = current_match.dropped = 0;
}

static inline void game_loop(){
    int x,y,projection_y;
    Piece current_piece;
    Piece *current_piece_ptr;
    while (!WindowShouldClose()){
        if(next_piece){
            can_swap=true;
            x=(GRID_WIDTH-4)/2;
            y=0;
            projection_y=-1;
            current_piece_ptr=get_from_queue();
            current_piece=*current_piece_ptr;
            next_piece=false;
        }
        else{
            del_piece(current_piece,x,y);
            put_projection(current_piece,true,x,y);
        }
        check_input(&current_piece,&x,&y,projection_y,&current_piece_ptr);
        y+=cycle/50;
        cycle%=50;
        projection_y=put_projection(current_piece,false,x,y);
        if(!put_piece(current_piece,ACTIVE,x,y)){
            if(y==0){
                init_game();
                continue;
            }
            ++current_match.dropped;
            put_piece(current_piece,ACTIVE,x,y-1);
            clear_lines();
            next_piece=true;
        }
        
        BeginDrawing();
            ClearBackground(RAYWHITE);
            draw_grid();
            draw_text();
            draw_hold();
            draw_queue();
        EndDrawing();
        
        if(!IsWindowMinimized()){
            ++cycle;
            ++current_match.time;
        }
        if(IsKeyPressed(KEY_F4))
            init_game();
    }
}

static inline void clear_lines(){
    for(int i=GRID_HEIGHT-1;i>=0;){
        if(check_line(i)){
            ++current_match.score;
            for(int i1=i;i1>0;--i1){
                for(int i2=0;i2<GRID_WIDTH;++i2)
                    grid[i2][i1]=grid[i2][i1-1];
            }
            for(int i3=0;i3<GRID_WIDTH;++i3)
                grid[i3][0].status=NONE;
        }
        else
            --i;
    }
}

static inline bool check_line(int y){
    for(int i=0;i<GRID_WIDTH;++i){
        if(grid[i][y].status!=ACTIVE)
            return false;
    }
    return true;
}

static bool put_piece(Piece piece, enum STATUS status, int x, int y){
    for(int i=0;i<4;++i){
        int block_x=x+X(piece.blocks[i]);
        int block_y=y+Y(piece.blocks[i]);
        if(grid[block_x][block_y].status==ACTIVE || block_x<0 || block_x>=GRID_WIDTH || block_y>=GRID_HEIGHT){
            del_blocks(piece,x,y,i);
            return false;
        }
        else{
            grid[block_x][block_y].status=status;
            grid[block_x][block_y].color=piece.color;
        }
    }
    return true;
}

static void rotate_piece(Piece *piece, enum ROTATION rotation){
    for(int i=0;i<4;++i){
        int new_x=X(piece->pivot)+((Y(piece->blocks[i])-Y(piece->pivot))*-1*rotation);
        int new_y=Y(piece->pivot)+((X(piece->blocks[i])-X(piece->pivot))*rotation);
        piece->blocks[i]=new_y*5+new_x;
    }
}

static void del_blocks(Piece piece, int x, int y, int n){
    for(int i=0;i<n;++i)
        grid[x+X(piece.blocks[i])][y+Y(piece.blocks[i])].status = NONE;
}

static inline void del_piece(Piece piece, int x, int y){
    del_blocks(piece,x,y,4);
}

static Piece *get_from_queue(){
    for(int i=sizeof(piece_queue)/sizeof(piece_queue[0])-1;i>=0&&piece_queue[i]==NULL;--i)
        piece_queue[i]=pieces[GetRandomValue(0,sizeof(pieces)/sizeof(pieces[0])-1)];
    Piece *next_piece=piece_queue[0];
    for(int i=0;i<sizeof(piece_queue)/sizeof(piece_queue[0])-1;++i)
        piece_queue[i]=piece_queue[i+1];
    piece_queue[sizeof(piece_queue)/sizeof(piece_queue[0])-1]=NULL;
    return next_piece;
}

static inline void draw_queue(){
    for(int i1=0;i1<sizeof(piece_queue)/sizeof(piece_queue[0])-1;++i1){
        for(int i2=0;i2<4;++i2){
            DrawTexture(block_texture,(WIDTH/2+GRID_WIDTH*BLOCKSIZE/2)+10+X(piece_queue[i1]->blocks[i2])*BLOCKSIZE,
                (HEIGHT-(GRID_HEIGHT-2)*BLOCKSIZE)/2+i1*4*BLOCKSIZE+Y(piece_queue[i1]->blocks[i2])*BLOCKSIZE, piece_queue[i1]->color);
        }
    }
}

static inline void draw_hold(){
    if(hold_piece!=NULL){
        for(int i=0;i<4;++i){
            DrawTexture(block_texture,(WIDTH/2-(GRID_WIDTH*BLOCKSIZE)/2)-10-4*BLOCKSIZE+X(hold_piece->blocks[i])*BLOCKSIZE,
                (HEIGHT-(GRID_HEIGHT-2)*BLOCKSIZE)/2+Y(hold_piece->blocks[i])*BLOCKSIZE,
                can_swap?hold_piece->color:LIGHTGRAY);
        }
    }
}

static int put_projection(Piece piece, bool del_projection, int x, int y){
    piece.color=Fade(piece.color,0.5);
    while(put_piece(piece,PROJECTION,x,y))
        del_piece(piece,x,y++);
    if(!del_projection)
        put_piece(piece,PROJECTION,x,--y);
    return y;
}

static inline void check_input(Piece *piece, int *x, int *y, int projection_y, Piece **current_piece_ptr){
    if(IsKeyPressed(KEY_SPACE)){
        *y=projection_y;
        cycle=50;
    }
    else if(IsKeyPressed(KEY_RIGHT)){
        if(put_piece(*piece,ACTIVE,++(*x),*y))
            del_piece(*piece,*x,*y);
        else
            --(*x);
    }
    else if(IsKeyPressed(KEY_LEFT)){
        if(put_piece(*piece,ACTIVE,--(*x),*y))
            del_piece(*piece,*x,*y);
        else
            ++(*x);
    }
    else if(IsKeyPressed(KEY_UP)){
        if(*current_piece_ptr!=&O_piece){
            rotate_piece(piece,RIGHT);
            if(!put_piece(*piece,ACTIVE,*x,*y))
                rotate_piece(piece,LEFT);
            else
                del_piece(*piece,*x,*y);
        }
    }
    else if(IsKeyPressed(KEY_RIGHT_SHIFT)){
        if(*current_piece_ptr!=&O_piece){
            rotate_piece(piece,LEFT);
            if(!put_piece(*piece,ACTIVE,*x,*y))
                rotate_piece(piece,RIGHT);
            else
                del_piece(*piece,*x,*y);
        }
    }
    else if(IsKeyPressed(KEY_C)){
        if(can_swap){
            if(hold_piece!=NULL){
                Piece *temp=hold_piece;
                hold_piece=*current_piece_ptr;
                *current_piece_ptr=temp;
            }
            else{
                hold_piece=*current_piece_ptr;
                *current_piece_ptr=get_from_queue();
            }
            *piece=**current_piece_ptr;
            *x=(GRID_WIDTH-4)/2;
            *y=0;
            can_swap=false;
            cycle=0;
        }
    }
    if(IsKeyDown(KEY_DOWN)){
        if(put_piece(*piece,ACTIVE,*x,++(*y))){
            cycle=0;
            del_piece(*piece,*x,*y);
        }
        else
            --(*y);
    }
}

static inline void draw_grid(){
    for(int i1=0;i1<GRID_WIDTH;++i1){
        for(int i2=2;i2<GRID_HEIGHT;++i2){
            if(grid[i1][i2].status!=NONE)
                DrawTexture(block_texture,(WIDTH/2-GRID_WIDTH*BLOCKSIZE/2)+i1*BLOCKSIZE,
                            (HEIGHT/2-(GRID_HEIGHT-2)*BLOCKSIZE/2)+(i2-2)*BLOCKSIZE, grid[i1][i2].color);
        }            
    }
    DrawRectangleLines(WIDTH/2-GRID_WIDTH*BLOCKSIZE/2,HEIGHT/2-(GRID_HEIGHT-2)*BLOCKSIZE/2,
                  GRID_WIDTH*BLOCKSIZE,(GRID_HEIGHT-2)*BLOCKSIZE,BLACK); // draw grid outline
}

static inline void draw_text(){
    char score[6];
    sprintf(score,"%u",current_match.score%100000);
    char time[6];
    sprintf(time,"%u",(current_match.time/60)%100000);
    char dropped[6];
    sprintf(dropped,"%u",current_match.dropped%100000);
    DrawText("Score:",10,120,20,RED);
    DrawText(score,10,140,20,BLACK);
    DrawText("Time:",10,165,20,RED);
    DrawText(time,10,185,20,BLACK);
    DrawText("Dropped:",10,210,20,RED);
    DrawText(dropped,10,230,20,BLACK);
}

static inline void end_game(){
    UnloadTexture(block_texture);
    CloseWindow();
}

int main(){
    init_stuff();
    init_game();
    game_loop();
    end_game();
    return 0;
}
