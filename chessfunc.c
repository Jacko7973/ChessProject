/* 
 * Jack O'Connor
 * Fund Comp Lab 11
 * chessfunc.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "gfx.h"

#include "chessfunc.h"


const char *PIECE_STR = " pnbrqk";


void create_board(Board *board, char *fen)
{
    // Creates a board from the specified fen string.

    board->arr = (Piece*) calloc(BOARD_DIM * BOARD_DIM, sizeof(Piece));
    board->highlights = (Bitboard*) calloc(3, sizeof(Bitboard));
    process_FEN(board, fen);
    board->winner = 0;
}


void display_board(Board *board)
{
    // Print a formatted representation of the board into the console.


    Piece *arr = board->arr;
    Piece piece;
    char piece_char;
    for (int i = 0; i < BOARD_DIM; i++) {
        for (int j = 0; j < BOARD_DIM; j++)
            printf(" -----");
        printf("\n");

        for (int j = 0; j < 3; j++) {
            for (int k = 0; k < BOARD_DIM; k++) {
                if (j == 1) {
                    piece = *(arr++);
                    // The piece's character is accessed in the PIECE_STR varriable
                    // at the index of the piece's value.
                    piece_char = PIECE_STR[piece & PIECE_BITMASK];
                    // White pieces are represented as uppercase, black as lowercase.
                    if ((piece & COLOR_BITMASK) == WHITE)
                        piece_char = toupper(piece_char);
                    printf("|  %c  ", piece_char);
                } else
                    printf("|     ");

            }
            printf("|\n");
        }
    }

    for (int j = 0; j < BOARD_DIM; j++)
        printf(" -----");
    printf("\n");
}


void process_FEN(Board *board, char *fen)
{
    // FEN notation is used to store the state of the board in a single string.
    // https://en.wikipedia.org/wiki/Forsyth%E2%80%93Edwards_Notation
    // This function assumes the FEN string is properly formatted.

    static const char *CASTLE_OPTIONS = "qkQK";

    char *tmp_fen = strdup(fen);
    char placement_str[70], turn[3], castle_str[6], enpassant_str[4];
    int half_move, full_move;

    strcpy(placement_str, strtok(tmp_fen, " "));
    strcpy(turn, strtok(NULL, " "));
    strcpy(castle_str, strtok(NULL, " "));
    strcpy(enpassant_str, strtok(NULL, " "));
    half_move = atoi(strtok(NULL, " "));
    full_move = atoi(strtok(NULL, " "));

    // Pre-compute the rooks to match castling options in FEN string.
    Piece* rooks = (Piece*) malloc(4 * sizeof(Piece));
    for (int i = 0; i < 4; i++) {
        rooks[i] = ROOK;
        if (!strchr(castle_str, CASTLE_OPTIONS[i]))
            rooks[i] |= MOVED;
    }
    int rook_i = 0;


    Piece *arr = board->arr;
    int file = 0, rank = 0;
    char *placement_ptr = placement_str;
    char curr = *(placement_ptr++);

    while (curr != '\0') {
        if (curr == '/') {
            // Go to the next rank if the character is a '/'.
            file = 0;
            rank++;
        } else if (isdigit(curr)) {
            // Skip `n` spaces if the current character is a number n.
            for (int j = 0; j < (curr - '0'); j++) {
                *(arr + rank * BOARD_DIM + file) = 0;
                file++;
            }
        } else {
            // Otherwise, the current character represents a piece.
            int col = isupper(curr) ? WHITE : BLACK;
            // It's color is represented by the case.
            if (col == WHITE)
                curr = tolower(curr);

            int piece = 0;
            // It's value can be retrieved by it's index in the PIECE_STR string.
            for (int j = 1; j < strlen(PIECE_STR); j++) {
                if (PIECE_STR[j] == curr) {
                    piece = j;
                    break;
                }
            }

            // Use the pre-computed rooks
            if (piece == ROOK && rook_i < 4) 
                piece = *(rooks + rook_i++);

            // Set the current piece to the proper value and color using the bitwise or operator.
            *(arr + rank * BOARD_DIM + file) = piece | col;
            file++;
        }

        // Increment the fen character pointer.
        curr = *(placement_ptr++);
    }

    // Set the current player's turn.
    board->turn = (turn[0] == 'w') ? WHITE : BLACK;

    // TODO
    // Set the castle availibility.

    if (strcmp(enpassant_str, "-") == 0)
        board->ep_target_pos = 64;
    else
        board->ep_target_pos = convert_coord(enpassant_str); 

    board->half_move_clock = half_move;
    board->move_count = full_move;
}


int get_valid_moves(V2Int p_pos, Bitboard *moves, Board *board, bool check_for_check)
{
    // Populates the `moves` pointer with valid positions that the piece at 'p_pos'
    // could move to, and returns the number of moves found.

    // Positions the knight can move to relative to itself.
    static const V2Int KNIGHT_OFFSETS[] = {
        {1, 2}, {2, 1}, {-1, 2}, {-2, 1}, {-1, -2}, {-2, -1}, {1, -2}, {2, -1},
    };
    // Positions of all eight neighbor cells relative to current position.
    static const V2Int NEIGHBOR_OFFSETS[] = {
        {1, 1}, {1, 0}, {1, -1}, {0, -1}, {-1, -1}, {-1, 0}, {-1, 1}, {0, 1},
    };

    int i = 0;

    Piece *p_ptr = get_piece(p_pos, board);
    int p_type = *p_ptr & PIECE_BITMASK;
    int p_col = *p_ptr & COLOR_BITMASK;
    int p_moved = *p_ptr & MOVED_BITMASK; // Used for castling.
    int dir = p_col == WHITE ? -1 : 1; // Pawns can only move forward.

    V2Int offset;
    V2Int new_pos;
    switch (p_type) {
        case EMPTY:
            // An empty cell has no valid moves.
            break;
        case PAWN:
            offset.x = 0;
            offset.y = dir;
            new_pos = add_V2Int(p_pos, offset); // One space movement.
            if (*get_piece(new_pos, board) == 0) {
                // Each potential move is passed to `verify_move` function to make
                // sure it is valid.
                if (verify_move(p_pos, new_pos, board, check_for_check)) {
                    update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                }

                if (!p_moved) { // Two space opening move.
                    offset.y = 2 * dir;
                    new_pos = add_V2Int(p_pos, offset);
                    if (verify_move(p_pos, new_pos, board, check_for_check) && *get_piece(new_pos, board) == 0) {
                        update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                        i++;
                    }
                }

            }

            // Attacking diagonal spaces.
            offset.x = -1;
            offset.y = dir;
            new_pos = add_V2Int(p_pos, offset);
            // These are only valid if there is an opposing piece in one of the diagonal squares.
            if (verify_move(p_pos, new_pos, board, check_for_check))
                if (*get_piece(new_pos, board) != 0 || new_pos.x + BOARD_DIM * new_pos.y == board->ep_target_pos) {
                    update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                }
            offset.x = 1;
            new_pos = add_V2Int(p_pos, offset);
            if (verify_move(p_pos, new_pos, board, check_for_check))
                if (*get_piece(new_pos, board) != 0 || new_pos.x + BOARD_DIM * new_pos.y == board->ep_target_pos) {
                    update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                }

            break;
        case KNIGHT:
            for (int j = 0; j < 8; j++) {
                // Loops through the KNIGHT_OFFSETS array, adding valid positions.
                offset = KNIGHT_OFFSETS[j];
                new_pos = add_V2Int(p_pos, offset);
                if (verify_move(p_pos, new_pos, board, check_for_check)) {
                    update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                }
            }
            break;
        case BISHOP:
            for (int j = 0; j < 8; j += 2) {
                // Moves on the diagonals, so offsets are located at even indexes in the
                // NEIGHBOR_OFFSETS array.
                new_pos = p_pos;
                offset = NEIGHBOR_OFFSETS[j];
                while (1) {
                    // Continue to add moves as long as the last move was valid.
                    new_pos = add_V2Int(new_pos, offset);
                    if (!verify_move(p_pos, new_pos, board, 0)) break;
                    if (verify_move(p_pos, new_pos, board, check_for_check))
                        update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                    if (*get_piece(new_pos, board) != 0) break;
                }
            }
            break;
        case ROOK:
            for (int j = 1; j < 8; j += 2) {
                // Same story as bishop, only the rook uses odd indexes in the NEIGHBOR_OFFSETS
                // array.
                new_pos = p_pos;
                offset = NEIGHBOR_OFFSETS[j];
                while (1) {
                    new_pos = add_V2Int(new_pos, offset);
                    if (!verify_move(p_pos, new_pos, board, 0)) break;
                    if (verify_move(p_pos, new_pos, board, check_for_check))
                        update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                    if (*get_piece(new_pos, board) != 0) break;
                }
            }
            break;
        case QUEEN:
            for (int j = 0; j < 8; j++) {
                // Same as bishop and rook, but queen can attack all directions.
                new_pos = p_pos;
                offset = NEIGHBOR_OFFSETS[j];
                while (1) {
                    new_pos = add_V2Int(new_pos, offset);
                    if (!verify_move(p_pos, new_pos, board, 0)) break;
                    if (verify_move(p_pos, new_pos, board, check_for_check))
                        update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                    if (*get_piece(new_pos, board) != 0) break;
                }
            }
            break;
        case KING:
            for (int j = 0; j < 8; j++) {
                // Can move to all offsets in NEIGHBOR_OFFSETS array.
                new_pos = add_V2Int(p_pos, NEIGHBOR_OFFSETS[j]);
                if (verify_move(p_pos, new_pos, board, check_for_check)) {
                    update_bitboard(moves, new_pos.x + BOARD_DIM * new_pos.y);
                    i++;
                }
            }

            if (p_moved || !check_for_check) break;
            // Check for castling availibility.
            Bitboard attacked = attacked_positions((p_col == WHITE) ? BLACK : WHITE, board, false);
            int king_file = (p_col == WHITE) ? 7 : 0;

            // First check queen-side castle.
            Pos rook_pos = 0 + BOARD_DIM * king_file;
            Piece rook = *(board->arr + rook_pos);
            bool availible = true;
            // Rook must be unmoved for castling to be an option.
            if ((rook & COLOR_BITMASK) == p_col && !(rook & MOVED)) {
                for (int i = 1; i < 4; i++) {
                    // All spaces between king and rook must be empty.
                    if (*(board->arr + rook_pos + i) != 0) {
                        availible = false;
                        break;
                    }
                    // None of these spaces can be controlled by enemy pieces either.
                    if (query_bitboard(&attacked, rook_pos + i)) {
                        availible = false;
                        break;
                    }
                }
                if (availible) {
                    update_bitboard(moves, rook_pos + 1);
                    i++;
                }
            }

            // Then check king-side.
            rook_pos = 7 + BOARD_DIM * king_file;
            rook = *(board->arr + rook_pos);
            availible = true;
            if ((rook & COLOR_BITMASK) == p_col && !(rook & MOVED)) {
                for (int i = 1; i < 3; i++) {
                    if (*(board->arr + rook_pos - i) != 0) {
                        availible = false;
                        break;
                    }
                    if (query_bitboard(&attacked, rook_pos - i)) {
                        availible = false;
                        break;
                    }
                }
                if (availible) {
                    update_bitboard(moves, rook_pos - 1);
                    i++;
                }
            }

            break;
    }

    return i;
}


Piece* get_piece(V2Int pos, Board *board)
{
    // Returns a pointer to the piece at the specified position.

    if (pos.x < 0 || pos.x >= BOARD_DIM)
        return NULL;
    if (pos.y < 0 || pos.y >= BOARD_DIM)
        return NULL;

    return (board->arr) + pos.x + (pos.y * BOARD_DIM);
}


bool verify_move(V2Int pos, V2Int new_pos, Board *board, bool check_for_check)
{
    // Check to see if moving the piece at `pos` to `new_pos` is valid.

    Piece *arr = board->arr;
    Piece *p = get_piece(pos, board);
    Piece *target = get_piece(new_pos, board);
    if (p == NULL || target == NULL) return 0;

    int p_col = *p & COLOR_BITMASK;
    int target_col = *target & COLOR_BITMASK;
    if (p_col == target_col) return 0;

    if (!check_for_check) return 1;
    // If check_for_check is set, the code below checks if this move
    // would put the current player in check. If so, the move is invalid.
    Board *new_board = (Board*) malloc(sizeof(Board));
    copy_board(new_board, board);
    make_move(pos, new_pos, new_board);

    if (in_check(new_board) & p_col) {
        free_board(new_board);
        return 0;
    }

    free_board(new_board);

    return 1;
}


short int in_check(Board *board)
{
    // Returns an integer telling if each color is in check or not.
    // Returns 0 if neither player in in check.

    Piece *curr = board->arr;
    // Locate the two kings on the board.
    Pos king_w, king_b;
    for (int i = 0; i < BOARD_DIM * BOARD_DIM; i++) {
        if ((*curr & PIECE_BITMASK) == KING) { 
            if ((*curr & COLOR_BITMASK) == WHITE)
                king_w = i;
            else
                king_b = i;
        }
        curr++;
    }

    short int in_check = 0;
    Bitboard attacked = attacked_positions(BLACK, board, false);
    if (query_bitboard(&attacked, king_w))
        in_check |= WHITE;
    
    attacked = attacked_positions(WHITE, board, false);
    if (query_bitboard(&attacked, king_b))
        in_check |= BLACK;

    return in_check;
}


Bitboard attacked_positions(PieceType bitmask, Board *board, bool check_for_check)
{
    // Retuens a bitboard indicating which positions are attacked
    // by the specified piece type.

    Bitboard attacked = 0;
    Piece *curr = board->arr;
    for (int i = 0; i < BOARD_DIM * BOARD_DIM; i++, curr++) {
        if ((*curr & bitmask) == 0)
            continue;

        V2Int tmp_pos = {i % BOARD_DIM, i / BOARD_DIM};
        Bitboard current_attack = 0;
        
        get_valid_moves(tmp_pos, &current_attack, board, check_for_check);
        attacked |= current_attack;
    }

    return attacked;
    
}


void copy_board(Board *dest, Board *board)
{
    // Make a copy of the board and store it in `new_board`.

    dest->arr = (Piece*) malloc(BOARD_DIM * BOARD_DIM * sizeof(Piece));
    memcpy(dest->arr, board->arr, BOARD_DIM * BOARD_DIM * sizeof(Piece));
    dest->turn = board->turn;
    dest->winner = board->winner;
    dest->ep_target_pos = board->ep_target_pos;
    dest->half_move_clock = board->half_move_clock;
    dest->move_count = board->move_count;
    // Highlights aren't used for copied boards.
    dest->highlights = NULL;
}


void free_board(Board *board)
{
    // Frees dynamic memory allocated to a board.

    free(board->arr);
    free(board->highlights);
}


void make_move(V2Int pos, V2Int target, Board *board)
{
    // Moves the piece at `pos` to `target`.

    Piece *piece = get_piece(pos, board);
    Piece *target_piece = get_piece(target, board);
    
    short int p_type = *piece & PIECE_BITMASK;
    if (*target_piece != 0 || p_type == PAWN) 
        board->half_move_clock = 0;

    // Check for en passant.
    if (p_type == PAWN && (target.x + BOARD_DIM * target.y) == board->ep_target_pos) {
        V2Int dir = {0, (*piece & COLOR_BITMASK) == WHITE ? 1 : -1};
        *get_piece(add_V2Int(target, dir), board) = 0;
    }

    if (p_type == KING && abs(sub_V2Int(pos, target).x) >= 2) {
        // If the user is castling, swap the two pieces on either side of the king;
        Pos target_pos = target.x + BOARD_DIM * target.y;
        Piece tmp = *(board->arr + target_pos + 1);
        *(board->arr + target_pos + 1) = *(board->arr + target_pos - 1);
        *(board->arr + target_pos - 1) = tmp;

        if (*(board->arr + target_pos + 1) != 0)
            *(board->arr + target_pos + 1) |= MOVED;
        if (*(board->arr + target_pos - 1) != 0)
            *(board->arr + target_pos - 1) |= MOVED;
    }


    if (p_type == PAWN && abs(sub_V2Int(pos, target).y) == 2)
        board->ep_target_pos = pos.x + BOARD_DIM * ((*piece & COLOR_BITMASK) == WHITE ? 5 : 2);
    else
        board->ep_target_pos = 64;

    // Also mark the moving piece as moved.
    *target_piece = (*piece) | MOVED;
    *piece = 0;
    
    if (board->turn == BLACK) (board->move_count)++;
    board->half_move_clock++;
    board->turn = (board->turn == WHITE) ? BLACK : WHITE;

}


V2Int add_V2Int(V2Int a, V2Int b)
{
    // Add two V2Int structs.

    return (V2Int) {a.x + b.x, a.y + b.y};
}


V2Int sub_V2Int(V2Int a, V2Int b)
{
    // Subtract vector b from a.

    return (V2Int) {a.x - b.x, a.y - b.y};
}


int cmp_V2Int(V2Int a, V2Int b)
{
    // Returns 1 if vectors are equal, 0 otherwise/

    return (a.x == b.x && a.y == b.y);
}


Pos convert_coord(char *coord)
{
    // Convert a standard coordinate string to a vector.
    
    Pos pos = 0;
    pos += coord[0] - 'a';
    pos += BOARD_DIM * (BOARD_DIM - (coord[1] - '0'));

    return pos;
}


void set_highlight(Pos pos, Highlight type, Board *board)
{
    // Set a highlight at the position `pos` with type 'type'.

    *(board->highlights + type) |= (Bitboard) 1 << pos;
}


void reset_highlights(Highlight type, Board *board)
{
    // Remove all highlights of type `type` using bitwise and.
    
    *(board->highlights + type) = (Bitboard) 0;
}


void fill_rectangle(int x1, int y1, int wid, int hei)
{
    // Draw a filled rectangle with top-left corner at (x1, y1)
    // and width `wid` and height `hei`.

    for (int y = 0; y < hei; y++) {
        for (int x = 0; x < wid; x++) {
            gfx_point(x1 + x, y1 + y);
        }
    }
}


void draw_board(int x, int y, int sq_len, Board *board)
{
    // Draws the board on the graphics window with top-left corner at (x, y),
    // ranks and files are `sq_len` pixels long.
    // Highlights squares using the `highlights` pointer.



    Piece *piece = board->arr;
    Bitboard selected = *(board->highlights + SELECTED);
    Bitboard availible = *(board->highlights + AVAILIBLE);
    Bitboard previous = *(board->highlights + PREVIOUS);
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            // Check to see if current square is highlighted.
            if (selected & 1)
                gfx_color(97, 176, 77);
            else if (availible & 1)
                gfx_color(84, 147, 186);
            else if (previous & 1)
                gfx_color(245, 129, 66);
            else {
            // Otherwise, color light or dark based on its file and rank.
                if ((i + j) % 2 == 0)
                    gfx_color(230, 201, 133);
                else
                    gfx_color(77, 60, 31);
            }
            fill_rectangle(x + j * sq_len, y + i * sq_len, sq_len, sq_len);

            // Draw a letter representing the piece if there is one at this position.
            if (*piece != 0) {
                if (((*piece) & COLOR_BITMASK) == WHITE) gfx_color(255, 255, 255);
                else gfx_color(0, 0, 0);
                char text[] = "\0\0";
                text[0] = toupper(PIECE_STR[(*piece) & PIECE_BITMASK]);
                gfx_text(x + (j + 0.5) * sq_len - 1, y + (i + 0.5) * sq_len + 2, text);
            }
            piece++;
            selected = selected >> 1;
            availible = availible >> 1;
            previous = previous >> 1;
        }
    }

    if (board->winner) 
        end_game(x + 4 * sq_len, y + 4 * sq_len, board->winner);
}


void update_bitboard(Bitboard *b, Pos p)
{
    // Sets the bit at position `p` to 1.

    *b |= (Bitboard)(1) << p;
}


bool query_bitboard(Bitboard *b, Pos p)
{
    // Returns the state of the bit at positions 'p'.

    return ((*b) >> p) & 1;
}


void print_bitboard(Bitboard *b)
{
    // Prints the bitboard out for debugging purposes.

    Bitboard tmp = *b;
    for (int i = 0; i < BOARD_DIM * BOARD_DIM; i++) {
        if (i != 0 && i % BOARD_DIM == 0)
            printf("\n");
        
        printf("%c ", (tmp & 1) == 0 ? '.' : '1');
        tmp = tmp >> 1;
    }
    printf("\n");
}


void end_game(int x, int y, short int winner)
{
    // Show a visual message displaying the winner of the game.

    gfx_color(255, 255, 255);
    fill_rectangle(x - 60, y - 30, 120, 60);
    gfx_color(0, 0, 0);
    char msg[50];
    if (winner == WHITE || winner == BLACK)
        sprintf(msg, "%s wins!", (winner == WHITE) ? "White" : "Black");
    else
        strcpy(msg, "Stalemate!");

    gfx_text(x - 3 * strlen(msg), y + 3, msg);
}



int total_moves(Board *board, int ply)
{
    // Returns the total number of availibe moves after 'ply' half-moves.
    // Used for testing purposes.

    if (ply == 0)
        return 0;

    int total = 0;

    for (int i = 0; i < BOARD_DIM * BOARD_DIM; i++) {
        Piece *curr = board->arr + i;
        if ((*curr & COLOR_BITMASK) != board->turn)
            continue;
        Bitboard moves = (Bitboard) 0;
        V2Int tmp1 = {i % BOARD_DIM, i / BOARD_DIM};
        total += get_valid_moves(tmp1, &moves, board, true);
        for (int j = 0; j < BOARD_DIM * BOARD_DIM; j++, moves >>= 1) {
            if (!(moves & 1))
                continue;
            V2Int tmp2 = {j % BOARD_DIM, j / BOARD_DIM};
            Board *new_board = (Board*) malloc(sizeof(Board));
            copy_board(new_board, board);
            make_move(tmp1, tmp2, new_board);
            total += total_moves(new_board, ply - 1);
            free_board(new_board);
        }
    }

    return total;


}
