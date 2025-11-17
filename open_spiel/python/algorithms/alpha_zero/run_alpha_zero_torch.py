"""
run_alpha_zero_selfplay.py
Minimaler, aber funktionsfähiger AlphaZero-Style Self-Play Trainer (PyTorch).

Was es macht:
- Lädt Spiel (blokus_duo) über pyspiel
- Führt Self-Play-Spiele mit einer MCTS-Politik aus (MCTS nutzt das aktuelle Netz als Policy/value-evaluator)
- Sammelt (obs, target_policy_from_MCTS, value) Beispiele in Replay Buffer
- Trainiert das Netz auf gesammelten Beispielen
- Wiederholt: Self-Play -> Train

Benötigt:
- open_spiel (pyspiel importierbar)
- open_spiel.python.algorithms.mcts (Standard OpenSpiel MCTS wrapper)
- PyTorch
"""

import random
import collections
import time
from typing import List, Tuple, Dict, Any

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
import pyspiel
from open_spiel.python.algorithms.mcts import MCTSBot

# Versuche den üblichen OpenSpiel-MCTS-Wrapper zu importieren
try:
    from open_spiel.python.algorithms import mcts as os_mcts
except Exception as e:
    raise ImportError(
        "Konnte open_spiel.python.algorithms.mcts nicht importieren. "
        "Stelle sicher, dass deine OpenSpiel-Version die MCTS-Python-API enthält.\n"
        f"Ursprünglicher Fehler: {e}"
    )

# -------------------------
# Konfiguration (anpassen)
# -------------------------
GAME_NAME = "blokus_duo"
NUM_ITERATIONS = 25           # Self-play <-> Train Zyklen
SELF_PLAY_GAMES_PER_ITER = 12
# Anzahl Self-Play-Spiele pro Iteration
MCTS_SIMULATIONS = 10        # Anzahl MCTS-Simulationen pro Zug (kann klein beginnen)
REPLAY_BUFFER_SIZE = 10000
BATCH_SIZE = 64
TRAIN_EPOCHS_PER_ITER = 2
LEARNING_RATE = 1e-3
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")
print(DEVICE)
PRINT_EVERY = 1
# -------------------------


class ResidualBlock(nn.Module):
    def __init__(self, channels):
        super().__init__()
        # Faltungsschicht 1
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, padding=1)
        self.bn1 = nn.BatchNorm2d(channels)
        # Faltungsschicht 2
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, padding=1)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x):
        # Speichere die Eingabe (Skip-Connection)
        identity = x

        # Block 1
        out = self.conv1(x)
        out = self.bn1(out)
        out = nn.ReLU(inplace=True)(out)

        # Block 2
        out = self.conv2(out)
        out = self.bn2(out)

        # Addiere die Eingabe zur Ausgabe und aktiviere (Residual Connection)
        out += identity
        out = nn.ReLU(inplace=True)(out)
        return out
