/* 
 * Jack O'Connor
 * Fund Comp Lab 11
 * chessfunc.h
*/
#include <stdbool.h>

#define BOARD_DIM (8)

// These macros are used for retrieving data from a Piece integer.
#define PIECE_BITMASK (7)    // First 3 bits represent piece value.
#define COLOR_BITMASK (24)   // 4th and 5th bit represent color.
#define MOVED_BITMASK (32)   // 6th bit tells if the piece has moved.


// All of the pieces's data can be stored in a single byte, including it's
// numerical value, it's color, and whether or not it has moved.
typedef char Piece;

// This enum represents the information stored in a Piece.
// For example, a white queen is could be decared as:
//      `Piece a = WHITE | QUEEN;`
typedef enum {
    EMPTY,
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING,
    WHITE = 8,
    BLACK = 16,
    MOVED = 32, 
} PieceType;

// Used for storing position vectors of pieces.
typedef struct {
    short int x, y;
} V2Int;


typedef char Pos;

// Taking advantage of the fact that a long int has 64 bits,
// each bit can represent a boolean value for a square on the board.
// https://www.chessprogramming.org/Efficient_Generation_of_Sliding_Piece_Attacks
typedef unsigned long int Bitboard;


// Used for highlighting specific squares on the board.
typedef enum {
    SELECTED,
    AVAILIBLE,
    PREVIOUS,
} Highlight;


// Data structure for a board.
typedef struct {
    Piece *arr;
    short int turn;
    short int winner;
    Pos ep_target_pos;
    int half_move_clock;
    int move_count;
    Bitboard *highlights;
} Board;


void create_board(Board *board, char *fen);
void display_board(Board *board);
void process_FEN(Board *board, char *fen);
int get_valid_moves(V2Int p_pos, Bitboard *moves, Board *board, bool check_for_check);
Piece* get_piece(V2Int pos, Board *board);
bool verify_move(V2Int pos, V2Int new_pos, Board *board, bool check_for_check);
short int in_check(Board *board);
Bitboard attacked_positions(PieceType type, Board *b, bool check_for_check);
void copy_board(Board *dest, Board *board);
void free_board(Board *board);
void make_move(V2Int pos, V2Int target, Board *board);
V2Int add_V2Int(V2Int a, V2Int b);
V2Int sub_V2Int(V2Int a, V2Int b);
int cmp_V2Int(V2Int a, V2Int b);
Pos convert_coord(char *coord);
void set_highlight(Pos pos, Highlight type, Board *board);
void reset_highlights(Highlight type, Board *board);
void fill_rectangle(int x1, int y1, int wid, int hei);
void draw_board(int x, int y, int sq_len, Board *board);
void update_bitboard(Bitboard *b, Pos p);
bool query_bitboard(Bitboard *b, Pos p);
void print_bitboard(Bitboard *b);
void end_game(int x, int y, short int winner);
int total_moves(Board *board, int ply);