import random
import pyspiel
import torch
import numpy as np
from open_spiel.python.algorithms.alpha_zero.run_alpha_zero_torch import AlphaZeroResNet  # dein Netz


def print_blokus_board(state):
    """
    Zeigt das 16x16 Blokus Duo Board als ASCII.
    Ignoriert die Bits für verbleibende Steine.
    """
    obs = np.array(state.observation_tensor())
    board_size = 16
    board = np.zeros((board_size, board_size), dtype=int)

    # Spieler 1 Layer
    layer1 = obs[:board_size*board_size].reshape((board_size, board_size))
    board[layer1 > 0] = 1

    # Spieler 2 Layer, falls vorhanden
    if len(obs) >= board_size*board_size + 21 + board_size*board_size:
        start2 = board_size*board_size + 21  # überspringe 21 Bits von Spieler 1
        layer2 = obs[start2:start2 + board_size*board_size].reshape((board_size, board_size))
        board[layer2 > 0] = 2

    # ASCII-Druck
    for row in board:
        line = ""
        for cell in row:
            if cell == 0:
                line += ". "
            elif cell == 1:
                line += "X "
            elif cell == 2:
                line += "O "
        print(line)
    print("-" * (board_size*2))

# --- SETTINGS ---
GAME_NAME = "blokus_duo"
DEVICE = "cpu"
MODEL_PATH = "alphazero_blokusduo_iter25.pth" # Beispielpfad
# -----------------

## 🤖 Testen der Spielstärke: Modell gegen Zufall

try:
    # --- Laden des Spiels ---
    game = pyspiel.load_game(GAME_NAME)
    state = game.new_initial_state()

    obs_size = len(state.observation_tensor())
    num_actions = game.num_distinct_actions()

    # --- Netz laden ---
    net = AlphaZeroResNet(obs_size, num_actions).to(DEVICE)
    net.load_state_dict(torch.load(MODEL_PATH, map_location=DEVICE))
    net.eval()
    print(f"Modell '{MODEL_PATH}' geladen und bereit.")

    # --- Spiel-Loop mit abwechselnden Spielern ---

    while not state.is_terminal():
        current_player = state.current_player()
        legal_actions = state.legal_actions()

        if current_player == 0:
            # Spieler 0: Das trainierte AlphaZero-Modell

            obs = np.array(state.observation_tensor(), dtype=np.float32)
            obs_t = torch.tensor(obs, dtype=torch.float32).unsqueeze(0).to(DEVICE)

            with torch.no_grad():
                logits, value = net(obs_t)
                probs = torch.softmax(logits, dim=-1).cpu().numpy().flatten()

            # Wähle die beste legale Aktion basierend auf der Policy
            legal_probs = probs[legal_actions]

            # Fehlerbehandlung, falls die Summe 0 ist (sollte bei korrekter Policy nicht passieren)
            if legal_probs.sum() > 0:
                legal_probs /= legal_probs.sum()
            else:
                print("ACHTUNG: Policy liefert 0 für alle legalen Aktionen. Wähle zufällig.")
                action = random.choice(legal_actions)

            action = legal_actions[np.argmax(legal_probs)]
            player_info = "AlphaZero Modell"

        elif current_player == 1:
            # Spieler 1: Zufallsstrategie

            action = random.choice(legal_actions)
            player_info = "Zufallsstrategie"

        else:
            raise ValueError(f"Ungültiger Spieler-Index: {current_player}")


        # Führe die gewählte Aktion aus
        state.apply_action(action)

        print(f"Zug {state.history()[-1].move_number}:")
        print(f"  Spieler {current_player} ({player_info}) zieht.")
        print(f"  Gewählte Aktion: {action}")
        print(f"  Board observation:\n{print_blokus_board(state)}")
        print("---")

    # Am Ende
    print("====================================")
    print("Game over!")
    print(f"Finaler Zustand: {state.history()[-1]}")
    print(f"Endergebnisse (Player 0, Player 1): {state.returns()}")
    print("====================================")

except FileNotFoundError:
    print(f"\nFEHLER: Modelldatei nicht gefunden unter {MODEL_PATH}.")
    print("Stelle sicher, dass du den Pfad zum trainierten .pth-Modell korrekt angegeben hast.")
except Exception as e:
    print(f"\nEin unerwarteter Fehler ist aufgetreten: {e}")