# --- Einfaches Netz (Policy + Value heads) ---
class AlphaZeroResNet(nn.Module):
    def __init__(self, input_channels, num_actions, num_blocks=4, channels=64):
        super().__init__()

        H, W = 16, 16 # Oder 14, 14, je nach Board-Größe
        self.input_channels = input_channels

        # --- 1. Initialer Faltungsblock ---
        self.initial_block = nn.Sequential(
            # Start mit 3x3 Faltung und einer großen Anzahl an Kanälen
            nn.Conv2d(input_channels, channels, kernel_size=3, padding=1),
            nn.BatchNorm2d(channels),
            nn.ReLU(inplace=True)
        )

        # --- 2. Residual Blocks (Backbone) ---
        self.residual_blocks = nn.Sequential(
            *[ResidualBlock(channels) for _ in range(num_blocks)]
        )

        # --- 3. Policy Head ---
        # Verkleinere die Kanäle vor dem Policy Head
        self.policy_conv = nn.Sequential(
            nn.Conv2d(channels, 2, kernel_size=1), # Reduziert auf 2 Kanäle
            nn.BatchNorm2d(2),
            nn.ReLU(inplace=True)
        )
        # Flache die Ausgabe ab für die Aktionen (z.B. 2 * 16 * 16 + 1 (Pass))
        self.policy_fc = nn.Linear((2 * H * W) + 42, num_actions)

        # --- 4. Value Head ---
        self.value_conv = nn.Sequential(
            nn.Conv2d(channels, 1, kernel_size=1),
            nn.BatchNorm2d(1),
            nn.ReLU(inplace=True)
        )

        # NEU: Value FC Eingabe: (1 * 16 * 16) + 42 globale Teile, geht in den ersten Linear Layer
        self.value_fc = nn.Sequential(
            nn.Linear((H * W) + 42, channels), # 1*256 + 42 = 298
            nn.ReLU(inplace=True),
            nn.Linear(channels, 1)
        )


    def forward(self, x_flat):
        B = x_flat.size(0)

        # 1. Lokale Board- und Ecken-Kanäle (4 Kanäle)
        # Position 0 bis 1023 (4 * 256)
        board_data_3D = x_flat[:, :4 * 256].view(B, 4, 16, 16) # CNN Input (4 Kanäle)

        # 2. Globale Teile-Infos (42 Werte)
        # Position 1024 bis 1065 (42 Werte)
        piece_data_global = x_flat[:, 4 * 256:] # MLP Input (42 Werte)

        # --- Hauptnetz (CNN nur mit lokalen Daten) ---
        h = self.initial_block(board_data_3D) # CNN Input ist nur 4 Kanäle!
        h = self.residual_blocks(h) # h Shape: (B, 128, 16, 16)

        # --- Policy Head ---
        p = self.policy_conv(h)
        # Flache die Policy-Ausgabe ab: (B, 2*16*16)
        p_flat = p.view(B, -1)

        # Konkateniere die flache Policy-Ausgabe mit den globalen Teilen-Infos
        policy_input = torch.cat([p_flat, piece_data_global], dim=1)

        logits = self.policy_fc(policy_input) # Policy FC muss jetzt 2*256 + 42 Eingänge haben

        # --- Value Head ---
        v = self.value_conv(h)
        # Flache die Value-Ausgabe ab: (B, 1*16*16)
        v_flat = v.view(B, -1)

        # Konkateniere die flache Value-Ausgabe mit den globalen Teilen-Infos
        value_input = torch.cat([v_flat, piece_data_global], dim=1)

        value = self.value_fc(value_input) # Value FC muss jetzt 1*256 + 42 Eingänge haben

        # Verwenden von tanh für Value-Skalierung [-1, 1]
        return logits, torch.tanh(value).squeeze(-1)

# --- Replay buffer ---
Example = collections.namedtuple("Example", ["obs", "policy", "value"])

class ReplayBuffer:
    def __init__(self, capacity):
        self.buffer = collections.deque(maxlen=capacity)

    def add(self, examples: List[Example]):
        self.buffer.extend(examples)

    def sample(self, batch_size):
        batch = random.sample(self.buffer, batch_size)
        obs = torch.tensor(np.array([b.obs for b in batch]), dtype=torch.float32, device=DEVICE)
        policy = torch.tensor([b.policy for b in batch], dtype=torch.float32, device=DEVICE)
        value = torch.tensor([b.value for b in batch], dtype=torch.float32, device=DEVICE)
        return obs, policy, value

    def __len__(self):
        return len(self.buffer)

class AlphaZeroEvaluator:
    """
    Evaluator-Wrapper für PyTorch ResNet, kompatibel mit dem minimalen MCTS-Interface.
    Gibt die Evaluation für alle Spieler zurück: [Player 0 Value, Player 1 Value].
    """
    def __init__(self, net: torch.nn.Module, num_actions: int, device: str):
        self.net = net
        self.num_actions = num_actions
        self.device = device
        self.net.eval()
        # Bei BlokusDuo sind es 2 Spieler.
        self.num_players = 2

    def evaluate(self, state) -> tuple[list[Any], list[Any]]:
        """
        Gibt die Policy und die Value-Evaluation aus Sicht aller Spieler zurück.

        Args:
            state: Der aktuelle OpenSpiel-Zustand.

        Returns:
            ([Player 0 Policy, Player 1 Policy], [Player 0 Value, Player 1 Value])
        """
        # 1. Zustand in Tensor umwandeln (muss mit der forward-Methode des ResNets übereinstimmen)
        obs = np.array(state.observation_tensor(), dtype=np.float32)
        obs_t = torch.tensor(obs, dtype=torch.float32).unsqueeze(0).to(self.device)

        with torch.no_grad():
            # 2. Netzinferenz
            logits, value_t = self.net(obs_t)

            # Policy-Vektor berechnen
            priors = torch.softmax(logits, dim=-1).cpu().numpy().flatten().tolist()

            # Value-Skalar abrufen (Ausgabe ist aus Sicht des Current Player, V_cur)
            v_cur = value_t.item()

            # 3. Policy-Vektor: AlphaZero hat nur einen Policy-Kopf (P) für alle Spieler.
        # Wir geben diesen Vektor für beide Spieler zurück, da die Policy Aktionen
        # in einer Policy-Iteration sind, nicht spielerspezifisch aufgeteilt.
        policy_list = [priors, priors]

        # 4. Value-Rückgabe (Returns für alle Spieler)
        # Die Netzausgabe V_cur ist aus Sicht des aktuellen Spielers.
        current_player = state.current_player()

        # BlokusDuo: Quasi-Nullsummenspiel. P0 gewinnt => V0 hoch, V1 niedrig.
        # Wir geben die Utility für beide Spieler zurück.
        if current_player == 0:
            v_player_0 = v_cur
            v_player_1 = -v_cur
        else: # current_player == 1
            v_player_1 = v_cur
            v_player_0 = -v_cur

        value_list = [v_player_0, v_player_1]

        # WICHTIG: Konvertierung in NumPy-Array, um Typfehler in der C++-Bindung zu verhindern.
        value_np = np.array(value_list, dtype=np.float32)

        return policy_list, value_list


