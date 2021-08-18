#include <raylib.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#define CELL_SIZE 20

typedef struct{
    bool is_uncovered   :1;
    bool is_mine        :1;
    bool is_flagged     :1;
}Cell;

static inline void init_stuff();
static inline void init_game();
static inline void game_loop();
static inline uint get_num(int,int);
static void place_mines(int,int);
static void uncover_cells(int,int);
static inline void draw_grid();
static inline void end_game();

static Cell *grid;
static Texture2D mine_texture,flag_texture;
static uint grid_width,grid_height,mines_n,flagged_n,remaining_cells;
static bool first_move,game_over;

static inline void init_stuff(){
    InitWindow(grid_width*CELL_SIZE, grid_height*CELL_SIZE, "Minesweeper");
    Image flag_image, mine_image;
    mine_image = LoadImage("assets/mine.png");          flag_image = LoadImage("assets/flag.png");
    ImageResize(&mine_image,CELL_SIZE,CELL_SIZE);       ImageResize(&flag_image,CELL_SIZE,CELL_SIZE);
    mine_texture = LoadTextureFromImage(mine_image);    flag_texture = LoadTextureFromImage(flag_image);
    UnloadImage(mine_image);                            UnloadImage(flag_image);
}

static inline void init_game(){
    memset(grid,0,(grid_width*grid_height)*sizeof(Cell));
    remaining_cells=grid_width*grid_height-mines_n;
    flagged_n=0;
    first_move=true;
    game_over=false;
    puts("--NEW GAME--");
}

#define grid(x,y) (grid[(y)*grid_width+(x)])
static inline void game_loop(){
    while (!WindowShouldClose()){

        BeginDrawing();
            ClearBackground(LIGHTGRAY);
            draw_grid();
        EndDrawing();

        if(!(game_over||remaining_cells==0)){
            if(IsMouseButtonReleased(0)){
                uint cell_x=GetMouseX()/CELL_SIZE;
                uint cell_y=GetMouseY()/CELL_SIZE;
                if(cell_x<grid_width&&cell_y<grid_height)
                    uncover_cells(cell_x,cell_y);
            }
            else if(IsMouseButtonPressed(1)){
                uint cell_x=GetMouseX()/CELL_SIZE;
                uint cell_y=GetMouseY()/CELL_SIZE;
                if(grid(cell_x,cell_y).is_flagged||grid(cell_x,cell_y).is_uncovered){
                    grid(cell_x,cell_y).is_flagged=false;
                    if(!grid(cell_x,cell_y).is_uncovered) --flagged_n;
                }
                else{
                    grid(cell_x,cell_y).is_flagged=true;
                    ++flagged_n;
                }
                printf("Mines to flag: %d\n",mines_n-flagged_n);
            }
        }
        if(IsKeyPressed(KEY_F4))
            init_game();
    }
}

static inline uint get_num(int x, int y){
    uint num=0;
    for(int i1=x-1; i1<=x+1; ++i1){
        for(int i2=y-1; i2<=y+1; ++i2){
            if(!(i1<0||i2<0||i1>=grid_width||i2>=grid_height)&&grid(i1,i2).is_mine)
                ++num;
        }
    }
    return num;
}

static void place_mines(int x, int y){
    uint *mines_shuffle=alloca((grid_width*grid_height-1)*sizeof(uint));
    int i1,i2;
    for(i2=i1=0;i1<grid_width*grid_height;++i1){
        if(!(abs((i1%grid_width)-x)<=1&&abs((i1/grid_width)-y)<=1)||(mines_n>=grid_width*grid_height-8&&i1!=y*grid_width+x))
            mines_shuffle[i2++]=i1;
    }
    for(--i2;i2>0;--i2){
        uint j=GetRandomValue(0,i2);
        uint temp=mines_shuffle[j];
        mines_shuffle[j] = mines_shuffle[i2];
        mines_shuffle[i2] = temp;
    }
    for(uint i=0;i<mines_n;++i)
        grid[mines_shuffle[i]].is_mine=true;
}

static void uncover_cells(int x, int y){
    if(grid(x,y).is_uncovered||grid(x,y).is_flagged)
        return;
    else if(grid(x,y).is_mine){
        game_over=true;
        return;
    }
    if(first_move){
        place_mines(x,y);
        first_move=false;
    }
    grid(x,y).is_uncovered=true;
    remaining_cells--;
    if(!get_num(x,y)){
        for(int i1=-1;i1<=1;++i1){
            for(int i2=-1;i2<=1;++i2){
                if(!(x+i1<0||y+i2<0||x+i1>=grid_width||y+i2>=grid_height))
                    uncover_cells(x+i1,y+i2);
            }
        }
    }
}

static inline void draw_grid(){
    for(uint i1=0; i1<grid_width; ++i1){
        for(uint i2=0; i2<grid_height; ++i2){
            if(grid(i1,i2).is_uncovered||game_over){
                if(grid(i1,i2).is_mine)
                    DrawTexture(mine_texture,i1*CELL_SIZE, i2*CELL_SIZE, WHITE);
                else{
                    uint n = get_num(i1,i2);
                    if (n>0){
                        const Color num_colors[8] = {BLUE,LIME,RED,DARKBLUE,MAROON,VIOLET,BLACK,DARKGRAY};
                        char num[2];
                        sprintf(num,"%u",n%9);
                        DrawText(num, i1*CELL_SIZE+CELL_SIZE/(n==1?3:4), i2*CELL_SIZE+CELL_SIZE/20, CELL_SIZE, num_colors[n-1]);
                    }
                }
            }
            else{
                DrawRectangle(i1*CELL_SIZE, i2*CELL_SIZE, CELL_SIZE, CELL_SIZE, GRAY);
                if(grid(i1,i2).is_flagged||remaining_cells==0)
                    DrawTexture(flag_texture,i1*CELL_SIZE, i2*CELL_SIZE, (remaining_cells==0?LIGHTGRAY:WHITE));
            }
            DrawRectangleLines(i1*CELL_SIZE, i2*CELL_SIZE, CELL_SIZE, CELL_SIZE, DARKGRAY);
        }
    }
}

static inline void end_game(){
    UnloadTexture(mine_texture);
    UnloadTexture(flag_texture);
    CloseWindow();
}

#define get_arg(a, b) (argc>=(a)?atoi(argv[(a)-1]):(b))
#define assert(x) if(!(x)){ fputs("Invalid parameters\n",stderr); exit(1); }
int main(int argc, char **argv){
    grid_width=get_arg(2,9);
    grid_height=get_arg(3,9);
    mines_n=get_arg(4,10);
    assert(grid_width<=30&&grid_height<=30&&mines_n<(grid_width*grid_height));
    assert(grid_width>=4&&grid_height>=4&&mines_n>0);
    init_stuff();
    grid=alloca((grid_width*grid_height)*sizeof(Cell));
    init_game();
    game_loop();
    end_game();
    return 0;
}
