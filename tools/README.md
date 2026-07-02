# tools/ — engine-vs-engine round-robin

A small Python arbiter that plays the five engines in `../src` against each other.

## Requirements
- `gcc` on PATH (MinGW-w64 UCRT is what these were built/tested with)
- `pip install chess`  (python-chess — the independent rules/adjudication authority)

## Run
```
python tools/arbiter.py            # compiles src/*.c into tools/build/, then plays
python tools/arbiter.py --no-build # reuse existing tools/build/*.exe
```
It plays a full round-robin (every pair twice, colors swapped), prints per-game
results and a final crosstable, and writes all games to `tools/pgn/all_games.pgn`.

## How it works
Each engine implements one extra stdin command (see `TOURNAMENT_PROTOCOL.md`):
```
play <depth> <m1> <m2> ...   ->  prints "bestmove <coord>"
```
The arbiter keeps one persistent process per engine, and each turn sends the whole
game so far as a UCI move list; the engine resets to the start position, replays
the moves, searches to the fixed depth, and returns its move. python-chess validates
legality and detects checkmate / stalemate / repetition / 50-move / insufficient
material. An engine that returns an illegal move, no move, or exceeds the per-move
timeout forfeits that game.

Search depths and the per-move timeout are configured at the top of `arbiter.py`
(`ENGINES`, `MOVE_TIMEOUT`). edsel1 uses a shallower depth because its heavy
per-node evaluation is much slower than the others.

## Last result
```
1. omega9   8.0/8   (swept the field; only engine with a mature search)
2. Logic18  4.5/8
3. gamma    4.0/8
4. alpha    2.5/8
5. edsel1   1.0/8
```