ROOT_EPSILON = 0.25
ROOT_DIRICHLET_ALPHA = 0.3
dirichlet_noise_config = (ROOT_EPSILON, ROOT_DIRICHLET_ALPHA)
# Fügen Sie dies nach der Definition des AlphaZeroEvaluator und vor train_selfplay hinzu:
def build_mcts_searcher(game, net, num_simulations, c_puct=1.4):
    """
    Erstellt und gibt einen OpenSpiel MCTSBot zurück.
    """
    # Erstelle den Evaluator, der dein PyTorch-Netz wrappt.
    #base_evaluator = MCTSBot(net, game.num_distinct_actions(), DEVICE)
    evaluator = AlphaZeroEvaluator(net, game.num_distinct_actions(), DEVICE)

    # Erstelle den offiziellen OpenSpiel MCTSBot.
    mcts_bot = os_mcts.MCTSBot(
        game=game,
        uct_c=c_puct,
        max_simulations=num_simulations,
        evaluator=evaluator,
        dirichlet_noise=dirichlet_noise_config,
    )
    return mcts_bot

# --- Self-play für ein Spiel ---
def self_play_single_game(game, net, num_simulations, c_puct=1.4) -> List[Example]:
    """
    Spielt ein Spiel mit dem OpenSpiel MCTSBot (der das PyTorch-Netz nutzt).
    Gibt die gesammelten Beispiele zurück.
    """
    state = game.new_initial_state()
    examples = []

    # MCTSBot für dieses Spiel erstellen
    # Da MCTSBot den Zustand (state) des Baumes speichert, muss er hier initialisiert werden.
    evaluator = AlphaZeroEvaluator(net, game.num_distinct_actions(), DEVICE)
    mcts_bot = os_mcts.MCTSBot(
        game=game,
        uct_c=c_puct,
        max_simulations=num_simulations,
        evaluator=evaluator,
        # Optional: Noise für bessere Exploration am Anfang des Spiels
        dirichlet_noise=dirichlet_noise_config,
    )

    while not state.is_terminal():
        current_player = state.current_player()

        # 1. MCTS-Suche durchführen
        # Die Suche füllt den MCTS-Baum mit den Besuchszahlen.
        mcts_bot.mcts_search(state)

        # 2. Target Policy abrufen (Besuchszahlen normalisiert)
        # Die MCTSBot-Policy ist der Vektor der normalisierten Besuchszahlen (Target Policy).
        # Liefert ein Numpy-Array der Größe num_actions.
        policy_vec = mcts_bot.get_policy_vector(state)

        # 3. Aktion auswählen (stochastisch zur Exploration)
        if policy_vec.sum() <= 0:
            action = random.choice(state.legal_actions())
        else:
            # Policy-Vektor ist bereits über alle Aktionen (auch illegale)
            # Wir wählen stochastisch basierend auf den Besuchszahlen
            action = int(np.random.choice(len(policy_vec), p=policy_vec))


        # 4. Beispiel speichern
        obs = np.array(state.observation_tensor(), dtype=np.float32)

        # Speichere obs, Target Policy, und den Spieler, der gezogen hat
        examples.append(Example(obs=obs, policy=policy_vec, value=(current_player, None)))

        # 5. Aktion ausführen
        state.apply_action(action)

    # Spielende -> Rückgabe (Value) berechnen und Examples finalisieren (wie in Ihrer Funktion)
    final_returns = state.returns()
    final_examples = []

    for ex in examples:
        player_at_state = ex.value[0]
        value = final_returns[player_at_state]

        # Bei BlokusDuo sind die Returns die Minuspunkte.
        # Da AlphaZero auf [-1, 1] trainiert, ist es oft besser,
        # den relativen Wert (z.B. (Punktegewinn des Spielers)) zu verwenden.
        # Hier verwenden wir der Einfachheit halber den Score.
        # Wenn Blokus-Returns Minuspunkte sind, möchte das Netz den niedrigsten Score erreichen.

        final_examples.append(Example(obs=ex.obs, policy=ex.policy, value=float(value)))

    return final_examples

