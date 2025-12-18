# AlphaZero für Blokus Duo

Dieses Repository enthält die Implementierung von AlphaZero für das Strategiespiel Blokus Duo im Rahmen meiner Bachelorarbeit.

## Über das Projekt

Diese Arbeit untersucht die Anwendung des AlphaZero-Algorithmus auf Blokus Duo, ein abstraktes Strategiespiel für zwei Spieler. Die Implementierung basiert auf dem [OpenSpiel-Framework](https://github.com/google-deepmind/open_spiel) von DeepMind.

**Bachelorarbeit**: *AlphaZero für Blokus-Duo: Analyse und Vergleich mit Minimax- und MCTS-Algorithmen*  
**Autor**: Leon Steinbach  


## Projektstruktur

```
open_spiel/
├── games/
│   └── blokus_duo/          # Blokus Duo Spielimplementierung
│       ├── blokus_duo.h
│       ├── blokus_duo.cc
│       └── blokus_duo_test.cc
├── algorithms/
│   ├── alpha_zero_torch/
│        ├── alpha_zero.cc    # AlphaZero Implementierung
│        └── alpha_zero.h     # AlphaZero Implementierung
│   ├── minimax.h            # Erweitert um iterative Vertiefung
│   └── minimax.cc           # Minimax-Algorithmus mit Iterative Deepening
└── config.json # AlphaZero Trainingsconfiguration
```

## Features

- **Blokus Duo Spielimplementierung**: Vollständige Implementierung der Spielregeln in C++ als OpenSpiel-Game
- **Minimax mit iterativer Vertiefung**: Erweiterung des OpenSpiel Minimax-Algorithmus um Iterative Deepening für effizientere Suche
- **AlphaZero-Integration**: Training von neuronalen Netzen mit dem AlphaZero-Algorithmus
- **Baseline-Vergleiche**: Evaluation gegen Minimax-Agenten unterschiedlicher Suchtiefe
- **Flexible Evaluationstools**: Skripte zum Vergleich verschiedener Spielstrategien

## Voraussetzungen

- Python 3.10 
- CMake 3.15 oder höher
- C++20-kompatibler Compiler 

## Installation


### 1. Abhängigkeiten installieren

```bash
./install.sh
# Python-Abhängigkeiten
pip install -r requirements.txt

# OpenSpiel bauen
mkdir build
cd build
cmake -DPython3_EXECUTABLE=$(which python3) -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
cd ..
```

### 2. Python-Pfad einrichten

```bash
export PYTHONPATH=$PYTHONPATH:$(pwd)
export PYTHONPATH=$PYTHONPATH:$(pwd)/build/python
```

## Verwendung

### Training starten

```bash
./build/examples/alpha_zero_torch_example config.json
```


### Evaluation durchführen

```bash
# AlphaZero vs. Minimax
./build/examples/alpha_zero_torch_game_example \
  --game=blokus_duo \
  --player1=az \
  --player2=minimax \
  --az_path1= [Pfad zu AlphaZero Modell] \
  --az_path2= [Pfad zu AlphaZero Modell] \
  --az_checkpoint1=-1 --az_checkpoint2=-1 \
  --max_simulations=200 \
  --rollout_count=16 \
  --az_threads=8 \
  --az_cache_size=65536 \
  --num_games=1
```


## Blokus Duo Spielregeln

Blokus Duo ist ein abstraktes Strategiespiel für zwei Spieler auf einem 14×14 Spielbrett. Jeder Spieler verfügt über 21 Polyomino-Steine unterschiedlicher Formen. Die Spieler platzieren abwechselnd ihre Steine auf dem Brett, wobei folgende Regeln gelten:

- Der erste Stein jedes Spielers muss eine Eckposition des Bretts berühren
- Nachfolgende Steine müssen eckenweise an eigene Steine angrenzen
- Steine dürfen sich nicht kantenweise mit eigenen Steinen berühren
- Das Spiel endet, wenn kein Spieler mehr Steine platzieren kann
- Gewinner ist, wer die wenigsten ungenutzten Quadrate übrig hat

### Unterschiede zum ursprünglichen OpenSpiel

Dieses Repository basiert auf OpenSpiel (Version/Commit: [XXX]) mit folgenden Ergänzungen:

1. **Blokus Duo Spielimplementierung** (`open_spiel/games/blokus_duo/`)
   - Vollständige Spiellogik inklusive aller 21 Polyomino-Steine
   - Effiziente Repräsentation des Spielzustands
   - Implementierung aller legalen Zugprüfungen
   - Integration in das OpenSpiel-Game-Interface

2. **Minimax mit iterativer Vertiefung** (`open_spiel/algorithms/minimax.*`)
   - Erweiterung des bestehenden Minimax-Algorithmus
   - Implementierung von Iterative Deepening für zeitgesteuerte Suche
   - Verbesserte Effizienz durch schrittweise Tiefensuche
   - Kompatibilität mit bestehenden OpenSpiel-Evaluatoren



### Minimax mit iterativer Vertiefung

Die iterative Vertiefung (Iterative Deepening) führt mehrere Minimax-Suchen mit steigender Tiefe durch:

- **Vorteil**: Bessere Anytime-Performance bei begrenzter Rechenzeit
- **Prinzip**: Suche mit Tiefe 1, 2, 3, ... bis zur maximalen Tiefe oder Zeitlimit
- **Overhead**: Minimal durch exponentielles Wachstum des Suchbaums



## Danksagungen

- OpenSpiel-Team von DeepMind für das ausgezeichnete Framework

## Kontakt

Leon Steinbach - leon.steinbach.com

---

*Diese Implementierung wurde im Rahmen einer Bachelorarbeit erstellt und dient primär akademischen Zwecken.*