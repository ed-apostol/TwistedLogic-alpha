#!/usr/bin/env python3
"""
Round-robin arbiter for the TwistedLogic engines.

Each engine speaks a tiny protocol (see TOURNAMENT_PROTOCOL.md):
    play <depth> <m1> <m2> ...   ->  prints a line containing 'bestmove <coord>'
The moves are UCI coordinate moves from the start position. The engine resets,
replays them, searches to <depth>, and returns its chosen move.

python-chess is the independent rules authority: legality, checkmate, stalemate,
threefold/50-move/insufficient-material draws, and PGN output. An engine that
returns an illegal move, no move, or times out forfeits the game.

Every pair plays 2 games with colors swapped.

Usage:  python tools/arbiter.py            # build engines from src/ then play
        python tools/arbiter.py --no-build # skip compilation (use existing exes)

Requires: gcc on PATH (MinGW-w64 UCRT), and `pip install chess`.
"""
import subprocess, threading, queue, time, itertools, os, sys, datetime, shutil
import chess, chess.pgn

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)               # tools/..  -> repo root
SRC  = os.path.join(REPO, "src")
BUILD = os.path.join(HERE, "build")        # tools/build/*.exe
PGN_DIR = os.path.join(HERE, "pgn")
EXE = ".exe" if os.name == "nt" else ""

# name -> (source file, search depth). Depths chosen so per-move time stays
# modest; edsel1's heavy per-node eval gets a shallower depth so it isn't the
# tournament's clock bottleneck. Tune here.
ENGINES = {
    "Logic18": ("Logic18.c", 5),
    "omega9":  ("omega9.c",  5),
    "gamma":   ("gamma.c",   5),
    "alpha":   ("alpha.c",   5),
    "edsel1":  ("edsel1.c",  4),
}

MOVE_TIMEOUT = 60.0    # seconds per move before forfeit
MAX_PLIES    = 400     # hard cap -> adjudicate draw


def build_engines():
    gcc = shutil.which("gcc")
    if not gcc:
        sys.exit("error: gcc not found on PATH (need MinGW-w64 UCRT).")
    os.makedirs(BUILD, exist_ok=True)
    for name, (src, _) in ENGINES.items():
        srcpath = os.path.join(SRC, src)
        exepath = os.path.join(BUILD, name + EXE)
        print("compiling %-9s <- %s ... " % (name, src), end="", flush=True)
        r = subprocess.run([gcc, "-O2", "-o", exepath, srcpath],
                           capture_output=True, text=True)
        if r.returncode != 0:
            print("FAILED\n" + r.stderr)
            sys.exit(1)
        print("ok")
    print()


class Engine:
    def __init__(self, name, depth):
        self.name, self.depth = name, depth
        self.exe = os.path.join(BUILD, name + EXE)
        self.p = self.q = self.reader = None
        self.launch()

    def launch(self):
        self.close()
        self.p = subprocess.Popen(
            [self.exe], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL, cwd=BUILD, bufsize=1,
            universal_newlines=True, encoding="latin-1", errors="replace")
        self.q = queue.Queue()
        self.reader = threading.Thread(target=self._read, daemon=True)
        self.reader.start()

    def _read(self):
        try:
            for line in self.p.stdout:
                self.q.put(line)
        except Exception:
            pass

    def close(self):
        if self.p:
            try:
                self.p.stdin.write("quit\n"); self.p.stdin.flush()
            except Exception:
                pass
            try:
                self.p.terminate()
            except Exception:
                pass
            self.p = None

    def bestmove(self, uci_moves):
        """Send a position (as a move list) and return the engine's UCI move
        string, or None on none/error/timeout/crash."""
        try:
            while True: self.q.get_nowait()   # drain stale output
        except queue.Empty:
            pass
        cmd = "play %d %s\n" % (self.depth, " ".join(uci_moves))
        try:
            self.p.stdin.write(cmd); self.p.stdin.flush()
        except Exception:
            return None
        deadline = time.time() + MOVE_TIMEOUT
        while time.time() < deadline:
            try:
                line = self.q.get(timeout=deadline - time.time())
            except queue.Empty:
                break
            if "bestmove" in line:
                tok = line.split("bestmove", 1)[1].split()
                if not tok:
                    return None
                mv = tok[0].strip().lower()
                return None if mv in ("none", "error", "") else mv
        return None  # timeout