# --- TRAINING LOOP (hauptfunktion) ---

def train_selfplay():
    # Lade Spiel + Initial-Shape
    game = pyspiel.load_game(GAME_NAME)
    print("Loaded game:", game)
    sample_state = game.new_initial_state()
    obs_size = len(sample_state.observation_tensor())
    num_actions = game.num_distinct_actions()
    print(f"obs_size={obs_size}, num_actions={num_actions}, device={DEVICE}")


    local_channels = 4

    # Netz, Optimizer, Buffer
    net = AlphaZeroResNet(input_channels=local_channels, num_actions=num_actions).to(DEVICE)
    optimizer = optim.Adam(net.parameters(), lr=LEARNING_RATE)
    replay = ReplayBuffer(REPLAY_BUFFER_SIZE)


    # MAIN LOOP
    for it in range(1, NUM_ITERATIONS + 1):
        t0 = time.time()
        # Self-play phase: spiele GAMES und fülle Replay
        collected = 0
        for g in range(SELF_PLAY_GAMES_PER_ITER):
            # Wir benutzen einen einfacheren self_play_impl, das auch player-speicherung macht
            examples = self_play_single_game(
                game=game,
                net=net,
                num_simulations=MCTS_SIMULATIONS,
                c_puct=1.4  # Sie können c_puct hier als Konstante definieren, falls nötig
            )

            # examples: list of (obs, policy, value)
            replay.add(examples)
            collected += len(examples)

            # --- TRAIN PHASE ---
        if len(replay) >= BATCH_SIZE:
            total_loss = 0.0
            total_value_loss = 0.0
            total_policy_loss = 0.0
            total_batches = 0

            for epoch in range(TRAIN_EPOCHS_PER_ITER):
                num_batches = max(1, len(replay) // BATCH_SIZE)
                for b in range(num_batches):
                    obs_b, policy_b, value_b = replay.sample(BATCH_SIZE)
                    logits, v_pred = net(obs_b)

                    # Policy loss: cross-entropy between target policy (probabilities) and logits
                    log_probs = torch.log_softmax(logits, dim=1)
                    policy_loss = -(policy_b * log_probs).sum(dim=1).mean()

                    # Value loss: mean squared error
                    value_loss = nn.functional.mse_loss(v_pred, value_b)

                    # Gesamtverlust
                    loss = policy_loss + value_loss

                    optimizer.zero_grad()
                    loss.backward()
                    optimizer.step()

                    total_loss += loss.item()
                    total_value_loss += value_loss.item()
                    total_policy_loss += policy_loss.item()
                    total_batches += 1

            # Mittelwerte berechnen
            avg_loss = total_loss / total_batches
            avg_value_loss = total_value_loss / total_batches
            avg_policy_loss = total_policy_loss / total_batches

            iter_model_path = f"alphazero_blokusduo_iter{it}.pth"
            torch.save(net.state_dict(), iter_model_path)
            print(f"Model saved after iteration {it} to {iter_model_path}")

            print(f"Iter {it}/{NUM_ITERATIONS}: "
                  f"collected={collected}, buffer={len(replay)}, "
                  f"avg_loss={avg_loss:.4f}, "
                  f"value_loss={avg_value_loss:.4f}, "
                  f"policy_loss={avg_policy_loss:.4f}, "
                  f"time={time.time()-t0:.1f}s")
        else:
            print(f"Iter {it}/{NUM_ITERATIONS}: collected={collected}, buffer={len(replay)}, waiting for enough samples")





# -------------------------
# Entrypoint
# -------------------------
if __name__ == "__main__":
    # Basic sanity check that game loads
    try:
        pyspiel.load_game(GAME_NAME)
    except Exception as e:
        print("Fehler beim Laden des Spiels:", e)
        raise

    train_selfplay()
