import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

# Daten laden
df = pd.read_csv('/home/leoney/CLionProjects/AlphaZero_BlokusDuo/open_spiel/open_spiel/move_times.csv')

# Plot erstellen
plt.figure(figsize=(10, 6))
sns.boxplot(x='player', y='move_time', data=df, showfliers=True, )

# Optik verbessern
plt.title('Rechenzeit pro Zug pro Agent')
plt.xlabel('Spieler / Agent ID')
plt.ylabel('Zeit in Sekunden')
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Speichern oder Anzeigen
plt.savefig('move_time_boxplot2.png')
plt.show()