def _termination(board):
    if board.is_checkmate(): return "checkmate"
    if board.is_stalemate(): return "stalemate"
    if board.is_insufficient_material(): return "insufficient material"
    if board.is_seventyfive_moves() or board.can_claim_fifty_moves(): return "fifty-move rule"
    if board.is_fivefold_repetition() or board.can_claim_threefold_repetition(): return "repetition"
    return "draw"


def play_game(white, black, round_label):
    board = chess.Board()
    game = chess.pgn.Game()
    game.headers["Event"] = "TwistedLogic round-robin"
    game.headers["White"] = white.name
    game.headers["Black"] = black.name
    game.headers["Round"] = round_label
    node = game
    reason = ""
    result = None

    while True:
        if board.is_game_over(claim_draw=True):
            result = board.result(claim_draw=True); reason = _termination(board); break
        if board.ply() >= MAX_PLIES:
            result = "1/2-1/2"; reason = "adjudicated draw (ply cap)"; break
        engine = white if board.turn == chess.WHITE else black
        mv = engine.bestmove([m.uci() for m in board.move_stack])
        loser_is_white = (board.turn == chess.WHITE)
        if mv is None:
            result = "0-1" if loser_is_white else "1-0"
            reason = "%s failed to return a move (none/timeout/crash)" % engine.name
            engine.launch()  # revive for later games
            break
        try:
            move = chess.Move.from_uci(mv)
        except Exception:
            move = None
        if move is None or move not in board.legal_moves:
            result = "0-1" if loser_is_white else "1-0"
            reason = "%s played illegal move '%s'" % (engine.name, mv)
            break
        board.push(move)
        node = node.add_variation(move)

    game.headers["Result"] = result
    game.headers["Termination"] = reason
    return result, reason, game, board.ply()


def main():
    if "--no-build" not in sys.argv:
        build_engines()
    os.makedirs(PGN_DIR, exist_ok=True)
    names = list(ENGINES.keys())
    engines = {n: Engine(n, ENGINES[n][1]) for n in names}

    points = {n: 0.0 for n in names}
    wdl = {n: [0, 0, 0] for n in names}
    head = {n: {m: 0.0 for m in names} for n in names}
    pgn_all = open(os.path.join(PGN_DIR, "all_games.pgn"), "w", encoding="utf-8")

    pairs = list(itertools.combinations(names, 2))
    total = len(pairs) * 2
    gi = 0
    t0 = time.time()
    for a, b in pairs:
        for white, black in ((a, b), (b, a)):
            gi += 1
            gt = time.time()
            result, reason, game, plies = play_game(engines[white], engines[black], "%d" % gi)
            if result == "1-0":
                points[white] += 1; wdl[white][0] += 1; wdl[black][2] += 1; head[white][black] += 1
            elif result == "0-1":
                points[black] += 1; wdl[black][0] += 1; wdl[white][2] += 1; head[black][white] += 1
            else:
                points[white] += 0.5; points[black] += 0.5
                wdl[white][1] += 1; wdl[black][1] += 1
                head[white][black] += 0.5; head[black][white] += 0.5
            print("[%2d/%2d] %-8s (W) vs %-8s (B): %-7s  %-42s  %3d plies  %5.1fs"
                  % (gi, total, white, black, result, reason, plies, time.time() - gt), flush=True)
            print(game, file=pgn_all, end="\n\n", flush=True)

    pgn_all.close()
    for e in engines.values():
        e.close()

    order = sorted(names, key=lambda n: (-points[n], n))
    print("\n" + "=" * 72)
    print("FINAL CROSSTABLE  (%d games, %.0fs total)" % (total, time.time() - t0))
    print("=" * 72)
    colw = 9
    header = "%-9s" % "Engine" + "".join("%*s" % (colw, n[:8]) for n in order) + \
             "%7s%4s%4s%4s%4s" % ("Pts", "W", "D", "L", "Gm")
    print(header)
    print("-" * len(header))
    for n in order:
        row = "%-9s" % n
        for m in order:
            row += "%*s" % (colw, ("--" if m == n else "%.1f" % head[n][m]))
        row += "%7.1f%4d%4d%4d%4d" % (points[n], wdl[n][0], wdl[n][1], wdl[n][2], sum(wdl[n]))
        print(row)
    print("-" * len(header))
    print("Depths: " + ", ".join("%s=%d" % (n, ENGINES[n][1]) for n in names))
    print("PGNs: " + os.path.join(PGN_DIR, "all_games.pgn"))


if __name__ == "__main__":
    main()
