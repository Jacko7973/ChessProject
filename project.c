/* 
 * Jack O'Connor
 * Fund Comp Lab 11
 * project.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "gfx.h"

#include "chessfunc.h"

int main(int argc, char *argv[])
{
    const int WIN_SZ = 500;
    const int MARGIN = 50;
    const int SQ_SZ = 50;

    gfx_open(WIN_SZ, WIN_SZ, "Chess");
    gfx_clear_color(150, 150, 150);

    Board *board = (Board*) malloc(sizeof(Board));
    // Set up the board with the stating FEN string.
    create_board(board, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    Pos pos, selected_pos;
    Piece *selected;
    int num_moves;
    Bitboard moves = 0;

    // printf("%d\n", total_moves(board, 4));

    char c;
    while (1) {
        gfx_clear();
        draw_board(MARGIN, MARGIN, SQ_SZ, board);

        (board->turn == WHITE) ? gfx_color(255, 255, 255) : gfx_color(0, 0, 0);
        char msg[25];
        sprintf(msg, "%s's turn", (board->turn == WHITE) ? "White" : "Black");
        gfx_text(WIN_SZ / 2 - 2 * strlen(msg), MARGIN - 5, msg);
        gfx_color(0, 0, 0);
        gfx_text(MARGIN, WIN_SZ - 10, "(q) Quit    (r) Retire");

        c = gfx_wait();
        // Reset the selected squares and availible squares.
        reset_highlights(SELECTED, board);
        reset_highlights(AVAILIBLE, board);
        if (c == 'q') // Quit the program if the user presses 'q'.
            break;
        else if (c == 'r') { // Current player is retireing.
            board->winner = (board->turn == WHITE) ? BLACK : WHITE;

        } else if (c == 1 && !board->winner) {
            // If the user clicked, store it's grid position relative to the board
            // in the `pos` variable.
            int x = (gfx_xpos() - MARGIN) / SQ_SZ;
            int y = (gfx_ypos() - MARGIN) / SQ_SZ;
            pos = x + BOARD_DIM * y;
            V2Int tmp = {x, y};
            // Make sure the position is on the board.
            if (0 <= x && x < BOARD_DIM && 0 <= y && y < BOARD_DIM) {
                // selected = get_piece(pos, board);
                selected = board->arr + pos;
                if (*selected != 0 && (*selected & COLOR_BITMASK) == board->turn) {
                    // If the user selected one of their own pieces, generate a list of
                    // valid moves using the `get_valid_moves` function.
                    selected_pos = pos;
                    moves = (Bitboard) 0;
                    
                    num_moves = get_valid_moves(tmp, &moves, board, 1);
                    // Highlight the selected position.
                    set_highlight(pos, SELECTED, board);
                    *(board->highlights + AVAILIBLE) = moves;
                } else {
                    // If the user selected a square that does not contain one of 
                    // their own pieces.
                    if (query_bitboard(&moves, pos)) {
                        V2Int tmp2 = {selected_pos % BOARD_DIM, selected_pos / BOARD_DIM};
                        make_move(tmp2, tmp, board);
                        reset_highlights(PREVIOUS, board);
                        reset_highlights(SELECTED, board);

                        set_highlight(selected_pos, PREVIOUS, board);
                        set_highlight(pos, PREVIOUS, board);
                        selected = NULL;


                        // Test to see if the game is over.
                        Bitboard attacked = attacked_positions(board->turn, board, true);
                        if (attacked == (Bitboard) 0) {
                            // Current player in has no valid moves.
                            if (in_check(board) & board->turn) // Checkmate.
                                board->winner = (board->turn == WHITE) ? BLACK : WHITE;
                            else // Stalemate.
                                board->winner = COLOR_BITMASK;
                        }

                        // 50 move rule stalemate.
                        if (board->half_move_clock == 50)
                            board->winner = COLOR_BITMASK;

                    }

                    num_moves = 0;
                    moves = (Bitboard) 0;
                }
            }
        }
    }

    // Free up dynamic memory.
    free_board(board);

    return 0;
}

