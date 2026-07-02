# Uniform tournament protocol (all 5 TwistedLogic engines must implement identically)

The engines already work and pass perft. We are adding ONE new stdin command so a
Python arbiter (python-chess) can drive engine-vs-engine games. Keep all existing
commands/behavior intact. Add this command to the engine's main command loop.

## The command

A single line read from stdin:

    play <D> <m1> <m2> ... <mk>

- `play` : literal token.
- `<D>`  : search depth, a positive integer (e.g. 4).
- `<m1>..<mk>` : k >= 0 moves in coordinate notation, describing the game from the
  STANDARD chess start position (White moves first), in order. Each move looks like
  `e2e4`, `g1f3`, `e1g1` (castling is encoded as the king's two-square move),
  `e7e8q` (promotion piece suffix lowercase: q/r/b/n). There may be zero moves
  (then just search the start position). Every move the arbiter sends is guaranteed
  legal.

## Engine behavior on `play`

1. Reset the internal board to the standard start position, White to move, full
   castling rights, no en-passant, ply/move counters zeroed. Do this CHEAPLY — the
   precomputed attack/hash/mask tables built once at startup do NOT need rebuilding;
   only the position/game state must be reset. (Do not re-run the expensive full
   init that memsets multi-MB hash tables every call if you can avoid it.)
2. Apply m1..mk in order using the engine's existing move-parsing + make-move code
   (the same matching your `usermove`/move-input path already does: generate moves,
   find the one whose coordinate string equals the input, make it). Toggle side each
   move. If a move somehow cannot be applied, print `bestmove error` and stop.
3. Run the engine's normal search to depth `<D>` (set any time limit effectively
   infinite so the fixed depth is what governs). Disable opening books and search
   info printing (`post`/verbose off) for this path if applicable.
4. Print EXACTLY one line, then flush stdout:

       bestmove <coord>

   where `<coord>` is the chosen move in the SAME coordinate format the engine
   accepts as input (standard orientation: a1 is bottom-left from White's side;
   files a-h, ranks 1-8; lowercase; promotion suffix lowercase). Examples:
   `bestmove e2e4`, `bestmove e7e8q`, `bestmove e1g1`.
   - If there is NO legal move (checkmate/stalemate): print `bestmove none`.
   - If the search returns nothing but legal moves exist: FALL BACK to the first
     legal move and output it. Never crash, never print a garbage/empty move.
5. Continue the loop (persistent process). `quit` must still exit.

## CRITICAL output rules (the arbiter parses this)

- The bestmove line must contain the token `bestmove` then one space then the
  coordinate, and nothing else garbled on that line. Other lines of output before it
  are tolerated (the arbiter scans each line for `bestmove`), but keep noise minimal.
- BUILD THE COORDINATE STRING CLEANLY. Do NOT use any `mov2str`/`displaymove` variant
  that emits a trailing space or a stray NUL byte (some of these files call
  `putchar('\0')` inside their move-to-string — do not use that here). Construct the
  coordinate directly from the move's from-square, to-square, and promotion piece.
- Lowercase only. For gamma/alpha/edsel1 the board is vertically flipped internally
  (index 0 = a8), so a square's printable rank is `8 - (sq>>3)` and file is
  `('a' + (sq & 7))` — matching how those engines already print moves. For
  omega9/Logic18 the board is natural (index 0 = a1), rank is `'1' + (sq>>3)`.
- Flush stdout after printing bestmove.

## Robustness

- Always return a legal move when one exists (fallback = first legal move).
- No crashes, no hangs, no infinite loops, no reads past end of the move list.
- A game can be long; make sure replaying ~200 moves from the start is fine
  (bump any per-game history array bound if needed).

## Quick self-test the arbiter will do

    printf 'play 4 e2e4 e7e5\nquit\n' | ./engine.exe
    -> must print a line 'bestmove <coord>' with a legal Black reply (e.g. bestmove b8c6)

    printf 'play 4\nquit\n' | ./engine.exe
    -> must print 'bestmove <coord>' with a legal White first move (e.g. bestmove e2e4)
